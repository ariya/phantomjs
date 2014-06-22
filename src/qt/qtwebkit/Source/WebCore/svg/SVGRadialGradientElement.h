/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
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

#ifndef SVGRadialGradientElement_h
#define SVGRadialGradientElement_h

#if ENABLE(SVG)
#include "SVGAnimatedLength.h"
#include "SVGGradientElement.h"

namespace WebCore {

struct RadialGradientAttributes;

class SVGRadialGradientElement FINAL : public SVGGradientElement {
public:
    static PassRefPtr<SVGRadialGradientElement> create(const QualifiedName&, Document*);

    bool collectGradientAttributes(RadialGradientAttributes&);

private:
    SVGRadialGradientElement(const QualifiedName&, Document*);

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual void svgAttributeChanged(const QualifiedName&);

    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

    virtual bool selfHasRelativeLengths() const;

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGRadialGradientElement)
        DECLARE_ANIMATED_LENGTH(Cx, cx)
        DECLARE_ANIMATED_LENGTH(Cy, cy)
        DECLARE_ANIMATED_LENGTH(R, r)
        DECLARE_ANIMATED_LENGTH(Fx, fx)
        DECLARE_ANIMATED_LENGTH(Fy, fy)
        DECLARE_ANIMATED_LENGTH(Fr, fr)
    END_DECLARE_ANIMATED_PROPERTIES
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
