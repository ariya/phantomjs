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

#include <QDebug>

#include <qt_windows.h>
#include <qsysinfo.h>

QT_BEGIN_NAMESPACE

void QCollatorPrivate::init()
{
    collator = 0;
}

void QCollatorPrivate::cleanup()
{
}

void QCollator::setCaseSensitivity(Qt::CaseSensitivity cs)
{
    detach();

    if (cs == Qt::CaseSensitive)
        d->collator &= ~NORM_IGNORECASE;
    else
        d->collator |= NORM_IGNORECASE;
}

Qt::CaseSensitivity QCollator::caseSensitivity() const
{
    return d->collator & NORM_IGNORECASE ? Qt::CaseInsensitive : Qt::CaseSensitive;
}

//NOTE: SORT_DIGITSASNUMBERS is available since win7
#ifndef SORT_DIGITSASNUMBERS
#define SORT_DIGITSASNUMBERS 8
#endif
void QCollator::setNumericMode(bool on)
{
    if (QSysInfo::windowsVersion() >= QSysInfo::WV_WINDOWS7) {
        detach();

        if (on)
            d->collator |= SORT_DIGITSASNUMBERS;
        else
            d->collator &= ~SORT_DIGITSASNUMBERS;
    } else {
        Q_UNUSED(on);
        qWarning() << "unsupported in the win collation implementation";
    }
}

bool QCollator::numericMode() const
{
    if (QSysInfo::windowsVersion() >= QSysInfo::WV_WINDOWS7) {
        return bool(d->collator & SORT_DIGITSASNUMBERS);
    } else {
        qWarning() << "unsupported in the win collation implementation";
        return false;
    }
}

void QCollator::setIgnorePunctuation(bool on)
{
    detach();

    if (on)
        d->collator |= NORM_IGNORESYMBOLS;
    else
        d->collator &= ~NORM_IGNORESYMBOLS;
}

bool QCollator::ignorePunctuation() const
{
    return bool(d->collator & NORM_IGNORESYMBOLS);
}

int QCollator::compare(const QChar *s1, int len1, const QChar *s2, int len2) const
{
    //* from Windows documentation *
    // Returns one of the following values if successful. To maintain the C runtime convention of
    // comparing strings, the value 2 can be subtracted from a nonzero return value. Then, the
    // meaning of <0, ==0, and >0 is consistent with the C runtime.

#ifndef Q_OS_WINRT
    return CompareString(LOCALE_USER_DEFAULT, d->collator,
                         reinterpret_cast<const wchar_t*>(s1), len1,
                         reinterpret_cast<const wchar_t*>(s2), len2) - 2;
#else // !Q_OS_WINRT
    return CompareStringEx(LOCALE_NAME_USER_DEFAULT, d->collator,
                           reinterpret_cast<LPCWSTR>(s1), len1,
                           reinterpret_cast<LPCWSTR>(s2), len2, NULL, NULL, 0) - 2;
#endif // Q_OS_WINRT
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
#ifndef Q_OS_WINRT
    int size = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | d->collator,
                           reinterpret_cast<const wchar_t*>(string.constData()), string.size(),
                           0, 0);
#elif defined(Q_OS_WINPHONE)
    int size = 0;
    Q_UNIMPLEMENTED();
#else // Q_OS_WINPHONE
    int size = LCMapStringEx(LOCALE_NAME_USER_DEFAULT, LCMAP_SORTKEY | d->collator,
                           reinterpret_cast<LPCWSTR>(string.constData()), string.size(),
                           0, 0, NULL, NULL, 0);
#endif // !Q_OS_WINPHONE
    QString ret(size, Qt::Uninitialized);
#ifndef Q_OS_WINRT
    int finalSize = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | d->collator,
                           reinterpret_cast<const wchar_t*>(string.constData()), string.size(),
                           reinterpret_cast<wchar_t*>(ret.data()), ret.size());
#elif defined(Q_OS_WINPHONE)
    int finalSize = 0;
#else // Q_OS_WINPHONE
    int finalSize = LCMapStringEx(LOCALE_NAME_USER_DEFAULT, LCMAP_SORTKEY | d->collator,
                           reinterpret_cast<LPCWSTR>(string.constData()), string.size(),
                           reinterpret_cast<LPWSTR>(ret.data()), ret.size(),
                           NULL, NULL, 0);
#endif // !Q_OS_WINPHONE
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
