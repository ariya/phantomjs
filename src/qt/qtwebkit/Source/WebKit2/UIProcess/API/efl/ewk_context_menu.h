/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
 * @file    ewk_context_menu.h
 * @brief   Describes the Ewk Context Menu API.
 */

#ifndef ewk_context_menu_h
#define ewk_context_menu_h

#include "ewk_defines.h"
#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a new Ewk_Context_Menu to be used as a submenu of an existing
 * Ewk_Context_Menu. The context menu is created by the ewk_view and
 * passed as an argument of ewk_view smart callback.
 *
 * @return the pointer to the new context menu
 *
 * @see ewk_context_menu_new_with_items
 */
EAPI Ewk_Context_Menu *ewk_context_menu_new(void);

/**
 * Creates a new Ewk_Context_Menu to be used as a submenu of an existing
 * Ewk_Context_Menu with the given initial items. The context menu is
 * created by the ewk_view and passed as an argument of ewk_view smart callback.
 *
 * @param items the list of initial items
 * @return the pointer to the new context menu
 *
 * @see ewk_context_menu_new
 */
EAPI Ewk_Context_Menu *ewk_context_menu_new_with_items(Eina_List *items);

/**
 * Appends the item of the context menu.
 *
 * @param menu the context menu 
 * @param item the item to append
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_menu_item_append(Ewk_Context_Menu *menu, Ewk_Context_Menu_Item *item);

/**
 * Removes the item of the context menu.
 *
 * @param menu the context menu 
 * @param item the item to remove
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_menu_item_remove(Ewk_Context_Menu *menu, Ewk_Context_Menu_Item *item);

/**
 * Hides the context menu.
 *
 * @param menu the context menu to hide
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_menu_hide(Ewk_Context_Menu *menu);

/**
 * Gets the list of items.
 *
 * @param o the context menu to get list of the items
 * @return the list of the items on success or @c NULL on failure
 */
EAPI const Eina_List *ewk_context_menu_items_get(const Ewk_Context_Menu *o);

/**
 * Selects the item from the context menu.
 *
 * @param menu the context menu
 * @param item the item is selected
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_menu_item_select(Ewk_Context_Menu *menu, Ewk_Context_Menu_Item *item);

#ifdef __cplusplus
}
#endif

#endif /* ewk_context_menu_h */
