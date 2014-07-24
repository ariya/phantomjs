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

#ifndef ewk_color_picker_private_h
#define ewk_color_picker_private_h

#if ENABLE(INPUT_TYPE_COLOR)

#include "WKRetainPtr.h"
#include <WebCore/Color.h>
#include <wtf/PassOwnPtr.h>

class EwkColorPicker {
public:
    static PassOwnPtr<EwkColorPicker> create(WKColorPickerResultListenerRef colorPickerListener, const WebCore::Color& initialColor)
    {
        return adoptPtr(new EwkColorPicker(colorPickerListener, initialColor));
    }

    const WebCore::Color& color() const;
    void setColor(const WebCore::Color&);

private:
    EwkColorPicker(WKColorPickerResultListenerRef colorPickerListener, const WebCore::Color& initialColor);

    WKRetainPtr<WKColorPickerResultListenerRef> m_colorPickerListener;
    WebCore::Color m_color;
};

#endif // ENABLE(INPUT_TYPE_COLOR)

#endif // ewk_color_picker_private_h
