/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file    ewk_url_response.h
 * @brief   Describes the Ewk URL response API.
 */

#ifndef ewk_url_response_h
#define ewk_url_response_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Declare Ewk_Url_Response as Ewk_Object.
 *
 * @see Ewk_Object
 */
typedef struct EwkObject Ewk_Url_Response;

/**
 * Query URL for this response.
 *
 * @param response response object to query.
 *
 * @return the URL pointer, that may be @c NULL. This pointer is
 *         guaranteed to be eina_stringshare, so whenever possible
 *         save yourself some cpu cycles and use
 *         eina_stringshare_ref() instead of eina_stringshare_add() or
 *         strdup().
 */
EAPI const char *ewk_url_response_url_get(const Ewk_Url_Response *response);

/**
 * Query HTTP status code for this response.
 *
 * HTTP status code are defined by:
 * http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
 *
 * @param response response object to query.
 *
 * @return the HTTP status code.
 */
EAPI int ewk_url_response_status_code_get(const Ewk_Url_Response *response);

/**
 * Query MIME type for this response.
 *
 * @param response response object to query.
 *
 * @return the MIME type pointer, that may be @c NULL. This pointer is
 *         guaranteed to be eina_stringshare, so whenever possible
 *         save yourself some cpu cycles and use
 *         eina_stringshare_ref() instead of eina_stringshare_add() or
 *         strdup().
 */
EAPI const char *ewk_url_response_mime_type_get(const Ewk_Url_Response *response);

/**
 * Get the expected content length of the #Ewk_Url_Response.
 *
 * It can be 0 if the server provided an incorrect or missing Content-Length.
 *
 * @param response a #Ewk_Url_Response.
 *
 * @return the expected content length of @a response or 0 in case of failure.
 */
EAPI unsigned long ewk_url_response_content_length_get(const Ewk_Url_Response *response);

#ifdef __cplusplus
}
#endif

#endif // ewk_url_response_h
