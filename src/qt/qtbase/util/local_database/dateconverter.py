#!/usr/bin/env python
#############################################################################
##
## Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
## Contact: http://www.qt-project.org/legal
##
## This file is part of the test suite of the Qt Toolkit.
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

import re

def _convert_pattern(pattern):
    # patterns from http://www.unicode.org/reports/tr35/#Date_Format_Patterns
    qt_regexps = {
        r"yyy{3,}" : "yyyy", # more that three digits hence convert to four-digit year
        r"L" : "M",          # stand-alone month names. not supported.
        r"g{1,}": "",        # modified julian day. not supported.
        r"S{1,}" : "",       # fractional seconds. not supported.
        r"A{1,}" : ""        # milliseconds in day. not supported.
    }
    qt_patterns = {
        "G" : "", "GG" : "", "GGG" : "", "GGGG" : "", "GGGGG" : "", # Era. not supported.
        "y" : "yyyy", # four-digit year without leading zeroes
        "Q" : "", "QQ" : "", "QQQ" : "", "QQQQ" : "", # quarter. not supported.
        "q" : "", "qq" : "", "qqq" : "", "qqqq" : "", # quarter. not supported.
        "MMMMM" : "MMM", # narrow month name.
        "LLLLL" : "MMM", # stand-alone narrow month name.
        "l" : "", # special symbol for chinese leap month. not supported.
        "w" : "", "W" : "", # week of year/month. not supported.
        "D" : "", "DD" : "", "DDD" : "", # day of year. not supported.
        "F" : "", # day of week in month. not supported.
        "E" : "ddd", "EE" : "ddd", "EEE" : "ddd", "EEEEE" : "ddd", "EEEE" : "dddd", # day of week
        "e" : "ddd", "ee" : "ddd", "eee" : "ddd", "eeeee" : "ddd", "eeee" : "dddd", # local day of week
        "c" : "ddd", "cc" : "ddd", "ccc" : "ddd", "ccccc" : "ddd", "cccc" : "dddd", # stand-alone local day of week
        "a" : "AP", # AM/PM
        "K" : "h", # Hour 0-11
        "k" : "H", # Hour 1-24
        "j" : "", # special reserved symbol.
        "z" : "t", "zz" : "t", "zzz" : "t", "zzzz" : "t", # timezone
        "Z" : "t", "ZZ" : "t", "ZZZ" : "t", "ZZZZ" : "t", # timezone
        "v" : "t", "vv" : "t", "vvv" : "t", "vvvv" : "t", # timezone
        "V" : "t", "VV" : "t", "VVV" : "t", "VVVV" : "t"  # timezone
    }
    if qt_patterns.has_key(pattern):
        return qt_patterns[pattern]
    for r,v in qt_regexps.items():
        pattern = re.sub(r, v, pattern)
    return pattern

def convert_date(input):
    result = ""
    patterns = "GyYuQqMLlwWdDFgEecahHKkjmsSAzZvV"
    last = ""
    inquote = 0
    chars_to_strip = " -"
    for c in input:
        if c == "'":
            inquote = inquote + 1
        if inquote % 2 == 0:
            if c in patterns:
                if not last:
                    last = c
                else:
                    if c in last:
                        last += c
                    else:
                        # pattern changed
                        converted = _convert_pattern(last)
                        result += converted
                        if not converted:
                            result = result.rstrip(chars_to_strip)
                        last = c
                continue
        if last:
            # pattern ended
            converted = _convert_pattern(last)
            result += converted
            if not converted:
                result = result.rstrip(chars_to_strip)
            last = ""
        result += c
    if last:
        converted = _convert_pattern(last)
        result += converted
        if not converted:
            result = result.rstrip(chars_to_strip)
    return result.lstrip(chars_to_strip)
