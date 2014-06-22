/*
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
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
#include "RenderSVGTextPath.h"

#include "FloatQuad.h"
#include "RenderBlock.h"
#include "SVGInlineTextBox.h"
#include "SVGNames.h"
#include "SVGPathData.h"
#include "SVGPathElement.h"
#include "SVGRootInlineBox.h"
#include "SVGTextPathElement.h"
#include "SVGTransformList.h"

namespace WebCore {

RenderSVGTextPath::RenderSVGTextPath(Element* element)
    : RenderSVGInline(element)
{
}

Path RenderSVGTextPath::layoutPath() const
{
    SVGTextPathElement* textPathElement = static_cast<SVGTextPathElement*>(node());
    Element* targetElement = SVGURIReference::targetElementFromIRIString(textPathElement->href(), textPathElement->document());
    if (!targetElement || !targetElement->hasTagName(SVGNames::pathTag))
        return Path();
    
    SVGPathElement* pathElement = toSVGPathElement(targetElement);
    
    Path pathData;
    updatePathFromGraphicsElement(pathElement, pathData);

    // Spec:  The transform attribute on the referenced 'path' element represents a
    // supplemental transformation relative to the current user coordinate system for
    // the current 'text' element, including any adjustments to the current user coordinate
    // system due to a possible transform attribute on the current 'text' element.
    // http://www.w3.org/TR/SVG/text.html#TextPathElement
    pathData.transform(pathElement->animatedLocalTransform());
    return pathData;
}

float RenderSVGTextPath::startOffset() const
{
    return static_cast<SVGTextPathElement*>(node())->startOffset().valueAsPercentage();
}

bool RenderSVGTextPath::exactAlignment() const
{
    return static_cast<SVGTextPathElement*>(node())->spacing() == SVGTextPathSpacingExact;
}

bool RenderSVGTextPath::stretchMethod() const
{
    return static_cast<SVGTextPathElement*>(node())->method() == SVGTextPathMethodStretch;
}

}

#endif // ENABLE(SVG)
