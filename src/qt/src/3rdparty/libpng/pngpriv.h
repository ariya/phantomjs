
/* pngpriv.h - private declarations for use inside libpng
 *
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1998-2011 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 *
 * Last changed in libpng 1.5.4 [July 7, 2011]
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 */

/* The symbols declared in this file (including the functions declared
 * as PNG_EXTERN) are PRIVATE.  They are not part of the libpng public
 * interface, and are not recommended for use by regular applications.
 * Some of them may become public in the future; others may stay private,
 * change in an incompatible way, or even disappear.
 * Although the libpng users are not forbidden to include this header,
 * they should be well aware of the issues that may arise from doing so.
 */

#ifndef PNGPRIV_H
#define PNGPRIV_H

#ifdef _MSC_VER
#  define _CRT_SECURE_NO_DEPRECATE
#endif

/* Feature Test Macros.  The following are defined here to ensure that correctly
 * implemented libraries reveal the APIs libpng needs to build and hide those
 * that are not needed and potentially damaging to the compilation.
 *
 * Feature Test Macros must be defined before any system header is included (see
 * POSIX 1003.1 2.8.2 "POSIX Symbols."
 *
 * These macros only have an effect if the operating system supports either
 * POSIX 1003.1 or C99, or both.  On other operating systems (particularly
 * Windows/Visual Studio) there is no effect; the OS specific tests below are
 * still required (as of 2011-05-02.)
 */
#define _POSIX_SOURCE 1 /* Just the POSIX 1003.1 and C89 APIs */

/* This is required for the definition of abort(), used as a last ditch
 * error handler when all else fails.
 */

#include <stdlib.h>

#define PNGLIB_BUILD
#ifdef PNG_USER_CONFIG
#  include "pngusr.h"
   /* These should have been defined in pngusr.h */
#  ifndef PNG_USER_PRIVATEBUILD
#    define PNG_USER_PRIVATEBUILD "Custom libpng build"
#  endif
#  ifndef PNG_USER_DLLFNAME_POSTFIX
#    define PNG_USER_DLLFNAME_POSTFIX "Cb"
#  endif
#endif
#include "png.h"
#include "pnginfo.h"
#include "pngstruct.h"

namespace PrivatePng {
/* This is used for 16 bit gamma tables - only the top level pointers are const,
 * this could be changed:
 */
typedef PNG_CONST png_uint_16p FAR * png_const_uint_16pp;
} // namespace PrivatePng

/* Added at libpng-1.2.9 */
/* Moved to pngpriv.h at libpng-1.5.0 */

/* config.h is created by and PNG_CONFIGURE_LIBPNG is set by the "configure"
 * script.  We may need it here to get the correct configuration on things
 * like limits.
 */
#ifdef PNG_CONFIGURE_LIBPNG
#  ifdef HAVE_CONFIG_H
#    include "config.h"
#  endif
#endif

/* Moved to pngpriv.h at libpng-1.5.0 */
/* NOTE: some of these may have been used in external applications as
 * these definitions were exposed in pngconf.h prior to 1.5.
 */

/* If you are running on a machine where you cannot allocate more
 * than 64K of memory at once, uncomment this.  While libpng will not
 * normally need that much memory in a chunk (unless you load up a very
 * large file), zlib needs to know how big of a chunk it can use, and
 * libpng thus makes sure to check any memory allocation to verify it
 * will fit into memory.
 *
 * zlib provides 'MAXSEG_64K' which, if defined, indicates the
 * same limit and pngconf.h (already included) sets the limit
 * if certain operating systems are detected.
 */
#if defined(MAXSEG_64K) && !defined(PNG_MAX_MALLOC_64K)
#  define PNG_MAX_MALLOC_64K
#endif

#ifndef PNG_UNUSED
/* Unused formal parameter warnings are silenced using the following macro
 * which is expected to have no bad effects on performance (optimizing
 * compilers will probably remove it entirely).  Note that if you replace
 * it with something other than whitespace, you must include the terminating
 * semicolon.
 */
#  define PNG_UNUSED(param) (void)param;
#endif

/* Just a little check that someone hasn't tried to define something
 * contradictory.
 */
#if (PNG_ZBUF_SIZE > 65536L) && defined(PNG_MAX_MALLOC_64K)
#  undef PNG_ZBUF_SIZE
#  define PNG_ZBUF_SIZE 65536L
#endif

/* PNG_STATIC is used to mark internal file scope functions if they need to be
 * accessed for implementation tests (see the code in tests/?*).
 */
#ifndef PNG_STATIC
#   define PNG_STATIC static
#endif

/* If warnings or errors are turned off the code is disabled or redirected here.
 * From 1.5.4 functions have been added to allow very limited formatting of
 * error and warning messages - this code will also be disabled here.
 */
#ifdef PNG_WARNINGS_SUPPORTED
#  define PNG_WARNING_PARAMETERS(p) png_warning_parameters p;
#else
#  define png_warning(s1,s2) ((void)(s1))
#  define png_chunk_warning(s1,s2) ((void)(s1))
#  define png_warning_parameter(p,number,string) ((void)0)
#  define png_warning_parameter_unsigned(p,number,format,value) ((void)0)
#  define png_warning_parameter_signed(p,number,format,value) ((void)0)
#  define png_formatted_warning(pp,p,message) ((void)(pp))
#  define PNG_WARNING_PARAMETERS(p)
#endif
#ifndef PNG_ERROR_TEXT_SUPPORTED
#  define png_error(s1,s2) png_err(s1)
#  define png_chunk_error(s1,s2) png_err(s1)
#  define png_fixed_error(s1,s2) png_err(s1)
#endif

#ifndef PNG_EXTERN
/* The functions exported by PNG_EXTERN are internal functions, which
 * aren't usually used outside the library (as far as I know), so it is
 * debatable if they should be exported at all.  In the future, when it
 * is possible to have run-time registry of chunk-handling functions,
 * some of these might be made available again.
#  define PNG_EXTERN extern
 */
#  define PNG_EXTERN
#endif

/* Some fixed point APIs are still required even if not exported because
 * they get used by the corresponding floating point APIs.  This magic
 * deals with this:
 */
#ifdef PNG_FIXED_POINT_SUPPORTED
#  define PNGFAPI PNGAPI
#else
#  define PNGFAPI /* PRIVATE */
#endif

/* Other defines specific to compilers can go here.  Try to keep
 * them inside an appropriate ifdef/endif pair for portability.
 */
#if defined(PNG_FLOATING_POINT_SUPPORTED) ||\
    defined(PNG_FLOATING_ARITHMETIC_SUPPORTED)
   /* png.c requires the following ANSI-C constants if the conversion of
    * floating point to ASCII is implemented therein:
    *
    *  DBL_DIG  Maximum number of decimal digits (can be set to any constant)
    *  DBL_MIN  Smallest normalized fp number (can be set to an arbitrary value)
    *  DBL_MAX  Maximum floating point number (can be set to an arbitrary value)
    */
#  include <float.h>

#  if (defined(__MWERKS__) && defined(macintosh)) || defined(applec) || \
    defined(THINK_C) || defined(__SC__) || defined(TARGET_OS_MAC)
     /* We need to check that <math.h> hasn't already been included earlier
      * as it seems it doesn't agree with <fp.h>, yet we should really use
      * <fp.h> if possible.
      */
#    if !defined(__MATH_H__) && !defined(__MATH_H) && !defined(__cmath__)
#      include <fp.h>
#    endif
#  else
#    include <math.h>
#  endif
#  if defined(_AMIGA) && defined(__SASC) && defined(_M68881)
     /* Amiga SAS/C: We must include builtin FPU functions when compiling using
      * MATH=68881
      */
#    include <m68881.h>
#  endif
#endif

/* This provides the non-ANSI (far) memory allocation routines. */
#if defined(__TURBOC__) && defined(__MSDOS__)
#  include <mem.h>
#  include <alloc.h>
#endif

#if defined(WIN32) || defined(_Windows) || defined(_WINDOWS) || \
    defined(_WIN32) || defined(__WIN32__)
#  if !defined(__SYMBIAN32__)
#    include <windows.h>  /* defines _WINDOWS_ macro */
#  endif
#endif

/* Moved here around 1.5.0beta36 from pngconf.h */
/* Users may want to use these so they are not private.  Any library
 * functions that are passed far data must be model-independent.
 */

/* Memory model/platform independent fns */
#ifndef PNG_ABORT
#  if defined(_WINDOWS_) || defined(_WIN32_WCE)
#    define PNG_ABORT() ExitProcess(0)
#  else
#    define PNG_ABORT() abort()
#  endif
#endif

#ifdef USE_FAR_KEYWORD
/* Use this to make far-to-near assignments */
#  define CHECK   1
#  define NOCHECK 0
#  define CVT_PTR(ptr) (png_far_to_near(png_ptr,ptr,CHECK))
#  define CVT_PTR_NOCHECK(ptr) (png_far_to_near(png_ptr,ptr,NOCHECK))
#  define png_strlen  _fstrlen
#  define png_memcmp  _fmemcmp    /* SJT: added */
#  define png_memcpy  _fmemcpy
#  define png_memset  _fmemset
#else
#  ifdef _WINDOWS_  /* Favor Windows over C runtime fns */
#    define CVT_PTR(ptr)         (ptr)
#    define CVT_PTR_NOCHECK(ptr) (ptr)
#    define png_strlen  lstrlenA
#    define png_memcmp  memcmp
#    define png_memcpy  CopyMemory
#    define png_memset  memset
#  else
#    define CVT_PTR(ptr)         (ptr)
#    define CVT_PTR_NOCHECK(ptr) (ptr)
#    define png_strlen  strlen
#    define png_memcmp  memcmp      /* SJT: added */
#    define png_memcpy  memcpy
#    define png_memset  memset
#  endif
#endif
/* End of memory model/platform independent support */
/* End of 1.5.0beta36 move from pngconf.h */

/* CONSTANTS and UTILITY MACROS
 * These are used internally by libpng and not exposed in the API
 */

/* Various modes of operation.  Note that after an init, mode is set to
 * zero automatically when the structure is created.  Three of these
 * are defined in png.h because they need to be visible to applications
 * that call png_set_unknown_chunk().
 */
/* #define PNG_HAVE_IHDR            0x01 (defined in png.h) */
/* #define PNG_HAVE_PLTE            0x02 (defined in png.h) */
#define PNG_HAVE_IDAT               0x04
/* #define PNG_AFTER_IDAT           0x08 (defined in png.h) */
#define PNG_HAVE_IEND               0x10
#define PNG_HAVE_gAMA               0x20
#define PNG_HAVE_cHRM               0x40
#define PNG_HAVE_sRGB               0x80
#define PNG_HAVE_CHUNK_HEADER      0x100
#define PNG_WROTE_tIME             0x200
#define PNG_WROTE_INFO_BEFORE_PLTE 0x400
#define PNG_BACKGROUND_IS_GRAY     0x800
#define PNG_HAVE_PNG_SIGNATURE    0x1000
#define PNG_HAVE_CHUNK_AFTER_IDAT 0x2000 /* Have another chunk after IDAT */

/* Flags for the transformations the PNG library does on the image data */
#define PNG_BGR                 0x0001
#define PNG_INTERLACE           0x0002
#define PNG_PACK                0x0004
#define PNG_SHIFT               0x0008
#define PNG_SWAP_BYTES          0x0010
#define PNG_INVERT_MONO         0x0020
#define PNG_QUANTIZE            0x0040
#define PNG_COMPOSE             0x0080     /* Was PNG_BACKGROUND */
#define PNG_BACKGROUND_EXPAND   0x0100
#define PNG_EXPAND_16           0x0200     /* Added to libpng 1.5.2 */
#define PNG_16_TO_8             0x0400     /* Becomes 'chop' in 1.5.4 */
#define PNG_RGBA                0x0800
#define PNG_EXPAND              0x1000
#define PNG_GAMMA               0x2000
#define PNG_GRAY_TO_RGB         0x4000
#define PNG_FILLER              0x8000L
#define PNG_PACKSWAP           0x10000L
#define PNG_SWAP_ALPHA         0x20000L
#define PNG_STRIP_ALPHA        0x40000L
#define PNG_INVERT_ALPHA       0x80000L
#define PNG_USER_TRANSFORM    0x100000L
#define PNG_RGB_TO_GRAY_ERR   0x200000L
#define PNG_RGB_TO_GRAY_WARN  0x400000L
#define PNG_RGB_TO_GRAY       0x600000L  /* two bits, RGB_TO_GRAY_ERR|WARN */
#define PNG_ENCODE_ALPHA      0x800000L  /* Added to libpng-1.5.4 */
#define PNG_ADD_ALPHA         0x1000000L  /* Added to libpng-1.2.7 */
#define PNG_EXPAND_tRNS       0x2000000L  /* Added to libpng-1.2.9 */
#define PNG_SCALE_16_TO_8     0x4000000L  /* Added to libpng-1.5.4 */
                       /*   0x8000000L  unused */
                       /*  0x10000000L  unused */
                       /*  0x20000000L  unused */
                       /*  0x40000000L  unused */

/* Flags for png_create_struct */
#define PNG_STRUCT_PNG   0x0001
#define PNG_STRUCT_INFO  0x0002

/* Scaling factor for filter heuristic weighting calculations */
#define PNG_WEIGHT_FACTOR (1<<(PNG_WEIGHT_SHIFT))
#define PNG_COST_FACTOR (1<<(PNG_COST_SHIFT))

/* Flags for the png_ptr->flags rather than declaring a byte for each one */
#define PNG_FLAG_ZLIB_CUSTOM_STRATEGY     0x0001
#define PNG_FLAG_ZLIB_CUSTOM_LEVEL        0x0002
#define PNG_FLAG_ZLIB_CUSTOM_MEM_LEVEL    0x0004
#define PNG_FLAG_ZLIB_CUSTOM_WINDOW_BITS  0x0008
#define PNG_FLAG_ZLIB_CUSTOM_METHOD       0x0010
#define PNG_FLAG_ZLIB_FINISHED            0x0020
#define PNG_FLAG_ROW_INIT                 0x0040
#define PNG_FLAG_FILLER_AFTER             0x0080
#define PNG_FLAG_CRC_ANCILLARY_USE        0x0100
#define PNG_FLAG_CRC_ANCILLARY_NOWARN     0x0200
#define PNG_FLAG_CRC_CRITICAL_USE         0x0400
#define PNG_FLAG_CRC_CRITICAL_IGNORE      0x0800
#define PNG_FLAG_ASSUME_sRGB              0x1000  /* Added to libpng-1.5.4 */
#define PNG_FLAG_OPTIMIZE_ALPHA           0x2000  /* Added to libpng-1.5.4 */
#define PNG_FLAG_DETECT_UNINITIALIZED     0x4000  /* Added to libpng-1.5.4 */
#define PNG_FLAG_KEEP_UNKNOWN_CHUNKS      0x8000L
#define PNG_FLAG_KEEP_UNSAFE_CHUNKS       0x10000L
#define PNG_FLAG_LIBRARY_MISMATCH         0x20000L
#define PNG_FLAG_STRIP_ERROR_NUMBERS      0x40000L
#define PNG_FLAG_STRIP_ERROR_TEXT         0x80000L
#define PNG_FLAG_MALLOC_NULL_MEM_OK       0x100000L
                                  /*      0x200000L  unused */
                                  /*      0x400000L  unused */
#define PNG_FLAG_BENIGN_ERRORS_WARN       0x800000L  /* Added to libpng-1.4.0 */
#define PNG_FLAG_ZTXT_CUSTOM_STRATEGY    0x1000000L  /* 5 lines added */
#define PNG_FLAG_ZTXT_CUSTOM_LEVEL       0x2000000L  /* to libpng-1.5.4 */
#define PNG_FLAG_ZTXT_CUSTOM_MEM_LEVEL   0x4000000L
#define PNG_FLAG_ZTXT_CUSTOM_WINDOW_BITS 0x8000000L
#define PNG_FLAG_ZTXT_CUSTOM_METHOD      0x10000000L
                                  /*     0x20000000L  unused */
                                  /*     0x40000000L  unused */

#define PNG_FLAG_CRC_ANCILLARY_MASK (PNG_FLAG_CRC_ANCILLARY_USE | \
                                     PNG_FLAG_CRC_ANCILLARY_NOWARN)

#define PNG_FLAG_CRC_CRITICAL_MASK  (PNG_FLAG_CRC_CRITICAL_USE | \
                                     PNG_FLAG_CRC_CRITICAL_IGNORE)

#define PNG_FLAG_CRC_MASK           (PNG_FLAG_CRC_ANCILLARY_MASK | \
                                     PNG_FLAG_CRC_CRITICAL_MASK)

/* zlib.h declares a magic type 'uInt' that limits the amount of data that zlib
 * can handle at once.  This type need be no larger than 16 bits (so maximum of
 * 65535), this define allows us to discover how big it is, but limited by the
 * maximuum for png_size_t.  The value can be overriden in a library build
 * (pngusr.h, or set it in CPPFLAGS) and it works to set it to a considerably
 * lower value (e.g. 255 works).  A lower value may help memory usage (slightly)
 * and may even improve performance on some systems (and degrade it on others.)
 */
#ifndef ZLIB_IO_MAX
#  define ZLIB_IO_MAX ((uInt)-1)
#endif

/* Save typing and make code easier to understand */

#define PNG_COLOR_DIST(c1, c2) (abs((int)((c1).red) - (int)((c2).red)) + \
   abs((int)((c1).green) - (int)((c2).green)) + \
   abs((int)((c1).blue) - (int)((c2).blue)))

/* Added to libpng-1.2.6 JB */
#define PNG_ROWBYTES(pixel_bits, width) \
    ((pixel_bits) >= 8 ? \
    ((png_size_t)(width) * (((png_size_t)(pixel_bits)) >> 3)) : \
    (( ((png_size_t)(width) * ((png_size_t)(pixel_bits))) + 7) >> 3) )

/* PNG_OUT_OF_RANGE returns true if value is outside the range
 * ideal-delta..ideal+delta.  Each argument is evaluated twice.
 * "ideal" and "delta" should be constants, normally simple
 * integers, "value" a variable. Added to libpng-1.2.6 JB
 */
#define PNG_OUT_OF_RANGE(value, ideal, delta) \
   ( (value) < (ideal)-(delta) || (value) > (ideal)+(delta) )

/* Conversions between fixed and floating point, only defined if
 * required (to make sure the code doesn't accidentally use float
 * when it is supposedly disabled.)
 */
#ifdef PNG_FLOATING_POINT_SUPPORTED
/* The floating point conversion can't overflow, though it can and
 * does lose accuracy relative to the original fixed point value.
 * In practice this doesn't matter because png_fixed_point only
 * stores numbers with very low precision.  The png_ptr and s
 * arguments are unused by default but are there in case error
 * checking becomes a requirement.
 */
#define png_float(png_ptr, fixed, s) (.00001 * (fixed))

/* The fixed point conversion performs range checking and evaluates
 * its argument multiple times, so must be used with care.  The
 * range checking uses the PNG specification values for a signed
 * 32 bit fixed point value except that the values are deliberately
 * rounded-to-zero to an integral value - 21474 (21474.83 is roughly
 * (2^31-1) * 100000). 's' is a string that describes the value being
 * converted.
 *
 * NOTE: this macro will raise a png_error if the range check fails,
 * therefore it is normally only appropriate to use this on values
 * that come from API calls or other sources where an out of range
 * error indicates a programming error, not a data error!
 *
 * NOTE: by default this is off - the macro is not used - because the
 * function call saves a lot of code.
 */
namespace PrivatePng {
#ifdef PNG_FIXED_POINT_MACRO_SUPPORTED
#define png_fixed(png_ptr, fp, s) ((fp) <= 21474 && (fp) >= -21474 ?\
    ((png_fixed_point)(100000 * (fp))) : (png_fixed_error(png_ptr, s),0))
#else
PNG_EXTERN png_fixed_point png_fixed PNGARG((png_structp png_ptr, double fp,
   png_const_charp text));
#endif
#endif
} // namespace PrivatePng

/* Constant strings for known chunk types.  If you need to add a chunk,
 * define the name here, and add an invocation of the macro wherever it's
 * needed.
 */
#define PNG_IHDR PNG_CONST png_byte png_IHDR[5] = { 73,  72,  68,  82, '\0'}
#define PNG_IDAT PNG_CONST png_byte png_IDAT[5] = { 73,  68,  65,  84, '\0'}
#define PNG_IEND PNG_CONST png_byte png_IEND[5] = { 73,  69,  78,  68, '\0'}
#define PNG_PLTE PNG_CONST png_byte png_PLTE[5] = { 80,  76,  84,  69, '\0'}
#define PNG_bKGD PNG_CONST png_byte png_bKGD[5] = { 98,  75,  71,  68, '\0'}
#define PNG_cHRM PNG_CONST png_byte png_cHRM[5] = { 99,  72,  82,  77, '\0'}
#define PNG_gAMA PNG_CONST png_byte png_gAMA[5] = {103,  65,  77,  65, '\0'}
#define PNG_hIST PNG_CONST png_byte png_hIST[5] = {104,  73,  83,  84, '\0'}
#define PNG_iCCP PNG_CONST png_byte png_iCCP[5] = {105,  67,  67,  80, '\0'}
#define PNG_iTXt PNG_CONST png_byte png_iTXt[5] = {105,  84,  88, 116, '\0'}
#define PNG_oFFs PNG_CONST png_byte png_oFFs[5] = {111,  70,  70, 115, '\0'}
#define PNG_pCAL PNG_CONST png_byte png_pCAL[5] = {112,  67,  65,  76, '\0'}
#define PNG_sCAL PNG_CONST png_byte png_sCAL[5] = {115,  67,  65,  76, '\0'}
#define PNG_pHYs PNG_CONST png_byte png_pHYs[5] = {112,  72,  89, 115, '\0'}
#define PNG_sBIT PNG_CONST png_byte png_sBIT[5] = {115,  66,  73,  84, '\0'}
#define PNG_sPLT PNG_CONST png_byte png_sPLT[5] = {115,  80,  76,  84, '\0'}
#define PNG_sRGB PNG_CONST png_byte png_sRGB[5] = {115,  82,  71,  66, '\0'}
#define PNG_sTER PNG_CONST png_byte png_sTER[5] = {115,  84,  69,  82, '\0'}
#define PNG_tEXt PNG_CONST png_byte png_tEXt[5] = {116,  69,  88, 116, '\0'}
#define PNG_tIME PNG_CONST png_byte png_tIME[5] = {116,  73,  77,  69, '\0'}
#define PNG_tRNS PNG_CONST png_byte png_tRNS[5] = {116,  82,  78,  83, '\0'}
#define PNG_zTXt PNG_CONST png_byte png_zTXt[5] = {122,  84,  88, 116, '\0'}

/* Gamma values (new at libpng-1.5.4): */
#define PNG_GAMMA_MAC_OLD 151724  /* Assume '1.8' is really 2.2/1.45! */
#define PNG_GAMMA_MAC_INVERSE 65909
#define PNG_GAMMA_sRGB_INVERSE 45455

/* These functions are used internally in the code.  They generally
 * shouldn't be used unless you are writing code to add or replace some
 * functionality in libpng.  More information about most functions can
 * be found in the files where the functions are located.
 */

namespace PrivatePng {

/* Check the user version string for compatibility, returns false if the version
 * numbers aren't compatible.
 */
PNG_EXTERN int png_user_version_check(png_structp png_ptr,
   png_const_charp user_png_ver);

/* Allocate memory for an internal libpng struct */
PNG_EXTERN PNG_FUNCTION(png_voidp,png_create_struct,PNGARG((int type)),
   PNG_ALLOCATED);

/* Free memory from internal libpng struct */
PNG_EXTERN void png_destroy_struct PNGARG((png_voidp struct_ptr));

PNG_EXTERN PNG_FUNCTION(png_voidp,png_create_struct_2,
   PNGARG((int type, png_malloc_ptr malloc_fn, png_voidp mem_ptr)),
   PNG_ALLOCATED);
PNG_EXTERN void png_destroy_struct_2 PNGARG((png_voidp struct_ptr,
    png_free_ptr free_fn, png_voidp mem_ptr));

/* Free any memory that info_ptr points to and reset struct. */
PNG_EXTERN void png_info_destroy PNGARG((png_structp png_ptr,
    png_infop info_ptr));

/* Function to allocate memory for zlib.  PNGAPI is disallowed. */
PNG_EXTERN PNG_FUNCTION(voidpf,png_zalloc,PNGARG((voidpf png_ptr, uInt items,
   uInt size)),PNG_ALLOCATED);

/* Function to free memory for zlib.  PNGAPI is disallowed. */
PNG_EXTERN void png_zfree PNGARG((voidpf png_ptr, voidpf ptr));

/* Next four functions are used internally as callbacks.  PNGCBAPI is required
 * but not PNG_EXPORT.  PNGAPI added at libpng version 1.2.3, changed to
 * PNGCBAPI at 1.5.0
 */

PNG_EXTERN void PNGCBAPI png_default_read_data PNGARG((png_structp png_ptr,
    png_bytep data, png_size_t length));

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
PNG_EXTERN void PNGCBAPI png_push_fill_buffer PNGARG((png_structp png_ptr,
    png_bytep buffer, png_size_t length));
#endif

PNG_EXTERN void PNGCBAPI png_default_write_data PNGARG((png_structp png_ptr,
    png_bytep data, png_size_t length));

#ifdef PNG_WRITE_FLUSH_SUPPORTED
#  ifdef PNG_STDIO_SUPPORTED
PNG_EXTERN void PNGCBAPI png_default_flush PNGARG((png_structp png_ptr));
#  endif
#endif

/* Reset the CRC variable */
PNG_EXTERN void png_reset_crc PNGARG((png_structp png_ptr));

/* Write the "data" buffer to whatever output you are using */
PNG_EXTERN void png_write_data PNGARG((png_structp png_ptr,
    png_const_bytep data, png_size_t length));

/* Read and check the PNG file signature */
PNG_EXTERN void png_read_sig PNGARG((png_structp png_ptr, png_infop info_ptr));

/* Read the chunk header (length + type name) */
PNG_EXTERN png_uint_32 png_read_chunk_header PNGARG((png_structp png_ptr));

/* Read data from whatever input you are using into the "data" buffer */
PNG_EXTERN void png_read_data PNGARG((png_structp png_ptr, png_bytep data,
    png_size_t length));

/* Read bytes into buf, and update png_ptr->crc */
PNG_EXTERN void png_crc_read PNGARG((png_structp png_ptr, png_bytep buf,
    png_size_t length));

/* Decompress data in a chunk that uses compression */
#if defined(PNG_READ_COMPRESSED_TEXT_SUPPORTED)
PNG_EXTERN void png_decompress_chunk PNGARG((png_structp png_ptr,
    int comp_type, png_size_t chunklength, png_size_t prefix_length,
    png_size_t *data_length));
#endif

/* Read "skip" bytes, read the file crc, and (optionally) verify png_ptr->crc */
PNG_EXTERN int png_crc_finish PNGARG((png_structp png_ptr, png_uint_32 skip));

/* Read the CRC from the file and compare it to the libpng calculated CRC */
PNG_EXTERN int png_crc_error PNGARG((png_structp png_ptr));

/* Calculate the CRC over a section of data.  Note that we are only
 * passing a maximum of 64K on systems that have this as a memory limit,
 * since this is the maximum buffer size we can specify.
 */
PNG_EXTERN void png_calculate_crc PNGARG((png_structp png_ptr,
    png_const_bytep ptr, png_size_t length));

#ifdef PNG_WRITE_FLUSH_SUPPORTED
PNG_EXTERN void png_flush PNGARG((png_structp png_ptr));
#endif

/* Write various chunks */

/* Write the IHDR chunk, and update the png_struct with the necessary
 * information.
 */
PNG_EXTERN void png_write_IHDR PNGARG((png_structp png_ptr, png_uint_32 width,
    png_uint_32 height,
    int bit_depth, int color_type, int compression_method, int filter_method,
    int interlace_method));

PNG_EXTERN void png_write_PLTE PNGARG((png_structp png_ptr,
    png_const_colorp palette, png_uint_32 num_pal));

PNG_EXTERN void png_write_IDAT PNGARG((png_structp png_ptr, png_bytep data,
    png_size_t length));

PNG_EXTERN void png_write_IEND PNGARG((png_structp png_ptr));

#ifdef PNG_WRITE_gAMA_SUPPORTED
#  ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXTERN void png_write_gAMA PNGARG((png_structp png_ptr, double file_gamma));
#  endif
#  ifdef PNG_FIXED_POINT_SUPPORTED
PNG_EXTERN void png_write_gAMA_fixed PNGARG((png_structp png_ptr,
    png_fixed_point file_gamma));
#  endif
#endif

#ifdef PNG_WRITE_sBIT_SUPPORTED
PNG_EXTERN void png_write_sBIT PNGARG((png_structp png_ptr,
    png_const_color_8p sbit, int color_type));
#endif

#ifdef PNG_WRITE_cHRM_SUPPORTED
#  ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXTERN void png_write_cHRM PNGARG((png_structp png_ptr,
    double white_x, double white_y,
    double red_x, double red_y, double green_x, double green_y,
    double blue_x, double blue_y));
#  endif
PNG_EXTERN void png_write_cHRM_fixed PNGARG((png_structp png_ptr,
    png_fixed_point int_white_x, png_fixed_point int_white_y,
    png_fixed_point int_red_x, png_fixed_point int_red_y, png_fixed_point
    int_green_x, png_fixed_point int_green_y, png_fixed_point int_blue_x,
    png_fixed_point int_blue_y));
#endif

#ifdef PNG_WRITE_sRGB_SUPPORTED
PNG_EXTERN void png_write_sRGB PNGARG((png_structp png_ptr,
    int intent));
#endif

#ifdef PNG_WRITE_iCCP_SUPPORTED
PNG_EXTERN void png_write_iCCP PNGARG((png_structp png_ptr,
    png_const_charp name, int compression_type,
    png_const_charp profile, int proflen));
   /* Note to maintainer: profile should be png_bytep */
#endif

#ifdef PNG_WRITE_sPLT_SUPPORTED
PNG_EXTERN void png_write_sPLT PNGARG((png_structp png_ptr,
    png_const_sPLT_tp palette));
#endif

#ifdef PNG_WRITE_tRNS_SUPPORTED
PNG_EXTERN void png_write_tRNS PNGARG((png_structp png_ptr,
    png_const_bytep trans, png_const_color_16p values, int number,
    int color_type));
#endif

#ifdef PNG_WRITE_bKGD_SUPPORTED
PNG_EXTERN void png_write_bKGD PNGARG((png_structp png_ptr,
    png_const_color_16p values, int color_type));
#endif

#ifdef PNG_WRITE_hIST_SUPPORTED
PNG_EXTERN void png_write_hIST PNGARG((png_structp png_ptr,
    png_const_uint_16p hist, int num_hist));
#endif

/* Chunks that have keywords */
#if defined(PNG_WRITE_TEXT_SUPPORTED) || defined(PNG_WRITE_pCAL_SUPPORTED) || \
    defined(PNG_WRITE_iCCP_SUPPORTED) || defined(PNG_WRITE_sPLT_SUPPORTED)
PNG_EXTERN png_size_t png_check_keyword PNGARG((png_structp png_ptr,
    png_const_charp key, png_charpp new_key));
#endif

#ifdef PNG_WRITE_tEXt_SUPPORTED
PNG_EXTERN void png_write_tEXt PNGARG((png_structp png_ptr, png_const_charp key,
    png_const_charp text, png_size_t text_len));
#endif

#ifdef PNG_WRITE_zTXt_SUPPORTED
PNG_EXTERN void png_write_zTXt PNGARG((png_structp png_ptr, png_const_charp key,
    png_const_charp text, png_size_t text_len, int compression));
#endif

#ifdef PNG_WRITE_iTXt_SUPPORTED
PNG_EXTERN void png_write_iTXt PNGARG((png_structp png_ptr,
    int compression, png_const_charp key, png_const_charp lang,
    png_const_charp lang_key, png_const_charp text));
#endif

#ifdef PNG_TEXT_SUPPORTED  /* Added at version 1.0.14 and 1.2.4 */
PNG_EXTERN int png_set_text_2 PNGARG((png_structp png_ptr,
    png_infop info_ptr, png_const_textp text_ptr, int num_text));
#endif

#ifdef PNG_WRITE_oFFs_SUPPORTED
PNG_EXTERN void png_write_oFFs PNGARG((png_structp png_ptr,
    png_int_32 x_offset, png_int_32 y_offset, int unit_type));
#endif

#ifdef PNG_WRITE_pCAL_SUPPORTED
PNG_EXTERN void png_write_pCAL PNGARG((png_structp png_ptr, png_charp purpose,
    png_int_32 X0, png_int_32 X1, int type, int nparams,
    png_const_charp units, png_charpp params));
#endif

#ifdef PNG_WRITE_pHYs_SUPPORTED
PNG_EXTERN void png_write_pHYs PNGARG((png_structp png_ptr,
    png_uint_32 x_pixels_per_unit, png_uint_32 y_pixels_per_unit,
    int unit_type));
#endif

#ifdef PNG_WRITE_tIME_SUPPORTED
PNG_EXTERN void png_write_tIME PNGARG((png_structp png_ptr,
    png_const_timep mod_time));
#endif

#ifdef PNG_WRITE_sCAL_SUPPORTED
PNG_EXTERN void png_write_sCAL_s PNGARG((png_structp png_ptr,
    int unit, png_const_charp width, png_const_charp height));
#endif

/* Called when finished processing a row of data */
PNG_EXTERN void png_write_finish_row PNGARG((png_structp png_ptr));

/* Internal use only.   Called before first row of data */
PNG_EXTERN void png_write_start_row PNGARG((png_structp png_ptr));

/* Combine a row of data, dealing with alpha, etc. if requested */
PNG_EXTERN void png_combine_row PNGARG((png_structp png_ptr, png_bytep row,
    int mask));

#ifdef PNG_READ_INTERLACING_SUPPORTED
/* Expand an interlaced row */
/* OLD pre-1.0.9 interface:
PNG_EXTERN void png_do_read_interlace PNGARG((png_row_infop row_info,
    png_bytep row, int pass, png_uint_32 transformations));
 */
PNG_EXTERN void png_do_read_interlace PNGARG((png_structp png_ptr));
#endif

/* GRR TO DO (2.0 or whenever):  simplify other internal calling interfaces */

#ifdef PNG_WRITE_INTERLACING_SUPPORTED
/* Grab pixels out of a row for an interlaced pass */
PNG_EXTERN void png_do_write_interlace PNGARG((png_row_infop row_info,
    png_bytep row, int pass));
#endif

/* Unfilter a row */
PNG_EXTERN void png_read_filter_row PNGARG((png_structp png_ptr,
    png_row_infop row_info, png_bytep row, png_const_bytep prev_row,
    int filter));

/* Choose the best filter to use and filter the row data */
PNG_EXTERN void png_write_find_filter PNGARG((png_structp png_ptr,
    png_row_infop row_info));

/* Finish a row while reading, dealing with interlacing passes, etc. */
PNG_EXTERN void png_read_finish_row PNGARG((png_structp png_ptr));

/* Initialize the row buffers, etc. */
PNG_EXTERN void png_read_start_row PNGARG((png_structp png_ptr));

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
/* Optional call to update the users info structure */
PNG_EXTERN void png_read_transform_info PNGARG((png_structp png_ptr,
    png_infop info_ptr));
#endif

/* These are the functions that do the transformations */
#ifdef PNG_READ_FILLER_SUPPORTED
PNG_EXTERN void png_do_read_filler PNGARG((png_row_infop row_info,
    png_bytep row, png_uint_32 filler, png_uint_32 flags));
#endif

#ifdef PNG_READ_SWAP_ALPHA_SUPPORTED
PNG_EXTERN void png_do_read_swap_alpha PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_WRITE_SWAP_ALPHA_SUPPORTED
PNG_EXTERN void png_do_write_swap_alpha PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_READ_INVERT_ALPHA_SUPPORTED
PNG_EXTERN void png_do_read_invert_alpha PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_WRITE_INVERT_ALPHA_SUPPORTED
PNG_EXTERN void png_do_write_invert_alpha PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#if defined(PNG_WRITE_FILLER_SUPPORTED) || \
    defined(PNG_READ_STRIP_ALPHA_SUPPORTED)
PNG_EXTERN void png_do_strip_channel PNGARG((png_row_infop row_info,
    png_bytep row, int at_start));
#endif

#ifdef PNG_16BIT_SUPPORTED
#if defined(PNG_READ_SWAP_SUPPORTED) || defined(PNG_WRITE_SWAP_SUPPORTED)
PNG_EXTERN void png_do_swap PNGARG((png_row_infop row_info,
    png_bytep row));
#endif
#endif

#if defined(PNG_READ_PACKSWAP_SUPPORTED) || \
    defined(PNG_WRITE_PACKSWAP_SUPPORTED)
PNG_EXTERN void png_do_packswap PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_READ_RGB_TO_GRAY_SUPPORTED
PNG_EXTERN int png_do_rgb_to_gray PNGARG((png_structp png_ptr,
    png_row_infop row_info, png_bytep row));
#endif

#ifdef PNG_READ_GRAY_TO_RGB_SUPPORTED
PNG_EXTERN void png_do_gray_to_rgb PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_READ_PACK_SUPPORTED
PNG_EXTERN void png_do_unpack PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_READ_SHIFT_SUPPORTED
PNG_EXTERN void png_do_unshift PNGARG((png_row_infop row_info,
    png_bytep row, png_const_color_8p sig_bits));
#endif

#if defined(PNG_READ_INVERT_SUPPORTED) || defined(PNG_WRITE_INVERT_SUPPORTED)
PNG_EXTERN void png_do_invert PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED
PNG_EXTERN void png_do_scale_16_to_8 PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_READ_STRIP_16_TO_8_SUPPORTED
PNG_EXTERN void png_do_chop PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_READ_QUANTIZE_SUPPORTED
PNG_EXTERN void png_do_quantize PNGARG((png_row_infop row_info,
    png_bytep row, png_const_bytep palette_lookup,
    png_const_bytep quantize_lookup));

#  ifdef PNG_CORRECT_PALETTE_SUPPORTED
PNG_EXTERN void png_correct_palette PNGARG((png_structp png_ptr,
    png_colorp palette, int num_palette));
#  endif
#endif

#if defined(PNG_READ_BGR_SUPPORTED) || defined(PNG_WRITE_BGR_SUPPORTED)
PNG_EXTERN void png_do_bgr PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

#ifdef PNG_WRITE_PACK_SUPPORTED
PNG_EXTERN void png_do_pack PNGARG((png_row_infop row_info,
   png_bytep row, png_uint_32 bit_depth));
#endif

#ifdef PNG_WRITE_SHIFT_SUPPORTED
PNG_EXTERN void png_do_shift PNGARG((png_row_infop row_info,
    png_bytep row, png_const_color_8p bit_depth));
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED) ||\
    defined(PNG_READ_ALPHA_MODE_SUPPORTED)
PNG_EXTERN void png_do_compose PNGARG((png_row_infop row_info,
    png_bytep row, png_structp png_ptr));
#endif

#ifdef PNG_READ_GAMMA_SUPPORTED
PNG_EXTERN void png_do_gamma PNGARG((png_row_infop row_info,
    png_bytep row, png_structp png_ptr));
#endif

#ifdef PNG_READ_ALPHA_MODE_SUPPORTED
PNG_EXTERN void png_do_encode_alpha PNGARG((png_row_infop row_info,
   png_bytep row, png_structp png_ptr));
#endif

#ifdef PNG_READ_EXPAND_SUPPORTED
PNG_EXTERN void png_do_expand_palette PNGARG((png_row_infop row_info,
    png_bytep row, png_const_colorp palette, png_const_bytep trans,
    int num_trans));
PNG_EXTERN void png_do_expand PNGARG((png_row_infop row_info,
    png_bytep row, png_const_color_16p trans_color));
#endif

#ifdef PNG_READ_EXPAND_16_SUPPORTED
PNG_EXTERN void png_do_expand_16 PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

/* The following decodes the appropriate chunks, and does error correction,
 * then calls the appropriate callback for the chunk if it is valid.
 */

/* Decode the IHDR chunk */
PNG_EXTERN void png_handle_IHDR PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
PNG_EXTERN void png_handle_PLTE PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
PNG_EXTERN void png_handle_IEND PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));

#ifdef PNG_READ_bKGD_SUPPORTED
PNG_EXTERN void png_handle_bKGD PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_cHRM_SUPPORTED
PNG_EXTERN void png_handle_cHRM PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_gAMA_SUPPORTED
PNG_EXTERN void png_handle_gAMA PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_hIST_SUPPORTED
PNG_EXTERN void png_handle_hIST PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_iCCP_SUPPORTED
PNG_EXTERN void png_handle_iCCP PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif /* PNG_READ_iCCP_SUPPORTED */

#ifdef PNG_READ_iTXt_SUPPORTED
PNG_EXTERN void png_handle_iTXt PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_oFFs_SUPPORTED
PNG_EXTERN void png_handle_oFFs PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_pCAL_SUPPORTED
PNG_EXTERN void png_handle_pCAL PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_pHYs_SUPPORTED
PNG_EXTERN void png_handle_pHYs PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_sBIT_SUPPORTED
PNG_EXTERN void png_handle_sBIT PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_sCAL_SUPPORTED
PNG_EXTERN void png_handle_sCAL PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_sPLT_SUPPORTED
PNG_EXTERN void png_handle_sPLT PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif /* PNG_READ_sPLT_SUPPORTED */

#ifdef PNG_READ_sRGB_SUPPORTED
PNG_EXTERN void png_handle_sRGB PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_tEXt_SUPPORTED
PNG_EXTERN void png_handle_tEXt PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_tIME_SUPPORTED
PNG_EXTERN void png_handle_tIME PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_tRNS_SUPPORTED
PNG_EXTERN void png_handle_tRNS PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

#ifdef PNG_READ_zTXt_SUPPORTED
PNG_EXTERN void png_handle_zTXt PNGARG((png_structp png_ptr, png_infop info_ptr,
    png_uint_32 length));
#endif

PNG_EXTERN void png_handle_unknown PNGARG((png_structp png_ptr,
    png_infop info_ptr, png_uint_32 length));

PNG_EXTERN void png_check_chunk_name PNGARG((png_structp png_ptr,
    png_const_bytep chunk_name));

/* Handle the transformations for reading and writing */
#ifdef PNG_READ_TRANSFORMS_SUPPORTED
PNG_EXTERN void png_do_read_transformations PNGARG((png_structp png_ptr));
#endif
#ifdef PNG_WRITE_TRANSFORMS_SUPPORTED
PNG_EXTERN void png_do_write_transformations PNGARG((png_structp png_ptr));
#endif

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
PNG_EXTERN void png_init_read_transformations PNGARG((png_structp png_ptr));
#endif

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
PNG_EXTERN void png_push_read_chunk PNGARG((png_structp png_ptr,
    png_infop info_ptr));
PNG_EXTERN void png_push_read_sig PNGARG((png_structp png_ptr,
    png_infop info_ptr));
PNG_EXTERN void png_push_check_crc PNGARG((png_structp png_ptr));
PNG_EXTERN void png_push_crc_skip PNGARG((png_structp png_ptr,
    png_uint_32 length));
PNG_EXTERN void png_push_crc_finish PNGARG((png_structp png_ptr));
PNG_EXTERN void png_push_save_buffer PNGARG((png_structp png_ptr));
PNG_EXTERN void png_push_restore_buffer PNGARG((png_structp png_ptr,
    png_bytep buffer, png_size_t buffer_length));
PNG_EXTERN void png_push_read_IDAT PNGARG((png_structp png_ptr));
PNG_EXTERN void png_process_IDAT_data PNGARG((png_structp png_ptr,
    png_bytep buffer, png_size_t buffer_length));
PNG_EXTERN void png_push_process_row PNGARG((png_structp png_ptr));
PNG_EXTERN void png_push_handle_unknown PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 length));
PNG_EXTERN void png_push_have_info PNGARG((png_structp png_ptr,
   png_infop info_ptr));
PNG_EXTERN void png_push_have_end PNGARG((png_structp png_ptr,
   png_infop info_ptr));
PNG_EXTERN void png_push_have_row PNGARG((png_structp png_ptr, png_bytep row));
PNG_EXTERN void png_push_read_end PNGARG((png_structp png_ptr,
    png_infop info_ptr));
PNG_EXTERN void png_process_some_data PNGARG((png_structp png_ptr,
    png_infop info_ptr));
PNG_EXTERN void png_read_push_finish_row PNGARG((png_structp png_ptr));
#  ifdef PNG_READ_tEXt_SUPPORTED
PNG_EXTERN void png_push_handle_tEXt PNGARG((png_structp png_ptr,
    png_infop info_ptr, png_uint_32 length));
PNG_EXTERN void png_push_read_tEXt PNGARG((png_structp png_ptr,
    png_infop info_ptr));
#  endif
#  ifdef PNG_READ_zTXt_SUPPORTED
PNG_EXTERN void png_push_handle_zTXt PNGARG((png_structp png_ptr,
    png_infop info_ptr, png_uint_32 length));
PNG_EXTERN void png_push_read_zTXt PNGARG((png_structp png_ptr,
    png_infop info_ptr));
#  endif
#  ifdef PNG_READ_iTXt_SUPPORTED
PNG_EXTERN void png_push_handle_iTXt PNGARG((png_structp png_ptr,
    png_infop info_ptr, png_uint_32 length));
PNG_EXTERN void png_push_read_iTXt PNGARG((png_structp png_ptr,
    png_infop info_ptr));
#  endif

#endif /* PNG_PROGRESSIVE_READ_SUPPORTED */

#ifdef PNG_MNG_FEATURES_SUPPORTED
PNG_EXTERN void png_do_read_intrapixel PNGARG((png_row_infop row_info,
    png_bytep row));
PNG_EXTERN void png_do_write_intrapixel PNGARG((png_row_infop row_info,
    png_bytep row));
#endif

/* Added at libpng version 1.4.0 */
#ifdef PNG_CHECK_cHRM_SUPPORTED
PNG_EXTERN int png_check_cHRM_fixed PNGARG((png_structp png_ptr,
    png_fixed_point int_white_x, png_fixed_point int_white_y,
    png_fixed_point int_red_x, png_fixed_point int_red_y, png_fixed_point
    int_green_x, png_fixed_point int_green_y, png_fixed_point int_blue_x,
    png_fixed_point int_blue_y));
#endif

#ifdef PNG_CHECK_cHRM_SUPPORTED
/* Added at libpng version 1.2.34 and 1.4.0 */
/* Currently only used by png_check_cHRM_fixed */
PNG_EXTERN void png_64bit_product PNGARG((long v1, long v2,
    unsigned long *hi_product, unsigned long *lo_product));
#endif

/* Added at libpng version 1.4.0 */
PNG_EXTERN void png_check_IHDR PNGARG((png_structp png_ptr,
    png_uint_32 width, png_uint_32 height, int bit_depth,
    int color_type, int interlace_type, int compression_type,
    int filter_type));

/* Free all memory used by the read (old method - NOT DLL EXPORTED) */
PNG_EXTERN void png_read_destroy PNGARG((png_structp png_ptr,
    png_infop info_ptr, png_infop end_info_ptr));

/* Free any memory used in png_ptr struct (old method - NOT DLL EXPORTED) */
PNG_EXTERN void png_write_destroy PNGARG((png_structp png_ptr));

#ifdef USE_FAR_KEYWORD  /* memory model conversion function */
PNG_EXTERN void *png_far_to_near PNGARG((png_structp png_ptr, png_voidp ptr,
    int check));
#endif /* USE_FAR_KEYWORD */

#if defined(PNG_FLOATING_POINT_SUPPORTED) && defined(PNG_ERROR_TEXT_SUPPORTED)
PNG_EXTERN PNG_FUNCTION(void, png_fixed_error, (png_structp png_ptr,
   png_const_charp name),PNG_NORETURN);
#endif

/* Puts 'string' into 'buffer' at buffer[pos], taking care never to overwrite
 * the end.  Always leaves the buffer nul terminated.  Never errors out (and
 * there is no error code.)
 */
PNG_EXTERN size_t png_safecat(png_charp buffer, size_t bufsize, size_t pos,
    png_const_charp string);

/* Various internal functions to handle formatted warning messages, currently
 * only implemented for warnings.
 */
#if defined(PNG_WARNINGS_SUPPORTED) || defined(PNG_TIME_RFC1123_SUPPORTED)
/* Utility to dump an unsigned value into a buffer, given a start pointer and
 * and end pointer (which should point just *beyond* the end of the buffer!)
 * Returns the pointer to the start of the formatted string.  This utility only
 * does unsigned values.
 */
PNG_EXTERN png_charp png_format_number(png_const_charp start, png_charp end,
   int format, png_alloc_size_t number);

/* Convenience macro that takes an array: */
#define PNG_FORMAT_NUMBER(buffer,format,number) \
   png_format_number(buffer, buffer + (sizeof buffer), format, number)

/* Suggested size for a number buffer (enough for 64 bits and a sign!) */
#define PNG_NUMBER_BUFFER_SIZE 24

/* These are the integer formats currently supported, the name is formed from
 * the standard printf(3) format string.
 */
#define PNG_NUMBER_FORMAT_u     1 /* chose unsigned API! */
#define PNG_NUMBER_FORMAT_02u   2
#define PNG_NUMBER_FORMAT_d     1 /* chose signed API! */
#define PNG_NUMBER_FORMAT_02d   2
#define PNG_NUMBER_FORMAT_x     3
#define PNG_NUMBER_FORMAT_02x   4
#define PNG_NUMBER_FORMAT_fixed 5 /* choose the signed API */
#endif

#ifdef PNG_WARNINGS_SUPPORTED
/* New defines and members adding in libpng-1.5.4 */
#  define PNG_WARNING_PARAMETER_SIZE 32
#  define PNG_WARNING_PARAMETER_COUNT 8

/* An l-value of this type has to be passed to the APIs below to cache the
 * values of the parameters to a formatted warning message.
 */
typedef char png_warning_parameters[PNG_WARNING_PARAMETER_COUNT][
   PNG_WARNING_PARAMETER_SIZE];

PNG_EXTERN void png_warning_parameter(png_warning_parameters p, int number,
    png_const_charp string);
    /* Parameters are limited in size to PNG_WARNING_PARAMETER_SIZE characters,
     * including the trailing '\0'.
     */
PNG_EXTERN void png_warning_parameter_unsigned(png_warning_parameters p,
    int number, int format, png_alloc_size_t value);
    /* Use png_alloc_size_t because it is an unsigned type as big as any we
     * need to output.  Use the following for a signed value.
     */
PNG_EXTERN void png_warning_parameter_signed(png_warning_parameters p,
    int number, int format, png_int_32 value);

PNG_EXTERN void png_formatted_warning(png_structp png_ptr,
    png_warning_parameters p, png_const_charp message);
    /* 'message' follows the X/Open approach of using @1, @2 to insert
     * parameters previously supplied using the above functions.  Errors in
     * specifying the paramters will simple result in garbage substitutions.
     */
#endif

/* ASCII to FP interfaces, currently only implemented if sCAL
 * support is required.
 */
#if defined(PNG_READ_sCAL_SUPPORTED)
/* MAX_DIGITS is actually the maximum number of characters in an sCAL
 * width or height, derived from the precision (number of significant
 * digits - a build time settable option) and assumpitions about the
 * maximum ridiculous exponent.
 */
#define PNG_sCAL_MAX_DIGITS (PNG_sCAL_PRECISION+1/*.*/+1/*E*/+10/*exponent*/)

#ifdef PNG_FLOATING_POINT_SUPPORTED
PNG_EXTERN void png_ascii_from_fp PNGARG((png_structp png_ptr, png_charp ascii,
    png_size_t size, double fp, unsigned int precision));
#endif /* FLOATING_POINT */

#ifdef PNG_FIXED_POINT_SUPPORTED
PNG_EXTERN void png_ascii_from_fixed PNGARG((png_structp png_ptr,
    png_charp ascii, png_size_t size, png_fixed_point fp));
#endif /* FIXED_POINT */
#endif /* READ_sCAL */

#if defined(PNG_sCAL_SUPPORTED) || defined(PNG_pCAL_SUPPORTED)
/* An internal API to validate the format of a floating point number.
 * The result is the index of the next character.  If the number is
 * not valid it will be the index of a character in the supposed number.
 *
 * The format of a number is defined in the PNG extensions specification
 * and this API is strictly conformant to that spec, not anyone elses!
 *
 * The format as a regular expression is:
 *
 * [+-]?[0-9]+.?([Ee][+-]?[0-9]+)?
 *
 * or:
 *
 * [+-]?.[0-9]+(.[0-9]+)?([Ee][+-]?[0-9]+)?
 *
 * The complexity is that either integer or fraction must be present and the
 * fraction is permitted to have no digits only if the integer is present.
 *
 * NOTE: The dangling E problem.
 *   There is a PNG valid floating point number in the following:
 *
 *       PNG floating point numb1.ers are not greedy.
 *
 *   Working this out requires *TWO* character lookahead (because of the
 *   sign), the parser does not do this - it will fail at the 'r' - this
 *   doesn't matter for PNG sCAL chunk values, but it requires more care
 *   if the value were ever to be embedded in something more complex.  Use
 *   ANSI-C strtod if you need the lookahead.
 */
/* State table for the parser. */
#define PNG_FP_INTEGER    0  /* before or in integer */
#define PNG_FP_FRACTION   1  /* before or in fraction */
#define PNG_FP_EXPONENT   2  /* before or in exponent */
#define PNG_FP_STATE      3  /* mask for the above */
#define PNG_FP_SAW_SIGN   4  /* Saw +/- in current state */
#define PNG_FP_SAW_DIGIT  8  /* Saw a digit in current state */
#define PNG_FP_SAW_DOT   16  /* Saw a dot in current state */
#define PNG_FP_SAW_E     32  /* Saw an E (or e) in current state */
#define PNG_FP_SAW_ANY   60  /* Saw any of the above 4 */

/* These three values don't affect the parser.  They are set but not used.
 */
#define PNG_FP_WAS_VALID 64  /* Preceding substring is a valid fp number */
#define PNG_FP_NEGATIVE 128  /* A negative number, including "-0" */
#define PNG_FP_NONZERO  256  /* A non-zero value */
#define PNG_FP_STICKY   448  /* The above three flags */

/* This is available for the caller to store in 'state' if required.  Do not
 * call the parser after setting it (the parser sometimes clears it.)
 */
#define PNG_FP_INVALID  512  /* Available for callers as a distinct value */

/* Result codes for the parser (boolean - true meants ok, false means
 * not ok yet.)
 */
#define PNG_FP_MAYBE      0  /* The number may be valid in the future */
#define PNG_FP_OK         1  /* The number is valid */

/* Tests on the sticky non-zero and negative flags.  To pass these checks
 * the state must also indicate that the whole number is valid - this is
 * achieved by testing PNG_FP_SAW_DIGIT (see the implementation for why this
 * is equivalent to PNG_FP_OK above.)
 */
#define PNG_FP_NZ_MASK (PNG_FP_SAW_DIGIT | PNG_FP_NEGATIVE | PNG_FP_NONZERO)
   /* NZ_MASK: the string is valid and a non-zero negative value */
#define PNG_FP_Z_MASK (PNG_FP_SAW_DIGIT | PNG_FP_NONZERO)
   /* Z MASK: the string is valid and a non-zero value. */
   /* PNG_FP_SAW_DIGIT: the string is valid. */
#define PNG_FP_IS_ZERO(state) (((state) & PNG_FP_Z_MASK) == PNG_FP_SAW_DIGIT)
#define PNG_FP_IS_POSITIVE(state) (((state) & PNG_FP_NZ_MASK) == PNG_FP_Z_MASK)
#define PNG_FP_IS_NEGATIVE(state) (((state) & PNG_FP_NZ_MASK) == PNG_FP_NZ_MASK)

/* The actual parser.  This can be called repeatedly, it updates
 * the index into the string and the state variable (which must
 * be initialzed to 0).  It returns a result code, as above.  There
 * is no point calling the parser any more if it fails to advance to
 * the end of the string - it is stuck on an invalid character (or
 * terminated by '\0').
 *
 * Note that the pointer will consume an E or even an E+ then leave
 * a 'maybe' state even though a preceding integer.fraction is valid.
 * The PNG_FP_WAS_VALID flag indicates that a preceding substring was
 * a valid number.  It's possible to recover from this by calling
 * the parser again (from the start, with state 0) but with a string
 * that omits the last character (i.e. set the size to the index of
 * the problem character.)  This has not been tested within libpng.
 */
PNG_EXTERN int png_check_fp_number PNGARG((png_const_charp string,
    png_size_t size, int *statep, png_size_tp whereami));

/* This is the same but it checks a complete string and returns true
 * only if it just contains a floating point number.  As of 1.5.4 this
 * function also returns the state at the end of parsing the number if
 * it was valid (otherwise it returns 0.)  This can be used for testing
 * for negative or zero values using the sticky flag.
 */
PNG_EXTERN int png_check_fp_string PNGARG((png_const_charp string,
    png_size_t size));
#endif /* pCAL || sCAL */

#if defined(PNG_READ_GAMMA_SUPPORTED) ||\
    defined(PNG_INCH_CONVERSIONS_SUPPORTED) || defined(PNG_READ_pHYs_SUPPORTED)
/* Added at libpng version 1.5.0 */
/* This is a utility to provide a*times/div (rounded) and indicate
 * if there is an overflow.  The result is a boolean - false (0)
 * for overflow, true (1) if no overflow, in which case *res
 * holds the result.
 */
PNG_EXTERN int png_muldiv PNGARG((png_fixed_point_p res, png_fixed_point a,
    png_int_32 multiplied_by, png_int_32 divided_by));
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_INCH_CONVERSIONS_SUPPORTED)
/* Same deal, but issue a warning on overflow and return 0. */
PNG_EXTERN png_fixed_point png_muldiv_warn PNGARG((png_structp png_ptr,
    png_fixed_point a, png_int_32 multiplied_by, png_int_32 divided_by));
#endif

#ifdef PNG_READ_GAMMA_SUPPORTED
/* Calculate a reciprocal - used for gamma values.  This returns
 * 0 if the argument is 0 in order to maintain an undefined value,
 * there are no warnings.
 */
PNG_EXTERN png_fixed_point png_reciprocal PNGARG((png_fixed_point a));

/* The same but gives a reciprocal of the product of two fixed point
 * values.  Accuracy is suitable for gamma calculations but this is
 * not exact - use png_muldiv for that.
 */
PNG_EXTERN png_fixed_point png_reciprocal2 PNGARG((png_fixed_point a,
    png_fixed_point b));
#endif

#ifdef PNG_READ_GAMMA_SUPPORTED
/* Internal fixed point gamma correction.  These APIs are called as
 * required to convert single values - they don't need to be fast,
 * they are not used when processing image pixel values.
 *
 * While the input is an 'unsigned' value it must actually be the
 * correct bit value - 0..255 or 0..65535 as required.
 */
PNG_EXTERN png_uint_16 png_gamma_correct PNGARG((png_structp png_ptr,
    unsigned int value, png_fixed_point gamma_value));
PNG_EXTERN int png_gamma_significant PNGARG((png_fixed_point gamma_value));
PNG_EXTERN png_uint_16 png_gamma_16bit_correct PNGARG((unsigned int value,
    png_fixed_point gamma_value));
PNG_EXTERN png_byte png_gamma_8bit_correct PNGARG((unsigned int value,
    png_fixed_point gamma_value));
PNG_EXTERN void png_build_gamma_table PNGARG((png_structp png_ptr,
    int bit_depth));
#endif

/* Maintainer: Put new private prototypes here ^ and in libpngpf.3 */


} // PrivatePng

#include "pngdebug.h"

#endif /* PNGPRIV_H */
