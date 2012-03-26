/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

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
#include "PluginContainerSymbian.h"

#include "FocusController.h"
#include "Frame.h"
#include "FrameView.h"
#include "Page.h"
#include "PlatformKeyboardEvent.h"
#include "PluginView.h"

#include <QApplication>
#include <QWidget>

using namespace WebCore;

PluginContainerSymbian::PluginContainerSymbian(PluginView* view, QWidget* parent, QGraphicsProxyWidget* proxy)
    : QWidget(parent)
    , m_pluginView(view)
    , m_proxy(proxy)
    , m_hasPendingGeometryChange(false)
{
}

PluginContainerSymbian::~PluginContainerSymbian()
{
}

void PluginContainerSymbian::requestGeometry(const QRect& rect, const QRegion& clip)
{
    if (m_windowRect != rect || m_clipRegion != clip) {
        m_windowRect = rect;
        m_clipRegion = clip;
        m_hasPendingGeometryChange = true;
    }
}

void PluginContainerSymbian::adjustGeometry()
{
    if (m_hasPendingGeometryChange) {
        setGeometry(m_windowRect);
        setMask(m_clipRegion);
        m_hasPendingGeometryChange = false;
    }
}

void PluginContainerSymbian::focusInEvent(QFocusEvent*)
{
    if (Page* page = m_pluginView->parentFrame()->page())
        page->focusController()->setActive(true);

    m_pluginView->focusPluginElement();
}

void PluginContainerSymbian::focusOutEvent(QFocusEvent*)
{
    if (Page* page = m_pluginView->parentFrame()->page())
        page->focusController()->setActive(false);
}
