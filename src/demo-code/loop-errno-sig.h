/*
 * demo-code/loop-errno-sig.h
 * 
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2014--2025 Alexandru Nedel
 *
 * (add your name here when you make significant changes to this file,
 *  if you want to)
 */

#include <signal.h>

unsigned long  get_num_acts(void);

void  test_close_ebadf(void);
void  loop_expecting_eacces(const char *message_preamble);

void  register_loop_err_sigactions(int sigaction_flags);

void  get_loop_err_sigset(sigset_t *out);
