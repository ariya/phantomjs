/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "VMInspector.h"

#if ENABLE(VMINSPECTOR)

#include <stdio.h>
#include <wtf/ASCIICType.h>
#include <wtf/text/WTFString.h>

namespace JSC {

const char* VMInspector::getTypeName(JSValue value)
{
    if (value.isInt32())
        return "<Int32>";
    if (value.isBoolean())
        return "<Boolean>";
    if (value.isNull())
        return "<Empty>";
    if (value.isUndefined())
        return "<Undefined>";
    if (value.isCell())
        return "<Cell>";
    if (value.isEmpty())
        return "<Empty>";
    return "";
}

void VMInspector::dumpFrame0(CallFrame* frame)
{
    dumpFrame(frame, 0, 0, 0, 0);
}

void VMInspector::dumpFrame(CallFrame* frame, const char* prefix,
                            const char* funcName, const char* file, int line)
{
    int frameCount = VMInspector::countFrames(frame);
    if (frameCount < 0)
        return;

    Instruction* vPC = 0;
    if (frame->codeBlock())
        vPC = frame->currentVPC();

    #define CAST reinterpret_cast

    if (prefix)
        printf("%s ", prefix);

    printf("frame [%d] %p { cb %p:%s, retPC %p:%s, scope %p:%s, callee %p:%s, callerFrame %p:%s, argc %d, vPC %p }",
        frameCount, frame,
        CAST<void*>(frame[JSStack::CodeBlock].payload()),
        getTypeName(frame[JSStack::CodeBlock].jsValue()),
        CAST<void*>(frame[JSStack::ReturnPC].payload()),
        getTypeName(frame[JSStack::ReturnPC].jsValue()),
        CAST<void*>(frame[JSStack::ScopeChain].payload()),
        getTypeName(frame[JSStack::ScopeChain].jsValue()),
        CAST<void*>(frame[JSStack::Callee].payload()),
        getTypeName(frame[JSStack::Callee].jsValue()),
        CAST<void*>(frame[JSStack::CallerFrame].callFrame()),
        getTypeName(frame[JSStack::CallerFrame].jsValue()),
        frame[JSStack::ArgumentCount].payload(),
        vPC);

    if (funcName || file || (line >= 0)) {
        printf(" @");
        if (funcName)
            printf(" %s", funcName);
        if (file)
            printf(" %s", file);
        if (line >= 0)
            printf(":%d", line);
    }
    printf("\n");
}

int VMInspector::countFrames(CallFrame* frame)
{
    int count = -1;
    while (frame && !frame->hasHostCallFrameFlag()) {
        count++;
        frame = frame->callerFrame();
    }
    return count;
}


//============================================================================
//  class FormatPrinter
//    - implements functionality to support fprintf.
//
//    The FormatPrinter classes do the real formatting and printing.
//    By default, the superclass FormatPrinter will print to stdout (printf).
//    Each of the subclass will implement the other ...printf() options.
//    The subclasses are:
//
//        FileFormatPrinter     - fprintf
//        StringFormatPrinter   - sprintf
//        StringNFormatPrinter  - snprintf

class FormatPrinter {
public:
    virtual ~FormatPrinter() { }

    void print(const char* format, va_list args);

protected:
    // Low level printers:
    bool printArg(const char* format, ...);
    virtual bool printArg(const char* format, va_list args);

    // JS type specific printers:
    void printWTFString(va_list args, bool verbose);
};


// The public print() function is the real workhorse behind the printf
// family of functions. print() deciphers the % formatting, translate them
// to primitive formats, and dispatches to underlying printArg() functions
// to do the printing.
// 
// The non-public internal printArg() function is virtual and is responsible
// for handling the variations between printf, fprintf, sprintf, and snprintf.

void FormatPrinter::print(const char* format, va_list args)
{
    const char* p = format;
    const char* errorStr;

    // buffer is only used for 2 purposes:
    // 1. To temporarily hold a copy of normal chars (not needing formatting)
    //    to be passed to printArg() and printed.
    //
    //    The incoming format string may contain a string of normal chars much
    //    longer than 128, but we handle this by breaking them out to 128 chars
    //    fragments and printing each fragment before re-using the buffer to
    //    load up the next fragment.
    //
    // 2. To hold a single "%..." format to be passed to printArg() to process
    //    a single va_arg.

    char buffer[129]; // 128 chars + null terminator.
    char* end = &buffer[sizeof(buffer) - 1];
    const char* startOfFormatSpecifier = 0;

    while (true) {
        char c = *p++;
        char* curr = buffer;

        // Print leading normal chars:
        while (c != '\0' && c != '%') {
            *curr++ = c;
            if (curr == end) {
                // Out of buffer space. Flush the fragment, and start over.
                *curr = '\0';
                bool success = printArg("%s", buffer);
                if (!success) {
                    errorStr = buffer;
                    goto handleError;
                }
                curr = buffer;
            }
            c = *p++;
        }
        // If we have stuff in the buffer, flush the fragment:
        if (curr != buffer) {
            ASSERT(curr < end + 1);
            *curr = '\0';
            bool success = printArg("%s", buffer);
            if (!success) {
                errorStr = buffer;
                goto handleError;
            }
        }

        // End if there are not more chars to print:
        if (c == '\0')
            break;

        // If we get here, we've must have seen a '%':
        startOfFormatSpecifier = p - 1;
        ASSERT(*startOfFormatSpecifier == '%');
        c = *p++;

        // Check for "%%" case:
        if (c == '%') {
            bool success = printArg("%c", '%');
            if (!success) {
                errorStr = p - 2;
                goto handleError;
            }
            continue;
        }

        // Check for JS (%J<x>) formatting extensions:
        if (c == 'J') {
            bool verbose = false;

            c = *p++;
            if (UNLIKELY(c == '\0')) {
                errorStr = p - 2; // Rewind to % in "%J\0"
                goto handleError;
            }

            if (c == '+') {
                verbose = true;
                c= *p++;
                if (UNLIKELY(c == '\0')) {
                    errorStr = p - 3; // Rewind to % in "%J+\0"
                    goto handleError;
                }
            }

            switch (c) {
            // %Js - WTF::String*
            case 's': {
                printWTFString(args, verbose);
                continue;
            }
            } // END switch.

        // Check for non-JS extensions:
        } else if (c == 'b') {
            int value = va_arg(args, int);
            printArg("%s", value ? "TRUE" : "FALSE");
            continue;
        }

        // If we didn't handle the format in one of the above cases,
        // rewind p and let the standard formatting check handle it
        // if possible:
        p = startOfFormatSpecifier;
        ASSERT(*p == '%');

        // Check for standard formatting:
        // A format specifier always starts with a % and ends with some
        // alphabet. We'll do the simple thing and scan until the next
        // alphabet, or the end of string.

        // In the following, we're going to use buffer as storage for a copy
        // of a single format specifier. Hence, conceptually, we can think of
        // 'buffer' as synonymous with 'argFormat' here:

#define ABORT_IF_FORMAT_TOO_LONG(curr) \
        do {                           \
            if (UNLIKELY(curr >= end)) \
                goto formatTooLong;    \
        } while (false)
        
        curr = buffer;
        *curr++ = *p++; // Output the first % in the format specifier.
        c = *p++; // Grab the next char in the format specifier.

        // Checks for leading modifiers e.g. "%-d":
        //     0, -, ' ', +, '\''
        if (c == '0' || c == '-' || c == ' ' || c == '+' || c == '\'' || c == '#') {
            ABORT_IF_FORMAT_TOO_LONG(curr);
            *curr++ = c;
            c = *p++;
        }

        // Checks for decimal digit field width modifiers e.g. "%2f":
        while (c >= '0' && c <= '9') {
            ABORT_IF_FORMAT_TOO_LONG(curr);
            *curr++ = c;
            c = *p++;
        }

        // Checks for '.' e.g. "%2.f":
        if (c == '.') {
            ABORT_IF_FORMAT_TOO_LONG(curr);
            *curr++ = c;
            c = *p++;

            // Checks for decimal digit precision modifiers  e.g. "%.2f":
            while (c >= '0' && c <= '9') {
                ABORT_IF_FORMAT_TOO_LONG(curr);
                *curr++ = c;
                c = *p++;
            }
        }

        // Checks for the modifier <m> where <m> can be:
        //     l, h, j, t, z
        // e.g. "%ld"
        if (c == 'l' || c == 'h' || c == 'j' || c == 't' || c == 'z' || c == 'L') {
            ABORT_IF_FORMAT_TOO_LONG(curr);
            *curr++ = c;
            char prevChar = c;
            c = *p++;

            // Checks for the modifier ll or hh in %<x><m>:
            if ((prevChar == 'l' || prevChar == 'h') && c == prevChar) {
                ABORT_IF_FORMAT_TOO_LONG(curr);
                *curr++ = c;
                c = *p++;
            }
        }

        // Checks for %<x> where <x> can be:
        //     d, i, n, o, u, x, X
        // But hey, we're just going to do the simple thing and allow any
        // alphabet. The user is expected to pass correct format specifiers.
        // We won't do any format checking here. We'll just pass it on, and the
        // underlying ...printf() implementation may do the needed checking
        // at its discretion.
        while (c != '\0' && !isASCIIAlpha(c)) {
            ABORT_IF_FORMAT_TOO_LONG(curr);
            *curr++ = c;
            c = *p++;
        }

        ABORT_IF_FORMAT_TOO_LONG(curr);
        *curr++ = c;
        if (c == '\0') {
            // Uh oh. Bad format. We should have gotten an alphabet instead.
            // Print the supposed format as a string instead:
            errorStr = buffer;
            goto handleError;
        }

        // Otherwise, we have the alpha that terminates the format.
        // Terminate the buffer (i.e. argFormat) string:
        ASSERT(isASCIIAlpha(c));
        ABORT_IF_FORMAT_TOO_LONG(curr);
        *curr = '\0';

        bool success = printArg(buffer, args);
        if (!success) {
            errorStr = buffer;
            goto handleError;
        }
    }
#undef ABORT_IF_FORMAT_TOO_LONG

    return;

formatTooLong:
    // Print the error string:
    ASSERT(!!startOfFormatSpecifier);
    p = startOfFormatSpecifier;
    ASSERT(p >= format);
    printArg("ERROR @ Format too long at \"%s\"\n", p);
    return;

handleError:
    // We've got an error. Can't do any more work. Print an error message if
    // possible and then just return.

    // The errorStr may be pointing into the middle of buffer, or the original
    // format string. Move the string to buffer for consistency, and also so
    // that we can strip it of newlines below.
    if (errorStr != buffer) {
        size_t length = strlen(errorStr);
        if (length > sizeof(buffer) - 1)
            length = sizeof(buffer) - 1;
        memmove(buffer, errorStr, length);
        buffer[length] = '\0'; // Terminate the moved error string.
    }
    // Strip the newlines:
    char* cp = buffer;
    while (*cp) {
        if (*cp == '\n' || *cp == '\r')
            *cp = ' ';
        cp++;
    }
    // Print the error string:
    printArg("ERROR @ \"%s\"\n", buffer);
}


bool FormatPrinter::printArg(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    bool success = printArg(format, args);
    va_end(args);
    return success;
}

bool FormatPrinter::printArg(const char* format, va_list args)
{
    int count = ::vprintf(format, args);
    return (count >= 0); // Fail if less than 0 chars printed.
}


// %Js - WTF::String*
// verbose mode prints: WTF::String "<your string>"
void FormatPrinter::printWTFString(va_list args, bool verbose)
{
    const String* str = va_arg(args, const String*);

    // Print verbose header if appropriate:
    if (verbose)
        printArg("WTF::String \"");

    // Print the string itself:
    if (!str->isEmpty()) {
        if (str->is8Bit()) {
            const LChar* chars = str->characters8();
            printArg("%s", reinterpret_cast<const char*>(chars));
        } else {
            const UChar* chars = str->characters16();
            printArg("%S", reinterpret_cast<const wchar_t*>(chars));
        }
    }

    // Print verbose footer if appropriate:
    if (verbose)
        printArg("\"");
}


//============================================================================
//  class FileFormatPrinter
//    - implements functionality to support fprintf.

class FileFormatPrinter: public FormatPrinter {
public:
    FileFormatPrinter(FILE*);
private:
    virtual bool printArg(const char* format, va_list args);

    FILE* m_file;
};

FileFormatPrinter::FileFormatPrinter(FILE* file)
    : m_file(file)
{ 
}

bool FileFormatPrinter::printArg(const char* format, va_list args)
{
    int count = ::vfprintf(m_file, format, args);
    return (count >= 0); // Fail if less than 0 chars printed.
}


//============================================================================
//  class StringFormatPrinter
//    - implements functionality to support sprintf.

class StringFormatPrinter: public FormatPrinter {
public:
    StringFormatPrinter(char* buffer);
private:
    virtual bool printArg(const char* format, va_list args);

    char* m_buffer;
};

StringFormatPrinter::StringFormatPrinter(char* buffer)
    : m_buffer(buffer)
{ 
}

bool StringFormatPrinter::printArg(const char* format, va_list args)
{
    int count = ::vsprintf(m_buffer, format, args);
    m_buffer += count;
    return (count >= 0); // Fail if less than 0 chars printed.
}


//============================================================================
//  class StringNFormatPrinter
//    - implements functionality to support snprintf.

class StringNFormatPrinter: public FormatPrinter {
public:
    StringNFormatPrinter(char* buffer, size_t);
private:
    virtual bool printArg(const char* format, va_list args);

    char* m_buffer;
    size_t m_size;
};


StringNFormatPrinter::StringNFormatPrinter(char* buffer, size_t size)
    : m_buffer(buffer)
    , m_size(size)
{
}

bool StringNFormatPrinter::printArg(const char* format, va_list args)
{
    if (m_size > 0) {
        int count = ::vsnprintf(m_buffer, m_size, format, args);

        // According to vsnprintf specs, ...
        bool success = (count >= 0);
        if (static_cast<size_t>(count) >= m_size) {
            // If count > size, then we didn't have enough buffer space.
            count = m_size;
        }

        // Adjust the buffer to what's left if appropriate:
        if (success) {
            m_buffer += count;
            m_size -= count;
        }
        return success;
    }
    // No more room to print. Declare it a fail:
    return false;
}


//============================================================================
//  VMInspector printf family of methods:

void VMInspector::fprintf(FILE* file, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    FileFormatPrinter(file).print(format, args);
    va_end(args);
}

void VMInspector::printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    FormatPrinter().print(format, args);
    va_end(args);
}

void VMInspector::sprintf(char* buffer, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    StringFormatPrinter(buffer).print(format, args);
    va_end(args);
}

void VMInspector::snprintf(char* buffer, size_t size, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    StringNFormatPrinter(buffer, size).print(format, args);
    va_end(args);
}

} // namespace JSC

#endif // ENABLE(VMINSPECTOR)
