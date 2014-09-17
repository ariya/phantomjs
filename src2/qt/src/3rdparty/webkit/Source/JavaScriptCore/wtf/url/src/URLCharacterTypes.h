// Copyright 2010, Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef URLCharacterTypes_h
#define URLCharacterTypes_h

namespace WTF {

class URLCharacterTypes {
public:
    static inline bool isQueryChar(unsigned char c) { return isCharOfType(c, QueryCharacter); }
    static inline bool isIPv4Char(unsigned char c) { return isCharOfType(c, IPv4Character); }
    static inline bool isHexChar(unsigned char c) { return isCharOfType(c, HexCharacter); }

private:
    enum CharTypes {
        InvalidCharacter = 0,
        QueryCharacter = 1 << 0,
        UserInfoCharacter = 1 << 1,
        IPv4Character = 1 << 2,
        HexCharacter = 1 << 3,
        DecimalCharacter = 1 << 4,
        OctalCharacter = 1 << 5,
    };

    static const unsigned char characterTypeTable[0x100];

    static inline bool isCharOfType(unsigned char c, CharTypes type)
    {
        return !!(characterTypeTable[c] & type);
    }
};

}

#endif
