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

#ifndef SVGDefsElement_h
#define SVGDefsElement_h

#if ENABLE(SVG)
#include "SVGAnimatedBoolean.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGGraphicsElement.h"

namespace WebCore {

class SVGDefsElement FINAL : public SVGGraphicsElement,
                             public SVGExternalResourcesRequired {
public:
    static PassRefPtr<SVGDefsElement> create(const QualifiedName&, Document*);

private:
    SVGDefsElement(const QualifiedName&, Document*);

    virtual bool isValid() const;

    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGDefsElement)
        DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)
    END_DECLARE_ANIMATED_PROPERTIES
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
