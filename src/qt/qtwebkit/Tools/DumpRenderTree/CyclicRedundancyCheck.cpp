/*
 * Copyright (C) 2010, Robert Eisele <robert@xarg.org> All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CyclicRedundancyCheck.h"

#include <wtf/Vector.h>

static void makeCrcTable(unsigned crcTable[256])
{
    for (unsigned i = 0; i < 256; i++) {
        unsigned c = i;
        for (int k = 0; k < 8; k++) {
            if (c & 1)
                c = -306674912 ^ ((c >> 1) & 0x7fffffff);
            else
                c = c >> 1;
        }
        crcTable[i] = c;
    }
}

unsigned computeCrc(const Vector<unsigned char>& buffer)
{
    static unsigned crcTable[256];
    static bool crcTableComputed = false;
    if (!crcTableComputed) {
        makeCrcTable(crcTable);
        crcTableComputed = true;
    }

    unsigned crc = 0xffffffffU;
    for (size_t i = 0; i < buffer.size(); ++i)
        crc = crcTable[(crc ^ buffer[i]) & 0xff] ^ ((crc >> 8) & 0x00ffffffU);
    return crc ^ 0xffffffffU;
}

