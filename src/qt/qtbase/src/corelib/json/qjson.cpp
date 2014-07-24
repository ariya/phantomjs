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

#include "qjson_p.h"
#include <qalgorithms.h>

QT_BEGIN_NAMESPACE

namespace QJsonPrivate
{

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
#define Q_TO_LITTLE_ENDIAN(x) (x)
#else
#define Q_TO_LITTLE_ENDIAN(x) ( ((x & 0xff) << 24) | ((x & 0xff00) << 8) | ((x & 0xff0000) >> 8) | ((x & 0xff000000) >> 24) )
#endif

static const Base emptyArray = { { Q_TO_LITTLE_ENDIAN(sizeof(Base)) }, { 0 }, { 0 } };
static const Base emptyObject = { { Q_TO_LITTLE_ENDIAN(sizeof(Base)) }, { 0 }, { 0 } };


void Data::compact()
{
    Q_ASSERT(sizeof(Value) == sizeof(offset));

    if (!compactionCounter)
        return;

    Base *base = header->root();
    int reserve = 0;
    if (base->is_object) {
        Object *o = static_cast<Object *>(base);
        for (int i = 0; i < (int)o->length; ++i)
            reserve += o->entryAt(i)->usedStorage(o);
    } else {
        Array *a = static_cast<Array *>(base);
        for (int i = 0; i < (int)a->length; ++i)
            reserve += (*a)[i].usedStorage(a);
    }

    int size = sizeof(Base) + reserve + base->length*sizeof(offset);
    int alloc = sizeof(Header) + size;
    Header *h = (Header *) malloc(alloc);
    h->tag = QJsonDocument::BinaryFormatTag;
    h->version = 1;
    Base *b = h->root();
    b->size = size;
    b->is_object = header->root()->is_object;
    b->length = base->length;
    b->tableOffset = reserve + sizeof(Array);

    int offset = sizeof(Base);
    if (b->is_object) {
        Object *o = static_cast<Object *>(base);
        Object *no = static_cast<Object *>(b);

        for (int i = 0; i < (int)o->length; ++i) {
            no->table()[i] = offset;

            const Entry *e = o->entryAt(i);
            Entry *ne = no->entryAt(i);
            int s = e->size();
            memcpy(ne, e, s);
            offset += s;
            int dataSize = e->value.usedStorage(o);
            if (dataSize) {
                memcpy((char *)no + offset, e->value.data(o), dataSize);
                ne->value.value = offset;
                offset += dataSize;
            }
        }
    } else {
        Array *a = static_cast<Array *>(base);
        Array *na = static_cast<Array *>(b);

        for (int i = 0; i < (int)a->length; ++i) {
            const Value &v = (*a)[i];
            Value &nv = (*na)[i];
            nv = v;
            int dataSize = v.usedStorage(a);
            if (dataSize) {
                memcpy((char *)na + offset, v.data(a), dataSize);
                nv.value = offset;
                offset += dataSize;
            }
        }
    }
    Q_ASSERT(offset == (int)b->tableOffset);

    free(header);
    header = h;
    this->alloc = alloc;
    compactionCounter = 0;
}

bool Data::valid() const
{
    if (header->tag != QJsonDocument::BinaryFormatTag || header->version != 1u)
        return false;

    bool res = false;
    if (header->root()->is_object)
        res = static_cast<Object *>(header->root())->isValid();
    else
        res = static_cast<Array *>(header->root())->isValid();

    return res;
}


int Base::reserveSpace(uint dataSize, int posInTable, uint numItems, bool replace)
{
    Q_ASSERT(posInTable >= 0 && posInTable <= (int)length);
    if (size + dataSize >= Value::MaxSize) {
        qWarning("QJson: Document too large to store in data structure %d %d %d", (uint)size, dataSize, Value::MaxSize);
        return 0;
    }

    offset off = tableOffset;
    // move table to new position
    if (replace) {
        memmove((char *)(table()) + dataSize, table(), length*sizeof(offset));
    } else {
        memmove((char *)(table() + posInTable + numItems) + dataSize, table() + posInTable, (length - posInTable)*sizeof(offset));
        memmove((char *)(table()) + dataSize, table(), posInTable*sizeof(offset));
    }
    tableOffset += dataSize;
    for (int i = 0; i < (int)numItems; ++i)
        table()[posInTable + i] = off;
    size += dataSize;
    if (!replace) {
        length += numItems;
        size += numItems * sizeof(offset);
    }
    return off;
}

void Base::removeItems(int pos, int numItems)
{
    Q_ASSERT(pos >= 0 && pos <= (int)length);
    if (pos + numItems < (int)length)
        memmove(table() + pos, table() + pos + numItems, (length - pos - numItems)*sizeof(offset));
    length -= numItems;
}

int Object::indexOf(const QString &key, bool *exists)
{
    int min = 0;
    int n = length;
    while (n > 0) {
        int half = n >> 1;
        int middle = min + half;
        if (*entryAt(middle) >= key) {
            n = half;
        } else {
            min = middle + 1;
            n -= half + 1;
        }
    }
    if (min < (int)length && *entryAt(min) == key) {
        *exists = true;
        return min;
    }
    *exists = false;
    return min;
}

bool Object::isValid() const
{
    if (tableOffset + length*sizeof(offset) > size)
        return false;

    for (uint i = 0; i < length; ++i) {
        offset entryOffset = table()[i];
        if (entryOffset + sizeof(Entry) >= tableOffset)
            return false;
        Entry *e = entryAt(i);
        int s = e->size();
        if (table()[i] + s > tableOffset)
            return false;
        if (!e->value.isValid(this))
            return false;
    }
    return true;
}



bool Array::isValid() const
{
    if (tableOffset + length*sizeof(offset) > size)
        return false;

    for (uint i = 0; i < length; ++i) {
        if (!at(i).isValid(this))
            return false;
    }
    return true;
}


bool Entry::operator ==(const QString &key) const
{
    if (value.latinKey)
        return (shallowLatin1Key() == key);
    else
        return (shallowKey() == key);
}

bool Entry::operator ==(const Entry &other) const
{
    if (value.latinKey) {
        if (other.value.latinKey)
            return shallowLatin1Key() == other.shallowLatin1Key();
        return shallowLatin1Key() == other.shallowKey();
    } else if (other.value.latinKey) {
        return shallowKey() == other.shallowLatin1Key();
    }
    return shallowKey() == other.shallowKey();
}

bool Entry::operator >=(const Entry &other) const
{
    if (value.latinKey) {
        if (other.value.latinKey)
            return shallowLatin1Key() >= other.shallowLatin1Key();
        return shallowLatin1Key() >= other.shallowKey();
    } else if (other.value.latinKey) {
        return shallowKey() >= other.shallowLatin1Key();
    }
    return shallowKey() >= other.shallowKey();
}


int Value::usedStorage(const Base *b) const
{
    int s = 0;
    switch (type) {
    case QJsonValue::Double:
        if (latinOrIntValue)
            break;
        s = sizeof(double);
        break;
    case QJsonValue::String: {
        char *d = data(b);
        if (latinOrIntValue)
            s = sizeof(ushort) + qFromLittleEndian(*(ushort *)d);
        else
            s = sizeof(int) + sizeof(ushort) * qFromLittleEndian(*(int *)d);
        break;
    }
    case QJsonValue::Array:
    case QJsonValue::Object:
        s = base(b)->size;
        break;
    case QJsonValue::Null:
    case QJsonValue::Bool:
    default:
        break;
    }
    return alignedSize(s);
}

bool Value::isValid(const Base *b) const
{
    int offset = 0;
    switch (type) {
    case QJsonValue::Double:
        if (latinOrIntValue)
            break;
        // fall through
    case QJsonValue::String:
    case QJsonValue::Array:
    case QJsonValue::Object:
        offset = value;
        break;
    case QJsonValue::Null:
    case QJsonValue::Bool:
    default:
        break;
    }

    if (!offset)
        return true;
    if (offset + sizeof(uint) > b->tableOffset)
        return false;

    int s = usedStorage(b);
    if (!s)
        return true;
    if (s < 0 || offset + s > (int)b->tableOffset)
        return false;
    if (type == QJsonValue::Array)
        return static_cast<Array *>(base(b))->isValid();
    if (type == QJsonValue::Object)
        return static_cast<Object *>(base(b))->isValid();
    return true;
}

/*!
    \internal
 */
int Value::requiredStorage(QJsonValue &v, bool *compressed)
{
    *compressed = false;
    switch (v.t) {
    case QJsonValue::Double:
        if (QJsonPrivate::compressedNumber(v.dbl) != INT_MAX) {
            *compressed = true;
            return 0;
        }
        return sizeof(double);
    case QJsonValue::String: {
        QString s = v.toString();
        *compressed = QJsonPrivate::useCompressed(s);
        return QJsonPrivate::qStringSize(s, *compressed);
    }
    case QJsonValue::Array:
    case QJsonValue::Object:
        if (v.d && v.d->compactionCounter) {
            v.detach();
            v.d->compact();
            v.base = static_cast<QJsonPrivate::Base *>(v.d->header->root());
        }
        return v.base ? v.base->size : sizeof(QJsonPrivate::Base);
    case QJsonValue::Undefined:
    case QJsonValue::Null:
    case QJsonValue::Bool:
        break;
    }
    return 0;
}

/*!
    \internal
 */
uint Value::valueToStore(const QJsonValue &v, uint offset)
{
    switch (v.t) {
    case QJsonValue::Undefined:
    case QJsonValue::Null:
        break;
    case QJsonValue::Bool:
        return v.b;
    case QJsonValue::Double: {
        int c = QJsonPrivate::compressedNumber(v.dbl);
        if (c != INT_MAX)
            return c;
    }
        // fall through
    case QJsonValue::String:
    case QJsonValue::Array:
    case QJsonValue::Object:
        return offset;
    }
    return 0;
}

/*!
    \internal
 */
void Value::copyData(const QJsonValue &v, char *dest, bool compressed)
{
    switch (v.t) {
    case QJsonValue::Double:
        if (!compressed) {
            qToLittleEndian(v.ui, (uchar *)dest);
        }
        break;
    case QJsonValue::String: {
        QString str = v.toString();
        QJsonPrivate::copyString(dest, str, compressed);
        break;
    }
    case QJsonValue::Array:
    case QJsonValue::Object: {
        const QJsonPrivate::Base *b = v.base;
        if (!b)
            b = (v.t == QJsonValue::Array ? &emptyArray : &emptyObject);
        memcpy(dest, b, b->size);
        break;
    }
    default:
        break;
    }
}

} // namespace QJsonPrivate

QT_END_NAMESPACE
