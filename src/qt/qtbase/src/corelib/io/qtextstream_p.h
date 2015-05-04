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

#ifndef QTEXTSTREAM_P_H
#define QTEXTSTREAM_P_H

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

#include "qtextstream.h"
#ifndef QT_NO_TEXTCODEC
#include "qtextcodec.h"
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_QOBJECT
class QDeviceClosedNotifier : public QObject
{
    Q_OBJECT
public:
    inline QDeviceClosedNotifier()
    { }

    inline void setupDevice(QTextStream *stream, QIODevice *device)
    {
        disconnect();
        if (device)
            connect(device, SIGNAL(aboutToClose()), this, SLOT(flushStream()));
        this->stream = stream;
    }

public Q_SLOTS:
    inline void flushStream() { stream->flush(); }

private:
    QTextStream *stream;
};
#endif

class QTextStreamPrivate
{
    Q_DECLARE_PUBLIC(QTextStream)
public:
    // streaming parameters
    class Params
    {
    public:
        void reset();

        int realNumberPrecision;
        int integerBase;
        int fieldWidth;
        QChar padChar;
        QTextStream::FieldAlignment fieldAlignment;
        QTextStream::RealNumberNotation realNumberNotation;
        QTextStream::NumberFlags numberFlags;
    };

    QTextStreamPrivate(QTextStream *q_ptr);
    ~QTextStreamPrivate();
    void reset();

    // device
    QIODevice *device;
#ifndef QT_NO_QOBJECT
    QDeviceClosedNotifier deviceClosedNotifier;
#endif

    // string
    QString *string;
    int stringOffset;
    QIODevice::OpenMode stringOpenMode;

#ifndef QT_NO_TEXTCODEC
    // codec
    QTextCodec *codec;
    QTextCodec::ConverterState readConverterState;
    QTextCodec::ConverterState writeConverterState;
    QTextCodec::ConverterState *readConverterSavedState;
#endif

    QString writeBuffer;
    QString readBuffer;
    int readBufferOffset;
    int readConverterSavedStateOffset; //the offset between readBufferStartDevicePos and that start of the buffer
    qint64 readBufferStartDevicePos;

    Params params;

    // status
    QTextStream::Status status;
    QLocale locale;
    QTextStream *q_ptr;

    int lastTokenSize;
    bool deleteDevice;
#ifndef QT_NO_TEXTCODEC
    bool autoDetectUnicode;
#endif

    // i/o
    enum TokenDelimiter {
        Space,
        NotSpace,
        EndOfLine
    };

    QString read(int maxlen);
    bool scan(const QChar **ptr, int *tokenLength,
              int maxlen, TokenDelimiter delimiter);
    inline const QChar *readPtr() const;
    inline void consumeLastToken();
    inline void consume(int nchars);
    void saveConverterState(qint64 newPos);
    void restoreToSavedConverterState();

    // Return value type for getNumber()
    enum NumberParsingStatus {
        npsOk,
        npsMissingDigit,
        npsInvalidPrefix
    };

    inline bool getChar(QChar *ch);
    inline void ungetChar(QChar ch);
    NumberParsingStatus getNumber(qulonglong *l);
    bool getReal(double *f);

    inline void write(const QString &data);
    inline void putString(const QString &ch, bool number = false);
    void putNumber(qulonglong number, bool negative);

    // buffers
    bool fillReadBuffer(qint64 maxBytes = -1);
    void resetReadBuffer();
    void flushWriteBuffer();
};

QT_END_NAMESPACE

#endif // QTEXTSTREAM_P_H
