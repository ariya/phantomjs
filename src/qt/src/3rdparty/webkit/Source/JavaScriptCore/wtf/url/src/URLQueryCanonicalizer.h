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


#ifndef URLQueryCanonicalizer_h
#define URLQueryCanonicalizer_h

#include "RawURLBuffer.h"
#include "URLBuffer.h"
#include "URLCharacterTypes.h"
#include "URLComponent.h"
#include "URLEscape.h"

namespace WTF {

template<typename InChar, typename OutChar, void convertCharset(const InChar*, int length, URLBuffer<char>&)>
class URLQueryCanonicalizer {
public:
    static void canonicalize(const InChar* spec, const URLComponent& query, URLBuffer<OutChar>& buffer, URLComponent& resultQuery)
    {
        if (query.length() < 0) {
            resultQuery = URLComponent();
            return;
        }

        buffer->append('?');
        resultQuery.setBegin(buffer->length());
        convertToQueryEncoding(spec, query, buffer);
        resultQuery.setLength(buffer->length() - resultQuery.begin());
    }

private:
    static bool isAllASCII(const InChar* spec, const URLComponent& query)
    {
        int end = query.end();
        for (int i = query.begin(); i < end; ++i) {
            if (static_cast<unsigned>(spec[i]) >= 0x80)
                return false;
        }
        return true;
    }

#ifndef NDEBUG
    static bool isRaw8Bit(const InChar* source, int length)
    {
        for (int i = source; i < length; ++i) {
            if (source[i] & 0xFF != source[i])
                return false;
        }
        return true;
    }
#endif

    static void appendRaw8BitQueryString(const InChar* source, int length, URLBuffer<OutChar>* buffer)
    {
        ASSERT(isRaw8Bit(source, length));
        for (int i = 0; i < length; ++i) {
            if (!URLCharacterTypes::isQueryChar(source[i]))
                appendURLEscapedCharacter(static_cast<unsigned char>(source[i]), buffer);
            else
                buffer->append(static_cast<char>(source[i]));
        }
    }

    static void convertToQueryEncoding(const InChar* spec, const URLComponent& query, URLBuffer<OutChar>& buffer)
    {
        if (isAllASCII(spec, query)) {
            appendRaw8BitQueryString(&spec[query.begin()], query.length(), buffer);
            return;
        }

        RawURLBuffer<char, 1024> convertedQuery;
        convertCharset(spec, query, convertedQuery);
        appendRaw8BitQueryString(convertedQuery.data(), convertedQuery.length(), buffer);
    }
};

}

#endif


