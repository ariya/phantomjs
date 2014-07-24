/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2010 Rob Buis <buis@kde.org>
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
#include "SVGTSpanElement.h"

#include "NodeRenderingContext.h"
#include "RenderInline.h"
#include "RenderSVGTSpan.h"
#include "SVGNames.h"

namespace WebCore {

inline SVGTSpanElement::SVGTSpanElement(const QualifiedName& tagName, Document* document)
    : SVGTextPositioningElement(tagName, document)
{
    ASSERT(hasTagName(SVGNames::tspanTag));
}

PassRefPtr<SVGTSpanElement> SVGTSpanElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGTSpanElement(tagName, document));
}

RenderObject* SVGTSpanElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSVGTSpan(this);
}

bool SVGTSpanElement::childShouldCreateRenderer(const NodeRenderingContext& childContext) const
{
    if (childContext.node()->isTextNode()
        || childContext.node()->hasTagName(SVGNames::aTag)
#if ENABLE(SVG_FONTS)
        || childContext.node()->hasTagName(SVGNames::altGlyphTag)
#endif
        || childContext.node()->hasTagName(SVGNames::trefTag)
        || childContext.node()->hasTagName(SVGNames::tspanTag))
        return true;

    return false;
}

bool SVGTSpanElement::rendererIsNeeded(const NodeRenderingContext& context)
{
    if (parentNode()
        && (parentNode()->hasTagName(SVGNames::aTag)
#if ENABLE(SVG_FONTS)
            || parentNode()->hasTagName(SVGNames::altGlyphTag)
#endif
            || parentNode()->hasTagName(SVGNames::textTag)
            || parentNode()->hasTagName(SVGNames::textPathTag)
            || parentNode()->hasTagName(SVGNames::tspanTag)))
        return StyledElement::rendererIsNeeded(context);

    return false;
}

}

#endif // ENABLE(SVG)
