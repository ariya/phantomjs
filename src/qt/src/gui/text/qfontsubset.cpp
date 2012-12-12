/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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
#include <qdebug.h>
#include "qfontsubset_p.h"
#include <qendian.h>
#include <qpainterpath.h>
#include "private/qpdf_p.h"
#include "private/qfunctions_p.h"

#ifdef Q_WS_X11
#include "private/qfontengine_x11_p.h"
#endif

#ifndef QT_NO_FREETYPE
#if defined(Q_WS_X11) || defined(Q_WS_QWS)
#    include "private/qfontengine_ft_p.h"
#endif
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

#ifndef QT_NO_PRINTER

QT_BEGIN_NAMESPACE

static const char * const agl =
".notdef\0space\0exclam\0quotedbl\0numbersign\0dollar\0percent\0ampersand\0"
"quotesingle\0parenleft\0parenright\0asterisk\0plus\0comma\0hyphen\0period\0"
"slash\0zero\0one\0two\0three\0four\0five\0six\0seven\0eight\0nine\0colon\0"
"semicolon\0less\0equal\0greater\0question\0at\0A\0B\0C\0D\0E\0F\0G\0H\0I\0J\0"
"K\0L\0M\0N\0O\0P\0Q\0R\0S\0T\0U\0V\0W\0X\0Y\0Z\0bracketleft\0backslash\0"
"bracketright\0asciicircum\0underscore\0grave\0a\0b\0c\0d\0e\0f\0g\0h\0i\0j\0"
"k\0l\0m\0n\0o\0p\0q\0r\0s\0t\0u\0v\0w\0x\0y\0z\0braceleft\0bar\0braceright\0"
"asciitilde\0space\0exclamdown\0cent\0sterling\0currency\0yen\0brokenbar\0"
"section\0dieresis\0copyright\0ordfeminine\0guillemotleft\0logicalnot\0"
"hyphen\0registered\0macron\0degree\0plusminus\0twosuperior\0threesuperior\0"
"acute\0mu\0paragraph\0periodcentered\0cedilla\0onesuperior\0ordmasculine\0"
"guillemotright\0onequarter\0onehalf\0threequarters\0questiondown\0Agrave\0"
"Aacute\0Acircumflex\0Atilde\0Adieresis\0Aring\0AE\0Ccedilla\0Egrave\0Eacute\0"
"Ecircumflex\0Edieresis\0Igrave\0Iacute\0Icircumflex\0Idieresis\0Eth\0Ntilde\0"
"Ograve\0Oacute\0Ocircumflex\0Otilde\0Odieresis\0multiply\0Oslash\0Ugrave\0"
"Uacute\0Ucircumflex\0Udieresis\0Yacute\0Thorn\0germandbls\0agrave\0aacute\0"
"acircumflex\0atilde\0adieresis\0aring\0ae\0ccedilla\0egrave\0eacute\0"
"ecircumflex\0edieresis\0igrave\0iacute\0icircumflex\0idieresis\0eth\0ntilde\0"
"ograve\0oacute\0ocircumflex\0otilde\0odieresis\0divide\0oslash\0ugrave\0"
"uacute\0ucircumflex\0udieresis\0yacute\0thorn\0ydieresis\0Amacron\0amacron\0"
"Abreve\0abreve\0Aogonek\0aogonek\0Cacute\0cacute\0Ccircumflex\0ccircumflex\0"
"Cdotaccent\0cdotaccent\0Ccaron\0ccaron\0Dcaron\0dcaron\0Dcroat\0dcroat\0"
"Emacron\0emacron\0Ebreve\0ebreve\0Edotaccent\0edotaccent\0Eogonek\0eogonek\0"
"Ecaron\0ecaron\0Gcircumflex\0gcircumflex\0Gbreve\0gbreve\0Gdotaccent\0"
"gdotaccent\0Gcommaaccent\0gcommaaccent\0Hcircumflex\0hcircumflex\0Hbar\0"
"hbar\0Itilde\0itilde\0Imacron\0imacron\0Ibreve\0ibreve\0Iogonek\0iogonek\0"
"Idotaccent\0dotlessi\0IJ\0ij\0Jcircumflex\0jcircumflex\0Kcommaaccent\0"
"kcommaaccent\0kgreenlandic\0Lacute\0lacute\0Lcommaaccent\0lcommaaccent\0"
"Lcaron\0lcaron\0Ldot\0ldot\0Lslash\0lslash\0Nacute\0nacute\0Ncommaaccent\0"
"ncommaaccent\0Ncaron\0ncaron\0napostrophe\0Eng\0eng\0Omacron\0omacron\0"
"Obreve\0obreve\0Ohungarumlaut\0ohungarumlaut\0OE\0oe\0Racute\0racute\0"
"Rcommaaccent\0rcommaaccent\0Rcaron\0rcaron\0Sacute\0sacute\0Scircumflex\0"
"scircumflex\0Scedilla\0scedilla\0Scaron\0scaron\0Tcaron\0tcaron\0Tbar\0tbar\0"
"Utilde\0utilde\0Umacron\0umacron\0Ubreve\0ubreve\0Uring\0uring\0"
"Uhungarumlaut\0uhungarumlaut\0Uogonek\0uogonek\0Wcircumflex\0wcircumflex\0"
"Ycircumflex\0ycircumflex\0Ydieresis\0Zacute\0zacute\0Zdotaccent\0zdotaccent\0"
"Zcaron\0zcaron\0longs\0florin\0Ohorn\0ohorn\0Uhorn\0uhorn\0Gcaron\0gcaron\0"
"Aringacute\0aringacute\0AEacute\0aeacute\0Oslashacute\0oslashacute\0"
"Scommaaccent\0scommaaccent\0Tcommaaccent\0tcommaaccent\0afii57929\0"
"afii64937\0circumflex\0caron\0breve\0dotaccent\0ring\0ogonek\0tilde\0"
"hungarumlaut\0gravecomb\0acutecomb\0tildecomb\0hookabovecomb\0dotbelowcomb\0"
"tonos\0dieresistonos\0Alphatonos\0anoteleia\0Epsilontonos\0Etatonos\0"
"Iotatonos\0Omicrontonos\0Upsilontonos\0Omegatonos\0iotadieresistonos\0Alpha\0"
"Beta\0Gamma\0Delta\0Epsilon\0Zeta\0Eta\0Theta\0Iota\0Kappa\0Lambda\0Mu\0Nu\0"
"Xi\0Omicron\0Pi\0Rho\0Sigma\0Tau\0Upsilon\0Phi\0Chi\0Psi\0Omega\0"
"Iotadieresis\0Upsilondieresis\0alphatonos\0epsilontonos\0etatonos\0"
"iotatonos\0upsilondieresistonos\0alpha\0beta\0gamma\0delta\0epsilon\0zeta\0"
"eta\0theta\0iota\0kappa\0lambda\0mu\0nu\0xi\0omicron\0pi\0rho\0sigma1\0"
"sigma\0tau\0upsilon\0phi\0chi\0psi\0omega\0iotadieresis\0upsilondieresis\0"
;

static const struct { quint16 u; quint16 index; } unicode_to_aglindex[] = {
    {0x0000, 0}, {0x0020, 8}, {0x0021, 14}, {0x0022, 21},
    {0x0023, 30}, {0x0024, 41}, {0x0025, 48}, {0x0026, 56},
    {0x0027, 66}, {0x0028, 78}, {0x0029, 88}, {0x002A, 99},
    {0x002B, 108}, {0x002C, 113}, {0x002D, 119}, {0x002E, 126},
    {0x002F, 133}, {0x0030, 139}, {0x0031, 144}, {0x0032, 148},
    {0x0033, 152}, {0x0034, 158}, {0x0035, 163}, {0x0036, 168},
    {0x0037, 172}, {0x0038, 178}, {0x0039, 184}, {0x003A, 189},
    {0x003B, 195}, {0x003C, 205}, {0x003D, 210}, {0x003E, 216},
    {0x003F, 224}, {0x0040, 233}, {0x0041, 236}, {0x0042, 238},
    {0x0043, 240}, {0x0044, 242}, {0x0045, 244}, {0x0046, 246},
    {0x0047, 248}, {0x0048, 250}, {0x0049, 252}, {0x004A, 254},
    {0x004B, 256}, {0x004C, 258}, {0x004D, 260}, {0x004E, 262},
    {0x004F, 264}, {0x0050, 266}, {0x0051, 268}, {0x0052, 270},
    {0x0053, 272}, {0x0054, 274}, {0x0055, 276}, {0x0056, 278},
    {0x0057, 280}, {0x0058, 282}, {0x0059, 284}, {0x005A, 286},
    {0x005B, 288}, {0x005C, 300}, {0x005D, 310}, {0x005E, 323},
    {0x005F, 335}, {0x0060, 346}, {0x0061, 352}, {0x0062, 354},
    {0x0063, 356}, {0x0064, 358}, {0x0065, 360}, {0x0066, 362},
    {0x0067, 364}, {0x0068, 366}, {0x0069, 368}, {0x006A, 370},
    {0x006B, 372}, {0x006C, 374}, {0x006D, 376}, {0x006E, 378},
    {0x006F, 380}, {0x0070, 382}, {0x0071, 384}, {0x0072, 386},
    {0x0073, 388}, {0x0074, 390}, {0x0075, 392}, {0x0076, 394},
    {0x0077, 396}, {0x0078, 398}, {0x0079, 400}, {0x007A, 402},
    {0x007B, 404}, {0x007C, 414}, {0x007D, 418}, {0x007E, 429},
    {0x00A0, 440}, {0x00A1, 446}, {0x00A2, 457}, {0x00A3, 462},
    {0x00A4, 471}, {0x00A5, 480}, {0x00A6, 484}, {0x00A7, 494},
    {0x00A8, 502}, {0x00A9, 511}, {0x00AA, 521}, {0x00AB, 533},
    {0x00AC, 547}, {0x00AD, 558}, {0x00AE, 565}, {0x00AF, 576},
    {0x00B0, 583}, {0x00B1, 590}, {0x00B2, 600}, {0x00B3, 612},
    {0x00B4, 626}, {0x00B5, 632}, {0x00B6, 635}, {0x00B7, 645},
    {0x00B8, 660}, {0x00B9, 668}, {0x00BA, 680}, {0x00BB, 693},
    {0x00BC, 708}, {0x00BD, 719}, {0x00BE, 727}, {0x00BF, 741},
    {0x00C0, 754}, {0x00C1, 761}, {0x00C2, 768}, {0x00C3, 780},
    {0x00C4, 787}, {0x00C5, 797}, {0x00C6, 803}, {0x00C7, 806},
    {0x00C8, 815}, {0x00C9, 822}, {0x00CA, 829}, {0x00CB, 841},
    {0x00CC, 851}, {0x00CD, 858}, {0x00CE, 865}, {0x00CF, 877},
    {0x00D0, 887}, {0x00D1, 891}, {0x00D2, 898}, {0x00D3, 905},
    {0x00D4, 912}, {0x00D5, 924}, {0x00D6, 931}, {0x00D7, 941},
    {0x00D8, 950}, {0x00D9, 957}, {0x00DA, 964}, {0x00DB, 971},
    {0x00DC, 983}, {0x00DD, 993}, {0x00DE, 1000}, {0x00DF, 1006},
    {0x00E0, 1017}, {0x00E1, 1024}, {0x00E2, 1031}, {0x00E3, 1043},
    {0x00E4, 1050}, {0x00E5, 1060}, {0x00E6, 1066}, {0x00E7, 1069},
    {0x00E8, 1078}, {0x00E9, 1085}, {0x00EA, 1092}, {0x00EB, 1104},
    {0x00EC, 1114}, {0x00ED, 1121}, {0x00EE, 1128}, {0x00EF, 1140},
    {0x00F0, 1150}, {0x00F1, 1154}, {0x00F2, 1161}, {0x00F3, 1168},
    {0x00F4, 1175}, {0x00F5, 1187}, {0x00F6, 1194}, {0x00F7, 1204},
    {0x00F8, 1211}, {0x00F9, 1218}, {0x00FA, 1225}, {0x00FB, 1232},
    {0x00FC, 1244}, {0x00FD, 1254}, {0x00FE, 1261}, {0x00FF, 1267},
    {0x0100, 1277}, {0x0101, 1285}, {0x0102, 1293}, {0x0103, 1300},
    {0x0104, 1307}, {0x0105, 1315}, {0x0106, 1323}, {0x0107, 1330},
    {0x0108, 1337}, {0x0109, 1349}, {0x010A, 1361}, {0x010B, 1372},
    {0x010C, 1383}, {0x010D, 1390}, {0x010E, 1397}, {0x010F, 1404},
    {0x0110, 1411}, {0x0111, 1418}, {0x0112, 1425}, {0x0113, 1433},
    {0x0114, 1441}, {0x0115, 1448}, {0x0116, 1455}, {0x0117, 1466},
    {0x0118, 1477}, {0x0119, 1485}, {0x011A, 1493}, {0x011B, 1500},
    {0x011C, 1507}, {0x011D, 1519}, {0x011E, 1531}, {0x011F, 1538},
    {0x0120, 1545}, {0x0121, 1556}, {0x0122, 1567}, {0x0123, 1580},
    {0x0124, 1593}, {0x0125, 1605}, {0x0126, 1617}, {0x0127, 1622},
    {0x0128, 1627}, {0x0129, 1634}, {0x012A, 1641}, {0x012B, 1649},
    {0x012C, 1657}, {0x012D, 1664}, {0x012E, 1671}, {0x012F, 1679},
    {0x0130, 1687}, {0x0131, 1698}, {0x0132, 1707}, {0x0133, 1710},
    {0x0134, 1713}, {0x0135, 1725}, {0x0136, 1737}, {0x0137, 1750},
    {0x0138, 1763}, {0x0139, 1776}, {0x013A, 1783}, {0x013B, 1790},
    {0x013C, 1803}, {0x013D, 1816}, {0x013E, 1823}, {0x013F, 1830},
    {0x0140, 1835}, {0x0141, 1840}, {0x0142, 1847}, {0x0143, 1854},
    {0x0144, 1861}, {0x0145, 1868}, {0x0146, 1881}, {0x0147, 1894},
    {0x0148, 1901}, {0x0149, 1908}, {0x014A, 1920}, {0x014B, 1924},
    {0x014C, 1928}, {0x014D, 1936}, {0x014E, 1944}, {0x014F, 1951},
    {0x0150, 1958}, {0x0151, 1972}, {0x0152, 1986}, {0x0153, 1989},
    {0x0154, 1992}, {0x0155, 1999}, {0x0156, 2006}, {0x0157, 2019},
    {0x0158, 2032}, {0x0159, 2039}, {0x015A, 2046}, {0x015B, 2053},
    {0x015C, 2060}, {0x015D, 2072}, {0x015E, 2084}, {0x015F, 2093},
    {0x0160, 2102}, {0x0161, 2109}, {0x0164, 2116}, {0x0165, 2123},
    {0x0166, 2130}, {0x0167, 2135}, {0x0168, 2140}, {0x0169, 2147},
    {0x016A, 2154}, {0x016B, 2162}, {0x016C, 2170}, {0x016D, 2177},
    {0x016E, 2184}, {0x016F, 2190}, {0x0170, 2196}, {0x0171, 2210},
    {0x0172, 2224}, {0x0173, 2232}, {0x0174, 2240}, {0x0175, 2252},
    {0x0176, 2264}, {0x0177, 2276}, {0x0178, 2288}, {0x0179, 2298},
    {0x017A, 2305}, {0x017B, 2312}, {0x017C, 2323}, {0x017D, 2334},
    {0x017E, 2341}, {0x017F, 2348}, {0x0192, 2354}, {0x01A0, 2361},
    {0x01A1, 2367}, {0x01AF, 2373}, {0x01B0, 2379}, {0x01E6, 2385},
    {0x01E7, 2392}, {0x01FA, 2399}, {0x01FB, 2410}, {0x01FC, 2421},
    {0x01FD, 2429}, {0x01FE, 2437}, {0x01FF, 2449}, {0x0218, 2461},
    {0x0219, 2474}, {0x021A, 2487}, {0x021B, 2500}, {0x02BC, 2513},
    {0x02BD, 2523}, {0x02C6, 2533}, {0x02C7, 2544}, {0x02D8, 2550},
    {0x02D9, 2556}, {0x02DA, 2566}, {0x02DB, 2571}, {0x02DC, 2578},
    {0x02DD, 2584}, {0x0300, 2597}, {0x0301, 2607}, {0x0303, 2617},
    {0x0309, 2627}, {0x0323, 2641}, {0x0384, 2654}, {0x0385, 2660},
    {0x0386, 2674}, {0x0387, 2685}, {0x0388, 2695}, {0x0389, 2708},
    {0x038A, 2717}, {0x038C, 2727}, {0x038E, 2740}, {0x038F, 2753},
    {0x0390, 2764}, {0x0391, 2782}, {0x0392, 2788}, {0x0393, 2793},
    {0x0394, 2799}, {0x0395, 2805}, {0x0396, 2813}, {0x0397, 2818},
    {0x0398, 2822}, {0x0399, 2828}, {0x039A, 2833}, {0x039B, 2839},
    {0x039C, 2846}, {0x039D, 2849}, {0x039E, 2852}, {0x039F, 2855},
    {0x03A0, 2863}, {0x03A1, 2866}, {0x03A3, 2870}, {0x03A4, 2876},
    {0x03A5, 2880}, {0x03A6, 2888}, {0x03A7, 2892}, {0x03A8, 2896},
    {0x03A9, 2900}, {0x03AA, 2906}, {0x03AB, 2919}, {0x03AC, 2935},
    {0x03AD, 2946}, {0x03AE, 2959}, {0x03AF, 2968}, {0x03B0, 2978},
    {0x03B1, 2999}, {0x03B2, 3005}, {0x03B3, 3010}, {0x03B4, 3016},
    {0x03B5, 3022}, {0x03B6, 3030}, {0x03B7, 3035}, {0x03B8, 3039},
    {0x03B9, 3045}, {0x03BA, 3050}, {0x03BB, 3056}, {0x03BC, 3063},
    {0x03BD, 3066}, {0x03BE, 3069}, {0x03BF, 3072}, {0x03C0, 3080},
    {0x03C1, 3083}, {0x03C2, 3087}, {0x03C3, 3094}, {0x03C4, 3100},
    {0x03C5, 3104}, {0x03C6, 3112}, {0x03C7, 3116}, {0x03C8, 3120},
    {0x03C9, 3124}, {0x03CA, 3130}, {0x03CB, 3143}, {0x03CC, 3159},
    {0x03CD, 3172}, {0x03CE, 3185}, {0x03D1, 3196}, {0x03D2, 3203},
    {0x03D5, 3212}, {0x03D6, 3217}, {0xFFFF, 3224}
};

// This map is used for symbol fonts to get the correct glyph names for the latin range
static const unsigned short symbol_map[0x100] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
    0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
    0x0020, 0x0021, 0x2200, 0x0023, 0x2203, 0x0025, 0x0026, 0x220b,
    0x0028, 0x0029, 0x2217, 0x002b, 0x002c, 0x2212, 0x002e, 0x002f,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,

    0x2245, 0x0391, 0x0392, 0x03a7, 0x0394, 0x0395, 0x03a6, 0x0393,
    0x0397, 0x0399, 0x03d1, 0x039a, 0x039b, 0x039c, 0x039d, 0x039f,
    0x03a0, 0x0398, 0x03a1, 0x03a3, 0x03a4, 0x03a5, 0x03c2, 0x03a9,
    0x039e, 0x03a8, 0x0396, 0x005b, 0x2234, 0x005d, 0x22a5, 0x005f,
    0xf8e5, 0x03b1, 0x03b2, 0x03c7, 0x03b4, 0x03b5, 0x03c6, 0x03b3,
    0x03b7, 0x03b9, 0x03d5, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03bf,
    0x03c0, 0x03b8, 0x03c1, 0x03c3, 0x03c4, 0x03c5, 0x03d6, 0x03c9,
    0x03be, 0x03c8, 0x03b6, 0x007b, 0x007c, 0x007d, 0x223c, 0x007f,

    0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
    0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f,
    0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
    0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f,
    0x20ac, 0x03d2, 0x2023, 0x2264, 0x2044, 0x221e, 0x0192, 0x2263,
    0x2666, 0x2665, 0x2660, 0x2194, 0x2190, 0x2191, 0x2192, 0x2193,
    0x00b0, 0x00b1, 0x2033, 0x2265, 0x00d7, 0x221d, 0x2202, 0x2022,
    0x00f7, 0x2260, 0x2261, 0x2248, 0x2026, 0xf8e6, 0xf8e7, 0x21b5,

    0x2135, 0x2111, 0x211c, 0x2118, 0x2297, 0x2295, 0x2205, 0x2229,
    0x222a, 0x2283, 0x2287, 0x2284, 0x2282, 0x2286, 0x2208, 0x2209,
    0x2220, 0x2207, 0xf6da, 0xf6d9, 0xf6db, 0x220f, 0x221a, 0x22c5,
    0x00ac, 0x2227, 0x2228, 0x21d4, 0x21d0, 0x21d1, 0x21d2, 0x21d3,
    0x25ca, 0x2329, 0xf8e8, 0xf8e9, 0xf8ea, 0x2211, 0xf8eb, 0xf8ec,
    0xf8ed, 0xf8ee, 0xf8ef, 0xf8f0, 0xf8f1, 0xf8f2, 0xf8f3, 0xf8f4,
    0x0000, 0x232a, 0x222b, 0x2320, 0xf8f5, 0x2321, 0xf8f6, 0xf8f7,
    0xf8f8, 0xf8f9, 0xf8fa, 0xf8fb, 0xf8fc, 0xf8fd, 0xf8fe, 0x0000,
};

// ---------------------------- PS/PDF helper methods -----------------------------------

QByteArray QFontSubset::glyphName(unsigned short unicode, bool symbol)
{
    if (symbol && unicode < 0x100)
        // map from latin1 to symbol
        unicode = symbol_map[unicode];

    int l = 0;
    while(unicode_to_aglindex[l].u < unicode)
        l++;
    if (unicode_to_aglindex[l].u == unicode)
        return agl + unicode_to_aglindex[l].index;

    char buffer[8];
    buffer[0] = 'u';
    buffer[1] = 'n';
    buffer[2] = 'i';
    QPdf::toHex(unicode, buffer+3);
    return buffer;
}

#ifndef QT_NO_FREETYPE
static FT_Face ft_face(const QFontEngine *engine)
{
#ifdef Q_WS_X11
#ifndef QT_NO_FONTCONFIG
    if (engine->type() == QFontEngine::Freetype) {
        const QFontEngineFT *ft = static_cast<const QFontEngineFT *>(engine);
        return ft->non_locked_face();
    } else
#endif
    if (engine->type() == QFontEngine::XLFD) {
        const QFontEngineXLFD *xlfd = static_cast<const QFontEngineXLFD *>(engine);
        return xlfd->non_locked_face();
    }
#endif
#ifdef Q_WS_QWS
    if (engine->type() == QFontEngine::Freetype) {
        const QFontEngineFT *ft = static_cast<const QFontEngineFT *>(engine);
        return ft->non_locked_face();
    }
#endif
    return 0;
}
#endif

QByteArray QFontSubset::glyphName(unsigned int glyph, const QVector<int> reverseMap) const
{
    uint glyphIndex = glyph_indices[glyph];

    if (glyphIndex == 0)
        return "/.notdef";

    QByteArray ba;
    QPdf::ByteStream s(&ba);
#ifndef QT_NO_FREETYPE
    FT_Face face = ft_face(fontEngine);

    char name[32];
    name[0] = 0;
    if (face && FT_HAS_GLYPH_NAMES(face)) {
#if defined(Q_WS_X11)
        if (fontEngine->type() == QFontEngine::XLFD)
            glyphIndex = static_cast<QFontEngineXLFD *>(fontEngine)->glyphIndexToFreetypeGlyphIndex(glyphIndex);
#endif
        FT_Get_Glyph_Name(face, glyphIndex, &name, 32);
        if (name[0] == '.') // fix broken PS fonts returning .notdef for many glyphs
            name[0] = 0;
    }
    if (name[0]) {
        s << '/' << name;
    } else
#endif
#if defined(Q_WS_X11)
    if (fontEngine->type() == QFontEngine::XLFD) {
        uint uc = static_cast<QFontEngineXLFD *>(fontEngine)->toUnicode(glyphIndex);
        s << '/' << glyphName(uc, false /* ### */);
    } else
#endif
    if (reverseMap[glyphIndex] && reverseMap[glyphIndex] < 0x10000) {
        s << '/' << glyphName(reverseMap[glyphIndex], false);
    } else {
        s << "/gl" << (int)glyphIndex;
    }
    return ba;
}


QByteArray QFontSubset::widthArray() const
{
    Q_ASSERT(!widths.isEmpty());

    QFontEngine::Properties properties = fontEngine->properties();

    QByteArray width;
    QPdf::ByteStream s(&width);
    QFixed scale = QFixed(1000)/emSquare;

    QFixed defWidth = widths[0];
    //qDebug("defWidth=%d, scale=%f", defWidth.toInt(), scale.toReal());
    for (int i = 0; i < nGlyphs(); ++i) {
        if (defWidth != widths[i])
            defWidth = 0;
    }
    if (defWidth > 0) {
        s << "/DW " << (defWidth*scale).toInt();
    } else {
        s << "/W [";
        for (int g = 0; g < nGlyphs();) {
            QFixed w = widths[g];
            int start = g;
            int startLinear = 0;
            ++g;
            while (g < nGlyphs()) {
                QFixed nw = widths[g];
                if (nw == w) {
                if (!startLinear)
                    startLinear = g - 1;
                } else {
                    if (startLinear > 0 && g - startLinear >= 10)
                        break;
                    startLinear = 0;
                }
                w = nw;
                ++g;
            }
            // qDebug("start=%x startLinear=%x g-1=%x",start,startLinear,g-1);
            if (g - startLinear < 10)
                startLinear = 0;
            int endnonlinear = startLinear ? startLinear : g;
            // qDebug("    startLinear=%x endnonlinear=%x", startLinear,endnonlinear);
            if (endnonlinear > start) {
                s << start << '[';
                for (int i = start; i < endnonlinear; ++i)
                    s << (widths[i]*scale).toInt();
                s << "]\n";
            }
            if (startLinear)
                s << startLinear << g - 1 << (widths[startLinear]*scale).toInt() << '\n';
        }
        s << "]\n";
    }
    return width;
}

static void checkRanges(QPdf::ByteStream &ts, QByteArray &ranges, int &nranges)
{
    if (++nranges > 100) {
        ts << nranges << "beginbfrange\n"
           << ranges << "endbfrange\n";
        ranges = QByteArray();
        nranges = 0;
    }
}

QVector<int> QFontSubset::getReverseMap() const
{
    QVector<int> reverseMap;
    reverseMap.resize(0x10000);
    for (uint i = 0; i < 0x10000; ++i)
        reverseMap[i] = 0;
    QGlyphLayoutArray<10> glyphs;
    for (uint uc = 0; uc < 0x10000; ++uc) {
        QChar ch(uc);
        int nglyphs = 10;
        fontEngine->stringToCMap(&ch, 1, &glyphs, &nglyphs, QTextEngine::GlyphIndicesOnly);
        int idx = glyph_indices.indexOf(glyphs.glyphs[0]);
        if (idx >= 0 && !reverseMap.at(idx))
            reverseMap[idx] = uc;
    }
    return reverseMap;
}

QByteArray QFontSubset::createToUnicodeMap() const
{
    QVector<int> reverseMap = getReverseMap();

    QByteArray touc;
    QPdf::ByteStream ts(&touc);
    ts << "/CIDInit /ProcSet findresource begin\n"
        "12 dict begin\n"
        "begincmap\n"
        "/CIDSystemInfo << /Registry (Adobe) /Ordering (UCS) /Supplement 0 >> def\n"
        "/CMapName /Adobe-Identity-UCS def\n"
        "/CMapType 2 def\n"
        "1 begincodespacerange\n"
        "<0000> <FFFF>\n"
        "endcodespacerange\n";

    int nranges = 1;
    QByteArray ranges = "<0000> <0000> <0000>\n";
    QPdf::ByteStream s(&ranges);

    char buf[5];
    for (int g = 1; g < nGlyphs(); ) {
        int uc0 = reverseMap.at(g);
        if (!uc0) {
            ++g;
            continue;
        }
        int start = g;
        int startLinear = 0;
        ++g;
        while (g < nGlyphs()) {
            int uc = reverseMap[g];
            // cmaps can't have the high byte changing within one range, so we need to break on that as well
            if (!uc || (g>>8) != (start >> 8))
                break;
            if (uc == uc0 + 1) {
                if (!startLinear)
                    startLinear = g - 1;
            } else {
                if (startLinear > 0 && g - startLinear >= 10)
                    break;
                startLinear = 0;
            }
            uc0 = uc;
            ++g;
        }
        // qDebug("start=%x startLinear=%x g-1=%x",start,startLinear,g-1);
        if (g - startLinear < 10)
            startLinear = 0;
        int endnonlinear = startLinear ? startLinear : g;
        // qDebug("    startLinear=%x endnonlinear=%x", startLinear,endnonlinear);
        if (endnonlinear > start) {
            s << '<' << QPdf::toHex((ushort)start, buf) << "> <";
            s << QPdf::toHex((ushort)(endnonlinear - 1), buf) << "> ";
            if (endnonlinear == start + 1) {
                s << '<' << QPdf::toHex((ushort)reverseMap[start], buf) << ">\n";
            } else {
                s << '[';
                for (int i = start; i < endnonlinear; ++i) {
                    s << '<' << QPdf::toHex((ushort)reverseMap[i], buf) << "> ";
                }
                s << "]\n";
            }
            checkRanges(ts, ranges, nranges);
        }
        if (startLinear) {
            while (startLinear < g) {
                int len = g - startLinear;
                int uc_start = reverseMap[startLinear];
                int uc_end = uc_start + len - 1;
                if ((uc_end >> 8) != (uc_start >> 8))
                    len = 256 - (uc_start & 0xff);
                s << '<' << QPdf::toHex((ushort)startLinear, buf) << "> <";
                s << QPdf::toHex((ushort)(startLinear + len - 1), buf) << "> ";
                s << '<' << QPdf::toHex((ushort)reverseMap[startLinear], buf) << ">\n";
                checkRanges(ts, ranges, nranges);
                startLinear += len;
            }
        }
    }
    if (nranges) {
        ts << nranges << "beginbfrange\n"
           << ranges << "endbfrange\n";
    }
    ts << "endcmap\n"
        "CMapName currentdict /CMap defineresource pop\n"
        "end\n"
        "end\n";

    return touc;
}

int QFontSubset::addGlyph(int index)
{
    int idx = glyph_indices.indexOf(index);
    if (idx < 0) {
        idx = glyph_indices.size();
        glyph_indices.append(index);
    }
    return idx;
}


// ------------------------------ Truetype generation ----------------------------------------------

typedef qint16 F2DOT14;
typedef quint32 Tag;
typedef quint16 GlyphID;
typedef quint16 Offset;


class QTtfStream {
public:
    QTtfStream(QByteArray &ba) : data((uchar *)ba.data()) { start = data; }
    QTtfStream &operator <<(quint8 v) { *data = v; ++data; return *this; }
    QTtfStream &operator <<(quint16 v) { qToBigEndian(v, data); data += sizeof(v); return *this; }
    QTtfStream &operator <<(quint32 v) { qToBigEndian(v, data); data += sizeof(v); return *this; }
    QTtfStream &operator <<(qint8 v) { *data = quint8(v); ++data; return *this; }
    QTtfStream &operator <<(qint16 v) { qToBigEndian(v, data); data += sizeof(v); return *this; }
    QTtfStream &operator <<(qint32 v) { qToBigEndian(v, data); data += sizeof(v); return *this; }
    QTtfStream &operator <<(qint64 v) { qToBigEndian(v, data); data += sizeof(v); return *this; }

    int offset() const { return data - start; }
    void setOffset(int o) { data = start + o; }
    void align4() { while (offset() & 3) { *data = '\0'; ++data; } }
private:
    uchar *data;
    uchar *start;
};

struct QTtfTable {
    Tag tag;
    QByteArray data;
};
Q_DECLARE_TYPEINFO(QTtfTable, Q_MOVABLE_TYPE);


struct qttf_head_table {
    qint32 font_revision;
    quint16 flags;
    qint64 created;
    qint64 modified;
    qint16 xMin;
    qint16 yMin;
    qint16 xMax;
    qint16 yMax;
    quint16 macStyle;
    qint16 indexToLocFormat;
};


struct qttf_hhea_table {
    qint16 ascender;
    qint16 descender;
    qint16 lineGap;
    quint16 maxAdvanceWidth;
    qint16 minLeftSideBearing;
    qint16 minRightSideBearing;
    qint16 xMaxExtent;
    quint16 numberOfHMetrics;
};


struct qttf_maxp_table {
    quint16 numGlyphs;
    quint16 maxPoints;
    quint16 maxContours;
    quint16 maxCompositePoints;
    quint16 maxCompositeContours;
    quint16 maxComponentElements;
    quint16 maxComponentDepth;
};

struct qttf_name_table {
    QString copyright;
    QString family;
    QString subfamily;
    QString postscript_name;
};


static QTtfTable generateHead(const qttf_head_table &head);
static QTtfTable generateHhea(const qttf_hhea_table &hhea);
static QTtfTable generateMaxp(const qttf_maxp_table &maxp);
static QTtfTable generateName(const qttf_name_table &name);

struct qttf_font_tables
{
    qttf_head_table head;
    qttf_hhea_table hhea;
    qttf_maxp_table maxp;
};


struct QTtfGlyph {
    quint16 index;
    qint16 xMin;
    qint16 xMax;
    qint16 yMin;
    qint16 yMax;
    quint16 advanceWidth;
    qint16 lsb;
    quint16 numContours;
    quint16 numPoints;
    QByteArray data;
};
Q_DECLARE_TYPEINFO(QTtfGlyph, Q_MOVABLE_TYPE);

static QTtfGlyph generateGlyph(int index, const QPainterPath &path, qreal advance, qreal lsb, qreal ppem);
// generates glyf, loca and hmtx
static QList<QTtfTable> generateGlyphTables(qttf_font_tables &tables, const QList<QTtfGlyph> &_glyphs);

static QByteArray bindFont(const QList<QTtfTable>& _tables);


static quint32 checksum(const QByteArray &table)
{
    quint32 sum = 0;
    int offset = 0;
    const uchar *d = (uchar *)table.constData();
    while (offset <= table.size()-3) {
        sum += qFromBigEndian<quint32>(d + offset);
        offset += 4;
    }
    int shift = 24;
    quint32 x = 0;
    while (offset < table.size()) {
        x |= ((quint32)d[offset]) << shift;
        ++offset;
        shift -= 8;
    }
    sum += x;

    return sum;
}

static QTtfTable generateHead(const qttf_head_table &head)
{
    const int head_size = 54;
    QTtfTable t;
    t.tag = MAKE_TAG('h', 'e', 'a', 'd');
    t.data.resize(head_size);

    QTtfStream s(t.data);

// qint32  Table version number  0x00010000 for version 1.0.
// qint32  fontRevision  Set by font manufacturer.
    s << qint32(0x00010000)
      << head.font_revision
// quint32  checkSumAdjustment  To compute: set it to 0, sum the entire font as quint32, then store 0xB1B0AFBA - sum.
      << quint32(0)
// quint32  magicNumber  Set to 0x5F0F3CF5.
      << quint32(0x5F0F3CF5)
// quint16  flags  Bit 0: Baseline for font at y=0;
// Bit 1: Left sidebearing point at x=0;
// Bit 2: Instructions may depend on point size;
// Bit 3: Force ppem to integer values for all internal scaler math; may use fractional ppem sizes if this bit is clear;
// Bit 4: Instructions may alter advance width (the advance widths might not scale linearly);
// Bits 5-10: These should be set according to  Apple's specification . However, they are not implemented in OpenType.
// Bit 11: Font data is 'lossless,' as a result of having been compressed and decompressed with the Agfa MicroType Express engine.
// Bit 12: Font converted (produce compatible metrics)
// Bit 13: Font optimized for ClearType
// Bit 14: Reserved, set to 0
// Bit 15: Reserved, set to 0
      << quint16(0)

// quint16  unitsPerEm  Valid range is from 16 to 16384. This value should be a power of 2 for fonts that have TrueType outlines.
      << quint16(2048)
// qint64  created  Number of seconds since 12:00 midnight, January 1, 1904. 64-bit integer
      << head.created
// qint64  modified  Number of seconds since 12:00 midnight, January 1, 1904. 64-bit integer
      << head.modified
// qint16  xMin  For all glyph bounding boxes.
// qint16  yMin  For all glyph bounding boxes.
// qint16  xMax  For all glyph bounding boxes.
// qint16  yMax  For all glyph bounding boxes.
      << head.xMin
      << head.yMin
      << head.xMax
      << head.yMax
// quint16  macStyle  Bit 0: Bold (if set to 1);
// Bit 1: Italic (if set to 1)
// Bit 2: Underline (if set to 1)
// Bit 3: Outline (if set to 1)
// Bit 4: Shadow (if set to 1)
// Bit 5: Condensed (if set to 1)
// Bit 6: Extended (if set to 1)
// Bits 7-15: Reserved (set to 0).
      << head.macStyle
// quint16  lowestRecPPEM  Smallest readable size in pixels.
      << quint16(6) // just a wild guess
// qint16  fontDirectionHint   0: Fully mixed directional glyphs;
      << qint16(0)
// 1: Only strongly left to right;
// 2: Like 1 but also contains neutrals;
// -1: Only strongly right to left;
// -2: Like -1 but also contains neutrals. 1
// qint16  indexToLocFormat  0 for short offsets, 1 for long.
      << head.indexToLocFormat
// qint16  glyphDataFormat  0 for current format.
      << qint16(0);

    Q_ASSERT(s.offset() == head_size);
    return t;
}


static QTtfTable generateHhea(const qttf_hhea_table &hhea)
{
    const int hhea_size = 36;
    QTtfTable t;
    t.tag = MAKE_TAG('h', 'h', 'e', 'a');
    t.data.resize(hhea_size);

    QTtfStream s(t.data);
// qint32  Table version number  0x00010000 for version 1.0.
    s << qint32(0x00010000)
// qint16  Ascender  Typographic ascent.  (Distance from baseline of highest ascender)
      << hhea.ascender
// qint16  Descender  Typographic descent.  (Distance from baseline of lowest descender)
      << hhea.descender
// qint16  LineGap  Typographic line gap.
// Negative LineGap values are treated as zero
// in Windows 3.1, System 6, and
// System 7.
      << hhea.lineGap
// quint16  advanceWidthMax  Maximum advance width value in 'hmtx' table.
      << hhea.maxAdvanceWidth
// qint16  minLeftSideBearing  Minimum left sidebearing value in 'hmtx' table.
      << hhea.minLeftSideBearing
// qint16  minRightSideBearing  Minimum right sidebearing value; calculated as Min(aw - lsb - (xMax - xMin)).
      << hhea.minRightSideBearing
// qint16  xMaxExtent  Max(lsb + (xMax - xMin)).
      << hhea.xMaxExtent
// qint16  caretSlopeRise  Used to calculate the slope of the cursor (rise/run); 1 for vertical.
      << qint16(1)
// qint16  caretSlopeRun  0 for vertical.
      << qint16(0)
// qint16  caretOffset  The amount by which a slanted highlight on a glyph needs to be shifted to produce the best appearance. Set to 0 for non-slanted fonts
      << qint16(0)
// qint16  (reserved)  set to 0
      << qint16(0)
// qint16  (reserved)  set to 0
      << qint16(0)
// qint16  (reserved)  set to 0
      << qint16(0)
// qint16  (reserved)  set to 0
      << qint16(0)
// qint16  metricDataFormat  0 for current format.
      << qint16(0)
// quint16  numberOfHMetrics  Number of hMetric entries in 'hmtx' table
      << hhea.numberOfHMetrics;

    Q_ASSERT(s.offset() == hhea_size);
    return t;
}


static QTtfTable generateMaxp(const qttf_maxp_table &maxp)
{
    const int maxp_size = 32;
    QTtfTable t;
    t.tag = MAKE_TAG('m', 'a', 'x', 'p');
    t.data.resize(maxp_size);

    QTtfStream s(t.data);

// qint32  Table version number  0x00010000 for version 1.0.
    s << qint32(0x00010000)
// quint16  numGlyphs  The number of glyphs in the font.
      << maxp.numGlyphs
// quint16  maxPoints  Maximum points in a non-composite glyph.
      << maxp.maxPoints
// quint16  maxContours  Maximum contours in a non-composite glyph.
      << maxp.maxContours
// quint16  maxCompositePoints  Maximum points in a composite glyph.
      << maxp.maxCompositePoints
// quint16  maxCompositeContours  Maximum contours in a composite glyph.
      << maxp.maxCompositeContours
// quint16  maxZones  1 if instructions do not use the twilight zone (Z0), or 2 if instructions do use Z0; should be set to 2 in most cases.
      << quint16(1) // we do not embed instructions
// quint16  maxTwilightPoints  Maximum points used in Z0.
      << quint16(0)
// quint16  maxStorage  Number of Storage Area locations.
      << quint16(0)
// quint16  maxFunctionDefs  Number of FDEFs.
      << quint16(0)
// quint16  maxInstructionDefs  Number of IDEFs.
      << quint16(0)
// quint16  maxStackElements  Maximum stack depth2.
      << quint16(0)
// quint16  maxSizeOfInstructions  Maximum byte count for glyph instructions.
      << quint16(0)
// quint16  maxComponentElements  Maximum number of components referenced at "top level" for any composite glyph.
      << maxp.maxComponentElements
// quint16  maxComponentDepth  Maximum levels of recursion; 1 for simple components.
      << maxp.maxComponentDepth;

    Q_ASSERT(s.offset() == maxp_size);
    return t;
}

struct QTtfNameRecord {
    quint16 nameId;
    QString value;
};

static QTtfTable generateName(const QList<QTtfNameRecord> &name);

static QTtfTable generateName(const qttf_name_table &name)
{
    QList<QTtfNameRecord> list;
    QTtfNameRecord rec;
    rec.nameId = 0;
    rec.value = name.copyright;
    list.append(rec);
    rec.nameId = 1;
    rec.value = name.family;
    list.append(rec);
    rec.nameId = 2;
    rec.value = name.subfamily;
    list.append(rec);
    rec.nameId = 4;
    rec.value = name.family;
    if (name.subfamily != QLatin1String("Regular"))
        rec.value += QLatin1Char(' ') + name.subfamily;
    list.append(rec);
    rec.nameId = 6;
    rec.value = name.postscript_name;
    list.append(rec);

    return generateName(list);
}

// ####### should probably generate Macintosh/Roman name entries as well
static QTtfTable generateName(const QList<QTtfNameRecord> &name)
{
    const int char_size = 2;

    QTtfTable t;
    t.tag = MAKE_TAG('n', 'a', 'm', 'e');

    const int name_size = 6 + 12*name.size();
    int string_size = 0;
    for (int i = 0; i < name.size(); ++i) {
        string_size += name.at(i).value.length()*char_size;
    }
    t.data.resize(name_size + string_size);

    QTtfStream s(t.data);
// quint16  format  Format selector (=0).
    s << quint16(0)
// quint16  count  Number of name records.
      << quint16(name.size())
// quint16  stringOffset  Offset to start of string storage (from start of table).
      << quint16(name_size);
// NameRecord  nameRecord[count]  The name records where count is the number of records.
// (Variable)

    int off = 0;
    for (int i = 0; i < name.size(); ++i) {
        int len = name.at(i).value.length()*char_size;
// quint16  platformID  Platform ID.
// quint16  encodingID  Platform-specific encoding ID.
// quint16  languageID  Language ID.
        s << quint16(3)
          << quint16(1)
          << quint16(0x0409) // en_US
// quint16  nameId  Name ID.
          << name.at(i).nameId
// quint16  length  String length (in bytes).
          << quint16(len)
// quint16  offset  String offset from start of storage area (in bytes).
          << quint16(off);
        off += len;
    }
    for (int i = 0; i < name.size(); ++i) {
        const QString &n = name.at(i).value;
        const ushort *uc = n.utf16();
        for (int i = 0; i < n.length(); ++i) {
            s << quint16(*uc);
            ++uc;
        }
    }
    return t;
}


enum Flags {
    OffCurve = 0,
    OnCurve = (1 << 0),
    XShortVector = (1 << 1),
    YShortVector = (1 << 2),
    Repeat = (1 << 3),
    XSame = (1 << 4),
    XShortPositive = (1 << 4),
    YSame = (1 << 5),
    YShortPositive = (1 << 5)
};
struct TTF_POINT {
    qint16 x;
    qint16 y;
    quint8 flags;
};
Q_DECLARE_TYPEINFO(TTF_POINT, Q_PRIMITIVE_TYPE);

static void convertPath(const QPainterPath &path, QList<TTF_POINT> *points, QList<int> *endPoints, qreal ppem)
{
    int numElements = path.elementCount();
    for (int i = 0; i < numElements - 1; ++i) {
        const QPainterPath::Element &e = path.elementAt(i);
        TTF_POINT p;
        p.x = qRound(e.x * 2048. / ppem);
        p.y = qRound(-e.y * 2048. / ppem);
        p.flags = 0;

        switch(e.type) {
        case QPainterPath::MoveToElement:
            if (i != 0) {
                // see if start and end points of the last contour agree
                int start = endPoints->size() ? endPoints->at(endPoints->size()-1) - 1 : 0;
                int end = points->size() - 1;
                if (points->at(end).x == points->at(start).x
                    && points->at(end).y == points->at(start).y)
                    points->takeLast();
                endPoints->append(points->size() - 1);
            }
            // fall through
        case QPainterPath::LineToElement:
            p.flags = OnCurve;
            break;
        case QPainterPath::CurveToElement: {
            // cubic bezier curve, we need to reduce to a list of quadratic curves
            TTF_POINT list[3*16 + 4]; // we need max 16 subdivisions
            list[3] = points->at(points->size() - 1);
            list[2] = p;
            const QPainterPath::Element &e2 = path.elementAt(++i);
            list[1].x = qRound(e2.x * 2048. / ppem);
            list[1].y = qRound(-e2.y * 2048. / ppem);
            const QPainterPath::Element &e3 = path.elementAt(++i);
            list[0].x = qRound(e3.x * 2048. / ppem);
            list[0].y = qRound(-e3.y * 2048. / ppem);

            TTF_POINT *base = list;

            bool try_reduce = points->size() > 1
                              && points->at(points->size() - 1).flags == OnCurve
                              && points->at(points->size() - 2).flags == OffCurve;
//             qDebug("generating beziers:");
            while (base >= list) {
                const int split_limit = 3;
//                 {
//                     qDebug("iteration:");
//                     TTF_POINT *x = list;
//                     while (x <= base + 3) {
//                         qDebug() << "    " << QPoint(x->x, x->y);
//                         ++x;
//                     }
//                 }
                Q_ASSERT(base - list < 3*16 + 1);
                // first see if we can easily reduce the cubic to a quadratic bezier curve
                int i1_x = base[1].x + ((base[1].x - base[0].x) >> 1);
                int i1_y = base[1].y + ((base[1].y - base[0].y) >> 1);
                int i2_x = base[2].x + ((base[2].x - base[3].x) >> 1);
                int i2_y = base[2].y + ((base[2].y - base[3].y) >> 1);
//                 qDebug() << "checking: i1=" << QPoint(i1_x, i1_y) << " i2=" << QPoint(i2_x, i2_y);
                if (qAbs(i1_x - i2_x) <= split_limit && qAbs(i1_y - i2_y) <= split_limit) {
                    // got a quadratic bezier curve
                    TTF_POINT np;
                    np.x = (i1_x + i2_x) >> 1;
                    np.y = (i1_y + i2_y) >> 1;
                    if (try_reduce) {
                        // see if we can optimize out the last onCurve point
                        int mx = (points->at(points->size() - 2).x + base[2].x) >> 1;
                        int my = (points->at(points->size() - 2).y + base[2].y) >> 1;
                        if (qAbs(mx - base[3].x) <= split_limit && qAbs(my = base[3].y) <= split_limit)
                            points->takeLast();
                        try_reduce = false;
                    }
                    np.flags = OffCurve;
                    points->append(np);
//                     qDebug() << "   appending offcurve point " << QPoint(np.x, np.y);
                    base -= 3;
                } else {
                    // need to split
//                     qDebug() << "  -> splitting";
                    qint16 a, b, c, d;
                    base[6].x = base[3].x;
                    c = base[1].x;
                    d = base[2].x;
                    base[1].x = a = ( base[0].x + c ) >> 1;
                    base[5].x = b = ( base[3].x + d ) >> 1;
                    c = ( c + d ) >> 1;
                    base[2].x = a = ( a + c ) >> 1;
                    base[4].x = b = ( b + c ) >> 1;
                    base[3].x = ( a + b ) >> 1;

                    base[6].y = base[3].y;
                    c = base[1].y;
                    d = base[2].y;
                    base[1].y = a = ( base[0].y + c ) >> 1;
                    base[5].y = b = ( base[3].y + d ) >> 1;
                    c = ( c + d ) >> 1;
                    base[2].y = a = ( a + c ) >> 1;
                    base[4].y = b = ( b + c ) >> 1;
                    base[3].y = ( a + b ) >> 1;
                    base += 3;
                }
            }
            p = list[0];
            p.flags = OnCurve;
            break;
        }
        case QPainterPath::CurveToDataElement:
            Q_ASSERT(false);
            break;
        }
//         qDebug() << "   appending oncurve point " << QPoint(p.x, p.y);
        points->append(p);
    }
    int start = endPoints->size() ? endPoints->at(endPoints->size()-1) + 1 : 0;
    int end = points->size() - 1;
    if (points->at(end).x == points->at(start).x
        && points->at(end).y == points->at(start).y)
        points->takeLast();
    endPoints->append(points->size() - 1);
}

static void getBounds(const QList<TTF_POINT> &points, qint16 *xmin, qint16 *xmax, qint16 *ymin, qint16 *ymax)
{
    *xmin = points.at(0).x;
    *xmax = *xmin;
    *ymin = points.at(0).y;
    *ymax = *ymin;

    for (int i = 1; i < points.size(); ++i) {
        *xmin = qMin(*xmin, points.at(i).x);
        *xmax = qMax(*xmax, points.at(i).x);
        *ymin = qMin(*ymin, points.at(i).y);
        *ymax = qMax(*ymax, points.at(i).y);
    }
}

static int convertToRelative(QList<TTF_POINT> *points)
{
    // convert points to relative and setup flags
//     qDebug() << "relative points:";
    qint16 prev_x = 0;
    qint16 prev_y = 0;
    int point_array_size = 0;
    for (int i = 0; i < points->size(); ++i) {
        const int x = points->at(i).x;
        const int y = points->at(i).y;
        TTF_POINT rel;
        rel.x = x - prev_x;
        rel.y = y - prev_y;
        rel.flags = points->at(i).flags;
        Q_ASSERT(rel.flags < 2);
        if (!rel.x) {
            rel.flags |= XSame;
        } else if (rel.x > 0 && rel.x < 256) {
            rel.flags |= XShortVector|XShortPositive;
            point_array_size++;
        } else if (rel.x < 0 && rel.x > -256) {
            rel.flags |= XShortVector;
            rel.x = -rel.x;
            point_array_size++;
        } else {
            point_array_size += 2;
        }
        if (!rel.y) {
            rel.flags |= YSame;
        } else if (rel.y > 0 && rel.y < 256) {
            rel.flags |= YShortVector|YShortPositive;
            point_array_size++;
        } else if (rel.y < 0 && rel.y > -256) {
            rel.flags |= YShortVector;
            rel.y = -rel.y;
            point_array_size++;
        } else {
            point_array_size += 2;
        }
        (*points)[i] = rel;
// #define toString(x) ((rel.flags & x) ? #x : "")
//         qDebug() << "    " << QPoint(rel.x, rel.y) << "flags="
//                  << toString(OnCurve) << toString(XShortVector)
//                  << (rel.flags & XShortVector ? toString(XShortPositive) : toString(XSame))
//                  << toString(YShortVector)
//                  << (rel.flags & YShortVector ? toString(YShortPositive) : toString(YSame));

        prev_x = x;
        prev_y = y;
    }
    return point_array_size;
}

static void getGlyphData(QTtfGlyph *glyph, const QList<TTF_POINT> &points, const QList<int> &endPoints, int point_array_size)
{
    const int max_size = 5*sizeof(qint16) // header
                         + endPoints.size()*sizeof(quint16) // end points of contours
                         + sizeof(quint16) // instruction length == 0
                         + points.size()*(1) // flags
                         + point_array_size; // coordinates

    glyph->data.resize(max_size);

    QTtfStream s(glyph->data);
    s << qint16(endPoints.size())
      << glyph->xMin << glyph->yMin << glyph->xMax << glyph->yMax;

    for (int i = 0; i < endPoints.size(); ++i)
        s << quint16(endPoints.at(i));
    s << quint16(0); // instruction length

    // emit flags
    for (int i = 0; i < points.size(); ++i)
        s << quint8(points.at(i).flags);
    // emit points
    for (int i = 0; i < points.size(); ++i) {
        quint8 flags = points.at(i).flags;
        qint16 x = points.at(i).x;

        if (flags & XShortVector)
            s << quint8(x);
        else if (!(flags & XSame))
            s << qint16(x);
    }
    for (int i = 0; i < points.size(); ++i) {
        quint8 flags = points.at(i).flags;
        qint16 y = points.at(i).y;

        if (flags & YShortVector)
            s << quint8(y);
        else if (!(flags & YSame))
            s << qint16(y);
    }

//     qDebug() << "offset=" << s.offset() << "max_size=" << max_size << "point_array_size=" << point_array_size;
    Q_ASSERT(s.offset() == max_size);

    glyph->numContours = endPoints.size();
    glyph->numPoints = points.size();
}

static QTtfGlyph generateGlyph(int index, const QPainterPath &path, qreal advance, qreal lsb, qreal ppem)
{
    QList<TTF_POINT> points;
    QList<int> endPoints;
    QTtfGlyph glyph;
    glyph.index = index;
    glyph.advanceWidth = qRound(advance * 2048. / ppem);
    glyph.lsb = qRound(lsb * 2048. / ppem);

    if (!path.elementCount()) {
        //qDebug("glyph %d is empty", index);
        lsb = 0;
        glyph.xMin = glyph.xMax = glyph.yMin = glyph.yMax = 0;
        glyph.numContours = 0;
        glyph.numPoints = 0;
        return glyph;
    }

    convertPath(path, &points, &endPoints, ppem);

//     qDebug() << "number of contours=" << endPoints.size();
//     for (int i = 0; i < points.size(); ++i)
//         qDebug() << "  point[" << i << "] = " << QPoint(points.at(i).x, points.at(i).y) << " flags=" << points.at(i).flags;
//     qDebug() << "endPoints:";
//     for (int i = 0; i < endPoints.size(); ++i)
//         qDebug() << endPoints.at(i);

    getBounds(points, &glyph.xMin, &glyph.xMax, &glyph.yMin, &glyph.yMax);
    int point_array_size = convertToRelative(&points);
    getGlyphData(&glyph, points, endPoints, point_array_size);
    return glyph;
}

Q_STATIC_GLOBAL_OPERATOR bool operator <(const QTtfGlyph &g1, const QTtfGlyph &g2)
{
    return g1.index < g2.index;
}

static QList<QTtfTable> generateGlyphTables(qttf_font_tables &tables, const QList<QTtfGlyph> &_glyphs)
{
    const int max_size_small = 65536*2;
    QList<QTtfGlyph> glyphs = _glyphs;
    qSort(glyphs);

    Q_ASSERT(tables.maxp.numGlyphs == glyphs.at(glyphs.size()-1).index + 1);
    int nGlyphs = tables.maxp.numGlyphs;

    int glyf_size = 0;
    for (int i = 0; i < glyphs.size(); ++i)
        glyf_size += (glyphs.at(i).data.size() + 3) & ~3;

    tables.head.indexToLocFormat = glyf_size < max_size_small ? 0 : 1;
    tables.hhea.numberOfHMetrics = nGlyphs;

    QTtfTable glyf;
    glyf.tag = MAKE_TAG('g', 'l', 'y', 'f');

    QTtfTable loca;
    loca.tag = MAKE_TAG('l', 'o', 'c', 'a');
    loca.data.resize(glyf_size < max_size_small ? (nGlyphs+1)*sizeof(quint16) : (nGlyphs+1)*sizeof(quint32));
    QTtfStream ls(loca.data);

    QTtfTable hmtx;
    hmtx.tag = MAKE_TAG('h', 'm', 't', 'x');
    hmtx.data.resize(nGlyphs*4);
    QTtfStream hs(hmtx.data);

    int pos = 0;
    for (int i = 0; i < nGlyphs; ++i) {
        int gpos = glyf.data.size();
        quint16 advance = 0;
        qint16 lsb = 0;

        if (glyphs[pos].index == i) {
            // emit glyph
//             qDebug("emitting glyph %d: size=%d", i, glyphs.at(i).data.size());
            glyf.data += glyphs.at(pos).data;
            while (glyf.data.size() & 1)
                glyf.data.append('\0');
            advance = glyphs.at(pos).advanceWidth;
            lsb = glyphs.at(pos).lsb;
            ++pos;
        }
        if (glyf_size < max_size_small) {
            // use short loca format
            ls << quint16(gpos>>1);
        } else {
            // use long loca format
            ls << quint32(gpos);
        }
        hs << advance
           << lsb;
    }
    if (glyf_size < max_size_small) {
        // use short loca format
        ls << quint16(glyf.data.size()>>1);
    } else {
        // use long loca format
        ls << quint32(glyf.data.size());
    }

    Q_ASSERT(loca.data.size() == ls.offset());
    Q_ASSERT(hmtx.data.size() == hs.offset());

    QList<QTtfTable> list;
    list.append(glyf);
    list.append(loca);
    list.append(hmtx);
    return list;
}

Q_STATIC_GLOBAL_OPERATOR bool operator <(const QTtfTable &t1, const QTtfTable &t2)
{
    return t1.tag < t2.tag;
}

static QByteArray bindFont(const QList<QTtfTable>& _tables)
{
    QList<QTtfTable> tables = _tables;

    qSort(tables);

    QByteArray font;
    const int header_size = sizeof(qint32) + 4*sizeof(quint16);
    const int directory_size = 4*sizeof(quint32)*tables.size();
    font.resize(header_size + directory_size);

    int log2 = 0;
    int pow = 1;
    int n = tables.size() >> 1;
    while (n) {
        ++log2;
        pow <<= 1;
        n >>= 1;
    }

    quint32 head_offset = 0;
    {
        QTtfStream f(font);
// Offset Table
// Type  Name  Description
//   qint32  sfnt version  0x00010000 for version 1.0.
//   quint16   numTables  Number of tables.
//   quint16   searchRange  (Maximum power of 2 <= numTables) x 16.
//   quint16   entrySelector  Log2(maximum power of 2 <= numTables).
//   quint16   rangeShift  NumTables x 16-searchRange.
        f << qint32(0x00010000)
          << quint16(tables.size())
          << quint16(16*pow)
          << quint16(log2)
          << quint16(16*(tables.size() - pow));

// Table Directory
// Type  Name  Description
//   quint32  tag  4 -byte identifier.
//   quint32  checkSum  CheckSum for this table.
//   quint32  offset  Offset from beginning of TrueType font file.
//   quint32  length  Length of this table.
        quint32 table_offset = header_size + directory_size;
        for (int i = 0; i < tables.size(); ++i) {
            const QTtfTable &t = tables.at(i);
            const quint32 size = (t.data.size() + 3) & ~3;
            if (t.tag == MAKE_TAG('h', 'e', 'a', 'd'))
                head_offset = table_offset;
            f << t.tag
              << checksum(t.data)
              << table_offset
              << t.data.size();
            table_offset += size;
#define TAG(x) char(t.tag >> 24) << char((t.tag >> 16) & 0xff) << char((t.tag >> 8) & 0xff) << char(t.tag & 0xff)
            //qDebug() << "table " << TAG(t.tag) << "has size " << t.data.size() << "stream at " << f.offset();
        }
    }
    for (int i = 0; i < tables.size(); ++i) {
        const QByteArray &t = tables.at(i).data;
        font += t;
        int s = t.size();
        while (s & 3) { font += '\0'; ++s; }
    }

    if (!head_offset) {
        qWarning("QFontSubset: Font misses 'head' table");
        return QByteArray();
    }

    // calculate the fonts checksum and qToBigEndian into 'head's checksum_adjust
    quint32 checksum_adjust = 0xB1B0AFBA - checksum(font);
    qToBigEndian(checksum_adjust, (uchar *)font.data() + head_offset + 8);

    return font;
}


/*
  PDF requires the following tables:

  head, hhea, loca, maxp, cvt , prep, glyf, hmtx, fpgm

  This means we don't have to add a os/2, post or name table. cvt , prep and fpgm could be empty
  if really required.
*/

QByteArray QFontSubset::toTruetype() const
{
    qttf_font_tables font;
    memset(&font, 0, sizeof(qttf_font_tables));

    qreal ppem = fontEngine->fontDef.pixelSize;
#define TO_TTF(x) qRound(x * 2048. / ppem)
    QList<QTtfGlyph> glyphs;

    QFontEngine::Properties properties = fontEngine->properties();
    // initialize some stuff needed in createWidthArray
    emSquare = 2048;
    widths.resize(nGlyphs());

    // head table
    font.head.font_revision = 0x00010000;
    font.head.flags = (1 << 2) | (1 << 4);
    font.head.created = 0; // ###
    font.head.modified = 0; // ###
    font.head.xMin = SHRT_MAX;
    font.head.xMax = SHRT_MIN;
    font.head.yMin = SHRT_MAX;
    font.head.yMax = SHRT_MIN;
    font.head.macStyle = (fontEngine->fontDef.weight > QFont::Normal) ? 1 : 0;
    font.head.macStyle |= (fontEngine->fontDef.styleHint != QFont::StyleNormal) ? 1 : 0;

    // hhea table
    font.hhea.ascender = qRound(properties.ascent);
    font.hhea.descender = -qRound(properties.descent);
    font.hhea.lineGap = qRound(properties.leading);
    font.hhea.maxAdvanceWidth = TO_TTF(fontEngine->maxCharWidth());
    font.hhea.minLeftSideBearing = TO_TTF(fontEngine->minLeftBearing());
    font.hhea.minRightSideBearing = TO_TTF(fontEngine->minRightBearing());
    font.hhea.xMaxExtent = SHRT_MIN;

    font.maxp.numGlyphs = 0;
    font.maxp.maxPoints = 0;
    font.maxp.maxContours = 0;
    font.maxp.maxCompositePoints = 0;
    font.maxp.maxCompositeContours = 0;
    font.maxp.maxComponentElements = 0;
    font.maxp.maxComponentDepth = 0;
    font.maxp.numGlyphs = nGlyphs();



    uint sumAdvances = 0;
    for (int i = 0; i < nGlyphs(); ++i) {
        glyph_t g = glyph_indices.at(i);
        QPainterPath path;
        glyph_metrics_t metric;
        fontEngine->getUnscaledGlyph(g, &path, &metric);
        if (noEmbed) {
            path = QPainterPath();
            if (g == 0)
                path.addRect(QRectF(0, 0, 1000, 1000));
        }
        QTtfGlyph glyph = generateGlyph(i, path, metric.xoff.toReal(), metric.x.toReal(), properties.emSquare.toReal());

        font.head.xMin = qMin(font.head.xMin, glyph.xMin);
        font.head.xMax = qMax(font.head.xMax, glyph.xMax);
        font.head.yMin = qMin(font.head.yMin, glyph.yMin);
        font.head.yMax = qMax(font.head.yMax, glyph.yMax);

        font.hhea.xMaxExtent = qMax(font.hhea.xMaxExtent, (qint16)(glyph.lsb + glyph.xMax - glyph.xMin));

        font.maxp.maxPoints = qMax(font.maxp.maxPoints, glyph.numPoints);
        font.maxp.maxContours = qMax(font.maxp.maxContours, glyph.numContours);

        if (glyph.xMax > glyph.xMin)
            sumAdvances += glyph.xMax - glyph.xMin;

//         qDebug("adding glyph %d size=%d", glyph.index, glyph.data.size());
        glyphs.append(glyph);
        widths[i] = glyph.advanceWidth;
    }


    QList<QTtfTable> tables = generateGlyphTables(font, glyphs);
    tables.append(generateHead(font.head));
    tables.append(generateHhea(font.hhea));
    tables.append(generateMaxp(font.maxp));
    // name
    QTtfTable name_table;
    name_table.tag = MAKE_TAG('n', 'a', 'm', 'e');
    if (!noEmbed)
        name_table.data = fontEngine->getSfntTable(name_table.tag);
    if (name_table.data.isEmpty()) {
        qttf_name_table name;
        if (noEmbed)
            name.copyright = QLatin1String("Fake font");
        else
            name.copyright = QLatin1String(properties.copyright);
        name.family = fontEngine->fontDef.family;
        name.subfamily = QLatin1String("Regular"); // ######
        name.postscript_name = QLatin1String(properties.postscriptName);
        name_table = generateName(name);
    }
    tables.append(name_table);

    if (!noEmbed) {
        QTtfTable os2;
        os2.tag = MAKE_TAG('O', 'S', '/', '2');
        os2.data = fontEngine->getSfntTable(os2.tag);
        if (!os2.data.isEmpty())
            tables.append(os2);
    }

    return bindFont(tables);
}

// ------------------ Type 1 generation ---------------------------

// needs at least 6 bytes of space in tmp
static const char *encodeNumber(int num, char *tmp)
{
    const char *ret = tmp;
    if(num >= -107 && num <= 107) {
        QPdf::toHex((uchar)(num + 139), tmp);
        tmp += 2;
    } else if (num > 107 && num <= 1131) {
        num -= 108;
        QPdf::toHex((uchar)((num >> 8) + 247), tmp);
        tmp += 2;
        QPdf::toHex((uchar)(num & 0xff), tmp);
        tmp += 2;
    } else if(num < - 107 && num >= -1131) {
        num += 108;
        num = -num;
        QPdf::toHex((uchar)((num >> 8) + 251), tmp);
        tmp += 2;
        QPdf::toHex((uchar)(num & 0xff), tmp);
        tmp += 2;
    } else {
        *tmp++ = 'f';
        *tmp++ = 'f';
        QPdf::toHex((uchar)(num >> 24), tmp);
        tmp += 2;
        QPdf::toHex((uchar)(num >> 16), tmp);
        tmp += 2;
        QPdf::toHex((uchar)(num >> 8), tmp);
        tmp += 2;
        QPdf::toHex((uchar)(num >> 0), tmp);
        tmp += 2;
    }
    *tmp = 0;
//     qDebug("encodeNumber: %d -> '%s'", num, ret);
    return ret;
}

static QByteArray charString(const QPainterPath &path, qreal advance, qreal lsb, qreal ppem)
{
    // the charstring commands we need
    const char *hsbw = "0D";
    const char *closepath = "09";
    const char *moveto[3] = { "16", "04", "15" };
    const char *lineto[3] = { "06", "07", "05" };
    const char *rcurveto = "08";
    const char *endchar = "0E";

    enum { horizontal = 1,  vertical = 2 };

    char tmp[16];

    qreal factor = 1000./ppem;

    int lsb_i = qRound(lsb*factor);
    int advance_i = qRound(advance*factor);
//     qDebug("--- charstring");

    // first of all add lsb and width to the charstring using the hsbw command
    QByteArray charstring;
    charstring += encodeNumber(lsb_i, tmp);
    charstring += encodeNumber(advance_i, tmp);
    charstring += hsbw;

    // add the path
    int xl = lsb_i;
    int yl = 0;
    bool openpath = false;
    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element &elm = path.elementAt(i);
        int x = qRound(elm.x*factor);
        int y = -qRound(elm.y*factor);
        int dx = x - xl;
        int dy = y - yl;
        if (elm.type == QPainterPath::MoveToElement && openpath) {
//             qDebug("closepath %s", closepath);
            charstring += closepath;
        }
        if (elm.type == QPainterPath::MoveToElement ||
            elm.type == QPainterPath::LineToElement) {
            int type = -1;
            if (dx || !dy) {
                charstring += encodeNumber(dx, tmp);
                type += horizontal;
//                 qDebug("horizontal");
            }
            if (dy) {
                charstring += encodeNumber(dy, tmp);
                type += vertical;
//                 qDebug("vertical");
            }
//             qDebug("moveto/lineto %s", (elm.type == QPainterPath::MoveToElement ? moveto[type] : lineto[type]));
            charstring += (elm.type == QPainterPath::MoveToElement ? moveto[type] : lineto[type]);
            openpath = true;
            xl = x;
            yl = y;
        } else {
            Q_ASSERT(elm.type == QPainterPath::CurveToElement);
            const QPainterPath::Element &elm2 = path.elementAt(++i);
            const QPainterPath::Element &elm3 = path.elementAt(++i);
            int x2 = qRound(elm2.x*factor);
            int y2 = -qRound(elm2.y*factor);
            int x3 = qRound(elm3.x*factor);
            int y3 = -qRound(elm3.y*factor);
            charstring += encodeNumber(dx, tmp);
            charstring += encodeNumber(dy, tmp);
            charstring += encodeNumber(x2 - x, tmp);
            charstring += encodeNumber(y2 - y, tmp);
            charstring += encodeNumber(x3 - x2, tmp);
            charstring += encodeNumber(y3 - y2, tmp);
            charstring += rcurveto;
            openpath = true;
            xl = x3;
            yl = y3;
//             qDebug("rcurveto");
        }
    }
    if (openpath)
        charstring += closepath;
    charstring += endchar;
    if (charstring.length() > 240) {
        int pos = 240;
        while (pos < charstring.length()) {
            charstring.insert(pos, '\n');
            pos += 241;
        }
    }
    return charstring;
}

#ifndef QT_NO_FREETYPE
static const char *helvetica_styles[4] = {
    "Helvetica",
    "Helvetica-Bold",
    "Helvetica-Oblique",
    "Helvetica-BoldOblique"
};
static const char *times_styles[4] = {
    "Times-Regular",
    "Times-Bold",
    "Times-Italic",
    "Times-BoldItalic"
};
static const char *courier_styles[4] = {
    "Courier",
    "Courier-Bold",
    "Courier-Oblique",
    "Courier-BoldOblique"
};
#endif

QByteArray QFontSubset::toType1() const
{
    QFontEngine::Properties properties = fontEngine->properties();
    QVector<int> reverseMap = getReverseMap();

    QByteArray font;
    QPdf::ByteStream s(&font);

    QByteArray id = QByteArray::number(object_id);
    QByteArray psname = properties.postscriptName;
    psname.replace(' ', "");

    standard_font = false;

#ifndef QT_NO_FREETYPE
    FT_Face face = ft_face(fontEngine);
    if (face && !FT_IS_SCALABLE(face)) {
        int style = 0;
        if (fontEngine->fontDef.style)
            style += 2;
        if (fontEngine->fontDef.weight >= QFont::Bold)
            style++;
        if (fontEngine->fontDef.family.contains(QLatin1String("Helvetica"))) {
            psname = helvetica_styles[style];
            standard_font = true;
        } else if (fontEngine->fontDef.family.contains(QLatin1String("Times"))) {
            psname = times_styles[style];
            standard_font = true;
        } else if (fontEngine->fontDef.family.contains(QLatin1String("Courier"))) {
            psname = courier_styles[style];
            standard_font = true;
        }
    }
#endif
    s << "/F" << id << "-Base\n";
    if (standard_font) {
            s << '/' << psname << " findfont\n"
                "0 dict copy dup /NumGlyphs 0 put dup /CMap 256 array put def\n";
    } else {
        s << "<<\n";
        if(!psname.isEmpty())
            s << "/FontName /" << psname << '\n';
        s << "/FontInfo <</FsType " << (int)fontEngine->fsType << ">>\n"
            "/FontType 1\n"
            "/PaintType 0\n"
            "/FontMatrix [.001 0 0 .001 0 0]\n"
            "/FontBBox { 0 0 0 0 }\n"
            "/Private <<\n"
            "/password 5839\n"
            "/MinFeature {16 16}\n"
            "/BlueValues []\n"
            "/lenIV -1\n"
            ">>\n"
            "/CharStrings << >>\n"
            "/NumGlyphs 0\n"
            "/CMap 256 array\n"
            ">> def\n";
    }
    s << type1AddedGlyphs();
    downloaded_glyphs = glyph_indices.size();

    return font;
}

QByteArray QFontSubset::type1AddedGlyphs() const
{
    if (downloaded_glyphs == glyph_indices.size())
        return QByteArray();

    QFontEngine::Properties properties = fontEngine->properties();
    QVector<int> reverseMap = getReverseMap();
    QByteArray glyphs;
    QPdf::ByteStream s(&glyphs);

    int nGlyphs = glyph_indices.size();
    QByteArray id = QByteArray::number(object_id);

    s << 'F' << id << "-Base [\n";
    for (int i = downloaded_glyphs; i < nGlyphs; ++i) {
        glyph_t g = glyph_indices.at(i);
        QPainterPath path;
        glyph_metrics_t metric;
        fontEngine->getUnscaledGlyph(g, &path, &metric);
        QByteArray charstring = charString(path, metric.xoff.toReal(), metric.x.toReal(),
                                             properties.emSquare.toReal());
        s << glyphName(i, reverseMap);
        if (!standard_font)
          s << "\n<" << charstring << ">\n";
    }
    s << (standard_font ? "] T1AddMapping\n" : "] T1AddGlyphs\n");
    return glyphs;
}

QT_END_NAMESPACE

#endif // QT_NO_PRINTER
