/*
 * play-utils/util-timeval.h
 *
 * Utility module for using 'struct timeval'
 */

#include <stdio.h>
#include <sys/time.h>


int  fill_timeval_from_double(struct timeval *dest_tval, double seconds);

void  show_timeval(const struct timeval *tval, FILE *out_stream);
