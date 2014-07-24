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

#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qjsonarray.h>
#include <qstringlist.h>
#include <qvariant.h>
#include <qdebug.h>
#include "qjsonwriter_p.h"
#include "qjsonparser_p.h"
#include "qjson_p.h"

QT_BEGIN_NAMESPACE

/*! \class QJsonDocument
    \inmodule QtCore
    \ingroup json
    \reentrant
    \since 5.0

    \brief The QJsonDocument class provides a way to read and write JSON documents.

    QJsonDocument is a class that wraps a complete JSON document and can read and
    write this document both from a UTF-8 encoded text based representation as well
    as Qt's own binary format.

    A JSON document can be converted from its text-based representation to a QJsonDocument
    using QJsonDocument::fromJson(). toJson() converts it back to text. The parser is very
    fast and efficient and converts the JSON to the binary representation used by Qt.

    Validity of the parsed document can be queried with !isNull()

    A document can be queried as to whether it contains an array or an object using isArray()
    and isObject(). The array or object contained in the document can be retrieved using
    array() or object() and then read or manipulated.

    A document can also be created from a stored binary representation using fromBinaryData() or
    fromRawData().

    \sa {JSON Support in Qt}, {JSON Save Game Example}
*/

/*!
 * Constructs an empty and invalid document.
 */
QJsonDocument::QJsonDocument()
    : d(0)
{
}

/*!
 * Creates a QJsonDocument from \a object.
 */
QJsonDocument::QJsonDocument(const QJsonObject &object)
    : d(0)
{
    setObject(object);
}

/*!
 * Constructs a QJsonDocument from \a array.
 */
QJsonDocument::QJsonDocument(const QJsonArray &array)
    : d(0)
{
    setArray(array);
}

/*!
    \internal
 */
QJsonDocument::QJsonDocument(QJsonPrivate::Data *data)
    : d(data)
{
    Q_ASSERT(d);
    d->ref.ref();
}

/*!
 Deletes the document.

 Binary data set with fromRawData is not freed.
 */
QJsonDocument::~QJsonDocument()
{
    if (d && !d->ref.deref())
        delete d;
}

/*!
 * Creates a copy of the \a other document.
 */
QJsonDocument::QJsonDocument(const QJsonDocument &other)
{
    d = other.d;
    if (d)
        d->ref.ref();
}

/*!
 * Assigns the \a other document to this QJsonDocument.
 * Returns a reference to this object.
 */
QJsonDocument &QJsonDocument::operator =(const QJsonDocument &other)
{
    if (d != other.d) {
        if (d && !d->ref.deref())
            delete d;
        d = other.d;
        if (d)
            d->ref.ref();
    }

    return *this;
}

/*! \enum QJsonDocument::DataValidation

  This value is used to tell QJsonDocument whether to validate the binary data
  when converting to a QJsonDocument using fromBinaryData() or fromRawData().

  \value Validate Validate the data before using it. This is the default.
  \value BypassValidation Bypasses data validation. Only use if you received the
  data from a trusted place and know it's valid, as using of invalid data can crash
  the application.
  */

/*!
 Creates a QJsonDocument that uses the first \a size bytes from
 \a data. It assumes \a data contains a binary encoded JSON document.
 The created document does not take ownership of \a data and the caller
 has to guarantee that \a data will not be deleted or modified as long as
 any QJsonDocument, QJsonObject or QJsonArray still references the data.

 \a data has to be aligned to a 4 byte boundary.

 \a validation decides whether the data is checked for validity before being used.
 By default the data is validated. If the \a data is not valid, the method returns
 a null document.

 Returns a QJsonDocument representing the data.

 \sa rawData(), fromBinaryData(), isNull(), DataValidation
 */
QJsonDocument QJsonDocument::fromRawData(const char *data, int size, DataValidation validation)
{
    if (quintptr(data) & 3) {
        qWarning() <<"QJsonDocument::fromRawData: data has to have 4 byte alignment";
        return QJsonDocument();
    }

    QJsonPrivate::Data *d = new QJsonPrivate::Data((char *)data, size);
    d->ownsData = false;

    if (validation != BypassValidation && !d->valid()) {
        delete d;
        return QJsonDocument();
    }

    return QJsonDocument(d);
}

/*!
  Returns the raw binary representation of the data
  \a size will contain the size of the returned data.

  This method is useful to e.g. stream the JSON document
  in it's binary form to a file.
 */
const char *QJsonDocument::rawData(int *size) const
{
    if (!d) {
        *size = 0;
        return 0;
    }
    *size = d->alloc;
    return d->rawData;
}

/*!
 Creates a QJsonDocument from \a data.

 \a validation decides whether the data is checked for validity before being used.
 By default the data is validated. If the \a data is not valid, the method returns
 a null document.

 \sa toBinaryData(), fromRawData(), isNull(), DataValidation
 */
QJsonDocument QJsonDocument::fromBinaryData(const QByteArray &data, DataValidation validation)
{
    if (data.size() < (int)(sizeof(QJsonPrivate::Header) + sizeof(QJsonPrivate::Base)))
        return QJsonDocument();

    QJsonPrivate::Header h;
    memcpy(&h, data.constData(), sizeof(QJsonPrivate::Header));
    QJsonPrivate::Base root;
    memcpy(&root, data.constData() + sizeof(QJsonPrivate::Header), sizeof(QJsonPrivate::Base));

    // do basic checks here, so we don't try to allocate more memory than we can.
    if (h.tag != QJsonDocument::BinaryFormatTag || h.version != 1u ||
        sizeof(QJsonPrivate::Header) + root.size > (uint)data.size())
        return QJsonDocument();

    const uint size = sizeof(QJsonPrivate::Header) + root.size;
    char *raw = (char *)malloc(size);
    if (!raw)
        return QJsonDocument();

    memcpy(raw, data.constData(), size);
    QJsonPrivate::Data *d = new QJsonPrivate::Data(raw, size);

    if (validation != BypassValidation && !d->valid()) {
        delete d;
        return QJsonDocument();
    }

    return QJsonDocument(d);
}

/*!
 Creates a QJsonDocument from the QVariant \a variant.

 If the \a variant contains any other type than a QVariantMap,
 QVariantList or QStringList, the returned document
 document is invalid.

 \sa toVariant()
 */
QJsonDocument QJsonDocument::fromVariant(const QVariant &variant)
{
    QJsonDocument doc;
    if (variant.type() == QVariant::Map) {
        doc.setObject(QJsonObject::fromVariantMap(variant.toMap()));
    } else if (variant.type() == QVariant::List) {
        doc.setArray(QJsonArray::fromVariantList(variant.toList()));
    } else if (variant.type() == QVariant::StringList) {
        doc.setArray(QJsonArray::fromStringList(variant.toStringList()));
    }
    return doc;
}

/*!
 Returns a QVariant representing the Json document.

 The returned variant will be a QVariantList if the document is
 a QJsonArray and a QVariantMap if the document is a QJsonObject.

 \sa fromVariant(), QJsonValue::toVariant()
 */
QVariant QJsonDocument::toVariant() const
{
    if (!d)
        return QVariant();

    if (d->header->root()->isArray())
        return QJsonArray(d, static_cast<QJsonPrivate::Array *>(d->header->root())).toVariantList();
    else
        return QJsonObject(d, static_cast<QJsonPrivate::Object *>(d->header->root())).toVariantMap();
}

/*!
 Converts the QJsonDocument to a UTF-8 encoded JSON document.

 \sa fromJson()
 */
#ifndef QT_JSON_READONLY
QByteArray QJsonDocument::toJson() const
{
    return toJson(Indented);
}
#endif

/*!
    \enum QJsonDocument::JsonFormat

    This value defines the format of the JSON byte array produced
    when converting to a QJsonDocument using toJson().

    \value Indented Defines human readable output as follows:
        \code
        {
            "Array": [
                true,
                999,
                "string"
            ],
            "Key": "Value",
            "null": null
        }
        \endcode

    \value Compact Defines a compact output as follows:
        \code
        {"Array": [true,999,"string"],"Key": "Value","null": null}
        \endcode
  */

/*!
    Converts the QJsonDocument to a UTF-8 encoded JSON document in the provided \a format.

    \sa fromJson(), JsonFormat
 */
#ifndef QT_JSON_READONLY
QByteArray QJsonDocument::toJson(JsonFormat format) const
{
    if (!d)
        return QByteArray();

    QByteArray json;

    if (d->header->root()->isArray())
        QJsonPrivate::Writer::arrayToJson(static_cast<QJsonPrivate::Array *>(d->header->root()), json, 0, (format == Compact));
    else
        QJsonPrivate::Writer::objectToJson(static_cast<QJsonPrivate::Object *>(d->header->root()), json, 0, (format == Compact));

    return json;
}
#endif

/*!
 Parses a UTF-8 encoded JSON document and creates a QJsonDocument
 from it.

 \a json contains the json document to be parsed.

 The optional \a error variable can be used to pass in a QJsonParseError data
 structure that will contain information about possible errors encountered during
 parsing.

 \sa toJson(), QJsonParseError
 */
QJsonDocument QJsonDocument::fromJson(const QByteArray &json, QJsonParseError *error)
{
    QJsonPrivate::Parser parser(json.constData(), json.length());
    return parser.parse(error);
}

/*!
    Returns \c true if the document doesn't contain any data.
 */
bool QJsonDocument::isEmpty() const
{
    if (!d)
        return true;

    return false;
}

/*!
 Returns a binary representation of the document.

 The binary representation is also the native format used internally in Qt,
 and is very efficient and fast to convert to and from.

 The binary format can be stored on disk and interchanged with other applications
 or computers. fromBinaryData() can be used to convert it back into a
 JSON document.

 \sa fromBinaryData()
 */
QByteArray QJsonDocument::toBinaryData() const
{
    if (!d || !d->rawData)
        return QByteArray();

    return QByteArray(d->rawData, d->header->root()->size + sizeof(QJsonPrivate::Header));
}

/*!
    Returns \c true if the document contains an array.

    \sa array(), isObject()
 */
bool QJsonDocument::isArray() const
{
    if (!d)
        return false;

    QJsonPrivate::Header *h = (QJsonPrivate::Header *)d->rawData;
    return h->root()->isArray();
}

/*!
    Returns \c true if the document contains an object.

    \sa object(), isArray()
 */
bool QJsonDocument::isObject() const
{
    if (!d)
        return false;

    QJsonPrivate::Header *h = (QJsonPrivate::Header *)d->rawData;
    return h->root()->isObject();
}

/*!
    Returns the QJsonObject contained in the document.

    Returns an empty object if the document contains an
    array.

    \sa isObject(), array(), setObject()
 */
QJsonObject QJsonDocument::object() const
{
    if (d) {
        QJsonPrivate::Base *b = d->header->root();
        if (b->isObject())
            return QJsonObject(d, static_cast<QJsonPrivate::Object *>(b));
    }
    return QJsonObject();
}

/*!
    Returns the QJsonArray contained in the document.

    Returns an empty array if the document contains an
    object.

    \sa isArray(), object(), setArray()
 */
QJsonArray QJsonDocument::array() const
{
    if (d) {
        QJsonPrivate::Base *b = d->header->root();
        if (b->isArray())
            return QJsonArray(d, static_cast<QJsonPrivate::Array *>(b));
    }
    return QJsonArray();
}

/*!
    Sets \a object as the main object of this document.

    \sa setArray(), object()
 */
void QJsonDocument::setObject(const QJsonObject &object)
{
    if (d && !d->ref.deref())
        delete d;

    d = object.d;

    if (!d) {
        d = new QJsonPrivate::Data(0, QJsonValue::Object);
    } else if (d->compactionCounter || object.o != d->header->root()) {
        QJsonObject o(object);
        if (d->compactionCounter)
            o.compact();
        else
            o.detach();
        d = o.d;
        d->ref.ref();
        return;
    }
    d->ref.ref();
}

/*!
    Sets \a array as the main object of this document.

    \sa setObject(), array()
 */
void QJsonDocument::setArray(const QJsonArray &array)
{
    if (d && !d->ref.deref())
        delete d;

    d = array.d;

    if (!d) {
        d = new QJsonPrivate::Data(0, QJsonValue::Array);
    } else if (d->compactionCounter || array.a != d->header->root()) {
        QJsonArray a(array);
        if (d->compactionCounter)
            a.compact();
        else
            a.detach();
        d = a.d;
        d->ref.ref();
        return;
    }
    d->ref.ref();
}

/*!
    Returns \c true if the \a other document is equal to this document.
 */
bool QJsonDocument::operator==(const QJsonDocument &other) const
{
    if (d == other.d)
        return true;

    if (!d || !other.d)
        return false;

    if (d->header->root()->isArray() != other.d->header->root()->isArray())
        return false;

    if (d->header->root()->isObject())
        return QJsonObject(d, static_cast<QJsonPrivate::Object *>(d->header->root()))
                == QJsonObject(other.d, static_cast<QJsonPrivate::Object *>(other.d->header->root()));
    else
        return QJsonArray(d, static_cast<QJsonPrivate::Array *>(d->header->root()))
                == QJsonArray(other.d, static_cast<QJsonPrivate::Array *>(other.d->header->root()));
}

/*!
 \fn bool QJsonDocument::operator!=(const QJsonDocument &other) const

    returns \c true if \a other is not equal to this document
 */

/*!
    returns \c true if this document is null.

    Null documents are documents created through the default constructor.

    Documents created from UTF-8 encoded text or the binary format are
    validated during parsing. If validation fails, the returned document
    will also be null.
 */
bool QJsonDocument::isNull() const
{
    return (d == 0);
}

#if !defined(QT_NO_DEBUG_STREAM) && !defined(QT_JSON_READONLY)
QDebug operator<<(QDebug dbg, const QJsonDocument &o)
{
    if (!o.d) {
        dbg << "QJsonDocument()";
        return dbg;
    }
    QByteArray json;
    if (o.d->header->root()->isArray())
        QJsonPrivate::Writer::arrayToJson(static_cast<QJsonPrivate::Array *>(o.d->header->root()), json, 0, true);
    else
        QJsonPrivate::Writer::objectToJson(static_cast<QJsonPrivate::Object *>(o.d->header->root()), json, 0, true);
    dbg.nospace() << "QJsonDocument("
                  << json.constData() // print as utf-8 string without extra quotation marks
                  << ")";
    return dbg.space();
}
#endif

QT_END_NAMESPACE
