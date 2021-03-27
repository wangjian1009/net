#ifndef PROMETHEUS_PROESS_LINUX_FDS_I_INCLUDED
#define PROMETHEUS_PROESS_LINUX_FDS_I_INCLUDED
#include "prometheus_process_provider_i.h"

int prometheus_process_linux_fds_count(prometheus_process_provider_t provider, const char *path);

#endif
