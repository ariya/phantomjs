/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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

#ifndef SVGMPathElement_h
#define SVGMPathElement_h

#if ENABLE(SVG)
#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedString.h"
#include "SVGElement.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGURIReference.h"

namespace WebCore {
    
class SVGPathElement;

class SVGMPathElement FINAL : public SVGElement,
                              public SVGURIReference,
                              public SVGExternalResourcesRequired {
public:
    static PassRefPtr<SVGMPathElement> create(const QualifiedName&, Document*);

    virtual ~SVGMPathElement();

    SVGPathElement* pathElement();

    void targetPathChanged();

private:
    SVGMPathElement(const QualifiedName&, Document*);

    void buildPendingResource();
    void clearResourceReferences();
    virtual InsertionNotificationRequest insertedInto(ContainerNode*) OVERRIDE;
    void removedFrom(ContainerNode*);

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual void svgAttributeChanged(const QualifiedName&) OVERRIDE;

    void notifyParentOfPathChange(ContainerNode*);

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGMPathElement)
        DECLARE_ANIMATED_STRING(Href, href)
        DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)
    END_DECLARE_ANIMATED_PROPERTIES
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGMPathElement_h
