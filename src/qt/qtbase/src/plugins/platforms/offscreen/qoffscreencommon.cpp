/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qoffscreencommon.h"
#include "qoffscreenwindow.h"

#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatformcursor.h>
#include <qpa/qplatformwindow.h>

QT_BEGIN_NAMESPACE

QPlatformWindow *QOffscreenScreen::windowContainingCursor = 0;

class QOffscreenCursor : public QPlatformCursor
{
public:
    QOffscreenCursor() : m_pos(10, 10) {}

    QPoint pos() const { return m_pos; }
    void setPos(const QPoint &pos)
    {
        m_pos = pos;
        QWindowList wl = QGuiApplication::topLevelWindows();
        QWindow *containing = 0;
        foreach (QWindow *w, wl) {
            if (w->type() != Qt::Desktop && w->isExposed() && w->geometry().contains(pos)) {
                containing = w;
                break;
            }
        }

        QPoint local = pos;
        if (containing)
            local -= containing->position();

        QWindow *previous = QOffscreenScreen::windowContainingCursor ? QOffscreenScreen::windowContainingCursor->window() : 0;

        if (containing != previous)
            QWindowSystemInterface::handleEnterLeaveEvent(containing, previous, local, pos);

        QWindowSystemInterface::handleMouseEvent(containing, local, pos, QGuiApplication::mouseButtons(), QGuiApplication::keyboardModifiers());

        QOffscreenScreen::windowContainingCursor = containing ? containing->handle() : 0;
    }

    void changeCursor(QCursor *windowCursor, QWindow *window)
    {
        Q_UNUSED(windowCursor);
        Q_UNUSED(window);
    }

private:
    QPoint m_pos;
};

QOffscreenScreen::QOffscreenScreen()
    : m_geometry(0, 0, 800, 600)
    , m_cursor(new QOffscreenCursor)
{
}

QPixmap QOffscreenScreen::grabWindow(WId id, int x, int y, int width, int height) const
{
    QRect rect(x, y, width, height);

    QOffscreenWindow *window = QOffscreenWindow::windowForWinId(id);
    if (!window || window->window()->type() == Qt::Desktop) {
        QWindowList wl = QGuiApplication::topLevelWindows();
        QWindow *containing = 0;
        foreach (QWindow *w, wl) {
            if (w->type() != Qt::Desktop && w->isExposed() && w->geometry().contains(rect)) {
                containing = w;
                break;
            }
        }

        if (!containing)
            return QPixmap();

        id = containing->winId();
        rect = rect.translated(-containing->geometry().topLeft());
    }

    QOffscreenBackingStore *store = QOffscreenBackingStore::backingStoreForWinId(id);
    if (store)
        return store->grabWindow(id, rect);
    return QPixmap();
}

QOffscreenBackingStore::QOffscreenBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
}

QOffscreenBackingStore::~QOffscreenBackingStore()
{
    clearHash();
}

QPaintDevice *QOffscreenBackingStore::paintDevice()
{
    return &m_image;
}

void QOffscreenBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);

    if (m_image.size().isEmpty())
        return;

    QSize imageSize = m_image.size();

    QRegion clipped = QRect(0, 0, window->width(), window->height());
    clipped &= QRect(0, 0, imageSize.width(), imageSize.height()).translated(-offset);

    QRect bounds = clipped.boundingRect().translated(offset);

    if (bounds.isNull())
        return;

    WId id = window->winId();

    m_windowAreaHash[id] = bounds;
    m_backingStoreForWinIdHash[id] = this;
}

void QOffscreenBackingStore::resize(const QSize &size, const QRegion &)
{
    QImage::Format format = QGuiApplication::primaryScreen()->handle()->format();
    if (m_image.size() != size)
        m_image = QImage(size, format);
    clearHash();
}

extern void qt_scrollRectInImage(QImage &img, const QRect &rect, const QPoint &offset);

bool QOffscreenBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    if (m_image.isNull())
        return false;

    const QVector<QRect> rects = area.rects();
    for (int i = 0; i < rects.size(); ++i)
        qt_scrollRectInImage(m_image, rects.at(i), QPoint(dx, dy));

    return true;
}

QPixmap QOffscreenBackingStore::grabWindow(WId window, const QRect &rect) const
{
    QRect area = m_windowAreaHash.value(window, QRect());
    if (area.isNull())
        return QPixmap();

    QRect adjusted = rect;
    if (adjusted.width() <= 0)
        adjusted.setWidth(area.width());
    if (adjusted.height() <= 0)
        adjusted.setHeight(area.height());

    adjusted = adjusted.translated(area.topLeft()) & area;

    if (adjusted.isEmpty())
        return QPixmap();

    return QPixmap::fromImage(m_image.copy(adjusted));
}

QOffscreenBackingStore *QOffscreenBackingStore::backingStoreForWinId(WId id)
{
    return m_backingStoreForWinIdHash.value(id, 0);
}

void QOffscreenBackingStore::clearHash()
{
    QList<WId> ids = m_windowAreaHash.keys();
    foreach (WId id, ids) {
        QHash<WId, QOffscreenBackingStore *>::iterator it = m_backingStoreForWinIdHash.find(id);
        if (it.value() == this)
            m_backingStoreForWinIdHash.remove(id);
    }
    m_windowAreaHash.clear();
}

QHash<WId, QOffscreenBackingStore *> QOffscreenBackingStore::m_backingStoreForWinIdHash;

QT_END_NAMESPACE
