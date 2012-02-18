/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
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

#ifndef SVGSymbolElement_h
#define SVGSymbolElement_h

#if ENABLE(SVG)
#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedPreserveAspectRatio.h"
#include "SVGAnimatedRect.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGFitToViewBox.h"
#include "SVGLangSpace.h"
#include "SVGStyledElement.h"

namespace WebCore {

class SVGSymbolElement : public SVGStyledElement,
                         public SVGLangSpace,
                         public SVGExternalResourcesRequired,
                         public SVGFitToViewBox {
public:
    static PassRefPtr<SVGSymbolElement> create(const QualifiedName&, Document*);

private:
    SVGSymbolElement(const QualifiedName&, Document*);

    virtual void parseMappedAttribute(Attribute*);
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual void synchronizeProperty(const QualifiedName&);
    virtual void fillAttributeToPropertyTypeMap();
    virtual AttributeToPropertyTypeMap& attributeToPropertyTypeMap();
    virtual bool rendererIsNeeded(RenderStyle*) { return false; }

    virtual bool selfHasRelativeLengths() const;

    // Animated property declarations

    // SVGExternalResourcesRequired
    DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)
 
    // SVGFitToViewBox
    DECLARE_ANIMATED_RECT(ViewBox, viewBox)
    DECLARE_ANIMATED_PRESERVEASPECTRATIO(PreserveAspectRatio, preserveAspectRatio) 
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
