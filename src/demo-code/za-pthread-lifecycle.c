/*
 * demo-code/za-pthread-lifecycle.c
 *
 * Create one or many threads (one per CLI arg) and try to join each of them,
 * including the detached threads --- which invokes undefined behavior!
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
#include <signal.h>
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


#define THREADS_MAX  12  /* Maximum number of threads that can be tracked */

#define THREAD_CONFIG_MAX  31  /* Maximum length of config string */


typedef struct {
    char  uc_config_buf[THREAD_CONFIG_MAX + 1];
} thread_config;


static int  n_threads = 0;

static thread_config  thread_configs[THREADS_MAX];
static pthread_t      thread_ids[THREADS_MAX];

static unsigned long  n_started = 0;


static int
find_thread_config (const char *config_str)
{
    int  ix;

    assert(0 <= n_threads);
    assert(n_threads <= THREADS_MAX);

    for (ix = 0; ix < n_threads; ++ix) {
        if (0 == strcmp(config_str, thread_configs[ix].uc_config_buf)) {
            return ix;
        }
    }

    return -1;
}

static int
add_thread_config (const char *config_str)
{
    const size_t  len = strlen(config_str);

    int  pos;

    if (len > THREAD_CONFIG_MAX) {
        return -1;
    }

    assert(0 <= n_threads);

    if (n_threads >= THREADS_MAX) {
        return -2;
    }

    pos = n_threads++;

    memcpy(thread_configs[pos].uc_config_buf, config_str, len);
    thread_configs[pos].uc_config_buf[len] = '\0';

    return pos;
}


static volatile sig_atomic_t  segv_sig = 0;

static void
sigsegv_handler (int signo)
{
    static const char  Message[] =
        "\nCaught SIGSEGV while testing pthread_join().\n";

    segv_sig = signo;

    write(STDERR_FILENO, Message, sizeof Message - 1);
}

static void
register_sigsegv_handler (void)
{
    struct sigaction  act;

    memset(&act, 0, sizeof act);

    act.sa_handler = &sigsegv_handler;
    sigemptyset(&act.sa_mask);

    /* If 'sa_flags' is zero, the signal handler ('sigsegv_handler')
     * could be invoked continously, using CPU aggressively, and
     * only SIGKILL or SIGSTOP would be usable to regain control of the process.
     *
     * Therefore we want signal disposition to be reset to SIG_DFL
     * on entry to signal handler:
     */
    act.sa_flags = SA_RESETHAND;

    if (sigaction(SIGSEGV, &act, NULL) < 0) {
        perror("sigaction(SIGSEGV)");
        exit(11);
    }
}

/*
 * Go back to the default signal disposition for SIGSEGV,
 * even if the signal was not caught = the handler registered
 * by the above function ('register_sigsegv_handler') was not executed.
 */
static void
unregister_sigsegv_handler (void)
{
    if (SIG_ERR == signal(SIGSEGV, SIG_DFL)) {
        perror("signal(SIGSEGV, SIG_DFL)");
        exit(12);
    }
}

static int
join_one_thread (int pos)
{
    char  err_buf[128];
    int   res;
    int   tjoin_res;
    void *thr_retval = NULL;

    sig_atomic_t  saved_segv_sig;

    /*
     * Current thread's info (current = the thread we are about to join):
     */
    thread_config *curr_config;
    const char    *curr_config_str;

    assert(0 < n_threads);
    assert(n_threads <= THREADS_MAX);
    assert(0 <= pos);
    assert(pos < n_threads);

    curr_config = &thread_configs[pos];
    curr_config_str = curr_config->uc_config_buf;

    printf("[%d] Trying to join thread '%s' ...\n",
           pos, curr_config_str);

    segv_sig = 0;
    register_sigsegv_handler();

    tjoin_res = pthread_join(thread_ids[pos], &thr_retval);

    saved_segv_sig = segv_sig;
    /*
     * Always go back to the default signal disposition for SIGSEGV ---
     * even if the signal was not caught during this attempt to join:
     */
    unregister_sigsegv_handler();

    if (tjoin_res == 0) {
        if (PTHREAD_CANCELED == thr_retval) {
            printf("[%d] PTHREAD_CANCELED (thread '%s'); segv_sig=%d.\n",
                   pos, curr_config_str, (int) saved_segv_sig);
            return 1;
        } else {
            if (curr_config == thr_retval) {
                printf("[%d] normal exit for thread '%s', expected value; segv_sig=%d.\n",
                       pos, curr_config_str, (int) saved_segv_sig);
                return 0;
            } else {
                printf("[%d] normal exit for thread '%s', unexpected value; segv_sig=%d.\n",
                       pos, curr_config_str, (int) saved_segv_sig);
                return 0;
            }
        }
    } else {
        res = strerror_r(tjoin_res, err_buf, sizeof err_buf);
        if (res != 0) {
            fprintf(stderr, "[%d] strerror_r(%d) failed, returning the errno value %d;"
                    " segv_sig=%d.\n",
                    pos, tjoin_res, res, (int) saved_segv_sig);
            exit(99);
        }

        printf("[%d] pthread_join() failed for '%s', returning the errno value %d = %s;"
               " segv_sig=%d.\n",
               pos, curr_config_str, tjoin_res, err_buf, (int) saved_segv_sig);
        return 2;
    }
}


static volatile sig_atomic_t  stop_sig = 0;

static void
soft_stop_handler (int signo)
{
    stop_sig = signo;
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


static void *
sleeper_thread_func (void *arg)
{
    thread_config *const tinfo = arg;

    const char *const message_preamble = tinfo->uc_config_buf;

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

    fill_timeval_from_double(&cycle_tval, cycle_time_s);

    fprintf(stdout, "%s Cycle time: ", message_preamble);
    show_timeval(&cycle_tval, stdout);
    fprintf(stdout, ".\n");

    while (stop_sig == 0) {
        tval = cycle_tval;
        errno = 0;
        sel_res = select(0, NULL, NULL, NULL, &tval);  /* sleep */
        sel_err = errno;
        ++n_cycles;

        if (sel_res == 0) {  /* timeout */
            /* TODO: maybe print a progress message (one dot per cycle?) */
#if 0  /* message commented out, does not seem to help */
            printf(" %s(%lu) ", message_preamble, n_cycles);
            fflush(stdout);
#endif
        } else {
            assert(sel_res == -1);

            res = strerror_r(sel_err, err_buf, sizeof err_buf);
            if (res != 0) {
                fprintf(stderr, "%s [%lu cycles: %lu intr, %lu fail]"
                        " strerror_r(%d) failed, returning the errno value %d.\n",
                        message_preamble, n_cycles, n_intr, n_fail, sel_err, res);
                exit(99);
            }

            if (EINTR == sel_err) {
                if (stop_sig != 0) {
                    fprintf(stderr, "%s [%lu cycles: %lu intr, %lu fail]"
                            " select() interrupted (probably signal %d): errno %d = %s\n",
                            message_preamble, n_cycles, n_intr, n_fail,
                            stop_sig, sel_err, err_buf);
                } else {
                    ++n_intr;
                    fprintf(stderr, "%s [%lu cycles: %lu intr, %lu fail]"
                            " select() unexpectedly interrupted: errno %d = %s\n",
                            message_preamble, n_cycles, n_intr, n_fail,
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
                        message_preamble, n_cycles, n_intr, n_fail, sel_err, err_buf);
                exit(90);
            } else {
                ++n_fail;
                fprintf(stderr, "%s [%lu cycles: %lu intr, %lu fail]"
                        " Unexpected errno %d from select(): %s\n",
                        message_preamble, n_cycles, n_intr, n_fail, sel_err, err_buf);
            }
        }
    }

    printf("  %s: Sleeping loop finished after %lu cycles, %lu intr, %lu fail.\n",
           message_preamble, n_cycles, n_intr, n_fail);
    fflush(stdout);

    return tinfo;
}


/*
 * Handle Argument (usually coming from command-line interface).
 * Each argument should describe a thread to be created/started.
 * This function handles one argument, but it can be any of the legal arguments.
 * Intended to be called repeatedly until command-line arguments are exhausted.
 */
static int
handle_arg_start_thread_ (const char *arg)
{
    pthread_attr_t  attr;
    pthread_attr_t *attr_p;

    char  err_buf[128];
    int   res;
    int   tcreate_res;
    int   pos;

    pos = find_thread_config(arg);
    if (pos >= 0) {
        fprintf(stderr, "Found thread config '%s' at %d\n", arg, pos);
        exit(5);
    }

    /*
     * Initialize the thread attributes object always, even if not needed,
     * so we can destroy it without any check after pthread_create() below.
     * Unconditional cleanup code is safer and easier to audit.
     */
    res = pthread_attr_init(&attr);
    if (res != 0) {
        fprintf(stderr, "[%d: '%s'] pthread_attr_init() failed, returning the errno value %d.\n",
                pos, arg, res);
        exit(6);
    }

    if (0 == strncmp("j", arg, 1)) { /* The prefix 'j' stands for "Joinable" */
        pos = add_thread_config(arg);
        if (pos < 0) {
            fprintf(stderr, "Could not add thread config '%s'\n", arg);
            exit(7);
        }

        attr_p = NULL;  /* default attributes are fine in this case */
    }
    else if (0 == strncmp("d", arg, 1)) { /* The prefix 'd' stands for "Detached" */
        pos = add_thread_config(arg);
        if (pos < 0) {
            fprintf(stderr, "Could not add thread config '%s'\n", arg);
            exit(8);
        }

        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        attr_p = &attr;
    }
    else {
        return -1;
    }

    tcreate_res = pthread_create(
                    &thread_ids[pos],
                    attr_p,
                    &sleeper_thread_func,
                    &thread_configs[pos]);

    pthread_attr_destroy(&attr);  /* Not 'attr_p', which may be NULL */

    if (tcreate_res == 0) {
        ++n_started;
    } else {
        res = strerror_r(tcreate_res, err_buf, sizeof err_buf);
        if (res != 0) {
            fprintf(stderr, "[%d] strerror_r(%d) failed, returning the errno value %d.\n",
                    pos, tcreate_res, res);
            exit(99);
        }

        printf("[%d] pthread_create() failed for '%s',"
               " returning the errno value %d = %s\n",
               pos, thread_configs[pos].uc_config_buf,
               tcreate_res, err_buf);
    }

    return 0;
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
        exit(21);
    }
    if (*end != '\0') {
        fprintf(stderr, "Unexpected text '%s' after cycle time %g\n",
                end, seconds);
        exit(22);
    }
    if (strto_err != 0) {
        fprintf(stderr, "Parsing cycle time '%s' failed with errno %d: %s\n",
                data, strto_err, strerror(strto_err));
        exit(23);
    }

    if (seconds < 0.0) {
        fprintf(stderr, "Cycle time must be positive or zero (got %g, original text was '%s')\n",
                seconds, data);
        exit(24);
    }

    return seconds;
}


static void
show_usage (FILE *out_stream)
{
    fprintf(out_stream, "Usage: [cycle_time=<Seconds_with_decimals>]"
            " <Threads:one_or_many(j...|d...)>\n");

    fprintf(out_stream, "  The thread name prefix 'j' stands for \"Joinable\".\n");
    fprintf(out_stream, "  The thread name prefix 'd' stands for \"Detached\".\n");
}

int
main (int argc, char* argv[])
{
    struct sigaction  act;

    unsigned long  n_normal_exit = 0;
    unsigned long  n_canceled = 0;
    unsigned long  n_join_fail = 0;

    int  arg_pos = 1;
    int  res;
    int  join_res;
    int  ix;

    const char *data;

    if (arg_pos < argc) {
        if (0 == strncmp("cycle_time=", argv[arg_pos], 11)) {
            data = argv[arg_pos] + 11;
            cycle_time_s = parse_cycle_time(data);
            ++arg_pos;
        }
    }

    memset(&act, 0, sizeof act);

    act.sa_handler = &soft_stop_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction(SIGINT)");
        exit(10);
    }

    printf("Pid = %ld\n", (long) getpid());

    printf("Trying to start the threads specified on the command line.\n");
    printf("(The threads will finish when this process receives SIGINT:\n"
           "    one way to cause SIGINT is to press Ctrl-C in terminal.)\n\n");

    /*
     * Start handling the thread arguments only _after_ setting up
     * the signal handling (soft stop, in this program)
     * because in this case the argument handling function
     * 'handle_arg_start_thread_' starts each thread immediately,
     * does _not_ just set up the configs for a postponed start
     * (as we do in the 'util-ex-threads' module).
     */
    for (; arg_pos < argc; ++arg_pos) {
        res = handle_arg_start_thread_(argv[arg_pos]);
        if (res != 0) {
            fprintf(stderr, "Unrecognized argument '%s' (error %d).\n",
                    argv[arg_pos], res);
            show_usage(stderr);
            return 2;
        }
    }

    printf("\n %lu threads started. Waiting for thread termination (join)...\n",
           n_started);

    assert(0 <= n_threads);
    assert(n_threads <= THREADS_MAX);

    /*
     * After this point, the program might terminate with signal 11,
     * Segmentation fault --- because
     * calling pthread_join() on a detached thread invokes undefined behavior.
     *
     * On GNU/Linux at least, some attempts to join a detached thread
     * may return the errno value 22 (Invalid argument); this
     * does _not_ guarantee that _every_ attempt to join a detached thread
     * will behave the same way!
     *
     * Undefined behavior is _not_ easily predicted:
     * a given program might not crash at same point every time.
     */
    for (ix = 0; ix < n_threads; ++ix) {
        join_res = join_one_thread(ix);
        switch (join_res)
        {
        case 0:
            ++n_normal_exit;
            break;
        case 1:
            ++n_canceled;
            break;
        case 2:
            ++n_join_fail;
            break;
        default:
            abort();
        }
    }

    printf("Normal exit: %lu, canceled: %lu; %lu could not be joined.\n",
           n_normal_exit, n_canceled, n_join_fail);

    return 0;
}
