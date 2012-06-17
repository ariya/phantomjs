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

#include "qplatformdefs.h"

#include "qplugin.h"
#include "qpluginloader.h"
#include <qfileinfo.h>
#include "qlibrary_p.h"
#include "qdebug.h"
#include "qdir.h"

#if defined(Q_OS_SYMBIAN)
# include <f32file.h>
# include "private/qcore_symbian_p.h"
#endif

#ifndef QT_NO_LIBRARY

QT_BEGIN_NAMESPACE

/*!
    \class QPluginLoader
    \reentrant
    \brief The QPluginLoader class loads a plugin at run-time.


    \ingroup plugins

    QPluginLoader provides access to a \l{How to Create Qt
    Plugins}{Qt plugin}. A Qt plugin is stored in a shared library (a
    DLL) and offers these benefits over shared libraries accessed
    using QLibrary:

    \list
    \o QPluginLoader checks that a plugin is linked against the same
       version of Qt as the application.
    \o QPluginLoader provides direct access to a root component object
       (instance()), instead of forcing you to resolve a C function manually.
    \endlist

    An instance of a QPluginLoader object operates on a single shared
    library file, which we call a plugin. It provides access to the
    functionality in the plugin in a platform-independent way. To
    specify which plugin to load, either pass a file name in
    the constructor or set it with setFileName().

    The most important functions are load() to dynamically load the
    plugin file, isLoaded() to check whether loading was successful,
    and instance() to access the root component in the plugin. The
    instance() function implicitly tries to load the plugin if it has
    not been loaded yet. Multiple instances of QPluginLoader can be
    used to access the same physical plugin.

    Once loaded, plugins remain in memory until all instances of
    QPluginLoader has been unloaded, or until the application
    terminates. You can attempt to unload a plugin using unload(),
    but if other instances of QPluginLoader are using the same
    library, the call will fail, and unloading will only happen when
    every instance has called unload(). Right before the unloading
    happen, the root component will also be deleted.

    In order to speed up loading and validation of plugins, some of
    the information that is collected during loading is cached in
    persistent memory (through QSettings). For instance, the result
    of a load operation (e.g. succeeded or failed) is stored in the
    cache, so that subsequent load operations don't try to load an
    invalid plugin. However, if the "last modified" timestamp of
    a plugin has changed, the plugin's cache entry is invalidated
    and the plugin is reloaded regardless of the values in the cache
    entry. The cache entry is then updated with the new result of the
    load operation.

    This also means that the timestamp must be updated each time the
    plugin or any dependent resources (such as a shared library) is
    updated, since the dependent resources might influence the result
    of loading a plugin.

    See \l{How to Create Qt Plugins} for more information about
    how to make your application extensible through plugins.

    Note that the QPluginLoader cannot be used if your application is
    statically linked against Qt. In this case, you will also have to
    link to plugins statically. You can use QLibrary if you need to
    load dynamic libraries in a statically linked application.

    \note In Symbian the plugin stub files must be used whenever a
    path to plugin is needed. For the purposes of loading plugins,
    the stubs can be considered to have the same name as the actual
    plugin binary. In practice they have ".qtplugin" extension
    instead of ".dll", but this difference is handled transparently
    by QPluginLoader and QLibrary to avoid need for Symbian specific
    plugin handling in most Qt applications. Plugin stubs are needed
    because Symbian Platform Security denies all access to the directory
    where the actual plugin binaries are located.

    \sa QLibrary, {Plug & Paint Example}
*/

/*!
    Constructs a plugin loader with the given \a parent.
*/
QPluginLoader::QPluginLoader(QObject *parent)
    : QObject(parent), d(0), did_load(false)
{
}

/*!
    Constructs a plugin loader with the given \a parent that will
    load the plugin specified by \a fileName.

    To be loadable, the file's suffix must be a valid suffix for a
    loadable library in accordance with the platform, e.g. \c .so on
    Unix, - \c .dylib on Mac OS X, and \c .dll on Windows. The suffix
    can be verified with QLibrary::isLibrary().

    Note: In Symbian the \a fileName must point to plugin stub file.

    \sa setFileName()
*/
QPluginLoader::QPluginLoader(const QString &fileName, QObject *parent)
    : QObject(parent), d(0), did_load(false)
{
    setFileName(fileName);
}

/*!
    Destroys the QPluginLoader object.

    Unless unload() was called explicitly, the plugin stays in memory
    until the application terminates.

    \sa isLoaded(), unload()
*/
QPluginLoader::~QPluginLoader()
{
    if (d)
        d->release();
}

/*!
    Returns the root component object of the plugin. The plugin is
    loaded if necessary. The function returns 0 if the plugin could
    not be loaded or if the root component object could not be
    instantiated.

    If the root component object was destroyed, calling this function
    creates a new instance.

    The root component, returned by this function, is not deleted when
    the QPluginLoader is destroyed. If you want to ensure that the root
    component is deleted, you should call unload() as soon you don't
    need to access the core component anymore.  When the library is
    finally unloaded, the root component will automatically be deleted.

    The component object is a QObject. Use qobject_cast() to access
    interfaces you are interested in.

    \sa load()
*/
QObject *QPluginLoader::instance()
{
    if (!load())
        return 0;
    if (!d->inst && d->instance)
        d->inst = d->instance();
    return d->inst.data();
}

/*!
    Loads the plugin and returns true if the plugin was loaded
    successfully; otherwise returns false. Since instance() always
    calls this function before resolving any symbols it is not
    necessary to call it explicitly. In some situations you might want
    the plugin loaded in advance, in which case you would use this
    function.

    \sa unload()
*/
bool QPluginLoader::load()
{
    if (!d || d->fileName.isEmpty())
        return false;
    if (did_load)
        return d->pHnd && d->instance;
    if (!d->isPlugin())
        return false;
    did_load = true;
    return d->loadPlugin();
}


/*!
    Unloads the plugin and returns true if the plugin could be
    unloaded; otherwise returns false.

    This happens automatically on application termination, so you
    shouldn't normally need to call this function.

    If other instances of QPluginLoader are using the same plugin, the
    call will fail, and unloading will only happen when every instance
    has called unload().

    Don't try to delete the root component. Instead rely on
    that unload() will automatically delete it when needed.

    \sa instance(), load()
*/
bool QPluginLoader::unload()
{
    if (did_load) {
        did_load = false;
        return d->unload();
    }
    if (d)  // Ouch
        d->errorString = tr("The plugin was not loaded.");
    return false;
}

/*!
    Returns true if the plugin is loaded; otherwise returns false.

    \sa load()
 */
bool QPluginLoader::isLoaded() const
{
    return d && d->pHnd && d->instance;
}

/*!
    \property QPluginLoader::fileName
    \brief the file name of the plugin

    To be loadable, the file's suffix must be a valid suffix for a
    loadable library in accordance with the platform, e.g. \c .so on
    Unix, \c .dylib on Mac OS X, and \c .dll on Windows. The suffix
    can be verified with QLibrary::isLibrary().

    If the file name does not exist, it will not be set. This property
    will then contain an empty string.

    By default, this property contains an empty string.

    Note: In Symbian the \a fileName must point to plugin stub file.

    \sa load()
*/
void QPluginLoader::setFileName(const QString &fileName)
{
#if defined(QT_SHARED)
    QLibrary::LoadHints lh;
    if (d) {
        lh = d->loadHints;
        d->release();
        d = 0;
        did_load = false;
    }

#if defined(Q_OS_SYMBIAN)
    // In Symbian we actually look for plugin stub, so modify the filename
    // to make canonicalFilePath find the file, if .dll is specified.
    QFileInfo fi(fileName);

    if (fi.suffix() == QLatin1String("dll")) {
        QString stubName = fileName;
        stubName.chop(3);
        stubName += QLatin1String("qtplugin");
        fi = QFileInfo(stubName);
    }

    QString fn = fi.canonicalFilePath();
    // If not found directly, check also all the available drives
    if (!fn.length()) {
        QString stubPath(fi.fileName().length() ? fi.absoluteFilePath() : QString());
        if (stubPath.length() > 1) {
            if (stubPath.at(1).toAscii() == ':')
                stubPath.remove(0,2);
            QFileInfoList driveList(QDir::drives());
            RFs rfs = qt_s60GetRFs();
            foreach(const QFileInfo& drive, driveList) {
                QString testFilePath(drive.absolutePath() + stubPath);
                testFilePath = QDir::cleanPath(testFilePath);
                // Use native Symbian code to check for file existence, because checking
                // for file from under non-existent protected dir like E:/private/<uid> using
                // QFile::exists causes platform security violations on most apps.
                QString nativePath = QDir::toNativeSeparators(testFilePath);
                TPtrC ptr(qt_QString2TPtrC(nativePath));
                TUint attributes;
                TInt err = rfs.Att(ptr, attributes);
                if (err == KErrNone) {
                    fn = testFilePath;
                    break;
                }
            }
        }
    }

#else
    QString fn = QFileInfo(fileName).canonicalFilePath();
#endif

    d = QLibraryPrivate::findOrCreate(fn);
    d->loadHints = lh;
    if (fn.isEmpty())
        d->errorString = QLibrary::tr("The shared library was not found.");
#else
    if (qt_debug_component()) {
        qWarning("Cannot load %s into a statically linked Qt library.",
            (const char*)QFile::encodeName(fileName));
    }
    Q_UNUSED(fileName);
#endif
}

QString QPluginLoader::fileName() const
{
    if (d)
        return d->fileName;
    return QString();
}

/*!
    \since 4.2

    Returns a text string with the description of the last error that occurred.
*/
QString QPluginLoader::errorString() const
{
    return (!d || d->errorString.isEmpty()) ? tr("Unknown error") : d->errorString;
}

typedef QList<QtPluginInstanceFunction> StaticInstanceFunctionList;
Q_GLOBAL_STATIC(StaticInstanceFunctionList, staticInstanceFunctionList)

/*! \since 4.4

    \property QPluginLoader::loadHints
    \brief Give the load() function some hints on how it should behave.

    You can give hints on how the symbols in the plugin are
    resolved. By default, none of the hints are set.

    See the documentation of QLibrary::loadHints for a complete
    description of how this property works.

    \sa QLibrary::loadHints
*/

void QPluginLoader::setLoadHints(QLibrary::LoadHints loadHints)
{
    if (!d) {
        d = QLibraryPrivate::findOrCreate(QString());   // ugly, but we need a d-ptr
        d->errorString.clear();
    }
    d->loadHints = loadHints;
}

QLibrary::LoadHints QPluginLoader::loadHints() const
{
    if (!d) {
        QPluginLoader *that = const_cast<QPluginLoader *>(this);
        that->d = QLibraryPrivate::findOrCreate(QString());   // ugly, but we need a d-ptr
        that->d->errorString.clear();
    }
    return d->loadHints;
}

/*!
    \relates QPluginLoader
    \since 4.4

    Registers the given \a function with the plugin loader.
*/
void Q_CORE_EXPORT qRegisterStaticPluginInstanceFunction(QtPluginInstanceFunction function)
{
    staticInstanceFunctionList()->append(function);
}

/*!
    Returns a list of static plugin instances (root components) held
    by the plugin loader.
*/
QObjectList QPluginLoader::staticInstances()
{
    QObjectList instances;
    StaticInstanceFunctionList *functions = staticInstanceFunctionList();
    if (functions) {
        for (int i = 0; i < functions->count(); ++i)
            instances.append((*functions)[i]());
    }
    return instances;
}

QT_END_NAMESPACE

#endif // QT_NO_LIBRARY
