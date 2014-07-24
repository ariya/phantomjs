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

#ifndef OpenTypeUtilities_h
#define OpenTypeUtilities_h

#include <windows.h>
#include <wtf/Forward.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

struct BigEndianUShort;
struct EOTPrefix;
class SharedBuffer;

#if OS(WINCE)
typedef unsigned __int8 UInt8;
#endif

struct EOTHeader {
    EOTHeader();

    size_t size() const { return m_buffer.size(); }
    const uint8_t* data() const { return m_buffer.data(); }

    EOTPrefix* prefix() { return reinterpret_cast<EOTPrefix*>(m_buffer.data()); }
    void updateEOTSize(size_t);
    void appendBigEndianString(const BigEndianUShort*, unsigned short length);
    void appendPaddingShort();

private:
    Vector<uint8_t, 512> m_buffer;
};

bool getEOTHeader(SharedBuffer* fontData, EOTHeader& eotHeader, size_t& overlayDst, size_t& overlaySrc, size_t& overlayLength);
HANDLE renameAndActivateFont(SharedBuffer*, const String&);

} // namespace WebCore

#endif // OpenTypeUtilities_h
