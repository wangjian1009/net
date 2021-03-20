#include "cpe/pal/pal_stdio.h"
#include "cpe/pal/pal_unistd.h"
#include "prometheus_process_procfs_i.h"

int prometheus_process_stat_read(prometheus_process_stat_t process_stat, const char * data) {
    self->comm = prometheus_malloc(128);

    sscanf(
        (const char *)stat_f->buf,
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
        &self->pid,                    // (1) pid  %d
        self->comm,                    // (2) comm  %s
        &self->state,                  // (3) state  %c
        &self->ppid,                   // (4) ppid  %d
        &self->pgrp,                   // (5) pgrp  %d
        &self->session,                // (6) session  %d
        &self->tty_nr,                 // (7) tty_nr  %d
        &self->tpgid,                  // (8) tpgid  %d
        &self->flags,                  // (9) flags  %u
        &self->minflt,                 // (10) minflt  %lu
        &self->cminflt,                // (11) cminflt  %lu
        &self->majflt,                 // (12) majflt  %lu
        &self->cmajflt,                // (13) cmajflt  %lu
        &self->utime,                  // (14) utime  %lu
        &self->stime,                  // (15) stime  %lu
        &self->cutime,                 // (16) cutime  %ld
        &self->cstime,                 // (17) cstime  %ld
        &self->priority,               // (18) priority  %ld
        &self->nice,                   // (19) nice  %ld
        &self->num_threads,            // (20) num_threads  %ld
        &self->itrealvalue,            // (21) itrealvalue  %ld
        &self->starttime,              // (22) starttime  %llu
        &self->vsize,                  // (23) vsize  %lu
        &self->rss,                    // (24) rss  %ld
        &self->rsslim,                 // (25) rsslim  %lu
        &self->startcode,              // (26) startcode  %lu  [PT]
        &self->endcode,                // (27) endcode  %lu  [PT]
        &self->startstack,             // (28) startstack  %lu  [PT]
        &self->kstkesp,                // (29) kstkesp  %lu  [PT]
        &self->kstkeip,                // (30) kstkeip  %lu  [PT]
        &self->signal,                 // (31) signal  %lu
        &self->blocked,                // (32) blocked  %lu
        &self->sigignore,              // (33) sigignore  %lu
        &self->sigcatch,               // (34) sigcatch  %lu
        &self->wchan,                  // (35) wchan  %lu  [PT]
        &self->nswap,                  // (36) nswap  %lu
        &self->cnswap,                 // (37) cnswap  %lu
        &self->exit_signal,            // (38) exit_signal  %d  (since Linux 2.1.22)
        &self->processor,              // (39) processor  %d  (since Linux 2.2.8)
        &self->rt_priority,            // (40) rt_priority  %u  (since Linux 2.5.19)
        &self->policy,                 // (41) policy  %u  (since Linux 2.5.19)
        &self->delayacct_blkio_ticks,  // (42) delayacct_blkio_ticks  %llu  (since Linux 2.6.18)
        &self->guest_time,             // (43) guest_time  %lu  (since Linux 2.6.24)
        &self->cguest_time,            // (44) cguest_time  %ld  (since Linux 2.6.24)
        &self->start_data,             // (45) start_data  %lu  (since Linux 3.3)  [PT]
        &self->end_data,               // (46) end_data  %lu  (since Linux 3.3)  [PT]
        &self->start_brk,              // (47) start_brk  %lu  (since Linux 3.3)  [PT]
        &self->arg_start,              // (48) arg_start  %lu  (since Linux 3.5)  [PT]
        &self->arg_end,                // (49) arg_end  %lu  (since Linux 3.5)  [PT]
        &self->env_start,              // (50) env_start  %lu  (since Linux 3.5)  [PT]
        &self->env_end,                // (51) env_end  %lu  (since Linux 3.5)  [PT]
        &self->exit_code               // (52) exit_code  %d  (since Linux 3.5)  [PT]
        );
    return self;
}

void prometheus_process_stat_free(prometheus_process_stat_t stat) {
    /* mem_free((void *)self->comm); */
    /* prometheus_free((void *)self); */
    /* self = NULL; */
}
