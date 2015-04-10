/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Google Inc. All rights reserved.
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

#include "Image.h"
#include <wtf/CheckedArithmetic.h>
#include <wtf/RefPtr.h>
#include <wtf/RetainPtr.h>
#include <wtf/Uint8ClampedArray.h>

#if (PLATFORM(MAC) && USE(CA) && (PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070))
#define WTF_USE_IOSURFACE_CANVAS_BACKING_STORE 1
#endif

typedef struct __IOSurface *IOSurfaceRef;
typedef struct CGColorSpace *CGColorSpaceRef;
typedef struct CGDataProvider *CGDataProviderRef;
typedef uint32_t CGBitmapInfo;

namespace WebCore {

class IntSize;

class ImageBufferData {
public:
    ImageBufferData(const IntSize&);

    void* m_data;
    
    RetainPtr<CGDataProviderRef> m_dataProvider;
    CGBitmapInfo m_bitmapInfo;
    Checked<unsigned, RecordOverflow> m_bytesPerRow;
    CGColorSpaceRef m_colorSpace;
    RetainPtr<IOSurfaceRef> m_surface;
#if !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED == 1070
    mutable double m_lastFlushTime;
#endif

    PassRefPtr<Uint8ClampedArray> getData(const IntRect&, const IntSize&, bool accelerateRendering, bool unmultiplied, float resolutionScale) const;
    void putData(Uint8ClampedArray*& source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, const IntSize&, bool accelerateRendering, bool unmultiplied, float resolutionScale);
};

} // namespace WebCore
