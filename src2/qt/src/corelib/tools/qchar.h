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

#ifndef QCHAR_H
#define QCHAR_H

#include <QtCore/qglobal.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class QString;

struct QLatin1Char
{
public:
    inline explicit QLatin1Char(char c) : ch(c) {}
#ifdef Q_COMPILER_MANGLES_RETURN_TYPE
    inline const char toLatin1() const { return ch; }
    inline const ushort unicode() const { return ushort(uchar(ch)); }
#else
    inline char toLatin1() const { return ch; }
    inline ushort unicode() const { return ushort(uchar(ch)); }
#endif

private:
    char ch;
};


class Q_CORE_EXPORT QChar {
public:
    QChar();
#ifndef QT_NO_CAST_FROM_ASCII
    QT_ASCII_CAST_WARN_CONSTRUCTOR QChar(char c);
    QT_ASCII_CAST_WARN_CONSTRUCTOR QChar(uchar c);
#endif
    QChar(QLatin1Char ch);
    QChar(uchar c, uchar r);
    inline QChar(ushort rc) : ucs(rc){}
    QChar(short rc);
    QChar(uint rc);
    QChar(int rc);
    enum SpecialCharacter {
        Null = 0x0000,
        Nbsp = 0x00a0,
        ReplacementCharacter = 0xfffd,
        ObjectReplacementCharacter = 0xfffc,
        ByteOrderMark = 0xfeff,
        ByteOrderSwapped = 0xfffe,
#ifdef QT3_SUPPORT
        null = Null,
        replacement = ReplacementCharacter,
        byteOrderMark = ByteOrderMark,
        byteOrderSwapped = ByteOrderSwapped,
        nbsp = Nbsp,
#endif
        ParagraphSeparator = 0x2029,
        LineSeparator = 0x2028
    };
    QChar(SpecialCharacter sc);

    // Unicode information

    enum Category
    {
        NoCategory,    // ### Qt 5: replace with Other_NotAssigned

        Mark_NonSpacing,          //   Mn
        Mark_SpacingCombining,    //   Mc
        Mark_Enclosing,           //   Me

        Number_DecimalDigit,      //   Nd
        Number_Letter,            //   Nl
        Number_Other,             //   No

        Separator_Space,          //   Zs
        Separator_Line,           //   Zl
        Separator_Paragraph,      //   Zp

        Other_Control,            //   Cc
        Other_Format,             //   Cf
        Other_Surrogate,          //   Cs
        Other_PrivateUse,         //   Co
        Other_NotAssigned,        //   Cn

        Letter_Uppercase,         //   Lu
        Letter_Lowercase,         //   Ll
        Letter_Titlecase,         //   Lt
        Letter_Modifier,          //   Lm
        Letter_Other,             //   Lo

        Punctuation_Connector,    //   Pc
        Punctuation_Dash,         //   Pd
        Punctuation_Open,         //   Ps
        Punctuation_Close,        //   Pe
        Punctuation_InitialQuote, //   Pi
        Punctuation_FinalQuote,   //   Pf
        Punctuation_Other,        //   Po

        Symbol_Math,              //   Sm
        Symbol_Currency,          //   Sc
        Symbol_Modifier,          //   Sk
        Symbol_Other,             //   So

        Punctuation_Dask = Punctuation_Dash // ### Qt 5: remove
    };

    enum Direction
    {
        DirL, DirR, DirEN, DirES, DirET, DirAN, DirCS, DirB, DirS, DirWS, DirON,
        DirLRE, DirLRO, DirAL, DirRLE, DirRLO, DirPDF, DirNSM, DirBN
    };

    enum Decomposition
    {
        NoDecomposition,
        Canonical,
        Font,
        NoBreak,
        Initial,
        Medial,
        Final,
        Isolated,
        Circle,
        Super,
        Sub,
        Vertical,
        Wide,
        Narrow,
        Small,
        Square,
        Compat,
        Fraction

#ifdef QT3_SUPPORT
        , Single = NoDecomposition
#endif
    };

    enum Joining
    {
        OtherJoining, Dual, Right, Center
    };

    enum CombiningClass
    {
        Combining_BelowLeftAttached       = 200,
        Combining_BelowAttached           = 202,
        Combining_BelowRightAttached      = 204,
        Combining_LeftAttached            = 208,
        Combining_RightAttached           = 210,
        Combining_AboveLeftAttached       = 212,
        Combining_AboveAttached           = 214,
        Combining_AboveRightAttached      = 216,

        Combining_BelowLeft               = 218,
        Combining_Below                   = 220,
        Combining_BelowRight              = 222,
        Combining_Left                    = 224,
        Combining_Right                   = 226,
        Combining_AboveLeft               = 228,
        Combining_Above                   = 230,
        Combining_AboveRight              = 232,

        Combining_DoubleBelow             = 233,
        Combining_DoubleAbove             = 234,
        Combining_IotaSubscript           = 240
    };

    enum UnicodeVersion {
        Unicode_Unassigned,    // ### Qt 5: assign with some constantly big value
        Unicode_1_1,
        Unicode_2_0,
        Unicode_2_1_2,
        Unicode_3_0,
        Unicode_3_1,
        Unicode_3_2,
        Unicode_4_0,
        Unicode_4_1,
        Unicode_5_0
    };
    // ****** WHEN ADDING FUNCTIONS, CONSIDER ADDING TO QCharRef TOO

    Category category() const;
    Direction direction() const;
    Joining joining() const;
    bool hasMirrored() const;
    unsigned char combiningClass() const;

    QChar mirroredChar() const;
    QString decomposition() const;
    Decomposition decompositionTag() const;

    int digitValue() const;
    QChar toLower() const;
    QChar toUpper() const;
    QChar toTitleCase() const;
    QChar toCaseFolded() const;

    UnicodeVersion unicodeVersion() const;

#ifdef Q_COMPILER_MANGLES_RETURN_TYPE
    const char toAscii() const;
    inline const char toLatin1() const;
    inline const ushort unicode() const { return ucs; }
#else
    char toAscii() const;
    inline char toLatin1() const;
    inline ushort unicode() const { return ucs; }
#endif
#ifdef Q_NO_PACKED_REFERENCE
    inline ushort &unicode() { return const_cast<ushort&>(ucs); }
#else
    inline ushort &unicode() { return ucs; }
#endif

    static QChar fromAscii(char c);
    static QChar fromLatin1(char c);

    inline bool isNull() const { return ucs == 0; }
    bool isPrint() const;
    bool isPunct() const;
    bool isSpace() const;
    bool isMark() const;
    bool isLetter() const;
    bool isNumber() const;
    bool isLetterOrNumber() const;
    bool isDigit() const;
    bool isSymbol() const;
    inline bool isLower() const { return category() == Letter_Lowercase; }
    inline bool isUpper() const { return category() == Letter_Uppercase; }
    inline bool isTitleCase() const { return category() == Letter_Titlecase; }

    inline bool isHighSurrogate() const {
        return ((ucs & 0xfc00) == 0xd800);
    }
    inline bool isLowSurrogate() const {
        return ((ucs & 0xfc00) == 0xdc00);
    }

    inline uchar cell() const { return uchar(ucs & 0xff); }
    inline uchar row() const { return uchar((ucs>>8)&0xff); }
    inline void setCell(uchar cell);
    inline void setRow(uchar row);

    static inline bool isHighSurrogate(uint ucs4) {
        return ((ucs4 & 0xfffffc00) == 0xd800);
    }
    static inline bool isLowSurrogate(uint ucs4) {
        return ((ucs4 & 0xfffffc00) == 0xdc00);
    }
    static inline bool requiresSurrogates(uint ucs4) {
        return (ucs4 >= 0x10000);
    }
    static inline uint surrogateToUcs4(ushort high, ushort low) {
        return (uint(high)<<10) + low - 0x35fdc00;
    }
    static inline uint surrogateToUcs4(QChar high, QChar low) {
        return (uint(high.ucs)<<10) + low.ucs - 0x35fdc00;
    }
    static inline ushort highSurrogate(uint ucs4) {
        return ushort((ucs4>>10) + 0xd7c0);
    }
    static inline ushort lowSurrogate(uint ucs4) {
        return ushort(ucs4%0x400 + 0xdc00);
    }

    static Category QT_FASTCALL category(uint ucs4);
    static Category QT_FASTCALL category(ushort ucs2);
    static Direction QT_FASTCALL direction(uint ucs4);
    static Direction QT_FASTCALL direction(ushort ucs2);
    static Joining QT_FASTCALL joining(uint ucs4);
    static Joining QT_FASTCALL joining(ushort ucs2);
    static unsigned char QT_FASTCALL combiningClass(uint ucs4);
    static unsigned char QT_FASTCALL combiningClass(ushort ucs2);

    static uint QT_FASTCALL mirroredChar(uint ucs4);
    static ushort QT_FASTCALL mirroredChar(ushort ucs2);
    static Decomposition QT_FASTCALL decompositionTag(uint ucs4);

    static int QT_FASTCALL digitValue(uint ucs4);
    static int QT_FASTCALL digitValue(ushort ucs2);
    static uint QT_FASTCALL toLower(uint ucs4);
    static ushort QT_FASTCALL toLower(ushort ucs2);
    static uint QT_FASTCALL toUpper(uint ucs4);
    static ushort QT_FASTCALL toUpper(ushort ucs2);
    static uint QT_FASTCALL toTitleCase(uint ucs4);
    static ushort QT_FASTCALL toTitleCase(ushort ucs2);
    static uint QT_FASTCALL toCaseFolded(uint ucs4);
    static ushort QT_FASTCALL toCaseFolded(ushort ucs2);

    static UnicodeVersion QT_FASTCALL unicodeVersion(uint ucs4);
    static UnicodeVersion QT_FASTCALL unicodeVersion(ushort ucs2);

    static UnicodeVersion QT_FASTCALL currentUnicodeVersion();

    static QString QT_FASTCALL decomposition(uint ucs4);

#ifdef QT3_SUPPORT
    inline QT3_SUPPORT bool mirrored() const { return hasMirrored(); }
    inline QT3_SUPPORT QChar lower() const { return toLower(); }
    inline QT3_SUPPORT QChar upper() const { return toUpper(); }
    static inline QT3_SUPPORT bool networkOrdered() {
        return QSysInfo::ByteOrder == QSysInfo::BigEndian;
    }
#ifdef Q_COMPILER_MANGLES_RETURN_TYPE
    inline QT3_SUPPORT const char latin1() const { return toLatin1(); }
    inline QT3_SUPPORT const char ascii() const { return toAscii(); }
#else
    inline QT3_SUPPORT char latin1() const { return toLatin1(); }
    inline QT3_SUPPORT char ascii() const { return toAscii(); }
#endif
#endif

private:
#ifdef QT_NO_CAST_FROM_ASCII
    QChar(char c);
    QChar(uchar c);
#endif
    ushort ucs;
}
#if (defined(__arm__) && defined(QT_NO_ARM_EABI))
    Q_PACKED
#endif
;

Q_DECLARE_TYPEINFO(QChar, Q_MOVABLE_TYPE);

inline QChar::QChar() : ucs(0) {}

#ifdef Q_COMPILER_MANGLES_RETURN_TYPE
inline const char QChar::toLatin1() const { return ucs > 0xff ? '\0' : char(ucs); }
#else
inline char QChar::toLatin1() const { return ucs > 0xff ? '\0' : char(ucs); }
#endif
inline QChar QChar::fromLatin1(char c) { return QChar(ushort(uchar(c))); }

inline QChar::QChar(uchar c, uchar r) : ucs(ushort((r << 8) | c)){}
inline QChar::QChar(short rc) : ucs(ushort(rc)){}
inline QChar::QChar(uint rc) : ucs(ushort(rc & 0xffff)){}
inline QChar::QChar(int rc) : ucs(ushort(rc & 0xffff)){}
inline QChar::QChar(SpecialCharacter s) : ucs(ushort(s)) {}
inline QChar::QChar(QLatin1Char ch) : ucs(ch.unicode()) {}

inline void QChar::setCell(uchar acell)
{ ucs = ushort((ucs & 0xff00) + acell); }
inline void QChar::setRow(uchar arow)
{ ucs = ushort((ushort(arow)<<8) + (ucs&0xff)); }

inline bool operator==(QChar c1, QChar c2) { return c1.unicode() == c2.unicode(); }
inline bool operator!=(QChar c1, QChar c2) { return c1.unicode() != c2.unicode(); }
inline bool operator<=(QChar c1, QChar c2) { return c1.unicode() <= c2.unicode(); }
inline bool operator>=(QChar c1, QChar c2) { return c1.unicode() >= c2.unicode(); }
inline bool operator<(QChar c1, QChar c2) { return c1.unicode() < c2.unicode(); }
inline bool operator>(QChar c1, QChar c2) { return c1.unicode() > c2.unicode(); }

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QChar &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QChar &);
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QCHAR_H
