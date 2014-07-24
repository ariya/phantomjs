/*
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2007-2008 Torch Mobile, Inc.
 * Copyright (C) 2012 Company 100 Inc.
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

#ifndef NativeImagePtr_h
#define NativeImagePtr_h

#if USE(CG)
typedef struct CGImage* CGImageRef;
#elif PLATFORM(QT)
#include "NativeImageQt.h"
#include <qglobal.h>
QT_BEGIN_NAMESPACE
class QPixmap;
QT_END_NAMESPACE
#elif USE(CAIRO)
#include "RefPtrCairo.h"
#elif USE(WINGDI)
#include "SharedBitmap.h"
#elif PLATFORM(BLACKBERRY)
namespace BlackBerry {
namespace Platform {
namespace Graphics {
class TiledImage;
}
}
}
#endif

namespace WebCore {

// FIXME: NativeImagePtr and PassNativeImagePtr should be smart
// pointers (see SVGImage::nativeImageForCurrentFrame()).
#if USE(CG)
typedef CGImageRef NativeImagePtr;
#elif PLATFORM(QT)
typedef QPixmap* NativeImagePtr;
#elif USE(CAIRO)
typedef RefPtr<cairo_surface_t> NativeImagePtr;
typedef PassRefPtr<cairo_surface_t> PassNativeImagePtr;
#elif USE(WINGDI)
typedef RefPtr<SharedBitmap> NativeImagePtr;
#elif PLATFORM(BLACKBERRY)
typedef BlackBerry::Platform::Graphics::TiledImage* NativeImagePtr;
#endif

#if !USE(CAIRO)
typedef NativeImagePtr PassNativeImagePtr;
#endif

}

#endif
