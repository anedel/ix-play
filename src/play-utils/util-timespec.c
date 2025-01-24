/*
 * play-utils/util-timespec.c
 *
 * Utility module for using 'struct timespec'
 */

#include "util-timespec.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>  /* for 'uintmax_t' */


static const long  nanosec_per_sec = 1000000000L;

static void
normalize_timespec_ (struct timespec *tspec)
{
    assert(tspec->tv_sec >= 0);
    assert(tspec->tv_nsec >= 0);

    if (tspec->tv_nsec >= nanosec_per_sec) {
        tspec->tv_nsec -= nanosec_per_sec;
        tspec->tv_sec  -= 1;
    }
}


/*
 * Intended to be used for relative timeouts ---
 * for example, passed to sigtimedwait() as third argument.
 * Mainly used for small time value = seconds or minutes,
 * definitely less than an hour (but there's no reason to have a limit).
 */
int
fill_timespec_from_double (struct timespec *dest_tspec, double seconds)
{
    double  sec_rounded_down;
    double  nanosec;

    if (seconds < 0.0) {
        /* Use a safe default: one second, at least; zero is a bad idea. */
        dest_tspec->tv_sec = 1;
        dest_tspec->tv_nsec = 0;
        return -1;
    }

    sec_rounded_down = floor(seconds);
    nanosec = ceil((seconds - sec_rounded_down) * (double) nanosec_per_sec);

    dest_tspec->tv_sec = (time_t) sec_rounded_down;
    dest_tspec->tv_nsec = (long) nanosec;

    normalize_timespec_(dest_tspec);

    return 0;
}

void
show_timespec (const struct timespec *tspec, FILE *out_stream)
{
    fprintf(out_stream, "timespec(tv_sec = %ju seconds, tv_nsec = %ld nanoseconds)",
            (uintmax_t) tspec->tv_sec,
            (long)      tspec->tv_nsec);
}
