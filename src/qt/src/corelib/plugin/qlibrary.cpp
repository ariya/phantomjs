
/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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
#include <qsettings.h>
#include <qdatetime.h>
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
#include "qelfparser_p.h"

QT_BEGIN_NAMESPACE

#ifdef QT_NO_DEBUG
#  define QLIBRARY_AS_DEBUG false
#else
#  define QLIBRARY_AS_DEBUG true
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
// We don't use separate debug and release libs on UNIX, so we want
// to allow loading plugins, regardless of how they were built.
#  define QT_NO_DEBUG_PLUGIN_CHECK
#endif

Q_GLOBAL_STATIC(QMutex, qt_library_mutex)

/*!
    \class QLibrary
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
    Unix), unless the file name has an absolute path. If the file
    cannot be found, QLibrary tries the name with different
    platform-specific file suffixes, like ".so" on Unix, ".dylib" on
    the Mac, or ".dll" on Windows and Symbian. This makes it possible
    to specify shared libraries that are only identified by their
    basename (i.e. without their suffix), so the same code will work
    on different operating systems.

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

    Note: In Symbian resolving symbols using their names is supported
    only if the library is built as STDDLL. Otherwise ordinals must
    be used. Also, in Symbian the path of the library is ignored and
    system default library location is always used.

    The following code snippet loads a library, resolves the symbol
    "mysymbol", and calls the function if everything succeeded. If
    something goes wrong, e.g. the library file does not exist or the
    symbol is not defined, the function pointer will be 0 and won't be
    called.

    \snippet doc/src/snippets/code/src_corelib_plugin_qlibrary.cpp 0

    The symbol must be exported as a C function from the library for
    resolve() to work. This means that the function must be wrapped in
    an \c{extern "C"} block if the library is compiled with a C++
    compiler. On Windows, this also requires the use of a \c dllexport
    macro; see resolve() for the details of how this is done. For
    convenience, there is a static resolve() function which you can
    use if you just want to call a function in a library without
    explicitly loading the library first:

    \snippet doc/src/snippets/code/src_corelib_plugin_qlibrary.cpp 1

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

    \sa loadHints
*/


#ifndef QT_NO_PLUGIN_CHECK
struct qt_token_info
{
    qt_token_info(const char *f, const ulong fc)
        : fields(f), field_count(fc), results(fc), lengths(fc)
    {
        results.fill(0);
        lengths.fill(0);
    }

    const char *fields;
    const ulong field_count;

    QVector<const char *> results;
    QVector<ulong> lengths;
};

/*
  return values:
       1 parse ok
       0 eos
      -1 parse error
*/
static int qt_tokenize(const char *s, ulong s_len, ulong *advance,
                        qt_token_info &token_info)
{
    if (!s)
        return -1;

    ulong pos = 0, field = 0, fieldlen = 0;
    char current;
    int ret = -1;
    *advance = 0;
    for (;;) {
        current = s[pos];

        // next char
        ++pos;
        ++fieldlen;
        ++*advance;

        if (! current || pos == s_len + 1) {
            // save result
            token_info.results[(int)field] = s;
            token_info.lengths[(int)field] = fieldlen - 1;

            // end of string
            ret = 0;
            break;
        }

        if (current == token_info.fields[field]) {
            // save result
            token_info.results[(int)field] = s;
            token_info.lengths[(int)field] = fieldlen - 1;

            // end of field
            fieldlen = 0;
            ++field;
            if (field == token_info.field_count - 1) {
                // parse ok
                ret = 1;
            }
            if (field == token_info.field_count) {
                // done parsing
                break;
            }

            // reset string and its length
            s = s + pos;
            s_len -= pos;
            pos = 0;
        }
    }

    return ret;
}

/*
  returns true if the string s was correctly parsed, false otherwise.
*/
static bool qt_parse_pattern(const char *s, uint *version, bool *debug, QByteArray *key)
{
    bool ret = true;

    qt_token_info pinfo("=\n", 2);
    int parse;
    ulong at = 0, advance, parselen = qstrlen(s);
    do {
        parse = qt_tokenize(s + at, parselen, &advance, pinfo);
        if (parse == -1) {
            ret = false;
            break;
        }

        at += advance;
        parselen -= advance;

        if (qstrncmp("version", pinfo.results[0], pinfo.lengths[0]) == 0) {
            // parse version string
            qt_token_info pinfo2("..-", 3);
            if (qt_tokenize(pinfo.results[1], pinfo.lengths[1],
                              &advance, pinfo2) != -1) {
                QByteArray m(pinfo2.results[0], pinfo2.lengths[0]);
                QByteArray n(pinfo2.results[1], pinfo2.lengths[1]);
                QByteArray p(pinfo2.results[2], pinfo2.lengths[2]);
                *version  = (m.toUInt() << 16) | (n.toUInt() << 8) | p.toUInt();
            } else {
                ret = false;
                break;
            }
        } else if (qstrncmp("debug", pinfo.results[0], pinfo.lengths[0]) == 0) {
            *debug = qstrncmp("true", pinfo.results[1], pinfo.lengths[1]) == 0;
        } else if (qstrncmp("buildkey", pinfo.results[0],
                              pinfo.lengths[0]) == 0){
            // save buildkey
            *key = QByteArray(pinfo.results[1], pinfo.lengths[1]);
        }
    } while (parse == 1 && parselen > 0);

    return ret;
}
#endif // QT_NO_PLUGIN_CHECK

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_SYMBIAN) && !defined(QT_NO_PLUGIN_CHECK)

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

  Returns false if version/key information is not present, or if the
                information could not be read.
  Returns  true if version/key information is present and successfully read.
*/
static bool qt_unix_query(const QString &library, uint *version, bool *debug, QByteArray *key, QLibraryPrivate *lib = 0)
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
       ELF binaries on GNU, have .qplugin sections.
    */
    long pos = 0;
    const char pattern[] = "pattern=QT_PLUGIN_VERIFICATION_DATA";
    const ulong plen = qstrlen(pattern);
#if defined (Q_OF_ELF) && defined(Q_CC_GNU)
    int r = QElfParser().parse(filedata, fdlen, library, lib, &pos, &fdlen);
    if (r == QElfParser::NoQtSection) {
        if (pos > 0) {
            // find inside .rodata
            long rel = qt_find_pattern(filedata + pos, fdlen, pattern, plen);
            if (rel < 0) {
                pos = -1;
            } else {
                pos += rel;
            }
        } else {
            pos = qt_find_pattern(filedata, fdlen, pattern, plen);
        }
    } else if (r != QElfParser::Ok) {
        if (lib && qt_debug_component()) {
            qWarning("QElfParser: %s",qPrintable(lib->errorString));
        }
        return false;
    }
#else
    pos = qt_find_pattern(filedata, fdlen, pattern, plen);
#endif // defined(Q_OF_ELF) && defined(Q_CC_GNU)
    bool ret = false;
    if (pos >= 0)
        ret = qt_parse_pattern(filedata + pos, version, debug, key);

    if (!ret && lib)
        lib->errorString = QLibrary::tr("Plugin verification data mismatch in '%1'").arg(library);
    file.close();
    return ret;
}

#endif // Q_OS_UNIX && !Q_OS_MAC && !defined(Q_OS_SYMBIAN) && !defined(QT_NO_PLUGIN_CHECK)

typedef QMap<QString, QLibraryPrivate*> LibraryMap;

struct LibraryData {
    LibraryMap libraryMap;
    QSet<QLibraryPrivate*> loadedLibs;
};

Q_GLOBAL_STATIC(LibraryData, libraryData)

static LibraryMap *libraryMap()
{
    LibraryData *data = libraryData();
    return data ? &data->libraryMap : 0;
}

QLibraryPrivate::QLibraryPrivate(const QString &canonicalFileName, const QString &version)
    :pHnd(0), fileName(canonicalFileName), fullVersion(version), instance(0), qt_version(0),
     libraryRefCount(1), libraryUnloadCount(0), pluginState(MightBeAPlugin)
{ libraryMap()->insert(canonicalFileName, this); }

QLibraryPrivate *QLibraryPrivate::findOrCreate(const QString &fileName, const QString &version)
{
    QMutexLocker locker(qt_library_mutex());
    if (QLibraryPrivate *lib = libraryMap()->value(fileName)) {
        lib->libraryRefCount.ref();
        return lib;
    }

    return new QLibraryPrivate(fileName, version);
}

QLibraryPrivate::~QLibraryPrivate()
{
    LibraryMap * const map = libraryMap();
    if (map) {
        QLibraryPrivate *that = map->take(fileName);
        Q_ASSERT(this == that);
	Q_UNUSED(that);
    }
}

void *QLibraryPrivate::resolve(const char *symbol)
{
    if (!pHnd)
        return 0;
    return resolve_sys(symbol);
}


bool QLibraryPrivate::load()
{
    libraryUnloadCount.ref();
    if (pHnd)
        return true;
    if (fileName.isEmpty())
        return false;

    bool ret = load_sys();
    if (ret) {
        //when loading a library we add a reference to it so that the QLibraryPrivate won't get deleted
        //this allows to unload the library at a later time
        if (LibraryData *lib = libraryData()) {
            lib->loadedLibs += this;
            libraryRefCount.ref();
        }
    }

    return ret;
}

bool QLibraryPrivate::unload()
{
    if (!pHnd)
        return false;
    if (!libraryUnloadCount.deref()) { // only unload if ALL QLibrary instance wanted to
        delete inst.data();
        if  (unload_sys()) {
            if (qt_debug_component())
                qWarning() << "QLibraryPrivate::unload succeeded on" << fileName;
            //when the library is unloaded, we release the reference on it so that 'this'
            //can get deleted
            if (LibraryData *lib = libraryData()) {
                if (lib->loadedLibs.remove(this))
                    libraryRefCount.deref();
            }
            pHnd = 0;
        }
    }

    return (pHnd == 0);
}

void QLibraryPrivate::release()
{
    QMutexLocker locker(qt_library_mutex());
    if (!libraryRefCount.deref())
        delete this;
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
#if defined(Q_OS_SYMBIAN)
        if (!instance) {
            // If resolving with function name failed (i.e. not STDDLL),
            // try resolving using known ordinal, which for
            // qt_plugin_instance function is always "2".
            instance = (QtPluginInstanceFunction)resolve("2");
        }
#endif
        return instance;
    }
    if (qt_debug_component())
        qWarning() << "QLibraryPrivate::loadPlugin failed on" << fileName << ":" << errorString;
    pluginState = IsNotAPlugin;
    return false;
}

/*!
    Returns true if \a fileName has a valid suffix for a loadable
    library; otherwise returns false.

    \table
    \header \i Platform \i Valid suffixes
    \row \i Windows     \i \c .dll, \c .DLL
    \row \i Unix/Linux  \i \c .so
    \row \i AIX  \i \c .a
    \row \i HP-UX       \i \c .sl, \c .so (HP-UXi)
    \row \i Mac OS X    \i \c .dylib, \c .bundle, \c .so
    \row \i Symbian     \i \c .dll
    \endtable

    Trailing versioning numbers on Unix are ignored.
 */
bool QLibrary::isLibrary(const QString &fileName)
{
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
    return fileName.endsWith(QLatin1String(".dll"), Qt::CaseInsensitive);
#elif defined(Q_OS_SYMBIAN)
    // Plugin stubs are also considered libraries in Symbian.
    return (fileName.endsWith(QLatin1String(".dll")) ||
            fileName.endsWith(QLatin1String(".qtplugin")));
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

#if defined (Q_OS_WIN) && defined(_MSC_VER) && _MSC_VER >= 1400  && !defined(Q_CC_INTEL)
#define QT_USE_MS_STD_EXCEPTION 1
const char* qt_try_versioninfo(void *pfn, bool *exceptionThrown)
{
    *exceptionThrown = false;
    const char *szData = 0;
    typedef const char * (*VerificationFunction)();
    VerificationFunction func = reinterpret_cast<VerificationFunction>(pfn);
    __try {
        if(func)
            szData =  func();
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        *exceptionThrown = true;
    }
    return szData;
}
#endif

#ifdef Q_CC_BOR
typedef const char * __stdcall (*QtPluginQueryVerificationDataFunction)();
#else
typedef const char * (*QtPluginQueryVerificationDataFunction)();
#endif

bool qt_get_verificationdata(QtPluginQueryVerificationDataFunction pfn, uint *qt_version, bool *debug, QByteArray *key, bool *exceptionThrown)
{
    *exceptionThrown = false;
    const char *szData = 0;
    if (!pfn)
        return false;
#ifdef QT_USE_MS_STD_EXCEPTION
    szData = qt_try_versioninfo((void *)pfn, exceptionThrown);
    if (*exceptionThrown)
        return false;
#else
    szData = pfn();
#endif

#ifdef QT_NO_PLUGIN_CHECK
	return true;
#else
	return qt_parse_pattern(szData, qt_version, debug, key);
#endif
}

bool QLibraryPrivate::isPlugin(QSettings *settings)
{
    errorString.clear();
    if (pluginState != MightBeAPlugin)
        return pluginState == IsAPlugin;

#ifndef QT_NO_PLUGIN_CHECK
    bool debug = !QLIBRARY_AS_DEBUG;
    QByteArray key;
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
        return false;
    }
#endif

    QFileInfo fileinfo(fileName);

#ifndef QT_NO_DATESTRING
    lastModified  = fileinfo.lastModified().toString(Qt::ISODate);
#endif
    QString regkey = QString::fromLatin1("Qt Plugin Cache %1.%2.%3/%4")
                     .arg((QT_VERSION & 0xff0000) >> 16)
                     .arg((QT_VERSION & 0xff00) >> 8)
                     .arg(QLIBRARY_AS_DEBUG ? QLatin1String("debug") : QLatin1String("false"))
                     .arg(fileName);
#ifdef Q_WS_MAC
    // On Mac, add the application arch to the reg key in order to
    // cache plugin information separately for each arch. This prevents
    // Qt from wrongly caching plugin load failures when the archs
    // don't match.
#if defined(__x86_64__)
    regkey += QLatin1String("-x86_64");
#elif defined(__i386__)
    regkey += QLatin1String("-i386");
#elif defined(__ppc64__)
    regkey += QLatin1String("-ppc64");
#elif defined(__ppc__)
    regkey += QLatin1String("-ppc");
#endif
#endif // Q_WS_MAC

    QStringList reg;
#ifndef QT_NO_SETTINGS
    if (!settings) {
        settings = QCoreApplicationPrivate::trolltechConf();
    }
    reg = settings->value(regkey).toStringList();
#endif
    if (reg.count() == 4 && lastModified == reg.at(3)) {
        qt_version = reg.at(0).toUInt(0, 16);
        debug = bool(reg.at(1).toInt());
        key = reg.at(2).toLatin1();
        success = qt_version != 0;
    } else {
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_SYMBIAN)
        if (!pHnd) {
            // use unix shortcut to avoid loading the library
            success = qt_unix_query(fileName, &qt_version, &debug, &key, this);
        } else
#endif
        {
            bool retryLoadLibrary = false;    // Only used on Windows with MS compiler.(false in other cases)
            do {
                bool temporary_load = false;
#ifdef Q_OS_WIN
                HMODULE hTempModule = 0;
#endif
                if (!pHnd) {
#ifdef Q_OS_WIN
                    DWORD dwFlags = (retryLoadLibrary) ? 0: DONT_RESOLVE_DLL_REFERENCES;
                    //avoid 'Bad Image' message box
                    UINT oldmode = SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
                    hTempModule = ::LoadLibraryEx((wchar_t*)QDir::toNativeSeparators(fileName).utf16(), 0, dwFlags);
                    SetErrorMode(oldmode);
#else
#  if defined(Q_OS_SYMBIAN)
                    //Guard against accidentally trying to load non-plugin libraries by making sure the stub exists
                    if (fileinfo.exists())
#  endif
                        temporary_load =  load_sys();
#endif
                }
#ifdef Q_OS_WIN
                QtPluginQueryVerificationDataFunction qtPluginQueryVerificationDataFunction = hTempModule ? (QtPluginQueryVerificationDataFunction)
#ifdef Q_OS_WINCE
                        ::GetProcAddress(hTempModule, L"qt_plugin_query_verification_data")
#else
                        ::GetProcAddress(hTempModule, "qt_plugin_query_verification_data")
#endif
                : (QtPluginQueryVerificationDataFunction) resolve("qt_plugin_query_verification_data");
#else
                QtPluginQueryVerificationDataFunction qtPluginQueryVerificationDataFunction = NULL;
#  if defined(Q_OS_SYMBIAN)
                if (temporary_load) {
                    qtPluginQueryVerificationDataFunction = (QtPluginQueryVerificationDataFunction) resolve("qt_plugin_query_verification_data");
                    // If resolving with function name failed (i.e. not STDDLL), try resolving using known ordinal
                    if (!qtPluginQueryVerificationDataFunction)
                        qtPluginQueryVerificationDataFunction = (QtPluginQueryVerificationDataFunction) resolve("1");
                }
#  else
                qtPluginQueryVerificationDataFunction = (QtPluginQueryVerificationDataFunction) resolve("qt_plugin_query_verification_data");
#  endif
#endif
                bool exceptionThrown = false;
                bool ret = qt_get_verificationdata(qtPluginQueryVerificationDataFunction,
                                                   &qt_version, &debug, &key, &exceptionThrown);
                if (!exceptionThrown) {
                    if (!ret) {
                        qt_version = 0;
                        key = "unknown";
                        if (temporary_load)
                            unload_sys();
                    } else {
                        success = true;
                    }
                    retryLoadLibrary = false;
                }
#ifdef QT_USE_MS_STD_EXCEPTION
                else {
                    // An exception was thrown when calling qt_plugin_query_verification_data().
                    // This usually happens when plugin is compiled with the /clr compiler flag,
                    // & will only work if the dependencies are loaded & DLLMain() is called.
                    // LoadLibrary() will do this, try once with this & if it fails don't load.
                    retryLoadLibrary = !retryLoadLibrary;
                }
#endif
#ifdef Q_OS_WIN
                if (hTempModule) {
                    BOOL ok = ::FreeLibrary(hTempModule);
                    if (ok) {
                        hTempModule = 0;
                    }

                }
#endif
            } while(retryLoadLibrary);  // Will be 'false' in all cases other than when an
                                        // exception is thrown(will happen only when using a MS compiler)
        }

        // Qt 4.5 compatibility: stl doesn't affect binary compatibility
        key.replace(" no-stl", "");

#ifndef QT_NO_SETTINGS
        QStringList queried;
        queried << QString::number(qt_version,16)
                << QString::number((int)debug)
                << QLatin1String(key)
                << lastModified;
        settings->setValue(regkey, queried);
#endif
    }

    if (!success) {
        if (errorString.isEmpty()){
            if (fileName.isEmpty())
                errorString = QLibrary::tr("The shared library was not found.");
            else
                errorString = QLibrary::tr("The file '%1' is not a valid Qt plugin.").arg(fileName);
        }
        return false;
    }

    pluginState = IsNotAPlugin; // be pessimistic

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
    } else if (key != QT_BUILD_KEY
               // we may have some compatibility keys, try them too:
#ifdef QT_BUILD_KEY_COMPAT
               && key != QT_BUILD_KEY_COMPAT
#endif
#ifdef QT_BUILD_KEY_COMPAT2
               && key != QT_BUILD_KEY_COMPAT2
#endif
#ifdef QT_BUILD_KEY_COMPAT3
               && key != QT_BUILD_KEY_COMPAT3
#endif
               ) {
        if (qt_debug_component()) {
            qWarning("In %s:\n"
                 "  Plugin uses incompatible Qt library\n"
                 "  expected build key \"%s\", got \"%s\"",
                 (const char*) QFile::encodeName(fileName),
                 QT_BUILD_KEY,
                 key.isEmpty() ? "<null>" : (const char *) key);
        }
        errorString = QLibrary::tr("The plugin '%1' uses incompatible Qt library."
                 " Expected build key \"%2\", got \"%3\"")
                 .arg(fileName)
                 .arg(QLatin1String(QT_BUILD_KEY))
                 .arg(key.isEmpty() ? QLatin1String("<null>") : QLatin1String((const char *) key));
#ifndef QT_NO_DEBUG_PLUGIN_CHECK
    } else if(debug != QLIBRARY_AS_DEBUG) {
        //don't issue a qWarning since we will hopefully find a non-debug? --Sam
        errorString = QLibrary::tr("The plugin '%1' uses incompatible Qt library."
                 " (Cannot mix debug and release libraries.)").arg(fileName);
#endif
    } else {
        pluginState = IsAPlugin;
    }

    return pluginState == IsAPlugin;
#else
    Q_UNUSED(settings);
    return pluginState == MightBeAPlugin;
#endif
}

/*!
    Loads the library and returns true if the library was loaded
    successfully; otherwise returns false. Since resolve() always
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
    Unloads the library and returns true if the library could be
    unloaded; otherwise returns false.

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
    Returns true if the library is loaded; otherwise returns false.

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

    Note: In Symbian the path portion of the \a fileName is ignored.
 */
QLibrary::QLibrary(const QString& fileName, QObject *parent)
    :QObject(parent), d(0), did_load(false)
{
    setFileName(fileName);
}


/*!
    Constructs a library object with the given \a parent that will
    load the library specified by \a fileName and major version number \a verNum.
    Currently, the version number is ignored on Windows and Symbian.

    We recommend omitting the file's suffix in \a fileName, since
    QLibrary will automatically look for the file with the appropriate
    suffix in accordance with the platform, e.g. ".so" on Unix,
    ".dylib" on Mac OS X, and ".dll" on Windows. (See \l{fileName}.)

    Note: In Symbian the path portion of the \a fileName is ignored.
*/
QLibrary::QLibrary(const QString& fileName, int verNum, QObject *parent)
    :QObject(parent), d(0), did_load(false)
{
    setFileNameAndVersion(fileName, verNum);
}

/*!
    Constructs a library object with the given \a parent that will
    load the library specified by \a fileName and full version number \a version.
    Currently, the version number is ignored on Windows and Symbian.

    We recommend omitting the file's suffix in \a fileName, since
    QLibrary will automatically look for the file with the appropriate
    suffix in accordance with the platform, e.g. ".so" on Unix,
    ".dylib" on Mac OS X, and ".dll" on Windows. (See \l{fileName}.)

    Note: In Symbian the path portion of the \a fileName is ignored.
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

    Note: In Symbian the path portion of the \a fileName is ignored.
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
    The \a versionNumber is ignored on Windows and Symbian.

    Note: In Symbian the path portion of the \a fileName is ignored.

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
    The \a version parameter is ignored on Windows and Symbian.

    Note: In Symbian the path portion of the \a fileName is ignored.

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
    \snippet doc/src/snippets/code/src_corelib_plugin_qlibrary.cpp 2

    The symbol must be exported as a C function from the library. This
    means that the function must be wrapped in an \c{extern "C"} if
    the library is compiled with a C++ compiler. On Windows you must
    also explicitly export the function from the DLL using the
    \c{__declspec(dllexport)} compiler directive, for example:

    \snippet doc/src/snippets/code/src_corelib_plugin_qlibrary.cpp 3

    with \c MY_EXPORT defined as

    \snippet doc/src/snippets/code/src_corelib_plugin_qlibrary.cpp 4

    Note: In Symbian resolving with symbol names works only if the loaded
    library was built as STDDLL. Otherwise, the ordinals must be used.
*/
void *QLibrary::resolve(const char *symbol)
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

    Note: In Symbian resolving with symbol names works only if the loaded
    library was built as STDDLL. Otherwise, the ordinals must be used.

    \sa resolve()
*/
void *QLibrary::resolve(const QString &fileName, const char *symbol)
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

    Note: In Symbian resolving with symbol names works only if the loaded
    library was built as STDDLL. Otherwise, the ordinals must be used.

    \sa resolve()
*/
void *QLibrary::resolve(const QString &fileName, int verNum, const char *symbol)
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

    Note: In Symbian resolving with symbol names works only if the loaded
    library was built as STDDLL. Otherwise, the ordinals must be used.

    \sa resolve()
*/
void *QLibrary::resolve(const QString &fileName, const QString &version, const char *symbol)
{
    QLibrary library(fileName, version);
    return library.resolve(symbol);
}

/*!
    \fn QString QLibrary::library() const

    Use fileName() instead.
*/

/*!
    \fn void QLibrary::setAutoUnload( bool b )

    Use load(), isLoaded(), and unload() as necessary instead.
*/

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
