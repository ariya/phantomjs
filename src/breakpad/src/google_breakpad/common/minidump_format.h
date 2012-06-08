/* Copyright (c) 2006, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

/* minidump_format.h: A cross-platform reimplementation of minidump-related
 * portions of DbgHelp.h from the Windows Platform SDK.
 *
 * (This is C99 source, please don't corrupt it with C++.)
 *
 * Structures that are defined by Microsoft to contain a zero-length array
 * are instead defined here to contain an array with one element, as
 * zero-length arrays are forbidden by standard C and C++.  In these cases,
 * *_minsize constants are provided to be used in place of sizeof.  For a
 * cleaner interface to these sizes when using C++, see minidump_size.h.
 *
 * These structures are also sufficient to populate minidump files.
 *
 * These definitions may be extended to support handling minidump files
 * for other CPUs and other operating systems.
 *
 * Because precise data type sizes are crucial for this implementation to
 * function properly and portably in terms of interoperability with minidumps
 * produced by DbgHelp on Windows, a set of primitive types with known sizes
 * are used as the basis of each structure defined by this file.  DbgHelp
 * on Windows is assumed to be the reference implementation; this file
 * seeks to provide a cross-platform compatible implementation.  To avoid
 * collisions with the types and values defined and used by DbgHelp in the
 * event that this implementation is used on Windows, each type and value
 * defined here is given a new name, beginning with "MD".  Names of the
 * equivalent types and values in the Windows Platform SDK are given in
 * comments.
 *
 * Author: Mark Mentovai */


#ifndef GOOGLE_BREAKPAD_COMMON_MINIDUMP_FORMAT_H__
#define GOOGLE_BREAKPAD_COMMON_MINIDUMP_FORMAT_H__

#include <stddef.h>

#include "google_breakpad/common/breakpad_types.h"


#if defined(_MSC_VER)
/* Disable "zero-sized array in struct/union" warnings when compiling in
 * MSVC.  DbgHelp.h does this too. */
#pragma warning(push)
#pragma warning(disable:4200)
#endif  /* _MSC_VER */


/*
 * guiddef.h
 */

typedef struct {
  u_int32_t data1;
  u_int16_t data2;
  u_int16_t data3;
  u_int8_t  data4[8];
} MDGUID;  /* GUID */


/*
 * WinNT.h
 */

/* Non-x86 CPU identifiers found in the high 24 bits of
 * (MDRawContext*).context_flags.  These aren't used by Breakpad, but are
 * defined here for reference, to avoid assigning values that conflict
 * (although some values already conflict). */
#define MD_CONTEXT_IA64  0x00080000  /* CONTEXT_IA64 */
/* Additional values from winnt.h in the Windows CE 5.0 SDK: */
#define MD_CONTEXT_SHX   0x000000c0  /* CONTEXT_SH4 (Super-H, includes SH3) */
#define MD_CONTEXT_MIPS  0x00010000  /* CONTEXT_R4000 (same value as x86?) */
#define MD_CONTEXT_ALPHA 0x00020000  /* CONTEXT_ALPHA */

/* As of Windows 7 SP1, the number of flag bits has increased to
 * include 0x40 (CONTEXT_XSTATE):
 * http://msdn.microsoft.com/en-us/library/hh134238%28v=vs.85%29.aspx */
#define MD_CONTEXT_CPU_MASK 0xffffff00


/* This is a base type for MDRawContextX86 and MDRawContextPPC.  This
 * structure should never be allocated directly.  The actual structure type
 * can be determined by examining the context_flags field. */
typedef struct {
  u_int32_t context_flags;
} MDRawContextBase;

#include "minidump_cpu_amd64.h"
#include "minidump_cpu_arm.h"
#include "minidump_cpu_ppc.h"
#include "minidump_cpu_ppc64.h"
#include "minidump_cpu_sparc.h"
#include "minidump_cpu_x86.h"

/*
 * WinVer.h
 */


typedef struct {
  u_int32_t signature;
  u_int32_t struct_version;
  u_int32_t file_version_hi;
  u_int32_t file_version_lo;
  u_int32_t product_version_hi;
  u_int32_t product_version_lo;
  u_int32_t file_flags_mask;    /* Identifies valid bits in fileFlags */
  u_int32_t file_flags;
  u_int32_t file_os;
  u_int32_t file_type;
  u_int32_t file_subtype;
  u_int32_t file_date_hi;
  u_int32_t file_date_lo;
} MDVSFixedFileInfo;  /* VS_FIXEDFILEINFO */

/* For (MDVSFixedFileInfo).signature */
#define MD_VSFIXEDFILEINFO_SIGNATURE 0xfeef04bd
     /* VS_FFI_SIGNATURE */

/* For (MDVSFixedFileInfo).version */
#define MD_VSFIXEDFILEINFO_VERSION 0x00010000
     /* VS_FFI_STRUCVERSION */

/* For (MDVSFixedFileInfo).file_flags_mask and
 * (MDVSFixedFileInfo).file_flags */
#define MD_VSFIXEDFILEINFO_FILE_FLAGS_DEBUG        0x00000001
     /* VS_FF_DEBUG */
#define MD_VSFIXEDFILEINFO_FILE_FLAGS_PRERELEASE   0x00000002
     /* VS_FF_PRERELEASE */
#define MD_VSFIXEDFILEINFO_FILE_FLAGS_PATCHED      0x00000004
     /* VS_FF_PATCHED */
#define MD_VSFIXEDFILEINFO_FILE_FLAGS_PRIVATEBUILD 0x00000008
     /* VS_FF_PRIVATEBUILD */
#define MD_VSFIXEDFILEINFO_FILE_FLAGS_INFOINFERRED 0x00000010
     /* VS_FF_INFOINFERRED */
#define MD_VSFIXEDFILEINFO_FILE_FLAGS_SPECIALBUILD 0x00000020
     /* VS_FF_SPECIALBUILD */

/* For (MDVSFixedFileInfo).file_os: high 16 bits */
#define MD_VSFIXEDFILEINFO_FILE_OS_UNKNOWN    0          /* VOS_UNKNOWN */
#define MD_VSFIXEDFILEINFO_FILE_OS_DOS        (1 << 16)  /* VOS_DOS */
#define MD_VSFIXEDFILEINFO_FILE_OS_OS216      (2 << 16)  /* VOS_OS216 */
#define MD_VSFIXEDFILEINFO_FILE_OS_OS232      (3 << 16)  /* VOS_OS232 */
#define MD_VSFIXEDFILEINFO_FILE_OS_NT         (4 << 16)  /* VOS_NT */
#define MD_VSFIXEDFILEINFO_FILE_OS_WINCE      (5 << 16)  /* VOS_WINCE */
/* Low 16 bits */
#define MD_VSFIXEDFILEINFO_FILE_OS__BASE      0          /* VOS__BASE */
#define MD_VSFIXEDFILEINFO_FILE_OS__WINDOWS16 1          /* VOS__WINDOWS16 */
#define MD_VSFIXEDFILEINFO_FILE_OS__PM16      2          /* VOS__PM16 */
#define MD_VSFIXEDFILEINFO_FILE_OS__PM32      3          /* VOS__PM32 */
#define MD_VSFIXEDFILEINFO_FILE_OS__WINDOWS32 4          /* VOS__WINDOWS32 */

/* For (MDVSFixedFileInfo).file_type */
#define MD_VSFIXEDFILEINFO_FILE_TYPE_UNKNOWN    0  /* VFT_UNKNOWN */
#define MD_VSFIXEDFILEINFO_FILE_TYPE_APP        1  /* VFT_APP */
#define MD_VSFIXEDFILEINFO_FILE_TYPE_DLL        2  /* VFT_DLL */
#define MD_VSFIXEDFILEINFO_FILE_TYPE_DRV        3  /* VFT_DLL */
#define MD_VSFIXEDFILEINFO_FILE_TYPE_FONT       4  /* VFT_FONT */
#define MD_VSFIXEDFILEINFO_FILE_TYPE_VXD        5  /* VFT_VXD */
#define MD_VSFIXEDFILEINFO_FILE_TYPE_STATIC_LIB 7  /* VFT_STATIC_LIB */

/* For (MDVSFixedFileInfo).file_subtype */
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_UNKNOWN                0
     /* VFT2_UNKNOWN */
/* with file_type = MD_VSFIXEDFILEINFO_FILETYPE_DRV */
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_PRINTER            1
     /* VFT2_DRV_PRINTER */
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_KEYBOARD           2
     /* VFT2_DRV_KEYBOARD */
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_LANGUAGE           3
     /* VFT2_DRV_LANGUAGE */
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_DISPLAY            4
     /* VFT2_DRV_DISPLAY */
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_MOUSE              5
     /* VFT2_DRV_MOUSE */
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_NETWORK            6
     /* VFT2_DRV_NETWORK */
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_SYSTEM             7
     /* VFT2_DRV_SYSTEM */
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_INSTALLABLE        8
     /* VFT2_DRV_INSTALLABLE */
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_SOUND              9
     /* VFT2_DRV_SOUND */
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_COMM              10
     /* VFT2_DRV_COMM */
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_INPUTMETHOD       11
     /* VFT2_DRV_INPUTMETHOD */
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_VERSIONED_PRINTER 12
     /* VFT2_DRV_VERSIONED_PRINTER */
/* with file_type = MD_VSFIXEDFILEINFO_FILETYPE_FONT */
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_FONT_RASTER            1
     /* VFT2_FONT_RASTER */
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_FONT_VECTOR            2
     /* VFT2_FONT_VECTOR */
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_FONT_TRUETYPE          3
     /* VFT2_FONT_TRUETYPE */


/*
 * DbgHelp.h
 */


/* An MDRVA is an offset into the minidump file.  The beginning of the
 * MDRawHeader is at offset 0. */
typedef u_int32_t MDRVA;  /* RVA */

typedef struct {
  u_int32_t data_size;
  MDRVA     rva;
} MDLocationDescriptor;  /* MINIDUMP_LOCATION_DESCRIPTOR */


typedef struct {
  /* The base address of the memory range on the host that produced the
   * minidump. */
  u_int64_t            start_of_memory_range;

  MDLocationDescriptor memory;
} MDMemoryDescriptor;  /* MINIDUMP_MEMORY_DESCRIPTOR */


typedef struct {
  u_int32_t signature;
  u_int32_t version;
  u_int32_t stream_count;
  MDRVA     stream_directory_rva;  /* A |stream_count|-sized array of
                                    * MDRawDirectory structures. */
  u_int32_t checksum;              /* Can be 0.  In fact, that's all that's
                                    * been found in minidump files. */
  u_int32_t time_date_stamp;       /* time_t */
  u_int64_t flags;
} MDRawHeader;  /* MINIDUMP_HEADER */

/* For (MDRawHeader).signature and (MDRawHeader).version.  Note that only the
 * low 16 bits of (MDRawHeader).version are MD_HEADER_VERSION.  Per the
 * documentation, the high 16 bits are implementation-specific. */
#define MD_HEADER_SIGNATURE 0x504d444d /* 'PMDM' */
     /* MINIDUMP_SIGNATURE */
#define MD_HEADER_VERSION   0x0000a793 /* 42899 */
     /* MINIDUMP_VERSION */

/* For (MDRawHeader).flags: */
typedef enum {
  /* MD_NORMAL is the standard type of minidump.  It includes full
   * streams for the thread list, module list, exception, system info,
   * and miscellaneous info.  A memory list stream is also present,
   * pointing to the same stack memory contained in the thread list,
   * as well as a 256-byte region around the instruction address that
   * was executing when the exception occurred.  Stack memory is from
   * 4 bytes below a thread's stack pointer up to the top of the
   * memory region encompassing the stack. */
  MD_NORMAL                            = 0x00000000,
  MD_WITH_DATA_SEGS                    = 0x00000001,
  MD_WITH_FULL_MEMORY                  = 0x00000002,
  MD_WITH_HANDLE_DATA                  = 0x00000004,
  MD_FILTER_MEMORY                     = 0x00000008,
  MD_SCAN_MEMORY                       = 0x00000010,
  MD_WITH_UNLOADED_MODULES             = 0x00000020,
  MD_WITH_INDIRECTLY_REFERENCED_MEMORY = 0x00000040,
  MD_FILTER_MODULE_PATHS               = 0x00000080,
  MD_WITH_PROCESS_THREAD_DATA          = 0x00000100,
  MD_WITH_PRIVATE_READ_WRITE_MEMORY    = 0x00000200,
  MD_WITHOUT_OPTIONAL_DATA             = 0x00000400,
  MD_WITH_FULL_MEMORY_INFO             = 0x00000800,
  MD_WITH_THREAD_INFO                  = 0x00001000,
  MD_WITH_CODE_SEGS                    = 0x00002000,
  MD_WITHOUT_AUXILLIARY_SEGS           = 0x00004000,
  MD_WITH_FULL_AUXILLIARY_STATE        = 0x00008000,
  MD_WITH_PRIVATE_WRITE_COPY_MEMORY    = 0x00010000,
  MD_IGNORE_INACCESSIBLE_MEMORY        = 0x00020000,
  MD_WITH_TOKEN_INFORMATION            = 0x00040000
} MDType;  /* MINIDUMP_TYPE */


typedef struct {
  u_int32_t            stream_type;
  MDLocationDescriptor location;
} MDRawDirectory;  /* MINIDUMP_DIRECTORY */

/* For (MDRawDirectory).stream_type */
typedef enum {
  MD_UNUSED_STREAM               =  0,
  MD_RESERVED_STREAM_0           =  1,
  MD_RESERVED_STREAM_1           =  2,
  MD_THREAD_LIST_STREAM          =  3,  /* MDRawThreadList */
  MD_MODULE_LIST_STREAM          =  4,  /* MDRawModuleList */
  MD_MEMORY_LIST_STREAM          =  5,  /* MDRawMemoryList */
  MD_EXCEPTION_STREAM            =  6,  /* MDRawExceptionStream */
  MD_SYSTEM_INFO_STREAM          =  7,  /* MDRawSystemInfo */
  MD_THREAD_EX_LIST_STREAM       =  8,
  MD_MEMORY_64_LIST_STREAM       =  9,
  MD_COMMENT_STREAM_A            = 10,
  MD_COMMENT_STREAM_W            = 11,
  MD_HANDLE_DATA_STREAM          = 12,
  MD_FUNCTION_TABLE_STREAM       = 13,
  MD_UNLOADED_MODULE_LIST_STREAM = 14,
  MD_MISC_INFO_STREAM            = 15,  /* MDRawMiscInfo */
  MD_MEMORY_INFO_LIST_STREAM     = 16,  /* MDRawMemoryInfoList */
  MD_THREAD_INFO_LIST_STREAM     = 17,
  MD_HANDLE_OPERATION_LIST_STREAM = 18,
  MD_LAST_RESERVED_STREAM        = 0x0000ffff,

  /* Breakpad extension types.  0x4767 = "Gg" */
  MD_BREAKPAD_INFO_STREAM        = 0x47670001,  /* MDRawBreakpadInfo */
  MD_ASSERTION_INFO_STREAM       = 0x47670002   /* MDRawAssertionInfo */
} MDStreamType;  /* MINIDUMP_STREAM_TYPE */


typedef struct {
  u_int32_t length;     /* Length of buffer in bytes (not characters),
                         * excluding 0-terminator */
  u_int16_t buffer[1];  /* UTF-16-encoded, 0-terminated */
} MDString;  /* MINIDUMP_STRING */

static const size_t MDString_minsize = offsetof(MDString, buffer[0]);


typedef struct {
  u_int32_t            thread_id;
  u_int32_t            suspend_count;
  u_int32_t            priority_class;
  u_int32_t            priority;
  u_int64_t            teb;             /* Thread environment block */
  MDMemoryDescriptor   stack;
  MDLocationDescriptor thread_context;  /* MDRawContext[CPU] */
} MDRawThread;  /* MINIDUMP_THREAD */


typedef struct {
  u_int32_t   number_of_threads;
  MDRawThread threads[1];
} MDRawThreadList;  /* MINIDUMP_THREAD_LIST */

static const size_t MDRawThreadList_minsize = offsetof(MDRawThreadList,
                                                       threads[0]);


typedef struct {
  u_int64_t            base_of_image;
  u_int32_t            size_of_image;
  u_int32_t            checksum;         /* 0 if unknown */
  u_int32_t            time_date_stamp;  /* time_t */
  MDRVA                module_name_rva;  /* MDString, pathname or filename */
  MDVSFixedFileInfo    version_info;

  /* The next field stores a CodeView record and is populated when a module's
   * debug information resides in a PDB file.  It identifies the PDB file. */
  MDLocationDescriptor cv_record;

  /* The next field is populated when a module's debug information resides
   * in a DBG file.  It identifies the DBG file.  This field is effectively
   * obsolete with modules built by recent toolchains. */
  MDLocationDescriptor misc_record;

  /* Alignment problem: reserved0 and reserved1 are defined by the platform
   * SDK as 64-bit quantities.  However, that results in a structure whose
   * alignment is unpredictable on different CPUs and ABIs.  If the ABI
   * specifies full alignment of 64-bit quantities in structures (as ppc
   * does), there will be padding between miscRecord and reserved0.  If
   * 64-bit quantities can be aligned on 32-bit boundaries (as on x86),
   * this padding will not exist.  (Note that the structure up to this point
   * contains 1 64-bit member followed by 21 32-bit members.)
   * As a workaround, reserved0 and reserved1 are instead defined here as
   * four 32-bit quantities.  This should be harmless, as there are
   * currently no known uses for these fields. */
  u_int32_t            reserved0[2];
  u_int32_t            reserved1[2];
} MDRawModule;  /* MINIDUMP_MODULE */

/* The inclusion of a 64-bit type in MINIDUMP_MODULE forces the struct to
 * be tail-padded out to a multiple of 64 bits under some ABIs (such as PPC).
 * This doesn't occur on systems that don't tail-pad in this manner.  Define
 * this macro to be the usable size of the MDRawModule struct, and use it in
 * place of sizeof(MDRawModule). */
#define MD_MODULE_SIZE 108


/* (MDRawModule).cv_record can reference MDCVInfoPDB20 or MDCVInfoPDB70.
 * Ref.: http://www.debuginfo.com/articles/debuginfomatch.html
 * MDCVInfoPDB70 is the expected structure type with recent toolchains. */

typedef struct {
  u_int32_t signature;
  u_int32_t offset;     /* Offset to debug data (expect 0 in minidump) */
} MDCVHeader;

typedef struct {
  MDCVHeader cv_header;
  u_int32_t  signature;         /* time_t debug information created */
  u_int32_t  age;               /* revision of PDB file */
  u_int8_t   pdb_file_name[1];  /* Pathname or filename of PDB file */
} MDCVInfoPDB20;

static const size_t MDCVInfoPDB20_minsize = offsetof(MDCVInfoPDB20,
                                                     pdb_file_name[0]);

#define MD_CVINFOPDB20_SIGNATURE 0x3031424e  /* cvHeader.signature = '01BN' */

typedef struct {
  u_int32_t cv_signature;
  MDGUID    signature;         /* GUID, identifies PDB file */
  u_int32_t age;               /* Identifies incremental changes to PDB file */
  u_int8_t  pdb_file_name[1];  /* Pathname or filename of PDB file,
                                * 0-terminated 8-bit character data (UTF-8?) */
} MDCVInfoPDB70;

static const size_t MDCVInfoPDB70_minsize = offsetof(MDCVInfoPDB70,
                                                     pdb_file_name[0]);

#define MD_CVINFOPDB70_SIGNATURE 0x53445352  /* cvSignature = 'SDSR' */

typedef struct {
  u_int32_t data1[2];
  u_int32_t data2;
  u_int32_t data3;
  u_int32_t data4;
  u_int32_t data5[3];
  u_int8_t extra[2];
} MDCVInfoELF;

/* In addition to the two CodeView record formats above, used for linking
 * to external pdb files, it is possible for debugging data to be carried
 * directly in the CodeView record itself.  These signature values will
 * be found in the first 4 bytes of the CodeView record.  Additional values
 * not commonly experienced in the wild are given by "Microsoft Symbol and
 * Type Information", http://www.x86.org/ftp/manuals/tools/sym.pdf, section
 * 7.2.  An in-depth description of the CodeView 4.1 format is given by
 * "Undocumented Windows 2000 Secrets", Windows 2000 Debugging Support/
 * Microsoft Symbol File Internals/CodeView Subsections,
 * http://www.rawol.com/features/undocumented/sbs-w2k-1-windows-2000-debugging-support.pdf
 */
#define MD_CVINFOCV41_SIGNATURE 0x3930424e  /* '90BN', CodeView 4.10. */
#define MD_CVINFOCV50_SIGNATURE 0x3131424e  /* '11BN', CodeView 5.0,
                                             * MS C7-format (/Z7). */

#define MD_CVINFOUNKNOWN_SIGNATURE 0xffffffff  /* An unlikely value. */

/* (MDRawModule).miscRecord can reference MDImageDebugMisc.  The Windows
 * structure is actually defined in WinNT.h.  This structure is effectively
 * obsolete with modules built by recent toolchains. */

typedef struct {
  u_int32_t data_type;    /* IMAGE_DEBUG_TYPE_*, not defined here because
                           * this debug record type is mostly obsolete. */
  u_int32_t length;       /* Length of entire MDImageDebugMisc structure */
  u_int8_t  unicode;      /* True if data is multibyte */
  u_int8_t  reserved[3];
  u_int8_t  data[1];
} MDImageDebugMisc;  /* IMAGE_DEBUG_MISC */

static const size_t MDImageDebugMisc_minsize = offsetof(MDImageDebugMisc,
                                                        data[0]);


typedef struct {
  u_int32_t   number_of_modules;
  MDRawModule modules[1];
} MDRawModuleList;  /* MINIDUMP_MODULE_LIST */

static const size_t MDRawModuleList_minsize = offsetof(MDRawModuleList,
                                                       modules[0]);


typedef struct {
  u_int32_t          number_of_memory_ranges;
  MDMemoryDescriptor memory_ranges[1];
} MDRawMemoryList;  /* MINIDUMP_MEMORY_LIST */

static const size_t MDRawMemoryList_minsize = offsetof(MDRawMemoryList,
                                                       memory_ranges[0]);


#define MD_EXCEPTION_MAXIMUM_PARAMETERS 15

typedef struct {
  u_int32_t exception_code;     /* Windows: MDExceptionCodeWin,
                                 * Mac OS X: MDExceptionMac,
                                 * Linux: MDExceptionCodeLinux. */
  u_int32_t exception_flags;    /* Windows: 1 if noncontinuable,
                                   Mac OS X: MDExceptionCodeMac. */
  u_int64_t exception_record;   /* Address (in the minidump-producing host's
                                 * memory) of another MDException, for
                                 * nested exceptions. */
  u_int64_t exception_address;  /* The address that caused the exception.
                                 * Mac OS X: exception subcode (which is
                                 *           typically the address). */
  u_int32_t number_parameters;  /* Number of valid elements in
                                 * exception_information. */
  u_int32_t __align;
  u_int64_t exception_information[MD_EXCEPTION_MAXIMUM_PARAMETERS];
} MDException;  /* MINIDUMP_EXCEPTION */

#include "minidump_exception_win32.h"
#include "minidump_exception_mac.h"
#include "minidump_exception_linux.h"
#include "minidump_exception_solaris.h"

typedef struct {
  u_int32_t            thread_id;         /* Thread in which the exception
                                           * occurred.  Corresponds to
                                           * (MDRawThread).thread_id. */
  u_int32_t            __align;
  MDException          exception_record;
  MDLocationDescriptor thread_context;    /* MDRawContext[CPU] */
} MDRawExceptionStream;  /* MINIDUMP_EXCEPTION_STREAM */


typedef union {
  struct {
    u_int32_t vendor_id[3];               /* cpuid 0: ebx, edx, ecx */
    u_int32_t version_information;        /* cpuid 1: eax */
    u_int32_t feature_information;        /* cpuid 1: edx */
    u_int32_t amd_extended_cpu_features;  /* cpuid 0x80000001, ebx */
  } x86_cpu_info;
  struct {
    u_int64_t processor_features[2];
  } other_cpu_info;
} MDCPUInformation;  /* CPU_INFORMATION */


typedef struct {
  /* The next 3 fields and numberOfProcessors are from the SYSTEM_INFO
   * structure as returned by GetSystemInfo */
  u_int16_t        processor_architecture;
  u_int16_t        processor_level;         /* x86: 5 = 586, 6 = 686, ... */
  u_int16_t        processor_revision;      /* x86: 0xMMSS, where MM=model,
                                             *      SS=stepping */

  u_int8_t         number_of_processors;
  u_int8_t         product_type;            /* Windows: VER_NT_* from WinNT.h */

  /* The next 5 fields are from the OSVERSIONINFO structure as returned
   * by GetVersionEx */
  u_int32_t        major_version;
  u_int32_t        minor_version;
  u_int32_t        build_number;
  u_int32_t        platform_id;
  MDRVA            csd_version_rva;  /* MDString further identifying the
                                      * host OS.
                                      * Windows: name of the installed OS
                                      *          service pack.
                                      * Mac OS X: the Apple OS build number
                                      *           (sw_vers -buildVersion).
                                      * Linux: uname -srvmo */

  u_int16_t        suite_mask;       /* Windows: VER_SUITE_* from WinNT.h */
  u_int16_t        reserved2;

  MDCPUInformation cpu;
} MDRawSystemInfo;  /* MINIDUMP_SYSTEM_INFO */

/* For (MDRawSystemInfo).processor_architecture: */
typedef enum {
  MD_CPU_ARCHITECTURE_X86       =  0,  /* PROCESSOR_ARCHITECTURE_INTEL */
  MD_CPU_ARCHITECTURE_MIPS      =  1,  /* PROCESSOR_ARCHITECTURE_MIPS */
  MD_CPU_ARCHITECTURE_ALPHA     =  2,  /* PROCESSOR_ARCHITECTURE_ALPHA */
  MD_CPU_ARCHITECTURE_PPC       =  3,  /* PROCESSOR_ARCHITECTURE_PPC */
  MD_CPU_ARCHITECTURE_SHX       =  4,  /* PROCESSOR_ARCHITECTURE_SHX
                                        * (Super-H) */
  MD_CPU_ARCHITECTURE_ARM       =  5,  /* PROCESSOR_ARCHITECTURE_ARM */
  MD_CPU_ARCHITECTURE_IA64      =  6,  /* PROCESSOR_ARCHITECTURE_IA64 */
  MD_CPU_ARCHITECTURE_ALPHA64   =  7,  /* PROCESSOR_ARCHITECTURE_ALPHA64 */
  MD_CPU_ARCHITECTURE_MSIL      =  8,  /* PROCESSOR_ARCHITECTURE_MSIL
                                        * (Microsoft Intermediate Language) */
  MD_CPU_ARCHITECTURE_AMD64     =  9,  /* PROCESSOR_ARCHITECTURE_AMD64 */
  MD_CPU_ARCHITECTURE_X86_WIN64 = 10,
      /* PROCESSOR_ARCHITECTURE_IA32_ON_WIN64 (WoW64) */
  MD_CPU_ARCHITECTURE_SPARC     = 0x8001, /* Breakpad-defined value for SPARC */
  MD_CPU_ARCHITECTURE_UNKNOWN   = 0xffff  /* PROCESSOR_ARCHITECTURE_UNKNOWN */
} MDCPUArchitecture;

/* For (MDRawSystemInfo).platform_id: */
typedef enum {
  MD_OS_WIN32S        = 0,  /* VER_PLATFORM_WIN32s (Windows 3.1) */
  MD_OS_WIN32_WINDOWS = 1,  /* VER_PLATFORM_WIN32_WINDOWS (Windows 95-98-Me) */
  MD_OS_WIN32_NT      = 2,  /* VER_PLATFORM_WIN32_NT (Windows NT, 2000+) */
  MD_OS_WIN32_CE      = 3,  /* VER_PLATFORM_WIN32_CE, VER_PLATFORM_WIN32_HH
                             * (Windows CE, Windows Mobile, "Handheld") */

  /* The following values are Breakpad-defined. */
  MD_OS_UNIX          = 0x8000,  /* Generic Unix-ish */
  MD_OS_MAC_OS_X      = 0x8101,  /* Mac OS X/Darwin */
  MD_OS_IOS           = 0x8102,  /* iOS */
  MD_OS_LINUX         = 0x8201,  /* Linux */
  MD_OS_SOLARIS       = 0x8202   /* Solaris */
} MDOSPlatform;


typedef struct {
  u_int32_t size_of_info;  /* Length of entire MDRawMiscInfo structure. */
  u_int32_t flags1;

  /* The next field is only valid if flags1 contains
   * MD_MISCINFO_FLAGS1_PROCESS_ID. */
  u_int32_t process_id;

  /* The next 3 fields are only valid if flags1 contains
   * MD_MISCINFO_FLAGS1_PROCESS_TIMES. */
  u_int32_t process_create_time;  /* time_t process started */
  u_int32_t process_user_time;    /* seconds of user CPU time */
  u_int32_t process_kernel_time;  /* seconds of kernel CPU time */

  /* The following fields are not present in MINIDUMP_MISC_INFO but are
   * in MINIDUMP_MISC_INFO_2.  When this struct is populated, these values
   * may not be set.  Use flags1 or sizeOfInfo to determine whether these
   * values are present.  These are only valid when flags1 contains
   * MD_MISCINFO_FLAGS1_PROCESSOR_POWER_INFO. */
  u_int32_t processor_max_mhz;
  u_int32_t processor_current_mhz;
  u_int32_t processor_mhz_limit;
  u_int32_t processor_max_idle_state;
  u_int32_t processor_current_idle_state;
} MDRawMiscInfo;  /* MINIDUMP_MISC_INFO, MINIDUMP_MISC_INFO2 */

#define MD_MISCINFO_SIZE 24
#define MD_MISCINFO2_SIZE 44

/* For (MDRawMiscInfo).flags1.  These values indicate which fields in the
 * MDRawMiscInfoStructure are valid. */
typedef enum {
  MD_MISCINFO_FLAGS1_PROCESS_ID           = 0x00000001,
      /* MINIDUMP_MISC1_PROCESS_ID */
  MD_MISCINFO_FLAGS1_PROCESS_TIMES        = 0x00000002,
      /* MINIDUMP_MISC1_PROCESS_TIMES */
  MD_MISCINFO_FLAGS1_PROCESSOR_POWER_INFO = 0x00000004
      /* MINIDUMP_MISC1_PROCESSOR_POWER_INFO */
} MDMiscInfoFlags1;

/* 
 * Around DbgHelp version 6.0, the style of new LIST structures changed
 * from including an array of length 1 at the end of the struct to
 * represent the variable-length data to including explicit
 * "size of header", "size of entry" and "number of entries" fields
 * in the header, presumably to allow backwards-compatibly-extending
 * the structures in the future. The actual list entries follow the
 * header data directly in this case.
 */

typedef struct {
  u_int32_t size_of_header;    /* sizeof(MDRawMemoryInfoList) */
  u_int32_t size_of_entry;     /* sizeof(MDRawMemoryInfo) */
  u_int64_t number_of_entries;
} MDRawMemoryInfoList;  /* MINIDUMP_MEMORY_INFO_LIST */

typedef struct {
  u_int64_t base_address;           /* Base address of a region of pages */
  u_int64_t allocation_base;        /* Base address of a range of pages
                                     * within this region. */
  u_int32_t allocation_protection;  /* Memory protection when this region
                                     * was originally allocated:
                                     * MDMemoryProtection */
  u_int32_t __alignment1;
  u_int64_t region_size;
  u_int32_t state;                  /* MDMemoryState */
  u_int32_t protection;             /* MDMemoryProtection */
  u_int32_t type;                   /* MDMemoryType */
  u_int32_t __alignment2;
} MDRawMemoryInfo;  /* MINIDUMP_MEMORY_INFO */

/* For (MDRawMemoryInfo).state */
typedef enum {
  MD_MEMORY_STATE_COMMIT   = 0x1000,  /* physical storage has been allocated */
  MD_MEMORY_STATE_RESERVE  = 0x2000,  /* reserved, but no physical storage */
  MD_MEMORY_STATE_FREE     = 0x10000  /* available to be allocated */
} MDMemoryState;

/* For (MDRawMemoryInfo).allocation_protection and .protection */
typedef enum {
  MD_MEMORY_PROTECT_NOACCESS          = 0x01,  /* PAGE_NOACCESS */
  MD_MEMORY_PROTECT_READONLY          = 0x02,  /* PAGE_READONLY */
  MD_MEMORY_PROTECT_READWRITE         = 0x04,  /* PAGE_READWRITE */
  MD_MEMORY_PROTECT_WRITECOPY         = 0x08,  /* PAGE_WRITECOPY */
  MD_MEMORY_PROTECT_EXECUTE           = 0x10,  /* PAGE_EXECUTE */
  MD_MEMORY_PROTECT_EXECUTE_READ      = 0x20,  /* PAGE_EXECUTE_READ */
  MD_MEMORY_PROTECT_EXECUTE_READWRITE = 0x40,  /* PAGE_EXECUTE_READWRITE */
  MD_MEMORY_PROTECT_EXECUTE_WRITECOPY = 0x80,  /* PAGE_EXECUTE_WRITECOPY */
  /* These options can be combined with the previous flags. */
  MD_MEMORY_PROTECT_GUARD             = 0x100,  /* PAGE_GUARD */
  MD_MEMORY_PROTECT_NOCACHE           = 0x200,  /* PAGE_NOCACHE */
  MD_MEMORY_PROTECT_WRITECOMBINE      = 0x400,  /* PAGE_WRITECOMBINE */
} MDMemoryProtection;

/* Used to mask the mutually exclusive options from the combinable flags. */
const u_int32_t MD_MEMORY_PROTECTION_ACCESS_MASK = 0xFF;

/* For (MDRawMemoryInfo).type */
typedef enum {
  MD_MEMORY_TYPE_PRIVATE = 0x20000,   /* not shared by other processes */
  MD_MEMORY_TYPE_MAPPED  = 0x40000,   /* mapped into the view of a section */
  MD_MEMORY_TYPE_IMAGE   = 0x1000000  /* mapped into the view of an image */
} MDMemoryType;

/*
 * Breakpad extension types
 */


typedef struct {
  /* validity is a bitmask with values from MDBreakpadInfoValidity, indicating
   * which of the other fields in the structure are valid. */
  u_int32_t validity;

  /* Thread ID of the handler thread.  dump_thread_id should correspond to
   * the thread_id of an MDRawThread in the minidump's MDRawThreadList if
   * a dedicated thread in that list was used to produce the minidump.  If
   * the MDRawThreadList does not contain a dedicated thread used to produce
   * the minidump, this field should be set to 0 and the validity field
   * must not contain MD_BREAKPAD_INFO_VALID_DUMP_THREAD_ID. */
  u_int32_t dump_thread_id;

  /* Thread ID of the thread that requested the minidump be produced.  As
   * with dump_thread_id, requesting_thread_id should correspond to the
   * thread_id of an MDRawThread in the minidump's MDRawThreadList.  For
   * minidumps produced as a result of an exception, requesting_thread_id
   * will be the same as the MDRawExceptionStream's thread_id field.  For
   * minidumps produced "manually" at the program's request,
   * requesting_thread_id will indicate which thread caused the dump to be
   * written.  If the minidump was produced at the request of something
   * other than a thread in the MDRawThreadList, this field should be set
   * to 0 and the validity field must not contain
   * MD_BREAKPAD_INFO_VALID_REQUESTING_THREAD_ID. */
  u_int32_t requesting_thread_id;
} MDRawBreakpadInfo;

/* For (MDRawBreakpadInfo).validity: */
typedef enum {
  /* When set, the dump_thread_id field is valid. */
  MD_BREAKPAD_INFO_VALID_DUMP_THREAD_ID       = 1 << 0,

  /* When set, the requesting_thread_id field is valid. */
  MD_BREAKPAD_INFO_VALID_REQUESTING_THREAD_ID = 1 << 1
} MDBreakpadInfoValidity;

typedef struct {
  /* expression, function, and file are 0-terminated UTF-16 strings.  They
   * may be truncated if necessary, but should always be 0-terminated when
   * written to a file.
   * Fixed-length strings are used because MiniDumpWriteDump doesn't offer
   * a way for user streams to point to arbitrary RVAs for strings. */
  u_int16_t expression[128];  /* Assertion that failed... */
  u_int16_t function[128];    /* ...within this function... */
  u_int16_t file[128];        /* ...in this file... */
  u_int32_t line;             /* ...at this line. */
  u_int32_t type;
} MDRawAssertionInfo;

/* For (MDRawAssertionInfo).type: */
typedef enum {
  MD_ASSERTION_INFO_TYPE_UNKNOWN = 0,

  /* Used for assertions that would be raised by the MSVC CRT but are
   * directed to an invalid parameter handler instead. */
  MD_ASSERTION_INFO_TYPE_INVALID_PARAMETER,

  /* Used for assertions that would be raised by the MSVC CRT but are
   * directed to a pure virtual call handler instead. */
  MD_ASSERTION_INFO_TYPE_PURE_VIRTUAL_CALL
} MDAssertionInfoData;

#if defined(_MSC_VER)
#pragma warning(pop)
#endif  /* _MSC_VER */


#endif  /* GOOGLE_BREAKPAD_COMMON_MINIDUMP_FORMAT_H__ */
