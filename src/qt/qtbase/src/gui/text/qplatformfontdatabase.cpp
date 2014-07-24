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

#include "qplatformfontdatabase.h"
#include <QtGui/private/qfontengine_p.h>
#include <QtGui/private/qfontengine_qpa_p.h>
#include <QtCore/QLibraryInfo>
#include <QtCore/QDir>

QT_BEGIN_NAMESPACE

void qt_registerFont(const QString &familyname, const QString &stylename,
                     const QString &foundryname, int weight,
                     QFont::Style style, int stretch, bool antialiased,
                     bool scalable, int pixelSize, bool fixedPitch,
                     const QSupportedWritingSystems &writingSystems, void *hanlde);

void qt_registerFontFamily(const QString &familyName);
void qt_registerAliasToFontFamily(const QString &familyName, const QString &alias);

/*!
    \fn void QPlatformFontDatabase::registerQPF2Font(const QByteArray &dataArray, void *handle)

    Registers the pre-rendered QPF2 font contained in the given \a dataArray.

    \sa registerFont()
*/
void QPlatformFontDatabase::registerQPF2Font(const QByteArray &dataArray, void *handle)
{
    if (dataArray.size() == 0)
        return;

    const uchar *data = reinterpret_cast<const uchar *>(dataArray.constData());
    if (QFontEngineQPA::verifyHeader(data, dataArray.size())) {
        QString fontName = QFontEngineQPA::extractHeaderField(data, QFontEngineQPA::Tag_FontName).toString();
        int pixelSize = QFontEngineQPA::extractHeaderField(data, QFontEngineQPA::Tag_PixelSize).toInt();
        QVariant weight = QFontEngineQPA::extractHeaderField(data, QFontEngineQPA::Tag_Weight);
        QVariant style = QFontEngineQPA::extractHeaderField(data, QFontEngineQPA::Tag_Style);
        QByteArray writingSystemBits = QFontEngineQPA::extractHeaderField(data, QFontEngineQPA::Tag_WritingSystems).toByteArray();

        if (!fontName.isEmpty() && pixelSize) {
            QFont::Weight fontWeight = QFont::Normal;
            if (weight.type() == QVariant::Int || weight.type() == QVariant::UInt)
                fontWeight = QFont::Weight(weight.toInt());

            QFont::Style fontStyle = static_cast<QFont::Style>(style.toInt());

            QSupportedWritingSystems writingSystems;
            for (int i = 0; i < writingSystemBits.count(); ++i) {
                uchar currentByte = writingSystemBits.at(i);
                for (int j = 0; j < 8; ++j) {
                    if (currentByte & 1)
                        writingSystems.setSupported(QFontDatabase::WritingSystem(i * 8 + j));
                    currentByte >>= 1;
                }
            }
            QFont::Stretch stretch = QFont::Unstretched;
            registerFont(fontName,QString(),QString(),fontWeight,fontStyle,stretch,true,false,pixelSize,false,writingSystems,handle);
        }
    } else {
        qDebug() << "header verification of QPF2 font failed. maybe it is corrupt?";
    }
}

/*!
    Registers a font with the given set of attributes describing the font's
    foundry, family name, style and stretch information, pixel size, and
    supported writing systems. Additional information about whether the font
    can be scaled and antialiased can also be provided.

    The foundry name and font family are described by \a foundryName and
    \a familyName. The font weight (light, normal, bold, etc.), style (normal,
    oblique, italic) and stretch information (condensed, expanded, unstretched,
    etc.) are specified by \a weight, \a style and \a stretch.

    Some fonts can be antialiased and scaled; \a scalable and \a antialiased
    can be set to true for fonts with these attributes. The intended pixel
    size of non-scalable fonts is specified by \a pixelSize; this value will be
    ignored for scalable fonts.

    The writing systems supported by the font are specified by the
    \a writingSystems argument.

    \sa registerQPF2Font(), registerFontFamily()
*/
void QPlatformFontDatabase::registerFont(const QString &familyname, const QString &stylename,
                                         const QString &foundryname, QFont::Weight weight,
                                         QFont::Style style, QFont::Stretch stretch, bool antialiased,
                                         bool scalable, int pixelSize, bool fixedPitch,
                                         const QSupportedWritingSystems &writingSystems, void *usrPtr)
{
    if (scalable)
        pixelSize = 0;

    qt_registerFont(familyname, stylename, foundryname, weight, style,
                    stretch, antialiased, scalable, pixelSize,
                    fixedPitch, writingSystems, usrPtr);
}

/*!
    Registers a font family with the font database. The font will be
    lazily populated by a callback to populateFamily() when the font
    database determines that the family needs population.

    \sa populateFamily(), registerFont()
*/
void QPlatformFontDatabase::registerFontFamily(const QString &familyName)
{
    qt_registerFontFamily(familyName);
}

class QWritingSystemsPrivate
{
public:
    QWritingSystemsPrivate()
        : ref(1)
        , vector(QFontDatabase::WritingSystemsCount,false)
    {
    }

    QWritingSystemsPrivate(const QWritingSystemsPrivate *other)
        : ref(1)
        , vector(other->vector)
    {
    }

    QAtomicInt ref;
    QVector<bool> vector;
};

/*!
    Constructs a new object to handle supported writing systems.
*/
QSupportedWritingSystems::QSupportedWritingSystems()
{
    d = new QWritingSystemsPrivate;
}

/*!
    Constructs a copy of the \a other writing systems object.
*/
QSupportedWritingSystems::QSupportedWritingSystems(const QSupportedWritingSystems &other)
{
    d = other.d;
    d->ref.ref();
}

/*!
    Constructs a copy of the \a other writing systems object.
*/
QSupportedWritingSystems &QSupportedWritingSystems::operator=(const QSupportedWritingSystems &other)
{
    if (d != other.d) {
        other.d->ref.ref();
        if (!d->ref.deref())
            delete d;
        d = other.d;
    }
    return *this;
}

/*!
    Destroys the supported writing systems object.
*/
QSupportedWritingSystems::~QSupportedWritingSystems()
{
    if (!d->ref.deref())
        delete d;
}

/*!
    \internal
*/
void QSupportedWritingSystems::detach()
{
    if (d->ref.load() != 1) {
        QWritingSystemsPrivate *newd = new QWritingSystemsPrivate(d);
        if (!d->ref.deref())
            delete d;
        d = newd;
    }
}

/*!
    Sets or clears support for the specified \a writingSystem based on the
    value given by \a support.
*/
void QSupportedWritingSystems::setSupported(QFontDatabase::WritingSystem writingSystem, bool support)
{
    detach();
    d->vector[writingSystem] = support;
}

/*!
    Returns \c true if the writing system specified by \a writingSystem is
    supported; otherwise returns \c false.
*/
bool QSupportedWritingSystems::supported(QFontDatabase::WritingSystem writingSystem) const
{
    return d->vector.at(writingSystem);
}

/*!
    \class QSupportedWritingSystems
    \brief The QSupportedWritingSystems class is used when registering fonts with the internal Qt
    fontdatabase
    \ingroup painting
    \inmodule QtGui

    Its to provide an easy to use interface for indicating what writing systems a specific font
    supports.

*/

/*!
    \internal
 */
QPlatformFontDatabase::~QPlatformFontDatabase()
{
}

/*!
  This function is called once at startup by Qt's internal font database.
  Reimplement this function in a subclass for a convenient place to initialize
  the internal font database.

  You may lazily populate the database by calling registerFontFamily() instead
  of registerFont(), in which case you'll get a callback to populateFamily()
  when the required family needs population. You then call registerFont() to
  finish population of the family.

  The default implementation looks in the fontDir() location and registers all
  QPF2 fonts.
*/
void QPlatformFontDatabase::populateFontDatabase()
{
    QString fontpath = fontDir();
    if(!QFile::exists(fontpath)) {
        qWarning("QFontDatabase: Cannot find font directory '%s' - is Qt installed correctly?",
                 qPrintable(QDir::toNativeSeparators(fontpath)));
        return;
    }

    QDir dir(fontpath);
    dir.setNameFilters(QStringList() << QLatin1String("*.qpf2"));
    dir.refresh();
    for (int i = 0; i < int(dir.count()); ++i) {
        const QByteArray fileName = QFile::encodeName(dir.absoluteFilePath(dir[i]));
        QFile file(QString::fromLocal8Bit(fileName));
        if (file.open(QFile::ReadOnly)) {
            const QByteArray fileData = file.readAll();
            QByteArray *fileDataPtr = new QByteArray(fileData);
            registerQPF2Font(fileData, fileDataPtr);
        }
    }
}

/*!
    This function is called whenever a lazily populated family, populated
    through registerFontFamily(), needs full population.

    You are expected to fully populate the family by calling registerFont()
    for each font that matches the family name.
*/
void QPlatformFontDatabase::populateFamily(const QString &familyName)
{
    Q_UNUSED(familyName);
}

/*!
    This function is called whenever the font database is invalidated.

    Reimplement this function to clear any internal data structures that
    will need to be rebuilt at the next call to populateFontDatabase().
*/
void QPlatformFontDatabase::invalidate()
{
}

/*!
    Returns a multi font engine in the specified \a script to encapsulate \a fontEngine with the
    option to fall back to the fonts given by \a fallbacks if \a fontEngine does not support
    a certain character.
*/
QFontEngineMulti *QPlatformFontDatabase::fontEngineMulti(QFontEngine *fontEngine, QChar::Script script)
{
    return new QFontEngineMultiQPA(fontEngine, script);
}

/*!
    Returns the font engine that can be used to render the font described by
    the font definition, \a fontDef, in the specified \a script.
*/
QFontEngine *QPlatformFontDatabase::fontEngine(const QFontDef &fontDef, void *handle)
{
    QByteArray *fileDataPtr = static_cast<QByteArray *>(handle);
    QFontEngineQPA *engine = new QFontEngineQPA(fontDef,*fileDataPtr);
    //qDebug() << fontDef.pixelSize << fontDef.weight << fontDef.style << fontDef.stretch << fontDef.styleHint << fontDef.styleStrategy << fontDef.family;
    return engine;
}

QFontEngine *QPlatformFontDatabase::fontEngine(const QByteArray &fontData, qreal pixelSize,
                                               QFont::HintingPreference hintingPreference)
{
    Q_UNUSED(fontData);
    Q_UNUSED(pixelSize);
    Q_UNUSED(hintingPreference);
    qWarning("This plugin does not support font engines created directly from font data");
    return 0;
}

/*!
    Returns a list of alternative fonts for the specified \a family and
    \a style and \a script using the \a styleHint given.
*/
QStringList QPlatformFontDatabase::fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const
{
    Q_UNUSED(family);
    Q_UNUSED(style);
    Q_UNUSED(styleHint);
    Q_UNUSED(script);
    return QStringList();
}

/*!
    Adds an application font described by the font contained supplied \a fontData
    or using the font contained in the file referenced by \a fileName. Returns
    a list of family names, or an empty list if the font could not be added.

    \note The default implementation of this function does not add an application
    font. Subclasses should reimplement this function to perform the necessary
    loading and registration of fonts.
*/
QStringList QPlatformFontDatabase::addApplicationFont(const QByteArray &fontData, const QString &fileName)
{
    Q_UNUSED(fontData);
    Q_UNUSED(fileName);

    qWarning("This plugin does not support application fonts");
    return QStringList();
}

/*!
    Releases the specified font \a handle.
*/
void QPlatformFontDatabase::releaseHandle(void *handle)
{
    QByteArray *fileDataPtr = static_cast<QByteArray *>(handle);
    delete fileDataPtr;
}

/*!
    Returns the directory containing the fonts used by the database.
*/
QString QPlatformFontDatabase::fontDir() const
{
    QString fontpath = QString::fromLocal8Bit(qgetenv("QT_QPA_FONTDIR"));
    if (fontpath.isEmpty()) {
        fontpath = QLibraryInfo::location(QLibraryInfo::LibrariesPath);
        fontpath += QLatin1String("/fonts");
    }

    return fontpath;
}

/*!
    Returns the default system font.

    \sa QGuiApplication::font()
    \since 5.0
*/

QFont QPlatformFontDatabase::defaultFont() const
{
    return QFont(QLatin1String("Helvetica"));
}

/*!
    Resolve alias to actual font family names.

    \since 5.0
 */

QString qt_resolveFontFamilyAlias(const QString &alias);

QString QPlatformFontDatabase::resolveFontFamilyAlias(const QString &family) const
{
    return qt_resolveFontFamilyAlias(family);
}

/*!
    Return true if all fonts are considered scalable when using this font database.
    Defaults to false.

    \since 5.0
 */

bool QPlatformFontDatabase::fontsAlwaysScalable() const
{
    return false;
}

/*!
    Return list of standard font sizes when using this font database.

    \since 5.0
 */

 QList<int> QPlatformFontDatabase::standardSizes() const
{
    QList<int> ret;
    static const unsigned short standard[] =
        { 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72, 0 };
    ret.reserve(int(sizeof(standard) / sizeof(standard[0])));
    const unsigned short *sizes = standard;
    while (*sizes) ret << *sizes++;
    return ret;
}


// ### copied to tools/makeqpf/qpf2.cpp

// see the Unicode subset bitfields in the MSDN docs
static const ushort requiredUnicodeBits[QFontDatabase::WritingSystemsCount][2] = {
    { 127, 127 }, // Any
    { 0, 127 },   // Latin
    { 7, 127 },   // Greek
    { 9, 127 },   // Cyrillic
    { 10, 127 },  // Armenian
    { 11, 127 },  // Hebrew
    { 13, 127 },  // Arabic
    { 71, 127 },  // Syriac
    { 72, 127 },  // Thaana
    { 15, 127 },  // Devanagari
    { 16, 127 },  // Bengali
    { 17, 127 },  // Gurmukhi
    { 18, 127 },  // Gujarati
    { 19, 127 },  // Oriya
    { 20, 127 },  // Tamil
    { 21, 127 },  // Telugu
    { 22, 127 },  // Kannada
    { 23, 127 },  // Malayalam
    { 73, 127 },  // Sinhala
    { 24, 127 },  // Thai
    { 25, 127 },  // Lao
    { 70, 127 },  // Tibetan
    { 74, 127 },  // Myanmar
    { 26, 127 },  // Georgian
    { 80, 127 },  // Khmer
    { 126, 127 }, // SimplifiedChinese
    { 126, 127 }, // TraditionalChinese
    { 126, 127 }, // Japanese
    { 56, 127 },  // Korean
    { 0, 127 },   // Vietnamese (same as latin1)
    { 126, 127 }, // Other
    { 78, 127 },  // Ogham
    { 79, 127 },  // Runic
    { 14, 127 },  // Nko
};

enum {
    Latin1CsbBit = 0,
    CentralEuropeCsbBit = 1,
    TurkishCsbBit = 4,
    BalticCsbBit = 7,
    CyrillicCsbBit = 2,
    GreekCsbBit = 3,
    HebrewCsbBit = 5,
    ArabicCsbBit = 6,
    VietnameseCsbBit = 8,
    SimplifiedChineseCsbBit = 18,
    TraditionalChineseCsbBit = 20,
    JapaneseCsbBit = 17,
    KoreanCsbBit = 19,
    KoreanJohabCsbBit = 21
};

/*!
    Helper function that determines the writing systems support by a given
    \a unicodeRange and \a codePageRange.

    \since 5.1
*/
QSupportedWritingSystems QPlatformFontDatabase::writingSystemsFromTrueTypeBits(quint32 unicodeRange[4], quint32 codePageRange[2])
{
    QSupportedWritingSystems writingSystems;

    bool hasScript = false;
    for (int i = 0; i < QFontDatabase::WritingSystemsCount; ++i) {
        int bit = requiredUnicodeBits[i][0];
        int index = bit/32;
        int flag = 1 << (bit&31);
        if (bit != 126 && (unicodeRange[index] & flag)) {
            bit = requiredUnicodeBits[i][1];
            index = bit/32;

            flag = 1 << (bit&31);
            if (bit == 127 || (unicodeRange[index] & flag)) {
                writingSystems.setSupported(QFontDatabase::WritingSystem(i));
                hasScript = true;
                // qDebug("font %s: index=%d, flag=%8x supports script %d", familyName.latin1(), index, flag, i);
            }
        }
    }
    if (!hasScript) {
        if (codePageRange[0] & ((1 << Latin1CsbBit) | (1 << CentralEuropeCsbBit) | (1 << TurkishCsbBit) | (1 << BalticCsbBit))) {
            writingSystems.setSupported(QFontDatabase::Latin);
            hasScript = true;
            //qDebug("font %s supports Latin", familyName.latin1());
        }
        if (codePageRange[0] & (1 << CyrillicCsbBit)) {
            writingSystems.setSupported(QFontDatabase::Cyrillic);
            hasScript = true;
            //qDebug("font %s supports Cyrillic", familyName.latin1());
        }
        if (codePageRange[0] & (1 << GreekCsbBit)) {
            writingSystems.setSupported(QFontDatabase::Greek);
            hasScript = true;
            //qDebug("font %s supports Greek", familyName.latin1());
        }
        if (codePageRange[0] & (1 << HebrewCsbBit)) {
            writingSystems.setSupported(QFontDatabase::Hebrew);
            hasScript = true;
            //qDebug("font %s supports Hebrew", familyName.latin1());
        }
        if (codePageRange[0] & (1 << ArabicCsbBit)) {
            writingSystems.setSupported(QFontDatabase::Arabic);
            hasScript = true;
            //qDebug("font %s supports Arabic", familyName.latin1());
        }
        if (codePageRange[0] & (1 << VietnameseCsbBit)) {
            writingSystems.setSupported(QFontDatabase::Vietnamese);
            hasScript = true;
            //qDebug("font %s supports Vietnamese", familyName.latin1());
        }
        if (codePageRange[0] & (1 << SimplifiedChineseCsbBit)) {
            writingSystems.setSupported(QFontDatabase::SimplifiedChinese);
            hasScript = true;
            //qDebug("font %s supports Simplified Chinese", familyName.latin1());
        }
        if (codePageRange[0] & (1 << TraditionalChineseCsbBit)) {
            writingSystems.setSupported(QFontDatabase::TraditionalChinese);
            hasScript = true;
            //qDebug("font %s supports Traditional Chinese", familyName.latin1());
        }
        if (codePageRange[0] & (1 << JapaneseCsbBit)) {
            writingSystems.setSupported(QFontDatabase::Japanese);
            hasScript = true;
            //qDebug("font %s supports Japanese", familyName.latin1());
        }
        if (codePageRange[0] & ((1 << KoreanCsbBit) | (1 << KoreanJohabCsbBit))) {
            writingSystems.setSupported(QFontDatabase::Korean);
            hasScript = true;
            //qDebug("font %s supports Korean", familyName.latin1());
        }
        if (!hasScript)
            writingSystems.setSupported(QFontDatabase::Symbol);
    }

    return writingSystems;
}

/*!
    Helper function that register the \a alias for the \a familyName.

    \since 5.2
*/

void QPlatformFontDatabase::registerAliasToFontFamily(const QString &familyName, const QString &alias)
{
    qt_registerAliasToFontFamily(familyName, alias);
}

/*!
    \class QPlatformFontDatabase
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa
    \ingroup painting

    \brief The QPlatformFontDatabase class makes it possible to customize how fonts
    are discovered and how they are rendered

    QPlatformFontDatabase is the superclass which is intended to let platform implementations use
    native font handling.

    Qt has its internal font database which it uses to discover available fonts on the
    user's system. To be able to populate this database subclass this class, and
    reimplement populateFontDatabase().

    Use the function registerFont() to populate the internal font database.

    Sometimes a specified font does not have the required glyphs; in such a case, the
    fallbackForFamily() function is called automatically to find alternative font
    families that can supply alternatives to the missing glyphs.

    \sa QSupportedWritingSystems
*/
QT_END_NAMESPACE
