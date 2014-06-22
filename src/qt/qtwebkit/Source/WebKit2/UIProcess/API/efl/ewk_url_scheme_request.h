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
 * @file    ewk_url_scheme_request.h
 * @brief   Describes the Ewk URL scheme request API.
 */

#ifndef ewk_url_scheme_request_h
#define ewk_url_scheme_request_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Declare Ewk_Url_Scheme_Request as Ewk_Object.
 *
 * @see Ewk_Object
 */
typedef struct EwkObject Ewk_Url_Scheme_Request;

/**
 * Query the URL scheme for this request.
 *
 * @param request request object to query.
 *
 * @return the URL scheme pointer, that may be @c NULL. This pointer is
 *         guaranteed to be eina_stringshare, so whenever possible
 *         save yourself some cpu cycles and use
 *         eina_stringshare_ref() instead of eina_stringshare_add() or
 *         strdup().
 */
EAPI const char *ewk_url_scheme_request_scheme_get(const Ewk_Url_Scheme_Request *request);

/**
 * Query the URL for this request.
 *
 * @param request request object to query.
 *
 * @return the URL pointer, that may be @c NULL. This pointer is
 *         guaranteed to be eina_stringshare, so whenever possible
 *         save yourself some cpu cycles and use
 *         eina_stringshare_ref() instead of eina_stringshare_add() or
 *         strdup().
 */
EAPI const char *ewk_url_scheme_request_url_get(const Ewk_Url_Scheme_Request *request);

/**
 * Query the path part of the URL for this request.
 *
 * @param request request object to query.
 *
 * @return the path pointer, that may be @c NULL. This pointer is
 *         guaranteed to be eina_stringshare, so whenever possible
 *         save yourself some cpu cycles and use
 *         eina_stringshare_ref() instead of eina_stringshare_add() or
 *         strdup().
 */
EAPI const char *ewk_url_scheme_request_path_get(const Ewk_Url_Scheme_Request *request);

/**
 * Finish a Ewk_Url_Scheme_Request by setting the content of the request and its mime type.
 *
 * @param request a Ewk_Url_Scheme_Request.
 * @param content_data the data content of the request (may be %c NULL if the content is empty).
 * @param content_length the length of the @a content_data.
 * @param mime_type the content type of the stream or %c NULL if not known
 */
EAPI Eina_Bool ewk_url_scheme_request_finish(Ewk_Url_Scheme_Request *request, const void *content_data, uint64_t content_length, const char *mime_type);

#ifdef __cplusplus
}
#endif

#endif // ewk_url_scheme_request_h
