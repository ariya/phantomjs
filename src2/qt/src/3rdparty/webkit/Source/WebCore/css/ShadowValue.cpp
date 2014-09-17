/**
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2009 Apple Computer, Inc.
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
#include "ShadowValue.h"

#include "CSSPrimitiveValue.h"
#include "PlatformString.h"

namespace WebCore {

// Used for text-shadow and box-shadow
ShadowValue::ShadowValue(PassRefPtr<CSSPrimitiveValue> _x,
    PassRefPtr<CSSPrimitiveValue> _y,
    PassRefPtr<CSSPrimitiveValue> _blur,
    PassRefPtr<CSSPrimitiveValue> _spread,
    PassRefPtr<CSSPrimitiveValue> _style,
    PassRefPtr<CSSPrimitiveValue> _color)
    : x(_x)
    , y(_y)
    , blur(_blur)
    , spread(_spread)
    , style(_style)
    , color(_color)
{
}

String ShadowValue::cssText() const
{
    String text("");

    if (color)
        text += color->cssText();
    if (x) {
        if (!text.isEmpty())
            text += " ";
        text += x->cssText();
    }
    if (y) {
        if (!text.isEmpty())
            text += " ";
        text += y->cssText();
    }
    if (blur) {
        if (!text.isEmpty())
            text += " ";
        text += blur->cssText();
    }
    if (spread) {
        if (!text.isEmpty())
            text += " ";
        text += spread->cssText();
    }
    if (style) {
        if (!text.isEmpty())
            text += " ";
        text += style->cssText();
    }

    return text;
}

}
