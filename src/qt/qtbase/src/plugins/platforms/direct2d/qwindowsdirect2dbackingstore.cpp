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

#include "qwindowsdirect2dbackingstore.h"
#include "qwindowsdirect2dintegration.h"
#include "qwindowsdirect2dcontext.h"
#include "qwindowsdirect2dpaintdevice.h"
#include "qwindowsdirect2dbitmap.h"
#include "qwindowsdirect2ddevicecontext.h"
#include "qwindowsdirect2dwindow.h"

#include "qwindowscontext.h"

#include <QtGui/QWindow>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsDirect2DBackingStore
    \brief Backing store for windows.
    \internal
    \ingroup qt-lighthouse-win
*/

static inline QWindowsDirect2DPlatformPixmap *platformPixmap(QPixmap *p)
{
    return static_cast<QWindowsDirect2DPlatformPixmap *>(p->handle());
}

QWindowsDirect2DBackingStore::QWindowsDirect2DBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
}

QWindowsDirect2DBackingStore::~QWindowsDirect2DBackingStore()
{
}

QPaintDevice *QWindowsDirect2DBackingStore::paintDevice()
{
    return m_pixmap.data();
}

void QWindowsDirect2DBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    QPlatformWindow *pw = window->handle();
    if (pw && m_pixmap)
        static_cast<QWindowsDirect2DWindow *>(pw)->flush(platformPixmap(m_pixmap.data())->bitmap(), region, offset);
}

void QWindowsDirect2DBackingStore::resize(const QSize &size, const QRegion &region)
{
    Q_UNUSED(region);

    QScopedPointer<QPixmap> oldPixmap(m_pixmap.take());
    m_pixmap.reset(new QPixmap(size.width(), size.height()));

    if (oldPixmap) {
        foreach (const QRect &rect, region.rects())
            platformPixmap(m_pixmap.data())->copy(oldPixmap->handle(), rect);
    }
}

QT_END_NAMESPACE
