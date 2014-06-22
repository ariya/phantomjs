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

#include "config.h"
#include "DragImage.h"

#if ENABLE(DRAG_SUPPORT)
#include "DragController.h"

#include "FontRenderingMode.h"

namespace WebCore {
    
DragImageRef fitDragImageToMaxSize(DragImageRef image, const IntSize& srcSize, const IntSize& size)
{
    float heightResizeRatio = 0.0f;
    float widthResizeRatio = 0.0f;
    float resizeRatio = -1.0f;
    IntSize originalSize = dragImageSize(image);
    
    if (srcSize.width() > size.width()) {
        widthResizeRatio = size.width() / (float)srcSize.width();
        resizeRatio = widthResizeRatio;
    }
    
    if (srcSize.height() > size.height()) {
        heightResizeRatio = size.height() / (float)srcSize.height();
        if ((resizeRatio < 0.0f) || (resizeRatio > heightResizeRatio))
            resizeRatio = heightResizeRatio;
    }
    
    if (srcSize == originalSize)
        return resizeRatio > 0.0f ? scaleDragImage(image, FloatSize(resizeRatio, resizeRatio)) : image;
    
    // The image was scaled in the webpage so at minimum we must account for that scaling
    float scalex = srcSize.width() / (float)originalSize.width();
    float scaley = srcSize.height() / (float)originalSize.height();
    if (resizeRatio > 0.0f) {
        scalex *= resizeRatio;
        scaley *= resizeRatio;
    }
    
    return scaleDragImage(image, FloatSize(scalex, scaley));
}

#if !PLATFORM(MAC) && (!PLATFORM(WIN) || OS(WINCE))
DragImageRef createDragImageForLink(KURL&, const String&, FontRenderingMode)
{
    return 0;
}
#endif

} // namespace WebCore

#endif // ENABLE(DRAG_SUPPORT)
