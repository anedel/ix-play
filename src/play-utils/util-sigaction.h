/*
 * play-utils/util-sigaction.h
 *
 */

#include <signal.h>
#include <stdio.h>


int  parse_sigaction_flags(int *dest_flags, const char *input);

void  show_sigaction_flags(int sigaction_flags, FILE *out_stream);
void  show_all_sigaction_flags(FILE *out_stream);

void  register_sa_handler(
        int    signo,
        void (*handler_func_p)(int),
        int    sigaction_flags);

void  register_sa_sigaction(
        int    signo,
        void (*sigaction_func_p)(int, siginfo_t *, void *),
        int    sigaction_flags);
