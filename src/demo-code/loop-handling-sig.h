/*
 * demo-code/loop-handling-sig.h
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

unsigned long  get_n_handled_async(void);

/*
 * Second arg ('cycle_time_s') is the Cycle Time in Seconds; decimals allowed.
 */
void  loop_waiting_signal(const char *message_preamble, double cycle_time_s);
void  loop_sleeping(const char *message_preamble, double cycle_time_s);

void  register_loop_handlesig_sigactions(int sigaction_flags);

void  get_loop_handlesig_sigset(sigset_t *out);
