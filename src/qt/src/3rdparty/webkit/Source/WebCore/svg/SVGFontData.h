/*
 * Copyright (C) 2008 Nikolas Zimmermann <zimmermann@kde.org>
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

#ifndef SVGFontData_h
#define SVGFontData_h

#if ENABLE(SVG_FONTS)
#include "SVGFontFaceElement.h"

namespace WebCore {

class SVGFontData {
    WTF_MAKE_NONCOPYABLE(SVGFontData); WTF_MAKE_FAST_ALLOCATED;
public:
    SVGFontData(SVGFontFaceElement*);
    virtual ~SVGFontData() { }

    SVGFontFaceElement* svgFontFaceElement() const { return m_svgFontFaceElement; }

    float horizontalOriginX() const { return m_horizontalOriginX; }
    float horizontalOriginY() const { return m_horizontalOriginY; }
    float horizontalAdvanceX() const { return m_horizontalAdvanceX; }

    float verticalOriginX() const { return m_verticalOriginX; }
    float verticalOriginY() const { return m_verticalOriginY; }
    float verticalAdvanceY() const { return m_verticalAdvanceY; }

private:
    // Ths SVGFontFaceElement is kept alive --
    // 1) in the external font case: by the CSSFontFaceSource, which holds a reference to the external SVG document
    //    containing the element;
    // 2) in the in-document font case: by virtue of being in the document tree and making sure that when it is removed
    //    from the document, it removes the @font-face rule it owns from the document's mapped element sheet and forces
    //    a style update.
    SVGFontFaceElement* m_svgFontFaceElement;

    float m_horizontalOriginX;
    float m_horizontalOriginY;
    float m_horizontalAdvanceX;

    float m_verticalOriginX;
    float m_verticalOriginY;
    float m_verticalAdvanceY;
};

} // namespace WebCore

#endif
#endif // SVGFontData_h
