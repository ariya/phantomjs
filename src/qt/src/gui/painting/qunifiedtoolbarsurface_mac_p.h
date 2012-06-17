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

#ifndef QUNIFIEDTOOLBARSURFACE_MAC_P_H
#define QUNIFIEDTOOLBARSURFACE_MAC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qwindowsurface_raster_p.h>
#include <QWidget>
#include <QToolBar>
#include <private/qwidget_p.h>
#include <private/qnativeimage_p.h>

#ifdef QT_MAC_USE_COCOA

QT_BEGIN_NAMESPACE

class QNativeImage;

//
// This is the implementation of the unified toolbar on Mac OS X
// with the graphics system raster.
//
// General idea:
// -------------
// We redirect the painting of widgets inside the unified toolbar
// to a special window surface, the QUnifiedToolbarSurface.
// We need a separate window surface because the unified toolbar
// is out of the content view.
// The input system is the same as for the unified toolbar with the
// native (CoreGraphics) engine.
//
// Execution flow:
// ---------------
// The unified toolbar is triggered by QMainWindow::setUnifiedTitleAndToolBarOnMac().
// It calls QMainWindowLayout::insertIntoMacToolbar() which will
// set all the appropriate variables (offsets, redirection, ...).
// When Qt tells a widget to repaint, QWidgetPrivate::drawWidget()
// checks if the widget is inside the unified toolbar and exits without
// painting is that is the case.
// We trigger the rendering of the unified toolbar in QWidget::repaint()
// and QWidget::update().
// We keep track of flush requests via "flushRequested" variable. That
// allow flush() to be a no-op if no repaint occurred for a widget.
// We rely on the needsDisplay: and drawRect: mecanism for drawing our
// content into the graphics context.
//
// Notes:
// ------
// The painting of items inside the unified toolbar is expensive.
// Too many repaints will drastically slow down the whole application.
//

class QUnifiedToolbarSurfacePrivate
{
public:
    QNativeImage *image;
    uint inSetGeometry : 1;
};

class Q_GUI_EXPORT QUnifiedToolbarSurface : public QRasterWindowSurface
{
public:
    QUnifiedToolbarSurface(QWidget *widget);
    ~QUnifiedToolbarSurface();

    void flush(QWidget *widget);
    void flush(QWidget *widget, const QRegion &region, const QPoint &offset);
    void setGeometry(const QRect &rect);
    void beginPaint(const QRegion &rgn);
    void insertToolbar(QWidget *toolbar, const QPoint &offset);
    void removeToolbar(QToolBar *toolbar);
    void updateToolbarOffset(QWidget *widget);
    void renderToolbar(QWidget *widget, bool forceFlush = false);
    void recursiveRedirect(QObject *widget, QWidget *parent_toolbar, const QPoint &offset);

    QPaintDevice *paintDevice();
    CGContextRef imageContext();

private:
    void prepareBuffer(QImage::Format format, QWidget *widget);
    void recursiveRemoval(QObject *object);

    Q_DECLARE_PRIVATE(QUnifiedToolbarSurface)
    QScopedPointer<QUnifiedToolbarSurfacePrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QT_MAC_USE_COCOA

#endif // QUNIFIEDTOOLBARSURFACE_MAC_P_H
