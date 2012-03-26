/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2008 Rob Buis <buis@kde.org>
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

#ifndef SVGTextPositioningElement_h
#define SVGTextPositioningElement_h

#if ENABLE(SVG)
#include "SVGAnimatedLengthList.h"
#include "SVGAnimatedNumberList.h"
#include "SVGTextContentElement.h"

namespace WebCore {

class SVGTextPositioningElement : public SVGTextContentElement {
public:
    static SVGTextPositioningElement* elementFromRenderer(RenderObject*);

protected:
    SVGTextPositioningElement(const QualifiedName&, Document*);

    virtual void parseMappedAttribute(Attribute*);
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual void synchronizeProperty(const QualifiedName&);
    void fillPassedAttributeToPropertyTypeMap(AttributeToPropertyTypeMap&);

    // Animated property declarations
    DECLARE_ANIMATED_LENGTH_LIST(X, x)
    DECLARE_ANIMATED_LENGTH_LIST(Y, y)
    DECLARE_ANIMATED_LENGTH_LIST(Dx, dx)
    DECLARE_ANIMATED_LENGTH_LIST(Dy, dy)
    DECLARE_ANIMATED_NUMBER_LIST(Rotate, rotate)
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
