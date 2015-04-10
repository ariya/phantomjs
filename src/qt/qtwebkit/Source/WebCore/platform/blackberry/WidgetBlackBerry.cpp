/*
 * Copyright (C) 2009, 2010, 2011 Research In Motion Limited. All rights reserved.
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
#include "Widget.h"

#include "Cursor.h"
#include "GraphicsContext.h"
#include "HostWindow.h"
#include "IntRect.h"
#include "NotImplemented.h"
#include "ScrollView.h"

namespace WebCore {

Widget::Widget(PlatformWidget widget)
{
    init(widget);
}

Widget::~Widget()
{
}

void Widget::hide()
{
    notImplemented();
}

void Widget::paint(GraphicsContext*, IntRect const&)
{
    notImplemented();
}

void Widget::setCursor(Cursor const& cursor)
{
    ScrollView* theRoot = root();
    if (!theRoot)
        return;
    PlatformPageClient pageClient = theRoot->hostWindow()->platformPageClient();

    if (pageClient)
        pageClient->setCursor(cursor.impl());
}

void Widget::setFocus(bool)
{
    notImplemented();
}

void Widget::setFrameRect(const IntRect& rect)
{
    m_frame = rect;

    frameRectsChanged();
}

void Widget::setIsSelected(bool)
{
    notImplemented();
}

void Widget::show()
{
    notImplemented();
}

IntRect Widget::frameRect() const
{
    return m_frame;
}

} // namespace WebCore
