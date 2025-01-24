/*
 * play-utils/util-sigaction.c
 *
 */

#include "util-sigaction.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


struct sa_flag_info {
    char        sfi_chr;
    int         sfi_value;
    const char *sfi_name;
    const char *sfi_description;
};

#define SFI_VALUE_NAME_(FlagName)  FlagName, #FlagName

static const struct sa_flag_info  Flags_Info[] = {
  {'i', SFI_VALUE_NAME_(SA_SIGINFO),
   "Pass extra info to signal handler"
  },
  {'r', SFI_VALUE_NAME_(SA_RESTART),
   "Restart some interruptible functions (instead of failing with EINTR)"
  },
  {'d', SFI_VALUE_NAME_(SA_RESETHAND),
   "Reset signal disposition (to SIG_DFL) on entry to signal handler"
  },
  {'s', SFI_VALUE_NAME_(SA_NOCLDSTOP),
   "Do not generate SIGCHLD when children stop or stopped children continue."
  },
  {'w', SFI_VALUE_NAME_(SA_NOCLDWAIT),
   "Do not create zombie processes on child death"
  },
  {'b', SFI_VALUE_NAME_(SA_NODEFER),
   "Causes signal not to be automatically blocked on entry to signal handler = \n"
   "do not prevent the signal from being received from within its own signal handler."
  },
  /* For now, skip this flag: "Causes signal delivery to occur on an alternate stack." */
};

#undef SFI_VALUE_NAME_

static const size_t  Flags_Count = sizeof Flags_Info / sizeof Flags_Info[0];


static int
map_sigaction_flag (int *dest_flag, char ch)
{
    switch (ch)
    {
    case 'i':
        *dest_flag = SA_SIGINFO;
        break;
    case 'r':
        *dest_flag = SA_RESTART;
        break;
    case 'd': /* lowercase 'D': stands for "Default" (see 'SIG_DFL') */
        *dest_flag = SA_RESETHAND;
        break;
    case 's': /* lowercase 'S': stands for "[no child] Stop" */
        *dest_flag = SA_NOCLDSTOP;
        break;
    case 'w': /* lowercase 'W': stands for "[no child] Wait" */
        *dest_flag = SA_NOCLDWAIT;
        break;
    case 'b': /* lowercase 'B': stands for "[no] Block" */
        *dest_flag = SA_NODEFER;
        break;
    default:
        return -1;
    }

    return 0;
}

int
parse_sigaction_flags (int *dest_flags, const char *input)
{
    const char *p = input;
    int  flag_val;
    int  res;

    while (*p) {
        flag_val = 0;
        res = map_sigaction_flag(&flag_val, *p);
        if (res != 0) {
            return res;
        }
        *dest_flags |= flag_val;
        ++p;
    }

    return 0;
}


void
show_sigaction_flags (int sigaction_flags, FILE *out_stream)
{
    size_t  ix;

    fprintf(out_stream, "\nsigaction flags: combined value 0x%x, CLI arg '",
            sigaction_flags);

    for (ix = 0; ix < Flags_Count; ++ix) {
        if (sigaction_flags & Flags_Info[ix].sfi_value) {
            fprintf(out_stream, "%c", Flags_Info[ix].sfi_chr);
        }
    }

    fprintf(out_stream, "'\nsigaction flag names:");

    for (ix = 0; ix < Flags_Count; ++ix) {
        if (sigaction_flags & Flags_Info[ix].sfi_value) {
            fprintf(out_stream, " %s,", Flags_Info[ix].sfi_name);
        }
    }

    fprintf(out_stream, "\nsigaction flag details:\n");

    for (ix = 0; ix < Flags_Count; ++ix) {
        if (sigaction_flags & Flags_Info[ix].sfi_value) {
            fprintf(out_stream, "'%c' -> %s = 0x%x\n  %s\n",
                    Flags_Info[ix].sfi_chr,
                    Flags_Info[ix].sfi_name,
                    Flags_Info[ix].sfi_value,
                    Flags_Info[ix].sfi_description);
        }
    }

    fprintf(out_stream, "\n");
}

void
show_all_sigaction_flags(FILE *out_stream)
{
    size_t  ix;

    fprintf(out_stream, "\nsigaction flag details:\n");

    for (ix = 0; ix < Flags_Count; ++ix) {
        fprintf(out_stream, "'%c' -> %s = 0x%x\n  %s\n",
                Flags_Info[ix].sfi_chr,
                Flags_Info[ix].sfi_name,
                Flags_Info[ix].sfi_value,
                Flags_Info[ix].sfi_description);
    }

    fprintf(out_stream, "\n");
}


void
register_sa_handler (
        int    signo,
        void (*handler_func_p)(int),
        int    sigaction_flags)
{
    struct sigaction  act;

    memset(&act, 0, sizeof act);

    act.sa_handler = handler_func_p;
    sigemptyset(&act.sa_mask);
    act.sa_flags = sigaction_flags & ~SA_SIGINFO;

    if (sigaction(signo, &act, NULL) < 0) {
        perror("sigaction ~SA_SIGINFO");
        exit(7);
    }
}

void
register_sa_sigaction (
        int    signo,
        void (*sigaction_func_p)(int, siginfo_t *, void *),
        int    sigaction_flags)
{
    struct sigaction  act;

    memset(&act, 0, sizeof act);

    act.sa_sigaction = sigaction_func_p;
    sigemptyset(&act.sa_mask);
    act.sa_flags = sigaction_flags | SA_SIGINFO;

    if (sigaction(signo, &act, NULL) < 0) {
        perror("sigaction SA_SIGINFO");
        exit(8);
    }
}
