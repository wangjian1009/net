#ifndef PROMETHEUS_PROCESS_LINUX_STATS_I_H
#define PROMETHEUS_PROCESS_LINUX_STATS_I_H
#include "prometheus_gauge.h"
#include "prometheus_process_provider_i.h"

/**
 * @brief Refer to man proc and search for /proc/[pid]/stat
 */
struct prometheus_process_linux_stat {
    int pid;                                   // (1) pid  %d
    char comm[128];                            // (2) comm  %s
    char state;                                // (3) state  %c
    int ppid;                                  // (4) ppid  %d
    int pgrp;                                  // (5) pgrp  %d
    int session;                               // (6) session  %d
    int tty_nr;                                // (7) tty_nr  %d
    int tpgid;                                 // (8) tpgid  %d
    unsigned flags;                            // (9) flags  %u
    unsigned long minflt;                      // (10) minflt  %lu
    unsigned long cminflt;                     // (11) cminflt  %lu
    unsigned long majflt;                      // (12) majflt  %lu
    unsigned long cmajflt;                     // (13) cmajflt  %lu
    unsigned long utime;                       // (14) utime  %lu
    unsigned long stime;                       // (15) stime  %lu
    long int cutime;                           // (16) cutime  %ld
    long int cstime;                           // (17) cstime  %ld
    long int priority;                         // (18) priority  %ld
    long int nice;                             // (19) nice  %ld
    long int num_threads;                      // (20) num_threads  %ld
    long int itrealvalue;                      // (21) itrealvalue  %ld
    unsigned long long starttime;              // (22) starttime  %llu
    unsigned long vsize;                       // (23) vsize  %lu
    long int rss;                              // (24) rss  %ld
    unsigned long rsslim;                      // (25) rsslim  %lu
    unsigned long startcode;                   // (26) startcode  %lu  [PT]
    unsigned long endcode;                     // (27) endcode  %lu  [PT]
    unsigned long startstack;                  // (28) startstack  %lu  [PT]
    unsigned long kstkesp;                     // (29) kstkesp  %lu  [PT]
    unsigned long kstkeip;                     // (30) kstkeip  %lu  [PT]
    unsigned long signal;                      // (31) signal  %lu
    unsigned long blocked;                     // (32) blocked  %lu
    unsigned long sigignore;                   // (33) sigignore  %lu
    unsigned long sigcatch;                    // (34) sigcatch  %lu
    unsigned long wchan;                       // (35) wchan  %lu  [PT]
    unsigned long nswap;                       // (36) nswap  %lu
    unsigned long cnswap;                      // (37) cnswap  %lu
    int exit_signal;                           // (38) exit_signal  %d  (since Linux 2.1.22)
    int processor;                             // (39) processor  %d  (since Linux 2.2.8)
    unsigned rt_priority;                      // (40) rt_priority  %u  (since Linux 2.5.19)
    unsigned policy;                           // (41) policy  %u  (since Linux 2.5.19)
    unsigned long long delayacct_blkio_ticks;  // (42) delayacct_blkio_ticks
    unsigned long guest_time;                  // (43) guest_time  %lu  (since Linux 2.6.24)
    long int cguest_time;                      // (44) cguest_time  %ld  (since Linux 2.6.24)
    unsigned long start_data;                  // (45) start_data  %lu  (since Linux 3.3)  [PT]
    unsigned long end_data;                    // (46) end_data  %lu  (since Linux 3.3)  [PT]
    unsigned long start_brk;                   // (47) start_brk  %lu  (since Linux 3.3)  [PT]
    unsigned long arg_start;                   // (48) arg_start  %lu  (since Linux 3.5)  [PT]
    unsigned long arg_end;                     // (49) arg_end  %lu  (since Linux 3.5)  [PT]
    unsigned long env_start;                   // (50) env_start  %lu  (since Linux 3.5)  [PT]
    unsigned long env_end;                     // (51) env_end  %lu  (since Linux 3.5)  [PT]
    int exit_code;                             // (52) exit_code  %d  (since Linux 3.5)  [PT]
};

void prometheus_process_linux_stat_init(prometheus_process_linux_stat_t process_stat);
void prometheus_process_linux_stat_fini(prometheus_process_linux_stat_t process_stat);

int prometheus_process_linux_stat_read(
    prometheus_process_provider_t provider,
    prometheus_process_linux_stat_t process_stat, const char * data);

int prometheus_process_linux_stat_load(
    prometheus_process_provider_t provider,
    prometheus_process_linux_stat_t process_stat);

#endif
