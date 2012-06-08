#include <stdio.h>

int main(int argc, char *argv[])
{
#ifdef FOO
  printf("FOO is defined\n");
#endif
  printf("VALUE is %d\n", VALUE);
  return 0;
}
