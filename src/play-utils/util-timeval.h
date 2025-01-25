/*
 * play-utils/util-timeval.h
 *
 * Utility module for using 'struct timeval'
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2014--2025 Alexandru Nedel
 *
 * (add your name here when you make significant changes to this file,
 *  if you want to)
 */

#include <stdio.h>
#include <sys/time.h>


int  fill_timeval_from_double(struct timeval *dest_tval, double seconds);

void  show_timeval(const struct timeval *tval, FILE *out_stream);
