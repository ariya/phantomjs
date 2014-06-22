/*
 * Copyright (C) 2006, 2008, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Nicholas Shanks <webkit@nickshanks.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FontCache.h"

#include "Font.h"
#include "FontGlyphs.h"
#include "FontPlatformData.h"
#include "FontSelector.h"
#include "WebKitFontFamilyNames.h"
#include <wtf/HashMap.h>
#include <wtf/ListHashSet.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/AtomicStringHash.h>
#include <wtf/text/StringHash.h>

#if ENABLE(OPENTYPE_VERTICAL)
#include "OpenTypeVerticalData.h"
#endif

using namespace WTF;

namespace WebCore {

FontCache* fontCache()
{
    DEFINE_STATIC_LOCAL(FontCache, globalFontCache, ());
    return &globalFontCache;
}

FontCache::FontCache()
    : m_purgePreventCount(0)
{
}

struct FontPlatformDataCacheKey {
    WTF_MAKE_FAST_ALLOCATED;
public:
    FontPlatformDataCacheKey() { }
    FontPlatformDataCacheKey(const AtomicString& family, const FontDescription& description)
        : m_fontDescriptionKey(description)
        , m_family(family)
    { }

    FontPlatformDataCacheKey(HashTableDeletedValueType) : m_fontDescriptionKey(hashTableDeletedSize()) { }
    bool isHashTableDeletedValue() const { return m_fontDescriptionKey.size == hashTableDeletedSize(); }

    bool operator==(const FontPlatformDataCacheKey& other) const
    {
        return equalIgnoringCase(m_family, other.m_family) && m_fontDescriptionKey == other.m_fontDescriptionKey;
    }

    FontDescriptionFontDataCacheKey m_fontDescriptionKey;
    AtomicString m_family;

private:
    static unsigned hashTableDeletedSize() { return 0xFFFFFFFFU; }
};

inline unsigned computeHash(const FontPlatformDataCacheKey& fontKey)
{
    return pairIntHash(CaseFoldingHash::hash(fontKey.m_family), fontKey.m_fontDescriptionKey.computeHash());
}

struct FontPlatformDataCacheKeyHash {
    static unsigned hash(const FontPlatformDataCacheKey& font)
    {
        return computeHash(font);
    }
         
    static bool equal(const FontPlatformDataCacheKey& a, const FontPlatformDataCacheKey& b)
    {
        return a == b;
    }

    static const bool safeToCompareToEmptyOrDeleted = true;
};

struct FontPlatformDataCacheKeyTraits : WTF::SimpleClassHashTraits<FontPlatformDataCacheKey> { };

typedef HashMap<FontPlatformDataCacheKey, OwnPtr<FontPlatformData>, FontPlatformDataCacheKeyHash, FontPlatformDataCacheKeyTraits> FontPlatformDataCache;

static FontPlatformDataCache* gFontPlatformDataCache = 0;

static bool familyNameEqualIgnoringCase(const AtomicString& familyName, const char* reference, unsigned length)
{
    ASSERT(length > 0);
    ASSERT(familyName.length() == length);
    ASSERT(strlen(reference) == length);
    const AtomicStringImpl* familyNameImpl = familyName.impl();
    if (familyNameImpl->is8Bit())
        return equalIgnoringCase(familyNameImpl->characters8(), reinterpret_cast<const LChar*>(reference), length);
    return equalIgnoringCase(familyNameImpl->characters16(), reinterpret_cast<const LChar*>(reference), length);
}

template<size_t length>
static inline bool familyNameEqualIgnoringCase(const AtomicString& familyName, const char (&reference)[length])
{
    return familyNameEqualIgnoringCase(familyName, reference, length - 1);
}

static const AtomicString alternateFamilyName(const AtomicString& familyName)
{
    // Alias Courier and Courier New.
    // Alias Times and Times New Roman.
    // Alias Arial and Helvetica.
    switch (familyName.length()) {
    case 5:
        if (familyNameEqualIgnoringCase(familyName, "Arial"))
            return AtomicString("Helvetica", AtomicString::ConstructFromLiteral);
        if (familyNameEqualIgnoringCase(familyName, "Times"))
            return AtomicString("Times New Roman", AtomicString::ConstructFromLiteral);
        break;
    case 7:
        if (familyNameEqualIgnoringCase(familyName, "Courier"))
            return AtomicString("Courier New", AtomicString::ConstructFromLiteral);
        break;
    case 9:
        if (familyNameEqualIgnoringCase(familyName, "Helvetica"))
            return AtomicString("Arial", AtomicString::ConstructFromLiteral);
        break;
#if !OS(WINDOWS)
    // On Windows, Courier New (truetype font) is always present and
    // Courier is a bitmap font. So, we don't want to map Courier New to
    // Courier.
    case 11:
        if (familyNameEqualIgnoringCase(familyName, "Courier New"))
            return AtomicString("Courier", AtomicString::ConstructFromLiteral);
        break;
#endif // !OS(WINDOWS)
    case 15:
        if (familyNameEqualIgnoringCase(familyName, "Times New Roman"))
            return AtomicString("Times", AtomicString::ConstructFromLiteral);
        break;
#if OS(WINDOWS)
    // On Windows, bitmap fonts are blocked altogether so that we have to 
    // alias MS Sans Serif (bitmap font) -> Microsoft Sans Serif (truetype font)
    case 13:
        if (familyNameEqualIgnoringCase(familyName, "MS Sans Serif"))
            return AtomicString("Microsoft Sans Serif", AtomicString::ConstructFromLiteral);
        break;

    // Alias MS Serif (bitmap) -> Times New Roman (truetype font). There's no 
    // 'Microsoft Sans Serif-equivalent' for Serif.
    case 8:
        if (familyNameEqualIgnoringCase(familyName, "MS Serif"))
            return AtomicString("Times New Roman", AtomicString::ConstructFromLiteral);
        break;
#endif // OS(WINDOWS)

    }

    return nullAtom;
}

FontPlatformData* FontCache::getCachedFontPlatformData(const FontDescription& fontDescription,
                                                       const AtomicString& passedFamilyName,
                                                       bool checkingAlternateName)
{
#if OS(WINDOWS) && ENABLE(OPENTYPE_VERTICAL)
    // Leading "@" in the font name enables Windows vertical flow flag for the font.
    // Because we do vertical flow by ourselves, we don't want to use the Windows feature.
    // IE disregards "@" regardless of the orientatoin, so we follow the behavior.
    const AtomicString& familyName = (passedFamilyName.isEmpty() || passedFamilyName[0] != '@') ?
        passedFamilyName : AtomicString(passedFamilyName.impl()->substring(1));
#else
    const AtomicString& familyName = passedFamilyName;
#endif

    if (!gFontPlatformDataCache) {
        gFontPlatformDataCache = new FontPlatformDataCache;
        platformInit();
    }

    FontPlatformDataCacheKey key(familyName, fontDescription);

    FontPlatformDataCache::AddResult result = gFontPlatformDataCache->add(key, nullptr);
    FontPlatformDataCache::iterator it = result.iterator;
    if (result.isNewEntry) {
        it->value = createFontPlatformData(fontDescription, familyName);

        if (!it->value && !checkingAlternateName) {
            // We were unable to find a font.  We have a small set of fonts that we alias to other names,
            // e.g., Arial/Helvetica, Courier/Courier New, etc.  Try looking up the font under the aliased name.
            const AtomicString alternateName = alternateFamilyName(familyName);
            if (!alternateName.isNull()) {
                FontPlatformData* fontPlatformDataForAlternateName = getCachedFontPlatformData(fontDescription, alternateName, true);
                // Lookup the key in the hash table again as the previous iterator may have
                // been invalidated by the recursive call to getCachedFontPlatformData().
                it = gFontPlatformDataCache->find(key);
                ASSERT(it != gFontPlatformDataCache->end());
                if (fontPlatformDataForAlternateName)
                    it->value = adoptPtr(new FontPlatformData(*fontPlatformDataForAlternateName));
            }
        }
    }

    return it->value.get();
}

#if ENABLE(OPENTYPE_VERTICAL)
typedef HashMap<FontCache::FontFileKey, RefPtr<OpenTypeVerticalData>, WTF::IntHash<FontCache::FontFileKey>, WTF::UnsignedWithZeroKeyHashTraits<FontCache::FontFileKey> > FontVerticalDataCache;

FontVerticalDataCache& fontVerticalDataCacheInstance()
{
    DEFINE_STATIC_LOCAL(FontVerticalDataCache, fontVerticalDataCache, ());
    return fontVerticalDataCache;
}

PassRefPtr<OpenTypeVerticalData> FontCache::getVerticalData(const FontFileKey& key, const FontPlatformData& platformData)
{
    FontVerticalDataCache& fontVerticalDataCache = fontVerticalDataCacheInstance();
    FontVerticalDataCache::iterator result = fontVerticalDataCache.find(key);
    if (result != fontVerticalDataCache.end())
        return result.get()->value;

    RefPtr<OpenTypeVerticalData> verticalData = OpenTypeVerticalData::create(platformData);
    if (!verticalData->isOpenType())
        verticalData.clear();
    fontVerticalDataCache.set(key, verticalData);
    return verticalData;
}
#endif

struct FontDataCacheKeyHash {
    static unsigned hash(const FontPlatformData& platformData)
    {
        return platformData.hash();
    }
         
    static bool equal(const FontPlatformData& a, const FontPlatformData& b)
    {
        return a == b;
    }

    static const bool safeToCompareToEmptyOrDeleted = true;
};

struct FontDataCacheKeyTraits : WTF::GenericHashTraits<FontPlatformData> {
    static const bool emptyValueIsZero = true;
    static const bool needsDestruction = true;
    static const FontPlatformData& emptyValue()
    {
        DEFINE_STATIC_LOCAL(FontPlatformData, key, (0.f, false, false));
        return key;
    }
    static void constructDeletedValue(FontPlatformData& slot)
    {
        new (NotNull, &slot) FontPlatformData(HashTableDeletedValue);
    }
    static bool isDeletedValue(const FontPlatformData& value)
    {
        return value.isHashTableDeletedValue();
    }
};

typedef HashMap<FontPlatformData, pair<RefPtr<SimpleFontData>, unsigned>, FontDataCacheKeyHash, FontDataCacheKeyTraits> FontDataCache;

static FontDataCache* gFontDataCache = 0;

const int cMaxInactiveFontData = 225;
const int cTargetInactiveFontData = 200;
static ListHashSet<RefPtr<SimpleFontData> >* gInactiveFontData = 0;

PassRefPtr<SimpleFontData> FontCache::getCachedFontData(const FontDescription& fontDescription, const AtomicString& family, bool checkingAlternateName, ShouldRetain shouldRetain)
{
    FontPlatformData* platformData = getCachedFontPlatformData(fontDescription, family, checkingAlternateName);
    if (!platformData)
        return 0;

    return getCachedFontData(platformData, shouldRetain);
}

PassRefPtr<SimpleFontData> FontCache::getCachedFontData(const FontPlatformData* platformData, ShouldRetain shouldRetain)
{
    if (!platformData)
        return 0;

#if !ASSERT_DISABLED
    if (shouldRetain == DoNotRetain)
        ASSERT(m_purgePreventCount);
#endif

    if (!gFontDataCache) {
        gFontDataCache = new FontDataCache;
        gInactiveFontData = new ListHashSet<RefPtr<SimpleFontData> >;
    }

    FontDataCache::iterator result = gFontDataCache->find(*platformData);
    if (result == gFontDataCache->end()) {
        pair<RefPtr<SimpleFontData>, unsigned> newValue(SimpleFontData::create(*platformData), shouldRetain == Retain ? 1 : 0);
        gFontDataCache->set(*platformData, newValue);
        if (shouldRetain == DoNotRetain)
            gInactiveFontData->add(newValue.first);
        return newValue.first.release();
    }

    if (!result.get()->value.second) {
        ASSERT(gInactiveFontData->contains(result.get()->value.first));
        gInactiveFontData->remove(result.get()->value.first);
    }

    if (shouldRetain == Retain)
        result.get()->value.second++;
    else if (!result.get()->value.second) {
        // If shouldRetain is DoNotRetain and count is 0, we want to remove the fontData from 
        // gInactiveFontData (above) and re-add here to update LRU position.
        gInactiveFontData->add(result.get()->value.first);
    }

    return result.get()->value.first;
}

SimpleFontData* FontCache::getNonRetainedLastResortFallbackFont(const FontDescription& fontDescription)
{
    return getLastResortFallbackFont(fontDescription, DoNotRetain).leakRef();
}

void FontCache::releaseFontData(const SimpleFontData* fontData)
{
    ASSERT(gFontDataCache);
    ASSERT(!fontData->isCustomFont());

    FontDataCache::iterator it = gFontDataCache->find(fontData->platformData());
    ASSERT(it != gFontDataCache->end());
    if (it == gFontDataCache->end())
        return;

    ASSERT(it->value.second);
    if (!--it->value.second)
        gInactiveFontData->add(it->value.first);
}

void FontCache::purgeInactiveFontDataIfNeeded()
{
    if (gInactiveFontData && !m_purgePreventCount && gInactiveFontData->size() > cMaxInactiveFontData)
        purgeInactiveFontData(gInactiveFontData->size() - cTargetInactiveFontData);
}

void FontCache::purgeInactiveFontData(int count)
{
    pruneUnreferencedEntriesFromFontGlyphsCache();

    if (!gInactiveFontData || m_purgePreventCount)
        return;

    static bool isPurging;  // Guard against reentry when e.g. a deleted FontData releases its small caps FontData.
    if (isPurging)
        return;

    isPurging = true;

    Vector<RefPtr<SimpleFontData>, 20> fontDataToDelete;
    ListHashSet<RefPtr<SimpleFontData> >::iterator end = gInactiveFontData->end();
    ListHashSet<RefPtr<SimpleFontData> >::iterator it = gInactiveFontData->begin();
    for (int i = 0; i < count && it != end; ++it, ++i) {
        RefPtr<SimpleFontData>& fontData = *it.get();
        gFontDataCache->remove(fontData->platformData());
        // We should not delete SimpleFontData here because deletion can modify gInactiveFontData. See http://trac.webkit.org/changeset/44011
        fontDataToDelete.append(fontData);
    }

    if (it == end) {
        // Removed everything
        gInactiveFontData->clear();
    } else {
        for (int i = 0; i < count; ++i)
            gInactiveFontData->remove(gInactiveFontData->begin());
    }

    fontDataToDelete.clear();

    if (gFontPlatformDataCache) {
        Vector<FontPlatformDataCacheKey> keysToRemove;
        keysToRemove.reserveInitialCapacity(gFontPlatformDataCache->size());
        FontPlatformDataCache::iterator platformDataEnd = gFontPlatformDataCache->end();
        for (FontPlatformDataCache::iterator platformData = gFontPlatformDataCache->begin(); platformData != platformDataEnd; ++platformData) {
            if (platformData->value && !gFontDataCache->contains(*platformData->value))
                keysToRemove.append(platformData->key);
        }
        
        size_t keysToRemoveCount = keysToRemove.size();
        for (size_t i = 0; i < keysToRemoveCount; ++i)
            gFontPlatformDataCache->remove(keysToRemove[i]);
    }

#if ENABLE(OPENTYPE_VERTICAL)
    FontVerticalDataCache& fontVerticalDataCache = fontVerticalDataCacheInstance();
    if (!fontVerticalDataCache.isEmpty()) {
        // Mark & sweep unused verticalData
        FontVerticalDataCache::iterator verticalDataEnd = fontVerticalDataCache.end();
        for (FontVerticalDataCache::iterator verticalData = fontVerticalDataCache.begin(); verticalData != verticalDataEnd; ++verticalData) {
            if (verticalData->value)
                verticalData->value->m_inFontCache = false;
        }
        FontDataCache::iterator fontDataEnd = gFontDataCache->end();
        for (FontDataCache::iterator fontData = gFontDataCache->begin(); fontData != fontDataEnd; ++fontData) {
            OpenTypeVerticalData* verticalData = const_cast<OpenTypeVerticalData*>(fontData->value.first->verticalData());
            if (verticalData)
                verticalData->m_inFontCache = true;
        }
        Vector<FontFileKey> keysToRemove;
        keysToRemove.reserveInitialCapacity(fontVerticalDataCache.size());
        for (FontVerticalDataCache::iterator verticalData = fontVerticalDataCache.begin(); verticalData != verticalDataEnd; ++verticalData) {
            if (!verticalData->value || !verticalData->value->m_inFontCache)
                keysToRemove.append(verticalData->key);
        }
        for (size_t i = 0, count = keysToRemove.size(); i < count; ++i)
            fontVerticalDataCache.take(keysToRemove[i]);
    }
#endif

    isPurging = false;
}

size_t FontCache::fontDataCount()
{
    if (gFontDataCache)
        return gFontDataCache->size();
    return 0;
}

size_t FontCache::inactiveFontDataCount()
{
    if (gInactiveFontData)
        return gInactiveFontData->size();
    return 0;
}

PassRefPtr<FontData> FontCache::getFontData(const FontDescription& description, int& familyIndex, FontSelector* fontSelector)
{
    ASSERT(familyIndex != cAllFamiliesScanned);
    RefPtr<FontData> result;

    bool isFirst = !familyIndex;
    int familyCount = description.familyCount();
    for (;familyIndex < familyCount && !result; ++familyIndex) {
        const AtomicString& family = description.familyAt(familyIndex);
        if (family.isEmpty())
            continue;
        if (fontSelector)
            result = fontSelector->getFontData(description, family);
        if (!result)
            result = getCachedFontData(description, family);
    }
    if (familyIndex == familyCount)
        familyIndex = cAllFamiliesScanned;

#if PLATFORM(MAC)
    if (!result) {
        // We didn't find a font. Try to find a similar font using our own specific knowledge about our platform.
        // For example on OS X, we know to map any families containing the words Arabic, Pashto, or Urdu to the
        // Geeza Pro font.
        result = similarFontPlatformData(description);
    }
#endif

    if (!result && isFirst) {
        // If it's the primary font that we couldn't find, we try the following. In all other cases, we will
        // just use per-character system fallback.
        if (fontSelector) {
            // Try the user's preferred standard font.
            if (RefPtr<FontData> data = fontSelector->getFontData(description, standardFamily))
                return data.release();
        }
        // Still no result.  Hand back our last resort fallback font.
        result = getLastResortFallbackFont(description);
    }
    return result.release();
}

static HashSet<FontSelector*>* gClients;

void FontCache::addClient(FontSelector* client)
{
    if (!gClients)
        gClients = new HashSet<FontSelector*>;

    ASSERT(!gClients->contains(client));
    gClients->add(client);
}

void FontCache::removeClient(FontSelector* client)
{
    ASSERT(gClients);
    ASSERT(gClients->contains(client));

    gClients->remove(client);
}

static unsigned short gGeneration = 0;

unsigned short FontCache::generation()
{
    return gGeneration;
}

void FontCache::invalidate()
{
    if (!gClients) {
        ASSERT(!gFontPlatformDataCache);
        return;
    }

    if (gFontPlatformDataCache)
        gFontPlatformDataCache->clear();
    invalidateFontGlyphsCache();

    gGeneration++;

    Vector<RefPtr<FontSelector> > clients;
    size_t numClients = gClients->size();
    clients.reserveInitialCapacity(numClients);
    HashSet<FontSelector*>::iterator end = gClients->end();
    for (HashSet<FontSelector*>::iterator it = gClients->begin(); it != end; ++it)
        clients.append(*it);

    ASSERT(numClients == clients.size());
    for (size_t i = 0; i < numClients; ++i)
        clients[i]->fontCacheInvalidated();

    purgeInactiveFontData();
}

} // namespace WebCore
