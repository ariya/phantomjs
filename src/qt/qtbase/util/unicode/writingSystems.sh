#!/bin/sh
#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is the build configuration utility of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:LGPL$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3.0 as published by the Free Software
## Foundation and appearing in the file LICENSE.GPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU General Public License version 3.0 requirements will be
## met: http://www.gnu.org/copyleft/gpl.html.
##
##
## $QT_END_LICENSE$
##
#############################################################################

#
# This script generates the QFontDatabase::WritingSystem enum.  It
# uses the Unicode 4.0 Scripts.txt data file as the source, with the
# following modifications: 
#
# * Inherited is removed
# * East Asian scripts (chapter 11) are renamed to: SimplifiedChinese,
#   TraditionalChinese, Japanese, Korean, Vietnamese
# * Additiona Modern scripts (chapter 12) are removed
# * Archaic scripts (chapter 13) are removed

grep -Ev "(^[[:space:]]*#|^$)" data/Scripts.txt \
          | awk '{print $3}' \
          | grep -Ev "(Inherited|Hangul|Ogham|Old_Italic|Runic|Gothic|Ugaritic|Linear_B|Cypriot|Katakana_Or_Hiragana|Ethiopic|Mongolian|Osmanya|Cherokee|Canadian_Aboriginal|Deseret|Shavian)" \
          | sed -e s,_,,g -e 's,^Common$,Any,' -e 's,^Hiragana$,SimplifiedChinese NEWLINE TraditionalChinese,' -e 's,^Katakana$,Japanese,' -e 's,^Bopomofo$,Korean,' -e 's,^Han$,Vietnamese,' -e 's,^#$,,' \
          | uniq > writingSystems
echo "" >> writingSystems
echo "Other" >> writingSystems
