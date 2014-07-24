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

#include <qfile.h>
#include "qlibrary_p.h"
#include <qcoreapplication.h>
#include <private/qfilesystementry_p.h>

#ifndef QT_NO_LIBRARY

#ifdef Q_OS_MAC
#  include <private/qcore_mac_p.h>
#endif

#if defined(QT_AOUT_UNDERSCORE)
#include <string.h>
#endif

#if (defined(Q_OS_VXWORKS) && !defined(VXWORKS_RTP)) || defined (Q_OS_NACL)
#define QT_NO_DYNAMIC_LIBRARY
#endif

QT_BEGIN_NAMESPACE

#if !defined(QT_HPUX_LD) && !defined(QT_NO_DYNAMIC_LIBRARY)
QT_BEGIN_INCLUDE_NAMESPACE
#include <dlfcn.h>
QT_END_INCLUDE_NAMESPACE
#endif

static QString qdlerror()
{
#if defined(QT_NO_DYNAMIC_LIBRARY)
    const char *err = "This platform does not support dynamic libraries.";
#elif !defined(QT_HPUX_LD)
    const char *err = dlerror();
#else
    const char *err = strerror(errno);
#endif
    return err ? QLatin1Char('(') + QString::fromLocal8Bit(err) + QLatin1Char(')'): QString();
}

QStringList QLibraryPrivate::suffixes_sys(const QString& fullVersion)
{
    QStringList suffixes;
#if defined(Q_OS_HPUX)
    // according to
    // http://docs.hp.com/en/B2355-90968/linkerdifferencesiapa.htm

    // In PA-RISC (PA-32 and PA-64) shared libraries are suffixed
    // with .sl. In IPF (32-bit and 64-bit), the shared libraries
    // are suffixed with .so. For compatibility, the IPF linker
    // also supports the .sl suffix.

    // But since we don't know if we are built on HPUX or HPUXi,
    // we support both .sl (and .<version>) and .so suffixes but
    // .so is preferred.
# if defined(__ia64)
    if (!fullVersion.isEmpty()) {
        suffixes << QString::fromLatin1(".so.%1").arg(fullVersion);
    } else {
        suffixes << QLatin1String(".so");
    }
# endif
    if (!fullVersion.isEmpty()) {
        suffixes << QString::fromLatin1(".sl.%1").arg(fullVersion);
        suffixes << QString::fromLatin1(".%1").arg(fullVersion);
    } else {
        suffixes << QLatin1String(".sl");
    }
#elif defined(Q_OS_AIX)
    suffixes << ".a";

#else
    if (!fullVersion.isEmpty()) {
        suffixes << QString::fromLatin1(".so.%1").arg(fullVersion);
    } else {
        suffixes << QLatin1String(".so");
    }
#endif
# ifdef Q_OS_MAC
    if (!fullVersion.isEmpty()) {
        suffixes << QString::fromLatin1(".%1.bundle").arg(fullVersion);
        suffixes << QString::fromLatin1(".%1.dylib").arg(fullVersion);
    } else {
        suffixes << QLatin1String(".bundle") << QLatin1String(".dylib");
    }
#endif
    return suffixes;
}

QStringList QLibraryPrivate::prefixes_sys()
{
    return QStringList() << QLatin1String("lib");
}

bool QLibraryPrivate::load_sys()
{
    QString attempt;
#if !defined(QT_NO_DYNAMIC_LIBRARY)
    QFileSystemEntry fsEntry(fileName);

    QString path = fsEntry.path();
    QString name = fsEntry.fileName();
    if (path == QLatin1String(".") && !fileName.startsWith(path))
        path.clear();
    else
        path += QLatin1Char('/');

    QStringList suffixes;
    QStringList prefixes;
    if (pluginState != IsAPlugin) {
        prefixes = prefixes_sys();
        suffixes = suffixes_sys(fullVersion);
    }
    int dlFlags = 0;
#if defined(QT_HPUX_LD)
    dlFlags = DYNAMIC_PATH | BIND_NONFATAL;
    if (loadHints & QLibrary::ResolveAllSymbolsHint) {
        dlFlags |= BIND_IMMEDIATE;
    } else {
        dlFlags |= BIND_DEFERRED;
    }
#else
    if (loadHints & QLibrary::ResolveAllSymbolsHint) {
        dlFlags |= RTLD_NOW;
    } else {
        dlFlags |= RTLD_LAZY;
    }
    if (loadHints & QLibrary::ExportExternalSymbolsHint) {
        dlFlags |= RTLD_GLOBAL;
    }
#if !defined(Q_OS_CYGWIN)
    else {
#if defined(Q_OS_MAC)
        if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_4)
#endif
        dlFlags |= RTLD_LOCAL;
    }
#endif

    // Provide access to RTLD_NODELETE flag on Unix
    // From GNU documentation on RTLD_NODELETE:
    // Do not unload the library during dlclose(). Consequently, the
    // library's specific static variables are not reinitialized if the
    // library is reloaded with dlopen() at a later time.
#ifdef RTLD_NODELETE
    if (loadHints & QLibrary::PreventUnloadHint) {
        dlFlags |= RTLD_NODELETE;
    }
#endif

#if defined(Q_OS_AIX)   // Not sure if any other platform actually support this thing.
    if (loadHints & QLibrary::LoadArchiveMemberHint) {
        dlFlags |= RTLD_MEMBER;
    }
#endif
#endif // QT_HPUX_LD

    // If the filename is an absolute path then we want to try that first as it is most likely
    // what the callee wants. If we have been given a non-absolute path then lets try the
    // native library name first to avoid unnecessary calls to dlopen().
    if (fsEntry.isAbsolute()) {
        suffixes.prepend(QString());
        prefixes.prepend(QString());
    } else {
        suffixes.append(QString());
        prefixes.append(QString());
    }

    bool retry = true;
    for(int prefix = 0; retry && !pHnd && prefix < prefixes.size(); prefix++) {
        for(int suffix = 0; retry && !pHnd && suffix < suffixes.size(); suffix++) {
            if (!prefixes.at(prefix).isEmpty() && name.startsWith(prefixes.at(prefix)))
                continue;
            if (!suffixes.at(suffix).isEmpty() && name.endsWith(suffixes.at(suffix)))
                continue;
            if (loadHints & QLibrary::LoadArchiveMemberHint) {
                attempt = name;
                int lparen = attempt.indexOf(QLatin1Char('('));
                if (lparen == -1)
                    lparen = attempt.count();
                attempt = path + prefixes.at(prefix) + attempt.insert(lparen, suffixes.at(suffix));
            } else {
                attempt = path + prefixes.at(prefix) + name + suffixes.at(suffix);
            }
#if defined(QT_HPUX_LD)
            pHnd = (void*)shl_load(QFile::encodeName(attempt), dlFlags, 0);
#else
            pHnd = dlopen(QFile::encodeName(attempt), dlFlags);
#endif

            if (!pHnd && fileName.startsWith(QLatin1Char('/')) && QFile::exists(attempt)) {
                // We only want to continue if dlopen failed due to that the shared library did not exist.
                // However, we are only able to apply this check for absolute filenames (since they are
                // not influenced by the content of LD_LIBRARY_PATH, /etc/ld.so.cache, DT_RPATH etc...)
                // This is all because dlerror is flawed and cannot tell us the reason why it failed.
                retry = false;
            }
        }
    }

#ifdef Q_OS_MAC
    if (!pHnd) {
        QByteArray utf8Bundle = fileName.toUtf8();
        QCFType<CFURLRef> bundleUrl = CFURLCreateFromFileSystemRepresentation(NULL, reinterpret_cast<const UInt8*>(utf8Bundle.data()), utf8Bundle.length(), true);
        QCFType<CFBundleRef> bundle = CFBundleCreate(NULL, bundleUrl);
        if(bundle) {
            QCFType<CFURLRef> url = CFBundleCopyExecutableURL(bundle);
            char executableFile[FILENAME_MAX];
            CFURLGetFileSystemRepresentation(url, true, reinterpret_cast<UInt8*>(executableFile), FILENAME_MAX);
            attempt = QString::fromUtf8(executableFile);
            pHnd = dlopen(QFile::encodeName(attempt), dlFlags);
        }
    }
#endif
#endif // QT_NO_DYNAMIC_LIBRARY
    if (!pHnd) {
        errorString = QLibrary::tr("Cannot load library %1: %2").arg(fileName).arg(qdlerror());
    }
    if (pHnd) {
        qualifiedFileName = attempt;
        errorString.clear();
    }
    return (pHnd != 0);
}

bool QLibraryPrivate::unload_sys()
{
#if !defined(QT_NO_DYNAMIC_LIBRARY)
#  if defined(QT_HPUX_LD)
    if (shl_unload((shl_t)pHnd)) {
#  else
    if (dlclose(pHnd)) {
#  endif
#  if defined (Q_OS_QNX)              // Workaround until fixed in QNX; fixes crash in
        char *error = dlerror();      // QtDeclarative auto test "qqmlenginecleanup" for instance
        if (!qstrcmp(error, "Shared objects still referenced")) // On QNX that's only "informative"
            return true;
        errorString = QLibrary::tr("Cannot unload library %1: %2").arg(fileName)
                                                                  .arg(QLatin1String(error));
#  else
        errorString = QLibrary::tr("Cannot unload library %1: %2").arg(fileName).arg(qdlerror());
#  endif
        return false;
    }
#endif
    errorString.clear();
    return true;
}

#ifdef Q_OS_LINUX
Q_CORE_EXPORT QFunctionPointer qt_linux_find_symbol_sys(const char *symbol)
{
    return QFunctionPointer(dlsym(RTLD_DEFAULT, symbol));
}
#endif

#ifdef Q_OS_MAC
Q_CORE_EXPORT QFunctionPointer qt_mac_resolve_sys(void *handle, const char *symbol)
{
    return QFunctionPointer(dlsym(handle, symbol));
}
#endif

QFunctionPointer QLibraryPrivate::resolve_sys(const char* symbol)
{
#if defined(QT_AOUT_UNDERSCORE)
    // older a.out systems add an underscore in front of symbols
    char* undrscr_symbol = new char[strlen(symbol)+2];
    undrscr_symbol[0] = '_';
    strcpy(undrscr_symbol+1, symbol);
    QFunctionPointer address = QFunctionPointer(dlsym(pHnd, undrscr_symbol));
    delete [] undrscr_symbol;
#elif defined(QT_HPUX_LD)
    QFunctionPointer address = 0;
    if (shl_findsym((shl_t*)&pHnd, symbol, TYPE_UNDEFINED, &address) < 0)
        address = 0;
#elif defined (QT_NO_DYNAMIC_LIBRARY)
    QFunctionPointer address = 0;
#else
    QFunctionPointer address = QFunctionPointer(dlsym(pHnd, symbol));
#endif
    if (!address) {
        errorString = QLibrary::tr("Cannot resolve symbol \"%1\" in %2: %3").arg(
            QString::fromLatin1(symbol)).arg(fileName).arg(qdlerror());
    } else {
        errorString.clear();
    }
    return address;
}

QT_END_NAMESPACE

#endif // QT_NO_LIBRARY
