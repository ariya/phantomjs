/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013 Intel Corporation
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

#include "qjsonwriter_p.h"
#include "qjson_p.h"
#include "private/qutfcodec_p.h"

QT_BEGIN_NAMESPACE

using namespace QJsonPrivate;

static void objectContentToJson(const QJsonPrivate::Object *o, QByteArray &json, int indent, bool compact);
static void arrayContentToJson(const QJsonPrivate::Array *a, QByteArray &json, int indent, bool compact);

static inline uchar hexdig(uint u)
{
    return (u < 0xa ? '0' + u : 'a' + u - 0xa);
}

static QByteArray escapedString(const QString &s)
{
    const uchar replacement = '?';
    QByteArray ba(s.length(), Qt::Uninitialized);

    uchar *cursor = reinterpret_cast<uchar *>(const_cast<char *>(ba.constData()));
    const uchar *ba_end = cursor + ba.length();
    const ushort *src = reinterpret_cast<const ushort *>(s.constBegin());
    const ushort *const end = reinterpret_cast<const ushort *>(s.constEnd());

    while (src != end) {
        if (cursor >= ba_end - 6) {
            // ensure we have enough space
            int pos = cursor - (const uchar *)ba.constData();
            ba.resize(ba.size()*2);
            cursor = (uchar *)ba.data() + pos;
            ba_end = (const uchar *)ba.constData() + ba.length();
        }

        uint u = *src++;
        if (u < 0x80) {
            if (u < 0x20 || u == 0x22 || u == 0x5c) {
                *cursor++ = '\\';
                switch (u) {
                case 0x22:
                    *cursor++ = '"';
                    break;
                case 0x5c:
                    *cursor++ = '\\';
                    break;
                case 0x8:
                    *cursor++ = 'b';
                    break;
                case 0xc:
                    *cursor++ = 'f';
                    break;
                case 0xa:
                    *cursor++ = 'n';
                    break;
                case 0xd:
                    *cursor++ = 'r';
                    break;
                case 0x9:
                    *cursor++ = 't';
                    break;
                default:
                    *cursor++ = 'u';
                    *cursor++ = '0';
                    *cursor++ = '0';
                    *cursor++ = hexdig(u>>4);
                    *cursor++ = hexdig(u & 0xf);
               }
            } else {
                *cursor++ = (uchar)u;
            }
        } else {
            if (QUtf8Functions::toUtf8<QUtf8BaseTraits>(u, cursor, src, end) < 0)
                *cursor++ = replacement;
        }
    }

    ba.resize(cursor - (const uchar *)ba.constData());
    return ba;
}

static void valueToJson(const QJsonPrivate::Base *b, const QJsonPrivate::Value &v, QByteArray &json, int indent, bool compact)
{
    QJsonValue::Type type = (QJsonValue::Type)(uint)v.type;
    switch (type) {
    case QJsonValue::Bool:
        json += v.toBoolean() ? "true" : "false";
        break;
    case QJsonValue::Double: {
        const double d = v.toDouble(b);
        if (qIsFinite(d)) // +2 to format to ensure the expected precision
            json += QByteArray::number(d, 'g', std::numeric_limits<double>::digits10 + 2); // ::digits10 is 15
        else
            json += "null"; // +INF || -INF || NaN (see RFC4627#section2.4)
        break;
    }
    case QJsonValue::String:
        json += '"';
        json += escapedString(v.toString(b));
        json += '"';
        break;
    case QJsonValue::Array:
        json += compact ? "[" : "[\n";
        arrayContentToJson(static_cast<QJsonPrivate::Array *>(v.base(b)), json, indent + (compact ? 0 : 1), compact);
        json += QByteArray(4*indent, ' ');
        json += "]";
        break;
    case QJsonValue::Object:
        json += compact ? "{" : "{\n";
        objectContentToJson(static_cast<QJsonPrivate::Object *>(v.base(b)), json, indent + (compact ? 0 : 1), compact);
        json += QByteArray(4*indent, ' ');
        json += "}";
        break;
    case QJsonValue::Null:
    default:
        json += "null";
    }
}

static void arrayContentToJson(const QJsonPrivate::Array *a, QByteArray &json, int indent, bool compact)
{
    if (!a || !a->length)
        return;

    QByteArray indentString(4*indent, ' ');

    uint i = 0;
    while (1) {
        json += indentString;
        valueToJson(a, a->at(i), json, indent, compact);

        if (++i == a->length) {
            if (!compact)
                json += '\n';
            break;
        }

        json += compact ? "," : ",\n";
    }
}


static void objectContentToJson(const QJsonPrivate::Object *o, QByteArray &json, int indent, bool compact)
{
    if (!o || !o->length)
        return;

    QByteArray indentString(4*indent, ' ');

    uint i = 0;
    while (1) {
        QJsonPrivate::Entry *e = o->entryAt(i);
        json += indentString;
        json += '"';
        json += escapedString(e->key());
        json += compact ? "\":" : "\": ";
        valueToJson(o, e->value, json, indent, compact);

        if (++i == o->length) {
            if (!compact)
                json += '\n';
            break;
        }

        json += compact ? "," : ",\n";
    }
}

void Writer::objectToJson(const QJsonPrivate::Object *o, QByteArray &json, int indent, bool compact)
{
    json.reserve(json.size() + (o ? (int)o->size : 16));
    json += compact ? "{" : "{\n";
    objectContentToJson(o, json, indent + (compact ? 0 : 1), compact);
    json += QByteArray(4*indent, ' ');
    json += compact ? "}" : "}\n";
}

void Writer::arrayToJson(const QJsonPrivate::Array *a, QByteArray &json, int indent, bool compact)
{
    json.reserve(json.size() + (a ? (int)a->size : 16));
    json += compact ? "[" : "[\n";
    arrayContentToJson(a, json, indent + (compact ? 0 : 1), compact);
    json += QByteArray(4*indent, ' ');
    json += compact ? "]" : "]\n";
}

QT_END_NAMESPACE
