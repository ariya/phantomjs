/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#ifndef SVGRect_h
#define SVGRect_h

#if ENABLE(SVG)
#include "FloatRect.h"
#include "SVGPropertyTraits.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

template<>
struct SVGPropertyTraits<FloatRect> {
    static FloatRect initialValue() { return FloatRect(); }
    static String toString(const FloatRect& type)
    {
        StringBuilder builder;
        builder.append(String::number(type.x()));
        builder.append(' ');
        builder.append(String::number(type.y()));
        builder.append(' ');
        builder.append(String::number(type.width()));
        builder.append(' ');
        builder.append(String::number(type.height()));
        return builder.toString();
    }
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGRect_h
