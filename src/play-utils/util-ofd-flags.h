/*
 * play-utils/util-ofd-flags.h
 * 
 * 
 * 'ofd' stands for "Open File Description"
 * (commonly called a "struct file" by Linux kernel developers)
 */

#include <stdio.h>


/*
 * Show the Open File Description Flags for the given fd;
 * tries to show all three kinds of OFD flags:
 *   - access modes
 *   - file creation flags
 *   - file status flags, the only flags that may be changed via fcntl()
 */
void  show_ofd_flags(int fd, FILE *out_stream);

int  set_ofd_status_flags(int fd, int status_flags);
int  clear_ofd_status_flags(int fd, int status_flags);
