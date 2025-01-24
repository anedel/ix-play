
#include <stdio.h>
#include <string.h>


#define LONGER_STRING(Fragment) \
  Fragment Fragment Fragment Fragment Fragment Fragment Fragment Fragment \
  Fragment Fragment Fragment Fragment Fragment Fragment Fragment Fragment \
  Fragment Fragment Fragment Fragment Fragment Fragment Fragment Fragment \
  Fragment Fragment Fragment Fragment Fragment Fragment Fragment Fragment


int
main (void)
{
    const char *const LongStr = LONGER_STRING(LONGER_STRING("123456789-") "\n");

    printf("za-printf3: length = %zu, value = '%s'\n",
           strlen(LongStr), LongStr);

    return 0;
}
