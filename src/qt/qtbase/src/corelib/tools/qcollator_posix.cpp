/****************************************************************************
**
** Copyright (C) 2013 Aleix Pol Gonzalez <aleixpol@kde.org>
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

#include "qcollator_p.h"
#include "qstringlist.h"
#include "qstring.h"

#include <cstring>
#include <cwchar>

QT_BEGIN_NAMESPACE

void QCollatorPrivate::init()
{
}

void QCollatorPrivate::cleanup()
{
}

void QCollator::setCaseSensitivity(Qt::CaseSensitivity cs)
{
    Q_UNUSED(cs);
    qWarning("unsupported in the posix collation implementation");
}

Qt::CaseSensitivity QCollator::caseSensitivity() const
{
    qWarning("unsupported in the posix collation implementation");
    return Qt::CaseSensitive;
}

void QCollator::setNumericMode(bool on)
{
    Q_UNUSED(on);
    qWarning("unsupported in the posix collation implementation");
}

bool QCollator::numericMode() const
{
    return true;
}

void QCollator::setIgnorePunctuation(bool on)
{
    Q_UNUSED(on);
    qWarning("unsupported in the posix collation implementation");
}

bool QCollator::ignorePunctuation() const
{
    qWarning("unsupported in the posix collation implementation");
    return false;
}

static void stringToWCharArray(QVarLengthArray<wchar_t> &ret, const QString &string)
{
    ret.resize(string.length());
    int len = string.toWCharArray(ret.data());
    ret.resize(len+1);
    ret[len] = 0;
}

int QCollator::compare(const QChar *s1, int len1, const QChar *s2, int len2) const
{
    QVarLengthArray<wchar_t> array1, array2;
    stringToWCharArray(array1, QString(s1, len1));
    stringToWCharArray(array2, QString(s2, len2));
    return std::wcscoll(array1.constData(), array2.constData());
}

int QCollator::compare(const QString &s1, const QString &s2) const
{
    QVarLengthArray<wchar_t> array1, array2;
    stringToWCharArray(array1, s1);
    stringToWCharArray(array2, s2);
    return std::wcscoll(array1.constData(), array2.constData());
}

int QCollator::compare(const QStringRef &s1, const QStringRef &s2) const
{
    return compare(s1.constData(), s1.size(), s2.constData(), s2.size());
}

QCollatorSortKey QCollator::sortKey(const QString &string) const
{
    QVarLengthArray<wchar_t> original;
    stringToWCharArray(original, string);
    QVector<wchar_t> result(string.size());
    size_t size = std::wcsxfrm(result.data(), original.constData(), string.size());
    if (size > uint(result.size())) {
        result.resize(size+1);
        size = std::wcsxfrm(result.data(), original.constData(), string.size());
    }
    result.resize(size+1);
    result[size] = 0;
    return QCollatorSortKey(new QCollatorSortKeyPrivate(result));
}

int QCollatorSortKey::compare(const QCollatorSortKey &otherKey) const
{
    return std::wcscmp(d->m_key.constData(),
                  otherKey.d->m_key.constData());
}

QT_END_NAMESPACE
