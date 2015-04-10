/*
 * Copyright (C) 2008, 2011, 2012, 2013 Apple Inc.  All rights reserved.
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
#include "CSSImageGeneratorValue.h"

#include "CSSCanvasValue.h"
#include "CSSCrossfadeValue.h"
#include "CSSGradientValue.h"
#include "Image.h"
#include "RenderObject.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

static const double timeToKeepCachedGeneratedImagesInSeconds = 3;

CSSImageGeneratorValue::CSSImageGeneratorValue(ClassType classType)
    : CSSValue(classType)
{
}

CSSImageGeneratorValue::~CSSImageGeneratorValue()
{
}

void CSSImageGeneratorValue::addClient(RenderObject* renderer)
{
    ASSERT(renderer);
    ref();
    m_clients.add(renderer);
}

void CSSImageGeneratorValue::removeClient(RenderObject* renderer)
{
    ASSERT(renderer);
    m_clients.remove(renderer);
    deref();
}

GeneratorGeneratedImage* CSSImageGeneratorValue::cachedImageForSize(IntSize size)
{
    if (size.isEmpty())
        return 0;

    CachedGeneratedImage* cachedGeneratedImage = m_images.get(size);
    if (!cachedGeneratedImage)
        return 0;

    cachedGeneratedImage->puntEvictionTimer();
    return cachedGeneratedImage->image();
}

void CSSImageGeneratorValue::saveCachedImageForSize(IntSize size, PassRefPtr<GeneratorGeneratedImage> image)
{
    ASSERT(!m_images.contains(size));
    m_images.add(size, adoptPtr(new CachedGeneratedImage(*this, size, image)));
}

void CSSImageGeneratorValue::evictCachedGeneratedImage(IntSize size)
{
    ASSERT(m_images.contains(size));
    m_images.remove(size);
}

CSSImageGeneratorValue::CachedGeneratedImage::CachedGeneratedImage(CSSImageGeneratorValue& owner, IntSize size, PassRefPtr<GeneratorGeneratedImage> image)
    : m_owner(owner)
    , m_size(size)
    , m_image(image)
    , m_evictionTimer(this, &CSSImageGeneratorValue::CachedGeneratedImage::evictionTimerFired, timeToKeepCachedGeneratedImagesInSeconds)
{
    m_evictionTimer.restart();
}

void CSSImageGeneratorValue::CachedGeneratedImage::evictionTimerFired(DeferrableOneShotTimer<CachedGeneratedImage>*)
{
    // NOTE: This is essentially a "delete this", the object is no longer valid after this line.
    m_owner.evictCachedGeneratedImage(m_size);
}

PassRefPtr<Image> CSSImageGeneratorValue::image(RenderObject* renderer, const IntSize& size)
{
    switch (classType()) {
    case CanvasClass:
        return static_cast<CSSCanvasValue*>(this)->image(renderer, size);
    case CrossfadeClass:
        return static_cast<CSSCrossfadeValue*>(this)->image(renderer, size);
    case LinearGradientClass:
        return static_cast<CSSLinearGradientValue*>(this)->image(renderer, size);
    case RadialGradientClass:
        return static_cast<CSSRadialGradientValue*>(this)->image(renderer, size);
    default:
        ASSERT_NOT_REACHED();
    }
    return 0;
}

bool CSSImageGeneratorValue::isFixedSize() const
{
    switch (classType()) {
    case CanvasClass:
        return static_cast<const CSSCanvasValue*>(this)->isFixedSize();
    case CrossfadeClass:
        return static_cast<const CSSCrossfadeValue*>(this)->isFixedSize();
    case LinearGradientClass:
        return static_cast<const CSSLinearGradientValue*>(this)->isFixedSize();
    case RadialGradientClass:
        return static_cast<const CSSRadialGradientValue*>(this)->isFixedSize();
    default:
        ASSERT_NOT_REACHED();
    }
    return false;
}

IntSize CSSImageGeneratorValue::fixedSize(const RenderObject* renderer)
{
    switch (classType()) {
    case CanvasClass:
        return static_cast<CSSCanvasValue*>(this)->fixedSize(renderer);
    case CrossfadeClass:
        return static_cast<CSSCrossfadeValue*>(this)->fixedSize(renderer);
    case LinearGradientClass:
        return static_cast<CSSLinearGradientValue*>(this)->fixedSize(renderer);
    case RadialGradientClass:
        return static_cast<CSSRadialGradientValue*>(this)->fixedSize(renderer);
    default:
        ASSERT_NOT_REACHED();
    }
    return IntSize();
}

bool CSSImageGeneratorValue::isPending() const
{
    switch (classType()) {
    case CrossfadeClass:
        return static_cast<const CSSCrossfadeValue*>(this)->isPending();
    case CanvasClass:
        return static_cast<const CSSCanvasValue*>(this)->isPending();
    case LinearGradientClass:
        return static_cast<const CSSLinearGradientValue*>(this)->isPending();
    case RadialGradientClass:
        return static_cast<const CSSRadialGradientValue*>(this)->isPending();
    default:
        ASSERT_NOT_REACHED();
    }
    return false;
}

bool CSSImageGeneratorValue::knownToBeOpaque(const RenderObject* renderer) const
{
    switch (classType()) {
    case CrossfadeClass:
        return static_cast<const CSSCrossfadeValue*>(this)->knownToBeOpaque(renderer);
    case CanvasClass:
        return false;
    case LinearGradientClass:
        return static_cast<const CSSLinearGradientValue*>(this)->knownToBeOpaque(renderer);
    case RadialGradientClass:
        return static_cast<const CSSRadialGradientValue*>(this)->knownToBeOpaque(renderer);
    default:
        ASSERT_NOT_REACHED();
    }
    return false;
}

void CSSImageGeneratorValue::loadSubimages(CachedResourceLoader* cachedResourceLoader)
{
    switch (classType()) {
    case CrossfadeClass:
        static_cast<CSSCrossfadeValue*>(this)->loadSubimages(cachedResourceLoader);
        break;
    case CanvasClass:
        static_cast<CSSCanvasValue*>(this)->loadSubimages(cachedResourceLoader);
        break;
    case LinearGradientClass:
        static_cast<CSSLinearGradientValue*>(this)->loadSubimages(cachedResourceLoader);
        break;
    case RadialGradientClass:
        static_cast<CSSRadialGradientValue*>(this)->loadSubimages(cachedResourceLoader);
        break;
    default:
        ASSERT_NOT_REACHED();
    }
}

} // namespace WebCore
