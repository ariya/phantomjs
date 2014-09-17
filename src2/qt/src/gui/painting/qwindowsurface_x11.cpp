/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <QtGui/QPaintDevice>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QWidget>

#include "private/qt_x11_p.h"
#include "private/qpixmap_x11_p.h"
#include "private/qwidget_p.h"
#include "qx11info_x11.h"
#include "qwindowsurface_x11_p.h"

QT_BEGIN_NAMESPACE

extern void *qt_getClipRects(const QRegion &r, int &num); // in qpaintengine_x11.cpp

struct QX11WindowSurfacePrivate
{
    QWidget *widget;
    QPixmap device;
#ifndef QT_NO_XRENDER
    bool translucentBackground;
#endif
};

QX11WindowSurface::QX11WindowSurface(QWidget *widget)
    : QWindowSurface(widget), d_ptr(new QX11WindowSurfacePrivate), gc(0)
{
    d_ptr->widget = widget;
#ifndef QT_NO_XRENDER
    d_ptr->translucentBackground = X11->use_xrender
        && widget->x11Info().depth() == 32;
#endif
}


QX11WindowSurface::~QX11WindowSurface()
{
    delete d_ptr;
    if (gc) {
        XFreeGC(X11->display, gc);
        gc = 0;
    }
}

QPaintDevice *QX11WindowSurface::paintDevice()
{
    return &d_ptr->device;
}

void QX11WindowSurface::beginPaint(const QRegion &rgn)
{
#ifndef QT_NO_XRENDER
    Q_ASSERT(!d_ptr->device.isNull());

    if (d_ptr->translucentBackground) {
        if (d_ptr->device.depth() != 32)
            static_cast<QX11PixmapData *>(d_ptr->device.data_ptr().data())->convertToARGB32();
        ::Picture src = X11->getSolidFill(d_ptr->device.x11Info().screen(), Qt::transparent);
        ::Picture dst = d_ptr->device.x11PictureHandle();
        const QVector<QRect> rects = rgn.rects();
        const int w = d_ptr->device.width();
        const int h = d_ptr->device.height();
        for (QVector<QRect>::const_iterator it = rects.begin(); it != rects.end(); ++it)
            XRenderComposite(X11->display, PictOpSrc, src, 0, dst,
                             0, 0, w, h, it->x(), it->y(),
                             it->width(), it->height());
    }
#endif
}

void QX11WindowSurface::flush(QWidget *widget, const QRegion &rgn, const QPoint &offset)
{
    if (d_ptr->device.isNull())
        return;

    QPoint wOffset = qt_qwidget_data(widget)->wrect.topLeft();
    QRegion wrgn(rgn);
    QRect br = rgn.boundingRect();
    if (!wOffset.isNull())
        wrgn.translate(-wOffset);
    QRect wbr = wrgn.boundingRect();

    int num;
    XRectangle *rects = (XRectangle *)qt_getClipRects(wrgn, num);
    if (num <= 0)
        return;
//         qDebug() << "XSetClipRectangles";
//         for  (int i = 0; i < num; ++i)
//             qDebug() << ' ' << i << rects[i].x << rects[i].x << rects[i].y << rects[i].width << rects[i].height;
    if (num != 1)
        XSetClipRectangles(X11->display, gc, 0, 0, rects, num, YXBanded);
    XCopyArea(X11->display, d_ptr->device.handle(), widget->handle(), gc,
              br.x() + offset.x(), br.y() + offset.y(), br.width(), br.height(), wbr.x(), wbr.y());
    if (num != 1)
        XSetClipMask(X11->display, gc, XNone);
}

void QX11WindowSurface::setGeometry(const QRect &rect)
{
    QWindowSurface::setGeometry(rect);

    const QSize size = rect.size();

    if (d_ptr->device.size() == size || size.width() <= 0 || size.height() <= 0)
        return;
#ifndef QT_NO_XRENDER
    if (d_ptr->translucentBackground) {
        QPixmap::x11SetDefaultScreen(d_ptr->widget->x11Info().screen());

        QX11PixmapData *data = new QX11PixmapData(QPixmapData::PixmapType);
        data->xinfo = d_ptr->widget->x11Info();
        data->resize(size.width(), size.height());
        d_ptr->device = QPixmap(data);
    } else
#endif
    {
        QPixmap::x11SetDefaultScreen(d_ptr->widget->x11Info().screen());

        QX11PixmapData *oldData = static_cast<QX11PixmapData *>(d_ptr->device.pixmapData());

        if (oldData && !(oldData->flags & QX11PixmapData::Uninitialized) && hasStaticContents()) {
            // Copy the content of the old pixmap into the new one.
            QX11PixmapData *newData = new QX11PixmapData(QPixmapData::PixmapType);
            newData->resize(size.width(), size.height());
            Q_ASSERT(oldData->d == newData->d);

            QRegion staticRegion(staticContents());
            // Make sure we're inside the boundaries of the old pixmap.
            staticRegion &= QRect(0, 0, oldData->w, oldData->h);
            const QRect boundingRect(staticRegion.boundingRect());
            const int dx = boundingRect.x();
            const int dy = boundingRect.y();

            int num;
            XRectangle *rects = (XRectangle *)qt_getClipRects(staticRegion, num);
            GC tmpGc = XCreateGC(X11->display, oldData->hd, 0, 0);
            XSetClipRectangles(X11->display, tmpGc, 0, 0, rects, num, YXBanded);
            XCopyArea(X11->display, oldData->hd, newData->hd, tmpGc,
                      dx, dy, qMin(boundingRect.width(), size.width()),
                      qMin(boundingRect.height(), size.height()), dx, dy);
            XFreeGC(X11->display, tmpGc);
            newData->flags &= ~QX11PixmapData::Uninitialized;

            d_ptr->device = QPixmap(newData);
        } else {
            d_ptr->device = QPixmap(size);
        }
    }

    if (gc) {
        XFreeGC(X11->display, gc);
        gc = 0;
    }
    if (!d_ptr->device.isNull()) {
        gc = XCreateGC(X11->display, d_ptr->device.handle(), 0, 0);
        XSetGraphicsExposures(X11->display, gc, False);
    }
}

bool QX11WindowSurface::scroll(const QRegion &area, int dx, int dy)
{
    QRect rect = area.boundingRect();

    if (d_ptr->device.isNull())
        return false;

    GC gc = XCreateGC(X11->display, d_ptr->device.handle(), 0, 0);
    XCopyArea(X11->display, d_ptr->device.handle(), d_ptr->device.handle(), gc,
              rect.x(), rect.y(), rect.width(), rect.height(),
              rect.x()+dx, rect.y()+dy);
    XFreeGC(X11->display, gc);

    return true;
}

QPixmap QX11WindowSurface::grabWidget(const QWidget *widget,
                                      const QRect& rect) const
{
    if (!widget || d_ptr->device.isNull())
        return QPixmap();

    QRect srcRect;

    // make sure the rect is inside the widget & clip to widget's rect
    if (!rect.isEmpty())
        srcRect = rect & widget->rect();
    else
        srcRect = widget->rect();

    if (srcRect.isEmpty())
        return QPixmap();

    // If it's a child widget we have to translate the coordinates
    if (widget != window())
        srcRect.translate(widget->mapTo(window(), QPoint(0, 0)));

    QPixmap::x11SetDefaultScreen(widget->x11Info().screen());
    QPixmap px(srcRect.width(), srcRect.height());

    GC tmpGc = XCreateGC(X11->display, d_ptr->device.handle(), 0, 0);

    // Copy srcRect from the backing store to the new pixmap
    XSetGraphicsExposures(X11->display, tmpGc, False);
    XCopyArea(X11->display, d_ptr->device.handle(), px.handle(), tmpGc,
              srcRect.x(), srcRect.y(), srcRect.width(), srcRect.height(), 0, 0);

    XFreeGC(X11->display, tmpGc);

    return px;
}

QWindowSurface::WindowSurfaceFeatures QX11WindowSurface::features() const
{
    WindowSurfaceFeatures features = QWindowSurface::PartialUpdates | QWindowSurface::PreservedContents;
#ifndef QT_NO_XRENDER
    if (!d_ptr->translucentBackground)
        features |= QWindowSurface::StaticContents;
#else
    features |= QWindowSurface::StaticContents;
#endif
    return features;
}

QT_END_NAMESPACE
