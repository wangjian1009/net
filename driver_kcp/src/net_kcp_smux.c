#include "net_dgram.h"
#include "net_address.h"
#include "net_kcp_smux_i.h"

void net_kcp_smux_process(net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source);

net_kcp_smux_t
net_kcp_smux_create(net_kcp_driver_t driver, net_address_t address) {
    net_kcp_smux_t mux = mem_alloc(driver->m_alloc, sizeof(struct net_kcp_smux));
    if (mux == NULL) {
        CPE_ERROR(
            driver->m_em, "kcp: mux %s: create: alloc fail",
            net_address_dump(net_kcp_driver_tmp_buffer(driver), address));
        return NULL;
    }

    mux->m_driver = driver;

    mux->m_dgram = net_dgram_create(driver->m_underline_driver, address, net_kcp_smux_process, mux);
    if (mux->m_dgram == NULL) {
        CPE_ERROR(
            driver->m_em, "kcp: mux %s: create: create dgram failed",
            net_address_dump(net_kcp_driver_tmp_buffer(driver), address));
        mem_free(driver->m_alloc, mux);
        return NULL;
    }
        
    TAILQ_INSERT_TAIL(&driver->m_muxes, mux, m_next);
    return mux;
}

void net_kcp_smux_free(net_kcp_smux_t mux) {
    net_kcp_driver_t driver = mux->m_driver;

    if (mux->m_dgram) {
        net_dgram_free(mux->m_dgram);
        mux->m_dgram = NULL;
    }

    TAILQ_REMOVE(&driver->m_muxes, mux, m_next);
    mem_free(driver->m_alloc, mux);
}

void net_kcp_smux_process(net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source) {
}
