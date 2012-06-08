#include <stdio.h>

extern void func1(void);

int main(int argc, char *argv[]) {
  printf("hello from link1\n");
  func1();
  return 0;
}
