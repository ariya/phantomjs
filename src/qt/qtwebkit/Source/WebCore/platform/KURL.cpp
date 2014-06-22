/*
 * Copyright (C) 2004, 2007, 2008, 2011, 2012, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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
#include "KURL.h"

#include "DecodeEscapeSequences.h"
#include "MIMETypeRegistry.h"
#include "TextEncoding.h"
#include <stdio.h>
#include <wtf/HashMap.h>
#include <wtf/HexNumber.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringHash.h>

#if USE(ICU_UNICODE)
#include <unicode/uidna.h>
#endif

// FIXME: This file makes too much use of the + operator on String.
// We either have to optimize that operator so it doesn't involve
// so many allocations, or change this to use StringBuffer instead.

using namespace std;
using namespace WTF;

namespace WebCore {

typedef Vector<char, 512> CharBuffer;
typedef Vector<UChar, 512> UCharBuffer;

static const unsigned maximumValidPortNumber = 0xFFFE;
static const unsigned invalidPortNumber = 0xFFFF;

static inline bool isLetterMatchIgnoringCase(UChar character, char lowercaseLetter)
{
    ASSERT(isASCIILower(lowercaseLetter));
    return (character | 0x20) == lowercaseLetter;
}

static const char wsScheme[] = {'w', 's'};
static const char ftpScheme[] = {'f', 't', 'p'};
static const char ftpPort[] = {'2', '1'};
static const char wssScheme[] = {'w', 's', 's'};
static const char fileScheme[] = {'f', 'i', 'l', 'e'};
static const char httpScheme[] = {'h', 't', 't', 'p'};
static const char httpPort[] = {'8', '0'};
static const char httpsScheme[] = {'h', 't', 't', 'p', 's'};
static const char httpsPort[] = {'4', '4', '3'};
static const char gopherScheme[] = {'g', 'o', 'p', 'h', 'e', 'r'};
static const char gopherPort[] = {'7', '0'};

static inline bool isLetterMatchIgnoringCase(char character, char lowercaseLetter)
{
    ASSERT(isASCIILower(lowercaseLetter));
    return (character | 0x20) == lowercaseLetter;
}

enum URLCharacterClasses {
    // alpha 
    SchemeFirstChar = 1 << 0,

    // ( alpha | digit | "+" | "-" | "." )
    SchemeChar = 1 << 1,

    // mark        = "-" | "_" | "." | "!" | "~" | "*" | "'" | "(" | ")"
    // unreserved  = alphanum | mark
    // ( unreserved | escaped | ";" | ":" | "&" | "=" | "+" | "$" | "," )
    UserInfoChar = 1 << 2,

    // alnum | "." | "-" | "%"
    // The above is what the specification says, but we are lenient to
    // match existing practice and also allow:
    // "_"
    HostnameChar = 1 << 3,

    // hexdigit | ":" | "%"
    IPv6Char = 1 << 4,

    // "#" | "?" | "/" | nul
    PathSegmentEndChar = 1 << 5,

    // not allowed in path
    BadChar = 1 << 6
};

static const unsigned char characterClassTable[256] = {
    /* 0 nul */ PathSegmentEndChar,    /* 1 soh */ BadChar,
    /* 2 stx */ BadChar,    /* 3 etx */ BadChar,
    /* 4 eot */ BadChar,    /* 5 enq */ BadChar,    /* 6 ack */ BadChar,    /* 7 bel */ BadChar,
    /* 8 bs */ BadChar,     /* 9 ht */ BadChar,     /* 10 nl */ BadChar,    /* 11 vt */ BadChar,
    /* 12 np */ BadChar,    /* 13 cr */ BadChar,    /* 14 so */ BadChar,    /* 15 si */ BadChar,
    /* 16 dle */ BadChar,   /* 17 dc1 */ BadChar,   /* 18 dc2 */ BadChar,   /* 19 dc3 */ BadChar,
    /* 20 dc4 */ BadChar,   /* 21 nak */ BadChar,   /* 22 syn */ BadChar,   /* 23 etb */ BadChar,
    /* 24 can */ BadChar,   /* 25 em */ BadChar,    /* 26 sub */ BadChar,   /* 27 esc */ BadChar,
    /* 28 fs */ BadChar,    /* 29 gs */ BadChar,    /* 30 rs */ BadChar,    /* 31 us */ BadChar,
    /* 32 sp */ BadChar,    /* 33  ! */ UserInfoChar,
    /* 34  " */ BadChar,    /* 35  # */ PathSegmentEndChar | BadChar,
    /* 36  $ */ UserInfoChar,    /* 37  % */ UserInfoChar | HostnameChar | IPv6Char | BadChar,
    /* 38  & */ UserInfoChar,    /* 39  ' */ UserInfoChar,
    /* 40  ( */ UserInfoChar,    /* 41  ) */ UserInfoChar,
    /* 42  * */ UserInfoChar,    /* 43  + */ SchemeChar | UserInfoChar,
    /* 44  , */ UserInfoChar,
    /* 45  - */ SchemeChar | UserInfoChar | HostnameChar,
    /* 46  . */ SchemeChar | UserInfoChar | HostnameChar | IPv6Char,
    /* 47  / */ PathSegmentEndChar,
    /* 48  0 */ SchemeChar | UserInfoChar | HostnameChar | IPv6Char, 
    /* 49  1 */ SchemeChar | UserInfoChar | HostnameChar | IPv6Char,    
    /* 50  2 */ SchemeChar | UserInfoChar | HostnameChar | IPv6Char, 
    /* 51  3 */ SchemeChar | UserInfoChar | HostnameChar | IPv6Char,
    /* 52  4 */ SchemeChar | UserInfoChar | HostnameChar | IPv6Char, 
    /* 53  5 */ SchemeChar | UserInfoChar | HostnameChar | IPv6Char,
    /* 54  6 */ SchemeChar | UserInfoChar | HostnameChar | IPv6Char, 
    /* 55  7 */ SchemeChar | UserInfoChar | HostnameChar | IPv6Char,
    /* 56  8 */ SchemeChar | UserInfoChar | HostnameChar | IPv6Char, 
    /* 57  9 */ SchemeChar | UserInfoChar | HostnameChar | IPv6Char,
    /* 58  : */ UserInfoChar | IPv6Char,    /* 59  ; */ UserInfoChar,
    /* 60  < */ BadChar,    /* 61  = */ UserInfoChar,
    /* 62  > */ BadChar,    /* 63  ? */ PathSegmentEndChar | BadChar,
    /* 64  @ */ 0,
    /* 65  A */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar | IPv6Char,    
    /* 66  B */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar | IPv6Char,
    /* 67  C */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar | IPv6Char,
    /* 68  D */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar | IPv6Char,
    /* 69  E */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar | IPv6Char,
    /* 70  F */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar | IPv6Char,
    /* 71  G */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 72  H */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 73  I */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 74  J */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 75  K */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 76  L */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 77  M */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 78  N */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 79  O */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 80  P */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 81  Q */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 82  R */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 83  S */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 84  T */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 85  U */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 86  V */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 87  W */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 88  X */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar, 
    /* 89  Y */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 90  Z */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 91  [ */ 0,
    /* 92  \ */ 0,    /* 93  ] */ 0,
    /* 94  ^ */ 0,
    /* 95  _ */ UserInfoChar | HostnameChar,
    /* 96  ` */ 0,
    /* 97  a */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar | IPv6Char,
    /* 98  b */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar | IPv6Char, 
    /* 99  c */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar | IPv6Char,
    /* 100  d */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar | IPv6Char, 
    /* 101  e */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar | IPv6Char,
    /* 102  f */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar | IPv6Char, 
    /* 103  g */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 104  h */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar, 
    /* 105  i */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 106  j */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar, 
    /* 107  k */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 108  l */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar, 
    /* 109  m */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 110  n */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar, 
    /* 111  o */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 112  p */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar, 
    /* 113  q */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 114  r */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar, 
    /* 115  s */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 116  t */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar, 
    /* 117  u */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 118  v */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar, 
    /* 119  w */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 120  x */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar, 
    /* 121  y */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar,
    /* 122  z */ SchemeFirstChar | SchemeChar | UserInfoChar | HostnameChar, 
    /* 123  { */ 0,
    /* 124  | */ 0,   /* 125  } */ 0,   /* 126  ~ */ UserInfoChar,   /* 127 del */ BadChar,
    /* 128 */ BadChar, /* 129 */ BadChar, /* 130 */ BadChar, /* 131 */ BadChar,
    /* 132 */ BadChar, /* 133 */ BadChar, /* 134 */ BadChar, /* 135 */ BadChar,
    /* 136 */ BadChar, /* 137 */ BadChar, /* 138 */ BadChar, /* 139 */ BadChar,
    /* 140 */ BadChar, /* 141 */ BadChar, /* 142 */ BadChar, /* 143 */ BadChar,
    /* 144 */ BadChar, /* 145 */ BadChar, /* 146 */ BadChar, /* 147 */ BadChar,
    /* 148 */ BadChar, /* 149 */ BadChar, /* 150 */ BadChar, /* 151 */ BadChar,
    /* 152 */ BadChar, /* 153 */ BadChar, /* 154 */ BadChar, /* 155 */ BadChar,
    /* 156 */ BadChar, /* 157 */ BadChar, /* 158 */ BadChar, /* 159 */ BadChar,
    /* 160 */ BadChar, /* 161 */ BadChar, /* 162 */ BadChar, /* 163 */ BadChar,
    /* 164 */ BadChar, /* 165 */ BadChar, /* 166 */ BadChar, /* 167 */ BadChar,
    /* 168 */ BadChar, /* 169 */ BadChar, /* 170 */ BadChar, /* 171 */ BadChar,
    /* 172 */ BadChar, /* 173 */ BadChar, /* 174 */ BadChar, /* 175 */ BadChar,
    /* 176 */ BadChar, /* 177 */ BadChar, /* 178 */ BadChar, /* 179 */ BadChar,
    /* 180 */ BadChar, /* 181 */ BadChar, /* 182 */ BadChar, /* 183 */ BadChar,
    /* 184 */ BadChar, /* 185 */ BadChar, /* 186 */ BadChar, /* 187 */ BadChar,
    /* 188 */ BadChar, /* 189 */ BadChar, /* 190 */ BadChar, /* 191 */ BadChar,
    /* 192 */ BadChar, /* 193 */ BadChar, /* 194 */ BadChar, /* 195 */ BadChar,
    /* 196 */ BadChar, /* 197 */ BadChar, /* 198 */ BadChar, /* 199 */ BadChar,
    /* 200 */ BadChar, /* 201 */ BadChar, /* 202 */ BadChar, /* 203 */ BadChar,
    /* 204 */ BadChar, /* 205 */ BadChar, /* 206 */ BadChar, /* 207 */ BadChar,
    /* 208 */ BadChar, /* 209 */ BadChar, /* 210 */ BadChar, /* 211 */ BadChar,
    /* 212 */ BadChar, /* 213 */ BadChar, /* 214 */ BadChar, /* 215 */ BadChar,
    /* 216 */ BadChar, /* 217 */ BadChar, /* 218 */ BadChar, /* 219 */ BadChar,
    /* 220 */ BadChar, /* 221 */ BadChar, /* 222 */ BadChar, /* 223 */ BadChar,
    /* 224 */ BadChar, /* 225 */ BadChar, /* 226 */ BadChar, /* 227 */ BadChar,
    /* 228 */ BadChar, /* 229 */ BadChar, /* 230 */ BadChar, /* 231 */ BadChar,
    /* 232 */ BadChar, /* 233 */ BadChar, /* 234 */ BadChar, /* 235 */ BadChar,
    /* 236 */ BadChar, /* 237 */ BadChar, /* 238 */ BadChar, /* 239 */ BadChar,
    /* 240 */ BadChar, /* 241 */ BadChar, /* 242 */ BadChar, /* 243 */ BadChar,
    /* 244 */ BadChar, /* 245 */ BadChar, /* 246 */ BadChar, /* 247 */ BadChar,
    /* 248 */ BadChar, /* 249 */ BadChar, /* 250 */ BadChar, /* 251 */ BadChar,
    /* 252 */ BadChar, /* 253 */ BadChar, /* 254 */ BadChar, /* 255 */ BadChar
};

static int copyPathRemovingDots(char* dst, const char* src, int srcStart, int srcEnd);
static void encodeRelativeString(const String& rel, const TextEncoding&, CharBuffer& ouput);
static String substituteBackslashes(const String&);

static inline bool isSchemeFirstChar(char c) { return characterClassTable[static_cast<unsigned char>(c)] & SchemeFirstChar; }
static inline bool isSchemeFirstChar(UChar c) { return c <= 0xff && (characterClassTable[c] & SchemeFirstChar); }
static inline bool isSchemeChar(char c) { return characterClassTable[static_cast<unsigned char>(c)] & SchemeChar; }
static inline bool isSchemeChar(UChar c) { return c <= 0xff && (characterClassTable[c] & SchemeChar); }
static inline bool isUserInfoChar(unsigned char c) { return characterClassTable[c] & UserInfoChar; }
static inline bool isHostnameChar(unsigned char c) { return characterClassTable[c] & HostnameChar; }
static inline bool isIPv6Char(unsigned char c) { return characterClassTable[c] & IPv6Char; }
static inline bool isPathSegmentEndChar(char c) { return characterClassTable[static_cast<unsigned char>(c)] & PathSegmentEndChar; }
static inline bool isPathSegmentEndChar(UChar c) { return c <= 0xff && (characterClassTable[c] & PathSegmentEndChar); }
static inline bool isBadChar(unsigned char c) { return characterClassTable[c] & BadChar; }
    
static inline bool isSchemeCharacterMatchIgnoringCase(char character, char schemeCharacter)
{
    ASSERT(isSchemeChar(character));
    ASSERT(schemeCharacter & 0x20);
    ASSERT(isASCIILower(schemeCharacter) || (!isASCIIUpper(schemeCharacter) && isSchemeChar(schemeCharacter)));
    return (character | 0x20) == schemeCharacter;
}

// Copies the source to the destination, assuming all the source characters are
// ASCII. The destination buffer must be large enough. Null characters are allowed
// in the source string, and no attempt is made to null-terminate the result.
static void copyASCII(const String& string, char* dest)
{
    if (string.isEmpty())
        return;

    if (string.is8Bit())
        memcpy(dest, string.characters8(), string.length());
    else {
        const UChar* src = string.characters16();
        size_t length = string.length();
        for (size_t i = 0; i < length; i++)
            dest[i] = static_cast<char>(src[i]);
    }
}

static void appendASCII(const String& base, const char* rel, size_t len, CharBuffer& buffer)
{
    buffer.resize(base.length() + len + 1);
    copyASCII(base, buffer.data());
    memcpy(buffer.data() + base.length(), rel, len);
    buffer[buffer.size() - 1] = '\0';
}

// FIXME: Move to WTFString.h eventually.
// Returns the index of the first index in string |s| of any of the characters
// in |toFind|. |toFind| should be a null-terminated string, all characters up
// to the null will be searched. Returns int if not found.
static int findFirstOf(const UChar* s, int sLen, int startPos, const char* toFind)
{
    for (int i = startPos; i < sLen; i++) {
        const char* cur = toFind;
        while (*cur) {
            if (s[i] == *(cur++))
                return i;
        }
    }
    return -1;
}

static inline void checkEncodedString(const String& url)
{
    ASSERT_UNUSED(url, url.containsOnlyASCII());
    ASSERT_UNUSED(url, url.isEmpty() || isSchemeFirstChar(url[0]));
}

inline bool KURL::protocolIs(const String& string, const char* protocol)
{
    return WebCore::protocolIs(string, protocol);
}

void KURL::invalidate()
{
    m_isValid = false;
    m_protocolIsInHTTPFamily = false;
    m_schemeEnd = 0;
    m_userStart = 0;
    m_userEnd = 0;
    m_passwordEnd = 0;
    m_hostEnd = 0;
    m_portEnd = 0;
    m_pathEnd = 0;
    m_pathAfterLastSlash = 0;
    m_queryEnd = 0;
    m_fragmentEnd = 0;
}

KURL::KURL(ParsedURLStringTag, const String& url)
{
    parse(url);
    ASSERT(url == m_string);
}

KURL::KURL(const KURL& base, const String& relative)
{
    init(base, relative, UTF8Encoding());
}

KURL::KURL(const KURL& base, const String& relative, const TextEncoding& encoding)
{
    // For UTF-{7,16,32}, we want to use UTF-8 for the query part as 
    // we do when submitting a form. A form with GET method
    // has its contents added to a URL as query params and it makes sense
    // to be consistent.
    init(base, relative, encoding.encodingForFormSubmission());
}

static bool shouldTrimFromURL(unsigned char c)
{
    // Browsers ignore leading/trailing whitespace and control
    // characters from URLs.  Note that c is an *unsigned* char here
    // so this comparison should only catch control characters.
    return c <= ' ';
}

void KURL::init(const KURL& base, const String& relative, const TextEncoding& encoding)
{
    // Allow resolutions with a null or empty base URL, but not with any other invalid one.
    // FIXME: Is this a good rule?
    if (!base.m_isValid && !base.isEmpty()) {
        m_string = relative;
        invalidate();
        return;
    }

    // For compatibility with Win IE, treat backslashes as if they were slashes,
    // as long as we're not dealing with javascript: or data: URLs.
    String rel = relative;
    if (rel.contains('\\') && !(protocolIsJavaScript(rel) || protocolIs(rel, "data")))
        rel = substituteBackslashes(rel);

    bool allASCII = rel.containsOnlyASCII();
    CharBuffer strBuffer;
    char* str;
    size_t len;
    if (allASCII) {
        len = rel.length();
        strBuffer.resize(len + 1);
        copyASCII(rel, strBuffer.data());
        strBuffer[len] = 0;
        str = strBuffer.data();
    } else {
        encodeRelativeString(rel, encoding, strBuffer);
        str = strBuffer.data();
        len = strlen(str);
    }

    // Get rid of leading whitespace and control characters.
    while (len && shouldTrimFromURL(*str)) {
        str++;
        --len;
    }

    // Get rid of trailing whitespace and control characters.
    while (len && shouldTrimFromURL(str[len - 1]))
        str[--len] = '\0';

    // According to the RFC, the reference should be interpreted as an
    // absolute URI if possible, using the "leftmost, longest"
    // algorithm. If the URI reference is absolute it will have a
    // scheme, meaning that it will have a colon before the first
    // non-scheme element.
    bool absolute = false;
    char* p = str;
    if (isSchemeFirstChar(*p)) {
        ++p;
        while (isSchemeChar(*p)) {
            ++p;
        }
        if (*p == ':') {
            if (p[1] != '/' && equalIgnoringCase(base.protocol(), String(str, p - str)) && base.isHierarchical())
                str = p + 1;
            else
                absolute = true;
        }
    }

    CharBuffer parseBuffer;

    if (absolute) {
        parse(str, &relative);
    } else {
        // If the base is empty or opaque (e.g. data: or javascript:), then the URL is invalid
        // unless the relative URL is a single fragment.
        if (!base.isHierarchical()) {
            if (str[0] == '#') {
                appendASCII(base.m_string.left(base.m_queryEnd), str, len, parseBuffer);
                parse(parseBuffer.data(), &relative);
            } else {
                m_string = relative;
                invalidate();
            }
            return;
        }

        switch (str[0]) {
        case '\0':
            // The reference is empty, so this is a reference to the same document with any fragment identifier removed.
            *this = base;
            removeFragmentIdentifier();
            break;
        case '#': {
            // must be fragment-only reference
            appendASCII(base.m_string.left(base.m_queryEnd), str, len, parseBuffer);
            parse(parseBuffer.data(), &relative);
            break;
        }
        case '?': {
            // query-only reference, special case needed for non-URL results
            appendASCII(base.m_string.left(base.m_pathEnd), str, len, parseBuffer);
            parse(parseBuffer.data(), &relative);
            break;
        }
        case '/':
            // must be net-path or absolute-path reference
            if (str[1] == '/') {
                // net-path
                appendASCII(base.m_string.left(base.m_schemeEnd + 1), str, len, parseBuffer);
                parse(parseBuffer.data(), &relative);
            } else {
                // abs-path
                appendASCII(base.m_string.left(base.m_portEnd), str, len, parseBuffer);
                parse(parseBuffer.data(), &relative);
            }
            break;
        default:
            {
                // must be relative-path reference

                // Base part plus relative part plus one possible slash added in between plus terminating \0 byte.
                const size_t bufferSize = base.m_pathEnd + 1 + len + 1;
                parseBuffer.resize(bufferSize);

                char* bufferPos = parseBuffer.data();
                char* bufferStart = bufferPos;

                // first copy everything before the path from the base
                CharBuffer baseStringBuffer(base.m_string.length());
                copyASCII(base.m_string, baseStringBuffer.data());
                const char* baseString = baseStringBuffer.data();
                const char* baseStringStart = baseString;
                const char* pathStart = baseStringStart + base.m_portEnd;
                while (baseStringStart < pathStart)
                    *bufferPos++ = *baseStringStart++;
                char* bufferPathStart = bufferPos;

                // now copy the base path
                const char* baseStringEnd = baseString + base.m_pathEnd;

                // go back to the last slash
                while (baseStringEnd > baseStringStart && baseStringEnd[-1] != '/')
                    baseStringEnd--;

                if (baseStringEnd == baseStringStart) {
                    // no path in base, add a path separator if necessary
                    if (base.m_schemeEnd + 1 != base.m_pathEnd && *str && *str != '?' && *str != '#')
                        *bufferPos++ = '/';
                } else {
                    bufferPos += copyPathRemovingDots(bufferPos, baseStringStart, 0, baseStringEnd - baseStringStart);
                }

                const char* relStringStart = str;
                const char* relStringPos = relStringStart;

                while (*relStringPos && *relStringPos != '?' && *relStringPos != '#') {
                    if (relStringPos[0] == '.' && bufferPos[-1] == '/') {
                        if (isPathSegmentEndChar(relStringPos[1])) {
                            // skip over "." segment
                            relStringPos += 1;
                            if (relStringPos[0] == '/')
                                relStringPos++;
                            continue;
                        } else if (relStringPos[1] == '.' && isPathSegmentEndChar(relStringPos[2])) {
                            // skip over ".." segment and rewind the last segment
                            // the RFC leaves it up to the app to decide what to do with excess
                            // ".." segments - we choose to drop them since some web content
                            // relies on this.
                            relStringPos += 2;
                            if (relStringPos[0] == '/')
                                relStringPos++;
                            if (bufferPos > bufferPathStart + 1)
                                bufferPos--;
                            while (bufferPos > bufferPathStart + 1  && bufferPos[-1] != '/')
                                bufferPos--;
                            continue;
                        }
                    }

                    *bufferPos = *relStringPos;
                    relStringPos++;
                    bufferPos++;
                }

                // all done with the path work, now copy any remainder
                // of the relative reference; this will also add a null terminator
                strncpy(bufferPos, relStringPos, bufferSize - (bufferPos - bufferStart));

                parse(parseBuffer.data(), &relative);

                ASSERT(strlen(parseBuffer.data()) + 1 <= parseBuffer.size());
                break;
            }
        }
    }
}

KURL KURL::copy() const
{
    KURL result = *this;
    result.m_string = result.m_string.isolatedCopy();
    return result;
}

String KURL::lastPathComponent() const
{
    if (!hasPath())
        return String();

    unsigned end = m_pathEnd - 1;
    if (m_string[end] == '/')
        --end;

    size_t start = m_string.reverseFind('/', end);
    if (start < static_cast<unsigned>(m_portEnd))
        return String();
    ++start;

    return m_string.substring(start, end - start + 1);
}

String KURL::protocol() const
{
    return m_string.left(m_schemeEnd);
}

String KURL::host() const
{
    int start = hostStart();
    return m_string.substring(start, m_hostEnd - start);
}

unsigned short KURL::port() const
{
    // We return a port of 0 if there is no port specified. This can happen in two situations:
    // 1) The URL contains no colon after the host name and before the path component of the URL.
    // 2) The URL contains a colon but there's no port number before the path component of the URL begins.
    if (m_hostEnd == m_portEnd || m_hostEnd == m_portEnd - 1)
        return 0;

    const UChar* stringData = m_string.characters();
    bool ok = false;
    unsigned number = charactersToUIntStrict(stringData + m_hostEnd + 1, m_portEnd - m_hostEnd - 1, &ok);
    if (!ok || number > maximumValidPortNumber)
        return invalidPortNumber;
    return number;
}

String KURL::pass() const
{
    if (m_passwordEnd == m_userEnd)
        return String();

    return decodeURLEscapeSequences(m_string.substring(m_userEnd + 1, m_passwordEnd - m_userEnd - 1)); 
}

String KURL::user() const
{
    return decodeURLEscapeSequences(m_string.substring(m_userStart, m_userEnd - m_userStart));
}

String KURL::fragmentIdentifier() const
{
    if (m_fragmentEnd == m_queryEnd)
        return String();

    return m_string.substring(m_queryEnd + 1, m_fragmentEnd - (m_queryEnd + 1));
}

bool KURL::hasFragmentIdentifier() const
{
    return m_fragmentEnd != m_queryEnd;
}

String KURL::baseAsString() const
{
    return m_string.left(m_pathAfterLastSlash);
}

#if !PLATFORM(QT) && !USE(CF)
String KURL::fileSystemPath() const
{
    if (!isValid() || !isLocalFile())
        return String();

    return decodeURLEscapeSequences(path());
}
#endif

#ifdef NDEBUG

static inline void assertProtocolIsGood(const char*)
{
}

#else

static void assertProtocolIsGood(const char* protocol)
{
    const char* p = protocol;
    while (*p) {
        ASSERT(*p > ' ' && *p < 0x7F && !(*p >= 'A' && *p <= 'Z'));
        ++p;
    }
}

#endif

bool KURL::protocolIs(const char* protocol) const
{
    assertProtocolIsGood(protocol);

    // JavaScript URLs are "valid" and should be executed even if KURL decides they are invalid.
    // The free function protocolIsJavaScript() should be used instead. 
    ASSERT(!equalIgnoringCase(protocol, String("javascript")));

    if (!m_isValid)
        return false;

    // Do the comparison without making a new string object.
    for (int i = 0; i < m_schemeEnd; ++i) {
        if (!protocol[i] || !isSchemeCharacterMatchIgnoringCase(m_string[i], protocol[i]))
            return false;
    }
    return !protocol[m_schemeEnd]; // We should have consumed all characters in the argument.
}

String KURL::query() const
{
    if (m_queryEnd == m_pathEnd)
        return String();

    return m_string.substring(m_pathEnd + 1, m_queryEnd - (m_pathEnd + 1)); 
}

String KURL::path() const
{
    return m_string.substring(m_portEnd, m_pathEnd - m_portEnd);
}

bool KURL::setProtocol(const String& s)
{
    // Firefox and IE remove everything after the first ':'.
    size_t separatorPosition = s.find(':');
    String newProtocol = s.substring(0, separatorPosition);

    if (!isValidProtocol(newProtocol))
        return false;

    if (!m_isValid) {
        parse(newProtocol + ':' + m_string);
        return true;
    }

    parse(newProtocol + m_string.substring(m_schemeEnd));
    return true;
}

void KURL::setHost(const String& s)
{
    if (!m_isValid)
        return;

    // FIXME: Non-ASCII characters must be encoded and escaped to match parse() expectations,
    // and to avoid changing more than just the host.

    bool slashSlashNeeded = m_userStart == m_schemeEnd + 1;

    parse(m_string.left(hostStart()) + (slashSlashNeeded ? "//" : "") + s + m_string.substring(m_hostEnd));
}

void KURL::removePort()
{
    if (m_hostEnd == m_portEnd)
        return;
    parse(m_string.left(m_hostEnd) + m_string.substring(m_portEnd));
}

void KURL::setPort(unsigned short i)
{
    if (!m_isValid)
        return;

    bool colonNeeded = m_portEnd == m_hostEnd;
    int portStart = (colonNeeded ? m_hostEnd : m_hostEnd + 1);

    parse(m_string.left(portStart) + (colonNeeded ? ":" : "") + String::number(i) + m_string.substring(m_portEnd));
}

void KURL::setHostAndPort(const String& hostAndPort)
{
    if (!m_isValid)
        return;

    // FIXME: Non-ASCII characters must be encoded and escaped to match parse() expectations,
    // and to avoid changing more than just host and port.

    bool slashSlashNeeded = m_userStart == m_schemeEnd + 1;

    parse(m_string.left(hostStart()) + (slashSlashNeeded ? "//" : "") + hostAndPort + m_string.substring(m_portEnd));
}

void KURL::setUser(const String& user)
{
    if (!m_isValid)
        return;

    // FIXME: Non-ASCII characters must be encoded and escaped to match parse() expectations,
    // and to avoid changing more than just the user login.

    int end = m_userEnd;
    if (!user.isEmpty()) {
        String u = user;
        if (m_userStart == m_schemeEnd + 1)
            u = "//" + u;
        // Add '@' if we didn't have one before.
        if (end == m_hostEnd || (end == m_passwordEnd && m_string[end] != '@'))
            u.append('@');
        parse(m_string.left(m_userStart) + u + m_string.substring(end));
    } else {
        // Remove '@' if we now have neither user nor password.
        if (m_userEnd == m_passwordEnd && end != m_hostEnd && m_string[end] == '@')
            end += 1;
        // We don't want to parse in the extremely common case where we are not going to make a change.
        if (m_userStart != end)
            parse(m_string.left(m_userStart) + m_string.substring(end));
    }
}

void KURL::setPass(const String& password)
{
    if (!m_isValid)
        return;

    // FIXME: Non-ASCII characters must be encoded and escaped to match parse() expectations,
    // and to avoid changing more than just the user password.

    int end = m_passwordEnd;
    if (!password.isEmpty()) {
        String p = ":" + password + "@";
        if (m_userEnd == m_schemeEnd + 1)
            p = "//" + p;
        // Eat the existing '@' since we are going to add our own.
        if (end != m_hostEnd && m_string[end] == '@')
            end += 1;
        parse(m_string.left(m_userEnd) + p + m_string.substring(end));
    } else {
        // Remove '@' if we now have neither user nor password.
        if (m_userStart == m_userEnd && end != m_hostEnd && m_string[end] == '@')
            end += 1;
        // We don't want to parse in the extremely common case where we are not going to make a change.
        if (m_userEnd != end)
            parse(m_string.left(m_userEnd) + m_string.substring(end));
    }
}

void KURL::setFragmentIdentifier(const String& s)
{
    if (!m_isValid)
        return;

    // FIXME: Non-ASCII characters must be encoded and escaped to match parse() expectations.
    parse(m_string.left(m_queryEnd) + "#" + s);
}

void KURL::removeFragmentIdentifier()
{
    if (!m_isValid)
        return;
    parse(m_string.left(m_queryEnd));
}
    
void KURL::setQuery(const String& query)
{
    if (!m_isValid)
        return;

    // FIXME: '#' and non-ASCII characters must be encoded and escaped.
    // Usually, the query is encoded using document encoding, not UTF-8, but we don't have
    // access to the document in this function.
    if ((query.isEmpty() || query[0] != '?') && !query.isNull())
        parse(m_string.left(m_pathEnd) + "?" + query + m_string.substring(m_queryEnd));
    else
        parse(m_string.left(m_pathEnd) + query + m_string.substring(m_queryEnd));

}

void KURL::setPath(const String& s)
{
    if (!m_isValid)
        return;

    // FIXME: encodeWithURLEscapeSequences does not correctly escape '#' and '?', so fragment and query parts
    // may be inadvertently affected.
    String path = s;
    if (path.isEmpty() || path[0] != '/')
        path = "/" + path;

    parse(m_string.left(m_portEnd) + encodeWithURLEscapeSequences(path) + m_string.substring(m_pathEnd));
}

String decodeURLEscapeSequences(const String& string)
{
    return decodeEscapeSequences<URLEscapeSequence>(string, UTF8Encoding());
}

String decodeURLEscapeSequences(const String& string, const TextEncoding& encoding)
{
    return decodeEscapeSequences<URLEscapeSequence>(string, encoding);
}

// Caution: This function does not bounds check.
static void appendEscapedChar(char*& buffer, unsigned char c)
{
    *buffer++ = '%';
    placeByteAsHex(c, buffer);
}

static void appendEscapingBadChars(char*& buffer, const char* strStart, size_t length)
{
    char* p = buffer;

    const char* str = strStart;
    const char* strEnd = strStart + length;
    while (str < strEnd) {
        unsigned char c = *str++;
        if (isBadChar(c)) {
            if (c == '%' || c == '?')
                *p++ = c;
            else if (c != 0x09 && c != 0x0a && c != 0x0d)
                appendEscapedChar(p, c);
        } else
            *p++ = c;
    }

    buffer = p;
}

static void escapeAndAppendNonHierarchicalPart(char*& buffer, const char* strStart, size_t length)
{
    char* p = buffer;

    const char* str = strStart;
    const char* strEnd = strStart + length;
    while (str < strEnd) {
        unsigned char c = *str++;
        // Strip CR, LF and Tab from fragments, per:
        // https://bugs.webkit.org/show_bug.cgi?id=8770
        if (c == 0x09 || c == 0x0a || c == 0x0d)
            continue;

        // Chrome and IE allow non-ascii characters in fragments, however doing
        // so would hit an ASSERT in checkEncodedString, so for now we don't.
        if (c < 0x20 || c >= 127) {
            appendEscapedChar(p, c);
            continue;
        }
        *p++ = c;
    }

    buffer = p;
}

// copy a path, accounting for "." and ".." segments
static int copyPathRemovingDots(char* dst, const char* src, int srcStart, int srcEnd)
{
    char* bufferPathStart = dst;

    // empty path is a special case, and need not have a leading slash
    if (srcStart != srcEnd) {
        const char* baseStringStart = src + srcStart;
        const char* baseStringEnd = src + srcEnd;
        const char* baseStringPos = baseStringStart;

        // this code is unprepared for paths that do not begin with a
        // slash and we should always have one in the source string
        ASSERT(baseStringPos[0] == '/');

        // copy the leading slash into the destination
        *dst = *baseStringPos;
        baseStringPos++;
        dst++;

        while (baseStringPos < baseStringEnd) {
            if (baseStringPos[0] == '.' && dst[-1] == '/') {
                if (baseStringPos[1] == '/' || baseStringPos + 1 == baseStringEnd) {
                    // skip over "." segment
                    baseStringPos += 2;
                    continue;
                } else if (baseStringPos[1] == '.' && (baseStringPos[2] == '/' ||
                                       baseStringPos + 2 == baseStringEnd)) {
                    // skip over ".." segment and rewind the last segment
                    // the RFC leaves it up to the app to decide what to do with excess
                    // ".." segments - we choose to drop them since some web content
                    // relies on this.
                    baseStringPos += 3;
                    if (dst > bufferPathStart + 1)
                        dst--;
                    while (dst > bufferPathStart && dst[-1] != '/')
                        dst--;
                    continue;
                }
            }

            *dst = *baseStringPos;
            baseStringPos++;
            dst++;
        }
    }
    *dst = '\0';
    return dst - bufferPathStart;
}

static inline bool hasSlashDotOrDotDot(const char* str)
{
    const unsigned char* p = reinterpret_cast<const unsigned char*>(str);
    if (!*p)
        return false;
    unsigned char pc = *p;
    while (unsigned char c = *++p) {
        if (c == '.' && (pc == '/' || pc == '.'))
            return true;
        pc = c;
    }
    return false;
}

void KURL::parse(const String& string)
{
    checkEncodedString(string);

    CharBuffer buffer(string.length() + 1);
    copyASCII(string, buffer.data());
    buffer[string.length()] = '\0';
    parse(buffer.data(), &string);
}

template<size_t length>
static inline bool equal(const char* a, const char (&b)[length])
{
    for (size_t i = 0; i < length; ++i) {
        if (a[i] != b[i])
            return false;
    }
    return true;
}

template<size_t lengthB>
static inline bool equal(const char* stringA, size_t lengthA, const char (&stringB)[lengthB])
{
    return lengthA == lengthB && equal(stringA, stringB);
}

// List of default schemes is taken from google-url:
// http://code.google.com/p/google-url/source/browse/trunk/src/url_canon_stdurl.cc#120
static inline bool isDefaultPortForScheme(const char* port, size_t portLength, const char* scheme, size_t schemeLength)
{
    // This switch is theoretically a performance optimization.  It came over when
    // the code was moved from google-url, but may be removed later.
    switch (schemeLength) {
    case 2:
        return equal(scheme, wsScheme) && equal(port, portLength, httpPort);
    case 3:
        if (equal(scheme, ftpScheme))
            return equal(port, portLength, ftpPort);
        if (equal(scheme, wssScheme))
            return equal(port, portLength, httpsPort);
        break;
    case 4:
        return equal(scheme, httpScheme) && equal(port, portLength, httpPort);
    case 5:
        return equal(scheme, httpsScheme) && equal(port, portLength, httpsPort);
    case 6:
        return equal(scheme, gopherScheme) && equal(port, portLength, gopherPort);
    }
    return false;
}

static inline bool hostPortIsEmptyButCredentialsArePresent(int hostStart, int portEnd, char userinfoEndChar)
{
    return userinfoEndChar == '@' && hostStart == portEnd;
}

static bool isNonFileHierarchicalScheme(const char* scheme, size_t schemeLength)
{
    switch (schemeLength) {
    case 2:
        return equal(scheme, wsScheme);
    case 3:
        return equal(scheme, ftpScheme) || equal(scheme, wssScheme);
    case 4:
        return equal(scheme, httpScheme);
    case 5:
        return equal(scheme, httpsScheme);
    case 6:
        return equal(scheme, gopherScheme);
    }
    return false;
}

static bool isCanonicalHostnameLowercaseForScheme(const char* scheme, size_t schemeLength)
{
    switch (schemeLength) {
    case 2:
        return equal(scheme, wsScheme);
    case 3:
        return equal(scheme, ftpScheme) || equal(scheme, wssScheme);
    case 4:
        return equal(scheme, httpScheme) || equal(scheme, fileScheme);
    case 5:
        return equal(scheme, httpsScheme);
    case 6:
        return equal(scheme, gopherScheme);
    }
    return false;
}

void KURL::parse(const char* url, const String* originalString)
{
    if (!url || url[0] == '\0') {
        // valid URL must be non-empty
        m_string = originalString ? *originalString : url;
        invalidate();
        return;
    }

    if (!isSchemeFirstChar(url[0])) {
        // scheme must start with an alphabetic character
        m_string = originalString ? *originalString : url;
        invalidate();
        return;
    }

    int schemeEnd = 0;
    while (isSchemeChar(url[schemeEnd]))
        schemeEnd++;

    if (url[schemeEnd] != ':') {
        m_string = originalString ? *originalString : url;
        invalidate();
        return;
    }

    int userStart = schemeEnd + 1;
    int userEnd;
    int passwordStart;
    int passwordEnd;
    int hostStart;
    int hostEnd;
    int portStart;
    int portEnd;

    bool hierarchical = url[schemeEnd + 1] == '/';
    bool hasSecondSlash = hierarchical && url[schemeEnd + 2] == '/';

    bool isFile = schemeEnd == 4
        && isLetterMatchIgnoringCase(url[0], 'f')
        && isLetterMatchIgnoringCase(url[1], 'i')
        && isLetterMatchIgnoringCase(url[2], 'l')
        && isLetterMatchIgnoringCase(url[3], 'e');

#if PLATFORM(BLACKBERRY)
    // Parse local: urls the same as file: urls.
    if (!isFile)
        isFile = schemeEnd == 5
            && isLetterMatchIgnoringCase(url[0], 'l')
            && isLetterMatchIgnoringCase(url[1], 'o')
            && isLetterMatchIgnoringCase(url[2], 'c')
            && isLetterMatchIgnoringCase(url[3], 'a')
            && isLetterMatchIgnoringCase(url[4], 'l');
#endif

    m_protocolIsInHTTPFamily = isLetterMatchIgnoringCase(url[0], 'h')
        && isLetterMatchIgnoringCase(url[1], 't')
        && isLetterMatchIgnoringCase(url[2], 't')
        && isLetterMatchIgnoringCase(url[3], 'p')
        && (url[4] == ':' || (isLetterMatchIgnoringCase(url[4], 's') && url[5] == ':'));

    if ((hierarchical && hasSecondSlash) || isNonFileHierarchicalScheme(url, schemeEnd)) {
        // The part after the scheme is either a net_path or an abs_path whose first path segment is empty.
        // Attempt to find an authority.
        // FIXME: Authority characters may be scanned twice, and it would be nice to be faster.

        if (hierarchical)
            userStart++;
        if (hasSecondSlash)
            userStart++;
        userEnd = userStart;

        int colonPos = 0;
        while (isUserInfoChar(url[userEnd])) {
            if (url[userEnd] == ':' && colonPos == 0)
                colonPos = userEnd;
            userEnd++;
        }

        if (url[userEnd] == '@') {
            // actual end of the userinfo, start on the host
            if (colonPos != 0) {
                passwordEnd = userEnd;
                userEnd = colonPos;
                passwordStart = colonPos + 1;
            } else
                passwordStart = passwordEnd = userEnd;

            hostStart = passwordEnd + 1;
        } else if (url[userEnd] == '[' || isPathSegmentEndChar(url[userEnd])) {
            // hit the end of the authority, must have been no user
            // or looks like an IPv6 hostname
            // either way, try to parse it as a hostname
            userEnd = userStart;
            passwordStart = passwordEnd = userEnd;
            hostStart = userStart;
        } else {
            // invalid character
            m_string = originalString ? *originalString : url;
            invalidate();
            return;
        }

        hostEnd = hostStart;

        // IPV6 IP address
        if (url[hostEnd] == '[') {
            hostEnd++;
            while (isIPv6Char(url[hostEnd]))
                hostEnd++;
            if (url[hostEnd] == ']')
                hostEnd++;
            else {
                // invalid character
                m_string = originalString ? *originalString : url;
                invalidate();
                return;
            }
        } else {
            while (isHostnameChar(url[hostEnd]))
                hostEnd++;
        }
        
        if (url[hostEnd] == ':') {
            portStart = portEnd = hostEnd + 1;
 
            // possible start of port
            portEnd = portStart;
            while (isASCIIDigit(url[portEnd]))
                portEnd++;
        } else
            portStart = portEnd = hostEnd;

        if (!isPathSegmentEndChar(url[portEnd])) {
            // invalid character
            m_string = originalString ? *originalString : url;
            invalidate();
            return;
        }

        if (hostPortIsEmptyButCredentialsArePresent(hostStart, portEnd, url[passwordEnd])) {
            m_string = originalString ? *originalString : url;
            invalidate();
            return;
        }

        if (userStart == portEnd && !m_protocolIsInHTTPFamily && !isFile) {
            // No authority found, which means that this is not a net_path, but rather an abs_path whose first two
            // path segments are empty. For file, http and https only, an empty authority is allowed.
            userStart -= 2;
            userEnd = userStart;
            passwordStart = userEnd;
            passwordEnd = passwordStart;
            hostStart = passwordEnd;
            hostEnd = hostStart;
            portStart = hostEnd;
            portEnd = hostEnd;
        }
    } else {
        // the part after the scheme must be an opaque_part or an abs_path
        userEnd = userStart;
        passwordStart = passwordEnd = userEnd;
        hostStart = hostEnd = passwordEnd;
        portStart = portEnd = hostEnd;
    }

    int pathStart = portEnd;
    int pathEnd = pathStart;
    while (url[pathEnd] && url[pathEnd] != '?' && url[pathEnd] != '#')
        pathEnd++;

    int queryStart = pathEnd;
    int queryEnd = queryStart;
    if (url[queryStart] == '?') {
        while (url[queryEnd] && url[queryEnd] != '#')
            queryEnd++;
    }

    int fragmentStart = queryEnd;
    int fragmentEnd = fragmentStart;
    if (url[fragmentStart] == '#') {
        fragmentStart++;
        fragmentEnd = fragmentStart;
        while (url[fragmentEnd])
            fragmentEnd++;
    }

    // assemble it all, remembering the real ranges

    Vector<char, 4096> buffer(fragmentEnd * 3 + 1);

    char *p = buffer.data();
    const char *strPtr = url;

    // copy in the scheme
    const char *schemeEndPtr = url + schemeEnd;
    while (strPtr < schemeEndPtr)
        *p++ = toASCIILower(*strPtr++);
    m_schemeEnd = p - buffer.data();

    bool hostIsLocalHost = portEnd - userStart == 9
        && isLetterMatchIgnoringCase(url[userStart], 'l')
        && isLetterMatchIgnoringCase(url[userStart+1], 'o')
        && isLetterMatchIgnoringCase(url[userStart+2], 'c')
        && isLetterMatchIgnoringCase(url[userStart+3], 'a')
        && isLetterMatchIgnoringCase(url[userStart+4], 'l')
        && isLetterMatchIgnoringCase(url[userStart+5], 'h')
        && isLetterMatchIgnoringCase(url[userStart+6], 'o')
        && isLetterMatchIgnoringCase(url[userStart+7], 's')
        && isLetterMatchIgnoringCase(url[userStart+8], 't');

    // File URLs need a host part unless it is just file:// or file://localhost
    bool degenerateFilePath = pathStart == pathEnd && (hostStart == hostEnd || hostIsLocalHost);

    // We drop empty credentials, but keep a colon in an empty host/port pair.
    // Removing hostname completely would change the structure of the URL on re-parsing.
    bool haveNonHostAuthorityPart = userStart != userEnd || passwordStart != passwordEnd || hostEnd != portEnd;

    // add ":" after scheme
    *p++ = ':';

    // if we have at least one authority part or a file URL - add "//" and authority
    if (isFile ? !degenerateFilePath : (haveNonHostAuthorityPart || hostStart != hostEnd)) {
        *p++ = '/';
        *p++ = '/';

        m_userStart = p - buffer.data();

        // copy in the user
        strPtr = url + userStart;
        const char* userEndPtr = url + userEnd;
        while (strPtr < userEndPtr) {
            char c = *strPtr++;
            ASSERT(isUserInfoChar(c));
            *p++ = c;
        }
        m_userEnd = p - buffer.data();

        // copy in the password
        if (passwordEnd != passwordStart) {
            *p++ = ':';
            strPtr = url + passwordStart;
            const char* passwordEndPtr = url + passwordEnd;
            while (strPtr < passwordEndPtr) {
                char c = *strPtr++;
                ASSERT(isUserInfoChar(c));
                *p++ = c;
            }
        }
        m_passwordEnd = p - buffer.data();

        // If we had any user info, add "@"
        if (p - buffer.data() != m_userStart)
            *p++ = '@';

        // copy in the host, except in the case of a file URL with authority="localhost"
        if (!(isFile && hostIsLocalHost && !haveNonHostAuthorityPart)) {
            strPtr = url + hostStart;
            const char* hostEndPtr = url + hostEnd;
            if (isCanonicalHostnameLowercaseForScheme(buffer.data(), m_schemeEnd)) {
                while (strPtr < hostEndPtr) {
                    char c = toASCIILower(*strPtr++);
                    ASSERT(isHostnameChar(c) || c == '[' || c == ']' || c == ':');
                    *p++ = c;
                }
            } else {
                while (strPtr < hostEndPtr) {
                    char c = *strPtr++;
                    ASSERT(isHostnameChar(c) || c == '[' || c == ']' || c == ':');
                    *p++ = c;
                }
            }
        }
        m_hostEnd = p - buffer.data();

        // Copy in the port if the URL has one (and it's not default). Also, copy it if there was no hostname, so that there is still something in authority component.
        if (hostEnd != portStart) {
            const char* portStr = url + portStart;
            size_t portLength = portEnd - portStart;
            if ((portLength && !isDefaultPortForScheme(portStr, portLength, buffer.data(), m_schemeEnd))
                || (hostStart == hostEnd && hostEnd != portStart)) {
                *p++ = ':';
                const char* portEndPtr = url + portEnd;
                while (portStr < portEndPtr)
                    *p++ = *portStr++;
            }
        }
        m_portEnd = p - buffer.data();
    } else {
        if (isFile) {
            ASSERT(degenerateFilePath);
            *p++ = '/';
            *p++ = '/';
        }
        m_userStart = m_userEnd = m_passwordEnd = m_hostEnd = m_portEnd = p - buffer.data();
    }

    // For canonicalization, ensure we have a '/' for no path.
    // Do this only for URL with protocol file, http or https.
    if ((m_protocolIsInHTTPFamily || isFile) && pathEnd == pathStart)
        *p++ = '/';

    // add path, escaping bad characters
    if (!hierarchical)
        escapeAndAppendNonHierarchicalPart(p, url + pathStart, pathEnd - pathStart);
    else if (!hasSlashDotOrDotDot(url))
        appendEscapingBadChars(p, url + pathStart, pathEnd - pathStart);
    else {
        CharBuffer pathBuffer(pathEnd - pathStart + 1);
        size_t length = copyPathRemovingDots(pathBuffer.data(), url, pathStart, pathEnd);
        appendEscapingBadChars(p, pathBuffer.data(), length);
    }

    m_pathEnd = p - buffer.data();

    // Find the position after the last slash in the path, or
    // the position before the path if there are no slashes in it.
    int i;
    for (i = m_pathEnd; i > m_portEnd; --i) {
        if (buffer[i - 1] == '/')
            break;
    }
    m_pathAfterLastSlash = i;

    // add query, escaping bad characters
    appendEscapingBadChars(p, url + queryStart, queryEnd - queryStart);
    m_queryEnd = p - buffer.data();

    // add fragment, escaping bad characters
    if (fragmentEnd != queryEnd) {
        *p++ = '#';
        escapeAndAppendNonHierarchicalPart(p, url + fragmentStart, fragmentEnd - fragmentStart);
    }
    m_fragmentEnd = p - buffer.data();

    ASSERT(p - buffer.data() <= static_cast<int>(buffer.size()));
    ASSERT(buffer.size() > 0);

    // If we didn't end up actually changing the original string and
    // it was already in a String, reuse it to avoid extra allocation.
    if (originalString && equal(originalString->impl(), buffer.data(), m_fragmentEnd))
        m_string = *originalString;
    else
        m_string = String(buffer.data(), m_fragmentEnd);

    m_isValid = true;
}

bool equalIgnoringFragmentIdentifier(const KURL& a, const KURL& b)
{
    if (a.m_queryEnd != b.m_queryEnd)
        return false;
    unsigned queryLength = a.m_queryEnd;
    for (unsigned i = 0; i < queryLength; ++i)
        if (a.string()[i] != b.string()[i])
            return false;
    return true;
}

bool protocolHostAndPortAreEqual(const KURL& a, const KURL& b)
{
    if (a.m_schemeEnd != b.m_schemeEnd)
        return false;

    int hostStartA = a.hostStart();
    int hostLengthA = a.hostEnd() - hostStartA;
    int hostStartB = b.hostStart();
    int hostLengthB = b.hostEnd() - b.hostStart();
    if (hostLengthA != hostLengthB)
        return false;

    // Check the scheme
    for (int i = 0; i < a.m_schemeEnd; ++i)
        if (a.string()[i] != b.string()[i])
            return false;

    // And the host
    for (int i = 0; i < hostLengthA; ++i)
        if (a.string()[hostStartA + i] != b.string()[hostStartB + i])
            return false;

    if (a.port() != b.port())
        return false;

    return true;
}

String encodeWithURLEscapeSequences(const String& notEncodedString)
{
    CString asUTF8 = notEncodedString.utf8();

    CharBuffer buffer(asUTF8.length() * 3 + 1);
    char* p = buffer.data();

    const char* str = asUTF8.data();
    const char* strEnd = str + asUTF8.length();
    while (str < strEnd) {
        unsigned char c = *str++;
        if (isBadChar(c))
            appendEscapedChar(p, c);
        else
            *p++ = c;
    }

    ASSERT(p - buffer.data() <= static_cast<int>(buffer.size()));

    return String(buffer.data(), p - buffer.data());
}

// Appends the punycoded hostname identified by the given string and length to
// the output buffer. The result will not be null terminated.
static void appendEncodedHostname(UCharBuffer& buffer, const UChar* str, unsigned strLen)
{
    // Needs to be big enough to hold an IDN-encoded name.
    // For host names bigger than this, we won't do IDN encoding, which is almost certainly OK.
    const unsigned hostnameBufferLength = 2048;

    if (strLen > hostnameBufferLength || charactersAreAllASCII(str, strLen)) {
        buffer.append(str, strLen);
        return;
    }

#if USE(ICU_UNICODE)
    UChar hostnameBuffer[hostnameBufferLength];
    UErrorCode error = U_ZERO_ERROR;
    int32_t numCharactersConverted = uidna_IDNToASCII(str, strLen, hostnameBuffer,
        hostnameBufferLength, UIDNA_ALLOW_UNASSIGNED, 0, &error);
    if (error == U_ZERO_ERROR)
        buffer.append(hostnameBuffer, numCharactersConverted);
#endif
}

static void findHostnamesInMailToURL(const UChar* str, int strLen, Vector<pair<int, int> >& nameRanges)
{
    // In a mailto: URL, host names come after a '@' character and end with a '>' or ',' or '?' or end of string character.
    // Skip quoted strings so that characters in them don't confuse us.
    // When we find a '?' character, we are past the part of the URL that contains host names.

    nameRanges.clear();

    int p = 0;
    while (1) {
        // Find start of host name or of quoted string.
        int hostnameOrStringStart = findFirstOf(str, strLen, p, "\"@?");
        if (hostnameOrStringStart == -1)
            return;
        UChar c = str[hostnameOrStringStart];
        p = hostnameOrStringStart + 1;

        if (c == '?')
            return;

        if (c == '@') {
            // Find end of host name.
            int hostnameStart = p;
            int hostnameEnd = findFirstOf(str, strLen, p, ">,?");
            bool done;
            if (hostnameEnd == -1) {
                hostnameEnd = strLen;
                done = true;
            } else {
                p = hostnameEnd;
                done = false;
            }

            nameRanges.append(make_pair(hostnameStart, hostnameEnd));

            if (done)
                return;
        } else {
            // Skip quoted string.
            ASSERT(c == '"');
            while (1) {
                int escapedCharacterOrStringEnd = findFirstOf(str, strLen, p, "\"\\");
                if (escapedCharacterOrStringEnd == -1)
                    return;

                c = str[escapedCharacterOrStringEnd];
                p = escapedCharacterOrStringEnd + 1;

                // If we are the end of the string, then break from the string loop back to the host name loop.
                if (c == '"')
                    break;

                // Skip escaped character.
                ASSERT(c == '\\');
                if (p == strLen)
                    return;

                ++p;
            }
        }
    }
}

static bool findHostnameInHierarchicalURL(const UChar* str, int strLen, int& startOffset, int& endOffset)
{
    // Find the host name in a hierarchical URL.
    // It comes after a "://" sequence, with scheme characters preceding, and
    // this should be the first colon in the string.
    // It ends with the end of the string or a ":" or a path segment ending character.
    // If there is a "@" character, the host part is just the part after the "@".
    int separator = findFirstOf(str, strLen, 0, ":");
    if (separator == -1 || separator + 2 >= strLen ||
        str[separator + 1] != '/' || str[separator + 2] != '/')
        return false;

    // Check that all characters before the :// are valid scheme characters.
    if (!isSchemeFirstChar(str[0]))
        return false;
    for (int i = 1; i < separator; ++i) {
        if (!isSchemeChar(str[i]))
            return false;
    }

    // Start after the separator.
    int authorityStart = separator + 3;

    // Find terminating character.
    int hostnameEnd = strLen;
    for (int i = authorityStart; i < strLen; ++i) {
        UChar c = str[i];
        if (c == ':' || (isPathSegmentEndChar(c) && c != 0)) {
            hostnameEnd = i;
            break;
        }
    }

    // Find "@" for the start of the host name.
    int userInfoTerminator = findFirstOf(str, strLen, authorityStart, "@");
    int hostnameStart;
    if (userInfoTerminator == -1 || userInfoTerminator > hostnameEnd)
        hostnameStart = authorityStart;
    else
        hostnameStart = userInfoTerminator + 1;

    startOffset = hostnameStart;
    endOffset = hostnameEnd;
    return true;
}

// Converts all hostnames found in the given input to punycode, preserving the
// rest of the URL unchanged. The output will NOT be null-terminated.
static void encodeHostnames(const String& str, UCharBuffer& output)
{
    output.clear();

    if (protocolIs(str, "mailto")) {
        Vector<pair<int, int> > hostnameRanges;
        findHostnamesInMailToURL(str.characters(), str.length(), hostnameRanges);
        int n = hostnameRanges.size();
        int p = 0;
        for (int i = 0; i < n; ++i) {
            const pair<int, int>& r = hostnameRanges[i];
            output.append(&str.characters()[p], r.first - p);
            appendEncodedHostname(output, &str.characters()[r.first], r.second - r.first);
            p = r.second;
        }
        // This will copy either everything after the last hostname, or the
        // whole thing if there is no hostname.
        output.append(&str.characters()[p], str.length() - p);
    } else {
        int hostStart, hostEnd;
        if (findHostnameInHierarchicalURL(str.characters(), str.length(), hostStart, hostEnd)) {
            output.append(str.characters(), hostStart); // Before hostname.
            appendEncodedHostname(output, &str.characters()[hostStart], hostEnd - hostStart);
            output.append(&str.characters()[hostEnd], str.length() - hostEnd); // After hostname.
        } else {
            // No hostname to encode, return the input.
            output.append(str.characters(), str.length());
        }
    }
}

static void encodeRelativeString(const String& rel, const TextEncoding& encoding, CharBuffer& output)
{
    UCharBuffer s;
    encodeHostnames(rel, s);

    TextEncoding pathEncoding(UTF8Encoding()); // Path is always encoded as UTF-8; other parts may depend on the scheme.

    int pathEnd = -1;
    if (encoding != pathEncoding && encoding.isValid() && !protocolIs(rel, "mailto") && !protocolIs(rel, "data") && !protocolIsJavaScript(rel)) {
        // Find the first instance of either # or ?, keep pathEnd at -1 otherwise.
        pathEnd = findFirstOf(s.data(), s.size(), 0, "#?");
    }

    if (pathEnd == -1) {
        CString decoded = pathEncoding.encode(s.data(), s.size(), URLEncodedEntitiesForUnencodables);
        output.resize(decoded.length());
        memcpy(output.data(), decoded.data(), decoded.length());
    } else {
        CString pathDecoded = pathEncoding.encode(s.data(), pathEnd, URLEncodedEntitiesForUnencodables);
        // Unencodable characters in URLs are represented by converting
        // them to XML entities and escaping non-alphanumeric characters.
        CString otherDecoded = encoding.encode(s.data() + pathEnd, s.size() - pathEnd, URLEncodedEntitiesForUnencodables);

        output.resize(pathDecoded.length() + otherDecoded.length());
        memcpy(output.data(), pathDecoded.data(), pathDecoded.length());
        memcpy(output.data() + pathDecoded.length(), otherDecoded.data(), otherDecoded.length());
    }
    output.append('\0'); // null-terminate the output.
}

static String substituteBackslashes(const String& string)
{
    size_t questionPos = string.find('?');
    size_t hashPos = string.find('#');
    unsigned pathEnd;

    if (hashPos != notFound && (questionPos == notFound || questionPos > hashPos))
        pathEnd = hashPos;
    else if (questionPos != notFound)
        pathEnd = questionPos;
    else
        pathEnd = string.length();

    return string.left(pathEnd).replace('\\','/') + string.substring(pathEnd);
}

bool KURL::isHierarchical() const
{
    if (!m_isValid)
        return false;
    ASSERT(m_string[m_schemeEnd] == ':');
    return m_string[m_schemeEnd + 1] == '/';
}

void KURL::copyToBuffer(Vector<char, 512>& buffer) const
{
    // FIXME: This throws away the high bytes of all the characters in the string!
    // That's fine for a valid URL, which is all ASCII, but not for invalid URLs.
    buffer.resize(m_string.length());
    copyASCII(m_string, buffer.data());
}

bool protocolIs(const String& url, const char* protocol)
{
    // Do the comparison without making a new string object.
    assertProtocolIsGood(protocol);
    for (int i = 0; ; ++i) {
        if (!protocol[i])
            return url[i] == ':';
        if (!isLetterMatchIgnoringCase(url[i], protocol[i]))
            return false;
    }
}

bool isValidProtocol(const String& protocol)
{
    // RFC3986: ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
    if (protocol.isEmpty())
        return false;
    if (!isSchemeFirstChar(protocol[0]))
        return false;
    unsigned protocolLength = protocol.length();
    for (unsigned i = 1; i < protocolLength; i++) {
        if (!isSchemeChar(protocol[i]))
            return false;
    }
    return true;
}

#ifndef NDEBUG
void KURL::print() const
{
    printf("%s\n", m_string.utf8().data());
}
#endif

String KURL::strippedForUseAsReferrer() const
{
    KURL referrer(*this);
    referrer.setUser(String());
    referrer.setPass(String());
    referrer.removeFragmentIdentifier();
    return referrer.string();
}

bool KURL::isLocalFile() const
{
    // Including feed here might be a bad idea since drag and drop uses this check
    // and including feed would allow feeds to potentially let someone's blog
    // read the contents of the clipboard on a drag, even without a drop.
    // Likewise with using the FrameLoader::shouldTreatURLAsLocal() function.
#if PLATFORM(QT)
    return protocolIs("file") || protocolIs("qrc");
#else
    return protocolIs("file");
#endif
}

bool protocolIsJavaScript(const String& url)
{
    return protocolIs(url, "javascript");
}

const KURL& blankURL()
{
    DEFINE_STATIC_LOCAL(KURL, staticBlankURL, (ParsedURLString, "about:blank"));
    return staticBlankURL;
}

bool KURL::isBlankURL() const
{
    return protocolIs("about");
}

bool isDefaultPortForProtocol(unsigned short port, const String& protocol)
{
    if (protocol.isEmpty())
        return false;

    typedef HashMap<String, unsigned, CaseFoldingHash> DefaultPortsMap;
    DEFINE_STATIC_LOCAL(DefaultPortsMap, defaultPorts, ());
    if (defaultPorts.isEmpty()) {
        defaultPorts.set("http", 80);
        defaultPorts.set("https", 443);
        defaultPorts.set("ftp", 21);
        defaultPorts.set("ftps", 990);
    }
    return defaultPorts.get(protocol) == port;
}

bool portAllowed(const KURL& url)
{
    unsigned short port = url.port();

    // Since most URLs don't have a port, return early for the "no port" case.
    if (!port)
        return true;

    // This blocked port list matches the port blocking that Mozilla implements.
    // See http://www.mozilla.org/projects/netlib/PortBanning.html for more information.
    static const unsigned short blockedPortList[] = {
        1,    // tcpmux
        7,    // echo
        9,    // discard
        11,   // systat
        13,   // daytime
        15,   // netstat
        17,   // qotd
        19,   // chargen
        20,   // FTP-data
        21,   // FTP-control
        22,   // SSH
        23,   // telnet
        25,   // SMTP
        37,   // time
        42,   // name
        43,   // nicname
        53,   // domain
        77,   // priv-rjs
        79,   // finger
        87,   // ttylink
        95,   // supdup
        101,  // hostriame
        102,  // iso-tsap
        103,  // gppitnp
        104,  // acr-nema
        109,  // POP2
        110,  // POP3
        111,  // sunrpc
        113,  // auth
        115,  // SFTP
        117,  // uucp-path
        119,  // nntp
        123,  // NTP
        135,  // loc-srv / epmap
        139,  // netbios
        143,  // IMAP2
        179,  // BGP
        389,  // LDAP
        465,  // SMTP+SSL
        512,  // print / exec
        513,  // login
        514,  // shell
        515,  // printer
        526,  // tempo
        530,  // courier
        531,  // Chat
        532,  // netnews
        540,  // UUCP
        556,  // remotefs
        563,  // NNTP+SSL
        587,  // ESMTP
        601,  // syslog-conn
        636,  // LDAP+SSL
        993,  // IMAP+SSL
        995,  // POP3+SSL
        2049, // NFS
        3659, // apple-sasl / PasswordServer [Apple addition]
        4045, // lockd
        6000, // X11
        6665, // Alternate IRC [Apple addition]
        6666, // Alternate IRC [Apple addition]
        6667, // Standard IRC [Apple addition]
        6668, // Alternate IRC [Apple addition]
        6669, // Alternate IRC [Apple addition]
        invalidPortNumber, // Used to block all invalid port numbers
    };
    const unsigned short* const blockedPortListEnd = blockedPortList + WTF_ARRAY_LENGTH(blockedPortList);

#ifndef NDEBUG
    // The port list must be sorted for binary_search to work.
    static bool checkedPortList = false;
    if (!checkedPortList) {
        for (const unsigned short* p = blockedPortList; p != blockedPortListEnd - 1; ++p)
            ASSERT(*p < *(p + 1));
        checkedPortList = true;
    }
#endif

    // If the port is not in the blocked port list, allow it.
    if (!binary_search(blockedPortList, blockedPortListEnd, port))
        return true;

    // Allow ports 21 and 22 for FTP URLs, as Mozilla does.
    if ((port == 21 || port == 22) && url.protocolIs("ftp"))
        return true;

    // Allow any port number in a file URL, since the port number is ignored.
    if (url.protocolIs("file"))
        return true;

#if PLATFORM(BLACKBERRY)
    if (url.protocolIs("local"))
        return true;
#endif

    return false;
}

String mimeTypeFromDataURL(const String& url)
{
    ASSERT(protocolIs(url, "data"));
    size_t index = url.find(';');
    if (index == notFound)
        index = url.find(',');
    if (index != notFound) {
        if (index > 5)
            return url.substring(5, index - 5).lower();
        return "text/plain"; // Data URLs with no MIME type are considered text/plain.
    }
    return "";
}

String mimeTypeFromURL(const KURL& url)
{
    String decodedPath = decodeURLEscapeSequences(url.path());
    String extension = decodedPath.substring(decodedPath.reverseFind('.') + 1);

    // We don't use MIMETypeRegistry::getMIMETypeForPath() because it returns "application/octet-stream" upon failure
    return MIMETypeRegistry::getMIMETypeForExtension(extension);
}

bool KURL::isSafeToSendToAnotherThread() const
{
    return m_string.isSafeToSendToAnotherThread();
}

String KURL::stringCenterEllipsizedToLength(unsigned length) const
{
    if (string().length() <= length)
        return string();

    return string().left(length / 2 - 1) + "..." + string().right(length / 2 - 2);
}

}
