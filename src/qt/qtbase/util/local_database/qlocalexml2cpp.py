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

import os
import sys
import tempfile
import datetime
import xml.dom.minidom

class Error:
    def __init__(self, msg):
        self.msg = msg
    def __str__(self):
        return self.msg

def wrap_list(lst):
    def split(lst, size):
        for i in range(len(lst)/size+1):
            yield lst[i*size:(i+1)*size]
    return ",\n".join(map(lambda x: ", ".join(x), split(lst, 20)))

def firstChildElt(parent, name):
    child = parent.firstChild
    while child:
        if child.nodeType == parent.ELEMENT_NODE \
            and (not name or child.nodeName == name):
            return child
        child = child.nextSibling
    return False

def nextSiblingElt(sibling, name):
    sib = sibling.nextSibling
    while sib:
        if sib.nodeType == sibling.ELEMENT_NODE \
            and (not name or sib.nodeName == name):
            return sib
        sib = sib.nextSibling
    return False

def eltText(elt):
    result = ""
    child = elt.firstChild
    while child:
        if child.nodeType == elt.TEXT_NODE:
            if result:
                result += " "
            result += child.nodeValue
        child = child.nextSibling
    return result

def loadLanguageMap(doc):
    result = {}

    language_list_elt = firstChildElt(doc.documentElement, "languageList")
    language_elt = firstChildElt(language_list_elt, "language")
    while language_elt:
        language_id = int(eltText(firstChildElt(language_elt, "id")))
        language_name = eltText(firstChildElt(language_elt, "name"))
        language_code = eltText(firstChildElt(language_elt, "code"))
        result[language_id] = (language_name, language_code)
        language_elt = nextSiblingElt(language_elt, "language")

    return result

def loadScriptMap(doc):
    result = {}

    script_list_elt = firstChildElt(doc.documentElement, "scriptList")
    script_elt = firstChildElt(script_list_elt, "script")
    while script_elt:
        script_id = int(eltText(firstChildElt(script_elt, "id")))
        script_name = eltText(firstChildElt(script_elt, "name"))
        script_code = eltText(firstChildElt(script_elt, "code"))
        result[script_id] = (script_name, script_code)
        script_elt = nextSiblingElt(script_elt, "script")

    return result

def loadCountryMap(doc):
    result = {}

    country_list_elt = firstChildElt(doc.documentElement, "countryList")
    country_elt = firstChildElt(country_list_elt, "country")
    while country_elt:
        country_id = int(eltText(firstChildElt(country_elt, "id")))
        country_name = eltText(firstChildElt(country_elt, "name"))
        country_code = eltText(firstChildElt(country_elt, "code"))
        result[country_id] = (country_name, country_code)
        country_elt = nextSiblingElt(country_elt, "country")

    return result

def loadLikelySubtagsMap(doc):
    result = {}

    i = 0
    list_elt = firstChildElt(doc.documentElement, "likelySubtags")
    elt = firstChildElt(list_elt, "likelySubtag")
    while elt:
        elt_from = firstChildElt(elt, "from")
        from_language = eltText(firstChildElt(elt_from, "language"));
        from_script = eltText(firstChildElt(elt_from, "script"));
        from_country = eltText(firstChildElt(elt_from, "country"));

        elt_to = firstChildElt(elt, "to")
        to_language = eltText(firstChildElt(elt_to, "language"));
        to_script = eltText(firstChildElt(elt_to, "script"));
        to_country = eltText(firstChildElt(elt_to, "country"));

        tmp = {}
        tmp["from"] = (from_language, from_script, from_country)
        tmp["to"] = (to_language, to_script, to_country)
        result[i] = tmp;
        i += 1
        elt = nextSiblingElt(elt, "likelySubtag");
    return result

def fixedScriptName(name, dupes):
    name = name.replace(" ", "")
    if name[-6:] != "Script":
        name = name + "Script";
    if name in dupes:
        sys.stderr.write("\n\n\nERROR: The script name '%s' is messy" % name)
        sys.exit(1);
    return name

def fixedCountryName(name, dupes):
    if name in dupes:
        return name.replace(" ", "") + "Country"
    return name.replace(" ", "")

def fixedLanguageName(name, dupes):
    if name in dupes:
        return name.replace(" ", "") + "Language"
    return name.replace(" ", "")

def findDupes(country_map, language_map):
    country_set = set([ v[0] for a, v in country_map.iteritems() ])
    language_set = set([ v[0] for a, v in language_map.iteritems() ])
    return country_set & language_set

def languageNameToId(name, language_map):
    for key in language_map.keys():
        if language_map[key][0] == name:
            return key
    return -1

def scriptNameToId(name, script_map):
    for key in script_map.keys():
        if script_map[key][0] == name:
            return key
    return -1

def countryNameToId(name, country_map):
    for key in country_map.keys():
        if country_map[key][0] == name:
            return key
    return -1

def convertFormat(format):
    result = ""
    i = 0
    while i < len(format):
        if format[i] == "'":
            result += "'"
            i += 1
            while i < len(format) and format[i] != "'":
                result += format[i]
                i += 1
            if i < len(format):
                result += "'"
                i += 1
        else:
            s = format[i:]
            if s.startswith("EEEE"):
                result += "dddd"
                i += 4
            elif s.startswith("EEE"):
                result += "ddd"
                i += 3
            elif s.startswith("a"):
                result += "AP"
                i += 1
            elif s.startswith("z"):
                result += "t"
                i += 1
            elif s.startswith("v"):
                i += 1
            else:
                result += format[i]
                i += 1

    return result

def convertToQtDayOfWeek(firstDay):
    qtDayOfWeek = {"mon":1, "tue":2, "wed":3, "thu":4, "fri":5, "sat":6, "sun":7}
    return qtDayOfWeek[firstDay]

def assertSingleChar(string):
    assert len(string) == 1, "This string is not allowed to be longer than 1 character"
    return string

class Locale:
    def __init__(self, elt):
        self.language = eltText(firstChildElt(elt, "language"))
        self.languageEndonym = eltText(firstChildElt(elt, "languageEndonym"))
        self.script = eltText(firstChildElt(elt, "script"))
        self.country = eltText(firstChildElt(elt, "country"))
        self.countryEndonym = eltText(firstChildElt(elt, "countryEndonym"))
        self.decimal = int(eltText(firstChildElt(elt, "decimal")))
        self.group = int(eltText(firstChildElt(elt, "group")))
        self.listDelim = int(eltText(firstChildElt(elt, "list")))
        self.percent = int(eltText(firstChildElt(elt, "percent")))
        self.zero = int(eltText(firstChildElt(elt, "zero")))
        self.minus = int(eltText(firstChildElt(elt, "minus")))
        self.plus = int(eltText(firstChildElt(elt, "plus")))
        self.exp = int(eltText(firstChildElt(elt, "exp")))
        self.quotationStart = ord(assertSingleChar(eltText(firstChildElt(elt, "quotationStart"))))
        self.quotationEnd = ord(assertSingleChar(eltText(firstChildElt(elt, "quotationEnd"))))
        self.alternateQuotationStart = ord(assertSingleChar(eltText(firstChildElt(elt, "alternateQuotationStart"))))
        self.alternateQuotationEnd = ord(assertSingleChar(eltText(firstChildElt(elt, "alternateQuotationEnd"))))
        self.listPatternPartStart = eltText(firstChildElt(elt, "listPatternPartStart"))
        self.listPatternPartMiddle = eltText(firstChildElt(elt, "listPatternPartMiddle"))
        self.listPatternPartEnd = eltText(firstChildElt(elt, "listPatternPartEnd"))
        self.listPatternPartTwo = eltText(firstChildElt(elt, "listPatternPartTwo"))
        self.am = eltText(firstChildElt(elt, "am"))
        self.pm = eltText(firstChildElt(elt, "pm"))
        self.firstDayOfWeek = convertToQtDayOfWeek(eltText(firstChildElt(elt, "firstDayOfWeek")))
        self.weekendStart = convertToQtDayOfWeek(eltText(firstChildElt(elt, "weekendStart")))
        self.weekendEnd = convertToQtDayOfWeek(eltText(firstChildElt(elt, "weekendEnd")))
        self.longDateFormat = convertFormat(eltText(firstChildElt(elt, "longDateFormat")))
        self.shortDateFormat = convertFormat(eltText(firstChildElt(elt, "shortDateFormat")))
        self.longTimeFormat = convertFormat(eltText(firstChildElt(elt, "longTimeFormat")))
        self.shortTimeFormat = convertFormat(eltText(firstChildElt(elt, "shortTimeFormat")))
        self.standaloneLongMonths = eltText(firstChildElt(elt, "standaloneLongMonths"))
        self.standaloneShortMonths = eltText(firstChildElt(elt, "standaloneShortMonths"))
        self.standaloneNarrowMonths = eltText(firstChildElt(elt, "standaloneNarrowMonths"))
        self.longMonths = eltText(firstChildElt(elt, "longMonths"))
        self.shortMonths = eltText(firstChildElt(elt, "shortMonths"))
        self.narrowMonths = eltText(firstChildElt(elt, "narrowMonths"))
        self.standaloneLongDays = eltText(firstChildElt(elt, "standaloneLongDays"))
        self.standaloneShortDays = eltText(firstChildElt(elt, "standaloneShortDays"))
        self.standaloneNarrowDays = eltText(firstChildElt(elt, "standaloneNarrowDays"))
        self.longDays = eltText(firstChildElt(elt, "longDays"))
        self.shortDays = eltText(firstChildElt(elt, "shortDays"))
        self.narrowDays = eltText(firstChildElt(elt, "narrowDays"))
        self.currencyIsoCode = eltText(firstChildElt(elt, "currencyIsoCode"))
        self.currencySymbol = eltText(firstChildElt(elt, "currencySymbol"))
        self.currencyDisplayName = eltText(firstChildElt(elt, "currencyDisplayName"))
        self.currencyDigits = int(eltText(firstChildElt(elt, "currencyDigits")))
        self.currencyRounding = int(eltText(firstChildElt(elt, "currencyRounding")))
        self.currencyFormat = eltText(firstChildElt(elt, "currencyFormat"))
        self.currencyNegativeFormat = eltText(firstChildElt(elt, "currencyNegativeFormat"))

def loadLocaleMap(doc, language_map, script_map, country_map, likely_subtags_map):
    result = {}

    locale_list_elt = firstChildElt(doc.documentElement, "localeList")
    locale_elt = firstChildElt(locale_list_elt, "locale")
    while locale_elt:
        locale = Locale(locale_elt)
        language_id = languageNameToId(locale.language, language_map)
        if language_id == -1:
            sys.stderr.write("Cannot find a language id for '%s'\n" % locale.language)
        script_id = scriptNameToId(locale.script, script_map)
        if script_id == -1:
            sys.stderr.write("Cannot find a script id for '%s'\n" % locale.script)
        country_id = countryNameToId(locale.country, country_map)
        if country_id == -1:
            sys.stderr.write("Cannot find a country id for '%s'\n" % locale.country)

        if language_id != 1: # C
            if country_id == 0:
                sys.stderr.write("loadLocaleMap: No country id for '%s'\n" % locale.language)

            if script_id == 0:
                # find default script for a given language and country (see http://www.unicode.org/reports/tr35/#Likely_Subtags)
                for key in likely_subtags_map.keys():
                    tmp = likely_subtags_map[key]
                    if tmp["from"][0] == locale.language and tmp["from"][1] == "AnyScript" and tmp["from"][2] == locale.country:
                        locale.script = tmp["to"][1]
                        script_id = scriptNameToId(locale.script, script_map)
                        break
            if script_id == 0 and country_id != 0:
                # try with no country
                for key in likely_subtags_map.keys():
                    tmp = likely_subtags_map[key]
                    if tmp["from"][0] == locale.language and tmp["from"][1] == "AnyScript" and tmp["from"][2] == "AnyCountry":
                        locale.script = tmp["to"][1]
                        script_id = scriptNameToId(locale.script, script_map)
                        break

        result[(language_id, script_id, country_id)] = locale

        locale_elt = nextSiblingElt(locale_elt, "locale")

    return result

def compareLocaleKeys(key1, key2):
    if key1 == key2:
        return 0

    if key1[0] == key2[0]:
        l1 = compareLocaleKeys.locale_map[key1]
        l2 = compareLocaleKeys.locale_map[key2]

        if (l1.language, l1.script) in compareLocaleKeys.default_map.keys():
            default = compareLocaleKeys.default_map[(l1.language, l1.script)]
            if l1.country == default:
                return -1
            if l2.country == default:
                return 1

        if key1[1] != key2[1]:
            if (l2.language, l2.script) in compareLocaleKeys.default_map.keys():
                default = compareLocaleKeys.default_map[(l2.language, l2.script)]
                if l2.country == default:
                    return 1
                if l1.country == default:
                    return -1

        if key1[1] != key2[1]:
            return key1[1] - key2[1]
    else:
        return key1[0] - key2[0]

    return key1[2] - key2[2]


def languageCount(language_id, locale_map):
    result = 0
    for key in locale_map.keys():
        if key[0] == language_id:
            result += 1
    return result

def unicode2hex(s):
    lst = []
    for x in s:
        v = ord(x)
        if v > 0xFFFF:
            # make a surrogate pair
            # copied from qchar.h
            high = (v >> 10) + 0xd7c0
            low = (v % 0x400 + 0xdc00)
            lst.append(hex(high))
            lst.append(hex(low))
        else:
            lst.append(hex(v))
    return lst

class StringDataToken:
    def __init__(self, index, length):
        if index > 0xFFFF or length > 0xFFFF:
            raise Error("Position exceeds ushort range: %d,%d " % (index, length))
        self.index = index
        self.length = length
    def __str__(self):
        return " %d,%d " % (self.index, self.length)

class StringData:
    def __init__(self):
        self.data = []
        self.hash = {}
    def append(self, s):
        if s in self.hash:
            return self.hash[s]

        lst = unicode2hex(s)
        index = len(self.data)
        if index > 65535:
            print "\n\n\n#error Data index is too big!"
            sys.stderr.write ("\n\n\nERROR: index exceeds the uint16 range! index = %d\n" % index)
            sys.exit(1)
        size = len(lst)
        if size >= 65535:
            print "\n\n\n#error Data is too big!"
            sys.stderr.write ("\n\n\nERROR: data size exceeds the uint16 range! size = %d\n" % size)
            sys.exit(1)
        token = None
        try:
            token = StringDataToken(index, size)
        except Error as e:
            sys.stderr.write("\n\n\nERROR: %s: on data '%s'" % (e, s))
            sys.exit(1)
        self.hash[s] = token
        self.data += lst
        return token

def escapedString(s):
    result = ""
    i = 0
    while i < len(s):
        if s[i] == '"':
            result += '\\"'
            i += 1
        else:
            result += s[i]
            i += 1
    s = result

    line = ""
    need_escape = False
    result = ""
    for c in s:
        if ord(c) < 128 and (not need_escape or ord(c.lower()) < ord('a') or ord(c.lower()) > ord('f')):
            line += c
            need_escape = False
        else:
            line += "\\x%02x" % (ord(c))
            need_escape = True
        if len(line) > 80:
            result = result + "\n" + "\"" + line + "\""
            line = ""
    line += "\\0"
    result = result + "\n" + "\"" + line + "\""
    if result[0] == "\n":
        result = result[1:]
    return result

def printEscapedString(s):
    print escapedString(s);


def currencyIsoCodeData(s):
    if s:
        return ",".join(map(lambda x: str(ord(x)), s))
    return "0,0,0"

def usage():
    print "Usage: qlocalexml2cpp.py <path-to-locale.xml> <path-to-qt-src-tree>"
    sys.exit(1)

GENERATED_BLOCK_START = "// GENERATED PART STARTS HERE\n"
GENERATED_BLOCK_END = "// GENERATED PART ENDS HERE\n"

def main():
    if len(sys.argv) != 3:
        usage()

    localexml = sys.argv[1]
    qtsrcdir = sys.argv[2]

    if not os.path.exists(qtsrcdir) or not os.path.exists(qtsrcdir):
        usage()
    if not os.path.isfile(qtsrcdir + "/src/corelib/tools/qlocale_data_p.h"):
        usage()
    if not os.path.isfile(qtsrcdir + "/src/corelib/tools/qlocale.h"):
        usage()
    if not os.path.isfile(qtsrcdir + "/src/corelib/tools/qlocale.qdoc"):
        usage()

    (data_temp_file, data_temp_file_path) = tempfile.mkstemp("qlocale_data_p", dir=qtsrcdir)
    data_temp_file = os.fdopen(data_temp_file, "w")
    qlocaledata_file = open(qtsrcdir + "/src/corelib/tools/qlocale_data_p.h", "r")
    s = qlocaledata_file.readline()
    while s and s != GENERATED_BLOCK_START:
        data_temp_file.write(s)
        s = qlocaledata_file.readline()
    data_temp_file.write(GENERATED_BLOCK_START)

    doc = xml.dom.minidom.parse(localexml)
    language_map = loadLanguageMap(doc)
    script_map = loadScriptMap(doc)
    country_map = loadCountryMap(doc)
    likely_subtags_map = loadLikelySubtagsMap(doc)
    default_map = {}
    for key in likely_subtags_map.keys():
        tmp = likely_subtags_map[key]
        if tmp["from"][1] == "AnyScript" and tmp["from"][2] == "AnyCountry" and tmp["to"][2] != "AnyCountry":
            default_map[(tmp["to"][0], tmp["to"][1])] = tmp["to"][2]
    locale_map = loadLocaleMap(doc, language_map, script_map, country_map, likely_subtags_map)
    dupes = findDupes(language_map, country_map)

    cldr_version = eltText(firstChildElt(doc.documentElement, "version"))

    data_temp_file.write("\n\
/*\n\
    This part of the file was generated on %s from the\n\
    Common Locale Data Repository v%s\n\
\n\
    http://www.unicode.org/cldr/\n\
\n\
    Do not change it, instead edit CLDR data and regenerate this file using\n\
    cldr2qlocalexml.py and qlocalexml2cpp.py.\n\
*/\n\n\n\
" % (str(datetime.date.today()), cldr_version) )

    # Likely subtags map
    data_temp_file.write("static const QLocaleId likely_subtags[] = {\n")
    index = 0
    for key in likely_subtags_map.keys():
        tmp = likely_subtags_map[key]
        from_language = languageNameToId(tmp["from"][0], language_map)
        from_script = scriptNameToId(tmp["from"][1], script_map)
        from_country = countryNameToId(tmp["from"][2], country_map)
        to_language = languageNameToId(tmp["to"][0], language_map)
        to_script = scriptNameToId(tmp["to"][1], script_map)
        to_country = countryNameToId(tmp["to"][2], country_map)

        cmnt_from = ""
        if from_language != 0:
            cmnt_from = cmnt_from + language_map[from_language][1]
        else:
            cmnt_from = cmnt_from + "und"
        if from_script != 0:
            if cmnt_from:
                cmnt_from = cmnt_from + "_"
            cmnt_from = cmnt_from + script_map[from_script][1]
        if from_country != 0:
            if cmnt_from:
                cmnt_from = cmnt_from + "_"
            cmnt_from = cmnt_from + country_map[from_country][1]
        cmnt_to = ""
        if to_language != 0:
            cmnt_to = cmnt_to + language_map[to_language][1]
        else:
            cmnt_to = cmnt_to + "und"
        if to_script != 0:
            if cmnt_to:
                cmnt_to = cmnt_to + "_"
            cmnt_to = cmnt_to + script_map[to_script][1]
        if to_country != 0:
            if cmnt_to:
                cmnt_to = cmnt_to + "_"
            cmnt_to = cmnt_to + country_map[to_country][1]

        data_temp_file.write("    ")
        data_temp_file.write("{ %3d, %2d, %3d }, { %3d, %2d, %3d }" % (from_language, from_script, from_country, to_language, to_script, to_country))
        index += 1
        if index != len(likely_subtags_map):
            data_temp_file.write(",")
        else:
            data_temp_file.write(" ")
        data_temp_file.write(" // %s -> %s\n" % (cmnt_from, cmnt_to))
    data_temp_file.write("};\n")

    data_temp_file.write("\n")

    # Locale index
    data_temp_file.write("static const quint16 locale_index[] = {\n")
    index = 0
    for key in language_map.keys():
        i = 0
        count = languageCount(key, locale_map)
        if count > 0:
            i = index
            index += count
        data_temp_file.write("%6d, // %s\n" % (i, language_map[key][0]))
    data_temp_file.write("     0 // trailing 0\n")
    data_temp_file.write("};\n")

    data_temp_file.write("\n")

    list_pattern_part_data = StringData()
    date_format_data = StringData()
    time_format_data = StringData()
    months_data = StringData()
    days_data = StringData()
    am_data = StringData()
    pm_data = StringData()
    currency_symbol_data = StringData()
    currency_display_name_data = StringData()
    currency_format_data = StringData()
    endonyms_data = StringData()

    # Locale data
    data_temp_file.write("static const QLocaleData locale_data[] = {\n")
    data_temp_file.write("//      lang   script terr    dec  group   list  prcnt   zero  minus  plus    exp quotStart quotEnd altQuotStart altQuotEnd lpStart lpMid lpEnd lpTwo sDtFmt lDtFmt sTmFmt lTmFmt ssMonth slMonth  sMonth lMonth  sDays  lDays  am,len      pm,len\n")

    locale_keys = locale_map.keys()
    compareLocaleKeys.default_map = default_map
    compareLocaleKeys.locale_map = locale_map
    locale_keys.sort(compareLocaleKeys)

    for key in locale_keys:
        l = locale_map[key]
        data_temp_file.write("    { %6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s, {%s}, %s,%s,%s,%s,%s,%s,%6d,%6d,%6d,%6d,%6d }, // %s/%s/%s\n" \
                    % (key[0], key[1], key[2],
                        l.decimal,
                        l.group,
                        l.listDelim,
                        l.percent,
                        l.zero,
                        l.minus,
                        l.plus,
                        l.exp,
                        l.quotationStart,
                        l.quotationEnd,
                        l.alternateQuotationStart,
                        l.alternateQuotationEnd,
                        list_pattern_part_data.append(l.listPatternPartStart),
                        list_pattern_part_data.append(l.listPatternPartMiddle),
                        list_pattern_part_data.append(l.listPatternPartEnd),
                        list_pattern_part_data.append(l.listPatternPartTwo),
                        date_format_data.append(l.shortDateFormat),
                        date_format_data.append(l.longDateFormat),
                        time_format_data.append(l.shortTimeFormat),
                        time_format_data.append(l.longTimeFormat),
                        months_data.append(l.standaloneShortMonths),
                        months_data.append(l.standaloneLongMonths),
                        months_data.append(l.standaloneNarrowMonths),
                        months_data.append(l.shortMonths),
                        months_data.append(l.longMonths),
                        months_data.append(l.narrowMonths),
                        days_data.append(l.standaloneShortDays),
                        days_data.append(l.standaloneLongDays),
                        days_data.append(l.standaloneNarrowDays),
                        days_data.append(l.shortDays),
                        days_data.append(l.longDays),
                        days_data.append(l.narrowDays),
                        am_data.append(l.am),
                        pm_data.append(l.pm),
                        currencyIsoCodeData(l.currencyIsoCode),
                        currency_symbol_data.append(l.currencySymbol),
                        currency_display_name_data.append(l.currencyDisplayName),
                        currency_format_data.append(l.currencyFormat),
                        currency_format_data.append(l.currencyNegativeFormat),
                        endonyms_data.append(l.languageEndonym),
                        endonyms_data.append(l.countryEndonym),
                        l.currencyDigits,
                        l.currencyRounding,
                        l.firstDayOfWeek,
                        l.weekendStart,
                        l.weekendEnd,
                        l.language,
                        l.script,
                        l.country))
    data_temp_file.write("    {      0,      0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,    0,0,    0,0,    0,0,   0,0,     0,0,     0,0,     0,0,     0,0,     0,0,     0,0,     0,0,    0,0,    0,0,    0,0,   0,0,   0,0,   0,0,   0,0,   0,0,   0,0,   0,0,   0,0, {0,0,0}, 0,0, 0,0, 0,0, 0,0, 0, 0, 0, 0, 0, 0,0, 0,0 }  // trailing 0s\n")
    data_temp_file.write("};\n")

    data_temp_file.write("\n")

    # List patterns data
    data_temp_file.write("static const ushort list_pattern_part_data[] = {\n")
    data_temp_file.write(wrap_list(list_pattern_part_data.data))
    data_temp_file.write("\n};\n")

    data_temp_file.write("\n")

    # Date format data
    data_temp_file.write("static const ushort date_format_data[] = {\n")
    data_temp_file.write(wrap_list(date_format_data.data))
    data_temp_file.write("\n};\n")

    data_temp_file.write("\n")

    # Time format data
    data_temp_file.write("static const ushort time_format_data[] = {\n")
    data_temp_file.write(wrap_list(time_format_data.data))
    data_temp_file.write("\n};\n")

    data_temp_file.write("\n")

    # Months data
    data_temp_file.write("static const ushort months_data[] = {\n")
    data_temp_file.write(wrap_list(months_data.data))
    data_temp_file.write("\n};\n")

    data_temp_file.write("\n")

    # Days data
    data_temp_file.write("static const ushort days_data[] = {\n")
    data_temp_file.write(wrap_list(days_data.data))
    data_temp_file.write("\n};\n")

    data_temp_file.write("\n")

    # AM data
    data_temp_file.write("static const ushort am_data[] = {\n")
    data_temp_file.write(wrap_list(am_data.data))
    data_temp_file.write("\n};\n")

    data_temp_file.write("\n")

    # PM data
    data_temp_file.write("static const ushort pm_data[] = {\n")
    data_temp_file.write(wrap_list(pm_data.data))
    data_temp_file.write("\n};\n")

    data_temp_file.write("\n")

    # Currency symbol data
    data_temp_file.write("static const ushort currency_symbol_data[] = {\n")
    data_temp_file.write(wrap_list(currency_symbol_data.data))
    data_temp_file.write("\n};\n")

    data_temp_file.write("\n")

    # Currency display name data
    data_temp_file.write("static const ushort currency_display_name_data[] = {\n")
    data_temp_file.write(wrap_list(currency_display_name_data.data))
    data_temp_file.write("\n};\n")

    data_temp_file.write("\n")

    # Currency format data
    data_temp_file.write("static const ushort currency_format_data[] = {\n")
    data_temp_file.write(wrap_list(currency_format_data.data))
    data_temp_file.write("\n};\n")

    # Endonyms data
    data_temp_file.write("static const ushort endonyms_data[] = {\n")
    data_temp_file.write(wrap_list(endonyms_data.data))
    data_temp_file.write("\n};\n")

    data_temp_file.write("\n")

    # Language name list
    data_temp_file.write("static const char language_name_list[] =\n")
    data_temp_file.write("\"Default\\0\"\n")
    for key in language_map.keys():
        if key == 0:
            continue
        data_temp_file.write("\"" + language_map[key][0] + "\\0\"\n")
    data_temp_file.write(";\n")

    data_temp_file.write("\n")

    # Language name index
    data_temp_file.write("static const quint16 language_name_index[] = {\n")
    data_temp_file.write("     0, // AnyLanguage\n")
    index = 8
    for key in language_map.keys():
        if key == 0:
            continue
        language = language_map[key][0]
        data_temp_file.write("%6d, // %s\n" % (index, language))
        index += len(language) + 1
    data_temp_file.write("};\n")

    data_temp_file.write("\n")

    # Script name list
    data_temp_file.write("static const char script_name_list[] =\n")
    data_temp_file.write("\"Default\\0\"\n")
    for key in script_map.keys():
        if key == 0:
            continue
        data_temp_file.write("\"" + script_map[key][0] + "\\0\"\n")
    data_temp_file.write(";\n")

    data_temp_file.write("\n")

    # Script name index
    data_temp_file.write("static const quint16 script_name_index[] = {\n")
    data_temp_file.write("     0, // AnyScript\n")
    index = 8
    for key in script_map.keys():
        if key == 0:
            continue
        script = script_map[key][0]
        data_temp_file.write("%6d, // %s\n" % (index, script))
        index += len(script) + 1
    data_temp_file.write("};\n")

    data_temp_file.write("\n")

    # Country name list
    data_temp_file.write("static const char country_name_list[] =\n")
    data_temp_file.write("\"Default\\0\"\n")
    for key in country_map.keys():
        if key == 0:
            continue
        data_temp_file.write("\"" + country_map[key][0] + "\\0\"\n")
    data_temp_file.write(";\n")

    data_temp_file.write("\n")

    # Country name index
    data_temp_file.write("static const quint16 country_name_index[] = {\n")
    data_temp_file.write("     0, // AnyCountry\n")
    index = 8
    for key in country_map.keys():
        if key == 0:
            continue
        country = country_map[key][0]
        data_temp_file.write("%6d, // %s\n" % (index, country))
        index += len(country) + 1
    data_temp_file.write("};\n")

    data_temp_file.write("\n")

    # Language code list
    data_temp_file.write("static const unsigned char language_code_list[] =\n")
    for key in language_map.keys():
        code = language_map[key][1]
        if len(code) == 2:
            code += r"\0"
        data_temp_file.write("\"%2s\" // %s\n" % (code, language_map[key][0]))
    data_temp_file.write(";\n")

    data_temp_file.write("\n")

    # Script code list
    data_temp_file.write("static const unsigned char script_code_list[] =\n")
    for key in script_map.keys():
        code = script_map[key][1]
        for i in range(4 - len(code)):
            code += "\\0"
        data_temp_file.write("\"%2s\" // %s\n" % (code, script_map[key][0]))
    data_temp_file.write(";\n")

    # Country code list
    data_temp_file.write("static const unsigned char country_code_list[] =\n")
    for key in country_map.keys():
        code = country_map[key][1]
        if len(code) == 2:
            code += "\\0"
        data_temp_file.write("\"%2s\" // %s\n" % (code, country_map[key][0]))
    data_temp_file.write(";\n")

    data_temp_file.write("\n")
    data_temp_file.write(GENERATED_BLOCK_END)
    s = qlocaledata_file.readline()
    # skip until end of the block
    while s and s != GENERATED_BLOCK_END:
        s = qlocaledata_file.readline()

    s = qlocaledata_file.readline()
    while s:
        data_temp_file.write(s)
        s = qlocaledata_file.readline()
    data_temp_file.close()
    qlocaledata_file.close()

    os.remove(qtsrcdir + "/src/corelib/tools/qlocale_data_p.h")
    os.rename(data_temp_file_path, qtsrcdir + "/src/corelib/tools/qlocale_data_p.h")

    # qlocale.h

    (qlocaleh_temp_file, qlocaleh_temp_file_path) = tempfile.mkstemp("qlocale.h", dir=qtsrcdir)
    qlocaleh_temp_file = os.fdopen(qlocaleh_temp_file, "w")
    qlocaleh_file = open(qtsrcdir + "/src/corelib/tools/qlocale.h", "r")
    s = qlocaleh_file.readline()
    while s and s != GENERATED_BLOCK_START:
        qlocaleh_temp_file.write(s)
        s = qlocaleh_file.readline()
    qlocaleh_temp_file.write(GENERATED_BLOCK_START)
    qlocaleh_temp_file.write("// see qlocale_data_p.h for more info on generated data\n")

    # Language enum
    qlocaleh_temp_file.write("    enum Language {\n")
    language = ""
    for key in language_map.keys():
        language = fixedLanguageName(language_map[key][0], dupes)
        qlocaleh_temp_file.write("        " + language + " = " + str(key) + ",\n")
    # legacy. should disappear at some point
    qlocaleh_temp_file.write("        Norwegian = NorwegianBokmal,\n")
    qlocaleh_temp_file.write("        Moldavian = Romanian,\n")
    qlocaleh_temp_file.write("        SerboCroatian = Serbian,\n")
    qlocaleh_temp_file.write("        Tagalog = Filipino,\n")
    qlocaleh_temp_file.write("        Twi = Akan,\n")
    # renamings
    qlocaleh_temp_file.write("        Afan = Oromo,\n")
    qlocaleh_temp_file.write("        Byelorussian = Belarusian,\n")
    qlocaleh_temp_file.write("        Bhutani = Dzongkha,\n")
    qlocaleh_temp_file.write("        Cambodian = Khmer,\n")
    qlocaleh_temp_file.write("        Kurundi = Rundi,\n")
    qlocaleh_temp_file.write("        RhaetoRomance = Romansh,\n")
    qlocaleh_temp_file.write("        Chewa = Nyanja,\n")
    qlocaleh_temp_file.write("        Frisian = WesternFrisian,\n")
    qlocaleh_temp_file.write("        Uigur = Uighur,\n")
    qlocaleh_temp_file.write("        LastLanguage = " + language + "\n")
    qlocaleh_temp_file.write("    };\n")

    qlocaleh_temp_file.write("\n")

    # Script enum
    qlocaleh_temp_file.write("    enum Script {\n")
    script = ""
    for key in script_map.keys():
        script = fixedScriptName(script_map[key][0], dupes)
        qlocaleh_temp_file.write("        " + script + " = " + str(key) + ",\n")
    # renamings
    qlocaleh_temp_file.write("        SimplifiedChineseScript = SimplifiedHanScript,\n")
    qlocaleh_temp_file.write("        TraditionalChineseScript = TraditionalHanScript,\n")
    qlocaleh_temp_file.write("        LastScript = " + script + "\n")
    qlocaleh_temp_file.write("    };\n")

    # Country enum
    qlocaleh_temp_file.write("    enum Country {\n")
    country = ""
    for key in country_map.keys():
        country = fixedCountryName(country_map[key][0], dupes)
        qlocaleh_temp_file.write("        " + country + " = " + str(key) + ",\n")
    # renamings
    qlocaleh_temp_file.write("        DemocraticRepublicOfCongo = CongoKinshasa,\n")
    qlocaleh_temp_file.write("        PeoplesRepublicOfCongo = CongoBrazzaville,\n")
    qlocaleh_temp_file.write("        DemocraticRepublicOfKorea = NorthKorea,\n")
    qlocaleh_temp_file.write("        RepublicOfKorea = SouthKorea,\n")
    qlocaleh_temp_file.write("        RussianFederation = Russia,\n")
    qlocaleh_temp_file.write("        SyrianArabRepublic = Syria,\n")
    qlocaleh_temp_file.write("        LastCountry = " + country + "\n")
    qlocaleh_temp_file.write("    };\n")

    qlocaleh_temp_file.write(GENERATED_BLOCK_END)
    s = qlocaleh_file.readline()
    # skip until end of the block
    while s and s != GENERATED_BLOCK_END:
        s = qlocaleh_file.readline()

    s = qlocaleh_file.readline()
    while s:
        qlocaleh_temp_file.write(s)
        s = qlocaleh_file.readline()
    qlocaleh_temp_file.close()
    qlocaleh_file.close()

    os.remove(qtsrcdir + "/src/corelib/tools/qlocale.h")
    os.rename(qlocaleh_temp_file_path, qtsrcdir + "/src/corelib/tools/qlocale.h")

    # qlocale.qdoc

    (qlocaleqdoc_temp_file, qlocaleqdoc_temp_file_path) = tempfile.mkstemp("qlocale.qdoc", dir=qtsrcdir)
    qlocaleqdoc_temp_file = os.fdopen(qlocaleqdoc_temp_file, "w")
    qlocaleqdoc_file = open(qtsrcdir + "/src/corelib/tools/qlocale.qdoc", "r")
    s = qlocaleqdoc_file.readline()
    DOCSTRING="    QLocale's data is based on Common Locale Data Repository "
    while s:
        if DOCSTRING in s:
            qlocaleqdoc_temp_file.write(DOCSTRING + "v" + cldr_version + ".\n")
        else:
            qlocaleqdoc_temp_file.write(s)
        s = qlocaleqdoc_file.readline()
    qlocaleqdoc_temp_file.close()
    qlocaleqdoc_file.close()

    os.remove(qtsrcdir + "/src/corelib/tools/qlocale.qdoc")
    os.rename(qlocaleqdoc_temp_file_path, qtsrcdir + "/src/corelib/tools/qlocale.qdoc")

if __name__ == "__main__":
    main()
