/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
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
#include "FontFallbackList.h"
#include "FontPlatformData.h"
#include "FontSelector.h"
#include <wtf/HashMap.h>
#include <wtf/ListHashSet.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringHash.h>

using namespace WTF;

namespace WebCore {

FontCache* fontCache()
{
    DEFINE_STATIC_LOCAL(FontCache, globalFontCache, ());
    return &globalFontCache;
}

FontCache::FontCache()
{
}

struct FontPlatformDataCacheKey {
    WTF_MAKE_FAST_ALLOCATED;
public:
    FontPlatformDataCacheKey(const AtomicString& family = AtomicString(), unsigned size = 0, unsigned weight = 0, bool italic = false,
                             bool isPrinterFont = false, FontRenderingMode renderingMode = NormalRenderingMode, FontOrientation orientation = Horizontal,
                             TextOrientation textOrientation = TextOrientationVerticalRight, FontWidthVariant widthVariant = RegularWidth)
        : m_size(size)
        , m_weight(weight)
        , m_family(family)
        , m_italic(italic)
        , m_printerFont(isPrinterFont)
        , m_renderingMode(renderingMode)
        , m_orientation(orientation)
        , m_textOrientation(textOrientation)
        , m_widthVariant(widthVariant)
    {
    }

    FontPlatformDataCacheKey(HashTableDeletedValueType) : m_size(hashTableDeletedSize()) { }
    bool isHashTableDeletedValue() const { return m_size == hashTableDeletedSize(); }

    bool operator==(const FontPlatformDataCacheKey& other) const
    {
        return equalIgnoringCase(m_family, other.m_family) && m_size == other.m_size && 
               m_weight == other.m_weight && m_italic == other.m_italic && m_printerFont == other.m_printerFont &&
               m_renderingMode == other.m_renderingMode && m_orientation == other.m_orientation && m_textOrientation == other.m_textOrientation && m_widthVariant == other.m_widthVariant;
    }

    unsigned m_size;
    unsigned m_weight;
    AtomicString m_family;
    bool m_italic;
    bool m_printerFont;
    FontRenderingMode m_renderingMode;
    FontOrientation m_orientation;
    TextOrientation m_textOrientation;
    FontWidthVariant m_widthVariant;

private:
    static unsigned hashTableDeletedSize() { return 0xFFFFFFFFU; }
};

inline unsigned computeHash(const FontPlatformDataCacheKey& fontKey)
{
    unsigned hashCodes[5] = {
        CaseFoldingHash::hash(fontKey.m_family),
        fontKey.m_size,
        fontKey.m_weight,
        fontKey.m_widthVariant,
        static_cast<unsigned>(fontKey.m_textOrientation) << 4 | static_cast<unsigned>(fontKey.m_orientation) << 3 | static_cast<unsigned>(fontKey.m_italic) << 2 | static_cast<unsigned>(fontKey.m_printerFont) << 1 | static_cast<unsigned>(fontKey.m_renderingMode)
    };
    return StringHasher::hashMemory<sizeof(hashCodes)>(hashCodes);
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

typedef HashMap<FontPlatformDataCacheKey, FontPlatformData*, FontPlatformDataCacheKeyHash, FontPlatformDataCacheKeyTraits> FontPlatformDataCache;

static FontPlatformDataCache* gFontPlatformDataCache = 0;

static const AtomicString& alternateFamilyName(const AtomicString& familyName)
{
    // Alias Courier <-> Courier New
    DEFINE_STATIC_LOCAL(AtomicString, courier, ("Courier"));
    DEFINE_STATIC_LOCAL(AtomicString, courierNew, ("Courier New"));
    if (equalIgnoringCase(familyName, courier))
        return courierNew;
#if !OS(WINDOWS)
    // On Windows, Courier New (truetype font) is always present and
    // Courier is a bitmap font. So, we don't want to map Courier New to
    // Courier.
    if (equalIgnoringCase(familyName, courierNew))
        return courier;
#endif

    // Alias Times and Times New Roman.
    DEFINE_STATIC_LOCAL(AtomicString, times, ("Times"));
    DEFINE_STATIC_LOCAL(AtomicString, timesNewRoman, ("Times New Roman"));
    if (equalIgnoringCase(familyName, times))
        return timesNewRoman;
    if (equalIgnoringCase(familyName, timesNewRoman))
        return times;
    
    // Alias Arial and Helvetica
    DEFINE_STATIC_LOCAL(AtomicString, arial, ("Arial"));
    DEFINE_STATIC_LOCAL(AtomicString, helvetica, ("Helvetica"));
    if (equalIgnoringCase(familyName, arial))
        return helvetica;
    if (equalIgnoringCase(familyName, helvetica))
        return arial;

#if OS(WINDOWS)
    // On Windows, bitmap fonts are blocked altogether so that we have to 
    // alias MS Sans Serif (bitmap font) -> Microsoft Sans Serif (truetype font)
    DEFINE_STATIC_LOCAL(AtomicString, msSans, ("MS Sans Serif"));
    DEFINE_STATIC_LOCAL(AtomicString, microsoftSans, ("Microsoft Sans Serif"));
    if (equalIgnoringCase(familyName, msSans))
        return microsoftSans;

    // Alias MS Serif (bitmap) -> Times New Roman (truetype font). There's no 
    // 'Microsoft Sans Serif-equivalent' for Serif. 
    static AtomicString msSerif("MS Serif");
    if (equalIgnoringCase(familyName, msSerif))
        return timesNewRoman;
#endif

    return emptyAtom;
}

FontPlatformData* FontCache::getCachedFontPlatformData(const FontDescription& fontDescription, 
                                                       const AtomicString& familyName,
                                                       bool checkingAlternateName)
{
    if (!gFontPlatformDataCache) {
        gFontPlatformDataCache = new FontPlatformDataCache;
        platformInit();
    }

    FontPlatformDataCacheKey key(familyName, fontDescription.computedPixelSize(), fontDescription.weight(), fontDescription.italic(),
                                 fontDescription.usePrinterFont(), fontDescription.renderingMode(), fontDescription.orientation(),
                                 fontDescription.textOrientation(), fontDescription.widthVariant());
    FontPlatformData* result = 0;
    bool foundResult;
    FontPlatformDataCache::iterator it = gFontPlatformDataCache->find(key);
    if (it == gFontPlatformDataCache->end()) {
        result = createFontPlatformData(fontDescription, familyName);
        gFontPlatformDataCache->set(key, result);
        foundResult = result;
    } else {
        result = it->second;
        foundResult = true;
    }

    if (!foundResult && !checkingAlternateName) {
        // We were unable to find a font.  We have a small set of fonts that we alias to other names, 
        // e.g., Arial/Helvetica, Courier/Courier New, etc.  Try looking up the font under the aliased name.
        const AtomicString& alternateName = alternateFamilyName(familyName);
        if (!alternateName.isEmpty())
            result = getCachedFontPlatformData(fontDescription, alternateName, true);
        if (result)
            gFontPlatformDataCache->set(key, new FontPlatformData(*result)); // Cache the result under the old name.
    }

    return result;
}

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
        new (&slot) FontPlatformData(HashTableDeletedValue);
    }
    static bool isDeletedValue(const FontPlatformData& value)
    {
        return value.isHashTableDeletedValue();
    }
};

typedef HashMap<FontPlatformData, pair<SimpleFontData*, unsigned>, FontDataCacheKeyHash, FontDataCacheKeyTraits> FontDataCache;

static FontDataCache* gFontDataCache = 0;

const int cMaxInactiveFontData = 120;  // Pretty Low Threshold
const int cTargetInactiveFontData = 100;
static ListHashSet<const SimpleFontData*>* gInactiveFontData = 0;

SimpleFontData* FontCache::getCachedFontData(const FontDescription& fontDescription, const AtomicString& family, bool checkingAlternateName)
{
    FontPlatformData* platformData = getCachedFontPlatformData(fontDescription, family, checkingAlternateName);
    if (!platformData)
        return 0;

    return getCachedFontData(platformData);
}

SimpleFontData* FontCache::getCachedFontData(const FontPlatformData* platformData)
{
    if (!platformData)
        return 0;

    if (!gFontDataCache) {
        gFontDataCache = new FontDataCache;
        gInactiveFontData = new ListHashSet<const SimpleFontData*>;
    }
    
    FontDataCache::iterator result = gFontDataCache->find(*platformData);
    if (result == gFontDataCache->end()) {
        pair<SimpleFontData*, unsigned> newValue(new SimpleFontData(*platformData), 1);
        gFontDataCache->set(*platformData, newValue);
        return newValue.first;
    }
    if (!result.get()->second.second++) {
        ASSERT(gInactiveFontData->contains(result.get()->second.first));
        gInactiveFontData->remove(result.get()->second.first);
    }

    return result.get()->second.first;
}

void FontCache::releaseFontData(const SimpleFontData* fontData)
{
    ASSERT(gFontDataCache);
    ASSERT(!fontData->isCustomFont());

    FontDataCache::iterator it = gFontDataCache->find(fontData->platformData());
    ASSERT(it != gFontDataCache->end());

    if (!--it->second.second) {
        gInactiveFontData->add(fontData);
        if (gInactiveFontData->size() > cMaxInactiveFontData)
            purgeInactiveFontData(gInactiveFontData->size() - cTargetInactiveFontData);
    }
}

void FontCache::purgeInactiveFontData(int count)
{
    if (!gInactiveFontData)
        return;

    static bool isPurging;  // Guard against reentry when e.g. a deleted FontData releases its small caps FontData.
    if (isPurging)
        return;

    isPurging = true;

    Vector<const SimpleFontData*, 20> fontDataToDelete;
    ListHashSet<const SimpleFontData*>::iterator end = gInactiveFontData->end();
    ListHashSet<const SimpleFontData*>::iterator it = gInactiveFontData->begin();
    for (int i = 0; i < count && it != end; ++it, ++i) {
        const SimpleFontData* fontData = *it.get();
        gFontDataCache->remove(fontData->platformData());
        fontDataToDelete.append(fontData);
    }

    if (it == end) {
        // Removed everything
        gInactiveFontData->clear();
    } else {
        for (int i = 0; i < count; ++i)
            gInactiveFontData->remove(gInactiveFontData->begin());
    }

    size_t fontDataToDeleteCount = fontDataToDelete.size();
    for (size_t i = 0; i < fontDataToDeleteCount; ++i)
        delete fontDataToDelete[i];

    if (gFontPlatformDataCache) {
        Vector<FontPlatformDataCacheKey> keysToRemove;
        keysToRemove.reserveInitialCapacity(gFontPlatformDataCache->size());
        FontPlatformDataCache::iterator platformDataEnd = gFontPlatformDataCache->end();
        for (FontPlatformDataCache::iterator platformData = gFontPlatformDataCache->begin(); platformData != platformDataEnd; ++platformData) {
            if (platformData->second && !gFontDataCache->contains(*platformData->second))
                keysToRemove.append(platformData->first);
        }
        
        size_t keysToRemoveCount = keysToRemove.size();
        for (size_t i = 0; i < keysToRemoveCount; ++i)
            delete gFontPlatformDataCache->take(keysToRemove[i]);
    }

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

const FontData* FontCache::getFontData(const Font& font, int& familyIndex, FontSelector* fontSelector)
{
    SimpleFontData* result = 0;

    int startIndex = familyIndex;
    const FontFamily* startFamily = &font.fontDescription().family();
    for (int i = 0; startFamily && i < startIndex; i++)
        startFamily = startFamily->next();
    const FontFamily* currFamily = startFamily;
    while (currFamily && !result) {
        familyIndex++;
        if (currFamily->family().length()) {
            if (fontSelector) {
                FontData* data = fontSelector->getFontData(font.fontDescription(), currFamily->family());
                if (data)
                    return data;
            }
            result = getCachedFontData(font.fontDescription(), currFamily->family());
        }
        currFamily = currFamily->next();
    }

    if (!currFamily)
        familyIndex = cAllFamiliesScanned;

    if (!result)
        // We didn't find a font. Try to find a similar font using our own specific knowledge about our platform.
        // For example on OS X, we know to map any families containing the words Arabic, Pashto, or Urdu to the
        // Geeza Pro font.
        result = getSimilarFontPlatformData(font);

    if (!result && startIndex == 0) {
        // If it's the primary font that we couldn't find, we try the following. In all other cases, we will
        // just use per-character system fallback.

        if (fontSelector) {
            // Try the user's preferred standard font.
            if (FontData* data = fontSelector->getFontData(font.fontDescription(), "-webkit-standard"))
                return data;
        }

        // Still no result.  Hand back our last resort fallback font.
        result = getLastResortFallbackFont(font.fontDescription());
    }
    return result;
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

static unsigned gGeneration = 0;

unsigned FontCache::generation()
{
    return gGeneration;
}

void FontCache::invalidate()
{
    if (!gClients) {
        ASSERT(!gFontPlatformDataCache);
        return;
    }

    if (gFontPlatformDataCache) {
        deleteAllValues(*gFontPlatformDataCache);
        delete gFontPlatformDataCache;
        gFontPlatformDataCache = new FontPlatformDataCache;
    }

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
