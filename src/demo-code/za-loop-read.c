/*
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2014--2025 Alexandru Nedel
 *
 * (add your name here when you make significant changes to this file,
 *  if you want to)
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>  /* for 'UINT_MAX' */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "util-ofd-flags.h"
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


static void
alarm_sig_handler (int signo)
{
    /* do nothing, just interrupt the read() */
}


static unsigned long  num_select_intr = 0;
static unsigned long  num_select_fail = 0;

static void
delay_ (const struct timeval *delay_tval)
{
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

    int      sel_res;
    errno_t  sel_err;

    tval = *delay_tval;
    errno = 0;
    sel_res = select(0, NULL, NULL, NULL, &tval);  /* sleep */
    sel_err = errno;

    if (sel_res == 0) {  /* timeout */
        /* TODO: maybe print a progress message (one dot per cycle?) */
    } else {
        assert(sel_res == -1);

        res = strerror_r(sel_err, err_buf, sizeof err_buf);
        if (res != 0) {
            fprintf(stderr, "[%lu intr, %lu fail]"
                    " strerror_r(%d) failed, returning the errno value %d.\n",
                    num_select_intr, num_select_fail, sel_err, res);
            exit(99);
        }

        if (EINTR == sel_err) {
            ++num_select_intr;
            fprintf(stderr, "[%lu intr, %lu fail]"
                    " select() interrupted: errno %d = %s\n",
                    num_select_intr, num_select_fail, sel_err, err_buf);
        } else if (EINVAL == sel_err) {
            /*
             * Given the way we call select() here (no file descriptors),
             * the only possible reason for EINVAL would be that
             * an invalid timeout interval was specified.
             * There is no hope that retrying could give a different result:
             * the timeval struct specifying the timeout would not change.
             * Therefore we exit immediately:
             */
            fprintf(stderr, "[%lu intr, %lu fail]"
                    " invalid timeout interval for select(): errno %d = %s\n",
                    num_select_intr, num_select_fail, sel_err, err_buf);
            exit(90);
        } else {
            ++num_select_fail;
            fprintf(stderr, "[%lu intr, %lu fail]"
                    " Unexpected errno %d from select(): %s\n",
                    num_select_intr, num_select_fail, sel_err, err_buf);
        }
    }
}


static unsigned
parse_uint_ (const char *const data)
{
    char *end = NULL;  /* for strto...() functions */

    errno_t  strto_err;

    unsigned long  ul_val;

    errno = 0;
    ul_val = strtoul(data, &end, 10);
    strto_err = errno;

    if (end == data) {
        fprintf(stderr, "Could not parse unsigned value '%s'\n",
                data);
        exit(11);
    }
    if (*end != '\0') {
        fprintf(stderr, "Unexpected text '%s' after unsigned value %lu\n",
                end, ul_val);
        exit(12);
    }
    if (strto_err != 0) {
        fprintf(stderr, "Parsing unsigned value '%s' failed with errno %d: %s\n",
                data, strto_err, strerror(strto_err));
        exit(13);
    }

    if (ul_val >= UINT_MAX) {
        fprintf(stderr, "Unsigned value too big: %lu > UINT_MAX = %u, original text was '%s'\n",
                ul_val, UINT_MAX, data);
        exit(14);
    }

    return (unsigned) ul_val;
}

static double
parse_delay_ (const char *const data)
{
    char *end = NULL;  /* for strto...() functions */

    errno_t  strto_err;

    double  seconds;

    errno = 0;
    seconds = strtod(data, &end);
    strto_err = errno;

    if (end == data) {
        fprintf(stderr, "Could not parse delay '%s'\n",
                data);
        exit(21);
    }
    if (*end != '\0') {
        fprintf(stderr, "Unexpected text '%s' after delay %g\n",
                end, seconds);
        exit(22);
    }
    if (strto_err != 0) {
        fprintf(stderr, "Parsing delay '%s' failed with errno %d: %s\n",
                data, strto_err, strerror(strto_err));
        exit(23);
    }

    if (seconds < 0.0) {
        fprintf(stderr, "Delay must be positive or zero (got %g, original text was '%s')\n",
                seconds, data);
        exit(24);
    }

    return seconds;
}


/* Zero means "no alarm": when calling alarm() with seconds = 0,
 * a pending alarm request, if any, is canceled.
 */
static unsigned  alarm_s = 0;

static double  delay_s = 2.4;

/*
 * Handle Argument (usually coming from command-line interface).
 * This function handles one argument, but it can be any of the legal arguments.
 * Intended to be called repeatedly until command-line arguments are exhausted.
 * Same type of argument/option can appear multiple times; then
 * its previous values will be discarded/overriden =
 * the last occurence that is valid/complete/usable will take effect
 * (as if it was the only one of its kind).
 */
static int
handle_arg_ (const char *arg)
{
    const char *data;

    if (0 == strncmp("alarm:", arg, 6)) {
        data = arg + 6;
        alarm_s = parse_uint_(data);
    }
    else if (0 == strncmp("delay:", arg, 6)) {
        data = arg + 6;
        delay_s = parse_delay_(data);
    }
    else if (0 == strcmp("nonblocking", arg)) {
        set_ofd_status_flags(STDIN_FILENO, O_NONBLOCK);
    }
    else {
        return -1;
    }

    return 0;
}


static void
show_usage (FILE *out_stream)
{
    fprintf(out_stream, "Usage: [alarm:<Seconds>] [delay:<Seconds_with_decimals>] [nonblocking]\n");
}

int
main (int argc, char* argv[])
{
    struct sigaction  act;

    struct timeval  delay_tval;

    char  buf[1024];

    int      num_read;
    errno_t  read_err;

    unsigned  alarm_rem;  /* time Remaining */

    int  arg_pos;
    int  res;

    for (arg_pos = 1; arg_pos < argc; ++arg_pos) {
        res = handle_arg_(argv[arg_pos]);
        if (res != 0) {
            fprintf(stderr, "Unrecognized argument '%s'.\n",
                    argv[arg_pos]);
            show_usage(stderr);
            return 2;
        }
    }

    /* We must override the default disposition for SIGALRM,
     * which is to terminate the process (therefore our read loop).
     *
     * But 'signal(SIGALRM, SIG_IGN)' does not work as expected:
     * the read() syscall would not be interrupted by SIGALRM anymore!
     *
     * To get the desired behavior (time out, but no process termination),
     * we must register a signal handler (could just do nothing):
     */
    memset(&act, 0, sizeof act);
    sigemptyset(&act.sa_mask);
    act.sa_handler = &alarm_sig_handler;
    act.sa_flags = 0;
    if (sigaction(SIGALRM, &act, NULL) < 0) {
        perror("sigaction(SIGALRM)");
        return 5;
    }

    fill_timeval_from_double(&delay_tval, delay_s);

    printf("Pid = %ld\n", (long) getpid());

    fprintf(stdout, "Delay: ");
    show_timeval(&delay_tval, stdout);
    fprintf(stdout, ".\n");

    while (1) {
        alarm_rem = alarm(alarm_s);
        assert(0 == alarm_rem);
            /* There should not be a previous alarm() request pending. */

        if (alarm_s > 0) {
            printf("Reading from fd %d with %u seconds timeout...\n",
                   STDIN_FILENO, alarm_s);
        } else {
            printf("Reading from fd %d...\n",
                   STDIN_FILENO);
        }

        num_read = read(STDIN_FILENO, buf, sizeof buf);
        read_err = errno;

        alarm_rem = alarm(0);  /* Cancel the pending alarm request, if any. */

        printf("read(STDIN_FILENO, buf, %zu) returned %d, errno %d = %s\n",
               sizeof buf, num_read, read_err, strerror(read_err));

        if (alarm_s > 0) {
            printf("Canceled alarm: %u seconds remaining out of %u requested.\n",
                   alarm_rem, alarm_s);
        } else {
            assert(0 == alarm_rem);
        }

        /* Do not print if delay is less than two seconds,
         * would just fill the screen with useless messages,
         * making the useful info harder to read.
         */
        if (delay_tval.tv_sec > 1) {
            printf("Sleeping %g seconds...\n", delay_s);
        }
        delay_(&delay_tval);
    }

    return 0;
}
