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

#ifndef SVGStopElement_h
#define SVGStopElement_h

#if ENABLE(SVG)
#include "SVGAnimatedNumber.h"
#include "SVGStyledElement.h"

namespace WebCore {

class SVGStopElement FINAL : public SVGStyledElement {
public:
    static PassRefPtr<SVGStopElement> create(const QualifiedName&, Document*);

    Color stopColorIncludingOpacity() const;

private:
    SVGStopElement(const QualifiedName&, Document*);

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual void svgAttributeChanged(const QualifiedName&);

    virtual bool isGradientStop() const OVERRIDE { return true; }

    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
    virtual bool rendererIsNeeded(const NodeRenderingContext&) OVERRIDE;

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGStopElement)
        DECLARE_ANIMATED_NUMBER(Offset, offset)
    END_DECLARE_ANIMATED_PROPERTIES
};

inline SVGStopElement* toSVGStopElement(SVGElement* element)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!element || element->isGradientStop());
    return static_cast<SVGStopElement*>(element);
}

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
