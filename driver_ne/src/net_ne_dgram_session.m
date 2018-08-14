#include "cpe/utils/ringbuffer.h"
#include "net_address.h"
#include "net_dgram.h"
#include "net_ne_dgram_session.h"
#include "net_ne_utils.h"

static const char * net_ne_dgram_state_str(NWUDPSessionState state);
static void net_ne_dgram_session_receive_cb(net_ne_dgram_session_t session, NSArray<NSData *> *datagrams, NSError *error);
static int net_ne_dgram_session_check_write(net_ne_dgram_session_t session);
    
net_ne_dgram_session_t net_ne_dgram_session_create(net_ne_dgram_t dgram, net_address_t remote_address) {
    net_ne_driver_t driver = net_ne_dgram_driver(dgram);
    net_ne_dgram_session_t session;

    session = TAILQ_FIRST(&driver->m_free_dgram_sessions);
    if (session) {
        TAILQ_REMOVE(&driver->m_free_dgram_sessions, session, m_next);
    }
    else {
        session = mem_alloc(driver->m_alloc, sizeof(struct net_ne_dgram_session));
        if (session == NULL) {
            CPE_ERROR(driver->m_em, "ne: alloc dgram session fail!");
            return NULL;
        }
    }

    session->m_dgram = dgram;
    session->m_writing = 0;
    session->m_remote_address = net_address_copy(net_ne_driver_schedule(driver), remote_address);
    if (session->m_remote_address == NULL) {
        CPE_ERROR(
            driver->m_em, "ne: dgram[-->%s]: dup remote address fail!",
            net_address_dump(net_ne_driver_tmp_buffer(driver), session->m_remote_address));
        session->m_dgram = (net_ne_dgram_t)driver;
        TAILQ_INSERT_TAIL(&driver->m_free_dgram_sessions, session, m_next);
        return NULL;
    }

    cpe_hash_entry_init(&session->m_hh_for_dgram);
    if (cpe_hash_table_insert_unique(&dgram->m_sessions, session) != 0) {
        CPE_ERROR(
            driver->m_em, "ne: dgram[-->%s]: duplicate!",
            net_address_dump(net_ne_driver_tmp_buffer(driver), session->m_remote_address));
        net_address_free(session->m_remote_address);
        session->m_dgram = (net_ne_dgram_t)driver;
        TAILQ_INSERT_TAIL(&driver->m_free_dgram_sessions, session, m_next);
        return NULL;
    }
    
    session->m_pending = [[NSMutableArray<NSData *> arrayWithCapacity: 0] retain];
    session->m_observer = [[[NetNeDgramSessionObserver alloc] init] retain];
    
    net_address_t local_address = net_dgram_address(net_dgram_from_data(dgram));
    
    session->m_session = [driver->m_tunnel_provider
                             createUDPSessionToEndpoint: net_ne_address_to_endoint(driver, remote_address)
                             fromEndpoint: local_address ? net_ne_address_to_endoint(driver, local_address) : nil];
    if (session->m_session == NULL) {
        CPE_ERROR(
            driver->m_em, "ne: dgram[-->%s]: socket create error",
            net_address_dump(net_ne_driver_tmp_buffer(driver), remote_address));
        [session->m_pending release];
        [session->m_observer release];
        cpe_hash_table_remove_by_ins(&dgram->m_sessions, session);
        net_address_free(session->m_remote_address);
        session->m_dgram = (net_ne_dgram_t)driver;
        TAILQ_INSERT_TAIL(&driver->m_free_dgram_sessions, session, m_next);
        return NULL;
    }
    [session->m_session retain];

    [session->m_session
        setReadHandler: (^(NSArray<NSData *> *datagrams, NSError *error) {
                net_ne_dgram_session_receive_cb(session, datagrams, error);
            })
        maxDatagrams: 1500];

    [session->m_session addObserver: session->m_observer
                         forKeyPath: @"state" 
                            options: NSKeyValueObservingOptionOld | NSKeyValueObservingOptionNew
                            context: nil];
    
    return session;
}

void net_ne_dgram_session_free(net_ne_dgram_session_t session) {
    net_ne_driver_t driver = net_ne_dgram_driver(session->m_dgram);
    
    if (session->m_session) {
        [session->m_session cancel];
        [session->m_session release];
        session->m_session = nil;
    }

    [session->m_pending release];
    session->m_pending = nil;

    session->m_observer->m_session = nil;
    [session->m_observer release];
    session->m_observer = nil;

    cpe_hash_table_remove_by_ins(&session->m_dgram->m_sessions, session);
    
    net_address_free(session->m_remote_address);

    session->m_dgram = (net_ne_dgram_t)driver;
    TAILQ_INSERT_TAIL(&driver->m_free_dgram_sessions, session, m_next);
}

void net_ne_dgram_session_free_all(net_ne_dgram_t dgram) {
    struct cpe_hash_it session_it;
    net_ne_dgram_session_t session;

    cpe_hash_it_init(&session_it, &dgram->m_sessions);

    session = cpe_hash_it_next(&session_it);
    while(session) {
        net_ne_dgram_session_t next = cpe_hash_it_next(&session_it);
        net_ne_dgram_session_free(session);
        session = next;
    }
}

void net_ne_dgram_session_real_free(net_ne_dgram_session_t session) {
    net_ne_driver_t driver = (net_ne_driver_t)session->m_dgram;
    TAILQ_REMOVE(&driver->m_free_dgram_sessions, session, m_next);
    mem_free(driver->m_alloc, session);
}

net_ne_dgram_session_t net_ne_dgram_session_find(net_ne_dgram_t dgram, net_address_t remote_address) {
    struct net_ne_dgram_session key;
    key.m_remote_address = remote_address;
    return cpe_hash_table_find(&dgram->m_sessions, &key);
}

int net_ne_dgram_session_send(net_ne_dgram_session_t session, void const * data, size_t data_len) {
    NSData * sendData = [NSData dataWithBytes: data length: data_len];
    [session->m_pending addObject: sendData];

    return net_ne_dgram_session_check_write(session);
}

static void net_ne_dgram_session_receive_cb(net_ne_dgram_session_t session, NSArray<NSData *> *datagrams, NSError *error) {
    dispatch_sync(
        dispatch_get_main_queue(),
        ^{
            net_ne_driver_t driver = net_ne_dgram_driver(session->m_dgram);

            if (error) {
                CPE_ERROR(
                    driver->m_em,
                    "ne: dgram[-->%s]: receive error, errro=%d %s",
                    net_address_dump(net_ne_driver_tmp_buffer(driver), session->m_remote_address),
                    (int)[error code],
                    [[error localizedDescription] UTF8String]);
                return;
            }

            int i;
            for(i = 0; i < datagrams.count; ++i) {
                NSData * data = datagrams[i];
                char buf[1500] = {0};

                if (data.length > sizeof(buf)) {
                    CPE_ERROR(
                        driver->m_em,
                        "ne: dgram[-->%s]: receive %d data, overflow, mut=%d",
                        net_address_dump(net_ne_driver_tmp_buffer(driver), session->m_remote_address),
                        (int)data.length, (int)sizeof(buf));
                    continue;
                }

                if (driver->m_debug) {
                    CPE_INFO(
                        driver->m_em, "ne: dgram[-->%s]: recv %d data",
                        net_address_dump(net_ne_driver_tmp_buffer(driver), session->m_remote_address),
                        (int)data.length);
                }

                memcpy(buf,  [data bytes], data.length);
                net_dgram_recv(net_dgram_from_data(session->m_dgram), session->m_remote_address, buf, data.length);

                if (driver->m_data_monitor_fun) {
                    driver->m_data_monitor_fun(driver->m_data_monitor_ctx, NULL, net_data_in, (uint32_t)data.length);
                }
            }
        });
}

static int net_ne_dgram_session_check_write(net_ne_dgram_session_t session) {
    net_ne_driver_t driver = net_ne_dgram_driver(session->m_dgram);
    
    if (session->m_session.state != NWUDPSessionStateReady) return 0;
    if (session->m_writing) return 0;
    if (session->m_pending.count == 0) return 0;

    session->m_writing = 1;

    uint32_t sendSize = (uint32_t)session->m_pending.firstObject.length;
    [session->m_session writeDatagram: session->m_pending.firstObject completionHandler: ^(NSError *error) {
            dispatch_sync(
                dispatch_get_main_queue(),
                ^{
                    session->m_writing = 0;
                    
                    if (error) {
                        CPE_ERROR(
                            driver->m_em, "ne: dgram[-->%s]: send error, error=%d (%s)",
                            net_address_dump(net_ne_driver_tmp_buffer(driver), session->m_remote_address),
                            (int)[error code],
                            [[error localizedDescription] UTF8String]);
                        return;
                    }

                    if (driver->m_debug) {
                        CPE_INFO(
                            driver->m_em, "ne: dgram[-->%s]: send data %d",
                            net_address_dump(net_ne_driver_tmp_buffer(driver), session->m_remote_address),
                            (int)sendSize);
                    }
                    
                    net_ne_dgram_session_check_write(session);
                });            
        }];
    [session->m_pending removeObjectAtIndex: 0];
    
    return 0;
}

static const char * net_ne_dgram_state_str(NWUDPSessionState state) {
    switch(state) {
    case NWUDPSessionStateInvalid:
        return "invalid";
    case NWUDPSessionStateWaiting:
        return "waiting";
    case NWUDPSessionStatePreparing:
        return "preparing";
    case NWUDPSessionStateReady:
        return "ready";
    case NWUDPSessionStateFailed:
        return "failed";
    case NWUDPSessionStateCancelled:
        return "canceled";
    default:
        return "Unknown";
    }
}

@implementation NetNeDgramSessionObserver
-(void)observeValueForKeyPath:(NSString *)keyPath 
                     ofObject:(id)object 
                       change:(NSDictionary *)change 
                      context:(void *)context
{
    dispatch_sync(
        dispatch_get_main_queue(),
        ^{
            @autoreleasepool {
                net_ne_dgram_session_t session = self->m_session;
                if (session == NULL) return;

                net_ne_driver_t driver = net_ne_dgram_driver(session->m_dgram);

                if ([keyPath isEqual:@"state"]) {
                    NWUDPSessionState oldState = (NWUDPSessionState)[[change objectForKey:@"old"] intValue];
                    NWUDPSessionState newState = (NWUDPSessionState)[[change objectForKey:@"new"] intValue];
        
                    CPE_INFO(
                        driver->m_em, "ne: dgram[-->%s]: state %s(%d) ==> %s(%d)!",
                        net_address_dump(net_ne_driver_tmp_buffer(driver), session->m_remote_address),
                        net_ne_dgram_state_str(oldState), (int)oldState, net_ne_dgram_state_str(newState), (int)newState);

                    switch(newState) {
                    case NWUDPSessionStateReady:
                        net_ne_dgram_session_check_write(session);
                        break;
                    default:
                        break;
                    }
                }
            }
        });
}
@end

uint32_t net_ne_dgram_session_address_hash(net_ne_dgram_session_t session, void * user_data) {
    return net_address_hash(session->m_remote_address);
}

int net_ne_dgram_session_address_eq(net_ne_dgram_session_t l, net_ne_dgram_session_t r, void * user_data) {
    return net_address_cmp(l->m_remote_address, r->m_remote_address) == 0 ? 1 : 0;
}
