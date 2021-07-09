#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_dirent.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/pal/pal_unistd.h"
#include "cpe/pal/pal_errno.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/vfs/vfs_dir.h"
#include "cpe/vfs/vfs_entry_info.h"
#include "prometheus_process_linux_fds_i.h"

int prometheus_process_linux_fds_count(prometheus_process_provider_t provider, uint32_t * count) {
    int r = 0;

    char path[50];
    int pid = (int)getpid();
    snprintf(path, sizeof(path), "/proc/%d/fd", pid);

    CPE_ERROR(provider->m_em, "xxxxxx: collect open fdns: path = %s", path);
    
    vfs_dir_t d = vfs_dir_open(provider->m_vfs_mgr, path);
    if (d == NULL) {
        CPE_ERROR(
            provider->m_em, "prometheus: process: fds: open dir %s fail, error=%d(%s)",
            path, errno, strerror(errno));
        return -1;
    }

    CPE_ERROR(provider->m_em, "xxxxxx: collect open fdns: path = %s, open success", path);

    struct vfs_entry_info_it entry_it;
    vfs_dir_entries(d, &entry_it);

    *count = 0;

    vfs_entry_info_t entry;
    while((entry = vfs_entry_info_it_next(&entry_it))) {
        if (strcmp(".", entry->m_name) == 0 || strcmp("..", entry->m_name) == 0) {
            continue;
        }
        (*count)++;
    }

    vfs_dir_close(d);
    CPE_ERROR(provider->m_em, "xxxxxx: collect open fdns: count=%d", *count);
    
    return 0;
}
