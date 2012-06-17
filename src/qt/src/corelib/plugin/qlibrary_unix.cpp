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

#include <qfile.h>
#include "qlibrary_p.h"
#include <qfileinfo.h>
#include <qcoreapplication.h>

#ifndef QT_NO_LIBRARY

#ifdef Q_OS_MAC
#  include <private/qcore_mac_p.h>
#endif

#if defined(QT_AOUT_UNDERSCORE)
#include <string.h>
#endif

#if defined(Q_OS_VXWORKS) || defined (Q_OS_NACL)
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

bool QLibraryPrivate::load_sys()
{
    QString attempt;
#if !defined(QT_NO_DYNAMIC_LIBRARY)
    QFileInfo fi(fileName);

#if defined(Q_OS_SYMBIAN)
    QString path; // In Symbian, always resolve with just the filename
    QString name;

    // Replace possible ".qtplugin" suffix with ".dll"
    if (fi.suffix() == QLatin1String("qtplugin"))
        name = fi.completeBaseName() + QLatin1String(".dll");
    else
        name = fi.fileName();
#else
    QString path = fi.path();
    QString name = fi.fileName();
    if (path == QLatin1String(".") && !fileName.startsWith(path))
        path.clear();
    else
        path += QLatin1Char('/');
#endif
    // The first filename we want to attempt to load is the filename as the callee specified.
    // Thus, the first attempt we do must be with an empty prefix and empty suffix.
    QStringList suffixes(QLatin1String("")), prefixes(QLatin1String(""));
    if (pluginState != IsAPlugin) {
#if !defined(Q_OS_SYMBIAN)
        prefixes << QLatin1String("lib");
#endif
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
#elif defined(Q_OS_SYMBIAN)
        suffixes << QLatin1String(".dll");
#else
#ifdef Q_OS_AIX
        suffixes << ".a";
#endif // Q_OS_AIX
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
#if defined(Q_OS_AIX)	// Not sure if any other platform actually support this thing.
    if (loadHints & QLibrary::LoadArchiveMemberHint) {
        dlFlags |= RTLD_MEMBER;
    }
#endif
#endif // QT_HPUX_LD
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

#if defined(Q_OS_SYMBIAN)
            // Never try again in symbian, dlopen already handles the library search logic,
            // and there is only one possible suffix.
            retry = false;
#else
            if (!pHnd && fileName.startsWith(QLatin1Char('/')) && QFile::exists(attempt)) {
                // We only want to continue if dlopen failed due to that the shared library did not exist.
                // However, we are only able to apply this check for absolute filenames (since they are
                // not influenced by the content of LD_LIBRARY_PATH, /etc/ld.so.cache, DT_RPATH etc...)
                // This is all because dlerror is flawed and cannot tell us the reason why it failed.
                retry = false;
            }
#endif
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
        errorString = QLibrary::tr("Cannot unload library %1: %2").arg(fileName).arg(qdlerror());
        return false;
    }
#endif
    errorString.clear();
    return true;
}

#ifdef Q_OS_MAC
Q_CORE_EXPORT void *qt_mac_resolve_sys(void *handle, const char *symbol)
{
    return dlsym(handle, symbol);
}
#endif

void* QLibraryPrivate::resolve_sys(const char* symbol)
{
#if defined(QT_AOUT_UNDERSCORE)
    // older a.out systems add an underscore in front of symbols
    char* undrscr_symbol = new char[strlen(symbol)+2];
    undrscr_symbol[0] = '_';
    strcpy(undrscr_symbol+1, symbol);
    void* address = dlsym(pHnd, undrscr_symbol);
    delete [] undrscr_symbol;
#elif defined(QT_HPUX_LD)
    void* address = 0;
    if (shl_findsym((shl_t*)&pHnd, symbol, TYPE_UNDEFINED, &address) < 0)
        address = 0;
#elif defined (QT_NO_DYNAMIC_LIBRARY)
    void *address = 0;
#else
    void* address = dlsym(pHnd, symbol);
#endif
    if (!address) {
        errorString = QLibrary::tr("Cannot resolve symbol \"%1\" in %2: %3").arg(
            QString::fromAscii(symbol)).arg(fileName).arg(qdlerror());
    } else {
        errorString.clear();
    }
    return address;
}

QT_END_NAMESPACE

#endif // QT_NO_LIBRARY
