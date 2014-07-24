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

#include "qunicodetables_p.h"
#include "qunicodetables.cpp"

#include <algorithm>

QT_BEGIN_NAMESPACE

#define FLAG(x) (1 << (x))

/*!
    \class QLatin1Char
    \inmodule QtCore
    \brief The QLatin1Char class provides an 8-bit ASCII/Latin-1 character.

    \ingroup string-processing

    This class is only useful to construct a QChar with 8-bit character.

    \sa QChar, QLatin1String, QString
*/

/*!
    \fn const char QLatin1Char::toLatin1() const

    Converts a Latin-1 character to an 8-bit ASCII representation of the character.
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
    \inmodule QtCore
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
    operating on the full range of Unicode characters, not just for the ASCII
    range. They all return true if the character is a certain type of character;
    otherwise they return false. These classification functions are
    isNull() (returns \c true if the character is '\\0'), isPrint()
    (true if the character is any sort of printable character,
    including whitespace), isPunct() (any sort of punctation),
    isMark() (Unicode Mark), isLetter() (a letter), isNumber() (any
    sort of numeric character, not just 0-9), isLetterOrNumber(), and
    isDigit() (decimal digits). All of these are wrappers around
    category() which return the Unicode-defined category of each
    character. Some of these also calculate the derived properties
    (for example isSpace() returns \c true if the character is of category
    Separator_* or an exceptional code point from Other_Control category).

    QChar also provides direction(), which indicates the "natural"
    writing direction of this character. The joiningType() function
    indicates how the character joins with it's neighbors (needed
    mostly for Arabic or Syriac) and finally hasMirrored(), which indicates
    whether the character needs to be mirrored when it is printed in
    it's "unnatural" writing direction.

    Composed Unicode characters (like \a ring) can be converted to
    decomposed Unicode ("a" followed by "ring above") by using decomposition().

    In Unicode, comparison is not necessarily possible and case
    conversion is very difficult at best. Unicode, covering the
    "entire" world, also includes most of the world's case and
    sorting problems. operator==() and friends will do comparison
    based purely on the numeric Unicode value (code point) of the
    characters, and toUpper() and toLower() will do case changes when
    the character has a well-defined uppercase/lowercase equivalent.
    For locale-dependent comparisons, use QString::localeAwareCompare().

    The conversion functions include unicode() (to a scalar),
    toLatin1() (to scalar, but converts all non-Latin-1 characters to
    0), row() (gives the Unicode row), cell() (gives the Unicode
    cell), digitValue() (gives the integer value of any of the
    numerous digit characters), and a host of constructors.

    QChar provides constructors and cast operators that make it easy
    to convert to and from traditional 8-bit \c{char}s. If you
    defined \c QT_NO_CAST_FROM_ASCII and \c QT_NO_CAST_TO_ASCII, as
    explained in the QString documentation, you will need to
    explicitly call fromLatin1(), or use QLatin1Char,
    to construct a QChar from an 8-bit \c char, and you will need to
    call toLatin1() to get the 8-bit value back.

    For more information see
    \l{http://www.unicode.org/ucd/}{"About the Unicode Character Database"}.

    \sa Unicode, QString, QLatin1Char
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
    \value Unicode_5_1  Version 5.1
    \value Unicode_5_2  Version 5.2
    \value Unicode_6_0  Version 6.0
    \value Unicode_6_1  Version 6.1
    \value Unicode_6_2  Version 6.2
    \value Unicode_6_3  Version 6.3  Since Qt 5.3
    \value Unicode_Unassigned  The value is not assigned to any character
                               in version 6.3 of Unicode.

    \sa unicodeVersion(), currentUnicodeVersion()
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

    \sa category()
*/

/*!
    \enum QChar::Script
    \since 5.1

    This enum type defines the Unicode script property values.

    For details about the Unicode script property values see
    \l{http://www.unicode.org/reports/tr24/}{Unicode Standard Annex #24}.

    In order to conform to C/C++ naming conventions "Script_" is prepended
    to the codes used in the Unicode Standard.

    \value Script_Unknown    For unassigned, private-use, noncharacter, and surrogate code points.
    \value Script_Inherited  For characters that may be used with multiple scripts
                             and that inherit their script from the preceding characters.
                             These include nonspacing marks, enclosing marks,
                             and zero width joiner/non-joiner characters.
    \value Script_Common     For characters that may be used with multiple scripts
                             and that do not inherit their script from the preceding characters.

    \value Script_Latin
    \value Script_Greek
    \value Script_Cyrillic
    \value Script_Armenian
    \value Script_Hebrew
    \value Script_Arabic
    \value Script_Syriac
    \value Script_Thaana
    \value Script_Devanagari
    \value Script_Bengali
    \value Script_Gurmukhi
    \value Script_Gujarati
    \value Script_Oriya
    \value Script_Tamil
    \value Script_Telugu
    \value Script_Kannada
    \value Script_Malayalam
    \value Script_Sinhala
    \value Script_Thai
    \value Script_Lao
    \value Script_Tibetan
    \value Script_Myanmar
    \value Script_Georgian
    \value Script_Hangul
    \value Script_Ethiopic
    \value Script_Cherokee
    \value Script_CanadianAboriginal
    \value Script_Ogham
    \value Script_Runic
    \value Script_Khmer
    \value Script_Mongolian
    \value Script_Hiragana
    \value Script_Katakana
    \value Script_Bopomofo
    \value Script_Han
    \value Script_Yi
    \value Script_OldItalic
    \value Script_Gothic
    \value Script_Deseret
    \value Script_Tagalog
    \value Script_Hanunoo
    \value Script_Buhid
    \value Script_Tagbanwa
    \value Script_Coptic
    \value Script_Limbu
    \value Script_TaiLe
    \value Script_LinearB
    \value Script_Ugaritic
    \value Script_Shavian
    \value Script_Osmanya
    \value Script_Cypriot
    \value Script_Braille
    \value Script_Buginese
    \value Script_NewTaiLue
    \value Script_Glagolitic
    \value Script_Tifinagh
    \value Script_SylotiNagri
    \value Script_OldPersian
    \value Script_Kharoshthi
    \value Script_Balinese
    \value Script_Cuneiform
    \value Script_Phoenician
    \value Script_PhagsPa
    \value Script_Nko
    \value Script_Sundanese
    \value Script_Lepcha
    \value Script_OlChiki
    \value Script_Vai
    \value Script_Saurashtra
    \value Script_KayahLi
    \value Script_Rejang
    \value Script_Lycian
    \value Script_Carian
    \value Script_Lydian
    \value Script_Cham
    \value Script_TaiTham
    \value Script_TaiViet
    \value Script_Avestan
    \value Script_EgyptianHieroglyphs
    \value Script_Samaritan
    \value Script_Lisu
    \value Script_Bamum
    \value Script_Javanese
    \value Script_MeeteiMayek
    \value Script_ImperialAramaic
    \value Script_OldSouthArabian
    \value Script_InscriptionalParthian
    \value Script_InscriptionalPahlavi
    \value Script_OldTurkic
    \value Script_Kaithi
    \value Script_Batak
    \value Script_Brahmi
    \value Script_Mandaic
    \value Script_Chakma
    \value Script_MeroiticCursive
    \value Script_MeroiticHieroglyphs
    \value Script_Miao
    \value Script_Sharada
    \value Script_SoraSompeng
    \value Script_Takri

    \omitvalue ScriptCount

    \sa script()
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
    \value DirFSI Since Qt 5.3
    \value DirL
    \value DirLRE
    \value DirLRI Since Qt 5.3
    \value DirLRO
    \value DirNSM
    \value DirON
    \value DirPDF
    \value DirPDI Since Qt 5.3
    \value DirR
    \value DirRLE
    \value DirRLI Since Qt 5.3
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

    \sa decomposition()
*/

/*!
    \enum QChar::JoiningType
    since 5.3

    This enum type defines the Unicode joining type attributes. See the
    \l{http://www.unicode.org/}{Unicode Standard} for a description of the values.

    In order to conform to C/C++ naming conventions "Joining_" is prepended
    to the codes used in the Unicode Standard.

    \value Joining_None
    \value Joining_Causing
    \value Joining_Dual
    \value Joining_Right
    \value Joining_Left
    \value Joining_Transparent

    \sa joiningType()
*/

#if QT_DEPRECATED_SINCE(5, 3)
/*!
    \enum QChar::Joining
    \deprecated in 5.3, use JoiningType instead.

    This enum type defines the Unicode joining attributes. See the
    \l{http://www.unicode.org/}{Unicode Standard} for a description
    of the values.

    \value Center
    \value Dual
    \value OtherJoining
    \value Right

    \sa joining()
*/
#endif

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
    \value Tabulation Character tabulation.
    \value LineFeed
    \value CarriageReturn
    \value Space
    \value Nbsp Non-breaking space.
    \value SoftHyphen
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
    \value LastValidCodePoint
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
    \fn QChar::QChar(char ch)

    Constructs a QChar corresponding to ASCII/Latin-1 character \a ch.
*/

/*!
    \fn QChar::QChar(uchar ch)

    Constructs a QChar corresponding to ASCII/Latin-1 character \a ch.
*/

/*!
    \fn QChar::QChar(uchar cell, uchar row)

    Constructs a QChar for Unicode cell \a cell in row \a row.

    \sa cell(), row()
*/

/*!
    \fn QChar::QChar(ushort code)

    Constructs a QChar for the character with Unicode code point \a code.
*/

/*!
    \fn QChar::QChar(short code)

    Constructs a QChar for the character with Unicode code point \a code.
*/

/*!
    \fn QChar::QChar(uint code)

    Constructs a QChar for the character with Unicode code point \a code.
*/

/*!
    \fn QChar::QChar(int code)

    Constructs a QChar for the character with Unicode code point \a code.
*/

/*!
    \fn bool QChar::isNull() const

    Returns \c true if the character is the Unicode character 0x0000
    ('\\0'); otherwise returns \c false.
*/

/*!
    \fn uchar QChar::cell() const

    Returns the cell (least significant byte) of the Unicode character.

    \sa row()
*/

/*!
    \fn uchar QChar::row() const

    Returns the row (most significant byte) of the Unicode character.

    \sa cell()
*/

/*!
    \fn bool QChar::isPrint() const

    Returns \c true if the character is a printable character; otherwise
    returns \c false. This is any character not of category Other_*.

    Note that this gives no indication of whether the character is
    available in a particular font.
*/

/*!
    \overload
    \since 5.0

    Returns \c true if the UCS-4-encoded character specified by \a ucs4 is
    a printable character; otherwise returns \c false.
    This is any character not of category Other_*.

    Note that this gives no indication of whether the character is
    available in a particular font.
*/
bool QChar::isPrint(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return false;
    const int test = FLAG(Other_Control) |
                     FLAG(Other_Format) |
                     FLAG(Other_Surrogate) |
                     FLAG(Other_PrivateUse) |
                     FLAG(Other_NotAssigned);
    return !(FLAG(qGetProp(ucs4)->category) & test);
}

/*!
    \fn bool QChar::isSpace() const

    Returns \c true if the character is a separator character
    (Separator_* categories or certain code points from Other_Control category);
    otherwise returns \c false.
*/

/*!
    \fn bool QChar::isSpace(uint ucs4)
    \overload
    \since 5.0

    Returns \c true if the UCS-4-encoded character specified by \a ucs4 is
    a separator character (Separator_* categories or certain code points
    from Other_Control category); otherwise returns \c false.
*/

/*!
    \internal
*/
bool QT_FASTCALL QChar::isSpace_helper(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return false;
    const int test = FLAG(Separator_Space) |
                     FLAG(Separator_Line) |
                     FLAG(Separator_Paragraph);
    return FLAG(qGetProp(ucs4)->category) & test;
}

/*!
    \fn bool QChar::isMark() const

    Returns \c true if the character is a mark (Mark_* categories);
    otherwise returns \c false.

    See QChar::Category for more information regarding marks.
*/

/*!
    \overload
    \since 5.0

    Returns \c true if the UCS-4-encoded character specified by \a ucs4 is
    a mark (Mark_* categories); otherwise returns \c false.
*/
bool QChar::isMark(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return false;
    const int test = FLAG(Mark_NonSpacing) |
                     FLAG(Mark_SpacingCombining) |
                     FLAG(Mark_Enclosing);
    return FLAG(qGetProp(ucs4)->category) & test;
}

/*!
    \fn bool QChar::isPunct() const

    Returns \c true if the character is a punctuation mark (Punctuation_*
    categories); otherwise returns \c false.
*/

/*!
    \overload
    \since 5.0

    Returns \c true if the UCS-4-encoded character specified by \a ucs4 is
    a punctuation mark (Punctuation_* categories); otherwise returns \c false.
*/
bool QChar::isPunct(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return false;
    const int test = FLAG(Punctuation_Connector) |
                     FLAG(Punctuation_Dash) |
                     FLAG(Punctuation_Open) |
                     FLAG(Punctuation_Close) |
                     FLAG(Punctuation_InitialQuote) |
                     FLAG(Punctuation_FinalQuote) |
                     FLAG(Punctuation_Other);
    return FLAG(qGetProp(ucs4)->category) & test;
}

/*!
    \fn bool QChar::isSymbol() const

    Returns \c true if the character is a symbol (Symbol_* categories);
    otherwise returns \c false.
*/

/*!
    \overload
    \since 5.0

    Returns \c true if the UCS-4-encoded character specified by \a ucs4 is
    a symbol (Symbol_* categories); otherwise returns \c false.
*/
bool QChar::isSymbol(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return false;
    const int test = FLAG(Symbol_Math) |
                     FLAG(Symbol_Currency) |
                     FLAG(Symbol_Modifier) |
                     FLAG(Symbol_Other);
    return FLAG(qGetProp(ucs4)->category) & test;
}

/*!
    \fn bool QChar::isLetter() const

    Returns \c true if the character is a letter (Letter_* categories);
    otherwise returns \c false.
*/

/*!
    \fn bool QChar::isLetter(uint ucs4)
    \overload
    \since 5.0

    Returns \c true if the UCS-4-encoded character specified by \a ucs4 is
    a letter (Letter_* categories); otherwise returns \c false.
*/

/*!
    \internal
*/
bool QT_FASTCALL QChar::isLetter_helper(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return false;
    const int test = FLAG(Letter_Uppercase) |
                     FLAG(Letter_Lowercase) |
                     FLAG(Letter_Titlecase) |
                     FLAG(Letter_Modifier) |
                     FLAG(Letter_Other);
    return FLAG(qGetProp(ucs4)->category) & test;
}

/*!
    \fn bool QChar::isNumber() const

    Returns \c true if the character is a number (Number_* categories,
    not just 0-9); otherwise returns \c false.

    \sa isDigit()
*/

/*!
    \fn bool QChar::isNumber(uint ucs4)
    \overload
    \since 5.0

    Returns \c true if the UCS-4-encoded character specified by \a ucs4 is
    a number (Number_* categories, not just 0-9); otherwise returns \c false.

    \sa isDigit()
*/

/*!
    \internal
*/
bool QT_FASTCALL QChar::isNumber_helper(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return false;
    const int test = FLAG(Number_DecimalDigit) |
                     FLAG(Number_Letter) |
                     FLAG(Number_Other);
    return FLAG(qGetProp(ucs4)->category) & test;
}

/*!
    \fn bool QChar::isLetterOrNumber() const

    Returns \c true if the character is a letter or number (Letter_* or
    Number_* categories); otherwise returns \c false.
*/

/*!
    \fn bool QChar::isLetterOrNumber(uint ucs4)
    \overload
    \since 5.0

    Returns \c true if the UCS-4-encoded character specified by \a ucs4 is
    a letter or number (Letter_* or Number_* categories); otherwise returns \c false.
*/

/*!
    \internal
*/
bool QT_FASTCALL QChar::isLetterOrNumber_helper(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return false;
    const int test = FLAG(Letter_Uppercase) |
                     FLAG(Letter_Lowercase) |
                     FLAG(Letter_Titlecase) |
                     FLAG(Letter_Modifier) |
                     FLAG(Letter_Other) |
                     FLAG(Number_DecimalDigit) |
                     FLAG(Number_Letter) |
                     FLAG(Number_Other);
    return FLAG(qGetProp(ucs4)->category) & test;
}

/*!
    \fn bool QChar::isDigit() const

    Returns \c true if the character is a decimal digit
    (Number_DecimalDigit); otherwise returns \c false.

    \sa isNumber()
*/

/*!
    \fn bool QChar::isDigit(uint ucs4)
    \overload
    \since 5.0

    Returns \c true if the UCS-4-encoded character specified by \a ucs4 is
    a decimal digit (Number_DecimalDigit); otherwise returns \c false.

    \sa isNumber()
*/

/*!
    \fn bool QChar::isNonCharacter() const
    \since 5.0

    Returns \c true if the QChar is a non-character; false otherwise.

    Unicode has a certain number of code points that are classified
    as "non-characters:" that is, they can be used for internal purposes
    in applications but cannot be used for text interchange.
    Those are the last two entries each Unicode Plane ([0xfffe..0xffff],
    [0x1fffe..0x1ffff], etc.) as well as the entries in range [0xfdd0..0xfdef].
*/

/*!
    \fn bool QChar::isHighSurrogate() const

    Returns \c true if the QChar is the high part of a UTF16 surrogate
    (for example if its code point is in range [0xd800..0xdbff]); false otherwise.
*/

/*!
    \fn bool QChar::isLowSurrogate() const

    Returns \c true if the QChar is the low part of a UTF16 surrogate
    (for example if its code point is in range [0xdc00..0xdfff]); false otherwise.
*/

/*!
    \fn bool QChar::isSurrogate() const
    \since 5.0

    Returns \c true if the QChar contains a code point that is in either
    the high or the low part of the UTF-16 surrogate range
    (for example if its code point is in range [0xd800..0xdfff]); false otherwise.
*/

/*!
    \fn static bool QChar::isNonCharacter(uint ucs4)
    \overload
    \since 5.0

    Returns \c true if the UCS-4-encoded character specified by \a ucs4
    is a non-character; false otherwise.

    Unicode has a certain number of code points that are classified
    as "non-characters:" that is, they can be used for internal purposes
    in applications but cannot be used for text interchange.
    Those are the last two entries each Unicode Plane ([0xfffe..0xffff],
    [0x1fffe..0x1ffff], etc.) as well as the entries in range [0xfdd0..0xfdef].
*/

/*!
    \fn static bool QChar::isHighSurrogate(uint ucs4)
    \overload

    Returns \c true if the UCS-4-encoded character specified by \a ucs4
    is the high part of a UTF16 surrogate
    (for example if its code point is in range [0xd800..0xdbff]); false otherwise.
*/

/*!
    \fn static bool QChar::isLowSurrogate(uint ucs4)
    \overload

    Returns \c true if the UCS-4-encoded character specified by \a ucs4
    is the low part of a UTF16 surrogate
    (for example if its code point is in range [0xdc00..0xdfff]); false otherwise.
*/

/*!
    \fn static bool QChar::isSurrogate(uint ucs4)
    \overload
    \since 5.0

    Returns \c true if the UCS-4-encoded character specified by \a ucs4
    contains a code point that is in either the high or the low part of the
    UTF-16 surrogate range (for example if its code point is in range [0xd800..0xdfff]);
    false otherwise.
*/

/*!
    \fn static bool QChar::requiresSurrogates(uint ucs4)

    Returns \c true if the UCS-4-encoded character specified by \a ucs4
    can be split into the high and low parts of a UTF16 surrogate
    (for example if its code point is greater than or equals to 0x10000);
    false otherwise.
*/

/*!
    \fn static uint QChar::surrogateToUcs4(ushort high, ushort low)

    Converts a UTF16 surrogate pair with the given \a high and \a low values
    to it's UCS-4-encoded code point.
*/

/*!
    \fn static uint QChar::surrogateToUcs4(QChar high, QChar low)
    \overload

    Converts a UTF16 surrogate pair (\a high, \a low) to it's UCS-4-encoded code point.
*/

/*!
    \fn static ushort QChar::highSurrogate(uint ucs4)

    Returns the high surrogate part of a UCS-4-encoded code point.
    The returned result is undefined if \a ucs4 is smaller than 0x10000.
*/

/*!
    \fn static ushort QChar::lowSurrogate(uint ucs4)

    Returns the low surrogate part of a UCS-4-encoded code point.
    The returned result is undefined if \a ucs4 is smaller than 0x10000.
*/

/*!
    \fn int QChar::digitValue() const

    Returns the numeric value of the digit, or -1 if the character is not a digit.
*/

/*!
    \overload
    Returns the numeric value of the digit specified by the UCS-4-encoded
    character, \a ucs4, or -1 if the character is not a digit.
*/
int QChar::digitValue(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return -1;
    return qGetProp(ucs4)->digitValue;
}

/*!
    \fn QChar::Category QChar::category() const

    Returns the character's category.
*/

/*!
    \overload
    Returns the category of the UCS-4-encoded character specified by \a ucs4.
*/
QChar::Category QChar::category(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return QChar::Other_NotAssigned;
    return (QChar::Category) qGetProp(ucs4)->category;
}

/*!
    \fn QChar::Direction QChar::direction() const

    Returns the character's direction.
*/

/*!
    \overload
    Returns the direction of the UCS-4-encoded character specified by \a ucs4.
*/
QChar::Direction QChar::direction(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return QChar::DirL;
    return (QChar::Direction) qGetProp(ucs4)->direction;
}

/*!
    \fn QChar::JoiningType QChar::joiningType() const
    \since 5.3

    Returns information about the joining type attributes of the character
    (needed for certain languages such as Arabic or Syriac).
*/

/*!
    \overload
    \since 5.3

    Returns information about the joining type attributes of the UCS-4-encoded
    character specified by \a ucs4
    (needed for certain languages such as Arabic or Syriac).
*/
QChar::JoiningType QChar::joiningType(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return QChar::Joining_None;
    return QChar::JoiningType(qGetProp(ucs4)->joining);
}

#if QT_DEPRECATED_SINCE(5, 3)
/*!
    \fn QChar::Joining QChar::joining() const
    \deprecated in 5.3, use joiningType() instead.

    Returns information about the joining properties of the character
    (needed for certain languages such as Arabic).
*/

/*!
    \overload
    \deprecated in 5.3, use joiningType() instead.

    Returns information about the joining properties of the UCS-4-encoded
    character specified by \a ucs4 (needed for certain languages such as Arabic).
*/
QChar::Joining QChar::joining(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return QChar::OtherJoining;
    switch (qGetProp(ucs4)->joining) {
    case QChar::Joining_Causing: return QChar::Center;
    case QChar::Joining_Dual: return QChar::Dual;
    case QChar::Joining_Right: return QChar::Right;
    default: break;
    }
    return QChar::OtherJoining;
}
#endif

/*!
    \fn bool QChar::hasMirrored() const

    Returns \c true if the character should be reversed if the text
    direction is reversed; otherwise returns \c false.

    A bit faster equivalent of (ch.mirroredChar() != ch).

    \sa mirroredChar()
*/

/*!
    \overload
    \since 5.0

    Returns \c true if the UCS-4-encoded character specified by \a ucs4
    should be reversed if the text direction is reversed; otherwise returns \c false.

    A bit faster equivalent of (QChar::mirroredChar(ucs4) != ucs4).

    \sa mirroredChar()
*/
bool QChar::hasMirrored(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return false;
    return qGetProp(ucs4)->mirrorDiff != 0;
}

/*!
    \fn bool QChar::isLower() const

    Returns \c true if the character is a lowercase letter, for example
    category() is Letter_Lowercase.

    \sa isUpper(), toLower(), toUpper()
*/

/*!
    \fn static bool QChar::isLower(uint ucs4)
    \overload
    \since 5.0

    Returns \c true if the UCS-4-encoded character specified by \a ucs4
    is a lowercase letter, for example category() is Letter_Lowercase.

    \sa isUpper(), toLower(), toUpper()
*/

/*!
    \fn bool QChar::isUpper() const

    Returns \c true if the character is an uppercase letter, for example
    category() is Letter_Uppercase.

    \sa isLower(), toUpper(), toLower()
*/

/*!
    \fn static bool QChar::isUpper(uint ucs4)
    \overload
    \since 5.0

    Returns \c true if the UCS-4-encoded character specified by \a ucs4
    is an uppercase letter, for example category() is Letter_Uppercase.

    \sa isLower(), toUpper(), toLower()
*/

/*!
    \fn bool QChar::isTitleCase() const

    Returns \c true if the character is a titlecase letter, for example
    category() is Letter_Titlecase.

    \sa isLower(), toUpper(), toLower(), toTitleCase()
*/

/*!
    \fn static bool QChar::isTitleCase(uint ucs4)
    \overload
    \since 5.0

    Returns \c true if the UCS-4-encoded character specified by \a ucs4
    is a titlecase letter, for example category() is Letter_Titlecase.

    \sa isLower(), toUpper(), toLower(), toTitleCase()
*/
/*!
    \fn QChar QChar::mirroredChar() const

    Returns the mirrored character if this character is a mirrored
    character; otherwise returns the character itself.

    \sa hasMirrored()
*/

/*!
    \overload
    Returns the mirrored character if the UCS-4-encoded character specified
    by \a ucs4 is a mirrored character; otherwise returns the character itself.

    \sa hasMirrored()
*/
uint QChar::mirroredChar(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return ucs4;
    return ucs4 + qGetProp(ucs4)->mirrorDiff;
}


// constants for Hangul (de)composition, see UAX #15
enum {
    Hangul_SBase = 0xac00,
    Hangul_LBase = 0x1100,
    Hangul_VBase = 0x1161,
    Hangul_TBase = 0x11a7,
    Hangul_LCount = 19,
    Hangul_VCount = 21,
    Hangul_TCount = 28,
    Hangul_NCount = Hangul_VCount * Hangul_TCount,
    Hangul_SCount = Hangul_LCount * Hangul_NCount
};

// buffer has to have a length of 3. It's needed for Hangul decomposition
static const unsigned short * QT_FASTCALL decompositionHelper
    (uint ucs4, int *length, int *tag, unsigned short *buffer)
{
    if (ucs4 >= Hangul_SBase && ucs4 < Hangul_SBase + Hangul_SCount) {
        // compute Hangul syllable decomposition as per UAX #15
        const uint SIndex = ucs4 - Hangul_SBase;
        buffer[0] = Hangul_LBase + SIndex / Hangul_NCount; // L
        buffer[1] = Hangul_VBase + (SIndex % Hangul_NCount) / Hangul_TCount; // V
        buffer[2] = Hangul_TBase + SIndex % Hangul_TCount; // T
        *length = buffer[2] == Hangul_TBase ? 2 : 3;
        *tag = QChar::Canonical;
        return buffer;
    }

    const unsigned short index = GET_DECOMPOSITION_INDEX(ucs4);
    if (index == 0xffff) {
        *length = 0;
        *tag = QChar::NoDecomposition;
        return 0;
    }

    const unsigned short *decomposition = uc_decomposition_map+index;
    *tag = (*decomposition) & 0xff;
    *length = (*decomposition) >> 8;
    return decomposition+1;
}

/*!
    Decomposes a character into it's constituent parts. Returns an empty string
    if no decomposition exists.
*/
QString QChar::decomposition() const
{
    return QChar::decomposition(ucs);
}

/*!
    \overload
    Decomposes the UCS-4-encoded character specified by \a ucs4 into it's
    constituent parts. Returns an empty string if no decomposition exists.
*/
QString QChar::decomposition(uint ucs4)
{
    unsigned short buffer[3];
    int length;
    int tag;
    const unsigned short *d = decompositionHelper(ucs4, &length, &tag, buffer);
    return QString(reinterpret_cast<const QChar *>(d), length);
}

/*!
    \fn QChar::Decomposition QChar::decompositionTag() const

    Returns the tag defining the composition of the character. Returns
    QChar::NoDecomposition if no decomposition exists.
*/

/*!
    \overload
    Returns the tag defining the composition of the UCS-4-encoded character
    specified by \a ucs4. Returns QChar::NoDecomposition if no decomposition exists.
*/
QChar::Decomposition QChar::decompositionTag(uint ucs4)
{
    if (ucs4 >= Hangul_SBase && ucs4 < Hangul_SBase + Hangul_SCount)
        return QChar::Canonical;
    const unsigned short index = GET_DECOMPOSITION_INDEX(ucs4);
    if (index == 0xffff)
        return QChar::NoDecomposition;
    return (QChar::Decomposition)(uc_decomposition_map[index] & 0xff);
}

/*!
    \fn unsigned char QChar::combiningClass() const

    Returns the combining class for the character as defined in the
    Unicode standard. This is mainly useful as a positioning hint for
    marks attached to a base character.

    The Qt text rendering engine uses this information to correctly
    position non-spacing marks around a base character.
*/

/*!
    \overload
    Returns the combining class for the UCS-4-encoded character specified by
    \a ucs4, as defined in the Unicode standard.
*/
unsigned char QChar::combiningClass(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return 0;
    return (unsigned char) qGetProp(ucs4)->combiningClass;
}

/*!
    \fn QChar::Script QChar::script() const
    \since 5.1

    Returns the Unicode script property value for this character.
*/

/*!
    \overload
    \since 5.1

    Returns the Unicode script property value for the character specified in
    its UCS-4-encoded form as \a ucs4.
*/
QChar::Script QChar::script(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return QChar::Script_Unknown;
    return (QChar::Script) qGetProp(ucs4)->script;
}

/*!
    \fn QChar::UnicodeVersion QChar::unicodeVersion() const

    Returns the Unicode version that introduced this character.
*/

/*!
    \overload
    Returns the Unicode version that introduced the character specified in
    its UCS-4-encoded form as \a ucs4.
*/
QChar::UnicodeVersion QChar::unicodeVersion(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return QChar::Unicode_Unassigned;
    return (QChar::UnicodeVersion) qGetProp(ucs4)->unicodeVersion;
}

/*!
    Returns the most recent supported Unicode version.
*/
QChar::UnicodeVersion QChar::currentUnicodeVersion()
{
    return UNICODE_DATA_VERSION;
}


template <typename T>
static inline T toLowerCase_helper(T uc)
{
    const QUnicodeTables::Properties *p = qGetProp(uc);
    if (p->lowerCaseSpecial) {
        const ushort *specialCase = specialCaseMap + p->lowerCaseDiff;
        return (*specialCase == 1) ? specialCase[1] : uc;
    }
    return uc + p->lowerCaseDiff;
}

template <typename T>
static inline T toUpperCase_helper(T uc)
{
    const QUnicodeTables::Properties *p = qGetProp(uc);
    if (p->upperCaseSpecial) {
        const ushort *specialCase = specialCaseMap + p->upperCaseDiff;
        return (*specialCase == 1) ? specialCase[1] : uc;
    }
    return uc + p->upperCaseDiff;
}

template <typename T>
static inline T toTitleCase_helper(T uc)
{
    const QUnicodeTables::Properties *p = qGetProp(uc);
    if (p->titleCaseSpecial) {
        const ushort *specialCase = specialCaseMap + p->titleCaseDiff;
        return (*specialCase == 1) ? specialCase[1] : uc;
    }
    return uc + p->titleCaseDiff;
}

template <typename T>
static inline T toCaseFolded_helper(T uc)
{
    const QUnicodeTables::Properties *p = qGetProp(uc);
    if (p->caseFoldSpecial) {
        const ushort *specialCase = specialCaseMap + p->caseFoldDiff;
        return (*specialCase == 1) ? specialCase[1] : uc;
    }
    return uc + p->caseFoldDiff;
}

/*!
    \fn QChar QChar::toLower() const

    Returns the lowercase equivalent if the character is uppercase or titlecase;
    otherwise returns the character itself.
*/

/*!
    \overload
    Returns the lowercase equivalent of the UCS-4-encoded character specified
    by \a ucs4 if the character is uppercase or titlecase; otherwise returns
    the character itself.
*/
uint QChar::toLower(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return ucs4;
    return toLowerCase_helper<uint>(ucs4);
}

/*!
    \fn QChar QChar::toUpper() const

    Returns the uppercase equivalent if the character is lowercase or titlecase;
    otherwise returns the character itself.
*/

/*!
    \overload
    Returns the uppercase equivalent of the UCS-4-encoded character specified
    by \a ucs4 if the character is lowercase or titlecase; otherwise returns
    the character itself.
*/
uint QChar::toUpper(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return ucs4;
    return toUpperCase_helper<uint>(ucs4);
}

/*!
    \fn QChar QChar::toTitleCase() const

    Returns the title case equivalent if the character is lowercase or uppercase;
    otherwise returns the character itself.
*/

/*!
    \overload
    Returns the title case equivalent of the UCS-4-encoded character specified
    by \a ucs4 if the character is lowercase or uppercase; otherwise returns
    the character itself.
*/
uint QChar::toTitleCase(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return ucs4;
    return toTitleCase_helper<uint>(ucs4);
}

static inline uint foldCase(const ushort *ch, const ushort *start)
{
    uint c = *ch;
    if (QChar(c).isLowSurrogate() && ch > start && QChar(*(ch - 1)).isHighSurrogate())
        c = QChar::surrogateToUcs4(*(ch - 1), c);
    return toCaseFolded_helper<uint>(c);
}

static inline uint foldCase(uint ch, uint &last)
{
    uint c = ch;
    if (QChar(c).isLowSurrogate() && QChar(last).isHighSurrogate())
        c = QChar::surrogateToUcs4(last, c);
    last = ch;
    return toCaseFolded_helper<uint>(c);
}

static inline ushort foldCase(ushort ch)
{
    return toCaseFolded_helper<ushort>(ch);
}

/*!
    \fn QChar QChar::toCaseFolded() const

    Returns the case folded equivalent of the character.
    For most Unicode characters this is the same as toLowerCase().
*/

/*!
    \overload
    Returns the case folded equivalent of the UCS-4-encoded character specified
    by \a ucs4. For most Unicode characters this is the same as toLowerCase().
*/
uint QChar::toCaseFolded(uint ucs4)
{
    if (ucs4 > LastValidCodePoint)
        return ucs4;
    return toCaseFolded_helper<uint>(ucs4);
}

/*!
    \fn char QChar::toLatin1() const

    Returns the Latin-1 character equivalent to the QChar, or 0. This
    is mainly useful for non-internationalized software.

    \note It is not possible to distinguish a non-Latin-1 character from a Latin-1 0
    (NUL) character. Prefer to use unicode(), which does not have this ambiguity.

    \sa unicode()
*/

/*!
    \fn QChar QChar::fromLatin1(char)

    Converts the Latin-1 character \a c to its equivalent QChar. This
    is mainly useful for non-internationalized software.

    An alternative is to use QLatin1Char.

    \sa toLatin1(), unicode()
*/

/*!
    \fn char QChar::toAscii() const
    \deprecated

    Returns the Latin-1 character value of the QChar, or 0 if the character is not
    representable.

    The main purpose of this function is to preserve ASCII characters used
    in C strings. This is mainly useful for developers of non-internationalized
    software.

    \note It is not possible to distinguish a non-Latin 1 character from an ASCII 0
    (NUL) character. Prefer to use unicode(), which does not have this ambiguity.

    \note This function does not check whether the character value is inside
    the valid range of US-ASCII.

    \sa toLatin1(), unicode()
*/

/*!
    \fn QChar QChar::fromAscii(char)
    \deprecated

    Converts the ASCII character \a c to it's equivalent QChar. This
    is mainly useful for non-internationalized software.

    An alternative is to use QLatin1Char.

    \sa fromLatin1(), unicode()
*/

#ifndef QT_NO_DATASTREAM
/*!
    \relates QChar

    Writes the char \a chr to the stream \a out.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator<<(QDataStream &out, QChar chr)
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

    Returns the numeric Unicode value of the QChar.
*/

/*****************************************************************************
  Documentation of QChar related functions
 *****************************************************************************/

/*!
    \fn bool operator==(QChar c1, QChar c2)

    \relates QChar

    Returns \c true if \a c1 and \a c2 are the same Unicode character;
    otherwise returns \c false.
*/

/*!
    \fn int operator!=(QChar c1, QChar c2)

    \relates QChar

    Returns \c true if \a c1 and \a c2 are not the same Unicode
    character; otherwise returns \c false.
*/

/*!
    \fn int operator<=(QChar c1, QChar c2)

    \relates QChar

    Returns \c true if the numeric Unicode value of \a c1 is less than
    or equal to that of \a c2; otherwise returns \c false.
*/

/*!
    \fn int operator>=(QChar c1, QChar c2)

    \relates QChar

    Returns \c true if the numeric Unicode value of \a c1 is greater than
    or equal to that of \a c2; otherwise returns \c false.
*/

/*!
    \fn int operator<(QChar c1, QChar c2)

    \relates QChar

    Returns \c true if the numeric Unicode value of \a c1 is less than
    that of \a c2; otherwise returns \c false.
*/

/*!
    \fn int operator>(QChar c1, QChar c2)

    \relates QChar

    Returns \c true if the numeric Unicode value of \a c1 is greater than
    that of \a c2; otherwise returns \c false.
*/


// ---------------------------------------------------------------------------


static void decomposeHelper(QString *str, bool canonical, QChar::UnicodeVersion version, int from)
{
    int length;
    int tag;
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

        if (QChar::unicodeVersion(ucs4) > version)
            continue;

        const unsigned short *d = decompositionHelper(ucs4, &length, &tag, buffer);
        if (!d || (canonical && tag != QChar::Canonical))
            continue;

        int pos = uc - utf16;
        s.replace(pos, QChar::requiresSurrogates(ucs4) ? 2 : 1, reinterpret_cast<const QChar *>(d), length);
        // since the replace invalidates the pointers and we do decomposition recursive
        utf16 = reinterpret_cast<unsigned short *>(s.data());
        uc = utf16 + pos + length;
    }
}


struct UCS2Pair {
    ushort u1;
    ushort u2;
};

inline bool operator<(const UCS2Pair &ligature1, const UCS2Pair &ligature2)
{ return ligature1.u1 < ligature2.u1; }
inline bool operator<(ushort u1, const UCS2Pair &ligature)
{ return u1 < ligature.u1; }
inline bool operator<(const UCS2Pair &ligature, ushort u1)
{ return ligature.u1 < u1; }

struct UCS2SurrogatePair {
    UCS2Pair p1;
    UCS2Pair p2;
};

inline bool operator<(const UCS2SurrogatePair &ligature1, const UCS2SurrogatePair &ligature2)
{ return QChar::surrogateToUcs4(ligature1.p1.u1, ligature1.p1.u2) < QChar::surrogateToUcs4(ligature2.p1.u1, ligature2.p1.u2); }
inline bool operator<(uint u1, const UCS2SurrogatePair &ligature)
{ return u1 < QChar::surrogateToUcs4(ligature.p1.u1, ligature.p1.u2); }
inline bool operator<(const UCS2SurrogatePair &ligature, uint u1)
{ return QChar::surrogateToUcs4(ligature.p1.u1, ligature.p1.u2) < u1; }

static uint inline ligatureHelper(uint u1, uint u2)
{
    if (u1 >= Hangul_LBase && u1 <= Hangul_SBase + Hangul_SCount) {
        // compute Hangul syllable composition as per UAX #15
        // hangul L-V pair
        const uint LIndex = u1 - Hangul_LBase;
        if (LIndex < Hangul_LCount) {
            const uint VIndex = u2 - Hangul_VBase;
            if (VIndex < Hangul_VCount)
                return Hangul_SBase + (LIndex * Hangul_VCount + VIndex) * Hangul_TCount;
        }
        // hangul LV-T pair
        const uint SIndex = u1 - Hangul_SBase;
        if (SIndex < Hangul_SCount && (SIndex % Hangul_TCount) == 0) {
            const uint TIndex = u2 - Hangul_TBase;
            if (TIndex <= Hangul_TCount)
                return u1 + TIndex;
        }
    }

    const unsigned short index = GET_LIGATURE_INDEX(u2);
    if (index == 0xffff)
        return 0;
    const unsigned short *ligatures = uc_ligature_map+index;
    ushort length = *ligatures++;
    if (QChar::requiresSurrogates(u1)) {
        const UCS2SurrogatePair *data = reinterpret_cast<const UCS2SurrogatePair *>(ligatures);
        const UCS2SurrogatePair *r = std::lower_bound(data, data + length, u1);
        if (r != data + length && QChar::surrogateToUcs4(r->p1.u1, r->p1.u2) == u1)
            return QChar::surrogateToUcs4(r->p2.u1, r->p2.u2);
    } else {
        const UCS2Pair *data = reinterpret_cast<const UCS2Pair *>(ligatures);
        const UCS2Pair *r = std::lower_bound(data, data + length, ushort(u1));
        if (r != data + length && r->u1 == ushort(u1))
            return r->u2;
    }

    return 0;
}

static void composeHelper(QString *str, QChar::UnicodeVersion version, int from)
{
    QString &s = *str;

    if (from < 0 || s.length() - from < 2)
        return;

    uint stcode = 0; // starter code point
    int starter = -1; // starter position
    int next = -1; // to prevent i == next
    int lastCombining = 255; // to prevent combining > lastCombining

    int pos = from;
    while (pos < s.length()) {
        int i = pos;
        uint uc = s.at(pos).unicode();
        if (QChar(uc).isHighSurrogate() && pos < s.length()-1) {
            ushort low = s.at(pos+1).unicode();
            if (QChar(low).isLowSurrogate()) {
                uc = QChar::surrogateToUcs4(uc, low);
                ++pos;
            }
        }

        const QUnicodeTables::Properties *p = qGetProp(uc);
        if (p->unicodeVersion > version) {
            starter = -1;
            next = -1; // to prevent i == next
            lastCombining = 255; // to prevent combining > lastCombining
            ++pos;
            continue;
        }

        int combining = p->combiningClass;
        if ((i == next || combining > lastCombining) && starter >= from) {
            // allowed to form ligature with S
            uint ligature = ligatureHelper(stcode, uc);
            if (ligature) {
                stcode = ligature;
                QChar *d = s.data();
                // ligatureHelper() never changes planes
                if (QChar::requiresSurrogates(ligature)) {
                    d[starter] = QChar::highSurrogate(ligature);
                    d[starter + 1] = QChar::lowSurrogate(ligature);
                    s.remove(i, 2);
                } else {
                    d[starter] = ligature;
                    s.remove(i, 1);
                }
                continue;
            }
        }
        if (combining == 0) {
            starter = i;
            stcode = uc;
            next = pos + 1;
        }
        lastCombining = combining;

        ++pos;
    }
}


static void canonicalOrderHelper(QString *str, QChar::UnicodeVersion version, int from)
{
    QString &s = *str;
    const int l = s.length()-1;

    uint u1, u2;
    ushort c1, c2;

    int pos = from;
    while (pos < l) {
        int p2 = pos+1;
        u1 = s.at(pos).unicode();
        if (QChar(u1).isHighSurrogate()) {
            ushort low = s.at(p2).unicode();
            if (QChar(low).isLowSurrogate()) {
                u1 = QChar::surrogateToUcs4(u1, low);
                if (p2 >= l)
                    break;
                ++p2;
            }
        }
        c1 = 0;

    advance:
        u2 = s.at(p2).unicode();
        if (QChar(u2).isHighSurrogate() && p2 < l) {
            ushort low = s.at(p2+1).unicode();
            if (QChar(low).isLowSurrogate()) {
                u2 = QChar::surrogateToUcs4(u2, low);
                ++p2;
            }
        }

        c2 = 0;
        {
            const QUnicodeTables::Properties *p = qGetProp(u2);
            if (p->unicodeVersion <= version)
                c2 = p->combiningClass;
        }
        if (c2 == 0) {
            pos = p2+1;
            continue;
        }

        if (c1 == 0) {
            const QUnicodeTables::Properties *p = qGetProp(u1);
            if (p->unicodeVersion <= version)
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

            u1 = u2;
            c1 = c2; // != 0
            p2 = pos + 1;
            if (QChar::requiresSurrogates(u1))
                ++p2;
            if (p2 > l)
                break;

            goto advance;
        }
    }
}

// returns true if the text is in a desired Normalization Form already; false otherwise.
// sets lastStable to the position of the last stable code point
static bool normalizationQuickCheckHelper(QString *str, QString::NormalizationForm mode, int from, int *lastStable)
{
    Q_STATIC_ASSERT(QString::NormalizationForm_D == 0);
    Q_STATIC_ASSERT(QString::NormalizationForm_C == 1);
    Q_STATIC_ASSERT(QString::NormalizationForm_KD == 2);
    Q_STATIC_ASSERT(QString::NormalizationForm_KC == 3);

    enum { NFQC_YES = 0, NFQC_NO = 1, NFQC_MAYBE = 3 };

    const ushort *string = reinterpret_cast<const ushort *>(str->constData());
    int length = str->length();

    // this avoids one out of bounds check in the loop
    while (length > from && QChar::isHighSurrogate(string[length - 1]))
        --length;

    uchar lastCombining = 0;
    for (int i = from; i < length; ++i) {
        int pos = i;
        uint uc = string[i];
        if (uc < 0x80) {
            // ASCII characters are stable code points
            lastCombining = 0;
            *lastStable = pos;
            continue;
        }

        if (QChar::isHighSurrogate(uc)) {
            ushort low = string[i + 1];
            if (!QChar::isLowSurrogate(low)) {
                // treat surrogate like stable code point
                lastCombining = 0;
                *lastStable = pos;
                continue;
            }
            ++i;
            uc = QChar::surrogateToUcs4(uc, low);
        }

        const QUnicodeTables::Properties *p = qGetProp(uc);

        if (p->combiningClass < lastCombining && p->combiningClass > 0)
            return false;

        const uchar check = (p->nfQuickCheck >> (mode << 1)) & 0x03;
        if (check != NFQC_YES)
            return false; // ### can we quick check NFQC_MAYBE ?

        lastCombining = p->combiningClass;
        if (lastCombining == 0)
            *lastStable = pos;
    }

    if (length != str->length()) // low surrogate parts at the end of text
        *lastStable = str->length() - 1;

    return true;
}

QT_END_NAMESPACE
