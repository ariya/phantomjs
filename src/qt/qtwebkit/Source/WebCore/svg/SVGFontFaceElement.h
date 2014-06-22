/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
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

#ifndef SVGFontFaceElement_h
#define SVGFontFaceElement_h

#if ENABLE(SVG_FONTS)
#include "SVGElement.h"

namespace WebCore {

class SVGFontElement;
class StyleRuleFontFace;

class SVGFontFaceElement FINAL : public SVGElement {
public:
    static PassRefPtr<SVGFontFaceElement> create(const QualifiedName&, Document*);

    unsigned unitsPerEm() const;
    int xHeight() const;
    float horizontalOriginX() const;
    float horizontalOriginY() const;
    float horizontalAdvanceX() const;
    float verticalOriginX() const;
    float verticalOriginY() const;
    float verticalAdvanceY() const;
    int ascent() const;
    int descent() const;
    String fontFamily() const;

    SVGFontElement* associatedFontElement() const;
    void rebuildFontFace();
    
    StyleRuleFontFace* fontFaceRule() const { return m_fontFaceRule.get(); }

private:
    SVGFontFaceElement(const QualifiedName&, Document*);

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;

    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);
    virtual InsertionNotificationRequest insertedInto(ContainerNode*) OVERRIDE;
    virtual void removedFrom(ContainerNode*) OVERRIDE;

    RefPtr<StyleRuleFontFace> m_fontFaceRule;
    SVGFontElement* m_fontElement;
};

} // namespace WebCore

#endif // ENABLE(SVG_FONTS)
#endif

// vim:ts=4:noet
