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

#include "qplatformdefs.h"
#include "qurl.h"
#include "private/qurltlds_p.h"
#include "private/qtldurl_p.h"
#include "QtCore/qstringlist.h"

QT_BEGIN_NAMESPACE

static bool containsTLDEntry(const QString &entry)
{
    int index = qHash(entry) % tldCount;
    int currentDomainIndex = tldIndices[index];
    while (currentDomainIndex < tldIndices[index+1]) {
        QString currentEntry = QString::fromUtf8(tldData + currentDomainIndex);
        if (currentEntry == entry)
            return true;
        currentDomainIndex += qstrlen(tldData + currentDomainIndex) + 1; // +1 for the ending \0
    }
    return false;
}

/*!
    \internal

    Return the top-level-domain per Qt's copy of the Mozilla public suffix list of
    \a domain.
*/

Q_CORE_EXPORT QString qTopLevelDomain(const QString &domain)
{
    QStringList sections = domain.toLower().split(QLatin1Char('.'), QString::SkipEmptyParts);
    if (sections.isEmpty())
        return QString();

    QString level, tld;
    for (int j = sections.count() - 1; j >= 0; --j) {
        level.prepend(QLatin1Char('.') + sections.at(j));
        if (qIsEffectiveTLD(level.right(level.size() - 1)))
            tld = level;
    }
    return tld;
}

/*!
    \internal

    Return true if \a domain is a top-level-domain per Qt's copy of the Mozilla public suffix list.
*/

Q_CORE_EXPORT bool qIsEffectiveTLD(const QString &domain)
{
    // for domain 'foo.bar.com':
    // 1. return if TLD table contains 'foo.bar.com'
    if (containsTLDEntry(domain))
        return true;

    if (domain.contains(QLatin1Char('.'))) {
        int count = domain.size() - domain.indexOf(QLatin1Char('.'));
        QString wildCardDomain;
        wildCardDomain.reserve(count + 1);
        wildCardDomain.append(QLatin1Char('*'));
        wildCardDomain.append(domain.right(count));
        // 2. if table contains '*.bar.com',
        // test if table contains '!foo.bar.com'
        if (containsTLDEntry(wildCardDomain)) {
            QString exceptionDomain;
            exceptionDomain.reserve(domain.size() + 1);
            exceptionDomain.append(QLatin1Char('!'));
            exceptionDomain.append(domain);
            return (! containsTLDEntry(exceptionDomain));
        }
    }
    return false;
}

QT_END_NAMESPACE
