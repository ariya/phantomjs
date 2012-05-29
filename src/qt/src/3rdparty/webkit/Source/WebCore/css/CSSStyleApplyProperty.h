/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CSSStyleApplyProperty_h
#define CSSStyleApplyProperty_h

#include "CSSPropertyNames.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class CSSStyleSelector;
class CSSValue;
class ApplyPropertyBase {
    WTF_MAKE_NONCOPYABLE(ApplyPropertyBase);
    WTF_MAKE_FAST_ALLOCATED;
public:
    ApplyPropertyBase() { }
    virtual ~ApplyPropertyBase() { }
    virtual void applyInheritValue(CSSStyleSelector*) const = 0;
    virtual void applyInitialValue(CSSStyleSelector*) const = 0;
    virtual void applyValue(CSSStyleSelector*, CSSValue*) const = 0;
};

class CSSStyleApplyProperty {
    WTF_MAKE_NONCOPYABLE(CSSStyleApplyProperty);
public:
    static const CSSStyleApplyProperty& sharedCSSStyleApplyProperty();

    void applyInheritValue(CSSPropertyID property, CSSStyleSelector* selector) const
    {
        ASSERT(implements(property));
        propertyHandler(property)->applyInheritValue(selector);
    }

    void applyInitialValue(CSSPropertyID property, CSSStyleSelector* selector) const
    {
        ASSERT(implements(property));
        propertyHandler(property)->applyInitialValue(selector);
    }

    void applyValue(CSSPropertyID property, CSSStyleSelector* selector, CSSValue* value) const
    {
        ASSERT(implements(property));
        propertyHandler(property)->applyValue(selector, value);
    }

    bool implements(CSSPropertyID property) const
    {
        return propertyHandler(property);
    }

private:
    CSSStyleApplyProperty();
    static int index(CSSPropertyID property)
    {
        return property - firstCSSProperty;
    }

    static bool valid(CSSPropertyID property)
    {
        int i = index(property);
        return i >= 0 && i < numCSSProperties;
    }

    void setPropertyHandler(CSSPropertyID property, ApplyPropertyBase* value)
    {
        ASSERT(valid(property));
        ASSERT(!propertyHandler(property));
        m_propertyMap[index(property)] = value;
    }

    void setPropertyHandler(CSSPropertyID newProperty, CSSPropertyID equivalentProperty)
    {
        ASSERT(valid(newProperty));
        ASSERT(valid(equivalentProperty));
        ASSERT(!propertyHandler(newProperty));
        m_propertyMap[index(newProperty)] = m_propertyMap[index(equivalentProperty)];
    }

    ApplyPropertyBase* propertyHandler(CSSPropertyID property) const
    {
        ASSERT(valid(property));
        return m_propertyMap[index(property)];
    }

    ApplyPropertyBase* m_propertyMap[numCSSProperties];
};

}

#endif // CSSStyleApplyProperty_h
