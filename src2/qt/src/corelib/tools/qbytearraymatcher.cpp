/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "qbytearraymatcher.h"

#include <limits.h>

QT_BEGIN_NAMESPACE

static inline void bm_init_skiptable(const uchar *cc, int len, uchar *skiptable)
{
    int l = qMin(len, 255);
    memset(skiptable, l, 256*sizeof(uchar));
    cc += len - l;
    while (l--)
        skiptable[*cc++] = l;
}

static inline int bm_find(const uchar *cc, int l, int index, const uchar *puc, uint pl,
                          const uchar *skiptable)
{
    if (pl == 0)
        return index > l ? -1 : index;
    const uint pl_minus_one = pl - 1;

    register const uchar *current = cc + index + pl_minus_one;
    const uchar *end = cc + l;
    while (current < end) {
        uint skip = skiptable[*current];
        if (!skip) {
            // possible match
            while (skip < pl) {
                if (*(current - skip) != puc[pl_minus_one - skip])
                    break;
                skip++;
            }
            if (skip > pl_minus_one) // we have a match
                return (current - cc) - skip + 1;

            // in case we don't have a match we are a bit inefficient as we only skip by one
            // when we have the non matching char in the string.
            if (skiptable[*(current - skip)] == pl)
                skip = pl - skip;
            else
                skip = 1;
        }
        if (current > end - skip)
            break;
        current += skip;
    }
    return -1; // not found
}

/*! \class QByteArrayMatcher
    \brief The QByteArrayMatcher class holds a sequence of bytes that
    can be quickly matched in a byte array.

    \ingroup tools
    \ingroup string-processing

    This class is useful when you have a sequence of bytes that you
    want to repeatedly match against some byte arrays (perhaps in a
    loop), or when you want to search for the same sequence of bytes
    multiple times in the same byte array. Using a matcher object and
    indexIn() is faster than matching a plain QByteArray with
    QByteArray::indexOf() if repeated matching takes place. This
    class offers no benefit if you are doing one-off byte array
    matches.

    Create the QByteArrayMatcher with the QByteArray you want to
    search for. Then call indexIn() on the QByteArray that you want to
    search.

    \sa QByteArray, QStringMatcher
*/

/*!
    Constructs an empty byte array matcher that won't match anything.
    Call setPattern() to give it a pattern to match.
*/
QByteArrayMatcher::QByteArrayMatcher()
    : d(0)
{
    p.p = 0;
    p.l = 0;
    qMemSet(p.q_skiptable, 0, sizeof(p.q_skiptable));
}

/*!
  Constructs a byte array matcher from \a pattern. \a pattern
  has the given \a length. \a pattern must remain in scope, but
  the destructor does not delete \a pattern.
 */
QByteArrayMatcher::QByteArrayMatcher(const char *pattern, int length)
    : d(0)
{
    p.p = reinterpret_cast<const uchar *>(pattern);
    p.l = length;
    bm_init_skiptable(p.p, p.l, p.q_skiptable);
}

/*!
    Constructs a byte array matcher that will search for \a pattern.
    Call indexIn() to perform a search.
*/
QByteArrayMatcher::QByteArrayMatcher(const QByteArray &pattern)
    : d(0), q_pattern(pattern)
{
    p.p = reinterpret_cast<const uchar *>(pattern.constData());
    p.l = pattern.size();
    bm_init_skiptable(p.p, p.l, p.q_skiptable);
}

/*!
    Copies the \a other byte array matcher to this byte array matcher.
*/
QByteArrayMatcher::QByteArrayMatcher(const QByteArrayMatcher &other)
    : d(0)
{
    operator=(other);
}

/*!
    Destroys the byte array matcher.
*/
QByteArrayMatcher::~QByteArrayMatcher()
{
}

/*!
    Assigns the \a other byte array matcher to this byte array matcher.
*/
QByteArrayMatcher &QByteArrayMatcher::operator=(const QByteArrayMatcher &other)
{
    q_pattern = other.q_pattern;
    memcpy(&p, &other.p, sizeof(p));
    return *this;
}

/*!
    Sets the byte array that this byte array matcher will search for
    to \a pattern.

    \sa pattern(), indexIn()
*/
void QByteArrayMatcher::setPattern(const QByteArray &pattern)
{
    q_pattern = pattern;
    p.p = reinterpret_cast<const uchar *>(pattern.constData());
    p.l = pattern.size();
    bm_init_skiptable(p.p, p.l, p.q_skiptable);
}

/*!
    Searches the byte array \a ba, from byte position \a from (default
    0, i.e. from the first byte), for the byte array pattern() that
    was set in the constructor or in the most recent call to
    setPattern(). Returns the position where the pattern() matched in
    \a ba, or -1 if no match was found.
*/
int QByteArrayMatcher::indexIn(const QByteArray &ba, int from) const
{
    if (from < 0)
        from = 0;
    return bm_find(reinterpret_cast<const uchar *>(ba.constData()), ba.size(), from,
                   p.p, p.l, p.q_skiptable);
}

/*!
    Searches the char string \a str, which has length \a len, from
    byte position \a from (default 0, i.e. from the first byte), for
    the byte array pattern() that was set in the constructor or in the
    most recent call to setPattern(). Returns the position where the
    pattern() matched in \a str, or -1 if no match was found.
*/
int QByteArrayMatcher::indexIn(const char *str, int len, int from) const
{
    if (from < 0)
        from = 0;
    return bm_find(reinterpret_cast<const uchar *>(str), len, from,
                   p.p, p.l, p.q_skiptable);
}

/*!
    \fn QByteArray QByteArrayMatcher::pattern() const

    Returns the byte array pattern that this byte array matcher will
    search for.

    \sa setPattern()
*/


static int findChar(const char *str, int len, char ch, int from)
{
    const uchar *s = (const uchar *)str;
    uchar c = (uchar)ch;
    if (from < 0)
        from = qMax(from + len, 0);
    if (from < len) {
        const uchar *n = s + from - 1;
        const uchar *e = s + len;
        while (++n != e)
            if (*n == c)
                return  n - s;
    }
    return -1;
}

/*! \internal
 */
static int qFindByteArrayBoyerMoore(
    const char *haystack, int haystackLen, int haystackOffset,
    const char *needle, int needleLen)
{
    uchar skiptable[256];
    bm_init_skiptable((const uchar *)needle, needleLen, skiptable);
    if (haystackOffset < 0)
        haystackOffset = 0;
    return bm_find((const uchar *)haystack, haystackLen, haystackOffset,
                   (const uchar *)needle, needleLen, skiptable);
}

#define REHASH(a) \
    if (sl_minus_1 < sizeof(uint) * CHAR_BIT) \
        hashHaystack -= (a) << sl_minus_1; \
    hashHaystack <<= 1

/*! \internal
 */
int qFindByteArray(
    const char *haystack0, int haystackLen, int from,
    const char *needle, int needleLen)
{
    const int l = haystackLen;
    const int sl = needleLen;
    if (from < 0)
        from += l;
    if (uint(sl + from) > (uint)l)
        return -1;
    if (!sl)
        return from;
    if (!l)
        return -1;

    if (sl == 1)
        return findChar(haystack0, haystackLen, needle[0], from);

    /*
      We use the Boyer-Moore algorithm in cases where the overhead
      for the skip table should pay off, otherwise we use a simple
      hash function.
    */
    if (l > 500 && sl > 5)
        return qFindByteArrayBoyerMoore(haystack0, haystackLen, from,
                                        needle, needleLen);

    /*
      We use some hashing for efficiency's sake. Instead of
      comparing strings, we compare the hash value of str with that
      of a part of this QString. Only if that matches, we call memcmp().
    */
    const char *haystack = haystack0 + from;
    const char *end = haystack0 + (l - sl);
    const uint sl_minus_1 = sl - 1;
    uint hashNeedle = 0, hashHaystack = 0;
    int idx;
    for (idx = 0; idx < sl; ++idx) {
        hashNeedle = ((hashNeedle<<1) + needle[idx]);
        hashHaystack = ((hashHaystack<<1) + haystack[idx]);
    }
    hashHaystack -= *(haystack + sl_minus_1);

    while (haystack <= end) {
        hashHaystack += *(haystack + sl_minus_1);
        if (hashHaystack == hashNeedle && *needle == *haystack
             && memcmp(needle, haystack, sl) == 0)
            return haystack - haystack0;

        REHASH(*haystack);
        ++haystack;
    }
    return -1;
}

QT_END_NAMESPACE
