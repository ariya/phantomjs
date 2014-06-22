/***************************************************************************
**
** Copyright (C) 2011 - 2013 BlackBerry Limited. All rights reserved.
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

#include "qqnxrasterbackingstore.h"
#include "qqnxrasterwindow.h"

#include <QtCore/QDebug>

#include <errno.h>

#if defined(QQNXRASTERBACKINGSTORE_DEBUG)
#define qRasterBackingStoreDebug qDebug
#else
#define qRasterBackingStoreDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxRasterBackingStore::QQnxRasterBackingStore(QWindow *window)
    : QPlatformBackingStore(window),
      m_hasUnflushedPaintOperations(false)
{
    qRasterBackingStoreDebug() << Q_FUNC_INFO << "w =" << window;

    m_window = window;
}

QQnxRasterBackingStore::~QQnxRasterBackingStore()
{
    qRasterBackingStoreDebug() << Q_FUNC_INFO << "w =" << window();
}

QPaintDevice *QQnxRasterBackingStore::paintDevice()
{
    if (platformWindow() && platformWindow()->hasBuffers())
        return platformWindow()->renderBuffer().image();

    return 0;
}

void QQnxRasterBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(offset)

    qRasterBackingStoreDebug() << Q_FUNC_INFO << "w =" << this->window();

    // Sometimes this method is called even though there is nothing to be
    // flushed, for instance, after an expose event directly follows a
    // geometry change event.
    if (!m_hasUnflushedPaintOperations)
            return;

    QQnxWindow *targetWindow = 0;
    if (window)
        targetWindow = static_cast<QQnxWindow *>(window->handle());

    // we only need to flush the platformWindow backing store, since this is
    // the buffer where all drawing operations of all windows, including the
    // child windows, are performed; conceptually ,child windows have no buffers
    // (actually they do have a 1x1 placeholder buffer due to libscreen limitations),
    // since Qt will only draw to the backing store of the top-level window.
    if (!targetWindow || targetWindow == platformWindow()) {

        // visit all pending scroll operations
        for (int i = m_scrollOpList.size() - 1; i >= 0; i--) {

            // do the scroll operation
            ScrollOp &op = m_scrollOpList[i];
            QRegion srcArea = op.totalArea.intersected( op.totalArea.translated(-op.dx, -op.dy) );
            platformWindow()->scroll(srcArea, op.dx, op.dy);
        }

        // clear all pending scroll operations
        m_scrollOpList.clear();

        // update the display with newly rendered content
        platformWindow()->post(region);
    }

    m_hasUnflushedPaintOperations = false;
}

void QQnxRasterBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(size);
    Q_UNUSED(staticContents);
    qRasterBackingStoreDebug() << Q_FUNC_INFO << "w =" << window() << ", s =" << size;

    // NOTE: defer resizing window buffers until next paint as
    // resize() can be called multiple times before a paint occurs
}

bool QQnxRasterBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    qRasterBackingStoreDebug() << Q_FUNC_INFO << "w =" << window();

    // calculate entire region affected by scroll operation (src + dst)
    QRegion totalArea = area.translated(dx, dy);
    totalArea += area;
    m_hasUnflushedPaintOperations = true;

    // visit all pending scroll operations
    for (int i = m_scrollOpList.size() - 1; i >= 0; i--) {

        ScrollOp &op = m_scrollOpList[i];
        if (op.totalArea == totalArea) {
            // same area is being scrolled again - update delta
            op.dx += dx;
            op.dy += dy;
            return true;
        } else if (op.totalArea.intersects(totalArea)) {
            // current scroll overlaps previous scroll but is
            // not equal in area - just paint everything
            qWarning("QQNX: pending scroll operations overlap but not equal");
            return false;
        }
    }

    // create new scroll operation
    m_scrollOpList.append( ScrollOp(totalArea, dx, dy) );
    return true;
}

void QQnxRasterBackingStore::beginPaint(const QRegion &region)
{
    Q_UNUSED(region);

    qRasterBackingStoreDebug() << Q_FUNC_INFO << "w =" << window();
    m_hasUnflushedPaintOperations = true;

    platformWindow()->adjustBufferSize();
}

void QQnxRasterBackingStore::endPaint()
{
    qRasterBackingStoreDebug() << Q_FUNC_INFO << "w =" << window();
}

QQnxRasterWindow *QQnxRasterBackingStore::platformWindow() const
{
  Q_ASSERT(m_window->handle());
  return static_cast<QQnxRasterWindow*>(m_window->handle());
}

QT_END_NAMESPACE
