/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/qtestcase.h>
#include <QtTest/qtestassert.h>

QT_BEGIN_NAMESPACE

/*! \internal
    Convert an ascii char key value to a Qt Key value.
    If the key is unknown a 0 is returned.

    Note: this may happen more than you like since not all known
    ascii keys _are_ converted already. So feel free to add all the keys you need.
  */
Qt::Key QTest::asciiToKey(char ascii)
{
    switch ((unsigned char)ascii) {
    case 0x08: return Qt::Key_Backspace;
    case 0x09: return Qt::Key_Tab;
    case 0x0b: return Qt::Key_Backtab;
    case 0x0d: return Qt::Key_Return;
    case 0x1b: return Qt::Key_Escape;
    case 0x13: return Qt::Key_Up;
    case 0x15: return Qt::Key_Down;
    case 0x20: return Qt::Key_Space;
    case 0x21: return Qt::Key_Exclam;
    case 0x22: return Qt::Key_QuoteDbl;
    case 0x23: return Qt::Key_NumberSign;
    case 0x24: return Qt::Key_Dollar;
    case 0x25: return Qt::Key_Percent;
    case 0x26: return Qt::Key_Ampersand;
    case 0x27: return Qt::Key_Apostrophe;
    case 0x28: return Qt::Key_ParenLeft;
    case 0x29: return Qt::Key_ParenRight;
    case 0x2a: return Qt::Key_Asterisk;
    case 0x2b: return Qt::Key_Plus;
    case 0x2c: return Qt::Key_Comma;
    case 0x2d: return Qt::Key_Minus;
    case 0x2e: return Qt::Key_Period;
    case 0x2f: return Qt::Key_Slash;
    case 0x30: return Qt::Key_0;
    case 0x31: return Qt::Key_1;
    case 0x32: return Qt::Key_2;
    case 0x33: return Qt::Key_3;
    case 0x34: return Qt::Key_4;
    case 0x35: return Qt::Key_5;
    case 0x36: return Qt::Key_6;
    case 0x37: return Qt::Key_7;
    case 0x38: return Qt::Key_8;
    case 0x39: return Qt::Key_9;
    case 0x3a: return Qt::Key_Colon;
    case 0x3b: return Qt::Key_Semicolon;
    case 0x3c: return Qt::Key_Less;
    case 0x3d: return Qt::Key_Equal;
    case 0x3e: return Qt::Key_Greater;
    case 0x3f: return Qt::Key_Question;
    case 0x40: return Qt::Key_At;
    case 0x41: return Qt::Key_A;
    case 0x42: return Qt::Key_B;
    case 0x43: return Qt::Key_C;
    case 0x44: return Qt::Key_D;
    case 0x45: return Qt::Key_E;
    case 0x46: return Qt::Key_F;
    case 0x47: return Qt::Key_G;
    case 0x48: return Qt::Key_H;
    case 0x49: return Qt::Key_I;
    case 0x4a: return Qt::Key_J;
    case 0x4b: return Qt::Key_K;
    case 0x4c: return Qt::Key_L;
    case 0x4d: return Qt::Key_M;
    case 0x4e: return Qt::Key_N;
    case 0x4f: return Qt::Key_O;
    case 0x50: return Qt::Key_P;
    case 0x51: return Qt::Key_Q;
    case 0x52: return Qt::Key_R;
    case 0x53: return Qt::Key_S;
    case 0x54: return Qt::Key_T;
    case 0x55: return Qt::Key_U;
    case 0x56: return Qt::Key_V;
    case 0x57: return Qt::Key_W;
    case 0x58: return Qt::Key_X;
    case 0x59: return Qt::Key_Y;
    case 0x5a: return Qt::Key_Z;
    case 0x5b: return Qt::Key_BracketLeft;
    case 0x5c: return Qt::Key_Backslash;
    case 0x5d: return Qt::Key_BracketRight;
    case 0x5e: return Qt::Key_AsciiCircum;
    case 0x5f: return Qt::Key_Underscore;
    case 0x60: return Qt::Key_QuoteLeft;
    case 0x61: return Qt::Key_A;
    case 0x62: return Qt::Key_B;
    case 0x63: return Qt::Key_C;
    case 0x64: return Qt::Key_D;
    case 0x65: return Qt::Key_E;
    case 0x66: return Qt::Key_F;
    case 0x67: return Qt::Key_G;
    case 0x68: return Qt::Key_H;
    case 0x69: return Qt::Key_I;
    case 0x6a: return Qt::Key_J;
    case 0x6b: return Qt::Key_K;
    case 0x6c: return Qt::Key_L;
    case 0x6d: return Qt::Key_M;
    case 0x6e: return Qt::Key_N;
    case 0x6f: return Qt::Key_O;
    case 0x70: return Qt::Key_P;
    case 0x71: return Qt::Key_Q;
    case 0x72: return Qt::Key_R;
    case 0x73: return Qt::Key_S;
    case 0x74: return Qt::Key_T;
    case 0x75: return Qt::Key_U;
    case 0x76: return Qt::Key_V;
    case 0x77: return Qt::Key_W;
    case 0x78: return Qt::Key_X;
    case 0x79: return Qt::Key_Y;
    case 0x7a: return Qt::Key_Z;
    case 0x7b: return Qt::Key_BraceLeft;
    case 0x7c: return Qt::Key_Bar;
    case 0x7d: return Qt::Key_BraceRight;
    case 0x7e: return Qt::Key_AsciiTilde;

    // Latin 1 codes adapted from X: keysymdef.h,v 1.21 94/08/28 16:17:06
    case 0xa0: return Qt::Key_nobreakspace;
    case 0xa1: return Qt::Key_exclamdown;
    case 0xa2: return Qt::Key_cent;
    case 0xa3: return Qt::Key_sterling;
    case 0xa4: return Qt::Key_currency;
    case 0xa5: return Qt::Key_yen;
    case 0xa6: return Qt::Key_brokenbar;
    case 0xa7: return Qt::Key_section;
    case 0xa8: return Qt::Key_diaeresis;
    case 0xa9: return Qt::Key_copyright;
    case 0xaa: return Qt::Key_ordfeminine;
    case 0xab: return Qt::Key_guillemotleft;
    case 0xac: return Qt::Key_notsign;
    case 0xad: return Qt::Key_hyphen;
    case 0xae: return Qt::Key_registered;
    case 0xaf: return Qt::Key_macron;
    case 0xb0: return Qt::Key_degree;
    case 0xb1: return Qt::Key_plusminus;
    case 0xb2: return Qt::Key_twosuperior;
    case 0xb3: return Qt::Key_threesuperior;
    case 0xb4: return Qt::Key_acute;
    case 0xb5: return Qt::Key_mu;
    case 0xb6: return Qt::Key_paragraph;
    case 0xb7: return Qt::Key_periodcentered;
    case 0xb8: return Qt::Key_cedilla;
    case 0xb9: return Qt::Key_onesuperior;
    case 0xba: return Qt::Key_masculine;
    case 0xbb: return Qt::Key_guillemotright;
    case 0xbc: return Qt::Key_onequarter;
    case 0xbd: return Qt::Key_onehalf;
    case 0xbe: return Qt::Key_threequarters;
    case 0xbf: return Qt::Key_questiondown;
    case 0xc0: return Qt::Key_Agrave;
    case 0xc1: return Qt::Key_Aacute;
    case 0xc2: return Qt::Key_Acircumflex;
    case 0xc3: return Qt::Key_Atilde;
    case 0xc4: return Qt::Key_Adiaeresis;
    case 0xc5: return Qt::Key_Aring;
    case 0xc6: return Qt::Key_AE;
    case 0xc7: return Qt::Key_Ccedilla;
    case 0xc8: return Qt::Key_Egrave;
    case 0xc9: return Qt::Key_Eacute;
    case 0xca: return Qt::Key_Ecircumflex;
    case 0xcb: return Qt::Key_Ediaeresis;
    case 0xcc: return Qt::Key_Igrave;
    case 0xcd: return Qt::Key_Iacute;
    case 0xce: return Qt::Key_Icircumflex;
    case 0xcf: return Qt::Key_Idiaeresis;
    case 0xd0: return Qt::Key_ETH;
    case 0xd1: return Qt::Key_Ntilde;
    case 0xd2: return Qt::Key_Ograve;
    case 0xd3: return Qt::Key_Oacute;
    case 0xd4: return Qt::Key_Ocircumflex;
    case 0xd5: return Qt::Key_Otilde;
    case 0xd6: return Qt::Key_Odiaeresis;
    case 0xd7: return Qt::Key_multiply;
    case 0xd8: return Qt::Key_Ooblique;
    case 0xd9: return Qt::Key_Ugrave;
    case 0xda: return Qt::Key_Uacute;
    case 0xdb: return Qt::Key_Ucircumflex;
    case 0xdc: return Qt::Key_Udiaeresis;
    case 0xdd: return Qt::Key_Yacute;
    case 0xde: return Qt::Key_THORN;
    case 0xdf: return Qt::Key_ssharp;
    case 0xe5: return Qt::Key_Aring;
    case 0xe6: return Qt::Key_AE;
    case 0xf7: return Qt::Key_division;
    case 0xf8: return Qt::Key_Ooblique;
    case 0xff: return Qt::Key_ydiaeresis;
    default: QTEST_ASSERT(false); return Qt::Key(0);
    }
}

/*! \internal
    Convert a Qt Key to an ascii char value.
    If the Qt key is unknown a 0 is returned.

    Note: this may happen more than you like since not all known
    Qt keys _are_ converted already. So feel free to add all the keys you need.
*/
char QTest::keyToAscii(Qt::Key key)
{
    switch (key) {
    case Qt::Key_Backspace: return 0x8; //BS
    case Qt::Key_Tab: return 0x09; // HT
    case Qt::Key_Backtab: return 0x0b; // VT
    case Qt::Key_Enter:
    case Qt::Key_Return: return 0x0d; // CR
    case Qt::Key_Escape: return 0x1b; // ESC
    case Qt::Key_Space: return 0x20;        // 7 bit printable ASCII
    case Qt::Key_Exclam: return 0x21;
    case Qt::Key_QuoteDbl: return 0x22;
    case Qt::Key_NumberSign: return 0x23;
    case Qt::Key_Dollar: return 0x24;
    case Qt::Key_Percent: return 0x25;
    case Qt::Key_Ampersand: return 0x26;
    case Qt::Key_Apostrophe: return 0x27;
    case Qt::Key_ParenLeft: return 0x28;
    case Qt::Key_ParenRight: return 0x29;
    case Qt::Key_Asterisk: return 0x2a;
    case Qt::Key_Plus: return 0x2b;
    case Qt::Key_Comma: return 0x2c;
    case Qt::Key_Minus: return 0x2d;
    case Qt::Key_Period: return 0x2e;
    case Qt::Key_Slash: return 0x2f;
    case Qt::Key_0: return 0x30;
    case Qt::Key_1: return 0x31;
    case Qt::Key_2: return 0x32;
    case Qt::Key_3: return 0x33;
    case Qt::Key_4: return 0x34;
    case Qt::Key_5: return 0x35;
    case Qt::Key_6: return 0x36;
    case Qt::Key_7: return 0x37;
    case Qt::Key_8: return 0x38;
    case Qt::Key_9: return 0x39;
    case Qt::Key_Colon: return 0x3a;
    case Qt::Key_Semicolon: return 0x3b;
    case Qt::Key_Less: return 0x3c;
    case Qt::Key_Equal: return 0x3d;
    case Qt::Key_Greater: return 0x3e;
    case Qt::Key_Question: return 0x3f;
    case Qt::Key_At: return 0x40;
    case Qt::Key_A: return 0x61; // 0x41 == 'A', 0x61 == 'a'
    case Qt::Key_B: return 0x62;
    case Qt::Key_C: return 0x63;
    case Qt::Key_D: return 0x64;
    case Qt::Key_E: return 0x65;
    case Qt::Key_F: return 0x66;
    case Qt::Key_G: return 0x67;
    case Qt::Key_H: return 0x68;
    case Qt::Key_I: return 0x69;
    case Qt::Key_J: return 0x6a;
    case Qt::Key_K: return 0x6b;
    case Qt::Key_L: return 0x6c;
    case Qt::Key_M: return 0x6d;
    case Qt::Key_N: return 0x6e;
    case Qt::Key_O: return 0x6f;
    case Qt::Key_P: return 0x70;
    case Qt::Key_Q: return 0x71;
    case Qt::Key_R: return 0x72;
    case Qt::Key_S: return 0x73;
    case Qt::Key_T: return 0x74;
    case Qt::Key_U: return 0x75;
    case Qt::Key_V: return 0x76;
    case Qt::Key_W: return 0x77;
    case Qt::Key_X: return 0x78;
    case Qt::Key_Y: return 0x79;
    case Qt::Key_Z: return 0x7a;
    case Qt::Key_BracketLeft: return 0x5b;
    case Qt::Key_Backslash: return 0x5c;
    case Qt::Key_BracketRight: return 0x5d;
    case Qt::Key_AsciiCircum: return 0x5e;
    case Qt::Key_Underscore: return 0x5f;
    case Qt::Key_QuoteLeft: return 0x60;

    case Qt::Key_BraceLeft: return 0x7b;
    case Qt::Key_Bar: return 0x7c;
    case Qt::Key_BraceRight: return 0x7d;
    case Qt::Key_AsciiTilde: return 0x7e;

    case Qt::Key_Delete: return 0;
    case Qt::Key_Insert: return 0; // = 0x1006,
    case Qt::Key_Pause: return 0; // = 0x1008,
    case Qt::Key_Print: return 0; // = 0x1009,
    case Qt::Key_SysReq: return 0; // = 0x100a,

    case Qt::Key_Clear: return 0; // = 0x100b,

    case Qt::Key_Home: return 0; // = 0x1010,        // cursor movement
    case Qt::Key_End: return 0; // = 0x1011,
    case Qt::Key_Left: return 0; // = 0x1012,
    case Qt::Key_Up: return 0; // = 0x1013,
    case Qt::Key_Right: return 0; // = 0x1014,
    case Qt::Key_Down: return 0; // = 0x1015,
    case Qt::Key_PageUp: return 0; // = 0x1016,
    case Qt::Key_PageDown: return 0; // = 0x1017,
    case Qt::Key_Shift: return 0; // = 0x1020,        // modifiers
    case Qt::Key_Control: return 0; // = 0x1021,
    case Qt::Key_Meta: return 0; // = 0x1022,
    case Qt::Key_Alt: return 0; // = 0x1023,
    case Qt::Key_CapsLock: return 0; // = 0x1024,
    case Qt::Key_NumLock: return 0; // = 0x1025,
    case Qt::Key_ScrollLock: return 0; // = 0x1026,
    case Qt::Key_F1: return 0; // = 0x1030,        // function keys
    case Qt::Key_F2: return 0; // = 0x1031,
    case Qt::Key_F3: return 0; // = 0x1032,
    case Qt::Key_F4: return 0; // = 0x1033,
    case Qt::Key_F5: return 0; // = 0x1034,
    case Qt::Key_F6: return 0; // = 0x1035,
    case Qt::Key_F7: return 0; // = 0x1036,
    case Qt::Key_F8: return 0; // = 0x1037,
    case Qt::Key_F9: return 0; // = 0x1038,
    case Qt::Key_F10: return 0; // = 0x1039,
    case Qt::Key_F11: return 0; // = 0x103a,
    case Qt::Key_F12: return 0; // = 0x103b,
    case Qt::Key_F13: return 0; // = 0x103c,
    case Qt::Key_F14: return 0; // = 0x103d,
    case Qt::Key_F15: return 0; // = 0x103e,
    case Qt::Key_F16: return 0; // = 0x103f,
    case Qt::Key_F17: return 0; // = 0x1040,
    case Qt::Key_F18: return 0; // = 0x1041,
    case Qt::Key_F19: return 0; // = 0x1042,
    case Qt::Key_F20: return 0; // = 0x1043,
    case Qt::Key_F21: return 0; // = 0x1044,
    case Qt::Key_F22: return 0; // = 0x1045,
    case Qt::Key_F23: return 0; // = 0x1046,
    case Qt::Key_F24: return 0; // = 0x1047,
    case Qt::Key_F25: return 0; // = 0x1048,        // F25 .. F35 only on X11
    case Qt::Key_F26: return 0; // = 0x1049,
    case Qt::Key_F27: return 0; // = 0x104a,
    case Qt::Key_F28: return 0; // = 0x104b,
    case Qt::Key_F29: return 0; // = 0x104c,
    case Qt::Key_F30: return 0; // = 0x104d,
    case Qt::Key_F31: return 0; // = 0x104e,
    case Qt::Key_F32: return 0; // = 0x104f,
    case Qt::Key_F33: return 0; // = 0x1050,
    case Qt::Key_F34: return 0; // = 0x1051,
    case Qt::Key_F35: return 0; // = 0x1052,
    case Qt::Key_Super_L: return 0; // = 0x1053,        // extra keys
    case Qt::Key_Super_R: return 0; // = 0x1054,
    case Qt::Key_Menu: return 0; // = 0x1055,
    case Qt::Key_Hyper_L: return 0; // = 0x1056,
    case Qt::Key_Hyper_R: return 0; // = 0x1057,
    case Qt::Key_Help: return 0; // = 0x1058,
    case Qt::Key_Direction_L: return 0; // = 0x1059,
    case Qt::Key_Direction_R: return 0; // = 0x1060,

    // Latin 1 codes adapted from X: keysymdef.h,v 1.21 94/08/28 16:17:06
    case Qt::Key_nobreakspace: return char(0xa0);
    case Qt::Key_exclamdown: return char(0xa1);
    case Qt::Key_cent: return char(0xa2);
    case Qt::Key_sterling: return char(0xa3);
    case Qt::Key_currency: return char(0xa4);
    case Qt::Key_yen: return char(0xa5);
    case Qt::Key_brokenbar: return char(0xa6);
    case Qt::Key_section: return char(0xa7);
    case Qt::Key_diaeresis: return char(0xa8);
    case Qt::Key_copyright: return char(0xa9);
    case Qt::Key_ordfeminine: return char(0xaa);
    case Qt::Key_guillemotleft: return char(0xab); // left angle quotation mar
    case Qt::Key_notsign: return char(0xac);
    case Qt::Key_hyphen: return char(0xad);
    case Qt::Key_registered: return char(0xae);
    case Qt::Key_macron: return char(0xaf);
    case Qt::Key_degree: return char(0xb0);
    case Qt::Key_plusminus: return char(0xb1);
    case Qt::Key_twosuperior: return char(0xb2);
    case Qt::Key_threesuperior: return char(0xb3);
    case Qt::Key_acute: return char(0xb4);
    case Qt::Key_mu: return char(0xb5);
    case Qt::Key_paragraph: return char(0xb6);
    case Qt::Key_periodcentered: return char(0xb7);
    case Qt::Key_cedilla: return char(0xb8);
    case Qt::Key_onesuperior: return char(0xb9);
    case Qt::Key_masculine: return char(0xba);
    case Qt::Key_guillemotright: return char(0xbb); // right angle quotation mar
    case Qt::Key_onequarter: return char(0xbc);
    case Qt::Key_onehalf: return char(0xbd);
    case Qt::Key_threequarters: return char(0xbe);
    case Qt::Key_questiondown: return char(0xbf);
    case Qt::Key_Agrave: return char(0xc0);
    case Qt::Key_Aacute: return char(0xc1);
    case Qt::Key_Acircumflex: return char(0xc2);
    case Qt::Key_Atilde: return char(0xc3);
    case Qt::Key_Adiaeresis: return char(0xc4);
    case Qt::Key_Aring: return char(0xe5);
    case Qt::Key_AE: return char(0xe6);
    case Qt::Key_Ccedilla: return char(0xc7);
    case Qt::Key_Egrave: return char(0xc8);
    case Qt::Key_Eacute: return char(0xc9);
    case Qt::Key_Ecircumflex: return char(0xca);
    case Qt::Key_Ediaeresis: return char(0xcb);
    case Qt::Key_Igrave: return char(0xcc);
    case Qt::Key_Iacute: return char(0xcd);
    case Qt::Key_Icircumflex: return char(0xce);
    case Qt::Key_Idiaeresis: return char(0xcf);
    case Qt::Key_ETH: return char(0xd0);
    case Qt::Key_Ntilde: return char(0xd1);
    case Qt::Key_Ograve: return char(0xd2);
    case Qt::Key_Oacute: return char(0xd3);
    case Qt::Key_Ocircumflex: return char(0xd4);
    case Qt::Key_Otilde: return char(0xd5);
    case Qt::Key_Odiaeresis: return char(0xd6);
    case Qt::Key_multiply: return char(0xd7);
    case Qt::Key_Ooblique: return char(0xf8);
    case Qt::Key_Ugrave: return char(0xd9);
    case Qt::Key_Uacute: return char(0xda);
    case Qt::Key_Ucircumflex: return char(0xdb);
    case Qt::Key_Udiaeresis: return char(0xdc);
    case Qt::Key_Yacute: return char(0xdd);
    case Qt::Key_THORN: return char(0xde);
    case Qt::Key_ssharp: return char(0xdf);
    case Qt::Key_division: return char(0xf7);
    case Qt::Key_ydiaeresis: return char(0xff);

    // multimedia/internet keys - ignored by default - see QKeyEvent c'tor

    case Qt::Key_Back : return 0; // = 0x1061,
    case Qt::Key_Forward : return 0; // = 0x1062,
    case Qt::Key_Stop : return 0; // = 0x1063,
    case Qt::Key_Refresh : return 0; // = 0x1064,

    case Qt::Key_VolumeDown: return 0; // = 0x1070,
    case Qt::Key_VolumeMute : return 0; // = 0x1071,
    case Qt::Key_VolumeUp: return 0; // = 0x1072,
    case Qt::Key_BassBoost: return 0; // = 0x1073,
    case Qt::Key_BassUp: return 0; // = 0x1074,
    case Qt::Key_BassDown: return 0; // = 0x1075,
    case Qt::Key_TrebleUp: return 0; // = 0x1076,
    case Qt::Key_TrebleDown: return 0; // = 0x1077,

    case Qt::Key_MediaPlay : return 0; // = 0x1080,
    case Qt::Key_MediaStop : return 0; // = 0x1081,
    case Qt::Key_MediaPrevious : return 0; // = 0x1082,
    case Qt::Key_MediaNext : return 0; // = 0x1083,
    case Qt::Key_MediaRecord: return 0; // = 0x1084,

    case Qt::Key_HomePage : return 0; // = 0x1090,
    case Qt::Key_Favorites : return 0; // = 0x1091,
    case Qt::Key_Search : return 0; // = 0x1092,
    case Qt::Key_Standby: return 0; // = 0x1093,
    case Qt::Key_OpenUrl: return 0; // = 0x1094,

    case Qt::Key_LaunchMail : return 0; // = 0x10a0,
    case Qt::Key_LaunchMedia: return 0; // = 0x10a1,
    case Qt::Key_Launch0 : return 0; // = 0x10a2,
    case Qt::Key_Launch1 : return 0; // = 0x10a3,
    case Qt::Key_Launch2 : return 0; // = 0x10a4,
    case Qt::Key_Launch3 : return 0; // = 0x10a5,
    case Qt::Key_Launch4 : return 0; // = 0x10a6,
    case Qt::Key_Launch5 : return 0; // = 0x10a7,
    case Qt::Key_Launch6 : return 0; // = 0x10a8,
    case Qt::Key_Launch7 : return 0; // = 0x10a9,
    case Qt::Key_Launch8 : return 0; // = 0x10aa,
    case Qt::Key_Launch9 : return 0; // = 0x10ab,
    case Qt::Key_LaunchA : return 0; // = 0x10ac,
    case Qt::Key_LaunchB : return 0; // = 0x10ad,
    case Qt::Key_LaunchC : return 0; // = 0x10ae,
    case Qt::Key_LaunchD : return 0; // = 0x10af,
    case Qt::Key_LaunchE : return 0; // = 0x10b0,
    case Qt::Key_LaunchF : return 0; // = 0x10b1,

    default: QTEST_ASSERT(false); return 0;
    }
}

QT_END_NAMESPACE
