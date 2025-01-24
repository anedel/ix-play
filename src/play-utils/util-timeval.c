/*
 * play-utils/util-timeval.c
 *
 * Utility module for using 'struct timeval'
 */

#include "util-timeval.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>  /* for 'uintmax_t' */


static const long  microsec_per_sec = 1000000L;

static void
normalize_timeval_ (struct timeval *tval)
{
    assert(tval->tv_sec >= 0);
    assert(tval->tv_usec >= 0);

    if (tval->tv_usec >= microsec_per_sec) {
        tval->tv_usec -= microsec_per_sec;
        tval->tv_sec  -= 1;
    }
}


/*
 * Intended to be used for relative timeouts ---
 * for example, passed to select() as fifth argument.
 * Mainly used for small time value = seconds or minutes,
 * definitely less than an hour (but there's no reason to have a limit).
 */
int
fill_timeval_from_double (struct timeval *dest_tval, double seconds)
{
    double  sec_rounded_down;
    double  microsec;

    if (seconds < 0.0) {
        /* Use a safe default: one second, at least; zero is a bad idea. */
        dest_tval->tv_sec = 1;
        dest_tval->tv_usec = 0;
        return -1;
    }

    sec_rounded_down = floor(seconds);
    microsec = ceil((seconds - sec_rounded_down) * (double) microsec_per_sec);

    dest_tval->tv_sec = (time_t) sec_rounded_down;
    dest_tval->tv_usec = (long) microsec;

    normalize_timeval_(dest_tval);

    return 0;
}

void
show_timeval (const struct timeval *tval, FILE *out_stream)
{
    fprintf(out_stream, "timeval(tv_sec = %ju seconds, tv_usec = %ld microseconds)",
            (uintmax_t) tval->tv_sec,
            (long)      tval->tv_usec);
}
