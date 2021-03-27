#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_dirent.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/pal/pal_unistd.h"
#include "cpe/pal/pal_errno.h"
#include "cpe/pal/pal_strings.h"
#include "prometheus_process_linux_fds_i.h"

int prometheus_process_linux_fds_count(prometheus_process_provider_t provider, const char * path) {
    int r = 0;

    char path_buf[50];
    if (path == NULL) {
        int pid = (int)getpid();
        snprintf(path_buf, sizeof(path_buf), "/proc/%d/fd", pid);
        path = path_buf;
    }

    DIR * d = opendir(path);
    if (d == NULL) {
        CPE_ERROR(
            provider->m_em, "prometheus: process: fds: open dir %s fail, error=%d(%s)",
            path, errno, strerror(errno));
        return -1;
    }

    struct dirent * de;
    int count = 0;
    while ((de = readdir(d)) != NULL) {
        if (strcmp(".", de->d_name) == 0 || strcmp("..", de->d_name) == 0) {
            continue;
        }
        count++;
    }

    if (closedir(d) != 0) {
        CPE_ERROR(
            provider->m_em, "prometheus: process: fds: close dir %s fail, error=%d(%s)",
            path, errno, strerror(errno));
        return -1;
    }

    return count;
}
