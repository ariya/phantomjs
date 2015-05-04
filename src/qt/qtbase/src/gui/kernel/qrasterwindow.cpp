/****************************************************************************
 **
 ** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
 ** Contact: http://www.qt-project.org/legal
 **
 ** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qrasterwindow.h"

#include <QtGui/private/qpaintdevicewindow_p.h>

#include <QtGui/QBackingStore>
#include <QtGui/QPainter>

QT_BEGIN_NAMESPACE

/*!
  \class QRasterWindow
  \inmodule QtGui
  \since 5.4
  \brief QRasterWindow is a convenience class for using QPainter on a QWindow

  QRasterWindow is a QWindow with a raster-based, non-OpenGL surface. On top of
  the functionality offered by QWindow, QRasterWindow adds a virtual
  paintEvent() function and the possibility to open a QPainter on itself. The
  underlying paint engine will be the raster one, meaning that all drawing will
  happen on the CPU. For performing accelerated, OpenGL-based drawing, use
  QOpenGLWindow instead.

  Internally the class is thin wrapper for QWindow and QBackingStore
  and is very similar to the \l{Raster Window Example}{Raster Window
  Example} that uses these classes directly.

  \sa QPaintDeviceWindow::paintEvent(), QPaintDeviceWindow::update()
*/

class QRasterWindowPrivate : public QPaintDeviceWindowPrivate
{
    Q_DECLARE_PUBLIC(QRasterWindow)
public:
    void beginPaint(const QRegion &region) Q_DECL_OVERRIDE
    {
        Q_Q(QRasterWindow);
        if (backingstore->size() != q->size()) {
            backingstore->resize(q->size());
            markWindowAsDirty();
        }
        backingstore->beginPaint(region);
    }

    void endPaint() Q_DECL_OVERRIDE
    {
        backingstore->endPaint();
    }

    void flush(const QRegion &region) Q_DECL_OVERRIDE
    {
        Q_Q(QRasterWindow);
        backingstore->flush(region, q);
    }

    QScopedPointer<QBackingStore> backingstore;
};

/*!
  Constructs a new QRasterWindow with \a parent.
*/
QRasterWindow::QRasterWindow(QWindow *parent)
    : QPaintDeviceWindow(*(new QRasterWindowPrivate), parent)
{
    setSurfaceType(QSurface::RasterSurface);
    d_func()->backingstore.reset(new QBackingStore(this));
}

/*!
  \internal
*/
int QRasterWindow::metric(PaintDeviceMetric metric) const
{
    Q_D(const QRasterWindow);

    switch (metric) {
    case PdmDepth:
        return d->backingstore->paintDevice()->depth();
    case PdmDevicePixelRatio:
        return d->backingstore->paintDevice()->devicePixelRatio();
    default:
        break;
    }
    return QPaintDeviceWindow::metric(metric);
}

/*!
  \internal
*/
QPaintDevice *QRasterWindow::redirected(QPoint *) const
{
    Q_D(const QRasterWindow);
    return d->backingstore->paintDevice();
}

QT_END_NAMESPACE
