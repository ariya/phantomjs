/*
 * Copyright (C) 2007, 2008, 2009, 2013 Apple Computer, Inc.
 * Copyright (C) 2010, 2011 Google Inc. All rights reserved.
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
#include "CSSParser.h"
#include "CSSRuleList.h"
#include "CSSStyleRule.h"
#include "CSSValueKeywords.h"
#include "CSSValueList.h"
#include "Editor.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "HTMLFontElement.h"
#include "HTMLInterchange.h"
#include "HTMLNames.h"
#include "Node.h"
#include "NodeTraversal.h"
#include "Position.h"
#include "QualifiedName.h"
#include "RenderStyle.h"
#include "StylePropertySet.h"
#include "StyleResolver.h"
#include "StyleRule.h"
#include "StyledElement.h"
#include "VisibleUnits.h"
#include "htmlediting.h"

namespace WebCore {

// Editing style properties must be preserved during editing operation.
// e.g. when a user inserts a new paragraph, all properties listed here must be copied to the new paragraph.
static const CSSPropertyID editingProperties[] = {
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
    CSSPropertyWebkitTextDecorationsInEffect,
    CSSPropertyWebkitTextFillColor,
    CSSPropertyWebkitTextStrokeColor,
    CSSPropertyWebkitTextStrokeWidth,

    // Non-inheritable properties
    CSSPropertyBackgroundColor,
    CSSPropertyTextDecoration,
};

const unsigned numAllEditingProperties = WTF_ARRAY_LENGTH(editingProperties);
const unsigned numInheritableEditingProperties = numAllEditingProperties - 2;

enum EditingPropertiesToInclude { OnlyInheritableEditingProperties, AllEditingProperties };
template <class StyleDeclarationType>
static PassRefPtr<MutableStylePropertySet> copyEditingProperties(StyleDeclarationType* style, EditingPropertiesToInclude type)
{
    if (type == AllEditingProperties)
        return style->copyPropertiesInSet(editingProperties, numAllEditingProperties);
    return style->copyPropertiesInSet(editingProperties, numInheritableEditingProperties);
}

static inline bool isEditingProperty(int id)
{
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(editingProperties); ++i) {
        if (editingProperties[i] == id)
            return true;
    }
    return false;
}

static PassRefPtr<MutableStylePropertySet> copyPropertiesFromComputedStyle(ComputedStyleExtractor& computedStyle, EditingStyle::PropertiesToInclude propertiesToInclude)
{
    switch (propertiesToInclude) {
    case EditingStyle::AllProperties:
        return computedStyle.copyProperties();
    case EditingStyle::OnlyEditingInheritableProperties:
        return copyEditingProperties(&computedStyle, OnlyInheritableEditingProperties);
    case EditingStyle::EditingPropertiesInEffect:
        return copyEditingProperties(&computedStyle, AllEditingProperties);
    }
    ASSERT_NOT_REACHED();
    return 0;
}

static PassRefPtr<MutableStylePropertySet> copyPropertiesFromComputedStyle(Node* node, EditingStyle::PropertiesToInclude propertiesToInclude)
{
    ComputedStyleExtractor computedStyle(node);
    return copyPropertiesFromComputedStyle(computedStyle, propertiesToInclude);
}

static PassRefPtr<CSSValue> extractPropertyValue(const StylePropertySet* style, CSSPropertyID propertyID)
{
    return style ? style->getPropertyCSSValue(propertyID) : PassRefPtr<CSSValue>();
}

static PassRefPtr<CSSValue> extractPropertyValue(ComputedStyleExtractor* computedStyle, CSSPropertyID propertyID)
{
    return computedStyle->propertyValue(propertyID);
}

template<typename T>
int identifierForStyleProperty(T* style, CSSPropertyID propertyID)
{
    RefPtr<CSSValue> value = extractPropertyValue(style, propertyID);
    if (!value || !value->isPrimitiveValue())
        return 0;
    return static_cast<CSSPrimitiveValue*>(value.get())->getValueID();
}

template<typename T> PassRefPtr<MutableStylePropertySet> getPropertiesNotIn(StylePropertySet* styleWithRedundantProperties, T* baseStyle);
enum LegacyFontSizeMode { AlwaysUseLegacyFontSize, UseLegacyFontSizeOnlyIfPixelValuesMatch };
static int legacyFontSizeFromCSSValue(Document*, CSSPrimitiveValue*, bool shouldUseFixedFontDefaultSize, LegacyFontSizeMode);
static bool isTransparentColorValue(CSSValue*);
static bool hasTransparentBackgroundColor(StylePropertySet*);
static PassRefPtr<CSSValue> backgroundColorInEffect(Node*);

class HTMLElementEquivalent {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<HTMLElementEquivalent> create(CSSPropertyID propertyID, CSSValueID primitiveValue, const QualifiedName& tagName)
    {
        return adoptPtr(new HTMLElementEquivalent(propertyID, primitiveValue, tagName));
    }

    virtual ~HTMLElementEquivalent() { }
    virtual bool matches(const Element* element) const { return !m_tagName || element->hasTagName(*m_tagName); }
    virtual bool hasAttribute() const { return false; }
    virtual bool propertyExistsInStyle(const StylePropertySet* style) const { return style->getPropertyCSSValue(m_propertyID); }
    virtual bool valueIsPresentInStyle(Element*, StylePropertySet*) const;
    virtual void addToStyle(Element*, EditingStyle*) const;

protected:
    HTMLElementEquivalent(CSSPropertyID);
    HTMLElementEquivalent(CSSPropertyID, const QualifiedName& tagName);
    HTMLElementEquivalent(CSSPropertyID, CSSValueID primitiveValue, const QualifiedName& tagName);
    const CSSPropertyID m_propertyID;
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

HTMLElementEquivalent::HTMLElementEquivalent(CSSPropertyID id, CSSValueID primitiveValue, const QualifiedName& tagName)
    : m_propertyID(id)
    , m_primitiveValue(CSSPrimitiveValue::createIdentifier(primitiveValue))
    , m_tagName(&tagName)
{
    ASSERT(primitiveValue != CSSValueInvalid);
}

bool HTMLElementEquivalent::valueIsPresentInStyle(Element* element, StylePropertySet* style) const
{
    RefPtr<CSSValue> value = style->getPropertyCSSValue(m_propertyID);
    return matches(element) && value && value->isPrimitiveValue() && static_cast<CSSPrimitiveValue*>(value.get())->getValueID() == m_primitiveValue->getValueID();
}

void HTMLElementEquivalent::addToStyle(Element*, EditingStyle* style) const
{
    style->setProperty(m_propertyID, m_primitiveValue->cssText());
}

class HTMLTextDecorationEquivalent : public HTMLElementEquivalent {
public:
    static PassOwnPtr<HTMLElementEquivalent> create(CSSValueID primitiveValue, const QualifiedName& tagName)
    {
        return adoptPtr(new HTMLTextDecorationEquivalent(primitiveValue, tagName));
    }
    virtual bool propertyExistsInStyle(const StylePropertySet*) const;
    virtual bool valueIsPresentInStyle(Element*, StylePropertySet*) const;

private:
    HTMLTextDecorationEquivalent(CSSValueID primitiveValue, const QualifiedName& tagName);
};

HTMLTextDecorationEquivalent::HTMLTextDecorationEquivalent(CSSValueID primitiveValue, const QualifiedName& tagName)
    : HTMLElementEquivalent(CSSPropertyTextDecoration, primitiveValue, tagName)
    // m_propertyID is used in HTMLElementEquivalent::addToStyle
{
}

bool HTMLTextDecorationEquivalent::propertyExistsInStyle(const StylePropertySet* style) const
{
    return style->getPropertyCSSValue(CSSPropertyWebkitTextDecorationsInEffect) || style->getPropertyCSSValue(CSSPropertyTextDecoration);
}

bool HTMLTextDecorationEquivalent::valueIsPresentInStyle(Element* element, StylePropertySet* style) const
{
    RefPtr<CSSValue> styleValue = style->getPropertyCSSValue(CSSPropertyWebkitTextDecorationsInEffect);
    if (!styleValue)
        styleValue = style->getPropertyCSSValue(CSSPropertyTextDecoration);
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

    bool matches(const Element* elem) const { return HTMLElementEquivalent::matches(elem) && elem->hasAttribute(m_attrName); }
    virtual bool hasAttribute() const { return true; }
    virtual bool valueIsPresentInStyle(Element*, StylePropertySet*) const;
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

bool HTMLAttributeEquivalent::valueIsPresentInStyle(Element* element, StylePropertySet* style) const
{
    RefPtr<CSSValue> value = attributeValueAsCSSValue(element);
    RefPtr<CSSValue> styleValue = style->getPropertyCSSValue(m_propertyID);
    
    return compareCSSValuePtr(value, styleValue);
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
    
    RefPtr<MutableStylePropertySet> dummyStyle;
    dummyStyle = MutableStylePropertySet::create();
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
    CSSValueID size;
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

EditingStyle::EditingStyle(const StylePropertySet* style)
    : m_mutableStyle(style ? style->mutableCopy() : 0)
    , m_shouldUseFixedDefaultFontSize(false)
    , m_fontSizeDelta(NoFontDelta)
{
    extractFontSizeDelta();
}

EditingStyle::EditingStyle(CSSPropertyID propertyID, const String& value)
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
    if (primitiveColor->isRGBColor())
        return primitiveColor->getRGBA32Value();
    
    RGBA32 rgba = 0;
    CSSParser::parseColor(rgba, colorValue->cssText());
    return rgba;
}

template<typename T>
static inline RGBA32 textColorFromStyle(T* style)
{
    return cssValueToRGBA(extractPropertyValue(style, CSSPropertyColor).get());
}

template<typename T>
static inline RGBA32 backgroundColorFromStyle(T* style)
{
    return cssValueToRGBA(extractPropertyValue(style, CSSPropertyBackgroundColor).get());
}

static inline RGBA32 rgbaBackgroundColorInEffect(Node* node)
{
    return cssValueToRGBA(backgroundColorInEffect(node).get());
}

static int textAlignResolvingStartAndEnd(int textAlign, int direction)
{
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
    case CSSValueStart:
        return direction != CSSValueRtl ? CSSValueLeft : CSSValueRight;
    case CSSValueEnd:
        return direction == CSSValueRtl ? CSSValueRight : CSSValueLeft;
    }
    return CSSValueInvalid;
}

template<typename T>
static int textAlignResolvingStartAndEnd(T* style)
{
    return textAlignResolvingStartAndEnd(identifierForStyleProperty(style, CSSPropertyTextAlign), identifierForStyleProperty(style, CSSPropertyDirection));
}

void EditingStyle::init(Node* node, PropertiesToInclude propertiesToInclude)
{
    if (isTabSpanTextNode(node))
        node = tabSpanNode(node)->parentNode();
    else if (isTabSpanNode(node))
        node = node->parentNode();

    ComputedStyleExtractor computedStyleAtPosition(node);
    // FIXME: It's strange to not set background-color and text-decoration when propertiesToInclude is EditingPropertiesInEffect.
    // However editing/selection/contains-boundaries.html fails without this ternary.
    m_mutableStyle = copyPropertiesFromComputedStyle(computedStyleAtPosition,
        propertiesToInclude == EditingPropertiesInEffect ? OnlyEditingInheritableProperties : propertiesToInclude);

    if (propertiesToInclude == EditingPropertiesInEffect) {
        if (RefPtr<CSSValue> value = backgroundColorInEffect(node))
            m_mutableStyle->setProperty(CSSPropertyBackgroundColor, value->cssText());
        if (RefPtr<CSSValue> value = computedStyleAtPosition.propertyValue(CSSPropertyWebkitTextDecorationsInEffect))
            m_mutableStyle->setProperty(CSSPropertyTextDecoration, value->cssText());
    }

    if (node && node->computedStyle()) {
        RenderStyle* renderStyle = node->computedStyle();
        removeTextFillAndStrokeColorsIfNeeded(renderStyle);
        if (renderStyle->fontDescription().keywordSize())
            m_mutableStyle->setProperty(CSSPropertyFontSize, computedStyleAtPosition.getFontSizeCSSValuePreferringKeyword()->cssText());
    }

    m_shouldUseFixedDefaultFontSize = computedStyleAtPosition.useFixedFontDefaultSize();
    extractFontSizeDelta();
}

void EditingStyle::removeTextFillAndStrokeColorsIfNeeded(RenderStyle* renderStyle)
{
    // If a node's text fill color is invalid, then its children use 
    // their font-color as their text fill color (they don't
    // inherit it).  Likewise for stroke color.
    if (!renderStyle->textFillColor().isValid())
        m_mutableStyle->removeProperty(CSSPropertyWebkitTextFillColor);
    if (!renderStyle->textStrokeColor().isValid())
        m_mutableStyle->removeProperty(CSSPropertyWebkitTextStrokeColor);
}

void EditingStyle::setProperty(CSSPropertyID propertyID, const String& value, bool important)
{
    if (!m_mutableStyle)
        m_mutableStyle = MutableStylePropertySet::create();

    m_mutableStyle->setProperty(propertyID, value, important);
}

void EditingStyle::extractFontSizeDelta()
{
    if (!m_mutableStyle)
        return;

    if (m_mutableStyle->getPropertyCSSValue(CSSPropertyFontSize)) {
        // Explicit font size overrides any delta.
        m_mutableStyle->removeProperty(CSSPropertyWebkitFontSizeDelta);
        return;
    }

    // Get the adjustment amount out of the style.
    RefPtr<CSSValue> value = m_mutableStyle->getPropertyCSSValue(CSSPropertyWebkitFontSizeDelta);
    if (!value || !value->isPrimitiveValue())
        return;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value.get());

    // Only PX handled now. If we handle more types in the future, perhaps
    // a switch statement here would be more appropriate.
    if (!primitiveValue->isPx())
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

    CSSValueID unicodeBidiValue = static_cast<CSSPrimitiveValue*>(unicodeBidi.get())->getValueID();
    if (unicodeBidiValue == CSSValueEmbed) {
        RefPtr<CSSValue> direction = m_mutableStyle->getPropertyCSSValue(CSSPropertyDirection);
        if (!direction || !direction->isPrimitiveValue())
            return false;

        writingDirection = static_cast<CSSPrimitiveValue*>(direction.get())->getValueID() == CSSValueLtr ? LeftToRightWritingDirection : RightToLeftWritingDirection;

        return true;
    }

    if (unicodeBidiValue == CSSValueNormal) {
        writingDirection = NaturalWritingDirection;
        return true;
    }

    return false;
}

void EditingStyle::setStyle(PassRefPtr<MutableStylePropertySet> style)
{
    m_mutableStyle = style;
    // FIXME: We should be able to figure out whether or not font is fixed width for mutable style.
    // We need to check font-family is monospace as in FontDescription but we don't want to duplicate code here.
    m_shouldUseFixedDefaultFontSize = false;
    extractFontSizeDelta();
}

void EditingStyle::overrideWithStyle(const StylePropertySet* style)
{
    return mergeStyle(style, OverrideValues);
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
        copy->m_mutableStyle = m_mutableStyle->mutableCopy();
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
    textDirection->m_mutableStyle = MutableStylePropertySet::create();
    textDirection->m_mutableStyle->setProperty(CSSPropertyUnicodeBidi, CSSValueEmbed, m_mutableStyle->propertyIsImportant(CSSPropertyUnicodeBidi));
    textDirection->m_mutableStyle->setProperty(CSSPropertyDirection, m_mutableStyle->getPropertyValue(CSSPropertyDirection),
        m_mutableStyle->propertyIsImportant(CSSPropertyDirection));

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
    RefPtr<MutableStylePropertySet> parentStyle = copyPropertiesFromComputedStyle(node->parentNode(), EditingPropertiesInEffect);
    RefPtr<MutableStylePropertySet> nodeStyle = copyPropertiesFromComputedStyle(node, EditingPropertiesInEffect);
    nodeStyle->removeEquivalentProperties(parentStyle.get());
    m_mutableStyle->removeEquivalentProperties(nodeStyle.get());
}

void EditingStyle::removeStyleConflictingWithStyleOfNode(Node* node)
{
    if (!node || !node->parentNode() || !m_mutableStyle)
        return;

    RefPtr<MutableStylePropertySet> parentStyle = copyPropertiesFromComputedStyle(node->parentNode(), EditingPropertiesInEffect);
    RefPtr<MutableStylePropertySet> nodeStyle = copyPropertiesFromComputedStyle(node, EditingPropertiesInEffect);
    nodeStyle->removeEquivalentProperties(parentStyle.get());

    unsigned propertyCount = nodeStyle->propertyCount();
    for (unsigned i = 0; i < propertyCount; ++i)
        m_mutableStyle->removeProperty(nodeStyle->propertyAt(i).id());
}

void EditingStyle::collapseTextDecorationProperties()
{
    if (!m_mutableStyle)
        return;

    RefPtr<CSSValue> textDecorationsInEffect = m_mutableStyle->getPropertyCSSValue(CSSPropertyWebkitTextDecorationsInEffect);
    if (!textDecorationsInEffect)
        return;

    if (textDecorationsInEffect->isValueList())
        m_mutableStyle->setProperty(CSSPropertyTextDecoration, textDecorationsInEffect->cssText(), m_mutableStyle->propertyIsImportant(CSSPropertyTextDecoration));
    else
        m_mutableStyle->removeProperty(CSSPropertyTextDecoration);
    m_mutableStyle->removeProperty(CSSPropertyWebkitTextDecorationsInEffect);
}

// CSS properties that create a visual difference only when applied to text.
static const CSSPropertyID textOnlyProperties[] = {
    CSSPropertyTextDecoration,
    CSSPropertyWebkitTextDecorationsInEffect,
    CSSPropertyFontStyle,
    CSSPropertyFontWeight,
    CSSPropertyColor,
};

TriState EditingStyle::triStateOfStyle(EditingStyle* style) const
{
    if (!style || !style->m_mutableStyle)
        return FalseTriState;
    return triStateOfStyle(style->m_mutableStyle.get(), DoNotIgnoreTextOnlyProperties);
}

template<typename T>
TriState EditingStyle::triStateOfStyle(T* styleToCompare, ShouldIgnoreTextOnlyProperties shouldIgnoreTextOnlyProperties) const
{
    RefPtr<MutableStylePropertySet> difference = getPropertiesNotIn(m_mutableStyle.get(), styleToCompare);

    if (shouldIgnoreTextOnlyProperties == IgnoreTextOnlyProperties)
        difference->removePropertiesInSet(textOnlyProperties, WTF_ARRAY_LENGTH(textOnlyProperties));

    if (difference->isEmpty())
        return TrueTriState;
    if (difference->propertyCount() == m_mutableStyle->propertyCount())
        return FalseTriState;

    return MixedTriState;
}

TriState EditingStyle::triStateOfStyle(const VisibleSelection& selection) const
{
    if (!selection.isCaretOrRange())
        return FalseTriState;

    if (selection.isCaret())
        return triStateOfStyle(EditingStyle::styleAtSelectionStart(selection).get());

    TriState state = FalseTriState;
    bool nodeIsStart = true;
    for (Node* node = selection.start().deprecatedNode(); node; node = NodeTraversal::next(node)) {
        if (node->renderer() && node->rendererIsEditable()) {
            ComputedStyleExtractor computedStyle(node);
            TriState nodeState = triStateOfStyle(&computedStyle, node->isTextNode() ? EditingStyle::DoNotIgnoreTextOnlyProperties : EditingStyle::IgnoreTextOnlyProperties);
            if (nodeIsStart) {
                state = nodeState;
                nodeIsStart = false;
            } else if (state != nodeState && node->isTextNode()) {
                state = MixedTriState;
                break;
            }
        }

        if (node == selection.end().deprecatedNode())
            break;
    }

    return state;
}

bool EditingStyle::conflictsWithInlineStyleOfElement(StyledElement* element, EditingStyle* extractedStyle, Vector<CSSPropertyID>* conflictingProperties) const
{
    ASSERT(element);
    ASSERT(!conflictingProperties || conflictingProperties->isEmpty());

    const StylePropertySet* inlineStyle = element->inlineStyle();
    if (!m_mutableStyle || !inlineStyle)
        return false;

    unsigned propertyCount = m_mutableStyle->propertyCount();
    for (unsigned i = 0; i < propertyCount; ++i) {
        CSSPropertyID propertyID = m_mutableStyle->propertyAt(i).id();

        // We don't override whitespace property of a tab span because that would collapse the tab into a space.
        if (propertyID == CSSPropertyWhiteSpace && isTabSpanNode(element))
            continue;

        if (propertyID == CSSPropertyWebkitTextDecorationsInEffect && inlineStyle->getPropertyCSSValue(CSSPropertyTextDecoration)) {
            if (!conflictingProperties)
                return true;
            conflictingProperties->append(CSSPropertyTextDecoration);
            if (extractedStyle)
                extractedStyle->setProperty(CSSPropertyTextDecoration, inlineStyle->getPropertyValue(CSSPropertyTextDecoration), inlineStyle->propertyIsImportant(CSSPropertyTextDecoration));
            continue;
        }

        if (!inlineStyle->getPropertyCSSValue(propertyID))
            continue;

        if (propertyID == CSSPropertyUnicodeBidi && inlineStyle->getPropertyCSSValue(CSSPropertyDirection)) {
            if (!conflictingProperties)
                return true;
            conflictingProperties->append(CSSPropertyDirection);
            if (extractedStyle)
                extractedStyle->setProperty(propertyID, inlineStyle->getPropertyValue(propertyID), inlineStyle->propertyIsImportant(propertyID));
        }

        if (!conflictingProperties)
            return true;

        conflictingProperties->append(propertyID);

        if (extractedStyle)
            extractedStyle->setProperty(propertyID, inlineStyle->getPropertyValue(propertyID), inlineStyle->propertyIsImportant(propertyID));
    }

    return conflictingProperties && !conflictingProperties->isEmpty();
}

static const Vector<OwnPtr<HTMLElementEquivalent> >& htmlElementEquivalents()
{
    DEFINE_STATIC_LOCAL(Vector<OwnPtr<HTMLElementEquivalent> >, HTMLElementEquivalents, ());

    if (!HTMLElementEquivalents.size()) {
        HTMLElementEquivalents.append(HTMLElementEquivalent::create(CSSPropertyFontWeight, CSSValueBold, HTMLNames::bTag));
        HTMLElementEquivalents.append(HTMLElementEquivalent::create(CSSPropertyFontWeight, CSSValueBold, HTMLNames::strongTag));
        HTMLElementEquivalents.append(HTMLElementEquivalent::create(CSSPropertyVerticalAlign, CSSValueSub, HTMLNames::subTag));
        HTMLElementEquivalents.append(HTMLElementEquivalent::create(CSSPropertyVerticalAlign, CSSValueSuper, HTMLNames::supTag));
        HTMLElementEquivalents.append(HTMLElementEquivalent::create(CSSPropertyFontStyle, CSSValueItalic, HTMLNames::iTag));
        HTMLElementEquivalents.append(HTMLElementEquivalent::create(CSSPropertyFontStyle, CSSValueItalic, HTMLNames::emTag));

        HTMLElementEquivalents.append(HTMLTextDecorationEquivalent::create(CSSValueUnderline, HTMLNames::uTag));
        HTMLElementEquivalents.append(HTMLTextDecorationEquivalent::create(CSSValueLineThrough, HTMLNames::sTag));
        HTMLElementEquivalents.append(HTMLTextDecorationEquivalent::create(CSSValueLineThrough, HTMLNames::strikeTag));
    }

    return HTMLElementEquivalents;
}


bool EditingStyle::conflictsWithImplicitStyleOfElement(HTMLElement* element, EditingStyle* extractedStyle, ShouldExtractMatchingStyle shouldExtractMatchingStyle) const
{
    if (!m_mutableStyle)
        return false;

    const Vector<OwnPtr<HTMLElementEquivalent> >& HTMLElementEquivalents = htmlElementEquivalents();
    for (size_t i = 0; i < HTMLElementEquivalents.size(); ++i) {
        const HTMLElementEquivalent* equivalent = HTMLElementEquivalents[i].get();
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
        // elementIsStyledSpanOrHTMLEquivalent depends on the fact each HTMLAttriuteEquivalent matches exactly one attribute
        // of exactly one element except dirAttr.
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
    if (!m_mutableStyle)
        return true;
    ComputedStyleExtractor computedStyle(node);
    return getPropertiesNotIn(m_mutableStyle.get(), &computedStyle)->isEmpty();
}

bool EditingStyle::elementIsStyledSpanOrHTMLEquivalent(const HTMLElement* element)
{
    bool elementIsSpanOrElementEquivalent = false;
    if (element->hasTagName(HTMLNames::spanTag))
        elementIsSpanOrElementEquivalent = true;
    else {
        const Vector<OwnPtr<HTMLElementEquivalent> >& HTMLElementEquivalents = htmlElementEquivalents();
        size_t i;
        for (i = 0; i < HTMLElementEquivalents.size(); ++i) {
            if (HTMLElementEquivalents[i]->matches(element)) {
                elementIsSpanOrElementEquivalent = true;
                break;
            }
        }
    }

    if (!element->hasAttributes())
        return elementIsSpanOrElementEquivalent; // span, b, etc... without any attributes

    unsigned matchedAttributes = 0;
    const Vector<OwnPtr<HTMLAttributeEquivalent> >& HTMLAttributeEquivalents = htmlAttributeEquivalents();
    for (size_t i = 0; i < HTMLAttributeEquivalents.size(); ++i) {
        if (HTMLAttributeEquivalents[i]->matches(element) && HTMLAttributeEquivalents[i]->attributeName() != HTMLNames::dirAttr)
            matchedAttributes++;
    }

    if (!elementIsSpanOrElementEquivalent && !matchedAttributes)
        return false; // element is not a span, a html element equivalent, or font element.
    
    if (element->getAttribute(HTMLNames::classAttr) == AppleStyleSpanClass)
        matchedAttributes++;

    if (element->hasAttribute(HTMLNames::styleAttr)) {
        if (const StylePropertySet* style = element->inlineStyle()) {
            unsigned propertyCount = style->propertyCount();
            for (unsigned i = 0; i < propertyCount; ++i) {
                if (!isEditingProperty(style->propertyAt(i).id()))
                    return false;
            }
        }
        matchedAttributes++;
    }

    // font with color attribute, span with style attribute, etc...
    ASSERT(matchedAttributes <= element->attributeCount());
    return matchedAttributes >= element->attributeCount();
}

void EditingStyle::prepareToApplyAt(const Position& position, ShouldPreserveWritingDirection shouldPreserveWritingDirection)
{
    if (!m_mutableStyle)
        return;

    // ReplaceSelectionCommand::handleStyleSpans() requires that this function only removes the editing style.
    // If this function was modified in the future to delete all redundant properties, then add a boolean value to indicate
    // which one of editingStyleAtPosition or computedStyle is called.
    RefPtr<EditingStyle> editingStyleAtPosition = EditingStyle::create(position, EditingPropertiesInEffect);
    StylePropertySet* styleAtPosition = editingStyleAtPosition->m_mutableStyle.get();

    RefPtr<CSSValue> unicodeBidi;
    RefPtr<CSSValue> direction;
    if (shouldPreserveWritingDirection == PreserveWritingDirection) {
        unicodeBidi = m_mutableStyle->getPropertyCSSValue(CSSPropertyUnicodeBidi);
        direction = m_mutableStyle->getPropertyCSSValue(CSSPropertyDirection);
    }

    m_mutableStyle->removeEquivalentProperties(styleAtPosition);

    if (textAlignResolvingStartAndEnd(m_mutableStyle.get()) == textAlignResolvingStartAndEnd(styleAtPosition))
        m_mutableStyle->removeProperty(CSSPropertyTextAlign);

    if (textColorFromStyle(m_mutableStyle.get()) == textColorFromStyle(styleAtPosition))
        m_mutableStyle->removeProperty(CSSPropertyColor);

    if (hasTransparentBackgroundColor(m_mutableStyle.get())
        || cssValueToRGBA(m_mutableStyle->getPropertyCSSValue(CSSPropertyBackgroundColor).get()) == rgbaBackgroundColorInEffect(position.containerNode()))
        m_mutableStyle->removeProperty(CSSPropertyBackgroundColor);

    if (unicodeBidi && unicodeBidi->isPrimitiveValue()) {
        m_mutableStyle->setProperty(CSSPropertyUnicodeBidi, static_cast<CSSValueID>(static_cast<CSSPrimitiveValue*>(unicodeBidi.get())->getValueID()));
        if (direction && direction->isPrimitiveValue())
            m_mutableStyle->setProperty(CSSPropertyDirection, static_cast<CSSValueID>(static_cast<CSSPrimitiveValue*>(direction.get())->getValueID()));
    }
}

void EditingStyle::mergeTypingStyle(Document* document)
{
    ASSERT(document);

    RefPtr<EditingStyle> typingStyle = document->frame()->selection()->typingStyle();
    if (!typingStyle || typingStyle == this)
        return;

    mergeStyle(typingStyle->style(), OverrideValues);
}

void EditingStyle::mergeInlineStyleOfElement(StyledElement* element, CSSPropertyOverrideMode mode, PropertiesToInclude propertiesToInclude)
{
    ASSERT(element);
    if (!element->inlineStyle())
        return;

    switch (propertiesToInclude) {
    case AllProperties:
        mergeStyle(element->inlineStyle(), mode);
        return;
    case OnlyEditingInheritableProperties:
        mergeStyle(copyEditingProperties(element->inlineStyle(), OnlyInheritableEditingProperties).get(), mode);
        return;
    case EditingPropertiesInEffect:
        mergeStyle(copyEditingProperties(element->inlineStyle(), AllEditingProperties).get(), mode);
        return;
    }
}

static inline bool elementMatchesAndPropertyIsNotInInlineStyleDecl(const HTMLElementEquivalent* equivalent, const StyledElement* element,
    EditingStyle::CSSPropertyOverrideMode mode, StylePropertySet* style)
{
    return equivalent->matches(element) && (!element->inlineStyle() || !equivalent->propertyExistsInStyle(element->inlineStyle()))
        && (mode == EditingStyle::OverrideValues || !equivalent->propertyExistsInStyle(style));
}

static PassRefPtr<MutableStylePropertySet> extractEditingProperties(const StylePropertySet* style, EditingStyle::PropertiesToInclude propertiesToInclude)
{
    if (!style)
        return 0;

    switch (propertiesToInclude) {
    case EditingStyle::AllProperties:
    case EditingStyle::EditingPropertiesInEffect:
        return copyEditingProperties(style, AllEditingProperties);
    case EditingStyle::OnlyEditingInheritableProperties:
        return copyEditingProperties(style, OnlyInheritableEditingProperties);
    }

    ASSERT_NOT_REACHED();
    return 0;
}

void EditingStyle::mergeInlineAndImplicitStyleOfElement(StyledElement* element, CSSPropertyOverrideMode mode, PropertiesToInclude propertiesToInclude)
{
    RefPtr<EditingStyle> styleFromRules = EditingStyle::create();
    styleFromRules->mergeStyleFromRulesForSerialization(element);
    styleFromRules->m_mutableStyle = extractEditingProperties(styleFromRules->m_mutableStyle.get(), propertiesToInclude);
    mergeStyle(styleFromRules->m_mutableStyle.get(), mode);

    mergeInlineStyleOfElement(element, mode, propertiesToInclude);

    const Vector<OwnPtr<HTMLElementEquivalent> >& elementEquivalents = htmlElementEquivalents();
    for (size_t i = 0; i < elementEquivalents.size(); ++i) {
        if (elementMatchesAndPropertyIsNotInInlineStyleDecl(elementEquivalents[i].get(), element, mode, m_mutableStyle.get()))
            elementEquivalents[i]->addToStyle(element, this);
    }

    const Vector<OwnPtr<HTMLAttributeEquivalent> >& attributeEquivalents = htmlAttributeEquivalents();
    for (size_t i = 0; i < attributeEquivalents.size(); ++i) {
        if (attributeEquivalents[i]->attributeName() == HTMLNames::dirAttr)
            continue; // We don't want to include directionality
        if (elementMatchesAndPropertyIsNotInInlineStyleDecl(attributeEquivalents[i].get(), element, mode, m_mutableStyle.get()))
            attributeEquivalents[i]->addToStyle(element, this);
    }
}

PassRefPtr<EditingStyle> EditingStyle::wrappingStyleForSerialization(Node* context, bool shouldAnnotate)
{
    RefPtr<EditingStyle> wrappingStyle;
    if (shouldAnnotate) {
        wrappingStyle = EditingStyle::create(context, EditingStyle::EditingPropertiesInEffect);

        // Styles that Mail blockquotes contribute should only be placed on the Mail blockquote,
        // to help us differentiate those styles from ones that the user has applied.
        // This helps us get the color of content pasted into blockquotes right.
        wrappingStyle->removeStyleAddedByNode(enclosingNodeOfType(firstPositionInOrBeforeNode(context), isMailBlockquote, CanCrossEditingBoundary));

        // Call collapseTextDecorationProperties first or otherwise it'll copy the value over from in-effect to text-decorations.
        wrappingStyle->collapseTextDecorationProperties();
        
        return wrappingStyle.release();
    }

    wrappingStyle = EditingStyle::create();

    // When not annotating for interchange, we only preserve inline style declarations.
    for (Node* node = context; node && !node->isDocumentNode(); node = node->parentNode()) {
        if (node->isStyledElement() && !isMailBlockquote(node)) {
            wrappingStyle->mergeInlineAndImplicitStyleOfElement(static_cast<StyledElement*>(node), EditingStyle::DoNotOverrideValues,
                EditingStyle::EditingPropertiesInEffect);
        }
    }

    return wrappingStyle.release();
}


static void mergeTextDecorationValues(CSSValueList* mergedValue, const CSSValueList* valueToMerge)
{
    DEFINE_STATIC_LOCAL(const RefPtr<CSSPrimitiveValue>, underline, (CSSPrimitiveValue::createIdentifier(CSSValueUnderline)));
    DEFINE_STATIC_LOCAL(const RefPtr<CSSPrimitiveValue>, lineThrough, (CSSPrimitiveValue::createIdentifier(CSSValueLineThrough)));

    if (valueToMerge->hasValue(underline.get()) && !mergedValue->hasValue(underline.get()))
        mergedValue->append(underline.get());

    if (valueToMerge->hasValue(lineThrough.get()) && !mergedValue->hasValue(lineThrough.get()))
        mergedValue->append(lineThrough.get());
}

void EditingStyle::mergeStyle(const StylePropertySet* style, CSSPropertyOverrideMode mode)
{
    if (!style)
        return;

    if (!m_mutableStyle) {
        m_mutableStyle = style->mutableCopy();
        return;
    }

    unsigned propertyCount = style->propertyCount();
    for (unsigned i = 0; i < propertyCount; ++i) {
        StylePropertySet::PropertyReference property = style->propertyAt(i);
        RefPtr<CSSValue> value = m_mutableStyle->getPropertyCSSValue(property.id());

        // text decorations never override values
        if ((property.id() == CSSPropertyTextDecoration || property.id() == CSSPropertyWebkitTextDecorationsInEffect) && property.value()->isValueList() && value) {
            if (value->isValueList()) {
                mergeTextDecorationValues(static_cast<CSSValueList*>(value.get()), static_cast<CSSValueList*>(property.value()));
                continue;
            }
            value = 0; // text-decoration: none is equivalent to not having the property
        }

        if (mode == OverrideValues || (mode == DoNotOverrideValues && !value))
            m_mutableStyle->setProperty(property.id(), property.value()->cssText(), property.isImportant());
    }

    int oldFontSizeDelta = m_fontSizeDelta;
    extractFontSizeDelta();
    m_fontSizeDelta += oldFontSizeDelta;
}

static PassRefPtr<MutableStylePropertySet> styleFromMatchedRulesForElement(Element* element, unsigned rulesToInclude)
{
    RefPtr<MutableStylePropertySet> style = MutableStylePropertySet::create();
    Vector<RefPtr<StyleRuleBase> > matchedRules = element->document()->ensureStyleResolver()->styleRulesForElement(element, rulesToInclude);
    for (unsigned i = 0; i < matchedRules.size(); ++i) {
        if (matchedRules[i]->isStyleRule())
            style->mergeAndOverrideOnConflict(static_pointer_cast<StyleRule>(matchedRules[i])->properties());
    }
    
    return style.release();
}

void EditingStyle::mergeStyleFromRules(StyledElement* element)
{
    RefPtr<MutableStylePropertySet> styleFromMatchedRules = styleFromMatchedRulesForElement(element,
        StyleResolver::AuthorCSSRules | StyleResolver::CrossOriginCSSRules);
    // Styles from the inline style declaration, held in the variable "style", take precedence 
    // over those from matched rules.
    if (m_mutableStyle)
        styleFromMatchedRules->mergeAndOverrideOnConflict(m_mutableStyle.get());

    clear();
    m_mutableStyle = styleFromMatchedRules;
}

void EditingStyle::mergeStyleFromRulesForSerialization(StyledElement* element)
{
    mergeStyleFromRules(element);

    // The property value, if it's a percentage, may not reflect the actual computed value.  
    // For example: style="height: 1%; overflow: visible;" in quirksmode
    // FIXME: There are others like this, see <rdar://problem/5195123> Slashdot copy/paste fidelity problem
    RefPtr<MutableStylePropertySet> fromComputedStyle = MutableStylePropertySet::create();
    ComputedStyleExtractor computedStyle(element);

    {
        unsigned propertyCount = m_mutableStyle->propertyCount();
        for (unsigned i = 0; i < propertyCount; ++i) {
            StylePropertySet::PropertyReference property = m_mutableStyle->propertyAt(i);
            CSSValue* value = property.value();
            if (!value->isPrimitiveValue())
                continue;
            if (static_cast<CSSPrimitiveValue*>(value)->isPercentage()) {
                if (RefPtr<CSSValue> computedPropertyValue = computedStyle.propertyValue(property.id()))
                    fromComputedStyle->addParsedProperty(CSSProperty(property.id(), computedPropertyValue.release()));
            }
        }
    }
    m_mutableStyle->mergeAndOverrideOnConflict(fromComputedStyle.get());
}

static void removePropertiesInStyle(MutableStylePropertySet* styleToRemovePropertiesFrom, StylePropertySet* style)
{
    unsigned propertyCount = style->propertyCount();
    Vector<CSSPropertyID> propertiesToRemove(propertyCount);
    for (unsigned i = 0; i < propertyCount; ++i)
        propertiesToRemove[i] = style->propertyAt(i).id();

    styleToRemovePropertiesFrom->removePropertiesInSet(propertiesToRemove.data(), propertiesToRemove.size());
}

void EditingStyle::removeStyleFromRulesAndContext(StyledElement* element, Node* context)
{
    ASSERT(element);
    if (!m_mutableStyle)
        return;

    // 1. Remove style from matched rules because style remain without repeating it in inline style declaration
    RefPtr<MutableStylePropertySet> styleFromMatchedRules = styleFromMatchedRulesForElement(element, StyleResolver::AllButEmptyCSSRules);
    if (styleFromMatchedRules && !styleFromMatchedRules->isEmpty())
        m_mutableStyle = getPropertiesNotIn(m_mutableStyle.get(), styleFromMatchedRules.get());

    // 2. Remove style present in context and not overriden by matched rules.
    RefPtr<EditingStyle> computedStyle = EditingStyle::create(context, EditingPropertiesInEffect);
    if (computedStyle->m_mutableStyle) {
        if (!computedStyle->m_mutableStyle->getPropertyCSSValue(CSSPropertyBackgroundColor))
            computedStyle->m_mutableStyle->setProperty(CSSPropertyBackgroundColor, CSSValueTransparent);

        removePropertiesInStyle(computedStyle->m_mutableStyle.get(), styleFromMatchedRules.get());
        m_mutableStyle = getPropertiesNotIn(m_mutableStyle.get(), computedStyle->m_mutableStyle.get());
    }

    // 3. If this element is a span and has display: inline or float: none, remove them unless they are overriden by rules.
    // These rules are added by serialization code to wrap text nodes.
    if (isStyleSpanOrSpanWithOnlyStyleAttribute(element)) {
        if (!styleFromMatchedRules->getPropertyCSSValue(CSSPropertyDisplay) && identifierForStyleProperty(m_mutableStyle.get(), CSSPropertyDisplay) == CSSValueInline)
            m_mutableStyle->removeProperty(CSSPropertyDisplay);
        if (!styleFromMatchedRules->getPropertyCSSValue(CSSPropertyFloat) && identifierForStyleProperty(m_mutableStyle.get(), CSSPropertyFloat) == CSSValueNone)
            m_mutableStyle->removeProperty(CSSPropertyFloat);
    }
}

void EditingStyle::removePropertiesInElementDefaultStyle(Element* element)
{
    if (!m_mutableStyle || m_mutableStyle->isEmpty())
        return;

    RefPtr<StylePropertySet> defaultStyle = styleFromMatchedRulesForElement(element, StyleResolver::UAAndUserCSSRules);

    removePropertiesInStyle(m_mutableStyle.get(), defaultStyle.get());
}

void EditingStyle::forceInline()
{
    if (!m_mutableStyle)
        m_mutableStyle = MutableStylePropertySet::create();
    const bool propertyIsImportant = true;
    m_mutableStyle->setProperty(CSSPropertyDisplay, CSSValueInline, propertyIsImportant);
}

int EditingStyle::legacyFontSize(Document* document) const
{
    RefPtr<CSSValue> cssValue = m_mutableStyle->getPropertyCSSValue(CSSPropertyFontSize);
    if (!cssValue || !cssValue->isPrimitiveValue())
        return 0;
    return legacyFontSizeFromCSSValue(document, static_cast<CSSPrimitiveValue*>(cssValue.get()),
        m_shouldUseFixedDefaultFontSize, AlwaysUseLegacyFontSize);
}

PassRefPtr<EditingStyle> EditingStyle::styleAtSelectionStart(const VisibleSelection& selection, bool shouldUseBackgroundColorInEffect)
{
    if (selection.isNone())
        return 0;

    Position position = adjustedSelectionStartForStyleComputation(selection);

    // If the pos is at the end of a text node, then this node is not fully selected. 
    // Move it to the next deep equivalent position to avoid removing the style from this node. 
    // e.g. if pos was at Position("hello", 5) in <b>hello<div>world</div></b>, we want Position("world", 0) instead. 
    // We only do this for range because caret at Position("hello", 5) in <b>hello</b>world should give you font-weight: bold. 
    Node* positionNode = position.containerNode(); 
    if (selection.isRange() && positionNode && positionNode->isTextNode() && position.computeOffsetInContainerNode() == positionNode->maxCharacterOffset()) 
        position = nextVisuallyDistinctCandidate(position); 

    Element* element = position.element();
    if (!element)
        return 0;

    RefPtr<EditingStyle> style = EditingStyle::create(element, EditingStyle::AllProperties);
    style->mergeTypingStyle(element->document());

    // If background color is transparent, traverse parent nodes until we hit a different value or document root
    // Also, if the selection is a range, ignore the background color at the start of selection,
    // and find the background color of the common ancestor.
    if (shouldUseBackgroundColorInEffect && (selection.isRange() || hasTransparentBackgroundColor(style->m_mutableStyle.get()))) {
        RefPtr<Range> range(selection.toNormalizedRange());
        if (PassRefPtr<CSSValue> value = backgroundColorInEffect(range->commonAncestorContainer(IGNORE_EXCEPTION)))
            style->setProperty(CSSPropertyBackgroundColor, value->cssText());
    }

    return style;
}

WritingDirection EditingStyle::textDirectionForSelection(const VisibleSelection& selection, EditingStyle* typingStyle, bool& hasNestedOrMultipleEmbeddings)
{
    hasNestedOrMultipleEmbeddings = true;

    if (selection.isNone())
        return NaturalWritingDirection;

    Position position = selection.start().downstream();

    Node* node = position.deprecatedNode();
    if (!node)
        return NaturalWritingDirection;

    Position end;
    if (selection.isRange()) {
        end = selection.end().upstream();

        Node* pastLast = Range::create(end.document(), position.parentAnchoredEquivalent(), end.parentAnchoredEquivalent())->pastLastNode();
        for (Node* n = node; n && n != pastLast; n = NodeTraversal::next(n)) {
            if (!n->isStyledElement())
                continue;

            RefPtr<CSSValue> unicodeBidi = ComputedStyleExtractor(n).propertyValue(CSSPropertyUnicodeBidi);
            if (!unicodeBidi || !unicodeBidi->isPrimitiveValue())
                continue;

            CSSValueID unicodeBidiValue = static_cast<CSSPrimitiveValue*>(unicodeBidi.get())->getValueID();
            if (unicodeBidiValue == CSSValueEmbed || unicodeBidiValue == CSSValueBidiOverride)
                return NaturalWritingDirection;
        }
    }

    if (selection.isCaret()) {
        WritingDirection direction;
        if (typingStyle && typingStyle->textDirection(direction)) {
            hasNestedOrMultipleEmbeddings = false;
            return direction;
        }
        node = selection.visibleStart().deepEquivalent().deprecatedNode();
    }

    // The selection is either a caret with no typing attributes or a range in which no embedding is added, so just use the start position
    // to decide.
    Node* block = enclosingBlock(node);
    WritingDirection foundDirection = NaturalWritingDirection;

    for (; node != block; node = node->parentNode()) {
        if (!node->isStyledElement())
            continue;

        ComputedStyleExtractor computedStyle(node);
        RefPtr<CSSValue> unicodeBidi = computedStyle.propertyValue(CSSPropertyUnicodeBidi);
        if (!unicodeBidi || !unicodeBidi->isPrimitiveValue())
            continue;

        CSSValueID unicodeBidiValue = static_cast<CSSPrimitiveValue*>(unicodeBidi.get())->getValueID();
        if (unicodeBidiValue == CSSValueNormal)
            continue;

        if (unicodeBidiValue == CSSValueBidiOverride)
            return NaturalWritingDirection;

        ASSERT(unicodeBidiValue == CSSValueEmbed);
        RefPtr<CSSValue> direction = computedStyle.propertyValue(CSSPropertyDirection);
        if (!direction || !direction->isPrimitiveValue())
            continue;

        CSSValueID directionValue = static_cast<CSSPrimitiveValue*>(direction.get())->getValueID();
        if (directionValue != CSSValueLtr && directionValue != CSSValueRtl)
            continue;

        if (foundDirection != NaturalWritingDirection)
            return NaturalWritingDirection;

        // In the range case, make sure that the embedding element persists until the end of the range.
        if (selection.isRange() && !end.deprecatedNode()->isDescendantOf(node))
            return NaturalWritingDirection;
        
        foundDirection = directionValue == CSSValueLtr ? LeftToRightWritingDirection : RightToLeftWritingDirection;
    }
    hasNestedOrMultipleEmbeddings = false;
    return foundDirection;
}

static void reconcileTextDecorationProperties(MutableStylePropertySet* style)
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

    Node* node = position.containerNode();
    if (!node)
        return;

    ComputedStyleExtractor computedStyle(node);

    // FIXME: take care of background-color in effect
    RefPtr<MutableStylePropertySet> mutableStyle = getPropertiesNotIn(style->style(), &computedStyle);

    reconcileTextDecorationProperties(mutableStyle.get());
    if (!document->frame()->editor().shouldStyleWithCSS())
        extractTextStyles(document, mutableStyle.get(), computedStyle.useFixedFontDefaultSize());

    // Changing the whitespace style in a tab span would collapse the tab into a space.
    if (isTabSpanTextNode(position.deprecatedNode()) || isTabSpanNode((position.deprecatedNode())))
        mutableStyle->removeProperty(CSSPropertyWhiteSpace);

    // If unicode-bidi is present in mutableStyle and direction is not, then add direction to mutableStyle.
    // FIXME: Shouldn't this be done in getPropertiesNotIn?
    if (mutableStyle->getPropertyCSSValue(CSSPropertyUnicodeBidi) && !style->style()->getPropertyCSSValue(CSSPropertyDirection))
        mutableStyle->setProperty(CSSPropertyDirection, style->style()->getPropertyValue(CSSPropertyDirection));

    // Save the result for later
    m_cssStyle = mutableStyle->asText().stripWhiteSpace();
}

static void setTextDecorationProperty(MutableStylePropertySet* style, const CSSValueList* newTextDecoration, CSSPropertyID propertyID)
{
    if (newTextDecoration->length())
        style->setProperty(propertyID, newTextDecoration->cssText(), style->propertyIsImportant(propertyID));
    else {
        // text-decoration: none is redundant since it does not remove any text decorations.
        ASSERT(!style->propertyIsImportant(propertyID));
        style->removeProperty(propertyID);
    }
}

void StyleChange::extractTextStyles(Document* document, MutableStylePropertySet* style, bool shouldUseFixedFontDefaultSize)
{
    ASSERT(style);

    if (identifierForStyleProperty(style, CSSPropertyFontWeight) == CSSValueBold) {
        style->removeProperty(CSSPropertyFontWeight);
        m_applyBold = true;
    }

    int fontStyle = identifierForStyleProperty(style, CSSPropertyFontStyle);
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

    int verticalAlign = identifierForStyleProperty(style, CSSPropertyVerticalAlign);
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
        m_applyFontColor = Color(textColorFromStyle(style)).serialized();
        style->removeProperty(CSSPropertyColor);
    }

    m_applyFontFace = style->getPropertyValue(CSSPropertyFontFamily);
    // Remove single quotes for Outlook 2007 compatibility. See https://bugs.webkit.org/show_bug.cgi?id=79448
    m_applyFontFace.replaceWithLiteral('\'', "");
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

static void diffTextDecorations(MutableStylePropertySet* style, CSSPropertyID propertID, CSSValue* refTextDecoration)
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

static bool fontWeightIsBold(CSSValue* fontWeight)
{
    if (!fontWeight)
        return false;
    if (!fontWeight->isPrimitiveValue())
        return false;

    // Because b tag can only bold text, there are only two states in plain html: bold and not bold.
    // Collapse all other values to either one of these two states for editing purposes.
    switch (static_cast<CSSPrimitiveValue*>(fontWeight)->getValueID()) {
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
        default:
            break;
    }

    ASSERT_NOT_REACHED(); // For CSSValueBolder and CSSValueLighter
    return false;
}

template<typename T>
static bool fontWeightIsBold(T* style)
{
    return fontWeightIsBold(extractPropertyValue(style, CSSPropertyFontWeight).get());
}

template<typename T>
static PassRefPtr<MutableStylePropertySet> extractPropertiesNotIn(StylePropertySet* styleWithRedundantProperties, T* baseStyle)
{
    ASSERT(styleWithRedundantProperties);
    RefPtr<MutableStylePropertySet> result = styleWithRedundantProperties->mutableCopy();

    result->removeEquivalentProperties(baseStyle);

    RefPtr<CSSValue> baseTextDecorationsInEffect = extractPropertyValue(baseStyle, CSSPropertyWebkitTextDecorationsInEffect);
    diffTextDecorations(result.get(), CSSPropertyTextDecoration, baseTextDecorationsInEffect.get());
    diffTextDecorations(result.get(), CSSPropertyWebkitTextDecorationsInEffect, baseTextDecorationsInEffect.get());

    if (extractPropertyValue(baseStyle, CSSPropertyFontWeight) && fontWeightIsBold(result.get()) == fontWeightIsBold(baseStyle))
        result->removeProperty(CSSPropertyFontWeight);

    if (extractPropertyValue(baseStyle, CSSPropertyColor) && textColorFromStyle(result.get()) == textColorFromStyle(baseStyle))
        result->removeProperty(CSSPropertyColor);

    if (extractPropertyValue(baseStyle, CSSPropertyTextAlign)
        && textAlignResolvingStartAndEnd(result.get()) == textAlignResolvingStartAndEnd(baseStyle))
        result->removeProperty(CSSPropertyTextAlign);

    if (extractPropertyValue(baseStyle, CSSPropertyBackgroundColor) && backgroundColorFromStyle(result.get()) == backgroundColorFromStyle(baseStyle))
        result->removeProperty(CSSPropertyBackgroundColor);

    return result.release();
}

template<typename T>
PassRefPtr<MutableStylePropertySet> getPropertiesNotIn(StylePropertySet* styleWithRedundantProperties, T* baseStyle)
{
    return extractPropertiesNotIn(styleWithRedundantProperties, baseStyle);
}

static bool isCSSValueLength(CSSPrimitiveValue* value)
{
    return value->isFontIndependentLength();
}

int legacyFontSizeFromCSSValue(Document* document, CSSPrimitiveValue* value, bool shouldUseFixedFontDefaultSize, LegacyFontSizeMode mode)
{
    if (isCSSValueLength(value)) {
        int pixelFontSize = value->getIntValue(CSSPrimitiveValue::CSS_PX);
        int legacyFontSize = StyleResolver::legacyFontSize(document, pixelFontSize, shouldUseFixedFontDefaultSize);
        // Use legacy font size only if pixel value matches exactly to that of legacy font size.
        int cssPrimitiveEquivalent = legacyFontSize - 1 + CSSValueXSmall;
        if (mode == AlwaysUseLegacyFontSize || StyleResolver::fontSizeForKeyword(document, cssPrimitiveEquivalent, shouldUseFixedFontDefaultSize) == pixelFontSize)
            return legacyFontSize;

        return 0;
    }

    if (CSSValueXSmall <= value->getValueID() && value->getValueID() <= CSSValueWebkitXxxLarge)
        return value->getValueID() - CSSValueXSmall + 1;

    return 0;
}

bool isTransparentColorValue(CSSValue* cssValue)
{
    if (!cssValue)
        return true;
    if (!cssValue->isPrimitiveValue())
        return false;
    CSSPrimitiveValue* value = static_cast<CSSPrimitiveValue*>(cssValue);    
    if (value->isRGBColor())
        return !alphaChannel(value->getRGBA32Value());    
    return value->getValueID() == CSSValueTransparent;
}

bool hasTransparentBackgroundColor(StylePropertySet* style)
{
    RefPtr<CSSValue> cssValue = style->getPropertyCSSValue(CSSPropertyBackgroundColor);
    return isTransparentColorValue(cssValue.get());
}

PassRefPtr<CSSValue> backgroundColorInEffect(Node* node)
{
    for (Node* ancestor = node; ancestor; ancestor = ancestor->parentNode()) {
        if (RefPtr<CSSValue> value = ComputedStyleExtractor(ancestor).propertyValue(CSSPropertyBackgroundColor)) {
            if (!isTransparentColorValue(value.get()))
                return value.release();
        }
    }
    return 0;
}

}
