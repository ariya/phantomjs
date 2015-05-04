/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013 Aleix Pol Gonzalez <aleixpol@kde.org>
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
    cleanup();

    UErrorCode status = U_ZERO_ERROR;
    QByteArray name = locale.bcp47Name().replace(QLatin1Char('-'), QLatin1Char('_')).toLatin1();
    collator = ucol_open(name.constData(), &status);
    if (U_FAILURE(status))
        qWarning("Could not create collator: %d", status);

    // enable normalization by default
    ucol_setAttribute(collator, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);

    // The strength attribute in ICU is rather badly documented. Basically UCOL_PRIMARY
    // ignores differences between base characters and accented characters as well as case.
    // So A and A-umlaut would compare equal.
    // UCOL_SECONDARY ignores case differences. UCOL_TERTIARY is the default in most languages
    // and does case sensitive comparison.
    // UCOL_QUATERNARY is used as default in a few languages such as Japanese to take care of some
    // additional differences in those languages.
    UColAttributeValue val = (caseSensitivity == Qt::CaseSensitive) ? UCOL_DEFAULT_STRENGTH : UCOL_SECONDARY;

    status = U_ZERO_ERROR;
    ucol_setAttribute(collator, UCOL_STRENGTH, val, &status);
    if (U_FAILURE(status))
        qWarning("ucol_setAttribute: Case First failed: %d", status);

    status = U_ZERO_ERROR;
    ucol_setAttribute(collator, UCOL_NUMERIC_COLLATION, numericMode ? UCOL_ON : UCOL_OFF, &status);
    if (U_FAILURE(status))
        qWarning("ucol_setAttribute: numeric collation failed: %d", status);

    status = U_ZERO_ERROR;
    ucol_setAttribute(collator, UCOL_ALTERNATE_HANDLING, ignorePunctuation ? UCOL_SHIFTED : UCOL_NON_IGNORABLE, &status);
    if (U_FAILURE(status))
        qWarning("ucol_setAttribute: Alternate handling failed: %d", status);

    dirty = false;
}

void QCollatorPrivate::cleanup()
{
    if (collator)
        ucol_close(collator);
    collator = 0;
}

int QCollator::compare(const QChar *s1, int len1, const QChar *s2, int len2) const
{
    if (d->dirty)
        d->init();

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
    if (d->dirty)
        d->init();

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
