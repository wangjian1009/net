#include "cpe/pal/pal_errno.h"
#include "cpe/pal/pal_unistd.h"
#include "cpe/pal/pal_string.h"
#include "net_progress.h"
#include "net_watcher.h"
#include "net_sock_progress_i.h"

static void net_sock_progress_on_event(void * ctx, int fd, uint8_t do_read, uint8_t do_write);

int net_sock_progress_init(net_progress_t base_progress, const char * cmd, net_progress_runing_mode_t mode) {
    net_driver_t base_driver = net_progress_driver(base_progress);
    net_sock_driver_t driver = net_driver_data(base_driver);
    net_sock_progress_t progress = net_progress_data(base_progress);

    progress->m_fp = popen(cmd, mode == net_progress_runing_mode_read ? "r" : "w");
    if (progress->m_fp == NULL) {
        CPE_ERROR(driver->m_em, "sock: progress: open %s, error=%d (%s)", cmd, errno, strerror(errno));
        return -1;
    }  

    progress->m_watcher = net_watcher_create(base_driver, 1, progress, net_sock_progress_on_event);
    if (progress->m_watcher == NULL) {
        CPE_ERROR(driver->m_em, "sock: progress: create watcher fail");
        pclose(progress->m_fp);
        return -1;
    }
    net_watcher_update(
        progress->m_watcher,
        mode == net_progress_runing_mode_read ? 1 : 0,
        mode == net_progress_runing_mode_write ? 1 : 0);

    return 0;
}

void net_sock_progress_fini(net_progress_t base_progress) {
    net_sock_progress_t progress = net_progress_data(base_progress);

    if (progress->m_watcher) {
        net_watcher_free(progress->m_watcher);
        progress->m_watcher = NULL;
    }

    if (progress->m_fp) {
        pclose(progress->m_fp);
        progress->m_fp = NULL;
    }
}

static void net_sock_progress_on_event(void * ctx, int fd, uint8_t do_read, uint8_t do_write) {
    net_sock_progress_t progress = ctx;
    net_progress_t base_progress = net_progress_from_data(progress);

    if (do_read) {
        uint32_t size;
        void * buf = net_progress_buf_alloc_suggest(base_progress, &size);
        fread(buf, 1, size, progress->m_fp);
    }

    if (do_write) {
    }
}
