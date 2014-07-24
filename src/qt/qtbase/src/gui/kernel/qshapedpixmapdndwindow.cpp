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

#include "qshapedpixmapdndwindow_p.h"

#include <QtGui/QPainter>
#include <QtGui/QCursor>

QT_BEGIN_NAMESPACE

QShapedPixmapWindow::QShapedPixmapWindow()
    : QWindow(),
      m_backingStore(0)
{
    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    setFormat(format);
    setSurfaceType(RasterSurface);
    setFlags(Qt::ToolTip | Qt::FramelessWindowHint |
                   Qt::X11BypassWindowManagerHint | Qt::WindowTransparentForInput);
    create();
    m_backingStore = new QBackingStore(this);
}

void QShapedPixmapWindow::render()
{
    QRect rect(QPoint(), geometry().size());

    m_backingStore->beginPaint(rect);

    QPaintDevice *device = m_backingStore->paintDevice();

    {
        QPainter p(device);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.drawPixmap(0, 0, m_pixmap);
    }

    m_backingStore->endPaint();
    m_backingStore->flush(rect);
}

void QShapedPixmapWindow::setPixmap(const QPixmap &pixmap)
{
    m_pixmap = pixmap;
}

void QShapedPixmapWindow::setHotspot(const QPoint &hotspot)
{
    m_hotSpot = hotspot;
}

void QShapedPixmapWindow::updateGeometry()
{
    QRect rect(QCursor::pos() - m_hotSpot, m_pixmap.size());
    if (m_pixmap.isNull())
        m_backingStore->resize(QSize(1,1));
    else if (m_backingStore->size() != m_pixmap.size())
        m_backingStore->resize(m_pixmap.size());
    setGeometry(rect);
}

void QShapedPixmapWindow::exposeEvent(QExposeEvent *)
{
    render();
}

QT_END_NAMESPACE
