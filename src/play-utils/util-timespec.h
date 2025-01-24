/*
 * play-utils/util-timespec.h
 *
 * Utility module for using 'struct timespec'
 */

#include <stdio.h>
#include <time.h>


int  fill_timespec_from_double(struct timespec *dest_tspec, double seconds);

void  show_timespec(const struct timespec *tspec, FILE *out_stream);
