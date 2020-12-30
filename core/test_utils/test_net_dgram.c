#include "test_net_dgram.h"

int test_net_dgram_init(net_dgram_t base_dgram) {
    test_net_dgram_t dgram = net_dgram_data(base_dgram);
    return 0;
}

void test_net_dgram_fini(net_dgram_t base_dgram) {
}

int test_net_dgram_send(net_dgram_t base_dgram, net_address_t target, void const * data, size_t data_len) {
    return -1;
}


