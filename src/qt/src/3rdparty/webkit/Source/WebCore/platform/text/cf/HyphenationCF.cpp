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
#include "Hyphenation.h"

#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)

#include "AtomicStringKeyedMRUCache.h"
#include "TextBreakIteratorInternalICU.h"
#include <wtf/ListHashSet.h>
#include <wtf/RetainPtr.h>

namespace WebCore {

#if !PLATFORM(WIN) || (defined(MAC_OS_X_VERSION_10_7) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7)

template<>
RetainPtr<CFLocaleRef> AtomicStringKeyedMRUCache<RetainPtr<CFLocaleRef> >::createValueForNullKey()
{
    RetainPtr<CFLocaleRef> locale(AdoptCF, CFLocaleCopyCurrent());

    return CFStringIsHyphenationAvailableForLocale(locale.get()) ? locale : 0;
}

template<>
RetainPtr<CFLocaleRef> AtomicStringKeyedMRUCache<RetainPtr<CFLocaleRef> >::createValueForKey(const AtomicString& localeIdentifier)
{
    RetainPtr<CFStringRef> cfLocaleIdentifier(AdoptCF, localeIdentifier.createCFString());
    RetainPtr<CFLocaleRef> locale(AdoptCF, CFLocaleCreate(kCFAllocatorDefault, cfLocaleIdentifier.get()));

    return CFStringIsHyphenationAvailableForLocale(locale.get()) ? locale : 0;
}

static AtomicStringKeyedMRUCache<RetainPtr<CFLocaleRef> >& cfLocaleCache()
{
    DEFINE_STATIC_LOCAL(AtomicStringKeyedMRUCache<RetainPtr<CFLocaleRef> >, cache, ());
    return cache;
}

bool canHyphenate(const AtomicString& localeIdentifier)
{
    return cfLocaleCache().get(localeIdentifier);
}

size_t lastHyphenLocation(const UChar* characters, size_t length, size_t beforeIndex, const AtomicString& localeIdentifier)
{
    RetainPtr<CFStringRef> string(AdoptCF, CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault, reinterpret_cast<const UniChar*>(characters), length, kCFAllocatorNull));

    RetainPtr<CFLocaleRef> locale = cfLocaleCache().get(localeIdentifier);
    ASSERT(locale);

    CFIndex result = CFStringGetHyphenationLocationBeforeIndex(string.get(), beforeIndex, CFRangeMake(0, length), 0, locale.get(), 0);
    return result == kCFNotFound ? 0 : result;
}

#else

bool canHyphenate(const AtomicString&)
{
    return false;
}

size_t lastHyphenLocation(const UChar*, size_t, size_t, const AtomicString&)
{
    ASSERT_NOT_REACHED();
    return 0;
}

#endif // PLATFORM(WIN) && (!defined(MAC_OS_X_VERSION_10_7) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_7)

} // namespace WebCore

#endif // !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
