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

#ifndef QWINDOWSBACKINGSTORE_H
#define QWINDOWSBACKINGSTORE_H

#include "qtwindows_additional.h"
#include "qwindowsscaling.h"

#include <qpa/qplatformbackingstore.h>
#include <QtCore/QScopedPointer>

QT_BEGIN_NAMESPACE

class QWindowsWindow;
class QWindowsNativeImage;

class QWindowsBackingStore : public QPlatformBackingStore
{
    Q_DISABLE_COPY(QWindowsBackingStore)
public:
    QWindowsBackingStore(QWindow *window);
    ~QWindowsBackingStore();

    QPaintDevice *paintDevice() Q_DECL_OVERRIDE;
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) Q_DECL_OVERRIDE
    {
        flushDp(window, QWindowsScaling::mapToNative(region.boundingRect()),
                offset * QWindowsScaling::factor());
    }
    void flushDp(QWindow *window, const QRect &boundingRect, const QPoint &offset);
    void resize(const QSize &size, const QRegion &r) Q_DECL_OVERRIDE;
    bool scroll(const QRegion &area, int dx, int dy) Q_DECL_OVERRIDE;
    void beginPaint(const QRegion &) Q_DECL_OVERRIDE;

    HDC getDC() const;

#ifndef QT_NO_OPENGL
    QImage toImage() const Q_DECL_OVERRIDE;
#endif

private:
    QScopedPointer<QWindowsNativeImage> m_image;
};

QT_END_NAMESPACE

#endif // QWINDOWSBACKINGSTORE_H
