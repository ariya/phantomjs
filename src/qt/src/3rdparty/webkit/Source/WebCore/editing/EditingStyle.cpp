/*
 * Copyright (C) 2007, 2008, 2009 Apple Computer, Inc.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "EditingStyle.h"

#include "ApplyStyleCommand.h"
#include "CSSComputedStyleDeclaration.h"
#include "CSSMutableStyleDeclaration.h"
#include "CSSParser.h"
#include "CSSStyleSelector.h"
#include "CSSValueKeywords.h"
#include "CSSValueList.h"
#include "Frame.h"
#include "HTMLFontElement.h"
#include "HTMLNames.h"
#include "Node.h"
#include "Position.h"
#include "RenderStyle.h"
#include "SelectionController.h"
#include "StyledElement.h"
#include "htmlediting.h"

namespace WebCore {

// Editing style properties must be preserved during editing operation.
// e.g. when a user inserts a new paragraph, all properties listed here must be copied to the new paragraph.
// FIXME: The current editingStyleProperties contains all inheritableProperties but we may not need to preserve all inheritable properties
static const int editingStyleProperties[] = {
    // CSS inheritable properties
    CSSPropertyBorderCollapse,
    CSSPropertyColor,
    CSSPropertyFontFamily,
    CSSPropertyFontSize,
    CSSPropertyFontStyle,
    CSSPropertyFontVariant,
    CSSPropertyFontWeight,
    CSSPropertyLetterSpacing,
    CSSPropertyLineHeight,
    CSSPropertyOrphans,
    CSSPropertyTextAlign,
    CSSPropertyTextIndent,
    CSSPropertyTextTransform,
    CSSPropertyWhiteSpace,
    CSSPropertyWidows,
    CSSPropertyWordSpacing,
    CSSPropertyWebkitBorderHorizontalSpacing,
    CSSPropertyWebkitBorderVerticalSpacing,
    CSSPropertyWebkitTextDecorationsInEffect,
    CSSPropertyWebkitTextFillColor,
    CSSPropertyWebkitTextSizeAdjust,
    CSSPropertyWebkitTextStrokeColor,
    CSSPropertyWebkitTextStrokeWidth,
};
size_t numEditingStyleProperties = WTF_ARRAY_LENGTH(editingStyleProperties);

static PassRefPtr<CSSMutableStyleDeclaration> copyEditingProperties(CSSStyleDeclaration* style)
{
    return style->copyPropertiesInSet(editingStyleProperties, numEditingStyleProperties);
}

static PassRefPtr<CSSMutableStyleDeclaration> editingStyleFromComputedStyle(PassRefPtr<CSSComputedStyleDeclaration> style)
{
    if (!style)
        return CSSMutableStyleDeclaration::create();
    return copyEditingProperties(style.get());
}

static RefPtr<CSSMutableStyleDeclaration> getPropertiesNotIn(CSSStyleDeclaration* styleWithRedundantProperties, CSSStyleDeclaration* baseStyle);
static RGBA32 getRGBAFontColor(CSSStyleDeclaration*);

class HTMLElementEquivalent {
public:
    static PassOwnPtr<HTMLElementEquivalent> create(CSSPropertyID propertyID, int primitiveValue, const QualifiedName& tagName)
    {
        return adoptPtr(new HTMLElementEquivalent(propertyID, primitiveValue, tagName));
    }

    virtual ~HTMLElementEquivalent() { }
    virtual bool matches(Element* element) const { return !m_tagName || element->hasTagName(*m_tagName); }
    virtual bool hasAttribute() const { return false; }
    bool propertyExistsInStyle(CSSStyleDeclaration* style) const { return style->getPropertyCSSValue(m_propertyID); }
    virtual bool valueIsPresentInStyle(Element*, CSSStyleDeclaration*) const;
    virtual void addToStyle(Element*, EditingStyle*) const;

protected:
    HTMLElementEquivalent(CSSPropertyID);
    HTMLElementEquivalent(CSSPropertyID, const QualifiedName& tagName);
    HTMLElementEquivalent(CSSPropertyID, int primitiveValue, const QualifiedName& tagName);
    const int m_propertyID;
    const RefPtr<CSSPrimitiveValue> m_primitiveValue;
    const QualifiedName* m_tagName; // We can store a pointer because HTML tag names are const global.
};

HTMLElementEquivalent::HTMLElementEquivalent(CSSPropertyID id)
    : m_propertyID(id)
    , m_tagName(0)
{
}

HTMLElementEquivalent::HTMLElementEquivalent(CSSPropertyID id, const QualifiedName& tagName)
    : m_propertyID(id)
    , m_tagName(&tagName)
{
}

HTMLElementEquivalent::HTMLElementEquivalent(CSSPropertyID id, int primitiveValue, const QualifiedName& tagName)
    : m_propertyID(id)
    , m_primitiveValue(CSSPrimitiveValue::createIdentifier(primitiveValue))
    , m_tagName(&tagName)
{
    ASSERT(primitiveValue != CSSValueInvalid);
}

bool HTMLElementEquivalent::valueIsPresentInStyle(Element* element, CSSStyleDeclaration* style) const
{
    RefPtr<CSSValue> value = style->getPropertyCSSValue(m_propertyID);
    return matches(element) && value && value->isPrimitiveValue() && static_cast<CSSPrimitiveValue*>(value.get())->getIdent() == m_primitiveValue->getIdent();
}

void HTMLElementEquivalent::addToStyle(Element*, EditingStyle* style) const
{
    style->setProperty(m_propertyID, m_primitiveValue->cssText());
}

class HTMLTextDecorationEquivalent : public HTMLElementEquivalent {
public:
    static PassOwnPtr<HTMLElementEquivalent> create(int primitiveValue, const QualifiedName& tagName)
    {
        return adoptPtr(new HTMLTextDecorationEquivalent(primitiveValue, tagName));
    }
    virtual bool valueIsPresentInStyle(Element*, CSSStyleDeclaration*) const;

private:
    HTMLTextDecorationEquivalent(int primitiveValue, const QualifiedName& tagName);
};

HTMLTextDecorationEquivalent::HTMLTextDecorationEquivalent(int primitiveValue, const QualifiedName& tagName)
    : HTMLElementEquivalent(CSSPropertyTextDecoration, primitiveValue, tagName)
{
}

bool HTMLTextDecorationEquivalent::valueIsPresentInStyle(Element* element, CSSStyleDeclaration* style) const
{
    RefPtr<CSSValue> styleValue = style->getPropertyCSSValue(m_propertyID);
    return matches(element) && styleValue && styleValue->isValueList() && static_cast<CSSValueList*>(styleValue.get())->hasValue(m_primitiveValue.get());
}

class HTMLAttributeEquivalent : public HTMLElementEquivalent {
public:
    static PassOwnPtr<HTMLAttributeEquivalent> create(CSSPropertyID propertyID, const QualifiedName& tagName, const QualifiedName& attrName)
    {
        return adoptPtr(new HTMLAttributeEquivalent(propertyID, tagName, attrName));
    }
    static PassOwnPtr<HTMLAttributeEquivalent> create(CSSPropertyID propertyID, const QualifiedName& attrName)
    {
        return adoptPtr(new HTMLAttributeEquivalent(propertyID, attrName));
    }

    bool matches(Element* elem) const { return HTMLElementEquivalent::matches(elem) && elem->hasAttribute(m_attrName); }
    virtual bool hasAttribute() const { return true; }
    virtual bool valueIsPresentInStyle(Element*, CSSStyleDeclaration*) const;
    virtual void addToStyle(Element*, EditingStyle*) const;
    virtual PassRefPtr<CSSValue> attributeValueAsCSSValue(Element*) const;
    inline const QualifiedName& attributeName() const { return m_attrName; }

protected:
    HTMLAttributeEquivalent(CSSPropertyID, const QualifiedName& tagName, const QualifiedName& attrName);
    HTMLAttributeEquivalent(CSSPropertyID, const QualifiedName& attrName);
    const QualifiedName& m_attrName; // We can store a reference because HTML attribute names are const global.
};

HTMLAttributeEquivalent::HTMLAttributeEquivalent(CSSPropertyID id, const QualifiedName& tagName, const QualifiedName& attrName)
    : HTMLElementEquivalent(id, tagName)
    , m_attrName(attrName)
{
}

HTMLAttributeEquivalent::HTMLAttributeEquivalent(CSSPropertyID id, const QualifiedName& attrName)
    : HTMLElementEquivalent(id)
    , m_attrName(attrName)
{
}

bool HTMLAttributeEquivalent::valueIsPresentInStyle(Element* element, CSSStyleDeclaration* style) const
{
    RefPtr<CSSValue> value = attributeValueAsCSSValue(element);
    RefPtr<CSSValue> styleValue = style->getPropertyCSSValue(m_propertyID);
    
    // FIXME: This is very inefficient way of comparing values
    // but we can't string compare attribute value and CSS property value.
    return value && styleValue && value->cssText() == styleValue->cssText();
}

void HTMLAttributeEquivalent::addToStyle(Element* element, EditingStyle* style) const
{
    if (RefPtr<CSSValue> value = attributeValueAsCSSValue(element))
        style->setProperty(m_propertyID, value->cssText());
}

PassRefPtr<CSSValue> HTMLAttributeEquivalent::attributeValueAsCSSValue(Element* element) const
{
    ASSERT(element);
    if (!element->hasAttribute(m_attrName))
        return 0;
    
    RefPtr<CSSMutableStyleDeclaration> dummyStyle;
    dummyStyle = CSSMutableStyleDeclaration::create();
    dummyStyle->setProperty(m_propertyID, element->getAttribute(m_attrName));
    return dummyStyle->getPropertyCSSValue(m_propertyID);
}

class HTMLFontSizeEquivalent : public HTMLAttributeEquivalent {
public:
    static PassOwnPtr<HTMLFontSizeEquivalent> create()
    {
        return adoptPtr(new HTMLFontSizeEquivalent());
    }
    virtual PassRefPtr<CSSValue> attributeValueAsCSSValue(Element*) const;

private:
    HTMLFontSizeEquivalent();
};

HTMLFontSizeEquivalent::HTMLFontSizeEquivalent()
    : HTMLAttributeEquivalent(CSSPropertyFontSize, HTMLNames::fontTag, HTMLNames::sizeAttr)
{
}

PassRefPtr<CSSValue> HTMLFontSizeEquivalent::attributeValueAsCSSValue(Element* element) const
{
    ASSERT(element);
    if (!element->hasAttribute(m_attrName))
        return 0;
    int size;
    if (!HTMLFontElement::cssValueFromFontSizeNumber(element->getAttribute(m_attrName), size))
        return 0;
    return CSSPrimitiveValue::createIdentifier(size);
}

float EditingStyle::NoFontDelta = 0.0f;

EditingStyle::EditingStyle()
    : m_shouldUseFixedDefaultFontSize(false)
    , m_fontSizeDelta(NoFontDelta)
{
}

EditingStyle::EditingStyle(Node* node, PropertiesToInclude propertiesToInclude)
    : m_shouldUseFixedDefaultFontSize(false)
    , m_fontSizeDelta(NoFontDelta)
{
    init(node, propertiesToInclude);
}

EditingStyle::EditingStyle(const Position& position, PropertiesToInclude propertiesToInclude)
    : m_shouldUseFixedDefaultFontSize(false)
    , m_fontSizeDelta(NoFontDelta)
{
    init(position.deprecatedNode(), propertiesToInclude);
}

EditingStyle::EditingStyle(const CSSStyleDeclaration* style)
    : m_mutableStyle(style->copy())
    , m_shouldUseFixedDefaultFontSize(false)
    , m_fontSizeDelta(NoFontDelta)
{
    extractFontSizeDelta();
}

EditingStyle::EditingStyle(int propertyID, const String& value)
    : m_mutableStyle(0)
    , m_shouldUseFixedDefaultFontSize(false)
    , m_fontSizeDelta(NoFontDelta)
{
    setProperty(propertyID, value);
}

EditingStyle::~EditingStyle()
{
}

static RGBA32 cssValueToRGBA(CSSValue* colorValue)
{
    if (!colorValue || !colorValue->isPrimitiveValue())
        return Color::transparent;
    
    CSSPrimitiveValue* primitiveColor = static_cast<CSSPrimitiveValue*>(colorValue);
    if (primitiveColor->primitiveType() == CSSPrimitiveValue::CSS_RGBCOLOR)
        return primitiveColor->getRGBA32Value();
    
    RGBA32 rgba = 0;
    CSSParser::parseColor(rgba, colorValue->cssText());
    return rgba;
}

static inline RGBA32 getRGBAFontColor(CSSStyleDeclaration* style)
{
    return cssValueToRGBA(style->getPropertyCSSValue(CSSPropertyColor).get());
}

static inline RGBA32 rgbaBackgroundColorInEffect(Node* node)
{
    return cssValueToRGBA(backgroundColorInEffect(node).get());
}

void EditingStyle::init(Node* node, PropertiesToInclude propertiesToInclude)
{
    if (isTabSpanTextNode(node))
        node = tabSpanNode(node)->parentNode();
    else if (isTabSpanNode(node))
        node = node->parentNode();

    RefPtr<CSSComputedStyleDeclaration> computedStyleAtPosition = computedStyle(node);
    m_mutableStyle = propertiesToInclude == AllProperties && computedStyleAtPosition ? computedStyleAtPosition->copy() : editingStyleFromComputedStyle(computedStyleAtPosition);

    if (propertiesToInclude == InheritablePropertiesAndBackgroundColorInEffect) {
        if (RefPtr<CSSValue> value = backgroundColorInEffect(node))
            m_mutableStyle->setProperty(CSSPropertyBackgroundColor, value->cssText());
    }

    if (node && node->computedStyle()) {
        RenderStyle* renderStyle = node->computedStyle();
        removeTextFillAndStrokeColorsIfNeeded(renderStyle);
        replaceFontSizeByKeywordIfPossible(renderStyle, computedStyleAtPosition.get());
    }

    m_shouldUseFixedDefaultFontSize = computedStyleAtPosition->useFixedFontDefaultSize();
    extractFontSizeDelta();
}

void EditingStyle::removeTextFillAndStrokeColorsIfNeeded(RenderStyle* renderStyle)
{
    // If a node's text fill color is invalid, then its children use 
    // their font-color as their text fill color (they don't
    // inherit it).  Likewise for stroke color.
    ExceptionCode ec = 0;
    if (!renderStyle->textFillColor().isValid())
        m_mutableStyle->removeProperty(CSSPropertyWebkitTextFillColor, ec);
    if (!renderStyle->textStrokeColor().isValid())
        m_mutableStyle->removeProperty(CSSPropertyWebkitTextStrokeColor, ec);
    ASSERT(!ec);
}

void EditingStyle::setProperty(int propertyID, const String& value, bool important)
{
    if (!m_mutableStyle)
        m_mutableStyle = CSSMutableStyleDeclaration::create();

    ExceptionCode ec;
    m_mutableStyle->setProperty(propertyID, value, important, ec);
}

void EditingStyle::replaceFontSizeByKeywordIfPossible(RenderStyle* renderStyle, CSSComputedStyleDeclaration* computedStyle)
{
    ASSERT(renderStyle);
    if (renderStyle->fontDescription().keywordSize())
        m_mutableStyle->setProperty(CSSPropertyFontSize, computedStyle->getFontSizeCSSValuePreferringKeyword()->cssText());
}

void EditingStyle::extractFontSizeDelta()
{
    if (m_mutableStyle->getPropertyCSSValue(CSSPropertyFontSize)) {
        // Explicit font size overrides any delta.
        m_mutableStyle->removeProperty(CSSPropertyWebkitFontSizeDelta);
        return;
    }

    // Get the adjustment amount out of the style.
    RefPtr<CSSValue> value = m_mutableStyle->getPropertyCSSValue(CSSPropertyWebkitFontSizeDelta);
    if (!value || value->cssValueType() != CSSValue::CSS_PRIMITIVE_VALUE)
        return;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value.get());

    // Only PX handled now. If we handle more types in the future, perhaps
    // a switch statement here would be more appropriate.
    if (primitiveValue->primitiveType() != CSSPrimitiveValue::CSS_PX)
        return;

    m_fontSizeDelta = primitiveValue->getFloatValue();
    m_mutableStyle->removeProperty(CSSPropertyWebkitFontSizeDelta);
}

bool EditingStyle::isEmpty() const
{
    return (!m_mutableStyle || m_mutableStyle->isEmpty()) && m_fontSizeDelta == NoFontDelta;
}

bool EditingStyle::textDirection(WritingDirection& writingDirection) const
{
    if (!m_mutableStyle)
        return false;

    RefPtr<CSSValue> unicodeBidi = m_mutableStyle->getPropertyCSSValue(CSSPropertyUnicodeBidi);
    if (!unicodeBidi || !unicodeBidi->isPrimitiveValue())
        return false;

    int unicodeBidiValue = static_cast<CSSPrimitiveValue*>(unicodeBidi.get())->getIdent();
    if (unicodeBidiValue == CSSValueEmbed) {
        RefPtr<CSSValue> direction = m_mutableStyle->getPropertyCSSValue(CSSPropertyDirection);
        if (!direction || !direction->isPrimitiveValue())
            return false;

        writingDirection = static_cast<CSSPrimitiveValue*>(direction.get())->getIdent() == CSSValueLtr ? LeftToRightWritingDirection : RightToLeftWritingDirection;

        return true;
    }

    if (unicodeBidiValue == CSSValueNormal) {
        writingDirection = NaturalWritingDirection;
        return true;
    }

    return false;
}

void EditingStyle::setStyle(PassRefPtr<CSSMutableStyleDeclaration> style)
{
    m_mutableStyle = style;
    // FIXME: We should be able to figure out whether or not font is fixed width for mutable style.
    // We need to check font-family is monospace as in FontDescription but we don't want to duplicate code here.
    m_shouldUseFixedDefaultFontSize = false;
    extractFontSizeDelta();
}

void EditingStyle::overrideWithStyle(const CSSMutableStyleDeclaration* style)
{
    if (!style || !style->length())
        return;
    if (!m_mutableStyle)
        m_mutableStyle = CSSMutableStyleDeclaration::create();
    m_mutableStyle->merge(style);
    extractFontSizeDelta();
}

void EditingStyle::clear()
{
    m_mutableStyle.clear();
    m_shouldUseFixedDefaultFontSize = false;
    m_fontSizeDelta = NoFontDelta;
}

PassRefPtr<EditingStyle> EditingStyle::copy() const
{
    RefPtr<EditingStyle> copy = EditingStyle::create();
    if (m_mutableStyle)
        copy->m_mutableStyle = m_mutableStyle->copy();
    copy->m_shouldUseFixedDefaultFontSize = m_shouldUseFixedDefaultFontSize;
    copy->m_fontSizeDelta = m_fontSizeDelta;
    return copy;
}

PassRefPtr<EditingStyle> EditingStyle::extractAndRemoveBlockProperties()
{
    RefPtr<EditingStyle> blockProperties = EditingStyle::create();
    if (!m_mutableStyle)
        return blockProperties;

    blockProperties->m_mutableStyle = m_mutableStyle->copyBlockProperties();
    m_mutableStyle->removeBlockProperties();

    return blockProperties;
}

PassRefPtr<EditingStyle> EditingStyle::extractAndRemoveTextDirection()
{
    RefPtr<EditingStyle> textDirection = EditingStyle::create();
    textDirection->m_mutableStyle = CSSMutableStyleDeclaration::create();
    textDirection->m_mutableStyle->setProperty(CSSPropertyUnicodeBidi, CSSValueEmbed, m_mutableStyle->getPropertyPriority(CSSPropertyUnicodeBidi));
    textDirection->m_mutableStyle->setProperty(CSSPropertyDirection, m_mutableStyle->getPropertyValue(CSSPropertyDirection),
        m_mutableStyle->getPropertyPriority(CSSPropertyDirection));

    m_mutableStyle->removeProperty(CSSPropertyUnicodeBidi);
    m_mutableStyle->removeProperty(CSSPropertyDirection);

    return textDirection;
}

void EditingStyle::removeBlockProperties()
{
    if (!m_mutableStyle)
        return;

    m_mutableStyle->removeBlockProperties();
}

void EditingStyle::removeStyleAddedByNode(Node* node)
{
    if (!node || !node->parentNode())
        return;
    RefPtr<CSSMutableStyleDeclaration> parentStyle = editingStyleFromComputedStyle(computedStyle(node->parentNode()));
    RefPtr<CSSMutableStyleDeclaration> nodeStyle = editingStyleFromComputedStyle(computedStyle(node));
    parentStyle->diff(nodeStyle.get());
    nodeStyle->diff(m_mutableStyle.get());
}

void EditingStyle::removeStyleConflictingWithStyleOfNode(Node* node)
{
    if (!node || !node->parentNode() || !m_mutableStyle)
        return;
    RefPtr<CSSMutableStyleDeclaration> parentStyle = editingStyleFromComputedStyle(computedStyle(node->parentNode()));
    RefPtr<CSSMutableStyleDeclaration> nodeStyle = editingStyleFromComputedStyle(computedStyle(node));
    parentStyle->diff(nodeStyle.get());

    CSSMutableStyleDeclaration::const_iterator end = nodeStyle->end();
    for (CSSMutableStyleDeclaration::const_iterator it = nodeStyle->begin(); it != end; ++it)
        m_mutableStyle->removeProperty(it->id());
}

void EditingStyle::removeNonEditingProperties()
{
    if (m_mutableStyle)
        m_mutableStyle = copyEditingProperties(m_mutableStyle.get());
}

void EditingStyle::collapseTextDecorationProperties()
{
    if (!m_mutableStyle)
        return;

    RefPtr<CSSValue> textDecorationsInEffect = m_mutableStyle->getPropertyCSSValue(CSSPropertyWebkitTextDecorationsInEffect);
    if (!textDecorationsInEffect)
        return;

    m_mutableStyle->setProperty(CSSPropertyTextDecoration, textDecorationsInEffect->cssText(), m_mutableStyle->getPropertyPriority(CSSPropertyTextDecoration));
    m_mutableStyle->removeProperty(CSSPropertyWebkitTextDecorationsInEffect);
}

// CSS properties that create a visual difference only when applied to text.
static const int textOnlyProperties[] = {
    CSSPropertyTextDecoration,
    CSSPropertyWebkitTextDecorationsInEffect,
    CSSPropertyFontStyle,
    CSSPropertyFontWeight,
    CSSPropertyColor,
};

TriState EditingStyle::triStateOfStyle(CSSStyleDeclaration* styleToCompare, ShouldIgnoreTextOnlyProperties shouldIgnoreTextOnlyProperties) const
{
    // FIXME: take care of background-color in effect
    RefPtr<CSSMutableStyleDeclaration> difference = getPropertiesNotIn(m_mutableStyle.get(), styleToCompare);

    if (shouldIgnoreTextOnlyProperties == IgnoreTextOnlyProperties)
        difference->removePropertiesInSet(textOnlyProperties, WTF_ARRAY_LENGTH(textOnlyProperties));

    if (!difference->length())
        return TrueTriState;
    if (difference->length() == m_mutableStyle->length())
        return FalseTriState;

    return MixedTriState;
}

bool EditingStyle::conflictsWithInlineStyleOfElement(StyledElement* element, EditingStyle* extractedStyle, Vector<CSSPropertyID>* conflictingProperties) const
{
    ASSERT(element);
    ASSERT(!conflictingProperties || conflictingProperties->isEmpty());

    CSSMutableStyleDeclaration* inlineStyle = element->inlineStyleDecl();
    if (!m_mutableStyle || !inlineStyle)
        return false;

    if (!conflictingProperties) {
        CSSMutableStyleDeclaration::const_iterator end = m_mutableStyle->end();
        for (CSSMutableStyleDeclaration::const_iterator it = m_mutableStyle->begin(); it != end; ++it) {
            CSSPropertyID propertyID = static_cast<CSSPropertyID>(it->id());

            // We don't override whitespace property of a tab span because that would collapse the tab into a space.
            if (propertyID == CSSPropertyWhiteSpace && isTabSpanNode(element))
                continue;

            if (inlineStyle->getPropertyCSSValue(propertyID))
                return true;
        }

        return false;
    }

    CSSMutableStyleDeclaration::const_iterator end = m_mutableStyle->end();
    for (CSSMutableStyleDeclaration::const_iterator it = m_mutableStyle->begin(); it != end; ++it) {
        CSSPropertyID propertyID = static_cast<CSSPropertyID>(it->id());
        if ((propertyID == CSSPropertyWhiteSpace && isTabSpanNode(element)) || !inlineStyle->getPropertyCSSValue(propertyID))
            continue;

        if (propertyID == CSSPropertyUnicodeBidi && inlineStyle->getPropertyCSSValue(CSSPropertyDirection)) {
            if (extractedStyle)
                extractedStyle->setProperty(propertyID, inlineStyle->getPropertyValue(propertyID), inlineStyle->getPropertyPriority(propertyID));
            conflictingProperties->append(CSSPropertyDirection);
        }

        conflictingProperties->append(propertyID);
        if (extractedStyle)
            extractedStyle->setProperty(propertyID, inlineStyle->getPropertyValue(propertyID), inlineStyle->getPropertyPriority(propertyID));
    }

    return !conflictingProperties->isEmpty();
}

bool EditingStyle::conflictsWithImplicitStyleOfElement(HTMLElement* element, EditingStyle* extractedStyle, ShouldExtractMatchingStyle shouldExtractMatchingStyle) const
{
    if (!m_mutableStyle)
        return false;

    static const HTMLElementEquivalent* HTMLEquivalents[] = {
        HTMLElementEquivalent::create(CSSPropertyFontWeight, CSSValueBold, HTMLNames::bTag).leakPtr(),
        HTMLElementEquivalent::create(CSSPropertyFontWeight, CSSValueBold, HTMLNames::strongTag).leakPtr(),
        HTMLElementEquivalent::create(CSSPropertyVerticalAlign, CSSValueSub, HTMLNames::subTag).leakPtr(),
        HTMLElementEquivalent::create(CSSPropertyVerticalAlign, CSSValueSuper, HTMLNames::supTag).leakPtr(),
        HTMLElementEquivalent::create(CSSPropertyFontStyle, CSSValueItalic, HTMLNames::iTag).leakPtr(),
        HTMLElementEquivalent::create(CSSPropertyFontStyle, CSSValueItalic, HTMLNames::emTag).leakPtr(),

        HTMLTextDecorationEquivalent::create(CSSValueUnderline, HTMLNames::uTag).leakPtr(),
        HTMLTextDecorationEquivalent::create(CSSValueLineThrough, HTMLNames::sTag).leakPtr(),
        HTMLTextDecorationEquivalent::create(CSSValueLineThrough, HTMLNames::strikeTag).leakPtr(),
    };

    for (size_t i = 0; i < WTF_ARRAY_LENGTH(HTMLEquivalents); ++i) {
        const HTMLElementEquivalent* equivalent = HTMLEquivalents[i];
        if (equivalent->matches(element) && equivalent->propertyExistsInStyle(m_mutableStyle.get())
            && (shouldExtractMatchingStyle == ExtractMatchingStyle || !equivalent->valueIsPresentInStyle(element, m_mutableStyle.get()))) {
            if (extractedStyle)
                equivalent->addToStyle(element, extractedStyle);
            return true;
        }
    }
    return false;
}

static const Vector<OwnPtr<HTMLAttributeEquivalent> >& htmlAttributeEquivalents()
{
    DEFINE_STATIC_LOCAL(Vector<OwnPtr<HTMLAttributeEquivalent> >, HTMLAttributeEquivalents, ());

    if (!HTMLAttributeEquivalents.size()) {
        HTMLAttributeEquivalents.append(HTMLAttributeEquivalent::create(CSSPropertyColor, HTMLNames::fontTag, HTMLNames::colorAttr));
        HTMLAttributeEquivalents.append(HTMLAttributeEquivalent::create(CSSPropertyFontFamily, HTMLNames::fontTag, HTMLNames::faceAttr));
        HTMLAttributeEquivalents.append(HTMLFontSizeEquivalent::create());

        HTMLAttributeEquivalents.append(HTMLAttributeEquivalent::create(CSSPropertyDirection, HTMLNames::dirAttr));
        HTMLAttributeEquivalents.append(HTMLAttributeEquivalent::create(CSSPropertyUnicodeBidi, HTMLNames::dirAttr));
    }

    return HTMLAttributeEquivalents;
}

bool EditingStyle::conflictsWithImplicitStyleOfAttributes(HTMLElement* element) const
{
    ASSERT(element);
    if (!m_mutableStyle)
        return false;

    const Vector<OwnPtr<HTMLAttributeEquivalent> >& HTMLAttributeEquivalents = htmlAttributeEquivalents();
    for (size_t i = 0; i < HTMLAttributeEquivalents.size(); ++i) {
        if (HTMLAttributeEquivalents[i]->matches(element) && HTMLAttributeEquivalents[i]->propertyExistsInStyle(m_mutableStyle.get())
            && !HTMLAttributeEquivalents[i]->valueIsPresentInStyle(element, m_mutableStyle.get()))
            return true;
    }

    return false;
}

bool EditingStyle::extractConflictingImplicitStyleOfAttributes(HTMLElement* element, ShouldPreserveWritingDirection shouldPreserveWritingDirection,
    EditingStyle* extractedStyle, Vector<QualifiedName>& conflictingAttributes, ShouldExtractMatchingStyle shouldExtractMatchingStyle) const
{
    ASSERT(element);
    // HTMLAttributeEquivalent::addToStyle doesn't support unicode-bidi and direction properties
    ASSERT(!extractedStyle || shouldPreserveWritingDirection == PreserveWritingDirection);
    if (!m_mutableStyle)
        return false;

    const Vector<OwnPtr<HTMLAttributeEquivalent> >& HTMLAttributeEquivalents = htmlAttributeEquivalents();
    bool removed = false;
    for (size_t i = 0; i < HTMLAttributeEquivalents.size(); ++i) {
        const HTMLAttributeEquivalent* equivalent = HTMLAttributeEquivalents[i].get();

        // unicode-bidi and direction are pushed down separately so don't push down with other styles.
        if (shouldPreserveWritingDirection == PreserveWritingDirection && equivalent->attributeName() == HTMLNames::dirAttr)
            continue;

        if (!equivalent->matches(element) || !equivalent->propertyExistsInStyle(m_mutableStyle.get())
            || (shouldExtractMatchingStyle == DoNotExtractMatchingStyle && equivalent->valueIsPresentInStyle(element, m_mutableStyle.get())))
            continue;

        if (extractedStyle)
            equivalent->addToStyle(element, extractedStyle);
        conflictingAttributes.append(equivalent->attributeName());
        removed = true;
    }

    return removed;
}

bool EditingStyle::styleIsPresentInComputedStyleOfNode(Node* node) const
{
    return !m_mutableStyle || !getPropertiesNotIn(m_mutableStyle.get(), computedStyle(node).get())->length();
}

void EditingStyle::prepareToApplyAt(const Position& position, ShouldPreserveWritingDirection shouldPreserveWritingDirection)
{
    if (!m_mutableStyle)
        return;

    // ReplaceSelectionCommand::handleStyleSpans() requires that this function only removes the editing style.
    // If this function was modified in the future to delete all redundant properties, then add a boolean value to indicate
    // which one of editingStyleAtPosition or computedStyle is called.
    RefPtr<EditingStyle> style = EditingStyle::create(position, InheritablePropertiesAndBackgroundColorInEffect);

    RefPtr<CSSValue> unicodeBidi;
    RefPtr<CSSValue> direction;
    if (shouldPreserveWritingDirection == PreserveWritingDirection) {
        unicodeBidi = m_mutableStyle->getPropertyCSSValue(CSSPropertyUnicodeBidi);
        direction = m_mutableStyle->getPropertyCSSValue(CSSPropertyDirection);
    }

    style->m_mutableStyle->diff(m_mutableStyle.get());
    if (getRGBAFontColor(m_mutableStyle.get()) == getRGBAFontColor(style->m_mutableStyle.get()))
        m_mutableStyle->removeProperty(CSSPropertyColor);

    if (hasTransparentBackgroundColor(m_mutableStyle.get())
        || cssValueToRGBA(m_mutableStyle->getPropertyCSSValue(CSSPropertyBackgroundColor).get()) == rgbaBackgroundColorInEffect(position.containerNode()))
        m_mutableStyle->removeProperty(CSSPropertyBackgroundColor);

    if (unicodeBidi && unicodeBidi->isPrimitiveValue()) {
        m_mutableStyle->setProperty(CSSPropertyUnicodeBidi, static_cast<CSSPrimitiveValue*>(unicodeBidi.get())->getIdent());
        if (direction && direction->isPrimitiveValue())
            m_mutableStyle->setProperty(CSSPropertyDirection, static_cast<CSSPrimitiveValue*>(direction.get())->getIdent());
    }
}

void EditingStyle::mergeTypingStyle(Document* document)
{
    ASSERT(document);

    RefPtr<EditingStyle> typingStyle = document->frame()->selection()->typingStyle();
    if (!typingStyle || typingStyle == this)
        return;

    mergeStyle(typingStyle->style());
}

void EditingStyle::mergeInlineStyleOfElement(StyledElement* element)
{
    ASSERT(element);
    mergeStyle(element->inlineStyleDecl());
}

void EditingStyle::mergeStyle(CSSMutableStyleDeclaration* style)
{
    if (!style)
        return;

    if (!m_mutableStyle) {
        m_mutableStyle = style->copy();
        return;
    }

    CSSMutableStyleDeclaration::const_iterator end = style->end();
    for (CSSMutableStyleDeclaration::const_iterator it = style->begin(); it != end; ++it) {
        RefPtr<CSSValue> value;
        if ((it->id() == CSSPropertyTextDecoration || it->id() == CSSPropertyWebkitTextDecorationsInEffect) && it->value()->isValueList()) {
            value = m_mutableStyle->getPropertyCSSValue(it->id());
            if (value && !value->isValueList())
                value = 0;
        }

        if (!value) {
            ExceptionCode ec;
            m_mutableStyle->setProperty(it->id(), it->value()->cssText(), it->isImportant(), ec);
            continue;
        }

        CSSValueList* newTextDecorations = static_cast<CSSValueList*>(it->value());
        CSSValueList* textDecorations = static_cast<CSSValueList*>(value.get());

        DEFINE_STATIC_LOCAL(const RefPtr<CSSPrimitiveValue>, underline, (CSSPrimitiveValue::createIdentifier(CSSValueUnderline)));
        DEFINE_STATIC_LOCAL(const RefPtr<CSSPrimitiveValue>, lineThrough, (CSSPrimitiveValue::createIdentifier(CSSValueLineThrough)));

        if (newTextDecorations->hasValue(underline.get()) && !textDecorations->hasValue(underline.get()))
            textDecorations->append(underline.get());

        if (newTextDecorations->hasValue(lineThrough.get()) && !textDecorations->hasValue(lineThrough.get()))
            textDecorations->append(lineThrough.get());
    }
}

static void reconcileTextDecorationProperties(CSSMutableStyleDeclaration* style)
{    
    RefPtr<CSSValue> textDecorationsInEffect = style->getPropertyCSSValue(CSSPropertyWebkitTextDecorationsInEffect);
    RefPtr<CSSValue> textDecoration = style->getPropertyCSSValue(CSSPropertyTextDecoration);
    // We shouldn't have both text-decoration and -webkit-text-decorations-in-effect because that wouldn't make sense.
    ASSERT(!textDecorationsInEffect || !textDecoration);
    if (textDecorationsInEffect) {
        style->setProperty(CSSPropertyTextDecoration, textDecorationsInEffect->cssText());
        style->removeProperty(CSSPropertyWebkitTextDecorationsInEffect);
        textDecoration = textDecorationsInEffect;
    }

    // If text-decoration is set to "none", remove the property because we don't want to add redundant "text-decoration: none".
    if (textDecoration && !textDecoration->isValueList())
        style->removeProperty(CSSPropertyTextDecoration);
}

StyleChange::StyleChange(EditingStyle* style, const Position& position)
    : m_applyBold(false)
    , m_applyItalic(false)
    , m_applyUnderline(false)
    , m_applyLineThrough(false)
    , m_applySubscript(false)
    , m_applySuperscript(false)
{
    Document* document = position.anchorNode() ? position.anchorNode()->document() : 0;
    if (!style || !style->style() || !document || !document->frame())
        return;

    RefPtr<CSSComputedStyleDeclaration> computedStyle = position.computedStyle();
    // FIXME: take care of background-color in effect
    RefPtr<CSSMutableStyleDeclaration> mutableStyle = getPropertiesNotIn(style->style(), computedStyle.get());

    reconcileTextDecorationProperties(mutableStyle.get());
    if (!document->frame()->editor()->shouldStyleWithCSS())
        extractTextStyles(document, mutableStyle.get(), computedStyle->useFixedFontDefaultSize());

    // Changing the whitespace style in a tab span would collapse the tab into a space.
    if (isTabSpanTextNode(position.deprecatedNode()) || isTabSpanNode((position.deprecatedNode())))
        mutableStyle->removeProperty(CSSPropertyWhiteSpace);

    // If unicode-bidi is present in mutableStyle and direction is not, then add direction to mutableStyle.
    // FIXME: Shouldn't this be done in getPropertiesNotIn?
    if (mutableStyle->getPropertyCSSValue(CSSPropertyUnicodeBidi) && !style->style()->getPropertyCSSValue(CSSPropertyDirection))
        mutableStyle->setProperty(CSSPropertyDirection, style->style()->getPropertyValue(CSSPropertyDirection));

    // Save the result for later
    m_cssStyle = mutableStyle->cssText().stripWhiteSpace();
}

static void setTextDecorationProperty(CSSMutableStyleDeclaration* style, const CSSValueList* newTextDecoration, int propertyID)
{
    if (newTextDecoration->length())
        style->setProperty(propertyID, newTextDecoration->cssText(), style->getPropertyPriority(propertyID));
    else {
        // text-decoration: none is redundant since it does not remove any text decorations.
        ASSERT(!style->getPropertyPriority(propertyID));
        style->removeProperty(propertyID);
    }
}

void StyleChange::extractTextStyles(Document* document, CSSMutableStyleDeclaration* style, bool shouldUseFixedFontDefaultSize)
{
    ASSERT(style);

    if (getIdentifierValue(style, CSSPropertyFontWeight) == CSSValueBold) {
        style->removeProperty(CSSPropertyFontWeight);
        m_applyBold = true;
    }

    int fontStyle = getIdentifierValue(style, CSSPropertyFontStyle);
    if (fontStyle == CSSValueItalic || fontStyle == CSSValueOblique) {
        style->removeProperty(CSSPropertyFontStyle);
        m_applyItalic = true;
    }

    // Assuming reconcileTextDecorationProperties has been called, there should not be -webkit-text-decorations-in-effect
    // Furthermore, text-decoration: none has been trimmed so that text-decoration property is always a CSSValueList.
    RefPtr<CSSValue> textDecoration = style->getPropertyCSSValue(CSSPropertyTextDecoration);
    if (textDecoration && textDecoration->isValueList()) {
        DEFINE_STATIC_LOCAL(RefPtr<CSSPrimitiveValue>, underline, (CSSPrimitiveValue::createIdentifier(CSSValueUnderline)));
        DEFINE_STATIC_LOCAL(RefPtr<CSSPrimitiveValue>, lineThrough, (CSSPrimitiveValue::createIdentifier(CSSValueLineThrough)));

        RefPtr<CSSValueList> newTextDecoration = static_cast<CSSValueList*>(textDecoration.get())->copy();
        if (newTextDecoration->removeAll(underline.get()))
            m_applyUnderline = true;
        if (newTextDecoration->removeAll(lineThrough.get()))
            m_applyLineThrough = true;

        // If trimTextDecorations, delete underline and line-through
        setTextDecorationProperty(style, newTextDecoration.get(), CSSPropertyTextDecoration);
    }

    int verticalAlign = getIdentifierValue(style, CSSPropertyVerticalAlign);
    switch (verticalAlign) {
    case CSSValueSub:
        style->removeProperty(CSSPropertyVerticalAlign);
        m_applySubscript = true;
        break;
    case CSSValueSuper:
        style->removeProperty(CSSPropertyVerticalAlign);
        m_applySuperscript = true;
        break;
    }

    if (style->getPropertyCSSValue(CSSPropertyColor)) {
        m_applyFontColor = Color(getRGBAFontColor(style)).serialized();
        style->removeProperty(CSSPropertyColor);
    }

    m_applyFontFace = style->getPropertyValue(CSSPropertyFontFamily);
    style->removeProperty(CSSPropertyFontFamily);

    if (RefPtr<CSSValue> fontSize = style->getPropertyCSSValue(CSSPropertyFontSize)) {
        if (!fontSize->isPrimitiveValue())
            style->removeProperty(CSSPropertyFontSize); // Can't make sense of the number. Put no font size.
        else if (int legacyFontSize = legacyFontSizeFromCSSValue(document, static_cast<CSSPrimitiveValue*>(fontSize.get()),
                shouldUseFixedFontDefaultSize, UseLegacyFontSizeOnlyIfPixelValuesMatch)) {
            m_applyFontSize = String::number(legacyFontSize);
            style->removeProperty(CSSPropertyFontSize);
        }
    }
}

static void diffTextDecorations(CSSMutableStyleDeclaration* style, int propertID, CSSValue* refTextDecoration)
{
    RefPtr<CSSValue> textDecoration = style->getPropertyCSSValue(propertID);
    if (!textDecoration || !textDecoration->isValueList() || !refTextDecoration || !refTextDecoration->isValueList())
        return;

    RefPtr<CSSValueList> newTextDecoration = static_cast<CSSValueList*>(textDecoration.get())->copy();
    CSSValueList* valuesInRefTextDecoration = static_cast<CSSValueList*>(refTextDecoration);

    for (size_t i = 0; i < valuesInRefTextDecoration->length(); i++)
        newTextDecoration->removeAll(valuesInRefTextDecoration->item(i));

    setTextDecorationProperty(style, newTextDecoration.get(), propertID);
}

static bool fontWeightIsBold(CSSStyleDeclaration* style)
{
    ASSERT(style);
    RefPtr<CSSValue> fontWeight = style->getPropertyCSSValue(CSSPropertyFontWeight);

    if (!fontWeight)
        return false;
    if (!fontWeight->isPrimitiveValue())
        return false;

    // Because b tag can only bold text, there are only two states in plain html: bold and not bold.
    // Collapse all other values to either one of these two states for editing purposes.
    switch (static_cast<CSSPrimitiveValue*>(fontWeight.get())->getIdent()) {
        case CSSValue100:
        case CSSValue200:
        case CSSValue300:
        case CSSValue400:
        case CSSValue500:
        case CSSValueNormal:
            return false;
        case CSSValueBold:
        case CSSValue600:
        case CSSValue700:
        case CSSValue800:
        case CSSValue900:
            return true;
    }

    ASSERT_NOT_REACHED(); // For CSSValueBolder and CSSValueLighter
    return false; // Make compiler happy
}

static int getTextAlignment(CSSStyleDeclaration* style)
{
    int textAlign = getIdentifierValue(style, CSSPropertyTextAlign);
    switch (textAlign) {
    case CSSValueCenter:
    case CSSValueWebkitCenter:
        return CSSValueCenter;
    case CSSValueJustify:
        return CSSValueJustify;
    case CSSValueLeft:
    case CSSValueWebkitLeft:
        return CSSValueLeft;
    case CSSValueRight:
    case CSSValueWebkitRight:
        return CSSValueRight;
    }
    return CSSValueInvalid;
}

RefPtr<CSSMutableStyleDeclaration> getPropertiesNotIn(CSSStyleDeclaration* styleWithRedundantProperties, CSSStyleDeclaration* baseStyle)
{
    ASSERT(styleWithRedundantProperties);
    ASSERT(baseStyle);
    RefPtr<CSSMutableStyleDeclaration> result = styleWithRedundantProperties->copy();

    baseStyle->diff(result.get());

    RefPtr<CSSValue> baseTextDecorationsInEffect = baseStyle->getPropertyCSSValue(CSSPropertyWebkitTextDecorationsInEffect);
    diffTextDecorations(result.get(), CSSPropertyTextDecoration, baseTextDecorationsInEffect.get());
    diffTextDecorations(result.get(), CSSPropertyWebkitTextDecorationsInEffect, baseTextDecorationsInEffect.get());

    if (fontWeightIsBold(result.get()) == fontWeightIsBold(baseStyle))
        result->removeProperty(CSSPropertyFontWeight);

    if (getRGBAFontColor(result.get()) == getRGBAFontColor(baseStyle))
        result->removeProperty(CSSPropertyColor);

    if (getTextAlignment(result.get()) == getTextAlignment(baseStyle))
        result->removeProperty(CSSPropertyTextAlign);

    return result;
}

int getIdentifierValue(CSSStyleDeclaration* style, int propertyID)
{
    if (!style)
        return 0;

    RefPtr<CSSValue> value = style->getPropertyCSSValue(propertyID);
    if (!value || !value->isPrimitiveValue())
        return 0;

    return static_cast<CSSPrimitiveValue*>(value.get())->getIdent();
}

static bool isCSSValueLength(CSSPrimitiveValue* value)
{
    return value->primitiveType() >= CSSPrimitiveValue::CSS_PX && value->primitiveType() <= CSSPrimitiveValue::CSS_PC;
}

int legacyFontSizeFromCSSValue(Document* document, CSSPrimitiveValue* value, bool shouldUseFixedFontDefaultSize, LegacyFontSizeMode mode)
{
    if (isCSSValueLength(value)) {
        int pixelFontSize = value->getIntValue(CSSPrimitiveValue::CSS_PX);
        int legacyFontSize = CSSStyleSelector::legacyFontSize(document, pixelFontSize, shouldUseFixedFontDefaultSize);
        // Use legacy font size only if pixel value matches exactly to that of legacy font size.
        int cssPrimitiveEquivalent = legacyFontSize - 1 + CSSValueXSmall;
        if (mode == AlwaysUseLegacyFontSize || CSSStyleSelector::fontSizeForKeyword(document, cssPrimitiveEquivalent, shouldUseFixedFontDefaultSize) == pixelFontSize)
            return legacyFontSize;

        return 0;
    }

    if (CSSValueXSmall <= value->getIdent() && value->getIdent() <= CSSValueWebkitXxxLarge)
        return value->getIdent() - CSSValueXSmall + 1;

    return 0;
}

bool hasTransparentBackgroundColor(CSSStyleDeclaration* style)
{
    RefPtr<CSSValue> cssValue = style->getPropertyCSSValue(CSSPropertyBackgroundColor);
    if (!cssValue)
        return true;
    
    if (!cssValue->isPrimitiveValue())
        return false;
    CSSPrimitiveValue* value = static_cast<CSSPrimitiveValue*>(cssValue.get());
    
    if (value->primitiveType() == CSSPrimitiveValue::CSS_RGBCOLOR)
        return !alphaChannel(value->getRGBA32Value());
    
    return value->getIdent() == CSSValueTransparent;
}

PassRefPtr<CSSValue> backgroundColorInEffect(Node* node)
{
    for (Node* ancestor = node; ancestor; ancestor = ancestor->parentNode()) {
        RefPtr<CSSComputedStyleDeclaration> ancestorStyle = computedStyle(ancestor);
        if (!hasTransparentBackgroundColor(ancestorStyle.get()))
            return ancestorStyle->getPropertyCSSValue(CSSPropertyBackgroundColor);
    }
    return 0;
}

}
