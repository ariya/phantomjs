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

#ifndef QSYSTEMLIBRARY_P_H
#define QSYSTEMLIBRARY_P_H

#include <QtCore/qglobal.h>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <QtCore/qstring.h>

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

    void *resolve(const char *symbol)
    {
        if (!m_didLoad)
            load();
        if (!m_handle)
            return 0;
#ifdef Q_OS_WINCE
        return (void*)GetProcAddress(m_handle, (const wchar_t*)QString::fromLatin1(symbol).utf16());
#else
        return (void*)GetProcAddress(m_handle, symbol);
#endif
    }

    static void *resolve(const QString &libraryName, const char *symbol)
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

#endif // Q_OS_WIN

#endif // QSYSTEMLIBRARY_P_H
