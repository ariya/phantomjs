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
#ifndef DefaultFullScreenVideoHandler_h
#define DefaultFullScreenVideoHandler_h

#include "qwebkitplatformplugin.h"

namespace WebKit {

class FullScreenVideoWidget;

// We do not use ENABLE or USE because moc does not expand these macros.
#if defined(WTF_USE_QT_MULTIMEDIA) && WTF_USE_QT_MULTIMEDIA
class DefaultFullScreenVideoHandler : public QWebFullScreenVideoHandler {
    Q_OBJECT
public:
    DefaultFullScreenVideoHandler();
    virtual ~DefaultFullScreenVideoHandler();
    bool requiresFullScreenForVideoPlayback() const;

public Q_SLOTS:
    void enterFullScreen(QMediaPlayer*);
    void exitFullScreen();

private:
    static bool s_shouldForceFullScreenVideoPlayback;
    FullScreenVideoWidget *m_fullScreenWidget;
};
#endif

} // namespace WebKit

#endif // DefaultFullScreenVideoHandler_h
