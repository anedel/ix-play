
#include <stdlib.h>


int
main (void)
{
    _Exit(0);
    return 44;  /* should not matter --- not reached */
}
