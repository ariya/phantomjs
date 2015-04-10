/*
 *  Copyright (C) 2007, 2009 Holger Hans Peter Freyther zecke@selfish.org
 *  Copyright (C) 2010 Gustavo Noronha Silva <gns@gnome.org>
 *  Copyright (C) 2010 Collabora Ltd.
 *  Copyright (C) 2010, 2011 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "MainFrameScrollbarGtk.h"

#include "GtkVersioning.h"
#include "IntRect.h"
#include "ScrollableArea.h"
#include <gtk/gtk.h>

using namespace WebCore;

PassRefPtr<MainFrameScrollbarGtk> MainFrameScrollbarGtk::create(ScrollableArea* scrollableArea, ScrollbarOrientation orientation)
{
    return adoptRef(new MainFrameScrollbarGtk(scrollableArea, orientation));
}

// A MainFrameScrollbar is just a non-painting scrollbar. Otherwise it is fully
// functional. A non-painting scrollbar allows a main-frame ScrollView to use
// a containing GtkScrolledWindow as its user interface. The ChromeClient in the
// WebKit layer just listens for scrolling and sizing changes and updates its
// container (the GtkScrolledWindow) accordingly. The ScrollView is responsible
// for deciding whether or not to create a MainFrameScrollbar or native scrollbar.
MainFrameScrollbarGtk::MainFrameScrollbarGtk(ScrollableArea* scrollableArea, ScrollbarOrientation orientation)
    : Scrollbar(scrollableArea, orientation, RegularScrollbar)
{
    // We don't want to take up any space.
    resize(0, 0);
}

void MainFrameScrollbarGtk::paint(GraphicsContext* context, const IntRect& rect)
{
    // Main frame scrollbars are not painted by WebCore.
    return;
}
