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
 * @file    ewk_file_chooser_request.h
 * @brief   Describes the Ewk File Chooser API.
 */

#ifndef ewk_file_chooser_request_h
#define ewk_file_chooser_request_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Declare Ewk_File_Chooser_Request as Ewk_Object.
 *
 * @see Ewk_Object
 */
typedef struct EwkObject Ewk_File_Chooser_Request;

/**
 * Queries if it is allowed to select multiple files or not.
 *
 * @param request request object to query
 *
 * @return @c EINA_TRUE if it is allowed to select multiple files,
 *         @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_file_chooser_request_allow_multiple_files_get(const Ewk_File_Chooser_Request *request);

/**
 * Queries the list of accepted MIME types.
 *
 * Possible MIME types are:
 * - "audio\/\*": All sound files are accepted
 * - "video\/\*": All video files are accepted
 * - "image\/\*": All image files are accepted
 * - standard IANA MIME type (see http://www.iana.org/assignments/media-types/ for a complete list)
 *
 * @param request request object to query
 *
 * @return The list of accepted MIME types. The list items are guaranteed to be stringshared.
 * The caller needs to free the list and its items after use
 */
EAPI Eina_List *ewk_file_chooser_request_accepted_mimetypes_get(const Ewk_File_Chooser_Request *request);

/**
 * Cancels the file chooser request.
 *
 * @param request request object to cancel
 *
 * @return @c EINA_TRUE if successful, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_file_chooser_request_cancel(Ewk_File_Chooser_Request *request);

/**
 * Sets the files chosen by the user.
 *
 * @param request request object to update
 *
 * @return @c EINA_TRUE if successful, @c EINA_FALSE otherwise
 *
 * @see ewk_file_chooser_request_file_choose()
 */
EAPI Eina_Bool ewk_file_chooser_request_files_choose(Ewk_File_Chooser_Request *request, const Eina_List *chosen_files);

/**
 * Sets the file chosen by the user.
 *
 * This is a convenience function in case only one file needs to be set.
 *
 * @param request request object to update
 *
 * @return @c EINA_TRUE if successful, @c EINA_FALSE otherwise
 *
 * @see ewk_file_chooser_request_files_choose()
 */
EAPI Eina_Bool ewk_file_chooser_request_file_choose(Ewk_File_Chooser_Request *request, const char *chosen_file);

#ifdef __cplusplus
}
#endif

#endif /* ewk_file_chooser_request_h */
