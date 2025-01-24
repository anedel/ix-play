
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "loop-handling-sig.h"
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


static int  block = 0;


static int
parse_sa_flags_str (const char *data)
{
    int  flags = 0;

    int  res = parse_sigaction_flags(&flags, data);

    if (res == 0) {
        return flags;
    } else {
        fprintf(stderr, "Bad sigaction flag(s) '%s'\n", data);
        exit(3);
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
    fprintf(out_stream, "Usage: [sa_flags=...] [cycle_time=<Seconds_with_decimals>] [block]\n");
    show_all_sigaction_flags(out_stream);
}

int
main (int argc, char* argv[])
{
    int  sigact_flags = SA_RESTART;

    sigset_t  waited_sigset;
    int  res;

    int  arg_pos = 1;

    const char *data;

    double  cycle_time = 2.4;

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

    if (arg_pos < argc) {
        if (0 == strcmp("block", argv[arg_pos])) {
            block = 1;
            ++arg_pos;
        }
    }

    if (arg_pos < argc) {
        fprintf(stderr, "Unrecognized argument '%s'.\n",
                argv[arg_pos]);
        show_usage(stderr);
        exit(2);
    }

    printf("Pid = %ld\n", (long) getpid());
    printf("SIGRTMIN = %d, SIGRTMAX = %d\n", SIGRTMIN, SIGRTMAX);

    show_sigaction_flags(sigact_flags, stdout);

    if (block) {
        get_loop_handlesig_sigset(&waited_sigset);
        res = sigprocmask(SIG_BLOCK, &waited_sigset, NULL);
        if (res != 0) {
            fprintf(stderr, "Could not block waited signals in main: %d\n",
                    res);
            return 3;
        }

        printf("Blocked the signals we are waiting for so the asynchronous signal handlers cannot catch them.\n");
    }

    register_loop_handlesig_sigactions(sigact_flags);

    loop_waiting_signal("", cycle_time);

    printf("\nThe signal handler executed %lu times.\n",
           get_n_handled_async());

    return 0;
}
