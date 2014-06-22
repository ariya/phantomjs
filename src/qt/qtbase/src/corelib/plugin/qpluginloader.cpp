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

#include "qplatformdefs.h"

#include "qplugin.h"
#include "qcoreapplication.h"
#include "qpluginloader.h"
#include <qfileinfo.h>
#include "qlibrary_p.h"
#include "qdebug.h"
#include "qdir.h"

#ifndef QT_NO_LIBRARY

QT_BEGIN_NAMESPACE

/*!
    \class QPluginLoader
    \inmodule QtCore
    \reentrant
    \brief The QPluginLoader class loads a plugin at run-time.


    \ingroup plugins

    QPluginLoader provides access to a \l{How to Create Qt
    Plugins}{Qt plugin}. A Qt plugin is stored in a shared library (a
    DLL) and offers these benefits over shared libraries accessed
    using QLibrary:

    \list
    \li QPluginLoader checks that a plugin is linked against the same
       version of Qt as the application.
    \li QPluginLoader provides direct access to a root component object
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

    See \l{How to Create Qt Plugins} for more information about
    how to make your application extensible through plugins.

    Note that the QPluginLoader cannot be used if your application is
    statically linked against Qt. In this case, you will also have to
    link to plugins statically. You can use QLibrary if you need to
    load dynamic libraries in a statically linked application.

    \sa QLibrary, {Plug & Paint Example}
*/

/*!
    \class QStaticPlugin
    \inmodule QtCore
    \since 5.2

    \brief QStaticPlugin is a struct containing a reference to a
    static plugin instance together with its meta data.

    \sa QPluginLoader, {How to Create Qt Plugins}
*/

/*!
    \fn QObject *QStaticPlugin::instance()

    Returns the plugin instance.

    \sa QPluginLoader::staticInstances()
*/

/*!
    \fn const char *QStaticPlugin::rawMetaData()

    Returns the raw meta data for the plugin.

    \sa metaData(), Q_PLUGIN_METADATA()
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
    if (!isLoaded() && !load())
        return 0;
    if (!d->inst && d->instance)
        d->inst = d->instance();
    return d->inst.data();
}

/*!
    Returns the meta data for this plugin. The meta data is data specified
    in a json format using the Q_PLUGIN_METADATA() macro when compiling
    the plugin.

    The meta data can be queried in a fast and inexpensive way without
    actually loading the plugin. This makes it possible to e.g. store
    capabilities of the plugin in there, and make the decision whether to
    load the plugin dependent on this meta data.
 */
QJsonObject QPluginLoader::metaData() const
{
    if (!d)
        return QJsonObject();
    return d->metaData;
}

/*!
    Loads the plugin and returns \c true if the plugin was loaded
    successfully; otherwise returns \c false. Since instance() always
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
    Unloads the plugin and returns \c true if the plugin could be
    unloaded; otherwise returns \c false.

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
    Returns \c true if the plugin is loaded; otherwise returns \c false.

    \sa load()
 */
bool QPluginLoader::isLoaded() const
{
    return d && d->pHnd && d->instance;
}

#if defined(QT_SHARED)
static QString locatePlugin(const QString& fileName)
{
    QStringList prefixes = QLibraryPrivate::prefixes_sys();
    prefixes.prepend(QString());
    QStringList suffixes = QLibraryPrivate::suffixes_sys(QString());
    suffixes.prepend(QString());

    // Split up "subdir/filename"
    const int slash = fileName.lastIndexOf('/');
    const QString baseName = fileName.mid(slash + 1);
    const QString basePath = fileName.left(slash + 1); // keep the '/'

    const bool debug = qt_debug_component();

    QStringList paths = QCoreApplication::libraryPaths();
    paths.prepend(QStringLiteral("./")); // search in current dir first
    foreach (const QString &path, paths) {
        foreach (const QString &prefix, prefixes) {
            foreach (const QString &suffix, suffixes) {
                const QString fn = path + QLatin1Char('/') + basePath + prefix + baseName + suffix;
                if (debug)
                    qDebug() << "Trying..." << fn;
                if (QFileInfo(fn).isFile())
                    return fn;
            }
        }
    }
    if (debug)
        qDebug() << fileName << "not found";
    return QString();
}
#endif

/*!
    \property QPluginLoader::fileName
    \brief the file name of the plugin

    We recommend omitting the file's suffix in the file name, since
    QPluginLoader will automatically look for the file with the appropriate
    suffix (see QLibrary::isLibrary()).

    When loading the plugin, QPluginLoader searches in the current directory and
    in all plugin locations specified by QCoreApplication::libraryPaths(),
    unless the file name has an absolute path. After loading the plugin
    successfully, fileName() returns the fully-qualified file name of
    the plugin, including the full path to the plugin if one was given
    in the constructor or passed to setFileName().

    If the file name does not exist, it will not be set. This property
    will then contain an empty string.

    By default, this property contains an empty string.

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

    QFileInfo fi(fileName);
    QString fn;
    if (fi.isAbsolute())
        fn = fi.canonicalFilePath();
    else
        fn = locatePlugin(fileName);

    d = QLibraryPrivate::findOrCreate(fn);
    d->loadHints = lh;
    if (fn.isEmpty())
        d->errorString = QLibrary::tr("The shared library was not found.");
    else
        d->updatePluginState();

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

typedef QVector<QStaticPlugin> StaticPluginList;
Q_GLOBAL_STATIC(StaticPluginList, staticPluginList)

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
    \since 5.0

    Registers the \a plugin specified with the plugin loader, and is used
    by Q_IMPORT_PLUGIN().
*/
void Q_CORE_EXPORT qRegisterStaticPluginFunction(QStaticPlugin plugin)
{
    staticPluginList()->append(plugin);
}

/*!
    Returns a list of static plugin instances (root components) held
    by the plugin loader.
    \sa staticPlugins()
*/
QObjectList QPluginLoader::staticInstances()
{
    QObjectList instances;
    const StaticPluginList *plugins = staticPluginList();
    if (plugins) {
        for (int i = 0; i < plugins->size(); ++i)
            instances += plugins->at(i).instance();
    }
    return instances;
}

/*!
    Returns a list of QStaticPlugins held by the plugin
    loader. The function is similar to \l staticInstances()
    with the addition that a QStaticPlugin also contains
    meta data information.
    \sa staticInstances()
*/
QVector<QStaticPlugin> QPluginLoader::staticPlugins()
{
    StaticPluginList *plugins = staticPluginList();
    if (plugins)
        return *plugins;
    return QVector<QStaticPlugin>();
}

/*!
    Returns a the meta data for the plugin as a QJsonObject.

    \sa rawMetaData()
*/
QJsonObject QStaticPlugin::metaData() const
{
    return QLibraryPrivate::fromRawMetaData(rawMetaData()).object();
}

QT_END_NAMESPACE

#endif // QT_NO_LIBRARY
