/*
 * Copyright (C) 2008, 2009 Apple Inc.  All rights reserved.
 * Copyright (C) 2009 Torch Mobile, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "OpenTypeUtilities.h"

#include "SharedBuffer.h"

namespace WebCore {

struct BigEndianUShort { 
    operator unsigned short() const { return (v & 0x00ff) << 8 | v >> 8; }
    BigEndianUShort(unsigned short u) : v((u & 0x00ff) << 8 | u >> 8) { }
    unsigned short v;
};

struct BigEndianULong { 
    operator unsigned() const { return (v & 0xff) << 24 | (v & 0xff00) << 8 | (v & 0xff0000) >> 8 | v >> 24; }
    BigEndianULong(unsigned u) : v((u & 0xff) << 24 | (u & 0xff00) << 8 | (u & 0xff0000) >> 8 | u >> 24) { }
    unsigned v;
};

#pragma pack(1)

struct EOTPrefix {
    unsigned eotSize;
    unsigned fontDataSize;
    unsigned version;
    unsigned flags;
    uint8_t fontPANOSE[10];
    uint8_t charset;
    uint8_t italic;
    unsigned weight;
    unsigned short fsType;
    unsigned short magicNumber;
    unsigned unicodeRange[4];
    unsigned codePageRange[2];
    unsigned checkSumAdjustment;
    unsigned reserved[4];
    unsigned short padding1;
};

struct TableDirectoryEntry {
    BigEndianULong tag;
    BigEndianULong checkSum;
    BigEndianULong offset;
    BigEndianULong length;
};

#if !USE(CG) || !defined(COREGRAPHICS_INCLUDES_CORESERVICES_HEADER)
// Fixed type is not defined on non-CG and Windows platforms. |version| in sfntHeader
// and headTable and |fontRevision| in headTable are of Fixed, but they're
// not actually refered to anywhere. Therefore, we just have to match
// the size (4 bytes). For the definition of Fixed type, see
// http://developer.apple.com/documentation/mac/Legacy/GXEnvironment/GXEnvironment-356.html#HEADING356-6.
typedef int32_t Fixed;
#endif

struct sfntHeader {
    Fixed version;
    BigEndianUShort numTables;
    BigEndianUShort searchRange;
    BigEndianUShort entrySelector;
    BigEndianUShort rangeShift;
    TableDirectoryEntry tables[1];
};

struct OS2Table {
    BigEndianUShort version;
    BigEndianUShort avgCharWidth;
    BigEndianUShort weightClass;
    BigEndianUShort widthClass;
    BigEndianUShort fsType;
    BigEndianUShort subscriptXSize;
    BigEndianUShort subscriptYSize;
    BigEndianUShort subscriptXOffset;
    BigEndianUShort subscriptYOffset;
    BigEndianUShort superscriptXSize;
    BigEndianUShort superscriptYSize;
    BigEndianUShort superscriptXOffset;
    BigEndianUShort superscriptYOffset;
    BigEndianUShort strikeoutSize;
    BigEndianUShort strikeoutPosition;
    BigEndianUShort familyClass;
    uint8_t panose[10];
    BigEndianULong unicodeRange[4];
    uint8_t vendID[4];
    BigEndianUShort fsSelection;
    BigEndianUShort firstCharIndex;
    BigEndianUShort lastCharIndex;
    BigEndianUShort typoAscender;
    BigEndianUShort typoDescender;
    BigEndianUShort typoLineGap;
    BigEndianUShort winAscent;
    BigEndianUShort winDescent;
    BigEndianULong codePageRange[2];
    BigEndianUShort xHeight;
    BigEndianUShort capHeight;
    BigEndianUShort defaultChar;
    BigEndianUShort breakChar;
    BigEndianUShort maxContext;
};

struct headTable {
    Fixed version;
    Fixed fontRevision;
    BigEndianULong checkSumAdjustment;
    BigEndianULong magicNumber;
    BigEndianUShort flags;
    BigEndianUShort unitsPerEm;
    long long created;
    long long modified;
    BigEndianUShort xMin;
    BigEndianUShort xMax;
    BigEndianUShort yMin;
    BigEndianUShort yMax;
    BigEndianUShort macStyle;
    BigEndianUShort lowestRectPPEM;
    BigEndianUShort fontDirectionHint;
    BigEndianUShort indexToLocFormat;
    BigEndianUShort glyphDataFormat;
};

struct nameRecord {
    BigEndianUShort platformID;
    BigEndianUShort encodingID;
    BigEndianUShort languageID;
    BigEndianUShort nameID;
    BigEndianUShort length;
    BigEndianUShort offset;
};

struct nameTable {
    BigEndianUShort format;
    BigEndianUShort count;
    BigEndianUShort stringOffset;
    nameRecord nameRecords[1];
};

#pragma pack()

EOTHeader::EOTHeader()
{
    m_buffer.resize(sizeof(EOTPrefix));
}

void EOTHeader::updateEOTSize(size_t fontDataSize)
{
    prefix()->eotSize = m_buffer.size() + fontDataSize;
}

void EOTHeader::appendBigEndianString(const BigEndianUShort* string, unsigned short length)
{
    size_t oldSize = m_buffer.size();
    m_buffer.resize(oldSize + length + 2 * sizeof(unsigned short));
    UChar* dst = reinterpret_cast<UChar*>(m_buffer.data() + oldSize);
    unsigned i = 0;
    dst[i++] = length;
    unsigned numCharacters = length / 2;
    for (unsigned j = 0; j < numCharacters; j++)
        dst[i++] = string[j];
    dst[i] = 0;
}

void EOTHeader::appendPaddingShort()
{
    unsigned short padding = 0;
    m_buffer.append(reinterpret_cast<uint8_t*>(&padding), sizeof(padding));
}

bool getEOTHeader(SharedBuffer* fontData, EOTHeader& eotHeader, size_t& overlayDst, size_t& overlaySrc, size_t& overlayLength)
{
    overlayDst = 0;
    overlaySrc = 0;
    overlayLength = 0;

    size_t dataLength = fontData->size();
    const char* data = fontData->data();

    EOTPrefix* prefix = eotHeader.prefix();

    prefix->fontDataSize = dataLength;
    prefix->version = 0x00020001;
    prefix->flags = 0;

    if (dataLength < offsetof(sfntHeader, tables))
        return false;

    const sfntHeader* sfnt = reinterpret_cast<const sfntHeader*>(data);

    if (dataLength < offsetof(sfntHeader, tables) + sfnt->numTables * sizeof(TableDirectoryEntry))
        return false;

    bool haveOS2 = false;
    bool haveHead = false;
    bool haveName = false;

    const BigEndianUShort* familyName = 0;
    unsigned short familyNameLength = 0;
    const BigEndianUShort* subfamilyName = 0;
    unsigned short subfamilyNameLength = 0;
    const BigEndianUShort* fullName = 0;
    unsigned short fullNameLength = 0;
    const BigEndianUShort* versionString = 0;
    unsigned short versionStringLength = 0;

    for (unsigned i = 0; i < sfnt->numTables; i++) {
        unsigned tableOffset = sfnt->tables[i].offset;
        unsigned tableLength = sfnt->tables[i].length;

        if (dataLength < tableOffset || dataLength < tableLength || dataLength < tableOffset + tableLength)
            return false;

        unsigned tableTag = sfnt->tables[i].tag;
        switch (tableTag) {
            case 'OS/2':
                {
                    if (dataLength < tableOffset + sizeof(OS2Table))
                        return false;

                    haveOS2 = true;
                    const OS2Table* OS2 = reinterpret_cast<const OS2Table*>(data + tableOffset);
                    for (unsigned j = 0; j < 10; j++)
                        prefix->fontPANOSE[j] = OS2->panose[j];
                    prefix->italic = OS2->fsSelection & 0x01;
                    prefix->weight = OS2->weightClass;
                    // FIXME: Should use OS2->fsType, but some TrueType fonts set it to an over-restrictive value.
                    // Since ATS does not enforce this on Mac OS X, we do not enforce it either.
                    prefix->fsType = 0;            
                    for (unsigned j = 0; j < 4; j++)
                        prefix->unicodeRange[j] = OS2->unicodeRange[j];
                    for (unsigned j = 0; j < 2; j++)
                        prefix->codePageRange[j] = OS2->codePageRange[j];
                    break;
                }
            case 'head':
                {
                    if (dataLength < tableOffset + sizeof(headTable))
                        return false;

                    haveHead = true;
                    const headTable* head = reinterpret_cast<const headTable*>(data + tableOffset);
                    prefix->checkSumAdjustment = head->checkSumAdjustment;
                    break;
                }
            case 'name':
                {
                    if (dataLength < tableOffset + offsetof(nameTable, nameRecords))
                        return false;

                    haveName = true;
                    const nameTable* name = reinterpret_cast<const nameTable*>(data + tableOffset);
                    for (int j = 0; j < name->count; j++) {
                        if (dataLength < tableOffset + offsetof(nameTable, nameRecords) + (j + 1) * sizeof(nameRecord))
                            return false;
                        if (name->nameRecords[j].platformID == 3 && name->nameRecords[j].encodingID == 1 && name->nameRecords[j].languageID == 0x0409) {
                            if (dataLength < tableOffset + name->stringOffset + name->nameRecords[j].offset + name->nameRecords[j].length)
                                return false;

                            unsigned short nameLength = name->nameRecords[j].length;
                            const BigEndianUShort* nameString = reinterpret_cast<const BigEndianUShort*>(data + tableOffset + name->stringOffset + name->nameRecords[j].offset);
                            
                            switch (name->nameRecords[j].nameID) {
                                case 1:
                                    familyNameLength = nameLength;
                                    familyName = nameString;
                                    break;
                                case 2:
                                    subfamilyNameLength = nameLength;
                                    subfamilyName = nameString;
                                    break;
                                case 4:
                                    fullNameLength = nameLength;
                                    fullName = nameString;
                                    break;
                                case 5:
                                    versionStringLength = nameLength;
                                    versionString = nameString;
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                    break;
                }
            default:
                break;
        }
        if (haveOS2 && haveHead && haveName)
            break;
    }

    prefix->charset = DEFAULT_CHARSET;
    prefix->magicNumber = 0x504c;
    prefix->reserved[0] = 0;
    prefix->reserved[1] = 0;
    prefix->reserved[2] = 0;
    prefix->reserved[3] = 0;
    prefix->padding1 = 0;

    eotHeader.appendBigEndianString(familyName, familyNameLength);
    eotHeader.appendBigEndianString(subfamilyName, subfamilyNameLength);
    eotHeader.appendBigEndianString(versionString, versionStringLength);

    // If possible, ensure that the family name is a prefix of the full name.
    if (fullNameLength >= familyNameLength && memcmp(familyName, fullName, familyNameLength)) {
        overlaySrc = reinterpret_cast<const char*>(fullName) - data;
        overlayDst = reinterpret_cast<const char*>(familyName) - data;
        overlayLength = familyNameLength;
    }
    eotHeader.appendBigEndianString(fullName, fullNameLength);

    eotHeader.appendPaddingShort();
    eotHeader.updateEOTSize(fontData->size());

    return true;
}

// code shared by renameFont and renameAndActivateFont
// adds fontName to the font table in fontData, and writes the new font table to rewrittenFontTable
// returns the size of the name table (which is used by renameAndActivateFont), or 0 on early abort
static size_t renameFontInternal(SharedBuffer* fontData, const String& fontName, Vector<char> &rewrittenFontData)
{
    size_t originalDataSize = fontData->size();
    const sfntHeader* sfnt = reinterpret_cast<const sfntHeader*>(fontData->data());

    unsigned t;
    for (t = 0; t < sfnt->numTables; ++t) {
        if (sfnt->tables[t].tag == 'name')
            break;
    }
    if (t == sfnt->numTables)
        return 0;

    const int nameRecordCount = 5;

    // Rounded up to a multiple of 4 to simplify the checksum calculation.
    size_t nameTableSize = ((offsetof(nameTable, nameRecords) + nameRecordCount * sizeof(nameRecord) + fontName.length() * sizeof(UChar)) & ~3) + 4;

    rewrittenFontData.resize(fontData->size() + nameTableSize);
    char* data = rewrittenFontData.data();
    memcpy(data, fontData->data(), originalDataSize);

    // Make the table directory entry point to the new 'name' table.
    sfntHeader* rewrittenSfnt = reinterpret_cast<sfntHeader*>(data);
    rewrittenSfnt->tables[t].length = nameTableSize;
    rewrittenSfnt->tables[t].offset = originalDataSize;

    // Write the new 'name' table after the original font data.
    nameTable* name = reinterpret_cast<nameTable*>(data + originalDataSize);
    name->format = 0;
    name->count = nameRecordCount;
    name->stringOffset = offsetof(nameTable, nameRecords) + nameRecordCount * sizeof(nameRecord);
    for (unsigned i = 0; i < nameRecordCount; ++i) {
        name->nameRecords[i].platformID = 3;
        name->nameRecords[i].encodingID = 1;
        name->nameRecords[i].languageID = 0x0409;
        name->nameRecords[i].offset = 0;
        name->nameRecords[i].length = fontName.length() * sizeof(UChar);
    }

    // The required 'name' record types: Family, Style, Unique, Full and PostScript.
    name->nameRecords[0].nameID = 1;
    name->nameRecords[1].nameID = 2;
    name->nameRecords[2].nameID = 3;
    name->nameRecords[3].nameID = 4;
    name->nameRecords[4].nameID = 6;

    for (unsigned i = 0; i < fontName.length(); ++i)
        reinterpret_cast<BigEndianUShort*>(data + originalDataSize + name->stringOffset)[i] = fontName[i];

    // Update the table checksum in the directory entry.
    rewrittenSfnt->tables[t].checkSum = 0;
    for (unsigned i = 0; i * sizeof(BigEndianULong) < nameTableSize; ++i)
        rewrittenSfnt->tables[t].checkSum = rewrittenSfnt->tables[t].checkSum + reinterpret_cast<BigEndianULong*>(name)[i];

    return nameTableSize;
}

#if OS(WINCE)
// AddFontMemResourceEx does not exist on WinCE, so we must handle the font data manually
// This function just renames the font and overwrites the old font data with the new
bool renameFont(SharedBuffer* fontData, const String& fontName)
{
    // abort if the data is too small to be a font header with a "tables" entry
    if (fontData->size() < offsetof(sfntHeader, tables))
        return false;

    // abort if the data is too small to hold all the tables specified in the header
    const sfntHeader* header = reinterpret_cast<const sfntHeader*>(fontData->data());
    if (fontData->size() < offsetof(sfntHeader, tables) + header->numTables * sizeof(TableDirectoryEntry))
        return false;

    Vector<char> rewrittenFontData;
    if (!renameFontInternal(fontData, fontName, rewrittenFontData))
        return false;

    fontData->clear();
    fontData->append(rewrittenFontData.data(), rewrittenFontData.size());
    return true;
}
#else
// Rename the font and install the new font data into the system
HANDLE renameAndActivateFont(SharedBuffer* fontData, const String& fontName)
{
    Vector<char> rewrittenFontData;
    size_t nameTableSize = renameFontInternal(fontData, fontName, rewrittenFontData);
    if (!nameTableSize)
        return 0;

    DWORD numFonts = 0;
    HANDLE fontHandle = AddFontMemResourceEx(rewrittenFontData.data(), fontData->size() + nameTableSize, 0, &numFonts);

    if (fontHandle && numFonts < 1) {
        RemoveFontMemResourceEx(fontHandle);
        return 0;
    }

    return fontHandle;
}
#endif

}
