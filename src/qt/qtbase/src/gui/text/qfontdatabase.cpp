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

#include "qfontdatabase.h"
#include "qdebug.h"
#include "qalgorithms.h"
#include "qguiapplication.h"
#include "qvarlengtharray.h" // here or earlier - workaround for VC++6
#include "qthread.h"
#include "qmutex.h"
#include "qfile.h"
#include "qfileinfo.h"
#include "qfontengine_p.h"
#include <qpa/qplatformintegration.h>

#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformfontdatabase.h>
#include <qpa/qplatformtheme.h>

#include <QtCore/qmath.h>

#include <stdlib.h>
#include <limits.h>
#include <algorithm>


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


QT_BEGIN_NAMESPACE

#define SMOOTH_SCALABLE 0xffff

bool qt_enable_test_font = false;

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
        || s.compare(QCoreApplication::translate("QFontDatabase", "Normal"), Qt::CaseInsensitive) == 0)
        return QFont::Normal;
    if (s == QLatin1String("bold")
        || s.compare(QCoreApplication::translate("QFontDatabase", "Bold"), Qt::CaseInsensitive) == 0)
        return QFont::Bold;
    if (s == QLatin1String("demibold") || s == QLatin1String("demi bold")
        || s.compare(QCoreApplication::translate("QFontDatabase", "Demi Bold"), Qt::CaseInsensitive) == 0)
        return QFont::DemiBold;
    if (s == QLatin1String("black")
        || s.compare(QCoreApplication::translate("QFontDatabase", "Black"), Qt::CaseInsensitive) == 0)
        return QFont::Black;
    if (s == QLatin1String("light"))
        return QFont::Light;

    if (s.contains(QLatin1String("bold"))
        || s.contains(QCoreApplication::translate("QFontDatabase", "Bold"), Qt::CaseInsensitive)) {
        if (s.contains(QLatin1String("demi"))
            || s.compare(QCoreApplication::translate("QFontDatabase", "Demi"), Qt::CaseInsensitive) == 0)
            return (int) QFont::DemiBold;
        return (int) QFont::Bold;
    }

    if (s.contains(QLatin1String("light"))
        || s.compare(QCoreApplication::translate("QFontDatabase", "Light"), Qt::CaseInsensitive) == 0)
        return (int) QFont::Light;

    if (s.contains(QLatin1String("black"))
        || s.compare(QCoreApplication::translate("QFontDatabase", "Black"), Qt::CaseInsensitive) == 0)
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

    void *handle;

    unsigned short pixelSize : 16;
};



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
    }

    ~QtFontStyle() {
        while (count) {
            // bitfield count-- in while condition does not work correctly in mwccsym2
            count--;
            QPlatformIntegration *integration = QGuiApplicationPrivate::platformIntegration();
            if (integration) {
                integration->fontDatabase()->releaseHandle(pixelSizes[count].handle);
            }
        }
        free(pixelSizes);
    }

    Key key;
    bool bitmapScalable : 1;
    bool smoothScalable : 1;
    signed int count    : 30;
    QtFontSize *pixelSizes;
    QString styleName;

    bool antialiased;

    QtFontSize *pixelSize(unsigned short size, bool = false);
};

QtFontStyle::Key::Key(const QString &styleString)
    : style(QFont::StyleNormal), weight(QFont::Normal), stretch(0)
{
    weight = getFontWeight(styleString);

    if (styleString.contains(QLatin1String("Italic"))
        || styleString.contains(QCoreApplication::translate("QFontDatabase", "Italic")))
        style = QFont::StyleItalic;
    else if (styleString.contains(QLatin1String("Oblique"))
             || styleString.contains(QCoreApplication::translate("QFontDatabase", "Oblique")))
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
    pixelSizes[count].handle = 0;
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

//     qDebug("adding key (weight=%d, style=%d, oblique=%d stretch=%d) at %d", key.weight, key.style, key.oblique, key.stretch, pos);
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
        Unsupported     = UnsupportedFT
    };

    QtFontFamily(const QString &n)
        :
        populated(false),
        fixedPitch(false),
        name(n), count(0), foundries(0)
        , bogusWritingSystems(false)
        , askedForFallback(false)
    {
        memset(writingSystems, 0, sizeof(writingSystems));
    }
    ~QtFontFamily() {
        while (count--)
            delete foundries[count];
        free(foundries);
    }

    bool populated : 1;
    bool fixedPitch : 1;

    QString name;
    QStringList aliases;
    int count;
    QtFontFoundry **foundries;

    bool bogusWritingSystems;
    QStringList fallbackFamilies;
    bool askedForFallback;
    unsigned char writingSystems[QFontDatabase::WritingSystemsCount];

    bool matchesFamilyName(const QString &familyName) const;
    QtFontFoundry *foundry(const QString &f, bool = false);

    void ensurePopulated();
};

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

bool QtFontFamily::matchesFamilyName(const QString &familyName) const
{
    return name.compare(familyName, Qt::CaseInsensitive) == 0 || aliases.contains(familyName, Qt::CaseInsensitive);
}

void QtFontFamily::ensurePopulated()
{
    if (populated)
        return;

    QGuiApplicationPrivate::platformIntegration()->fontDatabase()->populateFamily(name);
    Q_ASSERT(populated);
}

class QFontDatabasePrivate
{
public:
    QFontDatabasePrivate()
        : count(0), families(0), reregisterAppFonts(false)
    { }

    ~QFontDatabasePrivate() {
        free();
    }

    enum FamilyRequestFlags {
        RequestFamily = 0,
        EnsureCreated,
        EnsurePopulated
    };

    QtFontFamily *family(const QString &f, FamilyRequestFlags flags = EnsurePopulated);
    void free() {
        while (count--)
            delete families[count];
        ::free(families);
        families = 0;
        count = 0;
        // don't clear the memory fonts!
    }

    int count;
    QtFontFamily **families;



    struct ApplicationFont {
        QString fileName;
        QByteArray data;
        QStringList families;
    };
    QVector<ApplicationFont> applicationFonts;
    int addAppFont(const QByteArray &fontData, const QString &fileName);
    bool reregisterAppFonts;
    bool isApplicationFont(const QString &fileName);

    void invalidate();

    QStringList fallbackFamilies;
};

void QFontDatabasePrivate::invalidate()
{
    QFontCache::instance()->clear();
    free();
    QGuiApplicationPrivate::platformIntegration()->fontDatabase()->invalidate();
    emit static_cast<QGuiApplication *>(QCoreApplication::instance())->fontDatabaseChanged();
}

QtFontFamily *QFontDatabasePrivate::family(const QString &f, FamilyRequestFlags flags)
{
    QtFontFamily *fam = 0;

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
            fam = families[pos];
    }

    if (!fam && (flags & EnsureCreated)) {
        if (res < 0)
            pos++;

        // qDebug() << "adding family " << f.toLatin1() << " at " << pos << " total=" << count;
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

        fam = families[pos];
    }

    if (fam && (flags & EnsurePopulated))
        fam->ensurePopulated();

    return fam;
}



static const int scriptForWritingSystem[] = {
    QChar::Script_Common, // Any
    QChar::Script_Latin, // Latin
    QChar::Script_Greek, // Greek
    QChar::Script_Cyrillic, // Cyrillic
    QChar::Script_Armenian, // Armenian
    QChar::Script_Hebrew, // Hebrew
    QChar::Script_Arabic, // Arabic
    QChar::Script_Syriac, // Syriac
    QChar::Script_Thaana, // Thaana
    QChar::Script_Devanagari, // Devanagari
    QChar::Script_Bengali, // Bengali
    QChar::Script_Gurmukhi, // Gurmukhi
    QChar::Script_Gujarati, // Gujarati
    QChar::Script_Oriya, // Oriya
    QChar::Script_Tamil, // Tamil
    QChar::Script_Telugu, // Telugu
    QChar::Script_Kannada, // Kannada
    QChar::Script_Malayalam, // Malayalam
    QChar::Script_Sinhala, // Sinhala
    QChar::Script_Thai, // Thai
    QChar::Script_Lao, // Lao
    QChar::Script_Tibetan, // Tibetan
    QChar::Script_Myanmar, // Myanmar
    QChar::Script_Georgian, // Georgian
    QChar::Script_Khmer, // Khmer
    QChar::Script_Han, // SimplifiedChinese
    QChar::Script_Han, // TraditionalChinese
    QChar::Script_Han, // Japanese
    QChar::Script_Hangul, // Korean
    QChar::Script_Latin, // Vietnamese
    QChar::Script_Common, // Symbol
    QChar::Script_Ogham,  // Ogham
    QChar::Script_Runic, // Runic
    QChar::Script_Nko // Nko
};

Q_STATIC_ASSERT(sizeof(scriptForWritingSystem) / sizeof(scriptForWritingSystem[0]) == QFontDatabase::WritingSystemsCount);

Q_GUI_EXPORT int qt_script_for_writing_system(QFontDatabase::WritingSystem writingSystem)
{
    return scriptForWritingSystem[writingSystem];
}




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

static int match(int script, const QFontDef &request,
                 const QString &family_name, const QString &foundry_name, int force_encoding_id,
                 QtFontDesc *desc, const QList<int> &blacklisted);

static void initFontDef(const QtFontDesc &desc, const QFontDef &request, QFontDef *fontDef, bool multi)
{
    fontDef->family = desc.family->name;
    if (! desc.foundry->name.isEmpty() && desc.family->count > 1) {
        fontDef->family += QString::fromLatin1(" [");
        fontDef->family += desc.foundry->name;
        fontDef->family += QLatin1Char(']');
    }

    if (desc.style->smoothScalable
        || QGuiApplicationPrivate::platformIntegration()->fontDatabase()->fontsAlwaysScalable()
        || (desc.style->bitmapScalable && (request.styleStrategy & QFont::PreferMatch))) {
        fontDef->pixelSize = request.pixelSize;
    } else {
        fontDef->pixelSize = desc.size->pixelSize;
    }

    fontDef->styleHint     = request.styleHint;
    fontDef->styleStrategy = request.styleStrategy;

    if (!multi)
        fontDef->weight    = desc.style->key.weight;
    if (!multi)
        fontDef->style     = desc.style->key.style;
    fontDef->fixedPitch    = desc.family->fixedPitch;
    fontDef->stretch       = desc.style->key.stretch;
    fontDef->ignorePitch   = false;
}

static void getEngineData(const QFontPrivate *d, const QFontDef &def)
{
    // look for the requested font in the engine data cache
    d->engineData = QFontCache::instance()->findEngineData(def);
    if (!d->engineData) {
        // create a new one
        d->engineData = new QFontEngineData;
        QFontCache::instance()->insertEngineData(def, d->engineData);
    }
    d->engineData->ref.ref();
}

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

// used in qguiapplication.cpp
void qt_cleanupFontDatabase()
{
    QFontDatabasePrivate *db = privateDb();
    if (db)
        db->free();
}

// used in qfontengine_x11.cpp
QMutex *qt_fontdatabase_mutex()
{
    return fontDatabaseMutex();
}


void qt_registerFont(const QString &familyName, const QString &stylename,
                     const QString &foundryname, int weight,
                     QFont::Style style, int stretch, bool antialiased,
                     bool scalable, int pixelSize, bool fixedPitch,
                     const QSupportedWritingSystems &writingSystems, void *handle)
{
    QFontDatabasePrivate *d = privateDb();
//    qDebug() << "Adding font" << familyName << weight << style << pixelSize << antialiased;
    QtFontStyle::Key styleKey;
    styleKey.style = style;
    styleKey.weight = weight;
    styleKey.stretch = stretch;
    QtFontFamily *f = d->family(familyName, QFontDatabasePrivate::EnsureCreated);
    f->fixedPitch = fixedPitch;

    for (int i = 0; i < QFontDatabase::WritingSystemsCount; ++i) {
        if (writingSystems.supported(QFontDatabase::WritingSystem(i)))
            f->writingSystems[i] = QtFontFamily::Supported;
    }

    QtFontFoundry *foundry = f->foundry(foundryname, true);
    QtFontStyle *fontStyle = foundry->style(styleKey, stylename, true);
    fontStyle->smoothScalable = scalable;
    fontStyle->antialiased = antialiased;
    QtFontSize *size = fontStyle->pixelSize(pixelSize ? pixelSize : SMOOTH_SCALABLE, true);
    if (size->handle) {
        QPlatformIntegration *integration = QGuiApplicationPrivate::platformIntegration();
        if (integration)
            integration->fontDatabase()->releaseHandle(size->handle);
    }
    size->handle = handle;
    f->populated = true;
}

void qt_registerFontFamily(const QString &familyName)
{
    // Create uninitialized/unpopulated family
    privateDb()->family(familyName, QFontDatabasePrivate::EnsureCreated);
}

void qt_registerAliasToFontFamily(const QString &familyName, const QString &alias)
{
    if (alias.isEmpty())
        return;

    QFontDatabasePrivate *d = privateDb();
    QtFontFamily *f = d->family(familyName, QFontDatabasePrivate::RequestFamily);
    if (!f)
        return;

    if (f->aliases.contains(alias, Qt::CaseInsensitive))
        return;

    f->aliases.push_back(alias);
}

QString qt_resolveFontFamilyAlias(const QString &alias)
{
    if (!alias.isEmpty()) {
        const QFontDatabasePrivate *d = privateDb();
        for (int i = 0; i < d->count; ++i)
            if (d->families[i]->matchesFamilyName(alias))
                return d->families[i]->name;
    }
    return alias;
}

static QStringList fallbackFamilies(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script)
{
    QStringList retList = QGuiApplicationPrivate::platformIntegration()->fontDatabase()->fallbacksForFamily(family,style,styleHint,script);
    QFontDatabasePrivate *db = privateDb();

    QStringList::iterator i;
    for (i = retList.begin(); i != retList.end(); ++i) {
        bool contains = false;
        for (int j = 0; j < db->count; j++) {
            if (db->families[j]->matchesFamilyName(*i)) {
                contains = true;
                break;
            }
        }
        if (!contains) {
            i = retList.erase(i);
            i--;
        }
    }
    return retList;
}

static void registerFont(QFontDatabasePrivate::ApplicationFont *fnt);

static void initializeDb()
{
    QFontDatabasePrivate *db = privateDb();

    // init by asking for the platformfontdb for the first time or after invalidation
    if (!db->count)
        QGuiApplicationPrivate::platformIntegration()->fontDatabase()->populateFontDatabase();

    if (db->reregisterAppFonts) {
        for (int i = 0; i < db->applicationFonts.count(); i++) {
            if (!db->applicationFonts.at(i).families.isEmpty())
                registerFont(&db->applicationFonts[i]);
        }
        db->reregisterAppFonts = false;
    }
}

static inline void load(const QString & = QString(), int = -1)
{
    // Only initialize the database if it has been cleared or not initialized yet
    if (!privateDb()->count)
        initializeDb();
}

static
QFontEngine *loadSingleEngine(int script,
                              const QFontDef &request,
                              QtFontFoundry *foundry,
                              QtFontStyle *style, QtFontSize *size)
{
    Q_UNUSED(foundry);

    Q_ASSERT(size);
    QPlatformFontDatabase *pfdb = QGuiApplicationPrivate::platformIntegration()->fontDatabase();
    int pixelSize = size->pixelSize;
    if (!pixelSize || (style->smoothScalable && pixelSize == SMOOTH_SCALABLE)
        || pfdb->fontsAlwaysScalable()) {
        pixelSize = request.pixelSize;
    }

    QFontDef def = request;
    def.pixelSize = pixelSize;

    QFontCache::Key key(def,script);
    QFontEngine *engine = QFontCache::instance()->findEngine(key);
    if (!engine) {
        engine = pfdb->fontEngine(def, size->handle);
        if (engine) {
            Q_ASSERT(engine->type() != QFontEngine::Multi);
            // Also check for OpenType tables when using complex scripts
            if (!engine->supportsScript(QChar::Script(script))) {
                qWarning("  OpenType support missing for script %d", script);
                if (engine->ref.load() == 0)
                    delete engine;
                return 0;
            }

            QFontCache::instance()->insertEngine(key, engine);
        }
    }
    return engine;
}

static
QFontEngine *loadEngine(int script, const QFontDef &request,
                        QtFontFamily *family, QtFontFoundry *foundry,
                        QtFontStyle *style, QtFontSize *size)
{
    QFontEngine *engine = loadSingleEngine(script, request, foundry, style, size);
    Q_ASSERT(!engine || engine->type() != QFontEngine::Multi);
    if (engine && !(request.styleStrategy & QFont::NoFontMerging) && !engine->symbol) {
        // make sure that the db has all fallback families
        if (family && !family->askedForFallback) {
            QFont::Style fontStyle = QFont::Style(style->key.style);
            QFont::StyleHint styleHint = QFont::StyleHint(request.styleHint);
            if (styleHint == QFont::AnyStyle && request.fixedPitch)
                styleHint = QFont::TypeWriter;
            family->fallbackFamilies = fallbackFamilies(family->name, fontStyle, styleHint, QChar::Script(script));

            family->askedForFallback = true;
        }

        QStringList fallbacks = request.fallBackFamilies;
        if (family && !family->fallbackFamilies.isEmpty())
            fallbacks += family->fallbackFamilies;
        else
            fallbacks += privateDb()->fallbackFamilies;

        QPlatformFontDatabase *pfdb = QGuiApplicationPrivate::platformIntegration()->fontDatabase();
        QFontEngineMulti *pfMultiEngine = pfdb->fontEngineMulti(engine, QChar::Script(script));
        pfMultiEngine->setFallbackFamiliesList(fallbacks);
        engine = pfMultiEngine;

        // Cache Multi font engine as well in case we got the single
        // font engine when we are actually looking for a Multi one
        QFontCache::Key key(request, script, 1);
        QFontCache::instance()->insertEngine(key, engine);
    }

    return engine;
}

static void registerFont(QFontDatabasePrivate::ApplicationFont *fnt)
{
    QFontDatabasePrivate *db = privateDb();

    fnt->families = QGuiApplicationPrivate::platformIntegration()->fontDatabase()->addApplicationFont(fnt->data,fnt->fileName);

    db->reregisterAppFonts = true;
}

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


        // 4. find closest size match
        if (! size) {
            unsigned int distance = ~0u;
            for (int x = 0; x < style->count; ++x) {

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


        unsigned int this_score = 0x0000;
        enum {
            PitchMismatch       = 0x4000,
            StyleMismatch       = 0x2000,
            BitmapScaledPenalty = 0x1000,
            EncodingMismatch    = 0x0002
        };
        if (pitch != '*') {
            if ((pitch == 'm' && !family->fixedPitch)
                || (pitch == 'p' && family->fixedPitch))
                this_score += PitchMismatch;
        }
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
        } else {
            FM_DEBUG("          score %x no better than best %x", this_score, score);
        }
    }

    return score;
}

static bool matchFamilyName(const QString &familyName, QtFontFamily *f)
{
    if (familyName.isEmpty())
        return true;
    return f->matchesFamilyName(familyName);
}

/*!
    \internal

    Tries to find the best match for a given request and family/foundry
*/
static int match(int script, const QFontDef &request,
                 const QString &family_name, const QString &foundry_name, int force_encoding_id,
                 QtFontDesc *desc, const QList<int> &blacklistedFamilies)
{
    Q_UNUSED(force_encoding_id);
    int result = -1;

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

    desc->family = 0;
    desc->foundry = 0;
    desc->style = 0;
    desc->size = 0;
    desc->encoding = 0;
    desc->familyIndex = -1;

    unsigned int score = ~0u;

    load(family_name, script);

    const size_t writingSystem = std::find(scriptForWritingSystem, scriptForWritingSystem + QFontDatabase::WritingSystemsCount, script) - scriptForWritingSystem;

    QFontDatabasePrivate *db = privateDb();
    for (int x = 0; x < db->count; ++x) {
        if (blacklistedFamilies.contains(x))
            continue;
        QtFontDesc test;
        test.family = db->families[x];
        test.familyIndex = x;

        if (!matchFamilyName(family_name, test.family))
            continue;

        test.family->ensurePopulated();

        if (family_name.isEmpty())
            load(test.family->name, script);

        // Check if family is supported in the script we want
        if (script != QChar::Script_Common && !(test.family->writingSystems[writingSystem] & QtFontFamily::Supported))
            continue;

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

        if (newscore < score) {
            result = x;
            score = newscore;
            *desc = test;
        }
        if (newscore < 10) // xlfd instead of FT... just accept it
            break;
    }
    return result;
}

static QString styleStringHelper(int weight, QFont::Style style)
{
    QString result;
    if (weight >= QFont::Black)
        result = QCoreApplication::translate("QFontDatabase", "Black");
    else if (weight >= QFont::Bold)
        result = QCoreApplication::translate("QFontDatabase", "Bold");
    else if (weight >= QFont::DemiBold)
        result = QCoreApplication::translate("QFontDatabase", "Demi Bold");
    else if (weight < QFont::Normal)
        result = QCoreApplication::translate("QFontDatabase", "Light");

    if (style == QFont::StyleItalic)
        result += QLatin1Char(' ') + QCoreApplication::translate("QFontDatabase", "Italic");
    else if (style == QFont::StyleOblique)
        result += QLatin1Char(' ') + QCoreApplication::translate("QFontDatabase", "Oblique");

    if (result.isEmpty())
        result = QCoreApplication::translate("QFontDatabase", "Normal");

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
    \inmodule QtGui

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

    \snippet qfontdatabase/main.cpp 0
    \snippet qfontdatabase/main.cpp 1

    This example gets the list of font families, the list of
    styles for each family, and the point sizes that are available for
    each combination of family and style, displaying this information
    in a tree view.

    \sa QFont, QFontInfo, QFontMetrics, {Character Map Example}
*/

/*!
    Creates a font database object.
*/
QFontDatabase::QFontDatabase()
{
    if (!qApp || !QGuiApplicationPrivate::platformIntegration())
        qFatal("QFontDatabase: Must construct a QGuiApplication before accessing QFontDatabase");

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
    \enum QFontDatabase::SystemFont

    \value GeneralFont              The default system font.
    \value FixedFont                The fixed font that the system recommends.
    \value TitleFont                The system standard font for titles.
    \value SmallestReadableFont     The smallest readable system font.

    \since 5.2
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

    QList<WritingSystem> list;
    for (int i = 0; i < d->count; ++i) {
        QtFontFamily *family = d->families[i];
        family->ensurePopulated();

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
    std::sort(list.begin(), list.end());
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

    QStringList flist;
    for (int i = 0; i < d->count; i++) {
        QtFontFamily *f = d->families[i];
        if (f->populated && f->count == 0)
            continue;
        if (writingSystem != Any) {
            f->ensurePopulated();
            if (f->writingSystems[writingSystem] != QtFontFamily::Supported)
                continue;
        }
        if (!f->populated || f->count == 1) {
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
    Returns \c true if the font that has family \a family and style \a
    style is fixed pitch; otherwise returns \c false.
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
    return (f && f->fixedPitch);
}

/*!
    Returns \c true if the font that has family \a family and style \a
    style is a scalable bitmap font; otherwise returns \c false. Scaling
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
    Returns \c true if the font that has family \a family and style \a
    style is smoothly scalable; otherwise returns \c false. If this
    function returns \c true, it's safe to scale this font to any size,
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
    Returns \c true if the font that has family \a family and style \a
    style is scalable; otherwise returns \c false.

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
    Returns a list of the point sizes available for the font that has
    family \a family and style \a styleName. The list may be empty.

    \sa smoothSizes(), standardSizes()
*/
QList<int> QFontDatabase::pointSizes(const QString &family,
                                           const QString &styleName)
{
    if (QGuiApplicationPrivate::platformIntegration()->fontDatabase()->fontsAlwaysScalable())
        return standardSizes();

    bool smoothScalable = false;
    QString familyName, foundryName;
    parseFontName(family, foundryName, familyName);

    QMutexLocker locker(fontDatabaseMutex());

    QT_PREPEND_NAMESPACE(load)(familyName);

    QtFontStyle::Key styleKey(styleName);

    QList<int> sizes;

    QtFontFamily *fam = d->family(familyName);
    if (!fam) return sizes;


    const int dpi = qt_defaultDpiY(); // embedded

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
                    const uint pointSize = qRound(size->pixelSize * 72.0 / dpi);
                    if (! sizes.contains(pointSize))
                        sizes.append(pointSize);
                }
            }
        }
    }
 end:
    if (smoothScalable)
        return standardSizes();

    std::sort(sizes.begin(), sizes.end());
    return sizes;
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
    if (!f) return QGuiApplication::font();

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
        return QGuiApplication::font();

    QFont fnt(family, pointSize, s->key.weight);
    fnt.setStyle((QFont::Style)s->key.style);
    if (!s->styleName.isEmpty())
        fnt.setStyleName(s->styleName);
    return fnt;
}


/*!
    Returns the point sizes of a font that has family \a family and
    style \a styleName that will look attractive. The list may be empty.
    For non-scalable fonts and bitmap scalable fonts, this function
    is equivalent to pointSizes().

  \sa pointSizes(), standardSizes()
*/
QList<int> QFontDatabase::smoothSizes(const QString &family,
                                            const QString &styleName)
{
    if (QGuiApplicationPrivate::platformIntegration()->fontDatabase()->fontsAlwaysScalable())
        return standardSizes();

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

    const int dpi = qt_defaultDpiY(); // embedded

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
                    const uint pointSize = qRound(size->pixelSize * 72.0 / dpi);
                    if (! sizes.contains(pointSize))
                        sizes.append(pointSize);
                }
            }
        }
    }
 end:
    if (smoothScalable)
        return QFontDatabase::standardSizes();

    std::sort(sizes.begin(), sizes.end());
    return sizes;
}


/*!
    Returns a list of standard font sizes.

    \sa smoothSizes(), pointSizes()
*/
QList<int> QFontDatabase::standardSizes()
{
    return QGuiApplicationPrivate::platformIntegration()->fontDatabase()->standardSizes();
}


/*!
    Returns \c true if the font that has family \a family and style \a
    style is italic; otherwise returns \c false.

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
    Returns \c true if the font that has family \a family and style \a
    style is bold; otherwise returns \c false.

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
    return QCoreApplication::translate("QFontDatabase", name);
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

    \sa addApplicationFontFromData(), applicationFontFamilies(), removeApplicationFont()
*/
int QFontDatabase::addApplicationFont(const QString &fileName)
{
    QByteArray data;
    if (!QFileInfo(fileName).isNativePath()) {
        QFile f(fileName);
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

    \b{Note:} Adding application fonts on Unix/X11 platforms without fontconfig is
    currently not supported.

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
    \since 5.2

    Returns the most adequate font for a given \a type case for proper integration
    with the system's look and feel.

    \sa QGuiApplication::font()
*/

QFont QFontDatabase::systemFont(QFontDatabase::SystemFont type)
{
    const QFont *font = 0;
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme()) {
        switch (type) {
            case GeneralFont:
                font = theme->font(QPlatformTheme::SystemFont);
                break;
            case FixedFont:
                font = theme->font(QPlatformTheme::FixedFont);
                break;
            case TitleFont:
                font = theme->font(QPlatformTheme::TitleBarFont);
                break;
            case SmallestReadableFont:
                font = theme->font(QPlatformTheme::MiniFont);
                break;
        }
    }

    if (font)
        return *font;
    else if (QPlatformIntegration *integration = QGuiApplicationPrivate::platformIntegration())
        return integration->fontDatabase()->defaultFont();
    else
        return QFont();
}

/*!
    \fn bool QFontDatabase::removeApplicationFont(int id)
    \since 4.2

    Removes the previously loaded application font identified by \a
    id. Returns \c true if unloading of the font succeeded; otherwise
    returns \c false.

    \sa removeAllApplicationFonts(), addApplicationFont(),
        addApplicationFontFromData()
*/
bool QFontDatabase::removeApplicationFont(int handle)
{
    QMutexLocker locker(fontDatabaseMutex());

    QFontDatabasePrivate *db = privateDb();
    if (handle < 0 || handle >= db->applicationFonts.count())
        return false;

    db->applicationFonts[handle] = QFontDatabasePrivate::ApplicationFont();

    db->reregisterAppFonts = true;
    db->invalidate();
    return true;
}

/*!
    \fn bool QFontDatabase::removeAllApplicationFonts()
    \since 4.2

    Removes all application-local fonts previously added using addApplicationFont()
    and addApplicationFontFromData().

    Returns \c true if unloading of the fonts succeeded; otherwise
    returns \c false.

    \sa removeApplicationFont(), addApplicationFont(), addApplicationFontFromData()
*/
bool QFontDatabase::removeAllApplicationFonts()
{
    QMutexLocker locker(fontDatabaseMutex());

    QFontDatabasePrivate *db = privateDb();
    if (db->applicationFonts.isEmpty())
        return false;

    db->applicationFonts.clear();
    db->invalidate();
    return true;
}

/*!
    \fn bool QFontDatabase::supportsThreadedFontRendering()
    \since 4.4
    \deprecated

    Returns \c true if font rendering is supported outside the GUI
    thread, false otherwise. In other words, a return value of false
    means that all QPainter::drawText() calls outside the GUI thread
    will not produce readable output.

    As of 5.0, always returns \c true.

    \sa {Thread-Support in Qt Modules#Painting In Threads}{Painting In Threads}
*/
// QT_DEPRECATED_SINCE(5, 2)
bool QFontDatabase::supportsThreadedFontRendering()
{
    return true;
}

/*!
    \internal
*/
QFontEngine *
QFontDatabase::findFont(int script, const QFontPrivate *fp,
                        const QFontDef &request, bool multi)
{
    QMutexLocker locker(fontDatabaseMutex());

    const int force_encoding_id = -1;

    if (!privateDb()->count)
        initializeDb();

    QFontEngine *engine;
    QFontCache::Key key(request, script, multi ? 1 : 0);
    engine = QFontCache::instance()->findEngine(key);
    if (engine) {
        FM_DEBUG("Cache hit level 1");
        return engine;
    }

    QString family_name, foundry_name;

    parseFontName(request.family, foundry_name, family_name);

    if (qt_enable_test_font && request.family == QLatin1String("__Qt__Box__Engine__")) {
        engine =new QTestFontEngine(request.pixelSize);
        engine->fontDef = request;
    }

    QtFontDesc desc;
    QList<int> blackListed;
    int index = match(script, request, family_name, foundry_name, force_encoding_id, &desc, blackListed);
    if (index >= 0) {
        engine = loadEngine(script, request, desc.family, desc.foundry, desc.style, desc.size);
        if (!engine)
            blackListed.append(index);
    } else {
        FM_DEBUG("  NO MATCH FOUND\n");
    }

    if (engine && engine->type() != QFontEngine::TestFontEngine) {
        initFontDef(desc, request, &engine->fontDef, engine->type() == QFontEngine::Multi);

        if (fp) {
            QFontDef def = request;
            if (def.family.isEmpty()) {
                def.family = fp->request.family;
                def.family = def.family.left(def.family.indexOf(QLatin1Char(',')));
            }
        }
    }

    if (!engine) {
        if (!request.family.isEmpty()) {
            QStringList fallbacks = request.fallBackFamilies
                                  + fallbackFamilies(request.family,
                                                     QFont::Style(request.style),
                                                     QFont::StyleHint(request.styleHint),
                                                     QChar::Script(script));
            if (script > QChar::Script_Common)
                fallbacks += QString(); // Find the first font matching the specified script.

            for (int i = 0; !engine && i < fallbacks.size(); i++) {
                QFontDef def = request;
                def.family = fallbacks.at(i);
                QFontCache::Key key(def, script, multi ? 1 : 0);
                engine = QFontCache::instance()->findEngine(key);
                if (!engine) {
                    QtFontDesc desc;
                    do {
                        index = match(script, def, def.family, QLatin1String(""), 0, &desc, blackListed);
                        if (index >= 0) {
                            QFontDef loadDef = def;
                            if (loadDef.family.isEmpty())
                                loadDef.family = desc.family->name;
                            engine = loadEngine(script, loadDef, desc.family, desc.foundry, desc.style, desc.size);
                            if (engine)
                                initFontDef(desc, loadDef, &engine->fontDef, engine->type() == QFontEngine::Multi);
                            else
                                blackListed.append(index);
                        }
                    } while (index >= 0 && !engine);
                }
            }
        }

        if (!engine)
            engine = new QFontEngineBox(request.pixelSize);

        FM_DEBUG("returning box engine");
    }

    if (fp && fp->dpi > 0) {
        engine->fontDef.pointSize = qreal(double((engine->fontDef.pixelSize * 72) / fp->dpi));
    } else {
        engine->fontDef.pointSize = request.pointSize;
    }

    return engine;
}

void QFontDatabase::load(const QFontPrivate *d, int script)
{
    QFontDef req = d->request;

    if (req.pixelSize == -1) {
        req.pixelSize = floor(((req.pointSize * d->dpi) / 72) * 100 + 0.5) / 100;
        req.pixelSize = qRound(req.pixelSize);
    }
    if (req.pointSize < 0)
        req.pointSize = req.pixelSize*72.0/d->dpi;
    if (req.weight == 0)
        req.weight = QFont::Normal;
    if (req.stretch == 0)
        req.stretch = 100;

    // Until we specifically asked not to, try looking for Multi font engine
    // first, the last '1' indicates that we want Multi font engine instead
    // of single ones
    bool multi = !(req.styleStrategy & QFont::NoFontMerging);
    QFontCache::Key key(req, script, multi ? 1 : 0);

    if (!d->engineData)
        getEngineData(d, req);

    // the cached engineData could have already loaded the engine we want
    if (d->engineData->engines[script])
        return;

    QFontEngine *fe = QFontCache::instance()->findEngine(key);

    // list of families to try
    QStringList family_list;

    if (!req.family.isEmpty()) {
        QStringList familiesForRequest = familyList(req);

        // Add primary selection
        family_list << familiesForRequest.takeFirst();

        // Fallbacks requested in font request
        req.fallBackFamilies = familiesForRequest;

        // add the default family
        QString defaultFamily = QGuiApplication::font().family();
        if (! family_list.contains(defaultFamily))
            family_list << defaultFamily;

    }

    // null family means find the first font matching the specified script
    family_list << QString();

    QStringList::ConstIterator it = family_list.constBegin(), end = family_list.constEnd();
    for (; !fe && it != end; ++it) {
        req.family = *it;

        fe = QFontDatabase::findFont(script, d, req, multi);
        if (fe && (fe->type()==QFontEngine::Box) && !req.family.isEmpty()) {
            if (fe->ref.load() == 0)
                delete fe;

            fe = 0;
        }

        // No need to check requested fallback families again
        req.fallBackFamilies.clear();
    }

    if (fe->symbol || (d->request.styleStrategy & QFont::NoFontMerging)) {
        for (int i = 0; i < QChar::ScriptCount; ++i) {
            if (!d->engineData->engines[i]) {
                d->engineData->engines[i] = fe;
                fe->ref.ref();
            }
        }
    } else {
        d->engineData->engines[script] = fe;
        fe->ref.ref();
    }
}

QString QFontDatabase::resolveFontFamilyAlias(const QString &family)
{
    return QGuiApplicationPrivate::platformIntegration()->fontDatabase()->resolveFontFamilyAlias(family);
}


QT_END_NAMESPACE

