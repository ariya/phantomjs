/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WOFFFileFormat.h"
#include <zlib.h>

#if !USE(OPENTYPE_SANITIZER)

#include "SharedBuffer.h"
#include <wtf/ByteOrder.h>

namespace WebCore {

static bool readUInt32(SharedBuffer* buffer, size_t& offset, uint32_t& value)
{
    ASSERT_ARG(offset, offset <= buffer->size());
    if (buffer->size() - offset < sizeof(value))
        return false;

    value = ntohl(*reinterpret_cast_ptr<const uint32_t*>(buffer->data() + offset));
    offset += sizeof(value);

    return true;
}

static bool readUInt16(SharedBuffer* buffer, size_t& offset, uint16_t& value)
{
    ASSERT_ARG(offset, offset <= buffer->size());
    if (buffer->size() - offset < sizeof(value))
        return false;

    value = ntohs(*reinterpret_cast_ptr<const uint16_t*>(buffer->data() + offset));
    offset += sizeof(value);

    return true;
}

static bool writeUInt32(Vector<char>& vector, uint32_t value)
{
    uint32_t bigEndianValue = htonl(value);
    return vector.tryAppend(reinterpret_cast_ptr<char*>(&bigEndianValue), sizeof(bigEndianValue));
}

static bool writeUInt16(Vector<char>& vector, uint16_t value)
{
    uint16_t bigEndianValue = htons(value);
    return vector.tryAppend(reinterpret_cast_ptr<char*>(&bigEndianValue), sizeof(bigEndianValue));
}

static const uint32_t woffSignature = 0x774f4646; /* 'wOFF' */

bool isWOFF(SharedBuffer* buffer)
{
    size_t offset = 0;
    uint32_t signature;

    return readUInt32(buffer, offset, signature) && signature == woffSignature;
}

bool convertWOFFToSfnt(SharedBuffer* woff, Vector<char>& sfnt)
{
    ASSERT_ARG(sfnt, sfnt.isEmpty());

    size_t offset = 0;

    // Read the WOFF header.
    uint32_t signature;
    if (!readUInt32(woff, offset, signature) || signature != woffSignature) {
        ASSERT_NOT_REACHED();
        return false;
    }

    uint32_t flavor;
    if (!readUInt32(woff, offset, flavor))
        return false;

    uint32_t length;
    if (!readUInt32(woff, offset, length) || length != woff->size())
        return false;

    uint16_t numTables;
    if (!readUInt16(woff, offset, numTables))
        return false;

    if (!numTables || numTables > 0x0fff)
        return false;

    uint16_t reserved;
    if (!readUInt16(woff, offset, reserved) || reserved)
        return false;

    uint32_t totalSfntSize;
    if (!readUInt32(woff, offset, totalSfntSize))
        return false;

    if (woff->size() - offset < sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t))
        return false;

    offset += sizeof(uint16_t); // majorVersion
    offset += sizeof(uint16_t); // minorVersion
    offset += sizeof(uint32_t); // metaOffset
    offset += sizeof(uint32_t); // metaLength
    offset += sizeof(uint32_t); // metaOrigLength
    offset += sizeof(uint32_t); // privOffset
    offset += sizeof(uint32_t); // privLength

    // Check if the WOFF can supply as many tables as it claims it has.
    if (woff->size() - offset < numTables * 5 * sizeof(uint32_t))
        return false;

    // Write the sfnt offset subtable.
    uint16_t entrySelector = 0;
    uint16_t searchRange = 1;
    while (searchRange < numTables >> 1) {
        entrySelector++;
        searchRange <<= 1;
    }
    searchRange <<= 4;
    uint16_t rangeShift = (numTables << 4) - searchRange;

    if (!writeUInt32(sfnt, flavor)
        || !writeUInt16(sfnt, numTables)
        || !writeUInt16(sfnt, searchRange)
        || !writeUInt16(sfnt, entrySelector)
        || !writeUInt16(sfnt, rangeShift))
        return false;

    if (sfnt.size() > totalSfntSize)
        return false;

    if (totalSfntSize - sfnt.size() < numTables * 4 * sizeof(uint32_t))
        return false;

    size_t sfntTableDirectoryCursor = sfnt.size();
    sfnt.grow(sfnt.size() + numTables * 4 * sizeof(uint32_t));

    // Process tables.
    for (uint16_t i = 0; i < numTables; ++i) {
        // Read a WOFF table directory entry.
        uint32_t tableTag;
        if (!readUInt32(woff, offset, tableTag))
            return false;

        uint32_t tableOffset;
        if (!readUInt32(woff, offset, tableOffset))
            return false;

        uint32_t tableCompLength;
        if (!readUInt32(woff, offset, tableCompLength))
            return false;

        if (tableOffset > woff->size() || tableCompLength > woff->size() - tableOffset)
            return false;

        uint32_t tableOrigLength;
        if (!readUInt32(woff, offset, tableOrigLength) || tableCompLength > tableOrigLength)
            return false;

        if (tableOrigLength > totalSfntSize || sfnt.size() > totalSfntSize - tableOrigLength)
            return false;

        uint32_t tableOrigChecksum;
        if (!readUInt32(woff, offset, tableOrigChecksum))
            return false;

        // Write an sfnt table directory entry.
        uint32_t* sfntTableDirectoryPtr = reinterpret_cast_ptr<uint32_t*>(sfnt.data() + sfntTableDirectoryCursor);
        *sfntTableDirectoryPtr++ = htonl(tableTag);
        *sfntTableDirectoryPtr++ = htonl(tableOrigChecksum);
        *sfntTableDirectoryPtr++ = htonl(sfnt.size());
        *sfntTableDirectoryPtr++ = htonl(tableOrigLength);
        sfntTableDirectoryCursor += 4 * sizeof(uint32_t);

        if (tableCompLength == tableOrigLength) {
            // The table is not compressed.
            if (!sfnt.tryAppend(woff->data() + tableOffset, tableCompLength))
                return false;
        } else {
            uLongf destLen = tableOrigLength;
            if (!sfnt.tryReserveCapacity(sfnt.size() + tableOrigLength))
                return false;
            Bytef* dest = reinterpret_cast<Bytef*>(sfnt.end());
            sfnt.grow(sfnt.size() + tableOrigLength);
            if (uncompress(dest, &destLen, reinterpret_cast<const Bytef*>(woff->data() + tableOffset), tableCompLength) != Z_OK)
                return false;
            if (destLen != tableOrigLength)
                return false;
        }

        // Pad to a multiple of 4 bytes.
        while (sfnt.size() % 4)
            sfnt.append(0);
    }

    return sfnt.size() == totalSfntSize;
}
    
} // namespace WebCore

#endif // !USE(OPENTYPE_SANITIZER)
