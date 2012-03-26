/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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
#include "SVGColor.h"

#include "CSSParser.h"
#include "RGBColor.h"
#include "SVGException.h"

namespace WebCore {

SVGColor::SVGColor(const SVGColorType& colorType)
    : m_colorType(colorType)
{
}

PassRefPtr<RGBColor> SVGColor::rgbColor() const
{
    return RGBColor::create(m_color.rgb());
}

void SVGColor::setRGBColor(const String& rgbColor, ExceptionCode& ec)
{
    Color color = SVGColor::colorFromRGBColorString(rgbColor);
    if (!color.isValid()) {
        ec = SVGException::SVG_INVALID_VALUE_ERR;
        return;
    }

    m_color = color;
    m_colorType = SVG_COLORTYPE_RGBCOLOR;
    setNeedsStyleRecalc();
}

Color SVGColor::colorFromRGBColorString(const String& colorString)
{
    // FIXME: Rework css parser so it is more SVG aware.
    RGBA32 color;
    if (CSSParser::parseColor(color, colorString.stripWhiteSpace()))
        return color;
    return Color();
}

void SVGColor::setRGBColorICCColor(const String& rgbColor, const String& iccColor, ExceptionCode& ec)
{
    if (rgbColor.isEmpty() || iccColor.isEmpty()) {
        ec = SVGException::SVG_INVALID_VALUE_ERR;
        return;
    }

    // FIXME: No support for ICC colors. We're just ignoring it.
    setRGBColor(rgbColor, ec);
    if (ec)
        return;

    m_colorType = SVG_COLORTYPE_RGBCOLOR_ICCCOLOR;
    setNeedsStyleRecalc();
}

void SVGColor::setColor(unsigned short colorType, const String& rgbColor, const String& iccColor, ExceptionCode& ec)
{
    if (colorType > SVG_COLORTYPE_CURRENTCOLOR) {
        ec = SVGException::SVG_WRONG_TYPE_ERR;
        return;
    }

    bool requiresRGBColor = false;
    bool requiresICCColor = false;

    SVGColorType type = static_cast<SVGColorType>(colorType);
    switch (type) {
    case SVG_COLORTYPE_UNKNOWN:
        // Spec: It is invalid to attempt to define a new value of this type or to attempt to switch an existing value to this type.
        ec = SVGException::SVG_INVALID_VALUE_ERR;
        return;
    case SVG_COLORTYPE_RGBCOLOR_ICCCOLOR:
        requiresICCColor = true;
    case SVG_COLORTYPE_RGBCOLOR:
        requiresRGBColor = true;
        break;
    case SVG_COLORTYPE_CURRENTCOLOR:
        break;
    }

    // Spec: If colorType requires an RGBColor, then rgbColor must be a string that matches <color>; otherwise, rgbColor must be null.
    if (requiresRGBColor && rgbColor.isEmpty()) {
        ec = SVGException::SVG_INVALID_VALUE_ERR;
        return;
    }

    // Spec: If colorType requires an SVGICCColor, then iccColor must be a string that matches <icccolor>; otherwise, iccColor must be null.
    if (requiresICCColor && iccColor.isEmpty()) {
        ec = SVGException::SVG_INVALID_VALUE_ERR;
        return;
    }

    setNeedsStyleRecalc();
    m_colorType = type;
    if (!requiresRGBColor) {
        ASSERT(!requiresICCColor);
        m_color = Color();
        return;
    }

    if (requiresICCColor)
        setRGBColorICCColor(rgbColor, iccColor, ec);
    else
        setRGBColor(rgbColor, ec);
}

String SVGColor::cssText() const
{
    switch (m_colorType) {
    case SVG_COLORTYPE_UNKNOWN:
        return String();
    case SVG_COLORTYPE_RGBCOLOR_ICCCOLOR:
    case SVG_COLORTYPE_RGBCOLOR:
        // FIXME: No ICC color support.
        return m_color.serialized();
    case SVG_COLORTYPE_CURRENTCOLOR:
        if (m_color.isValid())
            return m_color.serialized();
        return "currentColor";
    }

    ASSERT_NOT_REACHED();
    return String();
}

}

#endif // ENABLE(SVG)
