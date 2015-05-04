/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
//#include <QDebug>
#include "qkmscursor.h"
#include "qkmsscreen.h"
#include "qkmsdevice.h"

QT_BEGIN_NAMESPACE

#ifndef DRM_CAP_CURSOR_WIDTH
#define DRM_CAP_CURSOR_WIDTH 0x8
#endif

#ifndef DRM_CAP_CURSOR_HEIGHT
#define DRM_CAP_CURSOR_HEIGHT 0x9
#endif

QKmsCursor::QKmsCursor(QKmsScreen *screen)
    : m_screen(screen),
      m_graphicsBufferManager(screen->device()->gbmDevice()),
      m_cursorImage(new QPlatformCursorImage(0, 0, 0, 0, 0, 0)),
      m_moved(false),
      m_cursorSize(64, 64)
{
    uint64_t value = 0;
    if (!drmGetCap(m_screen->device()->fd(), DRM_CAP_CURSOR_WIDTH, &value))
        m_cursorSize.setWidth(value);
    if (!drmGetCap(m_screen->device()->fd(), DRM_CAP_CURSOR_HEIGHT, &value))
        m_cursorSize.setHeight(value);

    m_cursorBufferObject = gbm_bo_create(m_graphicsBufferManager, m_cursorSize.width(), m_cursorSize.height(),
                                         GBM_FORMAT_ARGB8888, GBM_BO_USE_CURSOR_64X64 | GBM_BO_USE_WRITE);
}

QKmsCursor::~QKmsCursor()
{
    drmModeSetCursor(m_screen->device()->fd(), m_screen->crtcId(), 0, 0, 0);
    gbm_bo_destroy(m_cursorBufferObject);
}

void QKmsCursor::pointerEvent(const QMouseEvent &event)
{
    m_moved = true;
    int status = drmModeMoveCursor(m_screen->device()->fd(),
                                   m_screen->crtcId(),
                                   event.globalX(),
                                   event.globalY());
    if (status) {
        qWarning("failed to move cursor: %d", status);
    }
}

void QKmsCursor::changeCursor(QCursor *windowCursor, QWindow *window)
{
    Q_UNUSED(window)

    if (!m_moved)
        drmModeMoveCursor(m_screen->device()->fd(), m_screen->crtcId(), 0, 0);

    const Qt::CursorShape newShape = windowCursor ? windowCursor->shape() : Qt::ArrowCursor;
    if (newShape != Qt::BitmapCursor) {
        m_cursorImage->set(newShape);
    } else {
        m_cursorImage->set(windowCursor->pixmap().toImage(),
                           windowCursor->hotSpot().x(),
                           windowCursor->hotSpot().y());
    }

    if (m_cursorImage->image()->width() > m_cursorSize.width() || m_cursorImage->image()->width() > m_cursorSize.height())
        qWarning("cursor larger than %dx%d, cursor truncated", m_cursorSize.width(), m_cursorSize.height());

    QImage cursorImage = m_cursorImage->image()->convertToFormat(QImage::Format_ARGB32)
        .copy(0, 0, m_cursorSize.width(), m_cursorSize.height());
    gbm_bo_write(m_cursorBufferObject, cursorImage.constBits(), cursorImage.byteCount());

    quint32 handle = gbm_bo_get_handle(m_cursorBufferObject).u32;
    int status = drmModeSetCursor(m_screen->device()->fd(),
                                  m_screen->crtcId(), handle,
                                  m_cursorSize.width(), m_cursorSize.height());

    if (status) {
        qWarning("failed to set cursor: %d", status);
    }
}

QT_END_NAMESPACE
