/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "CGUtilities.h"

#include <wtf/RetainPtr.h>

namespace WebKit {

void paintImage(CGContextRef context, CGImageRef image, CGFloat scaleFactor, CGPoint destination, CGRect source)
{
    CGContextSaveGState(context);

    CGContextClipToRect(context, CGRectMake(destination.x, destination.y, source.size.width, source.size.height));
    CGContextScaleCTM(context, 1, -1);

    CGFloat imageHeight = CGImageGetHeight(image) / scaleFactor;
    CGFloat imageWidth = CGImageGetWidth(image) / scaleFactor;

    CGFloat destX = destination.x - source.origin.x;
    CGFloat destY = -imageHeight - destination.y + source.origin.y;

    CGContextDrawImage(context, CGRectMake(destX, destY, imageWidth, imageHeight), image);

    CGContextRestoreGState(context);
}

void paintBitmapContext(CGContextRef context, CGContextRef bitmapContext, CGFloat scaleFactor, CGPoint destination, CGRect source)
{
    RetainPtr<CGImageRef> image = adoptCF(CGBitmapContextCreateImage(bitmapContext));
    paintImage(context, image.get(), scaleFactor, destination, source);
}

} // namespace WebKit
