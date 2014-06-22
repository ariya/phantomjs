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
 * @file    ewk_form_submission_request.h
 * @brief   Describes the Ewk Form Submission Request API.
 *
 * @note Ewk_Form_Submission_Request provides information regarding
 * a form about the be submitted, in particular its text fields.
 */

#ifndef ewk_form_submission_request_h
#define ewk_form_submission_request_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Declare Ewk_Form_Submission_Request as Ewk_Object.
 *
 * @see Ewk_Object
 */
typedef struct EwkObject Ewk_Form_Submission_Request;

/**
 * Returns the list of field names contained in the form associated to @a request.
 *
 * @param request the request object to query.
 *
 * @return a #Eina_List with the form text fields names, or @c NULL in case of error.
 * The items of the list are guaranteed to be stringshared so use eina_stringshare_add()
 * instead of strdup() to copy them and free them using eina_stringshare_del().
 *
 * @see ewk_form_submission_request_field_value_get()
 */
EAPI Eina_List *ewk_form_submission_request_field_names_get(Ewk_Form_Submission_Request *request);

/**
 * Returns the value of specific field contained in the form associated to @a request.
 *
 * @param request the request object to query.
 * @param name name of the field to query the value for.
 *
 * @return a #Eina_List with the form text fields names, or @c NULL in case of error.
 * The string returned is guaranteed to be stringshared. You need to call
 * eina_stringshare_del() on the returned value once you are done with it.
 *
 * @see ewk_form_submission_request_field_names_get()
 */
EAPI const char *ewk_form_submission_request_field_value_get(Ewk_Form_Submission_Request *request, const char *name);

/**
 * Continues the form request submission.
 *
 * If you don't call this function explicitly, the form request will be submitted
 * upon @a request object destruction.
 *
 * @param request the request object to submit.
 *
 * @return @c EINA_TRUE is if successful, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool ewk_form_submission_request_submit(Ewk_Form_Submission_Request *request);

#ifdef __cplusplus
}
#endif

#endif // ewk_form_submission_request_h
