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

#include <qdir.h>
#include "qfontdatabase.h"
#include "qdebug.h"
#include "qalgorithms.h"
#include "qapplication.h"
#include "qvarlengtharray.h" // here or earlier - workaround for VC++6
#include "qthread.h"
#include "qmutex.h"
#include "private/qunicodetables_p.h"
#include "qfontengine_p.h"

#ifdef Q_WS_QPA
#include <QtGui/private/qapplication_p.h>
#include <QtGui/qplatformfontdatabase_qpa.h>
#include "qabstractfileengine.h"
#endif

#ifdef Q_WS_X11
#include <locale.h>
#endif
#include <stdlib.h>
#include <limits.h>

#if (defined(Q_WS_QWS)|| defined(Q_OS_SYMBIAN)) && !defined(QT_NO_FREETYPE)
#  include <ft2build.h>
#  include FT_TRUETYPE_TABLES_H
#endif

// #define QFONTDATABASE_DEBUG
#ifdef QFONTDATABASE_DEBUG
#  define FD_DEBUG qDebug
#else
#  define FD_DEBUG if (false) qDebug
#endif

// #define FONT_MATCH_DEBUG
#ifdef FONT_MATCH_DEBUG
#  define FM_DEBUG qDebug
#else
#  define FM_DEBUG if (false) qDebug
#endif

#if defined(Q_WS_WIN) && !defined(QT_NO_DIRECTWRITE)
#  include <dwrite.h>
#endif

QT_BEGIN_NAMESPACE

#define SMOOTH_SCALABLE 0xffff

bool qt_enable_test_font = false;

static QString styleStringHelper(int weight, QFont::Style style);

Q_AUTOTEST_EXPORT void qt_setQtEnableTestFont(bool value)
{
    qt_enable_test_font = value;
}

static int getFontWeight(const QString &weightString)
{
    QString s = weightString.toLower();

    // Test in decreasing order of commonness
    if (s == QLatin1String("medium") ||
        s == QLatin1String("normal")
        || s.compare(QApplication::translate("QFontDatabase", "Normal"), Qt::CaseInsensitive) == 0)
        return QFont::Normal;
    if (s == QLatin1String("bold")
        || s.compare(QApplication::translate("QFontDatabase", "Bold"), Qt::CaseInsensitive) == 0)
        return QFont::Bold;
    if (s == QLatin1String("demibold") || s == QLatin1String("demi bold")
        || s.compare(QApplication::translate("QFontDatabase", "Demi Bold"), Qt::CaseInsensitive) == 0)
        return QFont::DemiBold;
    if (s == QLatin1String("black")
        || s.compare(QApplication::translate("QFontDatabase", "Black"), Qt::CaseInsensitive) == 0)
        return QFont::Black;
    if (s == QLatin1String("light"))
        return QFont::Light;

    if (s.contains(QLatin1String("bold"))
        || s.contains(QApplication::translate("QFontDatabase", "Bold"), Qt::CaseInsensitive)) {
        if (s.contains(QLatin1String("demi"))
            || s.compare(QApplication::translate("QFontDatabase", "Demi"), Qt::CaseInsensitive) == 0)
            return (int) QFont::DemiBold;
        return (int) QFont::Bold;
    }

    if (s.contains(QLatin1String("light"))
        || s.compare(QApplication::translate("QFontDatabase", "Light"), Qt::CaseInsensitive) == 0)
        return (int) QFont::Light;

    if (s.contains(QLatin1String("black"))
        || s.compare(QApplication::translate("QFontDatabase", "Black"), Qt::CaseInsensitive) == 0)
        return (int) QFont::Black;

    return (int) QFont::Normal;
}

// convert 0 ~ 1000 integer to QFont::Weight
QFont::Weight weightFromInteger(int weight)
{
    if (weight < 400)
        return QFont::Light;
    else if (weight < 600)
        return QFont::Normal;
    else if (weight < 700)
        return QFont::DemiBold;
    else if (weight < 800)
        return QFont::Bold;
    else
        return QFont::Black;
}

struct QtFontEncoding
{
    signed int encoding : 16;

    uint xpoint   : 16;
    uint xres     : 8;
    uint yres     : 8;
    uint avgwidth : 16;
    uchar pitch   : 8;
};

struct  QtFontSize
{
#ifdef Q_WS_X11
    QtFontEncoding *encodings;
    QtFontEncoding *encodingID(int id, uint xpoint = 0, uint xres = 0,
                                uint yres = 0, uint avgwidth = 0, bool add = false);
    unsigned short count : 16;
#endif // Q_WS_X11

#if defined(Q_WS_QWS) || defined(Q_OS_SYMBIAN)
    QByteArray fileName;
    int fileIndex;
#endif // defined(Q_WS_QWS) || defined(Q_WS_QPA) || defined(Q_OS_SYMBIAN)
#if defined(Q_WS_QPA)
    void *handle;
#endif

    unsigned short pixelSize : 16;
};


#ifdef Q_WS_X11
QtFontEncoding *QtFontSize::encodingID(int id, uint xpoint, uint xres,
                                        uint yres, uint avgwidth, bool add)
{
    // we don't match using the xpoint, xres and yres parameters, only the id
    for (int i = 0; i < count; ++i) {
        if (encodings[i].encoding == id)
            return encodings + i;
    }

    if (!add) return 0;

    if (!(count % 4)) {
        QtFontEncoding *newEncodings = (QtFontEncoding *)
                    realloc(encodings,
                             (((count+4) >> 2) << 2) * sizeof(QtFontEncoding));
        Q_CHECK_PTR(newEncodings);
        encodings = newEncodings;
    }
    encodings[count].encoding = id;
    encodings[count].xpoint = xpoint;
    encodings[count].xres = xres;
    encodings[count].yres = yres;
    encodings[count].avgwidth = avgwidth;
    encodings[count].pitch = '*';
    return encodings + count++;
}
#endif // Q_WS_X11

struct QtFontStyle
{
    struct Key {
        Key(const QString &styleString);
        Key() : style(QFont::StyleNormal),
                weight(QFont::Normal), stretch(0) { }
        Key(const Key &o) : style(o.style), weight(o.weight), stretch(o.stretch) { }
        uint style : 2;
        signed int  weight : 8;
        signed int stretch : 12;

        bool operator==(const Key & other) {
            return (style == other.style && weight == other.weight &&
                    (stretch == 0 || other.stretch == 0 || stretch == other.stretch));
        }
        bool operator!=(const Key &other) {
            return !operator==(other);
        }
        bool operator <(const Key &o) {
            int x = (style << 12) + (weight << 14) + stretch;
            int y = (o.style << 12) + (o.weight << 14) + o.stretch;
            return (x < y);
        }
    };

    QtFontStyle(const Key &k)
        : key(k), bitmapScalable(false), smoothScalable(false),
          count(0), pixelSizes(0)
    {
#if defined(Q_WS_X11)
        weightName = setwidthName = 0;
#endif // Q_WS_X11
    }

    ~QtFontStyle() {
#ifdef Q_WS_X11
        delete [] weightName;
        delete [] setwidthName;
#endif
#if defined(Q_WS_X11) || defined(Q_WS_QWS) || defined(Q_WS_QPA) || defined(Q_OS_SYMBIAN)
        while (count) {
            // bitfield count-- in while condition does not work correctly in mwccsym2
            count--;
#ifdef Q_WS_X11
            free(pixelSizes[count].encodings);
#endif
#if defined(Q_WS_QWS) || defined(Q_OS_SYMBIAN)
            pixelSizes[count].fileName.~QByteArray();
#endif
#if defined (Q_WS_QPA)
            QPlatformIntegration *integration = QApplicationPrivate::platformIntegration();
            if (integration) { //on shut down there will be some that we don't release.
                integration->fontDatabase()->releaseHandle(pixelSizes[count].handle);
            }
#endif
        }
#endif
        free(pixelSizes);
    }

    Key key;
    bool bitmapScalable : 1;
    bool smoothScalable : 1;
    signed int count    : 30;
    QtFontSize *pixelSizes;
    QString styleName;

#ifdef Q_WS_X11
    const char *weightName;
    const char *setwidthName;
#endif // Q_WS_X11
#if defined(Q_WS_QWS) || defined(Q_WS_QPA) || defined(Q_OS_SYMBIAN)
    bool antialiased;
#endif

    QtFontSize *pixelSize(unsigned short size, bool = false);
};

QtFontStyle::Key::Key(const QString &styleString)
    : style(QFont::StyleNormal), weight(QFont::Normal), stretch(0)
{
    weight = getFontWeight(styleString);

    if (styleString.contains(QLatin1String("Italic"))
        || styleString.contains(QApplication::translate("QFontDatabase", "Italic")))
        style = QFont::StyleItalic;
    else if (styleString.contains(QLatin1String("Oblique"))
             || styleString.contains(QApplication::translate("QFontDatabase", "Oblique")))
        style = QFont::StyleOblique;
}

QtFontSize *QtFontStyle::pixelSize(unsigned short size, bool add)
{
    for (int i = 0; i < count; i++) {
        if (pixelSizes[i].pixelSize == size)
            return pixelSizes + i;
    }
    if (!add)
        return 0;

    if (!pixelSizes) {
        // Most style have only one font size, we avoid waisting memory
        QtFontSize *newPixelSizes = (QtFontSize *)malloc(sizeof(QtFontSize));
        Q_CHECK_PTR(newPixelSizes);
        pixelSizes = newPixelSizes;
    } else if (!(count % 8) || count == 1) {
        QtFontSize *newPixelSizes = (QtFontSize *)
                     realloc(pixelSizes,
                              (((count+8) >> 3) << 3) * sizeof(QtFontSize));
        Q_CHECK_PTR(newPixelSizes);
        pixelSizes = newPixelSizes;
    }
    pixelSizes[count].pixelSize = size;
#ifdef Q_WS_X11
    pixelSizes[count].count = 0;
    pixelSizes[count].encodings = 0;
#endif
#if defined(Q_WS_QWS) || defined(Q_OS_SYMBIAN)
    new (&pixelSizes[count].fileName) QByteArray;
    pixelSizes[count].fileIndex = 0;
#endif
#if defined(Q_WS_QPA)
    pixelSizes[count].handle = 0;
#endif
    return pixelSizes + (count++);
}

struct QtFontFoundry
{
    QtFontFoundry(const QString &n) : name(n), count(0), styles(0) {}
    ~QtFontFoundry() {
        while (count--)
            delete styles[count];
        free(styles);
    }

    QString name;

    int count;
    QtFontStyle **styles;
    QtFontStyle *style(const QtFontStyle::Key &, const QString & = QString(), bool = false);
};

QtFontStyle *QtFontFoundry::style(const QtFontStyle::Key &key, const QString &styleName, bool create)
{
    int pos = 0;
    for (; pos < count; pos++) {
        bool hasStyleName = !styleName.isEmpty(); // search styleName first if available
        if (hasStyleName && !styles[pos]->styleName.isEmpty()) {
            if (styles[pos]->styleName == styleName)
                return styles[pos];
        } else {
            if (styles[pos]->key == key)
                return styles[pos];
        }
    }
    if (!create)
        return 0;

    // qDebug("adding key (weight=%d, style=%d, stretch=%d) at %d", key.weight, key.style, key.stretch, pos);
    if (!(count % 8)) {
        QtFontStyle **newStyles = (QtFontStyle **)
                 realloc(styles, (((count+8) >> 3) << 3) * sizeof(QtFontStyle *));
        Q_CHECK_PTR(newStyles);
        styles = newStyles;
    }

    QtFontStyle *style = new QtFontStyle(key);
    style->styleName = styleName;
    styles[pos] = style;
    count++;
    return styles[pos];
}

struct  QtFontFamily
{
    enum WritingSystemStatus {
        Unknown         = 0,
        Supported       = 1,
        UnsupportedFT  = 2,
        UnsupportedXLFD = 4,
        Unsupported     = UnsupportedFT | UnsupportedXLFD
    };

    QtFontFamily(const QString &n)
        :
#ifdef Q_WS_X11
        fixedPitch(true), ftWritingSystemCheck(false),
        xlfdLoaded(false), synthetic(false), symbol_checked(false),
#else
        fixedPitch(false),
#endif
#ifdef Q_WS_WIN
        writingSystemCheck(false),
        loaded(false),
#endif
#if !defined(QWS) && defined(Q_OS_MAC)
        fixedPitchComputed(false),
#endif
        name(n), count(0), foundries(0)
#if defined(Q_WS_QWS) || defined(Q_WS_QPA) || defined(Q_OS_SYMBIAN) && !defined(QT_NO_FREETYPE)
        , bogusWritingSystems(false)
#endif
#if defined(Q_WS_QPA)
        , askedForFallback(false)
#endif
    {
        memset(writingSystems, 0, sizeof(writingSystems));
    }
    ~QtFontFamily() {
        while (count--)
            delete foundries[count];
        free(foundries);
    }

    bool fixedPitch : 1;
#ifdef Q_WS_X11
    bool ftWritingSystemCheck : 1;
    bool xlfdLoaded : 1;
    bool synthetic : 1;
#endif
#ifdef Q_WS_WIN
    bool writingSystemCheck : 1;
    bool loaded : 1;
#endif
#if !defined(QWS) && defined(Q_OS_MAC)
    bool fixedPitchComputed : 1;
#endif
#ifdef Q_WS_X11
    bool symbol_checked : 1;
#endif

    QString name;
#if defined(Q_WS_X11) ||   defined(Q_OS_SYMBIAN) && !defined(QT_NO_FREETYPE)
    QByteArray fontFilename;
    int fontFileIndex;
#endif
#ifdef Q_WS_WIN
    QString english_name;
#endif
    int count;
    QtFontFoundry **foundries;

#if defined(Q_WS_QWS) || defined(Q_WS_QPA) || defined(Q_OS_SYMBIAN) && !defined(QT_NO_FREETYPE)
    bool bogusWritingSystems;
    QStringList fallbackFamilies;
#endif
#if defined (Q_WS_QPA)
    bool askedForFallback;
#endif
    unsigned char writingSystems[QFontDatabase::WritingSystemsCount];

    QtFontFoundry *foundry(const QString &f, bool = false);
};

#if !defined(QWS) && defined(Q_OS_MAC)
inline static void qt_mac_get_fixed_pitch(QtFontFamily *f)
{
    if(f && !f->fixedPitchComputed) {
        QFontMetrics fm(f->name);
        f->fixedPitch = fm.width(QLatin1Char('i')) == fm.width(QLatin1Char('m'));
        f->fixedPitchComputed = true;
    }
}
#endif


QtFontFoundry *QtFontFamily::foundry(const QString &f, bool create)
{
    if (f.isNull() && count == 1)
        return foundries[0];

    for (int i = 0; i < count; i++) {
        if (foundries[i]->name.compare(f, Qt::CaseInsensitive) == 0)
            return foundries[i];
    }
    if (!create)
        return 0;

    if (!(count % 8)) {
        QtFontFoundry **newFoundries = (QtFontFoundry **)
                    realloc(foundries,
                             (((count+8) >> 3) << 3) * sizeof(QtFontFoundry *));
        Q_CHECK_PTR(newFoundries);
        foundries = newFoundries;
    }

    foundries[count] = new QtFontFoundry(f);
    return foundries[count++];
}

// ### copied to tools/makeqpf/qpf2.cpp

// see the Unicode subset bitfields in the MSDN docs
static int requiredUnicodeBits[QFontDatabase::WritingSystemsCount][2] = {
        // Any,
    { 127, 127 },
        // Latin,
    { 0, 127 },
        // Greek,
    { 7, 127 },
        // Cyrillic,
    { 9, 127 },
        // Armenian,
    { 10, 127 },
        // Hebrew,
    { 11, 127 },
        // Arabic,
    { 13, 127 },
        // Syriac,
    { 71, 127 },
    //Thaana,
    { 72, 127 },
    //Devanagari,
    { 15, 127 },
    //Bengali,
    { 16, 127 },
    //Gurmukhi,
    { 17, 127 },
    //Gujarati,
    { 18, 127 },
    //Oriya,
    { 19, 127 },
    //Tamil,
    { 20, 127 },
    //Telugu,
    { 21, 127 },
    //Kannada,
    { 22, 127 },
    //Malayalam,
    { 23, 127 },
    //Sinhala,
    { 73, 127 },
    //Thai,
    { 24, 127 },
    //Lao,
    { 25, 127 },
    //Tibetan,
    { 70, 127 },
    //Myanmar,
    { 74, 127 },
        // Georgian,
    { 26, 127 },
        // Khmer,
    { 80, 127 },
        // SimplifiedChinese,
    { 126, 127 },
        // TraditionalChinese,
    { 126, 127 },
        // Japanese,
    { 126, 127 },
        // Korean,
    { 56, 127 },
        // Vietnamese,
    { 0, 127 }, // same as latin1
        // Other,
    { 126, 127 },
        // Ogham,
    { 78, 127 },
        // Runic,
    { 79, 127 },
        // Nko,
    { 14, 127 },
};

#define SimplifiedChineseCsbBit 18
#define TraditionalChineseCsbBit 20
#define JapaneseCsbBit 17
#define KoreanCsbBit 21

QList<QFontDatabase::WritingSystem> qt_determine_writing_systems_from_truetype_bits(quint32 unicodeRange[4], quint32 codePageRange[2])
{
    QList<QFontDatabase::WritingSystem> writingSystems;
    bool hasScript = false;

    int i;
    for(i = 0; i < QFontDatabase::WritingSystemsCount; i++) {
        int bit = requiredUnicodeBits[i][0];
        int index = bit/32;
        int flag =  1 << (bit&31);
        if (bit != 126 && unicodeRange[index] & flag) {
            bit = requiredUnicodeBits[i][1];
            index = bit/32;

            flag =  1 << (bit&31);
            if (bit == 127 || unicodeRange[index] & flag) {
                writingSystems.append(QFontDatabase::WritingSystem(i));
                hasScript = true;
                // qDebug("font %s: index=%d, flag=%8x supports script %d", familyName.latin1(), index, flag, i);
            }
        }
    }
    if(codePageRange[0] & (1 << SimplifiedChineseCsbBit)) {
        writingSystems.append(QFontDatabase::SimplifiedChinese);
        hasScript = true;
        //qDebug("font %s supports Simplified Chinese", familyName.latin1());
    }
    if(codePageRange[0] & (1 << TraditionalChineseCsbBit)) {
        writingSystems.append(QFontDatabase::TraditionalChinese);
        hasScript = true;
        //qDebug("font %s supports Traditional Chinese", familyName.latin1());
    }
    if(codePageRange[0] & (1 << JapaneseCsbBit)) {
        writingSystems.append(QFontDatabase::Japanese);
        hasScript = true;
        //qDebug("font %s supports Japanese", familyName.latin1());
    }
    if(codePageRange[0] & (1 << KoreanCsbBit)) {
        writingSystems.append(QFontDatabase::Korean);
        hasScript = true;
        //qDebug("font %s supports Korean", familyName.latin1());
    }
    if (!hasScript)
        writingSystems.append(QFontDatabase::Symbol);

    return writingSystems;
}

#if defined(Q_OS_SYMBIAN) && defined(QT_NO_FREETYPE)
// class with virtual destructor, derived in qfontdatabase_s60.cpp
class QSymbianFontDatabaseExtras
{
public:
    virtual ~QSymbianFontDatabaseExtras() {}
};
#endif

class QFontDatabasePrivate
{
public:
    QFontDatabasePrivate()
        : count(0), families(0), reregisterAppFonts(false)
#if defined(Q_WS_QWS)
          , stream(0)
#endif
#if defined(Q_OS_SYMBIAN) && defined(QT_NO_FREETYPE)
          , symbianExtras(0)
#endif
#if defined(Q_WS_WIN) && !defined(QT_NO_DIRECTWRITE)
          , directWriteFactory(0)
          , directWriteGdiInterop(0)
#endif
    { }

    ~QFontDatabasePrivate() {
        free();
#if defined(Q_OS_SYMBIAN) && defined(QT_NO_FREETYPE)
        if (symbianExtras)
            delete symbianExtras;
#endif
#if defined(Q_WS_WIN) && !defined(QT_NO_DIRECTWRITE)
    if (directWriteGdiInterop)
        directWriteGdiInterop->Release();
    if (directWriteFactory != 0)
        directWriteFactory->Release();
#endif
    }
    QtFontFamily *family(const QString &f, bool = false);
    void free() {
        while (count--)
            delete families[count];
        ::free(families);
        families = 0;
        count = 0;
        // don't clear the memory fonts!
    }

    int count;
#if defined(Q_WS_X11) && !defined(QT_NO_FONTCONFIG)
    QString systemLang;
#endif
    QtFontFamily **families;

#if defined(Q_WS_WIN) && !defined(QT_NO_DIRECTWRITE)
    IDWriteFactory *directWriteFactory;
    IDWriteGdiInterop *directWriteGdiInterop;
#endif


    struct ApplicationFont {
        QString fileName;
        QByteArray data;
#if defined(Q_OS_WIN)
        HANDLE handle;
        bool memoryFont;
        QVector<FONTSIGNATURE> signatures;
#elif defined(Q_WS_MAC)
        ATSFontContainerRef handle;
#elif defined(Q_OS_SYMBIAN) && defined(QT_NO_FREETYPE)
        QString temporaryFileName;
        TInt screenDeviceFontFileId;
        TUid fontStoreFontFileUid;
#endif
        QStringList families;
    };
    QVector<ApplicationFont> applicationFonts;
    int addAppFont(const QByteArray &fontData, const QString &fileName);
    bool reregisterAppFonts;
    bool isApplicationFont(const QString &fileName);

    void invalidate();

#if defined(Q_WS_QWS)
    bool loadFromCache(const QString &fontPath);
    void addQPF2File(const QByteArray &file);
#endif // Q_WS_QWS
#if defined(Q_WS_QWS) || defined(Q_OS_SYMBIAN) && !defined(QT_NO_FREETYPE)
    void addFont(const QString &familyname, const char *foundryname, int weight,
                 bool italic, int pixelSize, const QByteArray &file, int fileIndex,
                 bool antialiased,
                 const QList<QFontDatabase::WritingSystem> &writingSystems = QList<QFontDatabase::WritingSystem>());
#ifndef QT_NO_FREETYPE
    QStringList addTTFile(const QByteArray &file, const QByteArray &fontData = QByteArray());
#endif // QT_NO_FREETYPE
#endif
#if defined(Q_WS_QWS)
    QDataStream *stream;
#elif defined(Q_OS_SYMBIAN) && defined(QT_NO_FREETYPE)
    QSymbianFontDatabaseExtras *symbianExtras;
#endif
#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
    QStringList fallbackFamilies;
#endif
};

void QFontDatabasePrivate::invalidate()
{
    QFontCache::instance()->clear();
    free();
    emit static_cast<QApplication *>(QApplication::instance())->fontDatabaseChanged();
}

QtFontFamily *QFontDatabasePrivate::family(const QString &f, bool create)
{
    int low = 0;
    int high = count;
    int pos = count / 2;
    int res = 1;
    if (count) {
        while ((res = families[pos]->name.compare(f, Qt::CaseInsensitive)) && pos != low) {
            if (res > 0)
                high = pos;
            else
                low = pos;
            pos = (high + low) / 2;
        }
        if (!res)
            return families[pos];
    }
    if (!create)
        return 0;

    if (res < 0)
        pos++;

    // qDebug("adding family %s at %d total=%d", f.latin1(), pos, count);
    if (!(count % 8)) {
        QtFontFamily **newFamilies = (QtFontFamily **)
                   realloc(families,
                            (((count+8) >> 3) << 3) * sizeof(QtFontFamily *));
        Q_CHECK_PTR(newFamilies);
        families = newFamilies;
    }

    QtFontFamily *family = new QtFontFamily(f);
    memmove(families + pos + 1, families + pos, (count-pos)*sizeof(QtFontFamily *));
    families[pos] = family;
    count++;
    return families[pos];
}

#if defined(Q_WS_QWS) ||  defined(Q_OS_SYMBIAN) && !defined(QT_NO_FREETYPE)
void QFontDatabasePrivate::addFont(const QString &familyname, const char *foundryname, int weight, bool italic, int pixelSize,
                                   const QByteArray &file, int fileIndex, bool antialiased,
                                   const QList<QFontDatabase::WritingSystem> &writingSystems)
{
//    qDebug() << "Adding font" << familyname << weight << italic << pixelSize << file << fileIndex << antialiased;
    QtFontStyle::Key styleKey;
    styleKey.style = italic ? QFont::StyleItalic : QFont::StyleNormal;
    styleKey.weight = weight;
    styleKey.stretch = 100;
    QtFontFamily *f = family(familyname, true);

    if (writingSystems.isEmpty()) {
        for (int ws = 1; ws < QFontDatabase::WritingSystemsCount; ++ws) {
            f->writingSystems[ws] = QtFontFamily::Supported;
        }
        f->bogusWritingSystems = true;
    } else {
        for (int i = 0; i < writingSystems.count(); ++i) {
            f->writingSystems[writingSystems.at(i)] = QtFontFamily::Supported;
        }
    }

    QtFontFoundry *foundry = f->foundry(QString::fromLatin1(foundryname), true);
    QtFontStyle *style = foundry->style(styleKey, QString(), true);
    style->smoothScalable = (pixelSize == 0);
    style->antialiased = antialiased;
    QtFontSize *size = style->pixelSize(pixelSize?pixelSize:SMOOTH_SCALABLE, true);
    size->fileName = file;
    size->fileIndex = fileIndex;

#if defined(Q_WS_QWS)
    if (stream) {
        *stream << familyname << foundry->name << weight << quint8(italic) << pixelSize
                << file << fileIndex << quint8(antialiased);
        *stream << quint8(writingSystems.count());
        for (int i = 0; i < writingSystems.count(); ++i)
            *stream << quint8(writingSystems.at(i));
    }
#else // ..in case of defined(Q_OS_SYMBIAN) && !defined(QT_NO_FREETYPE)
    f->fontFilename = file;
    f->fontFileIndex = fileIndex;
#endif
}
#endif

#if (defined(Q_WS_QWS) || defined(Q_OS_SYMBIAN)) && !defined(QT_NO_FREETYPE)
QStringList QFontDatabasePrivate::addTTFile(const QByteArray &file, const QByteArray &fontData)
{
    QStringList families;
    extern FT_Library qt_getFreetype();
    FT_Library library = qt_getFreetype();

    int index = 0;
    int numFaces = 0;
    do {
        FT_Face face;
        FT_Error error;
        if (!fontData.isEmpty()) {
            error = FT_New_Memory_Face(library, (const FT_Byte *)fontData.constData(), fontData.size(), index, &face);
        } else {
            error = FT_New_Face(library, file, index, &face);
        }
        if (error != FT_Err_Ok) {
            qDebug() << "FT_New_Face failed with index" << index << ":" << hex << error;
            break;
        }
        numFaces = face->num_faces;

        int weight = QFont::Normal;
        bool italic = face->style_flags & FT_STYLE_FLAG_ITALIC;

        if (face->style_flags & FT_STYLE_FLAG_BOLD)
            weight = QFont::Bold;

        QList<QFontDatabase::WritingSystem> writingSystems;
        // detect symbol fonts
        for (int i = 0; i < face->num_charmaps; ++i) {
            FT_CharMap cm = face->charmaps[i];
            if (cm->encoding == ft_encoding_adobe_custom
                    || cm->encoding == ft_encoding_symbol) {
                writingSystems.append(QFontDatabase::Symbol);
                break;
            }
        }
        if (writingSystems.isEmpty()) {
            TT_OS2 *os2 = (TT_OS2 *)FT_Get_Sfnt_Table(face, ft_sfnt_os2);
            if (os2) {
                quint32 unicodeRange[4] = {
                    static_cast<quint32>(os2->ulUnicodeRange1), static_cast<quint32>(os2->ulUnicodeRange2), static_cast<quint32>(os2->ulUnicodeRange3), static_cast<quint32>(os2->ulUnicodeRange4)
                };
                quint32 codePageRange[2] = {
                    static_cast<quint32>(os2->ulCodePageRange1), static_cast<quint32>(os2->ulCodePageRange2)
                };

                writingSystems = qt_determine_writing_systems_from_truetype_bits(unicodeRange, codePageRange);
                //for (int i = 0; i < writingSystems.count(); ++i)
                //    qDebug() << QFontDatabase::writingSystemName(writingSystems.at(i));
            }
        }

        QString family = QString::fromAscii(face->family_name);
        families.append(family);
        addFont(family, /*foundry*/ "", weight, italic,
                /*pixelsize*/ 0, file, index, /*antialias*/ true, writingSystems);

        FT_Done_Face(face);
        ++index;
    } while (index < numFaces);
    return families;
}
#endif

static const int scriptForWritingSystem[] = {
    QUnicodeTables::Common, // Any
    QUnicodeTables::Latin, // Latin
    QUnicodeTables::Greek, // Greek
    QUnicodeTables::Cyrillic, // Cyrillic
    QUnicodeTables::Armenian, // Armenian
    QUnicodeTables::Hebrew, // Hebrew
    QUnicodeTables::Arabic, // Arabic
    QUnicodeTables::Syriac, // Syriac
    QUnicodeTables::Thaana, // Thaana
    QUnicodeTables::Devanagari, // Devanagari
    QUnicodeTables::Bengali, // Bengali
    QUnicodeTables::Gurmukhi, // Gurmukhi
    QUnicodeTables::Gujarati, // Gujarati
    QUnicodeTables::Oriya, // Oriya
    QUnicodeTables::Tamil, // Tamil
    QUnicodeTables::Telugu, // Telugu
    QUnicodeTables::Kannada, // Kannada
    QUnicodeTables::Malayalam, // Malayalam
    QUnicodeTables::Sinhala, // Sinhala
    QUnicodeTables::Thai, // Thai
    QUnicodeTables::Lao, // Lao
    QUnicodeTables::Tibetan, // Tibetan
    QUnicodeTables::Myanmar, // Myanmar
    QUnicodeTables::Georgian, // Georgian
    QUnicodeTables::Khmer, // Khmer
    QUnicodeTables::Common, // SimplifiedChinese
    QUnicodeTables::Common, // TraditionalChinese
    QUnicodeTables::Common, // Japanese
    QUnicodeTables::Hangul, // Korean
    QUnicodeTables::Common, // Vietnamese
    QUnicodeTables::Common, // Yi
    QUnicodeTables::Common, // Tagalog
    QUnicodeTables::Common, // Hanunoo
    QUnicodeTables::Common, // Buhid
    QUnicodeTables::Common, // Tagbanwa
    QUnicodeTables::Common, // Limbu
    QUnicodeTables::Common, // TaiLe
    QUnicodeTables::Common, // Braille
    QUnicodeTables::Common, // Symbol
    QUnicodeTables::Ogham,  // Ogham
    QUnicodeTables::Runic, // Runic
    QUnicodeTables::Nko // Nko
};

int qt_script_for_writing_system(QFontDatabase::WritingSystem writingSystem)
{
    return scriptForWritingSystem[writingSystem];
}


#if defined Q_WS_QWS || (defined(Q_WS_X11) && !defined(QT_NO_FONTCONFIG)) || defined(Q_WS_WIN)
static inline bool requiresOpenType(int writingSystem)
{
    return ((writingSystem >= QFontDatabase::Syriac && writingSystem <= QFontDatabase::Sinhala)
            || writingSystem == QFontDatabase::Khmer || writingSystem == QFontDatabase::Nko);
}
static inline bool scriptRequiresOpenType(int script)
{
    return ((script >= QUnicodeTables::Syriac && script <= QUnicodeTables::Sinhala)
            || script == QUnicodeTables::Khmer || script == QUnicodeTables::Nko);
}
#endif


/*!
  \internal

  This makes sense of the font family name:

  if the family name contains a '[' and a ']', then we take the text
  between the square brackets as the foundry, and the text before the
  square brackets as the family (ie. "Arial [Monotype]")
*/
static void parseFontName(const QString &name, QString &foundry, QString &family)
{
    int i = name.indexOf(QLatin1Char('['));
    int li = name.lastIndexOf(QLatin1Char(']'));
    if (i >= 0 && li >= 0 && i < li) {
        foundry = name.mid(i + 1, li - i - 1);
        if (i > 0 && name[i - 1] == QLatin1Char(' '))
            i--;
        family = name.left(i);
    } else {
        foundry.clear();
        family = name;
    }

    // capitalize the family/foundry names
    bool space = true;
    QChar *s = family.data();
    int len = family.length();
    while(len--) {
        if (space) *s = s->toUpper();
        space = s->isSpace();
        ++s;
    }

    space = true;
    s = foundry.data();
    len = foundry.length();
    while(len--) {
        if (space) *s = s->toUpper();
        space = s->isSpace();
        ++s;
    }
}


struct QtFontDesc
{
    inline QtFontDesc() : family(0), foundry(0), style(0), size(0), encoding(0), familyIndex(-1) {}
    QtFontFamily *family;
    QtFontFoundry *foundry;
    QtFontStyle *style;
    QtFontSize *size;
    QtFontEncoding *encoding;
    int familyIndex;
};

#if !defined(Q_WS_MAC)
static void match(int script, const QFontDef &request,
                  const QString &family_name, const QString &foundry_name, int force_encoding_id,
                  QtFontDesc *desc, const QList<int> &blacklistedFamilies = QList<int>(), bool forceXLFD=false);

#if defined(Q_WS_X11) || defined(Q_WS_QWS) || defined(Q_WS_QPA)
static void initFontDef(const QtFontDesc &desc, const QFontDef &request, QFontDef *fontDef)
{
    fontDef->family = desc.family->name;
    if (! desc.foundry->name.isEmpty() && desc.family->count > 1) {
        fontDef->family += QString::fromLatin1(" [");
        fontDef->family += desc.foundry->name;
        fontDef->family += QLatin1Char(']');
    }

    if (desc.style->smoothScalable)
        fontDef->pixelSize = request.pixelSize;
    else if ((desc.style->bitmapScalable && (request.styleStrategy & QFont::PreferMatch)))
        fontDef->pixelSize = request.pixelSize;
    else
        fontDef->pixelSize = desc.size->pixelSize;

    fontDef->styleHint     = request.styleHint;
    fontDef->styleStrategy = request.styleStrategy;

    fontDef->weight        = desc.style->key.weight;
    fontDef->style         = desc.style->key.style;
    fontDef->fixedPitch    = desc.family->fixedPitch;
    fontDef->stretch       = desc.style->key.stretch;
    fontDef->ignorePitch   = false;
}
#endif
#endif

#if defined(Q_WS_X11) || defined(Q_WS_WIN) || defined(Q_OS_SYMBIAN) || defined(Q_WS_QPA)
static void getEngineData(const QFontPrivate *d, const QFontCache::Key &key)
{
    // look for the requested font in the engine data cache
    d->engineData = QFontCache::instance()->findEngineData(key);
    if (!d->engineData) {
        // create a new one
        d->engineData = new QFontEngineData;
        QFontCache::instance()->insertEngineData(key, d->engineData);
    } else {
        d->engineData->ref.ref();
    }
}
#endif

static QStringList familyList(const QFontDef &req)
{
    // list of families to try
    QStringList family_list;
    if (req.family.isEmpty())
        return family_list;

    QStringList list = req.family.split(QLatin1Char(','));
    for (int i = 0; i < list.size(); ++i) {
        QString str = list.at(i).trimmed();
        if ((str.startsWith(QLatin1Char('"')) && str.endsWith(QLatin1Char('"')))
            || (str.startsWith(QLatin1Char('\'')) && str.endsWith(QLatin1Char('\''))))
            str = str.mid(1, str.length() - 2);
        family_list << str;
    }

    // append the substitute list for each family in family_list
    QStringList subs_list;
    QStringList::ConstIterator it = family_list.constBegin(), end = family_list.constEnd();
    for (; it != end; ++it)
        subs_list += QFont::substitutes(*it);
//         qDebug() << "adding substs: " << subs_list;

    family_list += subs_list;

    return family_list;
}

Q_GLOBAL_STATIC(QFontDatabasePrivate, privateDb)
Q_GLOBAL_STATIC_WITH_ARGS(QMutex, fontDatabaseMutex, (QMutex::Recursive))

// used in qfontengine_x11.cpp
QMutex *qt_fontdatabase_mutex()
{
    return fontDatabaseMutex();
}

QT_BEGIN_INCLUDE_NAMESPACE
#if defined(Q_WS_X11)
#  include "qfontdatabase_x11.cpp"
#elif defined(Q_WS_MAC)
#  include "qfontdatabase_mac.cpp"
#elif defined(Q_WS_WIN)
#  include "qfontdatabase_win.cpp"
#elif defined(Q_WS_QWS)
#  include "qfontdatabase_qws.cpp"
#elif defined(Q_WS_QPA)
#  include "qfontdatabase_qpa.cpp"
#elif defined(Q_OS_SYMBIAN)
#  include "qfontdatabase_s60.cpp"
#endif
QT_END_INCLUDE_NAMESPACE

#if !defined(Q_WS_X11) && !defined(Q_WS_MAC)
QString QFontDatabase::resolveFontFamilyAlias(const QString &family)
{
    return family;
}
#endif

static QtFontStyle *bestStyle(QtFontFoundry *foundry, const QtFontStyle::Key &styleKey,
                              const QString &styleName = QString())
{
    int best = 0;
    int dist = 0xffff;

    for ( int i = 0; i < foundry->count; i++ ) {
        QtFontStyle *style = foundry->styles[i];

        if (!styleName.isEmpty() && styleName == style->styleName) {
            dist = 0;
            best = i;
            break;
        }

        int d = qAbs( styleKey.weight - style->key.weight );

        if ( styleKey.stretch != 0 && style->key.stretch != 0 ) {
            d += qAbs( styleKey.stretch - style->key.stretch );
        }

        if (styleKey.style != style->key.style) {
            if (styleKey.style != QFont::StyleNormal && style->key.style != QFont::StyleNormal)
                // one is italic, the other oblique
                d += 0x0001;
            else
                d += 0x1000;
        }

        if ( d < dist ) {
            best = i;
            dist = d;
        }
    }

    FM_DEBUG( "          best style has distance 0x%x", dist );
    return foundry->styles[best];
}

#if defined(Q_WS_X11)
static QtFontEncoding *findEncoding(int script, int styleStrategy,
                                    QtFontSize *size, int force_encoding_id)
{
    QtFontEncoding *encoding = 0;

    if (force_encoding_id >= 0) {
        encoding = size->encodingID(force_encoding_id);
        if (!encoding)
            FM_DEBUG("            required encoding_id not available");
        return encoding;
    }

    if (styleStrategy & (QFont::OpenGLCompatible | QFont::PreferBitmap)) {
        FM_DEBUG("            PreferBitmap and/or OpenGL set, skipping Freetype");
    } else {
        encoding = size->encodingID(-1); // -1 == prefer Freetype
        if (encoding)
            return encoding;
    }

    // FT not available, find an XLFD font, trying the default encoding first
    encoding = size->encodingID(QFontPrivate::defaultEncodingID);
    if (encoding) {
        // does it support the requested script?
        bool supportsScript = false;
        for (int ws = 1; !supportsScript && ws < QFontDatabase::WritingSystemsCount; ++ws) {
            if (scriptForWritingSystem[ws] != script)
                continue;
            supportsScript = writingSystems_for_xlfd_encoding[encoding->encoding][ws];
        }
        if (!supportsScript)
            encoding = 0;
    }
    // find the first encoding that supports the requested script
    for (int ws = 1; !encoding && ws < QFontDatabase::WritingSystemsCount; ++ws) {
        if (scriptForWritingSystem[ws] != script)
            continue;
        for (int x = 0; !encoding && x < size->count; ++x) {
            const int enc = size->encodings[x].encoding;
            if (writingSystems_for_xlfd_encoding[enc][ws])
                encoding = size->encodings + x;
        }
    }

    return encoding;
}
#endif // Q_WS_X11

#if !defined(Q_WS_MAC)
static
unsigned int bestFoundry(int script, unsigned int score, int styleStrategy,
                         const QtFontFamily *family, const QString &foundry_name,
                         QtFontStyle::Key styleKey, int pixelSize, char pitch,
                         QtFontDesc *desc, int force_encoding_id)
{
    Q_UNUSED(force_encoding_id);
    Q_UNUSED(script);
    Q_UNUSED(pitch);

    desc->foundry = 0;
    desc->style = 0;
    desc->size = 0;
    desc->encoding = 0;


    FM_DEBUG("  REMARK: looking for best foundry for family '%s' [%d]", family->name.toLatin1().constData(), family->count);

    for (int x = 0; x < family->count; ++x) {
        QtFontFoundry *foundry = family->foundries[x];
        if (!foundry_name.isEmpty() && foundry->name.compare(foundry_name, Qt::CaseInsensitive) != 0)
            continue;

        FM_DEBUG("          looking for matching style in foundry '%s' %d",
                 foundry->name.isEmpty() ? "-- none --" : foundry->name.toLatin1().constData(), foundry->count);

        QtFontStyle *style = bestStyle(foundry, styleKey);

        if (!style->smoothScalable && (styleStrategy & QFont::ForceOutline)) {
            FM_DEBUG("            ForceOutline set, but not smoothly scalable");
            continue;
        }

        int px = -1;
        QtFontSize *size = 0;

        // 1. see if we have an exact matching size
        if (!(styleStrategy & QFont::ForceOutline)) {
            size = style->pixelSize(pixelSize);
            if (size) {
                FM_DEBUG("          found exact size match (%d pixels)", size->pixelSize);
                px = size->pixelSize;
            }
        }

        // 2. see if we have a smoothly scalable font
        if (!size && style->smoothScalable && ! (styleStrategy & QFont::PreferBitmap)) {
            size = style->pixelSize(SMOOTH_SCALABLE);
            if (size) {
                FM_DEBUG("          found smoothly scalable font (%d pixels)", pixelSize);
                px = pixelSize;
            }
        }

        // 3. see if we have a bitmap scalable font
        if (!size && style->bitmapScalable && (styleStrategy & QFont::PreferMatch)) {
            size = style->pixelSize(0);
            if (size) {
                FM_DEBUG("          found bitmap scalable font (%d pixels)", pixelSize);
                px = pixelSize;
            }
        }

#ifdef Q_WS_X11
        QtFontEncoding *encoding = 0;
#endif

        // 4. find closest size match
        if (! size) {
            unsigned int distance = ~0u;
            for (int x = 0; x < style->count; ++x) {
#ifdef Q_WS_X11
                encoding =
                    findEncoding(script, styleStrategy, style->pixelSizes + x, force_encoding_id);
                if (!encoding) {
                    FM_DEBUG("          size %3d does not support the script we want",
                             style->pixelSizes[x].pixelSize);
                    continue;
                }
#endif

                unsigned int d;
                if (style->pixelSizes[x].pixelSize < pixelSize) {
                    // penalize sizes that are smaller than the
                    // requested size, due to truncation from floating
                    // point to integer conversions
                    d = pixelSize - style->pixelSizes[x].pixelSize + 1;
                } else {
                    d = style->pixelSizes[x].pixelSize - pixelSize;
                }

                if (d < distance) {
                    distance = d;
                    size = style->pixelSizes + x;
                    FM_DEBUG("          best size so far: %3d (%d)", size->pixelSize, pixelSize);
                }
            }

            if (!size) {
                FM_DEBUG("          no size supports the script we want");
                continue;
            }

            if (style->bitmapScalable && ! (styleStrategy & QFont::PreferQuality) &&
                (distance * 10 / pixelSize) >= 2) {
                // the closest size is not close enough, go ahead and
                // use a bitmap scaled font
                size = style->pixelSize(0);
                px = pixelSize;
            } else {
                px = size->pixelSize;
            }
        }

#ifdef Q_WS_X11
        if (size) {
            encoding = findEncoding(script, styleStrategy, size, force_encoding_id);
            if (!encoding) size = 0;
        }
        if (! encoding) {
            FM_DEBUG("          foundry doesn't support the script we want");
            continue;
        }
#endif // Q_WS_X11

        unsigned int this_score = 0x0000;
        enum {
            PitchMismatch       = 0x4000,
            StyleMismatch       = 0x2000,
            BitmapScaledPenalty = 0x1000,
            EncodingMismatch    = 0x0002,
            XLFDPenalty         = 0x0001
        };
#ifdef Q_WS_X11
        if (encoding->encoding != -1) {
            this_score += XLFDPenalty;
            if (encoding->encoding != QFontPrivate::defaultEncodingID)
                this_score += EncodingMismatch;
        }
        if (pitch != '*') {
            if (!(pitch == 'm' && encoding->pitch == 'c') && pitch != encoding->pitch)
                this_score += PitchMismatch;
        }
#else
        if (pitch != '*') {
#if !defined(QWS) && defined(Q_OS_MAC)
            qt_mac_get_fixed_pitch(const_cast<QtFontFamily*>(family));
#endif
            if ((pitch == 'm' && !family->fixedPitch)
                || (pitch == 'p' && family->fixedPitch))
                this_score += PitchMismatch;
        }
#endif
        if (styleKey != style->key)
            this_score += StyleMismatch;
        if (!style->smoothScalable && px != size->pixelSize) // bitmap scaled
            this_score += BitmapScaledPenalty;
        if (px != pixelSize) // close, but not exact, size match
            this_score += qAbs(px - pixelSize);

        if (this_score < score) {
            FM_DEBUG("          found a match: score %x best score so far %x",
                     this_score, score);

            score = this_score;
            desc->foundry = foundry;
            desc->style = style;
            desc->size = size;
#ifdef Q_WS_X11
            desc->encoding = encoding;
#endif // Q_WS_X11
        } else {
            FM_DEBUG("          score %x no better than best %x", this_score, score);
        }
    }

    return score;
}
#endif

#if !defined(Q_WS_MAC)
/*!
    \internal

    Tries to find the best match for a given request and family/foundry
*/
static void match(int script, const QFontDef &request,
                  const QString &family_name, const QString &foundry_name, int force_encoding_id,
                  QtFontDesc *desc, const QList<int> &blacklistedFamilies, bool forceXLFD)
{
    Q_UNUSED(force_encoding_id);

    QtFontStyle::Key styleKey;
    styleKey.style = request.style;
    styleKey.weight = request.weight;
    styleKey.stretch = request.stretch;
    char pitch = request.ignorePitch ? '*' : request.fixedPitch ? 'm' : 'p';


    FM_DEBUG("QFontDatabase::match\n"
             "  request:\n"
             "    family: %s [%s], script: %d\n"
             "    weight: %d, style: %d\n"
             "    stretch: %d\n"
             "    pixelSize: %g\n"
             "    pitch: %c",
             family_name.isEmpty() ? "-- first in script --" : family_name.toLatin1().constData(),
             foundry_name.isEmpty() ? "-- any --" : foundry_name.toLatin1().constData(),
             script, request.weight, request.style, request.stretch, request.pixelSize, pitch);
#if defined(FONT_MATCH_DEBUG) && defined(Q_WS_X11)
    if (force_encoding_id >= 0) {
        FM_DEBUG("    required encoding: %d", force_encoding_id);
    }
#endif

    desc->family = 0;
    desc->foundry = 0;
    desc->style = 0;
    desc->size = 0;
    desc->encoding = 0;
    desc->familyIndex = -1;

    unsigned int score = ~0u;

#ifdef Q_WS_X11
    load(family_name, script, forceXLFD);
#else
    Q_UNUSED(forceXLFD);
    load(family_name, script);
#endif

    QFontDatabasePrivate *db = privateDb();
    for (int x = 0; x < db->count; ++x) {
        if (blacklistedFamilies.contains(x))
            continue;
        QtFontDesc test;
        test.family = db->families[x];
        test.familyIndex = x;

        if (!family_name.isEmpty()
            && test.family->name.compare(family_name, Qt::CaseInsensitive) != 0
#ifdef Q_WS_WIN
            && test.family->english_name.compare(family_name, Qt::CaseInsensitive) != 0
#endif
            )
            continue;

        if (family_name.isEmpty())
            load(test.family->name, script);

        uint score_adjust = 0;

        bool supported = (script == QUnicodeTables::Common);
        for (int ws = 1; !supported && ws < QFontDatabase::WritingSystemsCount; ++ws) {
            if (scriptForWritingSystem[ws] != script)
                continue;
            if (test.family->writingSystems[ws] & QtFontFamily::Supported)
                supported = true;
        }
        if (!supported) {
            // family not supported in the script we want
            continue;
        }

        // as we know the script is supported, we can be sure
        // to find a matching font here.
        unsigned int newscore =
            bestFoundry(script, score, request.styleStrategy,
                        test.family, foundry_name, styleKey, request.pixelSize, pitch,
                        &test, force_encoding_id);
        if (test.foundry == 0) {
            // the specific foundry was not found, so look for
            // any foundry matching our requirements
            newscore = bestFoundry(script, score, request.styleStrategy, test.family,
                                   QString(), styleKey, request.pixelSize,
                                   pitch, &test, force_encoding_id);
        }
        newscore += score_adjust;

        if (newscore < score) {
            score = newscore;
            *desc = test;
        }
        if (newscore < 10) // xlfd instead of FT... just accept it
            break;
    }
}
#endif

static QString styleStringHelper(int weight, QFont::Style style)
{
    QString result;
    if (weight >= QFont::Black)
        result = QApplication::translate("QFontDatabase", "Black");
    else if (weight >= QFont::Bold)
        result = QApplication::translate("QFontDatabase", "Bold");
    else if (weight >= QFont::DemiBold)
        result = QApplication::translate("QFontDatabase", "Demi Bold");
    else if (weight < QFont::Normal)
        result = QApplication::translate("QFontDatabase", "Light");

    if (style == QFont::StyleItalic)
        result += QLatin1Char(' ') + QApplication::translate("QFontDatabase", "Italic");
    else if (style == QFont::StyleOblique)
        result += QLatin1Char(' ') + QApplication::translate("QFontDatabase", "Oblique");

    if (result.isEmpty())
        result = QApplication::translate("QFontDatabase", "Normal");

    return result.simplified();
}

/*!
    Returns a string that describes the style of the \a font. For
    example, "Bold Italic", "Bold", "Italic" or "Normal". An empty
    string may be returned.
*/
QString QFontDatabase::styleString(const QFont &font)
{
    return font.styleName().isEmpty() ? styleStringHelper(font.weight(), font.style())
                                      : font.styleName();
}

/*!
    Returns a string that describes the style of the \a fontInfo. For
    example, "Bold Italic", "Bold", "Italic" or "Normal". An empty
    string may be returned.
*/
QString QFontDatabase::styleString(const QFontInfo &fontInfo)
{
    return fontInfo.styleName().isEmpty() ? styleStringHelper(fontInfo.weight(), fontInfo.style())
                                          : fontInfo.styleName();
}


/*!
    \class QFontDatabase
    \threadsafe

    \brief The QFontDatabase class provides information about the fonts available in the underlying window system.

    \ingroup appearance

    The most common uses of this class are to query the database for
    the list of font families() and for the pointSizes() and styles()
    that are available for each family. An alternative to pointSizes()
    is smoothSizes() which returns the sizes at which a given family
    and style will look attractive.

    If the font family is available from two or more foundries the
    foundry name is included in the family name; for example:
    "Helvetica [Adobe]" and "Helvetica [Cronyx]". When you specify a
    family, you can either use the old hyphenated "foundry-family"
    format or the bracketed "family [foundry]" format; for example:
    "Cronyx-Helvetica" or "Helvetica [Cronyx]". If the family has a
    foundry it is always returned using the bracketed format, as is
    the case with the value returned by families().

    The font() function returns a QFont given a family, style and
    point size.

    A family and style combination can be checked to see if it is
    italic() or bold(), and to retrieve its weight(). Similarly we can
    call isBitmapScalable(), isSmoothlyScalable(), isScalable() and
    isFixedPitch().

    Use the styleString() to obtain a text version of a style.

    The QFontDatabase class also supports some static functions, for
    example, standardSizes(). You can retrieve the description of a
    writing system using writingSystemName(), and a sample of
    characters in a writing system with writingSystemSample().

    Example:

    \snippet doc/src/snippets/qfontdatabase/main.cpp 0
    \snippet doc/src/snippets/qfontdatabase/main.cpp 1

    This example gets the list of font families, the list of
    styles for each family, and the point sizes that are available for
    each combination of family and style, displaying this information
    in a tree view.

    \sa QFont, QFontInfo, QFontMetrics, QFontComboBox, {Character Map Example}
*/

/*!
    Creates a font database object.
*/
QFontDatabase::QFontDatabase()
{
    QMutexLocker locker(fontDatabaseMutex());
    createDatabase();
    d = privateDb();
}

/*!
    \enum QFontDatabase::WritingSystem

    \value Any
    \value Latin
    \value Greek
    \value Cyrillic
    \value Armenian
    \value Hebrew
    \value Arabic
    \value Syriac
    \value Thaana
    \value Devanagari
    \value Bengali
    \value Gurmukhi
    \value Gujarati
    \value Oriya
    \value Tamil
    \value Telugu
    \value Kannada
    \value Malayalam
    \value Sinhala
    \value Thai
    \value Lao
    \value Tibetan
    \value Myanmar
    \value Georgian
    \value Khmer
    \value SimplifiedChinese
    \value TraditionalChinese
    \value Japanese
    \value Korean
    \value Vietnamese
    \value Symbol
    \value Other (the same as Symbol)
    \value Ogham
    \value Runic
    \value Nko

    \omitvalue WritingSystemsCount
*/

/*!
    Returns a sorted list of the available writing systems. This is
    list generated from information about all installed fonts on the
    system.

    \sa families()
*/
QList<QFontDatabase::WritingSystem> QFontDatabase::writingSystems() const
{
    QMutexLocker locker(fontDatabaseMutex());

    QT_PREPEND_NAMESPACE(load)();
#ifdef Q_WS_X11
    checkSymbolFonts();
#endif

    QList<WritingSystem> list;
    for (int i = 0; i < d->count; ++i) {
        QtFontFamily *family = d->families[i];
        if (family->count == 0)
            continue;
        for (int x = Latin; x < WritingSystemsCount; ++x) {
            const WritingSystem writingSystem = WritingSystem(x);
            if (!(family->writingSystems[writingSystem] & QtFontFamily::Supported))
                continue;
            if (!list.contains(writingSystem))
                list.append(writingSystem);
        }
    }
    qSort(list);
    return list;
}


/*!
    Returns a sorted list of the writing systems supported by a given
    font \a family.

    \sa families()
*/
QList<QFontDatabase::WritingSystem> QFontDatabase::writingSystems(const QString &family) const
{
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());

    QT_PREPEND_NAMESPACE(load)();
#ifdef Q_WS_X11
    checkSymbolFonts(familyName);
#endif

    QList<WritingSystem> list;
    QtFontFamily *f = d->family(familyName);
    if (!f || f->count == 0)
        return list;

    for (int x = Latin; x < WritingSystemsCount; ++x) {
        const WritingSystem writingSystem = WritingSystem(x);
        if (f->writingSystems[writingSystem] & QtFontFamily::Supported)
            list.append(writingSystem);
    }
    return list;
}


/*!
    Returns a sorted list of the available font families which support
    the \a writingSystem.

    If a family exists in several foundries, the returned name for
    that font is in the form "family [foundry]". Examples: "Times
    [Adobe]", "Times [Cronyx]", "Palatino".

    \sa writingSystems()
*/
QStringList QFontDatabase::families(WritingSystem writingSystem) const
{
    QMutexLocker locker(fontDatabaseMutex());

    QT_PREPEND_NAMESPACE(load)();
#ifdef Q_WS_X11
    if (writingSystem != Any)
        checkSymbolFonts();
#endif

    QStringList flist;
    for (int i = 0; i < d->count; i++) {
        QtFontFamily *f = d->families[i];
        if (f->count == 0)
            continue;
        if (writingSystem != Any && (f->writingSystems[writingSystem] != QtFontFamily::Supported))
            continue;
        if (f->count == 1) {
            flist.append(f->name);
        } else {
            for (int j = 0; j < f->count; j++) {
                QString str = f->name;
                QString foundry = f->foundries[j]->name;
                if (!foundry.isEmpty()) {
                    str += QLatin1String(" [");
                    str += foundry;
                    str += QLatin1Char(']');
                }
                flist.append(str);
            }
        }
    }
    return flist;
}

/*!
    Returns a list of the styles available for the font family \a
    family. Some example styles: "Light", "Light Italic", "Bold",
    "Oblique", "Demi". The list may be empty.

    \sa families()
*/
QStringList QFontDatabase::styles(const QString &family) const
{
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());

    QT_PREPEND_NAMESPACE(load)(familyName);

    QStringList l;
    QtFontFamily *f = d->family(familyName);
    if (!f)
        return l;

    QtFontFoundry allStyles(foundryName);
    for (int j = 0; j < f->count; j++) {
        QtFontFoundry *foundry = f->foundries[j];
        if (foundryName.isEmpty() || foundry->name.compare(foundryName, Qt::CaseInsensitive) == 0) {
            for (int k = 0; k < foundry->count; k++) {
                QtFontStyle::Key ke(foundry->styles[k]->key);
                ke.stretch = 0;
                allStyles.style(ke, foundry->styles[k]->styleName, true);
            }
        }
    }

    for (int i = 0; i < allStyles.count; i++) {
        l.append(allStyles.styles[i]->styleName.isEmpty() ?
                 styleStringHelper(allStyles.styles[i]->key.weight,
                                   (QFont::Style)allStyles.styles[i]->key.style) :
                 allStyles.styles[i]->styleName);
    }
    return l;
}

/*!
    Returns true if the font that has family \a family and style \a
    style is fixed pitch; otherwise returns false.
*/

bool QFontDatabase::isFixedPitch(const QString &family,
                                 const QString &style) const
{
    Q_UNUSED(style);

    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());

    QT_PREPEND_NAMESPACE(load)(familyName);

    QtFontFamily *f = d->family(familyName);
#if !defined(QWS) && defined(Q_OS_MAC)
    qt_mac_get_fixed_pitch(f);
#endif
    return (f && f->fixedPitch);
}

/*!
    Returns true if the font that has family \a family and style \a
    style is a scalable bitmap font; otherwise returns false. Scaling
    a bitmap font usually produces an unattractive hardly readable
    result, because the pixels of the font are scaled. If you need to
    scale a bitmap font it is better to scale it to one of the fixed
    sizes returned by smoothSizes().

    \sa isScalable(), isSmoothlyScalable()
*/
bool QFontDatabase::isBitmapScalable(const QString &family,
                                      const QString &style) const
{
    bool bitmapScalable = false;
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());

    QT_PREPEND_NAMESPACE(load)(familyName);

    QtFontStyle::Key styleKey(style);

    QtFontFamily *f = d->family(familyName);
    if (!f) return bitmapScalable;

    for (int j = 0; j < f->count; j++) {
        QtFontFoundry *foundry = f->foundries[j];
        if (foundryName.isEmpty() || foundry->name.compare(foundryName, Qt::CaseInsensitive) == 0) {
            for (int k = 0; k < foundry->count; k++)
                if ((style.isEmpty() ||
                     foundry->styles[k]->styleName == style ||
                     foundry->styles[k]->key == styleKey)
                    && foundry->styles[k]->bitmapScalable && !foundry->styles[k]->smoothScalable) {
                    bitmapScalable = true;
                    goto end;
                }
        }
    }
 end:
    return bitmapScalable;
}


/*!
    Returns true if the font that has family \a family and style \a
    style is smoothly scalable; otherwise returns false. If this
    function returns true, it's safe to scale this font to any size,
    and the result will always look attractive.

    \sa isScalable(), isBitmapScalable()
*/
bool QFontDatabase::isSmoothlyScalable(const QString &family, const QString &style) const
{
    bool smoothScalable = false;
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());

    QT_PREPEND_NAMESPACE(load)(familyName);

    QtFontStyle::Key styleKey(style);

    QtFontFamily *f = d->family(familyName);
    if (!f) return smoothScalable;

    for (int j = 0; j < f->count; j++) {
        QtFontFoundry *foundry = f->foundries[j];
        if (foundryName.isEmpty() || foundry->name.compare(foundryName, Qt::CaseInsensitive) == 0) {
            for (int k = 0; k < foundry->count; k++)
                if ((style.isEmpty() ||
                     foundry->styles[k]->styleName == style ||
                     foundry->styles[k]->key == styleKey) && foundry->styles[k]->smoothScalable) {
                    smoothScalable = true;
                    goto end;
                }
        }
    }
 end:
    return smoothScalable;
}

/*!
    Returns true if the font that has family \a family and style \a
    style is scalable; otherwise returns false.

    \sa isBitmapScalable(), isSmoothlyScalable()
*/
bool  QFontDatabase::isScalable(const QString &family,
                                 const QString &style) const
{
    QMutexLocker locker(fontDatabaseMutex());
    if (isSmoothlyScalable(family, style))
        return true;
    return isBitmapScalable(family, style);
}


/*!
    \fn QList<int> QFontDatabase::pointSizes(const QString &family, const QString &style)
    Returns a list of the point sizes available for the font with the
    given \a family and \a style. The list may be empty.

    \sa smoothSizes(), standardSizes()
*/
QList<int> QFontDatabase::pointSizes(const QString &family,
                                           const QString &styleName)
{
#if defined(Q_WS_WIN)
    // windows and macosx are always smoothly scalable
    Q_UNUSED(family);
    Q_UNUSED(styleName);
    return standardSizes();
#else
    bool smoothScalable = false;
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());

    QT_PREPEND_NAMESPACE(load)(familyName);

    QtFontStyle::Key styleKey(styleName);

    QList<int> sizes;

    QtFontFamily *fam = d->family(familyName);
    if (!fam) return sizes;


#ifdef Q_WS_X11
    int dpi = QX11Info::appDpiY();
#else
    const int dpi = qt_defaultDpiY(); // embedded
#endif

    for (int j = 0; j < fam->count; j++) {
        QtFontFoundry *foundry = fam->foundries[j];
        if (foundryName.isEmpty() || foundry->name.compare(foundryName, Qt::CaseInsensitive) == 0) {
            QtFontStyle *style = foundry->style(styleKey, styleName);
            if (!style) continue;

            if (style->smoothScalable) {
                smoothScalable = true;
                goto end;
            }
            for (int l = 0; l < style->count; l++) {
                const QtFontSize *size = style->pixelSizes + l;

                if (size->pixelSize != 0 && size->pixelSize != USHRT_MAX) {
                    const uint pointSize = qRound(size->pixelSize * qreal(72.0) / dpi);
                    if (! sizes.contains(pointSize))
                        sizes.append(pointSize);
                }
            }
        }
    }
 end:
    if (smoothScalable)
        return standardSizes();

    qSort(sizes);
    return sizes;
#endif
}

/*!
    Returns a QFont object that has family \a family, style \a style
    and point size \a pointSize. If no matching font could be created,
    a QFont object that uses the application's default font is
    returned.
*/
QFont QFontDatabase::font(const QString &family, const QString &style,
                           int pointSize) const
{
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());

    QT_PREPEND_NAMESPACE(load)(familyName);

    QtFontFoundry allStyles(foundryName);
    QtFontFamily *f = d->family(familyName);
    if (!f) return QApplication::font();

    for (int j = 0; j < f->count; j++) {
        QtFontFoundry *foundry = f->foundries[j];
        if (foundryName.isEmpty() || foundry->name.compare(foundryName, Qt::CaseInsensitive) == 0) {
            for (int k = 0; k < foundry->count; k++)
                allStyles.style(foundry->styles[k]->key, foundry->styles[k]->styleName, true);
        }
    }

    QtFontStyle::Key styleKey(style);
    QtFontStyle *s = bestStyle(&allStyles, styleKey, style);

    if (!s) // no styles found?
        return QApplication::font();

    QFont fnt(family, pointSize, s->key.weight);
    fnt.setStyle((QFont::Style)s->key.style);
    if (!s->styleName.isEmpty())
        fnt.setStyleName(s->styleName);
    return fnt;
}


/*!
    \fn QList<int> QFontDatabase::smoothSizes(const QString &family, const QString &style)
    Returns the point sizes of a font with the given \a family and \a style
    that will look attractive. The list may be empty.
    For non-scalable fonts and bitmap scalable fonts, this function
    is equivalent to pointSizes().

  \sa pointSizes(), standardSizes()
*/
QList<int> QFontDatabase::smoothSizes(const QString &family,
                                            const QString &styleName)
{
#ifdef Q_WS_WIN
    Q_UNUSED(family);
    Q_UNUSED(styleName);
    return QFontDatabase::standardSizes();
#else
    bool smoothScalable = false;
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());

    QT_PREPEND_NAMESPACE(load)(familyName);

    QtFontStyle::Key styleKey(styleName);

    QList<int> sizes;

    QtFontFamily *fam = d->family(familyName);
    if (!fam)
        return sizes;

#ifdef Q_WS_X11
    int dpi = QX11Info::appDpiY();
#else
    const int dpi = qt_defaultDpiY(); // embedded
#endif

    for (int j = 0; j < fam->count; j++) {
        QtFontFoundry *foundry = fam->foundries[j];
        if (foundryName.isEmpty() || foundry->name.compare(foundryName, Qt::CaseInsensitive) == 0) {
            QtFontStyle *style = foundry->style(styleKey, styleName);
            if (!style) continue;

            if (style->smoothScalable) {
                smoothScalable = true;
                goto end;
            }
            for (int l = 0; l < style->count; l++) {
                const QtFontSize *size = style->pixelSizes + l;

                if (size->pixelSize != 0 && size->pixelSize != USHRT_MAX) {
                    const uint pointSize = qRound(size->pixelSize * qreal(72.0) / dpi);
                    if (! sizes.contains(pointSize))
                        sizes.append(pointSize);
                }
            }
        }
    }
 end:
    if (smoothScalable)
        return QFontDatabase::standardSizes();

    qSort(sizes);
    return sizes;
#endif
}


/*!
    Returns a list of standard font sizes.

    \sa smoothSizes(), pointSizes()
*/
QList<int> QFontDatabase::standardSizes()
{
    QList<int> ret;
    static const unsigned short standard[] =
        { 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72, 0 };
    const unsigned short *sizes = standard;
    while (*sizes) ret << *sizes++;
    return ret;
}


/*!
    Returns true if the font that has family \a family and style \a
    style is italic; otherwise returns false.

    \sa weight(), bold()
*/
bool QFontDatabase::italic(const QString &family, const QString &style) const
{
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());

    QT_PREPEND_NAMESPACE(load)(familyName);

    QtFontFoundry allStyles(foundryName);
    QtFontFamily *f = d->family(familyName);
    if (!f) return false;

    for (int j = 0; j < f->count; j++) {
        QtFontFoundry *foundry = f->foundries[j];
        if (foundryName.isEmpty() || foundry->name.compare(foundryName, Qt::CaseInsensitive) == 0) {
            for (int k = 0; k < foundry->count; k++)
                allStyles.style(foundry->styles[k]->key, foundry->styles[k]->styleName, true);
        }
    }

    QtFontStyle::Key styleKey(style);
    QtFontStyle *s = allStyles.style(styleKey, style);
    return s && s->key.style == QFont::StyleItalic;
}


/*!
    Returns true if the font that has family \a family and style \a
    style is bold; otherwise returns false.

    \sa italic(), weight()
*/
bool QFontDatabase::bold(const QString &family,
                          const QString &style) const
{
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());

    QT_PREPEND_NAMESPACE(load)(familyName);

    QtFontFoundry allStyles(foundryName);
    QtFontFamily *f = d->family(familyName);
    if (!f) return false;

    for (int j = 0; j < f->count; j++) {
        QtFontFoundry *foundry = f->foundries[j];
        if (foundryName.isEmpty() ||
            foundry->name.compare(foundryName, Qt::CaseInsensitive) == 0) {
            for (int k = 0; k < foundry->count; k++)
                allStyles.style(foundry->styles[k]->key, foundry->styles[k]->styleName, true);
        }
    }

    QtFontStyle::Key styleKey(style);
    QtFontStyle *s = allStyles.style(styleKey, style);
    return s && s->key.weight >= QFont::Bold;
}


/*!
    Returns the weight of the font that has family \a family and style
    \a style. If there is no such family and style combination,
    returns -1.

    \sa italic(), bold()
*/
int QFontDatabase::weight(const QString &family,
                           const QString &style) const
{
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());

    QT_PREPEND_NAMESPACE(load)(familyName);

    QtFontFoundry allStyles(foundryName);
    QtFontFamily *f = d->family(familyName);
    if (!f) return -1;

    for (int j = 0; j < f->count; j++) {
        QtFontFoundry *foundry = f->foundries[j];
        if (foundryName.isEmpty() ||
            foundry->name.compare(foundryName, Qt::CaseInsensitive) == 0) {
            for (int k = 0; k < foundry->count; k++)
                allStyles.style(foundry->styles[k]->key, foundry->styles[k]->styleName, true);
        }
    }

    QtFontStyle::Key styleKey(style);
    QtFontStyle *s = allStyles.style(styleKey, style);
    return s ? s->key.weight : -1;
}


/*! \internal */
bool QFontDatabase::hasFamily(const QString &family) const
{
    QString parsedFamily, foundry;
    parseFontName(family, foundry, parsedFamily);
    const QString familyAlias = resolveFontFamilyAlias(parsedFamily);
    return families().contains(familyAlias, Qt::CaseInsensitive);
}


/*!
    Returns the names the \a writingSystem (e.g. for displaying to the
    user in a dialog).
*/
QString QFontDatabase::writingSystemName(WritingSystem writingSystem)
{
    const char *name = 0;
    switch (writingSystem) {
    case Any:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Any");
        break;
    case Latin:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Latin");
        break;
    case Greek:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Greek");
        break;
    case Cyrillic:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Cyrillic");
        break;
    case Armenian:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Armenian");
        break;
    case Hebrew:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Hebrew");
        break;
    case Arabic:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Arabic");
        break;
    case Syriac:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Syriac");
        break;
    case Thaana:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Thaana");
        break;
    case Devanagari:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Devanagari");
        break;
    case Bengali:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Bengali");
        break;
    case Gurmukhi:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Gurmukhi");
        break;
    case Gujarati:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Gujarati");
        break;
    case Oriya:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Oriya");
        break;
    case Tamil:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Tamil");
        break;
    case Telugu:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Telugu");
        break;
    case Kannada:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Kannada");
        break;
    case Malayalam:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Malayalam");
        break;
    case Sinhala:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Sinhala");
        break;
    case Thai:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Thai");
        break;
    case Lao:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Lao");
        break;
    case Tibetan:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Tibetan");
        break;
    case Myanmar:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Myanmar");
        break;
    case Georgian:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Georgian");
        break;
    case Khmer:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Khmer");
        break;
    case SimplifiedChinese:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Simplified Chinese");
        break;
    case TraditionalChinese:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Traditional Chinese");
        break;
    case Japanese:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Japanese");
        break;
    case Korean:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Korean");
        break;
    case Vietnamese:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Vietnamese");
        break;
    case Symbol:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Symbol");
        break;
    case Ogham:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Ogham");
        break;
    case Runic:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "Runic");
        break;
    case Nko:
        name = QT_TRANSLATE_NOOP("QFontDatabase", "N'Ko");
        break;
    default:
        Q_ASSERT_X(false, "QFontDatabase::writingSystemName", "invalid 'writingSystem' parameter");
        break;
    }
    return QApplication::translate("QFontDatabase", name);
}


/*!
    Returns a string with sample characters from \a writingSystem.
*/
QString QFontDatabase::writingSystemSample(WritingSystem writingSystem)
{
    QString sample;
    switch (writingSystem) {
    case Any:
    case Symbol:
        // show only ascii characters
        sample += QLatin1String("AaBbzZ");
        break;
    case Latin:
        // This is cheating... we only show latin-1 characters so that we don't
        // end up loading lots of fonts - at least on X11...
        sample = QLatin1String("Aa");
        sample += QChar(0x00C3);
        sample += QChar(0x00E1);
        sample += QLatin1String("Zz");
        break;
    case Greek:
        sample += QChar(0x0393);
        sample += QChar(0x03B1);
        sample += QChar(0x03A9);
        sample += QChar(0x03C9);
        break;
    case Cyrillic:
        sample += QChar(0x0414);
        sample += QChar(0x0434);
        sample += QChar(0x0436);
        sample += QChar(0x044f);
        break;
    case Armenian:
        sample += QChar(0x053f);
        sample += QChar(0x054f);
        sample += QChar(0x056f);
        sample += QChar(0x057f);
        break;
    case Hebrew:
        sample += QChar(0x05D0);
        sample += QChar(0x05D1);
        sample += QChar(0x05D2);
        sample += QChar(0x05D3);
        break;
    case Arabic:
        sample += QChar(0x0628);
        sample += QChar(0x0629);
        sample += QChar(0x062A);
        sample += QChar(0x063A);
        break;
    case Syriac:
        sample += QChar(0x0715);
        sample += QChar(0x0725);
        sample += QChar(0x0716);
        sample += QChar(0x0726);
        break;
    case Thaana:
        sample += QChar(0x0784);
        sample += QChar(0x0794);
        sample += QChar(0x078c);
        sample += QChar(0x078d);
        break;
    case Devanagari:
        sample += QChar(0x0905);
        sample += QChar(0x0915);
        sample += QChar(0x0925);
        sample += QChar(0x0935);
        break;
    case Bengali:
        sample += QChar(0x0986);
        sample += QChar(0x0996);
        sample += QChar(0x09a6);
        sample += QChar(0x09b6);
        break;
    case Gurmukhi:
        sample += QChar(0x0a05);
        sample += QChar(0x0a15);
        sample += QChar(0x0a25);
        sample += QChar(0x0a35);
        break;
    case Gujarati:
        sample += QChar(0x0a85);
        sample += QChar(0x0a95);
        sample += QChar(0x0aa5);
        sample += QChar(0x0ab5);
        break;
    case Oriya:
        sample += QChar(0x0b06);
        sample += QChar(0x0b16);
        sample += QChar(0x0b2b);
        sample += QChar(0x0b36);
        break;
    case Tamil:
        sample += QChar(0x0b89);
        sample += QChar(0x0b99);
        sample += QChar(0x0ba9);
        sample += QChar(0x0bb9);
        break;
    case Telugu:
        sample += QChar(0x0c05);
        sample += QChar(0x0c15);
        sample += QChar(0x0c25);
        sample += QChar(0x0c35);
        break;
    case Kannada:
        sample += QChar(0x0c85);
        sample += QChar(0x0c95);
        sample += QChar(0x0ca5);
        sample += QChar(0x0cb5);
        break;
    case Malayalam:
        sample += QChar(0x0d05);
        sample += QChar(0x0d15);
        sample += QChar(0x0d25);
        sample += QChar(0x0d35);
        break;
    case Sinhala:
        sample += QChar(0x0d90);
        sample += QChar(0x0da0);
        sample += QChar(0x0db0);
        sample += QChar(0x0dc0);
        break;
    case Thai:
        sample += QChar(0x0e02);
        sample += QChar(0x0e12);
        sample += QChar(0x0e22);
        sample += QChar(0x0e32);
        break;
    case Lao:
        sample += QChar(0x0e8d);
        sample += QChar(0x0e9d);
        sample += QChar(0x0ead);
        sample += QChar(0x0ebd);
        break;
    case Tibetan:
        sample += QChar(0x0f00);
        sample += QChar(0x0f01);
        sample += QChar(0x0f02);
        sample += QChar(0x0f03);
        break;
    case Myanmar:
        sample += QChar(0x1000);
        sample += QChar(0x1001);
        sample += QChar(0x1002);
        sample += QChar(0x1003);
        break;
    case Georgian:
        sample += QChar(0x10a0);
        sample += QChar(0x10b0);
        sample += QChar(0x10c0);
        sample += QChar(0x10d0);
        break;
    case Khmer:
        sample += QChar(0x1780);
        sample += QChar(0x1790);
        sample += QChar(0x17b0);
        sample += QChar(0x17c0);
        break;
    case SimplifiedChinese:
        sample += QChar(0x4e2d);
        sample += QChar(0x6587);
        sample += QChar(0x8303);
        sample += QChar(0x4f8b);
        break;
    case TraditionalChinese:
        sample += QChar(0x4e2d);
        sample += QChar(0x6587);
        sample += QChar(0x7bc4);
        sample += QChar(0x4f8b);
        break;
    case Japanese:
        sample += QChar(0x30b5);
        sample += QChar(0x30f3);
        sample += QChar(0x30d7);
        sample += QChar(0x30eb);
        sample += QChar(0x3067);
        sample += QChar(0x3059);
        break;
    case Korean:
        sample += QChar(0xac00);
        sample += QChar(0xac11);
        sample += QChar(0xac1a);
        sample += QChar(0xac2f);
        break;
    case Vietnamese:
    {
        static const char vietnameseUtf8[] = {
            char(0xef), char(0xbb), char(0xbf), char(0xe1), char(0xbb), char(0x97),
            char(0xe1), char(0xbb), char(0x99),
            char(0xe1), char(0xbb), char(0x91),
            char(0xe1), char(0xbb), char(0x93),
        };
        sample += QString::fromUtf8(vietnameseUtf8, sizeof(vietnameseUtf8));
        break;
    }
    case Ogham:
        sample += QChar(0x1681);
        sample += QChar(0x1682);
        sample += QChar(0x1683);
        sample += QChar(0x1684);
        break;
    case Runic:
        sample += QChar(0x16a0);
        sample += QChar(0x16a1);
        sample += QChar(0x16a2);
        sample += QChar(0x16a3);
        break;
    case Nko:
        sample += QChar(0x7ca);
        sample += QChar(0x7cb);
        sample += QChar(0x7cc);
        sample += QChar(0x7cd);
        break;
    default:
        break;
    }
    return sample;
}


void QFontDatabase::parseFontName(const QString &name, QString &foundry, QString &family)
{
    QT_PREPEND_NAMESPACE(parseFontName)(name, foundry, family);
}

void QFontDatabase::createDatabase()
{ initializeDb(); }

// used from qfontengine_ft.cpp
Q_GUI_EXPORT QByteArray qt_fontdata_from_index(int index)
{
    QMutexLocker locker(fontDatabaseMutex());
    return privateDb()->applicationFonts.value(index).data;
}

int QFontDatabasePrivate::addAppFont(const QByteArray &fontData, const QString &fileName)
{
    QFontDatabasePrivate::ApplicationFont font;
    font.data = fontData;
    font.fileName = fileName;

    int i;
    for (i = 0; i < applicationFonts.count(); ++i)
        if (applicationFonts.at(i).families.isEmpty())
            break;
    if (i >= applicationFonts.count()) {
        applicationFonts.append(ApplicationFont());
        i = applicationFonts.count() - 1;
    }

    if (font.fileName.isEmpty() && !fontData.isEmpty())
        font.fileName = QString::fromLatin1(":qmemoryfonts/") + QString::number(i);

    registerFont(&font);
    if (font.families.isEmpty())
        return -1;

    applicationFonts[i] = font;

    invalidate();
    return i;
}

bool QFontDatabasePrivate::isApplicationFont(const QString &fileName)
{
    for (int i = 0; i < applicationFonts.count(); ++i)
        if (applicationFonts.at(i).fileName == fileName)
            return true;
    return false;
}

/*!
    \since 4.2

    Loads the font from the file specified by \a fileName and makes it available to
    the application. An ID is returned that can be used to remove the font again
    with removeApplicationFont() or to retrieve the list of family names contained
    in the font.

    The function returns -1 if the font could not be loaded.

    Currently only TrueType fonts, TrueType font collections, and OpenType fonts are
    supported.

    \note Adding application fonts on Unix/X11 platforms without fontconfig is
    currently not supported.

    \note On Symbian, the font family names get truncated to a length of 20 characters.

    \sa addApplicationFontFromData(), applicationFontFamilies(), removeApplicationFont()
*/
int QFontDatabase::addApplicationFont(const QString &fileName)
{
    QByteArray data;
    QFile f(fileName);
    if (!(f.fileEngine()->fileFlags(QAbstractFileEngine::FlagsMask) & QAbstractFileEngine::LocalDiskFlag)) {
        if (!f.open(QIODevice::ReadOnly))
            return -1;
        data = f.readAll();
    }
    QMutexLocker locker(fontDatabaseMutex());
    return privateDb()->addAppFont(data, fileName);
}

/*!
    \since 4.2

    Loads the font from binary data specified by \a fontData and makes it available to
    the application. An ID is returned that can be used to remove the font again
    with removeApplicationFont() or to retrieve the list of family names contained
    in the font.

    The function returns -1 if the font could not be loaded.

    Currently only TrueType fonts and TrueType font collections are supported.

    \bold{Note:} Adding application fonts on Unix/X11 platforms without fontconfig is
    currently not supported.

    \note On Symbian, the font family names get truncated to a length of 20 characters.

    \sa addApplicationFont(), applicationFontFamilies(), removeApplicationFont()
*/
int QFontDatabase::addApplicationFontFromData(const QByteArray &fontData)
{
    QMutexLocker locker(fontDatabaseMutex());
    return privateDb()->addAppFont(fontData, QString() /* fileName */);
}

/*!
    \since 4.2

    Returns a list of font families for the given application font identified by
    \a id.

    \sa addApplicationFont(), addApplicationFontFromData()
*/
QStringList QFontDatabase::applicationFontFamilies(int id)
{
    QMutexLocker locker(fontDatabaseMutex());
    return privateDb()->applicationFonts.value(id).families;
}

/*!
    \fn bool QFontDatabase::removeApplicationFont(int id)
    \since 4.2

    Removes the previously loaded application font identified by \a
    id. Returns true if unloading of the font succeeded; otherwise
    returns false.

    \sa removeAllApplicationFonts(), addApplicationFont(),
        addApplicationFontFromData()
*/

/*!
    \fn bool QFontDatabase::removeAllApplicationFonts()
    \since 4.2

    Removes all application-local fonts previously added using addApplicationFont()
    and addApplicationFontFromData().

    Returns true if unloading of the fonts succeeded; otherwise
    returns false.

    \sa removeApplicationFont(), addApplicationFont(), addApplicationFontFromData()
*/

/*!
    \fn bool QFontDatabase::supportsThreadedFontRendering()
    \since 4.4

    Returns true if font rendering is supported outside the GUI
    thread, false otherwise. In other words, a return value of false
    means that all QPainter::drawText() calls outside the GUI thread
    will not produce readable output.

    \sa {Thread-Support in Qt Modules#Painting In Threads}{Painting In Threads}
*/


QT_END_NAMESPACE

