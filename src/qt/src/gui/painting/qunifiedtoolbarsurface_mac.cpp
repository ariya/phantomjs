/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qunifiedtoolbarsurface_mac_p.h"
#include <private/qt_cocoa_helpers_mac_p.h>
#include <private/qbackingstore_p.h>
#include <private/qmainwindowlayout_p.h>

#include <QDebug>

#ifdef QT_MAC_USE_COCOA

QT_BEGIN_NAMESPACE

QUnifiedToolbarSurface::QUnifiedToolbarSurface(QWidget *widget)
    : QRasterWindowSurface(widget, false), d_ptr(new QUnifiedToolbarSurfacePrivate)
{
    d_ptr->image = 0;
    d_ptr->inSetGeometry = false;

    setGeometry(QRect(QPoint(0, 0), QSize(widget->width(), 100))); // FIXME: Fix height.
}

QUnifiedToolbarSurface::~QUnifiedToolbarSurface()
{
    if (d_ptr->image)
        delete d_ptr->image;
}

QPaintDevice *QUnifiedToolbarSurface::paintDevice()
{
    return &d_ptr->image->image;
}

void QUnifiedToolbarSurface::recursiveRedirect(QObject *object, QWidget *parent_toolbar, const QPoint &offset)
{
    if (object != 0) {
        if (object->isWidgetType()) {
            QWidget *widget = qobject_cast<QWidget *>(object);

            // We redirect the painting only if the widget is in the same window
            // and is not a window in itself.
            if (!(widget->windowType() & Qt::Window)) {
                widget->d_func()->unifiedSurface = this;
                widget->d_func()->isInUnifiedToolbar = true;
                widget->d_func()->toolbar_offset = offset;
                widget->d_func()->toolbar_ancestor = parent_toolbar;

                for (int i = 0; i < object->children().size(); ++i) {
                    recursiveRedirect(object->children().at(i), parent_toolbar, offset);
                }
            }
        }
    }
}

void QUnifiedToolbarSurface::insertToolbar(QWidget *toolbar, const QPoint &offset)
{
    setGeometry(QRect(QPoint(0, 0), QSize(offset.x() + toolbar->width(), 100))); // FIXME
    recursiveRedirect(toolbar, toolbar, offset);
}

// We basically undo what we set in recursiveRedirect().
void QUnifiedToolbarSurface::recursiveRemoval(QObject *object)
{
    if (object != 0) {
        if (object->isWidgetType()) {
            QWidget *widget = qobject_cast<QWidget *>(object);

            // If it's a pop-up or something similar, we don't redirect it.
            if (widget->windowType() & Qt::Window)
                return;

            widget->d_func()->unifiedSurface = 0;
            widget->d_func()->isInUnifiedToolbar = false;
            widget->d_func()->toolbar_offset = QPoint();
            widget->d_func()->toolbar_ancestor = 0;
        }

        for (int i = 0; i < object->children().size(); ++i) {
            recursiveRemoval(object->children().at(i));
        }
    }
}

void QUnifiedToolbarSurface::removeToolbar(QToolBar *toolbar)
{
    recursiveRemoval(toolbar);
}

void QUnifiedToolbarSurface::setGeometry(const QRect &rect)
{
    QWindowSurface::setGeometry(rect);
    Q_D(QUnifiedToolbarSurface);
    d->inSetGeometry = true;
    if (d->image == 0 || d->image->width() < rect.width() || d->image->height() < rect.height())
            prepareBuffer(QImage::Format_ARGB32_Premultiplied, window());
    d->inSetGeometry = false;

    // FIXME: set unified toolbar height.
}

void QUnifiedToolbarSurface::beginPaint(const QRegion &rgn)
{
    QPainter p(&d_ptr->image->image);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    const QVector<QRect> rects = rgn.rects();
    const QColor blank = Qt::transparent;
    for (QVector<QRect>::const_iterator it = rects.begin(); it != rects.end(); ++it) {
        p.fillRect(*it, blank);
    }
}

void QUnifiedToolbarSurface::updateToolbarOffset(QWidget *widget)
{
    QMainWindowLayout *mlayout = qobject_cast<QMainWindowLayout*> (widget->window()->layout());
    if (mlayout)
        mlayout->updateUnifiedToolbarOffset();
}

void QUnifiedToolbarSurface::flush(QWidget *widget, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);

    this->flush(widget);
}

void QUnifiedToolbarSurface::flush(QWidget *widget)
{
    Q_D(QUnifiedToolbarSurface);

    if (!d->image)
        return;

    if (widget->d_func()->flushRequested)
        qt_mac_setNeedsDisplay(widget);
}

void QUnifiedToolbarSurface::prepareBuffer(QImage::Format format, QWidget *widget)
{
    Q_D(QUnifiedToolbarSurface);

    int width = geometry().width();
    int height = 100; // FIXME
    if (d->image) {
        width = qMax(d->image->width(), width);
        height = qMax(d->image->height(), height);
    }

    if (width == 0 || height == 0) {
        delete d->image;
        d->image = 0;
        return;
    }

    QNativeImage *oldImage = d->image;

    d->image = new QNativeImage(width, height, format, false, widget);

    if (oldImage && d->inSetGeometry && hasStaticContents()) {
        // Make sure we use the const version of bits() (no detach).
        const uchar *src = const_cast<const QImage &>(oldImage->image).bits();
        uchar *dst = d->image->image.bits();

        const int srcBytesPerLine = oldImage->image.bytesPerLine();
        const int dstBytesPerLine = d->image->image.bytesPerLine();
        const int bytesPerPixel = oldImage->image.depth() >> 3;

        QRegion staticRegion(staticContents());
        // Make sure we're inside the boundaries of the old image.
        staticRegion &= QRect(0, 0, oldImage->image.width(), oldImage->image.height());
        const QVector<QRect> &rects = staticRegion.rects();
        const QRect *srcRect = rects.constData();

        // Copy the static content of the old image into the new one.
        int numRectsLeft = rects.size();
        do {
            const int bytesOffset = srcRect->x() * bytesPerPixel;
            const int dy = srcRect->y();

            // Adjust src and dst to point to the right offset.
            const uchar *s = src + dy * srcBytesPerLine + bytesOffset;
            uchar *d = dst + dy * dstBytesPerLine + bytesOffset;
            const int numBytes = srcRect->width() * bytesPerPixel;

            int numScanLinesLeft = srcRect->height();
            do {
                ::memcpy(d, s, numBytes);
                d += dstBytesPerLine;
                s += srcBytesPerLine;
            } while (--numScanLinesLeft);

            ++srcRect;
        } while (--numRectsLeft);
    }

    delete oldImage;
}

CGContextRef QUnifiedToolbarSurface::imageContext()
{
    Q_D(QUnifiedToolbarSurface);
    return d->image->cg;
}

void QUnifiedToolbarSurface::renderToolbar(QWidget *widget, bool forceFlush)
{
    QWidget *toolbar = widget->d_func()->toolbar_ancestor;

    updateToolbarOffset(toolbar);
    QRect beginPaintRect(toolbar->d_func()->toolbar_offset.x(), toolbar->d_func()->toolbar_offset.y(), toolbar->geometry().width(), toolbar->geometry().height());
    QRegion beginPaintRegion(beginPaintRect);

    beginPaint(beginPaintRegion);
    toolbar->render(paintDevice(), toolbar->d_func()->toolbar_offset, QRegion(toolbar->geometry()), QWidget::DrawChildren);
    toolbar->d_func()->flushRequested = true;

    if (forceFlush)
        flush(toolbar);
}

QT_END_NAMESPACE

#endif // QT_MAC_USE_COCOA
