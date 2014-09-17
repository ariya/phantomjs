/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include <qregexp.h>
#include <qstring.h>

#include "qxmlutils_p.h"

QT_BEGIN_NAMESPACE

/* TODO:
 * - isNameChar() doesn't have to be public, it's only needed in
 *   qdom.cpp -- refactor fixedXmlName() to use isNCName()
 * - A lot of functions can be inlined.
 */

class QXmlCharRange
{
public:
    ushort min;
    ushort max;
};
typedef const QXmlCharRange *RangeIter;

/*!
  Performs a binary search between \a begin and \a end inclusive, to check whether \a
  c is contained. Remember that the QXmlCharRange instances must be in numeric order.
 */
bool QXmlUtils::rangeContains(RangeIter begin, RangeIter end, const QChar c)
{
    const ushort cp(c.unicode());

    // check the first two ranges "manually" as characters in that
    // range are checked very often and we avoid the binary search below.
 
    if (cp <= begin->max)
        return cp >= begin->min;

    ++begin;

    if (begin == end)
        return false;

    if (cp <= begin->max)
        return cp >= begin->min;

    while (begin != end) {
        int delta = (end - begin) / 2;
        RangeIter mid = begin + delta;

        if (mid->min > cp)
            end = mid;
        else if (mid->max < cp)
            begin = mid;
        else
            return true;

        if (delta == 0)
            break;
    }

    return false;
}

// [85] BaseChar ::= ...

static const QXmlCharRange g_base_begin[] =
{
    {0x0041, 0x005A}, {0x0061, 0x007A}, {0x00C0, 0x00D6}, {0x00D8, 0x00F6}, {0x00F8, 0x00FF},
    {0x0100, 0x0131}, {0x0134, 0x013E}, {0x0141, 0x0148}, {0x014A, 0x017E}, {0x0180, 0x01C3},
    {0x01CD, 0x01F0}, {0x01F4, 0x01F5}, {0x01FA, 0x0217}, {0x0250, 0x02A8}, {0x02BB, 0x02C1},
    {0x0386, 0x0386}, {0x0388, 0x038A}, {0x038C, 0x038C}, {0x038E, 0x03A1}, {0x03A3, 0x03CE},
    {0x03D0, 0x03D6}, {0x03DA, 0x03DA}, {0x03DC, 0x03DC}, {0x03DE, 0x03DE}, {0x03E0, 0x03E0},
    {0x03E2, 0x03F3}, {0x0401, 0x040C}, {0x040E, 0x044F}, {0x0451, 0x045C}, {0x045E, 0x0481},
    {0x0490, 0x04C4}, {0x04C7, 0x04C8}, {0x04CB, 0x04CC}, {0x04D0, 0x04EB}, {0x04EE, 0x04F5},
    {0x04F8, 0x04F9}, {0x0531, 0x0556}, {0x0559, 0x0559}, {0x0561, 0x0586}, {0x05D0, 0x05EA},
    {0x05F0, 0x05F2}, {0x0621, 0x063A}, {0x0641, 0x064A}, {0x0671, 0x06B7}, {0x06BA, 0x06BE},
    {0x06C0, 0x06CE}, {0x06D0, 0x06D3}, {0x06D5, 0x06D5}, {0x06E5, 0x06E6}, {0x0905, 0x0939},
    {0x093D, 0x093D}, {0x0958, 0x0961}, {0x0985, 0x098C}, {0x098F, 0x0990}, {0x0993, 0x09A8},
    {0x09AA, 0x09B0}, {0x09B2, 0x09B2}, {0x09B6, 0x09B9}, {0x09DC, 0x09DD}, {0x09DF, 0x09E1},
    {0x09F0, 0x09F1}, {0x0A05, 0x0A0A}, {0x0A0F, 0x0A10}, {0x0A13, 0x0A28}, {0x0A2A, 0x0A30},
    {0x0A32, 0x0A33}, {0x0A35, 0x0A36}, {0x0A38, 0x0A39}, {0x0A59, 0x0A5C}, {0x0A5E, 0x0A5E},
    {0x0A72, 0x0A74}, {0x0A85, 0x0A8B}, {0x0A8D, 0x0A8D}, {0x0A8F, 0x0A91}, {0x0A93, 0x0AA8},
    {0x0AAA, 0x0AB0}, {0x0AB2, 0x0AB3}, {0x0AB5, 0x0AB9}, {0x0ABD, 0x0ABD}, {0x0AE0, 0x0AE0},
    {0x0B05, 0x0B0C}, {0x0B0F, 0x0B10}, {0x0B13, 0x0B28}, {0x0B2A, 0x0B30}, {0x0B32, 0x0B33},
    {0x0B36, 0x0B39}, {0x0B3D, 0x0B3D}, {0x0B5C, 0x0B5D}, {0x0B5F, 0x0B61}, {0x0B85, 0x0B8A},
    {0x0B8E, 0x0B90}, {0x0B92, 0x0B95}, {0x0B99, 0x0B9A}, {0x0B9C, 0x0B9C}, {0x0B9E, 0x0B9F},
    {0x0BA3, 0x0BA4}, {0x0BA8, 0x0BAA}, {0x0BAE, 0x0BB5}, {0x0BB7, 0x0BB9}, {0x0C05, 0x0C0C},
    {0x0C0E, 0x0C10}, {0x0C12, 0x0C28}, {0x0C2A, 0x0C33}, {0x0C35, 0x0C39}, {0x0C60, 0x0C61},
    {0x0C85, 0x0C8C}, {0x0C8E, 0x0C90}, {0x0C92, 0x0CA8}, {0x0CAA, 0x0CB3}, {0x0CB5, 0x0CB9},
    {0x0CDE, 0x0CDE}, {0x0CE0, 0x0CE1}, {0x0D05, 0x0D0C}, {0x0D0E, 0x0D10}, {0x0D12, 0x0D28},
    {0x0D2A, 0x0D39}, {0x0D60, 0x0D61}, {0x0E01, 0x0E2E}, {0x0E30, 0x0E30}, {0x0E32, 0x0E33},
    {0x0E40, 0x0E45}, {0x0E81, 0x0E82}, {0x0E84, 0x0E84}, {0x0E87, 0x0E88}, {0x0E8A, 0x0E8A},
    {0x0E8D, 0x0E8D}, {0x0E94, 0x0E97}, {0x0E99, 0x0E9F}, {0x0EA1, 0x0EA3}, {0x0EA5, 0x0EA5},
    {0x0EA7, 0x0EA7}, {0x0EAA, 0x0EAB}, {0x0EAD, 0x0EAE}, {0x0EB0, 0x0EB0}, {0x0EB2, 0x0EB3},
    {0x0EBD, 0x0EBD}, {0x0EC0, 0x0EC4}, {0x0F40, 0x0F47}, {0x0F49, 0x0F69}, {0x10A0, 0x10C5},
    {0x10D0, 0x10F6}, {0x1100, 0x1100}, {0x1102, 0x1103}, {0x1105, 0x1107}, {0x1109, 0x1109},
    {0x110B, 0x110C}, {0x110E, 0x1112}, {0x113C, 0x113C}, {0x113E, 0x113E}, {0x1140, 0x1140},
    {0x114C, 0x114C}, {0x114E, 0x114E}, {0x1150, 0x1150}, {0x1154, 0x1155}, {0x1159, 0x1159},
    {0x115F, 0x1161}, {0x1163, 0x1163}, {0x1165, 0x1165}, {0x1167, 0x1167}, {0x1169, 0x1169},
    {0x116D, 0x116E}, {0x1172, 0x1173}, {0x1175, 0x1175}, {0x119E, 0x119E}, {0x11A8, 0x11A8},
    {0x11AB, 0x11AB}, {0x11AE, 0x11AF}, {0x11B7, 0x11B8}, {0x11BA, 0x11BA}, {0x11BC, 0x11C2},
    {0x11EB, 0x11EB}, {0x11F0, 0x11F0}, {0x11F9, 0x11F9}, {0x1E00, 0x1E9B}, {0x1EA0, 0x1EF9},
    {0x1F00, 0x1F15}, {0x1F18, 0x1F1D}, {0x1F20, 0x1F45}, {0x1F48, 0x1F4D}, {0x1F50, 0x1F57},
    {0x1F59, 0x1F59}, {0x1F5B, 0x1F5B}, {0x1F5D, 0x1F5D}, {0x1F5F, 0x1F7D}, {0x1F80, 0x1FB4},
    {0x1FB6, 0x1FBC}, {0x1FBE, 0x1FBE}, {0x1FC2, 0x1FC4}, {0x1FC6, 0x1FCC}, {0x1FD0, 0x1FD3},
    {0x1FD6, 0x1FDB}, {0x1FE0, 0x1FEC}, {0x1FF2, 0x1FF4}, {0x1FF6, 0x1FFC}, {0x2126, 0x2126},
    {0x212A, 0x212B}, {0x212E, 0x212E}, {0x2180, 0x2182}, {0x3041, 0x3094}, {0x30A1, 0x30FA},
    {0x3105, 0x312C}, {0xAC00, 0xD7A3}
};
static const RangeIter g_base_end = g_base_begin + sizeof(g_base_begin) / sizeof(QXmlCharRange);

static const QXmlCharRange g_ideographic_begin[] =
{
    {0x3007, 0x3007}, {0x3021, 0x3029}, {0x4E00, 0x9FA5}
};
static const RangeIter g_ideographic_end = g_ideographic_begin + sizeof(g_ideographic_begin) / sizeof(QXmlCharRange);

bool QXmlUtils::isIdeographic(const QChar c)
{
    return rangeContains(g_ideographic_begin, g_ideographic_end, c);
}

static const QXmlCharRange g_combining_begin[] =
{
    {0x0300, 0x0345}, {0x0360, 0x0361}, {0x0483, 0x0486}, {0x0591, 0x05A1}, {0x05A3, 0x05B9},
    {0x05BB, 0x05BD}, {0x05BF, 0x05BF}, {0x05C1, 0x05C2}, {0x05C4, 0x05C4}, {0x064B, 0x0652},
    {0x0670, 0x0670}, {0x06D6, 0x06DC}, {0x06DD, 0x06DF}, {0x06E0, 0x06E4}, {0x06E7, 0x06E8},
    {0x06EA, 0x06ED}, {0x0901, 0x0903}, {0x093C, 0x093C}, {0x093E, 0x094C}, {0x094D, 0x094D},
    {0x0951, 0x0954}, {0x0962, 0x0963}, {0x0981, 0x0983}, {0x09BC, 0x09BC}, {0x09BE, 0x09BE},
    {0x09BF, 0x09BF}, {0x09C0, 0x09C4}, {0x09C7, 0x09C8}, {0x09CB, 0x09CD}, {0x09D7, 0x09D7},
    {0x09E2, 0x09E3}, {0x0A02, 0x0A02}, {0x0A3C, 0x0A3C}, {0x0A3E, 0x0A3E}, {0x0A3F, 0x0A3F},
    {0x0A40, 0x0A42}, {0x0A47, 0x0A48}, {0x0A4B, 0x0A4D}, {0x0A70, 0x0A71}, {0x0A81, 0x0A83},
    {0x0ABC, 0x0ABC}, {0x0ABE, 0x0AC5}, {0x0AC7, 0x0AC9}, {0x0ACB, 0x0ACD}, {0x0B01, 0x0B03},
    {0x0B3C, 0x0B3C}, {0x0B3E, 0x0B43}, {0x0B47, 0x0B48}, {0x0B4B, 0x0B4D}, {0x0B56, 0x0B57},
    {0x0B82, 0x0B83}, {0x0BBE, 0x0BC2}, {0x0BC6, 0x0BC8}, {0x0BCA, 0x0BCD}, {0x0BD7, 0x0BD7},
    {0x0C01, 0x0C03}, {0x0C3E, 0x0C44}, {0x0C46, 0x0C48}, {0x0C4A, 0x0C4D}, {0x0C55, 0x0C56},
    {0x0C82, 0x0C83}, {0x0CBE, 0x0CC4}, {0x0CC6, 0x0CC8}, {0x0CCA, 0x0CCD}, {0x0CD5, 0x0CD6},
    {0x0D02, 0x0D03}, {0x0D3E, 0x0D43}, {0x0D46, 0x0D48}, {0x0D4A, 0x0D4D}, {0x0D57, 0x0D57},
    {0x0E31, 0x0E31}, {0x0E34, 0x0E3A}, {0x0E47, 0x0E4E}, {0x0EB1, 0x0EB1}, {0x0EB4, 0x0EB9},
    {0x0EBB, 0x0EBC}, {0x0EC8, 0x0ECD}, {0x0F18, 0x0F19}, {0x0F35, 0x0F35}, {0x0F37, 0x0F37},
    {0x0F39, 0x0F39}, {0x0F3E, 0x0F3E}, {0x0F3F, 0x0F3F}, {0x0F71, 0x0F84}, {0x0F86, 0x0F8B},
    {0x0F90, 0x0F95}, {0x0F97, 0x0F97}, {0x0F99, 0x0FAD}, {0x0FB1, 0x0FB7}, {0x0FB9, 0x0FB9},
    {0x20D0, 0x20DC}, {0x20E1, 0x20E1}, {0x302A, 0x302F}, {0x3099, 0x3099}, {0x309A, 0x309A}
};
static const RangeIter g_combining_end = g_combining_begin + sizeof(g_combining_begin) / sizeof(QXmlCharRange);

bool QXmlUtils::isCombiningChar(const QChar c)
{
    return rangeContains(g_combining_begin, g_combining_end, c);
}

// [88] Digit ::= ...
static const QXmlCharRange g_digit_begin[] =
{
    {0x0030, 0x0039}, {0x0660, 0x0669}, {0x06F0, 0x06F9}, {0x0966, 0x096F}, {0x09E6, 0x09EF},
    {0x0A66, 0x0A6F}, {0x0AE6, 0x0AEF}, {0x0B66, 0x0B6F}, {0x0BE7, 0x0BEF}, {0x0C66, 0x0C6F},
    {0x0CE6, 0x0CEF}, {0x0D66, 0x0D6F}, {0x0E50, 0x0E59}, {0x0ED0, 0x0ED9}, {0x0F20, 0x0F29}
};
static const RangeIter g_digit_end = g_digit_begin + sizeof(g_digit_begin) / sizeof(QXmlCharRange);

bool QXmlUtils::isDigit(const QChar c)
{
    return rangeContains(g_digit_begin, g_digit_end, c);
}

// [89] Extender ::= ...
static const QXmlCharRange g_extender_begin[] =
{
    {0x00B7, 0x00B7}, {0x02D0, 0x02D0}, {0x02D1, 0x02D1}, {0x0387, 0x0387}, {0x0640, 0x0640},
    {0x0E46, 0x0E46}, {0x0EC6, 0x0EC6}, {0x3005, 0x3005}, {0x3031, 0x3035}, {0x309D, 0x309E},
    {0x30FC, 0x30FE}
};
static const RangeIter g_extender_end = g_extender_begin + sizeof(g_extender_begin) / sizeof(QXmlCharRange);

bool QXmlUtils::isExtender(const QChar c)
{
    return rangeContains(g_extender_begin, g_extender_end, c);
}

bool QXmlUtils::isBaseChar(const QChar c)
{
    return rangeContains(g_base_begin, g_base_end, c);
}

/*!
   \internal

   Determines whether \a encName is a valid instance of production [81]EncName in the XML 1.0
   specification. If it is, true is returned, otherwise false.

    \sa \l {http://www.w3.org/TR/REC-xml/#NT-EncName}
           {Extensible Markup Language (XML) 1.0 (Fourth Edition), [81] EncName}
 */
bool QXmlUtils::isEncName(const QString &encName)
{
    /* Right, we here have a dependency on QRegExp. Writing a manual parser to
     * replace that regexp is probably a 70 lines so I prioritize this to when
     * the dependency is considered alarming, or when the rest of the bugs
     * are fixed. */
    const QRegExp encNameRegExp(QLatin1String("[A-Za-z][A-Za-z0-9._\\-]*"));
    Q_ASSERT(encNameRegExp.isValid());

    return encNameRegExp.exactMatch(encName);
}

/*!
 \internal

   Determines whether \a c is a valid instance of production [84]Letter in the XML 1.0
   specification. If it is, true is returned, otherwise false.

    \sa \l {http://www.w3.org/TR/REC-xml/#NT-Letter}
           {Extensible Markup Language (XML) 1.0 (Fourth Edition), [84] Letter}
 */
bool QXmlUtils::isLetter(const QChar c)
{
    return isBaseChar(c) || isIdeographic(c);
}

/*!
   \internal

   Determines whether \a c is a valid instance of production [2]Char in the XML 1.0
   specification. If it is, true is returned, otherwise false.

    \sa \l {http://www.w3.org/TR/REC-xml/#NT-Char}
           {Extensible Markup Language (XML) 1.0 (Fourth Edition), [2] Char}
 */
bool QXmlUtils::isChar(const QChar c)
{
    return (c.unicode() >= 0x0020 && c.unicode() <= 0xD7FF)
           || c.unicode() == 0x0009
           || c.unicode() == 0x000A
           || c.unicode() == 0x000D
           || (c.unicode() >= 0xE000 && c.unicode() <= 0xFFFD);
}

/*!
   \internal

   Determines whether \a c is a valid instance of
   production [4]NameChar in the XML 1.0 specification. If it
   is, true is returned, otherwise false.

    \sa \l {http://www.w3.org/TR/REC-xml/#NT-NameChar}
           {Extensible Markup Language (XML) 1.0 (Fourth Edition), [4] NameChar}
 */
bool QXmlUtils::isNameChar(const QChar c)
{
    return isBaseChar(c)
           || isDigit(c)
           || c.unicode() == '.'
           || c.unicode() == '-'
           || c.unicode() == '_'
           || c.unicode() == ':'
           || isCombiningChar(c)
           || isIdeographic(c)
           || isExtender(c);
}

/*!
   \internal

   Determines whether \a c is a valid instance of
   production [12] PubidLiteral in the XML 1.0 specification. If it
   is, true is returned, otherwise false.

    \sa \l {http://www.w3.org/TR/REC-xml/#NT-PubidLiteral}
           {Extensible Markup Language (XML) 1.0 (Fourth Edition), [12] PubidLiteral}
 */
bool QXmlUtils::isPublicID(const QString &candidate)
{
    const int len = candidate.length();

    for(int i = 0; i < len; ++i)
    {
        const ushort cp = candidate.at(i).unicode();

        if ((cp >= 'a' && cp <= 'z')
            || (cp >= 'A' && cp <= 'Z')
            || (cp >= '0' && cp <= '9'))
        {
            continue;
        }

        switch (cp)
        {
            /* Fallthrough all these. */
            case 0x20:
            case 0x0D:
            case 0x0A:
            case '-':
            case '\'':
            case '(':
            case ')':
            case '+':
            case ',':
            case '.':
            case '/':
            case ':':
            case '=':
            case '?':
            case ';':
            case '!':
            case '*':
            case '#':
            case '@':
            case '$':
            case '_':
            case '%':
                continue;
            default:
                return false;
        }
    }

    return true;
}

/*!
   \internal

   Determines whether \a c is a valid instance of
   production [4]NCName in the XML 1.0 Namespaces specification. If it
   is, true is returned, otherwise false.

    \sa \l {http://www.w3.org/TR/REC-xml-names/#NT-NCName}
           {W3CNamespaces in XML 1.0 (Second Edition), [4] NCName}
 */
bool QXmlUtils::isNCName(const QStringRef &ncName)
{
    if(ncName.isEmpty())
        return false;

    const QChar first(ncName.at(0));

    if(!QXmlUtils::isLetter(first) && first.unicode() != '_' && first.unicode() != ':')
        return false;

    const int len = ncName.size();
    for(int i = 0; i < len; ++i)
    {
        const QChar &at = ncName.at(i);
        if(!QXmlUtils::isNameChar(at) || at == QLatin1Char(':'))
            return false;
    }

    return true;
}

QT_END_NAMESPACE
