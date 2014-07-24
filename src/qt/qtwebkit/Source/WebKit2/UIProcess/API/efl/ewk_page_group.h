/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
 * @file    ewk_page_group.h
 * @brief   Describes the Ewk Page Group API.
 */

#ifndef ewk_page_group_h
#define ewk_page_group_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Declare Ewk_Page_Group as Ewk_Object.
 *
 * @see Ewk_Object
 */
typedef struct EwkObject Ewk_Page_Group;

/**
 * Creates a new Ewk_Page_Group.
 *
 * The returned Ewk_Page_Group object @b should be unref'ed after use.
 *
 * @return Ewk_Page_Group object on success or @c NULL on failure
 *
 * @see ewk_object_unref
 */
EAPI Ewk_Page_Group *ewk_page_group_create(const char *identifier);

/**
 * Adds the user style sheet to this page group.
 *
 * @param page_group ewk_page_gorup object to add the user style sheet
 * @param source the user style sheet
 * @param url baseURL
 * @param white_list url list to allow adding the user style sheet
 * @param black_list url list to disallow adding the user style sheet
 * @param main_frame_only a state to apply the user style sheet only to the mainframe
 *
 * @return @c EINA_TRUE if the user style sheet is added to the page group, or
 *         @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_page_group_user_style_sheet_add(Ewk_Page_Group *page_group, const char *source, const char *base_url, Eina_List *white_list, Eina_List *black_list, Eina_Bool main_frame_only);

/**
 * Remove all the user style sheets from this page group.
 *
 * @param page_group page group object to remove all the user style sheets
 *
 * @return @c EINA_TRUE if all the user style sheet are removed from the page group, or
 *         @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_page_group_user_style_sheets_remove_all(Ewk_Page_Group *page_group);

#ifdef __cplusplus
}
#endif

#endif // ewk_page_group_h
