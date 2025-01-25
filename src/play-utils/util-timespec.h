/*
 * play-utils/util-timespec.h
 *
 * Utility module for using 'struct timespec'
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2014--2025 Alexandru Nedel
 *
 * (add your name here when you make significant changes to this file,
 *  if you want to)
 */

#include <stdio.h>
#include <time.h>


int  fill_timespec_from_double(struct timespec *dest_tspec, double seconds);

void  show_timespec(const struct timespec *tspec, FILE *out_stream);
