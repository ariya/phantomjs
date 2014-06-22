/*
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef QWebPageClient_h
#define QWebPageClient_h

#ifndef QT_NO_CURSOR
#include <QCursor>
#endif

#if USE(ACCELERATED_COMPOSITING)
#include <GraphicsLayer.h>
#endif

#if USE(3D_GRAPHICS)
#include <GraphicsContext3D.h>
#endif

#include <QPalette>
#include <QRect>

QT_BEGIN_NAMESPACE
class QStyle;
class QWindow;
QT_END_NAMESPACE

namespace WebCore {
class Widget;
}

class QWebPageClient {
public:
    virtual ~QWebPageClient() { }

    virtual bool isQWidgetClient() const { return false; }

    virtual void scroll(int dx, int dy, const QRect&) = 0;
    virtual void update(const QRect&) = 0;
    virtual void repaintViewport() = 0;
    virtual void setInputMethodEnabled(bool enable) = 0;
    virtual bool inputMethodEnabled() const = 0;
    virtual bool makeOpenGLContextCurrentIfAvailable() { return false; }

    virtual void setInputMethodHints(Qt::InputMethodHints hint) = 0;

#ifndef QT_NO_CURSOR
    inline void resetCursor()
    {
        if (!cursor().bitmap() && cursor().shape() == m_lastCursor.shape())
            return;
        updateCursor(m_lastCursor);
    }

    inline void setCursor(const QCursor& cursor)
    {
        m_lastCursor = cursor;
        if (!cursor.bitmap() && cursor.shape() == this->cursor().shape())
            return;
        updateCursor(cursor);
    }
#endif

    virtual QPalette palette() const = 0;
    virtual int screenNumber() const = 0;
    virtual QObject* ownerWidget() const = 0;
    virtual QRect geometryRelativeToOwnerWidget() const = 0;
    virtual QPoint mapToOwnerWindow(const QPoint&) const = 0;

    virtual QObject* pluginParent() const = 0;

    virtual QStyle* style() const = 0;

    virtual QRectF graphicsItemVisibleRect() const { return QRectF(); }

    virtual bool viewResizesToContentsEnabled() const = 0;

    virtual QRectF windowRect() const = 0;

    virtual void setWidgetVisible(WebCore::Widget*, bool visible) = 0;

    virtual QWindow* ownerWindow() const;

protected:
#ifndef QT_NO_CURSOR
    virtual QCursor cursor() const = 0;
    virtual void updateCursor(const QCursor& cursor) = 0;
#endif

private:
#ifndef QT_NO_CURSOR
    QCursor m_lastCursor;
#endif
};

#endif
