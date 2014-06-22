/*
 * Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
#include "DefaultFullScreenVideoHandler.h"

#if USE(QT_MULTIMEDIA)

#include "FullScreenVideoWidget.h"

using namespace WebKit;

bool DefaultFullScreenVideoHandler::s_shouldForceFullScreenVideoPlayback = false;

DefaultFullScreenVideoHandler::DefaultFullScreenVideoHandler()
    : QWebFullScreenVideoHandler()
    , m_fullScreenWidget(new FullScreenVideoWidget)
{
    connect(m_fullScreenWidget, SIGNAL(didExitFullScreen()), this, SIGNAL(fullScreenClosed()));
    m_fullScreenWidget->hide();

    m_fullScreenWidget->close();
}

DefaultFullScreenVideoHandler::~DefaultFullScreenVideoHandler()
{
    delete m_fullScreenWidget;
}

bool DefaultFullScreenVideoHandler::requiresFullScreenForVideoPlayback() const
{
    static bool initialized = false;
    if (!initialized) {
        QByteArray forceFullScreen = qgetenv("QT_WEBKIT_FORCE_FULLSCREEN_VIDEO");
        if (!forceFullScreen.isEmpty())
            s_shouldForceFullScreenVideoPlayback = true;

        initialized = true;
    }

    return s_shouldForceFullScreenVideoPlayback;
}

void DefaultFullScreenVideoHandler::enterFullScreen(QMediaPlayer* player)
{
    m_fullScreenWidget->show(player);
}

void DefaultFullScreenVideoHandler::exitFullScreen()
{
    m_fullScreenWidget->close();
}
#endif


