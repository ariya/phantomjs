/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/ustring.h>
#include <unicode/ures.h>

#include "qdebug.h"

QT_BEGIN_NAMESPACE

void QCollatorPrivate::init()
{
    UErrorCode status = U_ZERO_ERROR;
    QByteArray name = locale.bcp47Name().replace(QLatin1Char('-'), QLatin1Char('_')).toLatin1();
    collator = ucol_open(name.constData(), &status);
    if (U_FAILURE(status))
        qWarning("Could not create collator: %d", status);

    // enable normalization by default
    ucol_setAttribute(collator, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
}

void QCollatorPrivate::cleanup()
{
    if (collator)
        ucol_close(collator);
}

void QCollator::setCaseSensitivity(Qt::CaseSensitivity cs)
{
    detach();

    UColAttributeValue val = (cs == Qt::CaseSensitive) ? UCOL_UPPER_FIRST : UCOL_OFF;

    UErrorCode status = U_ZERO_ERROR;
    ucol_setAttribute(d->collator, UCOL_CASE_FIRST, val, &status);
    if (U_FAILURE(status))
        qWarning("ucol_setAttribute: Case First failed: %d", status);
}

Qt::CaseSensitivity QCollator::caseSensitivity() const
{
    UErrorCode status = U_ZERO_ERROR;
    UColAttributeValue attribute = ucol_getAttribute(d->collator, UCOL_CASE_FIRST, &status);
    return (attribute == UCOL_OFF) ? Qt::CaseInsensitive : Qt::CaseSensitive;
}

void QCollator::setNumericMode(bool on)
{
    detach();

    UErrorCode status = U_ZERO_ERROR;
    ucol_setAttribute(d->collator, UCOL_NUMERIC_COLLATION, on ? UCOL_ON : UCOL_OFF, &status);
    if (U_FAILURE(status))
        qWarning("ucol_setAttribute: numeric collation failed: %d", status);
}

bool QCollator::numericMode() const
{
    UErrorCode status;
    return ucol_getAttribute(d->collator, UCOL_NUMERIC_COLLATION, &status) == UCOL_ON;
}

void QCollator::setIgnorePunctuation(bool on)
{
    detach();

    UErrorCode status;
    ucol_setAttribute(d->collator, UCOL_ALTERNATE_HANDLING, on ? UCOL_SHIFTED : UCOL_NON_IGNORABLE, &status);
    if (U_FAILURE(status))
        qWarning("ucol_setAttribute: Alternate handling failed: %d", status);
}

bool QCollator::ignorePunctuation() const
{
    UErrorCode status;
    return ucol_getAttribute(d->collator, UCOL_ALTERNATE_HANDLING, &status) == UCOL_SHIFTED;
}

int QCollator::compare(const QChar *s1, int len1, const QChar *s2, int len2) const
{
    return ucol_strcoll(d->collator, (const UChar *)s1, len1, (const UChar *)s2, len2);
}

int QCollator::compare(const QString &s1, const QString &s2) const
{
    return compare(s1.constData(), s1.size(), s2.constData(), s2.size());
}

int QCollator::compare(const QStringRef &s1, const QStringRef &s2) const
{
    return compare(s1.constData(), s1.size(), s2.constData(), s2.size());
}

QCollatorSortKey QCollator::sortKey(const QString &string) const
{
    QByteArray result(16 + string.size() + (string.size() >> 2), Qt::Uninitialized);
    int size = ucol_getSortKey(d->collator, (const UChar *)string.constData(),
                               string.size(), (uint8_t *)result.data(), result.size());
    if (size > result.size()) {
        result.resize(size);
        size = ucol_getSortKey(d->collator, (const UChar *)string.constData(),
                               string.size(), (uint8_t *)result.data(), result.size());
    }
    result.truncate(size);
    return QCollatorSortKey(new QCollatorSortKeyPrivate(result));
}

int QCollatorSortKey::compare(const QCollatorSortKey &otherKey) const
{
    return qstrcmp(d->m_key, otherKey.d->m_key);
}

QT_END_NAMESPACE
