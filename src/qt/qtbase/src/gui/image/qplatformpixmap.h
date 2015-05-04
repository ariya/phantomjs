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

#ifndef QPLATFORMPIXMAP_H
#define QPLATFORMPIXMAP_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qpixmap.h>
#include <QtCore/qatomic.h>

QT_BEGIN_NAMESPACE


class QImageReader;

class Q_GUI_EXPORT QPlatformPixmap
{
public:
    enum PixelType {
        // WARNING: Do not change the first two
        // Must match QPixmap::Type
        PixmapType, BitmapType
    };

    enum ClassId { RasterClass, DirectFBClass,
                   BlitterClass, Direct2DClass,
                   CustomClass = 1024 };

    QPlatformPixmap(PixelType pixelType, int classId);
    virtual ~QPlatformPixmap();

    virtual QPlatformPixmap *createCompatiblePlatformPixmap() const;

    virtual void resize(int width, int height) = 0;
    virtual void fromImage(const QImage &image,
                           Qt::ImageConversionFlags flags) = 0;
    virtual void fromImageInPlace(QImage &image,
                                  Qt::ImageConversionFlags flags)
    {
        fromImage(image, flags);
    }

    virtual void fromImageReader(QImageReader *imageReader,
                                 Qt::ImageConversionFlags flags);

    virtual bool fromFile(const QString &filename, const char *format,
                          Qt::ImageConversionFlags flags);
    virtual bool fromData(const uchar *buffer, uint len, const char *format,
                          Qt::ImageConversionFlags flags);

    virtual void copy(const QPlatformPixmap *data, const QRect &rect);
    virtual bool scroll(int dx, int dy, const QRect &rect);

    virtual int metric(QPaintDevice::PaintDeviceMetric metric) const = 0;
    virtual void fill(const QColor &color) = 0;

    virtual bool hasAlphaChannel() const = 0;
    virtual QPixmap transformed(const QTransform &matrix,
                                Qt::TransformationMode mode) const;

    virtual QImage toImage() const = 0;
    virtual QImage toImage(const QRect &rect) const;
    virtual QPaintEngine* paintEngine() const = 0;

    inline int serialNumber() const { return ser_no; }

    inline PixelType pixelType() const { return type; }
    inline ClassId classId() const { return static_cast<ClassId>(id); }

    virtual qreal devicePixelRatio() const = 0;
    virtual void setDevicePixelRatio(qreal scaleFactor) = 0;

    virtual QImage* buffer();

    inline int width() const { return w; }
    inline int height() const { return h; }
    inline int colorCount() const { return metric(QPaintDevice::PdmNumColors); }
    inline int depth() const { return d; }
    inline bool isNull() const { return is_null; }
    inline qint64 cacheKey() const {
        int classKey = id;
        if (classKey >= 1024)
            classKey = -(classKey >> 10);
        return ((((qint64) classKey) << 56)
                | (((qint64) ser_no) << 32)
                | ((qint64) detach_no));
    }

    static QPlatformPixmap *create(int w, int h, PixelType type);

protected:

    void setSerialNumber(int serNo);
    void setDetachNumber(int detNo);
    int w;
    int h;
    int d;
    bool is_null;

private:
    friend class QPixmap;
    friend class QImagePixmapCleanupHooks; // Needs to set is_cached
    friend class QOpenGLTextureCache; //Needs to check the reference count
    friend class QExplicitlySharedDataPointer<QPlatformPixmap>;

    QAtomicInt ref;
    int detach_no;

    PixelType type;
    int id;
    int ser_no;
    uint is_cached;
};

#  define QT_XFORM_TYPE_MSBFIRST 0
#  define QT_XFORM_TYPE_LSBFIRST 1
extern bool qt_xForm_helper(const QTransform&, int, int, int, uchar*, int, int, int, const uchar*, int, int, int);

QT_END_NAMESPACE

#endif // QPLATFORMPIXMAP_H
