/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
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
#include "CSSMutableStyleDeclaration.h"

#include "CSSImageValue.h"
#include "CSSMutableValue.h"
#include "CSSParser.h"
#include "CSSPropertyLonghand.h"
#include "CSSPropertyNames.h"
#include "CSSRule.h"
#include "CSSStyleSheet.h"
#include "CSSValueKeywords.h"
#include "CSSValueList.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "InspectorInstrumentation.h"
#include "StyledElement.h"
#include <wtf/text/StringConcatenate.h>

using namespace std;

namespace WebCore {

CSSMutableStyleDeclaration::CSSMutableStyleDeclaration()
    : m_node(0)
    , m_strictParsing(false)
#ifndef NDEBUG
    , m_iteratorCount(0)
#endif
{
}

CSSMutableStyleDeclaration::CSSMutableStyleDeclaration(CSSRule* parent)
    : CSSStyleDeclaration(parent)
    , m_node(0)
    , m_strictParsing(!parent || parent->useStrictParsing())
#ifndef NDEBUG
    , m_iteratorCount(0)
#endif
{
}

CSSMutableStyleDeclaration::CSSMutableStyleDeclaration(CSSRule* parent, const Vector<CSSProperty>& properties)
    : CSSStyleDeclaration(parent)
    , m_properties(properties)
    , m_node(0)
    , m_strictParsing(!parent || parent->useStrictParsing())
#ifndef NDEBUG
    , m_iteratorCount(0)
#endif
{
    m_properties.shrinkToFit();
    // FIXME: This allows duplicate properties.
}

CSSMutableStyleDeclaration::CSSMutableStyleDeclaration(CSSRule* parent, const CSSProperty* const * properties, int numProperties)
    : CSSStyleDeclaration(parent)
    , m_node(0)
    , m_strictParsing(!parent || parent->useStrictParsing())
#ifndef NDEBUG
    , m_iteratorCount(0)
#endif
{
    m_properties.reserveInitialCapacity(numProperties);
    HashMap<int, bool> candidates;
    for (int i = 0; i < numProperties; ++i) {
        const CSSProperty *property = properties[i];
        ASSERT(property);
        bool important = property->isImportant();
        if (candidates.contains(property->id())) {
            if (!important && candidates.get(property->id()))
                continue;
            removeProperty(property->id(), false);
        }
        m_properties.append(*property);
        candidates.set(property->id(), important);
    }
}

CSSMutableStyleDeclaration::~CSSMutableStyleDeclaration()
{
    const CSSMutableStyleDeclarationConstIterator end = this->end();
    for (CSSMutableStyleDeclarationConstIterator it = begin(); it != end; ++it) {
        CSSValue* value = it->value();
        if (!value || !value->isMutableValue())
            continue;
        static_cast<CSSMutableValue*>(value)->setNode(0);
    }
}
 
CSSMutableStyleDeclaration& CSSMutableStyleDeclaration::operator=(const CSSMutableStyleDeclaration& other)
{
    ASSERT(!m_iteratorCount);
    // don't attach it to the same node, just leave the current m_node value
    m_properties = other.m_properties;
    m_strictParsing = other.m_strictParsing;
    return *this;
}

String CSSMutableStyleDeclaration::getPropertyValue(int propertyID) const
{
    RefPtr<CSSValue> value = getPropertyCSSValue(propertyID);
    if (value)
        return value->cssText();

    // Shorthand and 4-values properties
    switch (propertyID) {
        case CSSPropertyBorderSpacing: {
            const int properties[2] = { CSSPropertyWebkitBorderHorizontalSpacing, CSSPropertyWebkitBorderVerticalSpacing };
            return borderSpacingValue(properties);
        }
        case CSSPropertyBackgroundPosition: {
            // FIXME: Is this correct? The code in cssparser.cpp is confusing
            const int properties[2] = { CSSPropertyBackgroundPositionX, CSSPropertyBackgroundPositionY };
            return getLayeredShorthandValue(properties);
        }
        case CSSPropertyBackgroundRepeat: {
            const int properties[2] = { CSSPropertyBackgroundRepeatX, CSSPropertyBackgroundRepeatY };
            return getLayeredShorthandValue(properties);
        }
        case CSSPropertyBackground: {
            const int properties[9] = { CSSPropertyBackgroundColor,
                                        CSSPropertyBackgroundImage, 
                                        CSSPropertyBackgroundRepeatX, 
                                        CSSPropertyBackgroundRepeatY, 
                                        CSSPropertyBackgroundAttachment, 
                                        CSSPropertyBackgroundPositionX, 
                                        CSSPropertyBackgroundPositionY, 
                                        CSSPropertyBackgroundClip,
                                        CSSPropertyBackgroundOrigin }; 
            return getLayeredShorthandValue(properties);
        }
        case CSSPropertyBorder: {
            const int properties[3][4] = {{ CSSPropertyBorderTopWidth,
                                            CSSPropertyBorderRightWidth,
                                            CSSPropertyBorderBottomWidth,
                                            CSSPropertyBorderLeftWidth },
                                          { CSSPropertyBorderTopStyle,
                                            CSSPropertyBorderRightStyle,
                                            CSSPropertyBorderBottomStyle,
                                            CSSPropertyBorderLeftStyle },
                                          { CSSPropertyBorderTopColor,
                                            CSSPropertyBorderRightColor,
                                            CSSPropertyBorderBottomColor,
                                            CSSPropertyBorderLeftColor }};
            String res;
            for (size_t i = 0; i < WTF_ARRAY_LENGTH(properties); ++i) {
                String value = getCommonValue(properties[i]);
                if (!value.isNull()) {
                    if (!res.isNull())
                        res += " ";
                    res += value;
                }
            }
            return res;
        }
        case CSSPropertyBorderTop: {
            const int properties[3] = { CSSPropertyBorderTopWidth, CSSPropertyBorderTopStyle,
                                        CSSPropertyBorderTopColor};
            return getShorthandValue(properties);
        }
        case CSSPropertyBorderRight: {
            const int properties[3] = { CSSPropertyBorderRightWidth, CSSPropertyBorderRightStyle,
                                        CSSPropertyBorderRightColor};
            return getShorthandValue(properties);
        }
        case CSSPropertyBorderBottom: {
            const int properties[3] = { CSSPropertyBorderBottomWidth, CSSPropertyBorderBottomStyle,
                                        CSSPropertyBorderBottomColor};
            return getShorthandValue(properties);
        }
        case CSSPropertyBorderLeft: {
            const int properties[3] = { CSSPropertyBorderLeftWidth, CSSPropertyBorderLeftStyle,
                                        CSSPropertyBorderLeftColor};
            return getShorthandValue(properties);
        }
        case CSSPropertyOutline: {
            const int properties[3] = { CSSPropertyOutlineWidth, CSSPropertyOutlineStyle,
                                        CSSPropertyOutlineColor };
            return getShorthandValue(properties);
        }
        case CSSPropertyBorderColor: {
            const int properties[4] = { CSSPropertyBorderTopColor, CSSPropertyBorderRightColor,
                                        CSSPropertyBorderBottomColor, CSSPropertyBorderLeftColor };
            return get4Values(properties);
        }
        case CSSPropertyBorderWidth: {
            const int properties[4] = { CSSPropertyBorderTopWidth, CSSPropertyBorderRightWidth,
                                        CSSPropertyBorderBottomWidth, CSSPropertyBorderLeftWidth };
            return get4Values(properties);
        }
        case CSSPropertyBorderStyle: {
            const int properties[4] = { CSSPropertyBorderTopStyle, CSSPropertyBorderRightStyle,
                                        CSSPropertyBorderBottomStyle, CSSPropertyBorderLeftStyle };
            return get4Values(properties);
        }
        case CSSPropertyMargin: {
            const int properties[4] = { CSSPropertyMarginTop, CSSPropertyMarginRight,
                                        CSSPropertyMarginBottom, CSSPropertyMarginLeft };
            return get4Values(properties);
        }
        case CSSPropertyOverflow: {
            const int properties[2] = { CSSPropertyOverflowX, CSSPropertyOverflowY };
            return getCommonValue(properties);
        }
        case CSSPropertyPadding: {
            const int properties[4] = { CSSPropertyPaddingTop, CSSPropertyPaddingRight,
                                        CSSPropertyPaddingBottom, CSSPropertyPaddingLeft };
            return get4Values(properties);
        }
        case CSSPropertyListStyle: {
            const int properties[3] = { CSSPropertyListStyleType, CSSPropertyListStylePosition,
                                        CSSPropertyListStyleImage };
            return getShorthandValue(properties);
        }
        case CSSPropertyWebkitMaskPosition: {
            // FIXME: Is this correct? The code in cssparser.cpp is confusing
            const int properties[2] = { CSSPropertyWebkitMaskPositionX, CSSPropertyWebkitMaskPositionY };
            return getLayeredShorthandValue(properties);
        }
        case CSSPropertyWebkitMaskRepeat: {
            const int properties[2] = { CSSPropertyWebkitMaskRepeatX, CSSPropertyWebkitMaskRepeatY };
            return getLayeredShorthandValue(properties);
        }
        case CSSPropertyWebkitMask: {
            const int properties[] = { CSSPropertyWebkitMaskImage, CSSPropertyWebkitMaskRepeat, 
                                       CSSPropertyWebkitMaskAttachment, CSSPropertyWebkitMaskPosition, CSSPropertyWebkitMaskClip,
                                       CSSPropertyWebkitMaskOrigin };
            return getLayeredShorthandValue(properties);
        }
        case CSSPropertyWebkitTransformOrigin: {
            const int properties[3] = { CSSPropertyWebkitTransformOriginX,
                                        CSSPropertyWebkitTransformOriginY,
                                        CSSPropertyWebkitTransformOriginZ };
            return getShorthandValue(properties);
        }
        case CSSPropertyWebkitTransition: {
            const int properties[4] = { CSSPropertyWebkitTransitionProperty, CSSPropertyWebkitTransitionDuration,
                                        CSSPropertyWebkitTransitionTimingFunction, CSSPropertyWebkitTransitionDelay };
            return getLayeredShorthandValue(properties);
        }
        case CSSPropertyWebkitAnimation: {
            const int properties[7] = { CSSPropertyWebkitAnimationName, CSSPropertyWebkitAnimationDuration,
                                        CSSPropertyWebkitAnimationTimingFunction, CSSPropertyWebkitAnimationDelay,
                                        CSSPropertyWebkitAnimationIterationCount, CSSPropertyWebkitAnimationDirection,
                                        CSSPropertyWebkitAnimationFillMode };
            return getLayeredShorthandValue(properties);
        }
#if ENABLE(SVG)
        case CSSPropertyMarker: {
            RefPtr<CSSValue> value = getPropertyCSSValue(CSSPropertyMarkerStart);
            if (value)
                return value->cssText();
        }
#endif
    }
    return String();
}

String CSSMutableStyleDeclaration::borderSpacingValue(const int properties[2]) const
{
    RefPtr<CSSValue> horizontalValue = getPropertyCSSValue(properties[0]);
    RefPtr<CSSValue> verticalValue = getPropertyCSSValue(properties[1]);

    if (!horizontalValue)
        return String();
    ASSERT(verticalValue); // By <http://www.w3.org/TR/CSS21/tables.html#separated-borders>.

    String horizontalValueCSSText = horizontalValue->cssText();
    String verticalValueCSSText = verticalValue->cssText();
    if (horizontalValueCSSText == verticalValueCSSText)
        return horizontalValueCSSText;
    return makeString(horizontalValueCSSText, ' ', verticalValueCSSText);
}

String CSSMutableStyleDeclaration::get4Values(const int* properties) const
{
    // Assume the properties are in the usual order top, right, bottom, left.
    RefPtr<CSSValue> topValue = getPropertyCSSValue(properties[0]);
    RefPtr<CSSValue> rightValue = getPropertyCSSValue(properties[1]);
    RefPtr<CSSValue> bottomValue = getPropertyCSSValue(properties[2]);
    RefPtr<CSSValue> leftValue = getPropertyCSSValue(properties[3]);

    // All 4 properties must be specified.
    if (!topValue || !rightValue || !bottomValue || !leftValue)
        return String();

    bool showLeft = rightValue->cssText() != leftValue->cssText();
    bool showBottom = (topValue->cssText() != bottomValue->cssText()) || showLeft;
    bool showRight = (topValue->cssText() != rightValue->cssText()) || showBottom;

    String res = topValue->cssText();
    if (showRight)
        res += " " + rightValue->cssText();
    if (showBottom)
        res += " " + bottomValue->cssText();
    if (showLeft)
        res += " " + leftValue->cssText();

    return res;
}

String CSSMutableStyleDeclaration::getLayeredShorthandValue(const int* properties, size_t size) const
{
    String res;

    // Begin by collecting the properties into an array.
    Vector< RefPtr<CSSValue> > values(size);
    size_t numLayers = 0;
    
    for (size_t i = 0; i < size; ++i) {
        values[i] = getPropertyCSSValue(properties[i]);
        if (values[i]) {
            if (values[i]->isValueList()) {
                CSSValueList* valueList = static_cast<CSSValueList*>(values[i].get());
                numLayers = max(valueList->length(), numLayers);
            } else
                numLayers = max<size_t>(1U, numLayers);
        }
    }
    
    // Now stitch the properties together.  Implicit initial values are flagged as such and
    // can safely be omitted.
    for (size_t i = 0; i < numLayers; i++) {
        String layerRes;
        bool useRepeatXShorthand = false;
        bool useRepeatYShorthand = false;
        bool useSingleWordShorthand = false;
        for (size_t j = 0; j < size; j++) {
            RefPtr<CSSValue> value;
            if (values[j]) {
                if (values[j]->isValueList())
                    value = static_cast<CSSValueList*>(values[j].get())->item(i);
                else {
                    value = values[j];
                    
                    // Color only belongs in the last layer.
                    if (properties[j] == CSSPropertyBackgroundColor) {
                        if (i != numLayers - 1)
                            value = 0;
                    } else if (i != 0) // Other singletons only belong in the first layer.
                        value = 0;
                }
            }

            // We need to report background-repeat as it was written in the CSS. If the property is implicit,
            // then it was written with only one value. Here we figure out which value that was so we can
            // report back correctly. 
            if (properties[j] == CSSPropertyBackgroundRepeatX && isPropertyImplicit(properties[j])) {

                // BUG 49055: make sure the value was not reset in the layer check just above.
                if (j < size - 1 && properties[j + 1] == CSSPropertyBackgroundRepeatY && value) {
                    RefPtr<CSSValue> yValue;
                    RefPtr<CSSValue> nextValue = values[j + 1];
                    if (nextValue->isValueList())
                        yValue = static_cast<CSSValueList*>(nextValue.get())->itemWithoutBoundsCheck(i);
                    else
                        yValue = nextValue;
                        
                    int xId = static_cast<CSSPrimitiveValue*>(value.get())->getIdent();
                    int yId = static_cast<CSSPrimitiveValue*>(yValue.get())->getIdent();
                    if (xId != yId) {
                        if (xId == CSSValueRepeat && yId == CSSValueNoRepeat) {
                            useRepeatXShorthand = true;
                            ++j;
                        } else if (xId == CSSValueNoRepeat && yId == CSSValueRepeat) {
                            useRepeatYShorthand = true;
                            continue;
                        }
                    } else {
                        useSingleWordShorthand = true;
                        ++j;
                    }
                }
            }
            
            if (value && !value->isImplicitInitialValue()) {
                if (!layerRes.isNull())
                    layerRes += " ";
                if (useRepeatXShorthand) {
                    useRepeatXShorthand = false;
                    layerRes += getValueName(CSSValueRepeatX);
                } else if (useRepeatYShorthand) {
                    useRepeatYShorthand = false;
                    layerRes += getValueName(CSSValueRepeatY);
                } else if (useSingleWordShorthand) {
                    useSingleWordShorthand = false;
                    layerRes += value->cssText();
                } else
                    layerRes += value->cssText();
            }
        }
        
        if (!layerRes.isNull()) {
            if (!res.isNull())
                res += ", ";
            res += layerRes;
        }
    }

    return res;
}

String CSSMutableStyleDeclaration::getShorthandValue(const int* properties, size_t size) const
{
    String res;
    for (size_t i = 0; i < size; ++i) {
        if (!isPropertyImplicit(properties[i])) {
            RefPtr<CSSValue> value = getPropertyCSSValue(properties[i]);
            // FIXME: provide default value if !value
            if (value) {
                if (!res.isNull())
                    res += " ";
                res += value->cssText();
            }
        }
    }
    return res;
}

// only returns a non-null value if all properties have the same, non-null value
String CSSMutableStyleDeclaration::getCommonValue(const int* properties, size_t size) const
{
    String res;
    for (size_t i = 0; i < size; ++i) {
        RefPtr<CSSValue> value = getPropertyCSSValue(properties[i]);
        if (!value)
            return String();
        String text = value->cssText();
        if (text.isNull())
            return String();
        if (res.isNull())
            res = text;
        else if (res != text)
            return String();
    }
    return res;
}

PassRefPtr<CSSValue> CSSMutableStyleDeclaration::getPropertyCSSValue(int propertyID) const
{
    const CSSProperty* property = findPropertyWithId(propertyID);
    return property ? property->value() : 0;
}

bool CSSMutableStyleDeclaration::removeShorthandProperty(int propertyID, bool notifyChanged) 
{
    CSSPropertyLonghand longhand = longhandForProperty(propertyID);
    if (longhand.length()) {
        removePropertiesInSet(longhand.properties(), longhand.length(), notifyChanged);
        return true;
    }
    return false;
}

String CSSMutableStyleDeclaration::removeProperty(int propertyID, bool notifyChanged, bool returnText)
{
    ASSERT(!m_iteratorCount);

    if (removeShorthandProperty(propertyID, notifyChanged)) {
        // FIXME: Return an equivalent shorthand when possible.
        return String();
    }
   
    CSSProperty* foundProperty = findPropertyWithId(propertyID);
    if (!foundProperty)
        return String();
    
    String value = returnText ? foundProperty->value()->cssText() : String();

    // A more efficient removal strategy would involve marking entries as empty
    // and sweeping them when the vector grows too big.
    m_properties.remove(foundProperty - m_properties.data());

    if (notifyChanged)
        setNeedsStyleRecalc();

    return value;
}

bool CSSMutableStyleDeclaration::isInlineStyleDeclaration()
{
    // FIXME: Ideally, this should be factored better and there
    // should be a subclass of CSSMutableStyleDeclaration just
    // for inline style declarations that handles this
    return m_node && m_node->isStyledElement() && static_cast<StyledElement*>(m_node)->inlineStyleDecl() == this;
}

void CSSMutableStyleDeclaration::setNeedsStyleRecalc()
{
    if (m_node) {
        if (isInlineStyleDeclaration()) {
            m_node->setNeedsStyleRecalc(InlineStyleChange);
            static_cast<StyledElement*>(m_node)->invalidateStyleAttribute();
            if (m_node->document())
                InspectorInstrumentation::didInvalidateStyleAttr(m_node->document(), m_node);
        } else
            m_node->setNeedsStyleRecalc(FullStyleChange);
        return;
    }

    StyleBase* root = this;
    while (StyleBase* parent = root->parent())
        root = parent;
    if (root->isCSSStyleSheet()) {
        if (Document* document = static_cast<CSSStyleSheet*>(root)->document())
            document->styleSelectorChanged(DeferRecalcStyle);
    }
}

bool CSSMutableStyleDeclaration::getPropertyPriority(int propertyID) const
{
    const CSSProperty* property = findPropertyWithId(propertyID);
    return property ? property->isImportant() : false;
}

int CSSMutableStyleDeclaration::getPropertyShorthand(int propertyID) const
{
    const CSSProperty* property = findPropertyWithId(propertyID);
    return property ? property->shorthandID() : 0;
}

bool CSSMutableStyleDeclaration::isPropertyImplicit(int propertyID) const
{
    const CSSProperty* property = findPropertyWithId(propertyID);
    return property ? property->isImplicit() : false;
}

void CSSMutableStyleDeclaration::setProperty(int propertyID, const String& value, bool important, ExceptionCode& ec)
{
    ec = 0;
    setProperty(propertyID, value, important, true);
}

String CSSMutableStyleDeclaration::removeProperty(int propertyID, ExceptionCode& ec)
{
    ec = 0;
    return removeProperty(propertyID, true, true);
}

bool CSSMutableStyleDeclaration::setProperty(int propertyID, const String& value, bool important, bool notifyChanged)
{
    ASSERT(!m_iteratorCount);

    // Setting the value to an empty string just removes the property in both IE and Gecko.
    // Setting it to null seems to produce less consistent results, but we treat it just the same.
    if (value.isEmpty()) {
        removeProperty(propertyID, notifyChanged, false);
        return true;
    }

    // When replacing an existing property value, this moves the property to the end of the list.
    // Firefox preserves the position, and MSIE moves the property to the beginning.
    bool success = CSSParser::parseValue(this, propertyID, value, important, useStrictParsing());
    if (!success) {
        // CSS DOM requires raising SYNTAX_ERR here, but this is too dangerous for compatibility,
        // see <http://bugs.webkit.org/show_bug.cgi?id=7296>.
    } else if (notifyChanged)
        setNeedsStyleRecalc();

    return success;
}
    
void CSSMutableStyleDeclaration::setPropertyInternal(const CSSProperty& property, CSSProperty* slot)
{
    ASSERT(!m_iteratorCount);

    if (!removeShorthandProperty(property.id(), false)) {
        CSSProperty* toReplace = slot ? slot : findPropertyWithId(property.id());
        if (toReplace) {
            *toReplace = property;
            return;
        }
    }
    m_properties.append(property);
}

bool CSSMutableStyleDeclaration::setProperty(int propertyID, int value, bool important, bool notifyChanged)
{
    CSSProperty property(propertyID, CSSPrimitiveValue::createIdentifier(value), important);
    setPropertyInternal(property);
    if (notifyChanged)
        setNeedsStyleRecalc();
    return true;
}

bool CSSMutableStyleDeclaration::setProperty(int propertyID, double value, CSSPrimitiveValue::UnitTypes unit, bool important, bool notifyChanged)
{
    CSSProperty property(propertyID, CSSPrimitiveValue::create(value, unit), important);
    setPropertyInternal(property);
    if (notifyChanged)
        setNeedsStyleRecalc();
    return true;
}

void CSSMutableStyleDeclaration::setStringProperty(int propertyId, const String &value, CSSPrimitiveValue::UnitTypes type, bool important)
{
    ASSERT(!m_iteratorCount);

    setPropertyInternal(CSSProperty(propertyId, CSSPrimitiveValue::create(value, type), important));
    setNeedsStyleRecalc();
}

void CSSMutableStyleDeclaration::setImageProperty(int propertyId, const String& url, bool important)
{
    ASSERT(!m_iteratorCount);

    setPropertyInternal(CSSProperty(propertyId, CSSImageValue::create(url), important));
    setNeedsStyleRecalc();
}

void CSSMutableStyleDeclaration::parseDeclaration(const String& styleDeclaration)
{
    ASSERT(!m_iteratorCount);

    m_properties.clear();
    CSSParser parser(useStrictParsing());
    parser.parseDeclaration(this, styleDeclaration);
    setNeedsStyleRecalc();
}

void CSSMutableStyleDeclaration::addParsedProperties(const CSSProperty* const* properties, int numProperties)
{
    ASSERT(!m_iteratorCount);

    m_properties.reserveCapacity(numProperties);
    for (int i = 0; i < numProperties; ++i)
        addParsedProperty(*properties[i]);

    // FIXME: This probably should have a call to setNeedsStyleRecalc() if something changed. We may also wish to add
    // a notifyChanged argument to this function to follow the model of other functions in this class.
}

void CSSMutableStyleDeclaration::addParsedProperty(const CSSProperty& property)
{
    ASSERT(!m_iteratorCount);

    // Only add properties that have no !important counterpart present
    if (!getPropertyPriority(property.id()) || property.isImportant()) {
        removeProperty(property.id(), false);
        m_properties.append(property);
    }
}

void CSSMutableStyleDeclaration::setLengthProperty(int propertyId, const String& value, bool important, bool /*multiLength*/)
{
    ASSERT(!m_iteratorCount);

    bool parseMode = useStrictParsing();
    setStrictParsing(false);
    setProperty(propertyId, value, important);
    setStrictParsing(parseMode);
}

unsigned CSSMutableStyleDeclaration::virtualLength() const
{
    return length();
}

String CSSMutableStyleDeclaration::item(unsigned i) const
{
    if (i >= m_properties.size())
       return "";
    return getPropertyName(static_cast<CSSPropertyID>(m_properties[i].id()));
}

String CSSMutableStyleDeclaration::cssText() const
{
    String result = "";
    
    const CSSProperty* positionXProp = 0;
    const CSSProperty* positionYProp = 0;
    const CSSProperty* repeatXProp = 0;
    const CSSProperty* repeatYProp = 0;
    
    unsigned size = m_properties.size();
    for (unsigned n = 0; n < size; ++n) {
        const CSSProperty& prop = m_properties[n];
        if (prop.id() == CSSPropertyBackgroundPositionX)
            positionXProp = &prop;
        else if (prop.id() == CSSPropertyBackgroundPositionY)
            positionYProp = &prop;
        else if (prop.id() == CSSPropertyBackgroundRepeatX)
            repeatXProp = &prop;
        else if (prop.id() == CSSPropertyBackgroundRepeatY)
            repeatYProp = &prop;
        else
            result += prop.cssText();
    }
    
    // FIXME: This is a not-so-nice way to turn x/y positions into single background-position in output.
    // It is required because background-position-x/y are non-standard properties and WebKit generated output 
    // would not work in Firefox (<rdar://problem/5143183>)
    // It would be a better solution if background-position was CSS_PAIR.
    if (positionXProp && positionYProp && positionXProp->isImportant() == positionYProp->isImportant()) {
        String positionValue;
        const int properties[2] = { CSSPropertyBackgroundPositionX, CSSPropertyBackgroundPositionY };
        if (positionXProp->value()->isValueList() || positionYProp->value()->isValueList()) 
            positionValue = getLayeredShorthandValue(properties);
        else
            positionValue = positionXProp->value()->cssText() + " " + positionYProp->value()->cssText();
        result += "background-position: " + positionValue + (positionXProp->isImportant() ? " !important" : "") + "; ";
    } else {
        if (positionXProp) 
            result += positionXProp->cssText();
        if (positionYProp)
            result += positionYProp->cssText();
    }

    // FIXME: We need to do the same for background-repeat.
    if (repeatXProp && repeatYProp && repeatXProp->isImportant() == repeatYProp->isImportant()) {
        String repeatValue;
        const int repeatProperties[2] = { CSSPropertyBackgroundRepeatX, CSSPropertyBackgroundRepeatY };
        if (repeatXProp->value()->isValueList() || repeatYProp->value()->isValueList()) 
            repeatValue = getLayeredShorthandValue(repeatProperties);
        else
            repeatValue = repeatXProp->value()->cssText() + " " + repeatYProp->value()->cssText();
        result += "background-repeat: " + repeatValue + (repeatXProp->isImportant() ? " !important" : "") + "; ";
    } else {
        if (repeatXProp) 
            result += repeatXProp->cssText();
        if (repeatYProp)
            result += repeatYProp->cssText();
    }

    return result;
}

void CSSMutableStyleDeclaration::setCssText(const String& text, ExceptionCode& ec)
{
    ASSERT(!m_iteratorCount);

    ec = 0;
    m_properties.clear();
    CSSParser parser(useStrictParsing());
    parser.parseDeclaration(this, text);
    // FIXME: Detect syntax errors and set ec.
    setNeedsStyleRecalc();
}

void CSSMutableStyleDeclaration::merge(const CSSMutableStyleDeclaration* other, bool argOverridesOnConflict)
{
    ASSERT(!m_iteratorCount);

    unsigned size = other->m_properties.size();
    for (unsigned n = 0; n < size; ++n) {
        const CSSProperty& toMerge = other->m_properties[n];
        CSSProperty* old = findPropertyWithId(toMerge.id());
        if (old) {
            if (!argOverridesOnConflict && old->value())
                continue;
            setPropertyInternal(toMerge, old);
        } else
            m_properties.append(toMerge);
    }
    // FIXME: This probably should have a call to setNeedsStyleRecalc() if something changed. We may also wish to add
    // a notifyChanged argument to this function to follow the model of other functions in this class.
}

void CSSMutableStyleDeclaration::addSubresourceStyleURLs(ListHashSet<KURL>& urls)
{
    CSSStyleSheet* sheet = static_cast<CSSStyleSheet*>(stylesheet());
    size_t size = m_properties.size();
    for (size_t i = 0; i < size; ++i)
        m_properties[i].value()->addSubresourceStyleURLs(urls, sheet);
}

// This is the list of properties we want to copy in the copyBlockProperties() function.
// It is the list of CSS properties that apply specially to block-level elements.
static const int blockProperties[] = {
    CSSPropertyOrphans,
    CSSPropertyOverflow, // This can be also be applied to replaced elements
    CSSPropertyWebkitColumnCount,
    CSSPropertyWebkitColumnGap,
    CSSPropertyWebkitColumnRuleColor,
    CSSPropertyWebkitColumnRuleStyle,
    CSSPropertyWebkitColumnRuleWidth,
    CSSPropertyWebkitColumnBreakBefore,
    CSSPropertyWebkitColumnBreakAfter,
    CSSPropertyWebkitColumnBreakInside,
    CSSPropertyWebkitColumnWidth,
    CSSPropertyPageBreakAfter,
    CSSPropertyPageBreakBefore,
    CSSPropertyPageBreakInside,
    CSSPropertyTextAlign,
    CSSPropertyTextIndent,
    CSSPropertyWidows
};

const unsigned numBlockProperties = WTF_ARRAY_LENGTH(blockProperties);

PassRefPtr<CSSMutableStyleDeclaration> CSSMutableStyleDeclaration::copyBlockProperties() const
{
    return copyPropertiesInSet(blockProperties, numBlockProperties);
}

void CSSMutableStyleDeclaration::removeBlockProperties()
{
    removePropertiesInSet(blockProperties, numBlockProperties);
}

void CSSMutableStyleDeclaration::removePropertiesInSet(const int* set, unsigned length, bool notifyChanged)
{
    ASSERT(!m_iteratorCount);
    
    if (m_properties.isEmpty())
        return;
    
    // FIXME: This is always used with static sets and in that case constructing the hash repeatedly is pretty pointless.
    HashSet<int> toRemove;
    for (unsigned i = 0; i < length; ++i)
        toRemove.add(set[i]);
    
    Vector<CSSProperty, 4> newProperties;
    newProperties.reserveInitialCapacity(m_properties.size());
    
    unsigned size = m_properties.size();
    for (unsigned n = 0; n < size; ++n) {
        const CSSProperty& property = m_properties[n];
        // Not quite sure if the isImportant test is needed but it matches the existing behavior.
        if (!property.isImportant()) {
            if (toRemove.contains(property.id()))
                continue;
        }
        newProperties.append(property);
    }

    bool changed = newProperties.size() != m_properties.size();
    m_properties = newProperties;
    
    if (changed && notifyChanged)
        setNeedsStyleRecalc();
}

PassRefPtr<CSSMutableStyleDeclaration> CSSMutableStyleDeclaration::makeMutable()
{
    return this;
}

PassRefPtr<CSSMutableStyleDeclaration> CSSMutableStyleDeclaration::copy() const
{
    return adoptRef(new CSSMutableStyleDeclaration(0, m_properties));
}

const CSSProperty* CSSMutableStyleDeclaration::findPropertyWithId(int propertyID) const
{    
    for (int n = m_properties.size() - 1 ; n >= 0; --n) {
        if (propertyID == m_properties[n].m_id)
            return &m_properties[n];
    }
    return 0;
}

CSSProperty* CSSMutableStyleDeclaration::findPropertyWithId(int propertyID)
{
    for (int n = m_properties.size() - 1 ; n >= 0; --n) {
        if (propertyID == m_properties[n].m_id)
            return &m_properties[n];
    }
    return 0;
}

} // namespace WebCore
