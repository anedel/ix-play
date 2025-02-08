/*
 * demo-code/za-rtsig-send.c
 *
 */

#include <assert.h>
#include <errno.h>
#include <limits.h>  /* for 'INT_MAX', etc. */
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


static unsigned long   burst_size = 1;
static struct timeval  delay_between_bursts_tval;
static int  want_delay_between_bursts = 1;

static int  target_pid = 0;
static int  the_signo = 0;

static int  incr_signal_value = 0;
static int  curr_signal_value = 0;  /* May be changed after each signal sent */

static unsigned long long  num_calls = 0;
static unsigned long long  num_sent = 0;


static volatile sig_atomic_t  stop_sig = 0;

void
soft_stop_handler (int signo)
{
    stop_sig = signo;
}


/* Returning zero or positive value means relatively successful =
 * caller may repeat or try again (send another burst):
 */
static int
send_burst (void)
{
    unsigned long  ix;

    union sigval  val;

    int      my_res;
    errno_t  my_err;

    for (ix = 0; ix < burst_size; ++ix) {
        if (stop_sig != 0) {
            printf("Burst stopped by signal %d after %lu iterations; total %llu calls, %llu signals queued.\n",
                   stop_sig, ix, num_calls, num_sent);
            break;
        }

        val.sival_int = curr_signal_value;

        curr_signal_value += incr_signal_value;

        errno = 0;
        my_res = sigqueue(target_pid, the_signo, val);
        my_err = errno;
        ++num_calls;

        if (my_res == 0) {
            ++num_sent;
        } else {
            switch (my_err)
            {
            case EAGAIN:
                /* 'stdout', not 'stderr', because we expect this to happen occasionally */
                fprintf(stdout, "[%lu out of %lu; total %llu calls, %llu sent] sigqueue(sival_int=%d) failed, can retry: result %d, errno %d = %s\n",
                        ix, burst_size, num_calls, num_sent, val.sival_int,
                        my_res, my_err, strerror(my_err));
                return 1;

            case EINVAL:
            case EPERM:
            case ESRCH:
                fprintf(stderr, "[%lu out of %lu; total %llu calls, %llu sent] sigqueue(sival_int=%d) failed: result %d, errno %d = %s\n",
                        ix, burst_size, num_calls, num_sent, val.sival_int,
                        my_res, my_err, strerror(my_err));
                return -1000;

            default:
                fprintf(stderr, "[%lu out of %lu; total %llu calls, %llu sent] sigqueue(sival_int=%d) failed: result %d, unexpected errno %d = %s\n",
                        ix, burst_size, num_calls, num_sent, val.sival_int,
                        my_res, my_err, strerror(my_err));
                return -2345;
            }
        }
    }

    /* Successful; caller may repeat (send another burst): */
    return 0;
}

static void
loop_sending (void)
{
    unsigned long  num_bursts = 0;

    int  res;

    while (stop_sig == 0) {
        res = send_burst();
        ++num_bursts;

        if (res < 0) {
            printf("\nUnlikely to work if we try again, send_burst() returned %d.\n",
                   res);
            printf("\nStopped after %lu bursts; total %llu calls, %llu signals queued.\n",
                   num_bursts, num_calls, num_sent);
            return;
        }

        if (want_delay_between_bursts) {
            delay_(&delay_between_bursts_tval);
        }
    }

    printf("Stopped by signal %d after %lu bursts; total %llu calls, %llu signals queued.\n",
           stop_sig, num_bursts, num_calls, num_sent);
}


static void
register_soft_stop_handler (void)
{
    struct sigaction  act;

    memset(&act, 0, sizeof act);

    act.sa_handler = &soft_stop_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction(SIGINT)");
        exit(7);
    }
}


static int
parse_signo (const char *data)
{
    const int  Max_Signo = 999; /* way too big, just to stop obviously bad values */

    char *end = NULL;  /* for strto...() functions */

    errno_t  strto_err;

    long  num;

    errno = 0;
    num = strtol(data, &end, 10);
    strto_err = errno;

    if (end == data) {
        fprintf(stderr, "Could not parse signal number '%s'\n",
                data);
        exit(11);
    }
    if (*end != '\0') {
        fprintf(stderr, "Unexpected text '%s' after signal number %ld\n",
                end, num);
        exit(12);
    }
    if (strto_err != 0) {
        fprintf(stderr, "Parsing signal number '%s' failed with errno %d: %s\n",
                data, strto_err, strerror(strto_err));
        exit(13);
    }

    if (num < 0) {
        fprintf(stderr, "Signal number must be positive or zero (got %ld, original text was '%s')\n",
                num, data);
        exit(14);
    }
    if (num > Max_Signo) {
        fprintf(stderr, "Signal number is way too big: %ld, original text was '%s'\n",
                num, data);
        exit(15);
    }

    if (num > SIGRTMAX) {
        fprintf(stderr, "Warning: %ld is greater than the largest Real-Time signal number (SIGRTMAX=%d).\n",
                num, SIGRTMAX);
    }

    return (int) num;
}

static int
parse_pid (const char *data)
{
    char *end = NULL;  /* for strto...() functions */

    errno_t  strto_err;

    long  num;

    errno = 0;
    num = strtol(data, &end, 10);
    strto_err = errno;

    if (end == data) {
        fprintf(stderr, "Could not parse pid '%s'\n",
                data);
        exit(21);
    }
    if (*end != '\0') {
        fprintf(stderr, "Unexpected text '%s' after pid %ld\n",
                end, num);
        exit(22);
    }
    if (strto_err != 0) {
        fprintf(stderr, "Parsing pid '%s' failed with errno %d: %s\n",
                data, strto_err, strerror(strto_err));
        exit(23);
    }

    if (num <= 0) {
        fprintf(stderr, "Pid must be positive (got %ld, original text was '%s')\n",
                num, data);
        exit(24);
    }
    if (num >= INT_MAX) {
        fprintf(stderr, "Pid too big: %ld, original text was '%s'\n",
                num, data);
        exit(25);
    }

    return (int) num;
}

static int
parse_rtsig_val (const char *data)
{
    char *end = NULL;  /* for strto...() functions */

    errno_t  strto_err;

    long  num;

    errno = 0;
    num = strtol(data, &end, 10);
    strto_err = errno;

    if (end == data) {
        fprintf(stderr, "Could not parse RT signal value '%s'\n",
                data);
        exit(31);
    }
    if (*end != '\0') {
        fprintf(stderr, "Unexpected text '%s' after RT signal value %ld\n",
                end, num);
        exit(32);
    }
    if (strto_err != 0) {
        fprintf(stderr, "Parsing RT signal value '%s' failed with errno %d: %s\n",
                data, strto_err, strerror(strto_err));
        exit(33);
    }

    if (num <= INT_MIN) {
        fprintf(stderr, "RT signal value too small: %ld, original text was '%s'\n",
                num, data);
        exit(34);
    }
    if (num >= INT_MAX) {
        fprintf(stderr, "RT signal value too big: %ld, original text was '%s'\n",
                num, data);
        exit(35);
    }

    return (int) num;
}

static int
parse_step (const char *data)
{
    char *end = NULL;  /* for strto...() functions */

    errno_t  strto_err;

    long  num;

    errno = 0;
    num = strtol(data, &end, 10);
    strto_err = errno;

    if (end == data) {
        fprintf(stderr, "Could not parse step '%s'\n",
                data);
        exit(41);
    }
    if (*end != '\0') {
        fprintf(stderr, "Unexpected text '%s' after step %ld\n",
                end, num);
        exit(42);
    }
    if (strto_err != 0) {
        fprintf(stderr, "Parsing step '%s' failed with errno %d: %s\n",
                data, strto_err, strerror(strto_err));
        exit(43);
    }

    if (num <= INT_MIN) {
        fprintf(stderr, "Step too small: %ld, original text was '%s'\n",
                num, data);
        exit(44);
    }
    if (num >= INT_MAX) {
        fprintf(stderr, "Step too big: %ld, original text was '%s'\n",
                num, data);
        exit(45);
    }

    return (int) num;
}

static int
parse_burst_size (const char *data)
{
    char *end = NULL;  /* for strto...() functions */

    errno_t  strto_err;

    unsigned long  burst_size;

    errno = 0;
    burst_size = strtoul(data, &end, 10);
    strto_err = errno;

    if (end == data) {
        fprintf(stderr, "Could not parse burst size '%s'\n",
                data);
        exit(51);
    }
    if (*end != '\0') {
        fprintf(stderr, "Unexpected text '%s' after burst size %ld\n",
                end, burst_size);
        exit(52);
    }
    if (strto_err != 0) {
        fprintf(stderr, "Parsing burst size '%s' failed with errno %d: %s\n",
                data, strto_err, strerror(strto_err));
        exit(53);
    }

    if (burst_size <= 0) {
        fprintf(stderr, "Burst size must be positive (got %ld, original text was '%s')\n",
                burst_size, data);
        exit(54);
    }
    if (burst_size >= INT_MAX) {
        fprintf(stderr, "Burst size too big: %ld, original text was '%s'\n",
                burst_size, data);
        exit(55);
    }

    return burst_size;
}

static double
parse_delay (const char *data)
{
    char *end = NULL;  /* for strto...() functions */

    errno_t  strto_err;

    double  delay;

    errno = 0;
    delay = strtod(data, &end);
    strto_err = errno;

    if (end == data) {
        fprintf(stderr, "Could not parse delay '%s'\n",
                data);
        exit(61);
    }
    if (*end != '\0') {
        fprintf(stderr, "Unexpected text '%s' after delay %g\n",
                end, delay);
        exit(62);
    }
    if (strto_err != 0) {
        fprintf(stderr, "Parsing delay '%s' failed with errno %d: %s\n",
                data, strto_err, strerror(strto_err));
        exit(63);
    }

    if (delay < 0.0) {
        fprintf(stderr, "Delay must be positive or zero (got %g, original text was '%s')\n",
                delay, data);
        exit(64);
    }

    return delay;
}


/*
 * Handle Argument (usually coming from command-line interface).
 * This function handles one argument, but it can be any of the legal arguments.
 * Intended to be called repeatedly until command-line arguments are exhausted.
 * Same type of argument/option can appear multiple times; then
 * its values can either (1) accumulate, or (2) be discarded/overriden =
 * the last occurence that is valid/complete/usable will take effect
 * (as if it was the only one of its kind).
 */
static int
handle_arg (const char *arg)
{
    const char *data;

    double  delay;

    if (0 == strncmp("to:", arg, 3)) {
        data = arg + 3;
        target_pid = parse_pid(data);
    }
    else if (0 == strncmp("val:", arg, 4)) {
        data = arg + 4;
        curr_signal_value = parse_rtsig_val(data);
    }
    else if (0 == strcmp("incr", arg)) {
        incr_signal_value = 1;
    }
    else if (0 == strcmp("decr", arg)) {
        incr_signal_value = -1;
    }
    else if (0 == strncmp("incr:", arg, 5)) {
        data = arg + 5;
        incr_signal_value = parse_step(data);
    }
    else if (0 == strncmp("decr:", arg, 5)) {
        data = arg + 5;
        incr_signal_value = -parse_step(data);
    }
    else if (0 == strncmp("burst:", arg, 6)) {
        data = arg + 6;
        burst_size = parse_burst_size(data);
    }
    else if (0 == strncmp("delay:", arg, 6)) {
        data = arg + 6;
        delay = parse_delay(data);
        fill_timeval_from_double(&delay_between_bursts_tval, delay);
        want_delay_between_bursts = 1;
    }
    else {
        return -1;
    }

    return 0;
}


static void
show_usage (FILE *out_stream)
{
    fprintf(out_stream, "Usage: <Signo>  to:<Pid>\n"
            "  [val:<N>]  [incr | incr:<Step> | decr | decr:<Step>]\n"
            "  [burst:<Burst_Size>]  [delay:<Seconds_with_decimals>]\n");
}

static void
show_settings (FILE *out_stream)
{
    fprintf(out_stream, "Target Pid: %d;\n",
            target_pid);
    fprintf(out_stream, "Signal number: %d;\n",
            the_signo);

    fprintf(out_stream, "Value to send: %d;\n",
            curr_signal_value);
    fprintf(out_stream, "Value change step: %d (the value could be incremented or decremented);\n",
            incr_signal_value);

    fprintf(out_stream, "Burst size: %lu;\n",
            burst_size);

    fprintf(out_stream, "Delay between bursts: ");
    show_timeval(&delay_between_bursts_tval, out_stream);
    fprintf(out_stream, ";\n");

    fprintf(out_stream, "Want delay between bursts: %d.\n",
            want_delay_between_bursts);
}

int
main (int argc, char* argv[])
{
    int  arg_pos;
    int  res;

    if (argc < 3) {
        fprintf(stderr, "Bad arg count.\n");
        show_usage(stderr);
        return 1;
    }

    the_signo = parse_signo(argv[1]);

    fill_timeval_from_double(&delay_between_bursts_tval, 1.6);

    for (arg_pos = 2; arg_pos < argc; ++arg_pos) {
        res = handle_arg(argv[arg_pos]);
        if (res != 0) {
            fprintf(stderr, "Unrecognized argument '%s' (error %d).\n",
                    argv[arg_pos], res);
            show_usage(stderr);
            return 2;
        }
    }

    if (target_pid == 0) {
        fprintf(stderr, "Target PID must be specified.\n");
        show_usage(stderr);
        return 3;
    }

    show_settings(stdout);
    printf("\nMy Pid = %ld\n", (long) getpid());

    register_soft_stop_handler();
    loop_sending();

    return 0;
}
