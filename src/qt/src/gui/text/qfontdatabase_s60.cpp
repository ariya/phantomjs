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

#include <private/qapplication_p.h>
#include "qdir.h"
#include "qfont_p.h"
#include "qfontengine_s60_p.h"
#include "qabstractfileengine.h"
#include "qdesktopservices.h"
#include "qtemporaryfile.h"
#include "qtextcodec.h"
#include <private/qpixmap_raster_symbian_p.h>
#include <private/qt_s60_p.h>
#include "qendian.h"
#include <private/qcore_symbian_p.h>
#ifdef QT_NO_FREETYPE
#include <openfont.h>
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <graphics/openfontrasterizer.h> // COpenFontRasterizer has moved to a new header file
#endif // SYMBIAN_ENABLE_SPLIT_HEADERS
#endif // QT_NO_FREETYPE

#if !defined(SYMBIAN_VERSION_9_4) && !defined(SYMBIAN_VERSION_9_3) && !defined(SYMBIAN_VERSION_9_2)
#define SYMBIAN_LINKEDFONTS_SUPPORTED
#endif // !SYMBIAN_VERSION_9_4

QT_BEGIN_NAMESPACE

bool qt_symbian_isLinkedFont(const TDesC &typefaceName) // Also used in qfont_s60.cpp
{
    bool isLinkedFont = false;
#ifdef SYMBIAN_LINKEDFONTS_SUPPORTED
    const QString name((const QChar*)typefaceName.Ptr(), typefaceName.Length());
    isLinkedFont = name.endsWith(QLatin1String("LF")) && name == name.toUpper();
#endif // SYMBIAN_LINKEDFONTS_SUPPORTED
    return isLinkedFont;
}

QStringList qt_symbian_fontFamiliesOnFontServer() // Also used in qfont_s60.cpp
{
    QStringList result;
    QSymbianFbsHeapLock lock(QSymbianFbsHeapLock::Unlock);
    const int numTypeFaces = S60->screenDevice()->NumTypefaces();
    for (int i = 0; i < numTypeFaces; i++) {
        TTypefaceSupport typefaceSupport;
        S60->screenDevice()->TypefaceSupport(typefaceSupport, i);
        const QString familyName((const QChar *)typefaceSupport.iTypeface.iName.Ptr(), typefaceSupport.iTypeface.iName.Length());
        result.append(familyName);
    }
    lock.relock();
    return result;
}

QFileInfoList alternativeFilePaths(const QString &path, const QStringList &nameFilters,
    QDir::Filters filters = QDir::NoFilter, QDir::SortFlags sort = QDir::NoSort,
    bool uniqueFileNames = true)
{
    QFileInfoList result;

    // Prepare a 'soft to hard' drive list: W:, X: ... A:, Z:
    QStringList driveStrings;
    foreach (const QFileInfo &drive, QDir::drives())
        driveStrings.append(drive.absolutePath());
    driveStrings.sort();
    const QString zDriveString(QLatin1String("Z:/"));
    driveStrings.removeAll(zDriveString);
    driveStrings.prepend(zDriveString);

    QStringList uniqueFileNameList;
    for (int i = driveStrings.count() - 1; i >= 0; --i) {
        const QDir dirOnDrive(driveStrings.at(i) + path);
        const QFileInfoList entriesOnDrive = dirOnDrive.entryInfoList(nameFilters, filters, sort);
        if (uniqueFileNames) {
            foreach(const QFileInfo &entry, entriesOnDrive) {
                if (!uniqueFileNameList.contains(entry.fileName())) {
                    uniqueFileNameList.append(entry.fileName());
                    result.append(entry);
                }
            }
        } else {
            result.append(entriesOnDrive);
        }
    }
    return result;
}

#ifdef QT_NO_FREETYPE
class QSymbianFontDatabaseExtrasImplementation : public QSymbianFontDatabaseExtras
{
public:
    QSymbianFontDatabaseExtrasImplementation();
    ~QSymbianFontDatabaseExtrasImplementation();

    const QSymbianTypeFaceExtras *extras(const QString &typeface, bool bold, bool italic) const;
    void removeAppFontData(QFontDatabasePrivate::ApplicationFont *fnt);
    static inline bool appFontLimitReached();
    TUid addFontFileToFontStore(const QFileInfo &fontFileInfo);
    static void clear();

    static inline QString tempAppFontFolder();
    static const QString appFontMarkerPrefix;
    static QString appFontMarker(); // 'qaf<shortUid[+shortPid]>'

    struct CFontFromFontStoreReleaser {
        static inline void cleanup(CFont *font)
        {
            if (!font)
                return;
            const QSymbianFontDatabaseExtrasImplementation *dbExtras =
                    static_cast<const QSymbianFontDatabaseExtrasImplementation*>(privateDb()->symbianExtras);
            dbExtras->m_store->ReleaseFont(font);
        }
    };

    struct CFontFromScreenDeviceReleaser {
        static inline void cleanup(CFont *font)
        {
            if (!font)
                return;
            S60->screenDevice()->ReleaseFont(font);
        }
    };

// m_heap, m_store, m_rasterizer and m_extras are used if Symbian
// does not provide the Font Table API
    RHeap* m_heap;
    CFontStore *m_store;
    COpenFontRasterizer *m_rasterizer;
    mutable QList<const QSymbianTypeFaceExtras *> m_extras;

    mutable QSet<QString> m_applicationFontFamilies;
};

const QString QSymbianFontDatabaseExtrasImplementation::appFontMarkerPrefix =
        QLatin1String("Q");

inline QString QSymbianFontDatabaseExtrasImplementation::tempAppFontFolder()
{
    return QDir::toNativeSeparators(QDir::tempPath()) + QLatin1Char('\\');
}

QString QSymbianFontDatabaseExtrasImplementation::appFontMarker()
{
    static QString result;
    if (result.isEmpty()) {
        quint16 id = 0;
        if (QSymbianTypeFaceExtras::symbianFontTableApiAvailable()) {
            // We are allowed to load app fonts even from previous, crashed runs
            // of this application, since we can access the font tables.
            const quint32 uid = RProcess().Type().MostDerived().iUid;
            id = static_cast<quint16>(uid + (uid >> 16));
        } else {
            // If no font table Api is available, we must not even load a font
            // from a previous (crashed) run of this application. Reason: we
            // won't get the font tables, they are not in the CFontStore.
            // So, we use the pid, for more uniqueness.
            id = static_cast<quint16>(RProcess().Id().Id());
        }
        result = appFontMarkerPrefix + QString::fromLatin1("%1").arg(id & 0x7fff, 3, 32, QLatin1Char('0'));
        Q_ASSERT(appFontMarkerPrefix.length() == 1 && result.length() == 4);
    }
    return result;
}

static inline bool qt_symbian_fontNameHasAppFontMarker(const QString &fontName)
{
    const int idLength = 3; // Keep in sync with id length in appFontMarker().
    const QString &prefix = QSymbianFontDatabaseExtrasImplementation::appFontMarkerPrefix;
    if (fontName.length() < prefix.length() + idLength
            || fontName.mid(fontName.length() - idLength - prefix.length(), prefix.length()) != prefix)
        return false;
    // Testing if the the id is base32 data
    for (int i = fontName.length() - idLength; i < fontName.length(); ++i) {
        const QChar &c = fontName.at(i);
        if (!(c >= QLatin1Char('0') && c <= QLatin1Char('9')
              || c >= QLatin1Char('a') && c <= QLatin1Char('v')))
            return false;
    }
    return true;
}

// If fontName is an application font of this app, prepend the app font marker
QString qt_symbian_fontNameWithAppFontMarker(const QString &fontName)
{
    QFontDatabasePrivate *db = privateDb();
    Q_ASSERT(db);
    const QSymbianFontDatabaseExtrasImplementation *dbExtras =
            static_cast<const QSymbianFontDatabaseExtrasImplementation*>(db->symbianExtras);
    return dbExtras->m_applicationFontFamilies.contains(fontName) ?
                fontName + QSymbianFontDatabaseExtrasImplementation::appFontMarker()
              : fontName;
}

static inline QString qt_symbian_appFontNameWithoutMarker(const QString &markedFontName)
{
    return markedFontName.left(markedFontName.length()
                               - QSymbianFontDatabaseExtrasImplementation::appFontMarker().length());
}

QSymbianFontDatabaseExtrasImplementation::QSymbianFontDatabaseExtrasImplementation()
{
    if (!QSymbianTypeFaceExtras::symbianFontTableApiAvailable()) {
        QStringList filters;
        filters.append(QLatin1String("*.ttf"));
        filters.append(QLatin1String("*.ccc"));
        filters.append(QLatin1String("*.ltt"));
        const QFileInfoList fontFiles = alternativeFilePaths(QLatin1String("resource\\Fonts"), filters);

        const TInt heapMinLength = 0x1000;
        const TInt heapMaxLength = qMax(0x20000 * fontFiles.count(), heapMinLength);
        m_heap = User::ChunkHeap(NULL, heapMinLength, heapMaxLength);
        QT_TRAP_THROWING(
            m_store = CFontStore::NewL(m_heap);
            m_rasterizer = COpenFontRasterizer::NewL(TUid::Uid(0x101F7F5E));
            CleanupStack::PushL(m_rasterizer);
            m_store->InstallRasterizerL(m_rasterizer);
            CleanupStack::Pop(m_rasterizer););

        foreach (const QFileInfo &fontFileInfo, fontFiles)
            addFontFileToFontStore(fontFileInfo);
    }
}

void QSymbianFontDatabaseExtrasImplementation::clear()
{
    QFontDatabasePrivate *db = privateDb();
    if (!db)
        return;
    const QSymbianFontDatabaseExtrasImplementation *dbExtras =
            static_cast<const QSymbianFontDatabaseExtrasImplementation*>(db->symbianExtras);
    if (!dbExtras)
        return; // initializeDb() has never been called
    QSymbianTypeFaceExtrasHash &extrasHash = S60->fontData();
    if (QSymbianTypeFaceExtras::symbianFontTableApiAvailable()) {
        qDeleteAll(extrasHash);
    } else {
        typedef QList<const QSymbianTypeFaceExtras *>::iterator iterator;
        for (iterator p = dbExtras->m_extras.begin(); p != dbExtras->m_extras.end(); ++p) {
            dbExtras->m_store->ReleaseFont((*p)->fontOwner());
            delete *p;
        }
        dbExtras->m_extras.clear();
    }
    extrasHash.clear();
}

void qt_cleanup_symbianFontDatabase()
{
    static bool cleanupDone = false;
    if (cleanupDone)
        return;
    cleanupDone = true;

    QFontDatabasePrivate *db = privateDb();
    if (!db)
        return;

    QSymbianFontDatabaseExtrasImplementation::clear();

    if (!db->applicationFonts.isEmpty()) {
        QFontDatabase::removeAllApplicationFonts();
        // We remove the left over temporary font files of Qt application.
        // Active fonts are undeletable since the font server holds a handle
        // on them, so we do not need to worry to delete other running
        // applications' fonts.
        const QDir dir(QSymbianFontDatabaseExtrasImplementation::tempAppFontFolder());
        const QStringList filter(
                QSymbianFontDatabaseExtrasImplementation::appFontMarkerPrefix + QLatin1String("*.ttf"));
        foreach (const QFileInfo &ttfFile, dir.entryInfoList(filter))
            QFile(ttfFile.absoluteFilePath()).remove();
        db->applicationFonts.clear();
    }
}

QSymbianFontDatabaseExtrasImplementation::~QSymbianFontDatabaseExtrasImplementation()
{
    qt_cleanup_symbianFontDatabase();
    if (!QSymbianTypeFaceExtras::symbianFontTableApiAvailable()) {
        delete m_store;
        m_heap->Close();
    }
}

#ifndef FNTSTORE_H_INLINES_SUPPORT_FMM
/*
 Workaround: fntstore.h has an inlined function 'COpenFont* CBitmapFont::OpenFont()'
 that returns a private data member. The header will change between SDKs. But Qt has
 to build on any SDK version and run on other versions of Symbian OS.
 This function performs the needed pointer arithmetic to get the right COpenFont*
*/
COpenFont* OpenFontFromBitmapFont(const CBitmapFont* aBitmapFont)
{
    const TInt offsetIOpenFont = 92; // '_FOFF(CBitmapFont, iOpenFont)' ..if iOpenFont weren't private
    const TUint valueIOpenFont = *(TUint*)PtrAdd(aBitmapFont, offsetIOpenFont);
    return (valueIOpenFont & 1) ?
            (COpenFont*)PtrAdd(aBitmapFont, valueIOpenFont & ~1) : // New behavior: iOpenFont is offset
            (COpenFont*)valueIOpenFont; // Old behavior: iOpenFont is pointer
}
#endif // FNTSTORE_H_INLINES_SUPPORT_FMM

const QSymbianTypeFaceExtras *QSymbianFontDatabaseExtrasImplementation::extras(const QString &aTypeface,
                                                                               bool bold, bool italic) const
{
    QSymbianTypeFaceExtrasHash &extrasHash = S60->fontData();
    if (extrasHash.isEmpty() && QThread::currentThread() != QApplication::instance()->thread())
        S60->addThreadLocalReleaseFunc(clear);
    const QString typeface = qt_symbian_fontNameWithAppFontMarker(aTypeface);
    const QString searchKey = typeface + QString::number(int(bold)) + QString::number(int(italic));
    if (!extrasHash.contains(searchKey)) {
        TFontSpec searchSpec(qt_QString2TPtrC(typeface), 1);
        if (bold)
            searchSpec.iFontStyle.SetStrokeWeight(EStrokeWeightBold);
        if (italic)
            searchSpec.iFontStyle.SetPosture(EPostureItalic);

        CFont* font = NULL;
        if (QSymbianTypeFaceExtras::symbianFontTableApiAvailable()) {
            const TInt err = S60->screenDevice()->GetNearestFontToDesignHeightInPixels(font, searchSpec);
            Q_ASSERT(err == KErrNone && font);
            QScopedPointer<CFont, CFontFromScreenDeviceReleaser> sFont(font);
            QSymbianTypeFaceExtras *extras = new QSymbianTypeFaceExtras(font);
            sFont.take();
            extrasHash.insert(searchKey, extras);
        } else {
            const TInt err = m_store->GetNearestFontToDesignHeightInPixels(font, searchSpec);
            Q_ASSERT(err == KErrNone && font);
            const CBitmapFont *bitmapFont = static_cast<CBitmapFont*>(font);
            COpenFont *openFont =
#ifdef FNTSTORE_H_INLINES_SUPPORT_FMM
                bitmapFont->OpenFont();
#else // FNTSTORE_H_INLINES_SUPPORT_FMM
                OpenFontFromBitmapFont(bitmapFont);
#endif // FNTSTORE_H_INLINES_SUPPORT_FMM
            const TOpenFontFaceAttrib* const attrib = openFont->FaceAttrib();
            const QString foundKey =
                    QString((const QChar*)attrib->FullName().Ptr(), attrib->FullName().Length());
            if (!extrasHash.contains(foundKey)) {
                QScopedPointer<CFont, CFontFromFontStoreReleaser> sFont(font);
                QSymbianTypeFaceExtras *extras = new QSymbianTypeFaceExtras(font, openFont);
                sFont.take();
                m_extras.append(extras);
                extrasHash.insert(searchKey, extras);
                extrasHash.insert(foundKey, extras);
            } else {
                m_store->ReleaseFont(font);
                extrasHash.insert(searchKey, extrasHash.value(foundKey));
            }
        }
    }
    return extrasHash.value(searchKey);
}

void QSymbianFontDatabaseExtrasImplementation::removeAppFontData(
    QFontDatabasePrivate::ApplicationFont *fnt)
{
    clear();
    if (!QSymbianTypeFaceExtras::symbianFontTableApiAvailable()
            && fnt->fontStoreFontFileUid.iUid != 0)
        m_store->RemoveFile(fnt->fontStoreFontFileUid);
    if (!fnt->families.isEmpty())
        m_applicationFontFamilies.remove(fnt->families.first());
    if (fnt->screenDeviceFontFileId != 0)
        S60->screenDevice()->RemoveFile(fnt->screenDeviceFontFileId);
    QFile::remove(fnt->temporaryFileName);
    *fnt = QFontDatabasePrivate::ApplicationFont();
}

bool QSymbianFontDatabaseExtrasImplementation::appFontLimitReached()
{
    QFontDatabasePrivate *db = privateDb();
    if (!db)
        return false;
    const int maxAppFonts = 5;
    int registeredAppFonts = 0;
    foreach (const QFontDatabasePrivate::ApplicationFont &appFont, db->applicationFonts)
        if (!appFont.families.isEmpty() && ++registeredAppFonts == maxAppFonts)
            return true;
    return false;
}

TUid QSymbianFontDatabaseExtrasImplementation::addFontFileToFontStore(const QFileInfo &fontFileInfo)
{
    Q_ASSERT(!QSymbianTypeFaceExtras::symbianFontTableApiAvailable());
    const QString fontFile = QDir::toNativeSeparators(fontFileInfo.absoluteFilePath());
    const TPtrC fontFilePtr(qt_QString2TPtrC(fontFile));
    TUid fontUid = {0};
    TRAP_IGNORE(fontUid = m_store->AddFileL(fontFilePtr));
    return fontUid;
}

#else // QT_NO_FREETYPE
class QFontEngineFTS60 : public QFontEngineFT
{
public:
    QFontEngineFTS60(const QFontDef &fd);
};

QFontEngineFTS60::QFontEngineFTS60(const QFontDef &fd)
    : QFontEngineFT(fd)
{
    default_hint_style = HintFull;
}
#endif // QT_NO_FREETYPE

/*
 QFontEngineS60::pixelsToPoints, QFontEngineS60::pointsToPixels, QFontEngineMultiS60::QFontEngineMultiS60
 and QFontEngineMultiS60::QFontEngineMultiS60 should be in qfontengine_s60.cpp. But since also the
 Freetype based font rendering need them, they are here.
*/
qreal QFontEngineS60::pixelsToPoints(qreal pixels, Qt::Orientation orientation)
{
    CWsScreenDevice* device = S60->screenDevice();
    return (orientation == Qt::Horizontal?
        device->HorizontalPixelsToTwips(pixels)
        :device->VerticalPixelsToTwips(pixels)) / KTwipsPerPoint;
}

qreal QFontEngineS60::pointsToPixels(qreal points, Qt::Orientation orientation)
{
    CWsScreenDevice* device = S60->screenDevice();
    const int twips = points * KTwipsPerPoint;
    return orientation == Qt::Horizontal?
        device->HorizontalTwipsToPixels(twips)
        :device->VerticalTwipsToPixels(twips);
}

QFontEngineMultiS60::QFontEngineMultiS60(QFontEngine *first, int script, const QStringList &fallbackFamilies)
    : QFontEngineMulti(fallbackFamilies.size() + 1)
    , m_script(script)
    , m_fallbackFamilies(fallbackFamilies)
{
    engines[0] = first;
    first->ref.ref();
    fontDef = engines[0]->fontDef;
}

void QFontEngineMultiS60::loadEngine(int at)
{
    Q_ASSERT(at < engines.size());
    Q_ASSERT(engines.at(at) == 0);

    QFontDef request = fontDef;
    request.styleStrategy |= QFont::NoFontMerging;
    request.family = m_fallbackFamilies.at(at-1);
    engines[at] = QFontDatabase::findFont(m_script,
                                          /*fontprivate*/0,
                                          request);
    Q_ASSERT(engines[at]);
}

#ifdef QT_NO_FREETYPE
static bool registerScreenDeviceFont(int screenDeviceFontIndex,
                                     const QSymbianFontDatabaseExtrasImplementation *dbExtras)
{
    TTypefaceSupport typefaceSupport;
    S60->screenDevice()->TypefaceSupport(typefaceSupport, screenDeviceFontIndex);

    if (qt_symbian_isLinkedFont(typefaceSupport.iTypeface.iName))
        return false;

    QString familyName((const QChar*)typefaceSupport.iTypeface.iName.Ptr(), typefaceSupport.iTypeface.iName.Length());
    if (qt_symbian_fontNameHasAppFontMarker(familyName)) {
        const QString &marker = QSymbianFontDatabaseExtrasImplementation::appFontMarker();
        if (familyName.endsWith(marker)) {
            familyName = qt_symbian_appFontNameWithoutMarker(familyName);
            dbExtras->m_applicationFontFamilies.insert(familyName);
        } else {
            return false; // This was somebody else's application font. Skip it.
        }
    }

    CFont *font; // We have to get a font instance in order to know all the details
    TFontSpec fontSpec(typefaceSupport.iTypeface.iName, 11);
    if (S60->screenDevice()->GetNearestFontInPixels(font, fontSpec) != KErrNone)
        return false;
    QScopedPointer<CFont, QSymbianFontDatabaseExtrasImplementation::CFontFromScreenDeviceReleaser> sFont(font);
    if (font->TypeUid() != KCFbsFontUid)
        return false;
    TOpenFontFaceAttrib faceAttrib;
    const CFbsFont *cfbsFont = static_cast<const CFbsFont *>(font);
    cfbsFont->GetFaceAttrib(faceAttrib);

    QtFontStyle::Key styleKey;
    styleKey.style = faceAttrib.IsItalic()?QFont::StyleItalic:QFont::StyleNormal;
    styleKey.weight = faceAttrib.IsBold()?QFont::Bold:QFont::Normal;

    QtFontFamily *family = privateDb()->family(familyName, true);
    family->fixedPitch = faceAttrib.IsMonoWidth();
    QtFontFoundry *foundry = family->foundry(QString(), true);
    QtFontStyle *style = foundry->style(styleKey, QString(), true);
    style->smoothScalable = typefaceSupport.iIsScalable;
    style->pixelSize(0, true);

    const QSymbianTypeFaceExtras *typeFaceExtras =
            dbExtras->extras(familyName, faceAttrib.IsBold(), faceAttrib.IsItalic());
    const QByteArray os2Table = typeFaceExtras->getSfntTable(MAKE_TAG('O', 'S', '/', '2'));
    const unsigned char* data = reinterpret_cast<const unsigned char*>(os2Table.constData());
    const unsigned char* ulUnicodeRange = data + 42;
    quint32 unicodeRange[4] = {
        qFromBigEndian<quint32>(ulUnicodeRange),
        qFromBigEndian<quint32>(ulUnicodeRange + 4),
        qFromBigEndian<quint32>(ulUnicodeRange + 8),
        qFromBigEndian<quint32>(ulUnicodeRange + 12)
    };
    const unsigned char* ulCodePageRange = data + 78;
    quint32 codePageRange[2] = {
        qFromBigEndian<quint32>(ulCodePageRange),
        qFromBigEndian<quint32>(ulCodePageRange + 4)
    };
    const QList<QFontDatabase::WritingSystem> writingSystems =
        qt_determine_writing_systems_from_truetype_bits(unicodeRange, codePageRange);
    foreach (const QFontDatabase::WritingSystem system, writingSystems)
        family->writingSystems[system] = QtFontFamily::Supported;
    return true;
}
#endif

static void initializeDb()
{
    QFontDatabasePrivate *db = privateDb();
    if(!db || db->count)
        return;

#ifdef QT_NO_FREETYPE
    if (!db->symbianExtras)
        db->symbianExtras = new QSymbianFontDatabaseExtrasImplementation;

    QSymbianFbsHeapLock lock(QSymbianFbsHeapLock::Unlock);

    const int numTypeFaces = S60->screenDevice()->NumTypefaces();
    const QSymbianFontDatabaseExtrasImplementation *dbExtras =
            static_cast<const QSymbianFontDatabaseExtrasImplementation*>(db->symbianExtras);
    for (int i = 0; i < numTypeFaces; i++)
        registerScreenDeviceFont(i, dbExtras);

    // We have to clear/release all CFonts, here, in case one of the fonts is
    // an application font of another running Qt app. Otherwise the other Qt app
    // cannot remove it's application font, anymore -> "Zombie Font".
    QSymbianFontDatabaseExtrasImplementation::clear();

    lock.relock();

#else // QT_NO_FREETYPE
    QDir dir(QDesktopServices::storageLocation(QDesktopServices::FontsLocation));
    dir.setNameFilters(QStringList() << QLatin1String("*.ttf")
                       << QLatin1String("*.ttc") << QLatin1String("*.pfa")
                       << QLatin1String("*.pfb"));
    for (int i = 0; i < int(dir.count()); ++i) {
        const QByteArray file = QFile::encodeName(dir.absoluteFilePath(dir[i]));
        db->addTTFile(file);
    }
#endif // QT_NO_FREETYPE
}

static inline void load(const QString &family = QString(), int script = -1)
{
    Q_UNUSED(family)
    Q_UNUSED(script)
    initializeDb();
}

struct OffsetTable {
    quint32 sfntVersion;
    quint16 numTables, searchRange, entrySelector, rangeShift;
};

struct TableRecord {
    quint32 tag, checkSum, offset, length;
};

struct NameTableHead {
    quint16 format, count, stringOffset;
};

struct NameRecord {
    quint16 platformID, encodingID, languageID, nameID, length, offset;
};

static quint32 ttfCalcChecksum(const char *data, quint32 bytesCount)
{
    quint32 result = 0;
    const quint32 *ptr = reinterpret_cast<const quint32*>(data);
    const quint32 *endPtr =
            ptr + (bytesCount + sizeof(quint32) - 1) / sizeof(quint32);
    while (ptr < endPtr) {
        const quint32 unit32Value = *ptr++;
        result += qFromBigEndian(unit32Value);
    }
    return result;
}

static inline quint32 toDWordBoundary(quint32 value)
{
    return (value + 3) & ~3;
}

static inline quint32 dWordPadding(quint32 value)
{
    return (4 - (value & 3)) & 3;
}

static inline bool ttfMarkNameTable(QByteArray &table, const QString &marker)
{
    const quint32 tableLength = static_cast<quint32>(table.size());

    if (tableLength > 50000 // hard limit
            || tableLength < sizeof(NameTableHead)) // corrupt name table
        return false;

    const NameTableHead *head = reinterpret_cast<const NameTableHead*>(table.constData());
    const quint16 count = qFromBigEndian(head->count);
    const quint16 stringOffset = qFromBigEndian(head->stringOffset);
    if (count > 200 // hard limit
            || stringOffset >= tableLength // corrupt name table
            || sizeof(NameTableHead) + count * sizeof(NameRecord) >= tableLength) // corrupt name table
        return false;

    QTextEncoder encoder(QTextCodec::codecForName("UTF-16BE"), QTextCodec::IgnoreHeader);
    const QByteArray markerUtf16BE = encoder.fromUnicode(marker);
    const QByteArray markerAscii = marker.toAscii();

    QByteArray markedTable;
    markedTable.reserve(tableLength + marker.length() * 20); // Original size plus some extra
    markedTable.append(table, stringOffset);
    QByteArray markedStrings;
    quint32 stringDataCount = stringOffset;
    for (quint16 i = 0; i < count; ++i) {
        const quint32 nameRecordOffset = sizeof(NameTableHead) + sizeof(NameRecord) * i;
        NameRecord *nameRecord =
                reinterpret_cast<NameRecord*>(markedTable.data() + nameRecordOffset);
        const quint16 nameID = qFromBigEndian(nameRecord->nameID);
        const quint16 platformID = qFromBigEndian(nameRecord->platformID);
        const quint16 encodingID = qFromBigEndian(nameRecord->encodingID);
        const quint16 offset = qFromBigEndian(nameRecord->offset);
        const quint16 length = qFromBigEndian(nameRecord->length);
        stringDataCount += length;
        if (stringDataCount > 80000 // hard limit. String data may be > name table size. Multiple records can reference the same string.
                || static_cast<quint32>(stringOffset + offset + length) > tableLength) // String outside bounds
            return false;
        const bool needsMarker =
                nameID == 1 || nameID == 3 || nameID == 4 || nameID == 16 || nameID == 21;
        const bool isUnicode =
                platformID == 0 || platformID == 3 && encodingID == 1;
        const QByteArray originalString =
                QByteArray::fromRawData(table.constData() + stringOffset + offset, length);
        QByteArray markedString;
        if (needsMarker) {
            const int maxBytesLength = (KMaxTypefaceNameLength - marker.length()) * (isUnicode ? 2 : 1);
            markedString = originalString.left(maxBytesLength) + (isUnicode ? markerUtf16BE : markerAscii);
        } else {
            markedString = originalString;
        }
        nameRecord->offset = qToBigEndian(static_cast<quint16>(markedStrings.length()));
        nameRecord->length = qToBigEndian(static_cast<quint16>(markedString.length()));
        markedStrings.append(markedString);
    }
    markedTable.append(markedStrings);
    table = markedTable;
    return true;
}

const quint32 ttfMaxFileSize = 3500000;

static inline bool ttfMarkAppFont(QByteArray &ttf, const QString &marker)
{
    const quint32 ttfChecksumNumber = 0xb1b0afba;
    const quint32 alignment = 4;
    const quint32 ttfLength = static_cast<quint32>(ttf.size());
    if (ttfLength > ttfMaxFileSize // hard limit
            || ttfLength % alignment != 0 // ttf sizes are always factors of 4
            || ttfLength <= sizeof(OffsetTable) // ttf too short
            || ttfCalcChecksum(ttf.constData(), ttf.size()) != ttfChecksumNumber) // ttf checksum is invalid
        return false;

    const OffsetTable *offsetTable = reinterpret_cast<const OffsetTable*>(ttf.constData());
    const quint16 numTables = qFromBigEndian(offsetTable->numTables);
    const quint32 recordsLength =
            toDWordBoundary(sizeof(OffsetTable) + numTables * sizeof(TableRecord));
    if (numTables > 30 // hard limit
            || recordsLength + numTables * alignment > ttfLength) // Corrupt ttf. Tables would not fit, even if empty.
        return false;

    QByteArray markedTtf;
    markedTtf.reserve(ttfLength + marker.length() * 20); // Original size plus some extra
    markedTtf.append(ttf.constData(), recordsLength);

    const quint32 ttfCheckSumAdjustmentOffset = 8; // Offset from the start of 'head'
    int indexOfHeadTable = -1;
    quint32 ttfDataSize = recordsLength;
    typedef QPair<quint32, quint32> Range;
    QList<Range> memoryRanges;
    memoryRanges.reserve(numTables);
    for (int i = 0; i < numTables; ++i) {
        TableRecord *tableRecord =
                reinterpret_cast<TableRecord*>(markedTtf.data() + sizeof(OffsetTable) + i * sizeof(TableRecord));
        const quint32 offset = qFromBigEndian(tableRecord->offset);
        const quint32 length = qFromBigEndian(tableRecord->length);
        const quint32 lengthAligned = toDWordBoundary(length);
        ttfDataSize += lengthAligned;
        if (offset < recordsLength // must not intersect ttf header/records
                || offset % alignment != 0 // must be aligned
                || offset > ttfLength - alignment // table out of bounds
                || offset + lengthAligned > ttfLength // table out of bounds
                || ttfDataSize > ttfLength) // tables would not fit into the ttf
            return false;

        foreach (const Range &range, memoryRanges)
            if (offset < range.first + range.second && offset + lengthAligned > range.first)
                return false; // Overlaps with another table
        memoryRanges.append(Range(offset, lengthAligned));

        quint32 checkSum = qFromBigEndian(tableRecord->checkSum);
        if (tableRecord->tag == qToBigEndian(static_cast<quint32>('head'))) {
            if (length < ttfCheckSumAdjustmentOffset + sizeof(quint32))
                return false; // Invalid 'head' table
            const quint32 *checkSumAdjustmentTag =
                    reinterpret_cast<const quint32*>(ttf.constData() + offset + ttfCheckSumAdjustmentOffset);
            const quint32 checkSumAdjustment = qFromBigEndian(*checkSumAdjustmentTag);
            checkSum += checkSumAdjustment;
            indexOfHeadTable = i; // For the ttf checksum re-calculation, later
        }
        if (checkSum != ttfCalcChecksum(ttf.constData() + offset, length))
            return false; // Table checksum is invalid

        bool updateTableChecksum = false;
        QByteArray table;
        if (tableRecord->tag == qToBigEndian(static_cast<quint32>('name'))) {
            table = QByteArray(ttf.constData() + offset, length);
            if (!ttfMarkNameTable(table, marker))
                return false; // Name table was not markable.
            updateTableChecksum = true;
        } else {
            table = QByteArray::fromRawData(ttf.constData() + offset, length);
        }

        tableRecord->offset = qToBigEndian(markedTtf.size());
        tableRecord->length = qToBigEndian(table.size());
        markedTtf.append(table);
        markedTtf.append(QByteArray(dWordPadding(table.size()), 0)); // 0-padding
        if (updateTableChecksum) {
            TableRecord *tableRecord = // Need to recalculate, since markedTtf changed
                    reinterpret_cast<TableRecord*>(markedTtf.data() + sizeof(OffsetTable) + i * sizeof(TableRecord));
            const quint32 offset = qFromBigEndian(tableRecord->offset);
            const quint32 length = qFromBigEndian(tableRecord->length);
            tableRecord->checkSum = qToBigEndian(ttfCalcChecksum(markedTtf.constData() + offset, length));
        }
    }
    if (indexOfHeadTable == -1 // 'head' table is mandatory
            || ttfDataSize != ttfLength) // We do not allow ttf data "holes". Neither does Symbian.
        return false;
    TableRecord *headRecord =
            reinterpret_cast<TableRecord*>(markedTtf.data() + sizeof(OffsetTable) + indexOfHeadTable * sizeof(TableRecord));
    quint32 *checkSumAdjustmentTag =
            reinterpret_cast<quint32*>(markedTtf.data() + qFromBigEndian(headRecord->offset) + ttfCheckSumAdjustmentOffset);
    *checkSumAdjustmentTag = 0;
    const quint32 ttfChecksum = ttfCalcChecksum(markedTtf.constData(), markedTtf.count());
    *checkSumAdjustmentTag = qToBigEndian(ttfChecksumNumber - ttfChecksum);
    ttf = markedTtf;
    return true;
}

static inline bool ttfCanSymbianLoadFont(const QByteArray &data, const QString &fileName)
{
    bool result = false;
    QString ttfFileName;
    QFile tempFileGuard;
    QFileInfo info(fileName);
    if (!data.isEmpty()) {
        QTemporaryFile tempfile(QSymbianFontDatabaseExtrasImplementation::tempAppFontFolder()
                                + QSymbianFontDatabaseExtrasImplementation::appFontMarker()
                                + QLatin1String("XXXXXX.ttf"));
        if (!tempfile.open() || tempfile.write(data) == -1)
            return false;
        ttfFileName = QDir::toNativeSeparators(QFileInfo(tempfile).canonicalFilePath());
        tempfile.setAutoRemove(false);
        tempfile.close();
        tempFileGuard.setFileName(ttfFileName);
        if (!tempFileGuard.open(QIODevice::ReadOnly))
            return false;
    } else if (info.isFile()) {
        ttfFileName = QDir::toNativeSeparators(info.canonicalFilePath());
    } else {
        return false;
    }

    CFontStore *store = 0;
    RHeap* heap = User::ChunkHeap(NULL, 0x1000, 0x20000);
    if (heap) {
        QT_TRAP_THROWING(
            CleanupClosePushL(*heap);
            store = CFontStore::NewL(heap);
            CleanupStack::PushL(store);
            COpenFontRasterizer *rasterizer = COpenFontRasterizer::NewL(TUid::Uid(0x101F7F5E));
            CleanupStack::PushL(rasterizer);
            store->InstallRasterizerL(rasterizer);
            CleanupStack::Pop(rasterizer);
            TUid fontUid = {-1};
            TRAP_IGNORE(fontUid = store->AddFileL(qt_QString2TPtrC(ttfFileName)));
            if (fontUid.iUid != -1)
                result = true;
            CleanupStack::PopAndDestroy(2, heap); // heap, store
        );
    }

    if (tempFileGuard.isOpen())
        tempFileGuard.remove();

    return result;
}

static void registerFont(QFontDatabasePrivate::ApplicationFont *fnt)
{
    if (QSymbianFontDatabaseExtrasImplementation::appFontLimitReached()
            || fnt->data.size() > ttfMaxFileSize // hard limit
            || fnt->data.isEmpty() && (!fnt->fileName.endsWith(QLatin1String(".ttf"), Qt::CaseInsensitive) // Only buffer or .ttf
                                       || QFileInfo(fnt->fileName).size() > ttfMaxFileSize)) // hard limit
        return;

//    Using ttfCanSymbianLoadFont() causes crashes on app destruction (Symbian^3|PR1 and lower).
//    Therefore, not using it for now, but eventually in a later version.
//    if (!ttfCanSymbianLoadFont(fnt->data, fnt->fileName))
//        return;

    QFontDatabasePrivate *db = privateDb();
    if (!db)
        return;

    if (!db->count)
        initializeDb();

    QSymbianFontDatabaseExtrasImplementation *dbExtras =
            static_cast<QSymbianFontDatabaseExtrasImplementation*>(db->symbianExtras);
    if (!dbExtras)
        return;

    const QString &marker = QSymbianFontDatabaseExtrasImplementation::appFontMarker();

    // The QTemporaryFile object being used in the following section must be
    // destructed before letting Symbian load the TTF file. Symbian would not
    // load it otherwise, because QTemporaryFile will still keep some handle
    // on it. The scope is used to reduce the life time of the QTemporaryFile.
    // In order to prevent other processes from modifying the file between the
    // moment where the QTemporaryFile is destructed and the file is loaded by
    // Symbian, we have a QFile "tempFileGuard" outside the scope which opens
    // the file in ReadOnly mode while the QTemporaryFile is still alive.
    QFile tempFileGuard;
    {
        QTemporaryFile tempfile(QSymbianFontDatabaseExtrasImplementation::tempAppFontFolder()
                                + marker + QLatin1String("XXXXXX.ttf"));
        if (!tempfile.open())
            return;
        const QString tempFileName = QFileInfo(tempfile).canonicalFilePath();
        if (fnt->data.isEmpty()) {
            QFile sourceFile(fnt->fileName);
            if (!sourceFile.open(QIODevice::ReadOnly))
                return;
            fnt->data = sourceFile.readAll();
        }
        if (!ttfMarkAppFont(fnt->data, marker) || tempfile.write(fnt->data) == -1)
            return;
        tempfile.setAutoRemove(false);
        tempfile.close(); // Tempfile still keeps a file handle, forbidding write access
        fnt->data.clear(); // The TTF data was marked and saved. Not needed in memory, anymore.
        tempFileGuard.setFileName(tempFileName);
        if (!tempFileGuard.open(QIODevice::ReadOnly))
            return;
        fnt->temporaryFileName = tempFileName;
    }

    const QString fullFileName = QDir::toNativeSeparators(fnt->temporaryFileName);
    QSymbianFbsHeapLock lock(QSymbianFbsHeapLock::Unlock);
    const QStringList fontsOnServerBefore = qt_symbian_fontFamiliesOnFontServer();
    const TInt err =
            S60->screenDevice()->AddFile(qt_QString2TPtrC(fullFileName), fnt->screenDeviceFontFileId);
    tempFileGuard.close(); // Did its job
    const QStringList fontsOnServerAfter = qt_symbian_fontFamiliesOnFontServer();
    if (err == KErrNone && fontsOnServerBefore.count() < fontsOnServerAfter.count()) { // Added to screen device?
        int fontOnServerIndex = fontsOnServerAfter.count() - 1;
        for (int i = 0; i < fontsOnServerBefore.count(); i++) {
            if (fontsOnServerBefore.at(i) != fontsOnServerAfter.at(i)) {
                fontOnServerIndex = i;
                break;
            }
        }

        // Must remove all font engines with their CFonts, first.
        QFontCache::instance()->clear();
        db->free();
        QSymbianFontDatabaseExtrasImplementation::clear();

        if (!QSymbianTypeFaceExtras::symbianFontTableApiAvailable())
            fnt->fontStoreFontFileUid = dbExtras->addFontFileToFontStore(QFileInfo(fullFileName));

        const QString &appFontName = fontsOnServerAfter.at(fontOnServerIndex);
        fnt->families.append(qt_symbian_appFontNameWithoutMarker(appFontName));
        if (!qt_symbian_fontNameHasAppFontMarker(appFontName)
                || !registerScreenDeviceFont(fontOnServerIndex, dbExtras))
            dbExtras->removeAppFontData(fnt);
    } else {
        if (fnt->screenDeviceFontFileId > 0)
            S60->screenDevice()->RemoveFile(fnt->screenDeviceFontFileId); // May still have the file open!
        QFile::remove(fnt->temporaryFileName);
        *fnt = QFontDatabasePrivate::ApplicationFont();
    }
    lock.relock();
}

bool QFontDatabase::removeApplicationFont(int handle)
{
    QMutexLocker locker(fontDatabaseMutex());

    QFontDatabasePrivate *db = privateDb();
    if (!db || handle < 0 || handle >= db->applicationFonts.count())
        return false;
    QSymbianFontDatabaseExtrasImplementation *dbExtras =
            static_cast<QSymbianFontDatabaseExtrasImplementation*>(db->symbianExtras);
    if (!dbExtras)
        return false;

    QFontDatabasePrivate::ApplicationFont *fnt = &db->applicationFonts[handle];
    if (fnt->families.isEmpty())
        return true; // Nothing to remove. Return peacefully.

    // Must remove all font engines with their CFonts, first
    QFontCache::instance()->clear();
    db->free();
    dbExtras->removeAppFontData(fnt);

    db->invalidate(); // This will just emit 'fontDatabaseChanged()'
    return true;
}

bool QFontDatabase::removeAllApplicationFonts()
{
    QMutexLocker locker(fontDatabaseMutex());

    const int applicationFontsCount = privateDb()->applicationFonts.count();
    for (int i = 0; i < applicationFontsCount; ++i)
        if (!removeApplicationFont(i))
            return false;
    return true;
}

bool QFontDatabase::supportsThreadedFontRendering()
{
    return QSymbianTypeFaceExtras::symbianFontTableApiAvailable();
}

static
QFontDef cleanedFontDef(const QFontDef &req)
{
    QFontDef result = req;
    if (result.pixelSize <= 0) {
        result.pixelSize = QFontEngineS60::pointsToPixels(qMax(qreal(1.0), result.pointSize));
        result.pointSize = 0;
    }
    return result;
}

QFontEngine *QFontDatabase::findFont(int script, const QFontPrivate *d, const QFontDef &req)
{
    const QFontCache::Key key(cleanedFontDef(req), script);

    if (!privateDb()->count)
        initializeDb();

    QFontEngine *fe = QFontCache::instance()->findEngine(key);
    if (!fe) {
        // Making sure that fe->fontDef.family will be an existing font.
        initializeDb();
        QFontDatabasePrivate *db = privateDb();
        QtFontDesc desc;
        QList<int> blacklistedFamilies;
        match(script, key.def, key.def.family, QString(), -1, &desc, blacklistedFamilies);
        if (!desc.family) // falling back to application font
            desc.family = db->family(QApplication::font().defaultFamily());
        Q_ASSERT(desc.family);

        // Making sure that desc.family supports the requested script
        QtFontDesc mappedDesc;
        bool supportsScript = false;
        do {
            match(script, req, QString(), QString(), -1, &mappedDesc, blacklistedFamilies);
            if (mappedDesc.family == desc.family) {
                supportsScript = true;
                break;
            }
            blacklistedFamilies.append(mappedDesc.familyIndex);
        } while (mappedDesc.family);
        if (!supportsScript) {
            blacklistedFamilies.clear();
            match(script, req, QString(), QString(), -1, &mappedDesc, blacklistedFamilies);
            if (mappedDesc.family)
                desc = mappedDesc;
        }

        const QString fontFamily = desc.family->name;
        QFontDef request = req;
        request.family = fontFamily;
#ifdef QT_NO_FREETYPE
        const QSymbianFontDatabaseExtrasImplementation *dbExtras =
                static_cast<const QSymbianFontDatabaseExtrasImplementation*>(db->symbianExtras);
        const QSymbianTypeFaceExtras *typeFaceExtras =
                dbExtras->extras(fontFamily, request.weight > QFont::Normal, request.style != QFont::StyleNormal);

        // We need a valid pixelSize, e.g. for lineThickness()
        if (request.pixelSize < 0)
            request.pixelSize = request.pointSize * d->dpi / 72;

        fe = new QFontEngineS60(request, typeFaceExtras);
#else // QT_NO_FREETYPE
        Q_UNUSED(d)
        QFontEngine::FaceId faceId;
        const QtFontFamily * const reqQtFontFamily = db->family(fontFamily);
        faceId.filename = reqQtFontFamily->fontFilename;
        faceId.index = reqQtFontFamily->fontFileIndex;

        QFontEngineFTS60 *fte = new QFontEngineFTS60(cleanedFontDef(request));
        if (fte->init(faceId, true, QFontEngineFT::Format_A8))
            fe = fte;
        else
            delete fte;
#endif // QT_NO_FREETYPE

        Q_ASSERT(fe);
        if (script == QUnicodeTables::Common
            && !(req.styleStrategy & QFont::NoFontMerging)
            && !fe->symbol) {

            QStringList commonFonts;
            for (int ws = 1; ws < QFontDatabase::WritingSystemsCount; ++ws) {
                if (scriptForWritingSystem[ws] != script)
                    continue;
                for (int i = 0; i < db->count; ++i) {
                    if (db->families[i]->writingSystems[ws] & QtFontFamily::Supported)
                        commonFonts.append(db->families[i]->name);
                }
            }

            // Hack: Prioritize .ccc fonts
            const QString niceEastAsianFont(QLatin1String("Sans MT 936_S60"));
            if (commonFonts.removeAll(niceEastAsianFont) > 0)
                commonFonts.prepend(niceEastAsianFont);

            fe = new QFontEngineMultiS60(fe, script, commonFonts);
        }
    }
    fe->ref.ref();
    QFontCache::instance()->insertEngine(key, fe);
    return fe;
}

void QFontDatabase::load(const QFontPrivate *d, int script)
{
    QFontEngine *fe = 0;
    QFontDef req = d->request;

    if (!d->engineData) {
        const QFontCache::Key key(cleanedFontDef(req), script);
        getEngineData(d, key);
    }

    // the cached engineData could have already loaded the engine we want
    if (d->engineData->engines[script])
        fe = d->engineData->engines[script];

    if (!fe) {
        if (qt_enable_test_font && req.family == QLatin1String("__Qt__Box__Engine__")) {
            fe = new QTestFontEngine(req.pixelSize);
            fe->fontDef = req;
        } else {
            fe = findFont(script, d, req);
        }
        d->engineData->engines[script] = fe;
    }
}

QT_END_NAMESPACE
