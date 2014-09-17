/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#include "config.h"
#include "ResourceResponseBase.h"

#include "HTTPParsers.h"
#include "ResourceResponse.h"
#include <wtf/CurrentTime.h>
#include <wtf/MathExtras.h>
#include <wtf/StdLibExtras.h>

using namespace std;

namespace WebCore {

static void parseCacheHeader(const String& header, Vector<pair<String, String> >& result);

inline const ResourceResponse& ResourceResponseBase::asResourceResponse() const
{
    return *static_cast<const ResourceResponse*>(this);
}

ResourceResponseBase::ResourceResponseBase()  
    : m_expectedContentLength(0)
    , m_httpStatusCode(0)
    , m_lastModifiedDate(0)
    , m_wasCached(false)
    , m_connectionID(0)
    , m_connectionReused(false)
    , m_isNull(true)
    , m_haveParsedCacheControlHeader(false)
    , m_haveParsedAgeHeader(false)
    , m_haveParsedDateHeader(false)
    , m_haveParsedExpiresHeader(false)
    , m_haveParsedLastModifiedHeader(false)
    , m_cacheControlContainsNoCache(false)
    , m_cacheControlContainsNoStore(false)
    , m_cacheControlContainsMustRevalidate(false)
    , m_cacheControlMaxAge(0.0)
    , m_age(0.0)
    , m_date(0.0)
    , m_expires(0.0)
    , m_lastModified(0.0)
{
}

ResourceResponseBase::ResourceResponseBase(const KURL& url, const String& mimeType, long long expectedLength, const String& textEncodingName, const String& filename)
    : m_url(url)
    , m_mimeType(mimeType)
    , m_expectedContentLength(expectedLength)
    , m_textEncodingName(textEncodingName)
    , m_suggestedFilename(filename)
    , m_httpStatusCode(0)
    , m_lastModifiedDate(0)
    , m_wasCached(false)
    , m_connectionID(0)
    , m_connectionReused(false)
    , m_isNull(false)
    , m_haveParsedCacheControlHeader(false)
    , m_haveParsedAgeHeader(false)
    , m_haveParsedDateHeader(false)
    , m_haveParsedExpiresHeader(false)
    , m_haveParsedLastModifiedHeader(false)
    , m_cacheControlContainsNoCache(false)
    , m_cacheControlContainsNoStore(false)
    , m_cacheControlContainsMustRevalidate(false)
    , m_cacheControlMaxAge(0.0)
    , m_age(0.0)
    , m_date(0.0)
    , m_expires(0.0)
    , m_lastModified(0.0)
{
}

PassOwnPtr<ResourceResponse> ResourceResponseBase::adopt(PassOwnPtr<CrossThreadResourceResponseData> data)
{
    OwnPtr<ResourceResponse> response = adoptPtr(new ResourceResponse);
    response->setURL(data->m_url);
    response->setMimeType(data->m_mimeType);
    response->setExpectedContentLength(data->m_expectedContentLength);
    response->setTextEncodingName(data->m_textEncodingName);
    response->setSuggestedFilename(data->m_suggestedFilename);

    response->setHTTPStatusCode(data->m_httpStatusCode);
    response->setHTTPStatusText(data->m_httpStatusText);

    response->lazyInit(AllFields);
    response->m_httpHeaderFields.adopt(data->m_httpHeaders.release());
    response->setLastModifiedDate(data->m_lastModifiedDate);
    response->setResourceLoadTiming(data->m_resourceLoadTiming.release());
    response->doPlatformAdopt(data);
    return response.release();
}

PassOwnPtr<CrossThreadResourceResponseData> ResourceResponseBase::copyData() const
{
    OwnPtr<CrossThreadResourceResponseData> data = adoptPtr(new CrossThreadResourceResponseData);
    data->m_url = url().copy();
    data->m_mimeType = mimeType().crossThreadString();
    data->m_expectedContentLength = expectedContentLength();
    data->m_textEncodingName = textEncodingName().crossThreadString();
    data->m_suggestedFilename = suggestedFilename().crossThreadString();
    data->m_httpStatusCode = httpStatusCode();
    data->m_httpStatusText = httpStatusText().crossThreadString();
    data->m_httpHeaders = httpHeaderFields().copyData();
    data->m_lastModifiedDate = lastModifiedDate();
    if (m_resourceLoadTiming)
        data->m_resourceLoadTiming = m_resourceLoadTiming->deepCopy();
    return asResourceResponse().doPlatformCopyData(data.release());
}

bool ResourceResponseBase::isHTTP() const
{
    lazyInit(CommonFieldsOnly);

    String protocol = m_url.protocol();

    return equalIgnoringCase(protocol, "http")  || equalIgnoringCase(protocol, "https");
}

const KURL& ResourceResponseBase::url() const
{
    lazyInit(CommonFieldsOnly);

    return m_url; 
}

void ResourceResponseBase::setURL(const KURL& url)
{
    lazyInit(CommonFieldsOnly);
    m_isNull = false;

    m_url = url; 
}

const String& ResourceResponseBase::mimeType() const
{
    lazyInit(CommonFieldsOnly);

    return m_mimeType; 
}

void ResourceResponseBase::setMimeType(const String& mimeType)
{
    lazyInit(CommonFieldsOnly);
    m_isNull = false;

    m_mimeType = mimeType; 
}

long long ResourceResponseBase::expectedContentLength() const 
{
    lazyInit(CommonFieldsOnly);

    return m_expectedContentLength;
}

void ResourceResponseBase::setExpectedContentLength(long long expectedContentLength)
{
    lazyInit(CommonFieldsOnly);
    m_isNull = false;

    m_expectedContentLength = expectedContentLength; 
}

const String& ResourceResponseBase::textEncodingName() const
{
    lazyInit(CommonFieldsOnly);

    return m_textEncodingName;
}

void ResourceResponseBase::setTextEncodingName(const String& encodingName)
{
    lazyInit(CommonFieldsOnly);
    m_isNull = false;

    m_textEncodingName = encodingName; 
}

// FIXME should compute this on the fly
const String& ResourceResponseBase::suggestedFilename() const
{
    lazyInit(CommonFieldsOnly);

    return m_suggestedFilename;
}

void ResourceResponseBase::setSuggestedFilename(const String& suggestedName)
{
    lazyInit(CommonFieldsOnly);
    m_isNull = false;

    m_suggestedFilename = suggestedName; 
}

int ResourceResponseBase::httpStatusCode() const
{
    lazyInit(CommonFieldsOnly);

    return m_httpStatusCode;
}

void ResourceResponseBase::setHTTPStatusCode(int statusCode)
{
    lazyInit(CommonFieldsOnly);

    m_httpStatusCode = statusCode;
}

const String& ResourceResponseBase::httpStatusText() const 
{
    lazyInit(AllFields);

    return m_httpStatusText; 
}

void ResourceResponseBase::setHTTPStatusText(const String& statusText) 
{
    lazyInit(AllFields);

    m_httpStatusText = statusText; 
}

String ResourceResponseBase::httpHeaderField(const AtomicString& name) const
{
    lazyInit(CommonFieldsOnly);

    // If we already have the header, just return it instead of consuming memory by grabing all headers.
    String value = m_httpHeaderFields.get(name);
    if (!value.isEmpty())        
        return value;

    lazyInit(AllFields);

    return m_httpHeaderFields.get(name); 
}

String ResourceResponseBase::httpHeaderField(const char* name) const
{
    lazyInit(CommonFieldsOnly);

    // If we already have the header, just return it instead of consuming memory by grabing all headers.
    String value = m_httpHeaderFields.get(name);
    if (!value.isEmpty())
        return value;

    lazyInit(AllFields);

    return m_httpHeaderFields.get(name); 
}

void ResourceResponseBase::setHTTPHeaderField(const AtomicString& name, const String& value)
{
    lazyInit(AllFields);

    DEFINE_STATIC_LOCAL(const AtomicString, ageHeader, ("age"));
    DEFINE_STATIC_LOCAL(const AtomicString, cacheControlHeader, ("cache-control"));
    DEFINE_STATIC_LOCAL(const AtomicString, dateHeader, ("date"));
    DEFINE_STATIC_LOCAL(const AtomicString, expiresHeader, ("expires"));
    DEFINE_STATIC_LOCAL(const AtomicString, lastModifiedHeader, ("last-modified"));
    DEFINE_STATIC_LOCAL(const AtomicString, pragmaHeader, ("pragma"));
    if (equalIgnoringCase(name, ageHeader))
        m_haveParsedAgeHeader = false;
    else if (equalIgnoringCase(name, cacheControlHeader) || equalIgnoringCase(name, pragmaHeader))
        m_haveParsedCacheControlHeader = false;
    else if (equalIgnoringCase(name, dateHeader))
        m_haveParsedDateHeader = false;
    else if (equalIgnoringCase(name, expiresHeader))
        m_haveParsedExpiresHeader = false;
    else if (equalIgnoringCase(name, lastModifiedHeader))
        m_haveParsedLastModifiedHeader = false;

    m_httpHeaderFields.set(name, value);
}

const HTTPHeaderMap& ResourceResponseBase::httpHeaderFields() const
{
    lazyInit(AllFields);

    return m_httpHeaderFields;
}

void ResourceResponseBase::parseCacheControlDirectives() const
{
    ASSERT(!m_haveParsedCacheControlHeader);

    lazyInit(CommonFieldsOnly);

    m_haveParsedCacheControlHeader = true;

    m_cacheControlContainsMustRevalidate = false;
    m_cacheControlContainsNoCache = false;
    m_cacheControlMaxAge = numeric_limits<double>::quiet_NaN();

    DEFINE_STATIC_LOCAL(const AtomicString, cacheControlString, ("cache-control"));
    DEFINE_STATIC_LOCAL(const AtomicString, noCacheDirective, ("no-cache"));
    DEFINE_STATIC_LOCAL(const AtomicString, noStoreDirective, ("no-store"));
    DEFINE_STATIC_LOCAL(const AtomicString, mustRevalidateDirective, ("must-revalidate"));
    DEFINE_STATIC_LOCAL(const AtomicString, maxAgeDirective, ("max-age"));

    String cacheControlValue = m_httpHeaderFields.get(cacheControlString);
    if (!cacheControlValue.isEmpty()) {
        Vector<pair<String, String> > directives;
        parseCacheHeader(cacheControlValue, directives);

        size_t directivesSize = directives.size();
        for (size_t i = 0; i < directivesSize; ++i) {
            // RFC2616 14.9.1: A no-cache directive with a value is only meaningful for proxy caches.
            // It should be ignored by a browser level cache.
            if (equalIgnoringCase(directives[i].first, noCacheDirective) && directives[i].second.isEmpty())
                m_cacheControlContainsNoCache = true;
            else if (equalIgnoringCase(directives[i].first, noStoreDirective))
                m_cacheControlContainsNoStore = true;
            else if (equalIgnoringCase(directives[i].first, mustRevalidateDirective))
                m_cacheControlContainsMustRevalidate = true;
            else if (equalIgnoringCase(directives[i].first, maxAgeDirective)) {
                bool ok;
                double maxAge = directives[i].second.toDouble(&ok);
                if (ok)
                    m_cacheControlMaxAge = maxAge;
            }
        }
    }

    if (!m_cacheControlContainsNoCache) {
        // Handle Pragma: no-cache
        // This is deprecated and equivalent to Cache-control: no-cache
        // Don't bother tokenizing the value, it is not important
        DEFINE_STATIC_LOCAL(const AtomicString, pragmaHeader, ("pragma"));
        String pragmaValue = m_httpHeaderFields.get(pragmaHeader);

        m_cacheControlContainsNoCache = pragmaValue.lower().contains(noCacheDirective);
    }
}
    
bool ResourceResponseBase::cacheControlContainsNoCache() const
{
    if (!m_haveParsedCacheControlHeader)
        parseCacheControlDirectives();
    return m_cacheControlContainsNoCache;
}

bool ResourceResponseBase::cacheControlContainsNoStore() const
{
    if (!m_haveParsedCacheControlHeader)
        parseCacheControlDirectives();
    return m_cacheControlContainsNoStore;
}

bool ResourceResponseBase::cacheControlContainsMustRevalidate() const
{
    if (!m_haveParsedCacheControlHeader)
        parseCacheControlDirectives();
    return m_cacheControlContainsMustRevalidate;
}

bool ResourceResponseBase::hasCacheValidatorFields() const
{
    lazyInit(CommonFieldsOnly);

    DEFINE_STATIC_LOCAL(const AtomicString, lastModifiedHeader, ("last-modified"));
    DEFINE_STATIC_LOCAL(const AtomicString, eTagHeader, ("etag"));
    return !m_httpHeaderFields.get(lastModifiedHeader).isEmpty() || !m_httpHeaderFields.get(eTagHeader).isEmpty();
}

double ResourceResponseBase::cacheControlMaxAge() const
{
    if (!m_haveParsedCacheControlHeader)
        parseCacheControlDirectives();
    return m_cacheControlMaxAge;
}

static double parseDateValueInHeader(const HTTPHeaderMap& headers, const AtomicString& headerName)
{
    String headerValue = headers.get(headerName);
    if (headerValue.isEmpty())
        return std::numeric_limits<double>::quiet_NaN(); 
    // This handles all date formats required by RFC2616:
    // Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
    // Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
    // Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format
    double dateInMilliseconds = parseDate(headerValue);
    if (!isfinite(dateInMilliseconds))
        return std::numeric_limits<double>::quiet_NaN();
    return dateInMilliseconds / 1000;
}

double ResourceResponseBase::date() const
{
    lazyInit(CommonFieldsOnly);

    if (!m_haveParsedDateHeader) {
        DEFINE_STATIC_LOCAL(const AtomicString, headerName, ("date"));
        m_date = parseDateValueInHeader(m_httpHeaderFields, headerName);
        m_haveParsedDateHeader = true;
    }
    return m_date;
}

double ResourceResponseBase::age() const
{
    lazyInit(CommonFieldsOnly);

    if (!m_haveParsedAgeHeader) {
        DEFINE_STATIC_LOCAL(const AtomicString, headerName, ("age"));
        String headerValue = m_httpHeaderFields.get(headerName);
        bool ok;
        m_age = headerValue.toDouble(&ok);
        if (!ok)
            m_age = std::numeric_limits<double>::quiet_NaN();
        m_haveParsedAgeHeader = true;
    }
    return m_age;
}

double ResourceResponseBase::expires() const
{
    lazyInit(CommonFieldsOnly);

    if (!m_haveParsedExpiresHeader) {
        DEFINE_STATIC_LOCAL(const AtomicString, headerName, ("expires"));
        m_expires = parseDateValueInHeader(m_httpHeaderFields, headerName);
        m_haveParsedExpiresHeader = true;
    }
    return m_expires;
}

double ResourceResponseBase::lastModified() const
{
    lazyInit(CommonFieldsOnly);

    if (!m_haveParsedLastModifiedHeader) {
        DEFINE_STATIC_LOCAL(const AtomicString, headerName, ("last-modified"));
        m_lastModified = parseDateValueInHeader(m_httpHeaderFields, headerName);
        m_haveParsedLastModifiedHeader = true;
    }
    return m_lastModified;
}

bool ResourceResponseBase::isAttachment() const
{
    lazyInit(AllFields);

    DEFINE_STATIC_LOCAL(const AtomicString, headerName, ("content-disposition"));
    String value = m_httpHeaderFields.get(headerName);
    size_t loc = value.find(';');
    if (loc != notFound)
        value = value.left(loc);
    value = value.stripWhiteSpace();
    DEFINE_STATIC_LOCAL(const AtomicString, attachmentString, ("attachment"));
    return equalIgnoringCase(value, attachmentString);
}
  
void ResourceResponseBase::setLastModifiedDate(time_t lastModifiedDate)
{
    lazyInit(AllFields);

    m_lastModifiedDate = lastModifiedDate;
}

time_t ResourceResponseBase::lastModifiedDate() const
{
    lazyInit(AllFields);

    return m_lastModifiedDate;
}

bool ResourceResponseBase::wasCached() const
{
    lazyInit(AllFields);

    return m_wasCached;
}

void ResourceResponseBase::setWasCached(bool value)
{
    m_wasCached = value;
}

bool ResourceResponseBase::connectionReused() const
{
    lazyInit(AllFields);

    return m_connectionReused;
}

void ResourceResponseBase::setConnectionReused(bool connectionReused)
{
    lazyInit(AllFields);

    m_connectionReused = connectionReused;
}

unsigned ResourceResponseBase::connectionID() const
{
    lazyInit(AllFields);

    return m_connectionID;
}

void ResourceResponseBase::setConnectionID(unsigned connectionID)
{
    lazyInit(AllFields);

    m_connectionID = connectionID;
}

ResourceLoadTiming* ResourceResponseBase::resourceLoadTiming() const
{
    lazyInit(AllFields);

    return m_resourceLoadTiming.get();
}

void ResourceResponseBase::setResourceLoadTiming(PassRefPtr<ResourceLoadTiming> resourceLoadTiming)
{
    lazyInit(AllFields);

    m_resourceLoadTiming = resourceLoadTiming;
}

PassRefPtr<ResourceLoadInfo> ResourceResponseBase::resourceLoadInfo() const
{
    lazyInit(AllFields);

    return m_resourceLoadInfo.get();
}

void ResourceResponseBase::setResourceLoadInfo(PassRefPtr<ResourceLoadInfo> loadInfo)
{
    lazyInit(AllFields);

    m_resourceLoadInfo = loadInfo;
}

void ResourceResponseBase::lazyInit(InitLevel initLevel) const
{
    const_cast<ResourceResponse*>(static_cast<const ResourceResponse*>(this))->platformLazyInit(initLevel);
}
    
bool ResourceResponseBase::compare(const ResourceResponse& a, const ResourceResponse& b)
{
    if (a.isNull() != b.isNull())
        return false;  
    if (a.url() != b.url())
        return false;
    if (a.mimeType() != b.mimeType())
        return false;
    if (a.expectedContentLength() != b.expectedContentLength())
        return false;
    if (a.textEncodingName() != b.textEncodingName())
        return false;
    if (a.suggestedFilename() != b.suggestedFilename())
        return false;
    if (a.httpStatusCode() != b.httpStatusCode())
        return false;
    if (a.httpStatusText() != b.httpStatusText())
        return false;
    if (a.httpHeaderFields() != b.httpHeaderFields())
        return false;
    if (a.resourceLoadTiming() && b.resourceLoadTiming() && *a.resourceLoadTiming() == *b.resourceLoadTiming())
        return ResourceResponse::platformCompare(a, b);
    if (a.resourceLoadTiming() != b.resourceLoadTiming())
        return false;
    return ResourceResponse::platformCompare(a, b);
}

static bool isCacheHeaderSeparator(UChar c)
{
    // See RFC 2616, Section 2.2
    switch (c) {
        case '(':
        case ')':
        case '<':
        case '>':
        case '@':
        case ',':
        case ';':
        case ':':
        case '\\':
        case '"':
        case '/':
        case '[':
        case ']':
        case '?':
        case '=':
        case '{':
        case '}':
        case ' ':
        case '\t':
            return true;
        default:
            return false;
    }
}

static bool isControlCharacter(UChar c)
{
    return c < ' ' || c == 127;
}

static inline String trimToNextSeparator(const String& str)
{
    return str.substring(0, str.find(isCacheHeaderSeparator, 0));
}

static void parseCacheHeader(const String& header, Vector<pair<String, String> >& result)
{
    const String safeHeader = header.removeCharacters(isControlCharacter);
    unsigned max = safeHeader.length();
    for (unsigned pos = 0; pos < max; /* pos incremented in loop */) {
        size_t nextCommaPosition = safeHeader.find(',', pos);
        size_t nextEqualSignPosition = safeHeader.find('=', pos);
        if (nextEqualSignPosition != notFound && (nextEqualSignPosition < nextCommaPosition || nextCommaPosition == notFound)) {
            // Get directive name, parse right hand side of equal sign, then add to map
            String directive = trimToNextSeparator(safeHeader.substring(pos, nextEqualSignPosition - pos).stripWhiteSpace());
            pos += nextEqualSignPosition - pos + 1;

            String value = safeHeader.substring(pos, max - pos).stripWhiteSpace();
            if (value[0] == '"') {
                // The value is a quoted string
                size_t nextDoubleQuotePosition = value.find('"', 1);
                if (nextDoubleQuotePosition != notFound) {
                    // Store the value as a quoted string without quotes
                    result.append(pair<String, String>(directive, value.substring(1, nextDoubleQuotePosition - 1).stripWhiteSpace()));
                    pos += (safeHeader.find('"', pos) - pos) + nextDoubleQuotePosition + 1;
                    // Move past next comma, if there is one
                    size_t nextCommaPosition2 = safeHeader.find(',', pos);
                    if (nextCommaPosition2 != notFound)
                        pos += nextCommaPosition2 - pos + 1;
                    else
                        return; // Parse error if there is anything left with no comma
                } else {
                    // Parse error; just use the rest as the value
                    result.append(pair<String, String>(directive, trimToNextSeparator(value.substring(1, value.length() - 1).stripWhiteSpace())));
                    return;
                }
            } else {
                // The value is a token until the next comma
                size_t nextCommaPosition2 = value.find(',', 0);
                if (nextCommaPosition2 != notFound) {
                    // The value is delimited by the next comma
                    result.append(pair<String, String>(directive, trimToNextSeparator(value.substring(0, nextCommaPosition2).stripWhiteSpace())));
                    pos += (safeHeader.find(',', pos) - pos) + 1;
                } else {
                    // The rest is the value; no change to value needed
                    result.append(pair<String, String>(directive, trimToNextSeparator(value)));
                    return;
                }
            }
        } else if (nextCommaPosition != notFound && (nextCommaPosition < nextEqualSignPosition || nextEqualSignPosition == notFound)) {
            // Add directive to map with empty string as value
            result.append(pair<String, String>(trimToNextSeparator(safeHeader.substring(pos, nextCommaPosition - pos).stripWhiteSpace()), ""));
            pos += nextCommaPosition - pos + 1;
        } else {
            // Add last directive to map with empty string as value
            result.append(pair<String, String>(trimToNextSeparator(safeHeader.substring(pos, max - pos).stripWhiteSpace()), ""));
            return;
        }
    }
}

}
