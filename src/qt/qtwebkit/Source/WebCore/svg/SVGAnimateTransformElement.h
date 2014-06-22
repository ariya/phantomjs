/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef SVGAnimateTransformElement_h
#define SVGAnimateTransformElement_h

#if ENABLE(SVG)
#include "SVGAnimateElement.h"
#include "SVGTransform.h"

namespace WebCore {

class AffineTransform;

class SVGAnimateTransformElement FINAL : public SVGAnimateElement {
public:
    static PassRefPtr<SVGAnimateTransformElement> create(const QualifiedName&, Document*);

    SVGTransform::SVGTransformType transformType() const { return m_type; }

private:
    SVGAnimateTransformElement(const QualifiedName&, Document*);
    
    virtual bool hasValidAttributeType();

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;

    SVGTransform::SVGTransformType m_type;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGAnimateTransformElement_h
