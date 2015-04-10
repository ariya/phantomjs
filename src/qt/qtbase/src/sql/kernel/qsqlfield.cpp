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

#include "qsqlfield.h"
#include "qatomic.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

class QSqlFieldPrivate
{
public:
    QSqlFieldPrivate(const QString &name,
              QVariant::Type type) :
        ref(1), nm(name), ro(false), type(type), req(QSqlField::Unknown),
        len(-1), prec(-1), tp(-1), gen(true), autoval(false)
    {
    }

    QSqlFieldPrivate(const QSqlFieldPrivate &other)
        : ref(1),
          nm(other.nm),
          ro(other.ro),
          type(other.type),
          req(other.req),
          len(other.len),
          prec(other.prec),
          def(other.def),
          tp(other.tp),
          gen(other.gen),
          autoval(other.autoval)
    {}

    bool operator==(const QSqlFieldPrivate& other) const
    {
        return (nm == other.nm
                && ro == other.ro
                && type == other.type
                && req == other.req
                && len == other.len
                && prec == other.prec
                && def == other.def
                && gen == other.gen
                && autoval == other.autoval);
    }

    QAtomicInt ref;
    QString nm;
    uint ro: 1;
    QVariant::Type type;
    QSqlField::RequiredStatus req;
    int len;
    int prec;
    QVariant def;
    int tp;
    uint gen: 1;
    uint autoval: 1;
};


/*!
    \class QSqlField
    \brief The QSqlField class manipulates the fields in SQL database tables
    and views.

    \ingroup database
    \ingroup shared
    \inmodule QtSql

    QSqlField represents the characteristics of a single column in a
    database table or view, such as the data type and column name. A
    field also contains the value of the database column, which can be
    viewed or changed.

    Field data values are stored as QVariants. Using an incompatible
    type is not permitted. For example:

    \snippet sqldatabase/sqldatabase.cpp 2

    However, the field will attempt to cast certain data types to the
    field data type where possible:

    \snippet sqldatabase/sqldatabase.cpp 3

    QSqlField objects are rarely created explicitly in application
    code. They are usually accessed indirectly through \l{QSqlRecord}s
    that already contain a list of fields. For example:

    \snippet sqldatabase/sqldatabase.cpp 4
    \dots
    \snippet sqldatabase/sqldatabase.cpp 5
    \snippet sqldatabase/sqldatabase.cpp 6

    A QSqlField object can provide some meta-data about the field, for
    example, its name(), variant type(), length(), precision(),
    defaultValue(), typeID(), and its requiredStatus(),
    isGenerated() and isReadOnly(). The field's data can be
    checked to see if it isNull(), and its value() retrieved. When
    editing the data can be set with setValue() or set to NULL with
    clear().

    \sa QSqlRecord
*/

/*!
    \enum QSqlField::RequiredStatus

    Specifies whether the field is required or optional.

    \value Required  The field must be specified when inserting records.
    \value Optional  The fields doesn't have to be specified when inserting records.
    \value Unknown  The database driver couldn't determine whether the field is required or
                    optional.

    \sa requiredStatus()
*/

/*!
    Constructs an empty field called \a fieldName of variant type \a
    type.

    \sa setRequiredStatus(), setLength(), setPrecision(), setDefaultValue(),
        setGenerated(), setReadOnly()
*/
QSqlField::QSqlField(const QString& fieldName, QVariant::Type type)
{
    d = new QSqlFieldPrivate(fieldName, type);
    val = QVariant(type);
}

/*!
    Constructs a copy of \a other.
*/

QSqlField::QSqlField(const QSqlField& other)
{
    d = other.d;
    d->ref.ref();
    val = other.val;
}

/*!
    Sets the field equal to \a other.
*/

QSqlField& QSqlField::operator=(const QSqlField& other)
{
    qAtomicAssign(d, other.d);
    val = other.val;
    return *this;
}


/*! \fn bool QSqlField::operator!=(const QSqlField &other) const
    Returns \c true if the field is unequal to \a other; otherwise returns
    false.
*/

/*!
    Returns \c true if the field is equal to \a other; otherwise returns
    false.
*/
bool QSqlField::operator==(const QSqlField& other) const
{
    return ((d == other.d || *d == *other.d)
            && val == other.val);
}

/*!
    Destroys the object and frees any allocated resources.
*/

QSqlField::~QSqlField()
{
    if (!d->ref.deref())
        delete d;
}

/*!
    Sets the required status of this field to \a required.

    \sa requiredStatus(), setType(), setLength(), setPrecision(),
        setDefaultValue(), setGenerated(), setReadOnly()
*/
void QSqlField::setRequiredStatus(RequiredStatus required)
{
    detach();
    d->req = required;
}

/*! \fn void QSqlField::setRequired(bool required)

    Sets the required status of this field to \l Required if \a
    required is true; otherwise sets it to \l Optional.

    \sa setRequiredStatus(), requiredStatus()
*/

/*!
    Sets the field's length to \a fieldLength. For strings this is the
    maximum number of characters the string can hold; the meaning
    varies for other types.

    \sa length(), setType(), setRequiredStatus(), setPrecision(),
        setDefaultValue(), setGenerated(), setReadOnly()
*/
void QSqlField::setLength(int fieldLength)
{
    detach();
    d->len = fieldLength;
}

/*!
    Sets the field's \a precision. This only affects numeric fields.

    \sa precision(), setType(), setRequiredStatus(), setLength(),
        setDefaultValue(), setGenerated(), setReadOnly()
*/
void QSqlField::setPrecision(int precision)
{
    detach();
    d->prec = precision;
}

/*!
    Sets the default value used for this field to \a value.

    \sa defaultValue(), value(), setType(), setRequiredStatus(),
        setLength(), setPrecision(), setGenerated(), setReadOnly()
*/
void QSqlField::setDefaultValue(const QVariant &value)
{
    detach();
    d->def = value;
}

/*!
    \internal
*/
void QSqlField::setSqlType(int type)
{
    detach();
    d->tp = type;
}

/*!
    Sets the generated state. If \a gen is false, no SQL will
    be generated for this field; otherwise, Qt classes such as
    QSqlQueryModel and QSqlTableModel will generate SQL for this
    field.

    \sa isGenerated(), setType(), setRequiredStatus(), setLength(),
        setPrecision(), setDefaultValue(), setReadOnly()
*/
void QSqlField::setGenerated(bool gen)
{
    detach();
    d->gen = gen;
}


/*!
    Sets the value of the field to \a value. If the field is read-only
    (isReadOnly() returns \c true), nothing happens.

    If the data type of \a value differs from the field's current
    data type, an attempt is made to cast it to the proper type. This
    preserves the data type of the field in the case of assignment,
    e.g. a QString to an integer data type.

    To set the value to NULL, use clear().

    \sa value(), isReadOnly(), defaultValue()
*/

void QSqlField::setValue(const QVariant& value)
{
    if (isReadOnly())
        return;
    val = value;
}

/*!
    Clears the value of the field and sets it to NULL.
    If the field is read-only, nothing happens.

    \sa setValue(), isReadOnly(), requiredStatus()
*/

void QSqlField::clear()
{
    if (isReadOnly())
        return;
    val = QVariant(type());
}

/*!
    Sets the name of the field to \a name.

    \sa name()
*/

void QSqlField::setName(const QString& name)
{
    detach();
    d->nm = name;
}

/*!
    Sets the read only flag of the field's value to \a readOnly. A
    read-only field cannot have its value set with setValue() and
    cannot be cleared to NULL with clear().
*/
void QSqlField::setReadOnly(bool readOnly)
{
    detach();
    d->ro = readOnly;
}

/*!
    \fn QVariant QSqlField::value() const

    Returns the value of the field as a QVariant.

    Use isNull() to check if the field's value is NULL.
*/

/*!
    Returns the name of the field.

    \sa setName()
*/
QString QSqlField::name() const
{
    return d->nm;
}

/*!
    Returns the field's type as stored in the database.
    Note that the actual value might have a different type,
    Numerical values that are too large to store in a long
    int or double are usually stored as strings to prevent
    precision loss.

    \sa setType()
*/
QVariant::Type QSqlField::type() const
{
    return d->type;
}

/*!
    Set's the field's variant type to \a type.

    \sa type(), setRequiredStatus(), setLength(), setPrecision(),
        setDefaultValue(), setGenerated(), setReadOnly()
*/
void QSqlField::setType(QVariant::Type type)
{
    detach();
    d->type = type;
    if (!val.isValid())
        val = QVariant(type);
}


/*!
    Returns \c true if the field's value is read-only; otherwise returns
    false.

    \sa setReadOnly(), type(), requiredStatus(), length(), precision(),
        defaultValue(), isGenerated()
*/
bool QSqlField::isReadOnly() const
{ return d->ro; }

/*!
    Returns \c true if the field's value is NULL; otherwise returns
    false.

    \sa value()
*/
bool QSqlField::isNull() const
{ return val.isNull(); }

/*! \internal
*/
void QSqlField::detach()
{
    qAtomicDetach(d);
}

/*!
    Returns \c true if this is a required field; otherwise returns \c false.
    An \c INSERT will fail if a required field does not have a value.

    \sa setRequiredStatus(), type(), length(), precision(), defaultValue(),
        isGenerated()
*/
QSqlField::RequiredStatus QSqlField::requiredStatus() const
{
    return d->req;
}

/*!
    Returns the field's length.

    If the returned value is negative, it means that the information
    is not available from the database.

    \sa setLength(), type(), requiredStatus(), precision(), defaultValue(),
        isGenerated()
*/
int QSqlField::length() const
{
    return d->len;
}

/*!
    Returns the field's precision; this is only meaningful for numeric
    types.

    If the returned value is negative, it means that the information
    is not available from the database.

    \sa setPrecision(), type(), requiredStatus(), length(), defaultValue(),
        isGenerated()
*/
int QSqlField::precision() const
{
    return d->prec;
}

/*!
    Returns the field's default value (which may be NULL).

    \sa setDefaultValue(), type(), requiredStatus(), length(), precision(),
        isGenerated()
*/
QVariant QSqlField::defaultValue() const
{
    return d->def;
}

/*!
    \internal

    Returns the type ID for the field.

    If the returned value is negative, it means that the information
    is not available from the database.
*/
int QSqlField::typeID() const
{
    return d->tp;
}

/*!
    Returns \c true if the field is generated; otherwise returns
    false.

    \sa setGenerated(), type(), requiredStatus(), length(), precision(),
        defaultValue()
*/
bool QSqlField::isGenerated() const
{
    return d->gen;
}

/*!
    Returns \c true if the field's variant type is valid; otherwise
    returns \c false.
*/
bool QSqlField::isValid() const
{
    return d->type != QVariant::Invalid;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QSqlField &f)
{
    dbg.nospace() << "QSqlField(" << f.name() << ", " << QMetaType::typeName(f.type());
    if (f.length() >= 0)
        dbg.nospace() << ", length: " << f.length();
    if (f.precision() >= 0)
        dbg.nospace() << ", precision: " << f.precision();
    if (f.requiredStatus() != QSqlField::Unknown)
        dbg.nospace() << ", required: "
                      << (f.requiredStatus() == QSqlField::Required ? "yes" : "no");
    dbg.nospace() << ", generated: " << (f.isGenerated() ? "yes" : "no");
    if (f.typeID() >= 0)
        dbg.nospace() << ", typeID: " << f.typeID();
    if (!f.defaultValue().isNull())
        dbg.nospace() << ", auto-value: \"" << f.defaultValue() << '\"';
    dbg.nospace() << ')';
    return dbg.space();
}
#endif

/*!
    Returns \c true if the value is auto-generated by the database,
    for example auto-increment primary key values.

    \sa setAutoValue()
*/
bool QSqlField::isAutoValue() const
{
    return d->autoval;
}

/*!
    Marks the field as an auto-generated value if \a autoVal
    is true.

    \sa isAutoValue()
 */
void QSqlField::setAutoValue(bool autoVal)
{
    detach();
    d->autoval = autoVal;
}

QT_END_NAMESPACE
