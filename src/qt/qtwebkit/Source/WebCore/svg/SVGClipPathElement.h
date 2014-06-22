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

#ifndef SVGClipPathElement_h
#define SVGClipPathElement_h

#if ENABLE(SVG)
#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedEnumeration.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGGraphicsElement.h"
#include "SVGUnitTypes.h"

namespace WebCore {

class RenderObject;

class SVGClipPathElement FINAL : public SVGGraphicsElement,
                                 public SVGExternalResourcesRequired {
public:
    static PassRefPtr<SVGClipPathElement> create(const QualifiedName&, Document*);

private:
    SVGClipPathElement(const QualifiedName&, Document*);

    virtual bool isValid() const { return SVGTests::isValid(); }
    virtual bool needsPendingResourceHandling() const { return false; }

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGClipPathElement)
        DECLARE_ANIMATED_ENUMERATION(ClipPathUnits, clipPathUnits, SVGUnitTypes::SVGUnitType)
        DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)
    END_DECLARE_ANIMATED_PROPERTIES
};

}

#endif
#endif
