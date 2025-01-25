/*
 * play-utils/util-mutexattr.c
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2014--2025 Alexandru Nedel
 *
 * (add your name here when you make significant changes to this file,
 *  if you want to)
 */

#include "util-mutexattr.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>  /* for 'INT_MAX', etc. */
#include <stdlib.h>
#include <string.h>

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


struct mp_attr_value {
    int         av_value;
    const char *av_api_name;
    const char *av_ui_name;
};

/* Convenience macro for initializing the first two members: */
#define MP_AV_(ApiName)  ApiName, #ApiName


static const struct mp_attr_value *
find_av_by_value_ (
        const struct mp_attr_value entries[], size_t n_entries,
        int value)
{
    size_t  ix;

    for (ix = 0; ix < n_entries; ++ix) {
        if (entries[ix].av_value == value) {
            return &entries[ix];
        }
    }

    return NULL;
}


static const struct mp_attr_value  Type_Attr_Values[] = {
    { MP_AV_(PTHREAD_MUTEX_NORMAL),     "normal" },
    { MP_AV_(PTHREAD_MUTEX_RECURSIVE),  "recursive" },
    { MP_AV_(PTHREAD_MUTEX_RECURSIVE),  "rec" },  /* alternate name */
    /*
     * If there are shorter UI name(s) for same API name and value
     * they should come after the longer name(s), so
     * decoding (some 'show...' function) will use the longest name,
     * which is usually clearer or recommended by a standard
     * (if it is not better, put first the name you prefer to show).
     */
    { MP_AV_(PTHREAD_MUTEX_ERRORCHECK), "errorcheck" },
    { MP_AV_(PTHREAD_MUTEX_DEFAULT), "Default (should be equal to one of the above)" }
        /* 'Default' is for documentation (would be very inconvenient to enter) */
};
static const size_t  N_Type_Attr_Values =
    sizeof Type_Attr_Values / sizeof Type_Attr_Values[0];


static const struct mp_attr_value  Robust_Attr_Values[] = {
    { MP_AV_(PTHREAD_MUTEX_STALLED), "stalled" },  /* the default */
    { MP_AV_(PTHREAD_MUTEX_ROBUST),  "robust" }
};
static const size_t  N_Robust_Attr_Values =
    sizeof Robust_Attr_Values / sizeof Robust_Attr_Values[0];


static const struct mp_attr_value  ProcessShared_Attr_Values[] = {
    { MP_AV_(PTHREAD_PROCESS_PRIVATE), "private" },  /* the default */
    { MP_AV_(PTHREAD_PROCESS_SHARED),  "shared" }
};
static const size_t  N_ProcessShared_Attr_Values =
    sizeof ProcessShared_Attr_Values / sizeof ProcessShared_Attr_Values[0];


#ifdef __USE_UNIX98  /* mutex protocols are available: */

static const struct mp_attr_value  Protocol_Attr_Values[] = {
    { MP_AV_(PTHREAD_PRIO_NONE),    "none" },  /* the default */
    { MP_AV_(PTHREAD_PRIO_INHERIT), "inherit" },
    { MP_AV_(PTHREAD_PRIO_PROTECT), "protect" }
};
static const size_t  N_Protocol_Attr_Values =
    sizeof Protocol_Attr_Values / sizeof Protocol_Attr_Values[0];

#endif /* __USE_UNIX98, for mutex protocols. */


#undef MP_AV_


enum {
    Attr_Unchanged = 0,
    Attr_Set_OK = 1,
    Attr_Set_Failed = -2  /* errors are traditionally negative numbers */
};


static int
set_Type_attr_value_ (pthread_mutexattr_t *dest_attr, int type)
{
    int  old_val;
    int  res;

    res = pthread_mutexattr_gettype(dest_attr, &old_val);
    assert(0 == res);

    if (old_val == type) {
        return Attr_Unchanged;
    }

    res = pthread_mutexattr_settype(dest_attr, type);
    if (res != 0) {
        fprintf(stderr, "pthread_mutexattr_settype(%d) failed, returning the errno value %d = %s\n",
                type, res, strerror(res));
        return Attr_Set_Failed;
    }

    return Attr_Set_OK;
}

static int
put_Type_attr_value_ (mutexattr_parsing_info *mpinfo, int type)
{
    ++mpinfo->mp_in_counts.ma_type;
    mpinfo->mp_in_values.ma_type = type;
    return 0;
}

static int
parse_Type_attr_value_ (mutexattr_parsing_info *mpinfo, const char *input)
{
    const struct mp_attr_value *av = NULL;

    const char *known_str;
    size_t      known_len;

    size_t  ix;

    for (ix = 0; ix < N_Type_Attr_Values; ++ix) {
        known_str = Type_Attr_Values[ix].av_ui_name;
        known_len = strlen(known_str);

        if (0 == strncmp(known_str, input, known_len)) {
            mpinfo->mp_rem = input + known_len;
            av = &Type_Attr_Values[ix];
            break;
        }
    }

    if (NULL == av) {
        return -10;
    }

    return put_Type_attr_value_(mpinfo, av->av_value);
}


static int
set_Robust_attr_value_ (pthread_mutexattr_t *dest_attr, int robust)
{
    int  old_val;
    int  res;

    res = pthread_mutexattr_getrobust(dest_attr, &old_val);
    assert(0 == res);

    if (old_val == robust) {
        return Attr_Unchanged;
    }

    res = pthread_mutexattr_setrobust(dest_attr, robust);
    if (res != 0) {
        fprintf(stderr, "pthread_mutexattr_setrobust(%d) failed, returning the errno value %d = %s\n",
                robust, res, strerror(res));
        return Attr_Set_Failed;
    }

    return Attr_Set_OK;
}

static int
put_Robust_attr_value_ (mutexattr_parsing_info *mpinfo, int robust)
{
    ++mpinfo->mp_in_counts.ma_robust;
    mpinfo->mp_in_values.ma_robust = robust;
    return 0;
}

static int
parse_Robust_attr_value_ (mutexattr_parsing_info *mpinfo, const char *input)
{
    const struct mp_attr_value *av = NULL;

    const char *known_str;
    size_t      known_len;

    size_t  ix;

    for (ix = 0; ix < N_Robust_Attr_Values; ++ix) {
        known_str = Robust_Attr_Values[ix].av_ui_name;
        known_len = strlen(known_str);

        if (0 == strncmp(known_str, input, known_len)) {
            mpinfo->mp_rem = input + known_len;
            av = &Robust_Attr_Values[ix];
            break;
        }
    }

    if (NULL == av) {
        return -20;
    }

    return put_Robust_attr_value_(mpinfo, av->av_value);
}


static int
set_ProcessShared_attr_value_ (pthread_mutexattr_t *dest_attr, int pshared)
{
    int  old_val;
    int  res;

    res = pthread_mutexattr_getpshared(dest_attr, &old_val);
    assert(0 == res);

    if (old_val == pshared) {
        return Attr_Unchanged;
    }

    res = pthread_mutexattr_setpshared(dest_attr, pshared);
    if (res != 0) {
        fprintf(stderr, "pthread_mutexattr_setpshared(%d) failed, returning the errno value %d = %s\n",
                pshared, res, strerror(res));
        return Attr_Set_Failed;
    }

    return Attr_Set_OK;
}

static int
put_ProcessShared_attr_value_ (mutexattr_parsing_info *mpinfo, int pshared)
{
    ++mpinfo->mp_in_counts.ma_pshared;
    mpinfo->mp_in_values.ma_pshared = pshared;
    return 0;
}

static int
parse_ProcessShared_attr_value_ (mutexattr_parsing_info *mpinfo,
                                 const char *input)
{
    const struct mp_attr_value *av = NULL;

    const char *known_str;
    size_t      known_len;

    size_t  ix;

    for (ix = 0; ix < N_ProcessShared_Attr_Values; ++ix) {
        known_str = ProcessShared_Attr_Values[ix].av_ui_name;
        known_len = strlen(known_str);

        if (0 == strncmp(known_str, input, known_len)) {
            mpinfo->mp_rem = input + known_len;
            av = &ProcessShared_Attr_Values[ix];
            break;
        }
    }

    if (NULL == av) {
        return -30;
    }

    return put_ProcessShared_attr_value_(mpinfo, av->av_value);
}


#ifdef __USE_UNIX98  /* mutex protocols are available: */

static int
set_Protocol_attr_value_ (pthread_mutexattr_t *dest_attr, int protocol)
{
    int  old_val;
    int  res;

    res = pthread_mutexattr_getprotocol(dest_attr, &old_val);
    assert(0 == res);

    if (old_val == protocol) {
        return Attr_Unchanged;
    }

    res = pthread_mutexattr_setprotocol(dest_attr, protocol);
    if (res != 0) {
        fprintf(stderr, "pthread_mutexattr_setprotocol(%d) failed, returning the errno value %d = %s\n",
                protocol, res, strerror(res));
        return Attr_Set_Failed;
    }

    return Attr_Set_OK;
}

static int
put_Protocol_attr_value_ (mutexattr_parsing_info *mpinfo, int protocol)
{
    ++mpinfo->mp_in_counts.ma_protocol;
    mpinfo->mp_in_values.ma_protocol = protocol;
    return 0;
}

#endif /* __USE_UNIX98, for mutex protocols. */

static int
parse_Protocol_attr_value_ (mutexattr_parsing_info *mpinfo, const char *input)
{
#ifdef __USE_UNIX98  /* mutex protocols are available: */

    const struct mp_attr_value *av = NULL;

    const char *known_str;
    size_t      known_len;

    size_t  ix;

    for (ix = 0; ix < N_Protocol_Attr_Values; ++ix) {
        known_str = Protocol_Attr_Values[ix].av_ui_name;
        known_len = strlen(known_str);

        if (0 == strncmp(known_str, input, known_len)) {
            mpinfo->mp_rem = input + known_len;
            av = &Protocol_Attr_Values[ix];
            break;
        }
    }

    if (NULL == av) {
        return -40;
    }

    return put_Protocol_attr_value_(mpinfo, av->av_value);

#else
    return -1;
#endif /* __USE_UNIX98, for mutex protocols. */
}


static int
set_PriorityCeiling_attr_value_ (pthread_mutexattr_t *dest_attr,
                                 int prioceiling)
{
    int  old_val;
    int  res;

    res = pthread_mutexattr_getprioceiling(dest_attr, &old_val);
    /*
     * Not using 'assert(0 == res)' here because POSIX says that
     * pthread_mutexattr_getprioceiling() may fail returning EPERM, if
     * the caller does not have the privilege to perform the operation.
     * Maybe this should not apply to ..._get...; maybe it was
     * intended to apply only to the ..._set... function, but
     * the standard (IEEE Std 1003.1, 2013 Edition) is quite clear.
     */
    if (res != 0) {
        fprintf(stderr, "pthread_mutexattr_getprioceiling() failed, returning the errno value %d = %s\n",
                res, strerror(res));
        return Attr_Set_Failed;
    }

    if (old_val == prioceiling) {
        return Attr_Unchanged;
    }

    res = pthread_mutexattr_setprioceiling(dest_attr, prioceiling);
    if (res != 0) {
        fprintf(stderr, "pthread_mutexattr_setprioceiling(%d) failed, returning the errno value %d = %s\n",
                prioceiling, res, strerror(res));
        return Attr_Set_Failed;
    }

    return Attr_Set_OK;
}

static int
put_PriorityCeiling_attr_value_ (mutexattr_parsing_info *mpinfo,
                                 int prioceiling)
{
    ++mpinfo->mp_in_counts.ma_prioceiling;
    mpinfo->mp_in_values.ma_prioceiling = prioceiling;
    return 0;
}

static int
parse_PriorityCeiling_attr_value_ (mutexattr_parsing_info *mpinfo,
                                   const char *input)
{
    char *end = NULL;  /* for strto...() functions */

    errno_t  strto_err;

    long  num;

    errno = 0;
    num = strtol(input, &end, 10);
    strto_err = errno;

    if (end == input) {
        fprintf(stderr, "Could not parse Priority Ceiling text '%s'\n",
                input);
        return -51;
    }
    if (strto_err != 0) {
        fprintf(stderr, "Parsing Priority Ceiling text '%s' failed with errno %d: %s\n",
                input, strto_err, strerror(strto_err));
        return -53;
    }

    if (num <= 0) {
        fprintf(stderr, "Priority Ceiling must be positive (got %ld, original text was '%s')\n",
                num, input);
        return -54;
    }
    if (num >= INT_MAX) {
        fprintf(stderr, "Priority Ceiling too big: %ld, original text was '%s'\n",
                num, input);
        return -55;
    }

    mpinfo->mp_rem = end;

    return put_PriorityCeiling_attr_value_(mpinfo, (int) num);
}


static int
parse_one_mutexattr_ (mutexattr_parsing_info *mpinfo, const char *input)
{
    const char *data;

    if (0 == strncmp("type=", input, 5)) {
        data = input + 5;
        return parse_Type_attr_value_(mpinfo, data);
    }
    else if (0 == strncmp("t=", input, 2)) {
        data = input + 2;
        return parse_Type_attr_value_(mpinfo, data);
    }
    else if (0 == strncmp("robust=", input, 7)) {
        data = input + 7;
        return parse_Robust_attr_value_(mpinfo, data);
    }
    else if (0 == strncmp("robust", input, 6)) {
        mpinfo->mp_rem = input + 6;
        return put_Robust_attr_value_(mpinfo, PTHREAD_MUTEX_ROBUST);
    }
    else if (0 == strncmp("r", input, 1)) {
        mpinfo->mp_rem = input + 1;
        return put_Robust_attr_value_(mpinfo, PTHREAD_MUTEX_ROBUST);
    }
    else if (0 == strncmp("pshared=", input, 8)) {
        data = input + 8;
        return parse_ProcessShared_attr_value_(mpinfo, data);
    }
    else if (0 == strncmp("pshared", input, 7)) {
        mpinfo->mp_rem = input + 7;
        return put_ProcessShared_attr_value_(mpinfo, PTHREAD_PROCESS_SHARED);
    }
    else if (0 == strncmp("s", input, 1)) {
        mpinfo->mp_rem = input + 1;
        return put_ProcessShared_attr_value_(mpinfo, PTHREAD_PROCESS_SHARED);
    }
    else if (0 == strncmp("protocol=", input, 9)) {
        data = input + 9;
        return parse_Protocol_attr_value_(mpinfo, data);
    }
    else if (0 == strncmp("p=", input, 2)) {
        data = input + 2;
        return parse_Protocol_attr_value_(mpinfo, data);
    }
    else if (0 == strncmp("prioceiling=", input, 12)) {
        data = input + 12;
        return parse_PriorityCeiling_attr_value_(mpinfo, data);
    }
    else if (0 == strncmp("c=", input, 2)) {
        data = input + 2;
        return parse_PriorityCeiling_attr_value_(mpinfo, data);
    }
    else {
        return -5;
    }
}

int
parse_mutexattr_str (mutexattr_parsing_info *mpinfo, const char *input)
{
    const char *curr = input;
    int  res;

    while (*curr) {
        res = parse_one_mutexattr_(mpinfo, curr);
        if (res != 0) {
            mpinfo->mp_rem = curr;
            return res;
        }

        ++mpinfo->mp_n_parsed;

        curr = mpinfo->mp_rem;

        if (*curr) {
            /*
             * Expect comma as attribute separator or terminator:
             */
            if (',' == *curr) {
                ++curr;
            } else {
                ;
                return -7;
            }
        }
    }

    return 0;
}


static void
update_mstatus_ (mutexattr_setting_status *mstatus, int res)
{
    switch (res)
    {
    case Attr_Unchanged:
        ++mstatus->ms_n_unchanged;
        break;
    case Attr_Set_OK:
        ++mstatus->ms_n_changed;
        break;
    case Attr_Set_Failed:
        ++mstatus->ms_n_failed;
        break;
    default:
        abort();
    }
}

int
apply_mutexattr_settings (pthread_mutexattr_t *dest_attr,
                          mutexattr_setting_status *mstatus,
                          const mutexattr_parsing_info *mpinfo)
{
    int  res;

    if (mpinfo->mp_in_counts.ma_type > 0) {
        res = set_Type_attr_value_(dest_attr, mpinfo->mp_in_values.ma_type);
        update_mstatus_(mstatus, res);
    }

    if (mpinfo->mp_in_counts.ma_robust > 0) {
        res = set_Robust_attr_value_(dest_attr, mpinfo->mp_in_values.ma_robust);
        update_mstatus_(mstatus, res);
    }

    if (mpinfo->mp_in_counts.ma_pshared > 0) {
        res = set_ProcessShared_attr_value_(dest_attr, mpinfo->mp_in_values.ma_pshared);
        update_mstatus_(mstatus, res);
    }

#ifdef __USE_UNIX98  /* mutex protocols are available: */
    if (mpinfo->mp_in_counts.ma_protocol > 0) {
        res = set_Protocol_attr_value_(dest_attr, mpinfo->mp_in_values.ma_protocol);
        update_mstatus_(mstatus, res);
    }
#endif /* __USE_UNIX98, for mutex protocols. */

    if (mpinfo->mp_in_counts.ma_prioceiling > 0) {
        res = set_PriorityCeiling_attr_value_(dest_attr, mpinfo->mp_in_values.ma_prioceiling);
        update_mstatus_(mstatus, res);
    }

    return 0;
}


static void
show_av_entry_ (const struct mp_attr_value *av,
                const char *message_preamble,
                FILE *out_stream)
{
    fprintf(out_stream, "%s %s -> %s = %d\n",
            message_preamble, av->av_ui_name, av->av_api_name, av->av_value);
}


void
show_mutexattr_settings (const pthread_mutexattr_t *attr, FILE *out_stream)
{
    const struct mp_attr_value *av;

    int  val;
    int  res;

    val = 9871;  /* should be overwritten by next call */
    res = pthread_mutexattr_gettype(attr, &val);
    assert(0 == res);

    av = find_av_by_value_(Type_Attr_Values, N_Type_Attr_Values, val);
    if (av) {
        show_av_entry_(av, "  Type:", out_stream);
    } else {
        fprintf(out_stream, "  Name not known for Type attribute value %d.\n",
                val);
    }

    val = 9872;  /* should be overwritten by next call */
    res = pthread_mutexattr_getrobust(attr, &val);
    assert(0 == res);

    av = find_av_by_value_(Robust_Attr_Values, N_Robust_Attr_Values, val);
    if (av) {
        show_av_entry_(av, "  Robust:", out_stream);
    } else {
        fprintf(out_stream, "  Name not known for Robust attribute value %d.\n",
                val);
    }

    val = 9873;  /* should be overwritten by next call */
    res = pthread_mutexattr_getpshared(attr, &val);
    assert(0 == res);

    av = find_av_by_value_(ProcessShared_Attr_Values, N_ProcessShared_Attr_Values, val);
    if (av) {
        show_av_entry_(av, "  Process-Shared:", out_stream);
    } else {
        fprintf(out_stream, "  Name not known for Process-Shared attribute value %d.\n",
                val);
    }

#ifdef __USE_UNIX98  /* mutex protocols are available: */
    val = 9874;  /* should be overwritten by next call */
    res = pthread_mutexattr_getprotocol(attr, &val);
    assert(0 == res);

    av = find_av_by_value_(Protocol_Attr_Values, N_Protocol_Attr_Values, val);
    if (av) {
        show_av_entry_(av, "  Protocol:", out_stream);
    } else {
        fprintf(out_stream, "  Name not known for Protocol attribute value %d.\n",
                val);
    }
#endif /* __USE_UNIX98, for mutex protocols. */

    val = 9876;  /* should be overwritten by next call */
    res = pthread_mutexattr_getprioceiling(attr, &val);
    /*
     * Not using 'assert(0 == res)' here because POSIX says that
     * pthread_mutexattr_getprioceiling() may fail returning EPERM, if
     * the caller does not have the privilege to perform the operation.
     * Maybe this should not apply to ..._get...; maybe it was
     * intended to apply only to the ..._set... function, but
     * the standard (IEEE Std 1003.1, 2013 Edition) is quite clear.
     */
    if (0 == res) {
        fprintf(out_stream, "  Priority Ceiling: %d\n", val);
    } else {
        fprintf(out_stream, "  pthread_mutexattr_getprioceiling() failed, returning the errno value %d = %s\n",
                res, strerror(res));
    }
}


static void
show_av_entries_ (
        const struct mp_attr_value entries[], size_t n_entries,
        FILE *out_stream)
{
    static const char *const Val_Preamble = " ";
        /* one space is enough, because show_av_entry_() adds one too */

    size_t  ix;

    for (ix = 0; ix < n_entries; ++ix) {
        show_av_entry_(&entries[ix], Val_Preamble, out_stream);
    }
}

void
show_all_mutexattr_options (FILE *out_stream)
{
    fprintf(out_stream, "\nMutex Type ('type=...', or 't=...'):\n");
    show_av_entries_(Type_Attr_Values, N_Type_Attr_Values, out_stream);

    fprintf(out_stream, "\nMutex could be Robust (use 'r', 'robust', or explicit 'robust=...'):\n");
    show_av_entries_(Robust_Attr_Values, N_Robust_Attr_Values, out_stream);

    fprintf(out_stream, "\nMutex could be Process-Shared (use 's', 'pshared', or explicit 'pshared=...'):\n");
    show_av_entries_(ProcessShared_Attr_Values, N_ProcessShared_Attr_Values, out_stream);

#ifdef __USE_UNIX98  /* mutex protocols are available: */
    fprintf(out_stream, "\nMutex Protocol ('protocol=...', or 'p=...'):\n");
    show_av_entries_(Protocol_Attr_Values, N_Protocol_Attr_Values, out_stream);
#endif

    fprintf(out_stream, "\nMutex Priority Ceiling ('prioceiling=...', or 'c=...'):\n"
            "  positive integer.\n");
}
