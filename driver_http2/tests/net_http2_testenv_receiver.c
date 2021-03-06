#include "net_http2_testenv_receiver.h"

net_http2_testenv_receiver_t
net_http2_testenv_receiver_create(net_http2_testenv_t env) {
    net_http2_testenv_receiver_t receiver = mem_alloc(test_allocrator(), sizeof(struct net_http2_testenv_receiver));

    receiver->m_env = env;
    mem_buffer_init(&receiver->m_buffer, test_allocrator());

    TAILQ_INSERT_TAIL(&env->m_receivers, receiver, m_next);

    return receiver;
}

void net_http2_testenv_receiver_free(net_http2_testenv_receiver_t receiver) {
    net_http2_testenv_t env = receiver->m_env;

    mem_buffer_clear(&receiver->m_buffer);

    TAILQ_REMOVE(&env->m_receivers, receiver, m_next);
    mem_free(test_allocrator(), receiver);
}

