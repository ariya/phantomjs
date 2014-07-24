/*
 * Copyright (C) 2012, 2013 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SubimageCacheWithTimer.h"

#include <wtf/Vector.h>

#if CACHE_SUBIMAGES

namespace WebCore {

static const double subimageCacheClearDelay = 1;
static const int maxSubimageCacheSize = 300;

struct SubimageRequest {
    CGImageRef image;
    const FloatRect& rect;
    SubimageRequest(CGImageRef image, const FloatRect& rect) : image(image), rect(rect) { }
};

struct SubimageCacheAdder {
    static unsigned hash(const SubimageRequest& value)
    {
        return SubimageCacheWithTimer::SubimageCacheHash::hash(value.image, value.rect);
    }

    static bool equal(const SubimageCacheWithTimer::SubimageCacheEntry& a, const SubimageRequest& b)
    {
        return a.image == b.image && a.rect == b.rect;
    }

    static void translate(SubimageCacheWithTimer::SubimageCacheEntry& entry, const SubimageRequest& request, unsigned /*hashCode*/)
    {
        entry.image = request.image;
        entry.rect = request.rect;
        entry.subimage = adoptCF(CGImageCreateWithImageInRect(request.image, request.rect));
    }
};

SubimageCacheWithTimer::SubimageCacheWithTimer()
    : m_timer(this, &SubimageCacheWithTimer::invalidateCacheTimerFired, subimageCacheClearDelay)
{
}

void SubimageCacheWithTimer::invalidateCacheTimerFired(DeferrableOneShotTimer<SubimageCacheWithTimer>*)
{
    m_images.clear();
    m_cache.clear();
}

RetainPtr<CGImageRef> SubimageCacheWithTimer::getSubimage(CGImageRef image, const FloatRect& rect)
{
    m_timer.restart();
    if (m_cache.size() == maxSubimageCacheSize) {
        SubimageCacheEntry entry = *m_cache.begin();
        m_images.remove(entry.image.get());
        m_cache.remove(entry);
    }

    ASSERT(m_cache.size() < maxSubimageCacheSize);
    SubimageCache::AddResult result = m_cache.add<SubimageCacheAdder>(SubimageRequest(image, rect));
    if (result.isNewEntry)
        m_images.add(image);

    return result.iterator->subimage;
}

void SubimageCacheWithTimer::clearImage(CGImageRef image)
{
    if (m_images.contains(image)) {
        Vector<SubimageCacheEntry> toBeRemoved;
        SubimageCache::const_iterator end = m_cache.end();
        for (SubimageCache::const_iterator it = m_cache.begin(); it != end; ++it) {
            if (it->image.get() == image)
                toBeRemoved.append(*it);
        }

        for (Vector<SubimageCacheEntry>::iterator removeIt = toBeRemoved.begin(); removeIt != toBeRemoved.end(); ++removeIt)
            m_cache.remove(*removeIt);

        m_images.removeAll(image);
    }
}

SubimageCacheWithTimer& subimageCache()
{
    static SubimageCacheWithTimer& cache = *new SubimageCacheWithTimer;
    return cache;
}

}

#endif
