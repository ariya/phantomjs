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

#include "qipaddress_p.h"
#include "private/qlocale_tools_p.h"
#include "qvarlengtharray.h"

QT_BEGIN_NAMESPACE
namespace QIPAddressUtils {

static QString number(quint8 val, int base = 10)
{
    QChar zero(0x30);
    return val ? qulltoa(val, base, zero) : zero;
}

typedef QVarLengthArray<char, 64> Buffer;
static const QChar *checkedToAscii(Buffer &buffer, const QChar *begin, const QChar *end)
{
    const ushort *const ubegin = reinterpret_cast<const ushort *>(begin);
    const ushort *const uend = reinterpret_cast<const ushort *>(end);
    const ushort *src = ubegin;

    buffer.resize(uend - ubegin + 1);
    char *dst = buffer.data();

    while (src != uend) {
        if (*src >= 0x7f)
            return reinterpret_cast<const QChar *>(src);
        *dst++ = *src++;
    }
    *dst = '\0';
    return 0;
}

static bool parseIp4Internal(IPv4Address &address, const char *ptr, bool acceptLeadingZero);
bool parseIp4(IPv4Address &address, const QChar *begin, const QChar *end)
{
    Q_ASSERT(begin != end);
    Buffer buffer;
    if (checkedToAscii(buffer, begin, end))
        return false;

    const char *ptr = buffer.data();
    return parseIp4Internal(address, ptr, true);
}

static bool parseIp4Internal(IPv4Address &address, const char *ptr, bool acceptLeadingZero)
{
    address = 0;
    int dotCount = 0;
    while (dotCount < 4) {
        if (!acceptLeadingZero && *ptr == '0' &&
                ptr[1] != '.' && ptr[1] != '\0')
            return false;

        const char *endptr;
        bool ok;
        quint64 ll = qstrtoull(ptr, &endptr, 0, &ok);
        quint32 x = ll;
        if (!ok || endptr == ptr || ll != x)
            return false;

        if (*endptr == '.' || dotCount == 3) {
            if (x & ~0xff)
                return false;
            address <<= 8;
        } else if (dotCount == 2) {
            if (x & ~0xffff)
                return false;
            address <<= 16;
        } else if (dotCount == 1) {
            if (x & ~0xffffff)
                return false;
            address <<= 24;
        }
        address |= x;

        if (dotCount == 3 && *endptr != '\0')
            return false;
        else if (dotCount == 3 || *endptr == '\0')
            return true;
        ++dotCount;
        ptr = endptr + 1;
    }
    return false;
}

void toString(QString &appendTo, IPv4Address address)
{
    // reconstructing is easy
    // use the fast operator% that pre-calculates the size
    appendTo += number(address >> 24)
                % QLatin1Char('.')
                % number(address >> 16)
                % QLatin1Char('.')
                % number(address >> 8)
                % QLatin1Char('.')
                % number(address);
}

/*!
    \internal
    \since 5.0

    Parses one IPv6 address from \a begin to \a end and stores the
    representation in \a address. Returns null if everything was parsed
    correctly, or the pointer to the first bad character where parsing failed.
    If the parsing failed for a reason not related to a particular character,
    returns \a end.
*/
const QChar *parseIp6(IPv6Address &address, const QChar *begin, const QChar *end)
{
    Q_ASSERT(begin != end);
    Buffer buffer;
    const QChar *ret = checkedToAscii(buffer, begin, end);
    if (ret)
        return ret;

    const char *ptr = buffer.data();

    // count the colons
    int colonCount = 0;
    int dotCount = 0;
    while (*ptr) {
        if (*ptr == ':')
            ++colonCount;
        if (*ptr == '.')
            ++dotCount;
        ++ptr;
    }
    // IPv4-in-IPv6 addresses are stricter in what they accept
    if (dotCount != 0 && dotCount != 3)
        return end;

    memset(address, 0, sizeof address);
    if (colonCount == 2 && end - begin == 2) // "::"
        return 0;

    // if there's a double colon ("::"), this is how many zeroes it means
    int zeroWordsToFill;
    ptr = buffer.data();

    // there are two cases where 8 colons are allowed: at the ends
    // so test that before the colon-count test
    if ((ptr[0] == ':' && ptr[1] == ':') ||
            (ptr[end - begin - 2] == ':' && ptr[end - begin - 1] == ':')) {
        zeroWordsToFill = 9 - colonCount;
    } else if (colonCount < 2 || colonCount > 7) {
        return end;
    } else {
        zeroWordsToFill = 8 - colonCount;
    }
    if (dotCount)
        --zeroWordsToFill;

    int pos = 0;
    while (pos < 15) {
        if (*ptr == ':') {
            // empty field, we hope it's "::"
            if (zeroWordsToFill < 1)
                return begin + (ptr - buffer.data());
            if (pos == 0 || pos == colonCount * 2) {
                if (ptr[0] == '\0' || ptr[1] != ':')
                    return begin + (ptr - buffer.data());
                ++ptr;
            }
            pos += zeroWordsToFill * 2;
            zeroWordsToFill = 0;
            ++ptr;
            continue;
        }

        const char *endptr;
        bool ok;
        quint64 ll = qstrtoull(ptr, &endptr, 16, &ok);
        quint16 x = ll;

        if (!ok || ll != x)
            return begin + (ptr - buffer.data());

        if (*endptr == '.') {
            // this could be an IPv4 address
            // it's only valid in the last element
            if (pos != 12)
                return begin + (ptr - buffer.data());

            IPv4Address ip4;
            if (!parseIp4Internal(ip4, ptr, false))
                return begin + (ptr - buffer.data());

            address[12] = ip4 >> 24;
            address[13] = ip4 >> 16;
            address[14] = ip4 >> 8;
            address[15] = ip4;
            return 0;
        }

        address[pos++] = x >> 8;
        address[pos++] = x & 0xff;

        if (*endptr == '\0')
            break;
        if (*endptr != ':')
            return begin + (endptr - buffer.data());
        ptr = endptr + 1;
    }
    return pos == 16 ? 0 : end;
}

static inline QChar toHex(uchar c)
{
    return ushort(c > 9 ? c + 'a' - 0xA : c + '0');
}

void toString(QString &appendTo, IPv6Address address)
{
    // the longest IPv6 address possible is:
    //   "1111:2222:3333:4444:5555:6666:255.255.255.255"
    // however, this function never generates that. The longest it does
    // generate without an IPv4 address is:
    //   "1111:2222:3333:4444:5555:6666:7777:8888"
    // and the longest with an IPv4 address is:
    //   "::ffff:255.255.255.255"
    static const int Ip6AddressMaxLen = sizeof "1111:2222:3333:4444:5555:6666:7777:8888";
    static const int Ip6WithIp4AddressMaxLen = sizeof "::ffff:255.255.255.255";

    // check for the special cases
    const quint64 zeroes[] = { 0, 0 };
    bool embeddedIp4 = false;

    // we consider embedded IPv4 for:
    //  ::ffff:x.x.x.x
    //  ::x.x.x.y  except if the x are 0 too
    if (memcmp(address, zeroes, 10) == 0) {
        if (address[10] == 0xff && address[11] == 0xff) {
            embeddedIp4 = true;
        } else if (address[10] == 0 && address[11] == 0) {
            if (address[12] != 0 || address[13] != 0 || address[14] != 0) {
                embeddedIp4 = true;
            } else if (address[15] == 0) {
                appendTo.append(QLatin1String("::"));
                return;
            }
        }
    }

    // QString::reserve doesn't shrink, so it's fine to us
    appendTo.reserve(appendTo.size() +
                     (embeddedIp4 ? Ip6WithIp4AddressMaxLen : Ip6AddressMaxLen));

    // for finding where to place the "::"
    int zeroRunLength = 0; // in octets
    int zeroRunOffset = 0; // in octets
    for (int i = 0; i < 16; i += 2) {
        if (address[i] == 0 && address[i + 1] == 0) {
            // found a zero, scan forward to see how many more there are
            int j;
            for (j = i; j < 16; j += 2) {
                if (address[j] != 0 || address[j+1] != 0)
                    break;
            }

            if (j - i > zeroRunLength) {
                zeroRunLength = j - i;
                zeroRunOffset = i;
                i = j;
            }
        }
    }

    const QChar colon = ushort(':');
    if (zeroRunLength < 4)
        zeroRunOffset = -1;
    else if (zeroRunOffset == 0)
        appendTo.append(colon);

    for (int i = 0; i < 16; i += 2) {
        if (i == zeroRunOffset) {
            appendTo.append(colon);
            i += zeroRunLength - 2;
            continue;
        }

        if (i == 12 && embeddedIp4) {
            IPv4Address ip4 = address[12] << 24 |
                              address[13] << 16 |
                              address[14] << 8 |
                              address[15];
            toString(appendTo, ip4);
            return;
        }

        if (address[i]) {
            if (address[i] >> 4) {
                appendTo.append(toHex(address[i] >> 4));
                appendTo.append(toHex(address[i] & 0xf));
                appendTo.append(toHex(address[i + 1] >> 4));
                appendTo.append(toHex(address[i + 1] & 0xf));
            } else if (address[i] & 0xf) {
                appendTo.append(toHex(address[i] & 0xf));
                appendTo.append(toHex(address[i + 1] >> 4));
                appendTo.append(toHex(address[i + 1] & 0xf));
            }
        } else if (address[i + 1] >> 4) {
            appendTo.append(toHex(address[i + 1] >> 4));
            appendTo.append(toHex(address[i + 1] & 0xf));
        } else {
            appendTo.append(toHex(address[i + 1] & 0xf));
        }

        if (i != 14)
            appendTo.append(colon);
    }
}

}
QT_END_NAMESPACE
