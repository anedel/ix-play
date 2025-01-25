/*
 * demo-code/za-pthreads-condvar-sem.c
 *
 * Demonstration with POSIX Threads synchronizing with
 * Condition Variable and Semaphore
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2014--2025 Alexandru Nedel
 *
 * (add your name here when you make significant changes to this file,
 *  if you want to)
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "util-ex-threads.h"
#include "util-mutexattr.h"

/*
 * https://www.securecoding.cert.org/confluence/display/seccode/DCL09-C.+Declare+functions+that+return+errno+with+a+return+type+of+errno_t
 *
 * C11 Annex K introduced the new type 'errno_t' that
 * is defined to be type 'int' in 'errno.h' and elsewhere.
 * Many of the functions defined in C11 Annex K return values of this type.
 * The 'errno_t' type should be used as the type of an object that
 * may contain _only_ values that might be found in 'errno'.
 */
#ifndef __STDC_LIB_EXT1__
    typedef  int  errno_t;
#endif


static void
delay_ (const char *message_preamble, const volatile struct timeval *delay_tval)
{
    struct timeval  tval;
        /* Must be set to desired delay before each call to select(), because
         * select() may modify the object pointed to by the timeout argument.
         *
         * The Linux implementation of select() modifies the timeval struct
         * to reflect the amount of time that was not slept.
         * Most other implementations leave the timeout parameter unmodified:
         * for example, the BSD implementation of select().
         */

    char  err_buf[128];
    int   res;

    int      sel_res;
    errno_t  sel_err;

    tval = *delay_tval;

    /* Do not print if delay is less than two seconds,
     * would just fill the screen with useless messages,
     * making the useful info harder to read.
     */
    if (tval.tv_sec > 1) {
        printf(" %s: Sleeping %lu seconds...\n",
               message_preamble, (unsigned long) tval.tv_sec);
    }

    errno = 0;
    sel_res = select(0, NULL, NULL, NULL, &tval);  /* sleep */
    sel_err = errno;

    if (sel_res == 0) {  /* timeout */
        /* TODO: maybe print a progress message (one dot per cycle?) */
    } else {
        assert(sel_res == -1);

        res = strerror_r(sel_err, err_buf, sizeof err_buf);
        if (res != 0) {
            fprintf(stderr, "%s: strerror_r(%d) failed, returning the errno value %d.\n",
                    message_preamble, sel_err, res);
            exit(99);
        }

        if (EINTR == sel_err) {
            fprintf(stderr, "%s: select() interrupted: errno %d = %s\n",
                    message_preamble, sel_err, err_buf);
        } else if (EINVAL == sel_err) {
            /*
             * Given the way we call select() here (no file descriptors),
             * the only possible reason for EINVAL would be that
             * an invalid timeout interval was specified.
             *
             * The timeval struct specifying the timeout _could_ change
             * in this program (see the 'handle_command_' function below),
             * therefore retrying _could_ give a different result.
             *
             * But an invalid timeout interval causes immediate return from
             * select() so it is effectively a zero timeout ---
             * might fill the screen with fast scrolling messages,
             * interfering with the user's attempts to change the value
             * via command_loop_().
             *
             * Therefore it seems best to exit immediately; this would force
             * the user interface code to validate timeout intervals
             * if it's important to avoid termination.
             */
            fprintf(stderr, "%s: Invalid timeout interval for select(): errno %d = %s\n",
                    message_preamble, sel_err, err_buf);
            exit(90);
        } else {
            fprintf(stderr, "%s: Unexpected errno %d from select(): %s\n",
                    message_preamble, sel_err, err_buf);
        }
    }
}


static int  mutex_lock_count = 0;

/* Counting successful operations: */
static unsigned long  n_sem_trywaits = 0;
static unsigned long  n_sem_posts = 0;
static unsigned long  n_cond_signals = 0;
static unsigned long  n_cond_broadcasts = 0;

static void
show_command_prompt_ (void)
{
    printf("\nLock: %d; sem: %lu TryWait, %lu Post; cond: %lu Sig, %lu Bcast ops >>> ",
           mutex_lock_count,
           n_sem_trywaits, n_sem_posts,
           n_cond_signals, n_cond_broadcasts);
    fflush(stdout);
}

static void
show_command_counters_ (void)
{
    printf("Mutex lock count (changed by 'l', 'tl', and 'u' commands): %d\n"
           "Successful operations:\n"
           "  %lu sem_trywait() calls = attempting to decrement (lock) the semaphore,\n"
           "  %lu sem_post() calls = incrementing (unlocking) the semaphore,\n"
           "  %lu pthread_cond_signal() calls,\n"
           "  %lu pthread_cond_broadcast() calls.\n",
           mutex_lock_count,
           n_sem_trywaits, n_sem_posts,
           n_cond_signals, n_cond_broadcasts);
}


static volatile struct timeval  delay_tval = {2, 0};

static sem_t  sem;

static pthread_mutex_t  demo_mutex;
static pthread_cond_t   demo_condvar;

static void
init_synchronization_objects_ (const pthread_mutexattr_t *mutexattr)
{
    sem_init(&sem, 0, 0);

    pthread_mutex_init(&demo_mutex, mutexattr);
    pthread_cond_init(&demo_condvar, NULL);
}

static void
do_sem_trywaits_ (unsigned long n_calls_max)
{
    char  err_buf[128];
    int   res;

    unsigned long  ix;

    int      sem_res;
    errno_t  sem_err;

    for (ix = 0; ix < n_calls_max; ++ix) {
        sem_res = sem_trywait(&sem);
        sem_err = errno;

        if (sem_res == 0) {
            ++n_sem_trywaits;
        } else {
            res = strerror_r(sem_err, err_buf, sizeof err_buf);
            if (res != 0) {
                fprintf(stderr, " strerror_r(%d) failed, returning the errno value %d.\n",
                        sem_err, res);
                exit(92);
            }

            if (0 == ix) {
                printf("sem_trywait() failed with errno %d = %s\n",
                       sem_err, err_buf);
            } else {
                printf("sem_trywait() succeeded %lu times, then failed with errno %d = %s\n",
                       ix, sem_err, err_buf);
            }

            return;
        }
    }

    printf("sem_trywait() x %lu OK\n", ix);
}

static void
do_sem_posts_ (unsigned long n_calls_max)
{
    char  err_buf[128];
    int   res;

    unsigned long  ix;

    int      sem_res;
    errno_t  sem_err;

    for (ix = 0; ix < n_calls_max; ++ix) {
        sem_res = sem_post(&sem);
        sem_err = errno;

        if (sem_res == 0) {
            ++n_sem_posts;
        } else {
            res = strerror_r(sem_err, err_buf, sizeof err_buf);
            if (res != 0) {
                fprintf(stderr, " strerror_r(%d) failed, returning the errno value %d.\n",
                        sem_err, res);
                exit(93);
            }

            if (0 == ix) {
                printf("sem_post() failed with errno %d = %s\n",
                       sem_err, err_buf);
            } else {
                printf("sem_post() succeeded %lu times, then failed with errno %d = %s\n",
                       ix, sem_err, err_buf);
            }

            return;
        }
    }

    printf("sem_post() x %lu OK\n", ix);
}

static void
do_condvar_signals_ (unsigned long n_calls_max)
{
    char  err_buf[128];
    int   res;

    unsigned long  ix;

    errno_t  cond_res;

    for (ix = 0; ix < n_calls_max; ++ix) {
        cond_res = pthread_cond_signal(&demo_condvar);

        if (cond_res == 0) {
            ++n_cond_signals;
        } else {
            res = strerror_r(cond_res, err_buf, sizeof err_buf);
            if (res != 0) {
                fprintf(stderr, " strerror_r(%d) failed, returning the errno value %d.\n",
                        cond_res, res);
                exit(94);
            }

            if (0 == ix) {
                printf("pthread_cond_signal() failed, returning the errno value %d = %s\n",
                       cond_res, err_buf);
            } else {
                printf("pthread_cond_signal() succeeded %lu times, then failed, returning the errno value %d = %s\n",
                       ix, cond_res, err_buf);
            }

            return;
        }
    }

    printf("pthread_cond_signal() x %lu OK\n", ix);
}

static void
do_condvar_broadcasts_ (unsigned long n_calls_max)
{
    char  err_buf[128];
    int   res;

    unsigned long  ix;

    errno_t  cond_res;

    for (ix = 0; ix < n_calls_max; ++ix) {
        cond_res = pthread_cond_broadcast(&demo_condvar);

        if (cond_res == 0) {
            ++n_cond_broadcasts;
        } else {
            res = strerror_r(cond_res, err_buf, sizeof err_buf);
            if (res != 0) {
                fprintf(stderr, " strerror_r(%d) failed, returning the errno value %d.\n",
                        cond_res, res);
                exit(95);
            }

            if (0 == ix) {
                printf("pthread_cond_broadcast() failed, returning the errno value %d = %s\n",
                       cond_res, err_buf);
            } else {
                printf("pthread_cond_broadcast() succeeded %lu times, then failed, returning the errno value %d = %s\n",
                       ix, cond_res, err_buf);
            }

            return;
        }
    }

    printf("pthread_cond_broadcast() x %lu OK\n", ix);
}

static void
do_mutex_lock_ (void)
{
    char  err_buf[128];
    int   res;

    errno_t  mutex_res;

    mutex_res = pthread_mutex_lock(&demo_mutex);
    if (mutex_res == 0) {
        ++mutex_lock_count;
        printf("pthread_mutex_lock() OK\n");
    } else {
        res = strerror_r(mutex_res, err_buf, sizeof err_buf);
        if (res != 0) {
            fprintf(stderr, " strerror_r(%d) failed, returning the errno value %d.\n",
                    mutex_res, res);
            exit(96);
        }

        printf("pthread_mutex_lock() failed, returning the errno value %d = %s\n",
               mutex_res, err_buf);
    }
}

static void
do_mutex_trylock_ (void)
{
    char  err_buf[128];
    int   res;

    errno_t  mutex_res;

    mutex_res = pthread_mutex_trylock(&demo_mutex);
    if (mutex_res == 0) {
        ++mutex_lock_count;
        printf("pthread_mutex_trylock() OK\n");
    } else {
        res = strerror_r(mutex_res, err_buf, sizeof err_buf);
        if (res != 0) {
            fprintf(stderr, " strerror_r(%d) failed, returning the errno value %d.\n",
                    mutex_res, res);
            exit(97);
        }

        printf("pthread_mutex_trylock() failed, returning the errno value %d = %s\n",
               mutex_res, err_buf);
    }
}

static void
do_mutex_unlock_ (void)
{
    char  err_buf[128];
    int   res;

    errno_t  mutex_res;

    mutex_res = pthread_mutex_unlock(&demo_mutex);
    if (mutex_res == 0) {
        if (mutex_lock_count > 0) {
            --mutex_lock_count;
            printf("pthread_mutex_unlock() OK\n");
        } else {
            printf("pthread_mutex_unlock() OK but unnecessary\n");
        }
    } else {
        res = strerror_r(mutex_res, err_buf, sizeof err_buf);
        if (res != 0) {
            fprintf(stderr, " strerror_r(%d) failed, returning the errno value %d.\n",
                    mutex_res, res);
            exit(98);
        }

        printf("pthread_mutex_unlock() failed, returning the errno value %d = %s\n",
               mutex_res, err_buf);
    }
}


/* The leading 'CL_' stands for "Command Loop": */
enum {
    CL_Normal = 1,
    CL_Line_Handled = 2,
    CL_Quit_Requested = 4,
    CL_Parse_Fail = 8,
};


/* Forward declaration, to break circular dependency and
 * allow indirect recursive calls:
 */
static int  parse_line_with_commands_(const char *const commands_str);

static int
handle_repeat_command_ (const char *const subcommands_str,
                        unsigned long repeat_count)
{
    unsigned long  ix;
    int  res;

    if (1 == repeat_count) {
        return parse_line_with_commands_(subcommands_str);
    }

    printf("Repeating '%s' %lu times:\n", subcommands_str, repeat_count);

    for (ix = 0; ix < repeat_count; ++ix) {
        printf(" [%s: %lu / %lu]\n", subcommands_str, ix, repeat_count);

        res = parse_line_with_commands_(subcommands_str);
        switch (res) {
        case CL_Quit_Requested:
            printf("Quit requested after %lu out of %lu repetitions.\n",
                   ix, repeat_count);
            return res;
        case CL_Parse_Fail:
            /* Don't repeat a subcommand that could not be parsed: */
            printf("Subcommand failed after %lu out of %lu repetitions.\n",
                   ix, repeat_count);
            return res;
        }
    }

    printf(" Completed %lu repetitions of '%s'.\n", ix, subcommands_str);

    return CL_Line_Handled;
        /* the whole line was handled, not only the 'x' = repeat prefix */
}

static int
handle_command_ (const char *const cmd_str,
                 const char **after_cmd,
                 unsigned long ul_numeric_prefix)
{
    static const char *const Numeric_Prefix_Ignored_Str =
        "{repeat count ignored}";
    static const char *const Help_Str =
        "\n q = Quit;  mutex commands: l = Lock, tl = Try Lock, u = Unlock;\n"
        "  semaphore commands: p = Post = increment (unlock) the semaphore,\n"
        "        tw = Try Wait = attempt to decrement (lock) the semaphore;\n"
        "  condition variable commands: s = Signal, b = Broadcast.\n"
        "Any command can have a numeric prefix; a few don't make sense without it:\n"
        "  x = repeat following commands (rest of line) <Numeric_Prefix> times;\n"
        "  z = sleep <Numeric_Prefix> seconds (in the command loop = the main thread);\n"
        "  d = set the Delay between waits (in threads) to <Numeric_Prefix> seconds.\n";

    struct timeval  sleep_cmd_tval;

    char  first_cmd_chr = cmd_str[0];

    *after_cmd = cmd_str + 1;

    switch (tolower(first_cmd_chr))
    {
    case 'q': /* Quit */
        printf("Quitting.\n");
        show_command_counters_();
        return CL_Quit_Requested;

    case 'h': /* Help */
        puts(Help_Str);
        return CL_Line_Handled;

    case 'x': /* repeat rest of line */
        return handle_repeat_command_(cmd_str + 1, ul_numeric_prefix);

    case 'z': /* sleep */
        sleep_cmd_tval.tv_sec = (long) ul_numeric_prefix;
        sleep_cmd_tval.tv_usec = 0;
        delay_("Z command", &sleep_cmd_tval);
        break;

    case 'd': /* Delay */
        delay_tval.tv_sec = (long) ul_numeric_prefix;
        delay_tval.tv_usec = 0;
        printf("Delay set to %ld seconds.\n",
               (long) delay_tval.tv_sec);
        break;

    case 'p': /* sem Post */
        do_sem_posts_(ul_numeric_prefix);
        break;

    case 's': /* condvar Signal */
        do_condvar_signals_(ul_numeric_prefix);
        break;

    case 'b': /* condvar Broadcast */
        do_condvar_broadcasts_(ul_numeric_prefix);
        break;

    case 'l': /* mutex Lock */
        if (ul_numeric_prefix != 1) {
            printf("%s ", Numeric_Prefix_Ignored_Str);
        }
        do_mutex_lock_();
        break;

    case 't': /* Try to lock mutex or semaphore */
        *after_cmd = cmd_str + 2;
        switch (tolower(cmd_str[1]))
        {
        case 'l': /* mutex Try Lock */
            if (ul_numeric_prefix != 1) {
                printf("%s ", Numeric_Prefix_Ignored_Str);
            }
            do_mutex_trylock_();
            break;
        case 'w': /* sem Try Wait */
            do_sem_trywaits_(ul_numeric_prefix);
            break;
        default:
            printf("Unrecognized command '%s': expected 'l' or 'w' after 't'.\n",
                   cmd_str);
            return CL_Parse_Fail;
        }
        break;

    case 'u': /* mutex Unlock */
        if (ul_numeric_prefix != 1) {
            printf("%s ", Numeric_Prefix_Ignored_Str);
        }
        do_mutex_unlock_();
        break;

    default:
        printf("Unrecognized command '%s'\n",
               cmd_str);
        return CL_Parse_Fail;
    }

    return CL_Normal;
}


static int
parse_command_ (const char *const cmd_str, const char **after_cmd)
{
    static const unsigned long  Max_Numeric_Prefix = 999999UL;

    char *end_numprefix = NULL;

    errno_t  strto_err;

    unsigned long  ul_numeric_prefix;

    errno = 0;
    ul_numeric_prefix = strtoul(cmd_str, &end_numprefix, 10);
    strto_err = errno;

    if (end_numprefix == cmd_str) {  /* No repeat count before command: default is one */
        ul_numeric_prefix = 1;
    }
    if (*end_numprefix == '\0') {  /* No command after repeat count */
        fprintf(stderr, "No command after repeat count %lu\n",
                ul_numeric_prefix);
    }
    if (strto_err != 0) {
        fprintf(stderr, "Parsing unsigned value '%s' failed with errno %d: %s\n",
                cmd_str, strto_err, strerror(strto_err));
    }
    if (ul_numeric_prefix > Max_Numeric_Prefix) {
        fprintf(stderr, "Numeric prefix too big (%lu, max is %lu)\n",
                ul_numeric_prefix, Max_Numeric_Prefix);
    }

    return handle_command_(end_numprefix, after_cmd, ul_numeric_prefix);
}

static int
parse_line_with_commands_ (const char *const commands_str)
{
    int  res;

    const char *after_cmd;
    const char *cmd = commands_str;

    /* Accept multiple commands on same line.
     * Do _not_ look for a command separator or terminator:
     * where the text of a command finishes, try to parse another.
     */
    while (*cmd) {
        while (isspace(*cmd)) {
            ++cmd;  /* skip leading whitespace */
        }
        if ('\0' == *cmd) {
            break;
        }

        after_cmd = NULL;
        res = parse_command_(cmd, &after_cmd);
        switch (res) {
        case CL_Line_Handled:
        case CL_Quit_Requested:
        case CL_Parse_Fail:
            return res;
        }

        assert(after_cmd);  /* Must have been set by parse_command_() */
        assert(cmd < after_cmd);
        cmd = after_cmd;
    }

    return CL_Line_Handled;  /* the whole line was handled */
}

static void
command_loop_ (void)
{
    size_t  len;
    size_t  pos;

    char  command_buf[64];
    char *in_res;
    int   res;

    printf("'h' for help\n");

    do {
        show_command_prompt_();

        in_res = fgets(command_buf, sizeof command_buf, stdin);
        if (NULL == in_res) {
            printf("fgets(..., stdin) returned NULL = reports error or end of file.\n");
            break;
        }

        len = strlen(command_buf);
        if (0 == len) {
            printf("fgets(..., stdin) reported success, but the command buffer is empty.\n");
            continue;
        }

        assert(len >= 1);
        for (pos = len - 1; pos > 0 && isspace(command_buf[pos]); --pos) {
            command_buf[pos] = '\0';  /* drop trailing whitespace */
        }

        res = parse_line_with_commands_(command_buf);
        if (CL_Quit_Requested == res) {
            break;
        }
    } while (1);

    printf("Interaction finished.\n");
}


static void *
condvar_wait_thread_func (void *arg)
{
    uex_thread_info *const tinfo = arg;

    unsigned long  n_wakeups = 0;

    errno_t  mutex_res;
    errno_t  cond_res;

    while (1) {
        mutex_res = pthread_mutex_lock(&demo_mutex);
        if (mutex_res == 0) {
            printf(" %s [%lu wakeups] pthread_mutex_lock() OK\n",
                   tinfo->config_str, n_wakeups);
        } else {
            printf(" %s [%lu wakeups] pthread_mutex_lock() failed, returning the errno value %d.\n",
                   tinfo->config_str, n_wakeups, mutex_res);
        }

        cond_res = pthread_cond_wait(&demo_condvar, &demo_mutex);
        if (cond_res == 0) {
            ++n_wakeups;
            printf(" %s [%lu wakeups] pthread_cond_wait() OK\n",
                   tinfo->config_str, n_wakeups);
        } else {
            printf(" %s [%lu wakeups] pthread_cond_wait() failed, returning the errno value %d.\n",
                   tinfo->config_str, n_wakeups, cond_res);
        }

        mutex_res = pthread_mutex_unlock(&demo_mutex);
        if (mutex_res == 0) {
            printf(" %s [%lu wakeups] pthread_mutex_unlock() OK\n",
                   tinfo->config_str, n_wakeups);
        } else {
            printf(" %s [%lu wakeups] pthread_mutex_unlock() failed, returning the errno value %d.\n",
                   tinfo->config_str, n_wakeups, mutex_res);
        }

        delay_(tinfo->config_str, &delay_tval);
    }

    return tinfo;
}

static void *
sem_wait_thread_func (void *arg)
{
    uex_thread_info *const tinfo = arg;

    unsigned long  n_acquired = 0;

    int      swait_res;
    errno_t  swait_err;

    while (1) {
        printf(" %s [acquired %lu times] Calling sem_wait()...\n",
               tinfo->config_str, n_acquired);

        swait_res = sem_wait(&sem);
        swait_err = errno;

        if (swait_res == 0) {
            ++n_acquired;
            printf(" %s [acquired %lu times] sem_wait() == 0: Semaphore acquired OK\n",
                   tinfo->config_str, n_acquired);
        } else {
            printf(" %s [acquired %lu times] sem_wait() failed with errno %d.\n",
                   tinfo->config_str, n_acquired, swait_err);
        }

        delay_(tinfo->config_str, &delay_tval);
    }

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

    if (0 == strncmp("cv", arg, 2)) {
        /* The prefix 'cv' stands for "Condition Variable": */
        pos = uex_add_thread_config(arg, NULL, &condvar_wait_thread_func);
        if (pos < 0) {
            fprintf(stderr, "Could not add thread config '%s'\n", arg);
            exit(7);
        }
    }
    else if (0 == strncmp("s", arg, 1)) {
        /* The prefix 's' stands for "Semaphore": */
        pos = uex_add_thread_config(arg, NULL, &sem_wait_thread_func);
        if (pos < 0) {
            fprintf(stderr, "Could not add thread config '%s'\n", arg);
            exit(8);
        }
    }
    else {
        return -1;
    }

    return 0;
}


static void
show_usage (FILE *out_stream)
{
    fprintf(out_stream, "Usage: [mutexattr:...] <Threads:zero_or_many(cv...|s...)>\n");
    fprintf(out_stream, "  The thread name prefix 'cv' stands for \"Condition Variable\".\n");
    fprintf(out_stream, "  The thread name prefix 's' stands for \"Semaphore\".\n");

    show_all_mutexattr_options(out_stream);
}

int
main (int argc, char* argv[])
{
    /* Requested Mutex Attributes, used if CLI argument is present: */
    pthread_mutexattr_t  req_mutexattr;
    pthread_mutexattr_t *req_mutexattr_p = NULL;

    mutexattr_parsing_info    mpinfo;
    mutexattr_setting_status  mstatus;

    int  arg_pos = 1;
    int  mattr_res;
    int  res;

    const char *data;

    if (arg_pos < argc) {
        if (0 == strncmp("mutexattr:", argv[arg_pos], 10)) {
            data = argv[arg_pos] + 10;

            memset(&mpinfo, 0, sizeof mpinfo);

            mattr_res = parse_mutexattr_str(&mpinfo, data);
            if (mattr_res != 0) {
                fprintf(stderr, "Unrecognized mutex attr '%s' (error %d).\n",
                        mpinfo.mp_rem, mattr_res);
                show_usage(stderr);
                return 2;
            }

            req_mutexattr_p = &req_mutexattr;
            pthread_mutexattr_init(req_mutexattr_p);

            memset(&mstatus, 0, sizeof mstatus);

            res = apply_mutexattr_settings(req_mutexattr_p, &mstatus, &mpinfo);
            if (res != 0) {
                fprintf(stderr, "Failed to set mutex attributes (error %d).\n",
                        res);
                return 3;
            }

            ++arg_pos;
        }
    }

    for (; arg_pos < argc; ++arg_pos) {
        res = handle_arg(argv[arg_pos]);
        if (res != 0) {
            fprintf(stderr, "Unrecognized argument '%s' (error %d).\n",
                    argv[arg_pos], res);
            show_usage(stderr);
            return 4;
        }
    }

    printf("Pid = %ld\n", (long) getpid());

    init_synchronization_objects_(req_mutexattr_p);

    if (req_mutexattr_p) {
        printf("Created the demo mutex with the following attributes\n"
               "(actually changed %u values out of %u found in the CLI argument):\n",
               mstatus.ms_n_changed, mpinfo.mp_n_parsed);
        show_mutexattr_settings(req_mutexattr_p, stdout);

        pthread_mutexattr_destroy(req_mutexattr_p);
    } else {
        printf("Created the demo mutex using defaults for all attributes (NULL attr object).\n");
    }

    uex_start_threads();

    command_loop_();

    printf("\n");
    uex_cancel_threads();
    uex_join_threads();

    printf("\nThe  %lu times.\n",
           0);

    return 0;
}
