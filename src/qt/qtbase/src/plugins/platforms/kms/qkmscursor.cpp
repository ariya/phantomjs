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
//#include <QDebug>
#include "qkmscursor.h"
#include "qkmsscreen.h"
#include "qkmsdevice.h"

QT_BEGIN_NAMESPACE

QKmsCursor::QKmsCursor(QKmsScreen *screen)
    : m_screen(screen),
      m_graphicsBufferManager(screen->device()->gbmDevice()),
      m_cursorBufferObject(gbm_bo_create(m_graphicsBufferManager, 64, 64, GBM_FORMAT_ARGB8888,
                                         GBM_BO_USE_CURSOR_64X64|GBM_BO_USE_WRITE)),
      m_cursorImage(new QPlatformCursorImage(0, 0, 0, 0, 0, 0)),
      m_moved(false)
{
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

    if ((m_cursorImage->image()->width() > 64) || (m_cursorImage->image()->width() > 64))
        qWarning("Warning: cursor larger than 64x64; only 64x64 pixels will be shown.");

    QImage cursorImage = m_cursorImage->image()->
            convertToFormat(QImage::Format_ARGB32).copy(0, 0, 64, 64);
    gbm_bo_write(m_cursorBufferObject, cursorImage.constBits(), cursorImage.byteCount());

    quint32 handle = gbm_bo_get_handle(m_cursorBufferObject).u32;
    int status = drmModeSetCursor(m_screen->device()->fd(),
                                  m_screen->crtcId(), handle, 64, 64);

    if (status) {
        qWarning("failed to set cursor: %d", status);
    }
}

QT_END_NAMESPACE
