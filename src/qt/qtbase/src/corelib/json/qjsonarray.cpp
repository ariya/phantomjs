/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qjsonarray.h>
#include <qstringlist.h>
#include <qvariant.h>
#include <qdebug.h>

#include "qjsonwriter_p.h"
#include "qjson_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QJsonArray
    \inmodule QtCore
    \ingroup json
    \reentrant
    \since 5.0

    \brief The QJsonArray class encapsulates a JSON array.

    A JSON array is a list of values. The list can be manipulated by inserting and
    removing QJsonValue's from the array.

    A QJsonArray can be converted to and from a QVariantList. You can query the
    number of entries with size(), insert(), and remove() entries from it
    and iterate over its content using the standard C++ iterator pattern.

    QJsonArray is an implicitly shared class and shares the data with the document
    it has been created from as long as it is not being modified.

    You can convert the array to and from text based JSON through QJsonDocument.

    \sa {JSON Support in Qt}, {JSON Save Game Example}
*/

/*!
    \typedef QJsonArray::Iterator

    Qt-style synonym for QJsonArray::iterator.
*/

/*!
    \typedef QJsonArray::ConstIterator

    Qt-style synonym for QJsonArray::const_iterator.
*/

/*!
    \typedef QJsonArray::size_type

    Typedef for int. Provided for STL compatibility.
*/

/*!
    \typedef QJsonArray::value_type

    Typedef for QJsonValue. Provided for STL compatibility.
*/

/*!
    \typedef QJsonArray::difference_type

    Typedef for int. Provided for STL compatibility.
*/

/*!
    \typedef QJsonArray::pointer

    Typedef for QJsonValue *. Provided for STL compatibility.
*/

/*!
    \typedef QJsonArray::const_pointer

    Typedef for const QJsonValue *. Provided for STL compatibility.
*/

/*!
    \typedef QJsonArray::reference

    Typedef for QJsonValue &. Provided for STL compatibility.
*/

/*!
    \typedef QJsonArray::const_reference

    Typedef for const QJsonValue &. Provided for STL compatibility.
*/

/*!
    Creates an empty array.
 */
QJsonArray::QJsonArray()
    : d(0), a(0)
{
}

/*!
    \internal
 */
QJsonArray::QJsonArray(QJsonPrivate::Data *data, QJsonPrivate::Array *array)
    : d(data), a(array)
{
    Q_ASSERT(data);
    Q_ASSERT(array);
    d->ref.ref();
}

/*!
    Deletes the array.
 */
QJsonArray::~QJsonArray()
{
    if (d && !d->ref.deref())
        delete d;
}

/*!
    Creates a copy of \a other.

    Since QJsonArray is implicitly shared, the copy is shallow
    as long as the object doesn't get modified.
 */
QJsonArray::QJsonArray(const QJsonArray &other)
{
    d = other.d;
    a = other.a;
    if (d)
        d->ref.ref();
}

/*!
    Assigns \a other to this array.
 */
QJsonArray &QJsonArray::operator =(const QJsonArray &other)
{
    if (d != other.d) {
        if (d && !d->ref.deref())
            delete d;
        d = other.d;
        if (d)
            d->ref.ref();
    }
    a = other.a;

    return *this;
}

/*! \fn QJsonArray &QJsonArray::operator+=(const QJsonValue &value)

    Appends \a value to the array, and returns a reference to the array itself.

    \since 5.3
    \sa append(), operator<<()
*/

/*! \fn QJsonArray QJsonArray::operator+(const QJsonValue &value) const

    Returns an array that contains all the items in this array followed
    by the provided \a value.

    \since 5.3
    \sa operator+=()
*/

/*! \fn QJsonArray &QJsonArray::operator<<(const QJsonValue &value)

    Appends \a value to the array, and returns a reference to the array itself.

    \since 5.3
    \sa operator+=(), append()
*/

/*!
    Converts the string list \a list to a QJsonArray.

    The values in \a list will be converted to JSON values.

    \sa toVariantList(), QJsonValue::fromVariant()
 */
QJsonArray QJsonArray::fromStringList(const QStringList &list)
{
    QJsonArray array;
    for (QStringList::const_iterator it = list.constBegin(); it != list.constEnd(); ++it)
        array.append(QJsonValue(*it));
    return array;
}

/*!
    Converts the variant list \a list to a QJsonArray.

    The QVariant values in \a list will be converted to JSON values.

    \sa toVariantList(), QJsonValue::fromVariant()
 */
QJsonArray QJsonArray::fromVariantList(const QVariantList &list)
{
    QJsonArray array;
    for (QVariantList::const_iterator it = list.constBegin(); it != list.constEnd(); ++it)
        array.append(QJsonValue::fromVariant(*it));
    return array;
}

/*!
    Converts this object to a QVariantList.

    Returns the created map.
 */
QVariantList QJsonArray::toVariantList() const
{
    QVariantList list;

    if (a) {
        for (int i = 0; i < (int)a->length; ++i)
            list.append(QJsonValue(d, a, a->at(i)).toVariant());
    }
    return list;
}


/*!
    Returns the number of values stored in the array.
 */
int QJsonArray::size() const
{
    if (!d)
        return 0;

    return (int)a->length;
}

/*!
    \fn QJsonArray::count() const

    Same as size().

    \sa size()
*/

/*!
    Returns \c true if the object is empty. This is the same as size() == 0.

    \sa size()
 */
bool QJsonArray::isEmpty() const
{
    if (!d)
        return true;

    return !a->length;
}

/*!
    Returns a QJsonValue representing the value for index \a i.

    The returned QJsonValue is \c Undefined, if \a i is out of bounds.

 */
QJsonValue QJsonArray::at(int i) const
{
    if (!a || i < 0 || i >= (int)a->length)
        return QJsonValue(QJsonValue::Undefined);

    return QJsonValue(d, a, a->at(i));
}

/*!
    Returns the first value stored in the array.

    Same as \c at(0).

    \sa at()
 */
QJsonValue QJsonArray::first() const
{
    return at(0);
}

/*!
    Returns the last value stored in the array.

    Same as \c{at(size() - 1)}.

    \sa at()
 */
QJsonValue QJsonArray::last() const
{
    return at(a ? (a->length - 1) : 0);
}

/*!
    Inserts \a value at the beginning of the array.

    This is the same as \c{insert(0, value)} and will prepend \a value to the array.

    \sa append(), insert()
 */
void QJsonArray::prepend(const QJsonValue &value)
{
    insert(0, value);
}

/*!
    Inserts \a value at the end of the array.

    \sa prepend(), insert()
 */
void QJsonArray::append(const QJsonValue &value)
{
    insert(a ? (int)a->length : 0, value);
}

/*!
    Removes the value at index position \a i. \a i must be a valid
    index position in the array (i.e., \c{0 <= i < size()}).

    \sa insert(), replace()
 */
void QJsonArray::removeAt(int i)
{
    if (!a || i < 0 || i >= (int)a->length)
        return;

    detach();
    a->removeItems(i, 1);
    ++d->compactionCounter;
    if (d->compactionCounter > 32u && d->compactionCounter >= unsigned(a->length) / 2u)
        compact();
}

/*! \fn void QJsonArray::removeFirst()

    Removes the first item in the array. Calling this function is
    equivalent to calling \c{removeAt(0)}. The array must not be empty. If
    the array can be empty, call isEmpty() before calling this
    function.

    \sa removeAt(), removeLast()
*/

/*! \fn void QJsonArray::removeLast()

    Removes the last item in the array. Calling this function is
    equivalent to calling \c{removeAt(size() - 1)}. The array must not be
    empty. If the array can be empty, call isEmpty() before calling
    this function.

    \sa removeAt(), removeFirst()
*/

/*!
    Removes the item at index position \a i and returns it. \a i must
    be a valid index position in the array (i.e., \c{0 <= i < size()}).

    If you don't use the return value, removeAt() is more efficient.

    \sa removeAt()
 */
QJsonValue QJsonArray::takeAt(int i)
{
    if (!a || i < 0 || i >= (int)a->length)
        return QJsonValue(QJsonValue::Undefined);

    QJsonValue v(d, a, a->at(i));
    removeAt(i); // detaches
    return v;
}

/*!
    Inserts \a value at index position \a i in the array. If \a i
    is \c 0, the value is prepended to the array. If \a i is size(), the
    value is appended to the array.

    \sa append(), prepend(), replace(), removeAt()
 */
void QJsonArray::insert(int i, const QJsonValue &value)
{
    Q_ASSERT (i >= 0 && i <= (a ? (int)a->length : 0));
    QJsonValue val = value;

    bool compressed;
    int valueSize = QJsonPrivate::Value::requiredStorage(val, &compressed);

    detach(valueSize + sizeof(QJsonPrivate::Value));

    if (!a->length)
        a->tableOffset = sizeof(QJsonPrivate::Array);

    int valueOffset = a->reserveSpace(valueSize, i, 1, false);
    if (!valueOffset)
        return;

    QJsonPrivate::Value &v = (*a)[i];
    v.type = (val.t == QJsonValue::Undefined ? QJsonValue::Null : val.t);
    v.latinOrIntValue = compressed;
    v.latinKey = false;
    v.value = QJsonPrivate::Value::valueToStore(val, valueOffset);
    if (valueSize)
        QJsonPrivate::Value::copyData(val, (char *)a + valueOffset, compressed);
}

/*!
    \fn QJsonArray::iterator QJsonArray::insert(iterator before, const QJsonValue &value)

    Inserts \a value before the position pointed to by \a before, and returns an iterator
    pointing to the newly inserted item.

    \sa erase(), insert()
*/

/*!
    \fn QJsonArray::iterator QJsonArray::erase(iterator it)

    Removes the item pointed to by \a it, and returns an iterator pointing to the
    next item.

    \sa removeAt()
*/

/*!
    Replaces the item at index position \a i with \a value. \a i must
    be a valid index position in the array (i.e., \c{0 <= i < size()}).

    \sa operator[](), removeAt()
 */
void QJsonArray::replace(int i, const QJsonValue &value)
{
    Q_ASSERT (a && i >= 0 && i < (int)(a->length));
    QJsonValue val = value;

    bool compressed;
    int valueSize = QJsonPrivate::Value::requiredStorage(val, &compressed);

    detach(valueSize);

    if (!a->length)
        a->tableOffset = sizeof(QJsonPrivate::Array);

    int valueOffset = a->reserveSpace(valueSize, i, 1, true);
    if (!valueOffset)
        return;

    QJsonPrivate::Value &v = (*a)[i];
    v.type = (val.t == QJsonValue::Undefined ? QJsonValue::Null : val.t);
    v.latinOrIntValue = compressed;
    v.latinKey = false;
    v.value = QJsonPrivate::Value::valueToStore(val, valueOffset);
    if (valueSize)
        QJsonPrivate::Value::copyData(val, (char *)a + valueOffset, compressed);

    ++d->compactionCounter;
    if (d->compactionCounter > 32u && d->compactionCounter >= unsigned(a->length) / 2u)
        compact();
}

/*!
    Returns \c true if the array contains an occurrence of \a value, otherwise \c false.

    \sa count()
 */
bool QJsonArray::contains(const QJsonValue &value) const
{
    for (int i = 0; i < size(); i++) {
        if (at(i) == value)
            return true;
    }
    return false;
}

/*!
    Returns the value at index position \a i as a modifiable reference.
    \a i must be a valid index position in the array (i.e., \c{0 <= i <
    size()}).

    The return value is of type QJsonValueRef, a helper class for QJsonArray
    and QJsonObject. When you get an object of type QJsonValueRef, you can
    use it as if it were a reference to a QJsonValue. If you assign to it,
    the assignment will apply to the character in the QJsonArray of QJsonObject
    from which you got the reference.

    \sa at()
 */
QJsonValueRef QJsonArray::operator [](int i)
{
    Q_ASSERT(a && i >= 0 && i < (int)a->length);
    return QJsonValueRef(this, i);
}

/*!
    \overload

    Same as at().
 */
QJsonValue QJsonArray::operator[](int i) const
{
    return at(i);
}

/*!
    Returns \c true if this array is equal to \a other.
 */
bool QJsonArray::operator==(const QJsonArray &other) const
{
    if (a == other.a)
        return true;

    if (!a)
        return !other.a->length;
    if (!other.a)
        return !a->length;
    if (a->length != other.a->length)
        return false;

    for (int i = 0; i < (int)a->length; ++i) {
        if (QJsonValue(d, a, a->at(i)) != QJsonValue(other.d, other.a, other.a->at(i)))
            return false;
    }
    return true;
}

/*!
    Returns \c true if this array is not equal to \a other.
 */
bool QJsonArray::operator!=(const QJsonArray &other) const
{
    return !(*this == other);
}

/*! \fn QJsonArray::iterator QJsonArray::begin()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the first item in
    the array.

    \sa constBegin(), end()
*/

/*! \fn QJsonArray::const_iterator QJsonArray::begin() const

    \overload
*/

/*! \fn QJsonArray::const_iterator QJsonArray::constBegin() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first item
    in the array.

    \sa begin(), constEnd()
*/

/*! \fn QJsonArray::iterator QJsonArray::end()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the imaginary item
    after the last item in the array.

    \sa begin(), constEnd()
*/

/*! \fn const_iterator QJsonArray::end() const

    \overload
*/

/*! \fn QJsonArray::const_iterator QJsonArray::constEnd() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last item in the array.

    \sa constBegin(), end()
*/

/*! \fn void QJsonArray::push_back(const QJsonValue &value)

    This function is provided for STL compatibility. It is equivalent
    to \l{QJsonArray::append()}{append(value)} and will append \a value to the array.
*/

/*! \fn void QJsonArray::push_front(const QJsonValue &value)

    This function is provided for STL compatibility. It is equivalent
    to \l{QJsonArray::prepend()}{prepend(value)} and will prepend \a value to the array.
*/

/*! \fn void QJsonArray::pop_front()

    This function is provided for STL compatibility. It is equivalent
    to removeFirst(). The array must not be empty. If the array can be
    empty, call isEmpty() before calling this function.
*/

/*! \fn void QJsonArray::pop_back()

    This function is provided for STL compatibility. It is equivalent
    to removeLast(). The array must not be empty. If the array can be
    empty, call isEmpty() before calling this function.
*/

/*! \fn bool QJsonArray::empty() const

    This function is provided for STL compatibility. It is equivalent
    to isEmpty() and returns \c true if the array is empty.
*/

/*! \class QJsonArray::iterator
    \inmodule QtCore
    \brief The QJsonArray::iterator class provides an STL-style non-const iterator for QJsonArray.

    QJsonArray::iterator allows you to iterate over a QJsonArray
    and to modify the array item associated with the
    iterator. If you want to iterate over a const QJsonArray, use
    QJsonArray::const_iterator instead. It is generally a good practice to
    use QJsonArray::const_iterator on a non-const QJsonArray as well, unless
    you need to change the QJsonArray through the iterator. Const
    iterators are slightly faster and improves code readability.

    The default QJsonArray::iterator constructor creates an uninitialized
    iterator. You must initialize it using a QJsonArray function like
    QJsonArray::begin(), QJsonArray::end(), or QJsonArray::insert() before you can
    start iterating.

    Most QJsonArray functions accept an integer index rather than an
    iterator. For that reason, iterators are rarely useful in
    connection with QJsonArray. One place where STL-style iterators do
    make sense is as arguments to \l{generic algorithms}.

    Multiple iterators can be used on the same array. However, be
    aware that any non-const function call performed on the QJsonArray
    will render all existing iterators undefined.

    \sa QJsonArray::const_iterator
*/

/*! \typedef QJsonArray::iterator::iterator_category

  A synonym for \e {std::random_access_iterator_tag} indicating
  this iterator is a random access iterator.
*/

/*! \typedef QJsonArray::iterator::difference_type

    \internal
*/

/*! \typedef QJsonArray::iterator::value_type

    \internal
*/

/*! \typedef QJsonArray::iterator::reference

    \internal
*/

/*! \fn QJsonArray::iterator::iterator()

    Constructs an uninitialized iterator.

    Functions like operator*() and operator++() should not be called
    on an uninitialized iterator. Use operator=() to assign a value
    to it before using it.

    \sa QJsonArray::begin(), QJsonArray::end()
*/

/*! \fn QJsonArray::iterator::iterator(QJsonArray *array, int index)
    \internal
*/

/*! \fn QJsonValueRef QJsonArray::iterator::operator*() const

    Returns a modifiable reference to the current item.

    You can change the value of an item by using operator*() on the
    left side of an assignment.

    The return value is of type QJsonValueRef, a helper class for QJsonArray
    and QJsonObject. When you get an object of type QJsonValueRef, you can
    use it as if it were a reference to a QJsonValue. If you assign to it,
    the assignment will apply to the character in the QJsonArray of QJsonObject
    from which you got the reference.
*/

/*! \fn QJsonValueRef QJsonArray::iterator::operator[](int j) const

    Returns a modifiable reference to the item at offset \a j from the
    item pointed to by this iterator (the item at position \c{*this + j}).

    This function is provided to make QJsonArray iterators behave like C++
    pointers.

    The return value is of type QJsonValueRef, a helper class for QJsonArray
    and QJsonObject. When you get an object of type QJsonValueRef, you can
    use it as if it were a reference to a QJsonValue. If you assign to it,
    the assignment will apply to the character in the QJsonArray of QJsonObject
    from which you got the reference.

    \sa operator+()
*/

/*!
    \fn bool QJsonArray::iterator::operator==(const iterator &other) const
    \fn bool QJsonArray::iterator::operator==(const const_iterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn bool QJsonArray::iterator::operator!=(const iterator &other) const
    \fn bool QJsonArray::iterator::operator!=(const const_iterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn bool QJsonArray::iterator::operator<(const iterator& other) const
    \fn bool QJsonArray::iterator::operator<(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is less than
    the item pointed to by the \a other iterator.
*/

/*!
    \fn bool QJsonArray::iterator::operator<=(const iterator& other) const
    \fn bool QJsonArray::iterator::operator<=(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is less than
    or equal to the item pointed to by the \a other iterator.
*/

/*!
    \fn bool QJsonArray::iterator::operator>(const iterator& other) const
    \fn bool QJsonArray::iterator::operator>(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is greater
    than the item pointed to by the \a other iterator.
*/

/*!
    \fn bool QJsonArray::iterator::operator>=(const iterator& other) const
    \fn bool QJsonArray::iterator::operator>=(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is greater
    than or equal to the item pointed to by the \a other iterator.
*/

/*! \fn QJsonArray::iterator &QJsonArray::iterator::operator++()

    The prefix ++ operator, \c{++it}, advances the iterator to the
    next item in the array and returns an iterator to the new current
    item.

    Calling this function on QJsonArray::end() leads to undefined results.

    \sa operator--()
*/

/*! \fn QJsonArray::iterator QJsonArray::iterator::operator++(int)

    \overload

    The postfix ++ operator, \c{it++}, advances the iterator to the
    next item in the array and returns an iterator to the previously
    current item.
*/

/*! \fn QJsonArray::iterator &QJsonArray::iterator::operator--()

    The prefix -- operator, \c{--it}, makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QJsonArray::begin() leads to undefined results.

    \sa operator++()
*/

/*! \fn QJsonArray::iterator QJsonArray::iterator::operator--(int)

    \overload

    The postfix -- operator, \c{it--}, makes the preceding item
    current and returns an iterator to the previously current item.
*/

/*! \fn QJsonArray::iterator &QJsonArray::iterator::operator+=(int j)

    Advances the iterator by \a j items. If \a j is negative, the
    iterator goes backward.

    \sa operator-=(), operator+()
*/

/*! \fn QJsonArray::iterator &QJsonArray::iterator::operator-=(int j)

    Makes the iterator go back by \a j items. If \a j is negative,
    the iterator goes forward.

    \sa operator+=(), operator-()
*/

/*! \fn QJsonArray::iterator QJsonArray::iterator::operator+(int j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. If \a j is negative, the iterator goes backward.

    \sa operator-(), operator+=()
*/

/*! \fn QJsonArray::iterator QJsonArray::iterator::operator-(int j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. If \a j is negative, the iterator goes forward.

    \sa operator+(), operator-=()
*/

/*! \fn int QJsonArray::iterator::operator-(iterator other) const

    Returns the number of items between the item pointed to by \a
    other and the item pointed to by this iterator.
*/

/*! \class QJsonArray::const_iterator
    \inmodule QtCore
    \brief The QJsonArray::const_iterator class provides an STL-style const iterator for QJsonArray.

    QJsonArray::const_iterator allows you to iterate over a
    QJsonArray. If you want to modify the QJsonArray as
    you iterate over it, use QJsonArray::iterator instead. It is generally a
    good practice to use QJsonArray::const_iterator on a non-const QJsonArray
    as well, unless you need to change the QJsonArray through the
    iterator. Const iterators are slightly faster and improves
    code readability.

    The default QJsonArray::const_iterator constructor creates an
    uninitialized iterator. You must initialize it using a QJsonArray
    function like QJsonArray::constBegin(), QJsonArray::constEnd(), or
    QJsonArray::insert() before you can start iterating.

    Most QJsonArray functions accept an integer index rather than an
    iterator. For that reason, iterators are rarely useful in
    connection with QJsonArray. One place where STL-style iterators do
    make sense is as arguments to \l{generic algorithms}.

    Multiple iterators can be used on the same array. However, be
    aware that any non-const function call performed on the QJsonArray
    will render all existing iterators undefined.

    \sa QJsonArray::iterator
*/

/*! \fn QJsonArray::const_iterator::const_iterator()

    Constructs an uninitialized iterator.

    Functions like operator*() and operator++() should not be called
    on an uninitialized iterator. Use operator=() to assign a value
    to it before using it.

    \sa QJsonArray::constBegin(), QJsonArray::constEnd()
*/

/*! \fn QJsonArray::const_iterator::const_iterator(const QJsonArray *array, int index)
    \internal
*/

/*! \typedef QJsonArray::const_iterator::iterator_category

  A synonym for \e {std::random_access_iterator_tag} indicating
  this iterator is a random access iterator.
*/

/*! \typedef QJsonArray::const_iterator::difference_type

    \internal
*/

/*! \typedef QJsonArray::const_iterator::value_type

    \internal
*/

/*! \typedef QJsonArray::const_iterator::reference

    \internal
*/

/*! \fn QJsonArray::const_iterator::const_iterator(const const_iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn QJsonArray::const_iterator::const_iterator(const iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn QJsonValue QJsonArray::const_iterator::operator*() const

    Returns the current item.
*/

/*! \fn QJsonValue QJsonArray::const_iterator::operator[](int j) const

    Returns the item at offset \a j from the item pointed to by this iterator (the item at
    position \c{*this + j}).

    This function is provided to make QJsonArray iterators behave like C++
    pointers.

    \sa operator+()
*/

/*! \fn bool QJsonArray::const_iterator::operator==(const const_iterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*! \fn bool QJsonArray::const_iterator::operator!=(const const_iterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn bool QJsonArray::const_iterator::operator<(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is less than
    the item pointed to by the \a other iterator.
*/

/*!
    \fn bool QJsonArray::const_iterator::operator<=(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is less than
    or equal to the item pointed to by the \a other iterator.
*/

/*!
    \fn bool QJsonArray::const_iterator::operator>(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is greater
    than the item pointed to by the \a other iterator.
*/

/*!
    \fn bool QJsonArray::const_iterator::operator>=(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is greater
    than or equal to the item pointed to by the \a other iterator.
*/

/*! \fn QJsonArray::const_iterator &QJsonArray::const_iterator::operator++()

    The prefix ++ operator, \c{++it}, advances the iterator to the
    next item in the array and returns an iterator to the new current
    item.

    Calling this function on QJsonArray::end() leads to undefined results.

    \sa operator--()
*/

/*! \fn QJsonArray::const_iterator QJsonArray::const_iterator::operator++(int)

    \overload

    The postfix ++ operator, \c{it++}, advances the iterator to the
    next item in the array and returns an iterator to the previously
    current item.
*/

/*! \fn QJsonArray::const_iterator &QJsonArray::const_iterator::operator--()

    The prefix -- operator, \c{--it}, makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QJsonArray::begin() leads to undefined results.

    \sa operator++()
*/

/*! \fn QJsonArray::const_iterator QJsonArray::const_iterator::operator--(int)

    \overload

    The postfix -- operator, \c{it--}, makes the preceding item
    current and returns an iterator to the previously current item.
*/

/*! \fn QJsonArray::const_iterator &QJsonArray::const_iterator::operator+=(int j)

    Advances the iterator by \a j items. If \a j is negative, the
    iterator goes backward.

    \sa operator-=(), operator+()
*/

/*! \fn QJsonArray::const_iterator &QJsonArray::const_iterator::operator-=(int j)

    Makes the iterator go back by \a j items. If \a j is negative,
    the iterator goes forward.

    \sa operator+=(), operator-()
*/

/*! \fn QJsonArray::const_iterator QJsonArray::const_iterator::operator+(int j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. If \a j is negative, the iterator goes backward.

    \sa operator-(), operator+=()
*/

/*! \fn QJsonArray::const_iterator QJsonArray::const_iterator::operator-(int j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. If \a j is negative, the iterator goes forward.

    \sa operator+(), operator-=()
*/

/*! \fn int QJsonArray::const_iterator::operator-(const_iterator other) const

    Returns the number of items between the item pointed to by \a
    other and the item pointed to by this iterator.
*/


/*!
    \internal
 */
void QJsonArray::detach(uint reserve)
{
    if (!d) {
        d = new QJsonPrivate::Data(reserve, QJsonValue::Array);
        a = static_cast<QJsonPrivate::Array *>(d->header->root());
        d->ref.ref();
        return;
    }
    if (reserve == 0 && d->ref.load() == 1)
        return;

    QJsonPrivate::Data *x = d->clone(a, reserve);
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
    a = static_cast<QJsonPrivate::Array *>(d->header->root());
}

/*!
    \internal
 */
void QJsonArray::compact()
{
    if (!d || !d->compactionCounter)
        return;

    detach();
    d->compact();
    a = static_cast<QJsonPrivate::Array *>(d->header->root());
}


#if !defined(QT_NO_DEBUG_STREAM) && !defined(QT_JSON_READONLY)
QDebug operator<<(QDebug dbg, const QJsonArray &a)
{
    if (!a.a) {
        dbg << "QJsonArray()";
        return dbg;
    }
    QByteArray json;
    QJsonPrivate::Writer::arrayToJson(a.a, json, 0, true);
    dbg.nospace() << "QJsonArray("
                  << json.constData() // print as utf-8 string without extra quotation marks
                  << ")";
    return dbg.space();
}
#endif

QT_END_NAMESPACE

