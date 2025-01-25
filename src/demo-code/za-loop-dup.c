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
#include <stdio.h>
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


int
main (int argc, char* argv[])
{
    int      last_good_fd = -1;
    int      dup_fd;
    errno_t  dup_err;

    while (1) {
        dup_fd = dup(0);
        dup_err = errno;

        if (dup_fd < 0) {
            printf("dup(0) returned %d, errno %d = %s\n",
                   dup_fd, dup_err, strerror(dup_err));
            break;
        } else {
            last_good_fd = dup_fd;
        }
    }

    printf("\nLast good fd = %d\n", last_good_fd);

    return 0;
}
