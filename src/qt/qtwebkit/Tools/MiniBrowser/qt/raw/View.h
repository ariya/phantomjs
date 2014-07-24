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

#ifndef View_h
#define View_h

#include <QBasicTimer>
#include <QOpenGLContext>
#include <QWindow>
#include <WebKit2/qrawwebview_p.h>


class View :  public QWindow, private QRawWebViewClient {
public:
    View(const QString& url);
    ~View();

public: // QRawWebViewClient
    virtual void viewNeedsDisplay(const QRect&);
    virtual void viewRequestedScroll(const QPoint&) { }
    virtual void viewProcessCrashed() { }
    virtual void viewProcessRelaunched() { }
    virtual void viewContentSizeChanged(const QSize&) { }
    virtual void viewRequestedCursorOverride(const QCursor&);
    virtual void doneWithKeyEvent(const QKeyEvent*, bool wasHandled);
    virtual void doneWithTouchEvent(const QTouchEvent*, bool wasHandled) { }

protected:
    virtual void exposeEvent(QExposeEvent*);
    virtual void resizeEvent(QResizeEvent*);

    virtual void keyPressEvent(QKeyEvent* event) { m_webView->sendKeyEvent(event); }
    virtual void keyReleaseEvent(QKeyEvent* event) { m_webView->sendKeyEvent(event); }

    virtual void mouseDoubleClickEvent(QMouseEvent* event) { m_webView->sendMouseEvent(event, 2); }
    virtual void mouseMoveEvent(QMouseEvent* event) { m_webView->sendMouseEvent(event); }
    virtual void mousePressEvent(QMouseEvent* event) { m_webView->sendMouseEvent(event, 1); }
    virtual void mouseReleaseEvent(QMouseEvent* event) { m_webView->sendMouseEvent(event); }

private:
    QRawWebView* m_webView;
    QOpenGLContext *m_context;
    QBasicTimer m_paintTimer;

    QString m_url;
    bool m_active;

    virtual void timerEvent(QTimerEvent*);
};

#endif // View_h
