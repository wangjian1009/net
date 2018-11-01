#include "assert.h"
#include "net_link_i.h"
#include "net_endpoint_i.h"
#include "net_driver_i.h"

net_link_t
net_link_create(
    net_endpoint_t local, uint8_t local_is_tie, net_endpoint_t remote, uint8_t remote_is_tie)
{
    net_schedule_t schedule = local->m_driver->m_schedule;

    assert(local->m_link == NULL);
    assert(remote->m_link == NULL);
    
    net_link_t link = TAILQ_FIRST(&schedule->m_free_links);
    if (link) {
        TAILQ_REMOVE(&schedule->m_free_links, link, m_next);
    }
    else {
        link = mem_alloc(schedule->m_alloc, sizeof(struct net_link));
        if (link == NULL) {
            CPE_ERROR(schedule->m_em, "net_link_create: alloc fail!");
            return NULL;
        }
    }

    local->m_link = link;
    link->m_local = local;
    link->m_local_is_tie = local_is_tie;

    remote->m_link = link;
    link->m_remote = remote;
    link->m_remote_is_tie = remote_is_tie;
    
    return link;
}

void net_link_free(net_link_t link) {
    net_schedule_t schedule = link->m_local->m_driver->m_schedule;

    assert(link->m_local->m_link == link);
    link->m_local->m_link = NULL;

    if (link->m_local_is_tie) {
        net_endpoint_set_state(link->m_local, net_endpoint_state_deleting);
        link->m_local = NULL;
    }

    assert(link->m_remote->m_link == link);
    link->m_remote->m_link = NULL;

    if (link->m_remote_is_tie) {
        net_endpoint_set_state(link->m_remote, net_endpoint_state_deleting);
        link->m_remote = NULL;
    }

    link->m_local = (net_endpoint_t)schedule;
    TAILQ_INSERT_TAIL(&schedule->m_free_links, link, m_next);
}

void net_link_real_free(net_link_t link) {
    net_schedule_t schedule = (net_schedule_t)link->m_local;
    TAILQ_REMOVE(&schedule->m_free_links, link, m_next);
    mem_free(schedule->m_alloc, link);
}

uint8_t net_link_is_tie(net_link_t link, net_endpoint_t endpoint) {
    if (link->m_local == endpoint) {
        return link->m_local_is_tie;
    }
    else {
        assert(link->m_remote == endpoint);
        return link->m_remote_is_tie;
    }
}

void net_link_set_tie(net_link_t link, net_endpoint_t endpoint, uint8_t is_tie) {
    if (link->m_local == endpoint) {
        link->m_local_is_tie = is_tie;
    }
    else {
        assert(link->m_remote == endpoint);
        link->m_remote_is_tie = is_tie;
    }
}
