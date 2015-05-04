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

#ifndef QWINDOWSDIRECT2DDEVICECONTEXT_H
#define QWINDOWSDIRECT2DDEVICECONTEXT_H

#include "qwindowsdirect2dhelpers.h"

#include <QtCore/QScopedPointer>

QT_BEGIN_NAMESPACE

/*
 * Convenience class for handling device contexts. We have to call BeginDraw
 * before anything can happen, and EndDraw once we're done, for every frame and
 * pretty much any kind of operation.
 *
 * Unfortunately, these calls cannot be interleaved, and there is no way to check
 * what state a device context is in.
 *
 * The end result is that the following throws an error if we don't track it:
 *      QPixmap pmap;
 *      QPainter painter(&pmap);
 *      pmap.clear();
 *
 * Here BeginDraw would first be called through the paint device, then when we clear
 * the pixmap we would have to call it again. There is no way to know what state
 * the device context is in when performing the clear, and activating the dc is an
 * error. Bummer.
 *
 * Hence we keep a reference count here and only activate/deactivate the device
 * if the refcount is zero.
 *
 * In a nutshell: Do not call BeginDraw/EndDraw yourself on the device pointer, do
 * so through the begin/end members below.
 */

class QWindowsDirect2DDeviceContextPrivate;
class QWindowsDirect2DDeviceContext
{
    Q_DECLARE_PRIVATE(QWindowsDirect2DDeviceContext)
    friend class QWindowsDirect2DDeviceContextSuspender;
public:
    QWindowsDirect2DDeviceContext(ID2D1DeviceContext *dc);
    ~QWindowsDirect2DDeviceContext();

    ID2D1DeviceContext *get() const;

    void begin();
    bool end();

private:
    void suspend();
    void resume();

    QScopedPointer<QWindowsDirect2DDeviceContextPrivate> d_ptr;
};

class QWindowsDirect2DDeviceContextSuspender {
    Q_DISABLE_COPY(QWindowsDirect2DDeviceContextSuspender)

    QWindowsDirect2DDeviceContext *m_dc;
public:
    QWindowsDirect2DDeviceContextSuspender(QWindowsDirect2DDeviceContext *dc);
    ~QWindowsDirect2DDeviceContextSuspender();

    void resume();
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DDEVICECONTEXT_H
