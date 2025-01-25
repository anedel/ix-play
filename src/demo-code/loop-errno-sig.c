/*
 * demo-code/loop-errno-sig.c
 * 
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2014--2025 Alexandru Nedel
 *
 * (add your name here when you make significant changes to this file,
 *  if you want to)
 */

#include "loop-errno-sig.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>  /* for S_IRWXU */
#include <unistd.h>

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


static volatile sig_atomic_t  stop_sig = 0;
static volatile sig_atomic_t  act_sig = 0;
static volatile unsigned long  n_acts = 0;

unsigned long
get_n_acts (void)
{
    return n_acts;
}

static void
soft_stop_handler (int signo)
{
    stop_sig = signo;
}

static void
act_fail_handler_1arg (int signo)
{
    act_sig = signo;
    close(-1);  /* should set errno to EBADF */
    ++n_acts;
}

static void
act_fail_handler_3args (int signo, siginfo_t *info, void *other)
{
    act_sig = signo;
    close(-1);  /* should set errno to EBADF */
    ++n_acts;
}

void
test_close_ebadf (void)
{
    char  err_buf[128];
    int   res;

    const int      close_res = close(-1);
    const errno_t  close_err = errno;

    if (close_res != -1) {
        fprintf(stderr, "Unexpected return value %d from close(-1)\n", close_res);
        exit(4);
    }

    res = strerror_r(close_err, err_buf, sizeof err_buf);
    if (res != 0) {
        fprintf(stderr, "strerror_r(%d) failed, returning the errno value %d.\n",
                close_err, res);
        exit(98);
    }

    if (close_err != EBADF) {
        fprintf(stderr, "Unexpected errno %d from close(-1): %s\n",
                close_err, err_buf);
        exit(5);
    }

    fprintf(stdout, "Got the expected errno %d from close(-1): %s\n",
            close_err, err_buf);
}

void
loop_expecting_eacces (const char *message_preamble)
{
    char  err_buf[128];
    int   res;

    unsigned long  n_calls = 0;
    unsigned long  n_unexpected = 0;
    unsigned long  n_sig_detected = 0;
    sig_atomic_t  act_sig_copy;

    int      my_res;
    errno_t  my_err;

    while (stop_sig == 0) {
        /* Attempt to create dir in the root directory, as a normal user;
         * should fail with EACCES --- POSIX says:
         * "Search permission is denied on a component of the path prefix, or
         * write permission is denied on the parent directory of
         * the directory to be created."  (In our case we expect the latter.)
         */
        errno = 0;
        my_res = mkdir("/should-fail", S_IRWXU);
        my_err = errno;
        ++n_calls;

        if (my_res != -1) {
            fprintf(stderr, "%s [%lu calls, %lu signals] Unexpected return value %d from mkdir()\n",
                    message_preamble, n_calls, n_sig_detected, my_res);
            exit(9);
        }

        res = strerror_r(my_err, err_buf, sizeof err_buf);
        if (res != 0) {
            fprintf(stderr, "%s [%lu calls, %lu signals] strerror_r(%d) failed, returning the errno value %d.\n",
                    message_preamble, n_calls, n_sig_detected, my_err, res);
            exit(99);
        }

        if (my_err != EACCES) {
            ++n_unexpected;
            /* 'stdout', not 'stderr', because this is what we expect to happen occasionally */
            fprintf(stdout, "%s [%lu calls, %lu signals] Unexpected errno %d from mkdir(): %s\n",
                    message_preamble, n_calls, n_sig_detected, my_err, err_buf);
        }

        act_sig_copy = act_sig;
        act_sig = 0;

        if (act_sig_copy != 0) {
            ++n_sig_detected;
        }
    }

    printf("\n%s Stopped by signal %d after"
           "\n%s  %lu calls made,"
           "\n%s  %lu signals with interfering action detected,"
           "\n%s  %lu cases of unexpected errno value.\n",
           message_preamble, (int) stop_sig,
           message_preamble, n_calls,
           message_preamble, n_sig_detected,
           message_preamble, n_unexpected);
}


void
register_loop_err_sigactions (int sigaction_flags)
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
        act.sa_sigaction = &act_fail_handler_3args;
    } else {
        act.sa_handler = &act_fail_handler_1arg;
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
get_loop_err_sigset (sigset_t *out)
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
