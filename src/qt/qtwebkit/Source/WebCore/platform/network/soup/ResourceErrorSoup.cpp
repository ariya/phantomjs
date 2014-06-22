/*
 * Copyright (C) 2012 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY IGALIA S.L. ``AS IS'' AND ANY
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
#include "ResourceError.h"

#include "LocalizedStrings.h"
#include <libsoup/soup.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

namespace WebCore {

static String failingURI(SoupURI* soupURI)
{
    ASSERT(soupURI);
    GOwnPtr<char> uri(soup_uri_to_string(soupURI, FALSE));
    return uri.get();
}

static String failingURI(SoupRequest* request)
{
    ASSERT(request);
    return failingURI(soup_request_get_uri(request));
}

ResourceError ResourceError::transportError(SoupRequest* request, int statusCode, const String& reasonPhrase)
{
    return ResourceError(g_quark_to_string(SOUP_HTTP_ERROR), statusCode,
        failingURI(request), reasonPhrase);
}

ResourceError ResourceError::httpError(SoupMessage* message, GError* error, SoupRequest* request)
{
    if (message && SOUP_STATUS_IS_TRANSPORT_ERROR(message->status_code))
        return transportError(request, message->status_code,
            String::fromUTF8(message->reason_phrase));
    else
        return genericGError(error, request);
}

ResourceError ResourceError::authenticationError(SoupMessage* message)
{
    ASSERT(message);
    return ResourceError(g_quark_to_string(SOUP_HTTP_ERROR), message->status_code,
        failingURI(soup_message_get_uri(message)), String::fromUTF8(message->reason_phrase));
}

ResourceError ResourceError::genericGError(GError* error, SoupRequest* request)
{
    return ResourceError(g_quark_to_string(error->domain), error->code,
        failingURI(request), String::fromUTF8(error->message));
}

ResourceError ResourceError::tlsError(SoupRequest* request, unsigned tlsErrors, GTlsCertificate* certificate)
{
    ResourceError resourceError(g_quark_to_string(SOUP_HTTP_ERROR), SOUP_STATUS_SSL_FAILED,
        failingURI(request), unacceptableTLSCertificate());
    resourceError.setTLSErrors(tlsErrors);
    resourceError.setCertificate(certificate);
    return resourceError;
}

ResourceError ResourceError::timeoutError(const String& failingURL)
{
    // FIXME: This should probably either be integrated into Errors(Gtk/EFL).h or the
    // networking errors from those files should be moved here.

    // Use the same value as in NSURLError.h
    static const int timeoutError = -1001;
    static const char* const  errorDomain = "WebKitNetworkError";
    ResourceError error = ResourceError(errorDomain, timeoutError, failingURL, "Request timed out");
    error.setIsTimeout(true);
    return error;
}

void ResourceError::platformCopy(ResourceError& errorCopy) const
{
    errorCopy.m_certificate = m_certificate;
    errorCopy.m_tlsErrors = m_tlsErrors;
}

bool ResourceError::platformCompare(const ResourceError& a, const ResourceError& b)
{
    return a.tlsErrors() == b.tlsErrors();
}

} // namespace WebCore
