/*
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007, 2009 Holger Hans Peter Freyther
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Widget.h"

#include "Cursor.h"
#include "GraphicsContext.h"
#include "GtkVersioning.h"
#include "HostWindow.h"
#include "IntRect.h"
#include "ScrollView.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>

namespace WebCore {

Widget::Widget(PlatformWidget widget)
{
    init(widget);
}

Widget::~Widget()
{
    ASSERT(!parent());

    releasePlatformWidget();
}

void Widget::setFocus(bool focused)
{
}

void Widget::setCursor(const Cursor& cursor)
{
    ScrollView* view = root();
    if (!view)
        return;
    view->hostWindow()->setCursor(cursor);
}

void Widget::show()
{
    setSelfVisible(true);

    if (isParentVisible() && platformWidget())
        gtk_widget_show(platformWidget());
}

void Widget::hide()
{
    setSelfVisible(false);

    if (isParentVisible() && platformWidget())
        gtk_widget_hide(platformWidget());
}

void Widget::paint(GraphicsContext* context, const IntRect& rect)
{
}

void Widget::setIsSelected(bool isSelected)
{
    if (!platformWidget())
        return;

    // See if the platformWidget has a webkit-widget-is-selected property
    // and set it afterwards.
    GParamSpec* spec = g_object_class_find_property(G_OBJECT_GET_CLASS(platformWidget()),
                                                    "webkit-widget-is-selected");
    if (!spec)
        return;

    g_object_set(platformWidget(), "webkit-widget-is-selected", isSelected, NULL);
}

IntRect Widget::frameRect() const
{
    return m_frame;
}

void Widget::setFrameRect(const IntRect& rect)
{
    m_frame = rect;
    frameRectsChanged();
}

void Widget::releasePlatformWidget()
{
    if (!platformWidget())
         return;
    g_object_unref(platformWidget());
}

void Widget::retainPlatformWidget()
{
    if (!platformWidget())
         return;
    g_object_ref_sink(platformWidget());
}

}
