/*
 * play-utils/util-input.c
 * 
 */

#include "util-input.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

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


#define DISCARD_BUF_MAX  1024


static int
get_first_input_char_ (int in_fd, FILE *report_stream)
{
    char  buf[8];

    const ssize_t  n_read = read(in_fd, buf, sizeof buf);

    if (n_read < 0) {
        perror("get_first_input_char_: read");
        return -11;
    }
    if (n_read == 0) {
        return -12;
    }

    return buf[0];
}

void
discard_input (int in_fd, FILE *report_stream, size_t bufsize)
{
    unsigned long long  n_discarded = 0;
    char  buf[DISCARD_BUF_MAX];

    ssize_t  n_read;
    errno_t  read_err;

    assert(bufsize <= DISCARD_BUF_MAX);

    fprintf(report_stream, "\nDiscarding input from fd %d, buf[%zu]: ",
            in_fd, bufsize);
    fflush(report_stream);

    do {
        n_read = read(in_fd, buf, bufsize);
        read_err = errno;

        fprintf(report_stream, "%zd, ", n_read);
        fflush(report_stream);

        if (n_read < 0) {
            if (EAGAIN != read_err) {
                fprintf(stderr, "discard_input: read(%d) error %d = %s\n",
                        in_fd, read_err, strerror(read_err));
            }

            return;
        }

        n_discarded += n_read;
    } while (n_read > 0);

    fprintf(report_stream, "Discarded %llu bytes.\n", n_discarded);
    fflush(report_stream);
}

int
discard_pending_input (int in_fd, FILE *report_stream)
{
    int  res;
    int  new_flags;

    const int  old_flags = fcntl(in_fd, F_GETFL, 0);

    if (old_flags < 0) {
        perror("discard_pending_input: fcntl F_GETFL");
        return -2;
    }

    if ((old_flags & O_NONBLOCK) == O_NONBLOCK) {
        discard_input(in_fd, report_stream, DISCARD_BUF_MAX);
    } else {
        new_flags = old_flags | O_NONBLOCK;

        res = fcntl(in_fd, F_SETFL, new_flags);
        if (res < 0) {
            perror("discard_pending_input: fcntl F_SETFL setting O_NONBLOCK");
            return -3;
        }

        discard_input(in_fd, report_stream, DISCARD_BUF_MAX);

        res = fcntl(in_fd, F_SETFL, old_flags);
        if (res < 0) {
            perror("discard_pending_input: fcntl F_SETFL restoring");
            return -4;
        }
    }

    return 0;
}

int
wait_for_input_char (int in_fd, FILE *report_stream)
{
    int  in_ch;
    int  res;
    int  new_flags;

    const int  old_flags = fcntl(in_fd, F_GETFL, 0);

    if (old_flags < 0) {
        perror("wait_for_input_char: fcntl F_GETFL");
        return -6;
    }

    if ((old_flags & O_NONBLOCK) == 0) { /* Already blocking, no change needed */
        in_ch = get_first_input_char_(in_fd, report_stream);
    } else {
        new_flags = old_flags & ~O_NONBLOCK;

        res = fcntl(in_fd, F_SETFL, new_flags);
        if (res < 0) {
            perror("wait_for_input_char: fcntl F_SETFL clearing O_NONBLOCK");
            return -7;
        }

        in_ch = get_first_input_char_(in_fd, report_stream);

        res = fcntl(in_fd, F_SETFL, old_flags);
        if (res < 0) {
            perror("wait_for_input_char: fcntl F_SETFL restoring");
            return -8;
        }
    }

    assert(0 < in_ch);
    assert(in_ch < 128);

    return in_ch;
}

int
pause_prompt (int in_fd, FILE *out_stream, const char *prompt)
{
    
    discard_pending_input(in_fd, out_stream);

    fprintf(out_stream, "\n%s\n", prompt);
    fflush(out_stream);

    return wait_for_input_char(in_fd, out_stream);
}
