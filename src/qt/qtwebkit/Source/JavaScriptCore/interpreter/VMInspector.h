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

#ifndef VMInspector_h
#define VMInspector_h

#define ENABLE_VMINSPECTOR 0

#include "CallFrame.h"
#include "JSCJSValue.h"
#include <stdarg.h>
#include <stdio.h>
#include <wtf/text/WTFString.h>

namespace JSC {

#if ENABLE(VMINSPECTOR)

class VMInspector {
public:    
    static JS_EXPORT_PRIVATE const char* getTypeName(JSValue);
    static JS_EXPORT_PRIVATE void dumpFrame0(CallFrame*);
    static JS_EXPORT_PRIVATE void dumpFrame(CallFrame*, const char* prefix = 0, const char* funcName = 0, const char* file = 0, int line = -1);
    static JS_EXPORT_PRIVATE int countFrames(CallFrame*);

    // Special family of ...printf() functions that support, in addition to the
    // standard % formats (e.g. %d, %s, etc), the following extra JSC formatting
    // options, %J<x>, where <x> consists of:
    //
    //   +    - verbose mode modifier.
    //          Used in combination with other options. Must come after the %J.
    //   s    - WTF::String*
    //
    // Examples of usage:
    //
    //    WTF::String str("My WTF String");
    //
    //    // Printing the string. Will print:
    //    // The wtf string says: "My WTF String" and is NOT EMPTY.
    //
    //    VMInspector::printf("The wtf string says: \"%Js\" and is %s\n",
    //        &str, str.isEmpty()?"EMPTY":"NOT EMPTY");
    //
    //    // Printing the string with verbose mode. Will print:
    //    // <WTF::String "My WTF String">
    //
    //    VMInspector::printf("<%J+s>\n", &str);
    //
    // Also added some convenience non-JS formats:
    //
    //   %b    - boolean (va_args will look for an int).
    //           Prints TRUE if non-zero, else prints FALSE.
    //
    // Caution: the user is expected to pass the correctly matched arguments
    // to pair with the corresponding % fomatting.

    static JS_EXPORT_PRIVATE void fprintf(FILE*, const char* format, ...);
    static JS_EXPORT_PRIVATE void printf(const char* format, ...);
    static JS_EXPORT_PRIVATE void sprintf(char*, const char* format, ...);
    static JS_EXPORT_PRIVATE void snprintf(char*, size_t, const char* format, ...);
};

#endif // ENABLE(VMINSPECTOR)

} // namespace JSC

#endif // VMInspector.h
