/*
 * Copyright (C) 2009, 2010, 2011 Research In Motion Limited. All rights reserved.
 * Copyright (c) 2008, 2009, Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "PlatformScreen.h"

#include "FloatRect.h"
#include "Frame.h"
#include "FrameView.h"
#include "NotImplemented.h"
#include "Page.h"
#include "Settings.h"
#include "Widget.h"

#include <BlackBerryPlatformScreen.h>

namespace WebCore {

bool screenIsMonochrome(Widget*)
{
    return false;
}

int screenDepthPerComponent(Widget*)
{
    return 8;
}

int screenDepth(Widget*)
{
    return 24;
}

// Use logical (density-independent) pixels instead of physical screen pixels.
static FloatRect toUserSpace(FloatRect rect, Widget* widget)
{
    if (widget->isFrameView()) {
        Page* page = toFrameView(widget)->frame()->page();
        if (page && !page->settings()->applyDeviceScaleFactorInCompositor()) {
            rect.scale(1 / page->deviceScaleFactor());
            rect.setSize(expandedIntSize(rect.size()));
        }
    }
    return rect;
}

FloatRect screenAvailableRect(Widget* widget)
{
    if (!widget)
        return FloatRect();
    return toUserSpace(IntRect(IntPoint::zero(), IntSize(BlackBerry::Platform::Graphics::Screen::primaryScreen()->size())), widget);
}

FloatRect screenRect(Widget* widget)
{
    if (!widget)
        return FloatRect();
    return toUserSpace(IntRect(IntPoint::zero(), IntSize(BlackBerry::Platform::Graphics::Screen::primaryScreen()->size())), widget);
}

void screenColorProfile(ColorProfile&)
{
    notImplemented();
}

} // namespace WebCore
