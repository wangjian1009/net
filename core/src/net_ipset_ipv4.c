#include "cpe/utils/bitarry.h"
#include "net_ipset_i.h"
#include "net_ipset_node.h"
#include "net_ipset_node_cache.h"

#define NET_ADDRESS_DATA struct net_address_data_ipv4

#define NET_IP_BIT_SIZE  32
#define NET_IP_DISCRIMINATOR_VALUE  1

#define NET_IPSET_NAME(basename) net_ipset_ipv4_##basename
#define NET_IPSET_PRENAME(basename) net_ipset_##basename##_ipv4

#include "net_ipset_template.c.in"
