
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>


int
main (void)
{
    syscall(SYS_exit_group, 0);  /* Linux-specific */
    return 55;  /* should not matter --- not reached */
}
