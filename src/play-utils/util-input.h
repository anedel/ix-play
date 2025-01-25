/*
 * play-utils/util-input.h
 * 
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2014--2025 Alexandru Nedel
 *
 * (add your name here when you make significant changes to this file,
 *  if you want to)
 */

#include <stdio.h>


void  discard_input(int in_fd, FILE *report_stream, size_t bufsize);
int   discard_pending_input(int in_fd, FILE *report_stream);

int  wait_for_input_char(int in_fd, FILE *report_stream);

int  pause_prompt(int in_fd, FILE *out_stream, const char *prompt);

static inline int
pause_any_key (int in_fd, FILE *out_stream)
{
    return pause_prompt(in_fd, out_stream, "Press any key to continue");
}
