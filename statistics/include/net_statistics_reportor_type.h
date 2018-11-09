#ifndef NET_STATISTICS_REPORTOR_TYPE_H_INCLEDED
#define NET_STATISTICS_REPORTOR_TYPE_H_INCLEDED
#include "net_statistics_system.h"

CPE_BEGIN_DECL

typedef int (*net_statistics_reportor_init_fun_t)(void * ctx, net_statistics_reportor_t reportor);
typedef void (*net_statistics_reportor_fini_fun_t)(void * ctx, net_statistics_reportor_t reportor);

net_statistics_reportor_type_t
net_statistics_reportor_type_create(
    net_statistics_t statistics,
    const char * name,
    void * ctx,
    uint16_t capacity,
    net_statistics_reportor_init_fun_t init_fun,
    net_statistics_reportor_fini_fun_t fini_fun);

void net_statistics_reportor_type_free(net_statistics_reportor_type_t reportor_type);

CPE_END_DECL

#endif
