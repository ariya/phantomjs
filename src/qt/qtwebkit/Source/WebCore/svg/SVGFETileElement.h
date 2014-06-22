/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
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

#ifndef SVGFETileElement_h
#define SVGFETileElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FETile.h"
#include "SVGFilterPrimitiveStandardAttributes.h"

namespace WebCore {

class SVGFETileElement FINAL : public SVGFilterPrimitiveStandardAttributes {
public:
    static PassRefPtr<SVGFETileElement> create(const QualifiedName&, Document*);

private:
    SVGFETileElement(const QualifiedName&, Document*);

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual PassRefPtr<FilterEffect> build(SVGFilterBuilder*, Filter*);

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGFETileElement)
        DECLARE_ANIMATED_STRING(In1, in1)
    END_DECLARE_ANIMATED_PROPERTIES
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
