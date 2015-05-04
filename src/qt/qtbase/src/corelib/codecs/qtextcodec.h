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

#ifndef QTEXTCODEC_H
#define QTEXTCODEC_H

#include <QtCore/qstring.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_TEXTCODEC

class QTextCodec;
class QIODevice;

class QTextDecoder;
class QTextEncoder;

class Q_CORE_EXPORT QTextCodec
{
    Q_DISABLE_COPY(QTextCodec)
public:
    static QTextCodec* codecForName(const QByteArray &name);
    static QTextCodec* codecForName(const char *name) { return codecForName(QByteArray(name)); }
    static QTextCodec* codecForMib(int mib);

    static QList<QByteArray> availableCodecs();
    static QList<int> availableMibs();

    static QTextCodec* codecForLocale();
    static void setCodecForLocale(QTextCodec *c);

#if QT_DEPRECATED_SINCE(5, 0)
    QT_DEPRECATED static QTextCodec *codecForTr() { return codecForMib(106); /* Utf8 */ }
#endif

    static QTextCodec *codecForHtml(const QByteArray &ba);
    static QTextCodec *codecForHtml(const QByteArray &ba, QTextCodec *defaultCodec);

    static QTextCodec *codecForUtfText(const QByteArray &ba);
    static QTextCodec *codecForUtfText(const QByteArray &ba, QTextCodec *defaultCodec);

    bool canEncode(QChar) const;
    bool canEncode(const QString&) const;

    QString toUnicode(const QByteArray&) const;
    QString toUnicode(const char* chars) const;
    QByteArray fromUnicode(const QString& uc) const;
    enum ConversionFlag {
        DefaultConversion,
        ConvertInvalidToNull = 0x80000000,
        IgnoreHeader = 0x1,
        FreeFunction = 0x2
    };
    Q_DECLARE_FLAGS(ConversionFlags, ConversionFlag)

    struct Q_CORE_EXPORT ConverterState {
        ConverterState(ConversionFlags f = DefaultConversion)
            : flags(f), remainingChars(0), invalidChars(0), d(0) { state_data[0] = state_data[1] = state_data[2] = 0; }
        ~ConverterState();
        ConversionFlags flags;
        int remainingChars;
        int invalidChars;
        uint state_data[3];
        void *d;
    private:
        Q_DISABLE_COPY(ConverterState)
    };

    QString toUnicode(const char *in, int length, ConverterState *state = 0) const
        { return convertToUnicode(in, length, state); }
    QByteArray fromUnicode(const QChar *in, int length, ConverterState *state = 0) const
        { return convertFromUnicode(in, length, state); }

    QTextDecoder* makeDecoder(ConversionFlags flags = DefaultConversion) const;
    QTextEncoder* makeEncoder(ConversionFlags flags = DefaultConversion) const;

    virtual QByteArray name() const = 0;
    virtual QList<QByteArray> aliases() const;
    virtual int mibEnum() const = 0;

protected:
    virtual QString convertToUnicode(const char *in, int length, ConverterState *state) const = 0;
    virtual QByteArray convertFromUnicode(const QChar *in, int length, ConverterState *state) const = 0;

    QTextCodec();
    virtual ~QTextCodec();

private:
    friend struct QCoreGlobalData;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QTextCodec::ConversionFlags)

class Q_CORE_EXPORT QTextEncoder {
    Q_DISABLE_COPY(QTextEncoder)
public:
    explicit QTextEncoder(const QTextCodec *codec) : c(codec), state() {}
    QTextEncoder(const QTextCodec *codec, QTextCodec::ConversionFlags flags);
    ~QTextEncoder();
    QByteArray fromUnicode(const QString& str);
    QByteArray fromUnicode(const QChar *uc, int len);
    bool hasFailure() const;
private:
    const QTextCodec *c;
    QTextCodec::ConverterState state;
};

class Q_CORE_EXPORT QTextDecoder {
    Q_DISABLE_COPY(QTextDecoder)
public:
    explicit QTextDecoder(const QTextCodec *codec) : c(codec), state() {}
    QTextDecoder(const QTextCodec *codec, QTextCodec::ConversionFlags flags);
    ~QTextDecoder();
    QString toUnicode(const char* chars, int len);
    QString toUnicode(const QByteArray &ba);
    void toUnicode(QString *target, const char *chars, int len);
    bool hasFailure() const;
private:
    const QTextCodec *c;
    QTextCodec::ConverterState state;
};

#endif // QT_NO_TEXTCODEC

QT_END_NAMESPACE

#endif // QTEXTCODEC_H
