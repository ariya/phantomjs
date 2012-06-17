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

#ifndef QUTFCODEC_P_H
#define QUTFCODEC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qtextcodec.h"
#include "private/qtextcodec_p.h"

QT_BEGIN_NAMESPACE

enum DataEndianness
{
    DetectEndianness,
    BigEndianness,
    LittleEndianness
};

struct QUtf8
{
    static QString convertToUnicode(const char *, int, QTextCodec::ConverterState *);
    static QByteArray convertFromUnicode(const QChar *, int, QTextCodec::ConverterState *);
};

struct QUtf16
{
    static QString convertToUnicode(const char *, int, QTextCodec::ConverterState *, DataEndianness = DetectEndianness);
    static QByteArray convertFromUnicode(const QChar *, int, QTextCodec::ConverterState *, DataEndianness = DetectEndianness);
};

struct QUtf32
{
    static QString convertToUnicode(const char *, int, QTextCodec::ConverterState *, DataEndianness = DetectEndianness);
    static QByteArray convertFromUnicode(const QChar *, int, QTextCodec::ConverterState *, DataEndianness = DetectEndianness);
};

#ifndef QT_NO_TEXTCODEC

class QUtf8Codec : public QTextCodec {
public:
    ~QUtf8Codec();

    QByteArray name() const;
    int mibEnum() const;

    QString convertToUnicode(const char *, int, ConverterState *) const;
    QByteArray convertFromUnicode(const QChar *, int, ConverterState *) const;
    void convertToUnicode(QString *target, const char *, int, ConverterState *) const;
};

class QUtf16Codec : public QTextCodec {
protected:
public:
    QUtf16Codec() { e = DetectEndianness; }
    ~QUtf16Codec();

    QByteArray name() const;
    QList<QByteArray> aliases() const;
    int mibEnum() const;

    QString convertToUnicode(const char *, int, ConverterState *) const;
    QByteArray convertFromUnicode(const QChar *, int, ConverterState *) const;

protected:
    DataEndianness e;
};

class QUtf16BECodec : public QUtf16Codec {
public:
    QUtf16BECodec() : QUtf16Codec() { e = BigEndianness; }
    QByteArray name() const;
    QList<QByteArray> aliases() const;
    int mibEnum() const;
};

class QUtf16LECodec : public QUtf16Codec {
public:
    QUtf16LECodec() : QUtf16Codec() { e = LittleEndianness; }
    QByteArray name() const;
    QList<QByteArray> aliases() const;
    int mibEnum() const;
};

class QUtf32Codec : public QTextCodec {
public:
    QUtf32Codec() { e = DetectEndianness; }
    ~QUtf32Codec();

    QByteArray name() const;
    QList<QByteArray> aliases() const;
    int mibEnum() const;

    QString convertToUnicode(const char *, int, ConverterState *) const;
    QByteArray convertFromUnicode(const QChar *, int, ConverterState *) const;

protected:
    DataEndianness e;
};

class QUtf32BECodec : public QUtf32Codec {
public:
    QUtf32BECodec() : QUtf32Codec() { e = BigEndianness; }
    QByteArray name() const;
    QList<QByteArray> aliases() const;
    int mibEnum() const;
};

class QUtf32LECodec : public QUtf32Codec {
public:
    QUtf32LECodec() : QUtf32Codec() { e = LittleEndianness; }
    QByteArray name() const;
    QList<QByteArray> aliases() const;
    int mibEnum() const;
};


#endif // QT_NO_TEXTCODEC

QT_END_NAMESPACE

#endif // QUTFCODEC_P_H
