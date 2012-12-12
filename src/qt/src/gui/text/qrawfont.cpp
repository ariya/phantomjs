/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include <QtCore/qendian.h>

QT_BEGIN_NAMESPACE

/*!
   \class QRawFont
   \brief The QRawFont class provides access to a single physical instance of a font.
   \since 4.8

   \ingroup text
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
   platforms, FreeType on Symbian and Linux platforms and CoreText on Mac OS X. For other
   font back-ends, the APIs will be disabled.

   QRawFont can be constructed in a number of ways:
   \list
   \o It can be constructed by calling QTextLayout::glyphs() or QTextFragment::glyphs(). The
      returned QGlyphs objects will contain QRawFont objects which represent the actual fonts
      used to render each portion of the text.
   \o It can be constructed by passing a QFont object to QRawFont::fromFont(). The function
      will return a QRawFont object representing the font that will be selected as response to
      the QFont query and the selected writing system.
   \o It can be constructed by passing a file name or QByteArray directly to the QRawFont
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
   Returns true if the QRawFont is valid and false otherwise.
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
        return d->fontEngine->alphaRGBMapForGlyph(glyphIndex, QFixed(), 0, transform);

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
   Returns true if this QRawFont is equal to \a other. Otherwise, returns false.
*/
bool QRawFont::operator==(const QRawFont &other) const
{
    return d->fontEngine == other.d->fontEngine;
}

/*!
    \fn bool QRawFont::operator!=(const QRawFont &other) const

    Returns true if this QRawFont is not equal to \a other. Otherwise, returns false.
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
    if (!d->isValid())
        return QVector<quint32>();

    int nglyphs = text.size();
    QVarLengthGlyphLayoutArray glyphs(nglyphs);
    if (!glyphIndexesForChars(text.data(), text.size(), glyphs.glyphs, &nglyphs)) {
        glyphs.resize(nglyphs);
        if (!glyphIndexesForChars(text.data(), text.size(), glyphs.glyphs, &nglyphs)) {
            Q_ASSERT_X(false, Q_FUNC_INFO, "stringToCMap shouldn't fail twice");
            return QVector<quint32>();
        }
    }

    QVector<quint32> glyphIndexes;
    for (int i=0; i<nglyphs; ++i)
        glyphIndexes.append(glyphs.glyphs[i]);

    return glyphIndexes;
}

/*!
   Converts a string of unicode points to glyph indexes using the CMAP table in the
   underlying font. The function works like glyphIndexesForString() except it take
   an array (\a chars), the results will be returned though \a glyphIndexes array
   and number of glyphs will be set in \a numGlyphs. The size of \a glyphIndexes array
   must be at least \a numChars, if that's still not enough, this function will return
   false, then you can resize \a glyphIndexes from the size returned in \a numGlyphs.

   \sa glyphIndexesForString(), advancesForGlyphIndexes(), QGlyphRun,
       QTextLayout::glyphRuns(), QTextFragment::glyphRuns()
*/
bool QRawFont::glyphIndexesForChars(const QChar *chars, int numChars, quint32 *glyphIndexes, int *numGlyphs) const
{
    if (!d->isValid())
        return false;

    QGlyphLayout glyphs;
    glyphs.glyphs = glyphIndexes;
    return d->fontEngine->stringToCMap(chars, numChars, &glyphs, numGlyphs, QTextEngine::GlyphIndicesOnly);
}

/*!
   Returns the QRawFont's advances for each of the \a glyphIndexes in pixel units. The advances
   give the distance from the position of a given glyph to where the next glyph should be drawn
   to make it appear as if the two glyphs are unspaced.

   \sa QTextLine::horizontalAdvance(), QFontMetricsF::width()
*/
QVector<QPointF> QRawFont::advancesForGlyphIndexes(const QVector<quint32> &glyphIndexes) const
{
    if (!d->isValid())
        return QVector<QPointF>();

    int numGlyphs = glyphIndexes.size();
    QVarLengthGlyphLayoutArray glyphs(numGlyphs);
    qMemCopy(glyphs.glyphs, glyphIndexes.data(), numGlyphs * sizeof(quint32));

    d->fontEngine->recalcAdvances(&glyphs, 0);

    QVector<QPointF> advances;
    for (int i=0; i<numGlyphs; ++i)
        advances.append(QPointF(glyphs.advances_x[i].toReal(), glyphs.advances_y[i].toReal()));

    return advances;
}

/*!
   Returns the QRawFont's advances for each of the \a glyphIndexes in pixel units. The advances
   give the distance from the position of a given glyph to where the next glyph should be drawn
   to make it appear as if the two glyphs are unspaced. The glyph indexes are given with the
   array \a glyphIndexes while the results are returned through \a advances, both of them must
   have \a numGlyphs elements.

   \sa QTextLine::horizontalAdvance(), QFontMetricsF::width()
*/
bool QRawFont::advancesForGlyphIndexes(const quint32 *glyphIndexes, QPointF *advances, int numGlyphs) const
{
    if (!d->isValid())
        return false;

    QGlyphLayout glyphs;
    glyphs.glyphs = const_cast<HB_Glyph *>(glyphIndexes);
    glyphs.numGlyphs = numGlyphs;
    QVarLengthArray<QFixed> advances_x(numGlyphs);
    QVarLengthArray<QFixed> advances_y(numGlyphs);
    glyphs.advances_x = advances_x.data();
    glyphs.advances_y = advances_y.data();

    d->fontEngine->recalcAdvances(&glyphs, 0);

    for (int i=0; i<numGlyphs; ++i)
        advances[i] = QPointF(glyphs.advances_x[i].toReal(), glyphs.advances_y[i].toReal());

    return true;
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

// From qfontdatabase.cpp
extern QList<QFontDatabase::WritingSystem> qt_determine_writing_systems_from_truetype_bits(quint32 unicodeRange[4], quint32 codePageRange[2]);

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

            return qt_determine_writing_systems_from_truetype_bits(unicodeRanges, codepageRanges);
        }
    }

    return QList<QFontDatabase::WritingSystem>();
}

/*!
    Returns true if the font has a glyph that corresponds to the given \a character.

    \sa supportedWritingSystems()
*/
bool QRawFont::supportsCharacter(QChar character) const
{
    return d->isValid() && d->fontEngine->canRender(&character, 1);
}

/*!
    \overload
   Returns true if the font has a glyph that corresponds to the UCS-4 encoded character \a ucs4.

   \sa supportedWritingSystems()
*/
bool QRawFont::supportsCharacter(quint32 ucs4) const
{
    QChar str[2];
    int len;
    if (!QChar::requiresSurrogates(ucs4)) {
        str[0] = QChar(ucs4);
        len = 1;
    } else {
        str[0] = QChar(QChar::highSurrogate(ucs4));
        str[1] = QChar(QChar::lowSurrogate(ucs4));
        len = 2;
    }

    return d->isValid() && d->fontEngine->canRender(str, len);
}

// qfontdatabase.cpp
extern int qt_script_for_writing_system(QFontDatabase::WritingSystem writingSystem);

/*!
   Fetches the physical representation based on a \a font query. The physical font returned is
   the font that will be preferred by Qt in order to display text in the selected \a writingSystem.
*/
QRawFont QRawFont::fromFont(const QFont &font, QFontDatabase::WritingSystem writingSystem)
{
    QRawFont rawFont;
#if defined(Q_WS_MAC)
    QTextLayout layout(QFontDatabase::writingSystemSample(writingSystem), font);
    layout.beginLayout();
    QTextLine line = layout.createLine();
    layout.endLayout();
    QList<QGlyphRun> list = layout.glyphRuns();
    if (list.size()) {
        // Pick the one matches the family name we originally requested,
        // if none of them match, just pick the first one
        for (int i = 0; i < list.size(); i++) {
            rawFont = list.at(i).rawFont();
            if (rawFont.familyName() == font.family())
                return rawFont;
        }
        return list.at(0).rawFont();
    }
#else
    QFontPrivate *font_d = QFontPrivate::get(font);
    int script = qt_script_for_writing_system(writingSystem);
    QFontEngine *fe = font_d->engineForScript(script);

    if (fe != 0 && fe->type() == QFontEngine::Multi) {
        QFontEngineMulti *multiEngine = static_cast<QFontEngineMulti *>(fe);
        fe = multiEngine->engine(0);
        if (fe == 0) {
            multiEngine->loadEngine(0);
            fe = multiEngine->engine(0);
        }
    }

    if (fe != 0) {
        rawFont.d.data()->fontEngine = fe;
        rawFont.d.data()->fontEngine->ref.ref();
        rawFont.d.data()->hintingPreference = font.hintingPreference();
    }
#endif
    return rawFont;
}

/*!
   Sets the pixel size with which this font should be rendered to \a pixelSize.
*/
void QRawFont::setPixelSize(qreal pixelSize)
{
    if (d->fontEngine == 0)
        return;

    d.detach();
    QFontEngine *oldFontEngine = d->fontEngine;

    d->fontEngine = d->fontEngine->cloneWithSize(pixelSize);
    if (d->fontEngine != 0)
        d->fontEngine->ref.ref();

    oldFontEngine->ref.deref();
    if (oldFontEngine->cache_count == 0 && oldFontEngine->ref == 0)
        delete oldFontEngine;
}

/*!
    \internal
*/
void QRawFontPrivate::cleanUp()
{
    platformCleanUp();
    if (fontEngine != 0) {
        fontEngine->ref.deref();
        if (fontEngine->cache_count == 0 && fontEngine->ref == 0)
            delete fontEngine;
        fontEngine = 0;
    }
    hintingPreference = QFont::PreferDefaultHinting;
}

#endif // QT_NO_RAWFONT

QT_END_NAMESPACE
