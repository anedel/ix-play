
#include <stdio.h>
#include <unistd.h>


#define NEXT_PROG  "za-rtsig-wait-sync"

int
main (void)
{
    printf("za-exec4 Pid = %ld\n", (long) getpid());

    sleep(5);

    execl(NEXT_PROG, NEXT_PROG, NULL);
    perror("execl('" NEXT_PROG "')");

    return 1;  /* failure, because exec...() should not return if succeeds */
}
