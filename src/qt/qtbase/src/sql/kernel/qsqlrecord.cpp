/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSql module of the Qt Toolkit.
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

#include "qsqlrecord.h"

#include "qdebug.h"
#include "qstringlist.h"
#include "qatomic.h"
#include "qsqlfield.h"
#include "qstring.h"
#include "qvector.h"

QT_BEGIN_NAMESPACE

class QSqlRecordPrivate
{
public:
    QSqlRecordPrivate();
    QSqlRecordPrivate(const QSqlRecordPrivate &other);

    inline bool contains(int index) { return index >= 0 && index < fields.count(); }
    QString createField(int index, const QString &prefix) const;

    QVector<QSqlField> fields;
    QAtomicInt ref;
};

QSqlRecordPrivate::QSqlRecordPrivate() : ref(1)
{
}

QSqlRecordPrivate::QSqlRecordPrivate(const QSqlRecordPrivate &other): fields(other.fields), ref(1)
{
}

/*! \internal
    Just for compat
*/
QString QSqlRecordPrivate::createField(int index, const QString &prefix) const
{
    QString f;
    if (!prefix.isEmpty())
        f = prefix + QLatin1Char('.');
    f += fields.at(index).name();
    return f;
}

/*!
    \class QSqlRecord
    \brief The QSqlRecord class encapsulates a database record.

    \ingroup database
    \ingroup shared
    \inmodule QtSql

    The QSqlRecord class encapsulates the functionality and
    characteristics of a database record (usually a row in a table or
    view within the database). QSqlRecord supports adding and
    removing fields as well as setting and retrieving field values.

    The values of a record's fields' can be set by name or position
    with setValue(); if you want to set a field to null use
    setNull(). To find the position of a field by name use indexOf(),
    and to find the name of a field at a particular position use
    fieldName(). Use field() to retrieve a QSqlField object for a
    given field. Use contains() to see if the record contains a
    particular field name.

    When queries are generated to be executed on the database only
    those fields for which isGenerated() is true are included in the
    generated SQL.

    A record can have fields added with append() or insert(), replaced
    with replace(), and removed with remove(). All the fields can be
    removed with clear(). The number of fields is given by count();
    all their values can be cleared (to null) using clearValues().

    \sa QSqlField, QSqlQuery::record()
*/


/*!
    Constructs an empty record.

    \sa isEmpty(), append(), insert()
*/

QSqlRecord::QSqlRecord()
{
    d = new QSqlRecordPrivate();
}

/*!
    Constructs a copy of \a other.

    QSqlRecord is \l{implicitly shared}. This means you can make copies
    of a record in \l{constant time}.
*/

QSqlRecord::QSqlRecord(const QSqlRecord& other)
{
    d = other.d;
    d->ref.ref();
}

/*!
    Sets the record equal to \a other.

    QSqlRecord is \l{implicitly shared}. This means you can make copies
    of a record in \l{constant time}.
*/

QSqlRecord& QSqlRecord::operator=(const QSqlRecord& other)
{
    qAtomicAssign(d, other.d);
    return *this;
}

/*!
    Destroys the object and frees any allocated resources.
*/

QSqlRecord::~QSqlRecord()
{
    if (!d->ref.deref())
        delete d;
}

/*!
    \fn bool QSqlRecord::operator!=(const QSqlRecord &other) const

    Returns \c true if this object is not identical to \a other;
    otherwise returns \c false.

    \sa operator==()
*/

/*!
    Returns \c true if this object is identical to \a other (i.e., has
    the same fields in the same order); otherwise returns \c false.

    \sa operator!=()
*/
bool QSqlRecord::operator==(const QSqlRecord &other) const
{
    return d->fields == other.d->fields;
}

/*!
    Returns the value of the field located at position \a index in
    the record. If \a index is out of bounds, an invalid QVariant
    is returned.

    \sa fieldName(), isNull()
*/

QVariant QSqlRecord::value(int index) const
{
    return d->fields.value(index).value();
}

/*!
    \overload

    Returns the value of the field called \a name in the record. If
    field \a name does not exist an invalid variant is returned.

    \sa indexOf()
*/

QVariant QSqlRecord::value(const QString& name) const
{
    return value(indexOf(name));
}

/*!
    Returns the name of the field at position \a index. If the field
    does not exist, an empty string is returned.

    \sa indexOf()
*/

QString QSqlRecord::fieldName(int index) const
{
    return d->fields.value(index).name();
}

/*!
    Returns the position of the field called \a name within the
    record, or -1 if it cannot be found. Field names are not
    case-sensitive. If more than one field matches, the first one is
    returned.

    \sa fieldName()
*/

int QSqlRecord::indexOf(const QString& name) const
{
    QString nm = name.toUpper();
    for (int i = 0; i < count(); ++i) {
        if (d->fields.at(i).name().toUpper() == nm) // TODO: case-insensitive comparison
            return i;
    }
    return -1;
}

/*!
    Returns the field at position \a index. If the \a index
    is out of range, function returns
    a \l{default-constructed value}.
 */
QSqlField QSqlRecord::field(int index) const
{
    return d->fields.value(index);
}

/*! \overload
    Returns the field called \a name.
 */
QSqlField QSqlRecord::field(const QString &name) const
{
    return field(indexOf(name));
}


/*!
    Append a copy of field \a field to the end of the record.

    \sa insert(), replace(), remove()
*/

void QSqlRecord::append(const QSqlField& field)
{
    detach();
    d->fields.append(field);
}

/*!
    Inserts the field \a field at position \a pos in the record.

    \sa append(), replace(), remove()
 */
void QSqlRecord::insert(int pos, const QSqlField& field)
{
   detach();
   d->fields.insert(pos, field);
}

/*!
    Replaces the field at position \a pos with the given \a field. If
    \a pos is out of range, nothing happens.

    \sa append(), insert(), remove()
*/

void QSqlRecord::replace(int pos, const QSqlField& field)
{
    if (!d->contains(pos))
        return;

    detach();
    d->fields[pos] = field;
}

/*!
    Removes the field at position \a pos. If \a pos is out of range,
    nothing happens.

    \sa append(), insert(), replace()
*/

void QSqlRecord::remove(int pos)
{
    if (!d->contains(pos))
        return;

    detach();
    d->fields.remove(pos);
}

/*!
    Removes all the record's fields.

    \sa clearValues(), isEmpty()
*/

void QSqlRecord::clear()
{
    detach();
    d->fields.clear();
}

/*!
    Returns \c true if there are no fields in the record; otherwise
    returns \c false.

    \sa append(), insert(), clear()
*/

bool QSqlRecord::isEmpty() const
{
    return d->fields.isEmpty();
}


/*!
    Returns \c true if there is a field in the record called \a name;
    otherwise returns \c false.
*/

bool QSqlRecord::contains(const QString& name) const
{
    return indexOf(name) >= 0;
}

/*!
    Clears the value of all fields in the record and sets each field
    to null.

    \sa setValue()
*/

void QSqlRecord::clearValues()
{
    detach();
    int count = d->fields.count();
    for (int i = 0; i < count; ++i)
        d->fields[i].clear();
}

/*!
    Sets the generated flag for the field called \a name to \a
    generated. If the field does not exist, nothing happens. Only
    fields that have \a generated set to true are included in the SQL
    that is generated by QSqlQueryModel for example.

    \sa isGenerated()
*/

void QSqlRecord::setGenerated(const QString& name, bool generated)
{
    setGenerated(indexOf(name), generated);
}

/*!
    \overload

    Sets the generated flag for the field \a index to \a generated.

    \sa isGenerated()
*/

void QSqlRecord::setGenerated(int index, bool generated)
{
    if (!d->contains(index))
        return;
    detach();
    d->fields[index].setGenerated(generated);
}

/*!
    \overload

    Returns \c true if the field \a index is null or if there is no field at
    position \a index; otherwise returns \c false.
*/
bool QSqlRecord::isNull(int index) const
{
    return d->fields.value(index).isNull();
}

/*!
    Returns \c true if the field called \a name is null or if there is no
    field called \a name; otherwise returns \c false.

    \sa setNull()
*/
bool QSqlRecord::isNull(const QString& name) const
{
    return isNull(indexOf(name));
}

/*!
    Sets the value of field \a index to null. If the field does not exist,
    nothing happens.

    \sa setValue()
*/
void QSqlRecord::setNull(int index)
{
    if (!d->contains(index))
        return;
    detach();
    d->fields[index].clear();
}

/*!
    \overload

    Sets the value of the field called \a name to null. If the field
    does not exist, nothing happens.
*/
void QSqlRecord::setNull(const QString& name)
{
    setNull(indexOf(name));
}


/*!
    Returns \c true if the record has a field called \a name and this
    field is to be generated (the default); otherwise returns \c false.

    \sa setGenerated()
*/
bool QSqlRecord::isGenerated(const QString& name) const
{
    return isGenerated(indexOf(name));
}

/*! \overload

    Returns \c true if the record has a field at position \a index and this
    field is to be generated (the default); otherwise returns \c false.

    \sa setGenerated()
*/
bool QSqlRecord::isGenerated(int index) const
{
    return d->fields.value(index).isGenerated();
}

/*!
    Returns the number of fields in the record.

    \sa isEmpty()
*/

int QSqlRecord::count() const
{
    return d->fields.count();
}

/*!
    Sets the value of the field at position \a index to \a val. If the
    field does not exist, nothing happens.

    \sa setNull()
*/

void QSqlRecord::setValue(int index, const QVariant& val)
{
    if (!d->contains(index))
        return;
    detach();
    d->fields[index].setValue(val);
}


/*!
    \overload

    Sets the value of the field called \a name to \a val. If the field
    does not exist, nothing happens.
*/

void QSqlRecord::setValue(const QString& name, const QVariant& val)
{
    setValue(indexOf(name), val);
}


/*! \internal
*/
void QSqlRecord::detach()
{
    qAtomicDetach(d);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QSqlRecord &r)
{
    dbg << "QSqlRecord(" << r.count() << ')';
    for (int i = 0; i < r.count(); ++i)
        dbg << '\n' << QString::fromLatin1("%1:").arg(i, 2) << r.field(i) << r.value(i).toString();
    return dbg;
}
#endif

/*!
    \since 5.1
    Returns a record containing the fields represented in \a keyFields set to values
    that match by field name.
*/
QSqlRecord QSqlRecord::keyValues(const QSqlRecord &keyFields) const
{
    QSqlRecord retValues(keyFields);

    for (int i = retValues.count() - 1; i >= 0; --i)
        retValues.setValue(i, value(retValues.fieldName(i)));

    return retValues;
}

QT_END_NAMESPACE
