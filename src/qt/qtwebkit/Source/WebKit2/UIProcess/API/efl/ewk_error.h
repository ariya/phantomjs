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
 * @file    ewk_error.h
 * @brief   Describes the Web Error API.
 */

#ifndef ewk_error_h
#define ewk_error_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Creates a type name for @a Ewk_Error. */
typedef struct EwkError Ewk_Error;

/// Creates a type name for Ewk_Error_Type.
typedef enum {
    EWK_ERROR_TYPE_NONE,
    EWK_ERROR_TYPE_INTERNAL,
    EWK_ERROR_TYPE_NETWORK,
    EWK_ERROR_TYPE_POLICY,
    EWK_ERROR_TYPE_PLUGIN,
    EWK_ERROR_TYPE_DOWNLOAD,
    EWK_ERROR_TYPE_PRINT
} Ewk_Error_Type;

/**
 * Query type for this error.
 *
 * @param error error object to query.
 *
 * @return the error type, that may be @c NULL. This pointer is
 *         guaranteed to be eina_stringshare, so whenever possible
 *         save yourself some cpu cycles and use
 *         eina_stringshare_ref() instead of eina_stringshare_add() or
 *         strdup().
 */
EAPI Ewk_Error_Type ewk_error_type_get(const Ewk_Error *error);

/**
 * Query failing URL for this error.
 *
 * URL that failed loading.
 *
 * @param error error object to query.
 *
 * @return the URL pointer, that may be @c NULL. This pointer is
 *         guaranteed to be eina_stringshare, so whenever possible
 *         save yourself some cpu cycles and use
 *         eina_stringshare_ref() instead of eina_stringshare_add() or
 *         strdup().
 */
EAPI const char *ewk_error_url_get(const Ewk_Error *error);

/**
 * Query HTTP error code.
 *
 * @param error error object to query.
 *
 * @return the HTTP error code.
 */
EAPI int ewk_error_code_get(const Ewk_Error *error);

/**
 * Query description for this error.
 *
 * @param error error object to query.
 *
 * @return the description pointer, that may be @c NULL. This pointer is
 *         guaranteed to be eina_stringshare, so whenever possible
 *         save yourself some cpu cycles and use
 *         eina_stringshare_ref() instead of eina_stringshare_add() or
 *         strdup().
 */
EAPI const char *ewk_error_description_get(const Ewk_Error *error);

/**
 * Query if error should be treated as a cancellation.
 *
 * @param error error object to query.
 *
 * @return @c EINA_TRUE if this error should be treated as a cancellation, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_error_cancellation_get(const Ewk_Error *error);

#ifdef __cplusplus
}
#endif

#endif // ewk_error_h
