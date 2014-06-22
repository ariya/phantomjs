/*
 * Copyright (C) 2009 Gustavo Noronha Silva
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "ResourceRequest.h"

#include "GOwnPtrSoup.h"
#include "HTTPParsers.h"
#include "MIMETypeRegistry.h"
#include "SoupURIUtils.h"
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

using namespace std;

namespace WebCore {

void ResourceRequest::updateSoupMessageHeaders(SoupMessageHeaders* soupHeaders) const
{
    const HTTPHeaderMap& headers = httpHeaderFields();
    if (!headers.isEmpty()) {
        HTTPHeaderMap::const_iterator end = headers.end();
        for (HTTPHeaderMap::const_iterator it = headers.begin(); it != end; ++it)
            soup_message_headers_append(soupHeaders, it->key.string().utf8().data(), it->value.utf8().data());
    }
}

void ResourceRequest::updateFromSoupMessageHeaders(SoupMessageHeaders* soupHeaders)
{
    m_httpHeaderFields.clear();
    SoupMessageHeadersIter headersIter;
    soup_message_headers_iter_init(&headersIter, soupHeaders);
    const char* headerName;
    const char* headerValue;
    while (soup_message_headers_iter_next(&headersIter, &headerName, &headerValue))
        m_httpHeaderFields.set(String::fromUTF8(headerName), String::fromUTF8(headerValue));
}

void ResourceRequest::updateSoupMessage(SoupMessage* soupMessage) const
{
    g_object_set(soupMessage, SOUP_MESSAGE_METHOD, httpMethod().utf8().data(), NULL);

    GOwnPtr<SoupURI> uri(soupURI());
    soup_message_set_uri(soupMessage, uri.get());

    updateSoupMessageHeaders(soupMessage->request_headers);

    String firstPartyString = firstPartyForCookies().string();
    if (!firstPartyString.isEmpty()) {
        GOwnPtr<SoupURI> firstParty(soup_uri_new(firstPartyString.utf8().data()));
        soup_message_set_first_party(soupMessage, firstParty.get());
    }

    soup_message_set_flags(soupMessage, m_soupFlags);
}

SoupMessage* ResourceRequest::toSoupMessage() const
{
    SoupMessage* soupMessage = soup_message_new(httpMethod().utf8().data(), url().string().utf8().data());
    if (!soupMessage)
        return 0;

    updateSoupMessageHeaders(soupMessage->request_headers);

    String firstPartyString = firstPartyForCookies().string();
    if (!firstPartyString.isEmpty()) {
        GOwnPtr<SoupURI> firstParty(soup_uri_new(firstPartyString.utf8().data()));
        soup_message_set_first_party(soupMessage, firstParty.get());
    }

    soup_message_set_flags(soupMessage, m_soupFlags);

    // Body data is only handled at ResourceHandleSoup::startHttp for
    // now; this is because this may not be a good place to go
    // openning and mmapping files. We should maybe revisit this.
    return soupMessage;
}

void ResourceRequest::updateFromSoupMessage(SoupMessage* soupMessage)
{
    bool shouldPortBeResetToZero = m_url.hasPort() && !m_url.port();
    m_url = soupURIToKURL(soup_message_get_uri(soupMessage));

    // SoupURI cannot differeniate between an explicitly specified port 0 and
    // no port specified.
    if (shouldPortBeResetToZero)
        m_url.setPort(0);

    m_httpMethod = String::fromUTF8(soupMessage->method);

    updateFromSoupMessageHeaders(soupMessage->request_headers);

    if (soupMessage->request_body->data)
        m_httpBody = FormData::create(soupMessage->request_body->data, soupMessage->request_body->length);

    SoupURI* firstParty = soup_message_get_first_party(soupMessage);
    if (firstParty)
        m_firstPartyForCookies = soupURIToKURL(firstParty);

    m_soupFlags = soup_message_get_flags(soupMessage);

    // FIXME: m_allowCookies should probably be handled here and on
    // doUpdatePlatformRequest somehow.
}

unsigned initializeMaximumHTTPConnectionCountPerHost()
{
    // Soup has its own queue control; it wants to have all requests
    // given to it, so that it is able to look ahead, and schedule
    // them in a good way.
    return 10000;
}

SoupURI* ResourceRequest::soupURI() const
{
    // WebKit does not support fragment identifiers in data URLs, but soup does.
    // Before passing the URL to soup, we should make sure to urlencode any '#'
    // characters, so that soup does not interpret them as fragment identifiers.
    // See http://wkbug.com/68089
    if (m_url.protocolIsData()) {
        String urlString = m_url.string();
        urlString.replace("#", "%23");
        return soup_uri_new(urlString.utf8().data());
    }

    KURL url = m_url;
    url.removeFragmentIdentifier();
    SoupURI* uri = soup_uri_new(url.string().utf8().data());

    // Versions of libsoup prior to 2.42 have a soup_uri_new that will convert empty passwords that are not
    // prefixed by a colon into null. Some parts of soup like the SoupAuthenticationManager will only be active
    // when both the username and password are non-null. When we have credentials, empty usernames and passwords
    // should be empty strings instead of null.
    if (!url.user().isEmpty() || !url.pass().isEmpty()) {
        soup_uri_set_user(uri, url.user().utf8().data());
        soup_uri_set_password(uri, url.pass().utf8().data());
    }
    return uri;
}

}
