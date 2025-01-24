/*
 * play-utils/util-ex-threads.c
 *
 * Utility module for Experiments/Examples/Exercises with Threads
 */

#include "util-ex-threads.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int  uex_n_threads = 0;

static uex_thread_config  uex_thread_configs[UEX_THREADS_MAX];

static uex_thread_info  uex_thread_structs[UEX_THREADS_MAX];
static pthread_t        uex_thread_ids[UEX_THREADS_MAX];


int
uex_get_n_threads (void)
{
    assert(0 <= uex_n_threads);
    assert(uex_n_threads <= UEX_THREADS_MAX);

    return uex_n_threads;
}

int
uex_find_thread_config_by_prefix (const char *config_str, size_t len_to_check)
{
    int  ix;

    assert(0 < len_to_check);
    assert(len_to_check <= UEX_THREAD_CONFIG_MAX);

    assert(0 <= uex_n_threads);
    assert(uex_n_threads <= UEX_THREADS_MAX);

    for (ix = 0; ix < uex_n_threads; ++ix) {
        if (0 == strncmp(config_str, uex_thread_configs[ix].uc_config_buf, len_to_check)) {
            return ix;
        }
    }

    return -1;
}

int
uex_add_thread_config (const char *config_str,
                       const pthread_attr_t *attr,
                       void * (*start_routine)(void *))
{
    const size_t  len = strlen(config_str);

    int  pos;

    if (len > UEX_THREAD_CONFIG_MAX) {
        return -1;
    }

    assert(0 <= uex_n_threads);

    if (uex_n_threads >= UEX_THREADS_MAX) {
        return -2;
    }

    pos = uex_n_threads++;

    memcpy(uex_thread_configs[pos].uc_config_buf, config_str, len);
    uex_thread_configs[pos].uc_config_buf[len] = '\0';

    uex_thread_configs[pos].uc_attr = attr;
    uex_thread_configs[pos].uc_start_routine = start_routine;

    return pos;
}

static int
uex_start_one_thread (int pos)
{
    char  err_buf[128];
    int   res;
    int   tcreate_res;

    assert(0 < uex_n_threads);
    assert(uex_n_threads <= UEX_THREADS_MAX);
    assert(0 <= pos);
    assert(pos < uex_n_threads);

    /* The entry must not be in use: */
    assert(NULL == uex_thread_structs[pos].config_str);

    /* DO NOT check: (NULL == uex_thread_ids[pos]);
     * zero is neither safer nor more portable than NULL for this purpose.
     * 'pthread_t' is NOT guaranteed to be a pointer or integer.
     */

    memset(&uex_thread_structs[pos], 0, sizeof uex_thread_structs[0]);

    uex_thread_structs[pos].config_str = uex_thread_configs[pos].uc_config_buf;

    tcreate_res = pthread_create(
                    &uex_thread_ids[pos],
                    uex_thread_configs[pos].uc_attr,
                    uex_thread_configs[pos].uc_start_routine,
                    &uex_thread_structs[pos]);
    if (tcreate_res != 0) {
        res = strerror_r(tcreate_res, err_buf, sizeof err_buf);
        if (res != 0) {
            fprintf(stderr, "[%d] strerror_r(%d) failed, returning the errno value %d.\n",
                    pos, tcreate_res, res);
            exit(99);
        }

        printf("[%d] pthread_create() failed for '%s',"
               " returning the errno value %d = %s\n",
               pos, uex_thread_configs[pos].uc_config_buf,
               tcreate_res, err_buf);
    }

    return tcreate_res;
}

void
uex_start_threads (void)
{
    unsigned long  n_success = 0;
    unsigned long  n_start_fail = 0;

    int  start_res;
    int  ix;

    assert(0 <= uex_n_threads);
    assert(uex_n_threads <= UEX_THREADS_MAX);

    for (ix = 0; ix < uex_n_threads; ++ix) {
        start_res = uex_start_one_thread(ix);
        if (start_res != 0) {
            ++n_start_fail;
        } else {
            ++n_success;
        }
    }

    printf("Started %lu, failed %lu\n",
           n_success, n_start_fail);
}


static int
uex_cancel_one_thread (int pos)
{
    char  err_buf[128];
    int   res;
    int   tcancel_res;

    const char *config_str;

    assert(0 < uex_n_threads);
    assert(uex_n_threads <= UEX_THREADS_MAX);
    assert(0 <= pos);
    assert(pos < uex_n_threads);

    config_str = uex_thread_configs[pos].uc_config_buf;

    tcancel_res = pthread_cancel(uex_thread_ids[pos]);

    if (tcancel_res == 0) {
        /* The cancellation request was made succesfully, but
         * the target thread is not required to act on it immediately.
         */
        printf("[%d] Cancellation request sent for thread '%s'.\n",
               pos, config_str);
        return 0;
    } else {
        res = strerror_r(tcancel_res, err_buf, sizeof err_buf);
        if (res != 0) {
            fprintf(stderr, "[%d] strerror_r(%d) failed, returning the errno value %d.\n",
                    pos, tcancel_res, res);
            exit(98);
        }

        printf("[%d] pthread_cancel() failed for '%s', returning the errno value %d = %s\n",
               pos, config_str, tcancel_res, err_buf);
        return -1;
    }
}

void
uex_cancel_threads (void)
{
    unsigned long  n_cancel_requested = 0;
    unsigned long  n_cancel_fail = 0;

    int  cancel_res;
    int  ix;

    assert(0 <= uex_n_threads);
    assert(uex_n_threads <= UEX_THREADS_MAX);

    for (ix = 0; ix < uex_n_threads; ++ix) {
        cancel_res = uex_cancel_one_thread(ix);
        switch (cancel_res)
        {
        case 0:
            ++n_cancel_requested;
            break;
        case -1:
            ++n_cancel_fail;
            break;
        default:
            abort();
        }
    }

    printf("Cancellation requests sent for %lu threads; could not send for %lu.\n",
           n_cancel_requested, n_cancel_fail);
}


static int
uex_join_one_thread (int pos)
{
    char  err_buf[128];
    int   res;
    int   tjoin_res;
    void *thr_retval = NULL;

    /*
     * Current thread's info (current = the thread we are about to join):
     */
    uex_thread_info *curr_info;
    const char      *config_str;

    assert(0 < uex_n_threads);
    assert(uex_n_threads <= UEX_THREADS_MAX);
    assert(0 <= pos);
    assert(pos < uex_n_threads);

    curr_info = &uex_thread_structs[pos];
    config_str = uex_thread_configs[pos].uc_config_buf;

    printf("[%d] Trying to join thread '%s' ...\n",
           pos, config_str);

    tjoin_res = pthread_join(uex_thread_ids[pos], &thr_retval);

    if (tjoin_res == 0) {
        if (PTHREAD_CANCELED == thr_retval) {
            printf("[%d] PTHREAD_CANCELED (thread '%s')\n",
                   pos, config_str);
            return 1;
        } else {
            if (curr_info == thr_retval) {
                printf("[%d] normal exit for thread '%s', expected value\n",
                       pos, config_str);
                return 0;
            } else {
                printf("[%d] normal exit for thread '%s', unexpected value\n",
                       pos, config_str);
                return 0;
            }
        }
    } else {
        res = strerror_r(tjoin_res, err_buf, sizeof err_buf);
        if (res != 0) {
            fprintf(stderr, "[%d] strerror_r(%d) failed, returning the errno value %d.\n",
                    pos, tjoin_res, res);
            exit(97);
        }

        printf("[%d] pthread_join() failed for '%s', returning the errno value %d = %s\n",
               pos, config_str, tjoin_res, err_buf);
        return 2;
    }
}

void
uex_join_threads (void)
{
    unsigned long  n_normal = 0;
    unsigned long  n_canceled = 0;
    unsigned long  n_join_fail = 0;

    int  join_res;
    int  ix;

    assert(0 <= uex_n_threads);
    assert(uex_n_threads <= UEX_THREADS_MAX);

    for (ix = 0; ix < uex_n_threads; ++ix) {
        join_res = uex_join_one_thread(ix);
        switch (join_res)
        {
        case 0:
            ++n_normal;
            break;
        case 1:
            ++n_canceled;
            break;
        case 2:
            ++n_join_fail;
            break;
        default:
            abort();
        }
    }

    printf("Normal exit: %lu, canceled: %lu; %lu could not be joined.\n",
           n_normal, n_canceled, n_join_fail);
}
