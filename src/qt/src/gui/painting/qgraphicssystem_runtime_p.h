/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QGRAPHICSSYSTEM_RUNTIME_P_H
#define QGRAPHICSSYSTEM_RUNTIME_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qgraphicssystem_p.h"

#include <private/qpixmapdata_p.h>

QT_BEGIN_NAMESPACE

class QRuntimeGraphicsSystem;

class Q_GUI_EXPORT QRuntimePixmapData : public QPixmapData {
public:
    QRuntimePixmapData(const QRuntimeGraphicsSystem *gs, PixelType type);
    ~QRuntimePixmapData();

    virtual QPixmapData *createCompatiblePixmapData() const;
    virtual void resize(int width, int height);
    virtual void fromImage(const QImage &image,
                           Qt::ImageConversionFlags flags);

    virtual bool fromFile(const QString &filename, const char *format,
                          Qt::ImageConversionFlags flags);
    virtual bool fromData(const uchar *buffer, uint len, const char *format,
                          Qt::ImageConversionFlags flags);

    virtual void copy(const QPixmapData *data, const QRect &rect);
    virtual bool scroll(int dx, int dy, const QRect &rect);

    virtual int metric(QPaintDevice::PaintDeviceMetric metric) const;
    virtual void fill(const QColor &color);
    virtual QBitmap mask() const;
    virtual void setMask(const QBitmap &mask);
    virtual bool hasAlphaChannel() const;
    virtual QPixmap transformed(const QTransform &matrix,
                                Qt::TransformationMode mode) const;
    virtual void setAlphaChannel(const QPixmap &alphaChannel);
    virtual QPixmap alphaChannel() const;
    virtual QImage toImage() const;
    virtual QPaintEngine *paintEngine() const;

    virtual QImage *buffer();

    void readBackInfo();

    QPixmapData *m_data;

#if defined(Q_OS_SYMBIAN)
    void* toNativeType(NativeType type);
    void fromNativeType(void* pixmap, NativeType type);
#endif

    virtual QPixmapData *runtimeData() const;

private:
    const QRuntimeGraphicsSystem *m_graphicsSystem;

};

class QRuntimeWindowSurface : public QWindowSurface {
public:
    QRuntimeWindowSurface(const QRuntimeGraphicsSystem *gs, QWidget *window);
    ~QRuntimeWindowSurface();

    virtual QPaintDevice *paintDevice();
    virtual void flush(QWidget *widget, const QRegion &region,
                       const QPoint &offset);
    virtual void setGeometry(const QRect &rect);

    virtual bool scroll(const QRegion &area, int dx, int dy);

    virtual void beginPaint(const QRegion &);
    virtual void endPaint(const QRegion &);

    virtual QImage* buffer(const QWidget *widget);
    virtual QPixmap grabWidget(const QWidget *widget, const QRect& rectangle = QRect()) const;

    virtual QPoint offset(const QWidget *widget) const;

    virtual WindowSurfaceFeatures features() const;

    QScopedPointer<QWindowSurface> m_windowSurface;
    QScopedPointer<QWindowSurface> m_pendingWindowSurface;

private:
    const QRuntimeGraphicsSystem *m_graphicsSystem;
};

class QRuntimeGraphicsSystem : public QGraphicsSystem
{
public:

    enum WindowSurfaceDestroyPolicy
    {
        DestroyImmediately,
        DestroyAfterFirstFlush
    };

public:
    QRuntimeGraphicsSystem();

    QPixmapData *createPixmapData(QPixmapData::PixelType type) const;
    QWindowSurface *createWindowSurface(QWidget *widget) const;

    void removePixmapData(QRuntimePixmapData *pixmapData) const;
    void removeWindowSurface(QRuntimeWindowSurface *windowSurface) const;

    void setGraphicsSystem(const QString &name);
    QString graphicsSystemName() const { return m_graphicsSystemName; }

    void setWindowSurfaceDestroyPolicy(WindowSurfaceDestroyPolicy policy)
    {
        m_windowSurfaceDestroyPolicy = policy;
    }

    int windowSurfaceDestroyPolicy() const { return m_windowSurfaceDestroyPolicy; }


private:
    int m_windowSurfaceDestroyPolicy;
    QGraphicsSystem *m_graphicsSystem;
    mutable QList<QRuntimePixmapData *> m_pixmapDatas;
    mutable QList<QRuntimeWindowSurface *> m_windowSurfaces;
    QString m_graphicsSystemName;

    QString m_pendingGraphicsSystemName;

    friend class QRuntimePixmapData;
    friend class QRuntimeWindowSurface;
    friend class QMeeGoGraphicsSystem;
};

QT_END_NAMESPACE

#endif
