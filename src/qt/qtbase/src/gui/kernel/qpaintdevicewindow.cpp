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

#include "qpaintdevicewindow_p.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

QT_BEGIN_NAMESPACE

/*!
    \class QPaintDeviceWindow
    \inmodule QtGui
    \since 5.4
    \brief Convenience subclass of QWindow that is also a QPaintDevice.

    QPaintDeviceWindow is like a regular QWindow, with the added functionality
    of being a paint device too. Whenever the content needs to be updated,
    the virtual paintEvent() function is called. Subclasses, that reimplement
    this function, can then simply open a QPainter on the window.

    \note This class cannot directly be used in applications. It rather serves
    as a base for subclasses like QOpenGLWindow.

    \sa QOpenGLWindow
*/

/*!
    Marks the entire window as dirty and schedules a repaint.

    \note Subsequent calls to this function before the next paint
    event will get ignored.
*/
void QPaintDeviceWindow::update()
{
    update(QRect(QPoint(0,0), size()));
}

/*!
    Marks the \a rect of the window as dirty and schedules a repaint.

    \note Subsequent calls to this function before the next paint
    event will get ignored.
*/
void QPaintDeviceWindow::update(const QRect &rect)
{
    Q_D(QPaintDeviceWindow);
    d->dirtyRegion += rect;
    d->triggerUpdate();
}

/*!
    Marks the \a region of the window as dirty and schedules a repaint.

    \note Subsequent calls to this function before the next paint
    event will get ignored.
*/
void QPaintDeviceWindow::update(const QRegion &region)
{
    Q_D(QPaintDeviceWindow);
    d->dirtyRegion += region;
    d->triggerUpdate();
}

/*!
    Handles paint events passed in the \a event parameter.

    The default implementation does nothing. Reimplement this function to
    perform painting. If necessary, the dirty area is retrievable from
    the \a event.
*/
void QPaintDeviceWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    // Do nothing
}

/*!
  \internal
 */
int QPaintDeviceWindow::metric(PaintDeviceMetric metric) const
{
    QScreen *screen = this->screen();
    if (!screen && QGuiApplication::primaryScreen())
        screen = QGuiApplication::primaryScreen();

    switch (metric) {
    case PdmWidth:
        return width();
    case PdmWidthMM:
        if (screen)
            return width() * screen->physicalSize().width() / screen->geometry().width();
        break;
    case PdmHeight:
        return height();
    case PdmHeightMM:
        if (screen)
            return height() * screen->physicalSize().height() / screen->geometry().height();
        break;
    case PdmDpiX:
        if (screen)
            return qRound(screen->logicalDotsPerInchX());
        break;
    case PdmDpiY:
        if (screen)
            return qRound(screen->logicalDotsPerInchY());
        break;
    case PdmPhysicalDpiX:
        if (screen)
            return qRound(screen->physicalDotsPerInchX());
        break;
    case PdmPhysicalDpiY:
        if (screen)
            return qRound(screen->physicalDotsPerInchY());
        break;
    case PdmDevicePixelRatio:
        if (screen)
            return screen->devicePixelRatio();
        break;
    default:
        break;
    }

    return QPaintDevice::metric(metric);
}

/*!
  \internal
 */
void QPaintDeviceWindow::exposeEvent(QExposeEvent *exposeEvent)
{
    Q_UNUSED(exposeEvent);
    Q_D(QPaintDeviceWindow);
    if (isExposed()) {
        d->markWindowAsDirty();
        // Do not rely on exposeEvent->region() as it has some issues for the
        // time being, namely that it is sometimes in local coordinates,
        // sometimes relative to the parent, depending on the platform plugin.
        // We require local coords here.
        d->doFlush(QRect(QPoint(0, 0), size()));
    }
}

/*!
  \internal
 */
bool QPaintDeviceWindow::event(QEvent *event)
{
    Q_D(QPaintDeviceWindow);

    if (event->type() == QEvent::UpdateRequest) {
        d->paintEventSent = false;
        if (handle()) // platform window may be gone when the window is closed during app exit
            d->handleUpdateEvent();
        return true;
    }

    return QWindow::event(event);
}

/*!
  \internal
 */
QPaintDeviceWindow::QPaintDeviceWindow(QPaintDeviceWindowPrivate &dd, QWindow *parent)
    : QWindow(dd, parent)
{
}

/*!
  \internal
 */
QPaintEngine *QPaintDeviceWindow::paintEngine() const
{
    return 0;
}

QT_END_NAMESPACE
