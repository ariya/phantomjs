/*
 * Copyright (C) 2007, 2010 Rob Buis <buis@kde.org>
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
#include "SVGViewSpec.h"

#include "Document.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"
#include "SVGSVGElement.h"
#include "SVGTransformable.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_RECT(SVGViewSpec, SVGNames::viewBoxAttr, ViewBox, viewBox)
DEFINE_ANIMATED_PRESERVEASPECTRATIO(SVGViewSpec, SVGNames::preserveAspectRatioAttr, PreserveAspectRatio, preserveAspectRatio)

SVGViewSpec::SVGViewSpec(SVGElement* contextElement)
    : m_contextElement(contextElement)
{
}

void SVGViewSpec::setTransform(const String& transform)
{
    SVGTransformable::parseTransformAttribute(m_transform, transform);
}

void SVGViewSpec::setViewBoxString(const String& viewBoxStr)
{
    FloatRect viewBox;
    const UChar* c = viewBoxStr.characters();
    const UChar* end = c + viewBoxStr.length();
    if (!parseViewBox(m_contextElement->document(), c, end, viewBox, false))
         return;
    setViewBoxBaseValue(viewBox);
}

void SVGViewSpec::setPreserveAspectRatioString(const String& preserve)
{
    SVGPreserveAspectRatio::parsePreserveAspectRatio(this, preserve);
}

void SVGViewSpec::setViewTargetString(const String& viewTargetString)
{
    m_viewTargetString = viewTargetString;
}

SVGElement* SVGViewSpec::viewTarget() const
{
    return static_cast<SVGElement*>(m_contextElement->treeScope()->getElementById(m_viewTargetString));
}

static const UChar svgViewSpec[] = {'s', 'v', 'g', 'V', 'i', 'e', 'w'};
static const UChar viewBoxSpec[] = {'v', 'i', 'e', 'w', 'B', 'o', 'x'};
static const UChar preserveAspectRatioSpec[] = {'p', 'r', 'e', 's', 'e', 'r', 'v', 'e', 'A', 's', 'p', 'e', 'c', 't', 'R', 'a', 't', 'i', 'o'};
static const UChar transformSpec[] = {'t', 'r', 'a', 'n', 's', 'f', 'o', 'r', 'm'};
static const UChar zoomAndPanSpec[] = {'z', 'o', 'o', 'm', 'A', 'n', 'd', 'P', 'a', 'n'};
static const UChar viewTargetSpec[] =  {'v', 'i', 'e', 'w', 'T', 'a', 'r', 'g', 'e', 't'};

bool SVGViewSpec::parseViewSpec(const String& viewSpec)
{
    const UChar* currViewSpec = viewSpec.characters();
    const UChar* end = currViewSpec + viewSpec.length();

    if (currViewSpec >= end)
        return false;

    if (!skipString(currViewSpec, end, svgViewSpec, WTF_ARRAY_LENGTH(svgViewSpec)))
        return false;

    if (currViewSpec >= end || *currViewSpec != '(')
        return false;
    currViewSpec++;

    while (currViewSpec < end && *currViewSpec != ')') {
        if (*currViewSpec == 'v') {
            if (skipString(currViewSpec, end, viewBoxSpec, WTF_ARRAY_LENGTH(viewBoxSpec))) {
                if (currViewSpec >= end || *currViewSpec != '(')
                    return false;
                currViewSpec++;
                FloatRect viewBox;
                if (!parseViewBox(m_contextElement->document(), currViewSpec, end, viewBox, false))
                    return false;
                setViewBoxBaseValue(viewBox);
                if (currViewSpec >= end || *currViewSpec != ')')
                    return false;
                currViewSpec++;
            } else if (skipString(currViewSpec, end, viewTargetSpec, WTF_ARRAY_LENGTH(viewTargetSpec))) {
                if (currViewSpec >= end || *currViewSpec != '(')
                    return false;
                const UChar* viewTargetStart = ++currViewSpec;
                while (currViewSpec < end && *currViewSpec != ')')
                    currViewSpec++;
                if (currViewSpec >= end)
                    return false;
                setViewTargetString(String(viewTargetStart, currViewSpec - viewTargetStart));
                currViewSpec++;
            } else
                return false;
        } else if (*currViewSpec == 'z') {
            if (!skipString(currViewSpec, end, zoomAndPanSpec, WTF_ARRAY_LENGTH(zoomAndPanSpec)))
                return false;
            if (currViewSpec >= end || *currViewSpec != '(')
                return false;
            currViewSpec++;
            if (!parseZoomAndPan(currViewSpec, end))
                return false;
            if (currViewSpec >= end || *currViewSpec != ')')
                return false;
            currViewSpec++;
        } else if (*currViewSpec == 'p') {
            if (!skipString(currViewSpec, end, preserveAspectRatioSpec, WTF_ARRAY_LENGTH(preserveAspectRatioSpec)))
                return false;
            if (currViewSpec >= end || *currViewSpec != '(')
                return false;
            currViewSpec++;
            bool result = false; 
            setPreserveAspectRatioBaseValue(SVGPreserveAspectRatio::parsePreserveAspectRatio(currViewSpec, end, false, result));
            if (!result)
                return false;
            if (currViewSpec >= end || *currViewSpec != ')')
                return false;
            currViewSpec++;
        } else if (*currViewSpec == 't') {
            if (!skipString(currViewSpec, end, transformSpec, WTF_ARRAY_LENGTH(transformSpec)))
                return false;
            if (currViewSpec >= end || *currViewSpec != '(')
                return false;
            currViewSpec++;
            SVGTransformable::parseTransformAttribute(m_transform, currViewSpec, end, SVGTransformable::DoNotClearList);
            if (currViewSpec >= end || *currViewSpec != ')')
                return false;
            currViewSpec++;
        } else
            return false;

        if (currViewSpec < end && *currViewSpec == ';')
            currViewSpec++;
    }
    
    if (currViewSpec >= end || *currViewSpec != ')')
        return false;

    return true;
}

}

#endif // ENABLE(SVG)
