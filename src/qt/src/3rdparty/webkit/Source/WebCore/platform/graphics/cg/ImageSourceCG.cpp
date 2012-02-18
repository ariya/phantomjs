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

#include "config.h"
#include "ImageSource.h"

#if USE(CG)
#include "ImageSourceCG.h"

#include "IntPoint.h"
#include "IntSize.h"
#include "MIMETypeRegistry.h"
#include "SharedBuffer.h"
#include <ApplicationServices/ApplicationServices.h>
#include <wtf/UnusedParam.h>

using namespace std;

namespace WebCore {

const CFStringRef kCGImageSourceShouldPreferRGB32 = CFSTR("kCGImageSourceShouldPreferRGB32");

// kCGImagePropertyGIFUnclampedDelayTime is available in the ImageIO framework headers on some versions
// of SnowLeopard. It's not possible to detect whether the constant is available so we define our own here
// that won't conflict with ImageIO's version when it is available.
const CFStringRef WebCoreCGImagePropertyGIFUnclampedDelayTime = CFSTR("UnclampedDelayTime");

#if !PLATFORM(MAC)
size_t sharedBufferGetBytesAtPosition(void* info, void* buffer, off_t position, size_t count)
{
    SharedBuffer* sharedBuffer = static_cast<SharedBuffer*>(info);
    size_t sourceSize = sharedBuffer->size();
    if (position >= sourceSize)
        return 0;

    const char* source = sharedBuffer->data() + position;
    size_t amount = min<size_t>(count, sourceSize - position);
    memcpy(buffer, source, amount);
    return amount;
}

void sharedBufferRelease(void* info)
{
    SharedBuffer* sharedBuffer = static_cast<SharedBuffer*>(info);
    sharedBuffer->deref();
}
#endif

ImageSource::ImageSource(ImageSource::AlphaOption alphaOption, ImageSource::GammaAndColorProfileOption gammaAndColorProfileOption)
    : m_decoder(0)
    // FIXME: m_premultiplyAlpha is ignored in cg at the moment.
    , m_alphaOption(alphaOption)
    , m_gammaAndColorProfileOption(gammaAndColorProfileOption)
{
}

ImageSource::~ImageSource()
{
    clear(true);
}

void ImageSource::clear(bool destroyAllFrames, size_t, SharedBuffer* data, bool allDataReceived)
{
#if !defined(BUILDING_ON_LEOPARD)
    // Recent versions of ImageIO discard previously decoded image frames if the client
    // application no longer holds references to them, so there's no need to throw away
    // the decoder unless we're explicitly asked to destroy all of the frames.

    if (!destroyAllFrames)
        return;
#else
    // Older versions of ImageIO hold references to previously decoded image frames.
    // There is no API to selectively release some of the frames it is holding, and
    // if we don't release the frames we use too much memory on large images.
    // Destroying the decoder is the only way to release previous frames.

    UNUSED_PARAM(destroyAllFrames);
#endif

    if (m_decoder) {
        CFRelease(m_decoder);
        m_decoder = 0;
    }
    if (data)
        setData(data, allDataReceived);
}

static CFDictionaryRef imageSourceOptions()
{
    static CFDictionaryRef options;

    if (!options) {
        const unsigned numOptions = 2;
        const void* keys[numOptions] = { kCGImageSourceShouldCache, kCGImageSourceShouldPreferRGB32 };
        const void* values[numOptions] = { kCFBooleanTrue, kCFBooleanTrue };
        options = CFDictionaryCreate(NULL, keys, values, numOptions, 
            &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    }
    return options;
}

bool ImageSource::initialized() const
{
    return m_decoder;
}

void ImageSource::setData(SharedBuffer* data, bool allDataReceived)
{
#if PLATFORM(MAC)
    if (!m_decoder)
        m_decoder = CGImageSourceCreateIncremental(0);
    // On Mac the NSData inside the SharedBuffer can be secretly appended to without the SharedBuffer's knowledge.  We use SharedBuffer's ability
    // to wrap itself inside CFData to get around this, ensuring that ImageIO is really looking at the SharedBuffer.
    RetainPtr<CFDataRef> cfData(AdoptCF, data->createCFData());
    CGImageSourceUpdateData(m_decoder, cfData.get(), allDataReceived);
#else
    if (!m_decoder) {
        m_decoder = CGImageSourceCreateIncremental(0);
    } else if (allDataReceived) {
#if !PLATFORM(WIN)
        // 10.6 bug workaround: image sources with final=false fail to draw into PDF contexts, so re-create image source
        // when data is complete. <rdar://problem/7874035> (<http://openradar.appspot.com/7874035>)
        CFRelease(m_decoder);
        m_decoder = CGImageSourceCreateIncremental(0);
#endif
    }
    // Create a CGDataProvider to wrap the SharedBuffer.
    data->ref();
    // We use the GetBytesAtPosition callback rather than the GetBytePointer one because SharedBuffer
    // does not provide a way to lock down the byte pointer and guarantee that it won't move, which
    // is a requirement for using the GetBytePointer callback.
    CGDataProviderDirectCallbacks providerCallbacks = { 0, 0, 0, sharedBufferGetBytesAtPosition, sharedBufferRelease };
    RetainPtr<CGDataProviderRef> dataProvider(AdoptCF, CGDataProviderCreateDirect(data, data->size(), &providerCallbacks));
    CGImageSourceUpdateDataProvider(m_decoder, dataProvider.get(), allDataReceived);
#endif
}

String ImageSource::filenameExtension() const
{
    if (!m_decoder)
        return String();
    CFStringRef imageSourceType = CGImageSourceGetType(m_decoder);
    return WebCore::preferredExtensionForImageSourceType(imageSourceType);
}

bool ImageSource::isSizeAvailable()
{
    bool result = false;
    CGImageSourceStatus imageSourceStatus = CGImageSourceGetStatus(m_decoder);

    // Ragnaros yells: TOO SOON! You have awakened me TOO SOON, Executus!
    if (imageSourceStatus >= kCGImageStatusIncomplete) {
        RetainPtr<CFDictionaryRef> image0Properties(AdoptCF, CGImageSourceCopyPropertiesAtIndex(m_decoder, 0, imageSourceOptions()));
        if (image0Properties) {
            CFNumberRef widthNumber = (CFNumberRef)CFDictionaryGetValue(image0Properties.get(), kCGImagePropertyPixelWidth);
            CFNumberRef heightNumber = (CFNumberRef)CFDictionaryGetValue(image0Properties.get(), kCGImagePropertyPixelHeight);
            result = widthNumber && heightNumber;
        }
    }
    
    return result;
}

IntSize ImageSource::frameSizeAtIndex(size_t index) const
{
    IntSize result;
    RetainPtr<CFDictionaryRef> properties(AdoptCF, CGImageSourceCopyPropertiesAtIndex(m_decoder, index, imageSourceOptions()));
    if (properties) {
        int w = 0, h = 0;
        CFNumberRef num = (CFNumberRef)CFDictionaryGetValue(properties.get(), kCGImagePropertyPixelWidth);
        if (num)
            CFNumberGetValue(num, kCFNumberIntType, &w);
        num = (CFNumberRef)CFDictionaryGetValue(properties.get(), kCGImagePropertyPixelHeight);
        if (num)
            CFNumberGetValue(num, kCFNumberIntType, &h);
        result = IntSize(w, h);
    }
    return result;
}

IntSize ImageSource::size() const
{
    return frameSizeAtIndex(0);
}

bool ImageSource::getHotSpot(IntPoint& hotSpot) const
{
    RetainPtr<CFDictionaryRef> properties(AdoptCF, CGImageSourceCopyPropertiesAtIndex(m_decoder, 0, imageSourceOptions()));
    if (!properties)
        return false;

    int x = -1, y = -1;
    CFNumberRef num = (CFNumberRef)CFDictionaryGetValue(properties.get(), CFSTR("hotspotX"));
    if (!num || !CFNumberGetValue(num, kCFNumberIntType, &x))
        return false;

    num = (CFNumberRef)CFDictionaryGetValue(properties.get(), CFSTR("hotspotY"));
    if (!num || !CFNumberGetValue(num, kCFNumberIntType, &y))
        return false;

    if (x < 0 || y < 0)
        return false;

    hotSpot = IntPoint(x, y);
    return true;
}

size_t ImageSource::bytesDecodedToDetermineProperties() const
{
    // Measured by tracing malloc/calloc calls on Mac OS 10.6.6, x86_64.
    // A non-zero value ensures cached images with no decoded frames still enter
    // the live decoded resources list when the CGImageSource decodes image
    // properties, allowing the cache to prune the partially decoded image.
    // This value is likely to be inaccurate on other platforms, but the overall
    // behavior is unchanged.
    return 13088;
}
    
int ImageSource::repetitionCount()
{
    int result = cAnimationLoopOnce; // No property means loop once.
    if (!initialized())
        return result;

    RetainPtr<CFDictionaryRef> properties(AdoptCF, CGImageSourceCopyProperties(m_decoder, imageSourceOptions()));
    if (properties) {
        CFDictionaryRef gifProperties = (CFDictionaryRef)CFDictionaryGetValue(properties.get(), kCGImagePropertyGIFDictionary);
        if (gifProperties) {
            CFNumberRef num = (CFNumberRef)CFDictionaryGetValue(gifProperties, kCGImagePropertyGIFLoopCount);
            if (num) {
                // A property with value 0 means loop forever.
                CFNumberGetValue(num, kCFNumberIntType, &result);
                if (!result)
                    result = cAnimationLoopInfinite;
            }
        } else
            result = cAnimationNone; // Turns out we're not a GIF after all, so we don't animate.
    }
    
    return result;
}

size_t ImageSource::frameCount() const
{
    return m_decoder ? CGImageSourceGetCount(m_decoder) : 0;
}

CGImageRef ImageSource::createFrameAtIndex(size_t index)
{
    if (!initialized())
        return 0;

    RetainPtr<CGImageRef> image(AdoptCF, CGImageSourceCreateImageAtIndex(m_decoder, index, imageSourceOptions()));
    CFStringRef imageUTI = CGImageSourceGetType(m_decoder);
    static const CFStringRef xbmUTI = CFSTR("public.xbitmap-image");
    if (!imageUTI || !CFEqual(imageUTI, xbmUTI))
        return image.releaseRef();
    
    // If it is an xbm image, mask out all the white areas to render them transparent.
    const CGFloat maskingColors[6] = {255, 255,  255, 255, 255, 255};
    RetainPtr<CGImageRef> maskedImage(AdoptCF, CGImageCreateWithMaskingColors(image.get(), maskingColors));
    if (!maskedImage)
        return image.releaseRef();

    return maskedImage.releaseRef();
}

bool ImageSource::frameIsCompleteAtIndex(size_t index)
{
    ASSERT(frameCount());

    // CGImageSourceGetStatusAtIndex claims that all frames of a multi-frame image are incomplete
    // when we've not yet received the complete data for an image that is using an incremental data
    // source (<rdar://problem/7679174>). We work around this by special-casing all frames except the
    // last in an image and treating them as complete if they are present and reported as being
    // incomplete. We do this on the assumption that loading new data can only modify the existing last
    // frame or append new frames. The last frame is only treated as being complete if the image source
    // reports it as such. This ensures that it is truly the last frame of the image rather than just
    // the last that we currently have data for.

    CGImageSourceStatus frameStatus = CGImageSourceGetStatusAtIndex(m_decoder, index);
    if (index < frameCount() - 1)
        return frameStatus >= kCGImageStatusIncomplete;

    return frameStatus == kCGImageStatusComplete;
}

float ImageSource::frameDurationAtIndex(size_t index)
{
    if (!initialized())
        return 0;

    float duration = 0;
    RetainPtr<CFDictionaryRef> properties(AdoptCF, CGImageSourceCopyPropertiesAtIndex(m_decoder, index, imageSourceOptions()));
    if (properties) {
        CFDictionaryRef typeProperties = (CFDictionaryRef)CFDictionaryGetValue(properties.get(), kCGImagePropertyGIFDictionary);
        if (typeProperties) {
            if (CFNumberRef num = (CFNumberRef)CFDictionaryGetValue(typeProperties, WebCoreCGImagePropertyGIFUnclampedDelayTime)) {
                // Use the unclamped frame delay if it exists.
                CFNumberGetValue(num, kCFNumberFloatType, &duration);
            } else if (CFNumberRef num = (CFNumberRef)CFDictionaryGetValue(typeProperties, kCGImagePropertyGIFDelayTime)) {
                // Fall back to the clamped frame delay if the unclamped frame delay does not exist.
                CFNumberGetValue(num, kCFNumberFloatType, &duration);
            }
        }
    }

    // Many annoying ads specify a 0 duration to make an image flash as quickly as possible.
    // We follow Firefox's behavior and use a duration of 100 ms for any frames that specify
    // a duration of <= 10 ms. See <rdar://problem/7689300> and <http://webkit.org/b/36082>
    // for more information.
    if (duration < 0.011f)
        return 0.100f;
    return duration;
}

bool ImageSource::frameHasAlphaAtIndex(size_t)
{
    if (!m_decoder)
        return false;

    CFStringRef imageType = CGImageSourceGetType(m_decoder);

    // Return false if there is no image type or the image type is JPEG, because
    // JPEG does not support alpha transparency.
    if (!imageType || CFEqual(imageType, CFSTR("public.jpeg")))
        return false;

    // FIXME: Could return false for other non-transparent image formats.
    // FIXME: Could maybe return false for a GIF Frame if we have enough info in the GIF properties dictionary
    // to determine whether or not a transparent color was defined.
    return true;
}

}

#endif // USE(CG)
