/*
 * Copyright (C) 2011 Brent Fulgham <bfulgham@webkit.org>
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

#ifndef DIBPixelData_h
#define DIBPixelData_h

#include "IntRect.h"
#include "IntSize.h"
#include <windows.h>

#if USE(CG)
#include <CoreFoundation/CFBase.h>
#else
typedef unsigned char UInt8;
#endif

namespace WebCore {

class DIBPixelData {
    public:
        DIBPixelData()
            : m_bitmapBuffer(0)
            , m_bitmapBufferLength(0)
            , m_bytesPerRow(0)
            , m_bitsPerPixel(0)
        {
        }
        DIBPixelData(HBITMAP);

        void initialize(HBITMAP);

#ifndef NDEBUG
        void writeToFile(LPCWSTR);
#endif

        UInt8* buffer() const { return m_bitmapBuffer; }
        unsigned bufferLength() const { return m_bitmapBufferLength; }
        const IntSize& size() const { return m_size; }
        unsigned bytesPerRow() const { return m_bytesPerRow; }
        unsigned short bitsPerPixel() const { return m_bitsPerPixel; }
        static void setRGBABitmapAlpha(HDC, const IntRect&, unsigned char);

    private:
        UInt8* m_bitmapBuffer;
        unsigned m_bitmapBufferLength;
        IntSize m_size;
        unsigned m_bytesPerRow;
        unsigned short m_bitsPerPixel;
};

} // namespace WebCore

#endif // DIBPixelData_h
