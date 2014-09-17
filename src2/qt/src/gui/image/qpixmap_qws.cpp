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

#include <qpixmap.h>
#include <qapplication.h>
#include <qwidget.h>
#include <qdesktopwidget.h>
#include <qscreen_qws.h>
#include <qwsdisplay_qws.h>
#include <private/qdrawhelper_p.h>
#include <private/qpixmap_raster_p.h>


QT_BEGIN_NAMESPACE

QPixmap QPixmap::grabWindow(WId window, int x, int y, int w, int h)
{
    QWidget *widget = QWidget::find(window);
    if (!widget)
        return QPixmap();

    QRect grabRect = widget->frameGeometry();
    if (!widget->isWindow())
        grabRect.translate(widget->parentWidget()->mapToGlobal(QPoint()));
    if (w < 0)
        w = widget->width() - x;
    if (h < 0)
        h = widget->height() - y;
    grabRect &= QRect(x, y, w, h).translated(widget->mapToGlobal(QPoint()));

    QScreen *screen = qt_screen;
    QDesktopWidget *desktop = QApplication::desktop();
    if (!desktop)
        return QPixmap();
    if (desktop->numScreens() > 1) {
        const int screenNo = desktop->screenNumber(widget);
        if (screenNo != -1)
            screen = qt_screen->subScreens().at(screenNo);
        grabRect = grabRect.translated(-screen->region().boundingRect().topLeft());
    }

    if (screen->pixelFormat() == QImage::Format_Invalid) {
        qWarning("QPixmap::grabWindow(): Unable to copy pixels from framebuffer");
        return QPixmap();
    }

    if (screen->isTransformed()) {
        const QSize screenSize(screen->width(), screen->height());
        grabRect = screen->mapToDevice(grabRect, screenSize);
    }

    QWSDisplay::grab(false);
    QPixmap pixmap;
    QImage img(screen->base(),
               screen->deviceWidth(), screen->deviceHeight(),
               screen->linestep(), screen->pixelFormat());
    img = img.copy(grabRect);
    QWSDisplay::ungrab();

    if (screen->isTransformed()) {
        QMatrix matrix;
        switch (screen->transformOrientation()) {
        case 1: matrix.rotate(90); break;
        case 2: matrix.rotate(180); break;
        case 3: matrix.rotate(270); break;
        default: break;
        }
        img = img.transformed(matrix);
    }

    if (screen->pixelType() == QScreen::BGRPixel)
        img = img.rgbSwapped();

    return QPixmap::fromImage(img);
}

QRgb* QPixmap::clut() const
{
    if (data && data->classId() == QPixmapData::RasterClass) {
        const QRasterPixmapData *d = static_cast<const QRasterPixmapData*>(data.data());
        return d->image.colorTable().data();
    }

    return 0;
}

int QPixmap::numCols() const
{
    return colorCount();
}

int QPixmap::colorCount() const
{
    if (data && data->classId() == QPixmapData::RasterClass) {
        const QRasterPixmapData *d = static_cast<const QRasterPixmapData*>(data.data());
        return d->image.colorCount();
    }

    return 0;
}

const uchar* QPixmap::qwsBits() const
{
    if (data && data->classId() == QPixmapData::RasterClass) {
        const QRasterPixmapData *d = static_cast<const QRasterPixmapData*>(data.data());
        return d->image.bits();
    }

    return 0;
}

int QPixmap::qwsBytesPerLine() const
{
    if (data && data->classId() == QPixmapData::RasterClass) {
        const QRasterPixmapData *d = static_cast<const QRasterPixmapData*>(data.data());
        return d->image.bytesPerLine();
    }

    return 0;
}

QT_END_NAMESPACE
