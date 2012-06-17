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

#include "qabstractfontengine_qws.h"
#include "qabstractfontengine_p.h"

#include <private/qtextengine_p.h>
#include <private/qpaintengine_raster_p.h>

#include <qmath.h>

QT_BEGIN_NAMESPACE

class QFontEngineInfoPrivate
{
public:
    inline QFontEngineInfoPrivate()
        : pixelSize(0), weight(QFont::Normal), style(QFont::StyleNormal)
    {}

    QString family;
    qreal pixelSize;
    int weight;
    QFont::Style style;
    QList<QFontDatabase::WritingSystem> writingSystems;
};

/*!
    \class QFontEngineInfo
    \preliminary
    \brief The QFontEngineInfo class describes a specific font provided by a font engine plugin.
    \since 4.3
    \ingroup qws

    \tableofcontents

    QFontEngineInfo is used to describe a request of a font to a font engine plugin as well as to
    describe the actual fonts a plugin provides.

    \sa QAbstractFontEngine, QFontEnginePlugin
*/

/*!
   Constructs a new empty QFontEngineInfo.
*/
QFontEngineInfo::QFontEngineInfo()
{
    d = new QFontEngineInfoPrivate;
}

/*!
   Constructs a new QFontEngineInfo with the specified \a family.
   The resulting object represents a freely scalable font with normal
   weight and style.
*/
QFontEngineInfo::QFontEngineInfo(const QString &family)
{
    d = new QFontEngineInfoPrivate;
    d->family = family;
}

/*!
   Creates a new font engine info object with the same attributes as \a other.
*/
QFontEngineInfo::QFontEngineInfo(const QFontEngineInfo &other)
    : d(new QFontEngineInfoPrivate(*other.d))
{
}

/*!
   Assigns \a other to this font engine info object, and returns a reference
   to this.
*/
QFontEngineInfo &QFontEngineInfo::operator=(const QFontEngineInfo &other)
{
    *d = *other.d;
    return *this;
}

/*!
   Destroys this QFontEngineInfo object.
*/
QFontEngineInfo::~QFontEngineInfo()
{
    delete d;
}

/*!
   \property QFontEngineInfo::family
   the family name of the font
*/

void QFontEngineInfo::setFamily(const QString &family)
{
    d->family = family;
}

QString QFontEngineInfo::family() const
{
    return d->family;
}

/*!
   \property QFontEngineInfo::pixelSize
   the pixel size of the font

   A pixel size of 0 represents a freely scalable font.
*/

void QFontEngineInfo::setPixelSize(qreal size)
{
    d->pixelSize = size;
}

qreal QFontEngineInfo::pixelSize() const
{
    return d->pixelSize;
}

/*!
   \property QFontEngineInfo::weight
   the weight of the font

   The value should be from the \l{QFont::Weight} enumeration.
*/

void QFontEngineInfo::setWeight(int weight)
{
    d->weight = weight;
}

int QFontEngineInfo::weight() const
{
    return d->weight;
}

/*!
   \property QFontEngineInfo::style
   the style of the font
*/

void QFontEngineInfo::setStyle(QFont::Style style)
{
    d->style = style;
}

QFont::Style QFontEngineInfo::style() const
{
    return d->style;
}

/*!
   \property QFontEngineInfo::writingSystems
   the writing systems supported by the font

   An empty list means that any writing system is supported.
*/

QList<QFontDatabase::WritingSystem> QFontEngineInfo::writingSystems() const
{
    return d->writingSystems;
}

void QFontEngineInfo::setWritingSystems(const QList<QFontDatabase::WritingSystem> &writingSystems)
{
    d->writingSystems = writingSystems;
}

class QFontEnginePluginPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QFontEnginePlugin)

    QString foundry;
};

/*!
    \class QFontEnginePlugin
    \preliminary
    \brief The QFontEnginePlugin class is the base class for font engine factory plugins in Qt for Embedded Linux.
    \since 4.3
    \ingroup qws
    \ingroup plugins

    \tableofcontents

    QFontEnginePlugin is provided by font engine plugins to create
    instances of subclasses of QAbstractFontEngine.

    The member functions create() and availableFontEngines() must be
    implemented.

    \sa QAbstractFontEngine, QFontEngineInfo
*/

/*!
  Creates a font engine plugin that creates font engines with the
  specified \a foundry and \a parent.
*/
QFontEnginePlugin::QFontEnginePlugin(const QString &foundry, QObject *parent)
    : QObject(*new QFontEnginePluginPrivate, parent)
{
    Q_D(QFontEnginePlugin);
    d->foundry = foundry;
}

/*!
   Destroys this font engine plugin.
*/
QFontEnginePlugin::~QFontEnginePlugin()
{
}

/*!
   Returns a list of foundries the font engine plugin provides.
   The default implementation returns the foundry specified with the constructor.
*/
QStringList QFontEnginePlugin::keys() const
{
    Q_D(const QFontEnginePlugin);
    return QStringList(d->foundry);
}

/*!
    \fn QAbstractFontEngine *QFontEnginePlugin::create(const QFontEngineInfo &info)

    Implemented in subclasses to create a new font engine that provides a font that
    matches \a info.
*/

/*!
    \fn QList<QFontEngineInfo> QFontEnginePlugin::availableFontEngines() const

    Implemented in subclasses to return a list of QFontEngineInfo objects that represents all font
    engines the plugin can create.
*/

class QAbstractFontEnginePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QAbstractFontEngine)
public:
};

//The <classname> class is|provides|contains|specifies...
/*!
    \class QAbstractFontEngine
    \preliminary
    \brief The QAbstractFontEngine class is the base class for font engine plugins in Qt for Embedded Linux.
    \since 4.3
    \ingroup qws

    \tableofcontents

    QAbstractFontEngine is implemented by font engine plugins through QFontEnginePlugin.

    \sa QFontEnginePlugin, QFontEngineInfo
*/

/*!
   \enum QAbstractFontEngine::Capability

   This enum describes the capabilities of a font engine.

   \value CanRenderGlyphs_Gray The font engine can render individual glyphs into 8 bpp images.
   \value CanRenderGlyphs_Mono The font engine can render individual glyphs into 1 bpp images.
   \value CanRenderGlyphs The font engine can render individual glyphs into images.
   \value CanOutlineGlyphs The font engine can convert glyphs to painter paths.
*/

/*!
   \enum QAbstractFontEngine::FontProperty

   This enum describes the properties of a font provided by a font engine.

   \value Ascent The ascent of the font, specified as a 26.6 fixed point value.
   \value Descent The descent of the font, specified as a 26.6 fixed point value.
   \value Leading The leading of the font, specified as a 26.6 fixed point value.
   \value XHeight The 'x' height of the font, specified as a 26.6 fixed point value.
   \value AverageCharWidth The average character width of the font, specified as a 26.6 fixed point value.
   \value LineThickness The thickness of the underline and strikeout lines for the font, specified as a 26.6 fixed point value.
   \value UnderlinePosition The distance from the base line to the underline position for the font, specified as a 26.6 fixed point value.
   \value MaxCharWidth The width of the widest character in the font, specified as a 26.6 fixed point value.
   \value MinLeftBearing The minimum left bearing of the font, specified as a 26.6 fixed point value.
   \value MinRightBearing The maximum right bearing of the font, specified as a 26.6 fixed point value.
   \value GlyphCount The number of glyphs in the font, specified as an integer value.
   \value CacheGlyphsHint A boolean value specifying whether rendered glyphs should be cached by Qt.
   \value OutlineGlyphsHint A boolean value specifying whether the font engine prefers outline drawing over image rendering for uncached glyphs.
*/

/*!
   \enum QAbstractFontEngine::TextShapingFlag

   This enum describes flags controlling conversion of characters to glyphs and their metrics.

   \value RightToLeft The text is used in a right-to-left context.
   \value ReturnDesignMetrics Return font design metrics instead of pixel metrics.
*/

/*!
   \typedef QAbstractFontEngine::Fixed

   This type is \c int, interpreted as a 26.6 fixed point value.
*/

/*!
   \class QAbstractFontEngine::GlyphMetrics
   \brief QAbstractFontEngine::GlyphMetrics defines the metrics of a single glyph.
   \preliminary
   \since 4.3
*/

/*!
   \variable QAbstractFontEngine::GlyphMetrics::x

   The horizontal offset from the origin.
*/

/*!
   \fn QAbstractFontEngine::GlyphMetrics::GlyphMetrics()

   Constructs an empty glyph metrics object with all values
   set to zero.
*/

/*!
   \variable QAbstractFontEngine::GlyphMetrics::y

   The vertical offset from the origin (baseline).
*/

/*!
   \variable QAbstractFontEngine::GlyphMetrics::width

   The width of the glyph.
*/

/*!
   \variable QAbstractFontEngine::GlyphMetrics::height

   The height of the glyph.
*/

/*!
   \variable QAbstractFontEngine::GlyphMetrics::advance

   The advance of the glyph.
*/

/*!
   \class QAbstractFontEngine::FixedPoint
   \brief QAbstractFontEngine::FixedPoint defines a point in the place using 26.6 fixed point precision.
   \preliminary
   \since 4.3
*/

/*!
   \variable QAbstractFontEngine::FixedPoint::x

   The x coordinate of this point.
*/

/*!
   \variable QAbstractFontEngine::FixedPoint::y

   The y coordinate of this point.
*/

/*!
   Constructs a new QAbstractFontEngine with the given \a parent.
*/
QAbstractFontEngine::QAbstractFontEngine(QObject *parent)
    : QObject(*new QAbstractFontEnginePrivate, parent)
{
}

/*!
   Destroys this QAbstractFontEngine object.
*/
QAbstractFontEngine::~QAbstractFontEngine()
{
}

/*!
    \fn QAbstractFontEngine::Capabilities QAbstractFontEngine::capabilities() const

    Implemented in subclasses to specify the font engine's capabilities. The return value
    may be cached by the caller and is expected not to change during the lifetime of the
    font engine.
*/

/*!
    \fn QVariant QAbstractFontEngine::fontProperty(FontProperty property) const

    Implemented in subclasses to return the value of the font attribute \a property. The return
    value may be cached by the caller and is expected not to change during the lifetime of the font
    engine.
*/

/*!
    \fn bool QAbstractFontEngine::convertStringToGlyphIndices(const QChar *string, int length, uint *glyphs, int *numGlyphs, TextShapingFlags flags) const

    Implemented in subclasses to convert the characters specified by \a string and \a length to
    glyph indicies, using \a flags. The glyph indicies should be returned in the \a glyphs array
    provided by the caller. The maximum size of \a glyphs is specified by the value pointed to by \a
    numGlyphs. If successful, the subclass implementation sets the value pointed to by \a numGlyphs
    to the actual number of glyph indices generated, and returns true. Otherwise, e.g. if there is
    not enough space in the provided \a glyphs array, it should set \a numGlyphs to the number of
    glyphs needed for the conversion and return false.
*/

/*!
    \fn void QAbstractFontEngine::getGlyphAdvances(const uint *glyphs, int numGlyphs, Fixed *advances, TextShapingFlags flags) const

    Implemented in subclasses to retrieve the advances of the array specified by \a glyphs and \a
    numGlyphs, using \a flags. The result is returned in \a advances, which is allocated by the
    caller and contains \a numGlyphs elements.
*/

/*!
    \fn QAbstractFontEngine::GlyphMetrics QAbstractFontEngine::glyphMetrics(uint glyph) const

    Implemented in subclass to return the metrics for \a glyph.
*/

/*!
   Implemented in subclasses to render the specified \a glyph into a \a buffer with the given \a depth ,
   \a bytesPerLine and \a height.

   Returns true if rendering succeeded, false otherwise.
*/
bool QAbstractFontEngine::renderGlyph(uint glyph, int depth, int bytesPerLine, int height, uchar *buffer)
{
    Q_UNUSED(glyph)
    Q_UNUSED(depth)
    Q_UNUSED(bytesPerLine)
    Q_UNUSED(height)
    Q_UNUSED(buffer)
    qWarning("QAbstractFontEngine: renderGlyph is not implemented in font plugin!");
    return false;
}

/*!
   Implemented in subclasses to add the outline of the glyphs specified by \a glyphs and \a
   numGlyphs at the specified \a positions to the painter path \a path.
*/
void QAbstractFontEngine::addGlyphOutlinesToPath(uint *glyphs, int numGlyphs, FixedPoint *positions, QPainterPath *path)
{
    Q_UNUSED(glyphs)
    Q_UNUSED(numGlyphs)
    Q_UNUSED(positions)
    Q_UNUSED(path)
    qWarning("QAbstractFontEngine: addGlyphOutlinesToPath is not implemented in font plugin!");
}

/*
bool QAbstractFontEngine::supportsExtension(Extension extension) const
{
    Q_UNUSED(extension)
    return false;
}

QVariant QAbstractFontEngine::extension(Extension extension, const QVariant &argument)
{
    Q_UNUSED(argument)
    Q_UNUSED(extension)
    return QVariant();
}
*/

QProxyFontEngine::QProxyFontEngine(QAbstractFontEngine *customEngine, const QFontDef &def)
    : engine(customEngine)
{
    fontDef = def;
    engineCapabilities = engine->capabilities();
}

QProxyFontEngine::~QProxyFontEngine()
{
    delete engine;
}

bool QProxyFontEngine::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags) const
{
    if (*nglyphs < len) {
        *nglyphs = len;
        return false;
    }

    QVarLengthArray<uint> glyphIndicies(*nglyphs);
    if (!engine->convertStringToGlyphIndices(str, len, glyphIndicies.data(), nglyphs, QAbstractFontEngine::TextShapingFlags(int(flags))))
        return false;

    // ### use memcopy instead
    for (int i = 0; i < *nglyphs; ++i) {
        glyphs->glyphs[i] = glyphIndicies[i];
    }
    glyphs->numGlyphs = *nglyphs;

    recalcAdvances(glyphs, flags);
    return true;
}

void QProxyFontEngine::recalcAdvances(QGlyphLayout *glyphs, QTextEngine::ShaperFlags flags) const
{
    const int nglyphs = glyphs->numGlyphs;

    QVarLengthArray<QAbstractFontEngine::Fixed> advances(nglyphs);
    engine->getGlyphAdvances(glyphs->glyphs, nglyphs, advances.data(), QAbstractFontEngine::TextShapingFlags(int(flags)));


    // ### use memcopy instead
    for (int i = 0; i < nglyphs; ++i) {
        glyphs->advances_x[i] = QFixed::fromFixed(advances[i]);
        glyphs->advances_y[i] = 0;
    }
}


static QImage alphaMapFromPath(QFontEngine *fe, glyph_t glyph)
{
    glyph_metrics_t gm = fe->boundingBox(glyph);
    int glyph_x = qFloor(gm.x.toReal());
    int glyph_y = qFloor(gm.y.toReal());
    int glyph_width = qCeil((gm.x + gm.width).toReal()) -  glyph_x;
    int glyph_height = qCeil((gm.y + gm.height).toReal()) - glyph_y;

    if (glyph_width <= 0 || glyph_height <= 0)
        return QImage();
    QFixedPoint pt;
    pt.x = 0;
    pt.y = -glyph_y; // the baseline
    QPainterPath path;
    QImage im(glyph_width + qAbs(glyph_x) + 4, glyph_height, QImage::Format_ARGB32_Premultiplied);
    im.fill(Qt::transparent);
    QPainter p(&im);
    p.setRenderHint(QPainter::Antialiasing);
    fe->addGlyphsToPath(&glyph, &pt, 1, &path, 0);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.drawPath(path);
    p.end();

    QImage indexed(im.width(), im.height(), QImage::Format_Indexed8);
    QVector<QRgb> colors(256);
    for (int i=0; i<256; ++i)
        colors[i] = qRgba(0, 0, 0, i);
    indexed.setColorTable(colors);

    for (int y=0; y<im.height(); ++y) {
        uchar *dst = (uchar *) indexed.scanLine(y);
        uint *src = (uint *) im.scanLine(y);
        for (int x=0; x<im.width(); ++x)
            dst[x] = qAlpha(src[x]);
    }

    return indexed;
}


QImage QProxyFontEngine::alphaMapForGlyph(glyph_t glyph)
{
    if (!(engineCapabilities & QAbstractFontEngine::CanRenderGlyphs_Gray))
        return alphaMapFromPath(this, glyph);

    QAbstractFontEngine::GlyphMetrics metrics = engine->glyphMetrics(glyph);
    if (metrics.width <= 0 || metrics.height <= 0)
        return QImage();

    QImage img(metrics.width >> 6, metrics.height >> 6, QImage::Format_Indexed8);

    // ### we should have QImage::Format_GrayScale8
    static QVector<QRgb> colorMap;
    if (colorMap.isEmpty()) {
        colorMap.resize(256);
        for (int i=0; i<256; ++i)
            colorMap[i] = qRgba(0, 0, 0, i);
    }

    img.setColorTable(colorMap);

    engine->renderGlyph(glyph, /*depth*/8, img.bytesPerLine(), img.height(), img.bits());

    return img;
}

void QProxyFontEngine::addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int nglyphs, QPainterPath *path, QTextItem::RenderFlags flags)
{
    if (engineCapabilities & QAbstractFontEngine::CanOutlineGlyphs)
        engine->addGlyphOutlinesToPath(glyphs, nglyphs, reinterpret_cast<QAbstractFontEngine::FixedPoint *>(positions), path);
    else
        QFontEngine::addGlyphsToPath(glyphs, positions, nglyphs, path, flags);
}

glyph_metrics_t QProxyFontEngine::boundingBox(const QGlyphLayout &glyphs)
{
    if (glyphs.numGlyphs == 0)
        return glyph_metrics_t();

    QFixed w = 0;
    for (int i = 0; i < glyphs.numGlyphs; ++i)
        w += glyphs.effectiveAdvance(i);

    return glyph_metrics_t(0, -ascent(), w, ascent() + descent(), w, 0);
}

glyph_metrics_t QProxyFontEngine::boundingBox(glyph_t glyph)
{
    glyph_metrics_t m;

    QAbstractFontEngine::GlyphMetrics metrics = engine->glyphMetrics(glyph);
    m.x = QFixed::fromFixed(metrics.x);
    m.y = QFixed::fromFixed(metrics.y);
    m.width = QFixed::fromFixed(metrics.width);
    m.height = QFixed::fromFixed(metrics.height);
    m.xoff = QFixed::fromFixed(metrics.advance);

    return m;
}

QFixed QProxyFontEngine::ascent() const
{
    return QFixed::fromFixed(engine->fontProperty(QAbstractFontEngine::Ascent).toInt());
}

QFixed QProxyFontEngine::descent() const
{
    return QFixed::fromFixed(engine->fontProperty(QAbstractFontEngine::Descent).toInt());
}

QFixed QProxyFontEngine::leading() const
{
    return QFixed::fromFixed(engine->fontProperty(QAbstractFontEngine::Leading).toInt());
}

QFixed QProxyFontEngine::xHeight() const
{
    return QFixed::fromFixed(engine->fontProperty(QAbstractFontEngine::XHeight).toInt());
}

QFixed QProxyFontEngine::averageCharWidth() const
{
    return QFixed::fromFixed(engine->fontProperty(QAbstractFontEngine::AverageCharWidth).toInt());
}

QFixed QProxyFontEngine::lineThickness() const
{
    return QFixed::fromFixed(engine->fontProperty(QAbstractFontEngine::LineThickness).toInt());
}

QFixed QProxyFontEngine::underlinePosition() const
{
    return QFixed::fromFixed(engine->fontProperty(QAbstractFontEngine::UnderlinePosition).toInt());
}

qreal QProxyFontEngine::maxCharWidth() const
{
    return QFixed::fromFixed(engine->fontProperty(QAbstractFontEngine::MaxCharWidth).toInt()).toReal();
}

qreal QProxyFontEngine::minLeftBearing() const
{
    return QFixed::fromFixed(engine->fontProperty(QAbstractFontEngine::MinLeftBearing).toInt()).toReal();
}

qreal QProxyFontEngine::minRightBearing() const
{
    return QFixed::fromFixed(engine->fontProperty(QAbstractFontEngine::MinRightBearing).toInt()).toReal();
}

int QProxyFontEngine::glyphCount() const
{
    return engine->fontProperty(QAbstractFontEngine::GlyphCount).toInt();
}

bool QProxyFontEngine::canRender(const QChar *string, int len)
{
    QVarLengthArray<uint> glyphs(len);
    int numGlyphs = len;

    if (!engine->convertStringToGlyphIndices(string, len, glyphs.data(), &numGlyphs, /*flags*/0))
        return false;

    for (int i = 0; i < numGlyphs; ++i)
        if (!glyphs[i])
            return false;

    return true;
}

void QProxyFontEngine::draw(QPaintEngine *p, qreal _x, qreal _y, const QTextItemInt &si)
{
    QPaintEngineState *pState = p->state;
    QRasterPaintEngine *paintEngine = static_cast<QRasterPaintEngine*>(p);

    QTransform matrix = pState->transform();
    matrix.translate(_x, _y);
    QFixed x = QFixed::fromReal(matrix.dx());
    QFixed y = QFixed::fromReal(matrix.dy());

    QVarLengthArray<QFixedPoint> positions;
    QVarLengthArray<glyph_t> glyphs;
    getGlyphPositions(si.glyphs, matrix, si.flags, glyphs, positions);
    if (glyphs.size() == 0)
        return;

    for(int i = 0; i < glyphs.size(); i++) {
        QImage glyph = alphaMapForGlyph(glyphs[i]);
        if (glyph.isNull())
            continue;

        if (glyph.format() != QImage::Format_Indexed8
            && glyph.format() != QImage::Format_Mono)
            continue;

        QAbstractFontEngine::GlyphMetrics metrics = engine->glyphMetrics(glyphs[i]);

        int depth = glyph.format() == QImage::Format_Mono ? 1 : 8;
        paintEngine->alphaPenBlt(glyph.bits(), glyph.bytesPerLine(), depth,
                                 qRound(positions[i].x + QFixed::fromFixed(metrics.x)),
                                 qRound(positions[i].y + QFixed::fromFixed(metrics.y)),
                                 glyph.width(), glyph.height());
    }
}

/*
 * This is only called when we use the proxy fontengine directly (without sharing the rendered
 * glyphs). So we prefer outline rendering over rendering of unshared glyphs. That decision is
 * done in qfontdatabase_qws.cpp by looking at the ShareGlyphsHint and the pixel size of the font.
 */
bool QProxyFontEngine::drawAsOutline() const
{
    if (!(engineCapabilities & QAbstractFontEngine::CanOutlineGlyphs))
        return false;

    QVariant outlineHint = engine->fontProperty(QAbstractFontEngine::OutlineGlyphsHint);
    return !outlineHint.isValid() || outlineHint.toBool();
}

QT_END_NAMESPACE
