/*
 * Copyright (C) 2006 Apple Inc. All rights reserved.
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

#ifndef SVGForeignObjectElement_h
#define SVGForeignObjectElement_h

#if ENABLE(SVG) && ENABLE(SVG_FOREIGN_OBJECT)
#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedLength.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTests.h"
#include "SVGURIReference.h"

namespace WebCore {

class SVGForeignObjectElement : public SVGStyledTransformableElement,
                                public SVGTests,
                                public SVGLangSpace,
                                public SVGExternalResourcesRequired {
public:
    static PassRefPtr<SVGForeignObjectElement> create(const QualifiedName&, Document*);

private:
    SVGForeignObjectElement(const QualifiedName&, Document*);

    virtual bool isValid() const { return SVGTests::isValid(); }
    virtual void parseMappedAttribute(Attribute*);
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual void synchronizeProperty(const QualifiedName&);
    virtual void fillAttributeToPropertyTypeMap();
    virtual AttributeToPropertyTypeMap& attributeToPropertyTypeMap();

    virtual bool childShouldCreateRenderer(Node*) const;
    virtual RenderObject* createRenderer(RenderArena* arena, RenderStyle* style);

    virtual bool selfHasRelativeLengths() const;

    // Animated property declarations
    DECLARE_ANIMATED_LENGTH(X, x)
    DECLARE_ANIMATED_LENGTH(Y, y)
    DECLARE_ANIMATED_LENGTH(Width, width)
    DECLARE_ANIMATED_LENGTH(Height, height)

    // SVGURIReference
    DECLARE_ANIMATED_STRING(Href, href)

    // SVGExternalResourcesRequired
    DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)
};

} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(SVG_FOREIGN_OBJECT)
#endif
