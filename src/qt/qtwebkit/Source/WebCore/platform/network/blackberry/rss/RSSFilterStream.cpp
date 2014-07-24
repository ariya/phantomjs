/*
 * Copyright (C) 2009, 2010, 2011, 2012, 2013 Research In Motion Limited. All rights reserved.
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

#include "config.h"
#include "RSSFilterStream.h"

#include "RSS10Parser.h"
#include "RSS20Parser.h"
#include "RSSAtomParser.h"
#include "RSSGenerator.h"
#include "TextCodecICU.h"

#include <BlackBerryPlatformAssert.h>
#include <BlackBerryPlatformLog.h>
#include <ScopePointer.h>
#include <network/NetworkRequest.h>
#include <wtf/ASCIICType.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

using BlackBerry::Platform::LogLevelCritical;
using BlackBerry::Platform::LogLevelInfo;
using BlackBerry::Platform::LogLevelWarn;
using BlackBerry::Platform::NetworkRequest;

namespace WebCore {

static const char* const s_utf8EncodingName = "UTF-8";
static const char* const s_latin1EncodingName = "ISO-8859-1";
static const char* const s_gbkEncodingName = "GBK";

static const char* const s_contentEncodingHeaderKey = "Content-Encoding";
static const char* const s_contentLengthHeaderKey = "Content-Length";
static const char* const s_contentTypeHeaderKey = "Content-Type";

static int isASCIISpaceLowerByte(int ch)
{
    return isASCIISpace<int>(ch & 0xff);
}

static std::string& stripWhiteSpace(std::string& str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(isASCIISpaceLowerByte))));
    str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(isASCIISpaceLowerByte))).base(), str.end());
    return str;
}

static inline bool equalIgnoringCase(const std::string& first, const char* second)
{
    return !strcasecmp(first.c_str(), second);
}

static inline bool isAtomMIMEType(const std::string& mimeType)
{
    return equalIgnoringCase(mimeType, "application/atom+xml");
}

static inline bool isRSSMIMEType(const std::string& mimeType)
{
    return equalIgnoringCase(mimeType, "application/rss+xml");
}

static inline bool isPotentialRSSMIMEType(const std::string& mimeType)
{
    return !strncmp(mimeType.data(), "text/xml", 8) || !strncmp(mimeType.data(), "application/xml", 15);
}

static inline bool isRSSContent(RSSFilterStream::ResourceType type)
{
    return type == RSSFilterStream::TypeRSS10
        || type == RSSFilterStream::TypeRSS20
        || type == RSSFilterStream::TypeRSSAtom;
}

static RSSFilterStream::ResourceType RSSTypeFromContentType(const std::string& contentType)
{
    if (contentType.empty())
        return RSSFilterStream::TypeUnknown;

    std::string mimeType;
    std::string version;
    bool isRSS = false;

    // The contentType can contain both a mime type and version attributes
    // If we already know the type is Atom, we don't need either of these, so skip it
    for (size_t start = 0, delimiter = 0; delimiter != std::string::npos; start = delimiter + 1) {
        delimiter = contentType.find(';', start);

        std::string component = contentType.substr(start, delimiter == std::string::npos ? delimiter : delimiter - start);

        if (mimeType.empty()) {
            if (isAtomMIMEType(component))
                return RSSFilterStream::TypeRSSAtom;

            if (isRSSMIMEType(component)) {
                // definitely not a version attribute, skip to next token
                mimeType = component;
                isRSS = true;
                continue;
            }
        }

        size_t equalsign = component.find('=');
        if (equalsign != std::string::npos && equalIgnoringCase(component.substr(0, equalsign), "version"))
            version = component.substr(equalsign + 1);

        // Check if we now have found both MIME type and version
        if (!mimeType.empty() && !version.empty())
            break;
    }

    if (isRSS) {
        if (!version.compare("1.0"))
            return RSSFilterStream::TypeRSS10;

        return RSSFilterStream::TypeRSS20;
    }

    return RSSFilterStream::TypeUnknown;
}

static RSSFilterStream::ResourceType RSSTypeFromContent(const char* str, int strLen)
{
    // Locate xml signature.
    const char* pos = str;

    // Locate the RSS tags.
    while (true) {
        pos = strchr(pos, '<');
        if (!pos || (pos - str >= strLen)) {
            BBLOG(LogLevelInfo, "RSSTypeFromContent(): can not find any start tag");
            return RSSFilterStream::TypeUnknown;
        }

        ++pos;

        if (!strncasecmp(pos, "rss", 3)) {
            if (isSpaceOrNewline(*(pos + 3))) {
                BBLOG(LogLevelInfo, "RSSTypeFromContent(): found RSS 2.0 tag");
                return RSSFilterStream::TypeRSS20;
            }

            BBLOG(LogLevelInfo, "RSSTypeFromContent(): wrong RSS 2.0 tag: %.*s", 32, pos);
            return RSSFilterStream::TypeUnknown;
        }

        if (!strncasecmp(pos, "feed", 4)) {
            if (isSpaceOrNewline(*(pos + 4))) {
                BBLOG(LogLevelInfo, "RSSTypeFromContent(): found Atom 1.0 tag");
                return RSSFilterStream::TypeRSSAtom;
            }

            BBLOG(LogLevelInfo, "RSSTypeFromContent(): wrong Atom 1.0 tag: %.*s", 32, pos);
            return RSSFilterStream::TypeUnknown;
        }

        if (!strncasecmp(pos, "rdf:RDF", 7)) {
            if (isSpaceOrNewline(*(pos + 7))) {
                BBLOG(LogLevelInfo, "RSSTypeFromContent(): found RSS 1.0 tag");
                return RSSFilterStream::TypeRSS10;
            }

            BBLOG(LogLevelInfo, "RSSTypeFromContent(): wrong RSS 1.0 tag: %.*s", 32, pos);
            return RSSFilterStream::TypeUnknown;
        }

        if (!strncasecmp(pos, "?xml", 4)) {
            BBLOG(LogLevelInfo, "RSSTypeFromContent(): found another xml tag");
            // This is another xml info, skip it.
            pos += 4;
            pos = strstr(pos, "?>");

            if (!pos || (pos - str >= strLen)) {
                BBLOG(LogLevelInfo, "RSSTypeFromContent(): extra xml tag too long");
                return RSSFilterStream::TypeUnknown;
            }

            pos += 2;
            continue;
        }

        if (!strncasecmp(pos, "!--", 3)) {
            BBLOG(LogLevelInfo, "RSSTypeFromContent(): found comments");
            // This is comments, skip it.
            pos += 3;
            pos = strstr(pos, "-->");

            if (!pos || (pos - str >= strLen)) {
                BBLOG(LogLevelInfo, "RSSTypeFromContent(): comments too long");
                return RSSFilterStream::TypeUnknown;
            }

            pos += 3;
            continue;
        }

        BBLOG(LogLevelInfo, "RSSTypeFromContent(): non rss type (%.*s) found", 32, pos);
        return RSSFilterStream::TypeUnknown;
    }

    return RSSFilterStream::TypeUnknown;
}

static PassOwnPtr<RSSParserBase> createParser(RSSFilterStream::ResourceType type)
{
    BLACKBERRY_ASSERT(isRSSContent(type));

    switch (type) {
    case RSSFilterStream::TypeRSSAtom:
        return adoptPtr(new RSSAtomParser());
    case RSSFilterStream::TypeRSS10:
        return adoptPtr(new RSS10Parser());
    case RSSFilterStream::TypeRSS20:
        return adoptPtr(new RSS20Parser());
    default:
        ASSERT_NOT_REACHED();
        return adoptPtr(new RSS20Parser());
    }
}

static bool findXMLEncodingPosition(const char* str, const char*& encodingStart, const char*& encodingEnd)
{
    encodingStart = strstr(str, "<?xml ");
    if (!encodingStart)
        return false;

    encodingStart += 6; // Length of "<?xml ".
    char* endPos = strstr(const_cast<char*>(encodingStart), "?>");
    if (!endPos)
        return false;

    encodingStart = strstr(const_cast<char*>(encodingStart), "encoding");

    if (!encodingStart || encodingStart > endPos)
        return false;

    encodingStart += 8;

    while (encodingStart < endPos && *encodingStart <= ' ')
        ++encodingStart;

    if (encodingStart == endPos || *encodingStart != '=')
        return false;

    ++encodingStart;

    while (encodingStart < endPos && *encodingStart <= ' ')
        ++encodingStart;

    if (encodingStart == endPos)
        return false;

    char quote = *encodingStart;
    if (quote != '\"' && quote != '\'')
        return false;

    ++encodingStart;
    encodingEnd = strchr(const_cast<char*>(encodingStart), quote);

    if (!encodingEnd || encodingEnd == encodingStart || encodingEnd > endPos)
        return false;

    return true;
}

static bool findXMLLanguagePosition(const char* str, const char*& langStart, const char*& langEnd)
{
    langStart = strstr(const_cast<char*>(str), "<language>");
    if (!langStart)
        return false;

    langStart += 10; // Length of "<language>".
    char* endPos = strstr(const_cast<char*>(langStart), "</language>");
    if (!endPos)
        return false;

    while (langStart < endPos && *langStart <= ' ')
        ++langStart;

    if (langStart == endPos)
        return false;

    langEnd = endPos - 1;

    while (langEnd > langStart && *langEnd <= ' ')
        --langEnd;

    ++langEnd;
    return true;
}

static const char* defaultEncodingForLanguage(const char* language)
{
    if (!strcasecmp(language, "en") || !strcasecmp(language, "en-US"))
        return s_latin1EncodingName;
    if (!strcasecmp(language, "zh-cn"))
        return s_gbkEncodingName;

    return 0;
}

static bool isTranscodingNeeded(const std::string& encoding)
{
    // When there's no encoding information, or the encoding can not be found in all encodings
    // supported in our phone, we will try to transcode the content anyway, supposed to ASCII.
    if (encoding.empty())
        return true;

    return TextEncoding(s_utf8EncodingName) != TextEncoding(encoding.c_str());
}

enum TranscodeResult {
    Success,
    SourceEncodingUnsupported,
    SourceBroken,
    TargetEncodingUnsupported,
    TargetBufferInsufficient
};

static TranscodeResult transcode(const char* sourceEncoding,
    const char* targetEncoding,
    const char*& sourceStart,
    int sourceLength,
    char*& targetStart,
    unsigned targetLength)
{
    TextEncoding textEncodingSource(sourceEncoding);
    if (!textEncodingSource.isValid())
        return SourceEncodingUnsupported;

    TextEncoding textEncodingTarget(targetEncoding);
    if (!textEncodingTarget.isValid())
        return TargetEncodingUnsupported;

    bool sawError = false;
    String ucs2 = TextCodecICU(textEncodingSource).decode(sourceStart, sourceLength, true, true, sawError);

    if (sawError)
        return SourceBroken;

    CString encoded = TextCodecICU(textEncodingTarget).encode(ucs2.characters(), ucs2.length(), WebCore::EntitiesForUnencodables);
    if (encoded.length() > targetLength)
        return TargetBufferInsufficient;

    strncpy(targetStart, encoded.data(), encoded.length());
    targetStart += encoded.length();

    return Success;
}

static bool transcodeContent(const std::string& content, const std::string& encoding, std::string& result)
{
    unsigned utf8length = content.size() * 3 / 2; // Use maximum number utf-8 length.
    ScopeArray<char> buffer(new char[utf8length + 1]);

    std::string xmlHeaderString;

    const char* start = content.c_str();
    const char* xmlStart = strstr(start, "<?xml ");
    const char* xmlEnd;
    if (xmlStart) {
        xmlEnd = strstr(xmlStart, "?>");
        if (!xmlEnd)
            return false;
        xmlEnd += 2; // Length of "?>".

        const char* encodingStart;
        const char* encodingEnd;
        if (findXMLEncodingPosition(start, encodingStart, encodingEnd)) {
            xmlHeaderString.assign(start, encodingStart - start);
            xmlHeaderString.append(s_utf8EncodingName);
            xmlHeaderString.append(encodingEnd, xmlEnd - encodingEnd);
        } else {
            xmlHeaderString.assign(start, (xmlEnd - 2) - start);
            xmlHeaderString.append(" encoding=\"UTF-8\"?>");
        }
    } else {
        xmlEnd = start;
        xmlHeaderString = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    }

    const char* contentStart = xmlEnd;
    char* bufferStart = &buffer[0];

    TranscodeResult transcodeResult = transcode(encoding.c_str(), s_utf8EncodingName, contentStart, content.length() - (xmlEnd - xmlStart), bufferStart, utf8length);

    // If the encoding is not supported, or if we failed to transcode the content, we will just
    // encode the content to ASCII content, and replacing any non-ASCII character with a question
    // mark. This will be better than displaying the raw text, as the raw text can not be displayed
    // correctly anyway, as their encoding is not supported.

    if (Success != transcodeResult) {
        int i = 0;
        for (; contentStart < start + content.size() ; ++contentStart, ++i) {
            if (isASCIIPrintable(*contentStart))
                buffer[i] = *contentStart;
            else
                buffer[i] = '?';
        }

        buffer[i] = 0;
    } else
        *bufferStart = 0;

    result.assign(xmlHeaderString);
    result.append(&buffer[0]);

    return true;
}

RSSFilterStream::RSSFilterStream()
    : FilterStream()
    , m_resourceType(TypeUnknown)
    , m_contentTypeIndex(-1)
    , m_charsetChecked(false)
    , m_encodingChecked(false)
{
}

void RSSFilterStream::notifyStatusReceived(int status, const BlackBerry::Platform::String& message)
{
    // Non-HTTP errors have no data to display, and redirects will never be displayed,
    // so no need to check if they have RSS data
    // HTTP errors may have data, and in theory it could be RSS data (although it's highly
    // unlikely) so better to keep checking them.
    if (status < 0 || (300 <= status && status < 400 && status != 304)) {
        m_resourceType = TypeNotRSS;
        // It is possible we will get 2 status code.
        sendSavedHeaders();
    }

    FilterStream::notifyStatusReceived(status, message);
}

void RSSFilterStream::notifyHeadersReceived(const NetworkRequest::HeaderList& headers)
{
    if (!isRSSContent(m_resourceType)) {
        NetworkRequest::HeaderList::const_iterator end = headers.end();
        NetworkRequest::HeaderList::const_iterator iter = headers.begin();
        for (; iter != end; ++iter) {
            if (equalIgnoringCase(iter->first.toStdString(), s_contentTypeHeaderKey)) {
                if (iter->second.find("xml") != std::string::npos) {
                    m_contentTypeIndex = std::distance(NetworkRequest::HeaderList::const_iterator(headers.begin()), iter);
                    ResourceType type = RSSTypeFromContentType(iter->second.toStdString());
                    if (isRSSContent(type))
                        m_resourceType = type;
                    else if (!isPotentialRSSMIMEType(iter->second.toStdString()))
                        m_resourceType = TypeNotRSS;
                }
                break;
            }
        }
    }

    if (m_contentTypeIndex < 0)
        m_resourceType = TypeNotRSS;

    if (m_resourceType == TypeNotRSS) {
        sendSavedHeaders();
        FilterStream::notifyHeadersReceived(headers);
    } else
        saveHeaders(headers);
}

void RSSFilterStream::notifyDataReceived(BlackBerry::Platform::NetworkBuffer* buffer)
{
    // Here we assume the first packet of data is enough for us to sniff the content type.
    // If someday some decent real life web site void this assumption, we should re-implement
    // this code. But for now it should be just fine.
    if (m_resourceType == TypeUnknown)
        m_resourceType = RSSTypeFromContent(BlackBerry::Platform::networkBufferData(buffer), BlackBerry::Platform::networkBufferDataLength(buffer));

    // If we don't know the resource type at this stage, then it is not a RSS content.
    if (m_resourceType == TypeUnknown)
        m_resourceType = TypeNotRSS;

    if (isRSSContent(m_resourceType))
        appendData(buffer);
    else {
        sendSavedHeaders();
        FilterStream::notifyDataReceived(buffer);
    }
}

void RSSFilterStream::notifyClose(int status)
{
    // If there was no data, we might get here with the type still unknown. No data at all -> no RSS
    // data.
    if (m_resourceType == TypeUnknown)
        m_resourceType = TypeNotRSS;

    if (isRSSContent(m_resourceType))
        handleRSSContent();
    else {
        sendSavedHeaders();
        FilterStream::notifyClose(status);
    }
}

bool RSSFilterStream::convertContentToHtml(std::string& result)
{
    bool success = false;
    const std::string& base = url().toStdString();
    const std::string& enc = encoding();

    OwnPtr<RSSParserBase> parser = createParser(m_resourceType);

    if (isTranscodingNeeded(enc)) {
        std::string transcodedContent;
        if (transcodeContent(m_content, enc, transcodedContent))
            success = parser->parseBuffer(transcodedContent.data(), transcodedContent.length(), base.c_str(), s_utf8EncodingName);
    } else
        success = parser->parseBuffer(m_content.data(), m_content.length(), base.c_str(), s_utf8EncodingName);

    if (!success)
        return false;

    // FIXME:
    // The HTML string generated below purports to be a UTF8-encoded
    // WTF::String, although its characters8() data should be Latin1.
    // We build then extract this string, pretending that we don't know
    // that we pass incorrectly-encoded char data both ways.
    // We should use BlackBerry::Platform::String instead of WTF::String.
    OwnPtr<RSSGenerator> generator = adoptPtr(new RSSGenerator());
    String html = generator->generateHtml(parser->m_root);
    ASSERT(html.is8Bit());
    result.assign(reinterpret_cast<const char*>(html.characters8()), html.length());

    return true;
}

void RSSFilterStream::handleRSSContent()
{
    BLACKBERRY_ASSERT(isRSSContent(m_resourceType));

    BlackBerry::Platform::NetworkBuffer* buffer;
    std::string html;
    if (!convertContentToHtml(html))
        buffer = BlackBerry::Platform::createNetworkBufferByCopyingData(m_content.c_str(), m_content.size());
    else {
        updateRSSHeaders(html.size());
        buffer = BlackBerry::Platform::createNetworkBufferByCopyingData(html.c_str(), html.size());
    }

    sendSavedHeaders();
    FilterStream::notifyDataReceived(buffer);
    BlackBerry::Platform::releaseNetworkBuffer(buffer);
    FilterStream::notifyClose(FilterStream::StatusSuccess);
}

const std::string& RSSFilterStream::charset()
{
    BLACKBERRY_ASSERT(m_contentTypeIndex >= 0);

    if (m_charsetChecked)
        return m_charset;

    m_charsetChecked = true;

    static const char* charsetPrefix = "charset=";
    static const int charsetPrefixLen = strlen(charsetPrefix);
    size_t pos = (m_headers.at(m_contentTypeIndex).second).find(charsetPrefix);
    if (pos != std::string::npos) {
        m_charset = m_headers.at(m_contentTypeIndex).second.substr(pos + charsetPrefixLen).toStdString();
        stripWhiteSpace(m_charset);
    }

    return m_charset;
}

const std::string& RSSFilterStream::encoding()
{
    if (m_encodingChecked)
        return m_encoding;

    m_encodingChecked = true;
    const std::string& str = charset();

    if (!str.empty()) {
        m_encoding = str;
        return m_encoding;
    }

    const char* start = m_content.c_str();
    const char* encodingStart = 0;
    const char* encodingEnd = 0;
    if (findXMLEncodingPosition(start, encodingStart, encodingEnd)) {
        m_encoding = std::string(encodingStart, encodingEnd - encodingStart);
        return m_encoding;
    }

    if (findXMLLanguagePosition(start, encodingStart, encodingEnd)) {
        std::string languageString(encodingStart, encodingEnd - encodingStart);
        const char* defaultEncoding = defaultEncodingForLanguage(languageString.c_str());
        if (defaultEncoding) {
            m_encoding = defaultEncoding;
            return m_encoding;
        }
    }

    return m_encoding;
}

void RSSFilterStream::saveHeaders(const BlackBerry::Platform::NetworkRequest::HeaderList& headers)
{
    m_headers = headers;
}

void RSSFilterStream::removeHeader(const char* key)
{
    NetworkRequest::HeaderList::iterator end = m_headers.end();
    for (NetworkRequest::HeaderList::iterator iter = m_headers.begin(); iter != end; ++iter) {
        if (!strcasecmp(iter->first.c_str(), key)) {
            m_headers.erase(iter);
            return;
        }
    }
}

void RSSFilterStream::updateHeader(const char* key, const BlackBerry::Platform::String& value)
{
    NetworkRequest::HeaderList::iterator iter = m_headers.begin();
    NetworkRequest::HeaderList::iterator end = m_headers.end();
    for (; iter != end; ++iter) {
        if (!strcasecmp(iter->first.c_str(), key)) {
            iter->second = value;
            return;
        }
    }

    m_headers.insert(iter, std::make_pair(BlackBerry::Platform::String::fromAscii(key), value));
}

void RSSFilterStream::updateRSSHeaders(size_t size)
{
    STATIC_LOCAL_STRING(s_htmlContentType, "text/html; charset=utf-8");

    removeHeader(s_contentEncodingHeaderKey);
    updateHeader(s_contentTypeHeaderKey, s_htmlContentType);
    updateHeader(s_contentLengthHeaderKey, BlackBerry::Platform::String::number(size));
}

void RSSFilterStream::sendSavedHeaders()
{
    if (!m_headers.empty()) {
        FilterStream::notifyHeadersReceived(m_headers);
        m_contentTypeIndex = -1;
        m_headers.clear();
    }
}

void RSSFilterStream::appendData(BlackBerry::Platform::NetworkBuffer* buffer)
{
    m_content.append(networkBufferData(buffer), networkBufferDataLength(buffer));
}

} // namespace WebCore
