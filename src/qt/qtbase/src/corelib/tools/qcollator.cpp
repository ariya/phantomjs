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

#include "qdebug.h"

QT_BEGIN_NAMESPACE


/*!
    \class QCollator
    \inmodule QtCore
    \brief The QCollator class compares strings according to a localized collation algorithm.

    \since 5.2

    \reentrant
    \ingroup i18n
    \ingroup string-processing
    \ingroup shared

    QCollator is initialized with a QLocale and an optional collation strategy. It tries to
    initialize the collator with the specified values. The collator can then be used to compare
    and sort strings in a locale dependent fashion.

    A QCollator object can be used together with template based sorting algorithms such as std::sort
    to sort a list of QStrings.

    In addition to the locale and collation strategy, several optional flags can be set that influence
    the result of the collation.
*/

/*!
    Constructs a QCollator from \a locale. If \a locale is not specified
    QLocale::default() will be used.

    \sa setLocale()
 */
QCollator::QCollator(const QLocale &locale)
    : d(new QCollatorPrivate)
{
    d->locale = locale;
    d->init();
}

/*!
    Creates a copy of \a other.
 */
QCollator::QCollator(const QCollator &other)
    : d(other.d)
{
    d->ref.ref();
}

/*!
    Destroys the collator.
 */
QCollator::~QCollator()
{
    if (d && !d->ref.deref())
        delete d;
}

/*!
    Assigns \a other to this collator.
 */
QCollator &QCollator::operator=(const QCollator &other)
{
    if (this != &other) {
        if (d && !d->ref.deref())
            delete d;
        d = other.d;
        if (d) d->ref.ref();
    }
    return *this;
}

/*
    \fn void QCollator::QCollator(QCollator &&other)

    Move constructor. Moves from \a other into this collator.

    Note that a moved-from QCollator can only be destroyed or assigned
    to. The effect of calling other functions than the destructor or
    one of the assignment operators is undefined.
*/

/*
    \fn QCollator &QCollator::operator=(QCollator &&other)

    Move-assigns from \a other to this collator.

    Note that a moved-from QCollator can only be destroyed or assigned
    to. The effect of calling other functions than the destructor or
    one of the assignment operators is undefined.
*/

/*!
    \fn void QCollator::swap(QCollator &other)

    Swaps this collator with \a other. This function is very fast and
    never fails.
*/

/*!
    \internal
 */
void QCollator::detach()
{
    if (d->ref.load() != 1) {
        QCollatorPrivate *x = new QCollatorPrivate;
        x->ref.store(1);
        x->locale = d->locale;
        x->collator = 0;
        if (!d->ref.deref())
            delete d;
        d = x;
        d->init();
    }
}

/*!
    Sets the locale of the collator to \a locale.
 */
void QCollator::setLocale(const QLocale &locale)
{
    detach();
    d->clear();
    d->locale = locale;
    d->init();
}

/*!
    Returns the locale of the collator.
 */
QLocale QCollator::locale() const
{
    return d->locale;
}

/*!
    \fn void QCollator::setCaseSensitivity(Qt::CaseSensitivity sensitivity)

    Sets the case \a sensitivity of the collator.

    \sa caseSensitivity()
 */

/*!
    \fn Qt::CaseSensitivity QCollator::caseSensitivity() const

    Returns case sensitivity of the collator.

    \sa setCaseSensitivity()
 */

/*!
    \fn void QCollator::setNumericMode(bool on)

    Enables numeric sorting mode when \a on is set to true.

    This will enable proper sorting of numeric digits, so that e.g. 100 sorts after 99.

    By default this mode is off.

    \sa numericMode()
 */

/*!
    \fn bool QCollator::numericMode() const

    Returns \c true if numeric sorting is enabled, false otherwise.

    \sa setNumericMode()
 */

/*!
    \fn void QCollator::setIgnorePunctuation(bool on)

    If \a on is set to true, punctuation characters and symbols are ignored when determining sort order.

    The default is locale dependent.

    \sa ignorePunctuation()
 */

/*!
    \fn bool QCollator::ignorePunctuation() const

    Returns \c true if punctuation characters and symbols are ignored when determining sort order.

    \sa setIgnorePunctuation()
 */

/*!
    \fn int QCollator::compare(const QString &s1, const QString &s2) const

    Compares \a s1 with \a s2. Returns -1, 0 or 1 depending on whether \a s1 is
    smaller, equal or larger than \a s2.
 */

/*!
    \fn bool QCollator::operator()(const QString &s1, const QString &s2) const
    \internal
*/

/*!
    \fn int QCollator::compare(const QStringRef &s1, const QStringRef &s2) const
    \overload

    Compares \a s1 with \a s2. Returns -1, 0 or 1 depending on whether \a s1 is
    smaller, equal or larger than \a s2.
 */

/*!
    \fn int QCollator::compare(const QChar *s1, int len1, const QChar *s2, int len2) const
    \overload

    Compares \a s1 with \a s2. \a len1 and \a len2 specify the length of the
    QChar arrays pointer to by \a s1 and \a s2.

    Returns -1, 0 or 1 depending on whether \a s1 is smaller, equal or larger than \a s2.
 */

/*!
    \fn QCollatorSortKey QCollator::sortKey(const QString &string) const

    Returns a sortKey for \a string.

    Creating the sort key is usually somewhat slower, than using the compare()
    methods directly. But if the string is compared repeatedly (e.g. when sorting
    a whole list of strings), it's usually faster to create the sort keys for each
    string and then sort using the keys.
 */

/*!
    \class QCollatorSortKey
    \inmodule QtCore
    \brief The QCollatorSortKey class can be used to speed up string collation.

    \since 5.2

    The QCollatorSortKey class is always created by QCollator::sortKey()
    and is used for fast strings collation, for example when collating many strings.

    \reentrant
    \ingroup i18n
    \ingroup string-processing
    \ingroup shared

    \sa QCollator, QCollator::sortKey()
*/

/*!
    \internal
 */
QCollatorSortKey::QCollatorSortKey(QCollatorSortKeyPrivate *d)
    : d(d)
{
}

/*!
    Constructs a copy of the \a other collator key.
*/
QCollatorSortKey::QCollatorSortKey(const QCollatorSortKey &other)
    : d(other.d)
{
}

/*!
    Destroys the collator key.
 */
QCollatorSortKey::~QCollatorSortKey()
{
}

/*!
    Assigns \a other to this collator key.
 */
QCollatorSortKey& QCollatorSortKey::operator=(const QCollatorSortKey &other)
{
    if (this != &other) {
        d = other.d;
    }
    return *this;
}

/*!
    \fn bool operator<(const QCollatorSortKey &lhs, const QCollatorSortKey &rhs)
    \relates QCollatorSortKey

    According to the QCollator that created the keys, returns \c true if \a lhs
    should be sorted before \a rhs; otherwise returns \c false.

    \sa QCollatorSortKey::compare()
 */

/*!
    \fn int QCollatorSortKey::compare(const QCollatorSortKey &otherKey) const

    Compares the key to \a otherKey. Returns a negative value if the key
    is less than \a otherKey, 0 if the key is equal to \a otherKey or a
    positive value if the key is greater than \a otherKey.

    \sa operator<()
 */

QT_END_NAMESPACE
