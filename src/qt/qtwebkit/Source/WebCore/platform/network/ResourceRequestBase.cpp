/*
 * Copyright (C) 2003, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2009, 2012 Google Inc. All rights reserved.
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
#include "ResourceRequestBase.h"

#include "ResourceRequest.h"

using namespace std;

namespace WebCore {

#if !USE(SOUP) && (!PLATFORM(MAC) || USE(CFNETWORK)) && !PLATFORM(QT)
double ResourceRequestBase::s_defaultTimeoutInterval = INT_MAX;
#else
// Will use NSURLRequest default timeout unless set to a non-zero value with setDefaultTimeoutInterval().
// For libsoup the timeout enabled with integer milliseconds. We set 0 as the default value to avoid integer overflow.
double ResourceRequestBase::s_defaultTimeoutInterval = 0;
#endif

inline const ResourceRequest& ResourceRequestBase::asResourceRequest() const
{
    return *static_cast<const ResourceRequest*>(this);
}

PassOwnPtr<ResourceRequest> ResourceRequestBase::adopt(PassOwnPtr<CrossThreadResourceRequestData> data)
{
    OwnPtr<ResourceRequest> request = adoptPtr(new ResourceRequest());
    request->setURL(data->m_url);
    request->setCachePolicy(data->m_cachePolicy);
    request->setTimeoutInterval(data->m_timeoutInterval);
    request->setFirstPartyForCookies(data->m_firstPartyForCookies);
    request->setHTTPMethod(data->m_httpMethod);
    request->setPriority(data->m_priority);

    request->updateResourceRequest();
    request->m_httpHeaderFields.adopt(data->m_httpHeaders.release());

    size_t encodingCount = data->m_responseContentDispositionEncodingFallbackArray.size();
    if (encodingCount > 0) {
        String encoding1 = data->m_responseContentDispositionEncodingFallbackArray[0];
        String encoding2;
        String encoding3;
        if (encodingCount > 1) {
            encoding2 = data->m_responseContentDispositionEncodingFallbackArray[1];
            if (encodingCount > 2)
                encoding3 = data->m_responseContentDispositionEncodingFallbackArray[2];
        }
        ASSERT(encodingCount <= 3);
        request->setResponseContentDispositionEncodingFallbackArray(encoding1, encoding2, encoding3);
    }
    request->setHTTPBody(data->m_httpBody);
    request->setAllowCookies(data->m_allowCookies);
    request->doPlatformAdopt(data);
    return request.release();
}

PassOwnPtr<CrossThreadResourceRequestData> ResourceRequestBase::copyData() const
{
    OwnPtr<CrossThreadResourceRequestData> data = adoptPtr(new CrossThreadResourceRequestData());
    data->m_url = url().copy();
    data->m_cachePolicy = cachePolicy();
    data->m_timeoutInterval = timeoutInterval();
    data->m_firstPartyForCookies = firstPartyForCookies().copy();
    data->m_httpMethod = httpMethod().isolatedCopy();
    data->m_httpHeaders = httpHeaderFields().copyData();
    data->m_priority = priority();

    data->m_responseContentDispositionEncodingFallbackArray.reserveInitialCapacity(m_responseContentDispositionEncodingFallbackArray.size());
    size_t encodingArraySize = m_responseContentDispositionEncodingFallbackArray.size();
    for (size_t index = 0; index < encodingArraySize; ++index) {
        data->m_responseContentDispositionEncodingFallbackArray.append(m_responseContentDispositionEncodingFallbackArray[index].isolatedCopy());
    }
    if (m_httpBody)
        data->m_httpBody = m_httpBody->deepCopy();
    data->m_allowCookies = m_allowCookies;
    return asResourceRequest().doPlatformCopyData(data.release());
}

bool ResourceRequestBase::isEmpty() const
{
    updateResourceRequest(); 
    
    return m_url.isEmpty(); 
}

bool ResourceRequestBase::isNull() const
{
    updateResourceRequest(); 
    
    return m_url.isNull();
}

const KURL& ResourceRequestBase::url() const 
{
    updateResourceRequest(); 
    
    return m_url;
}

void ResourceRequestBase::setURL(const KURL& url)
{ 
    updateResourceRequest(); 

    m_url = url; 
    
    m_platformRequestUpdated = false;
}

void ResourceRequestBase::removeCredentials()
{
    updateResourceRequest(); 

    if (m_url.user().isEmpty() && m_url.pass().isEmpty())
        return;

    m_url.setUser(String());
    m_url.setPass(String());

    m_platformRequestUpdated = false;
}

ResourceRequestCachePolicy ResourceRequestBase::cachePolicy() const
{
    updateResourceRequest(); 
    
    return m_cachePolicy; 
}

void ResourceRequestBase::setCachePolicy(ResourceRequestCachePolicy cachePolicy)
{
    updateResourceRequest(); 

    if (m_cachePolicy == cachePolicy)
        return;
    
    m_cachePolicy = cachePolicy;
    
    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

double ResourceRequestBase::timeoutInterval() const
{
    updateResourceRequest(); 
    
    return m_timeoutInterval; 
}

void ResourceRequestBase::setTimeoutInterval(double timeoutInterval) 
{
    updateResourceRequest(); 
    
    if (m_timeoutInterval == timeoutInterval)
        return;

    m_timeoutInterval = timeoutInterval;
    
    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

const KURL& ResourceRequestBase::firstPartyForCookies() const
{
    updateResourceRequest(); 
    
    return m_firstPartyForCookies;
}

void ResourceRequestBase::setFirstPartyForCookies(const KURL& firstPartyForCookies)
{ 
    updateResourceRequest(); 

    if (m_firstPartyForCookies == firstPartyForCookies)
        return;

    m_firstPartyForCookies = firstPartyForCookies;
    
    m_platformRequestUpdated = false;
}

const String& ResourceRequestBase::httpMethod() const
{
    updateResourceRequest(); 
    
    return m_httpMethod; 
}

void ResourceRequestBase::setHTTPMethod(const String& httpMethod) 
{
    updateResourceRequest(); 

    if (m_httpMethod == httpMethod)
        return;

    m_httpMethod = httpMethod;
    
    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

const HTTPHeaderMap& ResourceRequestBase::httpHeaderFields() const
{
    updateResourceRequest(); 

    return m_httpHeaderFields; 
}

String ResourceRequestBase::httpHeaderField(const AtomicString& name) const
{
    updateResourceRequest(); 
    
    return m_httpHeaderFields.get(name);
}

String ResourceRequestBase::httpHeaderField(const char* name) const
{
    updateResourceRequest(); 
    
    return m_httpHeaderFields.get(name);
}

void ResourceRequestBase::setHTTPHeaderField(const AtomicString& name, const String& value)
{
    updateResourceRequest(); 
    
    m_httpHeaderFields.set(name, value); 
    
    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

void ResourceRequestBase::setHTTPHeaderField(const char* name, const String& value)
{
    setHTTPHeaderField(AtomicString(name), value);
}

void ResourceRequestBase::clearHTTPAuthorization()
{
    updateResourceRequest(); 

    HTTPHeaderMap::iterator iter = m_httpHeaderFields.find("Authorization");
    if (iter == m_httpHeaderFields.end())
        return;

    m_httpHeaderFields.remove(iter);

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

void ResourceRequestBase::clearHTTPContentType()
{
    updateResourceRequest(); 

    m_httpHeaderFields.remove("Content-Type");

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

void ResourceRequestBase::clearHTTPReferrer()
{
    updateResourceRequest(); 

    m_httpHeaderFields.remove("Referer");

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

void ResourceRequestBase::clearHTTPOrigin()
{
    updateResourceRequest(); 

    m_httpHeaderFields.remove("Origin");

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

void ResourceRequestBase::clearHTTPUserAgent()
{
    updateResourceRequest(); 

    m_httpHeaderFields.remove("User-Agent");

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

void ResourceRequestBase::clearHTTPAccept()
{
    updateResourceRequest(); 

    m_httpHeaderFields.remove("Accept");

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

void ResourceRequestBase::setResponseContentDispositionEncodingFallbackArray(const String& encoding1, const String& encoding2, const String& encoding3)
{
    updateResourceRequest(); 
    
    m_responseContentDispositionEncodingFallbackArray.clear();
    m_responseContentDispositionEncodingFallbackArray.reserveInitialCapacity(!encoding1.isNull() + !encoding2.isNull() + !encoding3.isNull());
    if (!encoding1.isNull())
        m_responseContentDispositionEncodingFallbackArray.uncheckedAppend(encoding1);
    if (!encoding2.isNull())
        m_responseContentDispositionEncodingFallbackArray.uncheckedAppend(encoding2);
    if (!encoding3.isNull())
        m_responseContentDispositionEncodingFallbackArray.uncheckedAppend(encoding3);
    
    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

FormData* ResourceRequestBase::httpBody() const
{
    updateResourceRequest(UpdateHTTPBody);

    return m_httpBody.get();
}

void ResourceRequestBase::setHTTPBody(PassRefPtr<FormData> httpBody)
{
    updateResourceRequest();

    m_httpBody = httpBody;

    m_resourceRequestBodyUpdated = true;

    if (url().protocolIsInHTTPFamily())
        m_platformRequestBodyUpdated = false;
}

bool ResourceRequestBase::allowCookies() const
{
    updateResourceRequest(); 
    
    return m_allowCookies;
}

void ResourceRequestBase::setAllowCookies(bool allowCookies)
{
    updateResourceRequest(); 

    if (m_allowCookies == allowCookies)
        return;

    m_allowCookies = allowCookies;
    
    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

ResourceLoadPriority ResourceRequestBase::priority() const
{
    updateResourceRequest();

    return m_priority;
}

void ResourceRequestBase::setPriority(ResourceLoadPriority priority)
{
    updateResourceRequest();

    if (m_priority == priority)
        return;

    m_priority = priority;

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

void ResourceRequestBase::addHTTPHeaderField(const AtomicString& name, const String& value) 
{
    updateResourceRequest();
    HTTPHeaderMap::AddResult result = m_httpHeaderFields.add(name, value);
    if (!result.isNewEntry)
        result.iterator->value = result.iterator->value + ',' + value;

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

void ResourceRequestBase::addHTTPHeaderFields(const HTTPHeaderMap& headerFields)
{
    HTTPHeaderMap::const_iterator end = headerFields.end();
    for (HTTPHeaderMap::const_iterator it = headerFields.begin(); it != end; ++it)
        addHTTPHeaderField(it->key, it->value);
}

bool equalIgnoringHeaderFields(const ResourceRequestBase& a, const ResourceRequestBase& b)
{
    if (a.url() != b.url())
        return false;
    
    if (a.cachePolicy() != b.cachePolicy())
        return false;
    
    if (a.timeoutInterval() != b.timeoutInterval())
        return false;
    
    if (a.firstPartyForCookies() != b.firstPartyForCookies())
        return false;
    
    if (a.httpMethod() != b.httpMethod())
        return false;
    
    if (a.allowCookies() != b.allowCookies())
        return false;
    
    if (a.priority() != b.priority())
        return false;

    FormData* formDataA = a.httpBody();
    FormData* formDataB = b.httpBody();
    
    if (!formDataA)
        return !formDataB;
    if (!formDataB)
        return !formDataA;
    
    if (*formDataA != *formDataB)
        return false;
    
    return true;
}

bool ResourceRequestBase::compare(const ResourceRequest& a, const ResourceRequest& b)
{
    if (!equalIgnoringHeaderFields(a, b))
        return false;
    
    if (a.httpHeaderFields() != b.httpHeaderFields())
        return false;
        
    return ResourceRequest::platformCompare(a, b);
}

bool ResourceRequestBase::isConditional() const
{
    return (m_httpHeaderFields.contains("If-Match") ||
            m_httpHeaderFields.contains("If-Modified-Since") ||
            m_httpHeaderFields.contains("If-None-Match") ||
            m_httpHeaderFields.contains("If-Range") ||
            m_httpHeaderFields.contains("If-Unmodified-Since"));
}

void ResourceRequestBase::makeUnconditional()
{
    m_httpHeaderFields.remove("If-Match");
    m_httpHeaderFields.remove("If-Modified-Since");
    m_httpHeaderFields.remove("If-None-Match");
    m_httpHeaderFields.remove("If-Range");
    m_httpHeaderFields.remove("If-Unmodified-Since");
}

double ResourceRequestBase::defaultTimeoutInterval()
{
    return s_defaultTimeoutInterval;
}

void ResourceRequestBase::setDefaultTimeoutInterval(double timeoutInterval)
{
    s_defaultTimeoutInterval = timeoutInterval;
}

void ResourceRequestBase::updatePlatformRequest(HTTPBodyUpdatePolicy bodyPolicy) const
{
    if (!m_platformRequestUpdated) {
        ASSERT(m_resourceRequestUpdated);
        const_cast<ResourceRequest&>(asResourceRequest()).doUpdatePlatformRequest();
        m_platformRequestUpdated = true;
    }

    if (!m_platformRequestBodyUpdated && bodyPolicy == UpdateHTTPBody) {
        ASSERT(m_resourceRequestBodyUpdated);
        const_cast<ResourceRequest&>(asResourceRequest()).doUpdatePlatformHTTPBody();
        m_platformRequestBodyUpdated = true;
    }
}

void ResourceRequestBase::updateResourceRequest(HTTPBodyUpdatePolicy bodyPolicy) const
{
    if (!m_resourceRequestUpdated) {
        ASSERT(m_platformRequestUpdated);
        const_cast<ResourceRequest&>(asResourceRequest()).doUpdateResourceRequest();
        m_resourceRequestUpdated = true;
    }

    if (!m_resourceRequestBodyUpdated && bodyPolicy == UpdateHTTPBody) {
        ASSERT(m_platformRequestBodyUpdated);
        const_cast<ResourceRequest&>(asResourceRequest()).doUpdateResourceHTTPBody();
        m_resourceRequestBodyUpdated = true;
    }
}

#if !PLATFORM(MAC) && !USE(CFNETWORK) && !USE(SOUP) && !PLATFORM(QT) && !PLATFORM(BLACKBERRY)
unsigned initializeMaximumHTTPConnectionCountPerHost()
{
    // This is used by the loader to control the number of issued parallel load requests. 
    // Four seems to be a common default in HTTP frameworks.
    return 4;
}
#endif

}
