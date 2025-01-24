
#include <stdio.h>
#include <unistd.h>


#define NEXT_PROG  "za-exec2"

int
main (void)
{
    printf("za-exec1 Pid = %ld\n", (long) getpid());

    sleep(5);

    execl(NEXT_PROG, NEXT_PROG, NULL);
    perror("execl('" NEXT_PROG "')");

    return 1;  /* failure, because exec...() should not return if succeeds */
}
