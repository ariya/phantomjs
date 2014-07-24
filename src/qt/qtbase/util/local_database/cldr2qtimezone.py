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


# Script to parse the CLDR supplemental/windowsZones.xml file and encode for use in QTimeZone
# XML structure is as follows:
#
# <supplementalData>
#     <version number="$Revision: 7825 $"/>
#     <generation date="$Date: 2012-10-10 14:45:31 -0700 (Wed, 10 Oct 2012) $"/>
#     <windowsZones>
#         <mapTimezones otherVersion="7dc0101" typeVersion="2012f">
#             <!-- (UTC-08:00) Pacific Time (US & Canada) -->
#             <mapZone other="Pacific Standard Time" territory="001" type="America/Los_Angeles"/>
#             <mapZone other="Pacific Standard Time" territory="CA"  type="America/Vancouver America/Dawson America/Whitehorse"/>
#             <mapZone other="Pacific Standard Time" territory="MX"  type="America/Tijuana"/>
#             <mapZone other="Pacific Standard Time" territory="US"  type="America/Los_Angeles"/>
#             <mapZone other="Pacific Standard Time" territory="ZZ"  type="PST8PDT"/>
#       </mapTimezones>
#     </windowsZones>
# </supplementalData>

import os
import sys
import datetime
import tempfile
import enumdata
import xpathlite
from  xpathlite import DraftResolution
import re
import qlocalexml2cpp

findAlias = xpathlite.findAlias
findEntry = xpathlite.findEntry
findEntryInFile = xpathlite._findEntryInFile
findTagsInFile = xpathlite.findTagsInFile
unicode2hex = qlocalexml2cpp.unicode2hex
wrap_list = qlocalexml2cpp.wrap_list

class ByteArrayData:
    def __init__(self):
        self.data = []
        self.hash = {}
    def append(self, s):
        s = s + '\0'
        if s in self.hash:
            return self.hash[s]

        lst = unicode2hex(s)
        index = len(self.data)
        if index > 65535:
            print "\n\n\n#error Data index is too big!"
            sys.stderr.write ("\n\n\nERROR: index exceeds the uint16 range! index = %d\n" % index)
            sys.exit(1)
        self.hash[s] = index
        self.data += lst
        return index

# List of currently known Windows IDs.  If script fails on missing ID plase add it here
# Not public so may be safely changed.
# Windows Key : [ Windows Id, Offset Seconds ]
windowsIdList = {
    1 : [ u'Afghanistan Standard Time',        16200  ],
    2 : [ u'Alaskan Standard Time',           -32400  ],
    3 : [ u'Arab Standard Time',               10800  ],
    4 : [ u'Arabian Standard Time',            14400  ],
    5 : [ u'Arabic Standard Time',             10800  ],
    6 : [ u'Argentina Standard Time',          10800  ],
    7 : [ u'Atlantic Standard Time',          -14400  ],
    8 : [ u'AUS Central Standard Time',        34200  ],
    9 : [ u'AUS Eastern Standard Time',        36000  ],
   10 : [ u'Azerbaijan Standard Time',         14400  ],
   11 : [ u'Azores Standard Time',             -3600  ],
   12 : [ u'Bahia Standard Time',              10800  ],
   13 : [ u'Bangladesh Standard Time',         21600  ],
   14 : [ u'Canada Central Standard Time',    -21600  ],
   15 : [ u'Cape Verde Standard Time',         -3600  ],
   16 : [ u'Caucasus Standard Time',           14400  ],
   17 : [ u'Cen. Australia Standard Time',     34200  ],
   18 : [ u'Central America Standard Time',   -21600  ],
   19 : [ u'Central Asia Standard Time',       21600  ],
   20 : [ u'Central Brazilian Standard Time', -14400  ],
   21 : [ u'Central Europe Standard Time',      3600  ],
   22 : [ u'Central European Standard Time',    3600  ],
   23 : [ u'Central Pacific Standard Time',    39600  ],
   24 : [ u'Central Standard Time (Mexico)',  -21600  ],
   25 : [ u'Central Standard Time',           -21600  ],
   26 : [ u'China Standard Time',              28800  ],
   27 : [ u'Dateline Standard Time',          -43200  ],
   28 : [ u'E. Africa Standard Time',          10800  ],
   29 : [ u'E. Australia Standard Time',       36000  ],
   30 : [ u'E. Europe Standard Time',           7200  ],
   31 : [ u'E. South America Standard Time',  -10800  ],
   32 : [ u'Eastern Standard Time',           -18000  ],
   33 : [ u'Egypt Standard Time',               7200  ],
   34 : [ u'Ekaterinburg Standard Time',       21600  ],
   35 : [ u'Fiji Standard Time',               43200  ],
   36 : [ u'FLE Standard Time',                 7200  ],
   37 : [ u'Georgian Standard Time',           14400  ],
   38 : [ u'GMT Standard Time',                    0  ],
   39 : [ u'Greenland Standard Time',          10800  ],
   40 : [ u'Greenwich Standard Time',              0  ],
   41 : [ u'GTB Standard Time',                 7200  ],
   42 : [ u'Hawaiian Standard Time',          -36000  ],
   43 : [ u'India Standard Time',              19800  ],
   44 : [ u'Iran Standard Time',               12600  ],
   45 : [ u'Israel Standard Time',              7200  ],
   46 : [ u'Jordan Standard Time',              7200  ],
   47 : [ u'Kaliningrad Standard Time',        10800  ],
   48 : [ u'Korea Standard Time',              32400  ],
   49 : [ u'Magadan Standard Time',            43200  ],
   50 : [ u'Mauritius Standard Time',          14400  ],
   51 : [ u'Middle East Standard Time',         7200  ],
   52 : [ u'Montevideo Standard Time',         10800  ],
   53 : [ u'Morocco Standard Time',                0  ],
   54 : [ u'Mountain Standard Time (Mexico)', -25200  ],
   55 : [ u'Mountain Standard Time',          -25200  ],
   56 : [ u'Myanmar Standard Time',            23400  ],
   57 : [ u'N. Central Asia Standard Time',    23400  ],
   58 : [ u'Namibia Standard Time',             3600  ],
   59 : [ u'Nepal Standard Time',              20700  ],
   60 : [ u'New Zealand Standard Time',        43200  ],
   61 : [ u'Newfoundland Standard Time',      -12600  ],
   62 : [ u'North Asia East Standard Time',    32400  ],
   63 : [ u'North Asia Standard Time',         28800  ],
   64 : [ u'Pacific SA Standard Time',        -14400  ],
   65 : [ u'Pacific Standard Time (Mexico)',  -28800  ],
   66 : [ u'Pacific Standard Time',           -28800  ],
   67 : [ u'Pakistan Standard Time',           18000  ],
   68 : [ u'Paraguay Standard Time',          -14400  ],
   69 : [ u'Romance Standard Time',             3600  ],
   70 : [ u'Russian Standard Time',            14400  ],
   71 : [ u'SA Eastern Standard Time',         10800  ],
   72 : [ u'SA Pacific Standard Time',        -18000  ],
   73 : [ u'SA Western Standard Time',        -14400  ],
   74 : [ u'Samoa Standard Time',              46800  ],
   75 : [ u'SE Asia Standard Time',            23400  ],
   76 : [ u'Singapore Standard Time',          28800  ],
   77 : [ u'South Africa Standard Time',        7200  ],
   78 : [ u'Sri Lanka Standard Time',          19800  ],
   79 : [ u'Syria Standard Time',               7200  ],
   80 : [ u'Taipei Standard Time',             28800  ],
   81 : [ u'Tasmania Standard Time',           36000  ],
   82 : [ u'Tokyo Standard Time',              32400  ],
   83 : [ u'Tonga Standard Time',              46800  ],
   84 : [ u'Turkey Standard Time',              7200  ],
   85 : [ u'Ulaanbaatar Standard Time',        28800  ],
   86 : [ u'US Eastern Standard Time',        -18000  ],
   87 : [ u'US Mountain Standard Time',       -25200  ],
   88 : [ u'UTC-02',                           -7200  ],
   89 : [ u'UTC-11',                          -39600  ],
   90 : [ u'UTC',                                  0  ],
   91 : [ u'UTC+12',                           43200  ],
   92 : [ u'Venezuela Standard Time',         -16200  ],
   93 : [ u'Vladivostok Standard Time',        39600  ],
   94 : [ u'W. Australia Standard Time',       28800  ],
   95 : [ u'W. Central Africa Standard Time',   3600  ],
   96 : [ u'W. Europe Standard Time',           3600  ],
   97 : [ u'West Asia Standard Time',          18000  ],
   98 : [ u'West Pacific Standard Time',       36000  ],
   99 : [ u'Yakutsk Standard Time',            36000  ],
   100: [ u'Libya Standard Time',              3600   ]
}

def windowsIdToKey(windowsId):
    for windowsKey in windowsIdList:
        if windowsIdList[windowsKey][0] == windowsId:
            return windowsKey
    return 0

# List of standard UTC IDs to use.  Not public so may be safely changed.
# Do not remove ID's as is part of API/behavior guarantee
# Key : [ UTC Id, Offset Seconds ]
utcIdList = {
    0 : [ u'UTC',            0  ],  # Goes first so is default
    1 : [ u'UTC-14:00', -50400  ],
    2 : [ u'UTC-13:00', -46800  ],
    3 : [ u'UTC-12:00', -43200  ],
    4 : [ u'UTC-11:00', -39600  ],
    5 : [ u'UTC-10:00', -36000  ],
    6 : [ u'UTC-09:00', -32400  ],
    7 : [ u'UTC-08:00', -28800  ],
    8 : [ u'UTC-07:00', -25200  ],
    9 : [ u'UTC-06:00', -21600  ],
   10 : [ u'UTC-05:00', -18000  ],
   11 : [ u'UTC-04:30', -16200  ],
   12 : [ u'UTC-04:00', -14400  ],
   13 : [ u'UTC-03:30', -12600  ],
   14 : [ u'UTC-03:00', -10800  ],
   15 : [ u'UTC-02:00',  -7200  ],
   16 : [ u'UTC-01:00',  -3600  ],
   17 : [ u'UTC-00:00',      0  ],
   18 : [ u'UTC+00:00',      0  ],
   19 : [ u'UTC+01:00',   3600  ],
   20 : [ u'UTC+02:00',   7200  ],
   21 : [ u'UTC+03:00',  10800  ],
   22 : [ u'UTC+03:30',  12600  ],
   23 : [ u'UTC+04:00',  14400  ],
   24 : [ u'UTC+04:30',  16200  ],
   25 : [ u'UTC+05:00',  18000  ],
   26 : [ u'UTC+05:30',  19800  ],
   27 : [ u'UTC+05:45',  20700  ],
   28 : [ u'UTC+06:00',  21600  ],
   29 : [ u'UTC+06:30',  23400  ],
   30 : [ u'UTC+07:00',  25200  ],
   31 : [ u'UTC+08:00',  28800  ],
   32 : [ u'UTC+09:00',  32400  ],
   33 : [ u'UTC+09:30',  34200  ],
   34 : [ u'UTC+10:00',  36000  ],
   35 : [ u'UTC+11:00',  39600  ],
   36 : [ u'UTC+12:00',  43200  ],
   37 : [ u'UTC+13:00',  46800  ],
   38 : [ u'UTC+14:00',  50400  ]
}

def usage():
    print "Usage: cldr2qtimezone.py <path to cldr core/common> <path to qtbase>"
    sys.exit()

if len(sys.argv) != 3:
    usage()

cldrPath = sys.argv[1]
qtPath = sys.argv[2]

if not os.path.isdir(cldrPath) or not os.path.isdir(qtPath):
    usage()

windowsZonesPath = cldrPath + "/supplemental/windowsZones.xml"
tempFileDir = qtPath + "/src/corelib/tools"
dataFilePath = qtPath + "/src/corelib/tools/qtimezoneprivate_data_p.h"

if not os.path.isfile(windowsZonesPath):
    usage()

if not os.path.isfile(dataFilePath):
    usage()

# [[u'version', [(u'number', u'$Revision: 7825 $')]]]
versionNumber = findTagsInFile(windowsZonesPath, "version")[0][1][0][1]

# [[u'generation', [(u'date', u'$Date: 2012-10-10 14:45:31 -0700 (Wed, 10 Oct 2012) $')]]]
generationDate = findTagsInFile(windowsZonesPath, "generation")[0][1][0][1]

mapTimezones = findTagsInFile(windowsZonesPath, "windowsZones/mapTimezones")

defaultDict = {}
windowsIdDict = {}

if mapTimezones:
    for mapZone in mapTimezones:
        # [u'mapZone', [(u'territory', u'MH'), (u'other', u'UTC+12'), (u'type', u'Pacific/Majuro Pacific/Kwajalein')]]
        if mapZone[0] == u'mapZone':
            data = {}
            for attribute in mapZone[1]:
                if attribute[0] == u'other':
                    data['windowsId'] = attribute[1]
                if attribute[0] == u'territory':
                    data['countryCode'] = attribute[1]
                if attribute[0] == u'type':
                    data['ianaList'] = attribute[1]

            data['windowsKey'] = windowsIdToKey(data['windowsId'])
            if data['windowsKey'] <= 0:
                raise xpathlite.Error("Unknown Windows ID, please add \"%s\"" % data['windowsId'])

            countryId = 0
            if data['countryCode'] == u'001':
                defaultDict[data['windowsKey']] = data['ianaList']
            else:
                data['countryId'] = enumdata.countryCodeToId(data['countryCode'])
                if data['countryId'] < 0:
                    raise xpathlite.Error("Unknown Country Code \"%s\"" % data['countryCode'])
                data['country'] = enumdata.country_list[data['countryId']][0]
                windowsIdDict[data['windowsKey'], data['countryId']] = data

print "Input file parsed, now writing data"

GENERATED_BLOCK_START = "// GENERATED PART STARTS HERE\n"
GENERATED_BLOCK_END = "// GENERATED PART ENDS HERE\n"

# Create a temp file to write the new data into
(newTempFile, newTempFilePath) = tempfile.mkstemp("qtimezone_data_p", dir=tempFileDir)
newTempFile = os.fdopen(newTempFile, "w")

# Open the old file and copy over the first non-generated section to the new file
oldDataFile = open(dataFilePath, "r")
s = oldDataFile.readline()
while s and s != GENERATED_BLOCK_START:
    newTempFile.write(s)
    s = oldDataFile.readline()

# Write out generated block start tag and warning
newTempFile.write(GENERATED_BLOCK_START)
newTempFile.write("\n\
/*\n\
    This part of the file was generated on %s from the\n\
    Common Locale Data Repository supplemental/windowsZones.xml file\n\
    %s %s\n\
\n\
    http://www.unicode.org/cldr/\n\
\n\
    Do not change this data, only generate it using cldr2qtimezone.py.\n\
*/\n\n" % (str(datetime.date.today()), versionNumber, generationDate) )

windowsIdData = ByteArrayData()
ianaIdData = ByteArrayData()

# Write Windows/IANA table
newTempFile.write("// Windows ID Key, Country Enum, IANA ID Index\n")
newTempFile.write("static const QZoneData zoneDataTable[] = {\n")
for index in windowsIdDict:
    data = windowsIdDict[index]
    newTempFile.write("    { %6d,%6d,%6d }, // %s / %s\n" \
                         % (data['windowsKey'],
                            data['countryId'],
                            ianaIdData.append(data['ianaList']),
                            data['windowsId'],
                            data['country']))
newTempFile.write("    {      0,     0,     0 } // Trailing zeroes\n")
newTempFile.write("};\n\n")

print "Done Zone Data"

# Write Windows ID key table
newTempFile.write("// Windows ID Key, Windows ID Index, IANA ID Index, UTC Offset\n")
newTempFile.write("static const QWindowsData windowsDataTable[] = {\n")
for windowsKey in windowsIdList:
    newTempFile.write("    { %6d,%6d,%6d,%6d }, // %s\n" \
                         % (windowsKey,
                            windowsIdData.append(windowsIdList[windowsKey][0]),
                            ianaIdData.append(defaultDict[windowsKey]),
                            windowsIdList[windowsKey][1],
                            windowsIdList[windowsKey][0]))
newTempFile.write("    {      0,     0,     0,     0 } // Trailing zeroes\n")
newTempFile.write("};\n\n")

print "Done Windows Data Table"

# Write UTC ID key table
newTempFile.write("// IANA ID Index, UTC Offset\n")
newTempFile.write("static const QUtcData utcDataTable[] = {\n")
for index in utcIdList:
    data = utcIdList[index]
    newTempFile.write("    { %6d,%6d }, // %s\n" \
                         % (ianaIdData.append(data[0]),
                            data[1],
                            data[0]))
newTempFile.write("    {     0,      0 } // Trailing zeroes\n")
newTempFile.write("};\n\n")

print "Done UTC Data Table"

# Write out Windows ID's data
newTempFile.write("static const char windowsIdData[] = {\n")
newTempFile.write(wrap_list(windowsIdData.data))
newTempFile.write("\n};\n\n")

# Write out IANA ID's data
newTempFile.write("static const char ianaIdData[] = {\n")
newTempFile.write(wrap_list(ianaIdData.data))
newTempFile.write("\n};\n")

print "Done ID Data Table"

# Write out the end of generated block tag
newTempFile.write(GENERATED_BLOCK_END)
s = oldDataFile.readline()

# Skip through the old generated data in the old file
while s and s != GENERATED_BLOCK_END:
    s = oldDataFile.readline()

# Now copy the rest of the original file into the new file
s = oldDataFile.readline()
while s:
    newTempFile.write(s)
    s = oldDataFile.readline()

# Now close the old and new file, delete the old file and copy the new file in its place
newTempFile.close()
oldDataFile.close()
os.remove(dataFilePath)
os.rename(newTempFilePath, dataFilePath)

print "Data generation completed, please check the new file at " + dataFilePath
