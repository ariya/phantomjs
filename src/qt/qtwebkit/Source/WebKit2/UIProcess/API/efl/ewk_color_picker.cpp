/*
   Copyright (C) 2011 Samsung Electronics
   Copyright (C) 2012 Intel Corporation. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "ewk_color_picker.h"

#include "WKColorPickerResultListener.h"
#include "WKString.h"
#include "ewk_color_picker_private.h"
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

#if ENABLE(INPUT_TYPE_COLOR)
EwkColorPicker::EwkColorPicker(WKColorPickerResultListenerRef colorPickerListener, const Color& initialColor)
    : m_colorPickerListener(colorPickerListener)
    , m_color(initialColor)
{
}

void EwkColorPicker::setColor(const Color& color)
{
    WKRetainPtr<WKStringRef> colorString(AdoptWK, WKStringCreateWithUTF8CString(color.serialized().utf8().data()));
    WKColorPickerResultListenerSetColor(m_colorPickerListener.get(), colorString.get());
}

const Color& EwkColorPicker::color() const
{
    return m_color;
}
#endif

Eina_Bool ewk_color_picker_color_set(Ewk_Color_Picker* colorPicker, int r, int g, int b, int a)
{
#if ENABLE(INPUT_TYPE_COLOR)
    EINA_SAFETY_ON_NULL_RETURN_VAL(colorPicker, false);

    colorPicker->setColor(Color(r, g, b, a));

    return true;
#else
    UNUSED_PARAM(colorPicker);
    UNUSED_PARAM(r);
    UNUSED_PARAM(g);
    UNUSED_PARAM(b);
    UNUSED_PARAM(a);
    return false;
#endif
}

Eina_Bool ewk_color_picker_color_get(const Ewk_Color_Picker* colorPicker, int* r, int* g, int* b, int* a)
{
#if ENABLE(INPUT_TYPE_COLOR)
    EINA_SAFETY_ON_NULL_RETURN_VAL(colorPicker, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(r, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(g, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(b, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(a, false);

    const Color& color = colorPicker->color();
    *r = color.red();
    *g = color.green();
    *b = color.blue();
    *a = color.alpha();

    return true;
#else
    UNUSED_PARAM(colorPicker);
    UNUSED_PARAM(r);
    UNUSED_PARAM(g);
    UNUSED_PARAM(b);
    UNUSED_PARAM(a);
    return false;
#endif
}
