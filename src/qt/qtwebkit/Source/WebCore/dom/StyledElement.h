/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef StyledElement_h
#define StyledElement_h

#include "CSSPrimitiveValue.h"
#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "Element.h"

namespace WebCore {

class Attribute;
class MutableStylePropertySet;
class PropertySetCSSStyleDeclaration;
class StylePropertySet;

struct PresentationAttributeCacheKey;

class StyledElement : public Element {
public:
    virtual ~StyledElement();

    virtual const StylePropertySet* additionalPresentationAttributeStyle() { return 0; }
    void invalidateStyleAttribute();

    const StylePropertySet* inlineStyle() const { return elementData() ? elementData()->m_inlineStyle.get() : 0; }
    
    bool setInlineStyleProperty(CSSPropertyID, CSSValueID identifier, bool important = false);
    bool setInlineStyleProperty(CSSPropertyID, CSSPropertyID identifier, bool important = false);
    bool setInlineStyleProperty(CSSPropertyID, double value, CSSPrimitiveValue::UnitTypes, bool important = false);
    bool setInlineStyleProperty(CSSPropertyID, const String& value, bool important = false);
    bool removeInlineStyleProperty(CSSPropertyID);
    void removeAllInlineStyleProperties();

    void synchronizeStyleAttributeInternal() const;
    
    virtual CSSStyleDeclaration* style() OVERRIDE FINAL;

    const StylePropertySet* presentationAttributeStyle();
    virtual void collectStyleForPresentationAttribute(const QualifiedName&, const AtomicString&, MutableStylePropertySet*) { }

protected:
    StyledElement(const QualifiedName& name, Document* document, ConstructionType type)
        : Element(name, document, type)
    {
    }

    virtual void attributeChanged(const QualifiedName&, const AtomicString&, AttributeModificationReason = ModifiedDirectly) OVERRIDE;

    virtual bool isPresentationAttribute(const QualifiedName&) const { return false; }

    void addPropertyToPresentationAttributeStyle(MutableStylePropertySet*, CSSPropertyID, CSSValueID identifier);
    void addPropertyToPresentationAttributeStyle(MutableStylePropertySet*, CSSPropertyID, double value, CSSPrimitiveValue::UnitTypes);
    void addPropertyToPresentationAttributeStyle(MutableStylePropertySet*, CSSPropertyID, const String& value);

    virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;

private:
    void styleAttributeChanged(const AtomicString& newStyleString, AttributeModificationReason);

    void inlineStyleChanged();
    PropertySetCSSStyleDeclaration* inlineStyleCSSOMWrapper();
    void setInlineStyleFromString(const AtomicString&);
    MutableStylePropertySet* ensureMutableInlineStyle();

    void makePresentationAttributeCacheKey(PresentationAttributeCacheKey&) const;
    void rebuildPresentationAttributeStyle();
};

inline void StyledElement::invalidateStyleAttribute()
{
    ASSERT(elementData());
    elementData()->m_styleAttributeIsDirty = true;
}

inline const StylePropertySet* StyledElement::presentationAttributeStyle()
{
    if (!elementData())
        return 0;
    if (elementData()->m_presentationAttributeStyleIsDirty)
        rebuildPresentationAttributeStyle();
    return elementData()->presentationAttributeStyle();
}

} //namespace

#endif
