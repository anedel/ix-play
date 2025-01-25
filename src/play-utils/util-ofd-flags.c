/*
 * play-utils/util-ofd-flags.c
 *
 * 'ofd' stands for "Open File Description"
 * (commonly called a "struct file" by Linux kernel developers).
 *
 * This code is intended for educational purpose.
 * It's _not_ a good example of production code!
 * Error reporting and handling are especially unsuitable for production code;
 * designed for convenience in small programs: experiments and demonstrations.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2014--2025 Alexandru Nedel
 *
 * (add your name here when you make significant changes to this file,
 *  if you want to)
 */

#include "util-ofd-flags.h"

#include <fcntl.h>


static void
show_access_mode_ (int ofd_flags, FILE *out_stream)
{
    const int  access_mode = ofd_flags & O_ACCMODE;

    switch (access_mode)
    {
    case O_RDONLY:
        fprintf(out_stream, "Access mode: O_RDONLY");
        break;
    case O_WRONLY:
        fprintf(out_stream, "Access mode: O_WRONLY");
        break;
    case O_RDWR:
        fprintf(out_stream, "Access mode: O_RDWR");
        break;
    default:
        fprintf(out_stream, "Unexpected access mode");
        break;
    }

    fprintf(out_stream, " (%d)\n", access_mode);
}

static void
show_file_creation_flags_ (int ofd_flags, FILE *out_stream)
{
    fprintf(out_stream, "File creation flags (from 0x%x):", ofd_flags);

    if (ofd_flags & O_CREAT) {
        fprintf(out_stream, " O_CREAT,");
    }
    if (ofd_flags & O_EXCL) {
        fprintf(out_stream, " O_EXCL,");
    }
    if (ofd_flags & O_NOCTTY) {
        fprintf(out_stream, " O_NOCTTY,");
    }
    if (ofd_flags & O_TRUNC) {
        fprintf(out_stream, " O_TRUNC,");
    }
    if (ofd_flags & O_DIRECTORY) {
        fprintf(out_stream, " O_DIRECTORY,");
    }
    if (ofd_flags & O_NOFOLLOW) {
        fprintf(out_stream, " O_NOFOLLOW,");
    }

#ifdef O_LARGEFILE
    if (ofd_flags & O_LARGEFILE) {
        fprintf(out_stream, " O_LARGEFILE,");
    }
#endif

    fprintf(out_stream, "\n");
}

static void
show_file_status_flags_ (int ofd_flags, FILE *out_stream)
{
    if (ofd_flags & O_APPEND) {
        fprintf(out_stream, " O_APPEND,");
    }
    if (ofd_flags & O_NONBLOCK) {
        fprintf(out_stream, " O_NONBLOCK,");
    }
    if (ofd_flags & O_SYNC) {
        fprintf(out_stream, " O_SYNC,");
    }
    if (ofd_flags & O_DSYNC) {
        fprintf(out_stream, " O_DSYNC,");
    }
    if (ofd_flags & O_RSYNC) {
        fprintf(out_stream, " O_RSYNC,");
    }
    if (ofd_flags & O_ASYNC) {
        fprintf(out_stream, " O_ASYNC,");
    }

    fprintf(out_stream, "\n");
}


void
show_ofd_flags (int fd, FILE *out_stream)
{
    const int  ofd_flags = fcntl(fd, F_GETFL, 0);

    if (ofd_flags < 0) {
        perror("show_ofd_flags: fcntl F_GETFL");
        return;
    }

    fprintf(out_stream, "[%d] ", fd);
    show_access_mode_(ofd_flags, out_stream);
    show_file_creation_flags_(ofd_flags, out_stream); //does it work? or F_GETFL does not return these flags?

    fprintf(out_stream, "[%d] File status flags (from 0x%x):", fd, ofd_flags);
    show_file_status_flags_(ofd_flags, out_stream);
}


int
set_ofd_status_flags (int fd, int status_flags)
{
    int  res;
    int  new_flags;

    const int  old_flags = fcntl(fd, F_GETFL, 0);

    if (old_flags < 0) {
        perror("set_ofd_status_flags: fcntl F_GETFL");
        return -2;
    }

    new_flags = old_flags | status_flags;

    res = fcntl(fd, F_SETFL, new_flags);

    if (res < 0) {
        perror("set_ofd_status_flags: fcntl F_SETFL");
        return -3;
    }

    return old_flags;
}

int
clear_ofd_status_flags (int fd, int status_flags)
{
    int  res;
    int  new_flags;

    const int  old_flags = fcntl(fd, F_GETFL, 0);

    if (old_flags < 0) {
        perror("clear_ofd_status_flags: fcntl F_GETFL");
        return -6;
    }

    new_flags = old_flags & ~status_flags;

    res = fcntl(fd, F_SETFL, new_flags);

    if (res < 0) {
        perror("clear_ofd_status_flags: fcntl F_SETFL");
        return -7;
    }

    return old_flags;
}
