/***************************************************************************
**
** Copyright (C) 2013 - 2014 BlackBerry Limited. All rights reserved.
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

#include "qqnxglobal.h"

#include "qqnxrasterwindow.h"
#include "qqnxscreen.h"

#include <QDebug>

#include <errno.h>

#if defined(QQNXRASTERWINDOW_DEBUG)
#define qRasterWindowDebug qDebug
#else
#define qRasterWindowDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxRasterWindow::QQnxRasterWindow(QWindow *window, screen_context_t context, bool needRootWindow) :
    QQnxWindow(window, context, needRootWindow),
    m_currentBufferIndex(-1),
    m_previousBufferIndex(-1)
{
    initWindow();

    // Set window usage
    const int val = SCREEN_USAGE_NATIVE | SCREEN_USAGE_READ | SCREEN_USAGE_WRITE;
    const int result = screen_set_window_property_iv(nativeHandle(), SCREEN_PROPERTY_USAGE, &val);
    if (result != 0)
        qFatal("QQnxEglWindow: failed to set window alpha usage, errno=%d", errno);
}

void QQnxRasterWindow::post(const QRegion &dirty)
{
    // How double-buffering works
    // --------------------------
    //
    // The are two buffers, the previous one and the current one.
    // The previous buffer always contains the complete, full image of the whole window when it
    // was last posted.
    // The current buffer starts with the complete, full image of the second to last posting
    // of the window.
    //
    // During painting, Qt paints on the current buffer. Thus, when Qt has finished painting, the
    // current buffer contains the second to last image plus the newly painted regions.
    // Since the second to last image is too old, we copy over the image from the previous buffer, but
    // only for those regions that Qt didn't paint (because that would overwrite what Qt has just
    // painted). This is the copyPreviousToCurrent() call below.
    //
    // After the call to copyPreviousToCurrent(), the current buffer contains the complete, full image of the
    // whole window in its current state, and we call screen_post_window() to make the new buffer
    // available to libscreen (called "posting"). There, only the regions that Qt painted on are
    // posted, as nothing else has changed.
    //
    // After that, the previous and the current buffers are swapped, and the whole cycle starts anew.

    // Check if render buffer exists and something was rendered
    if (m_currentBufferIndex != -1 && !dirty.isEmpty()) {
        qRasterWindowDebug() << Q_FUNC_INFO << "window =" << window();
        QQnxBuffer &currentBuffer = m_buffers[m_currentBufferIndex];

        // Copy unmodified region from old render buffer to new render buffer;
        // required to allow partial updates
        QRegion preserve = m_previousDirty - dirty - m_scrolled;
        blitPreviousToCurrent(preserve, 0, 0);

        // Calculate region that changed
        QRegion modified = preserve + dirty + m_scrolled;
        QRect rect = modified.boundingRect();
        int dirtyRect[4] = { rect.x(), rect.y(), rect.x() + rect.width(), rect.y() + rect.height() };

        // Update the display with contents of render buffer
        Q_SCREEN_CHECKERROR(
                screen_post_window(nativeHandle(), currentBuffer.nativeBuffer(), 1, dirtyRect, 0),
                "Failed to post window");

        // Advance to next nender buffer
        m_previousBufferIndex = m_currentBufferIndex++;
        if (m_currentBufferIndex >= MAX_BUFFER_COUNT)
            m_currentBufferIndex = 0;

        // Save modified region and clear scrolled region
        m_previousDirty = dirty;
        m_scrolled = QRegion();

        windowPosted();
    }
}

void QQnxRasterWindow::scroll(const QRegion &region, int dx, int dy, bool flush)
{
    qRasterWindowDebug() << Q_FUNC_INFO << "window =" << window();
    blitPreviousToCurrent(region, dx, dy, flush);
    m_scrolled += region;
}

QQnxBuffer &QQnxRasterWindow::renderBuffer()
{
    qRasterWindowDebug() << Q_FUNC_INFO << "window =" << window();

    // Check if render buffer is invalid
    if (m_currentBufferIndex == -1) {
        // Get all buffers available for rendering
        screen_buffer_t buffers[MAX_BUFFER_COUNT];
        const int result = screen_get_window_property_pv(nativeHandle(), SCREEN_PROPERTY_RENDER_BUFFERS,
                                                   (void **)buffers);
        Q_SCREEN_CRITICALERROR(result, "Failed to query window buffers");

        // Wrap each buffer and clear
        for (int i = 0; i < MAX_BUFFER_COUNT; ++i) {
            m_buffers[i] = QQnxBuffer(buffers[i]);

            // Clear Buffer
            int bg[] = { SCREEN_BLIT_COLOR, 0x00000000, SCREEN_BLIT_END };
            Q_SCREEN_CHECKERROR(screen_fill(screen()->nativeContext(), buffers[i], bg),
                                "Failed to clear window buffer");
        }

        Q_SCREEN_CHECKERROR(screen_flush_blits(screen()->nativeContext(), 0),
                            "Failed to flush blits");

        // Use the first available render buffer
        m_currentBufferIndex = 0;
        m_previousBufferIndex = -1;
    }

    return m_buffers[m_currentBufferIndex];
}

void QQnxRasterWindow::setParent(const QPlatformWindow *wnd)
{
    QQnxWindow::setParent(wnd);
    adjustBufferSize();
}

void QQnxRasterWindow::adjustBufferSize()
{
    // When having a raster window we don't need any buffers, since
    // Qt will draw to the parent TLW backing store.
    const QSize windowSize = window()->parent() ? QSize(1,1) : window()->size();
    if (windowSize != bufferSize())
        setBufferSize(windowSize);
}

int QQnxRasterWindow::pixelFormat() const
{
    return screen()->nativeFormat();
}

void QQnxRasterWindow::resetBuffers()
{
    // Buffers were destroyed; reacquire them
    m_currentBufferIndex = -1;
    m_previousDirty = QRegion();
    m_scrolled = QRegion();
}

void QQnxRasterWindow::blitPreviousToCurrent(const QRegion &region, int dx, int dy, bool flush)
{
    qRasterWindowDebug() << Q_FUNC_INFO << "window =" << window();

    // Abort if previous buffer is invalid or if nothing to copy
    if (m_previousBufferIndex == -1 || region.isEmpty())
        return;

    QQnxBuffer &currentBuffer = m_buffers[m_currentBufferIndex];
    QQnxBuffer &previousBuffer = m_buffers[m_previousBufferIndex];

    // Break down region into non-overlapping rectangles
    const QVector<QRect> rects = region.rects();
    for (int i = rects.size() - 1; i >= 0; i--) {
        // Clip rectangle to bounds of target
        const QRect rect = rects[i].intersected(currentBuffer.rect());

        if (rect.isEmpty())
            continue;

        // Setup blit operation
        int attribs[] = { SCREEN_BLIT_SOURCE_X, rect.x(),
                          SCREEN_BLIT_SOURCE_Y, rect.y(),
                          SCREEN_BLIT_SOURCE_WIDTH, rect.width(),
                          SCREEN_BLIT_SOURCE_HEIGHT, rect.height(),
                          SCREEN_BLIT_DESTINATION_X, rect.x() + dx,
                          SCREEN_BLIT_DESTINATION_Y, rect.y() + dy,
                          SCREEN_BLIT_DESTINATION_WIDTH, rect.width(),
                          SCREEN_BLIT_DESTINATION_HEIGHT, rect.height(),
                          SCREEN_BLIT_END };

        // Queue blit operation
        Q_SCREEN_CHECKERROR(screen_blit(m_screenContext, currentBuffer.nativeBuffer(),
                                       previousBuffer.nativeBuffer(), attribs),
                            "Failed to blit buffers");
    }

    // Check if flush requested
    if (flush) {
        // Wait for all blits to complete
        Q_SCREEN_CHECKERROR(screen_flush_blits(m_screenContext, SCREEN_WAIT_IDLE),
                            "Failed to flush blits");

        // Buffer was modified outside the CPU
        currentBuffer.invalidateInCache();
    }
}

QT_END_NAMESPACE
