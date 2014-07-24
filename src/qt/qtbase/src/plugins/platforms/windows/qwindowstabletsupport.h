/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QWINDOWSTABLETSUPPORT_H
#define QWINDOWSTABLETSUPPORT_H

#include "qtwindowsglobal.h"

#if !defined(QT_NO_TABLETEVENT) && !defined(Q_OS_WINCE)

#include <QtCore/QVector>
#include <QtCore/QPointF>

#include <wintab.h>

QT_BEGIN_NAMESPACE

class QDebug;
class QWindow;
class QRect;

struct QWindowsWinTab32DLL
{
    QWindowsWinTab32DLL() : wTOpen(0), wTClose(0), wTInfo(0), wTEnable(0), wTOverlap(0), wTPacketsGet(0), wTGet(0),
        wTQueueSizeGet(0), wTQueueSizeSet(0) {}

    bool init();

    typedef HCTX (API *PtrWTOpen)(HWND, LPLOGCONTEXT, BOOL);
    typedef BOOL (API *PtrWTClose)(HCTX);
    typedef UINT (API *PtrWTInfo)(UINT, UINT, LPVOID);
    typedef BOOL (API *PtrWTEnable)(HCTX, BOOL);
    typedef BOOL (API *PtrWTOverlap)(HCTX, BOOL);
    typedef int  (API *PtrWTPacketsGet)(HCTX, int, LPVOID);
    typedef BOOL (API *PtrWTGet)(HCTX, LPLOGCONTEXT);
    typedef int  (API *PtrWTQueueSizeGet)(HCTX);
    typedef BOOL (API *PtrWTQueueSizeSet)(HCTX, int);

    PtrWTOpen wTOpen;
    PtrWTClose wTClose;
    PtrWTInfo wTInfo;
    PtrWTEnable wTEnable;
    PtrWTOverlap wTOverlap;
    PtrWTPacketsGet wTPacketsGet;
    PtrWTGet wTGet;
    PtrWTQueueSizeGet wTQueueSizeGet;
    PtrWTQueueSizeSet wTQueueSizeSet;
};

struct QWindowsTabletDeviceData
{
    QWindowsTabletDeviceData() : minPressure(0), maxPressure(0), minTanPressure(0),
        maxTanPressure(0), minX(0), maxX(0), minY(0), maxY(0), minZ(0), maxZ(0),
        uniqueId(0), currentDevice(0), currentPointerType(0) {}

    QPointF scaleCoordinates(int coordX, int coordY,const QRect &targetArea) const;
    qreal scalePressure(qreal p) const { return p / qreal(maxPressure - minPressure); }
    qreal scaleTangentialPressure(qreal p) const { return p / qreal(maxTanPressure - minTanPressure); }

    int minPressure;
    int maxPressure;
    int minTanPressure;
    int maxTanPressure;
    int minX, maxX, minY, maxY, minZ, maxZ;
    qint64 uniqueId;
    int currentDevice;
    int currentPointerType;
};

QDebug operator<<(QDebug d, const QWindowsTabletDeviceData &t);

class QWindowsTabletSupport
{
    Q_DISABLE_COPY(QWindowsTabletSupport)

    explicit QWindowsTabletSupport(HWND window, HCTX context);

public:
    ~QWindowsTabletSupport();

    static QWindowsTabletSupport *create();

    void notifyActivate();
    QString description() const;

    bool translateTabletProximityEvent(WPARAM wParam, LPARAM lParam);
    bool translateTabletPacketEvent();

    int absoluteRange() const { return m_absoluteRange; }
    void setAbsoluteRange(int a) { m_absoluteRange = a; }

private:
    unsigned options() const;
    QWindowsTabletDeviceData tabletInit(const quint64 uniqueId, const UINT cursorType) const;

    static QWindowsWinTab32DLL m_winTab32DLL;
    const HWND m_window;
    const HCTX m_context;
    int m_absoluteRange;
    bool m_tiltSupport;
    QVector<QWindowsTabletDeviceData> m_devices;
    int m_currentDevice;
    QPointF m_oldGlobalPosF;
};

QT_END_NAMESPACE

#endif // !QT_NO_TABLETEVENT && !Q_OS_WINCE
#endif // QWINDOWSTABLETSUPPORT_H
