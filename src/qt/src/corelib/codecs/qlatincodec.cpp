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

#include "qlatincodec_p.h"
#include "qlist.h"

#ifndef QT_NO_TEXTCODEC

QT_BEGIN_NAMESPACE

QLatin1Codec::~QLatin1Codec()
{
}

QString QLatin1Codec::convertToUnicode(const char *chars, int len, ConverterState *) const
{
    if (chars == 0)
        return QString();

    return QString::fromLatin1(chars, len);
}


QByteArray QLatin1Codec::convertFromUnicode(const QChar *ch, int len, ConverterState *state) const
{
    const char replacement = (state && state->flags & ConvertInvalidToNull) ? 0 : '?';
    QByteArray r(len, Qt::Uninitialized);
    char *d = r.data();
    int invalid = 0;
    for (int i = 0; i < len; ++i) {
        if (ch[i] > 0xff) {
            d[i] = replacement;
            ++invalid;
        } else {
            d[i] = (char)ch[i].cell();
        }
    }
    if (state) {
        state->invalidChars += invalid;
    }
    return r;
}

QByteArray QLatin1Codec::name() const
{
    return "ISO-8859-1";
}

QList<QByteArray> QLatin1Codec::aliases() const
{
    QList<QByteArray> list;
    list << "latin1"
         << "CP819"
         << "IBM819"
         << "iso-ir-100"
         << "csISOLatin1";
    return list;
}


int QLatin1Codec::mibEnum() const
{
    return 4;
}


QLatin15Codec::~QLatin15Codec()
{
}

QString QLatin15Codec::convertToUnicode(const char* chars, int len, ConverterState *) const
{
    if (chars == 0)
        return QString();

    QString str = QString::fromLatin1(chars, len);
    QChar *uc = str.data();
    while(len--) {
        switch(uc->unicode()) {
            case 0xa4:
                *uc = 0x20ac;
                break;
            case 0xa6:
                *uc = 0x0160;
                break;
            case 0xa8:
                *uc = 0x0161;
                break;
            case 0xb4:
                *uc = 0x017d;
                break;
            case 0xb8:
                *uc = 0x017e;
                break;
            case 0xbc:
                *uc = 0x0152;
                break;
            case 0xbd:
                *uc = 0x0153;
                break;
            case 0xbe:
                *uc = 0x0178;
                break;
            default:
                break;
        }
        uc++;
    }
    return str;
}

QByteArray QLatin15Codec::convertFromUnicode(const QChar *in, int length, ConverterState *state) const
{
    const char replacement = (state && state->flags & ConvertInvalidToNull) ? 0 : '?';
    QByteArray r(length, Qt::Uninitialized);
    char *d = r.data();
    int invalid = 0;
    for (int i = 0; i < length; ++i) {
        uchar c;
        ushort uc = in[i].unicode();
        if (uc < 0x0100) {
            if (uc > 0xa3) {
                switch(uc) {
                case 0xa4:
                case 0xa6:
                case 0xa8:
                case 0xb4:
                case 0xb8:
                case 0xbc:
                case 0xbd:
                case 0xbe:
                    c = replacement;
                    ++invalid;
                    break;
                default:
                    c = (unsigned char) uc;
                    break;
                }
            } else {
                c = (unsigned char) uc;
            }
        } else {
            if (uc == 0x20ac)
                c = 0xa4;
            else if ((uc & 0xff00) == 0x0100) {
                switch(uc) {
                case 0x0160:
                    c = 0xa6;
                    break;
                case 0x0161:
                    c = 0xa8;
                    break;
                case 0x017d:
                    c = 0xb4;
                    break;
                case 0x017e:
                    c = 0xb8;
                    break;
                case 0x0152:
                    c = 0xbc;
                    break;
                case 0x0153:
                    c = 0xbd;
                    break;
                case 0x0178:
                    c = 0xbe;
                    break;
                default:
                    c = replacement;
                    ++invalid;
                }
            } else {
                c = replacement;
                ++invalid;
            }
        }
        d[i] = (char)c;
    }
    if (state) {
        state->remainingChars = 0;
        state->invalidChars += invalid;
    }
    return r;
}


QByteArray QLatin15Codec::name() const
{
    return "ISO-8859-15";
}

QList<QByteArray> QLatin15Codec::aliases() const
{
    QList<QByteArray> list;
    list << "latin9";
    return list;
}

int QLatin15Codec::mibEnum() const
{
    return 111;
}

QT_END_NAMESPACE

#endif // QT_NO_TEXTCODEC
