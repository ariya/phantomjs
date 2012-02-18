/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
 */

#include "config.h"
#include "CSSStyleDeclaration.h"

#include "CSSMutableStyleDeclaration.h"
#include "CSSMutableValue.h"
#include "CSSParser.h"
#include "CSSProperty.h"
#include "CSSPropertyNames.h"
#include "CSSRule.h"
#include "Node.h"
#include "SVGElement.h"
#include <wtf/ASCIICType.h>

using namespace WTF;

namespace WebCore {

CSSStyleDeclaration::CSSStyleDeclaration(CSSRule* parent)
    : StyleBase(parent)
{
}

PassRefPtr<CSSValue> CSSStyleDeclaration::getPropertyCSSValue(const String& propertyName)
{
    int propID = cssPropertyID(propertyName);
    if (!propID)
        return 0;

    // Short-cut, not involving any change to the refcount.
    if (!isMutableStyleDeclaration())
        return getPropertyCSSValue(propID);

    // Slow path.
    RefPtr<CSSValue> value = getPropertyCSSValue(propID);
    if (!value || !value->isMutableValue())
        return value.release();

    Node* node = static_cast<CSSMutableStyleDeclaration*>(this)->node();
    if (!node || !node->isStyledElement())
        return value.release();

    Node* associatedNode = static_cast<CSSMutableValue*>(value.get())->node();
    if (associatedNode) {
        ASSERT(associatedNode == node);
        return value.release();
    }

    static_cast<CSSMutableValue*>(value.get())->setNode(node);
    return value.release();
}

String CSSStyleDeclaration::getPropertyValue(const String &propertyName)
{
    int propID = cssPropertyID(propertyName);
    if (!propID)
        return String();
    return getPropertyValue(propID);
}

String CSSStyleDeclaration::getPropertyPriority(const String& propertyName)
{
    int propID = cssPropertyID(propertyName);
    if (!propID)
        return String();
    return getPropertyPriority(propID) ? "important" : "";
}

String CSSStyleDeclaration::getPropertyShorthand(const String& propertyName)
{
    int propID = cssPropertyID(propertyName);
    if (!propID)
        return String();
    int shorthandID = getPropertyShorthand(propID);
    if (!shorthandID)
        return String();
    return getPropertyName(static_cast<CSSPropertyID>(shorthandID));
}

bool CSSStyleDeclaration::isPropertyImplicit(const String& propertyName)
{
    int propID = cssPropertyID(propertyName);
    if (!propID)
        return false;
    return isPropertyImplicit(propID);
}

void CSSStyleDeclaration::setProperty(const String& propertyName, const String& value, ExceptionCode& ec)
{
    size_t important = value.find("!important", 0, false);
    int propertyID = cssPropertyID(propertyName);
    if (!propertyID)
        return;
    if (important == notFound)
        setProperty(propertyID, value, false, ec);
    else
        setProperty(propertyID, value.left(important - 1), true, ec);
}

void CSSStyleDeclaration::setProperty(const String& propertyName, const String& value, const String& priority, ExceptionCode& ec)
{
    int propID = cssPropertyID(propertyName);
    if (!propID) {
        // FIXME: Should we raise an exception here?
        return;
    }
    bool important = priority.find("important", 0, false) != notFound;
    setProperty(propID, value, important, ec);
}

String CSSStyleDeclaration::removeProperty(const String& propertyName, ExceptionCode& ec)
{
    int propID = cssPropertyID(propertyName);
    if (!propID)
        return String();
    return removeProperty(propID, ec);
}

bool CSSStyleDeclaration::isPropertyName(const String& propertyName)
{
    return cssPropertyID(propertyName);
}

CSSRule* CSSStyleDeclaration::parentRule() const
{
    return (parent() && parent()->isRule()) ? static_cast<CSSRule*>(parent()) : 0;
}

bool CSSStyleDeclaration::cssPropertyMatches(const CSSProperty* property) const
{
    RefPtr<CSSValue> value = getPropertyCSSValue(property->id());
    return value && value->cssText() == property->value()->cssText();
}

void CSSStyleDeclaration::diff(CSSMutableStyleDeclaration* style) const
{
    if (!style)
        return;

    Vector<int> propertiesToRemove;
    {
        CSSMutableStyleDeclaration::const_iterator end = style->end();
        for (CSSMutableStyleDeclaration::const_iterator it = style->begin(); it != end; ++it) {
            const CSSProperty& property = *it;
            if (cssPropertyMatches(&property))
                propertiesToRemove.append(property.id());
        }
    }

    // FIXME: This should use mass removal.
    for (unsigned i = 0; i < propertiesToRemove.size(); i++)
        style->removeProperty(propertiesToRemove[i]);
}

PassRefPtr<CSSMutableStyleDeclaration> CSSStyleDeclaration::copyPropertiesInSet(const int* set, unsigned length) const
{
    Vector<CSSProperty> list;
    list.reserveInitialCapacity(length);
    for (unsigned i = 0; i < length; i++) {
        RefPtr<CSSValue> value = getPropertyCSSValue(set[i]);
        if (value)
            list.append(CSSProperty(set[i], value.release(), false));
    }
    return CSSMutableStyleDeclaration::create(list);
}

} // namespace WebCore
