/*
 * play-utils/util-mutexattr.h
 */

#include <pthread.h>
#include <stdio.h>


typedef struct {
    int  ma_type;
    int  ma_robust;
    int  ma_pshared;
    int  ma_protocol;
    int  ma_prioceiling;
} mutexattr_values;


typedef struct {
    const char *mp_rem;
        /* Remaining input: not parsed yet, or not successfully parsed. */

    mutexattr_values  mp_in_values;
    mutexattr_values  mp_in_counts;

    /*
     * Two counters of attributes/settings are most useful/interesting:
     *    (1) Parsed successfully = that part of the input was valid;
     *    (2) actually Changed in the destination attributes object =
     *            that part of the input mattered (was not redundant).
     *
     * If only first counter is incremented for a valid attribute/setting
     * from the input, it means that the parsed value matched the one
     * already configured in the destination attributes object
     * given by the first argument (pthread_mutexattr_t *dest_attr)
     * of apply_mutexattr_settings().
     */
    unsigned  mp_n_parsed;   /* (1) Parsed successfully */
} mutexattr_parsing_info;

typedef struct {
    unsigned  ms_n_unchanged;
    unsigned  ms_n_changed;  /* (2) actually Changed, see above comment */
    unsigned  ms_n_failed;
} mutexattr_setting_status;


/*
 * Returns zero for success.
 * Returns a negative value for failure, and updates the string position
 *      passed in first arg.
 */
int  parse_mutexattr_str(mutexattr_parsing_info *mpinfo,
                         const char *input);

/*
 * 'dest_attr' must have been initialized with pthread_mutexattr_init().
 *
 * We don't want to initialize the destination attributes object
 * in this function because we may want to use this function on
 * existing, partially configured, mutex attributes objects.
 *
 * Returns zero for success.
 * Returns a negative value for failure.
 */
int  apply_mutexattr_settings(pthread_mutexattr_t *dest_attr,
                              mutexattr_setting_status *mstatus,
                              const mutexattr_parsing_info *mpinfo);

void  show_mutexattr_settings(const pthread_mutexattr_t *attr, FILE *out_stream);
void  show_all_mutexattr_options(FILE *out_stream);
