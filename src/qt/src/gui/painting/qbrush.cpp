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

#include "qbrush.h"
#include "qpixmap.h"
#include "qbitmap.h"
#include "qpixmapcache.h"
#include "qdatastream.h"
#include "qvariant.h"
#include "qline.h"
#include "qdebug.h"
#include <QtCore/qcoreapplication.h>
#include "private/qstylehelper_p.h"
#include <QtCore/qnumeric.h>

QT_BEGIN_NAMESPACE

const uchar *qt_patternForBrush(int brushStyle, bool invert)
{
    Q_ASSERT(brushStyle > Qt::SolidPattern && brushStyle < Qt::LinearGradientPattern);
    if(invert) {
        static const uchar dense1_pat[] = { 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff };
        static const uchar dense2_pat[] = { 0x77, 0xff, 0xdd, 0xff, 0x77, 0xff, 0xdd, 0xff };
        static const uchar dense3_pat[] = { 0x55, 0xbb, 0x55, 0xee, 0x55, 0xbb, 0x55, 0xee };
        static const uchar dense4_pat[] = { 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55 };
        static const uchar dense5_pat[] = { 0xaa, 0x44, 0xaa, 0x11, 0xaa, 0x44, 0xaa, 0x11 };
        static const uchar dense6_pat[] = { 0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00 };
        static const uchar dense7_pat[] = { 0x00, 0x44, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00 };
        static const uchar hor_pat[]    = { 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00 };
        static const uchar ver_pat[]    = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 };
        static const uchar cross_pat[]  = { 0x10, 0x10, 0x10, 0xff, 0x10, 0x10, 0x10, 0x10 };
        static const uchar bdiag_pat[]  = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
        static const uchar fdiag_pat[]  = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
        static const uchar dcross_pat[] = { 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81 };
        static const uchar *const pat_tbl[] = {
            dense1_pat, dense2_pat, dense3_pat, dense4_pat, dense5_pat,
            dense6_pat, dense7_pat,
            hor_pat, ver_pat, cross_pat, bdiag_pat, fdiag_pat, dcross_pat };
        return pat_tbl[brushStyle - Qt::Dense1Pattern];
    }
    static const uchar dense1_pat[] = { 0x00, 0x44, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00 };
    static const uchar dense2_pat[] = { 0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00 };
    static const uchar dense3_pat[] = { 0xaa, 0x44, 0xaa, 0x11, 0xaa, 0x44, 0xaa, 0x11 };
    static const uchar dense4_pat[] = { 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa };
    static const uchar dense5_pat[] = { 0x55, 0xbb, 0x55, 0xee, 0x55, 0xbb, 0x55, 0xee };
    static const uchar dense6_pat[] = { 0x77, 0xff, 0xdd, 0xff, 0x77, 0xff, 0xdd, 0xff };
    static const uchar dense7_pat[] = { 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff };
    static const uchar hor_pat[]    = { 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff };
    static const uchar ver_pat[]    = { 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef };
    static const uchar cross_pat[]  = { 0xef, 0xef, 0xef, 0x00, 0xef, 0xef, 0xef, 0xef };
    static const uchar bdiag_pat[]  = { 0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe };
    static const uchar fdiag_pat[]  = { 0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f };
    static const uchar dcross_pat[] = { 0x7e, 0xbd, 0xdb, 0xe7, 0xe7, 0xdb, 0xbd, 0x7e };
    static const uchar *const pat_tbl[] = {
        dense1_pat, dense2_pat, dense3_pat, dense4_pat, dense5_pat,
        dense6_pat, dense7_pat,
        hor_pat, ver_pat, cross_pat, bdiag_pat, fdiag_pat, dcross_pat };
    return pat_tbl[brushStyle - Qt::Dense1Pattern];
}

QPixmap qt_pixmapForBrush(int brushStyle, bool invert)
{

    QPixmap pm;
    QString key = QLatin1Literal("$qt-brush$")
                  % HexString<uint>(brushStyle)
                  % QLatin1Char(invert ? '1' : '0');
    if (!QPixmapCache::find(key, pm)) {
        pm = QBitmap::fromData(QSize(8, 8), qt_patternForBrush(brushStyle, invert),
                               QImage::Format_MonoLSB);
        QPixmapCache::insert(key, pm);
    }

    return pm;
}

class QBrushPatternImageCache
{
public:
    QBrushPatternImageCache()
        : m_initialized(false)
    {
        init();
    }

    void init()
    {
        for (int style = Qt::Dense1Pattern; style <= Qt::DiagCrossPattern; ++style) {
            int i = style - Qt::Dense1Pattern;
            m_images[i][0] = QImage(qt_patternForBrush(style, 0), 8, 8, 1, QImage::Format_MonoLSB);
            m_images[i][1] = QImage(qt_patternForBrush(style, 1), 8, 8, 1, QImage::Format_MonoLSB);
        }
        m_initialized = true;
    }

    QImage getImage(int brushStyle, bool invert) const
    {
        Q_ASSERT(brushStyle >= Qt::Dense1Pattern && brushStyle <= Qt::DiagCrossPattern);
        if (!m_initialized)
            const_cast<QBrushPatternImageCache*>(this)->init();
        return m_images[brushStyle - Qt::Dense1Pattern][invert];
    }

    void cleanup() {
        for (int style = Qt::Dense1Pattern; style <= Qt::DiagCrossPattern; ++style) {
            int i = style - Qt::Dense1Pattern;
            m_images[i][0] = QImage();
            m_images[i][1] = QImage();
        }
        m_initialized = false;
    }

private:
    QImage m_images[Qt::DiagCrossPattern - Qt::Dense1Pattern + 1][2];
    bool m_initialized;
};

static void qt_cleanup_brush_pattern_image_cache();
Q_GLOBAL_STATIC_WITH_INITIALIZER(QBrushPatternImageCache, qt_brushPatternImageCache,
                                 {
                                     qAddPostRoutine(qt_cleanup_brush_pattern_image_cache);
                                 })

static void qt_cleanup_brush_pattern_image_cache()
{
    qt_brushPatternImageCache()->cleanup();
}

Q_GUI_EXPORT QImage qt_imageForBrush(int brushStyle, bool invert)
{
    return qt_brushPatternImageCache()->getImage(brushStyle, invert);
}

struct QTexturedBrushData : public QBrushData
{
    QTexturedBrushData() {
        m_has_pixmap_texture = false;
        m_pixmap = 0;
    }
    ~QTexturedBrushData() {
        delete m_pixmap;
    }

    void setPixmap(const QPixmap &pm) {
        delete m_pixmap;

        if (pm.isNull()) {
            m_pixmap = 0;
            m_has_pixmap_texture = false;
        } else {
            m_pixmap = new QPixmap(pm);
            m_has_pixmap_texture = true;
        }

        m_image = QImage();
    }

    void setImage(const QImage &image) {
        m_image = image;
        delete m_pixmap;
        m_pixmap = 0;
        m_has_pixmap_texture = false;
    }

    QPixmap &pixmap() {
        if (!m_pixmap) {
            m_pixmap = new QPixmap(QPixmap::fromImage(m_image));
        }
        return *m_pixmap;
    }

    QImage &image() {
        if (m_image.isNull() && m_pixmap)
            m_image = m_pixmap->toImage();
        return m_image;
    }

    QPixmap *m_pixmap;
    QImage m_image;
    bool m_has_pixmap_texture;
};

// returns true if the brush has a pixmap (or bitmap) set as the
// brush texture, false otherwise
bool Q_GUI_EXPORT qHasPixmapTexture(const QBrush& brush)
{
    if (brush.style() != Qt::TexturePattern)
        return false;
    QTexturedBrushData *tx_data = static_cast<QTexturedBrushData *>(brush.d.data());
    return tx_data->m_has_pixmap_texture;
}

struct QGradientBrushData : public QBrushData
{
    QGradient gradient;
};

struct QBrushDataPointerDeleter
{
    static inline void deleteData(QBrushData *d)
    {
        switch (d->style) {
        case Qt::TexturePattern:
            delete static_cast<QTexturedBrushData*>(d);
            break;
        case Qt::LinearGradientPattern:
        case Qt::RadialGradientPattern:
        case Qt::ConicalGradientPattern:
            delete static_cast<QGradientBrushData*>(d);
            break;
        default:
            delete d;
        }
    }

    static inline void cleanup(QBrushData *d)
    {
        if (d && !d->ref.deref()) {
            deleteData(d);
        }
    }
};

/*!
    \class QBrush
    \ingroup painting
    \ingroup shared

    \brief The QBrush class defines the fill pattern of shapes drawn
    by QPainter.

    A brush has a style, a color, a gradient and a texture.

    The brush style() defines the fill pattern using the
    Qt::BrushStyle enum. The default brush style is Qt::NoBrush
    (depending on how you construct a brush). This style tells the
    painter to not fill shapes. The standard style for filling is
    Qt::SolidPattern. The style can be set when the brush is created
    using the appropriate constructor, and in addition the setStyle()
    function provides means for altering the style once the brush is
    constructed.

    \image brush-styles.png Brush Styles

    The brush color() defines the color of the fill pattern. The color
    can either be one of Qt's predefined colors, Qt::GlobalColor, or
    any other custom QColor. The currently set color can be retrieved
    and altered using the color() and setColor() functions,
    respectively.

    The gradient() defines the gradient fill used when the current
    style is either Qt::LinearGradientPattern,
    Qt::RadialGradientPattern or Qt::ConicalGradientPattern. Gradient
    brushes are created by giving a QGradient as a constructor
    argument when creating the QBrush. Qt provides three different
    gradients: QLinearGradient, QConicalGradient, and QRadialGradient
    - all of which inherit QGradient.

    \snippet doc/src/snippets/brush/gradientcreationsnippet.cpp 0

    The texture() defines the pixmap used when the current style is
    Qt::TexturePattern.  You can create a brush with a texture by
    providing the pixmap when the brush is created or by using
    setTexture().

    Note that applying setTexture() makes style() ==
    Qt::TexturePattern, regardless of previous style
    settings. Also, calling setColor() will not make a difference if
    the style is a gradient. The same is the case if the style is
    Qt::TexturePattern style unless the current texture is a QBitmap.

    The isOpaque() function returns true if the brush is fully opaque
    otherwise false. A brush is considered opaque if:

    \list
    \o The alpha component of the color() is 255.
    \o Its texture() does not have an alpha channel and is not a QBitmap.
    \o The colors in the gradient() all have an alpha component that is 255.
    \endlist

    \table 100%
    \row
    \o \inlineimage brush-outline.png Outlines
    \o

    To specify the style and color of lines and outlines, use the
    QPainter's \l {QPen}{pen} combined with Qt::PenStyle and
    Qt::GlobalColor:

    \snippet doc/src/snippets/code/src_gui_painting_qbrush.cpp 0

    Note that, by default, QPainter renders the outline (using the
    currently set pen) when drawing shapes. Use \l {Qt::NoPen}{\c
    painter.setPen(Qt::NoPen)} to disable this behavior.

    \endtable

    For more information about painting in general, see the \l{Paint
    System}.

    \sa Qt::BrushStyle, QPainter, QColor
*/

#ifndef QT_NO_THREAD
// Special deleter that only deletes if the ref-count goes to zero
template <>
class QGlobalStaticDeleter<QBrushData>
{
public:
    QGlobalStatic<QBrushData> &globalStatic;
    QGlobalStaticDeleter(QGlobalStatic<QBrushData> &_globalStatic)
        : globalStatic(_globalStatic)
    { }

    inline ~QGlobalStaticDeleter()
    {
        if (!globalStatic.pointer->ref.deref())
            delete globalStatic.pointer;
        globalStatic.pointer = 0;
        globalStatic.destroyed = true;
    }
};
#endif

Q_GLOBAL_STATIC_WITH_INITIALIZER(QBrushData, nullBrushInstance,
                                 {
                                     x->ref = 1;
                                     x->style = Qt::BrushStyle(0);
                                     x->color = Qt::black;
                                 })

static bool qbrush_check_type(Qt::BrushStyle style) {
    switch (style) {
    case Qt::TexturePattern:
         qWarning("QBrush: Incorrect use of TexturePattern");
         break;
    case Qt::LinearGradientPattern:
    case Qt::RadialGradientPattern:
    case Qt::ConicalGradientPattern:
        qWarning("QBrush: Wrong use of a gradient pattern");
        break;
    default:
        return true;
    }
    return false;
}

/*!
  \internal
  Initializes the brush.
*/

void QBrush::init(const QColor &color, Qt::BrushStyle style)
{
    switch(style) {
    case Qt::NoBrush:
        d.reset(nullBrushInstance());
        d->ref.ref();
        if (d->color != color) setColor(color);
        return;
    case Qt::TexturePattern:
        d.reset(new QTexturedBrushData);
        break;
    case Qt::LinearGradientPattern:
    case Qt::RadialGradientPattern:
    case Qt::ConicalGradientPattern:
        d.reset(new QGradientBrushData);
        break;
    default:
        d.reset(new QBrushData);
        break;
    }
    d->ref = 1;
    d->style = style;
    d->color = color;
}

/*!
    Constructs a default black brush with the style Qt::NoBrush
    (i.e. this brush will not fill shapes).
*/

QBrush::QBrush()
    : d(nullBrushInstance())
{
    Q_ASSERT(d);
    d->ref.ref();
}

/*!
    Constructs a brush with a black color and a texture set to the
    given \a pixmap. The style is set to Qt::TexturePattern.

    \sa setTexture()
*/

QBrush::QBrush(const QPixmap &pixmap)
{
    init(Qt::black, Qt::TexturePattern);
    setTexture(pixmap);
}


/*!
    Constructs a brush with a black color and a texture set to the
    given \a image. The style is set to Qt::TexturePattern.

    \sa setTextureImage()
*/

QBrush::QBrush(const QImage &image)
{
    init(Qt::black, Qt::TexturePattern);
    setTextureImage(image);
}

/*!
    Constructs a black brush with the given \a style.

    \sa setStyle()
*/

QBrush::QBrush(Qt::BrushStyle style)
{
    if (qbrush_check_type(style))
        init(Qt::black, style);
    else {
        d.reset(nullBrushInstance());
        d->ref.ref();
    }
}

/*!
    Constructs a brush with the given \a color and \a style.

    \sa setColor(), setStyle()
*/

QBrush::QBrush(const QColor &color, Qt::BrushStyle style)
{
    if (qbrush_check_type(style))
        init(color, style);
    else {
        d.reset(nullBrushInstance());
        d->ref.ref();
    }
}

/*!
    \fn QBrush::QBrush(Qt::GlobalColor color, Qt::BrushStyle style)

    Constructs a brush with the given \a color and \a style.

    \sa setColor(), setStyle()
*/
QBrush::QBrush(Qt::GlobalColor color, Qt::BrushStyle style)
{
    if (qbrush_check_type(style))
        init(color, style);
    else {
        d.reset(nullBrushInstance());
        d->ref.ref();
    }
}

/*!
    Constructs a brush with the given \a color and the custom pattern
    stored in \a pixmap.

    The style is set to Qt::TexturePattern. The color will only have
    an effect for QBitmaps.

    \sa setColor(), setPixmap()
*/

QBrush::QBrush(const QColor &color, const QPixmap &pixmap)
{
    init(color, Qt::TexturePattern);
    setTexture(pixmap);
}

/*!

    Constructs a brush with the given \a color and the custom pattern
    stored in \a pixmap.

    The style is set to Qt::TexturePattern. The color will only have
    an effect for QBitmaps.

    \sa setColor(), setPixmap()
*/
QBrush::QBrush(Qt::GlobalColor color, const QPixmap &pixmap)
{
    init(color, Qt::TexturePattern);
    setTexture(pixmap);
}

/*!
    Constructs a copy of \a other.
*/

QBrush::QBrush(const QBrush &other)
    : d(other.d.data())
{
    d->ref.ref();
}

/*!
    Constructs a brush based on the given \a gradient.

    The brush style is set to the corresponding gradient style (either
    Qt::LinearGradientPattern, Qt::RadialGradientPattern or
    Qt::ConicalGradientPattern).
*/
QBrush::QBrush(const QGradient &gradient)
{
    Q_ASSERT_X(gradient.type() != QGradient::NoGradient, "QBrush::QBrush",
               "QGradient should not be used directly, use the linear, radial\n"
               "or conical gradients instead");

    const Qt::BrushStyle enum_table[] = {
        Qt::LinearGradientPattern,
        Qt::RadialGradientPattern,
        Qt::ConicalGradientPattern
    };

    init(QColor(), enum_table[gradient.type()]);
    QGradientBrushData *grad = static_cast<QGradientBrushData *>(d.data());
    grad->gradient = gradient;
}

/*!
    Destroys the brush.
*/

QBrush::~QBrush()
{
}

void QBrush::cleanUp(QBrushData *x)
{
    QBrushDataPointerDeleter::deleteData(x);
}


void QBrush::detach(Qt::BrushStyle newStyle)
{
    if (newStyle == d->style && d->ref == 1)
        return;

    QScopedPointer<QBrushData> x;
    switch(newStyle) {
    case Qt::TexturePattern: {
        QTexturedBrushData *tbd = new QTexturedBrushData;
        if (d->style == Qt::TexturePattern) {
            QTexturedBrushData *data = static_cast<QTexturedBrushData *>(d.data());
            if (data->m_has_pixmap_texture)
                tbd->setPixmap(data->pixmap());
            else
                tbd->setImage(data->image());
        }
        x.reset(tbd);
        break;
        }
    case Qt::LinearGradientPattern:
    case Qt::RadialGradientPattern:
    case Qt::ConicalGradientPattern:
        x.reset(new QGradientBrushData);
        static_cast<QGradientBrushData *>(x.data())->gradient =
            static_cast<QGradientBrushData *>(d.data())->gradient;
        break;
    default:
        x.reset(new QBrushData);
        break;
    }
    x->ref = 1;
    x->style = newStyle;
    x->color = d->color;
    x->transform = d->transform;
    d.reset(x.take());
}


/*!
    \fn QBrush &QBrush::operator=(const QBrush &brush)

    Assigns the given \a brush to \e this brush and returns a
    reference to \e this brush.
*/

QBrush &QBrush::operator=(const QBrush &b)
{
    if (d == b.d)
        return *this;

    b.d->ref.ref();
    d.reset(b.d.data());
    return *this;
}


/*!
    \fn void QBrush::swap(QBrush &other)
    \since 4.8

    Swaps brush \a other with this brush. This operation is very
    fast and never fails.
*/

/*!
   Returns the brush as a QVariant
*/
QBrush::operator QVariant() const
{
    return QVariant(QVariant::Brush, this);
}

/*!
    \fn Qt::BrushStyle QBrush::style() const

    Returns the brush style.

    \sa setStyle()
*/

/*!
    Sets the brush style to \a style.

    \sa style()
*/

void QBrush::setStyle(Qt::BrushStyle style)
{
    if (d->style == style)
        return;

    if (qbrush_check_type(style)) {
        detach(style);
        d->style = style;
    }
}


/*!
    \fn const QColor &QBrush::color() const

    Returns the brush color.

    \sa setColor()
*/

/*!
    \fn void QBrush::setColor(const QColor &color)

    Sets the brush color to the given \a color.

    Note that calling setColor() will not make a difference if the
    style is a gradient. The same is the case if the style is
    Qt::TexturePattern style unless the current texture is a QBitmap.

    \sa color()
*/

void QBrush::setColor(const QColor &c)
{
    detach(d->style);
    d->color = c;
}

/*!
    \fn void QBrush::setColor(Qt::GlobalColor color)
    \overload

    Sets the brush color to the given \a color.
*/


#ifdef QT3_SUPPORT

/*!
    \fn void QBrush::setPixmap(const QPixmap &pixmap)

    \compat

    Sets a custom pattern for this brush.

    Use setTexture() instead.
*/

/*!
    \fn QPixmap *QBrush::pixmap() const

    Returns a pointer to the custom brush pattern.

    Use texture() instead.
*/
QPixmap *QBrush::pixmap() const
{
    if (d->style != Qt::TexturePattern)
        return 0;
    QTexturedBrushData *data  = static_cast<QTexturedBrushData*>(d.data());
    QPixmap &pixmap = data->pixmap();
    return pixmap.isNull() ? 0 : &pixmap;
}
#endif

/*!
    \fn QPixmap QBrush::texture() const

    Returns the custom brush pattern, or a null pixmap if no custom brush pattern
    has been set.

    \sa setTexture()
*/
QPixmap QBrush::texture() const
{
    return d->style == Qt::TexturePattern
                     ? (static_cast<QTexturedBrushData *>(d.data()))->pixmap()
                     : QPixmap();
}

/*!
    Sets the brush pixmap to \a pixmap. The style is set to
    Qt::TexturePattern.

    The current brush color will only have an effect for monochrome
    pixmaps, i.e. for QPixmap::depth() == 1 (\l {QBitmap}{QBitmaps}).

    \sa texture()
*/

void QBrush::setTexture(const QPixmap &pixmap)
{
    if (!pixmap.isNull()) {
        detach(Qt::TexturePattern);
        QTexturedBrushData *data = static_cast<QTexturedBrushData *>(d.data());
        data->setPixmap(pixmap);
    } else {
        detach(Qt::NoBrush);
    }
}


/*!
    \since 4.2

    Returns the custom brush pattern, or a null image if no custom
    brush pattern has been set.

    If the texture was set as a QPixmap it will be converted to a
    QImage.

    \sa setTextureImage()
*/

QImage QBrush::textureImage() const
{
    return d->style == Qt::TexturePattern
                     ? (static_cast<QTexturedBrushData *>(d.data()))->image()
                     : QImage();
}


/*!
    \since 4.2

    Sets the brush image to \a image. The style is set to
    Qt::TexturePattern.

    Note the current brush color will \e not have any affect on
    monochrome images, as opposed to calling setTexture() with a
    QBitmap. If you want to change the color of monochrome image
    brushes, either convert the image to QBitmap with \c
    QBitmap::fromImage() and set the resulting QBitmap as a texture,
    or change the entries in the color table for the image.

    \sa textureImage(), setTexture()
*/

void QBrush::setTextureImage(const QImage &image)
{
    if (!image.isNull()) {
        detach(Qt::TexturePattern);
        QTexturedBrushData *data = static_cast<QTexturedBrushData *>(d.data());
        data->setImage(image);
    } else {
        detach(Qt::NoBrush);
    }
}


/*!
    Returns the gradient describing this brush.
*/
const QGradient *QBrush::gradient() const
{
    if (d->style == Qt::LinearGradientPattern
        || d->style == Qt::RadialGradientPattern
        || d->style == Qt::ConicalGradientPattern) {
        return &static_cast<const QGradientBrushData *>(d.data())->gradient;
    }
    return 0;
}

Q_GUI_EXPORT bool qt_isExtendedRadialGradient(const QBrush &brush)
{
    if (brush.style() == Qt::RadialGradientPattern) {
        const QGradient *g = brush.gradient();
        const QRadialGradient *rg = static_cast<const QRadialGradient *>(g);

        if (!qFuzzyIsNull(rg->focalRadius()))
            return true;

        QPointF delta = rg->focalPoint() - rg->center();
        if (delta.x() * delta.x() + delta.y() * delta.y() > rg->radius() * rg->radius())
            return true;
    }

    return false;
}

/*!
    Returns true if the brush is fully opaque otherwise false. A brush
    is considered opaque if:

    \list
    \i The alpha component of the color() is 255.
    \i Its texture() does not have an alpha channel and is not a QBitmap.
    \i The colors in the gradient() all have an alpha component that is 255.
    \i It is an extended radial gradient.
    \endlist
*/

bool QBrush::isOpaque() const
{
    bool opaqueColor = d->color.alpha() == 255;

    // Test awfully simple case first
    if (d->style == Qt::SolidPattern)
        return opaqueColor;

    if (qt_isExtendedRadialGradient(*this))
        return false;

    if (d->style == Qt::LinearGradientPattern
        || d->style == Qt::RadialGradientPattern
        || d->style == Qt::ConicalGradientPattern) {
        QGradientStops stops = gradient()->stops();
        for (int i=0; i<stops.size(); ++i)
            if (stops.at(i).second.alpha() != 255)
                return false;
        return true;
    } else if (d->style == Qt::TexturePattern) {
        return qHasPixmapTexture(*this)
            ? !texture().hasAlphaChannel() && !texture().isQBitmap()
            : !textureImage().hasAlphaChannel();
    }

    return false;
}


/*!
    \since 4.2

    Sets \a matrix as an explicit transformation matrix on the
    current brush. The brush transformation matrix is merged with
    QPainter transformation matrix to produce the final result.

    \sa matrix()
*/
void QBrush::setMatrix(const QMatrix &matrix)
{
    setTransform(QTransform(matrix));
}

/*!
    \since 4.3

    Sets \a matrix as an explicit transformation matrix on the
    current brush. The brush transformation matrix is merged with
    QPainter transformation matrix to produce the final result.

    \sa transform()
*/
void QBrush::setTransform(const QTransform &matrix)
{
    detach(d->style);
    d->transform = matrix;
}


/*!
    \fn void QBrush::matrix() const
    \since 4.2

    Returns the current transformation matrix for the brush.

    \sa setMatrix()
*/

/*!
    \fn bool QBrush::operator!=(const QBrush &brush) const

    Returns true if the brush is different from the given \a brush;
    otherwise returns false.

    Two brushes are different if they have different styles, colors or
    transforms or different pixmaps or gradients depending on the style.

    \sa operator==()
*/

/*!
    \fn bool QBrush::operator==(const QBrush &brush) const

    Returns true if the brush is equal to the given \a brush;
    otherwise returns false.

    Two brushes are equal if they have equal styles, colors and
    transforms and equal pixmaps or gradients depending on the style.

    \sa operator!=()
*/

bool QBrush::operator==(const QBrush &b) const
{
    if (b.d == d)
        return true;
    if (b.d->style != d->style || b.d->color != d->color || b.d->transform != d->transform)
        return false;
    switch (d->style) {
    case Qt::TexturePattern:
        {
            const QPixmap &us = (static_cast<QTexturedBrushData *>(d.data()))->pixmap();
            const QPixmap &them = (static_cast<QTexturedBrushData *>(b.d.data()))->pixmap();
            return ((us.isNull() && them.isNull()) || us.cacheKey() == them.cacheKey());
        }
    case Qt::LinearGradientPattern:
    case Qt::RadialGradientPattern:
    case Qt::ConicalGradientPattern:
        {
            const QGradientBrushData *d1 = static_cast<QGradientBrushData *>(d.data());
            const QGradientBrushData *d2 = static_cast<QGradientBrushData *>(b.d.data());
            return d1->gradient == d2->gradient;
        }
    default:
        return true;
    }
}

/*!
    \fn QBrush::operator const QColor&() const

    Returns the brush's color.

    Use color() instead.
*/

#ifndef QT_NO_DEBUG_STREAM
/*!
  \internal
*/
QDebug operator<<(QDebug dbg, const QBrush &b)
{
#ifndef Q_BROKEN_DEBUG_STREAM
    static const char *BRUSH_STYLES[] = {
     "NoBrush",
     "SolidPattern",
     "Dense1Pattern",
     "Dense2Pattern",
     "Dense3Pattern",
     "Dense4Pattern",
     "Dense5Pattern",
     "Dense6Pattern",
     "Dense7Pattern",
     "HorPattern",
     "VerPattern",
     "CrossPattern",
     "BDiagPattern",
     "FDiagPattern",
     "DiagCrossPattern",
     "LinearGradientPattern",
     "RadialGradientPattern",
     "ConicalGradientPattern",
     0, 0, 0, 0, 0, 0,
     "TexturePattern" // 24
    };

    dbg.nospace() << "QBrush(" << b.color() << ',' << BRUSH_STYLES[b.style()] << ')';
    return dbg.space();
#else
    qWarning("This compiler doesn't support streaming QBrush to QDebug");
    return dbg;
    Q_UNUSED(b);
#endif
}
#endif

/*****************************************************************************
  QBrush stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QBrush &brush)
    \relates QBrush

    Writes the given \a brush to the given \a stream and returns a
    reference to the \a stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &s, const QBrush &b)
{
    quint8 style = (quint8) b.style();
    bool gradient_style = false;

    if (style == Qt::LinearGradientPattern || style == Qt::RadialGradientPattern
        || style == Qt::ConicalGradientPattern)
        gradient_style = true;

    if (s.version() < QDataStream::Qt_4_0 && gradient_style)
        style = Qt::NoBrush;

    s << style << b.color();
    if (b.style() == Qt::TexturePattern) {
        s << b.texture();
    } else if (s.version() >= QDataStream::Qt_4_0 && gradient_style) {
        const QGradient *gradient = b.gradient();
        int type_as_int = int(gradient->type());
        s << type_as_int;
        if (s.version() >= QDataStream::Qt_4_3) {
            s << int(gradient->spread());
            s << int(gradient->coordinateMode());
        }

        if (s.version() >= QDataStream::Qt_4_5)
            s << int(gradient->interpolationMode());

        if (sizeof(qreal) == sizeof(double)) {
            s << gradient->stops();
        } else {
            // ensure that we write doubles here instead of streaming the stops
            // directly; otherwise, platforms that redefine qreal might generate
            // data that cannot be read on other platforms.
            QVector<QGradientStop> stops = gradient->stops();
            s << quint32(stops.size());
            for (int i = 0; i < stops.size(); ++i) {
                const QGradientStop &stop = stops.at(i);
                s << QPair<double, QColor>(double(stop.first), stop.second);
            }
        }

        if (gradient->type() == QGradient::LinearGradient) {
            s << static_cast<const QLinearGradient *>(gradient)->start();
            s << static_cast<const QLinearGradient *>(gradient)->finalStop();
        } else if (gradient->type() == QGradient::RadialGradient) {
            s << static_cast<const QRadialGradient *>(gradient)->center();
            s << static_cast<const QRadialGradient *>(gradient)->focalPoint();
            s << (double) static_cast<const QRadialGradient *>(gradient)->radius();
        } else { // type == Conical
            s << static_cast<const QConicalGradient *>(gradient)->center();
            s << (double) static_cast<const QConicalGradient *>(gradient)->angle();
        }
    }
    if (s.version() >= QDataStream::Qt_4_3)
        s << b.transform();
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QBrush &brush)
    \relates QBrush

    Reads the given \a brush from the given \a stream and returns a
    reference to the \a stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &s, QBrush &b)
{
    quint8 style;
    QColor color;
    s >> style;
    s >> color;
    if (style == Qt::TexturePattern) {
        QPixmap pm;
        s >> pm;
        b = QBrush(color, pm);
    } else if (style == Qt::LinearGradientPattern
               || style == Qt::RadialGradientPattern
               || style == Qt::ConicalGradientPattern) {

        int type_as_int;
        QGradient::Type type;
        QGradientStops stops;
        QGradient::CoordinateMode cmode = QGradient::LogicalMode;
        QGradient::Spread spread = QGradient::PadSpread;
        QGradient::InterpolationMode imode = QGradient::ColorInterpolation;

        s >> type_as_int;
        type = QGradient::Type(type_as_int);
        if (s.version() >= QDataStream::Qt_4_3) {
            s >> type_as_int;
            spread = QGradient::Spread(type_as_int);
            s >> type_as_int;
            cmode = QGradient::CoordinateMode(type_as_int);
        }

        if (s.version() >= QDataStream::Qt_4_5) {
            s >> type_as_int;
            imode = QGradient::InterpolationMode(type_as_int);
        }

        if (sizeof(qreal) == sizeof(double)) {
            s >> stops;
        } else {
            quint32 numStops;
            double n;
            QColor c;

            s >> numStops;
            for (quint32 i = 0; i < numStops; ++i) {
                s >> n >> c;
                stops << QPair<qreal, QColor>(n, c);
            }
        }

        if (type == QGradient::LinearGradient) {
            QPointF p1, p2;
            s >> p1;
            s >> p2;
            QLinearGradient lg(p1, p2);
            lg.setStops(stops);
            lg.setSpread(spread);
            lg.setCoordinateMode(cmode);
            lg.setInterpolationMode(imode);
            b = QBrush(lg);
        } else if (type == QGradient::RadialGradient) {
            QPointF center, focal;
            double radius;
            s >> center;
            s >> focal;
            s >> radius;
            QRadialGradient rg(center, radius, focal);
            rg.setStops(stops);
            rg.setSpread(spread);
            rg.setCoordinateMode(cmode);
            rg.setInterpolationMode(imode);
            b = QBrush(rg);
        } else { // type == QGradient::ConicalGradient
            QPointF center;
            double angle;
            s >> center;
            s >> angle;
            QConicalGradient cg(center, angle);
            cg.setStops(stops);
            cg.setSpread(spread);
            cg.setCoordinateMode(cmode);
            cg.setInterpolationMode(imode);
            b = QBrush(cg);
        }
    } else {
        b = QBrush(color, (Qt::BrushStyle)style);
    }
    if (s.version() >= QDataStream::Qt_4_3) {
        QTransform transform;
        s >> transform;
        b.setTransform(transform);
    }
    return s;
}
#endif // QT_NO_DATASTREAM

/*******************************************************************************
 * QGradient implementations
 */


/*!
    \class QGradient
    \ingroup painting
    \ingroup shared

    \brief The QGradient class is used in combination with QBrush to
    specify gradient fills.

    Qt currently supports three types of gradient fills:

    \list
    \o \e Linear gradients interpolate colors between start and end points.
    \o \e Simple radial gradients interpolate colors between a focal point
        and end points on a circle surrounding it.
    \o \e Extended radial gradients interpolate colors between a center and
        a focal circle.
    \o \e Conical gradients interpolate colors around a center point.
    \endlist

    A gradient's type can be retrieved using the type() function.
    Each of the types is represented by a subclass of QGradient:

    \table
    \header
    \o QLinearGradient
    \o QRadialGradient
    \o QConicalGradient
    \row
    \o \inlineimage qgradient-linear.png
    \o \inlineimage qgradient-radial.png
    \o \inlineimage qgradient-conical.png
    \endtable

    The colors in a gradient are defined using stop points of the
    QGradientStop type; i.e., a position and a color. Use the setColorAt()
    function to define a single stop point. Alternatively, use the
    setStops() function to define several stop points in one go. Note that
    the latter function \e replaces the current set of stop points.

    It is the gradient's complete set of stop points (accessible
    through the stops() function) that describes how the gradient area
    should be filled. If no stop points have been specified, a gradient
    of black at 0 to white at 1 is used.

    A diagonal linear gradient from black at (100, 100) to white at
    (200, 200) could be specified like this:

    \snippet doc/src/snippets/brush/brush.cpp 0

    A gradient can have an arbitrary number of stop points. The
    following would create a radial gradient starting with
    red in the center, blue and then green on the edges:

    \snippet doc/src/snippets/brush/brush.cpp 1

    It is possible to repeat or reflect the gradient outside its area
    by specifiying the \l {QGradient::Spread}{spread method} using the
    setSpread() function. The default is to pad the outside area with
    the color at the closest stop point. The currently set \l
    {QGradient::Spread}{spread method} can be retrieved using the
    spread() function. The QGradient::Spread enum defines three
    different methods:

    \table
    \row
    \o \inlineimage qradialgradient-pad.png
    \o \inlineimage qradialgradient-repeat.png
    \o \inlineimage qradialgradient-reflect.png
    \row
    \o \l {QGradient::PadSpread}{PadSpread}
    \o \l {QGradient::RepeatSpread}{RepeatSpread}
    \o \l {QGradient::ReflectSpread}{ReflectSpread}
    \endtable

    Note that the setSpread() function only has effect for linear and
    radial gradients. The reason is that the conical gradient is
    closed by definition, i.e. the \e conical gradient fills the
    entire circle from 0 - 360 degrees, while the boundary of a radial
    or a linear gradient can be specified through its radius or final
    stop points, respectively.

    The gradient coordinates can be specified in logical coordinates,
    relative to device coordinates, or relative to object bounding box coordinates.
    The \l {QGradient::CoordinateMode}{coordinate mode} can be set using the
    setCoordinateMode() function. The default is LogicalMode, where the
    gradient coordinates are specified in the same way as the object
    coordinates. To retrieve the currently set \l {QGradient::CoordinateMode}
    {coordinate mode} use coordinateMode().


    \sa {demos/gradients}{The Gradients Demo}, QBrush
*/

/*!
    \internal
*/
QGradient::QGradient()
    : m_type(NoGradient), dummy(0)
{
}


/*!
    \enum QGradient::Type

    Specifies the type of gradient.

    \value LinearGradient  Interpolates colors between start and end points
    (QLinearGradient).

    \value RadialGradient Interpolate colors between a focal point and end
    points on a circle surrounding it (QRadialGradient).

    \value ConicalGradient Interpolate colors around a center point (QConicalGradient).
    \value NoGradient No gradient is used.

    \sa type()
*/

/*!
    \enum QGradient::Spread

    Specifies how the area outside the gradient area should be
    filled.

    \value PadSpread The area is filled with the closest stop
    color. This is the default.

    \value RepeatSpread The gradient  is repeated outside the gradient
    area.

    \value ReflectSpread The gradient is reflected outside the
    gradient area.

    \sa spread(), setSpread()
*/

/*!
    \fn void QGradient::setSpread(Spread method)

    Specifies the spread \a method that should be used for this
    gradient.

    Note that this function only has effect for linear and radial
    gradients.

    \sa spread()
*/

/*!
    \fn QGradient::Spread QGradient::spread() const

    Returns the spread method use by this gradient. The default is
    PadSpread.

    \sa setSpread()
*/

/*!
    \fn QGradient::Type QGradient::type() const

    Returns the type of gradient.
*/

/*!
    \fn void QGradient::setColorAt(qreal position, const QColor &color)

    Creates a stop point at the given \a position with the given \a
    color. The given \a position must be in the range 0 to 1.

    \sa setStops(), stops()
*/

void QGradient::setColorAt(qreal pos, const QColor &color)
{
    if ((pos > 1 || pos < 0) && !qIsNaN(pos)) {
        qWarning("QGradient::setColorAt: Color position must be specified in the range 0 to 1");
        return;
    }

    int index = 0;
    if (!qIsNaN(pos))
        while (index < m_stops.size() && m_stops.at(index).first < pos) ++index;

    if (index < m_stops.size() && m_stops.at(index).first == pos)
        m_stops[index].second = color;
    else
        m_stops.insert(index, QGradientStop(pos, color));
}

/*!
    \fn void QGradient::setStops(const QGradientStops &stopPoints)

    Replaces the current set of stop points with the given \a
    stopPoints. The positions of the points must be in the range 0 to
    1, and must be sorted with the lowest point first.

    \sa setColorAt(), stops()
*/
void QGradient::setStops(const QGradientStops &stops)
{
    m_stops.clear();
    for (int i=0; i<stops.size(); ++i)
        setColorAt(stops.at(i).first, stops.at(i).second);
}


/*!
    Returns the stop points for this gradient.

    If no stop points have been specified, a gradient of black at 0 to white
    at 1 is used.

    \sa setStops(), setColorAt()
*/
QGradientStops QGradient::stops() const
{
    if (m_stops.isEmpty()) {
        QGradientStops tmp;
        tmp << QGradientStop(0, Qt::black) << QGradientStop(1, Qt::white);
        return tmp;
    }
    return m_stops;
}

#define Q_DUMMY_ACCESSOR union {void *p; uint i;}; p = dummy;

/*!
    \enum QGradient::CoordinateMode
    \since 4.4

    This enum specifies how gradient coordinates map to the paint
    device on which the gradient is used.

    \value LogicalMode This is the default mode. The gradient coordinates
    are specified logical space just like the object coordinates.
    \value StretchToDeviceMode In this mode the gradient coordinates
    are relative to the bounding rectangle of the paint device,
    with (0,0) in the top left corner, and (1,1) in the bottom right
    corner of the paint device.
    \value ObjectBoundingMode In this mode the gradient coordinates are
    relative to the bounding rectangle of the object being drawn, with
    (0,0) in the top left corner, and (1,1) in the bottom right corner
    of the object's bounding rectangle.
*/

/*!
    \since 4.4

    Returns the coordinate mode of this gradient. The default mode is
    LogicalMode.
*/
QGradient::CoordinateMode QGradient::coordinateMode() const
{
    Q_DUMMY_ACCESSOR
    return CoordinateMode(i & 0x03);
}

/*!
    \since 4.4

    Sets the coordinate mode of this gradient to \a mode. The default
    mode is LogicalMode.
*/
void QGradient::setCoordinateMode(CoordinateMode mode)
{
    Q_DUMMY_ACCESSOR
    i &= ~0x03;
    i |= uint(mode);
    dummy = p;
}

/*!
    \enum QGradient::InterpolationMode
    \since 4.5
    \internal

    \value ComponentInterpolation The color components and the alpha component are
    independently linearly interpolated.
    \value ColorInterpolation The colors are linearly interpolated in
    premultiplied color space.
*/

/*!
    \since 4.5
    \internal

    Returns the interpolation mode of this gradient. The default mode is
    ColorInterpolation.
*/
QGradient::InterpolationMode QGradient::interpolationMode() const
{
    Q_DUMMY_ACCESSOR
    return InterpolationMode((i >> 2) & 0x01);
}

/*!
    \since 4.5
    \internal

    Sets the interpolation mode of this gradient to \a mode. The default
    mode is ColorInterpolation.
*/
void QGradient::setInterpolationMode(InterpolationMode mode)
{
    Q_DUMMY_ACCESSOR
    i &= ~(1 << 2);
    i |= (uint(mode) << 2);
    dummy = p;
}

/*!
    \fn bool QGradient::operator!=(const QGradient &gradient) const
    \since 4.2

    Returns true if the gradient is the same as the other \a gradient
    specified; otherwise returns false.

    \sa operator==()
*/

/*!
    Returns true if the gradient is the same as the other \a gradient
    specified; otherwise returns false.

    \sa operator!=()
*/
bool QGradient::operator==(const QGradient &gradient) const
{
    if (gradient.m_type != m_type
        || gradient.m_spread != m_spread
        || gradient.dummy != dummy) return false;

    if (m_type == LinearGradient) {
        if (m_data.linear.x1 != gradient.m_data.linear.x1
            || m_data.linear.y1 != gradient.m_data.linear.y1
            || m_data.linear.x2 != gradient.m_data.linear.x2
            || m_data.linear.y2 != gradient.m_data.linear.y2)
            return false;
    } else if (m_type == RadialGradient) {
        if (m_data.radial.cx != gradient.m_data.radial.cx
            || m_data.radial.cy != gradient.m_data.radial.cy
            || m_data.radial.fx != gradient.m_data.radial.fx
            || m_data.radial.fy != gradient.m_data.radial.fy
            || m_data.radial.cradius != gradient.m_data.radial.cradius)
            return false;
    } else { // m_type == ConicalGradient
        if (m_data.conical.cx != gradient.m_data.conical.cx
            || m_data.conical.cy != gradient.m_data.conical.cy
            || m_data.conical.angle != gradient.m_data.conical.angle)
            return false;
    }

    return stops() == gradient.stops();
}

/*!
    \internal
*/
bool QGradient::operator==(const QGradient &gradient)
{
    return const_cast<const QGradient *>(this)->operator==(gradient);
}

/*!
    \class QLinearGradient
    \ingroup painting

    \brief The QLinearGradient class is used in combination with QBrush to
    specify a linear gradient brush.

    Linear gradients interpolate colors between start and end
    points. Outside these points the gradient is either padded,
    reflected or repeated depending on the currently set \l
    {QGradient::Spread}{spread} method:

    \table
    \row
    \o \inlineimage qlineargradient-pad.png
    \o \inlineimage qlineargradient-reflect.png
    \o \inlineimage qlineargradient-repeat.png
    \row
    \o \l {QGradient::PadSpread}{PadSpread} (default)
    \o \l {QGradient::ReflectSpread}{ReflectSpread}
    \o \l {QGradient::RepeatSpread}{RepeatSpread}
    \endtable

    The colors in a gradient is defined using stop points of the
    QGradientStop type, i.e. a position and a color. Use the
    QGradient::setColorAt() or the QGradient::setStops() function to
    define the stop points. It is the gradient's complete set of stop
    points that describes how the gradient area should be filled. If
    no stop points have been specified, a gradient of black at 0 to
    white at 1 is used.

    In addition to the functions inherited from QGradient, the
    QLinearGradient class provides the finalStop() function which
    returns the final stop point of the gradient, and the start()
    function returning the start point of the gradient.

    \sa QRadialGradient, QConicalGradient, {demos/gradients}{The
    Gradients Demo}
*/


/*!
    Constructs a default linear gradient with interpolation area
    between (0, 0) and (1, 1).

    \sa QGradient::setColorAt(), setStart(), setFinalStop()
*/

QLinearGradient::QLinearGradient()
{
    m_type = LinearGradient;
    m_spread = PadSpread;
    m_data.linear.x1 = 0;
    m_data.linear.y1 = 0;
    m_data.linear.x2 = 1;
    m_data.linear.y2 = 1;
}


/*!
    Constructs a linear gradient with interpolation area between the
    given \a start point and \a finalStop.

    \note The expected parameter values are in pixels.

    \sa QGradient::setColorAt(), QGradient::setStops()
*/
QLinearGradient::QLinearGradient(const QPointF &start, const QPointF &finalStop)
{
    m_type = LinearGradient;
    m_spread = PadSpread;
    m_data.linear.x1 = start.x();
    m_data.linear.y1 = start.y();
    m_data.linear.x2 = finalStop.x();
    m_data.linear.y2 = finalStop.y();
}

/*!
    \fn QLinearGradient::QLinearGradient(qreal x1, qreal y1, qreal x2, qreal y2)

    Constructs a linear gradient with interpolation area between (\a
    x1, \a y1) and (\a x2, \a y2).

    \note The expected parameter values are in pixels.

    \sa QGradient::setColorAt(), QGradient::setStops()
*/
QLinearGradient::QLinearGradient(qreal xStart, qreal yStart, qreal xFinalStop, qreal yFinalStop)
{
    m_type = LinearGradient;
    m_spread = PadSpread;
    m_data.linear.x1 = xStart;
    m_data.linear.y1 = yStart;
    m_data.linear.x2 = xFinalStop;
    m_data.linear.y2 = yFinalStop;
}


/*!
    Returns the start point of this linear gradient in logical coordinates.

    \sa QGradient::stops()
*/

QPointF QLinearGradient::start() const
{
    Q_ASSERT(m_type == LinearGradient);
    return QPointF(m_data.linear.x1, m_data.linear.y1);
}

/*!
    \fn void QLinearGradient::setStart(qreal x, qreal y)
    \overload
    \since 4.2

    Sets the start point of this linear gradient in logical
    coordinates to \a x, \a y.

    \sa start()
*/

/*!
    \since 4.2

    Sets the start point of this linear gradient in logical
    coordinates to \a start.

    \sa start()
*/

void QLinearGradient::setStart(const QPointF &start)
{
    Q_ASSERT(m_type == LinearGradient);
    m_data.linear.x1 = start.x();
    m_data.linear.y1 = start.y();
}


/*!
    \fn void QLinearGradient::setFinalStop(qreal x, qreal y)
    \overload
    \since 4.2

    Sets the final stop point of this linear gradient in logical
    coordinates to \a x, \a y.

    \sa start()
*/

/*!
    Returns the final stop point of this linear gradient in logical coordinates.

    \sa QGradient::stops()
*/

QPointF QLinearGradient::finalStop() const
{
    Q_ASSERT(m_type == LinearGradient);
    return QPointF(m_data.linear.x2, m_data.linear.y2);
}


/*!
    \since 4.2

    Sets the final stop point of this linear gradient in logical
    coordinates to \a stop.

    \sa finalStop()
*/

void QLinearGradient::setFinalStop(const QPointF &stop)
{
    Q_ASSERT(m_type == LinearGradient);
    m_data.linear.x2 = stop.x();
    m_data.linear.y2 = stop.y();
}


/*!
    \class QRadialGradient
    \ingroup painting

    \brief The QRadialGradient class is used in combination with QBrush to
    specify a radial gradient brush.

    Qt supports both simple and extended radial gradients.

    Simple radial gradients interpolate colors between a focal point and end
    points on a circle surrounding it. Extended radial gradients interpolate
    colors between a focal circle and a center circle. Points outside the cone
    defined by the two circles will be transparent. For simple radial gradients
    the focal point is adjusted to lie inside the center circle, whereas the
    focal point can have any position in an extended radial gradient.

    Outside the end points the gradient is either padded, reflected or repeated
    depending on the currently set \l {QGradient::Spread}{spread} method:

    \table
    \row
    \o \inlineimage qradialgradient-pad.png
    \o \inlineimage qradialgradient-reflect.png
    \o \inlineimage qradialgradient-repeat.png
    \row
    \o \l {QGradient::PadSpread}{PadSpread} (default)
    \o \l {QGradient::ReflectSpread}{ReflectSpread}
    \o \l {QGradient::RepeatSpread}{RepeatSpread}
    \endtable

    The colors in a gradient is defined using stop points of the
    QGradientStop type, i.e. a position and a color. Use the
    QGradient::setColorAt() or the QGradient::setStops() function to
    define the stop points. It is the gradient's complete set of stop
    points that describes how the gradient area should be filled.  If
    no stop points have been specified, a gradient of black at 0 to
    white at 1 is used.

    In addition to the functions inherited from QGradient, the
    QRadialGradient class provides the center(), focalPoint() and
    radius() functions returning the gradient's center, focal point
    and radius respectively.

    \sa QLinearGradient, QConicalGradient, {demos/gradients}{The
    Gradients Demo}
*/

static QPointF qt_radial_gradient_adapt_focal_point(const QPointF &center,
                                                    qreal radius,
                                                    const QPointF &focalPoint)
{
    // We have a one pixel buffer zone to avoid numerical instability on the
    // circle border
    //### this is hacky because technically we should adjust based on current matrix
    const qreal compensated_radius = radius - radius * qreal(0.001);
    QLineF line(center, focalPoint);
    if (line.length() > (compensated_radius))
        line.setLength(compensated_radius);
    return line.p2();
}

/*!
    Constructs a simple radial gradient with the given \a center, \a
    radius and \a focalPoint.

    \note If the given focal point is outside the circle defined by the
    \a center point and \a radius, it will be re-adjusted to lie at a point on
    the circle where it intersects with the line from \a center to
    \a focalPoint.

    \sa QGradient::setColorAt(), QGradient::setStops()
*/

QRadialGradient::QRadialGradient(const QPointF &center, qreal radius, const QPointF &focalPoint)
{
    m_type = RadialGradient;
    m_spread = PadSpread;
    m_data.radial.cx = center.x();
    m_data.radial.cy = center.y();
    m_data.radial.cradius = radius;

    QPointF adapted_focal = qt_radial_gradient_adapt_focal_point(center, radius, focalPoint);
    m_data.radial.fx = adapted_focal.x();
    m_data.radial.fy = adapted_focal.y();
}

/*!
    Constructs a simple radial gradient with the given \a center, \a
    radius and the focal point in the circle center.

    \sa QGradient::setColorAt(), QGradient::setStops()
*/
QRadialGradient::QRadialGradient(const QPointF &center, qreal radius)
{
    m_type = RadialGradient;
    m_spread = PadSpread;
    m_data.radial.cx = center.x();
    m_data.radial.cy = center.y();
    m_data.radial.cradius = radius;
    m_data.radial.fx = center.x();
    m_data.radial.fy = center.y();
}


/*!
    Constructs a simple radial gradient with the given center (\a cx, \a cy),
    \a radius and focal point (\a fx, \a fy).

    \note If the given focal point is outside the circle defined by the
    center (\a cx, \a cy) and the \a radius it will be re-adjusted to
    the intersection between the line from the center to the focal point
    and the circle.

    \sa QGradient::setColorAt(), QGradient::setStops()
*/

QRadialGradient::QRadialGradient(qreal cx, qreal cy, qreal radius, qreal fx, qreal fy)
{
    m_type = RadialGradient;
    m_spread = PadSpread;
    m_data.radial.cx = cx;
    m_data.radial.cy = cy;
    m_data.radial.cradius = radius;

    QPointF adapted_focal = qt_radial_gradient_adapt_focal_point(QPointF(cx, cy),
                                                                 radius,
                                                                 QPointF(fx, fy));

    m_data.radial.fx = adapted_focal.x();
    m_data.radial.fy = adapted_focal.y();
}

/*!
    Constructs a simple radial gradient with the center at (\a cx, \a cy) and the
    specified \a radius. The focal point lies at the center of the circle.

    \sa QGradient::setColorAt(), QGradient::setStops()
 */
QRadialGradient::QRadialGradient(qreal cx, qreal cy, qreal radius)
{
    m_type = RadialGradient;
    m_spread = PadSpread;
    m_data.radial.cx = cx;
    m_data.radial.cy = cy;
    m_data.radial.cradius = radius;
    m_data.radial.fx = cx;
    m_data.radial.fy = cy;
}


/*!
    Constructs a simple radial gradient with the center and focal point at
    (0, 0) with a radius of 1.
*/
QRadialGradient::QRadialGradient()
{
    m_type = RadialGradient;
    m_spread = PadSpread;
    m_data.radial.cx = 0;
    m_data.radial.cy = 0;
    m_data.radial.cradius = 1;
    m_data.radial.fx = 0;
    m_data.radial.fy = 0;
}

/*!
    \since 4.8

    Constructs an extended radial gradient with the given \a center, \a
    centerRadius, \a focalPoint, and \a focalRadius.
*/
QRadialGradient::QRadialGradient(const QPointF &center, qreal centerRadius, const QPointF &focalPoint, qreal focalRadius)
{
    m_type = RadialGradient;
    m_spread = PadSpread;
    m_data.radial.cx = center.x();
    m_data.radial.cy = center.y();
    m_data.radial.cradius = centerRadius;

    m_data.radial.fx = focalPoint.x();
    m_data.radial.fy = focalPoint.y();
    setFocalRadius(focalRadius);
}

/*!
    \since 4.8

    Constructs an extended radial gradient with the given center
    (\a cx, \a cy), center radius, \a centerRadius, focal point, (\a fx, \a fy),
    and focal radius \a focalRadius.
*/
QRadialGradient::QRadialGradient(qreal cx, qreal cy, qreal centerRadius, qreal fx, qreal fy, qreal focalRadius)
{
    m_type = RadialGradient;
    m_spread = PadSpread;
    m_data.radial.cx = cx;
    m_data.radial.cy = cy;
    m_data.radial.cradius = centerRadius;

    m_data.radial.fx = fx;
    m_data.radial.fy = fy;
    setFocalRadius(focalRadius);
}

/*!
    Returns the center of this radial gradient in logical coordinates.

    \sa QGradient::stops()
*/

QPointF QRadialGradient::center() const
{
    Q_ASSERT(m_type == RadialGradient);
    return QPointF(m_data.radial.cx, m_data.radial.cy);
}

/*!
    \fn void QRadialGradient::setCenter(qreal x, qreal y)
    \overload
    \since 4.2

    Sets the center of this radial gradient in logical coordinates
    to (\a x, \a y).

    \sa center()
*/

/*!
    \since 4.2

    Sets the center of this radial gradient in logical coordinates
    to \a center.

    \sa center()
*/

void QRadialGradient::setCenter(const QPointF &center)
{
    Q_ASSERT(m_type == RadialGradient);
    m_data.radial.cx = center.x();
    m_data.radial.cy = center.y();
}


/*!
    Returns the radius of this radial gradient in logical coordinates.

    Equivalent to centerRadius()

    \sa QGradient::stops()
*/

qreal QRadialGradient::radius() const
{
    Q_ASSERT(m_type == RadialGradient);
    return m_data.radial.cradius;
}


/*!
    \since 4.2

    Sets the radius of this radial gradient in logical coordinates
    to \a radius

    Equivalent to setCenterRadius()
*/
void QRadialGradient::setRadius(qreal radius)
{
    Q_ASSERT(m_type == RadialGradient);
    m_data.radial.cradius = radius;
}

/*!
    \since 4.8

    Returns the center radius of this radial gradient in logical
    coordinates.

    \sa QGradient::stops()
*/
qreal QRadialGradient::centerRadius() const
{
    Q_ASSERT(m_type == RadialGradient);
    return m_data.radial.cradius;
}

/*!
   \since 4.8

   Sets the center radius of this radial gradient in logical coordinates
   to \a radius
*/
void QRadialGradient::setCenterRadius(qreal radius)
{
    Q_ASSERT(m_type == RadialGradient);
    m_data.radial.cradius = radius;
}

/*!
    \since 4.8

    Returns the focal radius of this radial gradient in logical
    coordinates.

    \sa QGradient::stops()
*/
qreal QRadialGradient::focalRadius() const
{
    Q_ASSERT(m_type == RadialGradient);
    Q_DUMMY_ACCESSOR

    // mask away low three bits
    union { float f; quint32 i; } u;
    u.i = i & ~0x07;
    return u.f;
}

/*!
   \since 4.8

   Sets the focal radius of this radial gradient in logical coordinates
   to \a radius
*/
void QRadialGradient::setFocalRadius(qreal radius)
{
    Q_ASSERT(m_type == RadialGradient);
    Q_DUMMY_ACCESSOR

    // Since there's no QGradientData, we only have the dummy void * to
    // store additional data in. The three lowest bits are already
    // taken, thus we cut the three lowest bits from the significand
    // and store the radius as a float.
    union { float f; quint32 i; } u;
    u.f = float(radius);
    // add 0x04 to round up when we drop the three lowest bits
    i |= (u.i + 0x04) & ~0x07;
    dummy = p;
}

/*!
    Returns the focal point of this radial gradient in logical
    coordinates.

    \sa QGradient::stops()
*/

QPointF QRadialGradient::focalPoint() const
{
    Q_ASSERT(m_type == RadialGradient);
    return QPointF(m_data.radial.fx, m_data.radial.fy);
}

/*!
    \fn void QRadialGradient::setFocalPoint(qreal x, qreal y)
    \overload
    \since 4.2

    Sets the focal point of this radial gradient in logical
    coordinates to (\a x, \a y).

    \sa focalPoint()
*/

/*!
    \since 4.2

    Sets the focal point of this radial gradient in logical
    coordinates to \a focalPoint.

    \sa focalPoint()
*/

void QRadialGradient::setFocalPoint(const QPointF &focalPoint)
{
    Q_ASSERT(m_type == RadialGradient);
    m_data.radial.fx = focalPoint.x();
    m_data.radial.fy = focalPoint.y();
}



/*!
    \class QConicalGradient
    \ingroup painting

    \brief The QConicalGradient class is used in combination with QBrush to
    specify a conical gradient brush.

    Conical gradients interpolate interpolate colors counter-clockwise
    around a center point.

    \image qconicalgradient.png

    The colors in a gradient is defined using stop points of the
    QGradientStop type, i.e. a position and a color. Use the
    QGradient::setColorAt() or the QGradient::setStops() function to
    define the stop points. It is the gradient's complete set of stop
    points that describes how the gradient area should be filled. If
    no stop points have been specified, a gradient of black at 0 to
    white at 1 is used.

    In addition to the functions inherited from QGradient, the
    QConicalGradient class provides the angle() and center() functions
    returning the start angle and center of the gradient.

    Note that the setSpread() function has no effect for conical
    gradients. The reason is that the conical gradient is closed by
    definition, i.e. the conical gradient fills the entire circle from
    0 - 360 degrees, while the boundary of a radial or a linear
    gradient can be specified through its radius or final stop points,
    respectively.

    \sa QLinearGradient, QRadialGradient, {demos/gradients}{The
    Gradients Demo}
*/


/*!
    Constructs a conical gradient with the given \a center, starting
    the interpolation at the given \a angle. The \a angle must be
    specified in degrees between 0 and 360.

    \sa QGradient::setColorAt(), QGradient::setStops()
*/

QConicalGradient::QConicalGradient(const QPointF &center, qreal angle)
{
    m_type = ConicalGradient;
    m_spread = PadSpread;
    m_data.conical.cx = center.x();
    m_data.conical.cy = center.y();
    m_data.conical.angle = angle;
}


/*!
    Constructs a conical gradient with the given center (\a cx, \a
    cy), starting the interpolation at the given \a angle. The angle
    must be specified in degrees between 0 and 360.

    \sa QGradient::setColorAt(), QGradient::setStops()
*/

QConicalGradient::QConicalGradient(qreal cx, qreal cy, qreal angle)
{
    m_type = ConicalGradient;
    m_spread = PadSpread;
    m_data.conical.cx = cx;
    m_data.conical.cy = cy;
    m_data.conical.angle = angle;
}


/*!
    Constructs a conical with center at (0, 0) starting the
    interpolation at angle 0.

    \sa QGradient::setColorAt(), setCenter(), setAngle()
*/

QConicalGradient::QConicalGradient()
{
    m_type = ConicalGradient;
    m_spread = PadSpread;
    m_data.conical.cx = 0;
    m_data.conical.cy = 0;
    m_data.conical.angle = 0;
}


/*!
    Returns the center of the conical gradient in logical
    coordinates.

    \sa stops()
*/

QPointF QConicalGradient::center() const
{
    Q_ASSERT(m_type == ConicalGradient);
    return QPointF(m_data.conical.cx, m_data.conical.cy);
}


/*!
    \fn void QConicalGradient::setCenter(qreal x, qreal y)

    \overload

    Sets the center of this conical gradient in logical coordinates to
    (\a x, \a y).

    \sa center()
*/

/*!
    Sets the center of this conical gradient in logical coordinates to
    \a center.

    \sa center()
*/

void QConicalGradient::setCenter(const QPointF &center)
{
    Q_ASSERT(m_type == ConicalGradient);
    m_data.conical.cx = center.x();
    m_data.conical.cy = center.y();
}

/*!
    Returns the start angle of the conical gradient in logical
    coordinates.

    \sa stops()
*/

qreal QConicalGradient::angle() const
{
    Q_ASSERT(m_type == ConicalGradient);
    return m_data.conical.angle;
}


/*!
    \since 4.2

    Sets \a angle to be the start angle for this conical gradient in
    logical coordinates.

    \sa angle()
*/

void QConicalGradient::setAngle(qreal angle)
{
    Q_ASSERT(m_type == ConicalGradient);
    m_data.conical.angle = angle;
}

/*!
    \typedef QGradientStop
    \relates QGradient

    Typedef for QPair<\l qreal, QColor>.
*/

/*!
    \typedef QGradientStops
    \relates QGradient

    Typedef for QVector<QGradientStop>.
*/

/*!
    \typedef QBrush::DataPtr
    \internal
*/

/*!
    \fn DataPtr &QBrush::data_ptr()
    \internal
*/


/*!
    \fn bool QBrush::isDetached() const
    \internal
*/

/*!
    \fn QTransform QBrush::transform() const
    \since 4.3

    Returns the current transformation matrix for the brush.

    \sa setTransform()
*/

#undef Q_DUMMY_ACCESSOR

QT_END_NAMESPACE
