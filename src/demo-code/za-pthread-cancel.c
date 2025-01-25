/*
 * demo-code/za-pthread-cancel.c
 *
 * Create a single thread and try to cancel it.
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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

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
show_all_thread_cancellation_options_ (FILE *out_stream)
{
    fprintf(out_stream,
        "Cancellation state: Disabled or Enabled; None (use default, don't set)\n"
        "Cancellation type: Async or Deferred mode; None (use default, don't set)\n"
        "Cancellation request: 0 (don't send) or 1 (send)\n");
}

static char  cancelstate_chr = 'n';  /* option 'None' = Don't set cancellation state */
static char  canceltype_chr  = 'n';  /* option 'None' = Don't set cancellation type */
static char  cancel_request_chr = '1';

static void
apply_thread_cancellation_settings_ (void)
{
    switch (cancelstate_chr)
    {
    case 'd':
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        printf("Cancellation state: Disabled\n");
        break;
    case 'e':
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        printf("Cancellation state: Enabled\n");
        break;
    case 'n':  /* option 'None' = Don't set cancellation state */
        printf("Cancellation state: default (option=None); not calling pthread_setcancelstate()\n");
        break;
    default:
        printf("Unrecognized 'state' option, ignored: '%c'; not calling pthread_setcancelstate()\n",
               cancelstate_chr);
        break;
    }

    switch (canceltype_chr)
    {
    case 'a':
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
        printf("Cancellation type: Async mode\n");
        break;
    case 'd':
        pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
        printf("Cancellation type: Deferred mode\n");
        break;
    case 'n':  /* option 'None' = Don't set cancellation type */
        printf("Cancellation type: default (option=None); not calling pthread_setcanceltype()\n");
        break;
    default:
        printf("Unrecognized 'type' option, ignored: '%c'; not calling pthread_setcanceltype()\n",
               canceltype_chr);
        break;
    }
}


static double  cycle_time_s = 2.9999999999;
    /* Should be converted to exactly 3 (three) seconds.
     *
     * I chose a number with many nines to test the conversion.
     *
     * To get an integer number of seconds when converting to struct timeval
     * there must be at least six nines, followed by some non-zero digits;
     * if converting to 'struct timespec' at least nine nines are needed.
     */

static unsigned long  n_max_cycles = 5;


static void *
sleeper_thread_func (void *arg)
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

    unsigned long  n_cycles = 0;
    unsigned long  n_intr = 0;
    unsigned long  n_fail = 0;

    int      sel_res;
    errno_t  sel_err;

    apply_thread_cancellation_settings_();

    fill_timeval_from_double(&cycle_tval, cycle_time_s);

    fprintf(stdout, "\nCycle time: ");
    show_timeval(&cycle_tval, stdout);
    fprintf(stdout, ".\n");

    while (n_cycles < n_max_cycles) {
        tval = cycle_tval;
        errno = 0;
        sel_res = select(0, NULL, NULL, NULL, &tval);  /* sleep */
        sel_err = errno;
        ++n_cycles;

        if (sel_res == 0) {  /* timeout */
            printf("    [cycle %lu / %lu: OK]\n", n_cycles, n_max_cycles);
        } else {
            assert(sel_res == -1);

            res = strerror_r(sel_err, err_buf, sizeof err_buf);
            if (res != 0) {
                fprintf(stderr, "[%lu cycles: %lu intr, %lu fail]"
                        " strerror_r(%d) failed, returning the errno value %d.\n",
                        n_cycles, n_intr, n_fail, sel_err, res);
                exit(99);
            }

            if (EINTR == sel_err) {
                ++n_intr;
                fprintf(stderr, "[%lu cycles: %lu intr, %lu fail]"
                        " select() interrupted: errno %d = %s\n",
                        n_cycles, n_intr, n_fail, sel_err, err_buf);
            } else if (EINVAL == sel_err) {
                /*
                 * Given the way we call select() here (no file descriptors),
                 * the only possible reason for EINVAL would be that
                 * an invalid timeout interval was specified.
                 * There is no hope that retrying could give a different result:
                 * the timeval struct specifying the timeout would not change.
                 * Therefore we exit immediately:
                 */
                fprintf(stderr, "[%lu cycles: %lu intr, %lu fail]"
                        " invalid timeout interval for select(): errno %d = %s\n",
                        n_cycles, n_intr, n_fail, sel_err, err_buf);
                exit(90);
            } else {
                ++n_fail;
                fprintf(stderr, "[%lu cycles: %lu intr, %lu fail]"
                        " Unexpected errno %d from select(): %s\n",
                        n_cycles, n_intr, n_fail, sel_err, err_buf);
            }
        }
    }

    printf("\nSleeping loop finished after"
           "\n  %lu cycles,"
           "\n  %lu times select() was unexpectedly interrupted,"
           "\n  %lu failures.\n",
           n_cycles, n_intr, n_fail);
    fflush(stdout);

    return NULL;
}


static void
cancel_thread_ (pthread_t thread_id)
{
    char  err_buf[128];
    int   res;
    int   tcancel_res;

    tcancel_res = pthread_cancel(thread_id);
    if (tcancel_res != 0) {
        res = strerror_r(tcancel_res, err_buf, sizeof err_buf);
        if (res != 0) {
            fprintf(stderr, "strerror_r(%d) failed, returning the errno value %d.\n",
                    tcancel_res, res);
            exit(98);
        }

        printf("pthread_cancel() failed, returning the errno value %d = %s\n",
               tcancel_res, err_buf);
        exit(12);
    }

    printf("Cancellation request sent.\n");
}


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

    size_t  len;

    if (0 == strncmp("state:", arg, 6)) {
        data = arg + 6;

        len = strlen(data);
        if (len != 1) {
            fprintf(stderr, "Cancellation state should be one char (got '%s')\n",
                    data);
            exit(3);
        }

        cancelstate_chr = data[0];
    }
    else if (0 == strncmp("type:", arg, 5)) {
        data = arg + 5;

        len = strlen(data);
        if (len != 1) {
            fprintf(stderr, "Cancellation type should be one char (got '%s')\n",
                    data);
            exit(4);
        }

        canceltype_chr = data[0];
    }
    else if (0 == strncmp("req:", arg, 4)) {
        data = arg + 4;

        len = strlen(data);
        if (len != 1) {
            fprintf(stderr, "Cancellation request option should be one char (got '%s')\n",
                    data);
            exit(5);
        }

        cancel_request_chr = data[0];
    }
    else {
        return -1;
    }

    return 0;
}


static void
show_usage (FILE *out_stream)
{
    fprintf(out_stream, "Usage: [state:d|e|n] [type:a|d|n] [req:0|1]\n");
    show_all_thread_cancellation_options_(out_stream);
}

int
main (int argc, char* argv[])
{
    pthread_t  thread_id;

    char  err_buf[128];
    int   res;

    int   tcreate_res;

    int   tjoin_res;
    void *thr_retval = NULL;

    int  arg_pos;

    for (arg_pos = 1; arg_pos < argc; ++arg_pos) {
        res = handle_arg_(argv[arg_pos]);
        if (res != 0) {
            fprintf(stderr, "Unrecognized argument '%s' (error %d).\n",
                    argv[arg_pos], res);
            show_usage(stderr);
            return 2;
        }
    }

    tcreate_res = pthread_create(
                    &thread_id,
                    NULL,
                    &sleeper_thread_func,
                    NULL);
    if (tcreate_res != 0) {
        res = strerror_r(tcreate_res, err_buf, sizeof err_buf);
        if (res != 0) {
            fprintf(stderr, "strerror_r(%d) failed, returning the errno value %d.\n",
                    tcreate_res, res);
            exit(99);
        }

        printf("pthread_create() failed, returning the errno value %d = %s\n",
               tcreate_res, err_buf);
        exit(11);
    }

    printf("Thread started.\n");

    sleep(4);

    if (cancel_request_chr == '1') {
        cancel_thread_(thread_id);
    }

    printf("Waiting for thread termination (join)...\n");

    tjoin_res = pthread_join(thread_id, &thr_retval);

    if (tjoin_res == 0) {
        if (PTHREAD_CANCELED == thr_retval) {
            printf("\npthread_join(): PTHREAD_CANCELED\n");
        } else {
            printf("\npthread_join(): Normal exit\n");
        }
    } else {
        res = strerror_r(tjoin_res, err_buf, sizeof err_buf);
        if (res != 0) {
            fprintf(stderr, "strerror_r(%d) failed, returning the errno value %d.\n",
                    tjoin_res, res);
            exit(99);
        }

        printf("\npthread_join() failed, returning the errno value %d = %s\n",
               tjoin_res, err_buf);
    }

    return 0;
}
