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
#include <wtf/text/StringConcatenate.h>

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

SVGPaint::SVGPaint(const SVGPaintType& paintType, String uri)
    : SVGColor(colorTypeForPaintType(paintType))
    , m_paintType(paintType)
    , m_uri(uri)
{
}

void SVGPaint::setUri(const String& uri)
{
    // Spec: Sets the paintType to SVG_PAINTTYPE_URI_NONE and sets uri to the specified value.
    m_uri = uri;
    m_paintType = SVG_PAINTTYPE_URI_NONE;
    setColor(Color());
    setColorType(colorTypeForPaintType(m_paintType));
    setNeedsStyleRecalc();
}

void SVGPaint::setPaint(unsigned short paintType, const String& uri, const String& rgbColor, const String& iccColor, ExceptionCode& ec)
{
    if ((paintType > SVG_PAINTTYPE_RGBCOLOR_ICCCOLOR && paintType < SVG_PAINTTYPE_NONE) || paintType > SVG_PAINTTYPE_URI) {
        ec = SVGException::SVG_WRONG_TYPE_ERR;
        return;
    }

    bool requiresURI = false;

    SVGPaintType type = static_cast<SVGPaintType>(paintType);
    switch (type) {
    case SVG_PAINTTYPE_UNKNOWN:
        // Spec: It is invalid to attempt to define a new value of this type or to attempt to switch an existing value to this type.
        ec = SVGException::SVG_INVALID_VALUE_ERR;
        return;
    case SVG_PAINTTYPE_RGBCOLOR:
    case SVG_PAINTTYPE_RGBCOLOR_ICCCOLOR:
    case SVG_PAINTTYPE_NONE:
    case SVG_PAINTTYPE_CURRENTCOLOR:
        break;
    case SVG_PAINTTYPE_URI_NONE:
    case SVG_PAINTTYPE_URI_CURRENTCOLOR:
    case SVG_PAINTTYPE_URI_RGBCOLOR:
    case SVG_PAINTTYPE_URI_RGBCOLOR_ICCCOLOR:
    case SVG_PAINTTYPE_URI:
        requiresURI = true;
        break;
    };

    // Spec: If paintType requires a URI, then uri must be non-null; otherwise, uri must be null.
    if (requiresURI && uri.isEmpty()) { 
        ec = SVGException::SVG_INVALID_VALUE_ERR;
        return;
    }

    SVGColor::SVGColorType colorType = colorTypeForPaintType(type);
    if (colorType == SVGColor::SVG_COLORTYPE_UNKNOWN) {
        // The standard setColor() code path used in the else branch
        // raises an exception when attempting to switch to an unknown color type.
        // Here we explicitely want to reset to Color() and an unknown type, so force it.
        setColorType(colorType);
        setColor(Color());
    } else {
        setColor(colorType, rgbColor, iccColor, ec);
        if (ec)
            return;
    }

    m_paintType = type;
    m_uri = requiresURI ? uri : String();
    setNeedsStyleRecalc();
}

String SVGPaint::cssText() const
{
    switch (m_paintType) {
    case SVG_PAINTTYPE_UNKNOWN:
    case SVG_PAINTTYPE_RGBCOLOR:
    case SVG_PAINTTYPE_RGBCOLOR_ICCCOLOR:
    case SVG_PAINTTYPE_CURRENTCOLOR:
        return SVGColor::cssText();
    case SVG_PAINTTYPE_NONE:
        return "none";
    case SVG_PAINTTYPE_URI_NONE:
        return makeString(m_uri, " none");
    case SVG_PAINTTYPE_URI_CURRENTCOLOR:
    case SVG_PAINTTYPE_URI_RGBCOLOR:
    case SVG_PAINTTYPE_URI_RGBCOLOR_ICCCOLOR: {
        String color = SVGColor::cssText();
        if (color.isEmpty())
            return m_uri;
        return makeString(m_uri, ' ', color);
    }
    case SVG_PAINTTYPE_URI:
        return m_uri;
    };

    ASSERT_NOT_REACHED();
    return String();
}

bool SVGPaint::matchesTargetURI(const String& referenceId)
{
    switch (m_paintType) {
    case SVG_PAINTTYPE_UNKNOWN:
    case SVG_PAINTTYPE_RGBCOLOR:
    case SVG_PAINTTYPE_RGBCOLOR_ICCCOLOR:
    case SVG_PAINTTYPE_CURRENTCOLOR:
    case SVG_PAINTTYPE_NONE:
        return false;
    case SVG_PAINTTYPE_URI_NONE:
    case SVG_PAINTTYPE_URI_CURRENTCOLOR:
    case SVG_PAINTTYPE_URI_RGBCOLOR:
    case SVG_PAINTTYPE_URI_RGBCOLOR_ICCCOLOR:
    case SVG_PAINTTYPE_URI:
        return referenceId == SVGURIReference::getTarget(m_uri);
    }

    ASSERT_NOT_REACHED();
    return false;
}

}

#endif // ENABLE(SVG)
