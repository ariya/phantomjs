/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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

#ifndef SVGComponentTransferFunctionElement_h
#define SVGComponentTransferFunctionElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FEComponentTransfer.h"
#include "SVGAnimatedEnumeration.h"
#include "SVGAnimatedNumber.h"
#include "SVGAnimatedNumberList.h"

namespace WebCore {

class SVGComponentTransferFunctionElement : public SVGElement {
public:
    ComponentTransferFunction transferFunction() const;

protected:
    SVGComponentTransferFunctionElement(const QualifiedName&, Document*);

    virtual void parseMappedAttribute(Attribute*);
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual void synchronizeProperty(const QualifiedName&);
    virtual void fillAttributeToPropertyTypeMap();
    virtual AttributeToPropertyTypeMap& attributeToPropertyTypeMap();
    
private:
    // Animated property declarations
    DECLARE_ANIMATED_ENUMERATION(Type, type)
    DECLARE_ANIMATED_NUMBER_LIST(TableValues, tableValues)
    DECLARE_ANIMATED_NUMBER(Slope, slope)
    DECLARE_ANIMATED_NUMBER(Intercept, intercept)
    DECLARE_ANIMATED_NUMBER(Amplitude, amplitude)
    DECLARE_ANIMATED_NUMBER(Exponent, exponent)
    DECLARE_ANIMATED_NUMBER(Offset, offset)
};

} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(FILTERS)
#endif
