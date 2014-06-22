/*
 * Copyright (C) 2005 Alexander Kellett <lypanov@kde.org>
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

#ifndef SVGMaskElement_h
#define SVGMaskElement_h

#if ENABLE(SVG)
#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedEnumeration.h"
#include "SVGAnimatedLength.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGStyledElement.h"
#include "SVGTests.h"
#include "SVGUnitTypes.h"

namespace WebCore {

class SVGMaskElement FINAL : public SVGStyledElement,
                             public SVGTests,
                             public SVGExternalResourcesRequired {
public:
    static PassRefPtr<SVGMaskElement> create(const QualifiedName&, Document*);

private:
    SVGMaskElement(const QualifiedName&, Document*);

    virtual bool isValid() const { return SVGTests::isValid(); }
    virtual bool needsPendingResourceHandling() const { return false; }

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

    virtual bool selfHasRelativeLengths() const;

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGMaskElement)
        DECLARE_ANIMATED_ENUMERATION(MaskUnits, maskUnits, SVGUnitTypes::SVGUnitType)
        DECLARE_ANIMATED_ENUMERATION(MaskContentUnits, maskContentUnits, SVGUnitTypes::SVGUnitType)
        DECLARE_ANIMATED_LENGTH(X, x)
        DECLARE_ANIMATED_LENGTH(Y, y)
        DECLARE_ANIMATED_LENGTH(Width, width)
        DECLARE_ANIMATED_LENGTH(Height, height)
        DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)
    END_DECLARE_ANIMATED_PROPERTIES

    // SVGTests
    virtual void synchronizeRequiredFeatures() { SVGTests::synchronizeRequiredFeatures(this); }
    virtual void synchronizeRequiredExtensions() { SVGTests::synchronizeRequiredExtensions(this); }
    virtual void synchronizeSystemLanguage() { SVGTests::synchronizeSystemLanguage(this); }
};

}

#endif
#endif
