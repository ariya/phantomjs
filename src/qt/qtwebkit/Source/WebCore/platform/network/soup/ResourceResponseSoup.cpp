/*
 * Copyright (C) 2009 Gustavo Noronha Silva
 * Copyright (C) 2009 Collabora Ltd.
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
#include "ResourceResponse.h"

#include <wtf/gobject/GOwnPtr.h>
#include "HTTPParsers.h"
#include "MIMETypeRegistry.h"
#include "SoupURIUtils.h"
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

using namespace std;

namespace WebCore {

SoupMessage* ResourceResponse::toSoupMessage() const
{
    // This GET here is just because SoupMessage wants it, we dn't really know.
    SoupMessage* soupMessage = soup_message_new("GET", url().string().utf8().data());
    if (!soupMessage)
        return 0;

    soupMessage->status_code = httpStatusCode();

    const HTTPHeaderMap& headers = httpHeaderFields();
    SoupMessageHeaders* soupHeaders = soupMessage->response_headers;
    if (!headers.isEmpty()) {
        HTTPHeaderMap::const_iterator end = headers.end();
        for (HTTPHeaderMap::const_iterator it = headers.begin(); it != end; ++it)
            soup_message_headers_append(soupHeaders, it->key.string().utf8().data(), it->value.utf8().data());
    }

    soup_message_set_flags(soupMessage, m_soupFlags);

    g_object_set(G_OBJECT(soupMessage), "tls-certificate", m_certificate.get(), "tls-errors", m_tlsErrors, NULL);

    // Body data is not in the message.
    return soupMessage;
}

void ResourceResponse::updateFromSoupMessage(SoupMessage* soupMessage)
{
    m_url = soupURIToKURL(soup_message_get_uri(soupMessage));

    m_httpStatusCode = soupMessage->status_code;
    setHTTPStatusText(soupMessage->reason_phrase);

    m_soupFlags = soup_message_get_flags(soupMessage);

    GTlsCertificate* certificate = 0;
    soup_message_get_https_status(soupMessage, &certificate, &m_tlsErrors);
    m_certificate = certificate;

    updateFromSoupMessageHeaders(soupMessage->response_headers);
}

void ResourceResponse::updateFromSoupMessageHeaders(const SoupMessageHeaders* messageHeaders)
{
    SoupMessageHeaders* headers = const_cast<SoupMessageHeaders*>(messageHeaders);
    SoupMessageHeadersIter headersIter;
    const char* headerName;
    const char* headerValue;

    // updateFromSoupMessage could be called several times for the same ResourceResponse object,
    // thus, we need to clear old header values and update m_httpHeaderFields from soupMessage headers.
    m_httpHeaderFields.clear();

    soup_message_headers_iter_init(&headersIter, headers);
    while (soup_message_headers_iter_next(&headersIter, &headerName, &headerValue))
        addHTTPHeaderField(String::fromUTF8WithLatin1Fallback(headerName, strlen(headerName)), String::fromUTF8WithLatin1Fallback(headerValue, strlen(headerValue)));

    String contentType;
    const char* officialType = soup_message_headers_get_one(headers, "Content-Type");
    if (!m_sniffedContentType.isEmpty() && m_sniffedContentType != officialType)
        contentType = m_sniffedContentType;
    else
        contentType = officialType;
    setMimeType(extractMIMETypeFromMediaType(contentType));
    setTextEncodingName(extractCharsetFromMediaType(contentType));

    setExpectedContentLength(soup_message_headers_get_content_length(headers));
    setSuggestedFilename(filenameFromHTTPContentDisposition(httpHeaderField("Content-Disposition")));}

}
