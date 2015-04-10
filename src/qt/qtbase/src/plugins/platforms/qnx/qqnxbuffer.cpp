/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
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

#include "qqnxbuffer.h"

#include <QtCore/QDebug>

#include <errno.h>
#include <sys/mman.h>

#if defined(QQNXBUFFER_DEBUG)
#define qBufferDebug qDebug
#else
#define qBufferDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxBuffer::QQnxBuffer()
    : m_buffer(0)
{
    qBufferDebug() << Q_FUNC_INFO << "empty";
}

QQnxBuffer::QQnxBuffer(screen_buffer_t buffer)
    : m_buffer(buffer)
{
    qBufferDebug() << Q_FUNC_INFO << "normal";

    // Get size of buffer
    int size[2];
    Q_SCREEN_CRITICALERROR(screen_get_buffer_property_iv(buffer, SCREEN_PROPERTY_BUFFER_SIZE, size),
                        "Failed to query buffer size");

    // Get stride of buffer
    int stride;
    Q_SCREEN_CHECKERROR(screen_get_buffer_property_iv(buffer, SCREEN_PROPERTY_STRIDE, &stride),
                        "Failed to query buffer stride");

    // Get access to buffer's data
    errno = 0;
    uchar *dataPtr = 0;
    Q_SCREEN_CRITICALERROR(
            screen_get_buffer_property_pv(buffer, SCREEN_PROPERTY_POINTER, (void **)&dataPtr),
            "Failed to query buffer pointer");

    if (dataPtr == 0)
        qFatal("QQNX: buffer pointer is NULL, errno=%d", errno);

    // Get format of buffer
    int screenFormat;
    Q_SCREEN_CHECKERROR(
            screen_get_buffer_property_iv(buffer, SCREEN_PROPERTY_FORMAT, &screenFormat),
            "Failed to query buffer format");

    // Convert screen format to QImage format
    QImage::Format imageFormat = QImage::Format_Invalid;
    switch (screenFormat) {
    case SCREEN_FORMAT_RGBX4444:
        imageFormat = QImage::Format_RGB444;
        break;
    case SCREEN_FORMAT_RGBA4444:
        imageFormat = QImage::Format_ARGB4444_Premultiplied;
        break;
    case SCREEN_FORMAT_RGBX5551:
        imageFormat = QImage::Format_RGB555;
        break;
    case SCREEN_FORMAT_RGB565:
        imageFormat = QImage::Format_RGB16;
        break;
    case SCREEN_FORMAT_RGBX8888:
        imageFormat = QImage::Format_RGB32;
        break;
    case SCREEN_FORMAT_RGBA8888:
        imageFormat = QImage::Format_ARGB32_Premultiplied;
        break;
    default:
        qFatal("QQNX: unsupported buffer format, format=%d", screenFormat);
    }

    // wrap buffer in an image
    m_image = QImage(dataPtr, size[0], size[1], stride, imageFormat);
}

QQnxBuffer::QQnxBuffer(const QQnxBuffer &other)
    : m_buffer(other.m_buffer),
      m_image(other.m_image)
{
    qBufferDebug() << Q_FUNC_INFO << "copy";
}

QQnxBuffer::~QQnxBuffer()
{
    qBufferDebug() << Q_FUNC_INFO;
}

void QQnxBuffer::invalidateInCache()
{
    qBufferDebug() << Q_FUNC_INFO;

    // Verify native buffer exists
    if (m_buffer == 0)
        qFatal("QQNX: can't invalidate cache for null buffer");

    // Evict buffer's data from cache
    errno = 0;
    int result = msync(m_image.bits(), m_image.height() * m_image.bytesPerLine(), MS_INVALIDATE | MS_CACHE_ONLY);
    if (result != 0)
        qFatal("QQNX: failed to invalidate cache, errno=%d", errno);
}

QT_END_NAMESPACE
