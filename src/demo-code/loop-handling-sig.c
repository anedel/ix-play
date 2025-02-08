/*
 * demo-code/loop-handling-sig.c
 * 
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2014--2025 Alexandru Nedel
 *
 * (add your name here when you make significant changes to this file,
 *  if you want to)
 */

#include "loop-handling-sig.h"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "util-timespec.h"
#include "util-timeval.h"

/*
 * https://www.securecoding.cert.org/confluence/display/seccode/DCL09-C.+Declare+functions+that+return+errno+with+a+return+type+of+errno_t
 *
 * C11 Annex K introduced the new type 'errno_t' that
 * is defined to be type 'int' in 'errno.h' and elsewhere.
 * Many of the functions defined in C11 Annex K return values of this type.
 * The 'errno_t' type should be used as the type of an object that
 * may contain _only_ values that might be found in 'errno'.
 */
#ifndef __STDC_LIB_EXT1__
    typedef  int  errno_t;
#endif


static int  want_compact_info = 0;


static volatile sig_atomic_t  stop_sig = 0;
static volatile sig_atomic_t  act_sig = 0;
static volatile unsigned long  num_handled_async = 0;

unsigned long
get_num_handled_async (void)
{
    return num_handled_async;
}

static void
soft_stop_handler (int signo)
{
    stop_sig = signo;
}

static void
act_handler_1arg (int signo)
{
    act_sig = signo;
    ++num_handled_async;
}

static void
act_handler_3args (int signo, siginfo_t *info, void *other)
{
    act_sig = signo;
    ++num_handled_async;
}


static void
show_siginfo (const char *message_preamble, const siginfo_t *siginfo)
{
    printf("%s   si_signo=%d, si_code=%d, si_errno=%d;\n"
           "%s   Sending process: si_pid=%d, si_uid=%d;\n"
           "%s   si_status=%d, si_value.sival_int=%d.\n",
           message_preamble,
           siginfo->si_signo, siginfo->si_code, siginfo->si_errno,
           message_preamble, siginfo->si_pid, siginfo->si_uid,
           message_preamble, siginfo->si_status, siginfo->si_value.sival_int);
}

void
loop_waiting_signal (const char *message_preamble, double cycle_time_s)
{
    struct timespec  cycle_tspec;

    sigset_t   sigset;
    siginfo_t  siginfo;

    char  err_buf[128];
    int   res;

    unsigned long  num_cycles = 0;
    unsigned long  num_sync = 0;  /* Number of signals handled Synchronously */
    unsigned long  num_intr = 0;
    unsigned long  num_fail = 0;

    int      swait_res;
    errno_t  swait_err;

    get_loop_handlesig_sigset(&sigset);

    fill_timespec_from_double(&cycle_tspec, cycle_time_s);

    fprintf(stdout, "%s Cycle time: ", message_preamble);
    show_timespec(&cycle_tspec, stdout);
    fprintf(stdout, ".\n");

    while (stop_sig == 0) {
        errno = 0;
        swait_res = sigtimedwait(&sigset, &siginfo, &cycle_tspec);
        swait_err = errno;
        ++num_cycles;

        if (swait_res > 0) {
            ++num_sync;

            printf("%s [%lu cycles: %lu sync, %lu intr, %lu fail]"
                   " Synchronously handling signal %d:",
                   message_preamble, num_cycles, num_sync, num_intr, num_fail,
                   swait_res);

            if (want_compact_info) {
                printf(" sival_int = %d\n",
                       siginfo.si_value.sival_int);
            } else {
                printf("\n");
                show_siginfo(message_preamble, &siginfo);
                want_compact_info = 1;
            }
        } else {
            assert(swait_res == -1);

            if (EAGAIN == swait_err) {  /* timeout */
                /* TODO: maybe print a progress message (one dot per cycle?) */
            } else if (EINTR == swait_err) {
                if (stop_sig != 0) {
                    fprintf(stderr, "%s [%lu cycles: %lu sync, %lu intr, %lu fail]"
                            " sigtimedwait() interrupted (probably signal %d): errno %d.\n",
                            message_preamble, num_cycles, num_sync, num_intr, num_fail,
                            stop_sig, swait_err);
                } else {
                    ++num_intr;
                    fprintf(stderr, "%s [%lu cycles: %lu sync, %lu intr, %lu fail]"
                            " sigtimedwait() unexpectedly interrupted: errno %d.\n",
                            message_preamble, num_cycles, num_sync, num_intr, num_fail,
                            swait_err);
                }
            } else if (EINVAL == swait_err) {
                /*
                 * Per POSIX, the only possible reason for EINVAL would be that
                 * an invalid timeout interval was specified:
                 * "The timeout argument specified a tv_nsec value
                 * less than zero or greater than or equal to 1000 million."
                 *
                 * There is no hope that retrying could give a different result:
                 * the timespec struct specifying the timeout would not change.
                 * Therefore we exit immediately:
                 */
                fprintf(stderr, "%s [%lu cycles: %lu sync, %lu intr, %lu fail]"
                        " invalid timeout interval for sigtimedwait(): errno %d.\n",
                        message_preamble, num_cycles, num_sync, num_intr, num_fail,
                        swait_err);
                exit(91);
            } else {
                ++num_fail;

                res = strerror_r(swait_err, err_buf, sizeof err_buf);
                if (res != 0) {
                    fprintf(stderr, "%s [%lu cycles: %lu sync, %lu intr, %lu fail]"
                            " strerror_r(%d) failed, returning the errno value %d.\n",
                            message_preamble, num_cycles, num_sync, num_intr, num_fail,
                            swait_err, res);
                    exit(98);
                }

                fprintf(stderr, "%s [%lu cycles: %lu sync, %lu intr, %lu fail]"
                        " Unexpected errno %d from sigtimedwait(): %s\n",
                        message_preamble, num_cycles, num_sync, num_intr, num_fail,
                        swait_err, err_buf);
            }
        }
    }

    printf("\n%s Waiting loop stopped by signal %d after"
           "\n%s  %lu cycles,"
           "\n%s  %lu signals handled synchronously,"
           "\n%s  %lu times sigtimedwait() was unexpectedly interrupted,"
           "\n%s  %lu failures.\n",
           message_preamble, (int) stop_sig,
           message_preamble, num_cycles,
           message_preamble, num_sync,
           message_preamble, num_intr,
           message_preamble, num_fail);
}


void
loop_sleeping (const char *message_preamble, double cycle_time_s)
{
    struct timeval  cycle_tval;

    struct timeval  tval;
        /* Must be set to desired delay before each call to select(), because
         * select() may modify the object pointed to by the timeout argument.
         *
         * The Linux implementation of select() modifies the timeval struct
         * to reflect the amount of time that was not slept.
         * Most other implementations leave the timeout parameter unmodified:
         * for example, the BSD implementation of select().
         */

    char  err_buf[128];
    int   res;

    unsigned long  num_cycles = 0;
    unsigned long  num_intr = 0;
    unsigned long  num_fail = 0;

    int      sel_res;
    errno_t  sel_err;

    fill_timeval_from_double(&cycle_tval, cycle_time_s);

    fprintf(stdout, "%s Cycle time: ", message_preamble);
    show_timeval(&cycle_tval, stdout);
    fprintf(stdout, ".\n");

    while (stop_sig == 0) {
        tval = cycle_tval;
        errno = 0;
        sel_res = select(0, NULL, NULL, NULL, &tval);  /* sleep */
        sel_err = errno;
        ++num_cycles;

        if (sel_res == 0) {  /* timeout */
            /* TODO: maybe print a progress message (one dot per cycle?) */
        } else {
            assert(sel_res == -1);

            res = strerror_r(sel_err, err_buf, sizeof err_buf);
            if (res != 0) {
                fprintf(stderr, "%s [%lu cycles: %lu intr, %lu fail]"
                        " strerror_r(%d) failed, returning the errno value %d.\n",
                        message_preamble, num_cycles, num_intr, num_fail, sel_err, res);
                exit(99);
            }

            if (EINTR == sel_err) {
                if (stop_sig != 0) {
                    fprintf(stderr, "%s [%lu cycles: %lu intr, %lu fail]"
                            " select() interrupted (probably signal %d): errno %d = %s\n",
                            message_preamble, num_cycles, num_intr, num_fail,
                            stop_sig, sel_err, err_buf);
                } else {
                    ++num_intr;
                    fprintf(stderr, "%s [%lu cycles: %lu intr, %lu fail]"
                            " select() unexpectedly interrupted: errno %d = %s\n",
                            message_preamble, num_cycles, num_intr, num_fail,
                            sel_err, err_buf);
                }
            } else if (EINVAL == sel_err) {
                /*
                 * Given the way we call select() here (no file descriptors),
                 * the only possible reason for EINVAL would be that
                 * an invalid timeout interval was specified.
                 * There is no hope that retrying could give a different result:
                 * the timeval struct specifying the timeout would not change.
                 * Therefore we exit immediately:
                 */
                fprintf(stderr, "%s [%lu cycles: %lu intr, %lu fail]"
                        " invalid timeout interval for select(): errno %d = %s\n",
                        message_preamble, num_cycles, num_intr, num_fail, sel_err, err_buf);
                exit(90);
            } else {
                ++num_fail;
                fprintf(stderr, "%s [%lu cycles: %lu intr, %lu fail]"
                        " Unexpected errno %d from select(): %s\n",
                        message_preamble, num_cycles, num_intr, num_fail, sel_err, err_buf);
            }
        }
    }

    printf("\n%s Sleeping loop stopped by signal %d after"
           "\n%s  %lu cycles,"
           "\n%s  %lu times select() was unexpectedly interrupted,"
           "\n%s  %lu failures.\n",
           message_preamble, (int) stop_sig,
           message_preamble, num_cycles,
           message_preamble, num_intr,
           message_preamble, num_fail);
}


void
register_loop_handlesig_sigactions (int sigaction_flags)
{
    struct sigaction  act;

    memset(&act, 0, sizeof act);

    act.sa_handler = &soft_stop_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = sigaction_flags & ~SA_SIGINFO;

    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction(SIGINT)");
        exit(11);
    }
    if (sigaction(SIGRTMIN+1, &act, NULL) < 0) {
        perror("sigaction(SIGRTMIN+1)");
        exit(12);
    }
    if (sigaction(SIGRTMAX-1, &act, NULL) < 0) {
        perror("sigaction(SIGRTMAX-1)");
        exit(13);
    }

    if (sigaction_flags & SA_SIGINFO) {
        act.sa_sigaction = &act_handler_3args;
    } else {
        act.sa_handler = &act_handler_1arg;
    }

    sigemptyset(&act.sa_mask);
    act.sa_flags = sigaction_flags;

    if (sigaction(SIGUSR1, &act, NULL) < 0) {
        perror("sigaction(SIGUSR1)");
        exit(21);
    }
    if (sigaction(SIGUSR2, &act, NULL) < 0) {
        perror("sigaction(SIGUSR2)");
        exit(22);
    }
    if (sigaction(SIGRTMIN, &act, NULL) < 0) {
        perror("sigaction(SIGRTMIN)");
        exit(23);
    }
    if (sigaction(SIGRTMIN+2, &act, NULL) < 0) {
        perror("sigaction(SIGRTMIN+2)");
        exit(24);
    }
    if (sigaction(SIGRTMAX-2, &act, NULL) < 0) {
        perror("sigaction(SIGRTMAX-2)");
        exit(25);
    }
    if (sigaction(SIGRTMAX, &act, NULL) < 0) {
        perror("sigaction(SIGRTMAX)");
        exit(26);
    }
}

void
get_loop_handlesig_sigset (sigset_t *out)
{
    sigemptyset(out);

    /* DO NOT add: SIGINT --- Do not include the soft stop signals! */
    sigaddset(out, SIGUSR1);
    sigaddset(out, SIGUSR2);

    sigaddset(out, SIGRTMIN);
    /* DO NOT add: SIGRTMIN+1 --- Do not include the soft stop signals! */
    sigaddset(out, SIGRTMIN+2);

    sigaddset(out, SIGRTMAX-2);
    /* DO NOT add: SIGRTMAX-1 --- Do not include the soft stop signals! */
    sigaddset(out, SIGRTMAX);
}
