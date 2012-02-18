#ifdef __cplusplus
#define NULL __null
#else
#define NULL ((void *)0)
#endif

#include <ctype.h>
#include <float.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/types.h>

#ifdef __cplusplus

#include <list>
#include <typeinfo>

#endif

#ifdef __cplusplus
#define new ("if you use new/delete make sure to include config.h at the top of the file"()) 
#define delete ("if you use new/delete make sure to include config.h at the top of the file"()) 
#endif

/* Work around bug with C++ library that screws up Objective-C++ when exception support is disabled. */
#undef try
#undef catch

