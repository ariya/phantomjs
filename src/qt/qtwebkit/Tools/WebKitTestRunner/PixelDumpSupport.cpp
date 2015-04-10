/*
 * Copyright (C) 2009 Apple, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PixelDumpSupport.h"

#include "CyclicRedundancyCheck.h"
#include <cstdio>

static void appendIntToVector(unsigned number, Vector<unsigned char>& vector)
{
    size_t offset = vector.size();
    vector.grow(offset + 4);
    vector[offset] = ((number >> 24) & 0xff);
    vector[offset + 1] = ((number >> 16) & 0xff);
    vector[offset + 2] = ((number >> 8) & 0xff);
    vector[offset + 3] = (number & 0xff);
}

static void convertChecksumToPNGComment(const char* checksum, Vector<unsigned char>& bytesToAdd)
{
    // Chunks of PNG files are <length>, <type>, <data>, <crc>.
    static const char textCommentPrefix[] = "\x00\x00\x00\x29tEXtchecksum\x00";
    static const size_t prefixLength = sizeof(textCommentPrefix) - 1; // The -1 is for the null at the end of the char[].
    static const size_t checksumLength = 32;

    bytesToAdd.append(textCommentPrefix, prefixLength);
    bytesToAdd.append(checksum, checksumLength);

    Vector<unsigned char> dataToCrc;
    dataToCrc.append(textCommentPrefix + 4, prefixLength - 4); // Don't include the chunk length in the crc.
    dataToCrc.append(checksum, checksumLength);
    unsigned crc32 = computeCrc(dataToCrc);

    appendIntToVector(crc32, bytesToAdd);
}

static size_t offsetAfterIHDRChunk(const unsigned char* data, const size_t dataLength)
{
    const int pngHeaderLength = 8;
    const int pngIHDRChunkLength = 25; // chunk length + "IHDR" + 13 bytes of data + checksum
    return pngHeaderLength + pngIHDRChunkLength;
}

void printPNG(const unsigned char* data, const size_t dataLength, const char* checksum)
{
    Vector<unsigned char> bytesToAdd;
    convertChecksumToPNGComment(checksum, bytesToAdd);

    printf("Content-Type: %s\n", "image/png");
    printf("Content-Length: %lu\n", static_cast<unsigned long>(dataLength + bytesToAdd.size()));

    size_t insertOffset = offsetAfterIHDRChunk(data, dataLength);

    fwrite(data, 1, insertOffset, stdout);
    fwrite(bytesToAdd.data(), 1, bytesToAdd.size(), stdout);

    const size_t bytesToWriteInOneChunk = 1 << 15;
    data += insertOffset;
    size_t dataRemainingToWrite = dataLength - insertOffset;
    while (dataRemainingToWrite) {
        size_t bytesToWriteInThisChunk = std::min(dataRemainingToWrite, bytesToWriteInOneChunk);
        size_t bytesWritten = fwrite(data, 1, bytesToWriteInThisChunk, stdout);
        if (bytesWritten != bytesToWriteInThisChunk)
            break;
        dataRemainingToWrite -= bytesWritten;
        data += bytesWritten;
    }
}
