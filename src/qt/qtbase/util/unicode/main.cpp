/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the utils of the Qt Toolkit.
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

#include <qlist.h>
#include <qhash.h>
#include <qfile.h>
#include <qbytearray.h>
#include <qstring.h>
#include <qchar.h>
#include <qvector.h>
#include <qdebug.h>
#if 0
#include <private/qunicodetables_p.h>
#endif

#define DATA_VERSION_S "6.2"
#define DATA_VERSION_STR "QChar::Unicode_6_2"


static QHash<QByteArray, QChar::UnicodeVersion> age_map;

static void initAgeMap()
{
    struct AgeMap {
        const QChar::UnicodeVersion version;
        const char *age;
    } ageMap[] = {
        { QChar::Unicode_1_1,   "1.1" },
        { QChar::Unicode_2_0,   "2.0" },
        { QChar::Unicode_2_1_2, "2.1" },
        { QChar::Unicode_3_0,   "3.0" },
        { QChar::Unicode_3_1,   "3.1" },
        { QChar::Unicode_3_2,   "3.2" },
        { QChar::Unicode_4_0,   "4.0" },
        { QChar::Unicode_4_1,   "4.1" },
        { QChar::Unicode_5_0,   "5.0" },
        { QChar::Unicode_5_1,   "5.1" },
        { QChar::Unicode_5_2,   "5.2" },
        { QChar::Unicode_6_0,   "6.0" },
        { QChar::Unicode_6_1,   "6.1" },
        { QChar::Unicode_6_2,   "6.2" },
        { QChar::Unicode_6_3,   "6.3" },
        { QChar::Unicode_Unassigned, 0 }
    };
    AgeMap *d = ageMap;
    while (d->age) {
        age_map.insert(d->age, d->version);
        ++d;
    }
}


static QHash<QByteArray, QChar::Category> categoryMap;

static void initCategoryMap()
{
    struct Cat {
        QChar::Category cat;
        const char *name;
    } categories[] = {
        { QChar::Mark_NonSpacing,          "Mn" },
        { QChar::Mark_SpacingCombining,    "Mc" },
        { QChar::Mark_Enclosing,           "Me" },

        { QChar::Number_DecimalDigit,      "Nd" },
        { QChar::Number_Letter,            "Nl" },
        { QChar::Number_Other,             "No" },

        { QChar::Separator_Space,          "Zs" },
        { QChar::Separator_Line,           "Zl" },
        { QChar::Separator_Paragraph,      "Zp" },

        { QChar::Other_Control,            "Cc" },
        { QChar::Other_Format,             "Cf" },
        { QChar::Other_Surrogate,          "Cs" },
        { QChar::Other_PrivateUse,         "Co" },
        { QChar::Other_NotAssigned,        "Cn" },

        { QChar::Letter_Uppercase,         "Lu" },
        { QChar::Letter_Lowercase,         "Ll" },
        { QChar::Letter_Titlecase,         "Lt" },
        { QChar::Letter_Modifier,          "Lm" },
        { QChar::Letter_Other,             "Lo" },

        { QChar::Punctuation_Connector,    "Pc" },
        { QChar::Punctuation_Dash,         "Pd" },
        { QChar::Punctuation_Open,         "Ps" },
        { QChar::Punctuation_Close,        "Pe" },
        { QChar::Punctuation_InitialQuote, "Pi" },
        { QChar::Punctuation_FinalQuote,   "Pf" },
        { QChar::Punctuation_Other,        "Po" },

        { QChar::Symbol_Math,              "Sm" },
        { QChar::Symbol_Currency,          "Sc" },
        { QChar::Symbol_Modifier,          "Sk" },
        { QChar::Symbol_Other,             "So" },
        { QChar::Other_NotAssigned, 0 }
    };
    Cat *c = categories;
    while (c->name) {
        categoryMap.insert(c->name, c->cat);
        ++c;
    }
}


static QHash<QByteArray, QChar::Decomposition> decompositionMap;

static void initDecompositionMap()
{
    struct Dec {
        QChar::Decomposition dec;
        const char *name;
    } decompositions[] = {
        { QChar::Canonical, "<canonical>" },
        { QChar::Font, "<font>" },
        { QChar::NoBreak, "<noBreak>" },
        { QChar::Initial, "<initial>" },
        { QChar::Medial, "<medial>" },
        { QChar::Final, "<final>" },
        { QChar::Isolated, "<isolated>" },
        { QChar::Circle, "<circle>" },
        { QChar::Super, "<super>" },
        { QChar::Sub, "<sub>" },
        { QChar::Vertical, "<vertical>" },
        { QChar::Wide, "<wide>" },
        { QChar::Narrow, "<narrow>" },
        { QChar::Small, "<small>" },
        { QChar::Square, "<square>" },
        { QChar::Compat, "<compat>" },
        { QChar::Fraction, "<fraction>" },
        { QChar::NoDecomposition, 0 }
    };
    Dec *d = decompositions;
    while (d->name) {
        decompositionMap.insert(d->name, d->dec);
        ++d;
    }
}


enum Direction {
    DirL = QChar::DirL,
    DirR = QChar::DirR,
    DirEN = QChar::DirEN,
    DirES = QChar::DirES,
    DirET = QChar::DirET,
    DirAN = QChar::DirAN,
    DirCS = QChar::DirCS,
    DirB = QChar::DirB,
    DirS = QChar::DirS,
    DirWS = QChar::DirWS,
    DirON = QChar::DirON,
    DirLRE = QChar::DirLRE,
    DirLRO = QChar::DirLRO,
    DirAL = QChar::DirAL,
    DirRLE = QChar::DirRLE,
    DirRLO = QChar::DirRLO,
    DirPDF = QChar::DirPDF,
    DirNSM = QChar::DirNSM,
    DirBN = QChar::DirBN,
    DirLRI = QChar::DirLRI,
    DirRLI = QChar::DirRLI,
    DirFSI = QChar::DirFSI,
    DirPDI = QChar::DirPDI

    , Dir_Unassigned
};

static QHash<QByteArray, Direction> directionMap;

static void initDirectionMap()
{
    struct Dir {
        Direction dir;
        const char *name;
    } directions[] = {
        { DirL, "L" },
        { DirR, "R" },
        { DirEN, "EN" },
        { DirES, "ES" },
        { DirET, "ET" },
        { DirAN, "AN" },
        { DirCS, "CS" },
        { DirB, "B" },
        { DirS, "S" },
        { DirWS, "WS" },
        { DirON, "ON" },
        { DirLRE, "LRE" },
        { DirLRO, "LRO" },
        { DirAL, "AL" },
        { DirRLE, "RLE" },
        { DirRLO, "RLO" },
        { DirPDF, "PDF" },
        { DirNSM, "NSM" },
        { DirBN, "BN" },
        { DirLRI, "LRI" },
        { DirRLI, "RLI" },
        { DirFSI, "FSI" },
        { DirPDI, "PDI" },
        { Dir_Unassigned, 0 }
    };
    Dir *d = directions;
    while (d->name) {
        directionMap.insert(d->name, d->dir);
        ++d;
    }
}


enum JoiningType {
    Joining_None,
    Joining_Causing,
    Joining_Dual,
    Joining_Right,
    Joining_Left,
    Joining_Transparent

    , Joining_Unassigned
};

static QHash<QByteArray, JoiningType> joining_map;

static void initJoiningMap()
{
    struct JoiningList {
        JoiningType joining;
        const char *name;
    } joinings[] = {
        { Joining_None,        "U" },
        { Joining_Causing,     "C" },
        { Joining_Dual,        "D" },
        { Joining_Right,       "R" },
        { Joining_Left,        "L" },
        { Joining_Transparent, "T" },
        { Joining_Unassigned, 0 }
    };
    JoiningList *d = joinings;
    while (d->name) {
        joining_map.insert(d->name, d->joining);
        ++d;
    }
}


static const char *grapheme_break_class_string =
    "enum GraphemeBreakClass {\n"
    "    GraphemeBreak_Other,\n"
    "    GraphemeBreak_CR,\n"
    "    GraphemeBreak_LF,\n"
    "    GraphemeBreak_Control,\n"
    "    GraphemeBreak_Extend,\n"
    "    GraphemeBreak_RegionalIndicator,\n"
    "    GraphemeBreak_Prepend,\n"
    "    GraphemeBreak_SpacingMark,\n"
    "    GraphemeBreak_L,\n"
    "    GraphemeBreak_V,\n"
    "    GraphemeBreak_T,\n"
    "    GraphemeBreak_LV,\n"
    "    GraphemeBreak_LVT\n"
    "};\n\n";

enum GraphemeBreakClass {
    GraphemeBreak_Other,
    GraphemeBreak_CR,
    GraphemeBreak_LF,
    GraphemeBreak_Control,
    GraphemeBreak_Extend,
    GraphemeBreak_RegionalIndicator,
    GraphemeBreak_Prepend,
    GraphemeBreak_SpacingMark,
    GraphemeBreak_L,
    GraphemeBreak_V,
    GraphemeBreak_T,
    GraphemeBreak_LV,
    GraphemeBreak_LVT

    , GraphemeBreak_Unassigned
};

static QHash<QByteArray, GraphemeBreakClass> grapheme_break_map;

static void initGraphemeBreak()
{
    struct GraphemeBreakList {
        GraphemeBreakClass brk;
        const char *name;
    } breaks[] = {
        { GraphemeBreak_Other, "Other" },
        { GraphemeBreak_CR, "CR" },
        { GraphemeBreak_LF, "LF" },
        { GraphemeBreak_Control, "Control" },
        { GraphemeBreak_Extend, "Extend" },
        { GraphemeBreak_RegionalIndicator, "Regional_Indicator" },
        { GraphemeBreak_Prepend, "Prepend" },
        { GraphemeBreak_SpacingMark, "SpacingMark" },
        { GraphemeBreak_L, "L" },
        { GraphemeBreak_V, "V" },
        { GraphemeBreak_T, "T" },
        { GraphemeBreak_LV, "LV" },
        { GraphemeBreak_LVT, "LVT" },
        { GraphemeBreak_Unassigned, 0 }
    };
    GraphemeBreakList *d = breaks;
    while (d->name) {
        grapheme_break_map.insert(d->name, d->brk);
        ++d;
    }
}


static const char *word_break_class_string =
    "enum WordBreakClass {\n"
    "    WordBreak_Other,\n"
    "    WordBreak_CR,\n"
    "    WordBreak_LF,\n"
    "    WordBreak_Newline,\n"
    "    WordBreak_Extend,\n"
    "    WordBreak_RegionalIndicator,\n"
    "    WordBreak_Katakana,\n"
    "    WordBreak_HebrewLetter,\n"
    "    WordBreak_ALetter,\n"
    "    WordBreak_SingleQuote,\n"
    "    WordBreak_DoubleQuote,\n"
    "    WordBreak_MidNumLet,\n"
    "    WordBreak_MidLetter,\n"
    "    WordBreak_MidNum,\n"
    "    WordBreak_Numeric,\n"
    "    WordBreak_ExtendNumLet\n"
    "};\n\n";

enum WordBreakClass {
    WordBreak_Other,
    WordBreak_CR,
    WordBreak_LF,
    WordBreak_Newline,
    WordBreak_Extend,
    WordBreak_RegionalIndicator,
    WordBreak_Katakana,
    WordBreak_HebrewLetter,
    WordBreak_ALetter,
    WordBreak_SingleQuote,
    WordBreak_DoubleQuote,
    WordBreak_MidNumLet,
    WordBreak_MidLetter,
    WordBreak_MidNum,
    WordBreak_Numeric,
    WordBreak_ExtendNumLet

    , WordBreak_Unassigned
};

static QHash<QByteArray, WordBreakClass> word_break_map;

static void initWordBreak()
{
    struct WordBreakList {
        WordBreakClass brk;
        const char *name;
    } breaks[] = {
        { WordBreak_Other, "Other" },
        { WordBreak_CR, "CR" },
        { WordBreak_LF, "LF" },
        { WordBreak_Newline, "Newline" },
        { WordBreak_Extend, "Extend" },
        { WordBreak_Extend, "Format" },
        { WordBreak_RegionalIndicator, "Regional_Indicator" },
        { WordBreak_Katakana, "Katakana" },
        { WordBreak_HebrewLetter, "Hebrew_Letter" },
        { WordBreak_ALetter, "ALetter" },
        { WordBreak_SingleQuote, "Single_Quote" },
        { WordBreak_DoubleQuote, "Double_Quote" },
        { WordBreak_MidNumLet, "MidNumLet" },
        { WordBreak_MidLetter, "MidLetter" },
        { WordBreak_MidNum, "MidNum" },
        { WordBreak_Numeric, "Numeric" },
        { WordBreak_ExtendNumLet, "ExtendNumLet" },
        { WordBreak_Unassigned, 0 }
    };
    WordBreakList *d = breaks;
    while (d->name) {
        word_break_map.insert(d->name, d->brk);
        ++d;
    }
}


static const char *sentence_break_class_string =
    "enum SentenceBreakClass {\n"
    "    SentenceBreak_Other,\n"
    "    SentenceBreak_CR,\n"
    "    SentenceBreak_LF,\n"
    "    SentenceBreak_Sep,\n"
    "    SentenceBreak_Extend,\n"
    "    SentenceBreak_Sp,\n"
    "    SentenceBreak_Lower,\n"
    "    SentenceBreak_Upper,\n"
    "    SentenceBreak_OLetter,\n"
    "    SentenceBreak_Numeric,\n"
    "    SentenceBreak_ATerm,\n"
    "    SentenceBreak_SContinue,\n"
    "    SentenceBreak_STerm,\n"
    "    SentenceBreak_Close\n"
    "};\n\n";

enum SentenceBreakClass {
    SentenceBreak_Other,
    SentenceBreak_CR,
    SentenceBreak_LF,
    SentenceBreak_Sep,
    SentenceBreak_Extend,
    SentenceBreak_Sp,
    SentenceBreak_Lower,
    SentenceBreak_Upper,
    SentenceBreak_OLetter,
    SentenceBreak_Numeric,
    SentenceBreak_ATerm,
    SentenceBreak_SContinue,
    SentenceBreak_STerm,
    SentenceBreak_Close

    , SentenceBreak_Unassigned
};

static QHash<QByteArray, SentenceBreakClass> sentence_break_map;

static void initSentenceBreak()
{
    struct SentenceBreakList {
        SentenceBreakClass brk;
        const char *name;
    } breaks[] = {
        { SentenceBreak_Other, "Other" },
        { SentenceBreak_CR, "CR" },
        { SentenceBreak_LF, "LF" },
        { SentenceBreak_Sep, "Sep" },
        { SentenceBreak_Extend, "Extend" },
        { SentenceBreak_Extend, "Format" },
        { SentenceBreak_Sp, "Sp" },
        { SentenceBreak_Lower, "Lower" },
        { SentenceBreak_Upper, "Upper" },
        { SentenceBreak_OLetter, "OLetter" },
        { SentenceBreak_Numeric, "Numeric" },
        { SentenceBreak_ATerm, "ATerm" },
        { SentenceBreak_SContinue, "SContinue" },
        { SentenceBreak_STerm, "STerm" },
        { SentenceBreak_Close, "Close" },
        { SentenceBreak_Unassigned, 0 }
    };
    SentenceBreakList *d = breaks;
    while (d->name) {
        sentence_break_map.insert(d->name, d->brk);
        ++d;
    }
}


static const char *line_break_class_string =
    "// see http://www.unicode.org/reports/tr14/tr14-30.html\n"
    "// we don't use the XX and AI classes and map them to AL instead.\n"
    "enum LineBreakClass {\n"
    "    LineBreak_OP, LineBreak_CL, LineBreak_CP, LineBreak_QU, LineBreak_GL,\n"
    "    LineBreak_NS, LineBreak_EX, LineBreak_SY, LineBreak_IS, LineBreak_PR,\n"
    "    LineBreak_PO, LineBreak_NU, LineBreak_AL, LineBreak_HL, LineBreak_ID,\n"
    "    LineBreak_IN, LineBreak_HY, LineBreak_BA, LineBreak_BB, LineBreak_B2,\n"
    "    LineBreak_ZW, LineBreak_CM, LineBreak_WJ, LineBreak_H2, LineBreak_H3,\n"
    "    LineBreak_JL, LineBreak_JV, LineBreak_JT, LineBreak_RI, LineBreak_CB,\n"
    "    LineBreak_SA, LineBreak_SG, LineBreak_SP, LineBreak_CR, LineBreak_LF,\n"
    "    LineBreak_BK\n"
    "};\n\n";

enum LineBreakClass {
    LineBreak_OP, LineBreak_CL, LineBreak_CP, LineBreak_QU, LineBreak_GL,
    LineBreak_NS, LineBreak_EX, LineBreak_SY, LineBreak_IS, LineBreak_PR,
    LineBreak_PO, LineBreak_NU, LineBreak_AL, LineBreak_HL, LineBreak_ID,
    LineBreak_IN, LineBreak_HY, LineBreak_BA, LineBreak_BB, LineBreak_B2,
    LineBreak_ZW, LineBreak_CM, LineBreak_WJ, LineBreak_H2, LineBreak_H3,
    LineBreak_JL, LineBreak_JV, LineBreak_JT, LineBreak_RI, LineBreak_CB,
    LineBreak_SA, LineBreak_SG, LineBreak_SP, LineBreak_CR, LineBreak_LF,
    LineBreak_BK

    , LineBreak_Unassigned
};

static QHash<QByteArray, LineBreakClass> line_break_map;

static void initLineBreak()
{
    // ### Classes XX and AI are left out and mapped to AL for now.
    // ### Class NL is mapped to BK.
    // ### Treating characters of class CJ as class NS will give CSS strict line breaking;
    //     treating them as class ID will give CSS normal breaking.
    struct LineBreakList {
        LineBreakClass brk;
        const char *name;
    } breaks[] = {
        { LineBreak_BK, "BK" },
        { LineBreak_CR, "CR" },
        { LineBreak_LF, "LF" },
        { LineBreak_CM, "CM" },
        { LineBreak_BK, "NL" },
        { LineBreak_SG, "SG" },
        { LineBreak_WJ, "WJ" },
        { LineBreak_ZW, "ZW" },
        { LineBreak_GL, "GL" },
        { LineBreak_SP, "SP" },
        { LineBreak_B2, "B2" },
        { LineBreak_BA, "BA" },
        { LineBreak_BB, "BB" },
        { LineBreak_HY, "HY" },
        { LineBreak_CB, "CB" },
        { LineBreak_NS, "CJ" },
        { LineBreak_CL, "CL" },
        { LineBreak_CP, "CP" },
        { LineBreak_EX, "EX" },
        { LineBreak_IN, "IN" },
        { LineBreak_NS, "NS" },
        { LineBreak_OP, "OP" },
        { LineBreak_QU, "QU" },
        { LineBreak_IS, "IS" },
        { LineBreak_NU, "NU" },
        { LineBreak_PO, "PO" },
        { LineBreak_PR, "PR" },
        { LineBreak_SY, "SY" },
        { LineBreak_AL, "AI" },
        { LineBreak_AL, "AL" },
        { LineBreak_HL, "HL" },
        { LineBreak_H2, "H2" },
        { LineBreak_H3, "H3" },
        { LineBreak_ID, "ID" },
        { LineBreak_JL, "JL" },
        { LineBreak_JV, "JV" },
        { LineBreak_JT, "JT" },
        { LineBreak_RI, "RI" },
        { LineBreak_SA, "SA" },
        { LineBreak_AL, "XX" },
        { LineBreak_Unassigned, 0 }
    };
    LineBreakList *d = breaks;
    while (d->name) {
        line_break_map.insert(d->name, d->brk);
        ++d;
    }
}


static QHash<QByteArray, QChar::Script> scriptMap;

static void initScriptMap()
{
    struct Scrpt {
        QChar::Script script;
        const char *name;
    } scripts[] = {
        // general
        { QChar::Script_Unknown,                "Unknown" },
        { QChar::Script_Inherited,              "Inherited" },
        { QChar::Script_Common,                 "Common" },
        // pre-4.0
        { QChar::Script_Latin,                  "Latin" },
        { QChar::Script_Greek,                  "Greek" },
        { QChar::Script_Cyrillic,               "Cyrillic" },
        { QChar::Script_Armenian,               "Armenian" },
        { QChar::Script_Hebrew,                 "Hebrew" },
        { QChar::Script_Arabic,                 "Arabic" },
        { QChar::Script_Syriac,                 "Syriac" },
        { QChar::Script_Thaana,                 "Thaana" },
        { QChar::Script_Devanagari,             "Devanagari" },
        { QChar::Script_Bengali,                "Bengali" },
        { QChar::Script_Gurmukhi,               "Gurmukhi" },
        { QChar::Script_Gujarati,               "Gujarati" },
        { QChar::Script_Oriya,                  "Oriya" },
        { QChar::Script_Tamil,                  "Tamil" },
        { QChar::Script_Telugu,                 "Telugu" },
        { QChar::Script_Kannada,                "Kannada" },
        { QChar::Script_Malayalam,              "Malayalam" },
        { QChar::Script_Sinhala,                "Sinhala" },
        { QChar::Script_Thai,                   "Thai" },
        { QChar::Script_Lao,                    "Lao" },
        { QChar::Script_Tibetan,                "Tibetan" },
        { QChar::Script_Myanmar,                "Myanmar" },
        { QChar::Script_Georgian,               "Georgian" },
        { QChar::Script_Hangul,                 "Hangul" },
        { QChar::Script_Ethiopic,               "Ethiopic" },
        { QChar::Script_Cherokee,               "Cherokee" },
        { QChar::Script_CanadianAboriginal,     "CanadianAboriginal" },
        { QChar::Script_Ogham,                  "Ogham" },
        { QChar::Script_Runic,                  "Runic" },
        { QChar::Script_Khmer,                  "Khmer" },
        { QChar::Script_Mongolian,              "Mongolian" },
        { QChar::Script_Hiragana,               "Hiragana" },
        { QChar::Script_Katakana,               "Katakana" },
        { QChar::Script_Bopomofo,               "Bopomofo" },
        { QChar::Script_Han,                    "Han" },
        { QChar::Script_Yi,                     "Yi" },
        { QChar::Script_OldItalic,              "OldItalic" },
        { QChar::Script_Gothic,                 "Gothic" },
        { QChar::Script_Deseret,                "Deseret" },
        { QChar::Script_Tagalog,                "Tagalog" },
        { QChar::Script_Hanunoo,                "Hanunoo" },
        { QChar::Script_Buhid,                  "Buhid" },
        { QChar::Script_Tagbanwa,               "Tagbanwa" },
        { QChar::Script_Coptic,                 "Coptic" },
        // 4.0
        { QChar::Script_Limbu,                  "Limbu" },
        { QChar::Script_TaiLe,                  "TaiLe" },
        { QChar::Script_LinearB,                "LinearB" },
        { QChar::Script_Ugaritic,               "Ugaritic" },
        { QChar::Script_Shavian,                "Shavian" },
        { QChar::Script_Osmanya,                "Osmanya" },
        { QChar::Script_Cypriot,                "Cypriot" },
        { QChar::Script_Braille,                "Braille" },
        // 4.1
        { QChar::Script_Buginese,               "Buginese" },
        { QChar::Script_NewTaiLue,              "NewTaiLue" },
        { QChar::Script_Glagolitic,             "Glagolitic" },
        { QChar::Script_Tifinagh,               "Tifinagh" },
        { QChar::Script_SylotiNagri,            "SylotiNagri" },
        { QChar::Script_OldPersian,             "OldPersian" },
        { QChar::Script_Kharoshthi,             "Kharoshthi" },
        // 5.0
        { QChar::Script_Balinese,               "Balinese" },
        { QChar::Script_Cuneiform,              "Cuneiform" },
        { QChar::Script_Phoenician,             "Phoenician" },
        { QChar::Script_PhagsPa,                "PhagsPa" },
        { QChar::Script_Nko,                    "Nko" },
        // 5.1
        { QChar::Script_Sundanese,              "Sundanese" },
        { QChar::Script_Lepcha,                 "Lepcha" },
        { QChar::Script_OlChiki,                "OlChiki" },
        { QChar::Script_Vai,                    "Vai" },
        { QChar::Script_Saurashtra,             "Saurashtra" },
        { QChar::Script_KayahLi,                "KayahLi" },
        { QChar::Script_Rejang,                 "Rejang" },
        { QChar::Script_Lycian,                 "Lycian" },
        { QChar::Script_Carian,                 "Carian" },
        { QChar::Script_Lydian,                 "Lydian" },
        { QChar::Script_Cham,                   "Cham" },
        // 5.2
        { QChar::Script_TaiTham,                "TaiTham" },
        { QChar::Script_TaiViet,                "TaiViet" },
        { QChar::Script_Avestan,                "Avestan" },
        { QChar::Script_EgyptianHieroglyphs,    "EgyptianHieroglyphs" },
        { QChar::Script_Samaritan,              "Samaritan" },
        { QChar::Script_Lisu,                   "Lisu" },
        { QChar::Script_Bamum,                  "Bamum" },
        { QChar::Script_Javanese,               "Javanese" },
        { QChar::Script_MeeteiMayek,            "MeeteiMayek" },
        { QChar::Script_ImperialAramaic,        "ImperialAramaic" },
        { QChar::Script_OldSouthArabian,        "OldSouthArabian" },
        { QChar::Script_InscriptionalParthian,  "InscriptionalParthian" },
        { QChar::Script_InscriptionalPahlavi,   "InscriptionalPahlavi" },
        { QChar::Script_OldTurkic,              "OldTurkic" },
        { QChar::Script_Kaithi,                 "Kaithi" },
        // 6.0
        { QChar::Script_Batak,                  "Batak" },
        { QChar::Script_Brahmi,                 "Brahmi" },
        { QChar::Script_Mandaic,                "Mandaic" },
        // 6.1
        { QChar::Script_Chakma,                 "Chakma" },
        { QChar::Script_MeroiticCursive,        "MeroiticCursive" },
        { QChar::Script_MeroiticHieroglyphs,    "MeroiticHieroglyphs" },
        { QChar::Script_Miao,                   "Miao" },
        { QChar::Script_Sharada,                "Sharada" },
        { QChar::Script_SoraSompeng,            "SoraSompeng" },
        { QChar::Script_Takri,                  "Takri" },
        // unhandled
        { QChar::Script_Unknown,                0 }
    };
    Scrpt *p = scripts;
    while (p->name) {
        scriptMap.insert(p->name, p->script);
        ++p;
    }
}


// Keep this one in sync with the code in createPropertyInfo
static const char *property_string =
    "struct Properties {\n"
    "    ushort category            : 8; /* 5 used */\n"
    "    ushort direction           : 8; /* 5 used */\n"
    "    ushort combiningClass      : 8;\n"
    "    ushort joining             : 3;\n"
    "    signed short digitValue    : 5; /* 5 used */\n"
    "    signed short mirrorDiff    : 16;\n"
    "    signed short lowerCaseDiff : 16;\n"
    "    signed short upperCaseDiff : 16;\n"
    "    signed short titleCaseDiff : 16;\n"
    "    signed short caseFoldDiff  : 16;\n"
    "    ushort lowerCaseSpecial    : 1;\n"
    "    ushort upperCaseSpecial    : 1;\n"
    "    ushort titleCaseSpecial    : 1;\n"
    "    ushort caseFoldSpecial     : 1;\n"
    "    ushort unicodeVersion      : 4;\n"
    "    ushort nfQuickCheck        : 8;\n" // could be narrowed
    "    ushort graphemeBreakClass  : 4; /* 4 used */\n"
    "    ushort wordBreakClass      : 4; /* 4 used */\n"
    "    ushort sentenceBreakClass  : 8; /* 4 used */\n"
    "    ushort lineBreakClass      : 8; /* 6 used */\n"
    "    ushort script              : 8; /* 7 used */\n"
    "};\n\n"
    "Q_CORE_EXPORT const Properties * QT_FASTCALL properties(uint ucs4);\n"
    "Q_CORE_EXPORT const Properties * QT_FASTCALL properties(ushort ucs2);\n"
    "\n";

static const char *methods =
    "Q_CORE_EXPORT GraphemeBreakClass QT_FASTCALL graphemeBreakClass(uint ucs4);\n"
    "inline GraphemeBreakClass graphemeBreakClass(QChar ch)\n"
    "{ return graphemeBreakClass(ch.unicode()); }\n"
    "\n"
    "Q_CORE_EXPORT WordBreakClass QT_FASTCALL wordBreakClass(uint ucs4);\n"
    "inline WordBreakClass wordBreakClass(QChar ch)\n"
    "{ return wordBreakClass(ch.unicode()); }\n"
    "\n"
    "Q_CORE_EXPORT SentenceBreakClass QT_FASTCALL sentenceBreakClass(uint ucs4);\n"
    "inline SentenceBreakClass sentenceBreakClass(QChar ch)\n"
    "{ return sentenceBreakClass(ch.unicode()); }\n"
    "\n"
    "Q_CORE_EXPORT LineBreakClass QT_FASTCALL lineBreakClass(uint ucs4);\n"
    "inline LineBreakClass lineBreakClass(QChar ch)\n"
    "{ return lineBreakClass(ch.unicode()); }\n"
    "\n";

static const int SizeOfPropertiesStruct = 20;

struct PropertyFlags {
    bool operator==(const PropertyFlags &o) const {
        return (combiningClass == o.combiningClass
                && category == o.category
                && direction == o.direction
                && joining == o.joining
                && age == o.age
                && digitValue == o.digitValue
                && mirrorDiff == o.mirrorDiff
                && lowerCaseDiff == o.lowerCaseDiff
                && upperCaseDiff == o.upperCaseDiff
                && titleCaseDiff == o.titleCaseDiff
                && caseFoldDiff == o.caseFoldDiff
                && lowerCaseSpecial == o.lowerCaseSpecial
                && upperCaseSpecial == o.upperCaseSpecial
                && titleCaseSpecial == o.titleCaseSpecial
                && caseFoldSpecial == o.caseFoldSpecial
                && graphemeBreakClass == o.graphemeBreakClass
                && wordBreakClass == o.wordBreakClass
                && sentenceBreakClass == o.sentenceBreakClass
                && lineBreakClass == o.lineBreakClass
                && script == o.script
                && nfQuickCheck == o.nfQuickCheck
            );
    }
    // from UnicodeData.txt
    uchar combiningClass : 8;
    QChar::Category category : 5;
    QChar::Direction direction : 5;
    // from ArabicShaping.txt
    QChar::JoiningType joining : 3;
    // from DerivedAge.txt
    QChar::UnicodeVersion age : 4;
    int digitValue;

    int mirrorDiff : 16;

    int lowerCaseDiff;
    int upperCaseDiff;
    int titleCaseDiff;
    int caseFoldDiff;
    bool lowerCaseSpecial;
    bool upperCaseSpecial;
    bool titleCaseSpecial;
    bool caseFoldSpecial;
    GraphemeBreakClass graphemeBreakClass;
    WordBreakClass wordBreakClass;
    SentenceBreakClass sentenceBreakClass;
    LineBreakClass lineBreakClass;
    int script;
    // from DerivedNormalizationProps.txt
    uchar nfQuickCheck;
};


static QList<int> specialCaseMap;

static int appendToSpecialCaseMap(const QList<int> &map)
{
    QList<int> utf16map;
    for (int i = 0; i < map.size(); ++i) {
        int val = map.at(i);
        if (QChar::requiresSurrogates(val)) {
            utf16map << QChar::highSurrogate(val);
            utf16map << QChar::lowSurrogate(val);
        } else {
            utf16map << val;
        }
    }
    int length = utf16map.size();
    utf16map.prepend(length);

    if (specialCaseMap.isEmpty())
        specialCaseMap << 0; // placeholder

    int i = 1;
    while (i < specialCaseMap.size()) {
        int n = specialCaseMap.at(i);
        if (n == length) {
            int j;
            for (j = 1; j <= n; ++j) {
                if (specialCaseMap.at(i+j) != utf16map.at(j))
                    break;
            }
            if (j > n)
                return i;
        }
        i += n + 1;
    }

    int pos = specialCaseMap.size();
    specialCaseMap << utf16map;
    return pos;
}

static inline bool isDefaultIgnorable(uint ucs4)
{
    // Default_Ignorable_Code_Point:
    //  Generated from
    //    Other_Default_Ignorable_Code_Point + Cf + Variation_Selector
    //    - White_Space - FFF9..FFFB (Annotation Characters)
    //    - 0600..0604, 06DD, 070F, 110BD (exceptional Cf characters that should be visible)
    if (ucs4 <= 0xff)
        return ucs4 == 0xad;

    return ucs4 == 0x034f
            || (ucs4 >= 0x115f && ucs4 <= 0x1160)
            || (ucs4 >= 0x17b4 && ucs4 <= 0x17b5)
            || (ucs4 >= 0x180b && ucs4 <= 0x180d)
            || (ucs4 >= 0x200b && ucs4 <= 0x200f)
            || (ucs4 >= 0x202a && ucs4 <= 0x202e)
            || (ucs4 >= 0x2060 && ucs4 <= 0x206f)
            || ucs4 == 0x3164
            || (ucs4 >= 0xfe00 && ucs4 <= 0xfe0f)
            || ucs4 == 0xfeff
            || ucs4 == 0xffa0
            || (ucs4 >= 0xfff0 && ucs4 <= 0xfff8)
            || (ucs4 >= 0x1d173 && ucs4 <= 0xe0fff && (ucs4 <= 0x1d17a || ucs4 >= 0xe0000));
}

struct UnicodeData {
    UnicodeData(int codepoint = 0) {
        p.category = QChar::Other_NotAssigned; // Cn
        p.combiningClass = 0;

        p.direction = QChar::DirL;
        // DerivedBidiClass.txt
        // The unassigned code points that default to AL are in the ranges:
        //     [U+0600..U+07BF, U+08A0..U+08FF, U+FB50..U+FDCF, U+FDF0..U+FDFF, U+FE70..U+FEFF, U+1EE00..U+1EEFF]
        if ((codepoint >= 0x0600 && codepoint <= 0x07BF)
            || (codepoint >= 0x08A0 && codepoint <= 0x08FF)
            || (codepoint >= 0xFB50 && codepoint <= 0xFDCF)
            || (codepoint >= 0xFDF0 && codepoint <= 0xFDFF)
            || (codepoint >= 0xFE70 && codepoint <= 0xFEFF)
            || (codepoint >= 0x1EE00 && codepoint <= 0x1EEFF)) {
            p.direction = QChar::DirAL;
        }
        // The unassigned code points that default to R are in the ranges:
        //     [U+0590..U+05FF, U+07C0..U+089F, U+FB1D..U+FB4F, U+10800..U+10FFF, U+1E800..U+1EDFF, U+1EF00..U+1EFFF]
        else if ((codepoint >= 0x0590 && codepoint <= 0x05FF)
            || (codepoint >= 0x07C0 && codepoint <= 0x089F)
            || (codepoint >= 0xFB1D && codepoint <= 0xFB4F)
            || (codepoint >= 0x10800 && codepoint <= 0x10FFF)
            || (codepoint >= 0x1E800 && codepoint <= 0x1EDFF)
            || (codepoint >= 0x1EF00 && codepoint <= 0x1EFFF)) {
            p.direction = QChar::DirR;
        }
        // The unassigned code points that default to ET are in the range:
        //     [U+20A0..U+20CF]
        else if (codepoint >= 0x20A0 && codepoint <= 0x20CF) {
            p.direction = QChar::DirET;
        }
        // The unassigned code points that default to BN have one of the following properties:
        //     Default_Ignorable_Code_Point
        //     Noncharacter_Code_Point
        else if (QChar::isNonCharacter(codepoint) || isDefaultIgnorable(codepoint)) {
            p.direction = QChar::DirBN;
        }

        p.lineBreakClass = LineBreak_AL; // XX -> AL
        // LineBreak.txt
        // The unassigned code points that default to "ID" include ranges in the following blocks:
        //     [U+3400..U+4DBF, U+4E00..U+9FFF, U+F900..U+FAFF, U+20000..U+2A6DF, U+2A700..U+2B73F, U+2B740..U+2B81F, U+2F800..U+2FA1F, U+20000..U+2FFFD, U+30000..U+3FFFD]
        if ((codepoint >= 0x3400 && codepoint <= 0x4DBF)
            || (codepoint >= 0x4E00 && codepoint <= 0x9FFF)
            || (codepoint >= 0xF900 && codepoint <= 0xFAFF)
            || (codepoint >= 0x20000 && codepoint <= 0x2A6DF)
            || (codepoint >= 0x2A700 && codepoint <= 0x2B73F)
            || (codepoint >= 0x2B740 && codepoint <= 0x2B81F)
            || (codepoint >= 0x2F800 && codepoint <= 0x2FA1F)
            || (codepoint >= 0x20000 && codepoint <= 0x2FFFD)
            || (codepoint >= 0x30000 && codepoint <= 0x3FFFD)) {
            p.lineBreakClass = LineBreak_ID;
        }
        // The unassigned code points that default to "PR" comprise a range in the following block:
        //     [U+20A0..U+20CF]
        else if (codepoint >= 0x20A0 && codepoint <= 0x20CF) {
            p.lineBreakClass = LineBreak_PR;
        }

        mirroredChar = 0;
        decompositionType = QChar::NoDecomposition;
        p.joining = QChar::Joining_None;
        p.age = QChar::Unicode_Unassigned;
        p.mirrorDiff = 0;
        p.digitValue = -1;
        p.lowerCaseDiff = 0;
        p.upperCaseDiff = 0;
        p.titleCaseDiff = 0;
        p.caseFoldDiff = 0;
        p.lowerCaseSpecial = 0;
        p.upperCaseSpecial = 0;
        p.titleCaseSpecial = 0;
        p.caseFoldSpecial = 0;
        p.graphemeBreakClass = GraphemeBreak_Other;
        p.wordBreakClass = WordBreak_Other;
        p.sentenceBreakClass = SentenceBreak_Other;
        p.script = QChar::Script_Unknown;
        p.nfQuickCheck = 0;
        propertyIndex = -1;
        excludedComposition = false;
    }

    static UnicodeData &valueRef(int codepoint);

    PropertyFlags p;

    // from UnicodeData.txt
    QChar::Decomposition decompositionType;
    QList<int> decomposition;

    QList<int> specialFolding;

    // from BidiMirroring.txt
    int mirroredChar;

    // DerivedNormalizationProps.txt
    bool excludedComposition;

    // computed position of unicode property set
    int propertyIndex;
};

static QList<UnicodeData> unicodeData;

UnicodeData &UnicodeData::valueRef(int codepoint)
{
    static bool initialized = false;
    if (!initialized) {
        unicodeData.reserve(QChar::LastValidCodePoint + 1);
        for (int uc = 0; uc <= QChar::LastValidCodePoint; ++uc)
            unicodeData.append(UnicodeData(uc));
        initialized = true;
    }

    Q_ASSERT(codepoint <= 0x10ffff);
    return unicodeData[codepoint];
}


static QHash<int, int> decompositionLength;
static int highestComposedCharacter = 0;
static int numLigatures = 0;
static int highestLigature = 0;

struct Ligature {
    int u1;
    int u2;
    int ligature;
};
// we need them sorted after the first component for fast lookup
bool operator < (const Ligature &l1, const Ligature &l2)
{ return l1.u1 < l2.u1; }

static QHash<int, QList<Ligature> > ligatureHashes;

static QHash<int, int> combiningClassUsage;

static int maxLowerCaseDiff = 0;
static int maxUpperCaseDiff = 0;
static int maxTitleCaseDiff = 0;

static void readUnicodeData()
{
    qDebug("Reading UnicodeData.txt");

    enum UniDataFields {
        UD_Value,
        UD_Name,
        UD_Category,
        UD_CombiningClass,
        UD_BidiCategory,
        UD_Decomposition,
        UD_DecimalDigitValue,
        UD_DigitValue,
        UD_NumericValue,
        UD_Mirrored,
        UD_OldName,
        UD_Comment,
        UD_UpperCase,
        UD_LowerCase,
        UD_TitleCase
    };

    QFile f("data/UnicodeData.txt");
    if (!f.exists())
        qFatal("Couldn't find UnicodeData.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.truncate(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        if (line.isEmpty())
            continue;

        QList<QByteArray> properties = line.split(';');
        bool ok;
        int codepoint = properties[UD_Value].toInt(&ok, 16);
        Q_ASSERT(ok);
        Q_ASSERT(codepoint <= QChar::LastValidCodePoint);
        int lastCodepoint = codepoint;

        QByteArray name = properties[UD_Name];
        if (name.startsWith('<') && name.contains("First")) {
            QByteArray nextLine;
            nextLine.resize(1024);
            f.readLine(nextLine.data(), 1024);
            QList<QByteArray> properties = nextLine.split(';');
            Q_ASSERT(properties[UD_Name].startsWith('<') && properties[UD_Name].contains("Last"));
            lastCodepoint = properties[UD_Value].toInt(&ok, 16);
            Q_ASSERT(ok);
            Q_ASSERT(lastCodepoint <= QChar::LastValidCodePoint);
        }

        UnicodeData &data = UnicodeData::valueRef(codepoint);
        data.p.category = categoryMap.value(properties[UD_Category], QChar::Other_NotAssigned);
        data.p.combiningClass = properties[UD_CombiningClass].toInt();
        if (!combiningClassUsage.contains(data.p.combiningClass))
            combiningClassUsage[data.p.combiningClass] = 1;
        else
            ++combiningClassUsage[data.p.combiningClass];

        Direction dir = directionMap.value(properties[UD_BidiCategory], Dir_Unassigned);
        if (dir == Dir_Unassigned)
            qFatal("unhandled direction value: %s", properties[UD_BidiCategory].constData());
        data.p.direction = QChar::Direction(dir);

        if (!properties[UD_UpperCase].isEmpty()) {
            int upperCase = properties[UD_UpperCase].toInt(&ok, 16);
            Q_ASSERT(ok);
            int diff = upperCase - codepoint;
            if (qAbs(diff) >= (1<<14)) {
                qWarning() << "upperCaseDiff exceeded (" << hex << codepoint << "->" << upperCase << "); map it for special case";
                // if the condition below doesn't hold anymore we need to modify our special upper casing code in qchar.cpp
                Q_ASSERT(!QChar::requiresSurrogates(codepoint) && !QChar::requiresSurrogates(upperCase));
                data.p.upperCaseSpecial = true;
                data.p.upperCaseDiff = appendToSpecialCaseMap(QList<int>() << upperCase);
            } else {
                data.p.upperCaseDiff = diff;
                maxUpperCaseDiff = qMax(maxUpperCaseDiff, qAbs(diff));
            }
            if (QChar::requiresSurrogates(codepoint) || QChar::requiresSurrogates(upperCase)) {
                // if the conditions below doesn't hold anymore we need to modify our upper casing code
                Q_ASSERT(QChar::highSurrogate(codepoint) == QChar::highSurrogate(upperCase));
                Q_ASSERT(QChar::lowSurrogate(codepoint) + diff == QChar::lowSurrogate(upperCase));
            }
        }
        if (!properties[UD_LowerCase].isEmpty()) {
            int lowerCase = properties[UD_LowerCase].toInt(&ok, 16);
            Q_ASSERT(ok);
            int diff = lowerCase - codepoint;
            if (qAbs(diff) >= (1<<14)) {
                qWarning() << "lowerCaseDiff exceeded (" << hex << codepoint << "->" << lowerCase << "); map it for special case";
                // if the condition below doesn't hold anymore we need to modify our special lower casing code in qchar.cpp
                Q_ASSERT(!QChar::requiresSurrogates(codepoint) && !QChar::requiresSurrogates(lowerCase));
                data.p.lowerCaseSpecial = true;
                data.p.lowerCaseDiff = appendToSpecialCaseMap(QList<int>() << lowerCase);
            } else {
                data.p.lowerCaseDiff = diff;
                maxLowerCaseDiff = qMax(maxLowerCaseDiff, qAbs(diff));
            }
            if (QChar::requiresSurrogates(codepoint) || QChar::requiresSurrogates(lowerCase)) {
                // if the conditions below doesn't hold anymore we need to modify our lower casing code
                Q_ASSERT(QChar::highSurrogate(codepoint) == QChar::highSurrogate(lowerCase));
                Q_ASSERT(QChar::lowSurrogate(codepoint) + diff == QChar::lowSurrogate(lowerCase));
            }
        }
        // we want toTitleCase to map to ToUpper in case we don't have any titlecase.
        if (properties[UD_TitleCase].isEmpty())
            properties[UD_TitleCase] = properties[UD_UpperCase];
        if (!properties[UD_TitleCase].isEmpty()) {
            int titleCase = properties[UD_TitleCase].toInt(&ok, 16);
            Q_ASSERT(ok);
            int diff = titleCase - codepoint;
            if (qAbs(diff) >= (1<<14)) {
                qWarning() << "titleCaseDiff exceeded (" << hex << codepoint << "->" << titleCase << "); map it for special case";
                // if the condition below doesn't hold anymore we need to modify our special title casing code in qchar.cpp
                Q_ASSERT(!QChar::requiresSurrogates(codepoint) && !QChar::requiresSurrogates(titleCase));
                data.p.titleCaseSpecial = true;
                data.p.titleCaseDiff = appendToSpecialCaseMap(QList<int>() << titleCase);
            } else {
                data.p.titleCaseDiff = diff;
                maxTitleCaseDiff = qMax(maxTitleCaseDiff, qAbs(diff));
            }
            if (QChar::requiresSurrogates(codepoint) || QChar::requiresSurrogates(titleCase)) {
                // if the conditions below doesn't hold anymore we need to modify our title casing code
                Q_ASSERT(QChar::highSurrogate(codepoint) == QChar::highSurrogate(titleCase));
                Q_ASSERT(QChar::lowSurrogate(codepoint) + diff == QChar::lowSurrogate(titleCase));
            }
        }

        if (!properties[UD_DigitValue].isEmpty())
            data.p.digitValue = properties[UD_DigitValue].toInt();

        // decompositition
        QByteArray decomposition = properties[UD_Decomposition];
        if (!decomposition.isEmpty()) {
            highestComposedCharacter = qMax(highestComposedCharacter, codepoint);
            QList<QByteArray> d = decomposition.split(' ');
            if (d[0].contains('<')) {
                data.decompositionType = decompositionMap.value(d[0], QChar::NoDecomposition);
                if (data.decompositionType == QChar::NoDecomposition)
                    qFatal("unhandled decomposition type: %s", d[0].constData());
                d.takeFirst();
            } else {
                data.decompositionType = QChar::Canonical;
            }
            for (int i = 0; i < d.size(); ++i) {
                data.decomposition.append(d[i].toInt(&ok, 16));
                Q_ASSERT(ok);
            }
            ++decompositionLength[data.decomposition.size()];
        }

        for (int i = codepoint; i <= lastCodepoint; ++i)
            unicodeData[i] = data;
    }
}

static int maxMirroredDiff = 0;

static void readBidiMirroring()
{
    qDebug("Reading BidiMirroring.txt");

    QFile f("data/BidiMirroring.txt");
    if (!f.exists())
        qFatal("Couldn't find BidiMirroring.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);

        if (line.isEmpty())
            continue;
        line = line.replace(" ", "");

        QList<QByteArray> pair = line.split(';');
        Q_ASSERT(pair.size() == 2);

        bool ok;
        int codepoint = pair[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int mirror = pair[1].toInt(&ok, 16);
        Q_ASSERT(ok);

        UnicodeData &d = UnicodeData::valueRef(codepoint);
        d.mirroredChar = mirror;
        d.p.mirrorDiff = d.mirroredChar - codepoint;
        maxMirroredDiff = qMax(maxMirroredDiff, qAbs(d.p.mirrorDiff));
    }
}

static void readArabicShaping()
{
    qDebug("Reading ArabicShaping.txt");

    QFile f("data/ArabicShaping.txt");
    if (!f.exists())
        qFatal("Couldn't find ArabicShaping.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line = line.trimmed();

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 4);

        bool ok;
        int codepoint = l[0].toInt(&ok, 16);
        Q_ASSERT(ok);

        UnicodeData &d = UnicodeData::valueRef(codepoint);
        JoiningType joining = joining_map.value(l[2].trimmed(), Joining_Unassigned);
        switch (joining) {
        case Joining_Unassigned:
            qFatal("%x: unassigned or unhandled joining type: %s", codepoint, l[2].constData());
            break;
        case Joining_Transparent:
            if (d.p.category != QChar::Mark_NonSpacing && d.p.category != QChar::Mark_Enclosing && d.p.category != QChar::Other_Format) {
                qFatal("%x: joining type '%s' was met; the current implementation needs to be revised!",
                       codepoint, l[2].constData());
            }
            // fall through

        default:
            d.p.joining = QChar::JoiningType(joining);
            break;
        }
    }

    // Code points that are not explicitly listed in ArabicShaping.txt are either of joining type T or U:
    // - Those that not explicitly listed that are of General Category Mn, Me, or Cf have joining type T.
    // - All others not explicitly listed have joining type U.
    for (int codepoint = 0; codepoint <= QChar::LastValidCodePoint; ++codepoint) {
        UnicodeData &d = UnicodeData::valueRef(codepoint);
        if (d.p.joining == QChar::Joining_None) {
            if (d.p.category == QChar::Mark_NonSpacing || d.p.category == QChar::Mark_Enclosing || d.p.category == QChar::Other_Format)
                d.p.joining = QChar::Joining_Transparent;
        }
    }
}

static void readDerivedAge()
{
    qDebug("Reading DerivedAge.txt");

    QFile f("data/DerivedAge.txt");
    if (!f.exists())
        qFatal("Couldn't find DerivedAge.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 2);

        QByteArray codes = l[0];
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        QChar::UnicodeVersion age = age_map.value(l[1].trimmed(), QChar::Unicode_Unassigned);
        //qDebug() << hex << from << ".." << to << ba << age;
        if (age == QChar::Unicode_Unassigned)
            qFatal("unassigned or unhandled age value: %s", l[1].constData());

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            UnicodeData &d = UnicodeData::valueRef(codepoint);
            d.p.age = age;
        }
    }
}

static void readDerivedNormalizationProps()
{
    qDebug("Reading DerivedNormalizationProps.txt");

    QFile f("data/DerivedNormalizationProps.txt");
    if (!f.exists())
        qFatal("Couldn't find DerivedNormalizationProps.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);

        if (line.trimmed().isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() >= 2);

        QByteArray propName = l[1].trimmed();
        if (propName != "Full_Composition_Exclusion" &&
            propName != "NFD_QC" && propName != "NFC_QC" &&
            propName != "NFKD_QC" && propName != "NFKC_QC") {
            // ###
            continue;
        }

        QByteArray codes = l[0].trimmed();
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            UnicodeData &d = UnicodeData::valueRef(codepoint);
            if (propName == "Full_Composition_Exclusion") {
                d.excludedComposition = true;
            } else {
                Q_STATIC_ASSERT(QString::NormalizationForm_D == 0);
                Q_STATIC_ASSERT(QString::NormalizationForm_C == 1);
                Q_STATIC_ASSERT(QString::NormalizationForm_KD == 2);
                Q_STATIC_ASSERT(QString::NormalizationForm_KC == 3);

                QString::NormalizationForm form;
                if (propName == "NFD_QC")
                    form = QString::NormalizationForm_D;
                else if (propName == "NFC_QC")
                    form = QString::NormalizationForm_C;
                else if (propName == "NFKD_QC")
                    form = QString::NormalizationForm_KD;
                else// if (propName == "NFKC_QC")
                    form = QString::NormalizationForm_KC;

                Q_ASSERT(l.size() == 3);
                l[2] = l[2].trimmed();

                enum { NFQC_YES = 0, NFQC_NO = 1, NFQC_MAYBE = 3 };
                uchar ynm = (l[2] == "N" ? NFQC_NO : l[2] == "M" ? NFQC_MAYBE : NFQC_YES);
                if (ynm == NFQC_MAYBE) {
                    // if this changes, we need to revise the normalizationQuickCheckHelper() implementation
                    Q_ASSERT(form == QString::NormalizationForm_C || form == QString::NormalizationForm_KC);
                }
                d.p.nfQuickCheck |= (ynm << (form << 1)); // 2 bits per NF
            }
        }
    }

    for (int codepoint = 0; codepoint <= QChar::LastValidCodePoint; ++codepoint) {
        UnicodeData &d = UnicodeData::valueRef(codepoint);
        if (!d.excludedComposition
            && d.decompositionType == QChar::Canonical
            && d.decomposition.size() > 1) {
            Q_ASSERT(d.decomposition.size() == 2);

            int part1 = d.decomposition.at(0);
            int part2 = d.decomposition.at(1);

            // all non-starters are listed in DerivedNormalizationProps.txt
            // and already excluded from composition
            Q_ASSERT(UnicodeData::valueRef(part1).p.combiningClass == 0);

            ++numLigatures;
            highestLigature = qMax(highestLigature, part1);
            Ligature l = { part1, part2, codepoint };
            ligatureHashes[part2].append(l);
        }
    }
}


struct NormalizationCorrection {
    uint codepoint;
    uint mapped;
    int version;
};

static QByteArray createNormalizationCorrections()
{
    qDebug("Reading NormalizationCorrections.txt");

    QFile f("data/NormalizationCorrections.txt");
    if (!f.exists())
        qFatal("Couldn't find NormalizationCorrections.txt");

    f.open(QFile::ReadOnly);

    QByteArray out;

    out += "struct NormalizationCorrection {\n"
           "    uint ucs4;\n"
           "    uint old_mapping;\n"
           "    int version;\n"
           "};\n\n"

           "static const NormalizationCorrection uc_normalization_corrections[] = {\n";

    int maxVersion = 0;
    int numCorrections = 0;
    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        Q_ASSERT(!line.contains(".."));

        QList<QByteArray> fields = line.split(';');
        Q_ASSERT(fields.size() == 4);

        NormalizationCorrection c = { 0, 0, 0 };
        bool ok;
        c.codepoint = fields.at(0).toInt(&ok, 16);
        Q_ASSERT(ok);
        c.mapped = fields.at(1).toInt(&ok, 16);
        Q_ASSERT(ok);
        if (fields.at(3) == "3.2.0")
            c.version = QChar::Unicode_3_2;
        else if (fields.at(3) == "4.0.0")
            c.version = QChar::Unicode_4_0;
        else
            qFatal("unknown unicode version in NormalizationCorrection.txt");

        out += "    { 0x" + QByteArray::number(c.codepoint, 16) + ", 0x" + QByteArray::number(c.mapped, 16)
             + ", " + QString::number(c.version) + " },\n";
        ++numCorrections;
        maxVersion = qMax(c.version, maxVersion);
    }

    out += "};\n\n"

           "enum { NumNormalizationCorrections = " + QByteArray::number(numCorrections) + " };\n"
           "enum { NormalizationCorrectionsVersionMax = " + QByteArray::number(maxVersion) + " };\n\n";

    return out;
}

static void readLineBreak()
{
    qDebug("Reading LineBreak.txt");

    QFile f("data/LineBreak.txt");
    if (!f.exists())
        qFatal("Couldn't find LineBreak.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 2);

        QByteArray codes = l[0];
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        LineBreakClass lb = line_break_map.value(l[1], LineBreak_Unassigned);
        if (lb == LineBreak_Unassigned)
            qFatal("unassigned line break class: %s", l[1].constData());

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            UnicodeData &d = UnicodeData::valueRef(codepoint);
            d.p.lineBreakClass = lb;
        }
    }
}

static void readSpecialCasing()
{
    qDebug("Reading SpecialCasing.txt");

    QFile f("data/SpecialCasing.txt");
    if (!f.exists())
        qFatal("Couldn't find SpecialCasing.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');

        QByteArray condition = l.size() < 5 ? QByteArray() : l[4].trimmed();
        if (!condition.isEmpty())
            // #####
            continue;

        bool ok;
        int codepoint = l[0].trimmed().toInt(&ok, 16);
        Q_ASSERT(ok);

        // if the condition below doesn't hold anymore we need to modify our
        // lower/upper/title casing code and case folding code
        Q_ASSERT(!QChar::requiresSurrogates(codepoint));

//         qDebug() << "codepoint" << hex << codepoint;
//         qDebug() << line;

        QList<QByteArray> lower = l[1].trimmed().split(' ');
        QList<int> lowerMap;
        for (int i = 0; i < lower.size(); ++i) {
            bool ok;
            lowerMap.append(lower.at(i).toInt(&ok, 16));
            Q_ASSERT(ok);
        }

        QList<QByteArray> title = l[2].trimmed().split(' ');
        QList<int> titleMap;
        for (int i = 0; i < title.size(); ++i) {
            bool ok;
            titleMap.append(title.at(i).toInt(&ok, 16));
            Q_ASSERT(ok);
        }

        QList<QByteArray> upper = l[3].trimmed().split(' ');
        QList<int> upperMap;
        for (int i = 0; i < upper.size(); ++i) {
            bool ok;
            upperMap.append(upper.at(i).toInt(&ok, 16));
            Q_ASSERT(ok);
        }


        UnicodeData &ud = UnicodeData::valueRef(codepoint);
        Q_ASSERT(lowerMap.size() > 1 || lowerMap.at(0) == codepoint + ud.p.lowerCaseDiff);
        Q_ASSERT(titleMap.size() > 1 || titleMap.at(0) == codepoint + ud.p.titleCaseDiff);
        Q_ASSERT(upperMap.size() > 1 || upperMap.at(0) == codepoint + ud.p.upperCaseDiff);

        if (lowerMap.size() > 1) {
            ud.p.lowerCaseSpecial = true;
            ud.p.lowerCaseDiff = appendToSpecialCaseMap(lowerMap);
        }
        if (titleMap.size() > 1) {
            ud.p.titleCaseSpecial = true;
            ud.p.titleCaseDiff = appendToSpecialCaseMap(titleMap);
        }
        if (upperMap.size() > 1) {
            ud.p.upperCaseSpecial = true;
            ud.p.upperCaseDiff = appendToSpecialCaseMap(upperMap);
        }
    }
}

static int maxCaseFoldDiff = 0;

static void readCaseFolding()
{
    qDebug("Reading CaseFolding.txt");

    QFile f("data/CaseFolding.txt");
    if (!f.exists())
        qFatal("Couldn't find CaseFolding.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');

        bool ok;
        int codepoint = l[0].trimmed().toInt(&ok, 16);
        Q_ASSERT(ok);


        l[1] = l[1].trimmed();
        if (l[1] == "F" || l[1] == "T")
            continue;

//         qDebug() << "codepoint" << hex << codepoint;
//         qDebug() << line;
        QList<QByteArray> fold = l[2].trimmed().split(' ');
        QList<int> foldMap;
        for (int i = 0; i < fold.size(); ++i) {
            bool ok;
            foldMap.append(fold.at(i).toInt(&ok, 16));
            Q_ASSERT(ok);
        }

        UnicodeData &ud = UnicodeData::valueRef(codepoint);
        if (foldMap.size() == 1) {
            int caseFolded = foldMap.at(0);
            int diff = caseFolded - codepoint;
            if (qAbs(diff) >= (1<<14)) {
                qWarning() << "caseFoldDiff exceeded (" << hex << codepoint << "->" << caseFolded << "); map it for special case";
                // if the condition below doesn't hold anymore we need to modify our special case folding code in qchar.cpp
                Q_ASSERT(!QChar::requiresSurrogates(codepoint) && !QChar::requiresSurrogates(caseFolded));
                ud.p.caseFoldSpecial = true;
                ud.p.caseFoldDiff = appendToSpecialCaseMap(foldMap);
            } else {
                ud.p.caseFoldDiff = diff;
                maxCaseFoldDiff = qMax(maxCaseFoldDiff, qAbs(diff));
            }
            if (QChar::requiresSurrogates(codepoint) || QChar::requiresSurrogates(caseFolded)) {
                // if the conditions below doesn't hold anymore we need to modify our case folding code
                Q_ASSERT(QChar::highSurrogate(codepoint) == QChar::highSurrogate(caseFolded));
                Q_ASSERT(QChar::lowSurrogate(codepoint) + diff == QChar::lowSurrogate(caseFolded));
            }
//            if (caseFolded != codepoint + ud.p.lowerCaseDiff)
//                qDebug() << hex << codepoint;
        } else {
            qFatal("we currently don't support full case foldings");
//             qDebug() << "special" << hex << foldMap;
            ud.p.caseFoldSpecial = true;
            ud.p.caseFoldDiff = appendToSpecialCaseMap(foldMap);
        }
    }
}

static void readGraphemeBreak()
{
    qDebug("Reading GraphemeBreakProperty.txt");

    QFile f("data/GraphemeBreakProperty.txt");
    if (!f.exists())
        qFatal("Couldn't find GraphemeBreakProperty.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 2);

        QByteArray codes = l[0];
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        GraphemeBreakClass brk = grapheme_break_map.value(l[1], GraphemeBreak_Unassigned);
        if (brk == GraphemeBreak_Unassigned)
            qFatal("unassigned grapheme break class: %s", l[1].constData());

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            UnicodeData &ud = UnicodeData::valueRef(codepoint);
            ud.p.graphemeBreakClass = brk;
        }
    }
}

static void readWordBreak()
{
    qDebug("Reading WordBreakProperty.txt");

    QFile f("data/WordBreakProperty.txt");
    if (!f.exists())
        qFatal("Couldn't find WordBreakProperty.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 2);

        QByteArray codes = l[0];
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        WordBreakClass brk = word_break_map.value(l[1], WordBreak_Unassigned);
        if (brk == WordBreak_Unassigned)
            qFatal("unassigned word break class: %s", l[1].constData());

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            // ### [
            // as of Unicode 5.1, some punctuation marks were mapped to MidLetter and MidNumLet
            // which caused "hi.there" to be treated like if it were just a single word;
            // until we have a tailoring mechanism, retain the old behavior by remapping those characters here.
            if (codepoint == 0x002E) // FULL STOP
                brk = WordBreak_MidNum;
            else if (codepoint == 0x003A) // COLON
                brk = WordBreak_Other;
            // ] ###
            UnicodeData &ud = UnicodeData::valueRef(codepoint);
            ud.p.wordBreakClass = brk;
        }
    }
}

static void readSentenceBreak()
{
    qDebug("Reading SentenceBreakProperty.txt");

    QFile f("data/SentenceBreakProperty.txt");
    if (!f.exists())
        qFatal("Couldn't find SentenceBreakProperty.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 2);

        QByteArray codes = l[0];
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        SentenceBreakClass brk = sentence_break_map.value(l[1], SentenceBreak_Unassigned);
        if (brk == SentenceBreak_Unassigned)
            qFatal("unassigned sentence break class: %s", l[1].constData());

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            UnicodeData &ud = UnicodeData::valueRef(codepoint);
            ud.p.sentenceBreakClass = brk;
        }
    }
}

#if 0
// this piece of code does full case folding and comparison. We currently
// don't use it, since this gives lots of issues with things as case insensitive
// search and replace.
static inline void foldCase(uint ch, ushort *out)
{
    const QUnicodeTables::Properties *p = qGetProp(ch);
    if (!p->caseFoldSpecial) {
        *(out++) = ch + p->caseFoldDiff;
    } else {
        const ushort *folded = specialCaseMap + p->caseFoldDiff;
        ushort length = *folded++;
        while (length--)
            *out++ = *folded++;
    }
    *out = 0;
}

static int ucstricmp(const ushort *a, const ushort *ae, const ushort *b, const ushort *be)
{
    if (a == b)
        return 0;
    if (a == 0)
        return 1;
    if (b == 0)
        return -1;

    while (a != ae && b != be) {
        const QUnicodeTables::Properties *pa = qGetProp(*a);
        const QUnicodeTables::Properties *pb = qGetProp(*b);
        if (pa->caseFoldSpecial | pb->caseFoldSpecial)
            goto special;
            int diff = (int)(*a + pa->caseFoldDiff) - (int)(*b + pb->caseFoldDiff);
        if ((diff))
            return diff;
        ++a;
        ++b;
        }
    }
    if (a == ae) {
        if (b == be)
            return 0;
        return -1;
    }
    return 1;
special:
    ushort abuf[SPECIAL_CASE_MAX_LEN + 1];
    ushort bbuf[SPECIAL_CASE_MAX_LEN + 1];
    abuf[0] = bbuf[0] = 0;
    ushort *ap = abuf;
    ushort *bp = bbuf;
    while (1) {
        if (!*ap) {
            if (a == ae) {
                if (!*bp && b == be)
                    return 0;
                return -1;
            }
            foldCase(*(a++), abuf);
            ap = abuf;
        }
        if (!*bp) {
            if (b == be)
                return 1;
            foldCase(*(b++), bbuf);
            bp = bbuf;
        }
        if (*ap != *bp)
            return (int)*ap - (int)*bp;
        ++ap;
        ++bp;
    }
}


static int ucstricmp(const ushort *a, const ushort *ae, const uchar *b)
{
    if (a == 0)
        return 1;
    if (b == 0)
        return -1;

    while (a != ae && *b) {
        const QUnicodeTables::Properties *pa = qGetProp(*a);
        const QUnicodeTables::Properties *pb = qGetProp((ushort)*b);
        if (pa->caseFoldSpecial | pb->caseFoldSpecial)
            goto special;
        int diff = (int)(*a + pa->caseFoldDiff) - (int)(*b + pb->caseFoldDiff);
        if ((diff))
            return diff;
        ++a;
        ++b;
    }
    if (a == ae) {
        if (!*b)
            return 0;
        return -1;
    }
    return 1;

special:
    ushort abuf[SPECIAL_CASE_MAX_LEN + 1];
    ushort bbuf[SPECIAL_CASE_MAX_LEN + 1];
    abuf[0] = bbuf[0] = 0;
    ushort *ap = abuf;
    ushort *bp = bbuf;
    while (1) {
        if (!*ap) {
            if (a == ae) {
                if (!*bp && !*b)
                    return 0;
                return -1;
            }
            foldCase(*(a++), abuf);
            ap = abuf;
        }
        if (!*bp) {
            if (!*b)
                return 1;
            foldCase(*(b++), bbuf);
            bp = bbuf;
        }
        if (*ap != *bp)
            return (int)*ap - (int)*bp;
        ++ap;
        ++bp;
    }
}
#endif

#if 0
static QList<QByteArray> blockNames;
struct BlockInfo
{
    int blockIndex;
    int firstCodePoint;
    int lastCodePoint;
};
static QList<BlockInfo> blockInfoList;

static void readBlocks()
{
    qDebug("Reading Blocks.txt");

    QFile f("data/Blocks.txt");
    if (!f.exists())
        qFatal("Couldn't find Blocks.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line = f.readLine();
        line.resize(line.size() - 1);

        int comment = line.indexOf("#");
        if (comment >= 0)
            line = line.left(comment);

        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        int semicolon = line.indexOf(';');
        Q_ASSERT(semicolon >= 0);
        QByteArray codePoints = line.left(semicolon);
        QByteArray blockName = line.mid(semicolon + 1);

        int blockIndex = blockNames.indexOf(blockName);
        if (blockIndex == -1) {
            blockIndex = blockNames.size();
            blockNames.append(blockName);
        }

        codePoints.replace("..", ".");
        QList<QByteArray> cl = codePoints.split('.');

        bool ok;
        int first = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int last = first;
        if (cl.size() == 2) {
            last = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        BlockInfo blockInfo = { blockIndex, first, last };
        blockInfoList.append(blockInfo);
    }
}
#endif

static void readScripts()
{
    qDebug("Reading Scripts.txt");

    QFile f("data/Scripts.txt");
    if (!f.exists())
        qFatal("Couldn't find Scripts.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line = f.readLine();
        line.resize(line.size() - 1);

        int comment = line.indexOf("#");
        if (comment >= 0)
            line = line.left(comment);

        line.replace(" ", "");
        line.replace("_", "");

        if (line.isEmpty())
            continue;

        int semicolon = line.indexOf(';');
        Q_ASSERT(semicolon >= 0);
        QByteArray codePoints = line.left(semicolon);
        QByteArray scriptName = line.mid(semicolon + 1);

        codePoints.replace("..", ".");
        QList<QByteArray> cl = codePoints.split('.');

        bool ok;
        int first = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int last = first;
        if (cl.size() == 2) {
            last = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        if (!scriptMap.contains(scriptName))
            qFatal("Unhandled script property value: %s", scriptName.constData());
        QChar::Script script = scriptMap.value(scriptName, QChar::Script_Unknown);

        for (int codepoint = first; codepoint <= last; ++codepoint) {
            UnicodeData &ud = UnicodeData::valueRef(codepoint);
            ud.p.script = script;
        }
    }
}

#if 0
static void dump(int from, int to)
{
    for (int i = from; i <= to; ++i) {
        UnicodeData &d = UnicodeData::valueRef(i);
        qDebug("0x%04x: cat=%d combining=%d dir=%d case=%x mirror=%x joining=%d age=%d",
               i, d.p.category, d.p.combiningClass, d.p.direction, d.otherCase, d.mirroredChar, d.p.joining, d.p.age);
        if (d.decompositionType != QChar::NoDecomposition) {
            qDebug("    decomposition: type=%d, length=%d, first=%x", d.decompositionType, d.decomposition.size(),
                   d.decomposition[0]);
        }
    }
    qDebug(" ");
}
#endif

static QList<PropertyFlags> uniqueProperties;

static void computeUniqueProperties()
{
    qDebug("computeUniqueProperties:");
    for (int codepoint = 0; codepoint <= QChar::LastValidCodePoint; ++codepoint) {
        UnicodeData &d = UnicodeData::valueRef(codepoint);
        int index = uniqueProperties.indexOf(d.p);
        if (index == -1) {
            index = uniqueProperties.size();
            uniqueProperties.append(d.p);
        }
        d.propertyIndex = index;
    }
    qDebug("    %d unique unicode properties found", uniqueProperties.size());
}

struct UniqueBlock {
    inline UniqueBlock() : index(-1) {}

    inline bool operator==(const UniqueBlock &other) const
    { return values == other.values; }

    int index;
    QVector<int> values;
};

static QByteArray createPropertyInfo()
{
    qDebug("createPropertyInfo:");

    // we reserve one bit more than in the assert below for the sign
    Q_ASSERT(maxMirroredDiff < (1<<12));
    Q_ASSERT(maxLowerCaseDiff < (1<<14));
    Q_ASSERT(maxUpperCaseDiff < (1<<14));
    Q_ASSERT(maxTitleCaseDiff < (1<<14));
    Q_ASSERT(maxCaseFoldDiff < (1<<14));

    const int BMP_BLOCKSIZE = 32;
    const int BMP_SHIFT = 5;
    const int BMP_END = 0x11000;
    const int SMP_END = 0x110000;
    const int SMP_BLOCKSIZE = 256;
    const int SMP_SHIFT = 8;

    QList<UniqueBlock> uniqueBlocks;
    QVector<int> blockMap;
    int used = 0;

    for (int block = 0; block < BMP_END/BMP_BLOCKSIZE; ++block) {
        UniqueBlock b;
        b.values.reserve(BMP_BLOCKSIZE);
        for (int i = 0; i < BMP_BLOCKSIZE; ++i) {
            int uc = block*BMP_BLOCKSIZE + i;
            UnicodeData &d = UnicodeData::valueRef(uc);
            b.values.append(d.propertyIndex);
        }
        int index = uniqueBlocks.indexOf(b);
        if (index == -1) {
            index = uniqueBlocks.size();
            b.index = used;
            used += BMP_BLOCKSIZE;
            uniqueBlocks.append(b);
        }
        blockMap.append(uniqueBlocks.at(index).index);
    }
    int bmp_blocks = uniqueBlocks.size();

    for (int block = BMP_END/SMP_BLOCKSIZE; block < SMP_END/SMP_BLOCKSIZE; ++block) {
        UniqueBlock b;
        b.values.reserve(SMP_BLOCKSIZE);
        for (int i = 0; i < SMP_BLOCKSIZE; ++i) {
            int uc = block*SMP_BLOCKSIZE + i;
            UnicodeData &d = UnicodeData::valueRef(uc);
            b.values.append(d.propertyIndex);
        }
        int index = uniqueBlocks.indexOf(b);
        if (index == -1) {
            index = uniqueBlocks.size();
            b.index = used;
            used += SMP_BLOCKSIZE;
            uniqueBlocks.append(b);
        }
        blockMap.append(uniqueBlocks.at(index).index);
    }
    int smp_blocks = uniqueBlocks.size() - bmp_blocks;

    int bmp_block_data = bmp_blocks*BMP_BLOCKSIZE*sizeof(unsigned short);
    int bmp_trie = BMP_END/BMP_BLOCKSIZE*sizeof(unsigned short);
    int bmp_mem = bmp_block_data + bmp_trie;
    qDebug("    %d unique blocks in BMP.", bmp_blocks);
    qDebug("        block data uses: %d bytes", bmp_block_data);
    qDebug("        trie data uses : %d bytes", bmp_trie);

    int smp_block_data = smp_blocks*SMP_BLOCKSIZE*sizeof(unsigned short);
    int smp_trie = (SMP_END-BMP_END)/SMP_BLOCKSIZE*sizeof(unsigned short);
    int smp_mem = smp_block_data + smp_trie;
    qDebug("    %d unique blocks in SMP.", smp_blocks);
    qDebug("        block data uses: %d bytes", smp_block_data);
    qDebug("        trie data uses : %d bytes", smp_trie);

    int prop_data = uniqueProperties.size() * SizeOfPropertiesStruct;
    qDebug("\n        properties data uses : %d bytes", prop_data);
    qDebug("    memory usage: %d bytes", bmp_mem + smp_mem + prop_data);

    Q_ASSERT(blockMap.last() + blockMap.size() < (1<<(sizeof(unsigned short)*8)));

    QByteArray out;

    out += "static const unsigned short uc_property_trie[] = {\n";
    // first write the map
    out += "    // 0 - 0x" + QByteArray::number(BMP_END, 16);
    for (int i = 0; i < BMP_END/BMP_BLOCKSIZE; ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            if (!((i*BMP_BLOCKSIZE) % 0x1000))
                out += "\n";
            out += "\n    ";
        }
        out += QByteArray::number(blockMap.at(i) + blockMap.size());
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n\n    // 0x" + QByteArray::number(BMP_END, 16) + " - 0x" + QByteArray::number(SMP_END, 16) + "\n";
    for (int i = BMP_END/BMP_BLOCKSIZE; i < blockMap.size(); ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            if (!(i % (0x10000/SMP_BLOCKSIZE)))
                out += "\n";
            out += "\n    ";
        }
        out += QByteArray::number(blockMap.at(i) + blockMap.size());
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n";
    // write the data
    for (int i = 0; i < uniqueBlocks.size(); ++i) {
        if (out.endsWith(' '))
            out.chop(1);
        out += "\n";
        const UniqueBlock &b = uniqueBlocks.at(i);
        for (int j = 0; j < b.values.size(); ++j) {
            if (!(j % 8)) {
                if (out.endsWith(' '))
                    out.chop(1);
                out += "\n    ";
            }
            out += QByteArray::number(b.values.at(j));
            out += ", ";
        }
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n};\n\n";

    out += "#define GET_PROP_INDEX(ucs4) \\\n"
           "       (ucs4 < 0x" + QByteArray::number(BMP_END, 16) + " \\\n"
           "        ? (uc_property_trie[uc_property_trie[ucs4>>" + QByteArray::number(BMP_SHIFT) +
           "] + (ucs4 & 0x" + QByteArray::number(BMP_BLOCKSIZE-1, 16)+ ")]) \\\n"
           "        : (uc_property_trie[uc_property_trie[((ucs4 - 0x" + QByteArray::number(BMP_END, 16) +
           ")>>" + QByteArray::number(SMP_SHIFT) + ") + 0x" + QByteArray::number(BMP_END/BMP_BLOCKSIZE, 16) + "]"
           " + (ucs4 & 0x" + QByteArray::number(SMP_BLOCKSIZE-1, 16) + ")]))\n\n"
           "#define GET_PROP_INDEX_UCS2(ucs2) \\\n"
           "       (uc_property_trie[uc_property_trie[ucs2>>" + QByteArray::number(BMP_SHIFT) +
           "] + (ucs2 & 0x" + QByteArray::number(BMP_BLOCKSIZE-1, 16)+ ")])\n\n";

    out += "static const Properties uc_properties[] = {";
    // keep in sync with the property declaration
    for (int i = 0; i < uniqueProperties.size(); ++i) {
        const PropertyFlags &p = uniqueProperties.at(i);
        out += "\n    { ";
//     "        ushort category            : 8; /* 5 used */\n"
        out += QByteArray::number( p.category );
        out += ", ";
//     "        ushort direction           : 8; /* 5 used */\n"
        out += QByteArray::number( p.direction );
        out += ", ";
//     "        ushort combiningClass      : 8;\n"
        out += QByteArray::number( p.combiningClass );
        out += ", ";
//     "        ushort joining             : 3;\n"
        out += QByteArray::number( p.joining );
        out += ", ";
//     "        signed short digitValue    : 5; /* 5 used */\n"
        out += QByteArray::number( p.digitValue );
        out += ", ";
//     "        signed short mirrorDiff    : 16;\n"
//     "        signed short lowerCaseDiff : 16;\n"
//     "        signed short upperCaseDiff : 16;\n"
//     "        signed short titleCaseDiff : 16;\n"
//     "        signed short caseFoldDiff  : 16;\n"
        out += QByteArray::number( p.mirrorDiff );
        out += ", ";
        out += QByteArray::number( p.lowerCaseDiff );
        out += ", ";
        out += QByteArray::number( p.upperCaseDiff );
        out += ", ";
        out += QByteArray::number( p.titleCaseDiff );
        out += ", ";
        out += QByteArray::number( p.caseFoldDiff );
        out += ", ";
//     "        ushort lowerCaseSpecial    : 1;\n"
//     "        ushort upperCaseSpecial    : 1;\n"
//     "        ushort titleCaseSpecial    : 1;\n"
//     "        ushort caseFoldSpecial     : 1;\n"
        out += QByteArray::number( p.lowerCaseSpecial );
        out += ", ";
        out += QByteArray::number( p.upperCaseSpecial );
        out += ", ";
        out += QByteArray::number( p.titleCaseSpecial );
        out += ", ";
        out += QByteArray::number( p.caseFoldSpecial );
        out += ", ";
//     "        ushort unicodeVersion      : 4;\n"
        out += QByteArray::number( p.age );
        out += ", ";
//     "    ushort nfQuickCheck        : 8;\n"
        out += QByteArray::number( p.nfQuickCheck );
        out += ", ";
//     "        ushort graphemeBreakClass  : 4; /* 4 used */\n"
//     "        ushort wordBreakClass      : 4; /* 4 used */\n"
//     "        ushort sentenceBreakClass  : 8; /* 4 used */\n"
//     "        ushort lineBreakClass      : 8; /* 6 used */\n"
        out += QByteArray::number( p.graphemeBreakClass );
        out += ", ";
        out += QByteArray::number( p.wordBreakClass );
        out += ", ";
        out += QByteArray::number( p.sentenceBreakClass );
        out += ", ";
        out += QByteArray::number( p.lineBreakClass );
        out += ", ";
//     "        ushort script              : 8; /* 7 used */\n"
        out += QByteArray::number( p.script );
        out += " },";
    }
    out.chop(1);
    out += "\n};\n\n";


    out += "static inline const Properties *qGetProp(uint ucs4)\n"
           "{\n"
           "    const int index = GET_PROP_INDEX(ucs4);\n"
           "    return uc_properties + index;\n"
           "}\n"
           "\n"
           "static inline const Properties *qGetProp(ushort ucs2)\n"
           "{\n"
           "    const int index = GET_PROP_INDEX_UCS2(ucs2);\n"
           "    return uc_properties + index;\n"
           "}\n"
           "\n"
           "Q_CORE_EXPORT const Properties * QT_FASTCALL properties(uint ucs4)\n"
           "{\n"
           "    return qGetProp(ucs4);\n"
           "}\n"
           "\n"
           "Q_CORE_EXPORT const Properties * QT_FASTCALL properties(ushort ucs2)\n"
           "{\n"
           "    return qGetProp(ucs2);\n"
           "}\n\n";

    out += "Q_CORE_EXPORT GraphemeBreakClass QT_FASTCALL graphemeBreakClass(uint ucs4)\n"
           "{\n"
           "    return (GraphemeBreakClass)qGetProp(ucs4)->graphemeBreakClass;\n"
           "}\n"
           "\n"
           "Q_CORE_EXPORT WordBreakClass QT_FASTCALL wordBreakClass(uint ucs4)\n"
           "{\n"
           "    return (WordBreakClass)qGetProp(ucs4)->wordBreakClass;\n"
           "}\n"
           "\n"
           "Q_CORE_EXPORT SentenceBreakClass QT_FASTCALL sentenceBreakClass(uint ucs4)\n"
           "{\n"
           "    return (SentenceBreakClass)qGetProp(ucs4)->sentenceBreakClass;\n"
           "}\n"
           "\n"
           "Q_CORE_EXPORT LineBreakClass QT_FASTCALL lineBreakClass(uint ucs4)\n"
           "{\n"
           "    return (LineBreakClass)qGetProp(ucs4)->lineBreakClass;\n"
           "}\n"
           "\n";

    return out;
}

static QByteArray createSpecialCaseMap()
{
    qDebug("createSpecialCaseMap:");

    QByteArray out;

    out += "static const ushort specialCaseMap[] = {\n"
           "    0x0, // placeholder";
    int i = 1;
    while (i < specialCaseMap.size()) {
        out += "\n   ";
        int n = specialCaseMap.at(i);
        for (int j = 0; j <= n; ++j) {
            out += QByteArray(" 0x") + QByteArray::number(specialCaseMap.at(i+j), 16);
            out += ",";
        }
        i += n + 1;
    }
    out.chop(1);
    out += "\n};\n\n";

    qDebug("    memory usage: %d bytes", specialCaseMap.size()*sizeof(unsigned short));

    return out;
}


static QByteArray createCompositionInfo()
{
    qDebug("createCompositionInfo: highestComposedCharacter=0x%x", highestComposedCharacter);

    const int BMP_BLOCKSIZE = 16;
    const int BMP_SHIFT = 4;
    const int BMP_END = 0x3400; // start of Han
    const int SMP_END = 0x30000;
    const int SMP_BLOCKSIZE = 256;
    const int SMP_SHIFT = 8;

    if (SMP_END <= highestComposedCharacter)
        qFatal("end of table smaller than highest composed character 0x%x", highestComposedCharacter);

    QVector<unsigned short> decompositions;
    int tableIndex = 0;

    QList<UniqueBlock> uniqueBlocks;
    QVector<int> blockMap;
    int used = 0;

    for (int block = 0; block < BMP_END/BMP_BLOCKSIZE; ++block) {
        UniqueBlock b;
        b.values.reserve(BMP_BLOCKSIZE);
        for (int i = 0; i < BMP_BLOCKSIZE; ++i) {
            int uc = block*BMP_BLOCKSIZE + i;
            UnicodeData &d = UnicodeData::valueRef(uc);
            if (!d.decomposition.isEmpty()) {
                int utf16Length = 0;
                decompositions.append(0);
                for (int j = 0; j < d.decomposition.size(); ++j) {
                    int code = d.decomposition.at(j);
                    if (QChar::requiresSurrogates(code)) {
                        // save as surrogate pair
                        decompositions.append(QChar::highSurrogate(code));
                        decompositions.append(QChar::lowSurrogate(code));
                        utf16Length += 2;
                    } else {
                        decompositions.append(code);
                        utf16Length++;
                    }
                }
                decompositions[tableIndex] = d.decompositionType + (utf16Length<<8);
                b.values.append(tableIndex);
                tableIndex += utf16Length + 1;
            } else {
                b.values.append(0xffff);
            }
        }
        int index = uniqueBlocks.indexOf(b);
        if (index == -1) {
            index = uniqueBlocks.size();
            b.index = used;
            used += BMP_BLOCKSIZE;
            uniqueBlocks.append(b);
        }
        blockMap.append(uniqueBlocks.at(index).index);
    }
    int bmp_blocks = uniqueBlocks.size();

    for (int block = BMP_END/SMP_BLOCKSIZE; block < SMP_END/SMP_BLOCKSIZE; ++block) {
        UniqueBlock b;
        b.values.reserve(SMP_BLOCKSIZE);
        for (int i = 0; i < SMP_BLOCKSIZE; ++i) {
            int uc = block*SMP_BLOCKSIZE + i;
            UnicodeData &d = UnicodeData::valueRef(uc);
            if (!d.decomposition.isEmpty()) {
                int utf16Length = 0;
                decompositions.append(0);
                for (int j = 0; j < d.decomposition.size(); ++j) {
                    int code = d.decomposition.at(j);
                    if (QChar::requiresSurrogates(code)) {
                        // save as surrogate pair
                        decompositions.append(QChar::highSurrogate(code));
                        decompositions.append(QChar::lowSurrogate(code));
                        utf16Length += 2;
                    } else {
                        decompositions.append(code);
                        utf16Length++;
                    }
                }
                decompositions[tableIndex] = d.decompositionType + (utf16Length<<8);
                b.values.append(tableIndex);
                tableIndex += utf16Length + 1;
            } else {
                b.values.append(0xffff);
            }
        }
        int index = uniqueBlocks.indexOf(b);
        if (index == -1) {
            index = uniqueBlocks.size();
            b.index = used;
            used += SMP_BLOCKSIZE;
            uniqueBlocks.append(b);
        }
        blockMap.append(uniqueBlocks.at(index).index);
    }
    int smp_blocks = uniqueBlocks.size() - bmp_blocks;

    // if the condition below doesn't hold anymore we need to modify our decomposition code
    Q_ASSERT(tableIndex < 0xffff);

    int bmp_block_data = bmp_blocks*BMP_BLOCKSIZE*sizeof(unsigned short);
    int bmp_trie = BMP_END/BMP_BLOCKSIZE*sizeof(unsigned short);
    int bmp_mem = bmp_block_data + bmp_trie;
    qDebug("    %d unique blocks in BMP.", bmp_blocks);
    qDebug("        block data uses: %d bytes", bmp_block_data);
    qDebug("        trie data uses : %d bytes", bmp_trie);

    int smp_block_data = smp_blocks*SMP_BLOCKSIZE*sizeof(unsigned short);
    int smp_trie = (SMP_END-BMP_END)/SMP_BLOCKSIZE*sizeof(unsigned short);
    int smp_mem = smp_block_data + smp_trie;
    qDebug("    %d unique blocks in SMP.", smp_blocks);
    qDebug("        block data uses: %d bytes", smp_block_data);
    qDebug("        trie data uses : %d bytes", smp_trie);

    int decomposition_data = decompositions.size() * 2;
    qDebug("\n        decomposition data uses : %d bytes", decomposition_data);
    qDebug("    memory usage: %d bytes", bmp_mem + smp_mem + decomposition_data);

    Q_ASSERT(blockMap.last() + blockMap.size() < (1<<(sizeof(unsigned short)*8)));

    QByteArray out;

    out += "static const unsigned short uc_decomposition_trie[] = {\n";
    // first write the map
    out += "    // 0 - 0x" + QByteArray::number(BMP_END, 16);
    for (int i = 0; i < BMP_END/BMP_BLOCKSIZE; ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            if (!((i*BMP_BLOCKSIZE) % 0x1000))
                out += "\n";
            out += "\n    ";
        }
        out += QByteArray::number(blockMap.at(i) + blockMap.size());
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n\n    // 0x" + QByteArray::number(BMP_END, 16) + " - 0x" + QByteArray::number(SMP_END, 16) + "\n";
    for (int i = BMP_END/BMP_BLOCKSIZE; i < blockMap.size(); ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            if (!(i % (0x10000/SMP_BLOCKSIZE)))
                out += "\n";
            out += "\n    ";
        }
        out += QByteArray::number(blockMap.at(i) + blockMap.size());
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n";
    // write the data
    for (int i = 0; i < uniqueBlocks.size(); ++i) {
        if (out.endsWith(' '))
            out.chop(1);
        out += "\n";
        const UniqueBlock &b = uniqueBlocks.at(i);
        for (int j = 0; j < b.values.size(); ++j) {
            if (!(j % 8)) {
                if (out.endsWith(' '))
                    out.chop(1);
                out += "\n    ";
            }
            out += "0x" + QByteArray::number(b.values.at(j), 16);
            out += ", ";
        }
    }
    if (out.endsWith(' '))
        out.chop(2);
    out += "\n};\n\n";

    out += "#define GET_DECOMPOSITION_INDEX(ucs4) \\\n"
           "       (ucs4 < 0x" + QByteArray::number(BMP_END, 16) + " \\\n"
           "        ? (uc_decomposition_trie[uc_decomposition_trie[ucs4>>" + QByteArray::number(BMP_SHIFT) +
           "] + (ucs4 & 0x" + QByteArray::number(BMP_BLOCKSIZE-1, 16)+ ")]) \\\n"
           "        : (ucs4 < 0x" + QByteArray::number(SMP_END, 16) + "\\\n"
           "           ? uc_decomposition_trie[uc_decomposition_trie[((ucs4 - 0x" + QByteArray::number(BMP_END, 16) +
           ")>>" + QByteArray::number(SMP_SHIFT) + ") + 0x" + QByteArray::number(BMP_END/BMP_BLOCKSIZE, 16) + "]"
           " + (ucs4 & 0x" + QByteArray::number(SMP_BLOCKSIZE-1, 16) + ")]\\\n"
           "           : 0xffff))\n\n";

    out += "static const unsigned short uc_decomposition_map[] = {";
    for (int i = 0; i < decompositions.size(); ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            out += "\n    ";
        }
        out += "0x" + QByteArray::number(decompositions.at(i), 16);
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(2);
    out += "\n};\n\n";

    return out;
}

static QByteArray createLigatureInfo()
{
    qDebug("createLigatureInfo: numLigatures=%d, highestLigature=0x%x", numLigatures, highestLigature);

    for (int i = 0; i < ligatureHashes.size(); ++i) {
        const QList<Ligature> &l = ligatureHashes.value(i);
        for (int j = 0; j < l.size(); ++j) {
            // if the condition below doesn't hold anymore we need to modify our ligatureHelper code
            Q_ASSERT(QChar::requiresSurrogates(l.at(j).u2) == QChar::requiresSurrogates(l.at(j).ligature) &&
                     QChar::requiresSurrogates(l.at(j).u2) == QChar::requiresSurrogates(l.at(j).u1));
        }
    }

    const int BMP_BLOCKSIZE = 32;
    const int BMP_SHIFT = 5;
    const int BMP_END = 0x3100;
    const int SMP_END = 0x12000;
    const int SMP_BLOCKSIZE = 256;
    const int SMP_SHIFT = 8;

    if (SMP_END <= highestLigature)
        qFatal("end of table smaller than highest ligature character 0x%x", highestLigature);

    QList<unsigned short> ligatures;
    int tableIndex = 0;

    QList<UniqueBlock> uniqueBlocks;
    QVector<int> blockMap;
    int used = 0;

    for (int block = 0; block < BMP_END/BMP_BLOCKSIZE; ++block) {
        UniqueBlock b;
        b.values.reserve(BMP_BLOCKSIZE);
        for (int i = 0; i < BMP_BLOCKSIZE; ++i) {
            int uc = block*BMP_BLOCKSIZE + i;
            QList<Ligature> l = ligatureHashes.value(uc);
            if (!l.isEmpty()) {
                Q_ASSERT(!QChar::requiresSurrogates(uc));
                qSort(l); // needed for bsearch in ligatureHelper code

                ligatures.append(l.size());
                for (int j = 0; j < l.size(); ++j) {
                    ligatures.append(l.at(j).u1);
                    ligatures.append(l.at(j).ligature);
                }
                b.values.append(tableIndex);
                tableIndex += 2*l.size() + 1;
            } else {
                b.values.append(0xffff);
            }
        }
        int index = uniqueBlocks.indexOf(b);
        if (index == -1) {
            index = uniqueBlocks.size();
            b.index = used;
            used += BMP_BLOCKSIZE;
            uniqueBlocks.append(b);
        }
        blockMap.append(uniqueBlocks.at(index).index);
    }
    int bmp_blocks = uniqueBlocks.size();

    for (int block = BMP_END/SMP_BLOCKSIZE; block < SMP_END/SMP_BLOCKSIZE; ++block) {
        UniqueBlock b;
        b.values.reserve(SMP_BLOCKSIZE);
        for (int i = 0; i < SMP_BLOCKSIZE; ++i) {
            int uc = block*SMP_BLOCKSIZE + i;
            QList<Ligature> l = ligatureHashes.value(uc);
            if (!l.isEmpty()) {
                Q_ASSERT(QChar::requiresSurrogates(uc));
                qSort(l); // needed for bsearch in ligatureHelper code

                ligatures.append(l.size());
                for (int j = 0; j < l.size(); ++j) {
                    ligatures.append(QChar::highSurrogate(l.at(j).u1));
                    ligatures.append(QChar::lowSurrogate(l.at(j).u1));
                    ligatures.append(QChar::highSurrogate(l.at(j).ligature));
                    ligatures.append(QChar::lowSurrogate(l.at(j).ligature));
                }
                b.values.append(tableIndex);
                tableIndex += 4*l.size() + 1;
            } else {
                b.values.append(0xffff);
            }
        }
        int index = uniqueBlocks.indexOf(b);
        if (index == -1) {
            index = uniqueBlocks.size();
            b.index = used;
            used += SMP_BLOCKSIZE;
            uniqueBlocks.append(b);
        }
        blockMap.append(uniqueBlocks.at(index).index);
    }
    int smp_blocks = uniqueBlocks.size() - bmp_blocks;

    // if the condition below doesn't hold anymore we need to modify our composition code
    Q_ASSERT(tableIndex < 0xffff);

    int bmp_block_data = bmp_blocks*BMP_BLOCKSIZE*sizeof(unsigned short);
    int bmp_trie = BMP_END/BMP_BLOCKSIZE*sizeof(unsigned short);
    int bmp_mem = bmp_block_data + bmp_trie;
    qDebug("    %d unique blocks in BMP.", bmp_blocks);
    qDebug("        block data uses: %d bytes", bmp_block_data);
    qDebug("        trie data uses : %d bytes", bmp_trie);

    int smp_block_data = smp_blocks*SMP_BLOCKSIZE*sizeof(unsigned short);
    int smp_trie = (SMP_END-BMP_END)/SMP_BLOCKSIZE*sizeof(unsigned short);
    int smp_mem = smp_block_data + smp_trie;
    qDebug("    %d unique blocks in SMP.", smp_blocks);
    qDebug("        block data uses: %d bytes", smp_block_data);
    qDebug("        trie data uses : %d bytes", smp_trie);

    int ligature_data = ligatures.size() * 2;
    qDebug("\n        ligature data uses : %d bytes", ligature_data);
    qDebug("    memory usage: %d bytes", bmp_mem + smp_mem + ligature_data);

    Q_ASSERT(blockMap.last() + blockMap.size() < (1<<(sizeof(unsigned short)*8)));

    QByteArray out;

    out += "static const unsigned short uc_ligature_trie[] = {\n";
    // first write the map
    out += "    // 0 - 0x" + QByteArray::number(BMP_END, 16);
    for (int i = 0; i < BMP_END/BMP_BLOCKSIZE; ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            if (!((i*BMP_BLOCKSIZE) % 0x1000))
                out += "\n";
            out += "\n    ";
        }
        out += QByteArray::number(blockMap.at(i) + blockMap.size());
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n\n    // 0x" + QByteArray::number(BMP_END, 16) + " - 0x" + QByteArray::number(SMP_END, 16) + "\n";
    for (int i = BMP_END/BMP_BLOCKSIZE; i < blockMap.size(); ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            if (!(i % (0x10000/SMP_BLOCKSIZE)))
                out += "\n";
            out += "\n    ";
        }
        out += QByteArray::number(blockMap.at(i) + blockMap.size());
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n";
    // write the data
    for (int i = 0; i < uniqueBlocks.size(); ++i) {
        if (out.endsWith(' '))
            out.chop(1);
        out += "\n";
        const UniqueBlock &b = uniqueBlocks.at(i);
        for (int j = 0; j < b.values.size(); ++j) {
            if (!(j % 8)) {
                if (out.endsWith(' '))
                    out.chop(1);
                out += "\n    ";
            }
            out += "0x" + QByteArray::number(b.values.at(j), 16);
            out += ", ";
        }
    }
    if (out.endsWith(' '))
        out.chop(2);
    out += "\n};\n\n";

    out += "#define GET_LIGATURE_INDEX(ucs4) \\\n"
           "       (ucs4 < 0x" + QByteArray::number(BMP_END, 16) + " \\\n"
           "        ? (uc_ligature_trie[uc_ligature_trie[ucs4>>" + QByteArray::number(BMP_SHIFT) +
           "] + (ucs4 & 0x" + QByteArray::number(BMP_BLOCKSIZE-1, 16)+ ")]) \\\n"
           "        : (ucs4 < 0x" + QByteArray::number(SMP_END, 16) + "\\\n"
           "           ? uc_ligature_trie[uc_ligature_trie[((ucs4 - 0x" + QByteArray::number(BMP_END, 16) +
           ")>>" + QByteArray::number(SMP_SHIFT) + ") + 0x" + QByteArray::number(BMP_END/BMP_BLOCKSIZE, 16) + "]"
           " + (ucs4 & 0x" + QByteArray::number(SMP_BLOCKSIZE-1, 16) + ")]\\\n"
           "           : 0xffff))\n\n";

    out += "static const unsigned short uc_ligature_map[] = {";
    for (int i = 0; i < ligatures.size(); ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            out += "\n    ";
        }
        out += "0x" + QByteArray::number(ligatures.at(i), 16);
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(2);
    out += "\n};\n\n";

    return out;
}

QByteArray createCasingInfo()
{
    QByteArray out;

    out += "struct CasingInfo {\n"
           "    uint codePoint : 16;\n"
           "    uint flags : 8;\n"
           "    uint offset : 8;\n"
           "};\n\n";

    return out;
}


int main(int, char **)
{
    initAgeMap();
    initCategoryMap();
    initDecompositionMap();
    initDirectionMap();
    initJoiningMap();
    initGraphemeBreak();
    initWordBreak();
    initSentenceBreak();
    initLineBreak();
    initScriptMap();

    readUnicodeData();
    readBidiMirroring();
    readArabicShaping();
    readDerivedAge();
    readDerivedNormalizationProps();
    readSpecialCasing();
    readCaseFolding();
    // readBlocks();
    readScripts();
    readGraphemeBreak();
    readWordBreak();
    readSentenceBreak();
    readLineBreak();

    computeUniqueProperties();
    QByteArray properties = createPropertyInfo();
    QByteArray specialCases = createSpecialCaseMap();
    QByteArray compositions = createCompositionInfo();
    QByteArray ligatures = createLigatureInfo();
    QByteArray normalizationCorrections = createNormalizationCorrections();

    QByteArray header =
        "/****************************************************************************\n"
        "**\n"
        "** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).\n"
        "** Contact: http://www.qt-project.org/legal\n"
        "**\n"
        "** This file is part of the QtCore module of the Qt Toolkit.\n"
        "**\n"
        "** $QT_BEGIN_LICENSE:LGPL$\n"
        "** Commercial License Usage\n"
        "** Licensees holding valid commercial Qt licenses may use this file in\n"
        "** accordance with the commercial license agreement provided with the\n"
        "** Software or, alternatively, in accordance with the terms contained in\n"
        "** a written agreement between you and Digia.  For licensing terms and\n"
        "** conditions see http://qt.digia.com/licensing.  For further information\n"
        "** use the contact form at http://qt.digia.com/contact-us.\n"
        "**\n"
        "** GNU Lesser General Public License Usage\n"
        "** Alternatively, this file may be used under the terms of the GNU Lesser\n"
        "** General Public License version 2.1 as published by the Free Software\n"
        "** Foundation and appearing in the file LICENSE.LGPL included in the\n"
        "** packaging of this file.  Please review the following information to\n"
        "** ensure the GNU Lesser General Public License version 2.1 requirements\n"
        "** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.\n"
        "**\n"
        "** In addition, as a special exception, Digia gives you certain additional\n"
        "** rights.  These rights are described in the Digia Qt LGPL Exception\n"
        "** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.\n"
        "**\n"
        "** GNU General Public License Usage\n"
        "** Alternatively, this file may be used under the terms of the GNU\n"
        "** General Public License version 3.0 as published by the Free Software\n"
        "** Foundation and appearing in the file LICENSE.GPL included in the\n"
        "** packaging of this file.  Please review the following information to\n"
        "** ensure the GNU General Public License version 3.0 requirements will be\n"
        "** met: http://www.gnu.org/copyleft/gpl.html.\n"
        "**\n"
        "**\n"
        "** $QT_END_LICENSE$\n"
        "**\n"
        "****************************************************************************/\n\n";

    QByteArray note =
        "/* This file is autogenerated from the Unicode "DATA_VERSION_S" database. Do not edit */\n\n";

    QByteArray warning =
        "//\n"
        "//  W A R N I N G\n"
        "//  -------------\n"
        "//\n"
        "// This file is not part of the Qt API.  It exists for the convenience\n"
        "// of internal files.  This header file may change from version to version\n"
        "// without notice, or even be removed.\n"
        "//\n"
        "// We mean it.\n"
        "//\n\n";

    QFile f("../../src/corelib/tools/qunicodetables.cpp");
    f.open(QFile::WriteOnly|QFile::Truncate);
    f.write(header);
    f.write(note);
    f.write("#include \"qunicodetables_p.h\"\n\n");
    f.write("QT_BEGIN_NAMESPACE\n\n");
    f.write("namespace QUnicodeTables {\n\n");
    f.write(properties);
    f.write("\n");
    f.write(specialCases);
    f.write("\n");
    f.write(compositions);
    f.write(ligatures);
    f.write("\n");
    f.write(normalizationCorrections);
    f.write("} // namespace QUnicodeTables\n\n");
    f.write("using namespace QUnicodeTables;\n\n");
    f.write("QT_END_NAMESPACE\n");
    f.close();

    f.setFileName("../../src/corelib/tools/qunicodetables_p.h");
    f.open(QFile::WriteOnly | QFile::Truncate);
    f.write(header);
    f.write(note);
    f.write(warning);
    f.write("#ifndef QUNICODETABLES_P_H\n"
            "#define QUNICODETABLES_P_H\n\n"
            "#include <QtCore/qchar.h>\n\n"
            "QT_BEGIN_NAMESPACE\n\n");
    f.write("#define UNICODE_DATA_VERSION "DATA_VERSION_STR"\n\n");
    f.write("namespace QUnicodeTables {\n\n");
    f.write(property_string);
    f.write(grapheme_break_class_string);
    f.write(word_break_class_string);
    f.write(sentence_break_class_string);
    f.write(line_break_class_string);
    f.write(methods);
    f.write("} // namespace QUnicodeTables\n\n"
            "QT_END_NAMESPACE\n\n"
            "#endif // QUNICODETABLES_P_H\n");
    f.close();

    qDebug() << "maxMirroredDiff  = " << hex << maxMirroredDiff;
    qDebug() << "maxLowerCaseDiff = " << hex << maxLowerCaseDiff;
    qDebug() << "maxUpperCaseDiff = " << hex << maxUpperCaseDiff;
    qDebug() << "maxTitleCaseDiff = " << hex << maxTitleCaseDiff;
    qDebug() << "maxCaseFoldDiff  = " << hex << maxCaseFoldDiff;
#if 0
//     dump(0, 0x7f);
//     dump(0x620, 0x640);
//     dump(0x10000, 0x10020);
//     dump(0x10800, 0x10820);

    qDebug("decompositionLength used:");
    int totalcompositions = 0;
    int sum = 0;
    for (int i = 1; i < 20; ++i) {
        qDebug("    length %d used %d times", i, decompositionLength.value(i, 0));
        totalcompositions += i*decompositionLength.value(i, 0);
        sum += decompositionLength.value(i, 0);
    }
    qDebug("    len decomposition map %d, average length %f, num composed chars %d",
           totalcompositions, (float)totalcompositions/(float)sum, sum);
    qDebug("highest composed character %x", highestComposedCharacter);
    qDebug("num ligatures = %d highest=%x, maxLength=%d", numLigatures, highestLigature, longestLigature);

    qBubbleSort(ligatures);
    for (int i = 0; i < ligatures.size(); ++i)
        qDebug("%s", ligatures.at(i).data());

//     qDebug("combiningClass usage:");
//     int numClasses = 0;
//     for (int i = 0; i < 255; ++i) {
//         int num = combiningClassUsage.value(i, 0);
//         if (num) {
//             ++numClasses;
//             qDebug("    combiningClass %d used %d times", i, num);
//         }
//     }
//     qDebug("total of %d combining classes used", numClasses);

#endif
}
