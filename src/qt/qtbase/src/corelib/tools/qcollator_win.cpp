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

#include <QDebug>

#include <qt_windows.h>
#include <qsysinfo.h>

QT_BEGIN_NAMESPACE

//NOTE: SORT_DIGITSASNUMBERS is available since win7
#ifndef SORT_DIGITSASNUMBERS
#define SORT_DIGITSASNUMBERS 8
#endif

// implemented in qlocale_win.cpp
extern LCID qt_inIsoNametoLCID(const char *name);

void QCollatorPrivate::init()
{
    collator = 0;

#ifndef USE_COMPARESTRINGEX
    localeID = qt_inIsoNametoLCID(locale.bcp47Name().toUtf8().constData());
#else
    localeName = locale.bcp47Name();
#endif

    if (caseSensitivity == Qt::CaseInsensitive)
        collator |= NORM_IGNORECASE;

    if (numericMode) {
        if (QSysInfo::windowsVersion() >= QSysInfo::WV_WINDOWS7)
            collator |= SORT_DIGITSASNUMBERS;
        else
            qWarning() << "Numeric sorting unsupported on Windows versions older than Windows 7.";
    }

    if (ignorePunctuation)
        collator |= NORM_IGNORESYMBOLS;

    dirty = false;
}

void QCollatorPrivate::cleanup()
{
}


int QCollator::compare(const QChar *s1, int len1, const QChar *s2, int len2) const
{
    if (d->dirty)
        d->init();

    //* from Windows documentation *
    // Returns one of the following values if successful. To maintain the C runtime convention of
    // comparing strings, the value 2 can be subtracted from a nonzero return value. Then, the
    // meaning of <0, ==0, and >0 is consistent with the C runtime.

#ifndef USE_COMPARESTRINGEX
    return CompareString(d->localeID, d->collator,
                         reinterpret_cast<const wchar_t*>(s1), len1,
                         reinterpret_cast<const wchar_t*>(s2), len2) - 2;
#else
    return CompareStringEx(LPCWSTR(d->localeName.utf16()), d->collator,
                           reinterpret_cast<LPCWSTR>(s1), len1,
                           reinterpret_cast<LPCWSTR>(s2), len2, NULL, NULL, 0) - 2;
#endif
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

#ifndef USE_COMPARESTRINGEX
    int size = LCMapStringW(d->localeID, LCMAP_SORTKEY | d->collator,
                           reinterpret_cast<const wchar_t*>(string.constData()), string.size(),
                           0, 0);
#else
    int size = LCMapStringEx(LPCWSTR(d->localeName.utf16()), LCMAP_SORTKEY | d->collator,
                           reinterpret_cast<LPCWSTR>(string.constData()), string.size(),
                           0, 0, NULL, NULL, 0);
#endif
    QString ret(size, Qt::Uninitialized);
#ifndef USE_COMPARESTRINGEX
    int finalSize = LCMapStringW(d->localeID, LCMAP_SORTKEY | d->collator,
                           reinterpret_cast<const wchar_t*>(string.constData()), string.size(),
                           reinterpret_cast<wchar_t*>(ret.data()), ret.size());
#else
    int finalSize = LCMapStringEx(LPCWSTR(d->localeName.utf16()), LCMAP_SORTKEY | d->collator,
                           reinterpret_cast<LPCWSTR>(string.constData()), string.size(),
                           reinterpret_cast<LPWSTR>(ret.data()), ret.size(),
                           NULL, NULL, 0);
#endif
    if (finalSize == 0) {
        qWarning() << "there were problems when generating the ::sortKey by LCMapStringW with error:" << GetLastError();
    }
    return QCollatorSortKey(new QCollatorSortKeyPrivate(ret));
}

int QCollatorSortKey::compare(const QCollatorSortKey &otherKey) const
{
    return d->m_key.compare(otherKey.d->m_key);
}

QT_END_NAMESPACE
