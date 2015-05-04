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

#ifndef QPIXMAP_H
#define QPIXMAP_H

#include <QtGui/qpaintdevice.h>
#include <QtGui/qcolor.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qstring.h> // char*->QString conversion
#include <QtCore/qsharedpointer.h>
#include <QtGui/qimage.h>
#include <QtGui/qtransform.h>

QT_BEGIN_NAMESPACE


class QImageWriter;
class QImageReader;
class QColor;
class QVariant;
class QPlatformPixmap;

class Q_GUI_EXPORT QPixmap : public QPaintDevice
{
public:
    QPixmap();
    explicit QPixmap(QPlatformPixmap *data);
    QPixmap(int w, int h);
    explicit QPixmap(const QSize &);
    QPixmap(const QString& fileName, const char *format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor);
#ifndef QT_NO_IMAGEFORMAT_XPM
    explicit QPixmap(const char * const xpm[]);
#endif
    QPixmap(const QPixmap &);
    ~QPixmap();

    QPixmap &operator=(const QPixmap &);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QPixmap &operator=(QPixmap &&other)
    { qSwap(data, other.data); return *this; }
#endif
    inline void swap(QPixmap &other) { qSwap(data, other.data); }

    operator QVariant() const;

    bool isNull() const;
    int devType() const;

    int width() const;
    int height() const;
    QSize size() const;
    QRect rect() const;
    int depth() const;

    static int defaultDepth();

    void fill(const QColor &fillColor = Qt::white);
    void fill(const QPaintDevice *device, const QPoint &ofs);
    inline void fill(const QPaintDevice *device, int xofs, int yofs) { fill(device, QPoint(xofs, yofs)); }

    QBitmap mask() const;
    void setMask(const QBitmap &);

    qreal devicePixelRatio() const;
    void setDevicePixelRatio(qreal scaleFactor);

    bool hasAlpha() const;
    bool hasAlphaChannel() const;

#ifndef QT_NO_IMAGE_HEURISTIC_MASK
    QBitmap createHeuristicMask(bool clipTight = true) const;
#endif
    QBitmap createMaskFromColor(const QColor &maskColor, Qt::MaskMode mode = Qt::MaskInColor) const;

    static QPixmap grabWindow(WId, int x=0, int y=0, int w=-1, int h=-1);
    static QPixmap grabWidget(QObject *widget, const QRect &rect);
    static inline QPixmap grabWidget(QObject *widget, int x=0, int y=0, int w=-1, int h=-1)
    { return grabWidget(widget, QRect(x, y, w, h)); }

    inline QPixmap scaled(int w, int h, Qt::AspectRatioMode aspectMode = Qt::IgnoreAspectRatio,
                          Qt::TransformationMode mode = Qt::FastTransformation) const
        { return scaled(QSize(w, h), aspectMode, mode); }
    QPixmap scaled(const QSize &s, Qt::AspectRatioMode aspectMode = Qt::IgnoreAspectRatio,
                   Qt::TransformationMode mode = Qt::FastTransformation) const;
    QPixmap scaledToWidth(int w, Qt::TransformationMode mode = Qt::FastTransformation) const;
    QPixmap scaledToHeight(int h, Qt::TransformationMode mode = Qt::FastTransformation) const;
    QPixmap transformed(const QMatrix &, Qt::TransformationMode mode = Qt::FastTransformation) const;
    static QMatrix trueMatrix(const QMatrix &m, int w, int h);
    QPixmap transformed(const QTransform &, Qt::TransformationMode mode = Qt::FastTransformation) const;
    static QTransform trueMatrix(const QTransform &m, int w, int h);

    QImage toImage() const;
    static QPixmap fromImage(const QImage &image, Qt::ImageConversionFlags flags = Qt::AutoColor);
    static QPixmap fromImageReader(QImageReader *imageReader, Qt::ImageConversionFlags flags = Qt::AutoColor);
#ifdef Q_COMPILER_RVALUE_REFS
    static QPixmap fromImage(QImage &&image, Qt::ImageConversionFlags flags = Qt::AutoColor)
    {
        return fromImageInPlace(image, flags);
    }
#endif

    bool load(const QString& fileName, const char *format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor);
    bool loadFromData(const uchar *buf, uint len, const char* format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor);
    inline bool loadFromData(const QByteArray &data, const char* format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor);
    bool save(const QString& fileName, const char* format = 0, int quality = -1) const;
    bool save(QIODevice* device, const char* format = 0, int quality = -1) const;

    bool convertFromImage(const QImage &img, Qt::ImageConversionFlags flags = Qt::AutoColor);

    inline QPixmap copy(int x, int y, int width, int height) const;
    QPixmap copy(const QRect &rect = QRect()) const;

    inline void scroll(int dx, int dy, int x, int y, int width, int height, QRegion *exposed = 0);
    void scroll(int dx, int dy, const QRect &rect, QRegion *exposed = 0);

#if QT_DEPRECATED_SINCE(5, 0)
    QT_DEPRECATED inline int serialNumber() const { return cacheKey() >> 32; }
#endif
    qint64 cacheKey() const;

    bool isDetached() const;
    void detach();

    bool isQBitmap() const;

    QPaintEngine *paintEngine() const;

    inline bool operator!() const { return isNull(); }

#if QT_DEPRECATED_SINCE(5, 0)
    QT_DEPRECATED inline QPixmap alphaChannel() const;
    QT_DEPRECATED inline void setAlphaChannel(const QPixmap &);
#endif

protected:
    int metric(PaintDeviceMetric) const;
    static QPixmap fromImageInPlace(QImage &image, Qt::ImageConversionFlags flags = Qt::AutoColor);

private:
    QExplicitlySharedDataPointer<QPlatformPixmap> data;

    bool doImageIO(QImageWriter *io, int quality) const;

    QPixmap(const QSize &s, int type);
    void doInit(int, int, int);
    Q_DUMMY_COMPARISON_OPERATOR(QPixmap)
    friend class QPlatformPixmap;
    friend class QBitmap;
    friend class QPaintDevice;
    friend class QPainter;
    friend class QOpenGLWidget;
    friend class QWidgetPrivate;
    friend class QRasterBuffer;
#if !defined(QT_NO_DATASTREAM)
    friend Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QPixmap &);
#endif

public:
    QPlatformPixmap* handle() const;

public:
    typedef QExplicitlySharedDataPointer<QPlatformPixmap> DataPtr;
    inline DataPtr &data_ptr() { return data; }
};

Q_DECLARE_SHARED(QPixmap)

inline QPixmap QPixmap::copy(int ax, int ay, int awidth, int aheight) const
{
    return copy(QRect(ax, ay, awidth, aheight));
}

inline void QPixmap::scroll(int dx, int dy, int ax, int ay, int awidth, int aheight, QRegion *exposed)
{
    scroll(dx, dy, QRect(ax, ay, awidth, aheight), exposed);
}

inline bool QPixmap::loadFromData(const QByteArray &buf, const char *format,
                                  Qt::ImageConversionFlags flags)
{
    return loadFromData(reinterpret_cast<const uchar *>(buf.constData()), buf.size(), format, flags);
}

#if QT_DEPRECATED_SINCE(5, 0)
inline QPixmap QPixmap::alphaChannel() const
{
    return QPixmap::fromImage(toImage().alphaChannel());
}

inline void QPixmap::setAlphaChannel(const QPixmap &p)
{
    QImage image = toImage();
    image.setAlphaChannel(p.toImage());
    *this = QPixmap::fromImage(image);

}
#endif

/*****************************************************************************
 QPixmap stream functions
*****************************************************************************/

#if !defined(QT_NO_DATASTREAM)
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QPixmap &);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QPixmap &);
#endif

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QPixmap &);
#endif

QT_END_NAMESPACE

#endif // QPIXMAP_H
