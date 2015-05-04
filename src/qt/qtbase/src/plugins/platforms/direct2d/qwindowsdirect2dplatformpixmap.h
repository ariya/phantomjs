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

#ifndef QWINDOWSDIRECT2DPLATFORMPIXMAP_H
#define QWINDOWSDIRECT2DPLATFORMPIXMAP_H

#include "qwindowsdirect2dpaintengine.h"
#include <QtGui/qpa/qplatformpixmap.h>
#include <QtCore/QScopedPointer>

QT_BEGIN_NAMESPACE

class QWindowsDirect2DPlatformPixmapPrivate;
class QWindowsDirect2DBitmap;

class QWindowsDirect2DPlatformPixmap : public QPlatformPixmap
{
    Q_DECLARE_PRIVATE(QWindowsDirect2DPlatformPixmap)
public:
    QWindowsDirect2DPlatformPixmap(PixelType pixelType);

    // We do NOT take ownership of the bitmap through this constructor!
    QWindowsDirect2DPlatformPixmap(PixelType pixelType, QWindowsDirect2DPaintEngine::Flags flags, QWindowsDirect2DBitmap *bitmap);
    ~QWindowsDirect2DPlatformPixmap();

    void resize(int width, int height) Q_DECL_OVERRIDE;
    virtual void fromImage(const QImage &image,
                           Qt::ImageConversionFlags flags);

    int metric(QPaintDevice::PaintDeviceMetric metric) const Q_DECL_OVERRIDE;
    void fill(const QColor &color) Q_DECL_OVERRIDE;

    bool hasAlphaChannel() const Q_DECL_OVERRIDE;

    QImage toImage() const Q_DECL_OVERRIDE;
    QImage toImage(const QRect &rect) const Q_DECL_OVERRIDE;

    QPaintEngine* paintEngine() const Q_DECL_OVERRIDE;

    qreal devicePixelRatio() const Q_DECL_OVERRIDE;
    void setDevicePixelRatio(qreal scaleFactor) Q_DECL_OVERRIDE;

    QWindowsDirect2DBitmap *bitmap() const;

private:
    QScopedPointer<QWindowsDirect2DPlatformPixmapPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DPLATFORMPIXMAP_H
