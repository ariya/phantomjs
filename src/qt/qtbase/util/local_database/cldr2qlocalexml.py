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
import enumdata
import xpathlite
from  xpathlite import DraftResolution
from dateconverter import convert_date
from xml.sax.saxutils import escape, unescape
import re

findAlias = xpathlite.findAlias
findEntry = xpathlite.findEntry
findEntryInFile = xpathlite._findEntryInFile
findTagsInFile = xpathlite.findTagsInFile

def parse_number_format(patterns, data):
    # this is a very limited parsing of the number format for currency only.
    def skip_repeating_pattern(x):
        p = x.replace('0', '#').replace(',', '').replace('.', '')
        seen = False
        result = ''
        for c in p:
            if c == '#':
                if seen:
                    continue
                seen = True
            else:
                seen = False
            result = result + c
        return result
    patterns = patterns.split(';')
    result = []
    for pattern in patterns:
        pattern = skip_repeating_pattern(pattern)
        pattern = pattern.replace('#', "%1")
        # according to http://www.unicode.org/reports/tr35/#Number_Format_Patterns
        # there can be doubled or trippled currency sign, however none of the
        # locales use that.
        pattern = pattern.replace(u'\xa4', "%2")
        pattern = pattern.replace("''", "###").replace("'", '').replace("###", "'")
        pattern = pattern.replace('-', data['minus'])
        pattern = pattern.replace('+', data['plus'])
        result.append(pattern)
    return result

def parse_list_pattern_part_format(pattern):
    # this is a very limited parsing of the format for list pattern part only.
    result = ""
    result = pattern.replace("{0}", "%1")
    result = result.replace("{1}", "%2")
    result = result.replace("{2}", "%3")
    return result

def ordStr(c):
    if len(c) == 1:
        return str(ord(c))
    raise xpathlite.Error("Unable to handle value \"%s\"" % addEscapes(c))
    return "##########"

# the following functions are supposed to fix the problem with QLocale
# returning a character instead of strings for QLocale::exponential()
# and others. So we fallback to default values in these cases.
def fixOrdStrMinus(c):
    if len(c) == 1:
        return str(ord(c))
    return str(ord('-'))
def fixOrdStrPlus(c):
    if len(c) == 1:
        return str(ord(c))
    return str(ord('+'))
def fixOrdStrExp(c):
    if len(c) == 1:
        return str(ord(c))
    return str(ord('e'))
def fixOrdStrPercent(c):
    if len(c) == 1:
        return str(ord(c))
    return str(ord('%'))
def fixOrdStrList(c):
    if len(c) == 1:
        return str(ord(c))
    return str(ord(';'))

def generateLocaleInfo(path):
    (dir_name, file_name) = os.path.split(path)

    if not path.endswith(".xml"):
        return {}

    # skip legacy/compatibility ones
    alias = findAlias(path)
    if alias:
        raise xpathlite.Error("alias to \"%s\"" % alias)

    language_code = findEntryInFile(path, "identity/language", attribute="type")[0]
    if language_code == 'root':
        # just skip it
        return {}
    country_code = findEntryInFile(path, "identity/territory", attribute="type")[0]
    script_code = findEntryInFile(path, "identity/script", attribute="type")[0]
    variant_code = findEntryInFile(path, "identity/variant", attribute="type")[0]

    # we do not support variants
    # ### actually there is only one locale with variant: en_US_POSIX
    #     does anybody care about it at all?
    if variant_code:
        raise xpathlite.Error("we do not support variants (\"%s\")" % variant_code)

    language_id = enumdata.languageCodeToId(language_code)
    if language_id <= 0:
        raise xpathlite.Error("unknown language code \"%s\"" % language_code)
    language = enumdata.language_list[language_id][0]

    script_id = enumdata.scriptCodeToId(script_code)
    if script_id == -1:
        raise xpathlite.Error("unknown script code \"%s\"" % script_code)
    script = enumdata.script_list[script_id][0]

    # we should handle fully qualified names with the territory
    if not country_code:
        return {}
    country_id = enumdata.countryCodeToId(country_code)
    if country_id <= 0:
        raise xpathlite.Error("unknown country code \"%s\"" % country_code)
    country = enumdata.country_list[country_id][0]

    # So we say we accept only those values that have "contributed" or
    # "approved" resolution. see http://www.unicode.org/cldr/process.html
    # But we only respect the resolution for new datas for backward
    # compatibility.
    draft = DraftResolution.contributed

    result = {}
    result['language'] = language
    result['script'] = script
    result['country'] = country
    result['language_code'] = language_code
    result['country_code'] = country_code
    result['script_code'] = script_code
    result['variant_code'] = variant_code
    result['language_id'] = language_id
    result['script_id'] = script_id
    result['country_id'] = country_id

    supplementalPath = dir_name + "/../supplemental/supplementalData.xml"
    currencies = findTagsInFile(supplementalPath, "currencyData/region[iso3166=%s]"%country_code);
    result['currencyIsoCode'] = ''
    result['currencyDigits'] = 2
    result['currencyRounding'] = 1
    if currencies:
        for e in currencies:
            if e[0] == 'currency':
                tender = True
                t = filter(lambda x: x[0] == 'tender', e[1])
                if t and t[0][1] == 'false':
                    tender = False;
                if tender and not filter(lambda x: x[0] == 'to', e[1]):
                    result['currencyIsoCode'] = filter(lambda x: x[0] == 'iso4217', e[1])[0][1]
                    break
        if result['currencyIsoCode']:
            t = findTagsInFile(supplementalPath, "currencyData/fractions/info[iso4217=%s]"%result['currencyIsoCode']);
            if t and t[0][0] == 'info':
                result['currencyDigits'] = int(filter(lambda x: x[0] == 'digits', t[0][1])[0][1])
                result['currencyRounding'] = int(filter(lambda x: x[0] == 'rounding', t[0][1])[0][1])
    numbering_system = None
    try:
        numbering_system = findEntry(path, "numbers/defaultNumberingSystem")
    except:
        pass
    def findEntryDef(path, xpath, value=''):
        try:
            return findEntry(path, xpath)
        except xpathlite.Error:
            return value
    def get_number_in_system(path, xpath, numbering_system):
        if numbering_system:
            try:
                return findEntry(path, xpath + "[numberSystem=" + numbering_system + "]")
            except xpathlite.Error:
                # in CLDR 1.9 number system was refactored for numbers (but not for currency)
                # so if previous findEntry doesn't work we should try this:
                try:
                    return findEntry(path, xpath.replace("/symbols/", "/symbols[numberSystem=" + numbering_system + "]/"))
                except xpathlite.Error:
                    # fallback to default
                    pass
        return findEntry(path, xpath)

    result['decimal'] = get_number_in_system(path, "numbers/symbols/decimal", numbering_system)
    result['group'] = get_number_in_system(path, "numbers/symbols/group", numbering_system)
    result['list'] = get_number_in_system(path, "numbers/symbols/list", numbering_system)
    result['percent'] = get_number_in_system(path, "numbers/symbols/percentSign", numbering_system)
    try:
        numbering_systems = {}
        for ns in findTagsInFile(cldr_dir + "/../supplemental/numberingSystems.xml", "numberingSystems"):
            tmp = {}
            id = ""
            for data in ns[1:][0]: # ns looks like this: [u'numberingSystem', [(u'digits', u'0123456789'), (u'type', u'numeric'), (u'id', u'latn')]]
                tmp[data[0]] = data[1]
                if data[0] == u"id":
                    id = data[1]
            numbering_systems[id] = tmp
        result['zero'] = numbering_systems[numbering_system][u"digits"][0]
    except e:
        sys.stderr.write("Native zero detection problem:\n" + str(e) + "\n")
        result['zero'] = get_number_in_system(path, "numbers/symbols/nativeZeroDigit", numbering_system)
    result['minus'] = get_number_in_system(path, "numbers/symbols/minusSign", numbering_system)
    result['plus'] = get_number_in_system(path, "numbers/symbols/plusSign", numbering_system)
    result['exp'] = get_number_in_system(path, "numbers/symbols/exponential", numbering_system).lower()
    result['quotationStart'] = findEntry(path, "delimiters/quotationStart")
    result['quotationEnd'] = findEntry(path, "delimiters/quotationEnd")
    result['alternateQuotationStart'] = findEntry(path, "delimiters/alternateQuotationStart")
    result['alternateQuotationEnd'] = findEntry(path, "delimiters/alternateQuotationEnd")
    result['listPatternPartStart'] = parse_list_pattern_part_format(findEntry(path, "listPatterns/listPattern/listPatternPart[start]"))
    result['listPatternPartMiddle'] = parse_list_pattern_part_format(findEntry(path, "listPatterns/listPattern/listPatternPart[middle]"))
    result['listPatternPartEnd'] = parse_list_pattern_part_format(findEntry(path, "listPatterns/listPattern/listPatternPart[end]"))
    result['listPatternPartTwo'] = parse_list_pattern_part_format(findEntry(path, "listPatterns/listPattern/listPatternPart[2]"))
    result['am'] = findEntry(path, "dates/calendars/calendar[gregorian]/dayPeriods/dayPeriodContext[format]/dayPeriodWidth[wide]/dayPeriod[am]", draft)
    result['pm'] = findEntry(path, "dates/calendars/calendar[gregorian]/dayPeriods/dayPeriodContext[format]/dayPeriodWidth[wide]/dayPeriod[pm]", draft)
    result['longDateFormat'] = convert_date(findEntry(path, "dates/calendars/calendar[gregorian]/dateFormats/dateFormatLength[full]/dateFormat/pattern"))
    result['shortDateFormat'] = convert_date(findEntry(path, "dates/calendars/calendar[gregorian]/dateFormats/dateFormatLength[short]/dateFormat/pattern"))
    result['longTimeFormat'] = convert_date(findEntry(path, "dates/calendars/calendar[gregorian]/timeFormats/timeFormatLength[full]/timeFormat/pattern"))
    result['shortTimeFormat'] = convert_date(findEntry(path, "dates/calendars/calendar[gregorian]/timeFormats/timeFormatLength[short]/timeFormat/pattern"))

    endonym = None
    if country_code and script_code:
        endonym = findEntryDef(path, "localeDisplayNames/languages/language[type=%s_%s_%s]" % (language_code, script_code, country_code))
    if not endonym and script_code:
        endonym = findEntryDef(path, "localeDisplayNames/languages/language[type=%s_%s]" % (language_code, script_code))
    if not endonym and country_code:
        endonym = findEntryDef(path, "localeDisplayNames/languages/language[type=%s_%s]" % (language_code, country_code))
    if not endonym:
        endonym = findEntryDef(path, "localeDisplayNames/languages/language[type=%s]" % (language_code))
    result['language_endonym'] = endonym
    result['country_endonym'] = findEntryDef(path, "localeDisplayNames/territories/territory[type=%s]" % (country_code))

    currency_format = get_number_in_system(path, "numbers/currencyFormats/currencyFormatLength/currencyFormat/pattern", numbering_system)
    currency_format = parse_number_format(currency_format, result)
    result['currencyFormat'] = currency_format[0]
    result['currencyNegativeFormat'] = ''
    if len(currency_format) > 1:
        result['currencyNegativeFormat'] = currency_format[1]

    result['currencySymbol'] = ''
    result['currencyDisplayName'] = ''
    if result['currencyIsoCode']:
        result['currencySymbol'] = findEntryDef(path, "numbers/currencies/currency[%s]/symbol" % result['currencyIsoCode'])
        display_name_path = "numbers/currencies/currency[%s]/displayName" % result['currencyIsoCode']
        result['currencyDisplayName'] \
            = findEntryDef(path, display_name_path) + ";" \
            + findEntryDef(path, display_name_path + "[count=zero]")  + ";" \
            + findEntryDef(path, display_name_path + "[count=one]")   + ";" \
            + findEntryDef(path, display_name_path + "[count=two]")   + ";" \
            + findEntryDef(path, display_name_path + "[count=few]")   + ";" \
            + findEntryDef(path, display_name_path + "[count=many]")  + ";" \
            + findEntryDef(path, display_name_path + "[count=other]") + ";"

    standalone_long_month_path = "dates/calendars/calendar[gregorian]/months/monthContext[stand-alone]/monthWidth[wide]/month"
    result['standaloneLongMonths'] \
        = findEntry(path, standalone_long_month_path + "[1]") + ";" \
        + findEntry(path, standalone_long_month_path + "[2]") + ";" \
        + findEntry(path, standalone_long_month_path + "[3]") + ";" \
        + findEntry(path, standalone_long_month_path + "[4]") + ";" \
        + findEntry(path, standalone_long_month_path + "[5]") + ";" \
        + findEntry(path, standalone_long_month_path + "[6]") + ";" \
        + findEntry(path, standalone_long_month_path + "[7]") + ";" \
        + findEntry(path, standalone_long_month_path + "[8]") + ";" \
        + findEntry(path, standalone_long_month_path + "[9]") + ";" \
        + findEntry(path, standalone_long_month_path + "[10]") + ";" \
        + findEntry(path, standalone_long_month_path + "[11]") + ";" \
        + findEntry(path, standalone_long_month_path + "[12]") + ";"

    standalone_short_month_path = "dates/calendars/calendar[gregorian]/months/monthContext[stand-alone]/monthWidth[abbreviated]/month"
    result['standaloneShortMonths'] \
        = findEntry(path, standalone_short_month_path + "[1]") + ";" \
        + findEntry(path, standalone_short_month_path + "[2]") + ";" \
        + findEntry(path, standalone_short_month_path + "[3]") + ";" \
        + findEntry(path, standalone_short_month_path + "[4]") + ";" \
        + findEntry(path, standalone_short_month_path + "[5]") + ";" \
        + findEntry(path, standalone_short_month_path + "[6]") + ";" \
        + findEntry(path, standalone_short_month_path + "[7]") + ";" \
        + findEntry(path, standalone_short_month_path + "[8]") + ";" \
        + findEntry(path, standalone_short_month_path + "[9]") + ";" \
        + findEntry(path, standalone_short_month_path + "[10]") + ";" \
        + findEntry(path, standalone_short_month_path + "[11]") + ";" \
        + findEntry(path, standalone_short_month_path + "[12]") + ";"

    standalone_narrow_month_path = "dates/calendars/calendar[gregorian]/months/monthContext[stand-alone]/monthWidth[narrow]/month"
    result['standaloneNarrowMonths'] \
        = findEntry(path, standalone_narrow_month_path + "[1]") + ";" \
        + findEntry(path, standalone_narrow_month_path + "[2]") + ";" \
        + findEntry(path, standalone_narrow_month_path + "[3]") + ";" \
        + findEntry(path, standalone_narrow_month_path + "[4]") + ";" \
        + findEntry(path, standalone_narrow_month_path + "[5]") + ";" \
        + findEntry(path, standalone_narrow_month_path + "[6]") + ";" \
        + findEntry(path, standalone_narrow_month_path + "[7]") + ";" \
        + findEntry(path, standalone_narrow_month_path + "[8]") + ";" \
        + findEntry(path, standalone_narrow_month_path + "[9]") + ";" \
        + findEntry(path, standalone_narrow_month_path + "[10]") + ";" \
        + findEntry(path, standalone_narrow_month_path + "[11]") + ";" \
        + findEntry(path, standalone_narrow_month_path + "[12]") + ";"

    long_month_path = "dates/calendars/calendar[gregorian]/months/monthContext[format]/monthWidth[wide]/month"
    result['longMonths'] \
        = findEntry(path, long_month_path + "[1]") + ";" \
        + findEntry(path, long_month_path + "[2]") + ";" \
        + findEntry(path, long_month_path + "[3]") + ";" \
        + findEntry(path, long_month_path + "[4]") + ";" \
        + findEntry(path, long_month_path + "[5]") + ";" \
        + findEntry(path, long_month_path + "[6]") + ";" \
        + findEntry(path, long_month_path + "[7]") + ";" \
        + findEntry(path, long_month_path + "[8]") + ";" \
        + findEntry(path, long_month_path + "[9]") + ";" \
        + findEntry(path, long_month_path + "[10]") + ";" \
        + findEntry(path, long_month_path + "[11]") + ";" \
        + findEntry(path, long_month_path + "[12]") + ";"

    short_month_path = "dates/calendars/calendar[gregorian]/months/monthContext[format]/monthWidth[abbreviated]/month"
    result['shortMonths'] \
        = findEntry(path, short_month_path + "[1]") + ";" \
        + findEntry(path, short_month_path + "[2]") + ";" \
        + findEntry(path, short_month_path + "[3]") + ";" \
        + findEntry(path, short_month_path + "[4]") + ";" \
        + findEntry(path, short_month_path + "[5]") + ";" \
        + findEntry(path, short_month_path + "[6]") + ";" \
        + findEntry(path, short_month_path + "[7]") + ";" \
        + findEntry(path, short_month_path + "[8]") + ";" \
        + findEntry(path, short_month_path + "[9]") + ";" \
        + findEntry(path, short_month_path + "[10]") + ";" \
        + findEntry(path, short_month_path + "[11]") + ";" \
        + findEntry(path, short_month_path + "[12]") + ";"

    narrow_month_path = "dates/calendars/calendar[gregorian]/months/monthContext[format]/monthWidth[narrow]/month"
    result['narrowMonths'] \
        = findEntry(path, narrow_month_path + "[1]") + ";" \
        + findEntry(path, narrow_month_path + "[2]") + ";" \
        + findEntry(path, narrow_month_path + "[3]") + ";" \
        + findEntry(path, narrow_month_path + "[4]") + ";" \
        + findEntry(path, narrow_month_path + "[5]") + ";" \
        + findEntry(path, narrow_month_path + "[6]") + ";" \
        + findEntry(path, narrow_month_path + "[7]") + ";" \
        + findEntry(path, narrow_month_path + "[8]") + ";" \
        + findEntry(path, narrow_month_path + "[9]") + ";" \
        + findEntry(path, narrow_month_path + "[10]") + ";" \
        + findEntry(path, narrow_month_path + "[11]") + ";" \
        + findEntry(path, narrow_month_path + "[12]") + ";"

    long_day_path = "dates/calendars/calendar[gregorian]/days/dayContext[format]/dayWidth[wide]/day"
    result['longDays'] \
        = findEntry(path, long_day_path + "[sun]") + ";" \
        + findEntry(path, long_day_path + "[mon]") + ";" \
        + findEntry(path, long_day_path + "[tue]") + ";" \
        + findEntry(path, long_day_path + "[wed]") + ";" \
        + findEntry(path, long_day_path + "[thu]") + ";" \
        + findEntry(path, long_day_path + "[fri]") + ";" \
        + findEntry(path, long_day_path + "[sat]") + ";"

    short_day_path = "dates/calendars/calendar[gregorian]/days/dayContext[format]/dayWidth[abbreviated]/day"
    result['shortDays'] \
        = findEntry(path, short_day_path + "[sun]") + ";" \
        + findEntry(path, short_day_path + "[mon]") + ";" \
        + findEntry(path, short_day_path + "[tue]") + ";" \
        + findEntry(path, short_day_path + "[wed]") + ";" \
        + findEntry(path, short_day_path + "[thu]") + ";" \
        + findEntry(path, short_day_path + "[fri]") + ";" \
        + findEntry(path, short_day_path + "[sat]") + ";"

    narrow_day_path = "dates/calendars/calendar[gregorian]/days/dayContext[format]/dayWidth[narrow]/day"
    result['narrowDays'] \
        = findEntry(path, narrow_day_path + "[sun]") + ";" \
        + findEntry(path, narrow_day_path + "[mon]") + ";" \
        + findEntry(path, narrow_day_path + "[tue]") + ";" \
        + findEntry(path, narrow_day_path + "[wed]") + ";" \
        + findEntry(path, narrow_day_path + "[thu]") + ";" \
        + findEntry(path, narrow_day_path + "[fri]") + ";" \
        + findEntry(path, narrow_day_path + "[sat]") + ";"

    standalone_long_day_path = "dates/calendars/calendar[gregorian]/days/dayContext[stand-alone]/dayWidth[wide]/day"
    result['standaloneLongDays'] \
        = findEntry(path, standalone_long_day_path + "[sun]") + ";" \
        + findEntry(path, standalone_long_day_path + "[mon]") + ";" \
        + findEntry(path, standalone_long_day_path + "[tue]") + ";" \
        + findEntry(path, standalone_long_day_path + "[wed]") + ";" \
        + findEntry(path, standalone_long_day_path + "[thu]") + ";" \
        + findEntry(path, standalone_long_day_path + "[fri]") + ";" \
        + findEntry(path, standalone_long_day_path + "[sat]") + ";"

    standalone_short_day_path = "dates/calendars/calendar[gregorian]/days/dayContext[stand-alone]/dayWidth[abbreviated]/day"
    result['standaloneShortDays'] \
        = findEntry(path, standalone_short_day_path + "[sun]") + ";" \
        + findEntry(path, standalone_short_day_path + "[mon]") + ";" \
        + findEntry(path, standalone_short_day_path + "[tue]") + ";" \
        + findEntry(path, standalone_short_day_path + "[wed]") + ";" \
        + findEntry(path, standalone_short_day_path + "[thu]") + ";" \
        + findEntry(path, standalone_short_day_path + "[fri]") + ";" \
        + findEntry(path, standalone_short_day_path + "[sat]") + ";"

    standalone_narrow_day_path = "dates/calendars/calendar[gregorian]/days/dayContext[stand-alone]/dayWidth[narrow]/day"
    result['standaloneNarrowDays'] \
        = findEntry(path, standalone_narrow_day_path + "[sun]") + ";" \
        + findEntry(path, standalone_narrow_day_path + "[mon]") + ";" \
        + findEntry(path, standalone_narrow_day_path + "[tue]") + ";" \
        + findEntry(path, standalone_narrow_day_path + "[wed]") + ";" \
        + findEntry(path, standalone_narrow_day_path + "[thu]") + ";" \
        + findEntry(path, standalone_narrow_day_path + "[fri]") + ";" \
        + findEntry(path, standalone_narrow_day_path + "[sat]") + ";"

    return result

def addEscapes(s):
    result = ''
    for c in s:
        n = ord(c)
        if n < 128:
            result += c
        else:
            result += "\\x"
            result += "%02x" % (n)
    return result

def unicodeStr(s):
    utf8 = s.encode('utf-8')
    return "<size>" + str(len(utf8)) + "</size><data>" + addEscapes(utf8) + "</data>"

def usage():
    print "Usage: cldr2qlocalexml.py <path-to-cldr-main>"
    sys.exit()

def integrateWeekData(filePath):
    if not filePath.endswith(".xml"):
        return {}
    monFirstDayIn = findEntryInFile(filePath, "weekData/firstDay[day=mon]", attribute="territories")[0].split(" ")
    tueFirstDayIn = findEntryInFile(filePath, "weekData/firstDay[day=tue]", attribute="territories")[0].split(" ")
    wedFirstDayIn = findEntryInFile(filePath, "weekData/firstDay[day=wed]", attribute="territories")[0].split(" ")
    thuFirstDayIn = findEntryInFile(filePath, "weekData/firstDay[day=thu]", attribute="territories")[0].split(" ")
    friFirstDayIn = findEntryInFile(filePath, "weekData/firstDay[day=fri]", attribute="territories")[0].split(" ")
    satFirstDayIn = findEntryInFile(filePath, "weekData/firstDay[day=sat]", attribute="territories")[0].split(" ")
    sunFirstDayIn = findEntryInFile(filePath, "weekData/firstDay[day=sun]", attribute="territories")[0].split(" ")

    monWeekendStart = findEntryInFile(filePath, "weekData/weekendStart[day=mon]", attribute="territories")[0].split(" ")
    tueWeekendStart = findEntryInFile(filePath, "weekData/weekendStart[day=tue]", attribute="territories")[0].split(" ")
    wedWeekendStart = findEntryInFile(filePath, "weekData/weekendStart[day=wed]", attribute="territories")[0].split(" ")
    thuWeekendStart = findEntryInFile(filePath, "weekData/weekendStart[day=thu]", attribute="territories")[0].split(" ")
    friWeekendStart = findEntryInFile(filePath, "weekData/weekendStart[day=fri]", attribute="territories")[0].split(" ")
    satWeekendStart = findEntryInFile(filePath, "weekData/weekendStart[day=sat]", attribute="territories")[0].split(" ")
    sunWeekendStart = findEntryInFile(filePath, "weekData/weekendStart[day=sun]", attribute="territories")[0].split(" ")

    monWeekendEnd = findEntryInFile(filePath, "weekData/weekendEnd[day=mon]", attribute="territories")[0].split(" ")
    tueWeekendEnd = findEntryInFile(filePath, "weekData/weekendEnd[day=tue]", attribute="territories")[0].split(" ")
    wedWeekendEnd = findEntryInFile(filePath, "weekData/weekendEnd[day=wed]", attribute="territories")[0].split(" ")
    thuWeekendEnd = findEntryInFile(filePath, "weekData/weekendEnd[day=thu]", attribute="territories")[0].split(" ")
    friWeekendEnd = findEntryInFile(filePath, "weekData/weekendEnd[day=fri]", attribute="territories")[0].split(" ")
    satWeekendEnd = findEntryInFile(filePath, "weekData/weekendEnd[day=sat]", attribute="territories")[0].split(" ")
    sunWeekendEnd = findEntryInFile(filePath, "weekData/weekendEnd[day=sun]", attribute="territories")[0].split(" ")

    firstDayByCountryCode = {}
    for countryCode in monFirstDayIn:
        firstDayByCountryCode[countryCode] = "mon"
    for countryCode in tueFirstDayIn:
        firstDayByCountryCode[countryCode] = "tue"
    for countryCode in wedFirstDayIn:
        firstDayByCountryCode[countryCode] = "wed"
    for countryCode in thuFirstDayIn:
        firstDayByCountryCode[countryCode] = "thu"
    for countryCode in friFirstDayIn:
        firstDayByCountryCode[countryCode] = "fri"
    for countryCode in satFirstDayIn:
        firstDayByCountryCode[countryCode] = "sat"
    for countryCode in sunFirstDayIn:
        firstDayByCountryCode[countryCode] = "sun"

    weekendStartByCountryCode = {}
    for countryCode in monWeekendStart:
        weekendStartByCountryCode[countryCode] = "mon"
    for countryCode in tueWeekendStart:
        weekendStartByCountryCode[countryCode] = "tue"
    for countryCode in wedWeekendStart:
        weekendStartByCountryCode[countryCode] = "wed"
    for countryCode in thuWeekendStart:
        weekendStartByCountryCode[countryCode] = "thu"
    for countryCode in friWeekendStart:
        weekendStartByCountryCode[countryCode] = "fri"
    for countryCode in satWeekendStart:
        weekendStartByCountryCode[countryCode] = "sat"
    for countryCode in sunWeekendStart:
        weekendStartByCountryCode[countryCode] = "sun"

    weekendEndByCountryCode = {}
    for countryCode in monWeekendEnd:
        weekendEndByCountryCode[countryCode] = "mon"
    for countryCode in tueWeekendEnd:
        weekendEndByCountryCode[countryCode] = "tue"
    for countryCode in wedWeekendEnd:
        weekendEndByCountryCode[countryCode] = "wed"
    for countryCode in thuWeekendEnd:
        weekendEndByCountryCode[countryCode] = "thu"
    for countryCode in friWeekendEnd:
        weekendEndByCountryCode[countryCode] = "fri"
    for countryCode in satWeekendEnd:
        weekendEndByCountryCode[countryCode] = "sat"
    for countryCode in sunWeekendEnd:
        weekendEndByCountryCode[countryCode] = "sun"

    for (key,locale) in locale_database.iteritems():
        countryCode = locale['country_code']
        if countryCode in firstDayByCountryCode:
            locale_database[key]['firstDayOfWeek'] = firstDayByCountryCode[countryCode]
        else:
            locale_database[key]['firstDayOfWeek'] = firstDayByCountryCode["001"]

        if countryCode in weekendStartByCountryCode:
            locale_database[key]['weekendStart'] = weekendStartByCountryCode[countryCode]
        else:
            locale_database[key]['weekendStart'] = weekendStartByCountryCode["001"]

        if countryCode in weekendEndByCountryCode:
            locale_database[key]['weekendEnd'] = weekendEndByCountryCode[countryCode]
        else:
            locale_database[key]['weekendEnd'] = weekendEndByCountryCode["001"]

if len(sys.argv) != 2:
    usage()

cldr_dir = sys.argv[1]

if not os.path.isdir(cldr_dir):
    usage()

cldr_files = os.listdir(cldr_dir)

locale_database = {}
for file in cldr_files:
    try:
        l = generateLocaleInfo(cldr_dir + "/" + file)
        if not l:
            sys.stderr.write("skipping file \"" + file + "\"\n")
            continue
    except xpathlite.Error as e:
        sys.stderr.write("skipping file \"%s\" (%s)\n" % (file, str(e)))
        continue

    locale_database[(l['language_id'], l['script_id'], l['country_id'], l['variant_code'])] = l

integrateWeekData(cldr_dir+"/../supplemental/supplementalData.xml")
locale_keys = locale_database.keys()
locale_keys.sort()

cldr_version = 'unknown'
ldml = open(cldr_dir+"/../dtd/ldml.dtd", "r")
for line in ldml:
    if 'version cldrVersion CDATA #FIXED' in line:
        cldr_version = line.split('"')[1]

print "<localeDatabase>"
print "    <version>" + cldr_version + "</version>"
print "    <languageList>"
for id in enumdata.language_list:
    l = enumdata.language_list[id]
    print "        <language>"
    print "            <name>" + l[0] + "</name>"
    print "            <id>" + str(id) + "</id>"
    print "            <code>" + l[1] + "</code>"
    print "        </language>"
print "    </languageList>"

print "    <scriptList>"
for id in enumdata.script_list:
    l = enumdata.script_list[id]
    print "        <script>"
    print "            <name>" + l[0] + "</name>"
    print "            <id>" + str(id) + "</id>"
    print "            <code>" + l[1] + "</code>"
    print "        </script>"
print "    </scriptList>"

print "    <countryList>"
for id in enumdata.country_list:
    l = enumdata.country_list[id]
    print "        <country>"
    print "            <name>" + l[0] + "</name>"
    print "            <id>" + str(id) + "</id>"
    print "            <code>" + l[1] + "</code>"
    print "        </country>"
print "    </countryList>"

def _parseLocale(l):
    language = "AnyLanguage"
    script = "AnyScript"
    country = "AnyCountry"

    if l == "und":
        raise xpathlite.Error("we are treating unknown locale like C")

    items = l.split("_")
    language_code = items[0]
    if language_code != "und":
        language_id = enumdata.languageCodeToId(language_code)
        if language_id == -1:
            raise xpathlite.Error("unknown language code \"%s\"" % language_code)
        language = enumdata.language_list[language_id][0]

    if len(items) > 1:
        script_code = items[1]
        country_code = ""
        if len(items) > 2:
            country_code = items[2]
        if len(script_code) == 4:
            script_id = enumdata.scriptCodeToId(script_code)
            if script_id == -1:
                raise xpathlite.Error("unknown script code \"%s\"" % script_code)
            script = enumdata.script_list[script_id][0]
        else:
            country_code = script_code
        if country_code:
            country_id = enumdata.countryCodeToId(country_code)
            if country_id == -1:
                raise xpathlite.Error("unknown country code \"%s\"" % country_code)
            country = enumdata.country_list[country_id][0]

    return (language, script, country)

print "    <likelySubtags>"
for ns in findTagsInFile(cldr_dir + "/../supplemental/likelySubtags.xml", "likelySubtags"):
    tmp = {}
    for data in ns[1:][0]: # ns looks like this: [u'likelySubtag', [(u'from', u'aa'), (u'to', u'aa_Latn_ET')]]
        tmp[data[0]] = data[1]

    try:
        (from_language, from_script, from_country) = _parseLocale(tmp[u"from"])
    except xpathlite.Error as e:
        sys.stderr.write("skipping likelySubtag \"%s\" -> \"%s\" (%s)\n" % (tmp[u"from"], tmp[u"to"], str(e)))
        continue
    try:
        (to_language, to_script, to_country) = _parseLocale(tmp[u"to"])
    except xpathlite.Error as e:
        sys.stderr.write("skipping likelySubtag \"%s\" -> \"%s\" (%s)\n" % (tmp[u"from"], tmp[u"to"], str(e)))
        continue
    # substitute according to http://www.unicode.org/reports/tr35/#Likely_Subtags
    if to_country == "AnyCountry" and from_country != to_country:
        to_country = from_country
    if to_script == "AnyScript" and from_script != to_script:
        to_script = from_script

    print "        <likelySubtag>"
    print "            <from>"
    print "                <language>" + from_language + "</language>"
    print "                <script>" + from_script + "</script>"
    print "                <country>" + from_country + "</country>"
    print "            </from>"
    print "            <to>"
    print "                <language>" + to_language + "</language>"
    print "                <script>" + to_script + "</script>"
    print "                <country>" + to_country + "</country>"
    print "            </to>"
    print "        </likelySubtag>"
print "    </likelySubtags>"

print "    <localeList>"
print \
"        <locale>\n\
            <language>C</language>\n\
            <languageEndonym></languageEndonym>\n\
            <script>AnyScript</script>\n\
            <country>AnyCountry</country>\n\
            <countryEndonym></countryEndonym>\n\
            <decimal>46</decimal>\n\
            <group>44</group>\n\
            <list>59</list>\n\
            <percent>37</percent>\n\
            <zero>48</zero>\n\
            <minus>45</minus>\n\
            <plus>43</plus>\n\
            <exp>101</exp>\n\
            <quotationStart>\"</quotationStart>\n\
            <quotationEnd>\"</quotationEnd>\n\
            <alternateQuotationStart>\'</alternateQuotationStart>\n\
            <alternateQuotationEnd>\'</alternateQuotationEnd>\n\
            <listPatternPartStart>%1, %2</listPatternPartStart>\n\
            <listPatternPartMiddle>%1, %2</listPatternPartMiddle>\n\
            <listPatternPartEnd>%1, %2</listPatternPartEnd>\n\
            <listPatternPartTwo>%1, %2</listPatternPartTwo>\n\
            <am>AM</am>\n\
            <pm>PM</pm>\n\
            <firstDayOfWeek>mon</firstDayOfWeek>\n\
            <weekendStart>sat</weekendStart>\n\
            <weekendEnd>sun</weekendEnd>\n\
            <longDateFormat>EEEE, d MMMM yyyy</longDateFormat>\n\
            <shortDateFormat>d MMM yyyy</shortDateFormat>\n\
            <longTimeFormat>HH:mm:ss z</longTimeFormat>\n\
            <shortTimeFormat>HH:mm:ss</shortTimeFormat>\n\
            <standaloneLongMonths>January;February;March;April;May;June;July;August;September;October;November;December;</standaloneLongMonths>\n\
            <standaloneShortMonths>Jan;Feb;Mar;Apr;May;Jun;Jul;Aug;Sep;Oct;Nov;Dec;</standaloneShortMonths>\n\
            <standaloneNarrowMonths>J;F;M;A;M;J;J;A;S;O;N;D;</standaloneNarrowMonths>\n\
            <longMonths>January;February;March;April;May;June;July;August;September;October;November;December;</longMonths>\n\
            <shortMonths>Jan;Feb;Mar;Apr;May;Jun;Jul;Aug;Sep;Oct;Nov;Dec;</shortMonths>\n\
            <narrowMonths>1;2;3;4;5;6;7;8;9;10;11;12;</narrowMonths>\n\
            <longDays>Sunday;Monday;Tuesday;Wednesday;Thursday;Friday;Saturday;</longDays>\n\
            <shortDays>Sun;Mon;Tue;Wed;Thu;Fri;Sat;</shortDays>\n\
            <narrowDays>7;1;2;3;4;5;6;</narrowDays>\n\
            <standaloneLongDays>Sunday;Monday;Tuesday;Wednesday;Thursday;Friday;Saturday;</standaloneLongDays>\n\
            <standaloneShortDays>Sun;Mon;Tue;Wed;Thu;Fri;Sat;</standaloneShortDays>\n\
            <standaloneNarrowDays>S;M;T;W;T;F;S;</standaloneNarrowDays>\n\
            <currencyIsoCode></currencyIsoCode>\n\
            <currencySymbol></currencySymbol>\n\
            <currencyDisplayName>;;;;;;;</currencyDisplayName>\n\
            <currencyDigits>2</currencyDigits>\n\
            <currencyRounding>1</currencyRounding>\n\
            <currencyFormat>%1%2</currencyFormat>\n\
            <currencyNegativeFormat></currencyNegativeFormat>\n\
        </locale>"

for key in locale_keys:
    l = locale_database[key]

    print "        <locale>"
    print "            <language>" + l['language']        + "</language>"
    print "            <languageEndonym>" + escape(l['language_endonym']).encode('utf-8') + "</languageEndonym>"
    print "            <script>" + l['script']        + "</script>"
    print "            <country>"  + l['country']         + "</country>"
    print "            <countryEndonym>"  + escape(l['country_endonym']).encode('utf-8') + "</countryEndonym>"
    print "            <languagecode>" + l['language_code']        + "</languagecode>"
    print "            <scriptcode>" + l['script_code']        + "</scriptcode>"
    print "            <countrycode>"  + l['country_code']         + "</countrycode>"
    print "            <decimal>"  + ordStr(l['decimal']) + "</decimal>"
    print "            <group>"    + ordStr(l['group'])   + "</group>"
    print "            <list>"     + fixOrdStrList(l['list'])    + "</list>"
    print "            <percent>"  + fixOrdStrPercent(l['percent']) + "</percent>"
    print "            <zero>"     + ordStr(l['zero'])    + "</zero>"
    print "            <minus>"    + fixOrdStrMinus(l['minus'])   + "</minus>"
    print "            <plus>"     + fixOrdStrPlus(l['plus'])   + "</plus>"
    print "            <exp>"      + fixOrdStrExp(l['exp'])     + "</exp>"
    print "            <quotationStart>" + l['quotationStart'].encode('utf-8') + "</quotationStart>"
    print "            <quotationEnd>" + l['quotationEnd'].encode('utf-8')   + "</quotationEnd>"
    print "            <alternateQuotationStart>" + l['alternateQuotationStart'].encode('utf-8') + "</alternateQuotationStart>"
    print "            <alternateQuotationEnd>" + l['alternateQuotationEnd'].encode('utf-8')   + "</alternateQuotationEnd>"
    print "            <listPatternPartStart>" + l['listPatternPartStart'].encode('utf-8')   + "</listPatternPartStart>"
    print "            <listPatternPartMiddle>" + l['listPatternPartMiddle'].encode('utf-8')   + "</listPatternPartMiddle>"
    print "            <listPatternPartEnd>" + l['listPatternPartEnd'].encode('utf-8')   + "</listPatternPartEnd>"
    print "            <listPatternPartTwo>" + l['listPatternPartTwo'].encode('utf-8')   + "</listPatternPartTwo>"
    print "            <am>"       + l['am'].encode('utf-8') + "</am>"
    print "            <pm>"       + l['pm'].encode('utf-8') + "</pm>"
    print "            <firstDayOfWeek>"  + l['firstDayOfWeek'].encode('utf-8') + "</firstDayOfWeek>"
    print "            <weekendStart>"  + l['weekendStart'].encode('utf-8') + "</weekendStart>"
    print "            <weekendEnd>"  + l['weekendEnd'].encode('utf-8') + "</weekendEnd>"
    print "            <longDateFormat>"  + l['longDateFormat'].encode('utf-8')  + "</longDateFormat>"
    print "            <shortDateFormat>" + l['shortDateFormat'].encode('utf-8') + "</shortDateFormat>"
    print "            <longTimeFormat>"  + l['longTimeFormat'].encode('utf-8')  + "</longTimeFormat>"
    print "            <shortTimeFormat>" + l['shortTimeFormat'].encode('utf-8') + "</shortTimeFormat>"
    print "            <standaloneLongMonths>" + l['standaloneLongMonths'].encode('utf-8')      + "</standaloneLongMonths>"
    print "            <standaloneShortMonths>"+ l['standaloneShortMonths'].encode('utf-8')      + "</standaloneShortMonths>"
    print "            <standaloneNarrowMonths>"+ l['standaloneNarrowMonths'].encode('utf-8')      + "</standaloneNarrowMonths>"
    print "            <longMonths>"      + l['longMonths'].encode('utf-8')      + "</longMonths>"
    print "            <shortMonths>"     + l['shortMonths'].encode('utf-8')     + "</shortMonths>"
    print "            <narrowMonths>"     + l['narrowMonths'].encode('utf-8')     + "</narrowMonths>"
    print "            <longDays>"        + l['longDays'].encode('utf-8')        + "</longDays>"
    print "            <shortDays>"       + l['shortDays'].encode('utf-8')       + "</shortDays>"
    print "            <narrowDays>"       + l['narrowDays'].encode('utf-8')       + "</narrowDays>"
    print "            <standaloneLongDays>" + l['standaloneLongDays'].encode('utf-8')        + "</standaloneLongDays>"
    print "            <standaloneShortDays>" + l['standaloneShortDays'].encode('utf-8')       + "</standaloneShortDays>"
    print "            <standaloneNarrowDays>" + l['standaloneNarrowDays'].encode('utf-8')       + "</standaloneNarrowDays>"
    print "            <currencyIsoCode>" + l['currencyIsoCode'].encode('utf-8') + "</currencyIsoCode>"
    print "            <currencySymbol>" + l['currencySymbol'].encode('utf-8') + "</currencySymbol>"
    print "            <currencyDisplayName>" + l['currencyDisplayName'].encode('utf-8') + "</currencyDisplayName>"
    print "            <currencyDigits>" + str(l['currencyDigits']) + "</currencyDigits>"
    print "            <currencyRounding>" + str(l['currencyRounding']) + "</currencyRounding>"
    print "            <currencyFormat>" + l['currencyFormat'].encode('utf-8') + "</currencyFormat>"
    print "            <currencyNegativeFormat>" + l['currencyNegativeFormat'].encode('utf-8') + "</currencyNegativeFormat>"
    print "        </locale>"
print "    </localeList>"
print "</localeDatabase>"
