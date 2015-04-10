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
#ifdef Q_OS_WINRT

#include "qfunctions_winrt.h"
#include "qstring.h"
#include "qbytearray.h"
#include "qhash.h"

QT_USE_NAMESPACE

// Environment ------------------------------------------------------
inline QHash<QByteArray, QByteArray> &qt_app_environment()
{
    static QHash<QByteArray, QByteArray> internalEnvironment;
    return internalEnvironment;
}

errno_t qt_winrt_getenv_s(size_t* sizeNeeded, char* buffer, size_t bufferSize, const char* varName)
{
    if (!sizeNeeded)
        return EINVAL;

    if (!qt_app_environment().contains(varName)) {
        if (buffer)
            buffer[0] = '\0';
        return ENOENT;
    }

    QByteArray value = qt_app_environment().value(varName);
    if (!value.endsWith('\0')) // win32 guarantees terminated string
        value.append('\0');

    if (bufferSize < (size_t)value.size()) {
        *sizeNeeded = value.size();
        return ERANGE;
    }

    strcpy(buffer, value.constData());
    return 0;
}

errno_t qt_winrt__putenv_s(const char* varName, const char* value)
{
    QByteArray input = value;
    if (input.isEmpty()) {
        if (qt_app_environment().contains(varName))
            qt_app_environment().remove(varName);
    } else {
        // win32 on winrt guarantees terminated string
        if (!input.endsWith('\0'))
            input.append('\0');
        qt_app_environment()[varName] = input;
    }

    return 0;
}

void qt_winrt_tzset()
{
}

void qt_winrt__tzset()
{
}

#endif // Q_OS_WINRT
