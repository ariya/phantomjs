/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WKString_h
#define WKString_h

#include <WebKit2/WKBase.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(WIN32) && !defined(_WIN32) \
    && !((defined(__CC_ARM) || defined(__ARMCC__)) && !defined(__linux__)) /* RVCT */
    typedef unsigned short WKChar;
#else
    typedef wchar_t WKChar;
#endif

WK_EXPORT WKTypeID WKStringGetTypeID();

WK_EXPORT WKStringRef WKStringCreateWithUTF8CString(const char* string);

WK_EXPORT bool WKStringIsEmpty(WKStringRef string);

WK_EXPORT size_t WKStringGetLength(WKStringRef string);
WK_EXPORT size_t WKStringGetCharacters(WKStringRef string, WKChar* buffer, size_t bufferLength);

WK_EXPORT size_t WKStringGetMaximumUTF8CStringSize(WKStringRef string);
WK_EXPORT size_t WKStringGetUTF8CString(WKStringRef string, char* buffer, size_t bufferSize);

WK_EXPORT bool WKStringIsEqual(WKStringRef a, WKStringRef b);
WK_EXPORT bool WKStringIsEqualToUTF8CString(WKStringRef a, const char* b);
WK_EXPORT bool WKStringIsEqualToUTF8CStringIgnoringCase(WKStringRef a, const char* b);

#ifdef __cplusplus
}
#endif

#endif /* WKString_h */
