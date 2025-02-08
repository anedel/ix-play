/*
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2014--2025 Alexandru Nedel
 *
 * (add your name here when you make significant changes to this file,
 *  if you want to)
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "loop-errno-sig.h"
#include "util-sigaction.h"


static int
parse_sa_flags_str (const char *data)
{
    int  flags = 0;

    int  res = parse_sigaction_flags(&flags, data);

    if (res == 0) {
        return flags;
    } else {
        fprintf(stderr, "Bad sigaction flag(s) '%s'\n", data);
        exit(3);
    }
}


static void
show_usage (FILE *out_stream)
{
    fprintf(out_stream, "Usage: [sa_flags=...]\n");
    show_all_sigaction_flags(out_stream);
}

/* Forward declaration, to break circular dependency:
 * describe_errno() refers 'main' and is called by it.
 */
static void describe_errno(void);

int
main (int argc, char* argv[])
{
    int  sigact_flags = SA_RESTART;

    const char *data;

    if (argc == 2) {
        if (0 == strncmp("sa_flags=", argv[1], 9)) {
            data = argv[1] + 9;
            sigact_flags = parse_sa_flags_str(data);
        } else {
            fprintf(stderr, "Unrecognized argument '%s'.\n",
                    argv[1]);
            show_usage(stderr);
            exit(2);
        }
    }

    printf("Pid = %ld\n", (long) getpid());
    printf("SIGRTMIN = %d, SIGRTMAX = %d\n", SIGRTMIN, SIGRTMAX);

    show_sigaction_flags(sigact_flags, stdout);

    describe_errno();
    test_close_ebadf();

    register_loop_err_sigactions(sigact_flags);

    loop_expecting_eacces("");

    printf("\nThe signal handler with interfering action executed %lu times.\n",
           get_num_acts());

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
