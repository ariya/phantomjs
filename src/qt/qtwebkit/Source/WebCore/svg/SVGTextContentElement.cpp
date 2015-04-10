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
#include "FrameSelection.h"
#include "RenderObject.h"
#include "RenderSVGResource.h"
#include "RenderSVGText.h"
#include "SVGDocumentExtensions.h"
#include "SVGElementInstance.h"
#include "SVGNames.h"
#include "SVGTextQuery.h"
#include "XMLNames.h"

namespace WebCore {
 
// Define custom animated property 'textLength'.
const SVGPropertyInfo* SVGTextContentElement::textLengthPropertyInfo()
{
    static const SVGPropertyInfo* s_propertyInfo = 0;
    if (!s_propertyInfo) {
        s_propertyInfo = new SVGPropertyInfo(AnimatedLength,
                                             PropertyIsReadWrite,
                                             SVGNames::textLengthAttr,
                                             SVGNames::textLengthAttr.localName(),
                                             &SVGTextContentElement::synchronizeTextLength,
                                             &SVGTextContentElement::lookupOrCreateTextLengthWrapper);
    }
    return s_propertyInfo;
}

// Animated property definitions
DEFINE_ANIMATED_ENUMERATION(SVGTextContentElement, SVGNames::lengthAdjustAttr, LengthAdjust, lengthAdjust, SVGLengthAdjustType)
DEFINE_ANIMATED_BOOLEAN(SVGTextContentElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGTextContentElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(textLength)
    REGISTER_LOCAL_ANIMATED_PROPERTY(lengthAdjust)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGGraphicsElement)
END_REGISTER_ANIMATED_PROPERTIES

SVGTextContentElement::SVGTextContentElement(const QualifiedName& tagName, Document* document)
    : SVGGraphicsElement(tagName, document)
    , m_textLength(LengthModeOther)
    , m_specifiedTextLength(LengthModeOther)
    , m_lengthAdjust(SVGLengthAdjustSpacing)
{
    registerAnimatedPropertiesForSVGTextContentElement();
}

void SVGTextContentElement::synchronizeTextLength(SVGElement* contextElement)
{
    ASSERT(contextElement);
    SVGTextContentElement* ownerType = toSVGTextContentElement(contextElement);
    if (!ownerType->m_textLength.shouldSynchronize)
        return;
    AtomicString value(SVGPropertyTraits<SVGLength>::toString(ownerType->m_specifiedTextLength));
    ownerType->m_textLength.synchronize(ownerType, textLengthPropertyInfo()->attributeName, value);
}

PassRefPtr<SVGAnimatedProperty> SVGTextContentElement::lookupOrCreateTextLengthWrapper(SVGElement* contextElement)
{
    ASSERT(contextElement);
    SVGTextContentElement* ownerType = toSVGTextContentElement(contextElement);
    return SVGAnimatedProperty::lookupOrCreateWrapper<SVGTextContentElement, SVGAnimatedLength, SVGLength>
           (ownerType, textLengthPropertyInfo(), ownerType->m_textLength.value);
}

PassRefPtr<SVGAnimatedLength> SVGTextContentElement::textLengthAnimated()
{
    DEFINE_STATIC_LOCAL(SVGLength, defaultTextLength, (LengthModeOther));
    if (m_specifiedTextLength == defaultTextLength)
        m_textLength.value.newValueSpecifiedUnits(LengthTypeNumber, getComputedTextLength(), ASSERT_NO_EXCEPTION);

    m_textLength.shouldSynchronize = true;
    return static_pointer_cast<SVGAnimatedLength>(lookupOrCreateTextLengthWrapper(this));

}

unsigned SVGTextContentElement::getNumberOfChars()
{
    document()->updateLayoutIgnorePendingStylesheets();
    return SVGTextQuery(renderer()).numberOfCharacters();
}

float SVGTextContentElement::getComputedTextLength()
{
    document()->updateLayoutIgnorePendingStylesheets();
    return SVGTextQuery(renderer()).textLength();
}

float SVGTextContentElement::getSubStringLength(unsigned charnum, unsigned nchars, ExceptionCode& ec)
{
    document()->updateLayoutIgnorePendingStylesheets();

    unsigned numberOfChars = getNumberOfChars();
    if (charnum >= numberOfChars) {
        ec = INDEX_SIZE_ERR;
        return 0.0f;
    }

    return SVGTextQuery(renderer()).subStringLength(charnum, nchars);
}

SVGPoint SVGTextContentElement::getStartPositionOfChar(unsigned charnum, ExceptionCode& ec)
{
    document()->updateLayoutIgnorePendingStylesheets();

    if (charnum > getNumberOfChars()) {
        ec = INDEX_SIZE_ERR;
        return SVGPoint();
    }

    return SVGTextQuery(renderer()).startPositionOfCharacter(charnum);
}

SVGPoint SVGTextContentElement::getEndPositionOfChar(unsigned charnum, ExceptionCode& ec)
{
    document()->updateLayoutIgnorePendingStylesheets();

    if (charnum > getNumberOfChars()) {
        ec = INDEX_SIZE_ERR;
        return SVGPoint();
    }

    return SVGTextQuery(renderer()).endPositionOfCharacter(charnum);
}

FloatRect SVGTextContentElement::getExtentOfChar(unsigned charnum, ExceptionCode& ec)
{
    document()->updateLayoutIgnorePendingStylesheets();

    if (charnum > getNumberOfChars()) {
        ec = INDEX_SIZE_ERR;
        return FloatRect();
    }

    return SVGTextQuery(renderer()).extentOfCharacter(charnum);
}

float SVGTextContentElement::getRotationOfChar(unsigned charnum, ExceptionCode& ec)
{
    document()->updateLayoutIgnorePendingStylesheets();

    if (charnum > getNumberOfChars()) {
        ec = INDEX_SIZE_ERR;
        return 0.0f;
    }

    return SVGTextQuery(renderer()).rotationOfCharacter(charnum);
}

int SVGTextContentElement::getCharNumAtPosition(const SVGPoint& point)
{
    document()->updateLayoutIgnorePendingStylesheets();
    return SVGTextQuery(renderer()).characterNumberAtPosition(point);
}

void SVGTextContentElement::selectSubString(unsigned charnum, unsigned nchars, ExceptionCode& ec)
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

    FrameSelection* selection = document()->frame()->selection();
    if (!selection)
        return;

    // Find selection start
    VisiblePosition start(firstPositionInNode(const_cast<SVGTextContentElement*>(this)));
    for (unsigned i = 0; i < charnum; ++i)
        start = start.next();

    // Find selection end
    VisiblePosition end(start);
    for (unsigned i = 0; i < nchars; ++i)
        end = end.next();

    selection->setSelection(VisibleSelection(start, end));
}

bool SVGTextContentElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        SVGLangSpace::addSupportedAttributes(supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(supportedAttributes);
        supportedAttributes.add(SVGNames::lengthAdjustAttr);
        supportedAttributes.add(SVGNames::textLengthAttr);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

bool SVGTextContentElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (name.matches(XMLNames::spaceAttr))
        return true;
    return SVGGraphicsElement::isPresentationAttribute(name);
}

void SVGTextContentElement::collectStyleForPresentationAttribute(const QualifiedName& name, const AtomicString& value, MutableStylePropertySet* style)
{
    if (!isSupportedAttribute(name))
        SVGGraphicsElement::collectStyleForPresentationAttribute(name, value, style);
    else if (name.matches(XMLNames::spaceAttr)) {
        DEFINE_STATIC_LOCAL(const AtomicString, preserveString, ("preserve", AtomicString::ConstructFromLiteral));

        if (value == preserveString)
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWhiteSpace, CSSValuePre);
        else
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWhiteSpace, CSSValueNowrap);
    }
}

void SVGTextContentElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    SVGParsingError parseError = NoError;

    if (!isSupportedAttribute(name))
        SVGGraphicsElement::parseAttribute(name, value);
    else if (name == SVGNames::lengthAdjustAttr) {
        SVGLengthAdjustType propertyValue = SVGPropertyTraits<SVGLengthAdjustType>::fromString(value);
        if (propertyValue > 0)
            setLengthAdjustBaseValue(propertyValue);
    } else if (name == SVGNames::textLengthAttr) {
        m_textLength.value = SVGLength::construct(LengthModeOther, value, parseError, ForbidNegativeLengths);
    } else if (SVGExternalResourcesRequired::parseAttribute(name, value)) {
    } else if (SVGLangSpace::parseAttribute(name, value)) {
    } else
        ASSERT_NOT_REACHED();

    reportAttributeParsingError(parseError, name, value);
}

void SVGTextContentElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGGraphicsElement::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);

    if (attrName == SVGNames::textLengthAttr)
        m_specifiedTextLength = m_textLength.value;

    if (RenderObject* renderer = this->renderer())
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
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

    SVGElement* element = toSVGElement(renderer->node());
    ASSERT(element);

    if (!element->isTextContent())
        return 0;

    return toSVGTextContentElement(element);
}

}

#endif // ENABLE(SVG)
