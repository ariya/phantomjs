/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QJSONDOCUMENT_H
#define QJSONDOCUMENT_H

#include <QtCore/qjsonvalue.h>

QT_BEGIN_NAMESPACE

class QDebug;

namespace QJsonPrivate {
    class Parser;
}

struct Q_CORE_EXPORT QJsonParseError
{
    enum ParseError {
        NoError = 0,
        UnterminatedObject,
        MissingNameSeparator,
        UnterminatedArray,
        MissingValueSeparator,
        IllegalValue,
        TerminationByNumber,
        IllegalNumber,
        IllegalEscapeSequence,
        IllegalUTF8String,
        UnterminatedString,
        MissingObject,
        DeepNesting,
        DocumentTooLarge,
        GarbageAtEnd
    };

    QString    errorString() const;

    int        offset;
    ParseError error;
};

class Q_CORE_EXPORT QJsonDocument
{
public:
#ifdef Q_LITTLE_ENDIAN
    static const uint BinaryFormatTag = ('q') | ('b' << 8) | ('j' << 16) | ('s' << 24);
#else
    static const uint BinaryFormatTag = ('q' << 24) | ('b' << 16) | ('j' << 8) | ('s');
#endif

    QJsonDocument();
    explicit QJsonDocument(const QJsonObject &object);
    explicit QJsonDocument(const QJsonArray &array);
    ~QJsonDocument();

    QJsonDocument(const QJsonDocument &other);
    QJsonDocument &operator =(const QJsonDocument &other);

    enum DataValidation {
        Validate,
        BypassValidation
    };

    static QJsonDocument fromRawData(const char *data, int size, DataValidation validation = Validate);
    const char *rawData(int *size) const;

    static QJsonDocument fromBinaryData(const QByteArray &data, DataValidation validation  = Validate);
    QByteArray toBinaryData() const;

    static QJsonDocument fromVariant(const QVariant &variant);
    QVariant toVariant() const;

    enum JsonFormat {
        Indented,
        Compact
    };

    static QJsonDocument fromJson(const QByteArray &json, QJsonParseError *error = 0);

#ifdef Q_QDOC
    QByteArray toJson(JsonFormat format = Indented) const;
#elif !defined(QT_JSON_READONLY)
    QByteArray toJson() const; //### Merge in Qt6
    QByteArray toJson(JsonFormat format) const;
#endif

    bool isEmpty() const;
    bool isArray() const;
    bool isObject() const;

    QJsonObject object() const;
    QJsonArray array() const;

    void setObject(const QJsonObject &object);
    void setArray(const QJsonArray &array);

    bool operator==(const QJsonDocument &other) const;
    bool operator!=(const QJsonDocument &other) const { return !(*this == other); }

    bool isNull() const;

private:
    friend class QJsonValue;
    friend class QJsonPrivate::Data;
    friend class QJsonPrivate::Parser;
    friend Q_CORE_EXPORT QDebug operator<<(QDebug, const QJsonDocument &);

    QJsonDocument(QJsonPrivate::Data *data);

    QJsonPrivate::Data *d;
};

#if !defined(QT_NO_DEBUG_STREAM) && !defined(QT_JSON_READONLY)
Q_CORE_EXPORT QDebug operator<<(QDebug, const QJsonDocument &);
#endif

QT_END_NAMESPACE

#endif // QJSONDOCUMENT_H
