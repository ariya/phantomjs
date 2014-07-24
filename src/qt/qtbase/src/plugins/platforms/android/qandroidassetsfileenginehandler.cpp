/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qandroidassetsfileenginehandler.h"
#include "androidjnimain.h"

#include <QCoreApplication>
#include <QVector>

typedef QVector<QString> FilesList;

struct AndroidAssetDir
{
    AndroidAssetDir(AAssetDir* ad)
    {
        if (ad) {
            const char *fileName;
            while ((fileName = AAssetDir_getNextFileName(ad)))
                m_items.push_back(QString::fromUtf8(fileName));
            AAssetDir_close(ad);
        }
    }
    FilesList m_items;
};

class AndroidAbstractFileEngineIterator: public QAbstractFileEngineIterator
{
public:
    AndroidAbstractFileEngineIterator(QDir::Filters filters,
                                      const QStringList &nameFilters,
                                      QSharedPointer<AndroidAssetDir> asset,
                                      const QString &path)
        : QAbstractFileEngineIterator(filters, nameFilters)
    {
        m_items = asset->m_items;
        m_index = -1;
        m_path = path;
    }

    virtual QFileInfo currentFileInfo() const
    {
        return QFileInfo(currentFilePath());
    }

    virtual QString currentFileName() const
    {
        if (m_index < 0 || m_index >= m_items.size())
            return QString();
        QString fileName = m_items[m_index];
        if (fileName.endsWith(QLatin1Char('/')))
            fileName.chop(1);
        return fileName;
    }

    virtual QString currentFilePath() const
    {
        return m_path + currentFileName();
    }

    virtual bool hasNext() const
    {
        return m_items.size() && (m_index < m_items.size() - 1);
    }

    virtual QString next()
    {
        if (!hasNext())
            return QString();
        m_index++;
        return currentFileName();
    }

private:
    QString     m_path;
    FilesList   m_items;
    int         m_index;
};

class AndroidAbstractFileEngine: public QAbstractFileEngine
{
public:
    explicit AndroidAbstractFileEngine(AAsset *asset, const QString &fileName)
    {
        m_assetFile = asset;
        m_fileName = fileName;
    }

    explicit AndroidAbstractFileEngine(QSharedPointer<AndroidAssetDir> asset, const QString &fileName)
    {
        m_assetFile = 0;
        m_assetDir = asset;
        m_fileName =  fileName;
        if (!m_fileName.endsWith(QChar(QLatin1Char('/'))))
            m_fileName += "/";
    }

    ~AndroidAbstractFileEngine()
    {
        close();
    }

    virtual bool open(QIODevice::OpenMode openMode)
    {
        if (m_assetFile)
            return openMode & QIODevice::ReadOnly;
        return false;
    }

    virtual bool close()
    {
        if (m_assetFile) {
            AAsset_close(m_assetFile);
            m_assetFile = 0;
            return true;
        }
        return false;
    }

    virtual qint64 size() const
    {
        if (m_assetFile)
            return AAsset_getLength(m_assetFile);
        return -1;
    }

    virtual qint64 pos() const
    {
        if (m_assetFile)
            return AAsset_seek(m_assetFile, 0, SEEK_CUR);
        return -1;
    }

    virtual bool seek(qint64 pos)
    {
        if (m_assetFile)
            return pos == AAsset_seek(m_assetFile, pos, SEEK_SET);
        return false;
    }

    virtual qint64 read(char *data, qint64 maxlen)
    {
        if (m_assetFile)
            return AAsset_read(m_assetFile, data, maxlen);
        return -1;
    }

    virtual bool isSequential() const
    {
        return false;
    }

    virtual bool caseSensitive() const
    {
        return true;
    }

    virtual bool isRelativePath() const
    {
        return false;
    }

    virtual FileFlags fileFlags(FileFlags type = FileInfoAll) const
    {
        FileFlags flags(ReadOwnerPerm|ReadUserPerm|ReadGroupPerm|ReadOtherPerm|ExistsFlag);
        if (m_assetFile)
            flags |= FileType;
        if (!m_assetDir.isNull())
            flags |= DirectoryType;

        return type & flags;
    }

    virtual QString fileName(FileName file = DefaultName) const
    {
        int pos;
        switch (file) {
        case DefaultName:
        case AbsoluteName:
        case CanonicalName:
                return m_fileName;
        case BaseName:
            if ((pos = m_fileName.lastIndexOf(QChar(QLatin1Char('/')))) != -1)
                return m_fileName.mid(pos);
            else
                return m_fileName;
        case PathName:
        case AbsolutePathName:
        case CanonicalPathName:
            if ((pos = m_fileName.lastIndexOf(QChar(QLatin1Char('/')))) != -1)
                return m_fileName.left(pos);
            else
                return m_fileName;
        default:
            return QString();
        }
    }

    virtual void setFileName(const QString &file)
    {
        if (file == m_fileName)
            return;

        m_fileName = file;
        if (!m_fileName.endsWith(QChar(QLatin1Char('/'))))
            m_fileName += "/";

        close();
    }

    virtual Iterator *beginEntryList(QDir::Filters filters, const QStringList &filterNames)
    {
        if (!m_assetDir.isNull())
            return new AndroidAbstractFileEngineIterator(filters, filterNames, m_assetDir, m_fileName);
        return 0;
    }

private:
    AAsset *m_assetFile;
    QSharedPointer<AndroidAssetDir> m_assetDir;
    QString m_fileName;
};


AndroidAssetsFileEngineHandler::AndroidAssetsFileEngineHandler()
    : m_assetsCache(std::max(5, qgetenv("QT_ANDROID_MAX_ASSETS_CACHE_SIZE").toInt()))
    , m_hasPrepopulatedCache(false)
    , m_hasTriedPrepopulatingCache(false)
{
    m_assetManager = QtAndroid::assetManager();
}

AndroidAssetsFileEngineHandler::~AndroidAssetsFileEngineHandler()
{
}

void AndroidAssetsFileEngineHandler::prepopulateCache() const
{
    Q_ASSERT(!m_hasTriedPrepopulatingCache);
    m_hasTriedPrepopulatingCache = true;

    Q_ASSERT(m_assetsCache.isEmpty());

    // Failsafe: Don't read cache files that are larger than 1MB
    static qint64 maxPrepopulatedCacheSize = qMax(1024LL * 1024LL,
                                                  qgetenv("QT_ANDROID_MAX_PREPOPULATED_ASSETS_CACHE_SIZE").toLongLong());

    const char *fileName = "--Added-by-androiddeployqt--/qt_cache_pregenerated_file_list";
    AAsset *asset = AAssetManager_open(m_assetManager, fileName, AASSET_MODE_BUFFER);
    if (asset) {
        m_hasPrepopulatedCache = true;
        AndroidAbstractFileEngine fileEngine(asset, QString::fromLatin1(fileName));
        if (fileEngine.open(QIODevice::ReadOnly)) {
            qint64 size = fileEngine.size();

            if (size <= maxPrepopulatedCacheSize) {
                QByteArray bytes(size, Qt::Uninitialized);
                qint64 read = fileEngine.read(bytes.data(), size);
                if (read != size) {
                    qWarning("Failed to read prepopulated cache");
                    return;
                }

                QDataStream stream(&bytes, QIODevice::ReadOnly);
                stream.setVersion(QDataStream::Qt_5_3);
                if (stream.status() != QDataStream::Ok) {
                    qWarning("Failed to read prepopulated cache");
                    return;
                }

                while (!stream.atEnd()) {
                    QString directoryName;
                    stream >> directoryName;

                    int fileCount;
                    stream >> fileCount;

                    QVector<QString> fileList;
                    fileList.reserve(fileCount);
                    while (fileCount--) {
                        QString fileName;
                        stream >> fileName;
                        fileList.append(fileName);
                    }

                    QSharedPointer<AndroidAssetDir> *aad = new QSharedPointer<AndroidAssetDir>(new AndroidAssetDir(0));
                    (*aad)->m_items = fileList;

                    // Cost = 0, because we should always cache everything if there's a prepopulated cache
                    QByteArray key = directoryName != QLatin1String("/")
                            ? QByteArray("assets:/") + directoryName.toUtf8()
                            : QByteArray("assets:");

                    bool ok = m_assetsCache.insert(key, aad, 0);
                    if (!ok)
                        qWarning("Failed to insert in cache: %s", qPrintable(directoryName));
                }
            } else {
                qWarning("Prepopulated cache is too large to read.\n"
                         "Use environment variable QT_ANDROID_MAX_PREPOPULATED_ASSETS_CACHE_SIZE to adjust size.");
            }
        }
    }
}

QAbstractFileEngine * AndroidAssetsFileEngineHandler::create(const QString &fileName) const
{
    if (fileName.isEmpty())
        return 0;

    static QLatin1String assetsPrefix("assets:");
    if (!fileName.startsWith(assetsPrefix))
        return 0;

    static int prefixSize = assetsPrefix.size() + 1;

    QByteArray path;
    if (!fileName.endsWith(QLatin1Char('/'))) {
        path = fileName.toUtf8();
        if (path.size() > prefixSize) {
            AAsset *asset = AAssetManager_open(m_assetManager,
                                               path.constData() + prefixSize,
                                               AASSET_MODE_BUFFER);
            if (asset)
                return new AndroidAbstractFileEngine(asset, fileName);
        }
    }

    if (!path.size())
         path = fileName.left(fileName.length() - 1).toUtf8();


    m_assetsCacheMutext.lock();
    if (!m_hasTriedPrepopulatingCache)
        prepopulateCache();

    QSharedPointer<AndroidAssetDir> *aad = m_assetsCache.object(path);
    m_assetsCacheMutext.unlock();
    if (!aad) {
        if (!m_hasPrepopulatedCache && path.size() > prefixSize) {
            AAssetDir *assetDir = AAssetManager_openDir(m_assetManager, path.constData() + prefixSize);
            if (assetDir) {
                if (AAssetDir_getNextFileName(assetDir)) {
                    AAssetDir_rewind(assetDir);
                    aad = new QSharedPointer<AndroidAssetDir>(new AndroidAssetDir(assetDir));
                    m_assetsCacheMutext.lock();
                    m_assetsCache.insert(path, aad);
                    m_assetsCacheMutext.unlock();
                    return new AndroidAbstractFileEngine(*aad, fileName);
                } else {
                    AAssetDir_close(assetDir);
                }
            }
        }
    } else {
        return new AndroidAbstractFileEngine(*aad, fileName);
    }
    return 0;
}
