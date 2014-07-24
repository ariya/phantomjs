/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) Research In Motion Limited 2010-2011. All rights reserved.
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

#if ENABLE(SVG)
#include "SVGPaint.h"

#include "SVGException.h"
#include "SVGURIReference.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

static inline SVGColor::SVGColorType colorTypeForPaintType(const SVGPaint::SVGPaintType& paintType)
{
    switch (paintType) {
    case SVGPaint::SVG_PAINTTYPE_NONE:
    case SVGPaint::SVG_PAINTTYPE_UNKNOWN:
    case SVGPaint::SVG_PAINTTYPE_URI:
    case SVGPaint::SVG_PAINTTYPE_URI_NONE:
        return SVGColor::SVG_COLORTYPE_UNKNOWN;
    case SVGPaint::SVG_PAINTTYPE_URI_RGBCOLOR:
    case SVGPaint::SVG_PAINTTYPE_RGBCOLOR:
        return SVGColor::SVG_COLORTYPE_RGBCOLOR;
    case SVGPaint::SVG_PAINTTYPE_URI_RGBCOLOR_ICCCOLOR:
    case SVGPaint::SVG_PAINTTYPE_RGBCOLOR_ICCCOLOR:
        return SVGColor::SVG_COLORTYPE_RGBCOLOR_ICCCOLOR;
    case SVGPaint::SVG_PAINTTYPE_URI_CURRENTCOLOR:
    case SVGPaint::SVG_PAINTTYPE_CURRENTCOLOR:
        return SVGColor::SVG_COLORTYPE_CURRENTCOLOR;
    }

    ASSERT_NOT_REACHED();
    return SVGColor::SVG_COLORTYPE_UNKNOWN;
}

SVGPaint::SVGPaint(const SVGPaintType& paintType, const String& uri)
    : SVGColor(SVGPaintClass, colorTypeForPaintType(paintType))
    , m_paintType(paintType)
    , m_uri(uri)
{
}

void SVGPaint::setUri(const String&)
{
    // The whole SVGPaint interface is deprecated in SVG 1.1 (2nd edition).
    // The setters are the most problematic part so we remove the support for those first.
}

void SVGPaint::setPaint(unsigned short, const String&, const String&, const String&, ExceptionCode& ec)
{
    ec = NO_MODIFICATION_ALLOWED_ERR;
}

String SVGPaint::customCssText() const
{
    switch (m_paintType) {
    case SVG_PAINTTYPE_UNKNOWN:
    case SVG_PAINTTYPE_RGBCOLOR:
    case SVG_PAINTTYPE_RGBCOLOR_ICCCOLOR:
    case SVG_PAINTTYPE_CURRENTCOLOR:
        return SVGColor::customCssText();
    case SVG_PAINTTYPE_NONE:
        return "none";
    case SVG_PAINTTYPE_URI_NONE:
        return m_uri + " none";
    case SVG_PAINTTYPE_URI_CURRENTCOLOR:
    case SVG_PAINTTYPE_URI_RGBCOLOR:
    case SVG_PAINTTYPE_URI_RGBCOLOR_ICCCOLOR: {
        String color = SVGColor::customCssText();
        if (color.isEmpty())
            return m_uri;
        return m_uri + ' ' + color;
    }
    case SVG_PAINTTYPE_URI:
        return m_uri;
    };

    ASSERT_NOT_REACHED();
    return String();
}

SVGPaint::SVGPaint(const SVGPaint& cloneFrom)
    : SVGColor(SVGPaintClass, cloneFrom)
    , m_paintType(cloneFrom.m_paintType)
    , m_uri(cloneFrom.m_uri)
{
}

PassRefPtr<SVGPaint> SVGPaint::cloneForCSSOM() const
{
    return adoptRef(new SVGPaint(*this));
}

bool SVGPaint::equals(const SVGPaint& other) const
{
    return m_paintType == other.m_paintType && m_uri == other.m_uri && SVGColor::equals(other);
}

}

#endif // ENABLE(SVG)
