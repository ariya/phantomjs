/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qglobal.h"

#if !defined(QT_NO_RAWFONT)

#include "qrawfont.h"
#include "qrawfont_p.h"
#include "qplatformfontdatabase.h"

#include <QtCore/qendian.h>

QT_BEGIN_NAMESPACE

/*!
   \class QRawFont
   \brief The QRawFont class provides access to a single physical instance of a font.
   \since 4.8
   \inmodule QtGui

   \ingroup text
   \ingroup shared
   \mainclass

   \note QRawFont is a low level class. For most purposes QFont is a more appropriate class.

   Most commonly, when presenting text in a user interface, the exact fonts used
   to render the characters is to some extent unknown. This can be the case for several
   reasons: For instance, the actual, physical fonts present on the target system could be
   unexpected to the developers, or the text could contain user selected styles, sizes or
   writing systems that are not supported by font chosen in the code.

   Therefore, Qt's QFont class really represents a query for fonts. When text is interpreted,
   Qt will do its best to match the text to the query, but depending on the support, different
   fonts can be used behind the scenes.

   For most use cases, this is both expected and necessary, as it minimizes the possibility of
   text in the user interface being undisplayable. In some cases, however, more direct control
   over the process might be useful. It is for these use cases the QRawFont class exists.

   A QRawFont object represents a single, physical instance of a given font in a given pixel size.
   I.e. in the typical case it represents a set of TrueType or OpenType font tables and uses a
   user specified pixel size to convert metrics into logical pixel units. It can be used in
   combination with the QGlyphRun class to draw specific glyph indexes at specific positions, and
   also have accessors to some relevant data in the physical font.

   QRawFont only provides support for the main font technologies: GDI and DirectWrite on Windows
   platforms, FreeType on Linux platforms and CoreText on Mac OS X. For other
   font back-ends, the APIs will be disabled.

   QRawFont can be constructed in a number of ways:
   \list
   \li It can be constructed by calling QTextLayout::glyphs() or QTextFragment::glyphs(). The
      returned QGlyphs objects will contain QRawFont objects which represent the actual fonts
      used to render each portion of the text.
   \li It can be constructed by passing a QFont object to QRawFont::fromFont(). The function
      will return a QRawFont object representing the font that will be selected as response to
      the QFont query and the selected writing system.
   \li It can be constructed by passing a file name or QByteArray directly to the QRawFont
      constructor, or by calling loadFromFile() or loadFromData(). In this case, the
      font will not be registered in QFontDatabase, and it will not be available as part of
      regular font selection.
   \endlist

   QRawFont is considered local to the thread in which it is constructed (either using a
   constructor, or by calling loadFromData() or loadFromFile()). The QRawFont cannot be moved to a
   different thread, but will have to be recreated in the thread in question.

   \note For the requirement of caching glyph indexes and font selections for static text to avoid
   reshaping and relayouting in the inner loop of an application, a better choice is the QStaticText
   class, since it optimizes the memory cost of the cache and also provides the possibility of paint
   engine specific caches for an additional speed-up.
*/

/*!
    \enum QRawFont::AntialiasingType

    This enum represents the different ways a glyph can be rasterized in the function
    alphaMapForGlyph().

    \value PixelAntialiasing Will rasterize by measuring the coverage of the shape on whole pixels.
           The returned image contains the alpha values of each pixel based on the coverage of
           the glyph shape.
    \value SubPixelAntialiasing Will rasterize by measuring the coverage of each subpixel,
           returning a separate alpha value for each of the red, green and blue components of
           each pixel.
*/

/*!
    \enum QRawFont::LayoutFlag
    \since 5.1

    This enum tells the function advancesForGlyphIndexes() how to calculate the advances.

    \value SeparateAdvances Will calculate the advance for each glyph separately.
    \value KernedAdvances Will apply kerning between adjacent glyphs. Note that OpenType GPOS based
           kerning is currently not supported.
    \value UseDesignMetrics Use design metrics instead of hinted metrics adjusted to the resolution
           of the paint device.
           Can be OR-ed with any of the options above.
*/

/*!
   Constructs an invalid QRawFont.
*/
QRawFont::QRawFont()
    : d(new QRawFontPrivate)
{
}

/*!
   Constructs a QRawFont representing the font contained in the file referenced
   by \a fileName for the size (in pixels) given by \a pixelSize, and using the
   hinting preference specified by \a hintingPreference.

   \note The referenced file must contain a TrueType or OpenType font.
*/
QRawFont::QRawFont(const QString &fileName,
                   qreal pixelSize,
                   QFont::HintingPreference hintingPreference)
    : d(new QRawFontPrivate)
{
    loadFromFile(fileName, pixelSize, hintingPreference);
}

/*!
   Constructs a QRawFont representing the font contained in the supplied
   \a fontData for the size (in pixels) given by \a pixelSize, and using the
   hinting preference specified by \a hintingPreference.

   \note The data must contain a TrueType or OpenType font.
*/
QRawFont::QRawFont(const QByteArray &fontData,
                   qreal pixelSize,
                   QFont::HintingPreference hintingPreference)
    : d(new QRawFontPrivate)
{
    loadFromData(fontData, pixelSize, hintingPreference);
}

/*!
   Creates a QRawFont which is a copy of \a other.
*/
QRawFont::QRawFont(const QRawFont &other)
{
    d = other.d;
}

/*!
   Destroys the QRawFont
*/
QRawFont::~QRawFont()
{
}

/*!
  Assigns \a other to this QRawFont.
*/
QRawFont &QRawFont::operator=(const QRawFont &other)
{
    d = other.d;
    return *this;
}

/*!
  \fn void QRawFont::swap(QRawFont &other)
  \since 5.0

  Swaps this raw font with \a other. This function is very fast and
  never fails.
*/

/*!
   Returns \c true if the QRawFont is valid and false otherwise.
*/
bool QRawFont::isValid() const
{
    return d->isValid();
}

/*!
   Replaces the current QRawFont with the contents of the file referenced
   by \a fileName for the size (in pixels) given by \a pixelSize, and using the
   hinting preference specified by \a hintingPreference.

   The file must reference a TrueType or OpenType font.

   \sa loadFromData()
*/
void QRawFont::loadFromFile(const QString &fileName,
                            qreal pixelSize,
                            QFont::HintingPreference hintingPreference)
{
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly))
        loadFromData(file.readAll(), pixelSize, hintingPreference);
}

/*!
   Replaces the current QRawFont with the font contained in the supplied
   \a fontData for the size (in pixels) given by \a pixelSize, and using the
   hinting preference specified by \a hintingPreference.

   The \a fontData must contain a TrueType or OpenType font.

   \sa loadFromFile()
*/
void QRawFont::loadFromData(const QByteArray &fontData,
                            qreal pixelSize,
                            QFont::HintingPreference hintingPreference)
{
    d.detach();
    d->cleanUp();
    d->hintingPreference = hintingPreference;
    d->thread = QThread::currentThread();
    d->platformLoadFromData(fontData, pixelSize, hintingPreference);
}

/*!
   This function returns a rasterized image of the glyph at the given
   \a glyphIndex in the underlying font, using the \a transform specified.
   If the QRawFont is not valid, this function will return an invalid QImage.

   If \a antialiasingType is set to QRawFont::SubPixelAntialiasing, then the resulting image will be
   in QImage::Format_RGB32 and the RGB values of each pixel will represent the subpixel opacities of
   the pixel in the rasterization of the glyph. Otherwise, the image will be in the format of
   QImage::Format_Indexed8 and each pixel will contain the opacity of the pixel in the
   rasterization.

   \sa pathForGlyph(), QPainter::drawGlyphRun()
*/
QImage QRawFont::alphaMapForGlyph(quint32 glyphIndex, AntialiasingType antialiasingType,
                                  const QTransform &transform) const
{
    if (!d->isValid())
        return QImage();

    if (antialiasingType == SubPixelAntialiasing)
        return d->fontEngine->alphaRGBMapForGlyph(glyphIndex, QFixed(), transform);

    return d->fontEngine->alphaMapForGlyph(glyphIndex, QFixed(), transform);
}

/*!
   This function returns the shape of the glyph at a given \a glyphIndex in the underlying font
   if the QRawFont is valid. Otherwise, it returns an empty QPainterPath.

   The returned glyph will always be unhinted.

   \sa alphaMapForGlyph(), QPainterPath::addText()
*/
QPainterPath QRawFont::pathForGlyph(quint32 glyphIndex) const
{
    if (!d->isValid())
        return QPainterPath();

    QFixedPoint position;
    QPainterPath path;
    d->fontEngine->addGlyphsToPath(&glyphIndex, &position, 1, &path, 0);
    return path;
}

/*!
   Returns \c true if this QRawFont is equal to \a other. Otherwise, returns \c false.
*/
bool QRawFont::operator==(const QRawFont &other) const
{
    return d->fontEngine == other.d->fontEngine;
}

/*!
    \fn bool QRawFont::operator!=(const QRawFont &other) const

    Returns \c true if this QRawFont is not equal to \a other. Otherwise, returns \c false.
*/

/*!
   Returns the ascent of this QRawFont in pixel units.

   \sa QFontMetricsF::ascent()
*/
qreal QRawFont::ascent() const
{
    return d->isValid() ? d->fontEngine->ascent().toReal() : 0.0;
}

/*!
   Returns the descent of this QRawFont in pixel units.

   \sa QFontMetricsF::descent()
*/
qreal QRawFont::descent() const
{
    return d->isValid() ? d->fontEngine->descent().toReal() : 0.0;
}

/*!
   Returns the xHeight of this QRawFont in pixel units.

   \sa QFontMetricsF::xHeight()
*/
qreal QRawFont::xHeight() const
{
    return d->isValid() ? d->fontEngine->xHeight().toReal() : 0.0;
}

/*!
   Returns the leading of this QRawFont in pixel units.

   \sa QFontMetricsF::leading()
*/
qreal QRawFont::leading() const
{
    return d->isValid() ? d->fontEngine->leading().toReal() : 0.0;
}

/*!
   Returns the average character width of this QRawFont in pixel units.

   \sa QFontMetricsF::averageCharWidth()
*/
qreal QRawFont::averageCharWidth() const
{
    return d->isValid() ? d->fontEngine->averageCharWidth().toReal() : 0.0;
}

/*!
   Returns the width of the widest character in the font.

   \sa QFontMetricsF::maxWidth()
*/
qreal QRawFont::maxCharWidth() const
{
    return d->isValid() ? d->fontEngine->maxCharWidth() : 0.0;
}

/*!
   Returns the pixel size set for this QRawFont. The pixel size affects how glyphs are
   rasterized, the size of glyphs returned by pathForGlyph(), and is used to convert
   internal metrics from design units to logical pixel units.

   \sa setPixelSize()
*/
qreal QRawFont::pixelSize() const
{
    return d->isValid() ? d->fontEngine->fontDef.pixelSize : 0.0;
}

/*!
   Returns the number of design units define the width and height of the em square
   for this QRawFont. This value is used together with the pixel size when converting design metrics
   to pixel units, as the internal metrics are specified in design units and the pixel size gives
   the size of 1 em in pixels.

   \sa pixelSize(), setPixelSize()
*/
qreal QRawFont::unitsPerEm() const
{
    return d->isValid() ? d->fontEngine->emSquareSize().toReal() : 0.0;
}

/*!
   Returns the thickness for drawing lines (underline, overline, etc.)
   along with text drawn in this font.
 */
qreal QRawFont::lineThickness() const
{
    return d->isValid() ? d->fontEngine->lineThickness().toReal() : 0.0;
}

/*!
   Returns the position from baseline for drawing underlines below the text
   rendered with this font.
 */
qreal QRawFont::underlinePosition() const
{
    return d->isValid() ? d->fontEngine->underlinePosition().toReal() : 0.0;
}

/*!
   Returns the family name of this QRawFont.
*/
QString QRawFont::familyName() const
{
    return d->isValid() ? d->fontEngine->fontDef.family : QString();
}

/*!
   Returns the style name of this QRawFont.

   \sa QFont::styleName()
*/
QString QRawFont::styleName() const
{
    return d->isValid() ? d->fontEngine->fontDef.styleName : QString();
}

/*!
   Returns the style of this QRawFont.

   \sa QFont::style()
*/
QFont::Style QRawFont::style() const
{
    return d->isValid() ? QFont::Style(d->fontEngine->fontDef.style) : QFont::StyleNormal;
}

/*!
   Returns the weight of this QRawFont.

   \sa QFont::weight()
*/
int QRawFont::weight() const
{
    return d->isValid() ? int(d->fontEngine->fontDef.weight) : -1;
}

/*!
   Converts the string of unicode points given by \a text to glyph indexes
   using the CMAP table in the underlying font, and returns a vector containing
   the result.

   Note that, in cases where there are other tables in the font that affect the
   shaping of the text, the returned glyph indexes will not correctly represent
   the rendering of the text. To get the correctly shaped text, you can use
   QTextLayout to lay out and shape the text, then call QTextLayout::glyphs()
   to get the set of glyph index list and QRawFont pairs.

   \sa advancesForGlyphIndexes(), glyphIndexesForChars(), QGlyphRun, QTextLayout::glyphRuns(), QTextFragment::glyphRuns()
*/
QVector<quint32> QRawFont::glyphIndexesForString(const QString &text) const
{
    QVector<quint32> glyphIndexes;
    if (!d->isValid() || text.isEmpty())
        return glyphIndexes;

    int numGlyphs = text.size();
    glyphIndexes.resize(numGlyphs);

    QGlyphLayout glyphs;
    glyphs.numGlyphs = numGlyphs;
    glyphs.glyphs = glyphIndexes.data();
    if (!d->fontEngine->stringToCMap(text.data(), text.size(), &glyphs, &numGlyphs, QFontEngine::GlyphIndicesOnly))
        Q_UNREACHABLE();

    glyphIndexes.resize(numGlyphs);
    return glyphIndexes;
}

/*!
   Converts a string of unicode points to glyph indexes using the CMAP table in the
   underlying font. The function works like glyphIndexesForString() except it take
   an array (\a chars), the results will be returned though \a glyphIndexes array
   and number of glyphs will be set in \a numGlyphs. The size of \a glyphIndexes array
   must be at least \a numChars, if that's still not enough, this function will return
   false, then you can resize \a glyphIndexes from the size returned in \a numGlyphs.

   \sa glyphIndexesForString(), advancesForGlyphIndexes(), QGlyphRun, QTextLayout::glyphRuns(), QTextFragment::glyphRuns()
*/
bool QRawFont::glyphIndexesForChars(const QChar *chars, int numChars, quint32 *glyphIndexes, int *numGlyphs) const
{
    Q_ASSERT(numGlyphs);
    if (!d->isValid() || numChars <= 0) {
        *numGlyphs = 0;
        return false;
    }

    if (*numGlyphs <= 0 || !glyphIndexes) {
        *numGlyphs = numChars;
        return false;
    }

    QGlyphLayout glyphs;
    glyphs.numGlyphs = *numGlyphs;
    glyphs.glyphs = glyphIndexes;
    return d->fontEngine->stringToCMap(chars, numChars, &glyphs, numGlyphs, QFontEngine::GlyphIndicesOnly);
}

/*!
   \fn QVector<QPointF> QRawFont::advancesForGlyphIndexes(const QVector<quint32> &glyphIndexes, LayoutFlags layoutFlags) const
   \since 5.1

   Returns the QRawFont's advances for each of the \a glyphIndexes in pixel units. The advances
   give the distance from the position of a given glyph to where the next glyph should be drawn
   to make it appear as if the two glyphs are unspaced. How the advances are calculated is
   controlled by \a layoutFlags.

   \sa QTextLine::horizontalAdvance(), QFontMetricsF::width()
*/

/*!
   \fn QVector<QPointF> QRawFont::advancesForGlyphIndexes(const QVector<quint32> &glyphIndexes) const

   \overload

   Returns the QRawFont's advances for each of the \a glyphIndexes in pixel units. The advances
   give the distance from the position of a given glyph to where the next glyph should be drawn
   to make it appear as if the two glyphs are unspaced. The advance of each glyph is calculated
   separately.

   \sa QTextLine::horizontalAdvance(), QFontMetricsF::width()
*/

/*!
   \since 5.1

   Returns the QRawFont's advances for each of the \a glyphIndexes in pixel units. The advances
   give the distance from the position of a given glyph to where the next glyph should be drawn
   to make it appear as if the two glyphs are unspaced. The glyph indexes are given with the
   array \a glyphIndexes while the results are returned through \a advances, both of them must
   have \a numGlyphs elements. How the advances are calculated is controlled by \a layoutFlags.

   \sa QTextLine::horizontalAdvance(), QFontMetricsF::width()
*/
bool QRawFont::advancesForGlyphIndexes(const quint32 *glyphIndexes, QPointF *advances, int numGlyphs, LayoutFlags layoutFlags) const
{
    Q_ASSERT(glyphIndexes && advances);
    if (!d->isValid() || numGlyphs <= 0)
        return false;

    QVarLengthArray<QFixed> tmpAdvances(numGlyphs);

    QGlyphLayout glyphs;
    glyphs.glyphs = const_cast<glyph_t *>(glyphIndexes);
    glyphs.numGlyphs = numGlyphs;
    glyphs.advances = tmpAdvances.data();

    bool design = layoutFlags & UseDesignMetrics;

    d->fontEngine->recalcAdvances(&glyphs, design ? QFontEngine::DesignMetrics : QFontEngine::ShaperFlag(0));
    if (layoutFlags & KernedAdvances)
        d->fontEngine->doKerning(&glyphs, design ? QFontEngine::DesignMetrics : QFontEngine::ShaperFlag(0));

    for (int i=0; i<numGlyphs; ++i)
        advances[i] = QPointF(tmpAdvances[i].toReal(), 0.0);

    return true;
}

/*!
   \overload

   Returns the QRawFont's advances for each of the \a glyphIndexes in pixel units. The advances
   give the distance from the position of a given glyph to where the next glyph should be drawn
   to make it appear as if the two glyphs are unspaced. The glyph indexes are given with the
   array \a glyphIndexes while the results are returned through \a advances, both of them must
   have \a numGlyphs elements. The advance of each glyph is calculated separately

   \sa QTextLine::horizontalAdvance(), QFontMetricsF::width()
*/
bool QRawFont::advancesForGlyphIndexes(const quint32 *glyphIndexes, QPointF *advances, int numGlyphs) const
{
    return QRawFont::advancesForGlyphIndexes(glyphIndexes, advances, numGlyphs, SeparateAdvances);
}

/*!
   Returns the hinting preference used to construct this QRawFont.

   \sa QFont::hintingPreference()
*/
QFont::HintingPreference QRawFont::hintingPreference() const
{
    return d->isValid() ? d->hintingPreference : QFont::PreferDefaultHinting;
}

/*!
   Retrieves the sfnt table named \a tagName from the underlying physical font, or an empty
   byte array if no such table was found. The returned font table's byte order is Big Endian, like
   the sfnt format specifies. The \a tagName must be four characters long and should be formatted
   in the default endianness of the current platform.
*/
QByteArray QRawFont::fontTable(const char *tagName) const
{
    if (!d->isValid())
        return QByteArray();

    const quint32 *tagId = reinterpret_cast<const quint32 *>(tagName);
    return d->fontEngine->getSfntTable(qToBigEndian(*tagId));
}

/*!
   Returns a list of writing systems supported by the font according to designer supplied
   information in the font file. Please note that this does not guarantee support for a
   specific unicode point in the font. You can use the supportsCharacter() to check support
   for a single, specific character.

   \note The list is determined based on the unicode ranges and codepage ranges set in the font's
   OS/2 table and requires such a table to be present in the underlying font file.

   \sa supportsCharacter()
*/
QList<QFontDatabase::WritingSystem> QRawFont::supportedWritingSystems() const
{
    QList<QFontDatabase::WritingSystem> writingSystems;
    if (d->isValid()) {
        QByteArray os2Table = fontTable("OS/2");
        if (os2Table.size() > 86) {
            char *data = os2Table.data();
            quint32 *bigEndianUnicodeRanges = reinterpret_cast<quint32 *>(data + 42);
            quint32 *bigEndianCodepageRanges = reinterpret_cast<quint32 *>(data + 78);

            quint32 unicodeRanges[4];
            quint32 codepageRanges[2];

            for (int i=0; i<4; ++i) {
                if (i < 2)
                    codepageRanges[i] = qFromBigEndian(bigEndianCodepageRanges[i]);
                unicodeRanges[i] = qFromBigEndian(bigEndianUnicodeRanges[i]);
            }

            QSupportedWritingSystems ws = QPlatformFontDatabase::writingSystemsFromTrueTypeBits(unicodeRanges, codepageRanges);
            for (int i = 0; i < QFontDatabase::WritingSystemsCount; ++i) {
                if (ws.supported(QFontDatabase::WritingSystem(i)))
                    writingSystems.append(QFontDatabase::WritingSystem(i));
            }
        }
    }

    return writingSystems;
}

/*!
    Returns \c true if the font has a glyph that corresponds to the given \a character.

    \sa supportedWritingSystems()
*/
bool QRawFont::supportsCharacter(QChar character) const
{
    return supportsCharacter(character.unicode());
}

/*!
    \overload

   Returns \c true if the font has a glyph that corresponds to the UCS-4 encoded character \a ucs4.

   \sa supportedWritingSystems()
*/
bool QRawFont::supportsCharacter(uint ucs4) const
{
    return d->isValid() && d->fontEngine->canRender(ucs4);
}

// qfontdatabase.cpp
extern int qt_script_for_writing_system(QFontDatabase::WritingSystem writingSystem);

/*!
   Fetches the physical representation based on a \a font query. The physical font returned is
   the font that will be preferred by Qt in order to display text in the selected \a writingSystem.

   \warning This function is potentially expensive and should not be called in performance
   sensitive code.
*/
QRawFont QRawFont::fromFont(const QFont &font, QFontDatabase::WritingSystem writingSystem)
{
    QRawFont rawFont;
    QFontPrivate *font_d = QFontPrivate::get(font);
    int script = qt_script_for_writing_system(writingSystem);
    QFontEngine *fe = font_d->engineForScript(script);

    if (fe != 0 && fe->type() == QFontEngine::Multi) {
        QFontEngineMulti *multiEngine = static_cast<QFontEngineMulti *>(fe);
        fe = multiEngine->engine(0);
        Q_ASSERT(fe);
    }

    if (fe != 0) {
        rawFont.d.data()->fontEngine = fe;
        rawFont.d.data()->fontEngine->ref.ref();
        rawFont.d.data()->hintingPreference = font.hintingPreference();
    }
    return rawFont;
}

/*!
   Sets the pixel size with which this font should be rendered to \a pixelSize.
*/
void QRawFont::setPixelSize(qreal pixelSize)
{
    if (d->fontEngine == 0 || qFuzzyCompare(d->fontEngine->fontDef.pixelSize, pixelSize))
        return;

    d.detach();
    QFontEngine *oldFontEngine = d->fontEngine;

    d->fontEngine = d->fontEngine->cloneWithSize(pixelSize);
    if (d->fontEngine != 0)
        d->fontEngine->ref.ref();

    if (!oldFontEngine->ref.deref())
        delete oldFontEngine;
}

/*!
    \internal
*/
void QRawFontPrivate::cleanUp()
{
    platformCleanUp();
    if (fontEngine != 0) {
        if (!fontEngine->ref.deref())
            delete fontEngine;
        fontEngine = 0;
    }
    hintingPreference = QFont::PreferDefaultHinting;
}

/*!
  Returns the smallest rectangle containing the glyph with the given \a glyphIndex.

  \since 5.0
*/
QRectF QRawFont::boundingRect(quint32 glyphIndex) const
{
    if (!isValid())
        return QRectF();

    glyph_metrics_t gm = d->fontEngine->boundingBox(glyphIndex);
    return QRectF(gm.x.toReal(), gm.y.toReal(), gm.width.toReal(), gm.height.toReal());
}

#endif // QT_NO_RAWFONT

QT_END_NAMESPACE
