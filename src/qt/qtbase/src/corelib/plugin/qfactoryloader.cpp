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

#include "qfactoryloader_p.h"

#ifndef QT_NO_LIBRARY
#include "qfactoryinterface.h"
#include "qmap.h"
#include <qdir.h>
#include <qdebug.h>
#include "qmutex.h"
#include "qplugin.h"
#include "qpluginloader.h"
#include "private/qobject_p.h"
#include "private/qcoreapplication_p.h"
#include "qjsondocument.h"
#include "qjsonvalue.h"
#include "qjsonobject.h"
#include "qjsonarray.h"

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QList<QFactoryLoader *>, qt_factory_loaders)

Q_GLOBAL_STATIC_WITH_ARGS(QMutex, qt_factoryloader_mutex, (QMutex::Recursive))

class QFactoryLoaderPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QFactoryLoader)
public:
    QFactoryLoaderPrivate(){}
    ~QFactoryLoaderPrivate();
    mutable QMutex mutex;
    QByteArray iid;
    QList<QLibraryPrivate*> libraryList;
    QMap<QString,QLibraryPrivate*> keyMap;
    QString suffix;
    Qt::CaseSensitivity cs;
    QStringList loadedPaths;

    void unloadPath(const QString &path);
};

QFactoryLoaderPrivate::~QFactoryLoaderPrivate()
{
    for (int i = 0; i < libraryList.count(); ++i) {
        QLibraryPrivate *library = libraryList.at(i);
        library->unload();
        library->release();
    }
}

QFactoryLoader::QFactoryLoader(const char *iid,
                               const QString &suffix,
                               Qt::CaseSensitivity cs)
    : QObject(*new QFactoryLoaderPrivate)
{
    moveToThread(QCoreApplicationPrivate::mainThread());
    Q_D(QFactoryLoader);
    d->iid = iid;
    d->cs = cs;
    d->suffix = suffix;


    QMutexLocker locker(qt_factoryloader_mutex());
    update();
    qt_factory_loaders()->append(this);
}



void QFactoryLoader::update()
{
#ifdef QT_SHARED
    Q_D(QFactoryLoader);
    QStringList paths = QCoreApplication::libraryPaths();
    for (int i = 0; i < paths.count(); ++i) {
        const QString &pluginDir = paths.at(i);
        // Already loaded, skip it...
        if (d->loadedPaths.contains(pluginDir))
            continue;
        d->loadedPaths << pluginDir;

        QString path = pluginDir + d->suffix;

        if (qt_debug_component())
            qDebug() << "QFactoryLoader::QFactoryLoader() checking directory path" << path << "...";

        if (!QDir(path).exists(QLatin1String(".")))
            continue;

        QStringList plugins = QDir(path).entryList(QDir::Files);
        QLibraryPrivate *library = 0;

#ifdef Q_OS_MAC
        // Loading both the debug and release version of the cocoa plugins causes the objective-c runtime
        // to print "duplicate class definitions" warnings. Detect if QFactoryLoader is about to load both,
        // skip one of them (below).
        //
        // ### FIXME find a proper solution
        //
        const bool isLoadingDebugAndReleaseCocoa = plugins.contains(QStringLiteral("libqcocoa_debug.dylib"))
                && plugins.contains(QStringLiteral("libqcocoa.dylib"));
#endif
        for (int j = 0; j < plugins.count(); ++j) {
            QString fileName = QDir::cleanPath(path + QLatin1Char('/') + plugins.at(j));

#ifdef Q_OS_MAC
            if (isLoadingDebugAndReleaseCocoa) {
#ifdef QT_DEBUG
               if (fileName.contains(QStringLiteral("libqcocoa.dylib")))
                   continue;    // Skip release plugin in debug mode
#else
               if (fileName.contains(QStringLiteral("libqcocoa_debug.dylib")))
                   continue;    // Skip debug plugin in release mode
#endif
            }
#endif
            if (qt_debug_component()) {
                qDebug() << "QFactoryLoader::QFactoryLoader() looking at" << fileName;
            }
            library = QLibraryPrivate::findOrCreate(QFileInfo(fileName).canonicalFilePath());
            if (!library->isPlugin()) {
                if (qt_debug_component()) {
                    qDebug() << library->errorString;
                    qDebug() << "         not a plugin";
                }
                library->release();
                continue;
            }

            QStringList keys;
            bool metaDataOk = false;

            QString iid = library->metaData.value(QLatin1String("IID")).toString();
            if (iid == QLatin1String(d->iid.constData(), d->iid.size())) {
                QJsonObject object = library->metaData.value(QLatin1String("MetaData")).toObject();
                metaDataOk = true;

                QJsonArray k = object.value(QLatin1String("Keys")).toArray();
                for (int i = 0; i < k.size(); ++i)
                    keys += d->cs ? k.at(i).toString() : k.at(i).toString().toLower();
            }
            if (qt_debug_component())
                qDebug() << "Got keys from plugin meta data" << keys;


            if (!metaDataOk) {
                library->release();
                continue;
            }

            int keyUsageCount = 0;
            for (int k = 0; k < keys.count(); ++k) {
                // first come first serve, unless the first
                // library was built with a future Qt version,
                // whereas the new one has a Qt version that fits
                // better
                const QString &key = keys.at(k);
                QLibraryPrivate *previous = d->keyMap.value(key);
                int prev_qt_version = 0;
                if (previous) {
                    prev_qt_version = (int)previous->metaData.value(QLatin1String("version")).toDouble();
                }
                int qt_version = (int)library->metaData.value(QLatin1String("version")).toDouble();
                if (!previous || (prev_qt_version > QT_VERSION && qt_version <= QT_VERSION)) {
                    d->keyMap[key] = library;
                    ++keyUsageCount;
                }
            }
            if (keyUsageCount || keys.isEmpty())
                d->libraryList += library;
            else
                library->release();
        }
    }
#else
    Q_D(QFactoryLoader);
    if (qt_debug_component()) {
        qDebug() << "QFactoryLoader::QFactoryLoader() ignoring" << d->iid
                 << "since plugins are disabled in static builds";
    }
#endif
}

QFactoryLoader::~QFactoryLoader()
{
    QMutexLocker locker(qt_factoryloader_mutex());
    qt_factory_loaders()->removeAll(this);
}

QList<QJsonObject> QFactoryLoader::metaData() const
{
    Q_D(const QFactoryLoader);
    QMutexLocker locker(&d->mutex);
    QList<QJsonObject> metaData;
    for (int i = 0; i < d->libraryList.size(); ++i)
        metaData.append(d->libraryList.at(i)->metaData);

    foreach (const QStaticPlugin &plugin, QPluginLoader::staticPlugins()) {
        const QJsonObject object = plugin.metaData();
        if (object.value(QLatin1String("IID")) != QLatin1String(d->iid.constData(), d->iid.size()))
            continue;
        metaData.append(object);
    }
    return metaData;
}

QObject *QFactoryLoader::instance(int index) const
{
    Q_D(const QFactoryLoader);
    if (index < 0)
        return 0;

    if (index < d->libraryList.size()) {
        QLibraryPrivate *library = d->libraryList.at(index);
        if (library->instance || library->loadPlugin()) {
            if (!library->inst)
                library->inst = library->instance();
            QObject *obj = library->inst.data();
            if (obj) {
                if (!obj->parent())
                    obj->moveToThread(QCoreApplicationPrivate::mainThread());
                return obj;
            }
        }
        return 0;
    }

    index -= d->libraryList.size();
    QVector<QStaticPlugin> staticPlugins = QPluginLoader::staticPlugins();
    for (int i = 0; i < staticPlugins.count(); ++i) {
        const QJsonObject object = staticPlugins.at(i).metaData();
        if (object.value(QLatin1String("IID")) != QLatin1String(d->iid.constData(), d->iid.size()))
            continue;

        if (index == 0)
            return staticPlugins.at(i).instance();
        --index;
    }

    return 0;
}

#if defined(Q_OS_UNIX) && !defined (Q_OS_MAC)
QLibraryPrivate *QFactoryLoader::library(const QString &key) const
{
    Q_D(const QFactoryLoader);
    return d->keyMap.value(d->cs ? key : key.toLower());
}
#endif

void QFactoryLoader::refreshAll()
{
    QMutexLocker locker(qt_factoryloader_mutex());
    QList<QFactoryLoader *> *loaders = qt_factory_loaders();
    for (QList<QFactoryLoader *>::const_iterator it = loaders->constBegin();
         it != loaders->constEnd(); ++it) {
        (*it)->update();
    }
}

QMultiMap<int, QString> QFactoryLoader::keyMap() const
{
    QMultiMap<int, QString> result;
    const QString metaDataKey = QStringLiteral("MetaData");
    const QString keysKey = QStringLiteral("Keys");
    const QList<QJsonObject> metaDataList = metaData();
    for (int i = 0; i < metaDataList.size(); ++i) {
        const QJsonObject metaData = metaDataList.at(i).value(metaDataKey).toObject();
        const QJsonArray keys = metaData.value(keysKey).toArray();
        const int keyCount = keys.size();
        for (int k = 0; k < keyCount; ++k)
            result.insert(i, keys.at(k).toString());
    }
    return result;
}

int QFactoryLoader::indexOf(const QString &needle) const
{
    const QString metaDataKey = QStringLiteral("MetaData");
    const QString keysKey = QStringLiteral("Keys");
    const QList<QJsonObject> metaDataList = metaData();
    for (int i = 0; i < metaDataList.size(); ++i) {
        const QJsonObject metaData = metaDataList.at(i).value(metaDataKey).toObject();
        const QJsonArray keys = metaData.value(keysKey).toArray();
        const int keyCount = keys.size();
        for (int k = 0; k < keyCount; ++k) {
            if (!keys.at(k).toString().compare(needle, Qt::CaseInsensitive))
                return i;
        }
    }
    return -1;
}

QT_END_NAMESPACE

#endif // QT_NO_LIBRARY
