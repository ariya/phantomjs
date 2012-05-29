/**
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.
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
#include "FontValue.h"

#include "CSSValueList.h"
#include "CSSPrimitiveValue.h"
#include "PlatformString.h"

namespace WebCore {

String FontValue::cssText() const
{
    // font variant weight size / line-height family 

    String result("");

    if (style)
        result += style->cssText();
    if (variant) {
        if (!result.isEmpty())
            result += " ";
        result += variant->cssText();
    }
    if (weight) {
        if (!result.isEmpty())
            result += " ";
        result += weight->cssText();
    }
    if (size) {
        if (!result.isEmpty())
            result += " ";
        result += size->cssText();
    }
    if (lineHeight) {
        if (!size)
            result += " ";
        result += "/";
        result += lineHeight->cssText();
    }
    if (family) {
        if (!result.isEmpty())
            result += " ";
        result += family->cssText();
    }

    return result;
}

}
