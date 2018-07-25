#include "net_address.h"
#include "net_dgram.h"
#include "net_ne_dgram_session.h"
#include "net_ne_utils.h"

static void net_ne_dgram_session_receive_cb(net_ne_dgram_session_t session, NSArray<NSData *> *datagrams, NSError *error);

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
    
    net_address_t local_address = net_dgram_address(net_dgram_from_data(dgram));
    
    session->m_session = [driver->m_tunnel_provider
                             createUDPSessionToEndpoint: net_ne_address_to_endoint(driver, remote_address)
                             fromEndpoint: local_address ? net_ne_address_to_endoint(driver, local_address) : nil];
    if (session->m_session == NULL) {
        CPE_ERROR(driver->m_em, "ne: dgram: socket create error");
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
    
        //net_ne_driver_t driver = net_driver_data(net_dgram_driver(base_dgram));

    return session;
}

void net_ne_dgram_session_free(net_ne_dgram_session_t session) {
    net_ne_driver_t driver = net_ne_dgram_driver(session->m_dgram);
    
    if (session->m_session) {
        [session->m_session cancel];
        [session->m_session release];
        session->m_session = NULL;
    }

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
    // int nret = sendto(dgram->m_fd, data, data_len, 0, (struct sockaddr *)&addr, addr_len);
    // if (nret < 0) {
    //     CPE_ERROR(
    //         net_schedule_em(schedule), "ne: dgram: send %d data to %s fail, errno=%d (%s)",
    //         (int)data_len, net_address_dump(net_schedule_tmp_buffer(schedule), target),
    //         cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
    //     return -1;
    // }
    // else {
    //     if (driver->m_debug) {
    //         CPE_INFO(
    //             net_schedule_em(schedule), "ne: dgram: send %d data to %s",
    //             (int)data_len,
    //             net_address_dump(net_schedule_tmp_buffer(schedule), target));
    //     }

    //     if (driver->m_data_monitor_fun) {
    //         driver->m_data_monitor_fun(driver->m_data_monitor_ctx, NULL, net_data_out, (uint32_t)nret);
    //     }
    // }

    // return nret;
    return 0;
}

static void net_ne_dgram_session_receive_cb(net_ne_dgram_session_t session, NSArray<NSData *> *datagrams, NSError *error) {
    net_ne_driver_t driver = net_ne_dgram_driver(session->m_dgram);

    if (error) {
        CPE_ERROR(
            driver->m_em,
            "ne: dgram: receive error, errro=%d %s",
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
                "ne: dgram: receive %d data, overflow, mut=%d",
                (int)data.length, (int)sizeof(buf));
            continue;
        }

        memcpy(buf,  [data bytes], data.length);
        net_dgram_recv(net_dgram_from_data(session->m_dgram), session->m_remote_address, buf, data.length);

        if (driver->m_data_monitor_fun) {
            driver->m_data_monitor_fun(driver->m_data_monitor_ctx, NULL, net_data_in, (uint32_t)data.length);
        }
    }
}

uint32_t net_ne_dgram_session_hash(net_ne_dgram_session_t session, void * user_data) {
    return net_address_hash(session->m_remote_address);
}

int net_ne_dgram_session_eq(net_ne_dgram_session_t l, net_ne_dgram_session_t r, void * user_data) {
    return net_address_cmp(l->m_remote_address, r->m_remote_address) == 0 ? 1 : 0;
}
