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

#include "config.h"

#if ENABLE(SVG_FONTS)
#include "SVGFontData.h"

namespace WebCore {

SVGFontData::SVGFontData(SVGFontFaceElement* fontFaceElement)
    : m_svgFontFaceElement(fontFaceElement)
    , m_horizontalOriginX(fontFaceElement->horizontalOriginX())
    , m_horizontalOriginY(fontFaceElement->horizontalOriginY())
    , m_horizontalAdvanceX(fontFaceElement->horizontalAdvanceX())
    , m_verticalOriginX(fontFaceElement->verticalOriginX())
    , m_verticalOriginY(fontFaceElement->verticalOriginY())
    , m_verticalAdvanceY(fontFaceElement->verticalAdvanceY())
{
    ASSERT_ARG(fontFaceElement, fontFaceElement);
}

} // namespace WebCore

#endif
