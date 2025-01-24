
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "loop-errno-sig.h"
#include "util-ex-threads.h"
#include "util-sigaction.h"


static void *
loop_err_thread_func (void *arg)
{
    uex_thread_info *const tinfo = arg;

    loop_expecting_eacces(tinfo->config_str);

    return tinfo;
}


/*
 * Handle Argument (usually coming from command-line interface).
 * Each argument should describe a thread to be created/started.
 * This function handles one argument, but it can be any of the legal arguments.
 * Intended to be called repeatedly until command-line arguments are exhausted.
 */
static int
handle_arg (const char *arg)
{
    int  pos;

    pos = uex_find_thread_config_by_prefix(arg, UEX_THREAD_CONFIG_MAX);
    if (pos >= 0) {
        fprintf(stderr, "Found thread config '%s' at %d\n", arg, pos);
        exit(6);
    }

    if (0 == strncmp("le", arg, 2)) { /* The prefix 'le' stands for "Loop-Errno" */
        pos = uex_add_thread_config(arg, NULL, &loop_err_thread_func);
        if (pos < 0) {
            fprintf(stderr, "Could not add thread config '%s'\n", arg);
            exit(7);
        }
    }
    else {
        return -1;
    }

    return 0;
}


static int
parse_sa_flags_str (const char *data)
{
    int  flags = 0;

    int  res = parse_sigaction_flags(&flags, data);

    if (res == 0) {
        return flags;
    } else {
        fprintf(stderr, "Bad sigaction flag(s) '%s'\n", data);
        exit(8);
    }
}


static void
show_usage (FILE *out_stream)
{
    fprintf(out_stream, "Usage: [sa_flags=...] <Threads:one_or_many(le...)>\n");
    show_all_sigaction_flags(out_stream);
}

/* Forward declaration, to break circular dependency:
 * describe_errno() refers 'main' and is called by it.
 */
static void describe_errno(void);

int
main (int argc, char* argv[])
{
    sigset_t  interfering_sigset;

    int  sigact_flags = SA_RESTART;
    int  arg_pos = 1;
    int  res;
    const char *data;

    if (arg_pos < argc) {
        if (0 == strncmp("sa_flags=", argv[arg_pos], 9)) {
            data = argv[arg_pos] + 9;
            sigact_flags = parse_sa_flags_str(data);
            ++arg_pos;
        }
    }

    for (; arg_pos < argc; ++arg_pos) {
        res = handle_arg(argv[arg_pos]);
        if (res != 0) {
            fprintf(stderr, "Unrecognized argument '%s' (error %d).\n",
                    argv[arg_pos], res);
            show_usage(stderr);
            return 2;
        }
    }

    printf("Pid = %ld\n", (long) getpid());
    printf("SIGRTMIN = %d, SIGRTMAX = %d\n", SIGRTMIN, SIGRTMAX);

    show_sigaction_flags(sigact_flags, stdout);

    describe_errno();
    test_close_ebadf();

    register_loop_err_sigactions(sigact_flags);

    get_loop_err_sigset(&interfering_sigset);
    res = pthread_sigmask(SIG_BLOCK, &interfering_sigset, NULL);
    if (res != 0) {
        fprintf(stderr, "Could not block Loop-Err interfering signals in main thread: %d\n",
                res);
        return 3;
    }

    uex_start_threads();
    uex_join_threads();

    printf("\nThe signal handler with interfering action executed %lu times.\n",
           get_n_acts());

    return 0;
}


/* Macro definitions to expand macro for display as string.
 * Source / author:
 * http://stackoverflow.com/questions/1562074/how-do-i-show-the-value-of-a-define-at-compile-time
 */
#define VALUE_TO_STRING(x)  #x
#define VALUE(x)  VALUE_TO_STRING(x)
#define MACRO_NAME_VALUE(macro)  (#macro " = '" VALUE(macro) "'")


static void
describe_errno (void)
{
    static int  dummy_static;  /* to show a data address */

    void *const addr = &errno;

#ifdef  errno
    printf("'errno' is a macro, expands to '%s'\n", VALUE(errno));
    /* Another way to print same value: */
    printf("%s\n", MACRO_NAME_VALUE(errno));
#else
    printf("'errno' is not a macro\n");
#endif

    printf("'errno' address: %p;\n",
           addr);
    printf("data addresses: stack %p, static %p;\n",
           &addr, &dummy_static);
    printf("code addresses: 'strerror' %p, 'main' %p, this func %p.\n",
           &strerror, &main, &describe_errno);
}
