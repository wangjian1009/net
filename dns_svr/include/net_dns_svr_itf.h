#ifndef NET_DNS_SVR_ITF_H_INCLEDED
#define NET_DNS_SVR_ITF_H_INCLEDED
#include "net_dns_svr_types.h"

NET_BEGIN_DECL

net_dns_svr_itf_t net_dns_svr_itf_create(net_dns_svr_t dns_svr, net_dns_svr_itf_type_t type, net_address_t address, net_driver_t driver);
void net_dns_svr_itf_free(net_dns_svr_itf_t dns_itf);

net_dns_svr_t net_dns_itf_svr(net_dns_svr_itf_t dns_itf);
net_dns_svr_itf_type_t net_dns_itf_type(net_dns_svr_itf_t dns_itf);
net_address_t net_dns_itf_address(net_dns_svr_itf_t dns_itf);
net_driver_t net_dns_itf_driver(net_dns_svr_itf_t dns_itf);

const char * net_dns_svr_itf_type_str(net_dns_svr_itf_type_t type);

NET_END_DECL

#endif
