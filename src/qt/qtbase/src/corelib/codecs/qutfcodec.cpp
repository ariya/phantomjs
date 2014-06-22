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

#include "qutfcodec_p.h"
#include "qlist.h"
#include "qendian.h"
#include "qchar.h"

#include "private/qsimd_p.h"
#include "private/qstringiterator_p.h"

QT_BEGIN_NAMESPACE

enum { Endian = 0, Data = 1 };

static const uchar utf8bom[] = { 0xef, 0xbb, 0xbf };

#if defined(__SSE2__) && defined(QT_COMPILER_SUPPORTS_SSE2)
static inline bool simdEncodeAscii(uchar *&dst, const ushort *&nextAscii, const ushort *&src, const ushort *end)
{
    // do sixteen characters at a time
    for ( ; end - src >= 16; src += 16, dst += 16) {
        __m128i data1 = _mm_loadu_si128((__m128i*)src);
        __m128i data2 = _mm_loadu_si128(1+(__m128i*)src);


        // check if everything is ASCII
        // the highest ASCII value is U+007F
        // Do the packing directly:
        // The PACKUSWB instruction has packs a signed 16-bit integer to an unsigned 8-bit
        // with saturation. That is, anything from 0x0100 to 0x7fff is saturated to 0xff,
        // while all negatives (0x8000 to 0xffff) get saturated to 0x00. To detect non-ASCII,
        // we simply do a signed greater-than comparison to 0x00. That means we detect NULs as
        // "non-ASCII", but it's an acceptable compromise.
        __m128i packed = _mm_packus_epi16(data1, data2);
        __m128i nonAscii = _mm_cmpgt_epi8(packed, _mm_setzero_si128());

        // n will contain 1 bit set per character in [data1, data2] that is non-ASCII (or NUL)
        ushort n = ~_mm_movemask_epi8(nonAscii);
        if (n) {
            // copy the front part that is still ASCII
            while (!(n & 1)) {
                *dst++ = *src++;
                n >>= 1;
            }

            // find the next probable ASCII character
            // we don't want to load 32 bytes again in this loop if we know there are non-ASCII
            // characters still coming
            n = _bit_scan_reverse(n);
            nextAscii = src + n + 1;
            return false;
        }

        // pack
        _mm_storeu_si128((__m128i*)dst, packed);
    }
    return src == end;
}

static inline bool simdDecodeAscii(ushort *&dst, const uchar *&nextAscii, const uchar *&src, const uchar *end)
{
    // do sixteen characters at a time
    for ( ; end - src >= 16; src += 16, dst += 16) {
        __m128i data = _mm_loadu_si128((__m128i*)src);

        // check if everything is ASCII
        // movemask extracts the high bit of every byte, so n is non-zero if something isn't ASCII
        uint n = _mm_movemask_epi8(data);
        if (n) {
            // copy the front part that is still ASCII
            while (!(n & 1)) {
                *dst++ = *src++;
                n >>= 1;
            }

            // find the next probable ASCII character
            // we don't want to load 16 bytes again in this loop if we know there are non-ASCII
            // characters still coming
            n = _bit_scan_reverse(n);
            nextAscii = src + n + 1;
            return false;
        }

        // unpack
        _mm_storeu_si128((__m128i*)dst, _mm_unpacklo_epi8(data, _mm_setzero_si128()));
        _mm_storeu_si128(1+(__m128i*)dst, _mm_unpackhi_epi8(data, _mm_setzero_si128()));
    }
    return src == end;
}
#else
static inline bool simdEncodeAscii(uchar *, const ushort *, const ushort *, const ushort *)
{
    return false;
}

static inline bool simdDecodeAscii(ushort *, const uchar *, const uchar *, const uchar *)
{
    return false;
}
#endif

QByteArray QUtf8::convertFromUnicode(const QChar *uc, int len)
{
    // create a QByteArray with the worst case scenario size
    QByteArray result(len * 3, Qt::Uninitialized);
    uchar *dst = reinterpret_cast<uchar *>(const_cast<char *>(result.constData()));
    const ushort *src = reinterpret_cast<const ushort *>(uc);
    const ushort *const end = src + len;

    while (src != end) {
        const ushort *nextAscii = end;
        if (simdEncodeAscii(dst, nextAscii, src, end))
            break;

        do {
            ushort uc = *src++;
            int res = QUtf8Functions::toUtf8<QUtf8BaseTraits>(uc, dst, src, end);
            if (res < 0) {
                // encoding error - append '?'
                *dst++ = '?';
            }
        } while (src < nextAscii);
    }

    result.truncate(dst - reinterpret_cast<uchar *>(const_cast<char *>(result.constData())));
    return result;
}

QByteArray QUtf8::convertFromUnicode(const QChar *uc, int len, QTextCodec::ConverterState *state)
{
    uchar replacement = '?';
    int rlen = 3*len;
    int surrogate_high = -1;
    if (state) {
        if (state->flags & QTextCodec::ConvertInvalidToNull)
            replacement = 0;
        if (!(state->flags & QTextCodec::IgnoreHeader))
            rlen += 3;
        if (state->remainingChars)
            surrogate_high = state->state_data[0];
    }


    QByteArray rstr(rlen, Qt::Uninitialized);
    uchar *cursor = reinterpret_cast<uchar *>(const_cast<char *>(rstr.constData()));
    const ushort *src = reinterpret_cast<const ushort *>(uc);
    const ushort *const end = src + len;

    int invalid = 0;
    if (state && !(state->flags & QTextCodec::IgnoreHeader)) {
        // append UTF-8 BOM
        *cursor++ = utf8bom[0];
        *cursor++ = utf8bom[1];
        *cursor++ = utf8bom[2];
    }

    const ushort *nextAscii = src;
    while (src != end) {
        int res;
        ushort uc;
        if (surrogate_high != -1) {
            uc = surrogate_high;
            surrogate_high = -1;
            res = QUtf8Functions::toUtf8<QUtf8BaseTraits>(uc, cursor, src, end);
        } else {
            if (src >= nextAscii && simdEncodeAscii(cursor, nextAscii, src, end))
                break;

            uc = *src++;
            res = QUtf8Functions::toUtf8<QUtf8BaseTraits>(uc, cursor, src, end);
        }
        if (Q_LIKELY(res >= 0))
            continue;

        if (res == QUtf8BaseTraits::Error) {
            // encoding error
            ++invalid;
            *cursor++ = replacement;
        } else if (res == QUtf8BaseTraits::EndOfString) {
            surrogate_high = uc;
            break;
        }
    }

    rstr.resize(cursor - (const uchar*)rstr.constData());
    if (state) {
        state->invalidChars += invalid;
        state->flags |= QTextCodec::IgnoreHeader;
        state->remainingChars = 0;
        if (surrogate_high >= 0) {
            state->remainingChars = 1;
            state->state_data[0] = surrogate_high;
        }
    }
    return rstr;
}

QString QUtf8::convertToUnicode(const char *chars, int len)
{
    // UTF-8 to UTF-16 always needs the exact same number of words or less:
    //    UTF-8     UTF-16
    //   1 byte     1 word
    //   2 bytes    1 word
    //   3 bytes    1 word
    //   4 bytes    2 words (one surrogate pair)
    // That is, we'll use the full buffer if the input is US-ASCII (1-byte UTF-8),
    // half the buffer for U+0080-U+07FF text (e.g., Greek, Cyrillic, Arabic) or
    // non-BMP text, and one third of the buffer for U+0800-U+FFFF text (e.g, CJK).
    //
    // The table holds for invalid sequences too: we'll insert one replacement char
    // per invalid byte.
    QString result(len, Qt::Uninitialized);

    ushort *dst = reinterpret_cast<ushort *>(const_cast<QChar *>(result.constData()));
    const uchar *src = reinterpret_cast<const uchar *>(chars);
    const uchar *end = src + len;

    // attempt to do a full decoding in SIMD
    const uchar *nextAscii = end;
    if (!simdDecodeAscii(dst, nextAscii, src, end)) {
        // at least one non-ASCII entry
        // check if we failed to decode the UTF-8 BOM; if so, skip it
        if (Q_UNLIKELY(src == reinterpret_cast<const uchar *>(chars))
                && end - src >= 3
                && Q_UNLIKELY(src[0] == utf8bom[0] && src[1] == utf8bom[1] && src[2] == utf8bom[2])) {
            src += 3;
        }

        while (src < end) {
            nextAscii = end;
            if (simdDecodeAscii(dst, nextAscii, src, end))
                break;

            do {
                uchar b = *src++;
                int res = QUtf8Functions::fromUtf8<QUtf8BaseTraits>(b, dst, src, end);
                if (res < 0) {
                    // decoding error
                    *dst++ = QChar::ReplacementCharacter;
                }
            } while (src < nextAscii);
        }
    }

    result.truncate(dst - reinterpret_cast<const ushort *>(result.constData()));
    return result;
}

QString QUtf8::convertToUnicode(const char *chars, int len, QTextCodec::ConverterState *state)
{
    bool headerdone = false;
    ushort replacement = QChar::ReplacementCharacter;
    int need = 0;
    int invalid = 0;
    int res;
    uchar ch = 0;

    // See above for buffer requirements for stateless decoding. However, that
    // fails if the state is not empty. The following situations can add to the
    // requirements:
    //  state contains      chars starts with           requirement
    //   1 of 2 bytes       valid continuation          0
    //   2 of 3 bytes       same                        0
    //   3 bytes of 4       same                        +1 (need to insert surrogate pair)
    //   1 of 2 bytes       invalid continuation        +1 (need to insert replacement and restart)
    //   2 of 3 bytes       same                        +1 (same)
    //   3 of 4 bytes       same                        +1 (same)
    QString result(need + len + 1, Qt::Uninitialized);

    ushort *dst = reinterpret_cast<ushort *>(const_cast<QChar *>(result.constData()));
    const uchar *src = reinterpret_cast<const uchar *>(chars);
    const uchar *end = src + len;

    if (state) {
        if (state->flags & QTextCodec::IgnoreHeader)
            headerdone = true;
        if (state->flags & QTextCodec::ConvertInvalidToNull)
            replacement = QChar::Null;
        if (state->remainingChars) {
            // handle incoming state first
            uchar remainingCharsData[4]; // longest UTF-8 sequence possible
            int remainingCharsCount = state->remainingChars;
            int newCharsToCopy = qMin<int>(sizeof(remainingCharsData) - remainingCharsCount, end - src);

            memset(remainingCharsData, 0, sizeof(remainingCharsData));
            memcpy(remainingCharsData, &state->state_data[0], remainingCharsCount);
            memcpy(remainingCharsData + remainingCharsCount, src, newCharsToCopy);

            const uchar *begin = &remainingCharsData[1];
            res = QUtf8Functions::fromUtf8<QUtf8BaseTraits>(remainingCharsData[0], dst, begin,
                    static_cast<const uchar *>(remainingCharsData) + remainingCharsCount + newCharsToCopy);
            if (res == QUtf8BaseTraits::Error || (res == QUtf8BaseTraits::EndOfString && len == 0)) {
                // special case for len == 0:
                // if we were supplied an empty string, terminate the previous, unfinished sequence with error
                ++invalid;
                *dst++ = replacement;
            } else if (res == QUtf8BaseTraits::EndOfString) {
                // if we got EndOfString again, then there were too few bytes in src;
                // copy to our state and return
                state->remainingChars = remainingCharsCount + newCharsToCopy;
                memcpy(&state->state_data[0], remainingCharsData, state->remainingChars);
                return QString();
            } else if (!headerdone && res >= 0) {
                // eat the UTF-8 BOM
                headerdone = true;
                if (dst[-1] == 0xfeff)
                    --dst;
            }

            // adjust src now that we have maybe consumed a few chars
            if (res >= 0) {
                Q_ASSERT(res > remainingCharsCount);
                src += res - remainingCharsCount;
            }
        }
    }

    // main body, stateless decoding
    res = 0;
    const uchar *nextAscii = src;
    while (res >= 0 && src < end) {
        if (src >= nextAscii && simdDecodeAscii(dst, nextAscii, src, end))
            break;

        ch = *src++;
        res = QUtf8Functions::fromUtf8<QUtf8BaseTraits>(ch, dst, src, end);
        if (!headerdone && res >= 0) {
            headerdone = true;
            // eat the UTF-8 BOM
            if (dst[-1] == 0xfeff)
                --dst;
        }
        if (res == QUtf8BaseTraits::Error) {
            res = 0;
            ++invalid;
            *dst++ = replacement;
        }
    }

    if (!state && res == QUtf8BaseTraits::EndOfString) {
        // unterminated UTF sequence
        *dst++ = QChar::ReplacementCharacter;
        while (src++ < end)
            *dst++ = QChar::ReplacementCharacter;
    }

    result.truncate(dst - (ushort *)result.unicode());
    if (state) {
        state->invalidChars += invalid;
        if (headerdone)
            state->flags |= QTextCodec::IgnoreHeader;
        if (res == QUtf8BaseTraits::EndOfString) {
            --src; // unread the byte in ch
            state->remainingChars = end - src;
            memcpy(&state->state_data[0], src, end - src);
        } else {
            state->remainingChars = 0;
        }
    }
    return result;
}

QByteArray QUtf16::convertFromUnicode(const QChar *uc, int len, QTextCodec::ConverterState *state, DataEndianness e)
{
    DataEndianness endian = e;
    int length =  2*len;
    if (!state || (!(state->flags & QTextCodec::IgnoreHeader))) {
        length += 2;
    }
    if (e == DetectEndianness) {
        endian = (QSysInfo::ByteOrder == QSysInfo::BigEndian) ? BigEndianness : LittleEndianness;
    }

    QByteArray d;
    d.resize(length);
    char *data = d.data();
    if (!state || !(state->flags & QTextCodec::IgnoreHeader)) {
        QChar bom(QChar::ByteOrderMark);
        if (endian == BigEndianness) {
            data[0] = bom.row();
            data[1] = bom.cell();
        } else {
            data[0] = bom.cell();
            data[1] = bom.row();
        }
        data += 2;
    }
    if (endian == BigEndianness) {
        for (int i = 0; i < len; ++i) {
            *(data++) = uc[i].row();
            *(data++) = uc[i].cell();
        }
    } else {
        for (int i = 0; i < len; ++i) {
            *(data++) = uc[i].cell();
            *(data++) = uc[i].row();
        }
    }

    if (state) {
        state->remainingChars = 0;
        state->flags |= QTextCodec::IgnoreHeader;
    }
    return d;
}

QString QUtf16::convertToUnicode(const char *chars, int len, QTextCodec::ConverterState *state, DataEndianness e)
{
    DataEndianness endian = e;
    bool half = false;
    uchar buf = 0;
    bool headerdone = false;
    if (state) {
        headerdone = state->flags & QTextCodec::IgnoreHeader;
        if (endian == DetectEndianness)
            endian = (DataEndianness)state->state_data[Endian];
        if (state->remainingChars) {
            half = true;
            buf = state->state_data[Data];
        }
    }
    if (headerdone && endian == DetectEndianness)
        endian = (QSysInfo::ByteOrder == QSysInfo::BigEndian) ? BigEndianness : LittleEndianness;

    QString result(len, Qt::Uninitialized); // worst case
    QChar *qch = (QChar *)result.unicode();
    while (len--) {
        if (half) {
            QChar ch;
            if (endian == LittleEndianness) {
                ch.setRow(*chars++);
                ch.setCell(buf);
            } else {
                ch.setRow(buf);
                ch.setCell(*chars++);
            }
            if (!headerdone) {
                headerdone = true;
                if (endian == DetectEndianness) {
                    if (ch == QChar::ByteOrderSwapped) {
                        endian = LittleEndianness;
                    } else if (ch == QChar::ByteOrderMark) {
                        endian = BigEndianness;
                    } else {
                        if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
                            endian = BigEndianness;
                        } else {
                            endian = LittleEndianness;
                            ch = QChar((ch.unicode() >> 8) | ((ch.unicode() & 0xff) << 8));
                        }
                        *qch++ = ch;
                    }
                } else if (ch != QChar::ByteOrderMark) {
                    *qch++ = ch;
                }
            } else {
                *qch++ = ch;
            }
            half = false;
        } else {
            buf = *chars++;
            half = true;
        }
    }
    result.truncate(qch - result.unicode());

    if (state) {
        if (headerdone)
            state->flags |= QTextCodec::IgnoreHeader;
        state->state_data[Endian] = endian;
        if (half) {
            state->remainingChars = 1;
            state->state_data[Data] = buf;
        } else {
            state->remainingChars = 0;
            state->state_data[Data] = 0;
        }
    }
    return result;
}

QByteArray QUtf32::convertFromUnicode(const QChar *uc, int len, QTextCodec::ConverterState *state, DataEndianness e)
{
    DataEndianness endian = e;
    int length =  4*len;
    if (!state || (!(state->flags & QTextCodec::IgnoreHeader))) {
        length += 4;
    }
    if (e == DetectEndianness) {
        endian = (QSysInfo::ByteOrder == QSysInfo::BigEndian) ? BigEndianness : LittleEndianness;
    }

    QByteArray d(length, Qt::Uninitialized);
    char *data = d.data();
    if (!state || !(state->flags & QTextCodec::IgnoreHeader)) {
        if (endian == BigEndianness) {
            data[0] = 0;
            data[1] = 0;
            data[2] = (char)0xfe;
            data[3] = (char)0xff;
        } else {
            data[0] = (char)0xff;
            data[1] = (char)0xfe;
            data[2] = 0;
            data[3] = 0;
        }
        data += 4;
    }

    QStringIterator i(uc, uc + len);
    if (endian == BigEndianness) {
        while (i.hasNext()) {
            uint cp = i.next();

            *(data++) = cp >> 24;
            *(data++) = (cp >> 16) & 0xff;
            *(data++) = (cp >> 8) & 0xff;
            *(data++) = cp & 0xff;
        }
    } else {
        while (i.hasNext()) {
            uint cp = i.next();

            *(data++) = cp & 0xff;
            *(data++) = (cp >> 8) & 0xff;
            *(data++) = (cp >> 16) & 0xff;
            *(data++) = cp >> 24;
        }
    }

    if (state) {
        state->remainingChars = 0;
        state->flags |= QTextCodec::IgnoreHeader;
    }
    return d;
}

QString QUtf32::convertToUnicode(const char *chars, int len, QTextCodec::ConverterState *state, DataEndianness e)
{
    DataEndianness endian = e;
    uchar tuple[4];
    int num = 0;
    bool headerdone = false;
    if (state) {
        headerdone = state->flags & QTextCodec::IgnoreHeader;
        if (endian == DetectEndianness) {
            endian = (DataEndianness)state->state_data[Endian];
        }
        num = state->remainingChars;
        memcpy(tuple, &state->state_data[Data], 4);
    }
    if (headerdone && endian == DetectEndianness)
        endian = (QSysInfo::ByteOrder == QSysInfo::BigEndian) ? BigEndianness : LittleEndianness;

    QString result;
    result.resize((num + len) >> 2 << 1); // worst case
    QChar *qch = (QChar *)result.unicode();

    const char *end = chars + len;
    while (chars < end) {
        tuple[num++] = *chars++;
        if (num == 4) {
            if (!headerdone) {
                if (endian == DetectEndianness) {
                    if (tuple[0] == 0xff && tuple[1] == 0xfe && tuple[2] == 0 && tuple[3] == 0 && endian != BigEndianness) {
                        endian = LittleEndianness;
                        num = 0;
                        continue;
                    } else if (tuple[0] == 0 && tuple[1] == 0 && tuple[2] == 0xfe && tuple[3] == 0xff && endian != LittleEndianness) {
                        endian = BigEndianness;
                        num = 0;
                        continue;
                    } else if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
                        endian = BigEndianness;
                    } else {
                        endian = LittleEndianness;
                    }
                } else if (((endian == BigEndianness) ? qFromBigEndian<quint32>(tuple) : qFromLittleEndian<quint32>(tuple)) == QChar::ByteOrderMark) {
                    num = 0;
                    continue;
                }
            }
            uint code = (endian == BigEndianness) ? qFromBigEndian<quint32>(tuple) : qFromLittleEndian<quint32>(tuple);
            if (QChar::requiresSurrogates(code)) {
                *qch++ = QChar::highSurrogate(code);
                *qch++ = QChar::lowSurrogate(code);
            } else {
                *qch++ = code;
            }
            num = 0;
        }
    }
    result.truncate(qch - result.unicode());

    if (state) {
        if (headerdone)
            state->flags |= QTextCodec::IgnoreHeader;
        state->state_data[Endian] = endian;
        state->remainingChars = num;
        memcpy(&state->state_data[Data], tuple, 4);
    }
    return result;
}


#ifndef QT_NO_TEXTCODEC

QUtf8Codec::~QUtf8Codec()
{
}

QByteArray QUtf8Codec::convertFromUnicode(const QChar *uc, int len, ConverterState *state) const
{
    return QUtf8::convertFromUnicode(uc, len, state);
}

void QUtf8Codec::convertToUnicode(QString *target, const char *chars, int len, ConverterState *state) const
{
    *target += QUtf8::convertToUnicode(chars, len, state);
}

QString QUtf8Codec::convertToUnicode(const char *chars, int len, ConverterState *state) const
{
    return QUtf8::convertToUnicode(chars, len, state);
}

QByteArray QUtf8Codec::name() const
{
    return "UTF-8";
}

int QUtf8Codec::mibEnum() const
{
    return 106;
}

QUtf16Codec::~QUtf16Codec()
{
}

QByteArray QUtf16Codec::convertFromUnicode(const QChar *uc, int len, ConverterState *state) const
{
    return QUtf16::convertFromUnicode(uc, len, state, e);
}

QString QUtf16Codec::convertToUnicode(const char *chars, int len, ConverterState *state) const
{
    return QUtf16::convertToUnicode(chars, len, state, e);
}

int QUtf16Codec::mibEnum() const
{
    return 1015;
}

QByteArray QUtf16Codec::name() const
{
    return "UTF-16";
}

QList<QByteArray> QUtf16Codec::aliases() const
{
    return QList<QByteArray>();
}

int QUtf16BECodec::mibEnum() const
{
    return 1013;
}

QByteArray QUtf16BECodec::name() const
{
    return "UTF-16BE";
}

QList<QByteArray> QUtf16BECodec::aliases() const
{
    QList<QByteArray> list;
    return list;
}

int QUtf16LECodec::mibEnum() const
{
    return 1014;
}

QByteArray QUtf16LECodec::name() const
{
    return "UTF-16LE";
}

QList<QByteArray> QUtf16LECodec::aliases() const
{
    QList<QByteArray> list;
    return list;
}

QUtf32Codec::~QUtf32Codec()
{
}

QByteArray QUtf32Codec::convertFromUnicode(const QChar *uc, int len, ConverterState *state) const
{
    return QUtf32::convertFromUnicode(uc, len, state, e);
}

QString QUtf32Codec::convertToUnicode(const char *chars, int len, ConverterState *state) const
{
    return QUtf32::convertToUnicode(chars, len, state, e);
}

int QUtf32Codec::mibEnum() const
{
    return 1017;
}

QByteArray QUtf32Codec::name() const
{
    return "UTF-32";
}

QList<QByteArray> QUtf32Codec::aliases() const
{
    QList<QByteArray> list;
    return list;
}

int QUtf32BECodec::mibEnum() const
{
    return 1018;
}

QByteArray QUtf32BECodec::name() const
{
    return "UTF-32BE";
}

QList<QByteArray> QUtf32BECodec::aliases() const
{
    QList<QByteArray> list;
    return list;
}

int QUtf32LECodec::mibEnum() const
{
    return 1019;
}

QByteArray QUtf32LECodec::name() const
{
    return "UTF-32LE";
}

QList<QByteArray> QUtf32LECodec::aliases() const
{
    QList<QByteArray> list;
    return list;
}

#endif //QT_NO_TEXTCODEC

QT_END_NAMESPACE
