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

#include <qstringlist.h>
#include <qset.h>

QT_BEGIN_NAMESPACE

/*! \typedef QStringListIterator
    \relates QStringList

    The QStringListIterator type definition provides a Java-style const
    iterator for QStringList.

    QStringList provides both \l{Java-style iterators} and
    \l{STL-style iterators}. The Java-style const iterator is simply
    a type definition for QListIterator<QString>.

    \sa QMutableStringListIterator, QStringList::const_iterator
*/

/*! \typedef QMutableStringListIterator
    \relates QStringList

    The QStringListIterator type definition provides a Java-style
    non-const iterator for QStringList.

    QStringList provides both \l{Java-style iterators} and
    \l{STL-style iterators}. The Java-style non-const iterator is
    simply a type definition for QMutableListIterator<QString>.

    \sa QStringListIterator, QStringList::iterator
*/

/*!
    \class QStringList
    \brief The QStringList class provides a list of strings.

    \ingroup tools
    \ingroup shared
    \ingroup string-processing

    \reentrant

    QStringList inherits from QList<QString>. Like QList, QStringList is
    \l{implicitly shared}. It provides fast index-based access as well as fast
    insertions and removals. Passing string lists as value parameters is both
    fast and safe.

    All of QList's functionality also applies to QStringList. For example, you
    can use isEmpty() to test whether the list is empty, and you can call
    functions like append(), prepend(), insert(), replace(), removeAll(),
    removeAt(), removeFirst(), removeLast(), and removeOne() to modify a
    QStringList. In addition, QStringList provides a few convenience
    functions that make handling lists of strings easier:

    \tableofcontents

    \section1 Adding strings

    Strings can be added to a list using the \l
    {QList::append()}{append()}, \l
    {QList::operator+=()}{operator+=()} and \l
    {QStringList::operator<<()}{operator<<()} functions. For example:

    \snippet doc/src/snippets/qstringlist/main.cpp 0

    \section1 Iterating over the strings

    To iterate over a list, you can either use index positions or
    QList's Java-style and STL-style iterator types:

    Indexing:

    \snippet doc/src/snippets/qstringlist/main.cpp 1

    Java-style iterator:

    \snippet doc/src/snippets/qstringlist/main.cpp 2

    STL-style iterator:

    \snippet doc/src/snippets/qstringlist/main.cpp 3

    The QStringListIterator class is simply a type definition for
    QListIterator<QString>. QStringList also provide the
    QMutableStringListIterator class which is a type definition for
    QMutableListIterator<QString>.

    \section1 Manipulating the strings

    QStringList provides several functions allowing you to manipulate
    the contents of a list. You can concatenate all the strings in a
    string list into a single string (with an optional separator)
    using the join() function. For example:

    \snippet doc/src/snippets/qstringlist/main.cpp 4

    To break up a string into a string list, use the QString::split()
    function:

    \snippet doc/src/snippets/qstringlist/main.cpp 6

    The argument to split can be a single character, a string, or a
    QRegExp.

    In addition, the \l {QStringList::operator+()}{operator+()}
    function allows you to concatenate two string lists into one. To
    sort a string list, use the sort() function.

    QString list also provides the filter() function which lets you
    to extract a new list which contains only those strings which
    contain a particular substring (or match a particular regular
    expression):

    \snippet doc/src/snippets/qstringlist/main.cpp 7

    The contains() function tells you whether the list contains a
    given string, while the indexOf() function returns the index of
    the first occurrence of the given string. The lastIndexOf()
    function on the other hand, returns the index of the last
    occurrence of the string.

    Finally, the replaceInStrings() function calls QString::replace()
    on each string in the string list in turn. For example:

    \snippet doc/src/snippets/qstringlist/main.cpp 8

    \sa QString
*/

/*!
    \fn QStringList::QStringList()

    Constructs an empty string list.
*/

/*!
    \fn QStringList::QStringList(const QString &str)

    Constructs a string list that contains the given string, \a
    str. Longer lists are easily created like this:

    \snippet doc/src/snippets/qstringlist/main.cpp 9

    \sa append()
*/

/*!
    \fn QStringList::QStringList(const QStringList &other)

    Constructs a copy of the \a other string list.

    This operation takes \l{constant time} because QStringList is
    \l{implicitly shared}, making the process of returning a
    QStringList from a function very fast. If a shared instance is
    modified, it will be copied (copy-on-write), and that takes
    \l{linear time}.

    \sa operator=()
*/

/*!
    \fn QStringList::QStringList(const QList<QString> &other)

    Constructs a copy of \a other.

    This operation takes \l{constant time}, because QStringList is
    \l{implicitly shared}. This makes returning a QStringList from a
    function very fast. If a shared instance is modified, it will be
    copied (copy-on-write), and that takes \l{linear time}.

    \sa operator=()
*/

/*!
    \fn void QStringList::sort()

    Sorts the list of strings in ascending order (case sensitively).

    Sorting is performed using Qt's qSort() algorithm,
    which operates in \l{linear-logarithmic time}, i.e. O(\e{n} log \e{n}).

    If you want to sort your strings in an arbitrary order, consider
    using the QMap class. For example, you could use a QMap<QString,
    QString> to create a case-insensitive ordering (e.g. with the keys
    being lower-case versions of the strings, and the values being the
    strings), or a QMap<int, QString> to sort the strings by some
    integer index.

    \sa qSort()
*/
void QtPrivate::QStringList_sort(QStringList *that)
{
    qSort(*that);
}


#ifdef QT3_SUPPORT
/*!
    \fn QStringList QStringList::split(const QChar &sep, const QString &str, bool allowEmptyEntries)

    \overload

    This version of the function uses a QChar as separator.

    \sa join() QString::section()
*/

/*!
    \fn QStringList QStringList::split(const QString &sep, const QString &str, bool allowEmptyEntries)

    \overload

    This version of the function uses a QString as separator.

    \sa join() QString::section()
*/
#ifndef QT_NO_REGEXP
/*!
    \fn QStringList QStringList::split(const QRegExp &sep, const QString &str, bool allowEmptyEntries)

    Use QString::split(\a sep, QString::SkipEmptyParts) or
    QString::split(\a sep, QString::KeepEmptyParts) instead.

    Be aware that the QString::split()'s return value is a
    QStringList that always contains at least one element, even if \a
    str is empty.

    \sa join() QString::section()
*/
#endif
#endif // QT3_SUPPORT

/*!
    \fn QStringList QStringList::filter(const QString &str, Qt::CaseSensitivity cs) const

    Returns a list of all the strings containing the substring \a str.

    If \a cs is \l Qt::CaseSensitive (the default), the string
    comparison is case sensitive; otherwise the comparison is case
    insensitive.

    \snippet doc/src/snippets/qstringlist/main.cpp 5
    \snippet doc/src/snippets/qstringlist/main.cpp 10

    This is equivalent to

    \snippet doc/src/snippets/qstringlist/main.cpp 11
    \snippet doc/src/snippets/qstringlist/main.cpp 12

    \sa contains()
*/
QStringList QtPrivate::QStringList_filter(const QStringList *that, const QString &str,
                                          Qt::CaseSensitivity cs)
{
    QStringMatcher matcher(str, cs);
    QStringList res;
    for (int i = 0; i < that->size(); ++i)
        if (matcher.indexIn(that->at(i)) != -1)
            res << that->at(i);
    return res;
}


/*!
    \fn QBool QStringList::contains(const QString &str, Qt::CaseSensitivity cs) const

    Returns true if the list contains the string \a str; otherwise
    returns false. The search is case insensitive if \a cs is
    Qt::CaseInsensitive; the search is case sensitive by default.

    \sa indexOf(), lastIndexOf(), QString::contains()
 */
QBool QtPrivate::QStringList_contains(const QStringList *that, const QString &str,
                                      Qt::CaseSensitivity cs)
{
    for (int i = 0; i < that->size(); ++i) {
        const QString & string = that->at(i);
        if (string.length() == str.length() && str.compare(string, cs) == 0)
            return QBool(true);
    }
    return QBool(false);
}

#ifndef QT_NO_REGEXP
/*!
    \fn QStringList QStringList::filter(const QRegExp &rx) const

    \overload

    Returns a list of all the strings that match the regular
    expression \a rx.
*/
QStringList QtPrivate::QStringList_filter(const QStringList *that, const QRegExp &rx)
{
    QStringList res;
    for (int i = 0; i < that->size(); ++i)
        if (that->at(i).contains(rx))
            res << that->at(i);
    return res;
}
#endif

/*!
    \fn QStringList &QStringList::replaceInStrings(const QString &before, const QString &after, Qt::CaseSensitivity cs)

    Returns a string list where every string has had the \a before
    text replaced with the \a after text wherever the \a before text
    is found. The \a before text is matched case-sensitively or not
    depending on the \a cs flag.

    For example:

    \snippet doc/src/snippets/qstringlist/main.cpp 5
    \snippet doc/src/snippets/qstringlist/main.cpp 13

    \sa QString::replace()
*/
void QtPrivate::QStringList_replaceInStrings(QStringList *that, const QString &before,
                                             const QString &after, Qt::CaseSensitivity cs)
{
    for (int i = 0; i < that->size(); ++i)
        (*that)[i].replace(before, after, cs);
}


#ifndef QT_NO_REGEXP
/*!
    \fn QStringList &QStringList::replaceInStrings(const QRegExp &rx, const QString &after)
    \overload

    Replaces every occurrence of the regexp \a rx, in each of the
    string lists's strings, with \a after. Returns a reference to the
    string list.

    For example:

    \snippet doc/src/snippets/qstringlist/main.cpp 5
    \snippet doc/src/snippets/qstringlist/main.cpp 14

    For regular expressions that contain \l{capturing parentheses},
    occurrences of \bold{\\1}, \bold{\\2}, ..., in \a after are
    replaced with \a{rx}.cap(1), \a{rx}.cap(2), ...

    For example:

    \snippet doc/src/snippets/qstringlist/main.cpp 5
    \snippet doc/src/snippets/qstringlist/main.cpp 15
*/
void QtPrivate::QStringList_replaceInStrings(QStringList *that, const QRegExp &rx, const QString &after)
{
    for (int i = 0; i < that->size(); ++i)
        (*that)[i].replace(rx, after);
}
#endif

/*!
    \fn QString QStringList::join(const QString &separator) const

    Joins all the string list's strings into a single string with each
    element separated by the given \a separator (which can be an
    empty string).

    \sa QString::split()
*/
QString QtPrivate::QStringList_join(const QStringList *that, const QString &sep)
{
    int totalLength = 0;
    const int size = that->size();

    for (int i = 0; i < size; ++i)
        totalLength += that->at(i).size();

    if(size > 0)
        totalLength += sep.size() * (size - 1);

    QString res;
    if (totalLength == 0)
	return res;
    res.reserve(totalLength);
    for (int i = 0; i < that->size(); ++i) {
        if (i)
            res += sep;
        res += that->at(i);
    }
    return res;
}

/*!
    \fn QStringList QStringList::operator+(const QStringList &other) const

    Returns a string list that is the concatenation of this string
    list with the \a other string list.

    \sa append()
*/

/*!
    \fn QStringList &QStringList::operator<<(const QString &str)

    Appends the given string, \a str, to this string list and returns
    a reference to the string list.

    \sa append()
*/

/*!
    \fn QStringList &QStringList::operator<<(const QStringList &other)

    \overload

    Appends the \a other string list to the string list and returns a reference to
    the latter string list.
*/

#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator>>(QDataStream &in, QStringList &list)
    \relates QStringList

    Reads a string list from the given \a in stream into the specified
    \a list.

    \sa {Serializing Qt Data Types}
*/

/*!
    \fn QDataStream &operator<<(QDataStream &out, const QStringList &list)
    \relates QStringList

    Writes the given string \a list to the specified \a out stream.

    \sa {Serializing Qt Data Types}
*/
#endif // QT_NO_DATASTREAM

/*!
    \fn QStringList QStringList::grep(const QString &str, bool cs = true) const

    Use filter() instead.
*/

/*!
    \fn QStringList QStringList::grep(const QRegExp &rx) const

    Use filter() instead.
*/

/*!
    \fn QStringList &QStringList::gres(const QString &before, const QString &after, bool cs = true)

    Use replaceInStrings() instead.
*/

/*!
    \fn QStringList &QStringList::gres(const QRegExp &rx, const QString &after)

    Use replaceInStrings() instead.
*/

/*!
    \fn Iterator QStringList::fromLast()

    Use end() instead.

    \oldcode
    QStringList::Iterator i = list.fromLast();
    \newcode
    QStringList::Iterator i = list.isEmpty() ? list.end() : --list.end();
    \endcode
*/

/*!
    \fn ConstIterator QStringList::fromLast() const

    Use end() instead.

    \oldcode
    QStringList::ConstIterator i = list.fromLast();
    \newcode
    QStringList::ConstIterator i = list.isEmpty() ? list.end() : --list.end();
    \endcode
*/


#ifndef QT_NO_REGEXP
static int indexOfMutating(const QStringList *that, QRegExp &rx, int from)
{
    if (from < 0)
        from = qMax(from + that->size(), 0);
    for (int i = from; i < that->size(); ++i) {
        if (rx.exactMatch(that->at(i)))
            return i;
    }
    return -1;
}

static int lastIndexOfMutating(const QStringList *that, QRegExp &rx, int from)
{
    if (from < 0)
        from += that->size();
    else if (from >= that->size())
        from = that->size() - 1;
    for (int i = from; i >= 0; --i) {
        if (rx.exactMatch(that->at(i)))
            return i;
        }
    return -1;
}

/*!
    \fn int QStringList::indexOf(const QRegExp &rx, int from) const

    Returns the index position of the first exact match of \a rx in
    the list, searching forward from index position \a from. Returns
    -1 if no item matched.

    By default, this function is case sensitive.

    \sa lastIndexOf(), contains(), QRegExp::exactMatch()
*/
int QtPrivate::QStringList_indexOf(const QStringList *that, const QRegExp &rx, int from)
{
    QRegExp rx2(rx);
    return indexOfMutating(that, rx2, from);
}

/*!
    \fn int QStringList::indexOf(QRegExp &rx, int from) const
    \overload indexOf()
    \since 4.5

    Returns the index position of the first exact match of \a rx in
    the list, searching forward from index position \a from. Returns
    -1 if no item matched.

    By default, this function is case sensitive.

    If an item matched, the \a rx regular expression will contain the
    matched objects (see QRegExp::matchedLength, QRegExp::cap).

    \sa lastIndexOf(), contains(), QRegExp::exactMatch()
*/
int QtPrivate::QStringList_indexOf(const QStringList *that, QRegExp &rx, int from)
{
    return indexOfMutating(that, rx, from);
}

/*!
    \fn int QStringList::lastIndexOf(const QRegExp &rx, int from) const

    Returns the index position of the last exact match of \a rx in
    the list, searching backward from index position \a from. If \a
    from is -1 (the default), the search starts at the last item.
    Returns -1 if no item matched.

    By default, this function is case sensitive.

    \sa indexOf(), contains(), QRegExp::exactMatch()
*/
int QtPrivate::QStringList_lastIndexOf(const QStringList *that, const QRegExp &rx, int from)
{
    QRegExp rx2(rx);
    return lastIndexOfMutating(that, rx2, from);
}

/*!
    \fn int QStringList::lastIndexOf(QRegExp &rx, int from) const
    \overload lastIndexOf()
    \since 4.5

    Returns the index position of the last exact match of \a rx in
    the list, searching backward from index position \a from. If \a
    from is -1 (the default), the search starts at the last item.
    Returns -1 if no item matched.

    By default, this function is case sensitive.

    If an item matched, the \a rx regular expression will contain the
    matched objects (see QRegExp::matchedLength, QRegExp::cap).

    \sa indexOf(), contains(), QRegExp::exactMatch()
*/
int QtPrivate::QStringList_lastIndexOf(const QStringList *that, QRegExp &rx, int from)
{
    return lastIndexOfMutating(that, rx, from);
}
#endif

/*!
    \fn int QStringList::indexOf(const QString &value, int from = 0) const

    Returns the index position of the first occurrence of \a value in
    the list, searching forward from index position \a from. Returns
    -1 if no item matched.

    \sa lastIndexOf(), contains(), QList::indexOf()
*/

/*!
    \fn int QStringList::lastIndexOf(const QString &value, int from = -1) const

    Returns the index position of the last occurrence of \a value in
    the list, searching backward from index position \a from. If \a
    from is -1 (the default), the search starts at the last item.
    Returns -1 if no item matched.

    By default, this function is case sensitive.

    \sa indexOf(), QList::lastIndexOf()
*/

/*!
    \fn int QStringList::removeDuplicates()

    \since  4.5

    This function removes duplicate entries from a list.
    The entries do not have to be sorted. They will retain their
    original order.

    Returns the number of removed entries.
*/
int QtPrivate::QStringList_removeDuplicates(QStringList *that)
{
    int n = that->size();
    int j = 0;
    QSet<QString> seen;
    seen.reserve(n);
    for (int i = 0; i < n; ++i) {
        const QString &s = that->at(i);
        if (seen.contains(s))
            continue;
        seen.insert(s);
        if (j != i)
            (*that)[j] = s;
        ++j;
    }
    if (n != j)
        that->erase(that->begin() + j, that->end());
    return n - j;
}

/*! \fn QStringList::QStringList(std::initializer_list<QString> args)
    \since 4.8

    Construct a list from a std::initializer_list given by \a args.

    This constructor is only enabled if the compiler supports C++0x
*/


QT_END_NAMESPACE
