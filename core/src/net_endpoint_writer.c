#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "net_endpoint_writer.h"
#include "net_endpoint_i.h"

void net_endpoint_writer_init(net_endpoint_writer_t writer, net_endpoint_t o_ep, net_endpoint_buf_type_t o_buf) {
    bzero(writer, sizeof(*writer));

    writer->m_o_ep = o_ep;
    writer->m_o_buf = o_buf;
    writer->m_capacity = 0;
    writer->m_totall_len = 0;
    writer->m_wp = NULL;
}

void net_endpoint_writer_cancel(net_endpoint_writer_t writer) {
    if (writer->m_size > 0) {
        net_schedule_t schedule = net_endpoint_schedule(writer->m_o_ep);
        
        if (net_endpoint_state(writer->m_o_ep) == net_endpoint_state_deleting) {
            CPE_ERROR(
                schedule->m_em,
                "core: %s: writer cancel: endpoint is deleting, skip",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), writer->m_o_ep));
        }
        else if (!net_endpoint_buf_validate(writer->m_o_ep, writer->m_wp, writer->m_capacity)) {
            CPE_ERROR(
                schedule->m_em,
                "core: %s: writer cancel: validate fail, state=%s, ignore cancel",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), writer->m_o_ep),
                net_endpoint_state_str(net_endpoint_state(writer->m_o_ep)));
        }
        else {
            net_endpoint_buf_release(writer->m_o_ep);
        }
    }
}

int net_endpoint_writer_commit(net_endpoint_writer_t writer) {
    if (writer->m_size == 0) {
        return 0;
    }
    else {
        net_schedule_t schedule = net_endpoint_schedule(writer->m_o_ep);
        
        if (net_endpoint_state(writer->m_o_ep) == net_endpoint_state_deleting) {
            CPE_ERROR(
                schedule->m_em,
                "core: %s: writer commit: endpoint is deleting, skip",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), writer->m_o_ep));
            return -1;
        }
        else if (!net_endpoint_buf_validate(writer->m_o_ep, writer->m_wp, writer->m_capacity)) {
            CPE_ERROR(
                schedule->m_em,
                "core: %s: writer commit: validate fail, state=%s, ignore commit",
                net_endpoint_dump(net_schedule_tmp_buffer(schedule), writer->m_o_ep),
                net_endpoint_state_str(net_endpoint_state(writer->m_o_ep)));
            return -1;
        }
        else {
            int supply_rv = net_endpoint_buf_supply(writer->m_o_ep, writer->m_o_buf, writer->m_size);
            writer->m_totall_len += writer->m_size;
            writer->m_wp = NULL;
            writer->m_size = 0;
            
            if (supply_rv != 0) {
                CPE_ERROR(
                    schedule->m_em, "core: %s: writer commit: supply %d data fail",
                    net_endpoint_dump(net_schedule_tmp_buffer(schedule), writer->m_o_ep),
                    writer->m_size);
                return -1;
            }

            return 0;
        }
    }
}

int net_endpoint_writer_append(net_endpoint_writer_t writer, void const * data, uint32_t sz) {
    net_schedule_t schedule = net_endpoint_schedule(writer->m_o_ep);

    while(sz > 0) {
        if (net_endpoint_state(writer->m_o_ep) == net_endpoint_state_deleting) {
            return -1;
        }
        
        if (writer->m_wp == NULL) {
            assert(writer->m_size == 0);
            assert(writer->m_capacity == 0);
                   
            writer->m_wp = net_endpoint_buf_alloc_at_least(writer->m_o_ep, &writer->m_capacity);
            if (writer->m_wp == NULL) {
                CPE_ERROR(
                    schedule->m_em, "core: %s: writer append: alloc fail",
                    net_endpoint_dump(net_schedule_tmp_buffer(schedule), writer->m_o_ep));
                return -1;
            }
        }
        else {
            if (!net_endpoint_buf_validate(writer->m_o_ep, writer->m_wp, writer->m_capacity)) {
                CPE_ERROR(
                    schedule->m_em,
                    "core: %s: writer append: writer validate fail, state=%s",
                    net_endpoint_dump(net_schedule_tmp_buffer(schedule), writer->m_o_ep),
                    net_endpoint_state_str(net_endpoint_state(writer->m_o_ep)));
                return -1;
            }
        }
        
        assert(writer->m_capacity > 0);
        
        uint32_t copy_sz = sz;
        if (copy_sz > writer->m_capacity) {
            copy_sz = writer->m_capacity;
        }

        memcpy(writer->m_wp, data, copy_sz);
        writer->m_wp += copy_sz;
        writer->m_size += copy_sz;
        writer->m_capacity -= copy_sz;

        sz -= copy_sz;
        data = ((uint8_t const *)data) + copy_sz;

        if (writer->m_capacity == 0) {
            int supply_rv = net_endpoint_buf_supply(writer->m_o_ep, writer->m_o_buf, writer->m_size);
            writer->m_totall_len += writer->m_size;
            writer->m_wp = NULL;
            writer->m_size = 0;
            
            if (supply_rv != 0) {
                CPE_ERROR(
                    schedule->m_em, "core: %s: writer append, supply %d data fail",
                    net_endpoint_dump(net_schedule_tmp_buffer(schedule), writer->m_o_ep),
                    writer->m_size);
                return -1;
            }
        }
    }

    return 0;
}

int net_endpoint_writer_append_str(net_endpoint_writer_t writer, const char * msg) {
    return net_endpoint_writer_append(writer, msg, (uint32_t)strlen(msg));
}
