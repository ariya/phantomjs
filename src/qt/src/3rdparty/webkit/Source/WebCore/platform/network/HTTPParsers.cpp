/*
 * Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
#include "HTTPParsers.h"
#include "ResourceResponseBase.h"

#include "PlatformString.h"
#include <wtf/text/CString.h>
#include <wtf/DateMath.h>

using namespace WTF;

namespace WebCore {

// true if there is more to parse
static inline bool skipWhiteSpace(const String& str, unsigned& pos, bool fromHttpEquivMeta)
{
    unsigned len = str.length();

    if (fromHttpEquivMeta) {
        while (pos != len && str[pos] <= ' ')
            ++pos;
    } else {
        while (pos != len && (str[pos] == '\t' || str[pos] == ' '))
            ++pos;
    }

    return pos != len;
}

// Returns true if the function can match the whole token (case insensitive).
// Note: Might return pos == str.length()
static inline bool skipToken(const String& str, unsigned& pos, const char* token)
{
    unsigned len = str.length();

    while (pos != len && *token) {
        if (toASCIILower(str[pos]) != *token++)
            return false;
        ++pos;
    }

    return true;
}

ContentDispositionType contentDispositionType(const String& contentDisposition)
{   
    if (contentDisposition.isEmpty())
        return ContentDispositionNone;

    // Some broken sites just send
    // Content-Disposition: ; filename="file"
    // screen those out here.
    if (contentDisposition.startsWith(";"))
        return ContentDispositionNone;

    if (contentDisposition.startsWith("inline", false))
        return ContentDispositionInline;

    // Some broken sites just send
    // Content-Disposition: filename="file"
    // without a disposition token... screen those out.
    if (contentDisposition.startsWith("filename", false))
        return ContentDispositionNone;

    // Also in use is Content-Disposition: name="file"
    if (contentDisposition.startsWith("name", false))
        return ContentDispositionNone;

    // We have a content-disposition of "attachment" or unknown.
    // RFC 2183, section 2.8 says that an unknown disposition
    // value should be treated as "attachment"
    return ContentDispositionAttachment;  
}

bool parseHTTPRefresh(const String& refresh, bool fromHttpEquivMeta, double& delay, String& url)
{
    unsigned len = refresh.length();
    unsigned pos = 0;
    
    if (!skipWhiteSpace(refresh, pos, fromHttpEquivMeta))
        return false;
    
    while (pos != len && refresh[pos] != ',' && refresh[pos] != ';')
        ++pos;
    
    if (pos == len) { // no URL
        url = String();
        bool ok;
        delay = refresh.stripWhiteSpace().toDouble(&ok);
        return ok;
    } else {
        bool ok;
        delay = refresh.left(pos).stripWhiteSpace().toDouble(&ok);
        if (!ok)
            return false;
        
        ++pos;
        skipWhiteSpace(refresh, pos, fromHttpEquivMeta);
        unsigned urlStartPos = pos;
        if (refresh.find("url", urlStartPos, false) == urlStartPos) {
            urlStartPos += 3;
            skipWhiteSpace(refresh, urlStartPos, fromHttpEquivMeta);
            if (refresh[urlStartPos] == '=') {
                ++urlStartPos;
                skipWhiteSpace(refresh, urlStartPos, fromHttpEquivMeta);
            } else
                urlStartPos = pos;  // e.g. "Refresh: 0; url.html"
        }

        unsigned urlEndPos = len;

        if (refresh[urlStartPos] == '"' || refresh[urlStartPos] == '\'') {
            UChar quotationMark = refresh[urlStartPos];
            urlStartPos++;
            while (urlEndPos > urlStartPos) {
                urlEndPos--;
                if (refresh[urlEndPos] == quotationMark)
                    break;
            }
            
            // https://bugs.webkit.org/show_bug.cgi?id=27868
            // Sometimes there is no closing quote for the end of the URL even though there was an opening quote.
            // If we looped over the entire alleged URL string back to the opening quote, just go ahead and use everything
            // after the opening quote instead.
            if (urlEndPos == urlStartPos)
                urlEndPos = len;
        }

        url = refresh.substring(urlStartPos, urlEndPos - urlStartPos).stripWhiteSpace();
        return true;
    }
}

double parseDate(const String& value)
{
    return parseDateFromNullTerminatedCharacters(value.utf8().data());
}

String filenameFromHTTPContentDisposition(const String& value)
{
    Vector<String> keyValuePairs;
    value.split(';', keyValuePairs);

    unsigned length = keyValuePairs.size();
    for (unsigned i = 0; i < length; i++) {
        size_t valueStartPos = keyValuePairs[i].find('=');
        if (valueStartPos == notFound)
            continue;

        String key = keyValuePairs[i].left(valueStartPos).stripWhiteSpace();

        if (key.isEmpty() || key != "filename")
            continue;
        
        String value = keyValuePairs[i].substring(valueStartPos + 1).stripWhiteSpace();

        // Remove quotes if there are any
        if (value[0] == '\"')
            value = value.substring(1, value.length() - 2);

        return value;
    }

    return String();
}

String extractMIMETypeFromMediaType(const String& mediaType)
{
    Vector<UChar, 64> mimeType;
    unsigned length = mediaType.length();
    mimeType.reserveCapacity(length);
    for (unsigned i = 0; i < length; i++) {
        UChar c = mediaType[i];

        if (c == ';')
            break;

        // While RFC 2616 does not allow it, other browsers allow multiple values in the HTTP media
        // type header field, Content-Type. In such cases, the media type string passed here may contain
        // the multiple values separated by commas. For now, this code ignores text after the first comma,
        // which prevents it from simply failing to parse such types altogether. Later for better
        // compatibility we could consider using the first or last valid MIME type instead.
        // See https://bugs.webkit.org/show_bug.cgi?id=25352 for more discussion.
        if (c == ',')
            break;

        // FIXME: The following is not correct. RFC 2616 allows linear white space before and
        // after the MIME type, but not within the MIME type itself. And linear white space
        // includes only a few specific ASCII characters; a small subset of isSpaceOrNewline.
        // See https://bugs.webkit.org/show_bug.cgi?id=8644 for a bug tracking part of this.
        if (isSpaceOrNewline(c))
            continue;

        mimeType.append(c);
    }

    if (mimeType.size() == length)
        return mediaType;
    return String(mimeType.data(), mimeType.size());
}

String extractCharsetFromMediaType(const String& mediaType)
{
    unsigned int pos, len;
    findCharsetInMediaType(mediaType, pos, len);
    return mediaType.substring(pos, len);
}

void findCharsetInMediaType(const String& mediaType, unsigned int& charsetPos, unsigned int& charsetLen, unsigned int start)
{
    charsetPos = start;
    charsetLen = 0;

    size_t pos = start;
    unsigned length = mediaType.length();
    
    while (pos < length) {
        pos = mediaType.find("charset", pos, false);
        if (pos == notFound || pos == 0) {
            charsetLen = 0;
            return;
        }
        
        // is what we found a beginning of a word?
        if (mediaType[pos-1] > ' ' && mediaType[pos-1] != ';') {
            pos += 7;
            continue;
        }
        
        pos += 7;

        // skip whitespace
        while (pos != length && mediaType[pos] <= ' ')
            ++pos;
    
        if (mediaType[pos++] != '=') // this "charset" substring wasn't a parameter name, but there may be others
            continue;

        while (pos != length && (mediaType[pos] <= ' ' || mediaType[pos] == '"' || mediaType[pos] == '\''))
            ++pos;

        // we don't handle spaces within quoted parameter values, because charset names cannot have any
        unsigned endpos = pos;
        while (pos != length && mediaType[endpos] > ' ' && mediaType[endpos] != '"' && mediaType[endpos] != '\'' && mediaType[endpos] != ';')
            ++endpos;

        charsetPos = pos;
        charsetLen = endpos - pos;
        return;
    }
}

XSSProtectionDisposition parseXSSProtectionHeader(const String& header)
{
    String stippedHeader = header.stripWhiteSpace();

    if (stippedHeader.isEmpty())
        return XSSProtectionEnabled;

    if (stippedHeader[0] == '0')
        return XSSProtectionDisabled;

    unsigned length = header.length();
    unsigned pos = 0;
    if (stippedHeader[pos++] == '1'
        && skipWhiteSpace(stippedHeader, pos, false)
        && stippedHeader[pos++] == ';'
        && skipWhiteSpace(stippedHeader, pos, false)
        && skipToken(stippedHeader, pos, "mode")
        && skipWhiteSpace(stippedHeader, pos, false)
        && stippedHeader[pos++] == '='
        && skipWhiteSpace(stippedHeader, pos, false)
        && skipToken(stippedHeader, pos, "block")
        && pos == length)
        return XSSProtectionBlockEnabled;

    return XSSProtectionEnabled;
}

String extractReasonPhraseFromHTTPStatusLine(const String& statusLine)
{
    size_t spacePos = statusLine.find(' ');
    // Remove status code from the status line.
    spacePos = statusLine.find(' ', spacePos + 1);
    return statusLine.substring(spacePos + 1);
}

bool parseRange(const String& range, long long& rangeOffset, long long& rangeEnd, long long& rangeSuffixLength)
{
    // The format of "Range" header is defined in RFC 2616 Section 14.35.1.
    // http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.35.1
    // We don't support multiple range requests.

    rangeOffset = rangeEnd = rangeSuffixLength = -1;

    // The "bytes" unit identifier should be present.
    static const char bytesStart[] = "bytes="; 
    if (!range.startsWith(bytesStart, false))
        return false;
    String byteRange = range.substring(sizeof(bytesStart) - 1);

    // The '-' character needs to be present.
    int index = byteRange.find('-');
    if (index == -1)
        return false;

    // If the '-' character is at the beginning, the suffix length, which specifies the last N bytes, is provided.
    // Example:
    //     -500
    if (!index) {
        String suffixLengthString = byteRange.substring(index + 1).stripWhiteSpace();
        bool ok;
        long long value = suffixLengthString.toInt64Strict(&ok);
        if (ok)
            rangeSuffixLength = value;
        return true;
    }

    // Otherwise, the first-byte-position and the last-byte-position are provied.
    // Examples:
    //     0-499
    //     500-
    String firstBytePosStr = byteRange.left(index).stripWhiteSpace();
    bool ok;
    long long firstBytePos = firstBytePosStr.toInt64Strict(&ok);
    if (!ok)
        return false;

    String lastBytePosStr = byteRange.substring(index + 1).stripWhiteSpace();
    long long lastBytePos = -1;
    if (!lastBytePosStr.isEmpty()) {
        lastBytePos = lastBytePosStr.toInt64Strict(&ok);
        if (!ok)
            return false;
    }

    if (firstBytePos < 0 || !(lastBytePos == -1 || lastBytePos >= firstBytePos))
        return false;

    rangeOffset = firstBytePos;
    rangeEnd = lastBytePos;
    return true;
}

}
