
#include <unistd.h>  /* for _exit(): it's not in 'stdlib.h', but _Exit() is! */


int
main (void)
{
    _exit(0);
    return 33;  /* should not matter --- not reached */
}
