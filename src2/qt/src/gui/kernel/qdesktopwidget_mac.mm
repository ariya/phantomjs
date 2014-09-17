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

/****************************************************************************
**
** Copyright (c) 2007-2008, Apple, Inc.
**
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
**   * Redistributions of source code must retain the above copyright notice,
**     this list of conditions and the following disclaimer.
**
**   * Redistributions in binary form must reproduce the above copyright notice,
**     this list of conditions and the following disclaimer in the documentation
**     and/or other materials provided with the distribution.
**
**   * Neither the name of Apple, Inc. nor the names of its contributors
**     may be used to endorse or promote products derived from this software
**     without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************/

#import <Cocoa/Cocoa.h>

#include "qapplication.h"
#include "qdesktopwidget.h"
#include <private/qt_mac_p.h>
#include "qwidget_p.h"
#include <private/qt_cocoa_helpers_mac_p.h>
#include <private/qdesktopwidget_mac_p.h>

QT_BEGIN_NAMESPACE

QT_USE_NAMESPACE

/*****************************************************************************
  Externals
 *****************************************************************************/

/*****************************************************************************
  QDesktopWidget member functions
 *****************************************************************************/

Q_GLOBAL_STATIC(QDesktopWidgetImplementation, qdesktopWidgetImplementation)

QDesktopWidgetImplementation::QDesktopWidgetImplementation()
    : appScreen(0)
{
    onResize();
}

QDesktopWidgetImplementation::~QDesktopWidgetImplementation()
{
}

QDesktopWidgetImplementation *QDesktopWidgetImplementation::instance()
{
    return qdesktopWidgetImplementation();
}

QRect QDesktopWidgetImplementation::availableRect(int screenIndex) const
{
    if (screenIndex < 0 || screenIndex >= screenCount)
        screenIndex = appScreen;

	return availableRects[screenIndex].toRect(); 
}

QRect QDesktopWidgetImplementation::screenRect(int screenIndex) const
{
    if (screenIndex < 0 || screenIndex >= screenCount)
        screenIndex = appScreen;

    return screenRects[screenIndex].toRect();
}

void QDesktopWidgetImplementation::onResize()
{
    QMacCocoaAutoReleasePool pool; 
    NSArray *displays = [NSScreen screens]; 
    screenCount = [displays count]; 
 
    screenRects.clear(); 
    availableRects.clear(); 
    NSRect primaryRect = [[displays objectAtIndex:0] frame]; 
    for (int i = 0; i<screenCount; i++) {
        NSRect r = [[displays objectAtIndex:i] frame];
        int flippedY = - r.origin.y +                  // account for position offset and
              primaryRect.size.height - r.size.height; // height difference. 
        screenRects.append(QRectF(r.origin.x, flippedY, 
            r.size.width, r.size.height)); 

        r = [[displays objectAtIndex:i] visibleFrame];
        flippedY = - r.origin.y +                      // account for position offset and
              primaryRect.size.height - r.size.height; // height difference. 
        availableRects.append(QRectF(r.origin.x, flippedY, 
                r.size.width, r.size.height)); 
    }
}



QDesktopWidget::QDesktopWidget()
    : QWidget(0, Qt::Desktop)
{
    setObjectName(QLatin1String("desktop"));
    setAttribute(Qt::WA_WState_Visible);
}

QDesktopWidget::~QDesktopWidget()
{
}

bool QDesktopWidget::isVirtualDesktop() const
{
    return true;
}

int QDesktopWidget::primaryScreen() const
{
    return qdesktopWidgetImplementation()->appScreen;
}

int QDesktopWidget::numScreens() const
{
    return qdesktopWidgetImplementation()->screenCount;
}

QWidget *QDesktopWidget::screen(int)
{
    return this;
}

const QRect QDesktopWidget::availableGeometry(int screen) const
{
    return qdesktopWidgetImplementation()->availableRect(screen);
}

const QRect QDesktopWidget::screenGeometry(int screen) const
{
    return qdesktopWidgetImplementation()->screenRect(screen);
}

int QDesktopWidget::screenNumber(const QWidget *widget) const
{
    QDesktopWidgetImplementation *d = qdesktopWidgetImplementation();
    if (!widget)
        return d->appScreen;
    QRect frame = widget->frameGeometry();
    if (!widget->isWindow())
        frame.moveTopLeft(widget->mapToGlobal(QPoint(0,0)));
    int maxSize = -1, maxScreen = -1;
    for (int i = 0; i < d->screenCount; ++i) {
        QRect rr = d->screenRect(i);
        QRect sect = rr.intersected(frame);
        int size = sect.width() * sect.height();
        if (size > maxSize && sect.width() > 0 && sect.height() > 0) {
            maxSize = size;
            maxScreen = i;
        }
    }
    return maxScreen;
}

int QDesktopWidget::screenNumber(const QPoint &point) const
{
    QDesktopWidgetImplementation *d = qdesktopWidgetImplementation();
    int closestScreen = -1;
    int shortestDistance = INT_MAX;
    for (int i = 0; i < d->screenCount; ++i) {
        QRect rr = d->screenRect(i);
        int thisDistance = QWidgetPrivate::pointToRect(point, rr);
        if (thisDistance < shortestDistance) {
            shortestDistance = thisDistance;
            closestScreen = i;
        }
    }
    return closestScreen;
}

void QDesktopWidget::resizeEvent(QResizeEvent *)
{
    QDesktopWidgetImplementation *d = qdesktopWidgetImplementation();

    const int oldScreenCount = d->screenCount;
    const QVector<QRectF> oldRects(d->screenRects);
    const QVector<QRectF> oldWorks(d->availableRects);

    d->onResize();

    for (int i = 0; i < qMin(oldScreenCount, d->screenCount); ++i) {
        if (oldRects.at(i) != d->screenRects.at(i))
            emit resized(i);
    }
    for (int i = 0; i < qMin(oldScreenCount, d->screenCount); ++i) {
        if (oldWorks.at(i) != d->availableRects.at(i))
            emit workAreaResized(i);
    }

    if (oldScreenCount != d->screenCount)
        emit screenCountChanged(d->screenCount);
}

QT_END_NAMESPACE
