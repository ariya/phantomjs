/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qxcbbackingstore.h"

#include "qxcbconnection.h"
#include "qxcbscreen.h"
#include "qxcbwindow.h"

#include <xcb/shm.h>
#include <xcb/xcb_image.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdio.h>
#include <errno.h>

#include <qdebug.h>
#include <qpainter.h>
#include <qscreen.h>

#include <algorithm>
QT_BEGIN_NAMESPACE

class QXcbShmImage : public QXcbObject
{
public:
    QXcbShmImage(QXcbScreen *connection, const QSize &size, uint depth, QImage::Format format);
    ~QXcbShmImage() { destroy(); }

    QImage *image() { return &m_qimage; }
    QSize size() const { return m_qimage.size(); }

    void put(xcb_window_t window, const QPoint &dst, const QRect &source);
    void preparePaint(const QRegion &region);

private:
    void destroy();

    xcb_shm_segment_info_t m_shm_info;

    xcb_image_t *m_xcb_image;

    QImage m_qimage;

    xcb_gcontext_t m_gc;
    xcb_window_t m_gc_window;

    QRegion m_dirty;
};

QXcbShmImage::QXcbShmImage(QXcbScreen *screen, const QSize &size, uint depth, QImage::Format format)
    : QXcbObject(screen->connection())
    , m_gc(0)
    , m_gc_window(0)
{
    Q_XCB_NOOP(connection());

    const xcb_setup_t *setup = xcb_get_setup(xcb_connection());
    xcb_format_t *fmt = xcb_setup_pixmap_formats(setup);
    xcb_format_t *fmtend = fmt + xcb_setup_pixmap_formats_length(setup);
    for (; fmt != fmtend; ++fmt)
        if (fmt->depth == depth)
            break;

    Q_ASSERT(fmt != fmtend);

    m_xcb_image = xcb_image_create(size.width(), size.height(),
                                   XCB_IMAGE_FORMAT_Z_PIXMAP,
                                   fmt->scanline_pad,
                                   fmt->depth, fmt->bits_per_pixel, 0,
                                   QSysInfo::ByteOrder == QSysInfo::BigEndian ? XCB_IMAGE_ORDER_MSB_FIRST : XCB_IMAGE_ORDER_LSB_FIRST,
                                   XCB_IMAGE_ORDER_MSB_FIRST,
                                   0, ~0, 0);

    const int segmentSize = m_xcb_image->stride * m_xcb_image->height;
    if (!segmentSize)
        return;

    int id = shmget(IPC_PRIVATE, segmentSize, IPC_CREAT | 0600);
    if (id == -1)
        qWarning("QXcbShmImage: shmget() failed (%d) for size %d (%dx%d)",
                 errno, segmentSize, size.width(), size.height());
    else
        m_shm_info.shmid = id;
    m_shm_info.shmaddr = m_xcb_image->data = (quint8 *)shmat (m_shm_info.shmid, 0, 0);
    m_shm_info.shmseg = xcb_generate_id(xcb_connection());

    const xcb_query_extension_reply_t *shm_reply = xcb_get_extension_data(xcb_connection(), &xcb_shm_id);
    bool shm_present = shm_reply != NULL && shm_reply->present;
    xcb_generic_error_t *error = NULL;
    if (shm_present)
        error = xcb_request_check(xcb_connection(), xcb_shm_attach_checked(xcb_connection(), m_shm_info.shmseg, m_shm_info.shmid, false));
    if (!shm_present || error) {
        free(error);

        shmdt(m_shm_info.shmaddr);
        shmctl(m_shm_info.shmid, IPC_RMID, 0);

        m_shm_info.shmaddr = 0;

        m_xcb_image->data = (uint8_t *)malloc(segmentSize);
    } else {
        if (shmctl(m_shm_info.shmid, IPC_RMID, 0) == -1)
            qWarning() << "QXcbBackingStore: Error while marking the shared memory segment to be destroyed";
    }

    m_qimage = QImage( (uchar*) m_xcb_image->data, m_xcb_image->width, m_xcb_image->height, m_xcb_image->stride, format);
}

void QXcbShmImage::destroy()
{
    const int segmentSize = m_xcb_image ? (m_xcb_image->stride * m_xcb_image->height) : 0;
    if (segmentSize && m_shm_info.shmaddr)
        Q_XCB_CALL(xcb_shm_detach(xcb_connection(), m_shm_info.shmseg));

    xcb_image_destroy(m_xcb_image);

    if (segmentSize) {
        if (m_shm_info.shmaddr) {
            shmdt(m_shm_info.shmaddr);
            shmctl(m_shm_info.shmid, IPC_RMID, 0);
        } else {
            free(m_xcb_image->data);
        }
    }

    if (m_gc)
        Q_XCB_CALL(xcb_free_gc(xcb_connection(), m_gc));
}

void QXcbShmImage::put(xcb_window_t window, const QPoint &target, const QRect &source)
{
    Q_XCB_NOOP(connection());
    if (m_gc_window != window) {
        if (m_gc)
            Q_XCB_CALL(xcb_free_gc(xcb_connection(), m_gc));

        m_gc = xcb_generate_id(xcb_connection());
        Q_XCB_CALL(xcb_create_gc(xcb_connection(), m_gc, window, 0, 0));

        m_gc_window = window;
    }

    Q_XCB_NOOP(connection());
    if (m_shm_info.shmaddr) {
        xcb_image_shm_put(xcb_connection(),
                          window,
                          m_gc,
                          m_xcb_image,
                          m_shm_info,
                          source.x(),
                          source.y(),
                          target.x(),
                          target.y(),
                          source.width(),
                          source.height(),
                          false);
    } else {
        // If we upload the whole image in a single chunk, the result might be
        // larger than the server's maximum request size and stuff breaks.
        // To work around that, we upload the image in chunks where each chunk
        // is small enough for a single request.
        int src_x = source.x();
        int src_y = source.y();
        int target_x = target.x();
        int target_y = target.y();
        int width = source.width();
        int height = source.height();

        // We must make sure that each request is not larger than max_req_size.
        // Each request takes req_size + m_xcb_image->stride * height bytes.
        uint32_t max_req_size = xcb_get_maximum_request_length(xcb_connection());
        uint32_t req_size = sizeof(xcb_put_image_request_t);
        int rows_per_put = (max_req_size - req_size) / m_xcb_image->stride;

        // This assert could trigger if a single row has more pixels than fit in
        // a single PutImage request. However, max_req_size is guaranteed to be
        // at least 16384 bytes. That should be enough for quite large images.
        Q_ASSERT(rows_per_put > 0);

        // Convert the image to the native byte order.
        xcb_image_t *converted_image = xcb_image_native(xcb_connection(), m_xcb_image, 1);

        while (height > 0) {
            int rows = std::min(height, rows_per_put);

            xcb_image_t *subimage = xcb_image_subimage(converted_image, src_x, src_y, width, rows,
                                                       0, 0, 0);
            xcb_image_put(xcb_connection(),
                          window,
                          m_gc,
                          subimage,
                          target_x,
                          target_y,
                          0);

            xcb_image_destroy(subimage);

            src_y += rows;
            target_y += rows;
            height -= rows;
        }

        if (converted_image != m_xcb_image)
            xcb_image_destroy(converted_image);
    }
    Q_XCB_NOOP(connection());

    m_dirty = m_dirty | source;
}

void QXcbShmImage::preparePaint(const QRegion &region)
{
    // to prevent X from reading from the image region while we're writing to it
    if (m_dirty.intersects(region)) {
        connection()->sync();
        m_dirty = QRegion();
    }
}

QXcbBackingStore::QXcbBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_image(0)
    , m_syncingResize(false)
{
    QXcbScreen *screen = static_cast<QXcbScreen *>(window->screen()->handle());
    setConnection(screen->connection());
}

QXcbBackingStore::~QXcbBackingStore()
{
    delete m_image;
}

QPaintDevice *QXcbBackingStore::paintDevice()
{
    return m_image ? m_image->image() : 0;
}

void QXcbBackingStore::beginPaint(const QRegion &region)
{
    if (!m_image)
        return;

    m_image->preparePaint(region);

    if (m_image->image()->hasAlphaChannel()) {
        QPainter p(m_image->image());
        p.setCompositionMode(QPainter::CompositionMode_Source);
        const QVector<QRect> rects = region.rects();
        const QColor blank = Qt::transparent;
        for (QVector<QRect>::const_iterator it = rects.begin(); it != rects.end(); ++it) {
            p.fillRect(*it, blank);
        }
    }
}

QImage QXcbBackingStore::toImage() const
{
    return m_image && m_image->image() ? *m_image->image() : QImage();
}

void QXcbBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    if (!m_image || m_image->size().isEmpty())
        return;

    QSize imageSize = m_image->size();

    QRegion clipped = region;
    clipped &= QRect(0, 0, window->width(), window->height());
    clipped &= QRect(0, 0, imageSize.width(), imageSize.height()).translated(-offset);

    QRect bounds = clipped.boundingRect();

    if (bounds.isNull())
        return;

    Q_XCB_NOOP(connection());

    QXcbWindow *platformWindow = static_cast<QXcbWindow *>(window->handle());
    if (!platformWindow) {
        qWarning("QXcbBackingStore::flush: QWindow has no platform window (QTBUG-32681)");
        return;
    }

    QVector<QRect> rects = clipped.rects();
    for (int i = 0; i < rects.size(); ++i)
        m_image->put(platformWindow->xcb_window(), rects.at(i).topLeft(), rects.at(i).translated(offset));

    Q_XCB_NOOP(connection());

    if (m_syncingResize) {
        connection()->sync();
        m_syncingResize = false;
        platformWindow->updateSyncRequestCounter();
    } else {
        xcb_flush(xcb_connection());
    }
}

#ifndef QT_NO_OPENGL
void QXcbBackingStore::composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                                       QPlatformTextureList *textures, QOpenGLContext *context)
{
    QPlatformBackingStore::composeAndFlush(window, region, offset, textures, context);

    Q_XCB_NOOP(connection());

    if (m_syncingResize) {
        QXcbWindow *platformWindow = static_cast<QXcbWindow *>(window->handle());
        connection()->sync();
        m_syncingResize = false;
        platformWindow->updateSyncRequestCounter();
    } else {
        xcb_flush(xcb_connection());
    }
}
#endif // QT_NO_OPENGL

void QXcbBackingStore::resize(const QSize &size, const QRegion &)
{
    if (m_image && size == m_image->size())
        return;

    Q_XCB_NOOP(connection());

    QXcbScreen *screen = static_cast<QXcbScreen *>(window()->screen()->handle());
    QPlatformWindow *pw = window()->handle();
    if (!pw) {
        window()->create();
        pw = window()->handle();
    }
    QXcbWindow* win = static_cast<QXcbWindow *>(pw);

    delete m_image;
    m_image = new QXcbShmImage(screen, size, win->depth(), win->imageFormat());
    Q_XCB_NOOP(connection());

    m_syncingResize = true;
}

extern void qt_scrollRectInImage(QImage &img, const QRect &rect, const QPoint &offset);

bool QXcbBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    if (!m_image || m_image->image()->isNull())
        return false;

    m_image->preparePaint(area);

    const QVector<QRect> rects = area.rects();
    for (int i = 0; i < rects.size(); ++i)
        qt_scrollRectInImage(*m_image->image(), rects.at(i), QPoint(dx, dy));

    return true;
}

QT_END_NAMESPACE
