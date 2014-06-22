/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2008 Kenneth Rohde Christiansen.  All rights reserved.
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2010 Samsung Electronics
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

#include "config.h"
#include "Image.h"

#include "BitmapImage.h"
#include "CairoUtilitiesEfl.h"
#include "SharedBuffer.h"

#include <cairo.h>
#include <wtf/text/StringConcatenate.h>

namespace WebCore {

void BitmapImage::invalidatePlatformData()
{
}

static PassRefPtr<SharedBuffer> loadResourceSharedBufferFallback()
{
    return SharedBuffer::create(); // TODO: fallback image?
}

static PassRefPtr<SharedBuffer> loadResourceSharedBuffer(const char* name)
{
    RefPtr<SharedBuffer> buffer = SharedBuffer::createWithContentsOfFile(makeString(DATA_DIR "/webkit-1.0/images/", name, ".png"));    
    if (buffer)
        return buffer.release();
    return loadResourceSharedBufferFallback();
}

PassRefPtr<Image> Image::loadPlatformResource(const char* name)
{
    RefPtr<BitmapImage> img = BitmapImage::create();
    RefPtr<SharedBuffer> buffer = loadResourceSharedBuffer(name);
    img->setData(buffer.release(), true);
    return img.release();
}

Evas_Object* BitmapImage::getEvasObject(Evas* evas)
{
    RefPtr<cairo_surface_t> surface = nativeImageForCurrentFrame();
    return surface ? evasObjectFromCairoImageSurface(evas, surface.get()).leakRef() : 0;
}

}
