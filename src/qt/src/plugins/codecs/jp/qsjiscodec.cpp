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

// Most of the code here was originally written by Serika Kurusugawa
// a.k.a. Junji Takagi, and is included in Qt with the author's permission,
// and the grateful thanks of the Qt team.

/*! \class QSjisCodec
    \reentrant
    \internal
*/

#include "qsjiscodec.h"
#include "qlist.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_TEXTCODEC
enum {
    Esc = 0x1b
};

#define        IsKana(c)        (((c) >= 0xa1) && ((c) <= 0xdf))
#define        IsSjisChar1(c)        ((((c) >= 0x81) && ((c) <= 0x9f)) ||        \
                         (((c) >= 0xe0) && ((c) <= 0xfc)))
#define        IsSjisChar2(c)        (((c) >= 0x40) && ((c) != 0x7f) && ((c) <= 0xfc))
#define        IsUserDefinedChar1(c)        (((c) >= 0xf0) && ((c) <= 0xfc))

#define        QValidChar(u)        ((u) ? QChar((ushort)(u)) : QChar(QChar::ReplacementCharacter))

/*!
  Creates a Shift-JIS codec. Note that this is done automatically by
  the QApplication, you do not need construct your own.
*/
QSjisCodec::QSjisCodec() : conv(QJpUnicodeConv::newConverter(QJpUnicodeConv::Default))
{
}


/*!
  Destroys the Shift-JIS codec.
*/
QSjisCodec::~QSjisCodec()
{
    delete (QJpUnicodeConv*)conv;
    conv = 0;
}


QByteArray QSjisCodec::convertFromUnicode(const QChar *uc, int len, ConverterState *state) const
{
    char replacement = '?';
    if (state) {
        if (state->flags & ConvertInvalidToNull)
            replacement = 0;
    }
    int invalid = 0;

    int rlen = 2*len + 1;
    QByteArray rstr;
    rstr.resize(rlen);
    uchar* cursor = (uchar*)rstr.data();
    for (int i = 0; i < len; i++) {
        QChar ch = uc[i];
        uint j;
        if (ch.row() == 0x00 && ch.cell() < 0x80) {
            // ASCII
            *cursor++ = ch.cell();
        } else if ((j = conv->unicodeToJisx0201(ch.row(), ch.cell())) != 0) {
            // JIS X 0201 Latin or JIS X 0201 Kana
            *cursor++ = j;
        } else if ((j = conv->unicodeToSjis(ch.row(), ch.cell())) != 0) {
            // JIS X 0208
            *cursor++ = (j >> 8);
            *cursor++ = (j & 0xff);
        } else if ((j = conv->unicodeToSjisibmvdc(ch.row(), ch.cell())) != 0) {
            // JIS X 0208 IBM VDC
            *cursor++ = (j >> 8);
            *cursor++ = (j & 0xff);
        } else if ((j = conv->unicodeToCp932(ch.row(), ch.cell())) != 0) {
            // CP932 (for lead bytes 87, ee & ed)
            *cursor++ = (j >> 8);
            *cursor++ = (j & 0xff);
        } else if ((j = conv->unicodeToJisx0212(ch.row(), ch.cell())) != 0) {
            // JIS X 0212 (can't be encoded in ShiftJIS !)
            *cursor++ = 0x81;        // white square
            *cursor++ = 0xa0;        // white square
        } else {
            // Error
            *cursor++ = replacement;
            ++invalid;
        }
    }
    rstr.resize(cursor - (const uchar*)rstr.constData());

    if (state) {
        state->invalidChars += invalid;
    }
    return rstr;
}

QString QSjisCodec::convertToUnicode(const char* chars, int len, ConverterState *state) const
{
    uchar buf[1] = {0};
    int nbuf = 0;
    QChar replacement = QChar::ReplacementCharacter;
    if (state) {
        if (state->flags & ConvertInvalidToNull)
            replacement = QChar::Null;
        nbuf = state->remainingChars;
        buf[0] = state->state_data[0];
    }
    int invalid = 0;
    uint u= 0;
    QString result;
    for (int i=0; i<len; i++) {
        uchar ch = chars[i];
        switch (nbuf) {
        case 0:
            if (ch < 0x80) {
                result += QValidChar(ch);
            } else if (IsKana(ch)) {
                // JIS X 0201 Latin or JIS X 0201 Kana
                u = conv->jisx0201ToUnicode(ch);
                result += QValidChar(u);
            } else if (IsSjisChar1(ch)) {
                // JIS X 0208
                buf[0] = ch;
                nbuf = 1;
            } else {
                // Invalid
                result += replacement;
                ++invalid;
            }
            break;
        case 1:
            // JIS X 0208
            if (IsSjisChar2(ch)) {
                if ((u = conv->sjisibmvdcToUnicode(buf[0], ch))) {
                    result += QValidChar(u);
                } else if ((u = conv->cp932ToUnicode(buf[0], ch))) {
                    result += QValidChar(u);
                }
                else if (IsUserDefinedChar1(buf[0])) {
                    result += QChar::ReplacementCharacter;
                } else {
                    u = conv->sjisToUnicode(buf[0], ch);
                    result += QValidChar(u);
                }
            } else {
                // Invalid
                result += replacement;
                ++invalid;
            }
            nbuf = 0;
            break;
        }
    }

    if (state) {
        state->remainingChars = nbuf;
        state->state_data[0] = buf[0];
        state->invalidChars += invalid;
    }
    return result;
}


int QSjisCodec::_mibEnum()
{
    return 17;
}

QByteArray QSjisCodec::_name()
{
    return "Shift_JIS";
}

/*!
    Returns the codec's mime name.
*/
QList<QByteArray> QSjisCodec::_aliases()
{
    QList<QByteArray> list;
    list << "SJIS" // Qt 3 compat
         << "MS_Kanji";
    return list;
}
#endif // QT_NO_TEXTCODEC

QT_END_NAMESPACE
