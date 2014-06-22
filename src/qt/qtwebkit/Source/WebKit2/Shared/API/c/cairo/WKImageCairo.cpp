/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Igalia S.L.
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

#include "config.h"
#include "WKImageCairo.h"

#include "ShareableBitmap.h"
#include "WKSharedAPICast.h"
#include "WebImage.h"
#include <WebCore/GraphicsContext.h>
#include <WebCore/PlatformContextCairo.h>

using namespace WebKit;
using namespace WebCore;

cairo_surface_t* WKImageCreateCairoSurface(WKImageRef imageRef)
{
    // We cannot pass a RefPtr through the API here, so we just leak the reference.
    return toImpl(imageRef)->bitmap()->createCairoSurface().leakRef();
}

WKImageRef WKImageCreateFromCairoSurface(cairo_surface_t* surface, WKImageOptions options)
{
    IntSize imageSize(cairo_image_surface_get_width(surface), cairo_image_surface_get_height(surface));
    RefPtr<WebImage> webImage = WebImage::create(imageSize, toImageOptions(options));
    OwnPtr<GraphicsContext> graphicsContext = webImage->bitmap()->createGraphicsContext();

    cairo_t* cr = graphicsContext->platformContext()->cr();
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_rectangle(cr, 0, 0, imageSize.width(), imageSize.height());
    cairo_fill(cr);

    return toAPI(webImage.release().leakRef());
}
