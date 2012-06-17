/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qfactoryloader_p.h"

#ifndef QT_NO_LIBRARY
#include "qfactoryinterface.h"
#include "qmap.h"
#include <qdir.h>
#include <qsettings.h>
#include <qdebug.h>
#include "qmutex.h"
#include "qplugin.h"
#include "qpluginloader.h"
#include "qlibraryinfo.h"
#include "private/qobject_p.h"
#include "private/qcoreapplication_p.h"
#ifdef Q_OS_SYMBIAN
#include "private/qcore_symbian_p.h"
#include "private/qfilesystemwatcher_symbian_p.h"
#endif

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QList<QFactoryLoader *>, qt_factory_loaders)

Q_GLOBAL_STATIC_WITH_ARGS(QMutex, qt_factoryloader_mutex, (QMutex::Recursive))

#ifdef Q_OS_SYMBIAN
class QSymbianSystemPluginWatcher : public QSymbianFileSystemWatcherInterface
{
public:
    QSymbianSystemPluginWatcher();
    ~QSymbianSystemPluginWatcher();

    void watchForUpdates();
    void handlePathChanged(QNotifyChangeEvent *e);

    QList<QNotifyChangeEvent*> watchers;
    TDriveList drives;
};

Q_GLOBAL_STATIC(QSymbianSystemPluginWatcher, qt_symbian_system_plugin_watcher)
#endif

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
    QStringList keyList;
    QString suffix;
    Qt::CaseSensitivity cs;
    QStringList loadedPaths;

    void unloadPath(const QString &path);
};

QFactoryLoaderPrivate::~QFactoryLoaderPrivate()
{
    for (int i = 0; i < libraryList.count(); ++i)
        libraryList.at(i)->release();
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
#ifdef Q_OS_SYMBIAN
    // kick off Symbian plugin watcher for updates
    qt_symbian_system_plugin_watcher();
#endif
}


void QFactoryLoader::updateDir(const QString &pluginDir, QSettings& settings)
{
    Q_D(QFactoryLoader);
    QString path = pluginDir + d->suffix;
    if (!QDir(path).exists(QLatin1String(".")))
        return;

    QStringList plugins = QDir(path).entryList(QDir::Files);
    QLibraryPrivate *library = 0;
    for (int j = 0; j < plugins.count(); ++j) {
        QString fileName = QDir::cleanPath(path + QLatin1Char('/') + plugins.at(j));

        if (qt_debug_component()) {
            qDebug() << "QFactoryLoader::QFactoryLoader() looking at" << fileName;
        }
        library = QLibraryPrivate::findOrCreate(QFileInfo(fileName).canonicalFilePath());
        if (!library->isPlugin(&settings)) {
            if (qt_debug_component()) {
                qDebug() << library->errorString;
                qDebug() << "         not a plugin";
            }
            library->release();
            continue;
        }
        QString regkey = QString::fromLatin1("Qt Factory Cache %1.%2/%3:/%4")
                         .arg((QT_VERSION & 0xff0000) >> 16)
                         .arg((QT_VERSION & 0xff00) >> 8)
                         .arg(QLatin1String(d->iid))
                         .arg(fileName);
        QStringList reg, keys;
        reg = settings.value(regkey).toStringList();
        if (reg.count() && library->lastModified == reg[0]) {
            keys = reg;
            keys.removeFirst();
        } else {
            if (!library->loadPlugin()) {
                if (qt_debug_component()) {
                    qDebug() << library->errorString;
                    qDebug() << "           could not load";
                }
                library->release();
                continue;
            }
            QObject *instance = library->instance();
            if (!instance) {
                library->release();
                // ignore plugins that have a valid signature but cannot be loaded.
                continue;
            }
            QFactoryInterface *factory = qobject_cast<QFactoryInterface*>(instance);
            if (instance && factory && instance->qt_metacast(d->iid))
                keys = factory->keys();
            if (keys.isEmpty())
                library->unload();
            reg.clear();
            reg << library->lastModified;
            reg += keys;
            settings.setValue(regkey, reg);
        }
        if (qt_debug_component()) {
            qDebug() << "keys" << keys;
        }

        if (keys.isEmpty()) {
            library->release();
            continue;
        }

        int keysUsed = 0;
        for (int k = 0; k < keys.count(); ++k) {
            // first come first serve, unless the first
            // library was built with a future Qt version,
            // whereas the new one has a Qt version that fits
            // better
            QString key = keys.at(k);
            if (!d->cs)
                key = key.toLower();
            QLibraryPrivate *previous = d->keyMap.value(key);
            if (!previous || (previous->qt_version > QT_VERSION && library->qt_version <= QT_VERSION)) {
                d->keyMap[key] = library;
                d->keyList += keys.at(k);
                keysUsed++;
            }
        }
        if (keysUsed)
            d->libraryList += library;
        else
            library->release();
    }
}

void QFactoryLoader::update()
{
#ifdef QT_SHARED
    Q_D(QFactoryLoader);
    QStringList paths = QCoreApplication::libraryPaths();
    QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
    for (int i = 0; i < paths.count(); ++i) {
        const QString &pluginDir = paths.at(i);
        // Already loaded, skip it...
        if (d->loadedPaths.contains(pluginDir))
            continue;
        d->loadedPaths << pluginDir;
        updateDir(pluginDir, settings);
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

QStringList QFactoryLoader::keys() const
{
    Q_D(const QFactoryLoader);
    QMutexLocker locker(&d->mutex);
    QStringList keys = d->keyList;
    QObjectList instances = QPluginLoader::staticInstances();
    for (int i = 0; i < instances.count(); ++i)
        if (QFactoryInterface *factory = qobject_cast<QFactoryInterface*>(instances.at(i)))
            if (instances.at(i)->qt_metacast(d->iid))
                keys += factory->keys();
    return keys;
}

QObject *QFactoryLoader::instance(const QString &key) const
{
    Q_D(const QFactoryLoader);
    QMutexLocker locker(&d->mutex);
    QObjectList instances = QPluginLoader::staticInstances();
    for (int i = 0; i < instances.count(); ++i)
        if (QFactoryInterface *factory = qobject_cast<QFactoryInterface*>(instances.at(i)))
            if (instances.at(i)->qt_metacast(d->iid) && factory->keys().contains(key, Qt::CaseInsensitive))
                return instances.at(i);

    QString lowered = d->cs ? key : key.toLower();
    if (QLibraryPrivate* library = d->keyMap.value(lowered)) {
        if (library->instance || library->loadPlugin()) {
            if (QObject *obj = library->instance()) {
                if (obj && !obj->parent())
                    obj->moveToThread(QCoreApplicationPrivate::mainThread());
                return obj;
            }
        }
    }
    return 0;
}

#ifdef Q_WS_X11
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

#ifdef Q_OS_SYMBIAN
QSymbianSystemPluginWatcher::QSymbianSystemPluginWatcher()
{
    qt_s60GetRFs().DriveList(drives);
    watchForUpdates();
}

QSymbianSystemPluginWatcher::~QSymbianSystemPluginWatcher()
{
    qDeleteAll(watchers);
}

void QSymbianSystemPluginWatcher::watchForUpdates()
{
    QString installPathPlugins =  QLibraryInfo::location(QLibraryInfo::PluginsPath);
    if (installPathPlugins.at(1) == QChar(QLatin1Char(':')))
        return;

    installPathPlugins.prepend(QLatin1String("?:"));
    installPathPlugins = QDir::toNativeSeparators(installPathPlugins);
    RFs& fs = qt_s60GetRFs();
    for (int i=0; i<KMaxDrives; i++) {
        int attr = drives[i];
        if ((attr & KDriveAttLocal) && !(attr & KDriveAttRom)) {
            // start new watcher
            TChar driveLetter;
            fs.DriveToChar(i, driveLetter);
            installPathPlugins[0] = driveLetter;
            TPtrC ptr(qt_QString2TPtrC(installPathPlugins));
            QNotifyChangeEvent *event = q_check_ptr(new QNotifyChangeEvent(fs, ptr, this, true));
            watchers.push_back(event);
        }
    }
}

void QSymbianSystemPluginWatcher::handlePathChanged(QNotifyChangeEvent *e)
{
    QCoreApplicationPrivate::rebuildInstallLibraryPaths();
    QMutexLocker locker(qt_factoryloader_mutex());
    QString dirName(QDir::cleanPath(qt_TDesC2QString(e->watchedPath)));
    QList<QFactoryLoader *> *loaders = qt_factory_loaders();
    for (QList<QFactoryLoader *>::const_iterator it = loaders->constBegin();
         it != loaders->constEnd(); ++it) {
        QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
        (*it)->updateDir(dirName, settings);
    }
}

#endif

QT_END_NAMESPACE

#endif // QT_NO_LIBRARY
