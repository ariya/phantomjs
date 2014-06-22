/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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

#ifndef WebKitGraphics_h
#define WebKitGraphics_h

#include <windows.h>

extern "C" {

typedef struct CGColor* CGColorRef;
typedef struct CGContext* CGContextRef;

typedef wchar_t WCHAR;
typedef __nullterminated const WCHAR* LPCWSTR;
typedef LPCWSTR LPCTSTR;

struct WebFontDescription {
    LPCTSTR family;
    unsigned familyLength;
    float size;
    // FIXME: Change to weight.
    bool bold;
    bool italic;
};

struct WebTextRenderInfo
{
    DWORD structSize;
    CGContextRef cgContext;
    LPCTSTR text;
    int length;
    POINT pt;
    const WebFontDescription* description;
    CGColorRef color;
    int underlinedIndex;
    bool drawAsPassword;
    int overrideSmoothingLevel; // pass in -1 if caller does not want to override smoothing level
    SIZE shadowOffset;
    int shadowBlur;
    CGColorRef shadowColor;
};

void WebDrawText(WebTextRenderInfo*);
float TextFloatWidth(LPCTSTR text, int length, const WebFontDescription&);
void FontMetrics(const WebFontDescription&, int* ascent, int* descent, int* lineSpacing);

// buffer must be large enough to hold all of "text", including its null terminator. Returns the number of characters put in buffer (excluding the null terminator).
unsigned CenterTruncateStringToWidth(LPCTSTR text, int length, const WebFontDescription&, float width, WCHAR* buffer);
unsigned RightTruncateStringToWidth(LPCTSTR text, int length, const WebFontDescription&, float width, WCHAR* buffer);

void WebKitSetShouldUseFontSmoothing(bool);
bool WebKitShouldUseFontSmoothing();

}

#endif // !defined(WebKitGraphics_h)
