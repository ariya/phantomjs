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

#ifndef SubimageCacheWithTimer_h
#define SubimageCacheWithTimer_h

#include "FloatRect.h"
#include "Timer.h"

#include <CoreGraphics/CoreGraphics.h>
#include <wtf/HashCountedSet.h>
#include <wtf/HashSet.h>
#include <wtf/HashTraits.h>
#include <wtf/RetainPtr.h>

#define CACHE_SUBIMAGES 1

namespace WebCore {

#if CACHE_SUBIMAGES

class SubimageCacheWithTimer {
    WTF_MAKE_NONCOPYABLE(SubimageCacheWithTimer); WTF_MAKE_FAST_ALLOCATED;

public:
    struct SubimageCacheEntry {
        RetainPtr<CGImageRef> image;
        FloatRect rect;
        RetainPtr<CGImageRef> subimage;
    };

    struct SubimageCacheEntryTraits : WTF::GenericHashTraits<SubimageCacheEntry> {
        typedef HashTraits<RetainPtr<CGImageRef> > ImageTraits;

        static const bool emptyValueIsZero = true;

        static const bool hasIsEmptyValueFunction = true;
        static bool isEmptyValue(const SubimageCacheEntry& value) { return !value.image; }

        static void constructDeletedValue(SubimageCacheEntry& slot) { ImageTraits::constructDeletedValue(slot.image); }
        static bool isDeletedValue(const SubimageCacheEntry& value) { return ImageTraits::isDeletedValue(value.image); }
    };

    struct SubimageCacheHash {
        static unsigned hash(CGImageRef image, const FloatRect& rect)
        {
            return WTF::pairIntHash(PtrHash<CGImageRef>::hash(image),
                (static_cast<unsigned>(rect.x()) << 16) | static_cast<unsigned>(rect.y()));
        }
        static unsigned hash(const SubimageCacheEntry& key)
        {
            return hash(key.image.get(), key.rect);
        }
        static bool equal(const SubimageCacheEntry& a, const SubimageCacheEntry& b)
        {
            return a.image == b.image && a.rect == b.rect;
        }
        static const bool safeToCompareToEmptyOrDeleted = true;
    };

    typedef HashSet<SubimageCacheEntry, SubimageCacheHash, SubimageCacheEntryTraits> SubimageCache;

public:
    SubimageCacheWithTimer();
    RetainPtr<CGImageRef> getSubimage(CGImageRef, const FloatRect&);
    void clearImage(CGImageRef);

private:
    void invalidateCacheTimerFired(DeferrableOneShotTimer<SubimageCacheWithTimer>*);

    HashCountedSet<CGImageRef> m_images;
    SubimageCache m_cache;
    DeferrableOneShotTimer<SubimageCacheWithTimer> m_timer;
};

SubimageCacheWithTimer& subimageCache();

#endif // CACHE_SUBIMAGES

}

#endif // SubimageCacheWithTimer_h
