/*
 * Copyright (C) 2009, 2010, 2011 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef WebKitTextCodec_h
#define WebKitTextCodec_h

#include "BlackBerryGlobal.h"
#include <vector>

namespace BlackBerry {
namespace Platform {
class String;
}

namespace WebKit {

enum TranscodeResult {
    Success,
    SourceEncodingUnsupported,
    SourceBroken,
    TargetEncodingUnsupported,
    TargetBufferInsufficient
};

enum Base64DecodePolicy {
    Base64FailOnInvalidCharacter,
    Base64IgnoreWhitespace,
    Base64IgnoreInvalidCharacters
};

enum Base64EncodePolicy {
    Base64DoNotInsertCRLF,
    Base64InsertCRLF
};

BLACKBERRY_EXPORT bool isSameEncoding(const char* encoding1, const char* encoding2);
BLACKBERRY_EXPORT bool isASCIICompatibleEncoding(const char* encoding);
BLACKBERRY_EXPORT TranscodeResult transcode(const char* sourceEncoding, const char* targetEncoding, const char*& sourceStart, int sourceLength, char*& targetStart, unsigned targetLength);

BLACKBERRY_EXPORT bool base64Decode(const BlackBerry::Platform::String& base64, std::vector<char>& binary, Base64DecodePolicy);
BLACKBERRY_EXPORT bool base64Encode(const std::vector<char>& binary, BlackBerry::Platform::String& base64, Base64EncodePolicy);

BLACKBERRY_EXPORT void unescapeURL(const BlackBerry::Platform::String& escaped, BlackBerry::Platform::String& url);
BLACKBERRY_EXPORT void escapeURL(const BlackBerry::Platform::String& url, BlackBerry::Platform::String& escaped);

} // namespace WebKit
} // namespace BlackBerry

#endif // WebKitTextCodec_h
