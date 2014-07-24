/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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
#include "CSSValuePool.h"

#include "CSSParser.h"
#include "CSSStyleSheet.h"
#include "CSSValueKeywords.h"
#include "CSSValueList.h"

namespace WebCore {

CSSValuePool& cssValuePool()
{
    DEFINE_STATIC_LOCAL(CSSValuePool, pool, ());
    return pool;
}

CSSValuePool::CSSValuePool()
    : m_inheritedValue(CSSInheritedValue::create())
    , m_implicitInitialValue(CSSInitialValue::createImplicit())
    , m_explicitInitialValue(CSSInitialValue::createExplicit())
    , m_colorTransparent(CSSPrimitiveValue::createColor(Color::transparent))
    , m_colorWhite(CSSPrimitiveValue::createColor(Color::white))
    , m_colorBlack(CSSPrimitiveValue::createColor(Color::black))
{
}

PassRefPtr<CSSPrimitiveValue> CSSValuePool::createIdentifierValue(CSSValueID ident)
{
    if (!ident)
        return CSSPrimitiveValue::createIdentifier(ident);

    RELEASE_ASSERT(ident > 0 && ident < numCSSValueKeywords);
    if (!m_identifierValueCache[ident])
        m_identifierValueCache[ident] = CSSPrimitiveValue::createIdentifier(ident);
    return m_identifierValueCache[ident];
}

PassRefPtr<CSSPrimitiveValue> CSSValuePool::createIdentifierValue(CSSPropertyID ident)
{
    return CSSPrimitiveValue::createIdentifier(ident);
}

PassRefPtr<CSSPrimitiveValue> CSSValuePool::createColorValue(unsigned rgbValue)
{
    // These are the empty and deleted values of the hash table.
    if (rgbValue == Color::transparent)
        return m_colorTransparent;
    if (rgbValue == Color::white)
        return m_colorWhite;
    // Just because it is common.
    if (rgbValue == Color::black)
        return m_colorBlack;

    // Just wipe out the cache and start rebuilding if it gets too big.
    const int maximumColorCacheSize = 512;
    if (m_colorValueCache.size() > maximumColorCacheSize)
        m_colorValueCache.clear();

    RefPtr<CSSPrimitiveValue> dummyValue;
    ColorValueCache::AddResult entry = m_colorValueCache.add(rgbValue, dummyValue);
    if (entry.isNewEntry)
        entry.iterator->value = CSSPrimitiveValue::createColor(rgbValue);
    return entry.iterator->value;
}

PassRefPtr<CSSPrimitiveValue> CSSValuePool::createValue(double value, CSSPrimitiveValue::UnitTypes type)
{
    if (value < 0 || value > maximumCacheableIntegerValue)
        return CSSPrimitiveValue::create(value, type);

    int intValue = static_cast<int>(value);
    if (value != intValue)
        return CSSPrimitiveValue::create(value, type);

    RefPtr<CSSPrimitiveValue>* cache;
    switch (type) {
    case CSSPrimitiveValue::CSS_PX:
        cache = m_pixelValueCache;
        break;
    case CSSPrimitiveValue::CSS_PERCENTAGE:
        cache = m_percentValueCache;
        break;
    case CSSPrimitiveValue::CSS_NUMBER:
        cache = m_numberValueCache;
        break;
    default:
        return CSSPrimitiveValue::create(value, type);
    }

    if (!cache[intValue])
        cache[intValue] = CSSPrimitiveValue::create(value, type);
    return cache[intValue];
}

PassRefPtr<CSSPrimitiveValue> CSSValuePool::createFontFamilyValue(const String& familyName)
{
    RefPtr<CSSPrimitiveValue>& value = m_fontFamilyValueCache.add(familyName, 0).iterator->value;
    if (!value)
        value = CSSPrimitiveValue::create(familyName, CSSPrimitiveValue::CSS_STRING);
    return value;
}

PassRefPtr<CSSValueList> CSSValuePool::createFontFaceValue(const AtomicString& string)
{
    // Just wipe out the cache and start rebuilding if it gets too big.
    const int maximumFontFaceCacheSize = 128;
    if (m_fontFaceValueCache.size() > maximumFontFaceCacheSize)
        m_fontFaceValueCache.clear();

    RefPtr<CSSValueList>& value = m_fontFaceValueCache.add(string, 0).iterator->value;
    if (!value)
        value = CSSParser::parseFontFaceValue(string);
    return value;
}

void CSSValuePool::drain()
{
    m_colorValueCache.clear();
    m_fontFaceValueCache.clear();
    m_fontFamilyValueCache.clear();

    for (int i = 0; i < numCSSValueKeywords; ++i)
        m_identifierValueCache[i] = 0;

    for (int i = 0; i < maximumCacheableIntegerValue; ++i) {
        m_pixelValueCache[i] = 0;
        m_percentValueCache[i] = 0;
        m_numberValueCache[i] = 0;
    }
}

}
