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

#ifndef QPIXMAP_H
#define QPIXMAP_H

#include <QtGui/qpaintdevice.h>
#include <QtGui/qcolor.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qstring.h> // char*->QString conversion
#include <QtCore/qsharedpointer.h>
#include <QtGui/qimage.h>
#include <QtGui/qtransform.h>

QT_BEGIN_HEADER

#if defined(Q_OS_SYMBIAN)
class CFbsBitmap;
class RSgImage;
#endif

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QImageWriter;
class QImageReader;
class QColor;
class QVariant;
class QX11Info;
class QPixmapData;

class Q_GUI_EXPORT QPixmap : public QPaintDevice
{
public:
    QPixmap();
    explicit QPixmap(QPixmapData *data);
    QPixmap(int w, int h);
    QPixmap(const QSize &);
    QPixmap(const QString& fileName, const char *format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor);
#ifndef QT_NO_IMAGEFORMAT_XPM
    QPixmap(const char * const xpm[]);
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

    bool isNull() const; // ### Qt 5: make inline
    int devType() const;

    int width() const; // ### Qt 5: make inline
    int height() const; // ### Qt 5: make inline
    QSize size() const;
    QRect rect() const;
    int depth() const;

    static int defaultDepth();

    void fill(const QColor &fillColor = Qt::white);
    void fill(const QWidget *widget, const QPoint &ofs);
    inline void fill(const QWidget *widget, int xofs, int yofs) { fill(widget, QPoint(xofs, yofs)); }

    QBitmap mask() const;
    void setMask(const QBitmap &);

#ifdef QT_DEPRECATED
    QT_DEPRECATED QPixmap alphaChannel() const;
    QT_DEPRECATED void setAlphaChannel(const QPixmap &);
#endif

    bool hasAlpha() const;
    bool hasAlphaChannel() const;

#ifndef QT_NO_IMAGE_HEURISTIC_MASK
    QBitmap createHeuristicMask(bool clipTight = true) const;
#endif
    QBitmap createMaskFromColor(const QColor &maskColor) const; // ### Qt 5: remove
    QBitmap createMaskFromColor(const QColor &maskColor, Qt::MaskMode mode) const;

    static QPixmap grabWindow(WId, int x=0, int y=0, int w=-1, int h=-1);
    static QPixmap grabWidget(QWidget *widget, const QRect &rect);
    static inline QPixmap grabWidget(QWidget *widget, int x=0, int y=0, int w=-1, int h=-1)
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

    bool load(const QString& fileName, const char *format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor);
    bool loadFromData(const uchar *buf, uint len, const char* format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor);
    inline bool loadFromData(const QByteArray &data, const char* format = 0, Qt::ImageConversionFlags flags = Qt::AutoColor);
    bool save(const QString& fileName, const char* format = 0, int quality = -1) const;
    bool save(QIODevice* device, const char* format = 0, int quality = -1) const;

    bool convertFromImage(const QImage &img, Qt::ImageConversionFlags flags = Qt::AutoColor);

#if defined(Q_WS_WIN)
    enum HBitmapFormat {
        NoAlpha,
        PremultipliedAlpha,
        Alpha
    };

    HBITMAP toWinHBITMAP(HBitmapFormat format = NoAlpha) const;
    HICON toWinHICON() const;

    static QPixmap fromWinHBITMAP(HBITMAP hbitmap, HBitmapFormat format = NoAlpha);
    static QPixmap fromWinHICON(HICON hicon);
#endif

#if defined(Q_WS_MAC)
    CGImageRef toMacCGImageRef() const;
    static QPixmap fromMacCGImageRef(CGImageRef image);
#endif

#if defined(Q_OS_SYMBIAN)
    CFbsBitmap *toSymbianCFbsBitmap() const;
    static QPixmap fromSymbianCFbsBitmap(CFbsBitmap *bitmap);
    RSgImage* toSymbianRSgImage() const;
    static QPixmap fromSymbianRSgImage(RSgImage *sgImage);
#endif

    inline QPixmap copy(int x, int y, int width, int height) const;
    QPixmap copy(const QRect &rect = QRect()) const;

    inline void scroll(int dx, int dy, int x, int y, int width, int height, QRegion *exposed = 0);
    void scroll(int dx, int dy, const QRect &rect, QRegion *exposed = 0);

#ifdef QT_DEPRECATED
    QT_DEPRECATED int serialNumber() const;
#endif
    qint64 cacheKey() const;

    bool isDetached() const;
    void detach();

    bool isQBitmap() const;

#if defined(Q_WS_QWS)
    const uchar *qwsBits() const;
    int qwsBytesPerLine() const;
    QRgb *clut() const;
#ifdef QT_DEPRECATED
    QT_DEPRECATED int numCols() const;
#endif
    int colorCount() const;
#elif defined(Q_WS_MAC)
    Qt::HANDLE macQDHandle() const;
    Qt::HANDLE macQDAlphaHandle() const;
    Qt::HANDLE macCGHandle() const;
#elif defined(Q_WS_X11)
    enum ShareMode { ImplicitlyShared, ExplicitlyShared };

    static QPixmap fromX11Pixmap(Qt::HANDLE pixmap, ShareMode mode = ImplicitlyShared);
    static int x11SetDefaultScreen(int screen);
    void x11SetScreen(int screen);
    const QX11Info &x11Info() const;
    Qt::HANDLE x11PictureHandle() const;
#endif

#if defined(Q_WS_X11) || defined(Q_WS_QWS)
    Qt::HANDLE handle() const;
#endif

    QPaintEngine *paintEngine() const;

    inline bool operator!() const { return isNull(); }

protected:
    int metric(PaintDeviceMetric) const;

#ifdef QT3_SUPPORT
public:
    enum ColorMode { Auto, Color, Mono };
    QT3_SUPPORT_CONSTRUCTOR QPixmap(const QString& fileName, const char *format, ColorMode mode);
    QT3_SUPPORT bool load(const QString& fileName, const char *format, ColorMode mode);
    QT3_SUPPORT bool loadFromData(const uchar *buf, uint len, const char* format, ColorMode mode);
    QT3_SUPPORT_CONSTRUCTOR QPixmap(const QImage& image);
    QT3_SUPPORT QPixmap &operator=(const QImage &);
    inline QT3_SUPPORT QImage convertToImage() const { return toImage(); }
    QT3_SUPPORT bool convertFromImage(const QImage &, ColorMode mode);
    inline QT3_SUPPORT operator QImage() const { return toImage(); }
    inline QT3_SUPPORT QPixmap xForm(const QMatrix &matrix) const { return transformed(QTransform(matrix)); }
    inline QT3_SUPPORT bool selfMask() const { return false; }
private:
    void resize_helper(const QSize &s);
public:
    inline QT3_SUPPORT void resize(const QSize &s) { resize_helper(s); }
    inline QT3_SUPPORT void resize(int width, int height) { resize_helper(QSize(width, height)); }
#endif

private:
    QExplicitlySharedDataPointer<QPixmapData> data;

    bool doImageIO(QImageWriter *io, int quality) const;

    // ### Qt5: remove the following three lines
    enum Type { PixmapType, BitmapType }; // must match QPixmapData::PixelType
    QPixmap(const QSize &s, Type);
    void init(int, int, Type = PixmapType);

    QPixmap(const QSize &s, int type);
    void init(int, int, int);
    void deref();
#if defined(Q_WS_WIN)
    void initAlphaPixmap(uchar *bytes, int length, struct tagBITMAPINFO *bmi);
#endif
    Q_DUMMY_COMPARISON_OPERATOR(QPixmap)
#ifdef Q_WS_MAC
    friend CGContextRef qt_mac_cg_context(const QPaintDevice*);
    friend CGImageRef qt_mac_create_imagemask(const QPixmap&, const QRectF&);
    friend IconRef qt_mac_create_iconref(const QPixmap&);
    friend quint32 *qt_mac_pixmap_get_base(const QPixmap*);
    friend int qt_mac_pixmap_get_bytes_per_line(const QPixmap*);
#endif
    friend class QPixmapData;
    friend class QX11PixmapData;
    friend class QMacPixmapData;
    friend class QSymbianRasterPixmapData;
    friend class QBitmap;
    friend class QPaintDevice;
    friend class QPainter;
    friend class QGLWidget;
    friend class QX11PaintEngine;
    friend class QCoreGraphicsPaintEngine;
    friend class QWidgetPrivate;
    friend class QRasterBuffer;
#if !defined(QT_NO_DATASTREAM)
    friend Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QPixmap &);
#endif
    friend Q_GUI_EXPORT qint64 qt_pixmap_id(const QPixmap &pixmap);

public:
    QPixmapData* pixmapData() const;

public:
    typedef QExplicitlySharedDataPointer<QPixmapData> DataPtr;
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

/*****************************************************************************
 QPixmap stream functions
*****************************************************************************/

#if !defined(QT_NO_DATASTREAM)
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QPixmap &);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QPixmap &);
#endif

/*****************************************************************************
 QPixmap (and QImage) helper functions
*****************************************************************************/
#ifdef QT3_SUPPORT
QT3_SUPPORT Q_GUI_EXPORT void copyBlt(QPixmap *dst, int dx, int dy, const QPixmap *src,
                                    int sx=0, int sy=0, int sw=-1, int sh=-1);
#endif // QT3_SUPPORT

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPIXMAP_H
