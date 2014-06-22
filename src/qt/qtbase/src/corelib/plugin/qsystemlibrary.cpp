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

#include "qsystemlibrary_p.h"
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qfileinfo.h>

/*!

    \internal
    \class QSystemLibrary
    \inmodule QtCore

    The purpose of this class is to load only libraries that are located in
    well-known and trusted locations on the filesystem. It does not suffer from
    the security problem that QLibrary has, therefore it will never search in
    the current directory.

    The search order is the same as the order in DLL Safe search mode Windows,
    except that we don't search:
    * The current directory
    * The 16-bit system directory. (normally \c{c:\windows\system})
    * The Windows directory.  (normally \c{c:\windows})

    This means that the effective search order is:
    1. Application path.
    2. System libraries path.
    3. Trying all paths inside the PATH environment variable.

    Note, when onlySystemDirectory is true it will skip 1) and 3).

    DLL Safe search mode is documented in the "Dynamic-Link Library Search
    Order" document on MSDN.

    Since library loading code is sometimes shared between Windows and WinCE,
    this class can also be used on WinCE. However, its implementation just
    calls the LoadLibrary() function. This is ok since it is documented as not
    loading from the current directory on WinCE. This behaviour is documented
    in the documentation for LoadLibrary for Windows CE at MSDN.
    (http://msdn.microsoft.com/en-us/library/ms886736.aspx)
*/

QT_BEGIN_NAMESPACE

#if defined(Q_OS_WINCE)
HINSTANCE QSystemLibrary::load(const wchar_t *libraryName, bool onlySystemDirectory /* = true */)
{
    return ::LoadLibrary(libraryName);
}
#elif defined(Q_OS_WINRT)
HINSTANCE QSystemLibrary::load(const wchar_t *libraryName, bool onlySystemDirectory /* = true */)
{
    Q_UNUSED(onlySystemDirectory);
    return ::LoadPackagedLibrary(libraryName, 0);
}
#else

#if !defined(QT_BOOTSTRAPPED)
extern QString qAppFileName();
#endif

static QString qSystemDirectory()
{
    QVarLengthArray<wchar_t, MAX_PATH> fullPath;

    UINT retLen = ::GetSystemDirectory(fullPath.data(), MAX_PATH);
    if (retLen > MAX_PATH) {
        fullPath.resize(retLen);
        retLen = ::GetSystemDirectory(fullPath.data(), retLen);
    }
    // in some rare cases retLen might be 0
    return QString::fromWCharArray(fullPath.constData(), int(retLen));
}

HINSTANCE QSystemLibrary::load(const wchar_t *libraryName, bool onlySystemDirectory /* = true */)
{
    QStringList searchOrder;

#if !defined(QT_BOOTSTRAPPED)
    if (!onlySystemDirectory)
        searchOrder << QFileInfo(qAppFileName()).path();
#endif
    searchOrder << qSystemDirectory();

    if (!onlySystemDirectory) {
        const QString PATH(QLatin1String(qgetenv("PATH").constData()));
        searchOrder << PATH.split(QLatin1Char(';'), QString::SkipEmptyParts);
    }
    QString fileName = QString::fromWCharArray(libraryName);
    fileName.append(QLatin1String(".dll"));

    // Start looking in the order specified
    for (int i = 0; i < searchOrder.count(); ++i) {
        QString fullPathAttempt = searchOrder.at(i);
        if (!fullPathAttempt.endsWith(QLatin1Char('\\'))) {
            fullPathAttempt.append(QLatin1Char('\\'));
        }
        fullPathAttempt.append(fileName);
        HINSTANCE inst = ::LoadLibrary((const wchar_t *)fullPathAttempt.utf16());
        if (inst != 0)
            return inst;
    }
    return 0;

}

#endif  //Q_OS_WINCE

QT_END_NAMESPACE
