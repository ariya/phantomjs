/*
 * Copyright (C) 2003, 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// The vprintf_stderr_common function triggers this error in the Mac build.
// Feel free to remove this pragma if this file builds on Mac.
// According to http://gcc.gnu.org/onlinedocs/gcc-4.2.1/gcc/Diagnostic-Pragmas.html#Diagnostic-Pragmas
// we need to place this directive before any data or functions are defined.
#pragma GCC diagnostic ignored "-Wmissing-format-attribute"

#include "config.h"
#include "Assertions.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#if PLATFORM(MAC)
#include <CoreFoundation/CFString.h>
#endif

#if COMPILER(MSVC) && !OS(WINCE) && !PLATFORM(BREWMP)
#ifndef WINVER
#define WINVER 0x0500
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#include <crtdbg.h>
#endif

#if OS(WINDOWS)
#include <windows.h>
#endif

#if PLATFORM(BREWMP)
#include <AEEdbg.h>
#include <wtf/Vector.h>
#endif

#if PLATFORM(MAC)
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#endif

extern "C" {

#if PLATFORM(BREWMP)

static void printLog(const Vector<char>& buffer)
{
    // Each call to DBGPRINTF generates at most 128 bytes of output on the Windows SDK.
    // On Qualcomm chipset targets, DBGPRINTF() comes out the diag port (though this may change).
    // The length of each output string is constrained even more than on the Windows SDK.
#if COMPILER(MSVC)
    const int printBufferSize = 128;
#else
    const int printBufferSize = 32;
#endif

    char printBuffer[printBufferSize + 1];
    printBuffer[printBufferSize] = 0; // to guarantee null termination

    const char* p = buffer.data();
    const char* end = buffer.data() + buffer.size();
    while (p < end) {
        strncpy(printBuffer, p, printBufferSize);
        dbg_Message(printBuffer, DBG_MSG_LEVEL_HIGH, __FILE__, __LINE__);
        p += printBufferSize;
    }
}

#endif

WTF_ATTRIBUTE_PRINTF(1, 0)
static void vprintf_stderr_common(const char* format, va_list args)
{
#if PLATFORM(MAC)
    if (strstr(format, "%@")) {
        CFStringRef cfFormat = CFStringCreateWithCString(NULL, format, kCFStringEncodingUTF8);
        CFStringRef str = CFStringCreateWithFormatAndArguments(NULL, NULL, cfFormat, args);

        int length = CFStringGetMaximumSizeForEncoding(CFStringGetLength(str), kCFStringEncodingUTF8);
        char* buffer = (char*)malloc(length + 1);

        CFStringGetCString(str, buffer, length, kCFStringEncodingUTF8);

        fputs(buffer, stderr);

        free(buffer);
        CFRelease(str);
        CFRelease(cfFormat);
        return;
    }
#elif PLATFORM(BREWMP)
    // When str is 0, the return value is the number of bytes needed
    // to accept the result including null termination.
    int size = vsnprintf(0, 0, format, args);
    if (size > 0) {
        Vector<char> buffer(size);
        vsnprintf(buffer.data(), size, format, args);
        printLog(buffer);
    }

#elif HAVE(ISDEBUGGERPRESENT)
    if (IsDebuggerPresent()) {
        size_t size = 1024;

        do {
            char* buffer = (char*)malloc(size);

            if (buffer == NULL)
                break;

            if (_vsnprintf(buffer, size, format, args) != -1) {
#if OS(WINCE)
                // WinCE only supports wide chars
                wchar_t* wideBuffer = (wchar_t*)malloc(size * sizeof(wchar_t));
                if (wideBuffer == NULL)
                    break;
                for (unsigned int i = 0; i < size; ++i) {
                    if (!(wideBuffer[i] = buffer[i]))
                        break;
                }
                OutputDebugStringW(wideBuffer);
                free(wideBuffer);
#else
                OutputDebugStringA(buffer);
#endif
                free(buffer);
                break;
            }

            free(buffer);
            size *= 2;
        } while (size > 1024);
    }
#endif
#if OS(SYMBIAN)
    vfprintf(stdout, format, args);
#else
    vfprintf(stderr, format, args);
#endif
}

WTF_ATTRIBUTE_PRINTF(1, 2)
static void printf_stderr_common(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf_stderr_common(format, args);
    va_end(args);
}

static void printCallSite(const char* file, int line, const char* function)
{
#if OS(WINDOWS) && !OS(WINCE) && defined(_DEBUG)
    _CrtDbgReport(_CRT_WARN, file, line, NULL, "%s\n", function);
#else
    // By using this format, which matches the format used by MSVC for compiler errors, developers
    // using Visual Studio can double-click the file/line number in the Output Window to have the
    // editor navigate to that line of code. It seems fine for other developers, too.
    printf_stderr_common("%s(%d) : %s\n", file, line, function);
#endif
}

void WTFReportAssertionFailure(const char* file, int line, const char* function, const char* assertion)
{
    if (assertion)
        printf_stderr_common("ASSERTION FAILED: %s\n", assertion);
    else
        printf_stderr_common("SHOULD NEVER BE REACHED\n");
    printCallSite(file, line, function);
}

void WTFReportAssertionFailureWithMessage(const char* file, int line, const char* function, const char* assertion, const char* format, ...)
{
    printf_stderr_common("ASSERTION FAILED: ");
    va_list args;
    va_start(args, format);
    vprintf_stderr_common(format, args);
    va_end(args);
    printf_stderr_common("\n%s\n", assertion);
    printCallSite(file, line, function);
}

void WTFReportArgumentAssertionFailure(const char* file, int line, const char* function, const char* argName, const char* assertion)
{
    printf_stderr_common("ARGUMENT BAD: %s, %s\n", argName, assertion);
    printCallSite(file, line, function);
}

void WTFReportBacktrace()
{
#if PLATFORM(MAC)
    static const int maxFrames = 32;
    void* samples[maxFrames];
    int frames = backtrace(samples, maxFrames);

    for (int i = 1; i < frames; ++i) {
        void* pointer = samples[i];

        // Try to get a symbol name from the dynamic linker.
        Dl_info info;
        if (dladdr(pointer, &info) && info.dli_sname) {
            const char* mangledName = info.dli_sname;

            // Assume c++ & try to demangle the name.
            char* demangledName = abi::__cxa_demangle(mangledName, 0, 0, 0);
            if (demangledName) {
                fprintf(stderr, "%-3d %s\n", i, demangledName);
                free(demangledName);
            } else
                fprintf(stderr, "%-3d %s\n", i, mangledName);
        } else
            fprintf(stderr, "%-3d %p\n", i, pointer);
    }
#endif
}

void WTFReportFatalError(const char* file, int line, const char* function, const char* format, ...)
{
    printf_stderr_common("FATAL ERROR: ");
    va_list args;
    va_start(args, format);
    vprintf_stderr_common(format, args);
    va_end(args);
    printf_stderr_common("\n");
    printCallSite(file, line, function);
}

void WTFReportError(const char* file, int line, const char* function, const char* format, ...)
{
    printf_stderr_common("ERROR: ");
    va_list args;
    va_start(args, format);
    vprintf_stderr_common(format, args);
    va_end(args);
    printf_stderr_common("\n");
    printCallSite(file, line, function);
}

void WTFLog(WTFLogChannel* channel, const char* format, ...)
{
    if (channel->state != WTFLogChannelOn)
        return;

    va_list args;
    va_start(args, format);
    vprintf_stderr_common(format, args);
    va_end(args);
    
    size_t formatLength = strlen(format);
    if (formatLength && format[formatLength - 1] != '\n')
        printf_stderr_common("\n");
}

void WTFLogVerbose(const char* file, int line, const char* function, WTFLogChannel* channel, const char* format, ...)
{
    if (channel->state != WTFLogChannelOn)
        return;

    va_list args;
    va_start(args, format);
    vprintf_stderr_common(format, args);
    va_end(args);

    size_t formatLength = strlen(format);
    if (formatLength && format[formatLength - 1] != '\n')
        printf_stderr_common("\n");

    printCallSite(file, line, function);
}

} // extern "C"
