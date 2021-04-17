#include <assert.h>
#include <sys/wait.h>
#include "cpe/pal/pal_signal.h"
#include "cpe/pal/pal_errno.h"
#include "cpe/pal/pal_unistd.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils_sock/sock_utils.h"
#include "net_progress.h"
#include "net_watcher.h"
#include "net_sock_progress_i.h"

static void net_sock_progress_on_event(void * ctx, int fd, uint8_t do_read, uint8_t do_write);
static void net_sock_progress_check_close(net_sock_progress_t progress, uint8_t do_kill, int * stat);

int net_sock_progress_init(
    net_progress_t base_progress,
    const char * cmd, const char * argv[], net_progress_runing_mode_t mode)
{
    net_driver_t base_driver = net_progress_driver(base_progress);
    net_sock_driver_t driver = net_driver_data(base_driver);
    net_sock_progress_t progress = net_progress_data(base_progress);

    int pfd[2];
    if (pipe(pfd) < 0) {
        CPE_ERROR(driver->m_em, "sock: progress: crate pipe fail, error=%d (%s)", errno, strerror(errno));
        return -1;
    }

    progress->m_child_pid = fork();
    if (progress->m_child_pid < 0) {
        CPE_ERROR(driver->m_em, "sock: progress: fork fail, error=%d (%s)", errno, strerror(errno));
        close(pfd[0]); close(pfd[1]);
        return -1;
    }

    if (progress->m_child_pid == 0) {
        /*子进程*/
        if (mode == net_progress_runing_mode_read) {
            close(pfd[0]);
            if (pfd[1] != STDOUT_FILENO) {
                dup2(pfd[1], STDOUT_FILENO);
                close(pfd[1]);
            }
        }
        else {
            close(pfd[1]);
            if (pfd[0] != STDIN_FILENO) {
                dup2(pfd[0], STDIN_FILENO);
                close(pfd[0]);
            }
        }

        uint16_t arg_count = 0;
        while(argv[arg_count] != NULL) arg_count++;

        char * * dup_args = malloc(sizeof(char*) * (arg_count + 1));
        uint16_t i;
        for(i = 0; i < arg_count; i++) {
            dup_args[i] = strdup(argv[i]);
        }
        dup_args[arg_count] = NULL;

        closefrom(3);
  
        if (execv(cmd, dup_args) != 0) {
            fprintf(stderr, "execute %s fail, errno=%d (%s)", cmd, errno, strerror(errno));
        }
        
        _exit(errno);
    }

    if (mode == net_progress_runing_mode_read) {
        progress->m_fd = pfd[0];
        close(pfd[1]);
    } else {
        close(pfd[0]);
        progress->m_fd = pfd[1];
    }

    if (cpe_sock_set_none_block(progress->m_fd, 1) != 0) {
        CPE_ERROR(driver->m_em, "sock: progress: set non block fail, error=%d (%s)", errno, strerror(errno));
        close(progress->m_fd);
        return -1;
    }
    
    progress->m_watcher = net_watcher_create(base_driver, progress->m_fd, progress, net_sock_progress_on_event);
    if (progress->m_watcher == NULL) {
        CPE_ERROR(driver->m_em, "sock: progress: create watcher fail");
        close(progress->m_fd);
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
    net_driver_t base_driver = net_progress_driver(base_progress);
    net_sock_driver_t driver = net_driver_data(base_driver);

    net_sock_progress_check_close(progress, 1, NULL);
    
    assert(progress->m_watcher == NULL);
    assert(progress->m_fd == -1);
    assert(progress->m_child_pid == -1);
}

static void net_sock_progress_on_read(net_sock_progress_t progress) {
    net_progress_t base_progress = net_progress_from_data(progress);
    net_driver_t base_driver = net_progress_driver(base_progress);
    net_sock_driver_t driver = net_driver_data(base_driver);

    while (1) {
        uint32_t capacity = 512;
        void * buf = net_progress_buf_alloc_suggest(base_progress, &capacity);
        if (buf == NULL) {
            CPE_ERROR(
                driver->m_em, "sock: %s: alloc buf fail",
                net_progress_dump(net_sock_driver_tmp_buffer(driver), base_progress));
            net_sock_progress_check_close(progress, 1, NULL);
            net_progress_set_error(base_progress, ENOMEM, "OnReadAllocFail");
            return;
        }

        ssize_t rv = read(progress->m_fd, buf, capacity);
        if (rv < 0) {
            net_progress_buf_release(base_progress);
            
            if (errno == EINPROGRESS || errno == EWOULDBLOCK) {
                return;
            } else {
                CPE_ERROR(
                    driver->m_em, "sock: %s: read fail, errno=%d (%s)",
                    net_progress_dump(net_sock_driver_tmp_buffer(driver), base_progress),
                    errno, strerror(errno));
                net_sock_progress_check_close(progress, 1, NULL);
                net_progress_set_error(base_progress, errno, "ReadError");
                return;
            }
        } else if (rv == 0) {
            net_progress_buf_release(base_progress);

            int stat = -1;
            net_sock_progress_check_close(progress, 0, &stat);
            
            CPE_INFO(
                driver->m_em, "sock: %s: child completed, stat=%d",
                net_progress_dump(net_sock_driver_tmp_buffer(driver), base_progress), stat);
            net_progress_set_complete(base_progress, stat);
            return;
        } else {
            if (net_progress_buf_supply(base_progress, (uint32_t)rv) != 0) {
                CPE_ERROR(
                    driver->m_em, "sock: %s: supply fail",
                    net_progress_dump(net_sock_driver_tmp_buffer(driver), base_progress));
                net_sock_progress_check_close(progress, 1, NULL);
                net_progress_set_error(base_progress, EFAULT, "BufSupplyFail");
                return;
            }
        }
    }
}
    
static void net_sock_progress_on_event(void * ctx, int fd, uint8_t do_read, uint8_t do_write) {
    net_sock_progress_t progress = ctx;

    if (do_read) {
        net_sock_progress_on_read(progress);
    }

    if (do_write) {
    }
}

static void net_sock_progress_check_close(net_sock_progress_t progress, uint8_t do_kill, int * stat) {
    net_progress_t base_progress = net_progress_from_data(progress);
    net_driver_t base_driver = net_progress_driver(base_progress);
    net_sock_driver_t driver = net_driver_data(base_driver);

    if (progress->m_child_pid > 0) {
        if (do_kill) {
            if (kill(progress->m_child_pid, 9) != 0) {
                CPE_ERROR(
                    driver->m_em, "sock: %s: close child %d: kill error, errno=%d (%s)",
                    net_progress_dump(net_sock_driver_tmp_buffer(driver), base_progress),
                    progress->m_child_pid, errno, strerror(errno));
            }
        }

        int stat_buf;
        if (stat == NULL) stat = &stat_buf;
        while (waitpid(progress->m_child_pid, stat, 0) < 0) {
            if (errno != EINTR) {
                CPE_ERROR(
                    driver->m_em, "sock: %s: close child %d: wait pid fail, error=%d (%s)",
                    net_progress_dump(net_sock_driver_tmp_buffer(driver), base_progress),
                    progress->m_child_pid, errno, strerror(errno));
                break;
            }
        }

        CPE_INFO(
            driver->m_em, "sock: %s: close child %d success, state=%d",
            net_progress_dump(net_sock_driver_tmp_buffer(driver), base_progress),
            progress->m_child_pid, *stat);

        progress->m_child_pid = -1;
    }

    if (progress->m_watcher) {
        net_watcher_free(progress->m_watcher);
        progress->m_watcher = NULL;
    }
    
    if (progress->m_fd >= 0) {
        close(progress->m_fd);
        progress->m_fd = -1;
    }
}
