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
#include "CSSPrimitiveValueCache.h"

namespace WebCore {

CSSPrimitiveValueCache::CSSPrimitiveValueCache()
    : m_colorTransparent(CSSPrimitiveValue::createColor(Color::transparent))
    , m_colorWhite(CSSPrimitiveValue::createColor(Color::white))
    , m_colorBlack(CSSPrimitiveValue::createColor(Color::black))
    , m_pixelZero(CSSPrimitiveValue::create(0, CSSPrimitiveValue::CSS_PX))
    , m_percentZero(CSSPrimitiveValue::create(0, CSSPrimitiveValue::CSS_PERCENTAGE))
    , m_numberZero(CSSPrimitiveValue::create(0, CSSPrimitiveValue::CSS_NUMBER))
{
}

CSSPrimitiveValueCache::~CSSPrimitiveValueCache()
{
}

PassRefPtr<CSSPrimitiveValue> CSSPrimitiveValueCache::createIdentifierValue(int ident)
{
    if (ident <= 0 || ident >= numCSSValueKeywords)
        return CSSPrimitiveValue::createIdentifier(ident);

    RefPtr<CSSPrimitiveValue> dummyValue;
    pair<IdentifierValueCache::iterator, bool> entry = m_identifierValueCache.add(ident, dummyValue);
    if (entry.second)
        entry.first->second = CSSPrimitiveValue::createIdentifier(ident);
    return entry.first->second;
}

PassRefPtr<CSSPrimitiveValue> CSSPrimitiveValueCache::createColorValue(unsigned rgbValue)
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
    pair<ColorValueCache::iterator, bool> entry = m_colorValueCache.add(rgbValue, dummyValue);
    if (entry.second)
        entry.first->second = CSSPrimitiveValue::createColor(rgbValue);
    return entry.first->second;
}

PassRefPtr<CSSPrimitiveValue> CSSPrimitiveValueCache::createValue(double value, CSSPrimitiveValue::UnitTypes type)
{
    // Small positive integers repeat often.
    static const int maximumCacheableValue = 256;
    if (value < 0 || value > maximumCacheableValue)
        return CSSPrimitiveValue::create(value, type);

    int intValue = static_cast<int>(value);
    if (value != intValue)
        return CSSPrimitiveValue::create(value, type);
    
    IntegerValueCache* cache;
    switch (type) {
    case CSSPrimitiveValue::CSS_PX:
        if (intValue == 0)
            return m_pixelZero;
        cache = &m_pixelValueCache;
        break;
    case CSSPrimitiveValue::CSS_PERCENTAGE:
        if (intValue == 0)
            return m_percentZero;
        cache = &m_percentValueCache;
        break;
    case CSSPrimitiveValue::CSS_NUMBER:
        if (intValue == 0)
            return m_numberZero;
        cache = &m_numberValueCache;
        break;
    default:
        return CSSPrimitiveValue::create(value, type);
    }

    RefPtr<CSSPrimitiveValue> dummyValue;
    pair<IntegerValueCache::iterator, bool> entry = cache->add(intValue, dummyValue);
    if (entry.second)
        entry.first->second = CSSPrimitiveValue::create(value, type);
    return entry.first->second;
}

}
