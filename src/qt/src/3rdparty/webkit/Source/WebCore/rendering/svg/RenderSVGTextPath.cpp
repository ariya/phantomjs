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
#include "SVGPathElement.h"
#include "SVGRootInlineBox.h"
#include "SVGTextPathElement.h"
#include "SVGTransformList.h"

namespace WebCore {

RenderSVGTextPath::RenderSVGTextPath(Node* n)
    : RenderSVGInline(n)
    , m_startOffset(0.0f)
    , m_exactAlignment(true)
    , m_stretchMethod(false)
{
}

Path RenderSVGTextPath::layoutPath() const
{
    SVGTextPathElement* textPathElement = static_cast<SVGTextPathElement*>(node());
        String pathId = SVGURIReference::getTarget(textPathElement->href());
    Element* targetElement = textPathElement->treeScope()->getElementById(pathId);    
    if (!targetElement || !targetElement->hasTagName(SVGNames::pathTag))
        return Path();
    
    SVGPathElement* pathElement = static_cast<SVGPathElement*>(targetElement);
    
    Path pathData;
    pathElement->toPathData(pathData);
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
    return static_cast<SVGTextPathElement*>(node())->spacing() == SVG_TEXTPATH_SPACINGTYPE_EXACT;
}

bool RenderSVGTextPath::stretchMethod() const
{
    return static_cast<SVGTextPathElement*>(node())->method() == SVG_TEXTPATH_METHODTYPE_STRETCH;
}

}

#endif // ENABLE(SVG)
