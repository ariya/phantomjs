/*
 * Copyright (C) 2011 Igalia S.L
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
#include "PlatformVideoWindow.h"

#if USE(GSTREAMER)

#include <gst/gst.h>

using namespace WebCore;

PlatformVideoWindow::PlatformVideoWindow()
{
    m_window.adoptNS([[NSView alloc] init]);
    m_videoWindowId = reinterpret_cast<unsigned long>(m_window.get());
}

PlatformVideoWindow::~PlatformVideoWindow()
{
    m_videoWindowId = 0;
}

void PlatformVideoWindow::prepareForOverlay(GstMessage* message)
{
    if (gst_structure_has_name(message->structure, "have-ns-view")) {
        m_videoWindow = static_cast<PlatformWidget>(g_value_get_pointer(gst_structure_get_value(message->structure, "nsview")));
        ASSERT(m_videoWindow);
        [m_window.get() addSubview:m_videoWindow];
    }
}

#endif // USE(GSTREAMER)
