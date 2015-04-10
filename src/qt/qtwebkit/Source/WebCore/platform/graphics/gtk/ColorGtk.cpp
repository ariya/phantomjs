/*
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2011 Igalia S.L.
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
#include "Color.h"

#include <gdk/gdk.h>

namespace WebCore {

Color::Color(const GdkColor& c)
    : m_color(makeRGB(c.red >> 8, c.green >> 8, c.blue >> 8))
    , m_valid(true)
{
}

#ifndef GTK_API_VERSION_2
Color::Color(const GdkRGBA& c)
    : m_color(makeRGBA(static_cast<int>(c.red * 255),
                       static_cast<int>(c.green * 255),
                       static_cast<int>(c.blue * 255),
                       static_cast<int>(c.alpha * 255)))
    , m_valid(true)
{
}

Color::operator GdkRGBA() const
{
    double red, green, blue, alpha;
    getRGBA(red, green, blue, alpha);
    GdkRGBA rgba = { red, green, blue, alpha };
    return rgba;
}
#endif

}

// vim: ts=4 sw=4 et
