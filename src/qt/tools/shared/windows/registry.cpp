/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the qmake application of the Qt Toolkit.
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

#include <QtCore/qstringlist.h>
#include "registry_p.h"

QT_BEGIN_NAMESPACE

#ifdef Q_OS_WIN32
/*!
  Returns the path part of a registry key.
  e.g.
      For a key
          "Software\\Microsoft\\VisualStudio\\8.0\\Setup\\VC\\ProductDir"
      it returns
          "Software\\Microsoft\\VisualStudio\\8.0\\Setup\\VC\\"
*/
static QString keyPath(const QString &rKey)
{
    int idx = rKey.lastIndexOf(QLatin1Char('\\'));
    if (idx == -1)
        return QString();
    return rKey.left(idx + 1);
}

/*!
  Returns the name part of a registry key.
  e.g.
      For a key
          "Software\\Microsoft\\VisualStudio\\8.0\\Setup\\VC\\ProductDir"
      it returns
          "ProductDir"
*/
static QString keyName(const QString &rKey)
{
    int idx = rKey.lastIndexOf(QLatin1Char('\\'));
    if (idx == -1)
        return rKey;

    QString res(rKey.mid(idx + 1));
    if (res == QLatin1String("Default") || res == QLatin1String("."))
        res = QString();
    return res;
}
#endif

QString qt_readRegistryKey(HKEY parentHandle, const QString &rSubkey)
{
    QString result;

#ifdef Q_OS_WIN32
    QString rSubkeyName = keyName(rSubkey);
    QString rSubkeyPath = keyPath(rSubkey);

    HKEY handle = 0;
    LONG res = RegOpenKeyEx(parentHandle, (wchar_t*)rSubkeyPath.utf16(), 0, KEY_READ, &handle);

    if (res != ERROR_SUCCESS)
        return QString();

    // get the size and type of the value
    DWORD dataType;
    DWORD dataSize;
    res = RegQueryValueEx(handle, (wchar_t*)rSubkeyName.utf16(), 0, &dataType, 0, &dataSize);
    if (res != ERROR_SUCCESS) {
        RegCloseKey(handle);
        return QString();
    }

    // get the value
    QByteArray data(dataSize, 0);
    res = RegQueryValueEx(handle, (wchar_t*)rSubkeyName.utf16(), 0, 0,
                          reinterpret_cast<unsigned char*>(data.data()), &dataSize);
    if (res != ERROR_SUCCESS) {
        RegCloseKey(handle);
        return QString();
    }

    switch (dataType) {
        case REG_EXPAND_SZ:
        case REG_SZ: {
            result = QString::fromWCharArray(((const wchar_t *)data.constData()));
            break;
        }

        case REG_MULTI_SZ: {
            QStringList l;
            int i = 0;
            for (;;) {
                QString s = QString::fromWCharArray((const wchar_t *)data.constData() + i);
                i += s.length() + 1;

                if (s.isEmpty())
                    break;
                l.append(s);
            }
            result = l.join(QLatin1String(", "));
            break;
        }

        case REG_NONE:
        case REG_BINARY: {
            result = QString::fromWCharArray((const wchar_t *)data.constData(), data.size() / 2);
            break;
        }

        case REG_DWORD_BIG_ENDIAN:
        case REG_DWORD: {
            Q_ASSERT(data.size() == sizeof(int));
            int i;
            memcpy((char*)&i, data.constData(), sizeof(int));
            result = QString::number(i);
            break;
        }

        default:
            qWarning("QSettings: unknown data %u type in windows registry", quint32(dataType));
            break;
    }

    RegCloseKey(handle);
#else
    Q_UNUSED(parentHandle);
    Q_UNUSED(rSubkey)
#endif

    return result;
}

QT_END_NAMESPACE

