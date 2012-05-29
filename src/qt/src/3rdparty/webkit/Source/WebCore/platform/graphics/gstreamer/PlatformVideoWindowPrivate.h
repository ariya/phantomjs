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

#include <QTimer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QKeyEvent;
QT_END_NAMESPACE

namespace WebCore {

class HTMLVideoElement;

class FullScreenVideoWindow: public QWidget {
Q_OBJECT
public:
    FullScreenVideoWindow();
    void setVideoElement(HTMLVideoElement*);
signals:
    void closed();
protected:
    void closeEvent(QCloseEvent*);
    void keyPressEvent(QKeyEvent*);
    bool event(QEvent*);

public slots:
    void showFullScreen();

private slots:
    void hideCursor();

private:
    void showCursor();
    QTimer m_cursorTimer;
    HTMLVideoElement* m_mediaElement;
};


} // namespace WebCore


#endif // PlatformVideoWindowPrivate_h
