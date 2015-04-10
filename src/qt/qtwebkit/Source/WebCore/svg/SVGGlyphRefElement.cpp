/*
 * Copyright (C) 2011 Leo Yang <leoyang@webkit.org>
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

#if ENABLE(SVG) && ENABLE(SVG_FONTS)
#include "SVGGlyphRefElement.h"

#include "SVGGlyphElement.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"
#include "XLinkNames.h"
#include <wtf/text/AtomicString.h>

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGGlyphRefElement, XLinkNames::hrefAttr, Href, href)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGGlyphRefElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(href)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGStyledElement)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGGlyphRefElement::SVGGlyphRefElement(const QualifiedName& tagName, Document* document)
    : SVGStyledElement(tagName, document)
    , m_x(0)
    , m_y(0)
    , m_dx(0)
    , m_dy(0)
{
    ASSERT(hasTagName(SVGNames::glyphRefTag));
    registerAnimatedPropertiesForSVGGlyphRefElement();
}

PassRefPtr<SVGGlyphRefElement> SVGGlyphRefElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGGlyphRefElement(tagName, document));
}

bool SVGGlyphRefElement::hasValidGlyphElement(String& glyphName) const
{
    // FIXME: We only support xlink:href so far.
    // https://bugs.webkit.org/show_bug.cgi?id=64787
    Element* element = targetElementFromIRIString(getAttribute(XLinkNames::hrefAttr), document(), &glyphName);
    if (!element || !element->hasTagName(SVGNames::glyphTag))
        return false;
    return true;
}

void SVGGlyphRefElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    const UChar* startPtr = value.characters();
    const UChar* endPtr = startPtr + value.length();

    // FIXME: We need some error handling here.
    if (name == SVGNames::xAttr)
        parseNumber(startPtr, endPtr, m_x);
    else if (name == SVGNames::yAttr)
        parseNumber(startPtr, endPtr, m_y);
    else if (name == SVGNames::dxAttr)
        parseNumber(startPtr, endPtr, m_dx);
    else if (name == SVGNames::dyAttr)
        parseNumber(startPtr, endPtr, m_dy);
    else {
        if (SVGURIReference::parseAttribute(name, value))
            return;
        SVGStyledElement::parseAttribute(name, value);
    }
}

const AtomicString& SVGGlyphRefElement::glyphRef() const
{
    return fastGetAttribute(SVGNames::glyphRefAttr);
}

void SVGGlyphRefElement::setGlyphRef(const AtomicString&, ExceptionCode&)
{
    // FIXME: Set and honor attribute change.
    // https://bugs.webkit.org/show_bug.cgi?id=64787
}

void SVGGlyphRefElement::setX(float x, ExceptionCode&)
{
    // FIXME: Honor attribute change.
    // https://bugs.webkit.org/show_bug.cgi?id=64787
    m_x = x;
}

void SVGGlyphRefElement::setY(float y , ExceptionCode&)
{
    // FIXME: Honor attribute change.
    // https://bugs.webkit.org/show_bug.cgi?id=64787
    m_y = y;
}

void SVGGlyphRefElement::setDx(float dx, ExceptionCode&)
{
    // FIXME: Honor attribute change.
    // https://bugs.webkit.org/show_bug.cgi?id=64787
    m_dx = dx;
}

void SVGGlyphRefElement::setDy(float dy, ExceptionCode&)
{
    // FIXME: Honor attribute change.
    // https://bugs.webkit.org/show_bug.cgi?id=64787
    m_dy = dy;
}

}

#endif
