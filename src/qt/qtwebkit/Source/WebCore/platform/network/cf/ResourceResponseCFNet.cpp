/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
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
#include "ResourceResponse.h"

#if USE(CFNETWORK)

#include "HTTPParsers.h"
#include "MIMETypeRegistry.h"
#include <CFNetwork/CFURLResponsePriv.h>
#include <wtf/RetainPtr.h>

using namespace std;

// We would like a better value for a maximum time_t,
// but there is no way to do that in C with any certainty.
// INT_MAX should work well enough for our purposes.
#define MAX_TIME_T ((time_t)INT_MAX)    

namespace WebCore {

static CFStringRef const commonHeaderFields[] = {
    CFSTR("Age"), CFSTR("Cache-Control"), CFSTR("Content-Type"), CFSTR("Date"), CFSTR("Etag"), CFSTR("Expires"), CFSTR("Last-Modified"), CFSTR("Pragma")
};
static const int numCommonHeaderFields = sizeof(commonHeaderFields) / sizeof(CFStringRef);

CFURLResponseRef ResourceResponse::cfURLResponse() const
{
    if (!m_cfResponse && !m_isNull) {
        RetainPtr<CFURLRef> url = m_url.createCFURL();

        // FIXME: This creates a very incomplete CFURLResponse, which does not even have a status code.

        m_cfResponse = adoptCF(CFURLResponseCreate(0, url.get(), m_mimeType.createCFString().get(), m_expectedContentLength, m_textEncodingName.createCFString().get(), kCFURLCacheStorageAllowed));
    }

    return m_cfResponse.get();
}

static inline bool filenameHasSaneExtension(const String& filename)
{
    int dot = filename.find('.');

    // The dot can't be the first or last character in the filename.
    int length = filename.length();
    return dot > 0 && dot < length - 1;
}

static time_t toTimeT(CFAbsoluteTime time)
{
    static const double maxTimeAsDouble = std::numeric_limits<time_t>::max();
    static const double minTimeAsDouble = std::numeric_limits<time_t>::min();
    return static_cast<time_t>(min(max(minTimeAsDouble, time + kCFAbsoluteTimeIntervalSince1970), maxTimeAsDouble));
}

void ResourceResponse::platformLazyInit(InitLevel initLevel)
{
    if (m_initLevel > initLevel)
        return;

    if (m_isNull || !m_cfResponse.get())
        return;

    if (m_initLevel < CommonFieldsOnly && initLevel >= CommonFieldsOnly) {
        m_url = CFURLResponseGetURL(m_cfResponse.get());
        m_mimeType = CFURLResponseGetMIMEType(m_cfResponse.get());
        m_expectedContentLength = CFURLResponseGetExpectedContentLength(m_cfResponse.get());
        m_textEncodingName = CFURLResponseGetTextEncodingName(m_cfResponse.get());

        // Workaround for <rdar://problem/8757088>, can be removed once that is fixed.
        unsigned textEncodingNameLength = m_textEncodingName.length();
        if (textEncodingNameLength >= 2 && m_textEncodingName[0U] == '"' && m_textEncodingName[textEncodingNameLength - 1] == '"')
            m_textEncodingName = m_textEncodingName.substring(1, textEncodingNameLength - 2);

        m_lastModifiedDate = toTimeT(CFURLResponseGetLastModifiedDate(m_cfResponse.get()));

        CFHTTPMessageRef httpResponse = CFURLResponseGetHTTPResponse(m_cfResponse.get());
        if (httpResponse) {
            m_httpStatusCode = CFHTTPMessageGetResponseStatusCode(httpResponse);
            
            RetainPtr<CFDictionaryRef> headers = adoptCF(CFHTTPMessageCopyAllHeaderFields(httpResponse));
            
            for (int i = 0; i < numCommonHeaderFields; i++) {
                CFStringRef value;
                if (CFDictionaryGetValueIfPresent(headers.get(), commonHeaderFields[i], (const void **)&value))
                    m_httpHeaderFields.set(commonHeaderFields[i], value);
            }
        } else
            m_httpStatusCode = 0;
    }

    if (m_initLevel < CommonAndUncommonFields && initLevel >= CommonAndUncommonFields) {
        CFHTTPMessageRef httpResponse = CFURLResponseGetHTTPResponse(m_cfResponse.get());
        if (httpResponse) {
            RetainPtr<CFStringRef> statusLine = adoptCF(CFHTTPMessageCopyResponseStatusLine(httpResponse));
            m_httpStatusText = extractReasonPhraseFromHTTPStatusLine(statusLine.get());

            RetainPtr<CFDictionaryRef> headers = adoptCF(CFHTTPMessageCopyAllHeaderFields(httpResponse));
            CFIndex headerCount = CFDictionaryGetCount(headers.get());
            Vector<const void*, 128> keys(headerCount);
            Vector<const void*, 128> values(headerCount);
            CFDictionaryGetKeysAndValues(headers.get(), keys.data(), values.data());
            for (int i = 0; i < headerCount; ++i)
                m_httpHeaderFields.set((CFStringRef)keys[i], (CFStringRef)values[i]);
        }
    }
    
    if (m_initLevel < AllFields && initLevel >= AllFields) {
        RetainPtr<CFStringRef> suggestedFilename = adoptCF(CFURLResponseCopySuggestedFilename(m_cfResponse.get()));
        m_suggestedFilename = suggestedFilename.get();
    }

    m_initLevel = initLevel;
}
    
bool ResourceResponse::platformCompare(const ResourceResponse& a, const ResourceResponse& b)
{
    // CFEqual crashes if you pass it 0 so do an early check before calling it.
    if (!a.cfURLResponse() || !b.cfURLResponse())
        return a.cfURLResponse() == b.cfURLResponse();
    return CFEqual(a.cfURLResponse(), b.cfURLResponse());
}


} // namespace WebCore

#endif // USE(CFNETWORK)
