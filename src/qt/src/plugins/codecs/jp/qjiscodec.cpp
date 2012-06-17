/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

// Most of the code here was originally written by Serika Kurusugawa,
// a.k.a. Junji Takagi, and is included in Qt with the author's permission
// and the grateful thanks of the Qt team.

/*! \class QJisCodec
    \reentrant
    \internal
*/

#include "qjiscodec.h"
#include "qlist.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_TEXTCODEC
enum {
    Esc = 0x1b,
    So = 0x0e,         // Shift Out
    Si = 0x0f,         // Shift In

    ReverseSolidus = 0x5c,
    YenSign = 0x5c,
    Tilde = 0x7e,
    Overline = 0x7e
};

#define        IsKana(c)        (((c) >= 0xa1) && ((c) <= 0xdf))
#define        IsJisChar(c)        (((c) >= 0x21) && ((c) <= 0x7e))

#define        QValidChar(u)        ((u) ? QChar((ushort)(u)) : QChar(QChar::ReplacementCharacter))

enum Iso2022State{ Ascii, MinState = Ascii,
                   JISX0201_Latin, JISX0201_Kana,
                   JISX0208_1978, JISX0208_1983,
                   JISX0212, MaxState = JISX0212,
                   UnknownState };

static const char Esc_CHARS[] = "()*+-./";

static const char Esc_Ascii[]                 = {Esc, '(', 'B', 0 };
static const char Esc_JISX0201_Latin[]        = {Esc, '(', 'J', 0 };
static const char Esc_JISX0201_Kana[]        = {Esc, '(', 'I', 0 };
static const char Esc_JISX0208_1978[]        = {Esc, '$', '@', 0 };
static const char Esc_JISX0208_1983[]        = {Esc, '$', 'B', 0 };
static const char Esc_JISX0212[]        = {Esc, '$', '(', 'D', 0 };
static const char * const Esc_SEQ[] = { Esc_Ascii,
                                        Esc_JISX0201_Latin,
                                        Esc_JISX0201_Kana,
                                        Esc_JISX0208_1978,
                                        Esc_JISX0208_1983,
                                        Esc_JISX0212 };

/*! \internal */
QJisCodec::QJisCodec() : conv(QJpUnicodeConv::newConverter(QJpUnicodeConv::Default))
{
}


/*! \internal */
QJisCodec::~QJisCodec()
{
    delete (QJpUnicodeConv*)conv;
    conv = 0;
}

QByteArray QJisCodec::convertFromUnicode(const QChar *uc, int len, ConverterState *cs) const
{
    char replacement = '?';
    if (cs) {
        if (cs->flags & ConvertInvalidToNull)
            replacement = 0;
    }
    int invalid = 0;

    QByteArray result;
    Iso2022State state = Ascii;
    Iso2022State prev = Ascii;
    for (int i = 0; i < len; i++) {
        QChar ch = uc[i];
        uint j;
        if (ch.row() == 0x00 && ch.cell() < 0x80) {
            // Ascii
            if (state != JISX0201_Latin ||
                ch.cell() == ReverseSolidus || ch.cell() == Tilde) {
                state = Ascii;
            }
            j = ch.cell();
        } else if ((j = conv->unicodeToJisx0201(ch.row(), ch.cell())) != 0) {
            if (j < 0x80) {
                // JIS X 0201 Latin
                if (state != Ascii ||
                    ch.cell() == YenSign || ch.cell() == Overline) {
                    state = JISX0201_Latin;
                }
            } else {
                // JIS X 0201 Kana
                state = JISX0201_Kana;
                j &= 0x7f;
            }
        } else if ((j = conv->unicodeToJisx0208(ch.row(), ch.cell())) != 0) {
            // JIS X 0208
            state = JISX0208_1983;
        } else if ((j = conv->unicodeToJisx0212(ch.row(), ch.cell())) != 0) {
            // JIS X 0212
            state = JISX0212;
        } else {
            // Invalid
            state = UnknownState;
            j = replacement;
            ++invalid;
        }
        if (state != prev) {
            if (state == UnknownState) {
                result += Esc_Ascii;
            } else {
                result += Esc_SEQ[state - MinState];
            }
            prev = state;
        }
        if (j < 0x0100) {
            result += j & 0xff;
        } else {
            result += (j >> 8) & 0xff;
            result += j & 0xff;
        }
    }
    if (prev != Ascii) {
        result += Esc_Ascii;
    }

    if (cs) {
        cs->invalidChars += invalid;
    }
    return result;
}

QString QJisCodec::convertToUnicode(const char* chars, int len, ConverterState *cs) const
{
    uchar buf[4] = {0, 0, 0, 0};
    int nbuf = 0;
    Iso2022State state = Ascii, prev = Ascii;
    bool esc = false;
    QChar replacement = QChar::ReplacementCharacter;
    if (cs) {
        if (cs->flags & ConvertInvalidToNull)
            replacement = QChar::Null;
        nbuf = cs->remainingChars;
        buf[0] = (cs->state_data[0] >> 24) & 0xff;
        buf[1] = (cs->state_data[0] >> 16) & 0xff;
        buf[2] = (cs->state_data[0] >>  8) & 0xff;
        buf[3] = (cs->state_data[0] >>  0) & 0xff;
        state = (Iso2022State)((cs->state_data[1] >>  0) & 0xff);
        prev = (Iso2022State)((cs->state_data[1] >>  8) & 0xff);
        esc = cs->state_data[2];
    }
    int invalid = 0;

    QString result;
    for (int i=0; i<len; i++) {
        uchar ch = chars[i];
        if (esc) {
            // Escape sequence
            state = UnknownState;
            switch (nbuf) {
            case 0:
                if (ch == '$' || strchr(Esc_CHARS, ch)) {
                    buf[nbuf++] = ch;
                } else {
                    nbuf = 0;
                    esc = false;
                }
                break;
            case 1:
                if (buf[0] == '$') {
                    if (strchr(Esc_CHARS, ch)) {
                        buf[nbuf++] = ch;
                    } else {
                        switch (ch) {
                        case '@':
                            state = JISX0208_1978;        // Esc $ @
                            break;
                        case 'B':
                            state = JISX0208_1983;        // Esc $ B
                            break;
                        }
                        nbuf = 0;
                        esc = false;
                    }
                } else {
                    if (buf[0] == '(') {
                        switch (ch) {
                        case 'B':
                            state = Ascii;        // Esc (B
                            break;
                        case 'I':
                            state = JISX0201_Kana;        // Esc (I
                            break;
                        case 'J':
                            state = JISX0201_Latin;        // Esc (J
                            break;
                        }
                    }
                    nbuf = 0;
                    esc = false;
                }
                break;
            case 2:
                if (buf[1] == '(') {
                    switch (ch) {
                    case 'D':
                        state = JISX0212;        // Esc $ (D
                        break;
                    }
                }
                nbuf = 0;
                esc = false;
                break;
            }
        } else {
            if (ch == Esc) {
                // Escape sequence
                nbuf = 0;
                esc = true;
            } else if (ch == So) {
                // Shift out
                prev = state;
                state = JISX0201_Kana;
                nbuf = 0;
            } else if (ch == Si) {
                // Shift in
                if (prev == Ascii || prev == JISX0201_Latin) {
                    state = prev;
                } else {
                    state = Ascii;
                }
                nbuf = 0;
            } else {
                uint u;
                switch (nbuf) {
                case 0:
                    switch (state) {
                    case Ascii:
                        if (ch < 0x80) {
                            result += QLatin1Char(ch);
                            break;
                        }
                        /* fall through */
                    case JISX0201_Latin:
                        u = conv->jisx0201ToUnicode(ch);
                        result += QValidChar(u);
                        break;
                    case JISX0201_Kana:
                        u = conv->jisx0201ToUnicode(ch | 0x80);
                        result += QValidChar(u);
                        break;
                    case JISX0208_1978:
                    case JISX0208_1983:
                    case JISX0212:
                        buf[nbuf++] = ch;
                        break;
                    default:
                        result += QChar::ReplacementCharacter;
                        break;
                    }
                    break;
                case 1:
                    switch (state) {
                    case JISX0208_1978:
                    case JISX0208_1983:
                        u = conv->jisx0208ToUnicode(buf[0] & 0x7f, ch & 0x7f);
                        result += QValidChar(u);
                        break;
                    case JISX0212:
                        u = conv->jisx0212ToUnicode(buf[0] & 0x7f, ch & 0x7f);
                        result += QValidChar(u);
                        break;
                    default:
                        result += replacement;
                        ++invalid;
                        break;
                    }
                    nbuf = 0;
                    break;
                }
            }
        }
    }

    if (cs) {
        cs->remainingChars = nbuf;
        cs->invalidChars += invalid;
        cs->state_data[0] = (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
        cs->state_data[1] = (prev << 8) + state;
        cs->state_data[2] = esc;
    }

    return result;
}



/*! \internal */
int QJisCodec::_mibEnum()
{
    return 39;
}

/*! \internal */
QByteArray QJisCodec::_name()
{
    return "ISO-2022-JP";
}

/*!
    Returns the codec's mime name.
*/
QList<QByteArray> QJisCodec::_aliases()
{
    QList<QByteArray> list;
    list << "JIS7"; // Qt 3 compat
    return list;
}

#endif // QT_NO_TEXTCODEC

QT_END_NAMESPACE
