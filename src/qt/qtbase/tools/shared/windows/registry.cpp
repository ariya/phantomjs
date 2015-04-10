/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake application of the Qt Toolkit.
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

