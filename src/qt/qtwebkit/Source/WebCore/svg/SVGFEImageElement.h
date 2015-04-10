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

#ifndef SVGFEImageElement_h
#define SVGFEImageElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "CachedImageClient.h"
#include "CachedResourceHandle.h"
#include "ImageBuffer.h"
#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedPreserveAspectRatio.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGFEImage.h"
#include "SVGFilterPrimitiveStandardAttributes.h"
#include "SVGURIReference.h"

namespace WebCore {

class SVGFEImageElement FINAL : public SVGFilterPrimitiveStandardAttributes,
                                public SVGURIReference,
                                public SVGExternalResourcesRequired,
                                public CachedImageClient {
public:
    static PassRefPtr<SVGFEImageElement> create(const QualifiedName&, Document*);

    virtual ~SVGFEImageElement();

private:
    SVGFEImageElement(const QualifiedName&, Document*);

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual void notifyFinished(CachedResource*);

    virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;
    virtual PassRefPtr<FilterEffect> build(SVGFilterBuilder*, Filter*);

    void clearResourceReferences();
    void requestImageResource();

    virtual void buildPendingResource();
    virtual InsertionNotificationRequest insertedInto(ContainerNode*) OVERRIDE;
    virtual void removedFrom(ContainerNode*) OVERRIDE;

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGFEImageElement)
        DECLARE_ANIMATED_PRESERVEASPECTRATIO(PreserveAspectRatio, preserveAspectRatio)
        DECLARE_ANIMATED_STRING(Href, href)
        DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)
    END_DECLARE_ANIMATED_PROPERTIES

    CachedResourceHandle<CachedImage> m_cachedImage;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
