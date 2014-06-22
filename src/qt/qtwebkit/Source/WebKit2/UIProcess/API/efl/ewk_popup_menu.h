/*
 * Copyright (C) 2012 Intel Corporation
 * Copyright (C) 2012 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file    ewk_popup_menu.h
 * @brief   Describes the Ewk Popup Menu API.
 */

#ifndef ewk_popup_menu_h
#define ewk_popup_menu_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Creates a type name for Ewk_Popup_Menu */
typedef struct EwkPopupMenu Ewk_Popup_Menu;

/**
 * Selects index of current popup menu.
 *
 * @param popup_menu popup menu object
 * @param index index of item to select
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (e.g. the index is out of range)
 */
EAPI Eina_Bool ewk_popup_menu_selected_index_set(Ewk_Popup_Menu *popup_menu, unsigned index);

/**
 * Returns the index of the currently selected item in the popup menu.
 *
 * @param popup_menu popup menu object
 *
 * @return index of the currently selected popup item on success, @c 0 otherwise
 */
EAPI unsigned ewk_popup_menu_selected_index_get(const Ewk_Popup_Menu *popup_menu);

/**
 * Closes current popup menu.
 *
 * The Ewk_Popup_Menu object becomes invalid after calling this function.
 *
 * @param popup_menu popup menu object
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_popup_menu_close(Ewk_Popup_Menu *popup_menu);

/**
 * Retrieve the popup menu items
 *
 * @param popup_menu popup menu object
 *
 * @return @c list of popup menu items on success, @c NULL otherwise
 */
EAPI const Eina_List *ewk_popup_menu_items_get(const Ewk_Popup_Menu *popup_menu);

#ifdef __cplusplus
}
#endif

#endif /* ewk_popup_menu_h */
