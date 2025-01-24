/*
 * demo-code/loop-errno-sig.h
 * 
 */

#include <signal.h>

unsigned long  get_n_acts(void);

void  test_close_ebadf(void);
void  loop_expecting_eacces(const char *message_preamble);

void  register_loop_err_sigactions(int sigaction_flags);

void  get_loop_err_sigset(sigset_t *out);
