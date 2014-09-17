/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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

#ifndef SVGAltGlyphElement_h
#define SVGAltGlyphElement_h

#if ENABLE(SVG_FONTS)
#include "SVGTextPositioningElement.h"
#include "SVGURIReference.h"

namespace WebCore {

class SVGGlyphElement;

class SVGAltGlyphElement : public SVGTextPositioningElement,
                           public SVGURIReference {
public:
    static PassRefPtr<SVGAltGlyphElement> create(const QualifiedName&, Document*);

    const AtomicString& glyphRef() const;
    void setGlyphRef(const AtomicString&, ExceptionCode&);
    const AtomicString& format() const;
    void setFormat(const AtomicString&, ExceptionCode&);

    SVGGlyphElement* glyphElement() const;

private:
    SVGAltGlyphElement(const QualifiedName&, Document*);

    virtual void synchronizeProperty(const QualifiedName&);

    virtual void fillAttributeToPropertyTypeMap();
    virtual AttributeToPropertyTypeMap& attributeToPropertyTypeMap();

    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
    virtual bool childShouldCreateRenderer(Node*) const;

    // Animated property declarations

    // SVGURIReference
    DECLARE_ANIMATED_STRING(Href, href)
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
