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

#include "qpaintdevice.h"
#include "qpainter.h"
#include "qwidget.h"
#include "qbitmap.h"
#include "qapplication.h"
#include "qprinter.h"
#include <qdebug.h>
#include <private/qt_mac_p.h>
#include <private/qprintengine_mac_p.h>
#include <private/qpixmap_mac_p.h>
#include <private/qpixmap_raster_p.h>

QT_BEGIN_NAMESPACE

/*****************************************************************************
  Internal variables and functions
 *****************************************************************************/

/*! \internal */
float qt_mac_defaultDpi_x()
{
    // Mac OS X currently assumes things to be 72 dpi.
    // (see http://developer.apple.com/releasenotes/GraphicsImaging/RN-ResolutionIndependentUI/)
    // This may need to be re-worked as we go further in the resolution-independence stuff.
    return 72;
}

/*! \internal */
float qt_mac_defaultDpi_y()
{
    // Mac OS X currently assumes things to be 72 dpi.
    // (see http://developer.apple.com/releasenotes/GraphicsImaging/RN-ResolutionIndependentUI/)
    // This may need to be re-worked as we go further in the resolution-independence stuff.
    return 72;
}


/*! \internal

    Returns the QuickDraw CGrafPtr of the paint device. 0 is returned
    if it can't be obtained. Do not hold the pointer around for long
    as it can be relocated.

    \warning This function is only available on Mac OS X.
*/

Q_GUI_EXPORT GrafPtr qt_mac_qd_context(const QPaintDevice *device)
{
    if (device->devType() == QInternal::Pixmap) {
        return static_cast<GrafPtr>(static_cast<const QPixmap *>(device)->macQDHandle());
    } else if(device->devType() == QInternal::Widget) {
        return static_cast<GrafPtr>(static_cast<const QWidget *>(device)->macQDHandle());
    } else if(device->devType() == QInternal::Printer) {
        QPaintEngine *engine = static_cast<const QPrinter *>(device)->paintEngine();
        return static_cast<GrafPtr>(static_cast<const QMacPrintEngine *>(engine)->handle());
    }
    return 0;
}

extern CGColorSpaceRef qt_mac_colorSpaceForDeviceType(const QPaintDevice *pdev);

/*! \internal

    Returns the CoreGraphics CGContextRef of the paint device. 0 is
    returned if it can't be obtained. It is the caller's responsiblity to
    CGContextRelease the context when finished using it.

    \warning This function is only available on Mac OS X.
*/

Q_GUI_EXPORT CGContextRef qt_mac_cg_context(const QPaintDevice *pdev)
{
    if (pdev->devType() == QInternal::Pixmap) {
        const QPixmap *pm = static_cast<const QPixmap*>(pdev);
        CGColorSpaceRef colorspace = qt_mac_colorSpaceForDeviceType(pdev);
        uint flags = kCGImageAlphaPremultipliedFirst;
#ifdef kCGBitmapByteOrder32Host //only needed because CGImage.h added symbols in the minor version
        flags |= kCGBitmapByteOrder32Host;
#endif
        CGContextRef ret = 0;

        // It would make sense to put this into a mac #ifdef'ed
        // virtual function in the QPixmapData at some point
        if (pm->data->classId() == QPixmapData::MacClass) {
            const QMacPixmapData *pmData = static_cast<const QMacPixmapData*>(pm->data.data());
            ret = CGBitmapContextCreate(pmData->pixels, pmData->w, pmData->h,
                                                     8, pmData->bytesPerRow, colorspace,
                                                     flags);
            if(!ret)
                qWarning("QPaintDevice: Unable to create context for pixmap (%d/%d/%d)",
                         pmData->w, pmData->h, (pmData->bytesPerRow * pmData->h));
        } else if (pm->data->classId() == QPixmapData::RasterClass) {
            QImage *image = pm->data->buffer();
            ret = CGBitmapContextCreate(image->bits(), image->width(), image->height(),
                                        8, image->bytesPerLine(), colorspace, flags);
        }

        CGContextTranslateCTM(ret, 0, pm->height());
        CGContextScaleCTM(ret, 1, -1);
        return ret;
    } else if (pdev->devType() == QInternal::Widget) {
        CGContextRef ret = static_cast<CGContextRef>(static_cast<const QWidget *>(pdev)->macCGHandle());
        CGContextRetain(ret);
        return ret;
    } else if (pdev->devType() == QInternal::MacQuartz) {
        return static_cast<const QMacQuartzPaintDevice *>(pdev)->cgContext();
    }
    return 0;
}

QT_END_NAMESPACE
