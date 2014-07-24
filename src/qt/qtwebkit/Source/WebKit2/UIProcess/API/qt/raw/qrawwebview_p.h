/*
    Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

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

/*
    This file is not part of the public Qt Api. It may change without notice at any time in future.
    We make no commitment regarding source compatibility or binary compatibility.
*/

#ifndef qrawwebview_p_h
#define qrawwebview_p_h

#include "qwebkitglobal.h"

#include <WebKit2/WKContext.h>
#include <WebKit2/WKPage.h>
#include <WebKit2/WKPageGroup.h>

QT_BEGIN_NAMESPACE
class QRect;
class QRectF;
class QPoint;
class QSize;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QTouchEvent;
class QUrl;
class QMatrix4x4;
class QCursor;
QT_END_NAMESPACE

class QRawWebViewPrivate;

namespace WebCore {
class CoordinatedGraphicsScene;
}

class QRawWebViewClient {
public:
    virtual ~QRawWebViewClient() { }

    virtual void viewNeedsDisplay(const QRect&) = 0;
    virtual void viewRequestedScroll(const QPoint&) = 0;
    virtual void viewProcessCrashed() = 0;
    virtual void viewProcessRelaunched() = 0;
    virtual void viewContentSizeChanged(const QSize&) = 0;
    virtual void viewRequestedCursorOverride(const QCursor&) = 0;
    virtual void doneWithKeyEvent(const QKeyEvent*, bool wasHandled) = 0;
    virtual void doneWithTouchEvent(const QTouchEvent*, bool wasHandled) = 0;
};

class QWEBKIT_EXPORT QRawWebView {
public:
    QRawWebView(WKContextRef, WKPageGroupRef, QRawWebViewClient*);
    ~QRawWebView();

    void create();

    void setSize(const QSize&);
    QSize size() const;

    void setFocused(bool);
    bool isFocused() const;

    void setVisible(bool);
    bool isVisible() const;

    void setActive(bool);
    bool isActive() const;

    void setTransparentBackground(bool);
    bool transparentBackground() const;

    void setDrawBackground(bool);
    bool drawBackground() const;

    // Paints on the current GL context.
    void paint(const QMatrix4x4& transform, float opacity, unsigned paintFlags);

    WKPageRef pageRef();

    void sendKeyEvent(QKeyEvent*);
    void sendMouseEvent(QMouseEvent*, int clickCount = 0);
    void sendWheelEvent(QWheelEvent*);
    void sendTouchEvent(QTouchEvent*);

private:
    WebCore::CoordinatedGraphicsScene* coordinatedGraphicsScene() const;

    QRawWebViewPrivate* d;
};

#endif // qrawwebview_p_h
