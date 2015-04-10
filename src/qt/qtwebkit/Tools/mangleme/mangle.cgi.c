/*

   HTML manglizer
   --------------
   Copyright (C) 2004 by Michal Zalewski <lcamtuf@coredump.cx>

   HTML manglizer library. Logs random seeds to error-log; find the last entry before
   crash, then pass it to remangle.cgi to reproduce the problem.

 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tags.h"

#define R(x) (rand() % (x))

#define MAXTCOUNT 100
#define MAXPCOUNT 20
#define MAXSTR2   80

void make_up_value(void) {
  char c=R(2);
  
  if (c) putchar('"');

  switch (R(31)) {

    case 0: printf("javascript:"); make_up_value(); break;
//    case 1: printf("jar:"); make_up_value(); break;
    case 2: printf("mk:"); make_up_value(); break;
    case 3: printf("file:"); make_up_value(); break;
    case 4: printf("http:"); make_up_value(); break;
    case 5: printf("about:"); make_up_value(); break;
    case 6: printf("_blank"); break;
    case 7: printf("_self"); break;
    case 8: printf("top"); break;
    case 9: printf("left"); break;
    case 10: putchar('&'); make_up_value(); putchar(';'); break;
    case 11: make_up_value(); make_up_value(); break;
 
    case 12 ... 20: {
        int c = R(10) ? R(10) : (1 + R(MAXSTR2) * R(MAXSTR2));
        char* x = malloc(c);
        memset(x,R(256),c);
        fwrite(x,c,1,stdout);
        free(x);
        break;
      }
      
    case 21: printf("%s","%n%n%n%n%n%n"); break;
    case 22: putchar('#'); break;
    case 23: putchar('*'); break;
    default: if (R(2)) putchar('-'); printf("%d",rand()); break;
    
  }

  if (c) putchar('"');

}
  

void random_tag(void) {
  int tn, tc;
  
  do tn = R(MAXTAGS); while (!tags[tn][0]);
  tc = R(MAXPCOUNT) + 1;
  
  putchar('<');
  
  switch (R(10)) {
    case 0: putchar(R(256)); break;
    case 1: putchar('/');
  }
  
  printf("%s", tags[tn][0]);
  
  while (tc--) {
    int pn;
    switch (R(32)) {
      case 0: putchar(R(256)); 
      case 1: break;
      default: putchar(' ');
    }
    do pn = R(MAXPARS-1) + 1; while (!tags[tn][pn]);
    printf("%s", tags[tn][pn]);
    switch (R(32)) {
      case 0: putchar(R(256)); 
      case 1: break;
      default: putchar('=');
    }
    
    make_up_value();
    
  }
    
  putchar('>');
  
}


int main(int argc,char** argv) {
  int tc,seed;
  
  printf("Content-Type: text/html;charset=utf-8\nRefresh: 0;URL=mangle.cgi\n\n");
  printf("<HTML><HEAD><META HTTP-EQUIV=\"Refresh\" content=\"0;URL=mangle.cgi\">\n");
  printf("<script language=\"javascript\">setTimeout('window.location=\"mangle.cgi\"', 1000);</script>\n");

  seed = (time(0) ^ (getpid() << 16));
  fprintf(stderr,"[%u] Mangle attempt 0x%08x (%s) -- %s\n", (int)time(0), seed, getenv("HTTP_USER_AGENT"), getenv("REMOTE_ADDR"));
  srand(seed);
  
  tc = R(MAXTCOUNT) + 1;
  while (tc--) random_tag();
  fflush(0);
  return 0;
}
