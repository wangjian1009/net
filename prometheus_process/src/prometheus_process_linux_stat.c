#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/pal/pal_unistd.h"
#include "cpe/pal/pal_errno.h" 
#include "cpe/vfs/vfs_file.h"
#include "prometheus_process_linux_stat_i.h"

int prometheus_process_linux_stat_read(
    prometheus_process_provider_t provider, prometheus_process_linux_stat_t process_stat, const char * data)
{
    sscanf(
        data,
        "%d "                          // (1) pid  %d
        "%s "                          // (2) comm  %s
        "%c "                          // (3) state  %c
        "%d "                          // (4) ppid  %d
        "%d "                          // (5) pgrp  %d
        "%d "                          // (6) session  %d
        "%d "                          // (7) tty_nr  %d
        "%d "                          // (8) tpgid  %d
        "%u "                          // (9) flags  %u
        "%lu "                         // (10) minflt  %lu
        "%lu "                         // (11) cminflt  %lu
        "%lu "                         // (12) majflt  %lu
        "%lu "                         // (13) cmajflt  %lu
        "%lu "                         // (14) utime  %lu
        "%lu "                         // (15) stime  %lu
        "%ld "                         // (16) cutime  %ld
        "%ld "                         // (17) cstime  %ld
        "%ld "                         // (18) priority  %ld
        "%ld "                         // (19) nice  %ld
        "%ld "                         // (20) num_threads  %ld
        "%ld "                         // (21) itrealvalue  %ld
        "%llu "                        // (22) starttime  %llu
        "%lu "                         // (23) vsize  %lu
        "%ld "                         // (24) rss  %ld
        "%lu "                         // (25) rsslim  %lu
        "%lu "                         // (26) startcode  %lu  [PT]
        "%lu "                         // (27) endcode  %lu  [PT]
        "%lu "                         // (28) startstack  %lu  [PT]
        "%lu "                         // (29) kstkesp  %lu  [PT]
        "%lu "                         // (30) kstkeip  %lu  [PT]
        "%lu "                         // (31) signal  %lu
        "%lu "                         // (32) blocked  %lu
        "%lu "                         // (33) sigignore  %lu
        "%lu "                         // (34) sigcatch  %lu
        "%lu "                         // (35) wchan  %lu  [PT]
        "%lu "                         // (36) nswap  %lu
        "%lu "                         // (37) cnswap  %lu
        "%d "                          // (38) exit_signal  %d  (since Linux 2.1.22)
        "%d "                          // (39) processor  %d  (since Linux 2.2.8)
        "%u "                          // (40) rt_priority  %u  (since Linux 2.5.19)
        "%u "                          // (41) policy  %u  (since Linux 2.5.19)
        "%llu "                        // (42) delayacct_blkio_ticks  %llu  (since Linux 2.6.18)
        "%lu "                         // (43) guest_time  %lu  (since Linux 2.6.24)
        "%ld "                         // (44) cguest_time  %ld  (since Linux 2.6.24)
        "%lu "                         // (45) start_data  %lu  (since Linux 3.3)  [PT]
        "%lu "                         // (46) end_data  %lu  (since Linux 3.3)  [PT]
        "%lu "                         // (47) start_brk  %lu  (since Linux 3.3)  [PT]
        "%lu "                         // (48) arg_start  %lu  (since Linux 3.5)  [PT]
        "%lu "                         // (49) arg_end  %lu  (since Linux 3.5)  [PT]
        "%lu "                         // (50) env_start  %lu  (since Linux 3.5)  [PT]
        "%lu "                         // (51) env_end  %lu  (since Linux 3.5)  [PT]
        "%d ",                         // (52) exit_code  %d  (since Linux 3.5)  [PT]
        &process_stat->pid,                    // (1) pid  %d
        process_stat->comm,                    // (2) comm  %s
        &process_stat->state,                  // (3) state  %c
        &process_stat->ppid,                   // (4) ppid  %d
        &process_stat->pgrp,                   // (5) pgrp  %d
        &process_stat->session,                // (6) session  %d
        &process_stat->tty_nr,                 // (7) tty_nr  %d
        &process_stat->tpgid,                  // (8) tpgid  %d
        &process_stat->flags,                  // (9) flags  %u
        &process_stat->minflt,                 // (10) minflt  %lu
        &process_stat->cminflt,                // (11) cminflt  %lu
        &process_stat->majflt,                 // (12) majflt  %lu
        &process_stat->cmajflt,                // (13) cmajflt  %lu
        &process_stat->utime,                  // (14) utime  %lu
        &process_stat->stime,                  // (15) stime  %lu
        &process_stat->cutime,                 // (16) cutime  %ld
        &process_stat->cstime,                 // (17) cstime  %ld
        &process_stat->priority,               // (18) priority  %ld
        &process_stat->nice,                   // (19) nice  %ld
        &process_stat->num_threads,            // (20) num_threads  %ld
        &process_stat->itrealvalue,            // (21) itrealvalue  %ld
        &process_stat->starttime,              // (22) starttime  %llu
        &process_stat->vsize,                  // (23) vsize  %lu
        &process_stat->rss,                    // (24) rss  %ld
        &process_stat->rsslim,                 // (25) rsslim  %lu
        &process_stat->startcode,              // (26) startcode  %lu  [PT]
        &process_stat->endcode,                // (27) endcode  %lu  [PT]
        &process_stat->startstack,             // (28) startstack  %lu  [PT]
        &process_stat->kstkesp,                // (29) kstkesp  %lu  [PT]
        &process_stat->kstkeip,                // (30) kstkeip  %lu  [PT]
        &process_stat->signal,                 // (31) signal  %lu
        &process_stat->blocked,                // (32) blocked  %lu
        &process_stat->sigignore,              // (33) sigignore  %lu
        &process_stat->sigcatch,               // (34) sigcatch  %lu
        &process_stat->wchan,                  // (35) wchan  %lu  [PT]
        &process_stat->nswap,                  // (36) nswap  %lu
        &process_stat->cnswap,                 // (37) cnswap  %lu
        &process_stat->exit_signal,            // (38) exit_signal  %d  (since Linux 2.1.22)
        &process_stat->processor,              // (39) processor  %d  (since Linux 2.2.8)
        &process_stat->rt_priority,            // (40) rt_priority  %u  (since Linux 2.5.19)
        &process_stat->policy,                 // (41) policy  %u  (since Linux 2.5.19)
        &process_stat->delayacct_blkio_ticks,  // (42) delayacct_blkio_ticks  %llu  (since Linux 2.6.18)
        &process_stat->guest_time,             // (43) guest_time  %lu  (since Linux 2.6.24)
        &process_stat->cguest_time,            // (44) cguest_time  %ld  (since Linux 2.6.24)
        &process_stat->start_data,             // (45) start_data  %lu  (since Linux 3.3)  [PT]
        &process_stat->end_data,               // (46) end_data  %lu  (since Linux 3.3)  [PT]
        &process_stat->start_brk,              // (47) start_brk  %lu  (since Linux 3.3)  [PT]
        &process_stat->arg_start,              // (48) arg_start  %lu  (since Linux 3.5)  [PT]
        &process_stat->arg_end,                // (49) arg_end  %lu  (since Linux 3.5)  [PT]
        &process_stat->env_start,              // (50) env_start  %lu  (since Linux 3.5)  [PT]
        &process_stat->env_end,                // (51) env_end  %lu  (since Linux 3.5)  [PT]
        &process_stat->exit_code               // (52) exit_code  %d  (since Linux 3.5)  [PT]
        );
    return 0;
}

void prometheus_process_linux_stat_init(prometheus_process_linux_stat_t process_stat) {
    bzero(process_stat, sizeof(*process_stat));
}

void prometheus_process_linux_stat_fini(prometheus_process_linux_stat_t process_stat) {
}

int prometheus_process_linux_stat_load(
    prometheus_process_provider_t provider, prometheus_process_linux_stat_t process_stat)
{
    mem_buffer_clear_data(&provider->m_data_buffer);

    ssize_t rv = vfs_file_load_to_buffer_by_path(
        &provider->m_data_buffer, provider->m_vfs_mgr, provider->m_stat_path);
    if (rv < 0) {
        CPE_ERROR(
            provider->m_em, "prometheus: process: load stat: open file %s fail, error=%d (%s)",
            provider->m_stat_path, errno, strerror(errno));
        return -1;
    }

    if (rv == 0) {
        CPE_ERROR(provider->m_em, "prometheus: process: load stat: file %s is empty", provider->m_stat_path);
        return -1;
    }

    mem_buffer_append_char(&provider->m_data_buffer, 0);

    return prometheus_process_linux_stat_read(
        provider, process_stat, mem_buffer_make_continuous(&provider->m_data_buffer, 0));
}
