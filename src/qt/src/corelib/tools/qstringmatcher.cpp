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

#include "qstringmatcher.h"

QT_BEGIN_NAMESPACE

static void bm_init_skiptable(const ushort *uc, int len, uchar *skiptable, Qt::CaseSensitivity cs)
{
    int l = qMin(len, 255);
    memset(skiptable, l, 256*sizeof(uchar));
    uc += len - l;
    if (cs == Qt::CaseSensitive) {
        while (l--) {
            skiptable[*uc & 0xff] = l;
            uc++;
        }
    } else {
        const ushort *start = uc;
        while (l--) {
            skiptable[foldCase(uc, start) & 0xff] = l;
            uc++;
        }
    }
}

static inline int bm_find(const ushort *uc, uint l, int index, const ushort *puc, uint pl,
                          const uchar *skiptable, Qt::CaseSensitivity cs)
{
    if (pl == 0)
        return index > (int)l ? -1 : index;
    const uint pl_minus_one = pl - 1;

    register const ushort *current = uc + index + pl_minus_one;
    const ushort *end = uc + l;
    if (cs == Qt::CaseSensitive) {
        while (current < end) {
            uint skip = skiptable[*current & 0xff];
            if (!skip) {
                // possible match
                while (skip < pl) {
                    if (*(current - skip) != puc[pl_minus_one-skip])
                        break;
                    skip++;
                }
                if (skip > pl_minus_one) // we have a match
                    return (current - uc) - pl_minus_one;

                // in case we don't have a match we are a bit inefficient as we only skip by one
                // when we have the non matching char in the string.
                if (skiptable[*(current - skip) & 0xff] == pl)
                    skip = pl - skip;
                else
                    skip = 1;
            }
            if (current > end - skip)
                break;
            current += skip;
        }
    } else {
        while (current < end) {
            uint skip = skiptable[foldCase(current, uc) & 0xff];
            if (!skip) {
                // possible match
                while (skip < pl) {
                    if (foldCase(current - skip, uc) != foldCase(puc + pl_minus_one - skip, puc))
                        break;
                    skip++;
                }
                if (skip > pl_minus_one) // we have a match
                    return (current - uc) - pl_minus_one;
                // in case we don't have a match we are a bit inefficient as we only skip by one
                // when we have the non matching char in the string.
                if (skiptable[foldCase(current - skip, uc) & 0xff] == pl)
                    skip = pl - skip;
                else
                    skip = 1;
            }
            if (current > end - skip)
                break;
            current += skip;
        }
    }
    return -1; // not found
}

/*!
    \class QStringMatcher
    \brief The QStringMatcher class holds a sequence of characters that
    can be quickly matched in a Unicode string.

    \ingroup tools
    \ingroup string-processing

    This class is useful when you have a sequence of \l{QChar}s that
    you want to repeatedly match against some strings (perhaps in a
    loop), or when you want to search for the same sequence of
    characters multiple times in the same string. Using a matcher
    object and indexIn() is faster than matching a plain QString with
    QString::indexOf() if repeated matching takes place. This class
    offers no benefit if you are doing one-off string matches.

    Create the QStringMatcher with the QString you want to search
    for. Then call indexIn() on the QString that you want to search.

    \sa QString, QByteArrayMatcher, QRegExp
*/

/*!
    Constructs an empty string matcher that won't match anything.
    Call setPattern() to give it a pattern to match.
*/
QStringMatcher::QStringMatcher()
    : d_ptr(0), q_cs(Qt::CaseSensitive)
{
    qMemSet(q_data, 0, sizeof(q_data));
}

/*!
    Constructs a string matcher that will search for \a pattern, with
    case sensitivity \a cs.

    Call indexIn() to perform a search.
*/
QStringMatcher::QStringMatcher(const QString &pattern, Qt::CaseSensitivity cs)
    : d_ptr(0), q_pattern(pattern), q_cs(cs)
{
    p.uc = pattern.unicode();
    p.len = pattern.size();
    bm_init_skiptable((const ushort *)p.uc, p.len, p.q_skiptable, cs);
}

/*!
    \fn QStringMatcher::QStringMatcher(const QChar *uc, int length, Qt::CaseSensitivity cs)
    \since 4.5

    Constructs a string matcher that will search for the pattern referred to
    by \a uc with the given \a length and case sensitivity specified by \a cs.
*/
QStringMatcher::QStringMatcher(const QChar *uc, int len, Qt::CaseSensitivity cs)
    : d_ptr(0), q_cs(cs)
{
    p.uc = uc;
    p.len = len;
    bm_init_skiptable((const ushort *)p.uc, len, p.q_skiptable, cs);
}

/*!
    Copies the \a other string matcher to this string matcher.
*/
QStringMatcher::QStringMatcher(const QStringMatcher &other)
    : d_ptr(0)
{
    operator=(other);
}

/*!
    Destroys the string matcher.
*/
QStringMatcher::~QStringMatcher()
{
}

/*!
    Assigns the \a other string matcher to this string matcher.
*/
QStringMatcher &QStringMatcher::operator=(const QStringMatcher &other)
{
    if (this != &other) {
        q_pattern = other.q_pattern;
        q_cs = other.q_cs;
        memcpy(q_data, other.q_data, sizeof(q_data));
    }
    return *this;
}

/*!
    Sets the string that this string matcher will search for to \a
    pattern.

    \sa pattern(), setCaseSensitivity(), indexIn()
*/
void QStringMatcher::setPattern(const QString &pattern)
{
    q_pattern = pattern;
    p.uc = pattern.unicode();
    p.len = pattern.size();
    bm_init_skiptable((const ushort *)pattern.unicode(), pattern.size(), p.q_skiptable, q_cs);
}

/*!
    \fn QString QStringMatcher::pattern() const

    Returns the string pattern that this string matcher will search
    for.

    \sa setPattern()
*/

QString QStringMatcher::pattern() const
{
    if (!q_pattern.isEmpty())
        return q_pattern;
    return QString(p.uc, p.len);
}

/*!
    Sets the case sensitivity setting of this string matcher to \a
    cs.

    \sa caseSensitivity(), setPattern(), indexIn()
*/
void QStringMatcher::setCaseSensitivity(Qt::CaseSensitivity cs)
{
    if (cs == q_cs)
        return;
    bm_init_skiptable((const ushort *)q_pattern.unicode(), q_pattern.size(), p.q_skiptable, cs);
    q_cs = cs;
}

/*!
    Searches the string \a str from character position \a from
    (default 0, i.e. from the first character), for the string
    pattern() that was set in the constructor or in the most recent
    call to setPattern(). Returns the position where the pattern()
    matched in \a str, or -1 if no match was found.

    \sa setPattern(), setCaseSensitivity()
*/
int QStringMatcher::indexIn(const QString &str, int from) const
{
    if (from < 0)
        from = 0;
    return bm_find((const ushort *)str.unicode(), str.size(), from,
                   (const ushort *)p.uc, p.len,
                   p.q_skiptable, q_cs);
}

/*!
    \since 4.5

    Searches the string starting at \a str (of length \a length) from
    character position \a from (default 0, i.e. from the first
    character), for the string pattern() that was set in the
    constructor or in the most recent call to setPattern(). Returns
    the position where the pattern() matched in \a str, or -1 if no
    match was found.

    \sa setPattern(), setCaseSensitivity()
*/
int QStringMatcher::indexIn(const QChar *str, int length, int from) const
{
    if (from < 0)
        from = 0;
    return bm_find((const ushort *)str, length, from,
                   (const ushort *)p.uc, p.len,
                   p.q_skiptable, q_cs);
}

/*!
    \fn Qt::CaseSensitivity QStringMatcher::caseSensitivity() const

    Returns the case sensitivity setting for this string matcher.

    \sa setCaseSensitivity()
*/

/*!
    \internal
*/

int qFindStringBoyerMoore(
    const QChar *haystack, int haystackLen, int haystackOffset,
    const QChar *needle, int needleLen, Qt::CaseSensitivity cs)
{
    uchar skiptable[256];
    bm_init_skiptable((const ushort *)needle, needleLen, skiptable, cs);
    if (haystackOffset < 0)
        haystackOffset = 0;
    return bm_find((const ushort *)haystack, haystackLen, haystackOffset,
                   (const ushort *)needle, needleLen, skiptable, cs);
}

QT_END_NAMESPACE
