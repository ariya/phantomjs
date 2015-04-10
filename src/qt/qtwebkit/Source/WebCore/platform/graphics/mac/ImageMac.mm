/*
 * Copyright (C) 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
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

#import "config.h"
#import "BitmapImage.h"

#import "FloatRect.h"
#import "GraphicsContext.h"
#import "SharedBuffer.h"
#import <wtf/text/WTFString.h>

@interface WebCoreBundleFinder : NSObject
@end

@implementation WebCoreBundleFinder
@end

namespace WebCore {

void BitmapImage::invalidatePlatformData()
{
    if (m_frames.size() != 1)
        return;

    m_nsImage = 0;
    m_tiffRep = 0;
}

PassRefPtr<Image> Image::loadPlatformResource(const char *name)
{
    NSBundle *bundle = [NSBundle bundleForClass:[WebCoreBundleFinder class]];
    NSString *imagePath = [bundle pathForResource:[NSString stringWithUTF8String:name] ofType:@"png"];
    NSData *namedImageData = [NSData dataWithContentsOfFile:imagePath];
    if (namedImageData) {
        RefPtr<Image> image = BitmapImage::create();
        image->setData(SharedBuffer::wrapNSData(namedImageData), true);
        return image.release();
    }

    // We have reports indicating resource loads are failing, but we don't yet know the root cause(s).
    // Two theories are bad installs (image files are missing), and too-many-open-files.
    // See rdar://5607381
    ASSERT_NOT_REACHED();
    return Image::nullImage();
}

CFDataRef BitmapImage::getTIFFRepresentation()
{
    if (m_tiffRep)
        return m_tiffRep.get();
    
    unsigned numFrames = frameCount();
    
    // If numFrames is zero, we know for certain this image doesn't have valid data
    // Even though the call to CGImageDestinationCreateWithData will fail and we'll handle it gracefully,
    // in certain circumstances that call will spam the console with an error message
    if (!numFrames)
        return 0;

    Vector<CGImageRef> images;
    for (unsigned i = 0; i < numFrames; ++i ) {
        CGImageRef cgImage = frameAtIndex(i);
        if (cgImage)
            images.append(cgImage);
    }

    unsigned numValidFrames = images.size();
    
    RetainPtr<CFMutableDataRef> data = adoptCF(CFDataCreateMutable(0, 0));
    RetainPtr<CGImageDestinationRef> destination = adoptCF(CGImageDestinationCreateWithData(data.get(), kUTTypeTIFF, numValidFrames, 0));

    if (!destination)
        return 0;
    
    for (unsigned i = 0; i < numValidFrames; ++i)
        CGImageDestinationAddImage(destination.get(), images[i], 0);

    CGImageDestinationFinalize(destination.get());

    m_tiffRep = data;
    return m_tiffRep.get();
}

NSImage* BitmapImage::getNSImage()
{
    if (m_nsImage)
        return m_nsImage.get();

    CFDataRef data = getTIFFRepresentation();
    if (!data)
        return 0;
    
    m_nsImage = adoptNS([[NSImage alloc] initWithData:(NSData*)data]);
    return m_nsImage.get();
}

}
