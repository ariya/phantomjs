/****************************************************************************
**
** Copyright (C) 2012 Intel Corporation
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

#include "qurl.h"
#include "private/qutfcodec_p.h"

QT_BEGIN_NAMESPACE

// ### move to qurl_p.h
enum EncodingAction {
    DecodeCharacter = 0,
    LeaveCharacter = 1,
    EncodeCharacter = 2
};

// From RFC 3896, Appendix A Collected ABNF for URI
//    unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
//    reserved      = gen-delims / sub-delims
//    gen-delims    = ":" / "/" / "?" / "#" / "[" / "]" / "@"
//    sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
//                  / "*" / "+" / "," / ";" / "="
static const uchar defaultActionTable[96] = {
    2, // space
    1, // '!' (sub-delim)
    2, // '"'
    1, // '#' (gen-delim)
    1, // '$' (gen-delim)
    2, // '%' (percent)
    1, // '&' (gen-delim)
    1, // "'" (sub-delim)
    1, // '(' (sub-delim)
    1, // ')' (sub-delim)
    1, // '*' (sub-delim)
    1, // '+' (sub-delim)
    1, // ',' (sub-delim)
    0, // '-' (unreserved)
    0, // '.' (unreserved)
    1, // '/' (gen-delim)

    0, 0, 0, 0, 0,  // '0' to '4' (unreserved)
    0, 0, 0, 0, 0,  // '5' to '9' (unreserved)
    1, // ':' (gen-delim)
    1, // ';' (sub-delim)
    2, // '<'
    1, // '=' (sub-delim)
    2, // '>'
    1, // '?' (gen-delim)

    1, // '@' (gen-delim)
    0, 0, 0, 0, 0,  // 'A' to 'E' (unreserved)
    0, 0, 0, 0, 0,  // 'F' to 'J' (unreserved)
    0, 0, 0, 0, 0,  // 'K' to 'O' (unreserved)
    0, 0, 0, 0, 0,  // 'P' to 'T' (unreserved)
    0, 0, 0, 0, 0, 0,  // 'U' to 'Z' (unreserved)
    1, // '[' (gen-delim)
    2, // '\'
    1, // ']' (gen-delim)
    2, // '^'
    0, // '_' (unreserved)

    2, // '`'
    0, 0, 0, 0, 0,  // 'a' to 'e' (unreserved)
    0, 0, 0, 0, 0,  // 'f' to 'j' (unreserved)
    0, 0, 0, 0, 0,  // 'k' to 'o' (unreserved)
    0, 0, 0, 0, 0,  // 'p' to 't' (unreserved)
    0, 0, 0, 0, 0, 0,  // 'u' to 'z' (unreserved)
    2, // '{'
    2, // '|'
    2, // '}'
    0, // '~' (unreserved)

    2  // BSKP
};

// mask tables, in negative polarity
// 0x00 if it belongs to this category
// 0xff if it doesn't

static const uchar reservedMask[96] = {
    0xff, // space
    0xff, // '!' (sub-delim)
    0x00, // '"'
    0xff, // '#' (gen-delim)
    0xff, // '$' (gen-delim)
    0xff, // '%' (percent)
    0xff, // '&' (gen-delim)
    0xff, // "'" (sub-delim)
    0xff, // '(' (sub-delim)
    0xff, // ')' (sub-delim)
    0xff, // '*' (sub-delim)
    0xff, // '+' (sub-delim)
    0xff, // ',' (sub-delim)
    0xff, // '-' (unreserved)
    0xff, // '.' (unreserved)
    0xff, // '/' (gen-delim)

    0xff, 0xff, 0xff, 0xff, 0xff,  // '0' to '4' (unreserved)
    0xff, 0xff, 0xff, 0xff, 0xff,  // '5' to '9' (unreserved)
    0xff, // ':' (gen-delim)
    0xff, // ';' (sub-delim)
    0x00, // '<'
    0xff, // '=' (sub-delim)
    0x00, // '>'
    0xff, // '?' (gen-delim)

    0xff, // '@' (gen-delim)
    0xff, 0xff, 0xff, 0xff, 0xff,  // 'A' to 'E' (unreserved)
    0xff, 0xff, 0xff, 0xff, 0xff,  // 'F' to 'J' (unreserved)
    0xff, 0xff, 0xff, 0xff, 0xff,  // 'K' to 'O' (unreserved)
    0xff, 0xff, 0xff, 0xff, 0xff,  // 'P' to 'T' (unreserved)
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  // 'U' to 'Z' (unreserved)
    0xff, // '[' (gen-delim)
    0x00, // '\'
    0xff, // ']' (gen-delim)
    0x00, // '^'
    0xff, // '_' (unreserved)

    0x00, // '`'
    0xff, 0xff, 0xff, 0xff, 0xff,  // 'a' to 'e' (unreserved)
    0xff, 0xff, 0xff, 0xff, 0xff,  // 'f' to 'j' (unreserved)
    0xff, 0xff, 0xff, 0xff, 0xff,  // 'k' to 'o' (unreserved)
    0xff, 0xff, 0xff, 0xff, 0xff,  // 'p' to 't' (unreserved)
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  // 'u' to 'z' (unreserved)
    0x00, // '{'
    0x00, // '|'
    0x00, // '}'
    0xff, // '~' (unreserved)

    0xff  // BSKP
};

static inline bool isHex(ushort c)
{
    return (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F') ||
            (c >= '0' && c <= '9');
}

static inline bool isUpperHex(ushort c)
{
    // undefined behaviour if c isn't an hex char!
    return c < 0x60;
}

static inline ushort toUpperHex(ushort c)
{
    return isUpperHex(c) ? c : c - 0x20;
}

static inline ushort decodeNibble(ushort c)
{
    return c >= 'a' ? c - 'a' + 0xA :
           c >= 'A' ? c - 'A' + 0xA : c - '0';
}

// if the sequence at input is 2*HEXDIG, returns its decoding
// returns -1 if it isn't.
// assumes that the range has been checked already
static inline ushort decodePercentEncoding(const ushort *input)
{
    ushort c1 = input[1];
    ushort c2 = input[2];
    if (!isHex(c1) || !isHex(c2))
        return ushort(-1);
    return decodeNibble(c1) << 4 | decodeNibble(c2);
}

static inline ushort encodeNibble(ushort c)
{
    static const uchar hexnumbers[] = "0123456789ABCDEF";
    return hexnumbers[c & 0xf];
}

static void ensureDetached(QString &result, ushort *&output, const ushort *begin, const ushort *input, const ushort *end,
                           int add = 0)
{
    if (!output) {
        // now detach
        // create enough space if the rest of the string needed to be percent-encoded
        int charsProcessed = input - begin;
        int charsRemaining = end - input;
        int spaceNeeded = end - begin + 2 * charsRemaining + add;
        int origSize = result.size();
        result.resize(origSize + spaceNeeded);

        // we know that resize() above detached, so we bypass the reference count check
        output = const_cast<ushort *>(reinterpret_cast<const ushort *>(result.constData()))
                 + origSize;

        // copy the chars we've already processed
        int i;
        for (i = 0; i < charsProcessed; ++i)
            output[i] = begin[i];
        output += i;
    }
}

namespace {
struct QUrlUtf8Traits : public QUtf8BaseTraitsNoAscii
{
    // override: our "bytes" are three percent-encoded UTF-16 characters
    static void appendByte(ushort *&ptr, uchar b)
    {
        // b >= 0x80, by construction, so percent-encode
        *ptr++ = '%';
        *ptr++ = encodeNibble(b >> 4);
        *ptr++ = encodeNibble(b & 0xf);
    }

    static uchar peekByte(const ushort *ptr, int n = 0)
    {
        // decodePercentEncoding returns ushort(-1) if it can't decode,
        // which means we return 0xff, which is not a valid continuation byte.
        // If ptr[i * 3] is not '%', we'll multiply by zero and return 0,
        // also not a valid continuation byte (if it's '%', we multiply by 1).
        return uchar(decodePercentEncoding(ptr + n * 3))
                * uchar(ptr[n * 3] == '%');
    }

    static qptrdiff availableBytes(const ushort *ptr, const ushort *end)
    {
        return (end - ptr) / 3;
    }

    static void advanceByte(const ushort *&ptr, int n = 1)
    {
        ptr += n * 3;
    }
};
}

// returns true if we performed an UTF-8 decoding
static bool encodedUtf8ToUtf16(QString &result, ushort *&output, const ushort *begin, const ushort *&input,
                               const ushort *end, ushort decoded)
{
    uint ucs4, *dst = &ucs4;
    const ushort *src = input + 3;// skip the %XX that yielded \a decoded
    int charsNeeded = QUtf8Functions::fromUtf8<QUrlUtf8Traits>(decoded, dst, src, end);
    if (charsNeeded < 0)
        return false;

    if (!QChar::requiresSurrogates(ucs4)) {
        // UTF-8 decoded and no surrogates are required
        // detach if necessary
        // possibilities are: 6 chars (%XX%XX) -> one char; 9 chars (%XX%XX%XX) -> one char
        ensureDetached(result, output, begin, input, end, -3 * charsNeeded + 1);
        *output++ = ucs4;
    } else {
        // UTF-8 decoded to something that requires a surrogate pair
        // compressing from %XX%XX%XX%XX (12 chars) to two
        ensureDetached(result, output, begin, input, end, -10);
        *output++ = QChar::highSurrogate(ucs4);
        *output++ = QChar::lowSurrogate(ucs4);
    }

    input = src - 1;
    return true;
}

static void unicodeToEncodedUtf8(QString &result, ushort *&output, const ushort *begin,
                                 const ushort *&input, const ushort *end, ushort decoded)
{
    // calculate the utf8 length and ensure enough space is available
    int utf8len = QChar::isHighSurrogate(decoded) ? 4 : decoded >= 0x800 ? 3 : 2;

    // detach
    if (!output) {
        // we need 3 * utf8len for the encoded UTF-8 sequence
        // but ensureDetached already adds 3 for the char we're processing
        ensureDetached(result, output, begin, input, end, 3*utf8len - 3);
    } else {
        // verify that there's enough space or expand
        int charsRemaining = end - input - 1; // not including this one
        int pos = output - reinterpret_cast<const ushort *>(result.constData());
        int spaceRemaining = result.size() - pos;
        if (spaceRemaining < 3*charsRemaining + 3*utf8len) {
            // must resize
            result.resize(result.size() + 3*utf8len);

            // we know that resize() above detached, so we bypass the reference count check
            output = const_cast<ushort *>(reinterpret_cast<const ushort *>(result.constData()));
            output += pos;
        }
    }

    ++input;
    int res = QUtf8Functions::toUtf8<QUrlUtf8Traits>(decoded, output, input, end);
    --input;
    if (res < 0) {
        // bad surrogate pair sequence
        // we will encode bad UTF-16 to UTF-8
        // but they don't get decoded back

        // first of three bytes
        uchar c = 0xe0 | uchar(decoded >> 12);
        *output++ = '%';
        *output++ = 'E';
        *output++ = encodeNibble(c & 0xf);

        // second byte
        c = 0x80 | (uchar(decoded >> 6) & 0x3f);
        *output++ = '%';
        *output++ = encodeNibble(c >> 4);
        *output++ = encodeNibble(c & 0xf);

        // third byte
        c = 0x80 | (decoded & 0x3f);
        *output++ = '%';
        *output++ = encodeNibble(c >> 4);
        *output++ = encodeNibble(c & 0xf);
    }
}

static int recode(QString &result, const ushort *begin, const ushort *end, QUrl::ComponentFormattingOptions encoding,
                  const uchar *actionTable, bool retryBadEncoding)
{
    const int origSize = result.size();
    const ushort *input = begin;
    ushort *output = 0;

    EncodingAction action = EncodeCharacter;
    for ( ; input != end; ++input) {
        ushort c;
        // try a run where no change is necessary
        for ( ; input != end; ++input) {
            c = *input;
            if (c < 0x20U)
                action = EncodeCharacter;
            if (c < 0x20U || c >= 0x80U) // also: (c - 0x20 < 0x60U)
                goto non_trivial;
            action = EncodingAction(actionTable[c - ' ']);
            if (action == EncodeCharacter)
                goto non_trivial;
            if (output)
                *output++ = c;
        }
        break;

non_trivial:
        uint decoded;
        if (c == '%' && retryBadEncoding) {
            // always write "%25"
            ensureDetached(result, output, begin, input, end);
            *output++ = '%';
            *output++ = '2';
            *output++ = '5';
            continue;
        } else if (c == '%') {
            // check if the input is valid
            if (input + 2 >= end || (decoded = decodePercentEncoding(input)) == ushort(-1)) {
                // not valid, retry
                result.resize(origSize);
                return recode(result, begin, end, encoding, actionTable, true);
            }

            if (decoded >= 0x80) {
                // decode the UTF-8 sequence
                if (!(encoding & QUrl::EncodeUnicode) &&
                        encodedUtf8ToUtf16(result, output, begin, input, end, decoded))
                    continue;

                // decoding the encoded UTF-8 failed
                action = LeaveCharacter;
            } else if (decoded >= 0x20) {
                action = EncodingAction(actionTable[decoded - ' ']);
            }
        } else {
            decoded = c;
            if (decoded >= 0x80 && encoding & QUrl::EncodeUnicode) {
                // encode the UTF-8 sequence
                unicodeToEncodedUtf8(result, output, begin, input, end, decoded);
                continue;
            } else if (decoded >= 0x80) {
                if (output)
                    *output++ = c;
                continue;
            }
        }

        // there are six possibilities:
        //  current \ action  | DecodeCharacter | LeaveCharacter | EncodeCharacter
        //      decoded       |    1:leave      |    2:leave     |    3:encode
        //      encoded       |    4:decode     |    5:leave     |    6:leave
        // cases 1 and 2 were handled before this section

        if (c == '%' && action != DecodeCharacter) {
            // cases 5 and 6: it's encoded and we're leaving it as it is
            // except we're pedantic and we'll uppercase the hex
            if (output || !isUpperHex(input[1]) || !isUpperHex(input[2])) {
                ensureDetached(result, output, begin, input, end);
                *output++ = '%';
                *output++ = toUpperHex(*++input);
                *output++ = toUpperHex(*++input);
            }
        } else if (c == '%' && action == DecodeCharacter) {
            // case 4: we need to decode
            ensureDetached(result, output, begin, input, end);
            *output++ = decoded;
            input += 2;
        } else {
            // must be case 3: we need to encode
            ensureDetached(result, output, begin, input, end);
            *output++ = '%';
            *output++ = encodeNibble(c >> 4);
            *output++ = encodeNibble(c & 0xf);
        }
    }

    if (output) {
        int len = output - reinterpret_cast<const ushort *>(result.constData());
        result.truncate(len);
        return len - origSize;
    }
    return 0;
}

/*!
    \since 5.0
    \internal

    This function decodes a percent-encoded string located from \a begin to \a
    end, by appending each character to \a appendTo. It returns the number of
    characters appended. Each percent-encoded sequence is decoded as follows:

    \list
      \li from %00 to %7F: the exact decoded value is appended;
      \li from %80 to %FF: QChar::ReplacementCharacter is appended;
      \li bad encoding: original input is copied to the output, undecoded.
    \endlist

    Given the above, it's important for the input to already have all UTF-8
    percent sequences decoded by qt_urlRecode (that is, the input should not
    have been processed with QUrl::EncodeUnicode).

    The input should also be a valid percent-encoded sequence (the output of
    qt_urlRecode is always valid).
*/
static int decode(QString &appendTo, const ushort *begin, const ushort *end)
{
    const int origSize = appendTo.size();
    const ushort *input = begin;
    ushort *output = 0;
    while (input != end) {
        if (*input != '%') {
            if (output)
                *output++ = *input;
            ++input;
            continue;
        }

        if (Q_UNLIKELY(end - input < 3 || !isHex(input[1]) || !isHex(input[2]))) {
            // badly-encoded data
            appendTo.resize(origSize + (end - begin));
            memcpy(appendTo.begin() + origSize, begin, (end - begin) * sizeof(ushort));
            return end - begin;
        }

        if (Q_UNLIKELY(!output)) {
            // detach
            appendTo.resize(origSize + (end - begin));
            output = reinterpret_cast<ushort *>(appendTo.begin()) + origSize;
            memcpy(output, begin, (input - begin) * sizeof(ushort));
            output += input - begin;
        }

        ++input;
        *output++ = decodeNibble(input[0]) << 4 | decodeNibble(input[1]);
        if (output[-1] >= 0x80)
            output[-1] = QChar::ReplacementCharacter;
        input += 2;
    }

    if (output) {
        int len = output - reinterpret_cast<ushort *>(appendTo.begin());
        appendTo.truncate(len);
        return len - origSize;
    }
    return 0;
}

template <size_t N>
static void maskTable(uchar (&table)[N], const uchar (&mask)[N])
{
    for (size_t i = 0; i < N; ++i)
        table[i] &= mask[i];
}

/*!
    \internal

    Recodes the string from \a begin to \a end. If any transformations are
    done, append them to \a appendTo and return the number of characters added.
    If no transformations were required, return 0.

    The \a encoding option modifies the default behaviour:
    \list
    \li QUrl::DecodeReserved: if set, reserved characters will be decoded;
                              if unset, reserved characters will be encoded
    \li QUrl::EncodeSpaces: if set, spaces will be encoded to "%20"; if unset, they will be " "
    \li QUrl::EncodeUnicode: if set, characters above U+0080 will be encoded to their UTF-8
                             percent-encoded form; if unset, they will be decoded to UTF-16
    \li QUrl::FullyDecoded: if set, this function will decode all percent-encoded sequences,
                            including that of the percent character. The resulting string
                            will not be percent-encoded anymore. Use with caution!
                            In this mode, the behaviour is undefined if the input string
                            contains any percent-encoding sequences above %80.
                            Also, the function will not correct bad % sequences.
    \endlist

    Other flags are ignored (including QUrl::EncodeReserved).

    The \a tableModifications argument can be used to supply extra
    modifications to the tables, to be applied after the flags above are
    handled. It consists of a sequence of 16-bit values, where the low 8 bits
    indicate the character in question and the high 8 bits are either \c
    EncodeCharacter, \c LeaveCharacter or \c DecodeCharacter.

    This function corrects percent-encoded errors by interpreting every '%' as
    meaning "%25" (all percents in the same content).
 */

Q_AUTOTEST_EXPORT int
qt_urlRecode(QString &appendTo, const QChar *begin, const QChar *end,
             QUrl::ComponentFormattingOptions encoding, const ushort *tableModifications)
{
    uchar actionTable[sizeof defaultActionTable];
    if (encoding == QUrl::FullyDecoded) {
        return decode(appendTo, reinterpret_cast<const ushort *>(begin), reinterpret_cast<const ushort *>(end));
    }

    memcpy(actionTable, defaultActionTable, sizeof actionTable);
    if (encoding & QUrl::DecodeReserved)
        maskTable(actionTable, reservedMask);
    if (!(encoding & QUrl::EncodeSpaces))
        actionTable[0] = DecodeCharacter; // decode

    if (tableModifications) {
        for (const ushort *p = tableModifications; *p; ++p)
            actionTable[uchar(*p) - ' '] = *p >> 8;
    }

    return recode(appendTo, reinterpret_cast<const ushort *>(begin), reinterpret_cast<const ushort *>(end),
                  encoding, actionTable, false);
}

/*!
    \internal
    \since 5.0

    \a ba contains an 8-bit form of the component and it might be
    percent-encoded already. We can't use QString::fromUtf8 because it might
    contain non-UTF8 sequences. We can't use QByteArray::toPercentEncoding
    because it might already contain percent-encoded sequences. We can't use
    qt_urlRecode because it needs UTF-16 input.
*/
Q_AUTOTEST_EXPORT
QString qt_urlRecodeByteArray(const QByteArray &ba)
{
    if (ba.isNull())
        return QString();

    // scan ba for anything above or equal to 0x80
    // control points below 0x20 are fine in QString
    const char *in = ba.constData();
    const char *const end = ba.constEnd();
    for ( ; in < end; ++in) {
        if (*in & 0x80)
            break;
    }

    if (in == end) {
        // no non-ASCII found, we're safe to convert to QString
        return QString::fromLatin1(ba, ba.size());
    }

    // we found something that we need to encode
    QByteArray intermediate = ba;
    intermediate.resize(ba.size() * 3 - (in - ba.constData()));
    uchar *out = reinterpret_cast<uchar *>(intermediate.data() + (in - ba.constData()));
    for ( ; in < end; ++in) {
        if (*in & 0x80) {
            // encode
            *out++ = '%';
            *out++ = encodeNibble(uchar(*in) >> 4);
            *out++ = encodeNibble(uchar(*in) & 0xf);
        } else {
            // keep
            *out++ = uchar(*in);
        }
    }

    // now it's safe to call fromLatin1
    return QString::fromLatin1(intermediate, out - reinterpret_cast<uchar *>(intermediate.data()));
}

QT_END_NAMESPACE
