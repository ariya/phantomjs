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

#ifndef QICONVCODEC_P_H
#define QICONVCODEC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "qtextcodec.h"

#if defined(Q_OS_UNIX) && !defined(QT_NO_ICONV) && !defined(QT_BOOTSTRAPPED)

#ifdef Q_OS_MAC
typedef void * iconv_t;
#else
#include <iconv.h>
#endif

QT_BEGIN_NAMESPACE

class QIconvCodec: public QTextCodec
{
private:
    mutable QTextCodec *utf16Codec;

public:
    QIconvCodec();
    ~QIconvCodec();

    QString convertToUnicode(const char *, int, ConverterState *) const;
    QByteArray convertFromUnicode(const QChar *, int, ConverterState *) const;

    QByteArray name() const;
    int mibEnum() const;

    static iconv_t createIconv_t(const char *to, const char *from);

    class IconvState
    {
    public:
        IconvState(iconv_t x);
        ~IconvState();
        ConverterState internalState;
        char *buffer;
        int bufferLen;
        iconv_t cd;

        char array[8];

        void saveChars(const char *c, int count);
    };
};

QT_END_NAMESPACE

#endif // Q_OS_UNIX && !QT_NO_ICONV && !QT_BOOTSTRAPPED

#endif // QICONVCODEC_P_H
