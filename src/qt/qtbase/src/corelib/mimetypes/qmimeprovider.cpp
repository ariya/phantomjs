/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qmimeprovider_p.h"

#include "qmimetypeparser_p.h"
#include <qstandardpaths.h>
#include "qmimemagicrulematcher_p.h"

#include <QXmlStreamReader>
#include <QDir>
#include <QFile>
#include <QByteArrayMatcher>
#include <QDebug>
#include <QDateTime>
#include <QtEndian>

static void initResources()
{
    Q_INIT_RESOURCE(mimetypes);
}

QT_BEGIN_NAMESPACE

static QString fallbackParent(const QString &mimeTypeName)
{
    const QString myGroup = mimeTypeName.left(mimeTypeName.indexOf(QLatin1Char('/')));
    // All text/* types are subclasses of text/plain.
    if (myGroup == QLatin1String("text") && mimeTypeName != QLatin1String("text/plain"))
        return QLatin1String("text/plain");
    // All real-file mimetypes implicitly derive from application/octet-stream
    if (myGroup != QLatin1String("inode") &&
        // ignore non-file extensions
        myGroup != QLatin1String("all") && myGroup != QLatin1String("fonts") && myGroup != QLatin1String("print") && myGroup != QLatin1String("uri")
        && mimeTypeName != QLatin1String("application/octet-stream")) {
        return QLatin1String("application/octet-stream");
    }
    return QString();
}

QMimeProviderBase::QMimeProviderBase(QMimeDatabasePrivate *db)
    : m_db(db)
{
}

Q_CORE_EXPORT int qmime_secondsBetweenChecks = 5; // exported for the unit test

bool QMimeProviderBase::shouldCheck()
{
    const QDateTime now = QDateTime::currentDateTime();
    if (m_lastCheck.isValid() && m_lastCheck.secsTo(now) < qmime_secondsBetweenChecks)
        return false;
    m_lastCheck = now;
    return true;
}

QMimeBinaryProvider::QMimeBinaryProvider(QMimeDatabasePrivate *db)
    : QMimeProviderBase(db), m_mimetypeListLoaded(false)
{
}

#if defined(Q_OS_UNIX) && !defined(Q_OS_INTEGRITY)
#define QT_USE_MMAP
#endif

struct QMimeBinaryProvider::CacheFile
{
    CacheFile(const QString &fileName);
    ~CacheFile();

    bool isValid() const { return m_valid; }
    inline quint16 getUint16(int offset) const
    {
        return qFromBigEndian(*reinterpret_cast<quint16 *>(data + offset));
    }
    inline quint32 getUint32(int offset) const
    {
        return qFromBigEndian(*reinterpret_cast<quint32 *>(data + offset));
    }
    inline const char *getCharStar(int offset) const
    {
        return reinterpret_cast<const char *>(data + offset);
    }
    bool load();
    bool reload();

    QFile file;
    uchar *data;
    QDateTime m_mtime;
    bool m_valid;
};

QMimeBinaryProvider::CacheFile::CacheFile(const QString &fileName)
    : file(fileName), m_valid(false)
{
    load();
}

QMimeBinaryProvider::CacheFile::~CacheFile()
{
}

bool QMimeBinaryProvider::CacheFile::load()
{
    if (!file.open(QIODevice::ReadOnly))
        return false;
    data = file.map(0, file.size());
    if (data) {
        const int major = getUint16(0);
        const int minor = getUint16(2);
        m_valid = (major == 1 && minor >= 1 && minor <= 2);
    }
    m_mtime = QFileInfo(file).lastModified();
    return m_valid;
}

bool QMimeBinaryProvider::CacheFile::reload()
{
    //qDebug() << "reload!" << file->fileName();
    m_valid = false;
    if (file.isOpen()) {
        file.close();
    }
    data = 0;
    return load();
}

QMimeBinaryProvider::CacheFile *QMimeBinaryProvider::CacheFileList::findCacheFile(const QString &fileName) const
{
    for (const_iterator it = begin(); it != end(); ++it) {
        if ((*it)->file.fileName() == fileName)
            return *it;
    }
    return 0;
}

QMimeBinaryProvider::~QMimeBinaryProvider()
{
    qDeleteAll(m_cacheFiles);
}

// Position of the "list offsets" values, at the beginning of the mime.cache file
enum {
    PosAliasListOffset = 4,
    PosParentListOffset = 8,
    PosLiteralListOffset = 12,
    PosReverseSuffixTreeOffset = 16,
    PosGlobListOffset = 20,
    PosMagicListOffset = 24,
    // PosNamespaceListOffset = 28,
    PosIconsListOffset = 32,
    PosGenericIconsListOffset = 36
};

bool QMimeBinaryProvider::isValid()
{
#if defined(QT_USE_MMAP)
    if (!qEnvironmentVariableIsEmpty("QT_NO_MIME_CACHE"))
        return false;

    Q_ASSERT(m_cacheFiles.isEmpty()); // this method is only ever called once
    checkCache();

    if (m_cacheFiles.count() > 1)
        return true;
    if (m_cacheFiles.isEmpty())
        return false;

    // We found exactly one file; is it the user-modified mimes, or a system file?
    const QString foundFile = m_cacheFiles.first()->file.fileName();
    const QString localCacheFile = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/mime/mime.cache");

    return foundFile != localCacheFile;
#else
    return false;
#endif
}

bool QMimeBinaryProvider::CacheFileList::checkCacheChanged()
{
    bool somethingChanged = false;
    QMutableListIterator<CacheFile *> it(*this);
    while (it.hasNext()) {
        CacheFile *cacheFile = it.next();
        QFileInfo fileInfo(cacheFile->file);
        if (!fileInfo.exists()) { // This can't happen by just running update-mime-database. But the user could use rm -rf :-)
            delete cacheFile;
            it.remove();
            somethingChanged = true;
        } else if (fileInfo.lastModified() > cacheFile->m_mtime) {
            if (!cacheFile->reload()) {
                delete cacheFile;
                it.remove();
            }
            somethingChanged = true;
        }
    }
    return somethingChanged;
}

void QMimeBinaryProvider::checkCache()
{
    if (!shouldCheck())
        return;

    // First iterate over existing known cache files and check for uptodate
    if (m_cacheFiles.checkCacheChanged())
        m_mimetypeListLoaded = false;

    // Then check if new cache files appeared
    const QStringList cacheFileNames = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QLatin1String("mime/mime.cache"));
    if (cacheFileNames != m_cacheFileNames) {
        foreach (const QString &cacheFileName, cacheFileNames) {
            CacheFile *cacheFile = m_cacheFiles.findCacheFile(cacheFileName);
            if (!cacheFile) {
                //qDebug() << "new file:" << cacheFileName;
                cacheFile = new CacheFile(cacheFileName);
                if (cacheFile->isValid()) // verify version
                    m_cacheFiles.append(cacheFile);
                else
                    delete cacheFile;
            }
        }
        m_cacheFileNames = cacheFileNames;
        m_mimetypeListLoaded = false;
    }
}

static QMimeType mimeTypeForNameUnchecked(const QString &name)
{
    QMimeTypePrivate data;
    data.name = name;
    // The rest is retrieved on demand.
    // comment and globPatterns: in loadMimeTypePrivate
    // iconName: in loadIcon
    // genericIconName: in loadGenericIcon
    return QMimeType(data);
}

QMimeType QMimeBinaryProvider::mimeTypeForName(const QString &name)
{
    checkCache();
    if (!m_mimetypeListLoaded)
        loadMimeTypeList();
    if (!m_mimetypeNames.contains(name))
        return QMimeType(); // unknown mimetype
    return mimeTypeForNameUnchecked(name);
}

QStringList QMimeBinaryProvider::findByFileName(const QString &fileName, QString *foundSuffix)
{
    checkCache();
    if (fileName.isEmpty())
        return QStringList();
    const QString lowerFileName = fileName.toLower();
    QMimeGlobMatchResult result;
    // TODO this parses in the order (local, global). Check that it handles "NOGLOBS" correctly.
    foreach (CacheFile *cacheFile, m_cacheFiles) {
        matchGlobList(result, cacheFile, cacheFile->getUint32(PosLiteralListOffset), fileName);
        matchGlobList(result, cacheFile, cacheFile->getUint32(PosGlobListOffset), fileName);
        const int reverseSuffixTreeOffset = cacheFile->getUint32(PosReverseSuffixTreeOffset);
        const int numRoots = cacheFile->getUint32(reverseSuffixTreeOffset);
        const int firstRootOffset = cacheFile->getUint32(reverseSuffixTreeOffset + 4);
        matchSuffixTree(result, cacheFile, numRoots, firstRootOffset, lowerFileName, fileName.length() - 1, false);
        if (result.m_matchingMimeTypes.isEmpty())
            matchSuffixTree(result, cacheFile, numRoots, firstRootOffset, fileName, fileName.length() - 1, true);
    }
    if (foundSuffix)
        *foundSuffix = result.m_foundSuffix;
    return result.m_matchingMimeTypes;
}

void QMimeBinaryProvider::matchGlobList(QMimeGlobMatchResult &result, CacheFile *cacheFile, int off, const QString &fileName)
{
    const int numGlobs = cacheFile->getUint32(off);
    //qDebug() << "Loading" << numGlobs << "globs from" << cacheFile->file.fileName() << "at offset" << cacheFile->globListOffset;
    for (int i = 0; i < numGlobs; ++i) {
        const int globOffset = cacheFile->getUint32(off + 4 + 12 * i);
        const int mimeTypeOffset = cacheFile->getUint32(off + 4 + 12 * i + 4);
        const int flagsAndWeight = cacheFile->getUint32(off + 4 + 12 * i + 8);
        const int weight = flagsAndWeight & 0xff;
        const bool caseSensitive = flagsAndWeight & 0x100;
        const Qt::CaseSensitivity qtCaseSensitive = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
        const QString pattern = QLatin1String(cacheFile->getCharStar(globOffset));

        const char *mimeType = cacheFile->getCharStar(mimeTypeOffset);
        //qDebug() << pattern << mimeType << weight << caseSensitive;
        QMimeGlobPattern glob(pattern, QString() /*unused*/, weight, qtCaseSensitive);

        // TODO: this could be done faster for literals where a simple == would do.
        if (glob.matchFileName(fileName))
            result.addMatch(QLatin1String(mimeType), weight, pattern);
    }
}

bool QMimeBinaryProvider::matchSuffixTree(QMimeGlobMatchResult &result, QMimeBinaryProvider::CacheFile *cacheFile, int numEntries, int firstOffset, const QString &fileName, int charPos, bool caseSensitiveCheck)
{
    QChar fileChar = fileName[charPos];
    int min = 0;
    int max = numEntries - 1;
    while (min <= max) {
        const int mid = (min + max) / 2;
        const int off = firstOffset + 12 * mid;
        const QChar ch = cacheFile->getUint32(off);
        if (ch < fileChar)
            min = mid + 1;
        else if (ch > fileChar)
            max = mid - 1;
        else {
            --charPos;
            int numChildren = cacheFile->getUint32(off + 4);
            int childrenOffset = cacheFile->getUint32(off + 8);
            bool success = false;
            if (charPos > 0)
                success = matchSuffixTree(result, cacheFile, numChildren, childrenOffset, fileName, charPos, caseSensitiveCheck);
            if (!success) {
                for (int i = 0; i < numChildren; ++i) {
                    const int childOff = childrenOffset + 12 * i;
                    const int mch = cacheFile->getUint32(childOff);
                    if (mch != 0)
                        break;
                    const int mimeTypeOffset = cacheFile->getUint32(childOff + 4);
                    const char *mimeType = cacheFile->getCharStar(mimeTypeOffset);
                    const int flagsAndWeight = cacheFile->getUint32(childOff + 8);
                    const int weight = flagsAndWeight & 0xff;
                    const bool caseSensitive = flagsAndWeight & 0x100;
                    if (caseSensitiveCheck || !caseSensitive) {
                        result.addMatch(QLatin1String(mimeType), weight, QLatin1Char('*') + fileName.mid(charPos+1));
                        success = true;
                    }
                }
            }
            return success;
        }
    }
    return false;
}

bool QMimeBinaryProvider::matchMagicRule(QMimeBinaryProvider::CacheFile *cacheFile, int numMatchlets, int firstOffset, const QByteArray &data)
{
    const char *dataPtr = data.constData();
    const int dataSize = data.size();
    for (int matchlet = 0; matchlet < numMatchlets; ++matchlet) {
        const int off = firstOffset + matchlet * 32;
        const int rangeStart = cacheFile->getUint32(off);
        const int rangeLength = cacheFile->getUint32(off + 4);
        //const int wordSize = cacheFile->getUint32(off + 8);
        const int valueLength = cacheFile->getUint32(off + 12);
        const int valueOffset = cacheFile->getUint32(off + 16);
        const int maskOffset = cacheFile->getUint32(off + 20);
        const char *mask = maskOffset ? cacheFile->getCharStar(maskOffset) : NULL;

        if (!QMimeMagicRule::matchSubstring(dataPtr, dataSize, rangeStart, rangeLength, valueLength, cacheFile->getCharStar(valueOffset), mask))
            continue;

        const int numChildren = cacheFile->getUint32(off + 24);
        const int firstChildOffset = cacheFile->getUint32(off + 28);
        if (numChildren == 0) // No submatch? Then we are done.
            return true;
        // Check that one of the submatches matches too
        if (matchMagicRule(cacheFile, numChildren, firstChildOffset, data))
            return true;
    }
    return false;
}

QMimeType QMimeBinaryProvider::findByMagic(const QByteArray &data, int *accuracyPtr)
{
    checkCache();
    foreach (CacheFile *cacheFile, m_cacheFiles) {
        const int magicListOffset = cacheFile->getUint32(PosMagicListOffset);
        const int numMatches = cacheFile->getUint32(magicListOffset);
        //const int maxExtent = cacheFile->getUint32(magicListOffset + 4);
        const int firstMatchOffset = cacheFile->getUint32(magicListOffset + 8);

        for (int i = 0; i < numMatches; ++i) {
            const int off = firstMatchOffset + i * 16;
            const int numMatchlets = cacheFile->getUint32(off + 8);
            const int firstMatchletOffset = cacheFile->getUint32(off + 12);
            if (matchMagicRule(cacheFile, numMatchlets, firstMatchletOffset, data)) {
                const int mimeTypeOffset = cacheFile->getUint32(off + 4);
                const char *mimeType = cacheFile->getCharStar(mimeTypeOffset);
                *accuracyPtr = cacheFile->getUint32(off);
                // Return the first match. We have no rules for conflicting magic data...
                // (mime.cache itself is sorted, but what about local overrides with a lower prio?)
                return mimeTypeForNameUnchecked(QLatin1String(mimeType));
            }
        }
    }
    return QMimeType();
}

QStringList QMimeBinaryProvider::parents(const QString &mime)
{
    checkCache();
    const QByteArray mimeStr = mime.toLatin1();
    QStringList result;
    foreach (CacheFile *cacheFile, m_cacheFiles) {
        const int parentListOffset = cacheFile->getUint32(PosParentListOffset);
        const int numEntries = cacheFile->getUint32(parentListOffset);

        int begin = 0;
        int end = numEntries - 1;
        while (begin <= end) {
            const int medium = (begin + end) / 2;
            const int off = parentListOffset + 4 + 8 * medium;
            const int mimeOffset = cacheFile->getUint32(off);
            const char *aMime = cacheFile->getCharStar(mimeOffset);
            const int cmp = qstrcmp(aMime, mimeStr);
            if (cmp < 0) {
                begin = medium + 1;
            } else if (cmp > 0) {
                end = medium - 1;
            } else {
                const int parentsOffset = cacheFile->getUint32(off + 4);
                const int numParents = cacheFile->getUint32(parentsOffset);
                for (int i = 0; i < numParents; ++i) {
                    const int parentOffset = cacheFile->getUint32(parentsOffset + 4 + 4 * i);
                    const char *aParent = cacheFile->getCharStar(parentOffset);
                    result.append(QString::fromLatin1(aParent));
                }
                break;
            }
        }
    }
    if (result.isEmpty()) {
        const QString parent = fallbackParent(mime);
        if (!parent.isEmpty())
            result.append(parent);
    }
    return result;
}

QString QMimeBinaryProvider::resolveAlias(const QString &name)
{
    checkCache();
    const QByteArray input = name.toLatin1();
    foreach (CacheFile *cacheFile, m_cacheFiles) {
        const int aliasListOffset = cacheFile->getUint32(PosAliasListOffset);
        const int numEntries = cacheFile->getUint32(aliasListOffset);
        int begin = 0;
        int end = numEntries - 1;
        while (begin <= end) {
            const int medium = (begin + end) / 2;
            const int off = aliasListOffset + 4 + 8 * medium;
            const int aliasOffset = cacheFile->getUint32(off);
            const char *alias = cacheFile->getCharStar(aliasOffset);
            const int cmp = qstrcmp(alias, input);
            if (cmp < 0) {
                begin = medium + 1;
            } else if (cmp > 0) {
                end = medium - 1;
            } else {
                const int mimeOffset = cacheFile->getUint32(off + 4);
                const char *mimeType = cacheFile->getCharStar(mimeOffset);
                return QLatin1String(mimeType);
            }
        }
    }

    return name;
}

QStringList QMimeBinaryProvider::listAliases(const QString &name)
{
    checkCache();
    QStringList result;
    const QByteArray input = name.toLatin1();
    foreach (CacheFile *cacheFile, m_cacheFiles) {
        const int aliasListOffset = cacheFile->getUint32(PosAliasListOffset);
        const int numEntries = cacheFile->getUint32(aliasListOffset);
        for (int pos = 0; pos < numEntries; ++pos) {
            const int off = aliasListOffset + 4 + 8 * pos;
            const int mimeOffset = cacheFile->getUint32(off + 4);
            const char *mimeType = cacheFile->getCharStar(mimeOffset);

            if (input == mimeType) {
                const int aliasOffset = cacheFile->getUint32(off);
                const char *alias = cacheFile->getCharStar(aliasOffset);
                result.append(QString::fromLatin1(alias));
            }
        }
    }
    return result;
}

void QMimeBinaryProvider::loadMimeTypeList()
{
    if (!m_mimetypeListLoaded) {
        m_mimetypeListLoaded = true;
        m_mimetypeNames.clear();
        // Unfortunately mime.cache doesn't have a full list of all mimetypes.
        // So we have to parse the plain-text files called "types".
        const QStringList typesFilenames = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QLatin1String("mime/types"));
        foreach (const QString &typeFilename, typesFilenames) {
            QFile file(typeFilename);
            if (file.open(QIODevice::ReadOnly)) {
                while (!file.atEnd()) {
                    QByteArray line = file.readLine();
                    line.chop(1);
                    m_mimetypeNames.insert(QString::fromLatin1(line.constData(), line.size()));
                }
            }
        }
    }
}

QList<QMimeType> QMimeBinaryProvider::allMimeTypes()
{
    QList<QMimeType> result;
    loadMimeTypeList();

    for (QSet<QString>::const_iterator it = m_mimetypeNames.constBegin();
          it != m_mimetypeNames.constEnd(); ++it)
        result.append(mimeTypeForNameUnchecked(*it));

    return result;
}

void QMimeBinaryProvider::loadMimeTypePrivate(QMimeTypePrivate &data)
{
#ifdef QT_NO_XMLSTREAMREADER
    qWarning() << "Cannot load mime type since QXmlStreamReader is not available.";
    return;
#else
    if (data.loaded)
        return;
    data.loaded = true;
    // load comment and globPatterns

    const QString file = data.name + QLatin1String(".xml");
    const QStringList mimeFiles = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QString::fromLatin1("mime/") + file);
    if (mimeFiles.isEmpty()) {
        // TODO: ask Thiago about this
        qWarning() << "No file found for" << file << ", even though the file appeared in a directory listing.";
        qWarning() << "Either it was just removed, or the directory doesn't have executable permission...";
        qWarning() << QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QLatin1String("mime"), QStandardPaths::LocateDirectory);
        return;
    }

    QString comment;
    QString mainPattern;
    const QString preferredLanguage = QLocale::system().name();

    QListIterator<QString> mimeFilesIter(mimeFiles);
    mimeFilesIter.toBack();
    while (mimeFilesIter.hasPrevious()) { // global first, then local.
        const QString fullPath = mimeFilesIter.previous();
        QFile qfile(fullPath);
        if (!qfile.open(QFile::ReadOnly))
            continue;

        QXmlStreamReader xml(&qfile);
        if (xml.readNextStartElement()) {
            if (xml.name() != QLatin1String("mime-type")) {
                continue;
            }
            const QString name = xml.attributes().value(QLatin1String("type")).toString();
            if (name.isEmpty())
                continue;
            if (name != data.name) {
                qWarning() << "Got name" << name << "in file" << file << "expected" << data.name;
            }

            while (xml.readNextStartElement()) {
                const QStringRef tag = xml.name();
                if (tag == QLatin1String("comment")) {
                    QString lang = xml.attributes().value(QLatin1String("xml:lang")).toString();
                    const QString text = xml.readElementText();
                    if (lang.isEmpty()) {
                        lang = QLatin1String("en_US");
                    }
                    data.localeComments.insert(lang, text);
                    continue; // we called readElementText, so we're at the EndElement already.
                } else if (tag == QLatin1String("icon")) { // as written out by shared-mime-info >= 0.40
                    data.iconName = xml.attributes().value(QLatin1String("name")).toString();
                } else if (tag == QLatin1String("glob-deleteall")) { // as written out by shared-mime-info >= 0.70
                    data.globPatterns.clear();
                } else if (tag == QLatin1String("glob")) { // as written out by shared-mime-info >= 0.70
                    const QString pattern = xml.attributes().value(QLatin1String("pattern")).toString();
                    if (mainPattern.isEmpty() && pattern.startsWith(QLatin1Char('*'))) {
                        mainPattern = pattern;
                    }
                    if (!data.globPatterns.contains(pattern))
                        data.globPatterns.append(pattern);
                }
                xml.skipCurrentElement();
            }
            Q_ASSERT(xml.name() == QLatin1String("mime-type"));
        }
    }

    // Let's assume that shared-mime-info is at least version 0.70
    // Otherwise we would need 1) a version check, and 2) code for parsing patterns from the globs file.
#if 1
    if (!mainPattern.isEmpty() && data.globPatterns.first() != mainPattern) {
        // ensure it's first in the list of patterns
        data.globPatterns.removeAll(mainPattern);
        data.globPatterns.prepend(mainPattern);
    }
#else
    const bool globsInXml = sharedMimeInfoVersion() >= QT_VERSION_CHECK(0, 70, 0);
    if (globsInXml) {
        if (!mainPattern.isEmpty() && data.globPatterns.first() != mainPattern) {
            // ensure it's first in the list of patterns
            data.globPatterns.removeAll(mainPattern);
            data.globPatterns.prepend(mainPattern);
        }
    } else {
        // Fallback: get the patterns from the globs file
        // TODO: This would be the only way to support shared-mime-info < 0.70
        // But is this really worth the effort?
    }
#endif
#endif //QT_NO_XMLSTREAMREADER
}

// Binary search in the icons or generic-icons list
QString QMimeBinaryProvider::iconForMime(CacheFile *cacheFile, int posListOffset, const QByteArray &inputMime)
{
    const int iconsListOffset = cacheFile->getUint32(posListOffset);
    const int numIcons = cacheFile->getUint32(iconsListOffset);
    int begin = 0;
    int end = numIcons - 1;
    while (begin <= end) {
        const int medium = (begin + end) / 2;
        const int off = iconsListOffset + 4 + 8 * medium;
        const int mimeOffset = cacheFile->getUint32(off);
        const char *mime = cacheFile->getCharStar(mimeOffset);
        const int cmp = qstrcmp(mime, inputMime);
        if (cmp < 0)
            begin = medium + 1;
        else if (cmp > 0)
            end = medium - 1;
        else {
            const int iconOffset = cacheFile->getUint32(off + 4);
            return QLatin1String(cacheFile->getCharStar(iconOffset));
        }
    }
    return QString();
}

void QMimeBinaryProvider::loadIcon(QMimeTypePrivate &data)
{
    checkCache();
    const QByteArray inputMime = data.name.toLatin1();
    foreach (CacheFile *cacheFile, m_cacheFiles) {
        const QString icon = iconForMime(cacheFile, PosIconsListOffset, inputMime);
        if (!icon.isEmpty()) {
            data.iconName = icon;
            return;
        }
    }
}

void QMimeBinaryProvider::loadGenericIcon(QMimeTypePrivate &data)
{
    checkCache();
    const QByteArray inputMime = data.name.toLatin1();
    foreach (CacheFile *cacheFile, m_cacheFiles) {
        const QString icon = iconForMime(cacheFile, PosGenericIconsListOffset, inputMime);
        if (!icon.isEmpty()) {
            data.genericIconName = icon;
            return;
        }
    }
}

////

QMimeXMLProvider::QMimeXMLProvider(QMimeDatabasePrivate *db)
    : QMimeProviderBase(db), m_loaded(false)
{
    initResources();
}

bool QMimeXMLProvider::isValid()
{
    return true;
}

QMimeType QMimeXMLProvider::mimeTypeForName(const QString &name)
{
    ensureLoaded();

    return m_nameMimeTypeMap.value(name);
}

QStringList QMimeXMLProvider::findByFileName(const QString &fileName, QString *foundSuffix)
{
    ensureLoaded();

    const QStringList matchingMimeTypes = m_mimeTypeGlobs.matchingGlobs(fileName, foundSuffix);
    return matchingMimeTypes;
}

QMimeType QMimeXMLProvider::findByMagic(const QByteArray &data, int *accuracyPtr)
{
    ensureLoaded();

    QString candidate;

    foreach (const QMimeMagicRuleMatcher &matcher, m_magicMatchers) {
        if (matcher.matches(data)) {
            const int priority = matcher.priority();
            if (priority > *accuracyPtr) {
                *accuracyPtr = priority;
                candidate = matcher.mimetype();
            }
        }
    }
    return mimeTypeForName(candidate);
}

void QMimeXMLProvider::ensureLoaded()
{
    if (!m_loaded || shouldCheck()) {
        bool fdoXmlFound = false;
        QStringList allFiles;

        const QStringList packageDirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QLatin1String("mime/packages"), QStandardPaths::LocateDirectory);
        //qDebug() << "packageDirs=" << packageDirs;
        foreach (const QString &packageDir, packageDirs) {
            QDir dir(packageDir);
            const QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
            //qDebug() << static_cast<const void *>(this) << Q_FUNC_INFO << packageDir << files;
            if (!fdoXmlFound)
                fdoXmlFound = files.contains(QLatin1String("freedesktop.org.xml"));
            QStringList::const_iterator endIt(files.constEnd());
            for (QStringList::const_iterator it(files.constBegin()); it != endIt; ++it) {
                allFiles.append(packageDir + QLatin1Char('/') + *it);
            }
        }

        if (!fdoXmlFound) {
            // We could instead install the file as part of installing Qt?
            allFiles.prepend(QLatin1String(":/qt-project.org/qmime/freedesktop.org.xml"));
        }

        if (m_allFiles == allFiles)
            return;
        m_allFiles = allFiles;

        m_nameMimeTypeMap.clear();
        m_aliases.clear();
        m_parents.clear();
        m_mimeTypeGlobs.clear();
        m_magicMatchers.clear();

        //qDebug() << "Loading" << m_allFiles;

        foreach (const QString &file, allFiles)
            load(file);
    }
}

void QMimeXMLProvider::load(const QString &fileName)
{
    QString errorMessage;
    if (!load(fileName, &errorMessage))
        qWarning("QMimeDatabase: Error loading %s\n%s", qPrintable(fileName), qPrintable(errorMessage));
}

bool QMimeXMLProvider::load(const QString &fileName, QString *errorMessage)
{
    m_loaded = true;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = QString::fromLatin1("Cannot open %1: %2").arg(fileName, file.errorString());
        return false;
    }

    if (errorMessage)
        errorMessage->clear();

    QMimeTypeParser parser(*this);
    return parser.parse(&file, fileName, errorMessage);
}

void QMimeXMLProvider::addGlobPattern(const QMimeGlobPattern &glob)
{
    m_mimeTypeGlobs.addGlob(glob);
}

void QMimeXMLProvider::addMimeType(const QMimeType &mt)
{
    m_nameMimeTypeMap.insert(mt.name(), mt);
}

QStringList QMimeXMLProvider::parents(const QString &mime)
{
    ensureLoaded();
    QStringList result = m_parents.value(mime);
    if (result.isEmpty()) {
        const QString parent = fallbackParent(mime);
        if (!parent.isEmpty())
            result.append(parent);
    }
    return result;
}

void QMimeXMLProvider::addParent(const QString &child, const QString &parent)
{
    m_parents[child].append(parent);
}

QStringList QMimeXMLProvider::listAliases(const QString &name)
{
    ensureLoaded();
    // Iterate through the whole hash. This method is rarely used.
    return m_aliases.keys(name);
}

QString QMimeXMLProvider::resolveAlias(const QString &name)
{
    ensureLoaded();
    return m_aliases.value(name, name);
}

void QMimeXMLProvider::addAlias(const QString &alias, const QString &name)
{
    m_aliases.insert(alias, name);
}

QList<QMimeType> QMimeXMLProvider::allMimeTypes()
{
    ensureLoaded();
    return m_nameMimeTypeMap.values();
}

void QMimeXMLProvider::addMagicMatcher(const QMimeMagicRuleMatcher &matcher)
{
    m_magicMatchers.append(matcher);
}

QT_END_NAMESPACE
