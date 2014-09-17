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

#include "private/qxpmhandler_p.h"

#ifndef QT_NO_IMAGEFORMAT_XPM

#include <private/qcolor_p.h>
#include <qimage.h>
#include <qmap.h>
#include <qtextstream.h>
#include <qvariant.h>

#if defined(Q_CC_BOR)
// needed for qsort() because of a std namespace problem on Borland
#include "qplatformdefs.h"
#endif

QT_BEGIN_NAMESPACE

static quint64 xpmHash(const QString &str)
{
    unsigned int hashValue = 0;
    for (int i = 0; i < str.size(); ++i) {
        hashValue <<= 8;
        hashValue += (unsigned int)str.at(i).unicode();
    }
    return hashValue;
}
static quint64 xpmHash(char *str)
{
    unsigned int hashValue = 0;
    while (*str != '\0') {
        hashValue <<= 8;
        hashValue += (unsigned int)*str;
        ++str;
    }
    return hashValue;
}

#ifdef QRGB
#undef QRGB
#endif
#define QRGB(r,g,b) (r*65536 + g*256 + b)

static const int xpmRgbTblSize = 657;

static const struct XPMRGBData {
    uint  value;
    const char *name;
} xpmRgbTbl[] = {
  { QRGB(240,248,255),  "aliceblue" },
  { QRGB(250,235,215),  "antiquewhite" },
  { QRGB(255,239,219),  "antiquewhite1" },
  { QRGB(238,223,204),  "antiquewhite2" },
  { QRGB(205,192,176),  "antiquewhite3" },
  { QRGB(139,131,120),  "antiquewhite4" },
  { QRGB(127,255,212),  "aquamarine" },
  { QRGB(127,255,212),  "aquamarine1" },
  { QRGB(118,238,198),  "aquamarine2" },
  { QRGB(102,205,170),  "aquamarine3" },
  { QRGB( 69,139,116),  "aquamarine4" },
  { QRGB(240,255,255),  "azure" },
  { QRGB(240,255,255),  "azure1" },
  { QRGB(224,238,238),  "azure2" },
  { QRGB(193,205,205),  "azure3" },
  { QRGB(131,139,139),  "azure4" },
  { QRGB(245,245,220),  "beige" },
  { QRGB(255,228,196),  "bisque" },
  { QRGB(255,228,196),  "bisque1" },
  { QRGB(238,213,183),  "bisque2" },
  { QRGB(205,183,158),  "bisque3" },
  { QRGB(139,125,107),  "bisque4" },
  { QRGB(  0,  0,  0),  "black" },
  { QRGB(255,235,205),  "blanchedalmond" },
  { QRGB(  0,  0,255),  "blue" },
  { QRGB(  0,  0,255),  "blue1" },
  { QRGB(  0,  0,238),  "blue2" },
  { QRGB(  0,  0,205),  "blue3" },
  { QRGB(  0,  0,139),  "blue4" },
  { QRGB(138, 43,226),  "blueviolet" },
  { QRGB(165, 42, 42),  "brown" },
  { QRGB(255, 64, 64),  "brown1" },
  { QRGB(238, 59, 59),  "brown2" },
  { QRGB(205, 51, 51),  "brown3" },
  { QRGB(139, 35, 35),  "brown4" },
  { QRGB(222,184,135),  "burlywood" },
  { QRGB(255,211,155),  "burlywood1" },
  { QRGB(238,197,145),  "burlywood2" },
  { QRGB(205,170,125),  "burlywood3" },
  { QRGB(139,115, 85),  "burlywood4" },
  { QRGB( 95,158,160),  "cadetblue" },
  { QRGB(152,245,255),  "cadetblue1" },
  { QRGB(142,229,238),  "cadetblue2" },
  { QRGB(122,197,205),  "cadetblue3" },
  { QRGB( 83,134,139),  "cadetblue4" },
  { QRGB(127,255,  0),  "chartreuse" },
  { QRGB(127,255,  0),  "chartreuse1" },
  { QRGB(118,238,  0),  "chartreuse2" },
  { QRGB(102,205,  0),  "chartreuse3" },
  { QRGB( 69,139,  0),  "chartreuse4" },
  { QRGB(210,105, 30),  "chocolate" },
  { QRGB(255,127, 36),  "chocolate1" },
  { QRGB(238,118, 33),  "chocolate2" },
  { QRGB(205,102, 29),  "chocolate3" },
  { QRGB(139, 69, 19),  "chocolate4" },
  { QRGB(255,127, 80),  "coral" },
  { QRGB(255,114, 86),  "coral1" },
  { QRGB(238,106, 80),  "coral2" },
  { QRGB(205, 91, 69),  "coral3" },
  { QRGB(139, 62, 47),  "coral4" },
  { QRGB(100,149,237),  "cornflowerblue" },
  { QRGB(255,248,220),  "cornsilk" },
  { QRGB(255,248,220),  "cornsilk1" },
  { QRGB(238,232,205),  "cornsilk2" },
  { QRGB(205,200,177),  "cornsilk3" },
  { QRGB(139,136,120),  "cornsilk4" },
  { QRGB(  0,255,255),  "cyan" },
  { QRGB(  0,255,255),  "cyan1" },
  { QRGB(  0,238,238),  "cyan2" },
  { QRGB(  0,205,205),  "cyan3" },
  { QRGB(  0,139,139),  "cyan4" },
  { QRGB(  0,  0,139),  "darkblue" },
  { QRGB(  0,139,139),  "darkcyan" },
  { QRGB(184,134, 11),  "darkgoldenrod" },
  { QRGB(255,185, 15),  "darkgoldenrod1" },
  { QRGB(238,173, 14),  "darkgoldenrod2" },
  { QRGB(205,149, 12),  "darkgoldenrod3" },
  { QRGB(139,101,  8),  "darkgoldenrod4" },
  { QRGB(169,169,169),  "darkgray" },
  { QRGB(  0,100,  0),  "darkgreen" },
  { QRGB(169,169,169),  "darkgrey" },
  { QRGB(189,183,107),  "darkkhaki" },
  { QRGB(139,  0,139),  "darkmagenta" },
  { QRGB( 85,107, 47),  "darkolivegreen" },
  { QRGB(202,255,112),  "darkolivegreen1" },
  { QRGB(188,238,104),  "darkolivegreen2" },
  { QRGB(162,205, 90),  "darkolivegreen3" },
  { QRGB(110,139, 61),  "darkolivegreen4" },
  { QRGB(255,140,  0),  "darkorange" },
  { QRGB(255,127,  0),  "darkorange1" },
  { QRGB(238,118,  0),  "darkorange2" },
  { QRGB(205,102,  0),  "darkorange3" },
  { QRGB(139, 69,  0),  "darkorange4" },
  { QRGB(153, 50,204),  "darkorchid" },
  { QRGB(191, 62,255),  "darkorchid1" },
  { QRGB(178, 58,238),  "darkorchid2" },
  { QRGB(154, 50,205),  "darkorchid3" },
  { QRGB(104, 34,139),  "darkorchid4" },
  { QRGB(139,  0,  0),  "darkred" },
  { QRGB(233,150,122),  "darksalmon" },
  { QRGB(143,188,143),  "darkseagreen" },
  { QRGB(193,255,193),  "darkseagreen1" },
  { QRGB(180,238,180),  "darkseagreen2" },
  { QRGB(155,205,155),  "darkseagreen3" },
  { QRGB(105,139,105),  "darkseagreen4" },
  { QRGB( 72, 61,139),  "darkslateblue" },
  { QRGB( 47, 79, 79),  "darkslategray" },
  { QRGB(151,255,255),  "darkslategray1" },
  { QRGB(141,238,238),  "darkslategray2" },
  { QRGB(121,205,205),  "darkslategray3" },
  { QRGB( 82,139,139),  "darkslategray4" },
  { QRGB( 47, 79, 79),  "darkslategrey" },
  { QRGB(  0,206,209),  "darkturquoise" },
  { QRGB(148,  0,211),  "darkviolet" },
  { QRGB(255, 20,147),  "deeppink" },
  { QRGB(255, 20,147),  "deeppink1" },
  { QRGB(238, 18,137),  "deeppink2" },
  { QRGB(205, 16,118),  "deeppink3" },
  { QRGB(139, 10, 80),  "deeppink4" },
  { QRGB(  0,191,255),  "deepskyblue" },
  { QRGB(  0,191,255),  "deepskyblue1" },
  { QRGB(  0,178,238),  "deepskyblue2" },
  { QRGB(  0,154,205),  "deepskyblue3" },
  { QRGB(  0,104,139),  "deepskyblue4" },
  { QRGB(105,105,105),  "dimgray" },
  { QRGB(105,105,105),  "dimgrey" },
  { QRGB( 30,144,255),  "dodgerblue" },
  { QRGB( 30,144,255),  "dodgerblue1" },
  { QRGB( 28,134,238),  "dodgerblue2" },
  { QRGB( 24,116,205),  "dodgerblue3" },
  { QRGB( 16, 78,139),  "dodgerblue4" },
  { QRGB(178, 34, 34),  "firebrick" },
  { QRGB(255, 48, 48),  "firebrick1" },
  { QRGB(238, 44, 44),  "firebrick2" },
  { QRGB(205, 38, 38),  "firebrick3" },
  { QRGB(139, 26, 26),  "firebrick4" },
  { QRGB(255,250,240),  "floralwhite" },
  { QRGB( 34,139, 34),  "forestgreen" },
  { QRGB(220,220,220),  "gainsboro" },
  { QRGB(248,248,255),  "ghostwhite" },
  { QRGB(255,215,  0),  "gold" },
  { QRGB(255,215,  0),  "gold1" },
  { QRGB(238,201,  0),  "gold2" },
  { QRGB(205,173,  0),  "gold3" },
  { QRGB(139,117,  0),  "gold4" },
  { QRGB(218,165, 32),  "goldenrod" },
  { QRGB(255,193, 37),  "goldenrod1" },
  { QRGB(238,180, 34),  "goldenrod2" },
  { QRGB(205,155, 29),  "goldenrod3" },
  { QRGB(139,105, 20),  "goldenrod4" },
  { QRGB(190,190,190),  "gray" },
  { QRGB(  0,  0,  0),  "gray0" },
  { QRGB(  3,  3,  3),  "gray1" },
  { QRGB( 26, 26, 26),  "gray10" },
  { QRGB(255,255,255),  "gray100" },
  { QRGB( 28, 28, 28),  "gray11" },
  { QRGB( 31, 31, 31),  "gray12" },
  { QRGB( 33, 33, 33),  "gray13" },
  { QRGB( 36, 36, 36),  "gray14" },
  { QRGB( 38, 38, 38),  "gray15" },
  { QRGB( 41, 41, 41),  "gray16" },
  { QRGB( 43, 43, 43),  "gray17" },
  { QRGB( 46, 46, 46),  "gray18" },
  { QRGB( 48, 48, 48),  "gray19" },
  { QRGB(  5,  5,  5),  "gray2" },
  { QRGB( 51, 51, 51),  "gray20" },
  { QRGB( 54, 54, 54),  "gray21" },
  { QRGB( 56, 56, 56),  "gray22" },
  { QRGB( 59, 59, 59),  "gray23" },
  { QRGB( 61, 61, 61),  "gray24" },
  { QRGB( 64, 64, 64),  "gray25" },
  { QRGB( 66, 66, 66),  "gray26" },
  { QRGB( 69, 69, 69),  "gray27" },
  { QRGB( 71, 71, 71),  "gray28" },
  { QRGB( 74, 74, 74),  "gray29" },
  { QRGB(  8,  8,  8),  "gray3" },
  { QRGB( 77, 77, 77),  "gray30" },
  { QRGB( 79, 79, 79),  "gray31" },
  { QRGB( 82, 82, 82),  "gray32" },
  { QRGB( 84, 84, 84),  "gray33" },
  { QRGB( 87, 87, 87),  "gray34" },
  { QRGB( 89, 89, 89),  "gray35" },
  { QRGB( 92, 92, 92),  "gray36" },
  { QRGB( 94, 94, 94),  "gray37" },
  { QRGB( 97, 97, 97),  "gray38" },
  { QRGB( 99, 99, 99),  "gray39" },
  { QRGB( 10, 10, 10),  "gray4" },
  { QRGB(102,102,102),  "gray40" },
  { QRGB(105,105,105),  "gray41" },
  { QRGB(107,107,107),  "gray42" },
  { QRGB(110,110,110),  "gray43" },
  { QRGB(112,112,112),  "gray44" },
  { QRGB(115,115,115),  "gray45" },
  { QRGB(117,117,117),  "gray46" },
  { QRGB(120,120,120),  "gray47" },
  { QRGB(122,122,122),  "gray48" },
  { QRGB(125,125,125),  "gray49" },
  { QRGB( 13, 13, 13),  "gray5" },
  { QRGB(127,127,127),  "gray50" },
  { QRGB(130,130,130),  "gray51" },
  { QRGB(133,133,133),  "gray52" },
  { QRGB(135,135,135),  "gray53" },
  { QRGB(138,138,138),  "gray54" },
  { QRGB(140,140,140),  "gray55" },
  { QRGB(143,143,143),  "gray56" },
  { QRGB(145,145,145),  "gray57" },
  { QRGB(148,148,148),  "gray58" },
  { QRGB(150,150,150),  "gray59" },
  { QRGB( 15, 15, 15),  "gray6" },
  { QRGB(153,153,153),  "gray60" },
  { QRGB(156,156,156),  "gray61" },
  { QRGB(158,158,158),  "gray62" },
  { QRGB(161,161,161),  "gray63" },
  { QRGB(163,163,163),  "gray64" },
  { QRGB(166,166,166),  "gray65" },
  { QRGB(168,168,168),  "gray66" },
  { QRGB(171,171,171),  "gray67" },
  { QRGB(173,173,173),  "gray68" },
  { QRGB(176,176,176),  "gray69" },
  { QRGB( 18, 18, 18),  "gray7" },
  { QRGB(179,179,179),  "gray70" },
  { QRGB(181,181,181),  "gray71" },
  { QRGB(184,184,184),  "gray72" },
  { QRGB(186,186,186),  "gray73" },
  { QRGB(189,189,189),  "gray74" },
  { QRGB(191,191,191),  "gray75" },
  { QRGB(194,194,194),  "gray76" },
  { QRGB(196,196,196),  "gray77" },
  { QRGB(199,199,199),  "gray78" },
  { QRGB(201,201,201),  "gray79" },
  { QRGB( 20, 20, 20),  "gray8" },
  { QRGB(204,204,204),  "gray80" },
  { QRGB(207,207,207),  "gray81" },
  { QRGB(209,209,209),  "gray82" },
  { QRGB(212,212,212),  "gray83" },
  { QRGB(214,214,214),  "gray84" },
  { QRGB(217,217,217),  "gray85" },
  { QRGB(219,219,219),  "gray86" },
  { QRGB(222,222,222),  "gray87" },
  { QRGB(224,224,224),  "gray88" },
  { QRGB(227,227,227),  "gray89" },
  { QRGB( 23, 23, 23),  "gray9" },
  { QRGB(229,229,229),  "gray90" },
  { QRGB(232,232,232),  "gray91" },
  { QRGB(235,235,235),  "gray92" },
  { QRGB(237,237,237),  "gray93" },
  { QRGB(240,240,240),  "gray94" },
  { QRGB(242,242,242),  "gray95" },
  { QRGB(245,245,245),  "gray96" },
  { QRGB(247,247,247),  "gray97" },
  { QRGB(250,250,250),  "gray98" },
  { QRGB(252,252,252),  "gray99" },
  { QRGB(  0,255,  0),  "green" },
  { QRGB(  0,255,  0),  "green1" },
  { QRGB(  0,238,  0),  "green2" },
  { QRGB(  0,205,  0),  "green3" },
  { QRGB(  0,139,  0),  "green4" },
  { QRGB(173,255, 47),  "greenyellow" },
  { QRGB(190,190,190),  "grey" },
  { QRGB(  0,  0,  0),  "grey0" },
  { QRGB(  3,  3,  3),  "grey1" },
  { QRGB( 26, 26, 26),  "grey10" },
  { QRGB(255,255,255),  "grey100" },
  { QRGB( 28, 28, 28),  "grey11" },
  { QRGB( 31, 31, 31),  "grey12" },
  { QRGB( 33, 33, 33),  "grey13" },
  { QRGB( 36, 36, 36),  "grey14" },
  { QRGB( 38, 38, 38),  "grey15" },
  { QRGB( 41, 41, 41),  "grey16" },
  { QRGB( 43, 43, 43),  "grey17" },
  { QRGB( 46, 46, 46),  "grey18" },
  { QRGB( 48, 48, 48),  "grey19" },
  { QRGB(  5,  5,  5),  "grey2" },
  { QRGB( 51, 51, 51),  "grey20" },
  { QRGB( 54, 54, 54),  "grey21" },
  { QRGB( 56, 56, 56),  "grey22" },
  { QRGB( 59, 59, 59),  "grey23" },
  { QRGB( 61, 61, 61),  "grey24" },
  { QRGB( 64, 64, 64),  "grey25" },
  { QRGB( 66, 66, 66),  "grey26" },
  { QRGB( 69, 69, 69),  "grey27" },
  { QRGB( 71, 71, 71),  "grey28" },
  { QRGB( 74, 74, 74),  "grey29" },
  { QRGB(  8,  8,  8),  "grey3" },
  { QRGB( 77, 77, 77),  "grey30" },
  { QRGB( 79, 79, 79),  "grey31" },
  { QRGB( 82, 82, 82),  "grey32" },
  { QRGB( 84, 84, 84),  "grey33" },
  { QRGB( 87, 87, 87),  "grey34" },
  { QRGB( 89, 89, 89),  "grey35" },
  { QRGB( 92, 92, 92),  "grey36" },
  { QRGB( 94, 94, 94),  "grey37" },
  { QRGB( 97, 97, 97),  "grey38" },
  { QRGB( 99, 99, 99),  "grey39" },
  { QRGB( 10, 10, 10),  "grey4" },
  { QRGB(102,102,102),  "grey40" },
  { QRGB(105,105,105),  "grey41" },
  { QRGB(107,107,107),  "grey42" },
  { QRGB(110,110,110),  "grey43" },
  { QRGB(112,112,112),  "grey44" },
  { QRGB(115,115,115),  "grey45" },
  { QRGB(117,117,117),  "grey46" },
  { QRGB(120,120,120),  "grey47" },
  { QRGB(122,122,122),  "grey48" },
  { QRGB(125,125,125),  "grey49" },
  { QRGB( 13, 13, 13),  "grey5" },
  { QRGB(127,127,127),  "grey50" },
  { QRGB(130,130,130),  "grey51" },
  { QRGB(133,133,133),  "grey52" },
  { QRGB(135,135,135),  "grey53" },
  { QRGB(138,138,138),  "grey54" },
  { QRGB(140,140,140),  "grey55" },
  { QRGB(143,143,143),  "grey56" },
  { QRGB(145,145,145),  "grey57" },
  { QRGB(148,148,148),  "grey58" },
  { QRGB(150,150,150),  "grey59" },
  { QRGB( 15, 15, 15),  "grey6" },
  { QRGB(153,153,153),  "grey60" },
  { QRGB(156,156,156),  "grey61" },
  { QRGB(158,158,158),  "grey62" },
  { QRGB(161,161,161),  "grey63" },
  { QRGB(163,163,163),  "grey64" },
  { QRGB(166,166,166),  "grey65" },
  { QRGB(168,168,168),  "grey66" },
  { QRGB(171,171,171),  "grey67" },
  { QRGB(173,173,173),  "grey68" },
  { QRGB(176,176,176),  "grey69" },
  { QRGB( 18, 18, 18),  "grey7" },
  { QRGB(179,179,179),  "grey70" },
  { QRGB(181,181,181),  "grey71" },
  { QRGB(184,184,184),  "grey72" },
  { QRGB(186,186,186),  "grey73" },
  { QRGB(189,189,189),  "grey74" },
  { QRGB(191,191,191),  "grey75" },
  { QRGB(194,194,194),  "grey76" },
  { QRGB(196,196,196),  "grey77" },
  { QRGB(199,199,199),  "grey78" },
  { QRGB(201,201,201),  "grey79" },
  { QRGB( 20, 20, 20),  "grey8" },
  { QRGB(204,204,204),  "grey80" },
  { QRGB(207,207,207),  "grey81" },
  { QRGB(209,209,209),  "grey82" },
  { QRGB(212,212,212),  "grey83" },
  { QRGB(214,214,214),  "grey84" },
  { QRGB(217,217,217),  "grey85" },
  { QRGB(219,219,219),  "grey86" },
  { QRGB(222,222,222),  "grey87" },
  { QRGB(224,224,224),  "grey88" },
  { QRGB(227,227,227),  "grey89" },
  { QRGB( 23, 23, 23),  "grey9" },
  { QRGB(229,229,229),  "grey90" },
  { QRGB(232,232,232),  "grey91" },
  { QRGB(235,235,235),  "grey92" },
  { QRGB(237,237,237),  "grey93" },
  { QRGB(240,240,240),  "grey94" },
  { QRGB(242,242,242),  "grey95" },
  { QRGB(245,245,245),  "grey96" },
  { QRGB(247,247,247),  "grey97" },
  { QRGB(250,250,250),  "grey98" },
  { QRGB(252,252,252),  "grey99" },
  { QRGB(240,255,240),  "honeydew" },
  { QRGB(240,255,240),  "honeydew1" },
  { QRGB(224,238,224),  "honeydew2" },
  { QRGB(193,205,193),  "honeydew3" },
  { QRGB(131,139,131),  "honeydew4" },
  { QRGB(255,105,180),  "hotpink" },
  { QRGB(255,110,180),  "hotpink1" },
  { QRGB(238,106,167),  "hotpink2" },
  { QRGB(205, 96,144),  "hotpink3" },
  { QRGB(139, 58, 98),  "hotpink4" },
  { QRGB(205, 92, 92),  "indianred" },
  { QRGB(255,106,106),  "indianred1" },
  { QRGB(238, 99, 99),  "indianred2" },
  { QRGB(205, 85, 85),  "indianred3" },
  { QRGB(139, 58, 58),  "indianred4" },
  { QRGB(255,255,240),  "ivory" },
  { QRGB(255,255,240),  "ivory1" },
  { QRGB(238,238,224),  "ivory2" },
  { QRGB(205,205,193),  "ivory3" },
  { QRGB(139,139,131),  "ivory4" },
  { QRGB(240,230,140),  "khaki" },
  { QRGB(255,246,143),  "khaki1" },
  { QRGB(238,230,133),  "khaki2" },
  { QRGB(205,198,115),  "khaki3" },
  { QRGB(139,134, 78),  "khaki4" },
  { QRGB(230,230,250),  "lavender" },
  { QRGB(255,240,245),  "lavenderblush" },
  { QRGB(255,240,245),  "lavenderblush1" },
  { QRGB(238,224,229),  "lavenderblush2" },
  { QRGB(205,193,197),  "lavenderblush3" },
  { QRGB(139,131,134),  "lavenderblush4" },
  { QRGB(124,252,  0),  "lawngreen" },
  { QRGB(255,250,205),  "lemonchiffon" },
  { QRGB(255,250,205),  "lemonchiffon1" },
  { QRGB(238,233,191),  "lemonchiffon2" },
  { QRGB(205,201,165),  "lemonchiffon3" },
  { QRGB(139,137,112),  "lemonchiffon4" },
  { QRGB(173,216,230),  "lightblue" },
  { QRGB(191,239,255),  "lightblue1" },
  { QRGB(178,223,238),  "lightblue2" },
  { QRGB(154,192,205),  "lightblue3" },
  { QRGB(104,131,139),  "lightblue4" },
  { QRGB(240,128,128),  "lightcoral" },
  { QRGB(224,255,255),  "lightcyan" },
  { QRGB(224,255,255),  "lightcyan1" },
  { QRGB(209,238,238),  "lightcyan2" },
  { QRGB(180,205,205),  "lightcyan3" },
  { QRGB(122,139,139),  "lightcyan4" },
  { QRGB(238,221,130),  "lightgoldenrod" },
  { QRGB(255,236,139),  "lightgoldenrod1" },
  { QRGB(238,220,130),  "lightgoldenrod2" },
  { QRGB(205,190,112),  "lightgoldenrod3" },
  { QRGB(139,129, 76),  "lightgoldenrod4" },
  { QRGB(250,250,210),  "lightgoldenrodyellow" },
  { QRGB(211,211,211),  "lightgray" },
  { QRGB(144,238,144),  "lightgreen" },
  { QRGB(211,211,211),  "lightgrey" },
  { QRGB(255,182,193),  "lightpink" },
  { QRGB(255,174,185),  "lightpink1" },
  { QRGB(238,162,173),  "lightpink2" },
  { QRGB(205,140,149),  "lightpink3" },
  { QRGB(139, 95,101),  "lightpink4" },
  { QRGB(255,160,122),  "lightsalmon" },
  { QRGB(255,160,122),  "lightsalmon1" },
  { QRGB(238,149,114),  "lightsalmon2" },
  { QRGB(205,129, 98),  "lightsalmon3" },
  { QRGB(139, 87, 66),  "lightsalmon4" },
  { QRGB( 32,178,170),  "lightseagreen" },
  { QRGB(135,206,250),  "lightskyblue" },
  { QRGB(176,226,255),  "lightskyblue1" },
  { QRGB(164,211,238),  "lightskyblue2" },
  { QRGB(141,182,205),  "lightskyblue3" },
  { QRGB( 96,123,139),  "lightskyblue4" },
  { QRGB(132,112,255),  "lightslateblue" },
  { QRGB(119,136,153),  "lightslategray" },
  { QRGB(119,136,153),  "lightslategrey" },
  { QRGB(176,196,222),  "lightsteelblue" },
  { QRGB(202,225,255),  "lightsteelblue1" },
  { QRGB(188,210,238),  "lightsteelblue2" },
  { QRGB(162,181,205),  "lightsteelblue3" },
  { QRGB(110,123,139),  "lightsteelblue4" },
  { QRGB(255,255,224),  "lightyellow" },
  { QRGB(255,255,224),  "lightyellow1" },
  { QRGB(238,238,209),  "lightyellow2" },
  { QRGB(205,205,180),  "lightyellow3" },
  { QRGB(139,139,122),  "lightyellow4" },
  { QRGB( 50,205, 50),  "limegreen" },
  { QRGB(250,240,230),  "linen" },
  { QRGB(255,  0,255),  "magenta" },
  { QRGB(255,  0,255),  "magenta1" },
  { QRGB(238,  0,238),  "magenta2" },
  { QRGB(205,  0,205),  "magenta3" },
  { QRGB(139,  0,139),  "magenta4" },
  { QRGB(176, 48, 96),  "maroon" },
  { QRGB(255, 52,179),  "maroon1" },
  { QRGB(238, 48,167),  "maroon2" },
  { QRGB(205, 41,144),  "maroon3" },
  { QRGB(139, 28, 98),  "maroon4" },
  { QRGB(102,205,170),  "mediumaquamarine" },
  { QRGB(  0,  0,205),  "mediumblue" },
  { QRGB(186, 85,211),  "mediumorchid" },
  { QRGB(224,102,255),  "mediumorchid1" },
  { QRGB(209, 95,238),  "mediumorchid2" },
  { QRGB(180, 82,205),  "mediumorchid3" },
  { QRGB(122, 55,139),  "mediumorchid4" },
  { QRGB(147,112,219),  "mediumpurple" },
  { QRGB(171,130,255),  "mediumpurple1" },
  { QRGB(159,121,238),  "mediumpurple2" },
  { QRGB(137,104,205),  "mediumpurple3" },
  { QRGB( 93, 71,139),  "mediumpurple4" },
  { QRGB( 60,179,113),  "mediumseagreen" },
  { QRGB(123,104,238),  "mediumslateblue" },
  { QRGB(  0,250,154),  "mediumspringgreen" },
  { QRGB( 72,209,204),  "mediumturquoise" },
  { QRGB(199, 21,133),  "mediumvioletred" },
  { QRGB( 25, 25,112),  "midnightblue" },
  { QRGB(245,255,250),  "mintcream" },
  { QRGB(255,228,225),  "mistyrose" },
  { QRGB(255,228,225),  "mistyrose1" },
  { QRGB(238,213,210),  "mistyrose2" },
  { QRGB(205,183,181),  "mistyrose3" },
  { QRGB(139,125,123),  "mistyrose4" },
  { QRGB(255,228,181),  "moccasin" },
  { QRGB(255,222,173),  "navajowhite" },
  { QRGB(255,222,173),  "navajowhite1" },
  { QRGB(238,207,161),  "navajowhite2" },
  { QRGB(205,179,139),  "navajowhite3" },
  { QRGB(139,121, 94),  "navajowhite4" },
  { QRGB(  0,  0,128),  "navy" },
  { QRGB(  0,  0,128),  "navyblue" },
  { QRGB(253,245,230),  "oldlace" },
  { QRGB(107,142, 35),  "olivedrab" },
  { QRGB(192,255, 62),  "olivedrab1" },
  { QRGB(179,238, 58),  "olivedrab2" },
  { QRGB(154,205, 50),  "olivedrab3" },
  { QRGB(105,139, 34),  "olivedrab4" },
  { QRGB(255,165,  0),  "orange" },
  { QRGB(255,165,  0),  "orange1" },
  { QRGB(238,154,  0),  "orange2" },
  { QRGB(205,133,  0),  "orange3" },
  { QRGB(139, 90,  0),  "orange4" },
  { QRGB(255, 69,  0),  "orangered" },
  { QRGB(255, 69,  0),  "orangered1" },
  { QRGB(238, 64,  0),  "orangered2" },
  { QRGB(205, 55,  0),  "orangered3" },
  { QRGB(139, 37,  0),  "orangered4" },
  { QRGB(218,112,214),  "orchid" },
  { QRGB(255,131,250),  "orchid1" },
  { QRGB(238,122,233),  "orchid2" },
  { QRGB(205,105,201),  "orchid3" },
  { QRGB(139, 71,137),  "orchid4" },
  { QRGB(238,232,170),  "palegoldenrod" },
  { QRGB(152,251,152),  "palegreen" },
  { QRGB(154,255,154),  "palegreen1" },
  { QRGB(144,238,144),  "palegreen2" },
  { QRGB(124,205,124),  "palegreen3" },
  { QRGB( 84,139, 84),  "palegreen4" },
  { QRGB(175,238,238),  "paleturquoise" },
  { QRGB(187,255,255),  "paleturquoise1" },
  { QRGB(174,238,238),  "paleturquoise2" },
  { QRGB(150,205,205),  "paleturquoise3" },
  { QRGB(102,139,139),  "paleturquoise4" },
  { QRGB(219,112,147),  "palevioletred" },
  { QRGB(255,130,171),  "palevioletred1" },
  { QRGB(238,121,159),  "palevioletred2" },
  { QRGB(205,104,137),  "palevioletred3" },
  { QRGB(139, 71, 93),  "palevioletred4" },
  { QRGB(255,239,213),  "papayawhip" },
  { QRGB(255,218,185),  "peachpuff" },
  { QRGB(255,218,185),  "peachpuff1" },
  { QRGB(238,203,173),  "peachpuff2" },
  { QRGB(205,175,149),  "peachpuff3" },
  { QRGB(139,119,101),  "peachpuff4" },
  { QRGB(205,133, 63),  "peru" },
  { QRGB(255,192,203),  "pink" },
  { QRGB(255,181,197),  "pink1" },
  { QRGB(238,169,184),  "pink2" },
  { QRGB(205,145,158),  "pink3" },
  { QRGB(139, 99,108),  "pink4" },
  { QRGB(221,160,221),  "plum" },
  { QRGB(255,187,255),  "plum1" },
  { QRGB(238,174,238),  "plum2" },
  { QRGB(205,150,205),  "plum3" },
  { QRGB(139,102,139),  "plum4" },
  { QRGB(176,224,230),  "powderblue" },
  { QRGB(160, 32,240),  "purple" },
  { QRGB(155, 48,255),  "purple1" },
  { QRGB(145, 44,238),  "purple2" },
  { QRGB(125, 38,205),  "purple3" },
  { QRGB( 85, 26,139),  "purple4" },
  { QRGB(255,  0,  0),  "red" },
  { QRGB(255,  0,  0),  "red1" },
  { QRGB(238,  0,  0),  "red2" },
  { QRGB(205,  0,  0),  "red3" },
  { QRGB(139,  0,  0),  "red4" },
  { QRGB(188,143,143),  "rosybrown" },
  { QRGB(255,193,193),  "rosybrown1" },
  { QRGB(238,180,180),  "rosybrown2" },
  { QRGB(205,155,155),  "rosybrown3" },
  { QRGB(139,105,105),  "rosybrown4" },
  { QRGB( 65,105,225),  "royalblue" },
  { QRGB( 72,118,255),  "royalblue1" },
  { QRGB( 67,110,238),  "royalblue2" },
  { QRGB( 58, 95,205),  "royalblue3" },
  { QRGB( 39, 64,139),  "royalblue4" },
  { QRGB(139, 69, 19),  "saddlebrown" },
  { QRGB(250,128,114),  "salmon" },
  { QRGB(255,140,105),  "salmon1" },
  { QRGB(238,130, 98),  "salmon2" },
  { QRGB(205,112, 84),  "salmon3" },
  { QRGB(139, 76, 57),  "salmon4" },
  { QRGB(244,164, 96),  "sandybrown" },
  { QRGB( 46,139, 87),  "seagreen" },
  { QRGB( 84,255,159),  "seagreen1" },
  { QRGB( 78,238,148),  "seagreen2" },
  { QRGB( 67,205,128),  "seagreen3" },
  { QRGB( 46,139, 87),  "seagreen4" },
  { QRGB(255,245,238),  "seashell" },
  { QRGB(255,245,238),  "seashell1" },
  { QRGB(238,229,222),  "seashell2" },
  { QRGB(205,197,191),  "seashell3" },
  { QRGB(139,134,130),  "seashell4" },
  { QRGB(160, 82, 45),  "sienna" },
  { QRGB(255,130, 71),  "sienna1" },
  { QRGB(238,121, 66),  "sienna2" },
  { QRGB(205,104, 57),  "sienna3" },
  { QRGB(139, 71, 38),  "sienna4" },
  { QRGB(135,206,235),  "skyblue" },
  { QRGB(135,206,255),  "skyblue1" },
  { QRGB(126,192,238),  "skyblue2" },
  { QRGB(108,166,205),  "skyblue3" },
  { QRGB( 74,112,139),  "skyblue4" },
  { QRGB(106, 90,205),  "slateblue" },
  { QRGB(131,111,255),  "slateblue1" },
  { QRGB(122,103,238),  "slateblue2" },
  { QRGB(105, 89,205),  "slateblue3" },
  { QRGB( 71, 60,139),  "slateblue4" },
  { QRGB(112,128,144),  "slategray" },
  { QRGB(198,226,255),  "slategray1" },
  { QRGB(185,211,238),  "slategray2" },
  { QRGB(159,182,205),  "slategray3" },
  { QRGB(108,123,139),  "slategray4" },
  { QRGB(112,128,144),  "slategrey" },
  { QRGB(255,250,250),  "snow" },
  { QRGB(255,250,250),  "snow1" },
  { QRGB(238,233,233),  "snow2" },
  { QRGB(205,201,201),  "snow3" },
  { QRGB(139,137,137),  "snow4" },
  { QRGB(  0,255,127),  "springgreen" },
  { QRGB(  0,255,127),  "springgreen1" },
  { QRGB(  0,238,118),  "springgreen2" },
  { QRGB(  0,205,102),  "springgreen3" },
  { QRGB(  0,139, 69),  "springgreen4" },
  { QRGB( 70,130,180),  "steelblue" },
  { QRGB( 99,184,255),  "steelblue1" },
  { QRGB( 92,172,238),  "steelblue2" },
  { QRGB( 79,148,205),  "steelblue3" },
  { QRGB( 54,100,139),  "steelblue4" },
  { QRGB(210,180,140),  "tan" },
  { QRGB(255,165, 79),  "tan1" },
  { QRGB(238,154, 73),  "tan2" },
  { QRGB(205,133, 63),  "tan3" },
  { QRGB(139, 90, 43),  "tan4" },
  { QRGB(216,191,216),  "thistle" },
  { QRGB(255,225,255),  "thistle1" },
  { QRGB(238,210,238),  "thistle2" },
  { QRGB(205,181,205),  "thistle3" },
  { QRGB(139,123,139),  "thistle4" },
  { QRGB(255, 99, 71),  "tomato" },
  { QRGB(255, 99, 71),  "tomato1" },
  { QRGB(238, 92, 66),  "tomato2" },
  { QRGB(205, 79, 57),  "tomato3" },
  { QRGB(139, 54, 38),  "tomato4" },
  { QRGB( 64,224,208),  "turquoise" },
  { QRGB(  0,245,255),  "turquoise1" },
  { QRGB(  0,229,238),  "turquoise2" },
  { QRGB(  0,197,205),  "turquoise3" },
  { QRGB(  0,134,139),  "turquoise4" },
  { QRGB(238,130,238),  "violet" },
  { QRGB(208, 32,144),  "violetred" },
  { QRGB(255, 62,150),  "violetred1" },
  { QRGB(238, 58,140),  "violetred2" },
  { QRGB(205, 50,120),  "violetred3" },
  { QRGB(139, 34, 82),  "violetred4" },
  { QRGB(245,222,179),  "wheat" },
  { QRGB(255,231,186),  "wheat1" },
  { QRGB(238,216,174),  "wheat2" },
  { QRGB(205,186,150),  "wheat3" },
  { QRGB(139,126,102),  "wheat4" },
  { QRGB(255,255,255),  "white" },
  { QRGB(245,245,245),  "whitesmoke" },
  { QRGB(255,255,  0),  "yellow" },
  { QRGB(255,255,  0),  "yellow1" },
  { QRGB(238,238,  0),  "yellow2" },
  { QRGB(205,205,  0),  "yellow3" },
  { QRGB(139,139,  0),  "yellow4" },
  { QRGB(154,205, 50),  "yellowgreen" } };

inline bool operator<(const char *name, const XPMRGBData &data)
{ return qstrcmp(name, data.name) < 0; }
inline bool operator<(const XPMRGBData &data, const char *name)
{ return qstrcmp(data.name, name) < 0; }

static inline bool qt_get_named_xpm_rgb(const char *name_no_space, QRgb *rgb)
{
    const XPMRGBData *r = qBinaryFind(xpmRgbTbl, xpmRgbTbl + xpmRgbTblSize, name_no_space);
    if (r != xpmRgbTbl + xpmRgbTblSize) {
        *rgb = r->value;
        return true;
    } else {
        return false;
    }
}

/*****************************************************************************
  Misc. utility functions
 *****************************************************************************/
static QString fbname(const QString &fileName) // get file basename (sort of)
{
    QString s = fileName;
    if (!s.isEmpty()) {
        int i;
        if ((i = s.lastIndexOf(QLatin1Char('/'))) >= 0)
            s = s.mid(i);
        if ((i = s.lastIndexOf(QLatin1Char('\\'))) >= 0)
            s = s.mid(i);
        QRegExp r(QLatin1String("[a-zA-Z][a-zA-Z0-9_]*"));
        int p = r.indexIn(s);
        if (p == -1)
            s.clear();
        else
            s = s.mid(p, r.matchedLength());
    }
    if (s.isEmpty())
        s = QString::fromLatin1("dummy");
    return s;
}

// Skip until ", read until the next ", return the rest in *buf
// Returns false on error, true on success

static bool read_xpm_string(QByteArray &buf, QIODevice *d, const char * const *source, int &index,
                            QByteArray &state)
{
    if (source) {
        buf = source[index++];
        return true;
    }

    buf = "";
    bool gotQuote = false;
    int offset = 0;
    forever {
        if (offset == state.size() || state.isEmpty()) {
            char buf[2048];
            qint64 bytesRead = d->read(buf, sizeof(buf));
            if (bytesRead <= 0)
                return false;
            state = QByteArray(buf, int(bytesRead));
            offset = 0;
        }

        if (!gotQuote) {
            if (state.at(offset++) == '"')
                gotQuote = true;
        } else {
            char c = state.at(offset++);
            if (c == '"')
                break;
            buf += c;
        }
    }
    state.remove(0, offset);
    return true;
}

// Tests if the given prefix can be the start of an XPM color specification

static bool is_xpm_color_spec_prefix(const QByteArray& prefix)
{
    return prefix == "c" ||
           prefix == "g" ||
           prefix == "g4" ||
           prefix == "m" ||
           prefix == "s";
}

// Reads XPM header.

static bool read_xpm_header(
    QIODevice *device, const char * const * source, int& index, QByteArray &state,
    int *cpp, int *ncols, int *w, int *h)
{
    QByteArray buf(200, 0);

    if (!read_xpm_string(buf, device, source, index, state))
        return false;

#if defined(_MSC_VER) && _MSC_VER >= 1400 && !defined(Q_OS_WINCE)
        if (sscanf_s(buf, "%d %d %d %d", w, h, ncols, cpp) < 4)
#else
    if (sscanf(buf, "%d %d %d %d", w, h, ncols, cpp) < 4)
#endif
        return false;                                        // < 4 numbers parsed

    return true;
}

// Reads XPM body (color information & pixels).

static bool read_xpm_body(
    QIODevice *device, const char * const * source, int& index, QByteArray& state,
    int cpp, int ncols, int w, int h, QImage& image)
{
    QByteArray buf(200, 0);
    int i;

    if (cpp < 0 || cpp > 15)
        return false;

    // For > 256 colors, we delay creation of the image until
    // after we have read the color specifications, so that we can
    // create it in correct format (Format_RGB32 vs Format_ARGB32,
    // depending on absence or presence of "c none", respectively)
    if (ncols <= 256) {
        if (image.size() != QSize(w, h) || image.format() != QImage::Format_Indexed8) {
            image = QImage(w, h, QImage::Format_Indexed8);
            if (image.isNull())
                return false;
        }
        image.setColorCount(ncols);
    }

    QMap<quint64, int> colorMap;
    int currentColor;
    bool hasTransparency = false;

    for(currentColor=0; currentColor < ncols; ++currentColor) {
        if (!read_xpm_string(buf, device, source, index, state)) {
            qWarning("QImage: XPM color specification missing");
            return false;
        }
        QByteArray index;
        index = buf.left(cpp);
        buf = buf.mid(cpp).simplified().trimmed().toLower();
        QList<QByteArray> tokens = buf.split(' ');
        i = tokens.indexOf("c");
        if (i < 0)
            i = tokens.indexOf("g");
        if (i < 0)
            i = tokens.indexOf("g4");
        if (i < 0)
            i = tokens.indexOf("m");
        if (i < 0) {
            qWarning("QImage: XPM color specification is missing: %s", buf.constData());
            return false;        // no c/g/g4/m specification at all
        }
        QByteArray color;
        while ((++i < tokens.size()) && !is_xpm_color_spec_prefix(tokens.at(i))) {
            color.append(tokens.at(i));
        }
        if (color.isEmpty()) {
            qWarning("QImage: XPM color value is missing from specification: %s", buf.constData());
            return false;        // no color value
        }
        buf = color;
        if (buf == "none") {
            hasTransparency = true;
            int transparentColor = currentColor;
            if (ncols <= 256) {
                image.setColor(transparentColor, 0);
                colorMap.insert(xpmHash(QLatin1String(index.constData())), transparentColor);
            } else {
                colorMap.insert(xpmHash(QLatin1String(index.constData())), 0);
            }
        } else {
            QRgb c_rgb;
            if (((buf.length()-1) % 3) && (buf[0] == '#')) {
                buf.truncate(((buf.length()-1) / 4 * 3) + 1); // remove alpha channel left by imagemagick
            }
            if (buf[0] == '#') {
                qt_get_hex_rgb(buf, &c_rgb);
            } else {
                qt_get_named_xpm_rgb(buf, &c_rgb);
            }
            if (ncols <= 256) {
                image.setColor(currentColor, 0xff000000 | c_rgb);
                colorMap.insert(xpmHash(QLatin1String(index.constData())), currentColor);
            } else {
                colorMap.insert(xpmHash(QLatin1String(index.constData())), 0xff000000 | c_rgb);
            }
        }
    }

    if (ncols > 256) {
        // Now we can create 32-bit image of appropriate format
        QImage::Format format = hasTransparency ?
                                QImage::Format_ARGB32 : QImage::Format_RGB32;
        if (image.size() != QSize(w, h) || image.format() != format) {
            image = QImage(w, h, format);
            if (image.isNull())
                return false;
        }
    }

    // Read pixels
    for(int y=0; y<h; y++) {
        if (!read_xpm_string(buf, device, source, index, state)) {
            qWarning("QImage: XPM pixels missing on image line %d", y);
            return false;
        }
        if (image.depth() == 8) {
            uchar *p = image.scanLine(y);
            uchar *d = (uchar *)buf.data();
            uchar *end = d + buf.length();
            int x;
            if (cpp == 1) {
                char b[2];
                b[1] = '\0';
                for (x=0; x<w && d<end; x++) {
                    b[0] = *d++;
                    *p++ = (uchar)colorMap[xpmHash(b)];
                }
            } else {
                char b[16];
                b[cpp] = '\0';
                for (x=0; x<w && d<end; x++) {
                    memcpy(b, (char *)d, cpp);
                    *p++ = (uchar)colorMap[xpmHash(b)];
                    d += cpp;
                }
            }
            // avoid uninitialized memory for malformed xpms
            if (x < w) {
                qWarning("QImage: XPM pixels missing on image line %d (possibly a C++ trigraph).", y);
                memset(p, 0, w - x);
            }
        } else {
            QRgb *p = (QRgb*)image.scanLine(y);
            uchar *d = (uchar *)buf.data();
            uchar *end = d + buf.length();
            int x;
            char b[16];
            b[cpp] = '\0';
            for (x=0; x<w && d<end; x++) {
                memcpy(b, (char *)d, cpp);
                *p++ = (QRgb)colorMap[xpmHash(b)];
                d += cpp;
            }
            // avoid uninitialized memory for malformed xpms
            if (x < w) {
                qWarning("QImage: XPM pixels missing on image line %d (possibly a C++ trigraph).", y);
                memset(p, 0, (w - x)*4);
            }
        }
    }

    if (device) {
        // Rewind unused characters, and skip to the end of the XPM struct.
        for (int i = state.size() - 1; i >= 0; --i)
            device->ungetChar(state[i]);
        char c;
        while (device->getChar(&c) && c != ';') {}
        while (device->getChar(&c) && c != '\n') {}
    }
    return true;
}

//
// INTERNAL
//
// Reads an .xpm from either the QImageIO or from the QString *.
// One of the two HAS to be 0, the other one is used.
//

bool qt_read_xpm_image_or_array(QIODevice *device, const char * const * source, QImage &image)
{
    if (!source)
        return true;

    QByteArray buf(200, 0);
    QByteArray state;

    int cpp, ncols, w, h, index = 0;

    if (device) {
        // "/* XPM */"
        int readBytes;
        if ((readBytes = device->readLine(buf.data(), buf.size())) < 0)
            return false;

        if (buf.indexOf("/* XPM") != 0) {
            while (readBytes > 0) {
                device->ungetChar(buf.at(readBytes - 1));
                --readBytes;
            }
            return false;
        }// bad magic
    }

    if (!read_xpm_header(device, source, index, state, &cpp, &ncols, &w, &h))
        return false;

    return read_xpm_body(device, source, index, state, cpp, ncols, w, h, image);
}

static const char* xpm_color_name(int cpp, int index)
{
    static char returnable[5];
    static const char code[] = ".#abcdefghijklmnopqrstuvwxyzABCD"
                               "EFGHIJKLMNOPQRSTUVWXYZ0123456789";
    // cpp is limited to 4 and index is limited to 64^cpp
    if (cpp > 1) {
        if (cpp > 2) {
            if (cpp > 3) {
                returnable[3] = code[index % 64];
                index /= 64;
            } else
                returnable[3] = '\0';
            returnable[2] = code[index % 64];
            index /= 64;
        } else
            returnable[2] = '\0';
        // the following 4 lines are a joke!
        if (index == 0)
            index = 64*44+21;
        else if (index == 64*44+21)
            index = 0;
        returnable[1] = code[index % 64];
        index /= 64;
    } else
        returnable[1] = '\0';
    returnable[0] = code[index];

    return returnable;
}


// write XPM image data
static bool write_xpm_image(const QImage &sourceImage, QIODevice *device, const QString &fileName)
{
    if (!device->isWritable())
        return false;

    QImage image;
    if (sourceImage.depth() != 32)
        image = sourceImage.convertToFormat(QImage::Format_RGB32);
    else
        image = sourceImage;

    QMap<QRgb, int> colorMap;

    int w = image.width(), h = image.height(), ncolors = 0;
    int x, y;

    // build color table
    for(y=0; y<h; y++) {
        QRgb * yp = (QRgb *)image.scanLine(y);
        for(x=0; x<w; x++) {
            QRgb color = *(yp + x);
            if (!colorMap.contains(color))
                colorMap.insert(color, ncolors++);
        }
    }

    // number of 64-bit characters per pixel needed to encode all colors
    int cpp = 1;
    for (int k = 64; ncolors > k; k *= 64) {
        ++cpp;
        // limit to 4 characters per pixel
        // 64^4 colors is enough for a 4096x4096 image
         if (cpp > 4)
            break;
    }

    QString line;

    // write header
    QTextStream s(device);
    s << "/* XPM */" << endl
      << "static char *" << fbname(fileName) << "[]={" << endl
      << '\"' << w << ' ' << h << ' ' << ncolors << ' ' << cpp << '\"';

    // write palette
    QMap<QRgb, int>::Iterator c = colorMap.begin();
    while (c != colorMap.end()) {
        QRgb color = c.key();
        if (image.format() != QImage::Format_RGB32 && !qAlpha(color))
            line.sprintf("\"%s c None\"",
                          xpm_color_name(cpp, *c));
        else
            line.sprintf("\"%s c #%02x%02x%02x\"",
                          xpm_color_name(cpp, *c),
                          qRed(color),
                          qGreen(color),
                          qBlue(color));
        ++c;
        s << ',' << endl << line;
    }

    // write pixels, limit to 4 characters per pixel
    line.truncate(cpp*w);
    for(y=0; y<h; y++) {
        QRgb * yp = (QRgb *) image.scanLine(y);
        int cc = 0;
        for(x=0; x<w; x++) {
            int color = (int)(*(yp + x));
            QByteArray chars(xpm_color_name(cpp, colorMap[color]));
            line[cc++] = QLatin1Char(chars[0]);
            if (cpp > 1) {
                line[cc++] = QLatin1Char(chars[1]);
                if (cpp > 2) {
                    line[cc++] = QLatin1Char(chars[2]);
                    if (cpp > 3) {
                        line[cc++] = QLatin1Char(chars[3]);
                    }
                }
            }
        }
        s << ',' << endl << '\"' << line << '\"';
    }
    s << "};" << endl;
    return (s.status() == QTextStream::Ok);
}

QXpmHandler::QXpmHandler()
    : state(Ready), index(0)
{
}

bool QXpmHandler::readHeader()
{
    state = Error;
    if (!read_xpm_header(device(), 0, index, buffer, &cpp, &ncols, &width, &height))
        return false;
    state = ReadHeader;
    return true;
}

bool QXpmHandler::readImage(QImage *image)
{
    if (state == Error)
        return false;

    if (state == Ready && !readHeader()) {
        state = Error;
        return false;
    }

    if (!read_xpm_body(device(), 0, index, buffer, cpp, ncols, width, height, *image)) {
        state = Error;
        return false;
    }

    state = Ready;
    return true;
}

bool QXpmHandler::canRead() const
{
    if (state == Ready && !canRead(device()))
        return false;

    if (state != Error) {
        setFormat("xpm");
        return true;
    }

    return false;
}

bool QXpmHandler::canRead(QIODevice *device)
{
    if (!device) {
        qWarning("QXpmHandler::canRead() called with no device");
        return false;
    }

    char head[6];
    if (device->peek(head, sizeof(head)) != sizeof(head))
        return false;

    return qstrncmp(head, "/* XPM", 6) == 0;
}

bool QXpmHandler::read(QImage *image)
{
    if (!canRead())
        return false;
    return readImage(image);
}

bool QXpmHandler::write(const QImage &image)
{
    return write_xpm_image(image, device(), fileName);
}

bool QXpmHandler::supportsOption(ImageOption option) const
{
    return option == Name
        || option == Size
        || option == ImageFormat;
}

QVariant QXpmHandler::option(ImageOption option) const
{
    if (option == Name) {
        return fileName;
    } else if (option == Size) {
        if (state == Error)
            return QVariant();
        if (state == Ready && !const_cast<QXpmHandler*>(this)->readHeader())
            return QVariant();
        return QSize(width, height);
    } else if (option == ImageFormat) {
        if (state == Error)
            return QVariant();
        if (state == Ready && !const_cast<QXpmHandler*>(this)->readHeader())
            return QVariant();
        // If we have more than 256 colors in the table, we need to
        // figure out, if it contains transparency. That means reading
        // the whole color table, which is too much work work pre-checking
        // the image format
        if (ncols <= 256)
            return QImage::Format_Indexed8;
        else
            return QImage::Format_Invalid;
    }

    return QVariant();
}

void QXpmHandler::setOption(ImageOption option, const QVariant &value)
{
    if (option == Name)
        fileName = value.toString();
}

QByteArray QXpmHandler::name() const
{
    return "xpm";
}

QT_END_NAMESPACE

#endif // QT_NO_IMAGEFORMAT_XPM
