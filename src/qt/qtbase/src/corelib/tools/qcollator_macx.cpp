/****************************************************************************
**
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
#include <QtCore/private/qcore_mac_p.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFLocale.h>

#include <cstring>
#include <QDebug>

QT_BEGIN_NAMESPACE

void QCollatorPrivate::init()
{
    cleanup();
    LocaleRef localeRef;
    int rc = LocaleRefFromLocaleString(locale.bcp47Name().toLocal8Bit(), &localeRef);
    if (rc != 0)
        qWarning() << "couldn't initialize the locale";

    UInt32 options = 0;

    if (caseSensitivity == Qt::CaseInsensitive)
        options |= kUCCollateCaseInsensitiveMask;
    if (numericMode)
        options |= kUCCollateDigitsAsNumberMask | kUCCollateDigitsOverrideMask;
    if (ignorePunctuation)
        options |= kUCCollatePunctuationSignificantMask;

    OSStatus status = UCCreateCollator(
        localeRef,
        0,
        options,
        &collator
    );
    if (status != 0)
        qWarning() << "Couldn't initialize the collator";

    dirty = false;
}

void QCollatorPrivate::cleanup()
{
    if (collator)
        UCDisposeCollator(&collator);
    collator = 0;
}

int QCollator::compare(const QChar *s1, int len1, const QChar *s2, int len2) const
{
    if (d->dirty)
        d->init();

    SInt32 result;
    Boolean equivalent;
    UCCompareText(d->collator,
                  reinterpret_cast<const UniChar *>(s1), len1,
                  reinterpret_cast<const UniChar *>(s2), len2,
                  &equivalent,
                  &result);
    if (equivalent)
        return 0;
    return result < 0 ? -1 : 1;
}
int QCollator::compare(const QString &str1, const QString &str2) const
{
    return compare(str1.constData(), str1.size(), str2.constData(), str2.size());
}

int QCollator::compare(const QStringRef &s1, const QStringRef &s2) const
{
    return compare(s1.constData(), s1.size(), s2.constData(), s2.size());
}

QCollatorSortKey QCollator::sortKey(const QString &string) const
{
    if (d->dirty)
        d->init();

    //Documentation recommends having it 5 times as big as the input
    QVector<UCCollationValue> ret(string.size() * 5);
    ItemCount actualSize;
    int status = UCGetCollationKey(d->collator, reinterpret_cast<const UniChar *>(string.constData()), string.count(),
                                   ret.size(), &actualSize, ret.data());

    ret.resize(actualSize+1);
    if (status == kUCOutputBufferTooSmall) {
        UCGetCollationKey(d->collator, reinterpret_cast<const UniChar *>(string.constData()), string.count(),
                          ret.size(), &actualSize, ret.data());
    }
    ret[actualSize] = 0;
    return QCollatorSortKey(new QCollatorSortKeyPrivate(ret));
}

int QCollatorSortKey::compare(const QCollatorSortKey &key) const
{
    SInt32 order;
    UCCompareCollationKeys(d->m_key.data(), d->m_key.size(),
                           key.d->m_key.data(), key.d->m_key.size(),
                           0, &order);
    return order;
}

QT_END_NAMESPACE
