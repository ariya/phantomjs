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

#include <private/qcore_mac_p.h>
#include <new>
#include "qvarlengtharray.h"

QT_BEGIN_NAMESPACE

QString QCFString::toQString(CFStringRef str)
{
    if(!str)
        return QString();
    CFIndex length = CFStringGetLength(str);
    const UniChar *chars = CFStringGetCharactersPtr(str);
    if (chars)
        return QString(reinterpret_cast<const QChar *>(chars), length);

    QVarLengthArray<UniChar> buffer(length);
    CFStringGetCharacters(str, CFRangeMake(0, length), buffer.data());
    return QString(reinterpret_cast<const QChar *>(buffer.constData()), length);
}

QCFString::operator QString() const
{
    if (string.isEmpty() && type)
        const_cast<QCFString*>(this)->string = toQString(type);
    return string;
}

CFStringRef QCFString::toCFStringRef(const QString &string)
{
    return CFStringCreateWithCharacters(0, reinterpret_cast<const UniChar *>(string.unicode()),
                                        string.length());
}

QCFString::operator CFStringRef() const
{
    if (!type)
        const_cast<QCFString*>(this)->type = toCFStringRef(string);
    return type;
}

QT_END_NAMESPACE
