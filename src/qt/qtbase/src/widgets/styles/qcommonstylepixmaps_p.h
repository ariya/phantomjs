/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include <QtCore/qglobal.h>

#ifndef QT_NO_IMAGEFORMAT_XPM

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

static const char * const check_list_controller_xpm[] = {
"16 16 4 1",
"        c None",
".        c #000000000000",
"X        c #FFFFFFFF0000",
"o        c #C71BC30BC71B",
"                ",
"                ",
" ..........     ",
" .XXXXXXXX.     ",
" .XXXXXXXX.oo   ",
" .XXXXXXXX.oo   ",
" .XXXXXXXX.oo   ",
" .XXXXXXXX.oo   ",
" .XXXXXXXX.oo   ",
" .XXXXXXXX.oo   ",
" .XXXXXXXX.oo   ",
" ..........oo   ",
"   oooooooooo   ",
"   oooooooooo   ",
"                ",
"                "};


static const char * const tb_extension_arrow_v_xpm[] = {
    "5 8 3 1",
    "            c None",
    ".            c #000000",
    "+            c none",
    ".+++.",
    "..+..",
    "+...+",
    "++.++",
    ".+++.",
    "..+..",
    "+...+",
    "++.++"
};

static const char * const tb_extension_arrow_h_xpm[] = {
    "8 5 3 1",
    "            c None",
    ".            c #000000",
    "+            c none",
    "..++..++",
    "+..++..+",
    "++..++..",
    "+..++..+",
    "..++..++",
};

static const char * const filedialog_start_xpm[]={
    "16 15 8 1",
    "a c #cec6bd",
    "# c #000000",
    "e c #ffff00",
    "b c #999999",
    "f c #cccccc",
    "d c #dcdcdc",
    "c c #ffffff",
    ". c None",
    ".....######aaaaa",
    "...bb#cccc##aaaa",
    "..bcc#cccc#d#aaa",
    ".bcef#cccc#dd#aa",
    ".bcfe#cccc#####a",
    ".bcef#ccccccccc#",
    "bbbbbbbbbbbbccc#",
    "bccccccccccbbcc#",
    "bcefefefefee#bc#",
    ".bcefefefefef#c#",
    ".bcfefefefefe#c#",
    "..bcfefefefeeb##",
    "..bbbbbbbbbbbbb#",
    "...#############",
    "................"};

static const char * const filedialog_end_xpm[]={
    "16 15 9 1",
    "d c #a0a0a0",
    "c c #c3c3c3",
    "# c #cec6bd",
    ". c #000000",
    "f c #ffff00",
    "e c #999999",
    "g c #cccccc",
    "b c #ffffff",
    "a c None",
    "......####aaaaaa",
    ".bbbb..###aaaaaa",
    ".bbbb.c.##aaaaaa",
    ".bbbb....ddeeeea",
    ".bbbbbbb.bbbbbe.",
    ".bbbbbbb.bcfgfe.",
    "eeeeeeeeeeeeefe.",
    "ebbbbbbbbbbeege.",
    "ebfgfgfgfgff.ee.",
    "aebfgfgfgfgfg.e.",
    "aebgfgfgfgfgf.e.",
    "aaebgfgfgfgffe..",
    "aaeeeeeeeeeeeee.",
    "aaa.............",
    "aaaaaaaaaaaaaaaa"};


/* XPM */
static const char * const qt_menu_xpm[] = {
"16 16 72 1",
"  c None",
". c #65AF36",
"+ c #66B036",
"@ c #77B94C",
"# c #A7D28C",
"$ c #BADBA4",
"% c #A4D088",
"& c #72B646",
"* c #9ACB7A",
"= c #7FBD56",
"- c #85C05F",
"; c #F4F9F0",
"> c #FFFFFF",
", c #E5F1DC",
"' c #ECF5E7",
") c #7ABA50",
"! c #83BF5C",
"~ c #AED595",
"{ c #D7EACA",
"] c #A9D28D",
"^ c #BCDDA8",
"/ c #C4E0B1",
"( c #81BE59",
"_ c #D0E7C2",
": c #D4E9C6",
"< c #6FB542",
"[ c #6EB440",
"} c #88C162",
"| c #98CA78",
"1 c #F4F9F1",
"2 c #8FC56C",
"3 c #F1F8EC",
"4 c #E8F3E1",
"5 c #D4E9C7",
"6 c #74B748",
"7 c #80BE59",
"8 c #73B747",
"9 c #6DB43F",
"0 c #CBE4BA",
"a c #80BD58",
"b c #6DB33F",
"c c #FEFFFE",
"d c #68B138",
"e c #F9FCF7",
"f c #91C66F",
"g c #E8F3E0",
"h c #DCEDD0",
"i c #91C66E",
"j c #A3CF86",
"k c #C9E3B8",
"l c #B0D697",
"m c #E3F0DA",
"n c #95C873",
"o c #E6F2DE",
"p c #9ECD80",
"q c #BEDEAA",
"r c #C7E2B6",
"s c #79BA4F",
"t c #6EB441",
"u c #BCDCA7",
"v c #FAFCF8",
"w c #F6FAF3",
"x c #84BF5D",
"y c #EDF6E7",
"z c #FAFDF9",
"A c #88C263",
"B c #98CA77",
"C c #CDE5BE",
"D c #67B037",
"E c #D9EBCD",
"F c #6AB23C",
"G c #77B94D",
" .++++++++++++++",
".+++++++++++++++",
"+++@#$%&+++*=+++",
"++-;>,>')+!>~+++",
"++{>]+^>/(_>:~<+",
"+[>>}+|>123>456+",
"+7>>8+->>90>~+++",
"+a>>b+a>c[0>~+++",
"+de>=+f>g+0>~+++",
"++h>i+j>k+0>~+++",
"++l>mno>p+q>rst+",
"++duv>wl++xy>zA+",
"++++B>Cb++++&D++",
"+++++0zE++++++++",
"++++++FG+++++++.",
"++++++++++++++. "};

static const char * const qt_close_xpm[] = {
"10 10 2 1",
"# c #000000",
". c None",
"..........",
".##....##.",
"..##..##..",
"...####...",
"....##....",
"...####...",
"..##..##..",
".##....##.",
"..........",
".........."};

static const char * const qt_maximize_xpm[]={
"10 10 2 1",
"# c #000000",
". c None",
"#########.",
"#########.",
"#.......#.",
"#.......#.",
"#.......#.",
"#.......#.",
"#.......#.",
"#.......#.",
"#########.",
".........."};

static const char * const qt_minimize_xpm[] = {
"10 10 2 1",
"# c #000000",
". c None",
"..........",
"..........",
"..........",
"..........",
"..........",
"..........",
"..........",
".#######..",
".#######..",
".........."};

static const char * const qt_normalizeup_xpm[] = {
"10 10 2 1",
"# c #000000",
". c None",
"...######.",
"...######.",
"...#....#.",
".######.#.",
".######.#.",
".#....###.",
".#....#...",
".#....#...",
".######...",
".........."};

static const char * const qt_help_xpm[] = {
"10 10 2 1",
". c None",
"# c #000000",
"..........",
"..######..",
".##....##.",
"......##..",
".....##...",
"....##....",
"....##....",
"..........",
"....##....",
".........."};

static const char * const qt_shade_xpm[] = {
"10 10 2 1",
"# c #000000",
". c None",
"..........",
"..........",
"..........",
"..........",
"....#.....",
"...###....",
"..#####...",
".#######..",
"..........",
".........."};

static const char * const qt_unshade_xpm[] = {
"10 10 2 1",
"# c #000000",
". c None",
"..........",
"..........",
"..........",
".#######..",
"..#####...",
"...###....",
"....#.....",
"..........",
"..........",
".........."};

static const char * dock_widget_close_xpm[] = {
"8 8 2 1",
"# c #000000",
". c None",
"........",
".##..##.",
"..####..",
"...##...",
"..####..",
".##..##.",
"........",
"........"};

/* XPM */
static const char * const information_xpm[]={
"32 32 5 1",
". c None",
"c c #000000",
"* c #999999",
"a c #ffffff",
"b c #0000ff",
"...........********.............",
"........***aaaaaaaa***..........",
"......**aaaaaaaaaaaaaa**........",
".....*aaaaaaaaaaaaaaaaaa*.......",
"....*aaaaaaaabbbbaaaaaaaac......",
"...*aaaaaaaabbbbbbaaaaaaaac.....",
"..*aaaaaaaaabbbbbbaaaaaaaaac....",
".*aaaaaaaaaaabbbbaaaaaaaaaaac...",
".*aaaaaaaaaaaaaaaaaaaaaaaaaac*..",
"*aaaaaaaaaaaaaaaaaaaaaaaaaaaac*.",
"*aaaaaaaaaabbbbbbbaaaaaaaaaaac*.",
"*aaaaaaaaaaaabbbbbaaaaaaaaaaac**",
"*aaaaaaaaaaaabbbbbaaaaaaaaaaac**",
"*aaaaaaaaaaaabbbbbaaaaaaaaaaac**",
"*aaaaaaaaaaaabbbbbaaaaaaaaaaac**",
"*aaaaaaaaaaaabbbbbaaaaaaaaaaac**",
".*aaaaaaaaaaabbbbbaaaaaaaaaac***",
".*aaaaaaaaaaabbbbbaaaaaaaaaac***",
"..*aaaaaaaaaabbbbbaaaaaaaaac***.",
"...caaaaaaabbbbbbbbbaaaaaac****.",
"....caaaaaaaaaaaaaaaaaaaac****..",
".....caaaaaaaaaaaaaaaaaac****...",
"......ccaaaaaaaaaaaaaacc****....",
".......*cccaaaaaaaaccc*****.....",
"........***cccaaaac*******......",
"..........****caaac*****........",
".............*caaac**...........",
"...............caac**...........",
"................cac**...........",
".................cc**...........",
"..................***...........",
"...................**..........."};
/* XPM */
static const char* const warning_xpm[]={
"32 32 4 1",
". c None",
"a c #ffff00",
"* c #000000",
"b c #999999",
".............***................",
"............*aaa*...............",
"...........*aaaaa*b.............",
"...........*aaaaa*bb............",
"..........*aaaaaaa*bb...........",
"..........*aaaaaaa*bb...........",
".........*aaaaaaaaa*bb..........",
".........*aaaaaaaaa*bb..........",
"........*aaaaaaaaaaa*bb.........",
"........*aaaa***aaaa*bb.........",
".......*aaaa*****aaaa*bb........",
".......*aaaa*****aaaa*bb........",
"......*aaaaa*****aaaaa*bb.......",
"......*aaaaa*****aaaaa*bb.......",
".....*aaaaaa*****aaaaaa*bb......",
".....*aaaaaa*****aaaaaa*bb......",
"....*aaaaaaaa***aaaaaaaa*bb.....",
"....*aaaaaaaa***aaaaaaaa*bb.....",
"...*aaaaaaaaa***aaaaaaaaa*bb....",
"...*aaaaaaaaaa*aaaaaaaaaa*bb....",
"..*aaaaaaaaaaa*aaaaaaaaaaa*bb...",
"..*aaaaaaaaaaaaaaaaaaaaaaa*bb...",
".*aaaaaaaaaaaa**aaaaaaaaaaa*bb..",
".*aaaaaaaaaaa****aaaaaaaaaa*bb..",
"*aaaaaaaaaaaa****aaaaaaaaaaa*bb.",
"*aaaaaaaaaaaaa**aaaaaaaaaaaa*bb.",
"*aaaaaaaaaaaaaaaaaaaaaaaaaaa*bbb",
"*aaaaaaaaaaaaaaaaaaaaaaaaaaa*bbb",
".*aaaaaaaaaaaaaaaaaaaaaaaaa*bbbb",
"..*************************bbbbb",
"....bbbbbbbbbbbbbbbbbbbbbbbbbbb.",
".....bbbbbbbbbbbbbbbbbbbbbbbbb.."};
/* XPM */
static const char* const critical_xpm[]={
"32 32 4 1",
". c None",
"a c #999999",
"* c #ff0000",
"b c #ffffff",
"...........********.............",
".........************...........",
".......****************.........",
"......******************........",
".....********************a......",
"....**********************a.....",
"...************************a....",
"..*******b**********b*******a...",
"..******bbb********bbb******a...",
".******bbbbb******bbbbb******a..",
".*******bbbbb****bbbbb*******a..",
"*********bbbbb**bbbbb*********a.",
"**********bbbbbbbbbb**********a.",
"***********bbbbbbbb***********aa",
"************bbbbbb************aa",
"************bbbbbb************aa",
"***********bbbbbbbb***********aa",
"**********bbbbbbbbbb**********aa",
"*********bbbbb**bbbbb*********aa",
".*******bbbbb****bbbbb*******aa.",
".******bbbbb******bbbbb******aa.",
"..******bbb********bbb******aaa.",
"..*******b**********b*******aa..",
"...************************aaa..",
"....**********************aaa...",
"....a********************aaa....",
".....a******************aaa.....",
"......a****************aaa......",
".......aa************aaaa.......",
".........aa********aaaaa........",
"...........aaaaaaaaaaa..........",
".............aaaaaaa............"};
/* XPM */
static const char *const question_xpm[] = {
"32 32 5 1",
". c None",
"c c #000000",
"* c #999999",
"a c #ffffff",
"b c #0000ff",
"...........********.............",
"........***aaaaaaaa***..........",
"......**aaaaaaaaaaaaaa**........",
".....*aaaaaaaaaaaaaaaaaa*.......",
"....*aaaaaaaaaaaaaaaaaaaac......",
"...*aaaaaaaabbbbbbaaaaaaaac.....",
"..*aaaaaaaabaaabbbbaaaaaaaac....",
".*aaaaaaaabbaaaabbbbaaaaaaaac...",
".*aaaaaaaabbbbaabbbbaaaaaaaac*..",
"*aaaaaaaaabbbbaabbbbaaaaaaaaac*.",
"*aaaaaaaaaabbaabbbbaaaaaaaaaac*.",
"*aaaaaaaaaaaaabbbbaaaaaaaaaaac**",
"*aaaaaaaaaaaaabbbaaaaaaaaaaaac**",
"*aaaaaaaaaaaaabbaaaaaaaaaaaaac**",
"*aaaaaaaaaaaaabbaaaaaaaaaaaaac**",
"*aaaaaaaaaaaaaaaaaaaaaaaaaaaac**",
".*aaaaaaaaaaaabbaaaaaaaaaaaac***",
".*aaaaaaaaaaabbbbaaaaaaaaaaac***",
"..*aaaaaaaaaabbbbaaaaaaaaaac***.",
"...caaaaaaaaaabbaaaaaaaaaac****.",
"....caaaaaaaaaaaaaaaaaaaac****..",
".....caaaaaaaaaaaaaaaaaac****...",
"......ccaaaaaaaaaaaaaacc****....",
".......*cccaaaaaaaaccc*****.....",
"........***cccaaaac*******......",
"..........****caaac*****........",
".............*caaac**...........",
"...............caac**...........",
"................cac**...........",
".................cc**...........",
"..................***...........",
"...................**..........."};

#endif //QT_NO_IMAGEFORMAT_XPM
