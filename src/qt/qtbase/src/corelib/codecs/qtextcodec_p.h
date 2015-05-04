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

#ifndef QTEXTCODEC_P_H
#define QTEXTCODEC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QTextCodec class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "qtextcodec.h"
#include <string.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_TEXTCODEC

#if defined(Q_OS_MAC) || defined(Q_OS_ANDROID) || defined(Q_OS_QNX)
#define QT_LOCALE_IS_UTF8
#endif

typedef void (*QTextCodecStateFreeFunction)(QTextCodec::ConverterState*);

struct QTextCodecUnalignedPointer
{
    static inline QTextCodecStateFreeFunction decode(const uint *src)
    {
        quintptr data;
        memcpy(&data, src, sizeof(data));
        return reinterpret_cast<QTextCodecStateFreeFunction>(data);
    }
    static inline void encode(uint *dst, QTextCodecStateFreeFunction fn)
    {
        quintptr data = reinterpret_cast<quintptr>(fn);
        memcpy(dst, &data, sizeof(data));
    }
};

bool qTextCodecNameMatch(const char *a, const char *b);

#else

class QTextCodec
{
public:
    enum ConversionFlag {
        DefaultConversion,
        ConvertInvalidToNull = 0x80000000,
        IgnoreHeader = 0x1,
        FreeFunction = 0x2
    };
    Q_DECLARE_FLAGS(ConversionFlags, ConversionFlag)

    struct ConverterState {
        ConverterState(ConversionFlags f = DefaultConversion)
            : flags(f), remainingChars(0), invalidChars(0), d(0) { state_data[0] = state_data[1] = state_data[2] = 0; }
        ~ConverterState() { }
        ConversionFlags flags;
        int remainingChars;
        int invalidChars;
        uint state_data[3];
        void *d;
    private:
        Q_DISABLE_COPY(ConverterState)
    };
};

#endif //QT_NO_TEXTCODEC

QT_END_NAMESPACE

#endif
