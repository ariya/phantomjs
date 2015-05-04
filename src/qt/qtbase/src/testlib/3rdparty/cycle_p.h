/*
 * Copyright (c) 2003, 2006 Matteo Frigo
 * Copyright (c) 2003, 2006 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/* $Id: cycle.h,v 1.52 2006-02-08 02:36:47 athena Exp $ */

/* machine-dependent cycle counters code. Needs to be inlined. */

/***************************************************************************/
/* To use the cycle counters in your code, simply #include "cycle.h" (this
   file), and then use the functions/macros:

                 CycleCounterTicks getticks(void);

   CycleCounterTicks is an opaque typedef defined below, representing the current time.
   You extract the elapsed time between two calls to gettick() via:

                 double elapsed(CycleCounterTicks t1, CycleCounterTicks t0);

   which returns a double-precision variable in arbitrary units.  You
   are not expected to convert this into human units like seconds; it
   is intended only for *comparisons* of time intervals.

   (In order to use some of the OS-dependent timer routines like
   Solaris' gethrtime, you need to paste the autoconf snippet below
   into your configure.ac file and #include "config.h" before cycle.h,
   or define the relevant macros manually if you are not using autoconf.)
*/

/***************************************************************************/
/* This file uses macros like HAVE_GETHRTIME that are assumed to be
   defined according to whether the corresponding function/type/header
   is available on your system.  The necessary macros are most
   conveniently defined if you are using GNU autoconf, via the tests:

   dnl ---------------------------------------------------------------------

   AC_C_INLINE
   AC_HEADER_TIME
   AC_CHECK_HEADERS([sys/time.h c_asm.h intrinsics.h mach/mach_time.h])

   AC_CHECK_TYPE([hrtime_t],[AC_DEFINE(HAVE_HRTIME_T, 1, [Define to 1 if hrtime_t is defined in <sys/time.h>])],,[#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif])

   AC_CHECK_FUNCS([gethrtime read_real_time time_base_to_time clock_gettime mach_absolute_time])

   dnl Cray UNICOS _rtc() (real-time clock) intrinsic
   AC_MSG_CHECKING([for _rtc intrinsic])
   rtc_ok=yes
   AC_TRY_LINK([#ifdef HAVE_INTRINSICS_H
#include <intrinsics.h>
#endif], [_rtc()], [AC_DEFINE(HAVE__RTC,1,[Define if you have the UNICOS _rtc() intrinsic.])], [rtc_ok=no])
   AC_MSG_RESULT($rtc_ok)

   dnl ---------------------------------------------------------------------
*/

/***************************************************************************/

#ifndef QBENCHLIB_CYCLE_H
#define QBENCHLIB_CYCLE_H

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#define INLINE_ELAPSED(INL) static INL double elapsed(CycleCounterTicks t1, CycleCounterTicks t0) \
{									  \
     return (double)(t1 - t0);						  \
}

/*----------------------------------------------------------------*/
/* Solaris */
#if defined(HAVE_GETHRTIME) && defined(HAVE_HRTIME_T) && !defined(HAVE_TICK_COUNTER)
typedef hrtime_t CycleCounterTicks;

#define getticks gethrtime

INLINE_ELAPSED(inline)

#define HAVE_TICK_COUNTER
#endif

/*----------------------------------------------------------------*/
/* AIX v. 4+ routines to read the real-time clock or time-base register */
#if defined(HAVE_READ_REAL_TIME) && defined(HAVE_TIME_BASE_TO_TIME) && !defined(HAVE_TICK_COUNTER)
typedef timebasestruct_t CycleCounterTicks;

static inline CycleCounterTicks getticks(void)
{
     CycleCounterTicks t;
     read_real_time(&t, TIMEBASE_SZ);
     return t;
}

static inline double elapsed(CycleCounterTicks t1, CycleCounterTicks t0) /* time in nanoseconds */
{
     time_base_to_time(&t1, TIMEBASE_SZ);
     time_base_to_time(&t0, TIMEBASE_SZ);
     return ((t1.tb_high - t0.tb_high) * 1e9 + (t1.tb_low - t0.tb_low));
}

#define HAVE_TICK_COUNTER
#endif

/*----------------------------------------------------------------*/
/*
 * PowerPC ``cycle'' counter using the time base register.
 */
#if ((defined(__GNUC__) && (defined(__powerpc__) || defined(__ppc__))) || (defined(__MWERKS__) && defined(macintosh)))  && !defined(HAVE_TICK_COUNTER)
typedef unsigned long long CycleCounterTicks;

static __inline__ CycleCounterTicks getticks(void)
{
     unsigned int tbl, tbu0, tbu1;

     do {
	  __asm__ __volatile__ ("mftbu %0" : "=r"(tbu0));
	  __asm__ __volatile__ ("mftb %0" : "=r"(tbl));
	  __asm__ __volatile__ ("mftbu %0" : "=r"(tbu1));
     } while (tbu0 != tbu1);

     return (((unsigned long long)tbu0) << 32) | tbl;
}

INLINE_ELAPSED(__inline__)

#define HAVE_TICK_COUNTER
#endif

/* MacOS/Mach (Darwin) time-base register interface (unlike UpTime,
   from Carbon, requires no additional libraries to be linked). */
#if defined(HAVE_MACH_ABSOLUTE_TIME) && defined(HAVE_MACH_MACH_TIME_H) && !defined(HAVE_TICK_COUNTER)
#include <mach/mach_time.h>
typedef uint64_t CycleCounterTicks;
#define getticks mach_absolute_time
INLINE_ELAPSED(__inline__)
#define HAVE_TICK_COUNTER
#endif

/*----------------------------------------------------------------*/
/*
 * Pentium cycle counter
 */
#if (defined(__GNUC__) || defined(__ICC)) && defined(__i386__)  && !defined(HAVE_TICK_COUNTER)
typedef unsigned long long CycleCounterTicks;

static __inline__ CycleCounterTicks getticks(void)
{
     CycleCounterTicks ret;

     __asm__ __volatile__("rdtsc": "=A" (ret));
     /* no input, nothing else clobbered */
     return ret;
}

INLINE_ELAPSED(__inline__)

#define HAVE_TICK_COUNTER
#define TIME_MIN 5000.0   /* unreliable pentium IV cycle counter */
#endif

/* Visual C++ -- thanks to Morten Nissov for his help with this */
#if defined(_MSC_VER)
#if _MSC_VER >= 1200 && (_M_IX86 >= 500 || (defined(_WIN32_WCE) && defined(_X86_))) && !defined(HAVE_TICK_COUNTER)
#include <windows.h>
typedef LARGE_INTEGER CycleCounterTicks;
#define RDTSC __asm __emit 0fh __asm __emit 031h /* hack for VC++ 5.0 */

static __inline CycleCounterTicks getticks(void)
{
     CycleCounterTicks retval;

     __asm {
      RDTSC
      mov retval.HighPart, edx
      mov retval.LowPart, eax
     }
     return retval;
}

static __inline double elapsed(CycleCounterTicks t1, CycleCounterTicks t0)
{
     return (double)(t1.QuadPart - t0.QuadPart);
}

#define HAVE_TICK_COUNTER
#define TIME_MIN 5000.0   /* unreliable pentium IV cycle counter */
#endif
#endif

#if _MSC_VER >= 1400 && defined(_WIN32_WCE) && !defined(HAVE_TICK_COUNTER)
#include <windows.h>
typedef DWORD CycleCounterTicks;

static __inline CycleCounterTicks getticks(void)
{
    return GetTickCount();
}

static __inline double elapsed(CycleCounterTicks t1, CycleCounterTicks t0)
{
     return (double)(t1 - t0);
}

#define HAVE_TICK_COUNTER
#define TIME_MIN 5000.0
#endif

/*----------------------------------------------------------------*/
/*
 * X86-64 cycle counter
 */
#if (defined(__GNUC__) || defined(__ICC)) && defined(__x86_64__)  && !defined(HAVE_TICK_COUNTER)
typedef unsigned long long CycleCounterTicks;

static __inline__ CycleCounterTicks getticks(void)
{
     unsigned a, d;
     asm volatile("rdtsc" : "=a" (a), "=d" (d));
     return ((CycleCounterTicks)a) | (((CycleCounterTicks)d) << 32);
}

INLINE_ELAPSED(__inline__)

#define HAVE_TICK_COUNTER
#endif

/* PGI compiler, courtesy Cristiano Calonaci, Andrea Tarsi, & Roberto Gori.
   NOTE: this code will fail to link unless you use the -Masmkeyword compiler
   option (grrr). */
#if defined(__PGI) && defined(__x86_64__) && !defined(HAVE_TICK_COUNTER)
typedef unsigned long long CycleCounterTicks;
static CycleCounterTicks getticks(void)
{
    asm(" rdtsc; shl    $0x20,%rdx; mov    %eax,%eax; or     %rdx,%rax;    ");
}
INLINE_ELAPSED(__inline__)
#define HAVE_TICK_COUNTER
#endif

/* Visual C++ */
#if _MSC_VER >= 1400 && (defined(_M_AMD64) || defined(_M_X64)) && !defined(HAVE_TICK_COUNTER)
#include <intrin.h>

typedef unsigned __int64 CycleCounterTicks;

#define getticks __rdtsc

INLINE_ELAPSED(__inline)

#define HAVE_TICK_COUNTER
#endif

/*----------------------------------------------------------------*/
/*
 * IA64 cycle counter
 */

/* intel's icc/ecc compiler */
#if (defined(__EDG_VERSION) || defined(__ECC)) && defined(__ia64__) && !defined(HAVE_TICK_COUNTER)
typedef unsigned long CycleCounterTicks;
#include <ia64intrin.h>

static __inline__ CycleCounterTicks getticks(void)
{
     return __getReg(_IA64_REG_AR_ITC);
}

INLINE_ELAPSED(__inline__)

#define HAVE_TICK_COUNTER
#endif

/* gcc */
#if defined(__GNUC__) && defined(__ia64__) && !defined(HAVE_TICK_COUNTER)
typedef unsigned long CycleCounterTicks;

static __inline__ CycleCounterTicks getticks(void)
{
     CycleCounterTicks ret;

     __asm__ __volatile__ ("mov %0=ar.itc" : "=r"(ret));
     return ret;
}

INLINE_ELAPSED(__inline__)

#define HAVE_TICK_COUNTER
#endif

/* HP/UX IA64 compiler, courtesy Teresa L. Johnson: */
#if defined(__hpux) && defined(__ia64) && !defined(HAVE_TICK_COUNTER)
#include <machine/sys/inline.h>
typedef unsigned long CycleCounterTicks;

static inline CycleCounterTicks getticks(void)
{
     CycleCounterTicks ret;

     ret = _Asm_mov_from_ar (_AREG_ITC);
     return ret;
}

INLINE_ELAPSED(inline)

#define HAVE_TICK_COUNTER
#endif

/* Microsoft Visual C++ */
#if defined(_MSC_VER) && defined(_M_IA64) && !defined(HAVE_TICK_COUNTER)
typedef unsigned __int64 CycleCounterTicks;

#  ifdef __cplusplus
extern "C"
#  endif
ticks __getReg(int whichReg);
#pragma intrinsic(__getReg)

static __inline CycleCounterTicks getticks(void)
{
     volatile CycleCounterTicks temp;
     temp = __getReg(3116);
     return temp;
}

#define HAVE_TICK_COUNTER
#endif

/*----------------------------------------------------------------*/
/*
 * PA-RISC cycle counter
 */
#if (defined(__hppa__) || defined(__hppa)) && !defined(HAVE_TICK_COUNTER)
typedef unsigned long CycleCounterTicks;

#  ifdef __GNUC__
static __inline__ CycleCounterTicks getticks(void)
{
     CycleCounterTicks ret;

     __asm__ __volatile__("mfctl 16, %0": "=r" (ret));
     /* no input, nothing else clobbered */
     return ret;
}

INLINE_ELAPSED(inline)

#define HAVE_TICK_COUNTER

#  elif 0 // Doesn't compile
#  include <machine/inline.h>
static inline unsigned long getticks(void)
{
     register CycleCounterTicks ret;
     _MFCTL(16, ret);
     return ret;
}
#  endif

#endif

/*----------------------------------------------------------------*/
/* S390, courtesy of James Treacy */
#if defined(__GNUC__) && defined(__s390__) && !defined(HAVE_TICK_COUNTER)
typedef unsigned long long CycleCounterTicks;

static __inline__ CycleCounterTicks getticks(void)
{
     CycleCounterTicks cycles;
     __asm__("stck 0(%0)" : : "a" (&(cycles)) : "memory", "cc");
     return cycles;
}

INLINE_ELAPSED(__inline__)

#define HAVE_TICK_COUNTER
#endif
/*----------------------------------------------------------------*/
#if defined(__GNUC__) && defined(__alpha__) && !defined(HAVE_TICK_COUNTER)
/*
 * The 32-bit cycle counter on alpha overflows pretty quickly,
 * unfortunately.  A 1GHz machine overflows in 4 seconds.
 */
typedef unsigned int CycleCounterTicks;

static __inline__ CycleCounterTicks getticks(void)
{
     unsigned long cc;
     __asm__ __volatile__ ("rpcc %0" : "=r"(cc));
     return (cc & 0xFFFFFFFF);
}

INLINE_ELAPSED(__inline__)

#define HAVE_TICK_COUNTER
#endif

/*----------------------------------------------------------------*/
#if defined(__GNUC__) && defined(__sparc_v9__) && !defined(HAVE_TICK_COUNTER)
typedef unsigned long CycleCounterTicks;

static __inline__ CycleCounterTicks getticks(void)
{
     CycleCounterTicks ret;
     __asm__ __volatile__("rd %%tick, %0" : "=r" (ret));
     return ret;
}

INLINE_ELAPSED(__inline__)

#define HAVE_TICK_COUNTER
#endif

/*----------------------------------------------------------------*/
#if (defined(__DECC) || defined(__DECCXX)) && defined(__alpha) && defined(HAVE_C_ASM_H) && !defined(HAVE_TICK_COUNTER)
#  include <c_asm.h>
typedef unsigned int CycleCounterTicks;

static __inline CycleCounterTicks getticks(void)
{
     unsigned long cc;
     cc = asm("rpcc %v0");
     return (cc & 0xFFFFFFFF);
}

INLINE_ELAPSED(__inline)

#define HAVE_TICK_COUNTER
#endif
/*----------------------------------------------------------------*/
/* SGI/Irix */
#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_SGI_CYCLE) && !defined(HAVE_TICK_COUNTER)
typedef struct timespec CycleCounterTicks;

static inline CycleCounterTicks getticks(void)
{
     struct timespec t;
     clock_gettime(CLOCK_SGI_CYCLE, &t);
     return t;
}

static inline double elapsed(CycleCounterTicks t1, CycleCounterTicks t0)
{
     return (double)(t1.tv_sec - t0.tv_sec) * 1.0E9 +
	  (double)(t1.tv_nsec - t0.tv_nsec);
}
#define HAVE_TICK_COUNTER
#endif

/*----------------------------------------------------------------*/
/* Cray UNICOS _rtc() intrinsic function */
#if defined(HAVE__RTC) && !defined(HAVE_TICK_COUNTER)
#ifdef HAVE_INTRINSICS_H
#  include <intrinsics.h>
#endif

typedef long long CycleCounterTicks;

#define getticks _rtc

INLINE_ELAPSED(inline)

#define HAVE_TICK_COUNTER
#endif

/*----------------------------------------------------------------*/
/* Symbian */
#if defined(__SYMBIAN32__) && !defined(HAVE_TICK_COUNTER)
#include <e32std.h>

typedef TUint32 CycleCounterTicks;

static inline CycleCounterTicks getticks(void)
{
    return User::FastCounter();
}

INLINE_ELAPSED(inline)

#define HAVE_TICK_COUNTER
#endif

#endif // QBENCHLIB_CYCLE_H
