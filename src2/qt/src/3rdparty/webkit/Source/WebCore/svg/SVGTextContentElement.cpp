/*
 * Copyright (C) 2004, 2005, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Rob Buis <buis@kde.org>
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

#if ENABLE(SVG)
#include "SVGTextContentElement.h"

#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "Frame.h"
#include "RenderObject.h"
#include "RenderSVGResource.h"
#include "RenderSVGText.h"
#include "SVGDocumentExtensions.h"
#include "SVGNames.h"
#include "SVGTextQuery.h"
#include "SelectionController.h"
#include "XMLNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_ENUMERATION(SVGTextContentElement, SVGNames::lengthAdjustAttr, LengthAdjust, lengthAdjust)
DEFINE_ANIMATED_BOOLEAN(SVGTextContentElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

SVGTextContentElement::SVGTextContentElement(const QualifiedName& tagName, Document* document)
    : SVGStyledElement(tagName, document)
    , m_specifiedTextLength(LengthModeOther)
    , m_textLength(LengthModeOther)
    , m_lengthAdjust(LENGTHADJUST_SPACING)
{
}

void SVGTextContentElement::synchronizeTextLength()
{
    if (!m_textLength.shouldSynchronize)
        return;
    AtomicString value(SVGPropertyTraits<SVGLength>::toString(m_specifiedTextLength));
    SVGAnimatedPropertySynchronizer<true>::synchronize(this, SVGNames::textLengthAttr, value);
}

PassRefPtr<SVGAnimatedLength> SVGTextContentElement::textLengthAnimated()
{
    DEFINE_STATIC_LOCAL(SVGLength, defaultTextLength, (LengthModeOther));
    if (m_specifiedTextLength == defaultTextLength) {
        ExceptionCode ec = 0;
        m_textLength.value.newValueSpecifiedUnits(LengthTypeNumber, getComputedTextLength(), ec);
        ASSERT(!ec);
    }

    m_textLength.shouldSynchronize = true;
    return SVGAnimatedProperty::lookupOrCreateWrapper<SVGAnimatedLength, SVGLength>(this, SVGNames::textLengthAttr, SVGNames::textLengthAttr.localName(), m_textLength.value);
}

unsigned SVGTextContentElement::getNumberOfChars() const
{
    document()->updateLayoutIgnorePendingStylesheets();
    return SVGTextQuery(renderer()).numberOfCharacters();
}

float SVGTextContentElement::getComputedTextLength() const
{
    document()->updateLayoutIgnorePendingStylesheets();
    return SVGTextQuery(renderer()).textLength();
}

float SVGTextContentElement::getSubStringLength(unsigned charnum, unsigned nchars, ExceptionCode& ec) const
{
    document()->updateLayoutIgnorePendingStylesheets();

    unsigned numberOfChars = getNumberOfChars();
    if (charnum >= numberOfChars) {
        ec = INDEX_SIZE_ERR;
        return 0.0f;
    }

    return SVGTextQuery(renderer()).subStringLength(charnum, nchars);
}

FloatPoint SVGTextContentElement::getStartPositionOfChar(unsigned charnum, ExceptionCode& ec) const
{
    document()->updateLayoutIgnorePendingStylesheets();

    if (charnum > getNumberOfChars()) {
        ec = INDEX_SIZE_ERR;
        return FloatPoint();
    }

    return SVGTextQuery(renderer()).startPositionOfCharacter(charnum);
}

FloatPoint SVGTextContentElement::getEndPositionOfChar(unsigned charnum, ExceptionCode& ec) const
{
    document()->updateLayoutIgnorePendingStylesheets();

    if (charnum > getNumberOfChars()) {
        ec = INDEX_SIZE_ERR;
        return FloatPoint();
    }

    return SVGTextQuery(renderer()).endPositionOfCharacter(charnum);
}

FloatRect SVGTextContentElement::getExtentOfChar(unsigned charnum, ExceptionCode& ec) const
{
    document()->updateLayoutIgnorePendingStylesheets();

    if (charnum > getNumberOfChars()) {
        ec = INDEX_SIZE_ERR;
        return FloatRect();
    }

    return SVGTextQuery(renderer()).extentOfCharacter(charnum);
}

float SVGTextContentElement::getRotationOfChar(unsigned charnum, ExceptionCode& ec) const
{
    document()->updateLayoutIgnorePendingStylesheets();

    if (charnum > getNumberOfChars()) {
        ec = INDEX_SIZE_ERR;
        return 0.0f;
    }

    return SVGTextQuery(renderer()).rotationOfCharacter(charnum);
}

int SVGTextContentElement::getCharNumAtPosition(const FloatPoint& point) const
{
    document()->updateLayoutIgnorePendingStylesheets();
    return SVGTextQuery(renderer()).characterNumberAtPosition(point);
}

void SVGTextContentElement::selectSubString(unsigned charnum, unsigned nchars, ExceptionCode& ec) const
{
    unsigned numberOfChars = getNumberOfChars();
    if (charnum >= numberOfChars) {
        ec = INDEX_SIZE_ERR;
        return;
    }

    if (nchars > numberOfChars - charnum)
        nchars = numberOfChars - charnum;

    ASSERT(document());
    ASSERT(document()->frame());

    SelectionController* controller = document()->frame()->selection();
    if (!controller)
        return;

    // Find selection start
    VisiblePosition start(firstPositionInNode(const_cast<SVGTextContentElement*>(this)));
    for (unsigned i = 0; i < charnum; ++i)
        start = start.next();

    // Find selection end
    VisiblePosition end(start);
    for (unsigned i = 0; i < nchars; ++i)
        end = end.next();

    controller->setSelection(VisibleSelection(start, end));
}

void SVGTextContentElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == SVGNames::lengthAdjustAttr) {
        if (attr->value() == "spacing")
            setLengthAdjustBaseValue(LENGTHADJUST_SPACING);
        else if (attr->value() == "spacingAndGlyphs")
            setLengthAdjustBaseValue(LENGTHADJUST_SPACINGANDGLYPHS);
    } else if (attr->name() == SVGNames::textLengthAttr) {
        m_textLength.value = SVGLength(LengthModeOther, attr->value());
        if (m_textLength.value.value(this) < 0)
            document()->accessSVGExtensions()->reportError("A negative value for text attribute <textLength> is not allowed");
    } else {
        if (SVGTests::parseMappedAttribute(attr))
            return;
        if (SVGLangSpace::parseMappedAttribute(attr)) {
            if (attr->name().matches(XMLNames::spaceAttr)) {
                DEFINE_STATIC_LOCAL(const AtomicString, preserveString, ("preserve"));

                if (attr->value() == preserveString)
                    addCSSProperty(attr, CSSPropertyWhiteSpace, CSSValuePre);
                else
                    addCSSProperty(attr, CSSPropertyWhiteSpace, CSSValueNowrap);
            }
            return;
        }
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;

        SVGStyledElement::parseMappedAttribute(attr);
    }
}

void SVGTextContentElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGStyledElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeLengthAdjust();
        synchronizeTextLength();
        synchronizeExternalResourcesRequired();
        SVGTests::synchronizeProperties(this, attrName);
        return;
    }

    if (attrName == SVGNames::lengthAdjustAttr)
        synchronizeLengthAdjust();
    else if (attrName == SVGNames::textLengthAttr)
        synchronizeTextLength();
    else if (SVGExternalResourcesRequired::isKnownAttribute(attrName))
        synchronizeExternalResourcesRequired();
    else if (SVGTests::isKnownAttribute(attrName))
        SVGTests::synchronizeProperties(this, attrName);
}

void SVGTextContentElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGStyledElement::svgAttributeChanged(attrName);

    if (SVGTests::handleAttributeChange(this, attrName))
        return;

    if (attrName == SVGNames::textLengthAttr)
        m_specifiedTextLength = m_textLength.value;

    RenderObject* renderer = this->renderer();
    if (!renderer)
        return;

    if (attrName == SVGNames::textLengthAttr
        || attrName == SVGNames::lengthAdjustAttr)
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
}

void SVGTextContentElement::fillPassedAttributeToPropertyTypeMap(AttributeToPropertyTypeMap& attributeToPropertyTypeMap)
{
    SVGStyledElement::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);

    attributeToPropertyTypeMap.set(SVGNames::textLengthAttr, AnimatedLength);
    attributeToPropertyTypeMap.set(SVGNames::lengthAdjustAttr, AnimatedEnumeration);
}

bool SVGTextContentElement::selfHasRelativeLengths() const
{
    // Any element of the <text> subtree is advertized as using relative lengths.
    // On any window size change, we have to relayout the text subtree, as the
    // effective 'on-screen' font size may change.
    return true;
}

SVGTextContentElement* SVGTextContentElement::elementFromRenderer(RenderObject* renderer)
{
    if (!renderer)
        return 0;

    if (!renderer->isSVGText() && !renderer->isSVGInline())
        return 0;

    Node* node = renderer->node();
    ASSERT(node);
    ASSERT(node->isSVGElement());

    if (!node->hasTagName(SVGNames::textTag)
        && !node->hasTagName(SVGNames::tspanTag)
#if ENABLE(SVG_FONTS)
        && !node->hasTagName(SVGNames::altGlyphTag)
#endif
        && !node->hasTagName(SVGNames::trefTag)
        && !node->hasTagName(SVGNames::textPathTag))
        return 0;

    return static_cast<SVGTextContentElement*>(node);
}

void SVGTextContentElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    SVGStyledElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);

    if (changedByParser || !renderer())
        return;

    if (RenderSVGText* textRenderer = RenderSVGText::locateRenderSVGTextAncestor(renderer()))
        textRenderer->setNeedsPositioningValuesUpdate();
}

}

#endif // ENABLE(SVG)
