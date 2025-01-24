/*
 * play-utils/util-ex-threads.h
 *
 * Utility module for Experiments/Examples/Exercises with Threads
 */

#include <pthread.h>


#define UEX_THREADS_MAX  12  /* Maximum number of threads that can be tracked */
#define UEX_THREAD_CONFIG_MAX  31  /* Maximum length of config string */
#define UEX_THREAD_MESSAGE_MAX  67  /* Maximum length of stored message */


typedef struct {
    /*
     * Storing a non-NULL pointer here does _not_ imply ownership
     * of the thread attributes object; this module is _not_ responsible
     * for disposal --- could not do it safely anyway, since
     * it cannot tell whether the received object is shared or not
     * with other thread configuration records (instances of this struct).
     *
     * See uex_add_thread_config() below. 
     */
    const pthread_attr_t *uc_attr;

    void *              (*uc_start_routine)(void *);

    char  uc_config_buf[UEX_THREAD_CONFIG_MAX + 1];
} uex_thread_config;

typedef struct {
    /* Used for counting significant events, for debugging/experiment;
     * exact use depends on the thread's function and config string.
     */
    unsigned long long  count;

    const char *config_str;

    char  message_buf[UEX_THREAD_MESSAGE_MAX + 1];
} uex_thread_info;


int  uex_get_n_threads(void);

/*
 * Find entry with matching config string.
 * You must specify a prefix length ('len_to_check').
 * You can use this to enforce unique prefixes
 * (if prefix length is less than UEX_THREAD_CONFIG_MAX).
 */
int  uex_find_thread_config_by_prefix(const char *config_str, size_t len_to_check);

/*
 * Calling this function does not transfer ownership of 'attr' (if non-NULL)
 * to this module; the caller is responsible for proper disposal of
 * the thread attributes objects it provided, when they are not needed
 * anymore --- after uex_join_threads() it's sure we are finished.
 */
int  uex_add_thread_config(const char *config_str,
                           const pthread_attr_t *attr,
                           void * (*start_routine)(void *));

void  uex_start_threads(void);
void  uex_cancel_threads(void);
void  uex_join_threads(void);
