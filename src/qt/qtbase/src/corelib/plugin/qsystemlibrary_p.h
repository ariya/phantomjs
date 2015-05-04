/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSYSTEMLIBRARY_P_H
#define QSYSTEMLIBRARY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#ifdef Q_OS_WIN
#  include <QtCore/qstring.h>
#  include <qt_windows.h>

QT_BEGIN_NAMESPACE

class QSystemLibrary
{
public:
    explicit QSystemLibrary(const QString &libraryName)
    {
        m_libraryName = libraryName;
        m_handle = 0;
        m_didLoad = false;
    }

    explicit QSystemLibrary(const wchar_t *libraryName)
    {
        m_libraryName = QString::fromWCharArray(libraryName);
        m_handle = 0;
        m_didLoad = false;
    }

    bool load(bool onlySystemDirectory = true)
    {
        m_handle = load((const wchar_t *)m_libraryName.utf16(), onlySystemDirectory);
        m_didLoad = true;
        return (m_handle != 0);
    }

    bool isLoaded()
    {
        return (m_handle != 0);
    }

    QFunctionPointer resolve(const char *symbol)
    {
        if (!m_didLoad)
            load();
        if (!m_handle)
            return 0;
#ifdef Q_OS_WINCE
        return QFunctionPointer(GetProcAddress(m_handle, (const wchar_t*)QString::fromLatin1(symbol).utf16()));
#else
        return QFunctionPointer(GetProcAddress(m_handle, symbol));
#endif
    }

    static QFunctionPointer resolve(const QString &libraryName, const char *symbol)
    {
        return QSystemLibrary(libraryName).resolve(symbol);
    }

    static Q_CORE_EXPORT HINSTANCE load(const wchar_t *lpFileName, bool onlySystemDirectory = true);
private:
    HINSTANCE m_handle;
    QString m_libraryName;
    bool m_didLoad;
};

QT_END_NAMESPACE

#endif  //Q_OS_WIN

#endif  //QSYSTEMLIBRARY_P_H
