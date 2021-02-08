#include "net_mem_group_type_basic_i.h"
#include "net_mem_group_type_i.h"

net_mem_group_type_t
net_mem_group_type_basic_create(net_schedule_t schedule) {
    return net_mem_group_type_create(schedule, "basic", 0);
}
