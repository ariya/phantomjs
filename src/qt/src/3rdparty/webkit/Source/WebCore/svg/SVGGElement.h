/*
 * Copyright (C) 2004, 2005, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
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

#ifndef SVGGElement_h
#define SVGGElement_h

#if ENABLE(SVG)
#include "SVGAnimatedBoolean.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTests.h"

namespace WebCore {

class SVGGElement : public SVGStyledTransformableElement,
                    public SVGTests,
                    public SVGLangSpace,
                    public SVGExternalResourcesRequired {
public:
    static PassRefPtr<SVGGElement> create(const QualifiedName&, Document*);

    virtual bool isShadowTreeContainerElement() const { return false; }

protected:
    SVGGElement(const QualifiedName&, Document*);

    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

private:
    virtual bool isValid() const { return SVGTests::isValid(); }

    virtual void parseMappedAttribute(Attribute*);
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual void synchronizeProperty(const QualifiedName&);
    virtual void fillAttributeToPropertyTypeMap();
    virtual AttributeToPropertyTypeMap& attributeToPropertyTypeMap();

    virtual bool rendererIsNeeded(RenderStyle*);

    // Animated property declarations

    // SVGExternalResourcesRequired
    DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
