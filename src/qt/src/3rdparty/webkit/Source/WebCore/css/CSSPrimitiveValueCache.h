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
 
#ifndef CSSPrimitiveValueCache_h
#define CSSPrimitiveValueCache_h

#include "CSSPrimitiveValue.h"
#include "CSSValueKeywords.h"
#include <wtf/HashMap.h>
#include <wtf/RefPtr.h>

namespace WebCore {
    
class CSSPrimitiveValue;

class CSSPrimitiveValueCache : public RefCounted<CSSPrimitiveValueCache> {
public:
    static PassRefPtr<CSSPrimitiveValueCache> create() { return adoptRef(new CSSPrimitiveValueCache); }
    ~CSSPrimitiveValueCache();

    PassRefPtr<CSSPrimitiveValue> createIdentifierValue(int identifier);
    PassRefPtr<CSSPrimitiveValue> createColorValue(unsigned rgbValue);
    PassRefPtr<CSSPrimitiveValue> createValue(double value, CSSPrimitiveValue::UnitTypes);
    PassRefPtr<CSSPrimitiveValue> createValue(String value, CSSPrimitiveValue::UnitTypes type) { return CSSPrimitiveValue::create(value, type); }
    template<typename T> static PassRefPtr<CSSPrimitiveValue> createValue(T value) { return CSSPrimitiveValue::create(value); }
    
private:
    CSSPrimitiveValueCache();

    typedef HashMap<int, RefPtr<CSSPrimitiveValue> > IdentifierValueCache;
    IdentifierValueCache m_identifierValueCache;

    typedef HashMap<unsigned, RefPtr<CSSPrimitiveValue> > ColorValueCache;
    ColorValueCache m_colorValueCache;
    RefPtr<CSSPrimitiveValue> m_colorTransparent;
    RefPtr<CSSPrimitiveValue> m_colorWhite;
    RefPtr<CSSPrimitiveValue> m_colorBlack;
    
    typedef HashMap<int, RefPtr<CSSPrimitiveValue> > IntegerValueCache;
    RefPtr<CSSPrimitiveValue> m_pixelZero;
    RefPtr<CSSPrimitiveValue> m_percentZero;
    RefPtr<CSSPrimitiveValue> m_numberZero;
    IntegerValueCache m_pixelValueCache;
    IntegerValueCache m_percentValueCache;
    IntegerValueCache m_numberValueCache;
};

}

#endif
