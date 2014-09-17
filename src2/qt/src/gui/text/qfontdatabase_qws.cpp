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

#include "qdir.h"
#if defined(Q_WS_QWS)
#include "qscreen_qws.h" //so we can check for rotation
#include "qwindowsystem_qws.h"
#endif
#include "qlibraryinfo.h"
#include "qabstractfileengine.h"
#include <QtCore/qsettings.h>
#if !defined(QT_NO_FREETYPE)
#include "qfontengine_ft_p.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#endif
#include "qfontengine_qpf_p.h"
#include "private/qfactoryloader_p.h"
#include "private/qcore_unix_p.h" // overrides QT_OPEN
#include "qabstractfontengine_qws.h"
#include "qabstractfontengine_p.h"
#include <qdatetime.h>
#include "qplatformdefs.h"

// for mmap
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#ifdef QT_FONTS_ARE_RESOURCES
#include <qresource.h>
#endif

#ifdef Q_OS_QNX
// ### using QFontEngineQPF leads to artifacts on QNX
#  define QT_NO_QWS_SHARE_FONTS
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_LIBRARY
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QFontEngineFactoryInterface_iid, QLatin1String("/fontengines"), Qt::CaseInsensitive))
#endif

const quint8 DatabaseVersion = 4;

// QFontDatabasePrivate::addFont() went into qfontdatabase.cpp

#ifndef QT_NO_QWS_QPF2
void QFontDatabasePrivate::addQPF2File(const QByteArray &file)
{
#ifndef QT_FONTS_ARE_RESOURCES
    struct stat st;
    if (stat(file.constData(), &st))
        return;
    int f = QT_OPEN(file, O_RDONLY, 0);
    if (f < 0)
        return;
    const uchar *data = (const uchar *)mmap(0, st.st_size, PROT_READ, MAP_SHARED, f, 0);
    const int dataSize = st.st_size;
#else
    QResource res(QLatin1String(file.constData()));
    const uchar *data = res.data();
    const int dataSize = res.size();
    //qDebug() << "addQPF2File" << file << data;
#endif
    if (data && data != (const uchar *)MAP_FAILED) {
        if (QFontEngineQPF::verifyHeader(data, dataSize)) {
            QString fontName = QFontEngineQPF::extractHeaderField(data, QFontEngineQPF::Tag_FontName).toString();
            int pixelSize = QFontEngineQPF::extractHeaderField(data, QFontEngineQPF::Tag_PixelSize).toInt();
            QVariant weight = QFontEngineQPF::extractHeaderField(data, QFontEngineQPF::Tag_Weight);
            QVariant style = QFontEngineQPF::extractHeaderField(data, QFontEngineQPF::Tag_Style);
            QByteArray writingSystemBits = QFontEngineQPF::extractHeaderField(data, QFontEngineQPF::Tag_WritingSystems).toByteArray();

            if (!fontName.isEmpty() && pixelSize) {
                int fontWeight = 50;
                if (weight.type() == QVariant::Int || weight.type() == QVariant::UInt)
                    fontWeight = weight.toInt();

                bool italic = static_cast<QFont::Style>(style.toInt()) & QFont::StyleItalic;

                QList<QFontDatabase::WritingSystem> writingSystems;
                for (int i = 0; i < writingSystemBits.count(); ++i) {
                    uchar currentByte = writingSystemBits.at(i);
                    for (int j = 0; j < 8; ++j) {
                        if (currentByte & 1)
                            writingSystems << QFontDatabase::WritingSystem(i * 8 + j);
                        currentByte >>= 1;
                    }
                }

                addFont(fontName, /*foundry*/ "prerendered", fontWeight, italic,
                        pixelSize, file, /*fileIndex*/ 0,
                        /*antialiased*/ true, writingSystems);
            }
        } else {
            qDebug() << "header verification of QPF2 font" << file << "failed. maybe it is corrupt?";
        }
#ifndef QT_FONTS_ARE_RESOURCES
        munmap((void *)data, st.st_size);
#endif
    }
#ifndef QT_FONTS_ARE_RESOURCES
    QT_CLOSE(f);
#endif
}
#endif // QT_NO_QWS_QPF2

// QFontDatabasePrivate::addTTFile() went into qfontdatabase.cpp

static void registerFont(QFontDatabasePrivate::ApplicationFont *fnt);

extern QString qws_fontCacheDir();

#ifndef QT_FONTS_ARE_RESOURCES
bool QFontDatabasePrivate::loadFromCache(const QString &fontPath)
{
#ifdef Q_WS_QWS
    const bool weAreTheServer = QWSServer::instance();
#else
    const bool weAreTheServer = true; // assume single-process
#endif

    QString fontDirFile = fontPath + QLatin1String("/fontdir");

    QFile binaryDb(qws_fontCacheDir() + QLatin1String("/fontdb"));

    if (weAreTheServer) {
        QDateTime dbTimeStamp = QFileInfo(binaryDb.fileName()).lastModified();

        QDateTime fontPathTimeStamp = QFileInfo(fontPath).lastModified();
        if (dbTimeStamp < fontPathTimeStamp)
            return false; // let the caller create the cache

        if (QFile::exists(fontDirFile)) {
            QDateTime fontDirTimeStamp = QFileInfo(fontDirFile).lastModified();
            if (dbTimeStamp < fontDirTimeStamp)
                return false;
        }
    }

    if (!binaryDb.open(QIODevice::ReadOnly)) {
        if (weAreTheServer)
            return false; // let the caller create the cache
        qFatal("QFontDatabase::loadFromCache: Could not open font database cache!");
    }

    QDataStream stream(&binaryDb);
    quint8 version = 0;
    quint8 dataStreamVersion = 0;
    stream >> version >> dataStreamVersion;
    if (version != DatabaseVersion || dataStreamVersion != stream.version()) {
        if (weAreTheServer)
            return false; // let the caller create the cache
        qFatal("QFontDatabase::loadFromCache: Wrong version of the font database cache detected. Found %d/%d expected %d/%d",
               version, dataStreamVersion, DatabaseVersion, stream.version());
    }

    QString originalFontPath;
    stream >> originalFontPath;
    if (originalFontPath != fontPath) {
        if (weAreTheServer)
            return false; // let the caller create the cache
        qFatal("QFontDatabase::loadFromCache: Font path doesn't match. Found %s in database, expected %s", qPrintable(originalFontPath), qPrintable(fontPath));
    }

    QString familyname;
    stream >> familyname;
    //qDebug() << "populating database from" << binaryDb.fileName();
    while (!familyname.isEmpty() && !stream.atEnd()) {
        QString foundryname;
        int weight;
        quint8 italic;
        int pixelSize;
        QByteArray file;
        int fileIndex;
        quint8 antialiased;
        quint8 writingSystemCount;

        QList<QFontDatabase::WritingSystem> writingSystems;

        stream >> foundryname >> weight >> italic >> pixelSize
               >> file >> fileIndex >> antialiased >> writingSystemCount;

        for (quint8 i = 0; i < writingSystemCount; ++i) {
            quint8 ws;
            stream >> ws;
            writingSystems.append(QFontDatabase::WritingSystem(ws));
        }

        addFont(familyname, foundryname.toLatin1().constData(), weight, italic, pixelSize, file, fileIndex, antialiased,
                writingSystems);

        stream >> familyname;
    }

    stream >> fallbackFamilies;
    //qDebug() << "fallback families from cache:" << fallbackFamilies;
    return true;
}
#endif // QT_FONTS_ARE_RESOURCES

/*!
    \internal
*/

static QString qwsFontPath()
{
    QString fontpath = QString::fromLocal8Bit(qgetenv("QT_QWS_FONTDIR"));
    if (fontpath.isEmpty()) {
#ifdef QT_FONTS_ARE_RESOURCES
        fontpath = QLatin1String(":/qt/fonts");
#else
#ifndef QT_NO_SETTINGS
        fontpath = QLibraryInfo::location(QLibraryInfo::LibrariesPath);
        fontpath += QLatin1String("/fonts");
#else
        fontpath = QLatin1String("/lib/fonts");
#endif
#endif //QT_FONTS_ARE_RESOURCES
    }

    return fontpath;
}

#if defined(QFONTDATABASE_DEBUG) && defined(QT_FONTS_ARE_RESOURCES)
class FriendlyResource : public QResource
{
public:
    bool isDir () const { return QResource::isDir(); }
    bool isFile () const { return QResource::isFile(); }
    QStringList children () const { return QResource::children(); }
};
#endif
/*!
    \internal
*/
static void initializeDb()
{
    QFontDatabasePrivate *db = privateDb();
    if (!db || db->count)
        return;

    QString fontpath = qwsFontPath();
#ifndef QT_FONTS_ARE_RESOURCES
    QString fontDirFile = fontpath + QLatin1String("/fontdir");

    if(!QFile::exists(fontpath)) {
        qFatal("QFontDatabase: Cannot find font directory %s - is Qt installed correctly?",
               fontpath.toLocal8Bit().constData());
    }

    const bool loaded = db->loadFromCache(fontpath);

    if (db->reregisterAppFonts) {
        db->reregisterAppFonts = false;
        for (int i = 0; i < db->applicationFonts.count(); ++i)
            if (!db->applicationFonts.at(i).families.isEmpty()) {
                registerFont(&db->applicationFonts[i]);
            }
    }

    if (loaded)
        return;

    QString dbFileName = qws_fontCacheDir() + QLatin1String("/fontdb");

    QFile binaryDb(dbFileName + QLatin1String(".tmp"));
    binaryDb.open(QIODevice::WriteOnly | QIODevice::Truncate);
    db->stream = new QDataStream(&binaryDb);
    *db->stream << DatabaseVersion << quint8(db->stream->version()) << fontpath;
//    qDebug() << "creating binary database at" << binaryDb.fileName();

    // Load in font definition file
    FILE* fontdef=fopen(fontDirFile.toLocal8Bit().constData(),"r");
    if (fontdef) {
        char buf[200]="";
        char name[200]="";
        char render[200]="";
        char file[200]="";
        char isitalic[10]="";
        char flags[10]="";
        do {
            fgets(buf,200,fontdef);
            if (buf[0] != '#') {
                int weight=50;
                int size=0;
                sscanf(buf,"%s %s %s %s %d %d %s",name,file,render,isitalic,&weight,&size,flags);
                QString filename;
                if (file[0] != '/')
                    filename.append(fontpath).append(QLatin1Char('/'));
                filename += QLatin1String(file);
                bool italic = isitalic[0] == 'y';
                bool smooth = QByteArray(flags).contains('s');
                if (file[0] && QFile::exists(filename))
                    db->addFont(QString::fromUtf8(name), /*foundry*/"", weight, italic, size/10, QFile::encodeName(filename), /*fileIndex*/ 0, smooth);
            }
        } while (!feof(fontdef));
        fclose(fontdef);
    }


    QDir dir(fontpath, QLatin1String("*.qpf"));
    for (int i=0; i<int(dir.count()); i++) {
        int u0 = dir[i].indexOf(QLatin1Char('_'));
        int u1 = dir[i].indexOf(QLatin1Char('_'), u0+1);
        int u2 = dir[i].indexOf(QLatin1Char('_'), u1+1);
        int u3 = dir[i].indexOf(QLatin1Char('.'), u1+1);
        if (u2 < 0) u2 = u3;

        QString familyname = dir[i].left(u0);
        int pixelSize = dir[i].mid(u0+1,u1-u0-1).toInt()/10;
        bool italic = dir[i].mid(u2-1,1) == QLatin1String("i");
        int weight = dir[i].mid(u1+1,u2-u1-1-(italic?1:0)).toInt();

        db->addFont(familyname, /*foundry*/ "qt", weight, italic, pixelSize, QFile::encodeName(dir.absoluteFilePath(dir[i])),
                    /*fileIndex*/ 0, /*antialiased*/ true);
    }

#ifndef QT_NO_FREETYPE
    dir.setNameFilters(QStringList() << QLatin1String("*.ttf")
                       << QLatin1String("*.ttc") << QLatin1String("*.pfa")
                       << QLatin1String("*.pfb"));
    dir.refresh();
    for (int i = 0; i < int(dir.count()); ++i) {
        const QByteArray file = QFile::encodeName(dir.absoluteFilePath(dir[i]));
//        qDebug() << "looking at" << file;
        db->addTTFile(file);
    }
#endif

#ifndef QT_NO_QWS_QPF2
    dir.setNameFilters(QStringList() << QLatin1String("*.qpf2"));
    dir.refresh();
    for (int i = 0; i < int(dir.count()); ++i) {
        const QByteArray file = QFile::encodeName(dir.absoluteFilePath(dir[i]));
//        qDebug() << "looking at" << file;
        db->addQPF2File(file);
    }
#endif

#else //QT_FONTS_ARE_RESOURCES
#ifdef QFONTDATABASE_DEBUG
    {
        QResource fontdir(fontpath);
        FriendlyResource *fr = static_cast<FriendlyResource*>(&fontdir);
        qDebug() << "fontdir" << fr->isValid() << fr->isDir() << fr->children();

    }
#endif
#ifndef QT_NO_QWS_QPF2
    QDir dir(fontpath, QLatin1String("*.qpf2"));
    for (int i = 0; i < int(dir.count()); ++i) {
        const QByteArray file = QFile::encodeName(dir.absoluteFilePath(dir[i]));
        //qDebug() << "looking at" << file;
        db->addQPF2File(file);
    }
#endif
#endif //QT_FONTS_ARE_RESOURCES


#ifdef QFONTDATABASE_DEBUG
    // print the database
    for (int f = 0; f < db->count; f++) {
        QtFontFamily *family = db->families[f];
        FD_DEBUG("'%s' %s", qPrintable(family->name), (family->fixedPitch ? "fixed" : ""));
#if 0
        for (int i = 0; i < QFont::LastPrivateScript; ++i) {
            FD_DEBUG("\t%s: %s", qPrintable(QFontDatabase::scriptName((QFont::Script) i)),
                     ((family->scripts[i] & QtFontFamily::Supported) ? "Supported" :
                      (family->scripts[i] & QtFontFamily::UnSupported) == QtFontFamily::UnSupported ?
                      "UnSupported" : "Unknown"));
        }
#endif

        for (int fd = 0; fd < family->count; fd++) {
            QtFontFoundry *foundry = family->foundries[fd];
            FD_DEBUG("\t\t'%s'", qPrintable(foundry->name));
            for (int s = 0; s < foundry->count; s++) {
                QtFontStyle *style = foundry->styles[s];
                FD_DEBUG("\t\t\tstyle: style=%d weight=%d\n"
                         "\t\t\tstretch=%d",
                         style->key.style, style->key.weight,
                         style->key.stretch);
                if (style->smoothScalable)
                    FD_DEBUG("\t\t\t\tsmooth scalable");
                else if (style->bitmapScalable)
                    FD_DEBUG("\t\t\t\tbitmap scalable");
                if (style->pixelSizes) {
                    FD_DEBUG("\t\t\t\t%d pixel sizes",  style->count);
                    for (int z = 0; z < style->count; ++z) {
                        QtFontSize *size = style->pixelSizes + z;
                        FD_DEBUG("\t\t\t\t  size %5d",
                                  size->pixelSize);
                    }
                }
            }
        }
    }
#endif // QFONTDATABASE_DEBUG

#ifndef QT_NO_LIBRARY
    QStringList pluginFoundries = loader()->keys();
//    qDebug() << "plugin foundries:" << pluginFoundries;
    for (int i = 0; i < pluginFoundries.count(); ++i) {
        const QString foundry(pluginFoundries.at(i));

        QFontEngineFactoryInterface *factory = qobject_cast<QFontEngineFactoryInterface *>(loader()->instance(foundry));
        if (!factory) {
            qDebug() << "Could not load plugin for foundry" << foundry;
            continue;
        }

        QList<QFontEngineInfo> fonts = factory->availableFontEngines();
        for (int i = 0; i < fonts.count(); ++i) {
            QFontEngineInfo info = fonts.at(i);

            int weight = info.weight();
            if (weight <= 0)
                weight = QFont::Normal;

            db->addFont(info.family(), foundry.toLatin1().constData(),
                        weight, info.style() != QFont::StyleNormal,
                        qRound(info.pixelSize()), /*file*/QByteArray(),
                        /*fileIndex*/0, /*antiAliased*/true,
                        info.writingSystems());
        }
    }
#endif

#ifndef QT_FONTS_ARE_RESOURCES
    // the empty string/familyname signifies the end of the font list.
    *db->stream << QString();
#endif
    {
        bool coveredWritingSystems[QFontDatabase::WritingSystemsCount] = { 0 };

        db->fallbackFamilies.clear();

        for (int i = 0; i < db->count; ++i) {
            QtFontFamily *family = db->families[i];
            bool add = false;
            if (family->count == 0)
                continue;
            if (family->bogusWritingSystems)
                continue;
            for (int ws = 1; ws < QFontDatabase::WritingSystemsCount; ++ws) {
                if (coveredWritingSystems[ws])
                    continue;
                if (family->writingSystems[ws] & QtFontFamily::Supported) {
                    coveredWritingSystems[ws] = true;
                    add = true;
                }
            }
            if (add)
                db->fallbackFamilies << family->name;
        }
        //qDebug() << "fallbacks on the server:" << db->fallbackFamilies;
#ifndef QT_FONTS_ARE_RESOURCES
        *db->stream << db->fallbackFamilies;
#endif
    }
#ifndef QT_FONTS_ARE_RESOURCES
    delete db->stream;
    db->stream = 0;
    QFile::remove(dbFileName);
    binaryDb.rename(dbFileName);
#endif
}

// called from qwindowsystem_qws.cpp
void qt_qws_init_fontdb()
{
    initializeDb();
}

#ifndef QT_NO_SETTINGS
// called from qapplication_qws.cpp
void qt_applyFontDatabaseSettings(const QSettings &settings)
{
    initializeDb();
    QFontDatabasePrivate *db = privateDb();
    for (int i = 0; i < db->count; ++i) {
        QtFontFamily *family = db->families[i];
        if (settings.contains(family->name))
            family->fallbackFamilies = settings.value(family->name).toStringList();
    }

    if (settings.contains(QLatin1String("Global Fallbacks")))
        db->fallbackFamilies = settings.value(QLatin1String("Global Fallbacks")).toStringList();
}
#endif // QT_NO_SETTINGS

static inline void load(const QString & = QString(), int = -1)
{
    initializeDb();
}

#ifndef QT_NO_FREETYPE

#if (FREETYPE_MAJOR*10000+FREETYPE_MINOR*100+FREETYPE_PATCH) >= 20105
#define X_SIZE(face,i) ((face)->available_sizes[i].x_ppem)
#define Y_SIZE(face,i) ((face)->available_sizes[i].y_ppem)
#else
#define X_SIZE(face,i) ((face)->available_sizes[i].width << 6)
#define Y_SIZE(face,i) ((face)->available_sizes[i].height << 6)
#endif

#endif // QT_NO_FREETYPE

static
QFontEngine *loadSingleEngine(int script, const QFontPrivate *fp,
                              const QFontDef &request,
                              QtFontFamily *family, QtFontFoundry *foundry,
                              QtFontStyle *style, QtFontSize *size)
{
    Q_UNUSED(script);
    Q_UNUSED(fp);
#ifdef QT_NO_FREETYPE
    Q_UNUSED(foundry);
#endif
#ifdef QT_NO_QWS_QPF
    Q_UNUSED(family);
#endif
    Q_ASSERT(size);

    int pixelSize = size->pixelSize;
    if (!pixelSize || (style->smoothScalable && pixelSize == SMOOTH_SCALABLE))
        pixelSize = request.pixelSize;

#ifndef QT_NO_QWS_QPF2
    if (foundry->name == QLatin1String("prerendered")) {
#ifdef QT_FONTS_ARE_RESOURCES
        QResource res(QLatin1String(size->fileName.constData()));
        if (res.isValid()) {
            QFontEngineQPF *fe = new QFontEngineQPF(request, res.data(), res.size());
            if (fe->isValid())
                return fe;
            delete fe;
            qDebug() << "fontengine is not valid! " << size->fileName;
        } else {
            qDebug() << "Resource not valid" << size->fileName;
        }
#else
        int f = ::open(size->fileName, O_RDONLY, 0);
        if (f >= 0) {
            QFontEngineQPF *fe = new QFontEngineQPF(request, f);
            if (fe->isValid())
                return fe;
            delete fe; // will close f
            qDebug() << "fontengine is not valid!";
        }
#endif
    } else
#endif
    if ( foundry->name != QLatin1String("qt") ) { ///#### is this the best way????
        QString file = QFile::decodeName(size->fileName);

        QFontDef def = request;
        def.pixelSize = pixelSize;

#ifdef QT_NO_QWS_SHARE_FONTS
        bool shareFonts = false;
#else
        static bool dontShareFonts = !qgetenv("QWS_NO_SHARE_FONTS").isEmpty();
        bool shareFonts = !dontShareFonts;
#endif

        QScopedPointer<QFontEngine> engine;

#ifndef QT_NO_LIBRARY
        QFontEngineFactoryInterface *factory = qobject_cast<QFontEngineFactoryInterface *>(loader()->instance(foundry->name));
        if (factory) {
            QFontEngineInfo info;
            info.setFamily(request.family);
            info.setPixelSize(request.pixelSize);
            info.setStyle(QFont::Style(request.style));
            info.setWeight(request.weight);
            // #### antialiased

            QAbstractFontEngine *customEngine = factory->create(info);
            if (customEngine) {
                engine.reset(new QProxyFontEngine(customEngine, def));

                if (shareFonts) {
                    QVariant hint = customEngine->fontProperty(QAbstractFontEngine::CacheGlyphsHint);
                    if (hint.isValid())
                        shareFonts = hint.toBool();
                    else
                        shareFonts = (pixelSize < 64);
                }
            }
        }
#endif // QT_NO_LIBRARY
        if ((engine.isNull() && !file.isEmpty() && QFile::exists(file)) || privateDb()->isApplicationFont(file)) {
            QFontEngine::FaceId faceId;
            faceId.filename = file.toLocal8Bit();
            faceId.index = size->fileIndex;

#ifndef QT_NO_FREETYPE

            QScopedPointer<QFontEngineFT> fte(new QFontEngineFT(def));
            bool antialias = style->antialiased && !(request.styleStrategy & QFont::NoAntialias);
            if (fte->init(faceId, antialias,
                          antialias ? QFontEngineFT::Format_A8 : QFontEngineFT::Format_Mono)) {
#ifdef QT_NO_QWS_QPF2
                return fte.take();
#else
                // try to distinguish between bdf and ttf fonts we can pre-render
                // and don't try to share outline fonts
                shareFonts = shareFonts
                             && !fte->defaultGlyphs()->outline_drawing
                             && !fte->getSfntTable(MAKE_TAG('h', 'e', 'a', 'd')).isEmpty();
                engine.reset(fte.take());
#endif
            }
#endif // QT_NO_FREETYPE
        }
        if (!engine.isNull()) {
#if !defined(QT_NO_QWS_QPF2) && !defined(QT_FONTS_ARE_RESOURCES)
            if (shareFonts) {
                QScopedPointer<QFontEngineQPF> fe(new QFontEngineQPF(def, -1, engine.data()));
                engine.take();
                if (fe->isValid())
                    return fe.take();
                qWarning("Initializing QFontEngineQPF failed for %s", qPrintable(file));
                engine.reset(fe->takeRenderingEngine());
            }
#endif
            return engine.take();
        }
    } else
    {
#ifndef QT_NO_QWS_QPF
        QString fn = qwsFontPath();
        fn += QLatin1Char('/');
        fn += family->name.toLower()
              + QLatin1Char('_') + QString::number(pixelSize*10)
              + QLatin1Char('_') + QString::number(style->key.weight)
              + (style->key.style == QFont::StyleItalic ?
                 QLatin1String("i.qpf") : QLatin1String(".qpf"));
        //###rotation ###

        QFontEngine *fe = new QFontEngineQPF1(request, fn);
        return fe;
#endif // QT_NO_QWS_QPF
    }
    return new QFontEngineBox(pixelSize);
}

static
QFontEngine *loadEngine(int script, const QFontPrivate *fp,
                        const QFontDef &request,
                        QtFontFamily *family, QtFontFoundry *foundry,
                        QtFontStyle *style, QtFontSize *size)
{
    QScopedPointer<QFontEngine> engine(loadSingleEngine(script, fp, request, family, foundry,
                                       style, size));
#ifndef QT_NO_QWS_QPF
    if (!engine.isNull()
        && script == QUnicodeTables::Common
        && !(request.styleStrategy & QFont::NoFontMerging) && !engine->symbol) {

        QStringList fallbacks = privateDb()->fallbackFamilies;

        if (family && !family->fallbackFamilies.isEmpty())
            fallbacks = family->fallbackFamilies;

        QFontEngine *fe = new QFontEngineMultiQWS(engine.data(), script, fallbacks);
        engine.take();
        engine.reset(fe);
    }
#endif
    return engine.take();
}

static void registerFont(QFontDatabasePrivate::ApplicationFont *fnt)
{
    QFontDatabasePrivate *db = privateDb();
#ifdef QT_NO_FREETYPE
    Q_UNUSED(fnt);
#else
    fnt->families = db->addTTFile(QFile::encodeName(fnt->fileName), fnt->data);
    db->fallbackFamilies += fnt->families;
#endif
    db->reregisterAppFonts = true;
}

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

bool QFontDatabase::supportsThreadedFontRendering()
{
    return true;
}

QFontEngine *
QFontDatabase::findFont(int script, const QFontPrivate *fp,
                        const QFontDef &request)
{
    QMutexLocker locker(fontDatabaseMutex());

    const int force_encoding_id = -1;

    if (!privateDb()->count)
        initializeDb();

    QScopedPointer<QFontEngine> fe;
    if (fp) {
        if (fp->rawMode) {
            fe.reset(loadEngine(script, fp, request, 0, 0, 0, 0));

            // if we fail to load the rawmode font, use a 12pixel box engine instead
            if (fe.isNull())
                fe.reset(new QFontEngineBox(12));
            return fe.take();
        }

        QFontCache::Key key(request, script);
        fe.reset(QFontCache::instance()->findEngine(key));
        if (! fe.isNull())
            return fe.take();
    }

    QString family_name, foundry_name;
    QtFontStyle::Key styleKey;
    styleKey.style = request.style;
    styleKey.weight = request.weight;
    styleKey.stretch = request.stretch;
    char pitch = request.ignorePitch ? '*' : request.fixedPitch ? 'm' : 'p';

    parseFontName(request.family, foundry_name, family_name);

    FM_DEBUG("QFontDatabase::findFont\n"
             "  request:\n"
             "    family: %s [%s], script: %d\n"
             "    weight: %d, style: %d\n"
             "    stretch: %d\n"
             "    pixelSize: %g\n"
             "    pitch: %c",
             family_name.isEmpty() ? "-- first in script --" : family_name.toLatin1().constData(),
             foundry_name.isEmpty() ? "-- any --" : foundry_name.toLatin1().constData(),
             script, request.weight, request.style, request.stretch, request.pixelSize, pitch);

    if (qt_enable_test_font && request.family == QLatin1String("__Qt__Box__Engine__")) {
        fe.reset(new QTestFontEngine(request.pixelSize));
        fe->fontDef = request;
    }

    if (fe.isNull())
    {
        QtFontDesc desc;
        match(script, request, family_name, foundry_name, force_encoding_id, &desc);

        if (desc.family != 0 && desc.foundry != 0 && desc.style != 0
            ) {
            FM_DEBUG("  BEST:\n"
                     "    family: %s [%s]\n"
                     "    weight: %d, style: %d\n"
                     "    stretch: %d\n"
                     "    pixelSize: %d\n"
                     "    pitch: %c\n"
                     "    encoding: %d\n",
                     desc.family->name.toLatin1().constData(),
                     desc.foundry->name.isEmpty() ? "-- none --" : desc.foundry->name.toLatin1().constData(),
                     desc.style->key.weight, desc.style->key.style,
                     desc.style->key.stretch, desc.size ? desc.size->pixelSize : 0xffff,
                     'p', 0
                );

            fe.reset(loadEngine(script, fp, request, desc.family, desc.foundry, desc.style, desc.size
                ));
        } else {
            FM_DEBUG("  NO MATCH FOUND\n");
        }
        if (! fe.isNull())
            initFontDef(desc, request, &fe->fontDef);
    }

#ifndef QT_NO_FREETYPE
    if (! fe.isNull()) {
        if (scriptRequiresOpenType(script) && fe->type() == QFontEngine::Freetype) {
            HB_Face hbFace = static_cast<QFontEngineFT *>(fe.data())->harfbuzzFace();
            if (!hbFace || !hbFace->supported_scripts[script]) {
                FM_DEBUG("  OpenType support missing for script\n");
                fe.reset(0);
            }
        }
    }
#endif

    if (! fe.isNull()) {
        if (fp) {
            QFontDef def = request;
            if (def.family.isEmpty()) {
                def.family = fp->request.family;
                def.family = def.family.left(def.family.indexOf(QLatin1Char(',')));
            }
            QFontCache::Key key(def, script);
            QFontCache::instance()->insertEngine(key, fe.data());
        }
    }

    if (fe.isNull()) {
        if (!request.family.isEmpty())
            return 0;

        FM_DEBUG("returning box engine");

        fe.reset(new QFontEngineBox(request.pixelSize));

        if (fp) {
            QFontCache::Key key(request, script);
            QFontCache::instance()->insertEngine(key, fe.data());
        }
    }

    if (fp && fp->dpi > 0) {
        fe->fontDef.pointSize = qreal(double((fe->fontDef.pixelSize * 72) / fp->dpi));
    } else {
        fe->fontDef.pointSize = request.pointSize;
    }

    return fe.take();
}

void QFontDatabase::load(const QFontPrivate *d, int script)
{
    QFontDef req = d->request;

    if (req.pixelSize == -1)
        req.pixelSize = qRound(req.pointSize*d->dpi/72);
    if (req.pointSize < 0)
        req.pointSize = req.pixelSize*72.0/d->dpi;

    if (!d->engineData) {
        QFontCache::Key key(req, script);

        // look for the requested font in the engine data cache
        d->engineData = QFontCache::instance()->findEngineData(key);

        if (!d->engineData) {
            // create a new one
            d->engineData = new QFontEngineData;
            QT_TRY {
                QFontCache::instance()->insertEngineData(key, d->engineData);
            } QT_CATCH(...) {
                delete d->engineData;
                d->engineData = 0;
                QT_RETHROW;
            }
        } else {
            d->engineData->ref.ref();
        }
    }

    // the cached engineData could have already loaded the engine we want
    if (d->engineData->engines[script]) return;

    //    double scale = 1.0; // ### TODO: fix the scale calculations

    // list of families to try
    QStringList family_list;

    if (!req.family.isEmpty()) {
        family_list = req.family.split(QLatin1Char(','));

        // append the substitute list for each family in family_list
        QStringList subs_list;
        QStringList::ConstIterator it = family_list.constBegin(), end = family_list.constEnd();
        for (; it != end; ++it)
            subs_list += QFont::substitutes(*it);
        family_list += subs_list;

        // append the default fallback font for the specified script
        // family_list << ... ; ###########

        // add the default family
        QString defaultFamily = QApplication::font().family();
        if (! family_list.contains(defaultFamily))
            family_list << defaultFamily;

        // add QFont::defaultFamily() to the list, for compatibility with
        // previous versions
        family_list << QApplication::font().defaultFamily();
    }

    // null family means find the first font matching the specified script
    family_list << QString();

    // load the font
    QFontEngine *engine = 0;
    QStringList::ConstIterator it = family_list.constBegin(), end = family_list.constEnd();
    for (; !engine && it != end; ++it) {
        req.family = *it;

        engine = QFontDatabase::findFont(script, d, req);
        if (engine && (engine->type()==QFontEngine::Box) && !req.family.isEmpty())
            engine = 0;
    }

    engine->ref.ref();
    d->engineData->engines[script] = engine;
}


QT_END_NAMESPACE
