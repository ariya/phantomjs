/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013 Intel Corporation
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
#include "qlibrary.h"

#ifndef QT_NO_LIBRARY

#include "qlibrary_p.h"
#include <qstringlist.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qmutex.h>
#include <qmap.h>
#include <private/qcoreapplication_p.h>
#ifdef Q_OS_MAC
#  include <private/qcore_mac_p.h>
#endif
#ifndef NO_ERRNO_H
#include <errno.h>
#endif // NO_ERROR_H
#include <qdebug.h>
#include <qvector.h>
#include <qdir.h>
#include <qendian.h>
#include <qjsondocument.h>
#include <qjsonvalue.h>
#include "qelfparser_p.h"
#include "qmachparser_p.h"

QT_BEGIN_NAMESPACE

#ifdef QT_NO_DEBUG
#  define QLIBRARY_AS_DEBUG false
#else
#  define QLIBRARY_AS_DEBUG true
#endif

#if defined(Q_OS_UNIX)
// We don't use separate debug and release libs on UNIX, so we want
// to allow loading plugins, regardless of how they were built.
#  define QT_NO_DEBUG_PLUGIN_CHECK
#endif

/*!
    \class QLibrary
    \inmodule QtCore
    \reentrant
    \brief The QLibrary class loads shared libraries at runtime.


    \ingroup plugins

    An instance of a QLibrary object operates on a single shared
    object file (which we call a "library", but is also known as a
    "DLL"). A QLibrary provides access to the functionality in the
    library in a platform independent way. You can either pass a file
    name in the constructor, or set it explicitly with setFileName().
    When loading the library, QLibrary searches in all the
    system-specific library locations (e.g. \c LD_LIBRARY_PATH on
    Unix), unless the file name has an absolute path.

    If the file name is an absolute path then an attempt is made to
    load this path first. If the file cannot be found, QLibrary tries
    the name with different platform-specific file prefixes, like
    "lib" on Unix and Mac, and suffixes, like ".so" on Unix, ".dylib"
    on the Mac, or ".dll" on Windows.

    If the file path is not absolute then QLibrary modifies the search
    order to try the system-specific prefixes and suffixes first,
    followed by the file path specified.

    This makes it possible to specify shared libraries that are only
    identified by their basename (i.e. without their suffix), so the
    same code will work on different operating systems yet still
    minimise the number of attempts to find the library.

    The most important functions are load() to dynamically load the
    library file, isLoaded() to check whether loading was successful,
    and resolve() to resolve a symbol in the library. The resolve()
    function implicitly tries to load the library if it has not been
    loaded yet. Multiple instances of QLibrary can be used to access
    the same physical library. Once loaded, libraries remain in memory
    until the application terminates. You can attempt to unload a
    library using unload(), but if other instances of QLibrary are
    using the same library, the call will fail, and unloading will
    only happen when every instance has called unload().

    A typical use of QLibrary is to resolve an exported symbol in a
    library, and to call the C function that this symbol represents.
    This is called "explicit linking" in contrast to "implicit
    linking", which is done by the link step in the build process when
    linking an executable against a library.

    The following code snippet loads a library, resolves the symbol
    "mysymbol", and calls the function if everything succeeded. If
    something goes wrong, e.g. the library file does not exist or the
    symbol is not defined, the function pointer will be 0 and won't be
    called.

    \snippet code/src_corelib_plugin_qlibrary.cpp 0

    The symbol must be exported as a C function from the library for
    resolve() to work. This means that the function must be wrapped in
    an \c{extern "C"} block if the library is compiled with a C++
    compiler. On Windows, this also requires the use of a \c dllexport
    macro; see resolve() for the details of how this is done. For
    convenience, there is a static resolve() function which you can
    use if you just want to call a function in a library without
    explicitly loading the library first:

    \snippet code/src_corelib_plugin_qlibrary.cpp 1

    \sa QPluginLoader
*/

/*!
    \enum QLibrary::LoadHint

    This enum describes the possible hints that can be used to change the way
    libraries are handled when they are loaded. These values indicate how
    symbols are resolved when libraries are loaded, and are specified using
    the setLoadHints() function.

    \value ResolveAllSymbolsHint
    Causes all symbols in a library to be resolved when it is loaded, not
    simply when resolve() is called.
    \value ExportExternalSymbolsHint
    Exports unresolved and external symbols in the library so that they can be
    resolved in other dynamically-loaded libraries loaded later.
    \value LoadArchiveMemberHint
    Allows the file name of the library to specify a particular object file
    within an archive file.
    If this hint is given, the filename of the library consists of
    a path, which is a reference to an archive file, followed by
    a reference to the archive member.
    \value PreventUnloadHint
    Prevents the library from being unloaded from the address space if close()
    is called. The library's static variables are not reinitialized if open()
    is called at a later time.

    \sa loadHints
*/


static long qt_find_pattern(const char *s, ulong s_len,
                             const char *pattern, ulong p_len)
{
    /*
      we search from the end of the file because on the supported
      systems, the read-only data/text segments are placed at the end
      of the file.  HOWEVER, when building with debugging enabled, all
      the debug symbols are placed AFTER the data/text segments.

      what does this mean?  when building in release mode, the search
      is fast because the data we are looking for is at the end of the
      file... when building in debug mode, the search is slower
      because we have to skip over all the debugging symbols first
    */

    if (! s || ! pattern || p_len > s_len) return -1;
    ulong i, hs = 0, hp = 0, delta = s_len - p_len;

    for (i = 0; i < p_len; ++i) {
        hs += s[delta + i];
        hp += pattern[i];
    }
    i = delta;
    for (;;) {
        if (hs == hp && qstrncmp(s + i, pattern, p_len) == 0)
            return i;
        if (i == 0)
            break;
        --i;
        hs -= s[i + p_len];
        hs += s[i];
    }

    return -1;
}

/*
  This opens the specified library, mmaps it into memory, and searches
  for the QT_PLUGIN_VERIFICATION_DATA.  The advantage of this approach is that
  we can get the verification data without have to actually load the library.
  This lets us detect mismatches more safely.

  Returns \c false if version information is not present, or if the
                information could not be read.
  Returns  true if version information is present and successfully read.
*/
static bool findPatternUnloaded(const QString &library, QLibraryPrivate *lib)
{
    QFile file(library);
    if (!file.open(QIODevice::ReadOnly)) {
        if (lib)
            lib->errorString = file.errorString();
        if (qt_debug_component()) {
            qWarning("%s: %s", (const char*) QFile::encodeName(library),
                qPrintable(qt_error_string(errno)));
        }
        return false;
    }

    QByteArray data;
    const char *filedata = 0;
    ulong fdlen = file.size();
    filedata = (char *) file.map(0, fdlen);
    if (filedata == 0) {
        // try reading the data into memory instead
        data = file.readAll();
        filedata = data.constData();
        fdlen = data.size();
    }

    /*
       ELF and Mach-O binaries with GCC have .qplugin sections.
    */
    bool hasMetaData = false;
    long pos = 0;
    char pattern[] = "qTMETADATA  ";
    pattern[0] = 'Q'; // Ensure the pattern "QTMETADATA" is not found in this library should QPluginLoader ever encounter it.
    const ulong plen = qstrlen(pattern);
#if defined (Q_OF_ELF) && defined(Q_CC_GNU)
    int r = QElfParser().parse(filedata, fdlen, library, lib, &pos, &fdlen);
    if (r == QElfParser::Corrupt || r == QElfParser::NotElf) {
            if (lib && qt_debug_component()) {
                qWarning("QElfParser: %s",qPrintable(lib->errorString));
            }
            return false;
    } else if (r == QElfParser::QtMetaDataSection) {
        long rel = qt_find_pattern(filedata + pos, fdlen, pattern, plen);
        if (rel < 0)
            pos = -1;
        else
            pos += rel;
        hasMetaData = true;
    }
#elif defined (Q_OF_MACH_O)
    {
        QString errorString;
        int r = QMachOParser::parse(filedata, fdlen, library, &errorString, &pos, &fdlen);
        if (r == QMachOParser::NotSuitable) {
            if (qt_debug_component())
                qWarning("QMachOParser: %s", qPrintable(errorString));
            if (lib)
                lib->errorString = errorString;
            return false;
        }
        // even if the metadata section was not found, the Mach-O parser will
        // at least return the boundaries of the right architecture
        long rel = qt_find_pattern(filedata + pos, fdlen, pattern, plen);
        if (rel < 0)
            pos = -1;
        else
            pos += rel;
        hasMetaData = true;
    }
#else
    pos = qt_find_pattern(filedata, fdlen, pattern, plen);
    if (pos > 0)
        hasMetaData = true;
#endif // defined(Q_OF_ELF) && defined(Q_CC_GNU)

    bool ret = false;

    if (pos >= 0) {
        if (hasMetaData) {
            const char *data = filedata + pos;
            QJsonDocument doc = QLibraryPrivate::fromRawMetaData(data);
            lib->metaData = doc.object();
            if (qt_debug_component())
                qWarning("Found metadata in lib %s, metadata=\n%s\n",
                         library.toLocal8Bit().constData(), doc.toJson().constData());
            ret = !doc.isNull();
        }
    }

    if (!ret && lib)
        lib->errorString = QLibrary::tr("Plugin verification data mismatch in '%1'").arg(library);
    file.close();
    return ret;
}

static void installCoverageTool(QLibraryPrivate *libPrivate)
{
#ifdef __COVERAGESCANNER__
    /*
      __COVERAGESCANNER__ is defined when Qt has been instrumented for code
      coverage by TestCocoon. CoverageScanner is the name of the tool that
      generates the code instrumentation.
      This code is required here when code coverage analysis with TestCocoon
      is enabled in order to allow the loading application to register the plugin
      and then store its execution report. The execution report gathers information
      about each part of the plugin's code that has been used when
      the plugin was loaded by the launching application.
      The execution report for the plugin will go to the same execution report
      as the one defined for the application loading it.
    */

    int ret = __coveragescanner_register_library(libPrivate->fileName.toLocal8Bit());

    if (qt_debug_component()) {
        if (ret >= 0) {
            qDebug("%s: coverage data for %s registered",
                     Q_FUNC_INFO,
                     qPrintable(libPrivate->fileName));
        } else {
            qWarning("%s: could not register %s: error %d; coverage data may be incomplete",
                     Q_FUNC_INFO,
                     qPrintable(libPrivate->fileName),
                     ret);
        }
    }
#else
    Q_UNUSED(libPrivate);
#endif
}

class QLibraryStore
{
public:
    inline ~QLibraryStore();
    static inline QLibraryPrivate *findOrCreate(const QString &fileName, const QString &version);
    static inline void releaseLibrary(QLibraryPrivate *lib);

    static inline void cleanup();

private:
    static inline QLibraryStore *instance();

    // all members and instance() are protected by qt_library_mutex
    typedef QMap<QString, QLibraryPrivate*> LibraryMap;
    LibraryMap libraryMap;
};

static QBasicMutex qt_library_mutex;
static QLibraryStore *qt_library_data = 0;
static bool qt_library_data_once;

QLibraryStore::~QLibraryStore()
{
    qt_library_data = 0;
}

inline void QLibraryStore::cleanup()
{
    QLibraryStore *data = qt_library_data;
    if (!data)
        return;

    // find any libraries that are still loaded but have a no one attached to them
    LibraryMap::Iterator it = data->libraryMap.begin();
    for (; it != data->libraryMap.end(); ++it) {
        QLibraryPrivate *lib = it.value();
        if (lib->libraryRefCount.load() == 1) {
            if (lib->libraryUnloadCount.load() > 0) {
                Q_ASSERT(lib->pHnd);
                lib->libraryUnloadCount.store(1);
#ifdef __GLIBC__
                // glibc has a bug in unloading from global destructors
                // see https://bugzilla.novell.com/show_bug.cgi?id=622977
                // and http://sourceware.org/bugzilla/show_bug.cgi?id=11941
                lib->unload(QLibraryPrivate::NoUnloadSys);
#else
                lib->unload();
#endif
            }
            delete lib;
            it.value() = 0;
        }
    }

    if (qt_debug_component()) {
        // dump all objects that remain
        foreach (QLibraryPrivate *lib, data->libraryMap) {
            if (lib)
                qDebug() << "On QtCore unload," << lib->fileName << "was leaked, with"
                         << lib->libraryRefCount.load() << "users";
        }
    }

    delete data;
}

static void qlibraryCleanup()
{
    QLibraryStore::cleanup();
}
Q_DESTRUCTOR_FUNCTION(qlibraryCleanup)

// must be called with a locked mutex
QLibraryStore *QLibraryStore::instance()
{
    if (Q_UNLIKELY(!qt_library_data_once && !qt_library_data)) {
        // only create once per process lifetime
        qt_library_data = new QLibraryStore;
        qt_library_data_once = true;
    }
    return qt_library_data;
}

inline QLibraryPrivate *QLibraryStore::findOrCreate(const QString &fileName, const QString &version)
{
    QMutexLocker locker(&qt_library_mutex);
    QLibraryStore *data = instance();

    // check if this library is already loaded
    QLibraryPrivate *lib = 0;
    if (Q_LIKELY(data))
        lib = data->libraryMap.value(fileName);
    if (!lib)
        lib = new QLibraryPrivate(fileName, version);

    // track this library
    if (Q_LIKELY(data))
        data->libraryMap.insert(fileName, lib);

    lib->libraryRefCount.ref();
    return lib;
}

inline void QLibraryStore::releaseLibrary(QLibraryPrivate *lib)
{
    QMutexLocker locker(&qt_library_mutex);
    QLibraryStore *data = instance();

    if (lib->libraryRefCount.deref()) {
        // still in use
        return;
    }

    // no one else is using
    Q_ASSERT(lib->libraryUnloadCount.load() == 0);

    if (Q_LIKELY(data)) {
        QLibraryPrivate *that = data->libraryMap.take(lib->fileName);
        Q_ASSERT(lib == that);
        Q_UNUSED(that);
    }
    delete lib;
}

QLibraryPrivate::QLibraryPrivate(const QString &canonicalFileName, const QString &version)
    : pHnd(0), fileName(canonicalFileName), fullVersion(version), instance(0),
      loadHints(0),
      libraryRefCount(0), libraryUnloadCount(0), pluginState(MightBeAPlugin)
{ }

QLibraryPrivate *QLibraryPrivate::findOrCreate(const QString &fileName, const QString &version)
{
    return QLibraryStore::findOrCreate(fileName, version);
}

QLibraryPrivate::~QLibraryPrivate()
{
}

QFunctionPointer QLibraryPrivate::resolve(const char *symbol)
{
    if (!pHnd)
        return 0;
    return resolve_sys(symbol);
}


bool QLibraryPrivate::load()
{
    if (pHnd) {
        libraryUnloadCount.ref();
        return true;
    }
    if (fileName.isEmpty())
        return false;

    bool ret = load_sys();
    if (qt_debug_component())
        qDebug() << "loaded library" << fileName;
    if (ret) {
        //when loading a library we add a reference to it so that the QLibraryPrivate won't get deleted
        //this allows to unload the library at a later time
        libraryUnloadCount.ref();
        libraryRefCount.ref();
        installCoverageTool(this);
    }

    return ret;
}

bool QLibraryPrivate::unload(UnloadFlag flag)
{
    if (!pHnd)
        return false;
    if (libraryUnloadCount.load() > 0 && !libraryUnloadCount.deref()) { // only unload if ALL QLibrary instance wanted to
        delete inst.data();
        if (flag == NoUnloadSys || unload_sys()) {
            if (qt_debug_component())
                qWarning() << "QLibraryPrivate::unload succeeded on" << fileName
                           << (flag == NoUnloadSys ? "(faked)" : "");
            //when the library is unloaded, we release the reference on it so that 'this'
            //can get deleted
            libraryRefCount.deref();
            pHnd = 0;
            instance = 0;
        }
    }

    return (pHnd == 0);
}

void QLibraryPrivate::release()
{
    QLibraryStore::releaseLibrary(this);
}

bool QLibraryPrivate::loadPlugin()
{
    if (instance) {
        libraryUnloadCount.ref();
        return true;
    }
    if (pluginState == IsNotAPlugin)
        return false;
    if (load()) {
        instance = (QtPluginInstanceFunction)resolve("qt_plugin_instance");
        return instance;
    }
    if (qt_debug_component())
        qWarning() << "QLibraryPrivate::loadPlugin failed on" << fileName << ":" << errorString;
    pluginState = IsNotAPlugin;
    return false;
}

/*!
    Returns \c true if \a fileName has a valid suffix for a loadable
    library; otherwise returns \c false.

    \table
    \header \li Platform \li Valid suffixes
    \row \li Windows     \li \c .dll, \c .DLL
    \row \li Unix/Linux  \li \c .so
    \row \li AIX  \li \c .a
    \row \li HP-UX       \li \c .sl, \c .so (HP-UXi)
    \row \li Mac OS X    \li \c .dylib, \c .bundle, \c .so
    \endtable

    Trailing versioning numbers on Unix are ignored.
 */
bool QLibrary::isLibrary(const QString &fileName)
{
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
    return fileName.endsWith(QLatin1String(".dll"), Qt::CaseInsensitive);
#else
    QString completeSuffix = QFileInfo(fileName).completeSuffix();
    if (completeSuffix.isEmpty())
        return false;
    QStringList suffixes = completeSuffix.split(QLatin1Char('.'));
# if defined(Q_OS_DARWIN)

    // On Mac, libs look like libmylib.1.0.0.dylib
    const QString lastSuffix = suffixes.at(suffixes.count() - 1);
    const QString firstSuffix = suffixes.at(0);

    bool valid = (lastSuffix == QLatin1String("dylib")
            || firstSuffix == QLatin1String("so")
            || firstSuffix == QLatin1String("bundle"));

    return valid;
# else  // Generic Unix
    QStringList validSuffixList;

#  if defined(Q_OS_HPUX)
/*
    See "HP-UX Linker and Libraries User's Guide", section "Link-time Differences between PA-RISC and IPF":
    "In PA-RISC (PA-32 and PA-64) shared libraries are suffixed with .sl. In IPF (32-bit and 64-bit),
    the shared libraries are suffixed with .so. For compatibility, the IPF linker also supports the .sl suffix."
 */
    validSuffixList << QLatin1String("sl");
#   if defined __ia64
    validSuffixList << QLatin1String("so");
#   endif
#  elif defined(Q_OS_AIX)
    validSuffixList << QLatin1String("a") << QLatin1String("so");
#  elif defined(Q_OS_UNIX)
    validSuffixList << QLatin1String("so");
#  endif

    // Examples of valid library names:
    //  libfoo.so
    //  libfoo.so.0
    //  libfoo.so.0.3
    //  libfoo-0.3.so
    //  libfoo-0.3.so.0.3.0

    int suffix;
    int suffixPos = -1;
    for (suffix = 0; suffix < validSuffixList.count() && suffixPos == -1; ++suffix)
        suffixPos = suffixes.indexOf(validSuffixList.at(suffix));

    bool valid = suffixPos != -1;
    for (int i = suffixPos + 1; i < suffixes.count() && valid; ++i)
        if (i != suffixPos)
            suffixes.at(i).toInt(&valid);
    return valid;
# endif
#endif

}

typedef const char * (*QtPluginQueryVerificationDataFunction)();

static bool qt_get_metadata(QtPluginQueryVerificationDataFunction pfn, QLibraryPrivate *priv)
{
    const char *szData = 0;
    if (!pfn)
        return false;

    szData = pfn();
    if (!szData)
        return false;

    QJsonDocument doc = QLibraryPrivate::fromRawMetaData(szData);
    if (doc.isNull())
        return false;
    priv->metaData = doc.object();
    return true;
}

bool QLibraryPrivate::isPlugin()
{
    if (pluginState == MightBeAPlugin)
        updatePluginState();

    return pluginState == IsAPlugin;
}

void QLibraryPrivate::updatePluginState()
{
    errorString.clear();
    if (pluginState != MightBeAPlugin)
        return;

    bool success = false;

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    if (fileName.endsWith(QLatin1String(".debug"))) {
        // refuse to load a file that ends in .debug
        // these are the debug symbols from the libraries
        // the problem is that they are valid shared library files
        // and dlopen is known to crash while opening them

        // pretend we didn't see the file
        errorString = QLibrary::tr("The shared library was not found.");
        pluginState = IsNotAPlugin;
        return;
    }
#endif

    if (!pHnd) {
        // scan for the plugin metadata without loading
        success = findPatternUnloaded(fileName, this);
    } else {
        // library is already loaded (probably via QLibrary)
        // simply get the target function and call it.
        QtPluginQueryVerificationDataFunction getMetaData = NULL;
        getMetaData = (QtPluginQueryVerificationDataFunction) resolve("qt_plugin_query_metadata");
        success = qt_get_metadata(getMetaData, this);
    }

    if (!success) {
        if (errorString.isEmpty()){
            if (fileName.isEmpty())
                errorString = QLibrary::tr("The shared library was not found.");
            else
                errorString = QLibrary::tr("The file '%1' is not a valid Qt plugin.").arg(fileName);
        }
        pluginState = IsNotAPlugin;
        return;
    }

    pluginState = IsNotAPlugin; // be pessimistic

    uint qt_version = (uint)metaData.value(QLatin1String("version")).toDouble();
    bool debug = metaData.value(QLatin1String("debug")).toBool();
    if ((qt_version & 0x00ff00) > (QT_VERSION & 0x00ff00) || (qt_version & 0xff0000) != (QT_VERSION & 0xff0000)) {
        if (qt_debug_component()) {
            qWarning("In %s:\n"
                 "  Plugin uses incompatible Qt library (%d.%d.%d) [%s]",
                 (const char*) QFile::encodeName(fileName),
                 (qt_version&0xff0000) >> 16, (qt_version&0xff00) >> 8, qt_version&0xff,
                 debug ? "debug" : "release");
        }
        errorString = QLibrary::tr("The plugin '%1' uses incompatible Qt library. (%2.%3.%4) [%5]")
            .arg(fileName)
            .arg((qt_version&0xff0000) >> 16)
            .arg((qt_version&0xff00) >> 8)
            .arg(qt_version&0xff)
            .arg(debug ? QLatin1String("debug") : QLatin1String("release"));
#ifndef QT_NO_DEBUG_PLUGIN_CHECK
    } else if(debug != QLIBRARY_AS_DEBUG) {
        //don't issue a qWarning since we will hopefully find a non-debug? --Sam
        errorString = QLibrary::tr("The plugin '%1' uses incompatible Qt library."
                 " (Cannot mix debug and release libraries.)").arg(fileName);
#endif
    } else {
        pluginState = IsAPlugin;
    }
}

/*!
    Loads the library and returns \c true if the library was loaded
    successfully; otherwise returns \c false. Since resolve() always
    calls this function before resolving any symbols it is not
    necessary to call it explicitly. In some situations you might want
    the library loaded in advance, in which case you would use this
    function.

    \sa unload()
*/
bool QLibrary::load()
{
    if (!d)
        return false;
    if (did_load)
        return d->pHnd;
    did_load = true;
    return d->load();
}

/*!
    Unloads the library and returns \c true if the library could be
    unloaded; otherwise returns \c false.

    This happens automatically on application termination, so you
    shouldn't normally need to call this function.

    If other instances of QLibrary are using the same library, the
    call will fail, and unloading will only happen when every instance
    has called unload().

    Note that on Mac OS X 10.3 (Panther), dynamic libraries cannot be unloaded.

    \sa resolve(), load()
*/
bool QLibrary::unload()
{
    if (did_load) {
        did_load = false;
        return d->unload();
    }
    return false;
}

/*!
    Returns \c true if the library is loaded; otherwise returns \c false.

    \sa load()
 */
bool QLibrary::isLoaded() const
{
    return d && d->pHnd;
}


/*!
    Constructs a library with the given \a parent.
 */
QLibrary::QLibrary(QObject *parent)
    :QObject(parent), d(0), did_load(false)
{
}


/*!
    Constructs a library object with the given \a parent that will
    load the library specified by \a fileName.

    We recommend omitting the file's suffix in \a fileName, since
    QLibrary will automatically look for the file with the appropriate
    suffix in accordance with the platform, e.g. ".so" on Unix,
    ".dylib" on Mac OS X, and ".dll" on Windows. (See \l{fileName}.)
 */
QLibrary::QLibrary(const QString& fileName, QObject *parent)
    :QObject(parent), d(0), did_load(false)
{
    setFileName(fileName);
}


/*!
    Constructs a library object with the given \a parent that will
    load the library specified by \a fileName and major version number \a verNum.
    Currently, the version number is ignored on Windows.

    We recommend omitting the file's suffix in \a fileName, since
    QLibrary will automatically look for the file with the appropriate
    suffix in accordance with the platform, e.g. ".so" on Unix,
    ".dylib" on Mac OS X, and ".dll" on Windows. (See \l{fileName}.)
*/
QLibrary::QLibrary(const QString& fileName, int verNum, QObject *parent)
    :QObject(parent), d(0), did_load(false)
{
    setFileNameAndVersion(fileName, verNum);
}

/*!
    Constructs a library object with the given \a parent that will
    load the library specified by \a fileName and full version number \a version.
    Currently, the version number is ignored on Windows.

    We recommend omitting the file's suffix in \a fileName, since
    QLibrary will automatically look for the file with the appropriate
    suffix in accordance with the platform, e.g. ".so" on Unix,
    ".dylib" on Mac OS X, and ".dll" on Windows. (See \l{fileName}.)
 */
QLibrary::QLibrary(const QString& fileName, const QString &version, QObject *parent)
    :QObject(parent), d(0), did_load(false)
{
    setFileNameAndVersion(fileName, version);
}

/*!
    Destroys the QLibrary object.

    Unless unload() was called explicitly, the library stays in memory
    until the application terminates.

    \sa isLoaded(), unload()
*/
QLibrary::~QLibrary()
{
    if (d)
        d->release();
}


/*!
    \property QLibrary::fileName
    \brief the file name of the library

    We recommend omitting the file's suffix in the file name, since
    QLibrary will automatically look for the file with the appropriate
    suffix (see isLibrary()).

    When loading the library, QLibrary searches in all system-specific
    library locations (e.g. \c LD_LIBRARY_PATH on Unix), unless the
    file name has an absolute path. After loading the library
    successfully, fileName() returns the fully-qualified file name of
    the library, including the full path to the library if one was given
    in the constructor or passed to setFileName().

    For example, after successfully loading the "GL" library on Unix
    platforms, fileName() will return "libGL.so". If the file name was
    originally passed as "/usr/lib/libGL", fileName() will return
    "/usr/lib/libGL.so".
*/

void QLibrary::setFileName(const QString &fileName)
{
    QLibrary::LoadHints lh;
    if (d) {
        lh = d->loadHints;
        d->release();
        d = 0;
        did_load = false;
    }
    d = QLibraryPrivate::findOrCreate(fileName);
    d->loadHints = lh;
}

QString QLibrary::fileName() const
{
    if (d)
        return d->qualifiedFileName.isEmpty() ? d->fileName : d->qualifiedFileName;
    return QString();
}

/*!
    \fn void QLibrary::setFileNameAndVersion(const QString &fileName, int versionNumber)

    Sets the fileName property and major version number to \a fileName
    and \a versionNumber respectively.
    The \a versionNumber is ignored on Windows.

    \sa setFileName()
*/
void QLibrary::setFileNameAndVersion(const QString &fileName, int verNum)
{
    QLibrary::LoadHints lh;
    if (d) {
        lh = d->loadHints;
        d->release();
        d = 0;
        did_load = false;
    }
    d = QLibraryPrivate::findOrCreate(fileName, verNum >= 0 ? QString::number(verNum) : QString());
    d->loadHints = lh;
}

/*!
    \since 4.4

    Sets the fileName property and full version number to \a fileName
    and \a version respectively.
    The \a version parameter is ignored on Windows.

    \sa setFileName()
*/
void QLibrary::setFileNameAndVersion(const QString &fileName, const QString &version)
{
    QLibrary::LoadHints lh;
    if (d) {
        lh = d->loadHints;
        d->release();
        d = 0;
        did_load = false;
    }
    d = QLibraryPrivate::findOrCreate(fileName, version);
    d->loadHints = lh;
}

/*!
    Returns the address of the exported symbol \a symbol. The library is
    loaded if necessary. The function returns 0 if the symbol could
    not be resolved or if the library could not be loaded.

    Example:
    \snippet code/src_corelib_plugin_qlibrary.cpp 2

    The symbol must be exported as a C function from the library. This
    means that the function must be wrapped in an \c{extern "C"} if
    the library is compiled with a C++ compiler. On Windows you must
    also explicitly export the function from the DLL using the
    \c{__declspec(dllexport)} compiler directive, for example:

    \snippet code/src_corelib_plugin_qlibrary.cpp 3

    with \c MY_EXPORT defined as

    \snippet code/src_corelib_plugin_qlibrary.cpp 4
*/
QFunctionPointer QLibrary::resolve(const char *symbol)
{
    if (!isLoaded() && !load())
        return 0;
    return d->resolve(symbol);
}

/*!
    \overload

    Loads the library \a fileName and returns the address of the
    exported symbol \a symbol. Note that \a fileName should not
    include the platform-specific file suffix; (see \l{fileName}). The
    library remains loaded until the application exits.

    The function returns 0 if the symbol could not be resolved or if
    the library could not be loaded.

    \sa resolve()
*/
QFunctionPointer QLibrary::resolve(const QString &fileName, const char *symbol)
{
    QLibrary library(fileName);
    return library.resolve(symbol);
}

/*!
    \overload

    Loads the library \a fileName with major version number \a verNum and
    returns the address of the exported symbol \a symbol.
    Note that \a fileName should not include the platform-specific file suffix;
    (see \l{fileName}). The library remains loaded until the application exits.
    \a verNum is ignored on Windows.

    The function returns 0 if the symbol could not be resolved or if
    the library could not be loaded.

    \sa resolve()
*/
QFunctionPointer QLibrary::resolve(const QString &fileName, int verNum, const char *symbol)
{
    QLibrary library(fileName, verNum);
    return library.resolve(symbol);
}

/*!
    \overload
    \since 4.4

    Loads the library \a fileName with full version number \a version and
    returns the address of the exported symbol \a symbol.
    Note that \a fileName should not include the platform-specific file suffix;
    (see \l{fileName}). The library remains loaded until the application exits.
    \a version is ignored on Windows.

    The function returns 0 if the symbol could not be resolved or if
    the library could not be loaded.

    \sa resolve()
*/
QFunctionPointer QLibrary::resolve(const QString &fileName, const QString &version, const char *symbol)
{
    QLibrary library(fileName, version);
    return library.resolve(symbol);
}

/*!
    \since 4.2

    Returns a text string with the description of the last error that occurred.
    Currently, errorString will only be set if load(), unload() or resolve() for some reason fails.
*/
QString QLibrary::errorString() const
{
    return (!d || d->errorString.isEmpty()) ? tr("Unknown error") : d->errorString;
}

/*!
    \property QLibrary::loadHints
    \brief Give the load() function some hints on how it should behave.

    You can give some hints on how the symbols are resolved. Usually,
    the symbols are not resolved at load time, but resolved lazily,
    (that is, when resolve() is called). If you set the loadHint to
    ResolveAllSymbolsHint, then all symbols will be resolved at load time
    if the platform supports it.

    Setting ExportExternalSymbolsHint will make the external symbols in the
    library available for resolution in subsequent loaded libraries.

    If LoadArchiveMemberHint is set, the file name
    is composed of two components: A path which is a reference to an
    archive file followed by the second component which is the reference to
    the archive member. For instance, the fileName \c libGL.a(shr_64.o) will refer
    to the library \c shr_64.o in the archive file named \c libGL.a. This
    is only supported on the AIX platform.

    Setting PreventUnloadHint will only apply on Unix platforms.

    The interpretation of the load hints is platform dependent, and if
    you use it you are probably making some assumptions on which platform
    you are compiling for, so use them only if you understand the consequences
    of them.

    By default, none of these flags are set, so libraries will be loaded with
    lazy symbol resolution, and will not export external symbols for resolution
    in other dynamically-loaded libraries.
*/
void QLibrary::setLoadHints(LoadHints hints)
{
    if (!d) {
        d = QLibraryPrivate::findOrCreate(QString());   // ugly, but we need a d-ptr
        d->errorString.clear();
    }
    d->loadHints = hints;
}

QLibrary::LoadHints QLibrary::loadHints() const
{
    return d ? d->loadHints : (QLibrary::LoadHints)0;
}

/* Internal, for debugging */
bool qt_debug_component()
{
    static int debug_env = -1;
    if (debug_env == -1)
       debug_env = QT_PREPEND_NAMESPACE(qgetenv)("QT_DEBUG_PLUGINS").toInt();

    return debug_env != 0;
}

QT_END_NAMESPACE

#endif // QT_NO_LIBRARY
