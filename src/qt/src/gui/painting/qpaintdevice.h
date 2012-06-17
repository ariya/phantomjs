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

#ifndef QPAINTDEVICE_H
#define QPAINTDEVICE_H

#include <QtGui/qwindowdefs.h>
#include <QtCore/qrect.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#if defined(Q_WS_QWS)
class QWSDisplay;
#endif

class QPaintEngine;

class Q_GUI_EXPORT QPaintDevice                                // device for QPainter
{
public:
    enum PaintDeviceMetric {
        PdmWidth = 1,
        PdmHeight,
        PdmWidthMM,
        PdmHeightMM,
        PdmNumColors,
        PdmDepth,
        PdmDpiX,
        PdmDpiY,
        PdmPhysicalDpiX,
        PdmPhysicalDpiY
    };

    virtual ~QPaintDevice();

    virtual int devType() const;
    bool paintingActive() const;
    virtual QPaintEngine *paintEngine() const = 0;

#if defined(Q_WS_QWS)
    static QWSDisplay *qwsDisplay();
#endif

#ifdef Q_WS_WIN
    virtual HDC getDC() const;
    virtual void releaseDC(HDC hdc) const;
#endif

    int width() const { return metric(PdmWidth); }
    int height() const { return metric(PdmHeight); }
    int widthMM() const { return metric(PdmWidthMM); }
    int heightMM() const { return metric(PdmHeightMM); }
    int logicalDpiX() const { return metric(PdmDpiX); }
    int logicalDpiY() const { return metric(PdmDpiY); }
    int physicalDpiX() const { return metric(PdmPhysicalDpiX); }
    int physicalDpiY() const { return metric(PdmPhysicalDpiY); }
#ifdef QT_DEPRECATED
    QT_DEPRECATED int numColors() const { return metric(PdmNumColors); }
#endif
    int colorCount() const { return metric(PdmNumColors); }
    int depth() const { return metric(PdmDepth); }

protected:
    QPaintDevice();
    virtual int metric(PaintDeviceMetric metric) const;

    ushort        painters;                        // refcount

private:
    Q_DISABLE_COPY(QPaintDevice)

#if defined(Q_WS_X11) && defined(QT3_SUPPORT)
public:
    QT3_SUPPORT Display *x11Display() const;
    QT3_SUPPORT int x11Screen() const;
    QT3_SUPPORT int x11Depth() const;
    QT3_SUPPORT int x11Cells() const;
    QT3_SUPPORT Qt::HANDLE x11Colormap() const;
    QT3_SUPPORT bool x11DefaultColormap() const;
    QT3_SUPPORT void *x11Visual() const;
    QT3_SUPPORT bool x11DefaultVisual() const;

    static QT3_SUPPORT Display *x11AppDisplay();
    static QT3_SUPPORT int x11AppScreen();
    static QT3_SUPPORT int x11AppDepth(int screen = -1);
    static QT3_SUPPORT int x11AppCells(int screen = -1);
    static QT3_SUPPORT Qt::HANDLE x11AppRootWindow(int screen = -1);
    static QT3_SUPPORT Qt::HANDLE x11AppColormap(int screen = -1);
    static QT3_SUPPORT void *x11AppVisual(int screen = -1);
    static QT3_SUPPORT bool x11AppDefaultColormap(int screen =-1);
    static QT3_SUPPORT bool x11AppDefaultVisual(int screen =-1);
    static QT3_SUPPORT int x11AppDpiX(int screen = -1);
    static QT3_SUPPORT int x11AppDpiY(int screen = -1);
    static QT3_SUPPORT void x11SetAppDpiX(int, int);
    static QT3_SUPPORT void x11SetAppDpiY(int, int);
#endif

    friend class QPainter;
    friend class QFontEngineMac;
    friend class QX11PaintEngine;
    friend Q_GUI_EXPORT int qt_paint_device_metric(const QPaintDevice *device, PaintDeviceMetric metric);
};

#ifdef QT3_SUPPORT
QT3_SUPPORT Q_GUI_EXPORT
void bitBlt(QPaintDevice *dst, int dx, int dy,
             const QPaintDevice *src, int sx=0, int sy=0, int sw=-1, int sh=-1,
             bool ignoreMask=false);

QT3_SUPPORT Q_GUI_EXPORT
void bitBlt(QPaintDevice *dst, int dx, int dy,
             const QImage *src, int sx=0, int sy=0, int sw=-1, int sh=-1,
             int conversion_flags=0);

QT3_SUPPORT Q_GUI_EXPORT
void bitBlt(QPaintDevice *dst, const QPoint &dp,
            const QPaintDevice *src, const QRect &sr=QRect(0,0,-1,-1),
            bool ignoreMask=false);
#endif

/*****************************************************************************
  Inline functions
 *****************************************************************************/

inline int QPaintDevice::devType() const
{ return QInternal::UnknownDevice; }

inline bool QPaintDevice::paintingActive() const
{ return painters != 0; }

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPAINTDEVICE_H
