/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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
#include "SVGShadowTreeElements.h"

#include "Document.h"
#include "FloatSize.h"
#include "RenderObject.h"
#include "SVGNames.h"
#include "SVGUseElement.h"

namespace WebCore {

// SVGShadowTreeContainerElement

SVGShadowTreeContainerElement::SVGShadowTreeContainerElement(Document* document)
    : SVGGElement(SVGNames::gTag, document)
{
}

PassRefPtr<SVGShadowTreeContainerElement> SVGShadowTreeContainerElement::create(Document* document)
{
    return adoptRef(new SVGShadowTreeContainerElement(document));
}

FloatSize SVGShadowTreeContainerElement::containerTranslation() const
{
    return FloatSize(m_xOffset.value(this), m_yOffset.value(this));
}

PassRefPtr<Element> SVGShadowTreeContainerElement::cloneElementWithoutAttributesAndChildren() const
{
    return adoptRef(new SVGShadowTreeContainerElement(document()));
}
// SVGShadowTreeRootElement

inline SVGShadowTreeRootElement::SVGShadowTreeRootElement(Document* document, SVGUseElement* host)
    : SVGShadowTreeContainerElement(document)
{
    setParent(host);
    setInDocument();
}

PassRefPtr<SVGShadowTreeRootElement> SVGShadowTreeRootElement::create(Document* document, SVGUseElement* host)
{
    return adoptRef(new SVGShadowTreeRootElement(document, host));
}

void SVGShadowTreeRootElement::attachElement(PassRefPtr<RenderStyle> style, RenderArena* arena)
{
    ASSERT(svgShadowHost());

    // Create the renderer with the specified style
    RenderObject* renderer = createRenderer(arena, style.get());
    if (renderer) {
        setRenderer(renderer);
        renderer->setStyle(style);
    }

    // Set these explicitly since this normally happens during an attach()
    setAttached();

    // Add the renderer to the render tree
    if (renderer)
        svgShadowHost()->renderer()->addChild(renderer);
}

void SVGShadowTreeRootElement::clearSVGShadowHost()
{
    setParent(0);
}

}

#endif
