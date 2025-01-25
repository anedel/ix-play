/*
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2014--2025 Alexandru Nedel
 *
 * (add your name here when you make significant changes to this file,
 *  if you want to)
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "loop-handling-sig.h"
#include "util-ex-threads.h"
#include "util-sigaction.h"

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


static double  cycle_time = 2.4;


static void *
waiting_signal_thread_func (void *arg)
{
    uex_thread_info *const tinfo = arg;

    loop_waiting_signal(tinfo->config_str, cycle_time);

    return tinfo;
}

static void *
sleeping_thread_func (void *arg)
{
    uex_thread_info *const tinfo = arg;

    loop_sleeping(tinfo->config_str, cycle_time);

    return tinfo;
}


/*
 * Handle Argument (usually coming from command-line interface).
 * Each argument should describe a thread to be created/started.
 * This function handles one argument, but it can be any of the legal arguments.
 * Intended to be called repeatedly until command-line arguments are exhausted.
 */
static int
handle_arg (const char *arg)
{
    int  pos;

    pos = uex_find_thread_config_by_prefix(arg, UEX_THREAD_CONFIG_MAX);
    if (pos >= 0) {
        fprintf(stderr, "Found thread config '%s' at %d\n", arg, pos);
        exit(6);
    }

    if (0 == strncmp("w", arg, 1)) { /* The prefix 'w' stands for "Waiting" */
        pos = uex_add_thread_config(arg, NULL, &waiting_signal_thread_func);
        if (pos < 0) {
            fprintf(stderr, "Could not add thread config '%s'\n", arg);
            exit(7);
        }
    }
    else if (0 == strncmp("s", arg, 1)) { /* The prefix 's' stands for "Sleeping" */
        pos = uex_add_thread_config(arg, NULL, &sleeping_thread_func);
        if (pos < 0) {
            fprintf(stderr, "Could not add thread config '%s'\n", arg);
            exit(8);
        }
    }
    else {
        return -1;
    }

    return 0;
}


static int
parse_sa_flags_str (const char *data)
{
    int  flags = 0;

    int  res = parse_sigaction_flags(&flags, data);

    if (res == 0) {
        return flags;
    } else {
        fprintf(stderr, "Bad sigaction flag(s) '%s'\n", data);
        exit(10);
    }
}

static double
parse_cycle_time (const char *data)
{
    char *end = NULL;  /* for strto...() functions */

    errno_t  strto_err;

    double  seconds;

    errno = 0;
    seconds = strtod(data, &end);
    strto_err = errno;

    if (end == data) {
        fprintf(stderr, "Could not parse cycle time '%s'\n",
                data);
        exit(11);
    }
    if (*end != '\0') {
        fprintf(stderr, "Unexpected text '%s' after cycle time %g\n",
                end, seconds);
        exit(12);
    }
    if (strto_err != 0) {
        fprintf(stderr, "Parsing cycle time '%s' failed with errno %d: %s\n",
                data, strto_err, strerror(strto_err));
        exit(13);
    }

    if (seconds < 0.0) {
        fprintf(stderr, "Cycle time must be positive or zero (got %g, original text was '%s')\n",
                seconds, data);
        exit(14);
    }

    return seconds;
}


static void
show_usage (FILE *out_stream)
{
    fprintf(out_stream, "Usage: [sa_flags=...]"
            " [cycle_time=<Seconds_with_decimals>]"
            " <Threads:one_or_many(w...|s...)>\n");

    fprintf(out_stream, "  The thread name prefix 'w' stands for \"Waiting\".\n");
    fprintf(out_stream, "  The thread name prefix 's' stands for \"Sleeping\".\n");

    show_all_sigaction_flags(out_stream);
}

int
main (int argc, char* argv[])
{
    sigset_t  main_sigset;

    int  sigact_flags = SA_RESTART;
    int  arg_pos = 1;
    int  res;
    const char *data;

    if (arg_pos < argc) {
        if (0 == strncmp("sa_flags=", argv[arg_pos], 9)) {
            data = argv[arg_pos] + 9;
            sigact_flags = parse_sa_flags_str(data);
            ++arg_pos;
        }
    }

    if (arg_pos < argc) {
        if (0 == strncmp("cycle_time=", argv[arg_pos], 11)) {
            data = argv[arg_pos] + 11;
            cycle_time = parse_cycle_time(data);
            ++arg_pos;
        }
    }

    for (; arg_pos < argc; ++arg_pos) {
        res = handle_arg(argv[arg_pos]);
        if (res != 0) {
            fprintf(stderr, "Unrecognized argument '%s' (error %d).\n",
                    argv[arg_pos], res);
            show_usage(stderr);
            return 2;
        }
    }

    printf("Pid = %ld\n", (long) getpid());
    printf("SIGRTMIN = %d, SIGRTMAX = %d\n", SIGRTMIN, SIGRTMAX);

    show_sigaction_flags(sigact_flags, stdout);

    register_loop_handlesig_sigactions(sigact_flags);

    get_loop_handlesig_sigset(&main_sigset);
    res = pthread_sigmask(SIG_BLOCK, &main_sigset, NULL);
    if (res != 0) {
        fprintf(stderr, "Could not block in main thread the signals that we plan to wait on: %d\n",
                res);
        return 3;
    }

    uex_start_threads();
    uex_join_threads();

    printf("\nThe signal handler executed %lu times.\n",
           get_n_handled_async());

    return 0;
}
