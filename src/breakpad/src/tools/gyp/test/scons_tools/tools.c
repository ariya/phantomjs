#include <stdio.h>

int main(int argc, char *argv[])
{
#ifdef THIS_TOOL
  printf("Hello, world!\n");
#endif
  return 0;
}
