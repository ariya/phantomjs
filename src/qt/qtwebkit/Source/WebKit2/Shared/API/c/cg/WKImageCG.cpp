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

#include "config.h"
#include "WKImageCG.h"

#include "ShareableBitmap.h"
#include "WKSharedAPICast.h"
#include "WebImage.h"
#include <WebCore/ColorSpace.h>
#include <WebCore/GraphicsContext.h>

using namespace WebKit;
using namespace WebCore;

CGImageRef WKImageCreateCGImage(WKImageRef imageRef)
{
    WebImage* webImage = toImpl(imageRef);
    if (!webImage->bitmap())
        return 0;

    return webImage->bitmap()->makeCGImageCopy().leakRef();
}

WKImageRef WKImageCreateFromCGImage(CGImageRef imageRef, WKImageOptions options)
{
    if (!imageRef)
        return 0;
    
    IntSize imageSize(CGImageGetWidth(imageRef), CGImageGetHeight(imageRef));
    RefPtr<WebImage> webImage = WebImage::create(imageSize, toImageOptions(options));
    if (!webImage->bitmap())
        return 0;

    OwnPtr<GraphicsContext> graphicsContext = webImage->bitmap()->createGraphicsContext();
    FloatRect rect(FloatPoint(0, 0), imageSize);
    graphicsContext->clearRect(rect);
    graphicsContext->drawNativeImage(imageRef, imageSize, WebCore::ColorSpaceDeviceRGB, rect, rect);
    return toAPI(webImage.release().leakRef());
}
