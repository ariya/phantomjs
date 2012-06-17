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

// Don't define it while compiling this module, or USERS of Qt will
// not be able to link.
#ifdef QT_NO_CAST_FROM_ASCII
#  undef QT_NO_CAST_FROM_ASCII
#endif
#ifdef QT_NO_CAST_TO_ASCII
#  undef QT_NO_CAST_TO_ASCII
#endif
#include "qchar.h"

#include "qdatastream.h"
#include "qtextcodec.h"

#include "qunicodetables_p.h"
#include "qunicodetables.cpp"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_CODEC_FOR_C_STRINGS
#  ifdef QT_NO_TEXTCODEC
#    define QT_NO_CODEC_FOR_C_STRINGS
#  endif
#endif

#define FLAG(x) (1 << (x))

/*!
    \class QLatin1Char
    \brief The QLatin1Char class provides an 8-bit ASCII/Latin-1 character.

    \ingroup string-processing

    This class is only useful to avoid the codec for C strings business
    in the QChar(ch) constructor. You can avoid it by writing
    QChar(ch, 0).

    \sa QChar, QLatin1String, QString
*/

/*!
    \fn const char QLatin1Char::toLatin1() const

    Converts a Latin-1 character to an 8-bit ASCII representation of
    the character.
*/

/*!
    \fn const ushort QLatin1Char::unicode() const

    Converts a Latin-1 character to an 16-bit-encoded Unicode representation
    of the character.
*/

/*!
    \fn QLatin1Char::QLatin1Char(char c)

    Constructs a Latin-1 character for \a c. This constructor should be
    used when the encoding of the input character is known to be Latin-1.
*/

/*!
    \class QChar
    \brief The QChar class provides a 16-bit Unicode character.

    \ingroup string-processing
    \reentrant

    In Qt, Unicode characters are 16-bit entities without any markup
    or structure. This class represents such an entity. It is
    lightweight, so it can be used everywhere. Most compilers treat
    it like a \c{unsigned short}.

    QChar provides a full complement of testing/classification
    functions, converting to and from other formats, converting from
    composed to decomposed Unicode, and trying to compare and
    case-convert if you ask it to.

    The classification functions include functions like those in the
    standard C++ header \<cctype\> (formerly \<ctype.h\>), but
    operating on the full range of Unicode characters. They all
    return true if the character is a certain type of character;
    otherwise they return false. These classification functions are
    isNull() (returns true if the character is '\\0'), isPrint()
    (true if the character is any sort of printable character,
    including whitespace), isPunct() (any sort of punctation),
    isMark() (Unicode Mark), isLetter() (a letter), isNumber() (any
    sort of numeric character, not just 0-9), isLetterOrNumber(), and
    isDigit() (decimal digits). All of these are wrappers around
    category() which return the Unicode-defined category of each
    character.

    QChar also provides direction(), which indicates the "natural"
    writing direction of this character. The joining() function
    indicates how the character joins with its neighbors (needed
    mostly for Arabic) and finally hasMirrored(), which indicates
    whether the character needs to be mirrored when it is printed in
    its "unnatural" writing direction.

    Composed Unicode characters (like \aring) can be converted to
    decomposed Unicode ("a" followed by "ring above") by using
    decomposition().

    In Unicode, comparison is not necessarily possible and case
    conversion is very difficult at best. Unicode, covering the
    "entire" world, also includes most of the world's case and
    sorting problems. operator==() and friends will do comparison
    based purely on the numeric Unicode value (code point) of the
    characters, and toUpper() and toLower() will do case changes when
    the character has a well-defined uppercase/lowercase equivalent.
    For locale-dependent comparisons, use
    QString::localeAwareCompare().

    The conversion functions include unicode() (to a scalar),
    toLatin1() (to scalar, but converts all non-Latin-1 characters to
    0), row() (gives the Unicode row), cell() (gives the Unicode
    cell), digitValue() (gives the integer value of any of the
    numerous digit characters), and a host of constructors.

    QChar provides constructors and cast operators that make it easy
    to convert to and from traditional 8-bit \c{char}s. If you
    defined \c QT_NO_CAST_FROM_ASCII and \c QT_NO_CAST_TO_ASCII, as
    explained in the QString documentation, you will need to
    explicitly call fromAscii() or fromLatin1(), or use QLatin1Char,
    to construct a QChar from an 8-bit \c char, and you will need to
    call toAscii() or toLatin1() to get the 8-bit value back.

    \sa QString, Unicode, QLatin1Char
*/

/*!
    \enum QChar::UnicodeVersion

    Specifies which version of the \l{http://www.unicode.org/}{Unicode standard}
    introduced a certain character.

    \value Unicode_1_1  Version 1.1
    \value Unicode_2_0  Version 2.0
    \value Unicode_2_1_2  Version 2.1.2
    \value Unicode_3_0  Version 3.0
    \value Unicode_3_1  Version 3.1
    \value Unicode_3_2  Version 3.2
    \value Unicode_4_0  Version 4.0
    \value Unicode_4_1  Version 4.1
    \value Unicode_5_0  Version 5.0
    \value Unicode_Unassigned  The value is not assigned to any character
        in version 5.0 of Unicode.

    \sa unicodeVersion()
*/

/*!
    \enum QChar::Category

    This enum maps the Unicode character categories.

    The following characters are normative in Unicode:

    \value Mark_NonSpacing  Unicode class name Mn

    \value Mark_SpacingCombining  Unicode class name Mc

    \value Mark_Enclosing  Unicode class name Me

    \value Number_DecimalDigit  Unicode class name Nd

    \value Number_Letter  Unicode class name Nl

    \value Number_Other  Unicode class name No

    \value Separator_Space  Unicode class name Zs

    \value Separator_Line  Unicode class name Zl

    \value Separator_Paragraph  Unicode class name Zp

    \value Other_Control  Unicode class name Cc

    \value Other_Format  Unicode class name Cf

    \value Other_Surrogate  Unicode class name Cs

    \value Other_PrivateUse  Unicode class name Co

    \value Other_NotAssigned  Unicode class name Cn


    The following categories are informative in Unicode:

    \value Letter_Uppercase  Unicode class name Lu

    \value Letter_Lowercase  Unicode class name Ll

    \value Letter_Titlecase  Unicode class name Lt

    \value Letter_Modifier  Unicode class name Lm

    \value Letter_Other Unicode class name Lo

    \value Punctuation_Connector  Unicode class name Pc

    \value Punctuation_Dash  Unicode class name Pd

    \value Punctuation_Open  Unicode class name Ps

    \value Punctuation_Close  Unicode class name Pe

    \value Punctuation_InitialQuote  Unicode class name Pi

    \value Punctuation_FinalQuote  Unicode class name Pf

    \value Punctuation_Other  Unicode class name Po

    \value Symbol_Math  Unicode class name Sm

    \value Symbol_Currency  Unicode class name Sc

    \value Symbol_Modifier  Unicode class name Sk

    \value Symbol_Other  Unicode class name So

    \value NoCategory  Qt cannot find an appropriate category for the character.

    \omitvalue Punctuation_Dask

    \sa category()
*/

/*!
    \enum QChar::Direction

    This enum type defines the Unicode direction attributes. See the
    \l{http://www.unicode.org/}{Unicode Standard} for a description
    of the values.

    In order to conform to C/C++ naming conventions "Dir" is prepended
    to the codes used in the Unicode Standard.

    \value DirAL
    \value DirAN
    \value DirB
    \value DirBN
    \value DirCS
    \value DirEN
    \value DirES
    \value DirET
    \value DirL
    \value DirLRE
    \value DirLRO
    \value DirNSM
    \value DirON
    \value DirPDF
    \value DirR
    \value DirRLE
    \value DirRLO
    \value DirS
    \value DirWS

    \sa direction()
*/

/*!
    \enum QChar::Decomposition

    This enum type defines the Unicode decomposition attributes. See
    the \l{http://www.unicode.org/}{Unicode Standard} for a
    description of the values.

    \value NoDecomposition
    \value Canonical
    \value Circle
    \value Compat
    \value Final
    \value Font
    \value Fraction
    \value Initial
    \value Isolated
    \value Medial
    \value Narrow
    \value NoBreak
    \value Small
    \value Square
    \value Sub
    \value Super
    \value Vertical
    \value Wide

    \omitvalue Single

    \sa decomposition()
*/

/*!
    \enum QChar::Joining

    This enum type defines the Unicode joining attributes. See the
    \l{http://www.unicode.org/}{Unicode Standard} for a description
    of the values.

    \value Center
    \value Dual
    \value OtherJoining
    \value Right

    \sa joining()
*/

/*!
    \enum QChar::CombiningClass

    \internal

    This enum type defines names for some of the Unicode combining
    classes. See the \l{http://www.unicode.org/}{Unicode Standard}
    for a description of the values.

    \value Combining_Above
    \value Combining_AboveAttached
    \value Combining_AboveLeft
    \value Combining_AboveLeftAttached
    \value Combining_AboveRight
    \value Combining_AboveRightAttached
    \value Combining_Below
    \value Combining_BelowAttached
    \value Combining_BelowLeft
    \value Combining_BelowLeftAttached
    \value Combining_BelowRight
    \value Combining_BelowRightAttached
    \value Combining_DoubleAbove
    \value Combining_DoubleBelow
    \value Combining_IotaSubscript
    \value Combining_Left
    \value Combining_LeftAttached
    \value Combining_Right
    \value Combining_RightAttached
*/

/*!
    \enum QChar::SpecialCharacter

    \value Null A QChar with this value isNull().
    \value Nbsp Non-breaking space.
    \value ReplacementCharacter The character shown when a font has no glyph
           for a certain codepoint. A special question mark character is often
           used. Codecs use this codepoint when input data cannot be
           represented in Unicode.
    \value ObjectReplacementCharacter Used to represent an object such as an
           image when such objects cannot be presented.
    \value ByteOrderMark
    \value ByteOrderSwapped
    \value ParagraphSeparator
    \value LineSeparator

    \omitvalue null
    \omitvalue replacement
    \omitvalue byteOrderMark
    \omitvalue byteOrderSwapped
    \omitvalue nbsp
*/

/*!
    \fn void QChar::setCell(uchar cell)
    \internal
*/

/*!
    \fn void QChar::setRow(uchar row)
    \internal
*/

/*!
    \fn QChar::QChar()

    Constructs a null QChar ('\\0').

    \sa isNull()
*/

/*!
    \fn QChar::QChar(QLatin1Char ch)

    Constructs a QChar corresponding to ASCII/Latin-1 character \a ch.
*/

/*!
    \fn QChar::QChar(SpecialCharacter ch)

    Constructs a QChar for the predefined character value \a ch.
*/

/*!
    Constructs a QChar corresponding to ASCII/Latin-1 character \a
    ch.
*/
QChar::QChar(char ch)
{
#ifndef QT_NO_CODEC_FOR_C_STRINGS
    if (QTextCodec::codecForCStrings())
        // #####
        ucs =  QTextCodec::codecForCStrings()->toUnicode(&ch, 1).at(0).unicode();
    else
#endif
        ucs = uchar(ch);
}

/*!
    Constructs a QChar corresponding to ASCII/Latin-1 character \a ch.
*/
QChar::QChar(uchar ch)
{
#ifndef QT_NO_CODEC_FOR_C_STRINGS
    if (QTextCodec::codecForCStrings()) {
        // #####
        char c = char(ch);
        ucs =  QTextCodec::codecForCStrings()->toUnicode(&c, 1).at(0).unicode();
    } else
#endif
        ucs = ch;
}

/*!
    \fn QChar::QChar(uchar cell, uchar row)

    Constructs a QChar for Unicode cell \a cell in row \a row.

    \sa cell(), row()
*/

/*!
    \fn QChar::QChar(ushort code)

    Constructs a QChar for the character with Unicode code point \a
    code.
*/


/*!
    \fn QChar::QChar(short code)

    Constructs a QChar for the character with Unicode code point \a
    code.
*/


/*!
    \fn QChar::QChar(uint code)

    Constructs a QChar for the character with Unicode code point \a
    code.
*/


/*!
    \fn QChar::QChar(int code)

    Constructs a QChar for the character with Unicode code point \a
    code.
*/


/*!
    \fn bool QChar::isNull() const

    Returns true if the character is the Unicode character 0x0000
    ('\\0'); otherwise returns false.
*/

/*!
    \fn uchar QChar::cell() const

    Returns the cell (least significant byte) of the Unicode
    character.

    \sa row()
*/

/*!
    \fn uchar QChar::row() const

    Returns the row (most significant byte) of the Unicode character.

    \sa cell()
*/

/*!
    Returns true if the character is a printable character; otherwise
    returns false. This is any character not of category Cc or Cn.

    Note that this gives no indication of whether the character is
    available in a particular font.
*/
bool QChar::isPrint() const
{
    const int test = FLAG(Other_Control) |
                     FLAG(Other_NotAssigned);
    return !(FLAG(qGetProp(ucs)->category) & test);
}

/*!
    Returns true if the character is a separator character
    (Separator_* categories); otherwise returns false.
*/
bool QChar::isSpace() const
{
    if(ucs >= 9 && ucs <=13)
        return true;
    const int test = FLAG(Separator_Space) |
                     FLAG(Separator_Line) |
                     FLAG(Separator_Paragraph);
    return FLAG(qGetProp(ucs)->category) & test;
}

/*!
    Returns true if the character is a mark (Mark_* categories);
    otherwise returns false.

    See QChar::Category for more information regarding marks.
*/
bool QChar::isMark() const
{
    const int test = FLAG(Mark_NonSpacing) |
                     FLAG(Mark_SpacingCombining) |
                     FLAG(Mark_Enclosing);
    return FLAG(qGetProp(ucs)->category) & test;
}

/*!
    Returns true if the character is a punctuation mark (Punctuation_*
    categories); otherwise returns false.
*/
bool QChar::isPunct() const
{
    const int test = FLAG(Punctuation_Connector) |
                     FLAG(Punctuation_Dash) |
                     FLAG(Punctuation_Open) |
                     FLAG(Punctuation_Close) |
                     FLAG(Punctuation_InitialQuote) |
                     FLAG(Punctuation_FinalQuote) |
                     FLAG(Punctuation_Other);
    return FLAG(qGetProp(ucs)->category) & test;
}

/*!
    Returns true if the character is a letter (Letter_* categories);
    otherwise returns false.
*/
bool QChar::isLetter() const
{
    const int test = FLAG(Letter_Uppercase) |
                     FLAG(Letter_Lowercase) |
                     FLAG(Letter_Titlecase) |
                     FLAG(Letter_Modifier) |
                     FLAG(Letter_Other);
    return FLAG(qGetProp(ucs)->category) & test;
}

/*!
    Returns true if the character is a number (Number_* categories,
    not just 0-9); otherwise returns false.

    \sa isDigit()
*/
bool QChar::isNumber() const
{
    const int test = FLAG(Number_DecimalDigit) |
                     FLAG(Number_Letter) |
                     FLAG(Number_Other);
    return FLAG(qGetProp(ucs)->category) & test;
}

/*!
    Returns true if the character is a letter or number (Letter_* or
    Number_* categories); otherwise returns false.
*/
bool QChar::isLetterOrNumber() const
{
    const int test = FLAG(Letter_Uppercase) |
                     FLAG(Letter_Lowercase) |
                     FLAG(Letter_Titlecase) |
                     FLAG(Letter_Modifier) |
                     FLAG(Letter_Other) |
                     FLAG(Number_DecimalDigit) |
                     FLAG(Number_Letter) |
                     FLAG(Number_Other);
    return FLAG(qGetProp(ucs)->category) & test;
}


/*!
    Returns true if the character is a decimal digit
    (Number_DecimalDigit); otherwise returns false.
*/
bool QChar::isDigit() const
{
    return (qGetProp(ucs)->category == Number_DecimalDigit);
}


/*!
    Returns true if the character is a symbol (Symbol_* categories);
    otherwise returns false.
*/
bool QChar::isSymbol() const
{
    const int test = FLAG(Symbol_Math) |
                     FLAG(Symbol_Currency) |
                     FLAG(Symbol_Modifier) |
                     FLAG(Symbol_Other);
    return FLAG(qGetProp(ucs)->category) & test;
}

/*!
    \fn bool QChar::isHighSurrogate() const

    Returns true if the QChar is the high part of a utf16 surrogate
    (ie. if its code point is between 0xd800 and 0xdbff, inclusive).
*/

/*!
    \fn bool QChar::isLowSurrogate() const

    Returns true if the QChar is the low part of a utf16 surrogate
    (ie. if its code point is between 0xdc00 and 0xdfff, inclusive).
*/

/*!
    \fn static bool QChar::isHighSurrogate(uint ucs4)
    \since 4.7

    Returns true if the UCS-4-encoded character specified by \a ucs4
    is the high part of a utf16 surrogate
    (ie. if its code point is between 0xd800 and 0xdbff, inclusive).
*/

/*!
    \fn static bool QChar::isLowSurrogate(uint ucs4)
    \since 4.7

    Returns true if the UCS-4-encoded character specified by \a ucs4
    is the low part of a utf16 surrogate
    (ie. if its code point is between 0xdc00 and 0xdfff, inclusive).
*/

/*!
    \fn static bool QChar::requiresSurrogates(uint ucs4)
    \since 4.7

    Returns true if the UCS-4-encoded character specified by \a ucs4
    can be split into the high and low parts of a utf16 surrogate
    (ie. if its code point is greater than or equals to 0x10000).
*/

/*!
    \fn static uint QChar::surrogateToUcs4(ushort high, ushort low)

    Converts a UTF16 surrogate pair with the given \a high and \a low values
    to its UCS-4 code point.
*/

/*!
    \fn static uint QChar::surrogateToUcs4(QChar high, QChar low)

    Converts a utf16 surrogate pair (\a high, \a low) to its ucs4 code point.
*/

/*!
    \fn static ushort QChar::highSurrogate(uint ucs4)

    Returns the high surrogate value of a ucs4 code point.
    The returned result is undefined if \a ucs4 is smaller than 0x10000.
*/

/*!
    \fn static ushort QChar::lowSurrogate(uint ucs4)

    Returns the low surrogate value of a ucs4 code point.
    The returned result is undefined if \a ucs4 is smaller than 0x10000.
*/

/*!
    Returns the numeric value of the digit, or -1 if the character is
    not a digit.
*/
int QChar::digitValue() const
{
    return qGetProp(ucs)->digitValue;
}

/*!
    \overload
    Returns the numeric value of the digit, specified by the UCS-2-encoded
    character, \a ucs2, or -1 if the character is not a digit.
*/
int QChar::digitValue(ushort ucs2)
{
    return qGetProp(ucs2)->digitValue;
}

/*!
    \overload
    Returns the numeric value of the digit specified by the UCS-4-encoded
    character, \a ucs4, or -1 if the character is not a digit.
*/
int QChar::digitValue(uint ucs4)
{
    if (ucs4 > UNICODE_LAST_CODEPOINT)
        return 0;
    return qGetProp(ucs4)->digitValue;
}

/*!
    Returns the character's category.
*/
QChar::Category QChar::category() const
{
    return (QChar::Category) qGetProp(ucs)->category;
}

/*!
    \overload
    \since 4.3
    Returns the category of the UCS-4-encoded character specified by \a ucs4.
*/
QChar::Category QChar::category(uint ucs4)
{
    if (ucs4 > UNICODE_LAST_CODEPOINT)
        return QChar::NoCategory;
    return (QChar::Category) qGetProp(ucs4)->category;
}

/*!
    \overload
    Returns the category of the UCS-2-encoded character specified by \a ucs2.
*/
QChar::Category QChar::category(ushort ucs2)
{
    return (QChar::Category) qGetProp(ucs2)->category;
}


/*!
    Returns the character's direction.
*/
QChar::Direction QChar::direction() const
{
    return (QChar::Direction) qGetProp(ucs)->direction;
}

/*!
    \overload
    Returns the direction of the UCS-4-encoded character specified by \a ucs4.
*/
QChar::Direction QChar::direction(uint ucs4)
{
    if (ucs4 > UNICODE_LAST_CODEPOINT)
        return QChar::DirL;
    return (QChar::Direction) qGetProp(ucs4)->direction;
}

/*!
    \overload
    Returns the direction of the UCS-2-encoded character specified by \a ucs2.
*/
QChar::Direction QChar::direction(ushort ucs2)
{
    return (QChar::Direction) qGetProp(ucs2)->direction;
}

/*!
    Returns information about the joining properties of the character
    (needed for certain languages such as Arabic).
*/
QChar::Joining QChar::joining() const
{
    return (QChar::Joining) qGetProp(ucs)->joining;
}

/*!
    \overload
    Returns information about the joining properties of the UCS-4-encoded
    character specified by \a ucs4 (needed for certain languages such as
    Arabic).
*/
QChar::Joining QChar::joining(uint ucs4)
{
    if (ucs4 > UNICODE_LAST_CODEPOINT)
        return QChar::OtherJoining;
    return (QChar::Joining) qGetProp(ucs4)->joining;
}

/*!
    \overload
    Returns information about the joining properties of the UCS-2-encoded
    character specified by \a ucs2 (needed for certain languages such as
    Arabic).
*/
QChar::Joining QChar::joining(ushort ucs2)
{
    return (QChar::Joining) qGetProp(ucs2)->joining;
}


/*!
    Returns true if the character should be reversed if the text
    direction is reversed; otherwise returns false.

    Same as (ch.mirroredChar() != ch).

    \sa mirroredChar()
*/
bool QChar::hasMirrored() const
{
    return qGetProp(ucs)->mirrorDiff != 0;
}

/*!
    \fn bool QChar::isLower() const

    Returns true if the character is a lowercase letter, i.e.
    category() is Letter_Lowercase.

    \sa isUpper(), toLower(), toUpper()
*/

/*!
    \fn bool QChar::isUpper() const

    Returns true if the character is an uppercase letter, i.e.
    category() is Letter_Uppercase.

    \sa isLower(), toUpper(), toLower()
*/

/*!
    \fn bool QChar::isTitleCase() const
    \since 4.3

    Returns true if the character is a titlecase letter, i.e.
    category() is Letter_Titlecase.

    \sa isLower(), toUpper(), toLower(), toTitleCase()
*/

/*!
    Returns the mirrored character if this character is a mirrored
    character; otherwise returns the character itself.

    \sa hasMirrored()
*/
QChar QChar::mirroredChar() const
{
    return ucs + qGetProp(ucs)->mirrorDiff;
}

/*!
    \overload
    Returns the mirrored character if the UCS-4-encoded character specified
    by \a ucs4 is a mirrored character; otherwise returns the character itself.

    \sa hasMirrored()
*/
uint QChar::mirroredChar(uint ucs4)
{
    if (ucs4 > UNICODE_LAST_CODEPOINT)
        return ucs4;
    return ucs4 + qGetProp(ucs4)->mirrorDiff;
}

/*!
    \overload
    Returns the mirrored character if the UCS-2-encoded character specified
    by \a ucs2 is a mirrored character; otherwise returns the character itself.

    \sa hasMirrored()
*/
ushort QChar::mirroredChar(ushort ucs2)
{
    return ucs2 + qGetProp(ucs2)->mirrorDiff;
}


enum {
    Hangul_SBase = 0xac00,
    Hangul_LBase = 0x1100,
    Hangul_VBase = 0x1161,
    Hangul_TBase = 0x11a7,
    Hangul_SCount = 11172,
    Hangul_LCount = 19,
    Hangul_VCount = 21,
    Hangul_TCount = 28,
    Hangul_NCount = 21*28
};

// buffer has to have a length of 3. It's needed for Hangul decomposition
static const unsigned short * QT_FASTCALL decompositionHelper
    (uint ucs4, int *length, int *tag, unsigned short *buffer)
{
    *length = 0;
    if (ucs4 > UNICODE_LAST_CODEPOINT)
        return 0;
    if (ucs4 >= Hangul_SBase && ucs4 < Hangul_SBase + Hangul_SCount) {
        int SIndex = ucs4 - Hangul_SBase;
        buffer[0] = Hangul_LBase + SIndex / Hangul_NCount; // L
        buffer[1] = Hangul_VBase + (SIndex % Hangul_NCount) / Hangul_TCount; // V
        buffer[2] = Hangul_TBase + SIndex % Hangul_TCount; // T
        *length = buffer[2] == Hangul_TBase ? 2 : 3;
        *tag = QChar::Canonical;
        return buffer;
    }

    const unsigned short index = GET_DECOMPOSITION_INDEX(ucs4);
    if (index == 0xffff)
        return 0;
    const unsigned short *decomposition = uc_decomposition_map+index;
    *tag = (*decomposition) & 0xff;
    *length = (*decomposition) >> 8;
    return decomposition+1;
}

/*!
    Decomposes a character into its parts. Returns an empty string if
    no decomposition exists.
*/
QString QChar::decomposition() const
{
    return decomposition(ucs);
}

/*!
    \overload
    Decomposes the UCS-4-encoded character specified by \a ucs4 into its
    constituent parts. Returns an empty string if no decomposition exists.
*/
QString QChar::decomposition(uint ucs4)
{
    unsigned short buffer[3];
    int length;
    int tag;
    const unsigned short *d = decompositionHelper(ucs4, &length, &tag, buffer);
    return QString::fromUtf16(d, length);
}

/*!
    Returns the tag defining the composition of the character. Returns
    QChar::Single if no decomposition exists.
*/
QChar::Decomposition QChar::decompositionTag() const
{
    return decompositionTag(ucs);
}

/*!
    \overload
    Returns the tag defining the composition of the UCS-4-encoded character
    specified by \a ucs4. Returns QChar::Single if no decomposition exists.
*/
QChar::Decomposition QChar::decompositionTag(uint ucs4)
{
    if (ucs4 > UNICODE_LAST_CODEPOINT)
        return QChar::NoDecomposition;
    const unsigned short index = GET_DECOMPOSITION_INDEX(ucs4);
    if (index == 0xffff)
        return QChar::NoDecomposition;
    return (QChar::Decomposition)(uc_decomposition_map[index] & 0xff);
}

/*!
    Returns the combining class for the character as defined in the
    Unicode standard. This is mainly useful as a positioning hint for
    marks attached to a base character.

    The Qt text rendering engine uses this information to correctly
    position non-spacing marks around a base character.
*/
unsigned char QChar::combiningClass() const
{
    return (unsigned char) qGetProp(ucs)->combiningClass;
}

/*!
    \overload
    Returns the combining class for the UCS-4-encoded character specified by
    \a ucs4, as defined in the Unicode standard.
*/
unsigned char QChar::combiningClass(uint ucs4)
{
    if (ucs4 > UNICODE_LAST_CODEPOINT)
        return 0;
    return (unsigned char) qGetProp(ucs4)->combiningClass;
}

/*!
    \overload
    Returns the combining class for the UCS-2-encoded character specified by
    \a ucs2, as defined in the Unicode standard.
*/
unsigned char QChar::combiningClass(ushort ucs2)
{
    return (unsigned char) qGetProp(ucs2)->combiningClass;
}

/*!
    Returns the Unicode version that introduced this character.
*/
QChar::UnicodeVersion QChar::unicodeVersion() const
{
    return (QChar::UnicodeVersion) qGetProp(ucs)->unicodeVersion;
}

/*!
    \overload
    Returns the Unicode version that introduced the character specified in
    its UCS-4-encoded form as \a ucs4.
*/
QChar::UnicodeVersion QChar::unicodeVersion(uint ucs4)
{
    if (ucs4 > UNICODE_LAST_CODEPOINT)
        return QChar::Unicode_Unassigned;
    return (QChar::UnicodeVersion) qGetProp(ucs4)->unicodeVersion;
}

/*!
    \overload
    Returns the Unicode version that introduced the character specified in
    its UCS-2-encoded form as \a ucs2.
*/
QChar::UnicodeVersion QChar::unicodeVersion(ushort ucs2)
{
    return (QChar::UnicodeVersion) qGetProp(ucs2)->unicodeVersion;
}

/*!
    \since 4.8

    Returns the most recent supported Unicode version.
*/
QChar::UnicodeVersion QChar::currentUnicodeVersion()
{
    return UNICODE_DATA_VERSION;
}

/*!
    Returns the lowercase equivalent if the character is uppercase or titlecase;
    otherwise returns the character itself.
*/
QChar QChar::toLower() const
{
    const QUnicodeTables::Properties *p = qGetProp(ucs);
    if (!p->lowerCaseSpecial)
        return ucs + p->lowerCaseDiff;
    return ucs;
}

/*!
    \overload
    Returns the lowercase equivalent of the UCS-4-encoded character specified
    by \a ucs4 if the character is uppercase or titlecase; otherwise returns
    the character itself.
*/
uint QChar::toLower(uint ucs4)
{
    if (ucs4 > UNICODE_LAST_CODEPOINT)
        return ucs4;
    const QUnicodeTables::Properties *p = qGetProp(ucs4);
    if (!p->lowerCaseSpecial)
        return ucs4 + p->lowerCaseDiff;
    return ucs4;
}

/*!
    \overload
    Returns the lowercase equivalent of the UCS-2-encoded character specified
    by \a ucs2 if the character is uppercase or titlecase; otherwise returns
    the character itself.
*/
ushort QChar::toLower(ushort ucs2)
{
    const QUnicodeTables::Properties *p = qGetProp(ucs2);
    if (!p->lowerCaseSpecial)
        return ucs2 + p->lowerCaseDiff;
    return ucs2;
}

/*!
    Returns the uppercase equivalent if the character is lowercase or titlecase;
    otherwise returns the character itself.
*/
QChar QChar::toUpper() const
{
    const QUnicodeTables::Properties *p = qGetProp(ucs);
    if (!p->upperCaseSpecial)
        return ucs + p->upperCaseDiff;
    return ucs;
}

/*!
    \overload
    Returns the uppercase equivalent of the UCS-4-encoded character specified
    by \a ucs4 if the character is lowercase or titlecase; otherwise returns
    the character itself.
*/
uint QChar::toUpper(uint ucs4)
{
    if (ucs4 > UNICODE_LAST_CODEPOINT)
        return ucs4;
    const QUnicodeTables::Properties *p = qGetProp(ucs4);
    if (!p->upperCaseSpecial)
        return ucs4 + p->upperCaseDiff;
    return ucs4;
}

/*!
    \overload
    Returns the uppercase equivalent of the UCS-2-encoded character specified
    by \a ucs2 if the character is lowercase or titlecase; otherwise returns
    the character itself.
*/
ushort QChar::toUpper(ushort ucs2)
{
    const QUnicodeTables::Properties *p = qGetProp(ucs2);
    if (!p->upperCaseSpecial)
        return ucs2 + p->upperCaseDiff;
    return ucs2;
}

/*!
    Returns the title case equivalent if the character is lowercase or uppercase;
    otherwise returns the character itself.
*/
QChar QChar::toTitleCase() const
{
    const QUnicodeTables::Properties *p = qGetProp(ucs);
    if (!p->titleCaseSpecial)
        return ucs + p->titleCaseDiff;
    return ucs;
}

/*!
    \overload
    Returns the title case equivalent of the UCS-4-encoded character specified
    by \a ucs4 if the character is lowercase or uppercase; otherwise returns
    the character itself.
*/
uint QChar::toTitleCase(uint ucs4)
{
    if (ucs4 > UNICODE_LAST_CODEPOINT)
        return ucs4;
    const QUnicodeTables::Properties *p = qGetProp(ucs4);
    if (!p->titleCaseSpecial)
        return ucs4 + p->titleCaseDiff;
    return ucs4;
}

/*!
    \overload
    Returns the title case equivalent of the UCS-2-encoded character specified
    by \a ucs2 if the character is lowercase or uppercase; otherwise returns
    the character itself.
*/
ushort QChar::toTitleCase(ushort ucs2)
{
    const QUnicodeTables::Properties *p = qGetProp(ucs2);
    if (!p->titleCaseSpecial)
        return ucs2 + p->titleCaseDiff;
    return ucs2;
}


static inline uint foldCase(const ushort *ch, const ushort *start)
{
    uint c = *ch;
    if (QChar(c).isLowSurrogate() && ch > start && QChar(*(ch - 1)).isHighSurrogate())
        c = QChar::surrogateToUcs4(*(ch - 1), c);
    return *ch + qGetProp(c)->caseFoldDiff;
}

static inline uint foldCase(uint ch, uint &last)
{
    uint c = ch;
    if (QChar(c).isLowSurrogate() && QChar(last).isHighSurrogate())
        c = QChar::surrogateToUcs4(last, c);
    last = ch;
    return ch + qGetProp(c)->caseFoldDiff;
}

static inline ushort foldCase(ushort ch)
{
    return ch + qGetProp(ch)->caseFoldDiff;
}

/*!
    Returns the case folded equivalent of the character. For most Unicode characters this
    is the same as toLowerCase().
*/
QChar QChar::toCaseFolded() const
{
    return ucs + qGetProp(ucs)->caseFoldDiff;
}

/*!
    \overload
    Returns the case folded equivalent of the UCS-4-encoded character specified
    by \a ucs4. For most Unicode characters this is the same as toLowerCase().
*/
uint QChar::toCaseFolded(uint ucs4)
{
    if (ucs4 > UNICODE_LAST_CODEPOINT)
        return ucs4;
    return ucs4 + qGetProp(ucs4)->caseFoldDiff;
}

/*!
    \overload
    Returns the case folded equivalent of the UCS-2-encoded character specified
    by \a ucs2. For most Unicode characters this is the same as toLowerCase().
*/
ushort QChar::toCaseFolded(ushort ucs2)
{
    return ucs2 + qGetProp(ucs2)->caseFoldDiff;
}


/*!
    \fn char QChar::latin1() const

    Use toLatin1() instead.
*/

/*!
    \fn char QChar::ascii() const

    Use toAscii() instead.
*/

/*!
    \fn char QChar::toLatin1() const

    Returns the Latin-1 character equivalent to the QChar, or 0. This
    is mainly useful for non-internationalized software.

    \sa toAscii(), unicode(), QTextCodec::codecForCStrings()
*/

/*!
    \fn char QChar::toAscii() const
    Returns the character value of the QChar obtained using the current
    codec used to read C strings, or 0 if the character is not representable
    using this codec. The default codec handles Latin-1 encoded text,
    but this can be changed to assist developers writing source code using
    other encodings.

    The main purpose of this function is to preserve ASCII characters used
    in C strings. This is mainly useful for developers of non-internationalized
    software.

    \sa toLatin1(), unicode(), QTextCodec::codecForCStrings()
*/
#ifdef Q_COMPILER_MANGLES_RETURN_TYPE
const char QChar::toAscii() const
#else
char QChar::toAscii() const
#endif
{
#ifndef QT_NO_CODEC_FOR_C_STRINGS
    if (QTextCodec::codecForCStrings())
        // #####
        return QTextCodec::codecForCStrings()->fromUnicode(QString(*this)).at(0);
#endif
    return ucs > 0xff ? 0 : char(ucs);
}

/*!
    \fn QChar QChar::fromLatin1(char c)

    Converts the Latin-1 character \a c to its equivalent QChar. This
    is mainly useful for non-internationalized software.

    \sa fromAscii(), unicode(), QTextCodec::codecForCStrings()
*/

/*!
    Converts the ASCII character \a c to its equivalent QChar. This
    is mainly useful for non-internationalized software.

    An alternative is to use QLatin1Char.

    \sa fromLatin1(), unicode(), QTextCodec::codecForCStrings()
*/
QChar QChar::fromAscii(char c)
{
#ifndef QT_NO_CODEC_FOR_C_STRINGS
    if (QTextCodec::codecForCStrings())
        // #####
        return QTextCodec::codecForCStrings()->toUnicode(&c, 1).at(0).unicode();
#endif
    return QChar(ushort((uchar)c));
}

#ifndef QT_NO_DATASTREAM
/*!
    \relates QChar

    Writes the char \a chr to the stream \a out.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator<<(QDataStream &out, const QChar &chr)
{
    out << quint16(chr.unicode());
    return out;
}

/*!
    \relates QChar

    Reads a char from the stream \a in into char \a chr.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator>>(QDataStream &in, QChar &chr)
{
    quint16 u;
    in >> u;
    chr.unicode() = ushort(u);
    return in;
}
#endif // QT_NO_DATASTREAM

/*!
    \fn ushort & QChar::unicode()

    Returns a reference to the numeric Unicode value of the QChar.
*/

/*!
    \fn ushort QChar::unicode() const

    \overload
*/

/*****************************************************************************
  Documentation of QChar related functions
 *****************************************************************************/

/*!
    \fn bool operator==(QChar c1, QChar c2)

    \relates QChar

    Returns true if \a c1 and \a c2 are the same Unicode character;
    otherwise returns false.
*/

/*!
    \fn int operator!=(QChar c1, QChar c2)

    \relates QChar

    Returns true if \a c1 and \a c2 are not the same Unicode
    character; otherwise returns false.
*/

/*!
    \fn int operator<=(QChar c1, QChar c2)

    \relates QChar

    Returns true if the numeric Unicode value of \a c1 is less than
    or equal to that of \a c2; otherwise returns false.
*/

/*!
    \fn int operator>=(QChar c1, QChar c2)

    \relates QChar

    Returns true if the numeric Unicode value of \a c1 is greater than
    or equal to that of \a c2; otherwise returns false.
*/

/*!
    \fn int operator<(QChar c1, QChar c2)

    \relates QChar

    Returns true if the numeric Unicode value of \a c1 is less than
    that of \a c2; otherwise returns false.
*/

/*!
    \fn int operator>(QChar c1, QChar c2)

    \relates QChar

    Returns true if the numeric Unicode value of \a c1 is greater than
    that of \a c2; otherwise returns false.
*/

/*!
    \fn bool QChar::mirrored() const

    Use hasMirrored() instead.
*/

/*!
    \fn QChar QChar::lower() const

    Use toLower() instead.
*/

/*!
    \fn QChar QChar::upper() const

    Use toUpper() instead.
*/

/*!
    \fn bool QChar::networkOrdered()

    See if QSysInfo::ByteOrder == QSysInfo::BigEndian instead.
*/


// ---------------------------------------------------------------------------


static void decomposeHelper(QString *str, bool canonical, QChar::UnicodeVersion version, int from)
{
    unsigned short buffer[3];

    QString &s = *str;

    const unsigned short *utf16 = reinterpret_cast<unsigned short *>(s.data());
    const unsigned short *uc = utf16 + s.length();
    while (uc != utf16 + from) {
        uint ucs4 = *(--uc);
        if (QChar(ucs4).isLowSurrogate() && uc != utf16) {
            ushort high = *(uc - 1);
            if (QChar(high).isHighSurrogate()) {
                --uc;
                ucs4 = QChar::surrogateToUcs4(high, ucs4);
            }
        }
        QChar::UnicodeVersion v = QChar::unicodeVersion(ucs4);
        if (v == QChar::Unicode_Unassigned || v > version)
            continue;
        int length;
        int tag;
        const unsigned short *d = decompositionHelper(ucs4, &length, &tag, buffer);
        if (!d || (canonical && tag != QChar::Canonical))
            continue;

        int pos = uc - utf16;
        s.replace(pos, QChar::requiresSurrogates(ucs4) ? 2 : 1, reinterpret_cast<const QChar *>(d), length);
        // since the insert invalidates the pointers and we do decomposition recursive
        utf16 = reinterpret_cast<unsigned short *>(s.data());
        uc = utf16 + pos + length;
    }
}


struct UCS2Pair {
    ushort u1;
    ushort u2;
};

inline bool operator<(ushort u1, const UCS2Pair &ligature)
{ return u1 < ligature.u1; }
inline bool operator<(const UCS2Pair &ligature, ushort u1)
{ return ligature.u1 < u1; }

static ushort ligatureHelper(ushort u1, ushort u2)
{
    // hangul L-V pair
    int LIndex = u1 - Hangul_LBase;
    if (0 <= LIndex && LIndex < Hangul_LCount) {
        int VIndex = u2 - Hangul_VBase;
        if (0 <= VIndex && VIndex < Hangul_VCount)
            return Hangul_SBase + (LIndex * Hangul_VCount + VIndex) * Hangul_TCount;
    }

    // hangul LV-T pair
    int SIndex = u1 - Hangul_SBase;
    if (0 <= SIndex && SIndex < Hangul_SCount && (SIndex % Hangul_TCount) == 0) {
        int TIndex = u2 - Hangul_TBase;
        if (0 <= TIndex && TIndex <= Hangul_TCount)
            return u1 + TIndex;
    }

    const unsigned short index = GET_LIGATURE_INDEX(u2);
    if (index == 0xffff)
        return 0;
    const unsigned short *ligatures = uc_ligature_map+index;
    ushort length = *ligatures++;
    {
        const UCS2Pair *data = reinterpret_cast<const UCS2Pair *>(ligatures);
        const UCS2Pair *r = qBinaryFind(data, data + length, u1);
        if (r != data + length)
            return r->u2;
    }

    return 0;
}

static void composeHelper(QString *str, QChar::UnicodeVersion version, int from)
{
    QString &s = *str;

    if (s.length() - from < 2)
        return;

    // the loop can partly ignore high Unicode as all ligatures are in the BMP
    int starter = 0;
    int lastCombining = 0;
    int pos = from;
    while (pos < s.length()) {
        uint uc = s.at(pos).unicode();
        if (QChar(uc).isHighSurrogate() && pos < s.length()-1) {
            ushort low = s.at(pos+1).unicode();
            if (QChar(low).isLowSurrogate()) {
                uc = QChar::surrogateToUcs4(uc, low);
                ++pos;
            }
        }
        const QUnicodeTables::Properties *p = qGetProp(uc);
        if (p->unicodeVersion == QChar::Unicode_Unassigned || p->unicodeVersion > version) {
            starter = -1; // to prevent starter == pos - 1
            lastCombining = 0;
            ++pos;
            continue;
        }
        int combining = p->combiningClass;
        if (starter == pos - 1 || combining > lastCombining) {
            // allowed to form ligature with S
            QChar ligature = ligatureHelper(s.at(starter).unicode(), uc);
            if (ligature.unicode()) {
                s[starter] = ligature;
                s.remove(pos, 1);
                continue;
            }
        }
        if (!combining)
            starter = pos;
        lastCombining = combining;
        ++pos;
    }
}


static void canonicalOrderHelper(QString *str, QChar::UnicodeVersion version, int from)
{
    QString &s = *str;
    const int l = s.length()-1;
    int pos = from;
    while (pos < l) {
        int p2 = pos+1;
        uint u1 = s.at(pos).unicode();
        if (QChar(u1).isHighSurrogate()) {
            ushort low = s.at(p2).unicode();
            if (QChar(low).isLowSurrogate()) {
                u1 = QChar::surrogateToUcs4(u1, low);
                if (p2 >= l)
                    break;
                ++p2;
            }
        }
        uint u2 = s.at(p2).unicode();
        if (QChar(u2).isHighSurrogate() && p2 < l) {
            ushort low = s.at(p2+1).unicode();
            if (QChar(low).isLowSurrogate()) {
                u2 = QChar::surrogateToUcs4(u2, low);
                ++p2;
            }
        }

        ushort c2 = 0;
        {
            const QUnicodeTables::Properties *p = qGetProp(u2);
            if (p->unicodeVersion != QChar::Unicode_Unassigned && p->unicodeVersion <= version)
                c2 = p->combiningClass;
        }
        if (c2 == 0) {
            pos = p2+1;
            continue;
        }

        ushort c1 = 0;
        {
            const QUnicodeTables::Properties *p = qGetProp(u1);
            if (p->unicodeVersion != QChar::Unicode_Unassigned && p->unicodeVersion <= version)
                c1 = p->combiningClass;
        }

        if (c1 > c2) {
            QChar *uc = s.data();
            int p = pos;
            // exchange characters
            if (!QChar::requiresSurrogates(u2)) {
                uc[p++] = u2;
            } else {
                uc[p++] = QChar::highSurrogate(u2);
                uc[p++] = QChar::lowSurrogate(u2);
            }
            if (!QChar::requiresSurrogates(u1)) {
                uc[p++] = u1;
            } else {
                uc[p++] = QChar::highSurrogate(u1);
                uc[p++] = QChar::lowSurrogate(u1);
            }
            if (pos > 0)
                --pos;
            if (pos > 0 && s.at(pos).isLowSurrogate())
                --pos;
        } else {
            ++pos;
            if (QChar::requiresSurrogates(u1))
                ++pos;
        }
    }
}

QT_END_NAMESPACE
