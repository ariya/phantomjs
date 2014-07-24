/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef PlatformVideoWindowPrivate_h
#define PlatformVideoWindowPrivate_h
// We do not use ENABLE or USE because moc does not expand these macros.
#if defined(WTF_USE_GSTREAMER) && WTF_USE_GSTREAMER && defined(WTF_USE_NATIVE_FULLSCREEN_VIDEO) && WTF_USE_NATIVE_FULLSCREEN_VIDEO

#include <QEvent>
#include <QTimer>

#include <QWindow>

namespace WebCore {

class HTMLVideoElement;

class FullScreenVideoWindow: public QWindow {
Q_OBJECT
public:
    FullScreenVideoWindow();
    void setVideoElement(HTMLVideoElement*);
Q_SIGNALS:
    void closed();
protected:
    void keyPressEvent(QKeyEvent*);
    bool event(QEvent*);

public Q_SLOTS:
    void showFullScreen();

private Q_SLOTS:
    void hideCursor();

private:
    void showCursor();
    QTimer m_cursorTimer;
    HTMLVideoElement* m_mediaElement;
};


} // namespace WebCore

#endif // defined(WTF_USE_GSTREAMER) && WTF_USE_GSTREAMER && defined(WTF_USE_NATIVE_FULLSCREEN_VIDEO) && WTF_USE_NATIVE_FULLSCREEN_VIDEO
#endif // PlatformVideoWindowPrivate_h
