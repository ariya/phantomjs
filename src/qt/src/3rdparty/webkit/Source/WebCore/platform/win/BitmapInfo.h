/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Brent Fulgham
 * Copyright (C) 2007-2009 Torch Mobile, Inc. All Rights Reserved.
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
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

#ifndef BitmapInfo_h
#define BitmapInfo_h

#include "IntSize.h"
#include <windows.h>

namespace WebCore {

struct BitmapInfo : public BITMAPINFO {
    enum BitCount {
        BitCount1 = 1,
        BitCount4 = 4,
        BitCount8 = 8,
        BitCount16 = 16,
        BitCount24 = 24,
        BitCount32 = 32
    };

    BitmapInfo();
    static BitmapInfo create(const IntSize&, BitCount bitCount = BitCount32);
    static BitmapInfo createBottomUp(const IntSize&, BitCount bitCount = BitCount32);

    bool is16bit() const { return bmiHeader.biBitCount == 16; }
    bool is32bit() const { return bmiHeader.biBitCount == 32; }
    unsigned width() const { return abs(bmiHeader.biWidth); }
    unsigned height() const { return abs(bmiHeader.biHeight); }
    IntSize size() const { return IntSize(width(), height()); }
    unsigned bytesPerLine() const { return (width() * bmiHeader.biBitCount + 7) / 8; }
    unsigned paddedBytesPerLine() const { return (bytesPerLine() + 3) & ~0x3; }
    unsigned paddedWidth() const { return paddedBytesPerLine() * 8 / bmiHeader.biBitCount; }
    unsigned numPixels() const { return paddedWidth() * height(); }
};

} // namespace WebCore

#endif // BitmapInfo_h
