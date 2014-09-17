/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
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

#ifndef SystemInfo_h
#define SystemInfo_h

#include <wtf/text/WTFString.h>

namespace WebCore {

// NOTE: Keep these in order so callers can do things like
// "if (windowsVersion() >= WindowsVista) ...". It's OK to change or add values,
// though.
enum WindowsVersion {
    // CE-based versions
    WindowsCE1 = 0,
    WindowsCE2,
    WindowsCE3,
    WindowsCE4,
    WindowsCE5,
    WindowsCE6,
    WindowsCE7,
    // 3.x-based versions
    Windows3_1,
    // 9x-based versions
    Windows95,
    Windows98,
    WindowsME,
    // NT-based versions
    WindowsNT3,
    WindowsNT4,
    Windows2000,
    WindowsXP,
    WindowsServer2003,
    WindowsVista,
    WindowsServer2008,
    Windows7,
};

// If supplied, |major| and |minor| are set to the OSVERSIONINFO::dwMajorVersion
// and dwMinorVersion field values, respectively.
WindowsVersion windowsVersion(int* major = 0, int* minor = 0);

String windowsVersionForUAString();

} // namespace WebCore

#endif // SystemInfo_h
