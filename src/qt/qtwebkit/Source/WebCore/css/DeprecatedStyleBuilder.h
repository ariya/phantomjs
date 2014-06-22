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

#ifndef DeprecatedStyleBuilder_h
#define DeprecatedStyleBuilder_h

#include "CSSPropertyNames.h"
#include "StylePropertyShorthand.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class CSSValue;
class DeprecatedStyleBuilder;
class StyleResolver;

class PropertyHandler {
public:
    typedef void (*InheritFunction)(CSSPropertyID, StyleResolver*);
    typedef void (*InitialFunction)(CSSPropertyID, StyleResolver*);
    typedef void (*ApplyFunction)(CSSPropertyID, StyleResolver*, CSSValue*);
    PropertyHandler() : m_inherit(0), m_initial(0), m_apply(0) { }
    PropertyHandler(InheritFunction inherit, InitialFunction initial, ApplyFunction apply) : m_inherit(inherit), m_initial(initial), m_apply(apply) { }
    void applyInheritValue(CSSPropertyID propertyID, StyleResolver* styleResolver) const { ASSERT(m_inherit); (*m_inherit)(propertyID, styleResolver); }
    void applyInitialValue(CSSPropertyID propertyID, StyleResolver* styleResolver) const { ASSERT(m_initial); (*m_initial)(propertyID, styleResolver); }
    void applyValue(CSSPropertyID propertyID, StyleResolver* styleResolver, CSSValue* value) const { ASSERT(m_apply); (*m_apply)(propertyID, styleResolver, value); }
    bool isValid() const { return m_inherit && m_initial && m_apply; }
    InheritFunction inheritFunction() const { return m_inherit; }
    InitialFunction initialFunction() { return m_initial; }
    ApplyFunction applyFunction() { return m_apply; }
private:
    InheritFunction m_inherit;
    InitialFunction m_initial;
    ApplyFunction m_apply;
};

class DeprecatedStyleBuilder {
    WTF_MAKE_NONCOPYABLE(DeprecatedStyleBuilder); WTF_MAKE_FAST_ALLOCATED;
public:
    static const DeprecatedStyleBuilder& sharedStyleBuilder();

    const PropertyHandler& propertyHandler(CSSPropertyID property) const
    {
        ASSERT(valid(property));
        return m_propertyMap[index(property)];
    }
private:
    DeprecatedStyleBuilder();
    static int index(CSSPropertyID property)
    {
        return property - firstCSSProperty;
    }

    static bool valid(CSSPropertyID property)
    {
        int i = index(property);
        return i >= 0 && i < numCSSProperties;
    }

    void setPropertyHandler(CSSPropertyID property, const PropertyHandler& handler)
    {
        ASSERT(valid(property));
        ASSERT(!propertyHandler(property).isValid());
        ASSERT_WITH_MESSAGE(!isExpandedShorthand(property), "Shorthand property id = %d shouldn't be inserted into StyleBuilder. Shorthands should be expanded at parsing time.", property);
        m_propertyMap[index(property)] = handler;
    }

    void setPropertyHandler(CSSPropertyID newProperty, CSSPropertyID equivalentProperty)
    {
        ASSERT(valid(newProperty));
        ASSERT(valid(equivalentProperty));
        ASSERT(!propertyHandler(newProperty).isValid());
        ASSERT_WITH_MESSAGE(!isExpandedShorthand(newProperty), "Shorthand property id = %d shouldn't be inserted into StyleBuilder. Shorthands should be expanded at parsing time.", newProperty);
        ASSERT_WITH_MESSAGE(!isExpandedShorthand(equivalentProperty), "Shorthand property id = %d shouldn't be inserted into StyleBuilder. Shorthands should be expanded at parsing time.", equivalentProperty);
        m_propertyMap[index(newProperty)] = m_propertyMap[index(equivalentProperty)];
    }

    PropertyHandler m_propertyMap[numCSSProperties];
};

}

#endif // DeprecatedStyleBuilder_h
