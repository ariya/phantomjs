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
 * @file    ewk_context_menu_item.h
 * @brief   Describes the Ewk Context Menu Item API.
 */

#ifndef ewk_context_menu_item_h
#define ewk_context_menu_item_h

#include "ewk_defines.h"
#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \enum    Ewk_Context_Menu_Item_Type
 * @brief   Defines the types of the items for the context menu.
 * @info    Keep this in sync with WKContextMenuItemTypes.h
 */
typedef enum {
    EWK_ACTION_TYPE,
    EWK_CHECKABLE_ACTION_TYPE,
    EWK_SEPARATOR_TYPE,
    EWK_SUBMENU_TYPE
} Ewk_Context_Menu_Item_Type;

/**
 * \enum    Ewk_Context_Menu_Action
 * @brief   Provides the actions of items for the context menu.
 */
typedef enum {
    EWK_CONTEXT_MENU_ITEM_TAG_NO_ACTION,
    EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK_IN_NEW_WINDOW,
    EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_LINK_TO_DISK,
    EWK_CONTEXT_MENU_ITEM_TAG_COPY_LINK_TO_CLIPBOARD,
    EWK_CONTEXT_MENU_ITEM_TAG_OPEN_IMAGE_IN_NEW_WINDOW,
    EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_IMAGE_TO_DISK,
    EWK_CONTEXT_MENU_ITEM_TAG_COPY_IMAGE_TO_CLIPBOARD,
    EWK_CONTEXT_MENU_ITEM_TAG_COPY_IMAGE_URL_TO_CLIPBOARD,
    EWK_CONTEXT_MENU_ITEM_TAG_OPEN_FRAME_IN_NEW_WINDOW,
    EWK_CONTEXT_MENU_ITEM_TAG_COPY,
    EWK_CONTEXT_MENU_ITEM_TAG_GO_BACK,
    EWK_CONTEXT_MENU_ITEM_TAG_GO_FORWARD,
    EWK_CONTEXT_MENU_ITEM_TAG_STOP,
    EWK_CONTEXT_MENU_ITEM_TAG_RELOAD,
    EWK_CONTEXT_MENU_ITEM_TAG_CUT,
    EWK_CONTEXT_MENU_ITEM_TAG_PASTE,
    EWK_CONTEXT_MENU_ITEM_TAG_SELECT_ALL,
    EWK_CONTEXT_MENU_ITEM_TAG_SPELLING_GUESS,
    EWK_CONTEXT_MENU_ITEM_TAG_NO_GUESSES_FOUND,
    EWK_CONTEXT_MENU_ITEM_TAG_IGNORE_SPELLING,
    EWK_CONTEXT_MENU_ITEM_TAG_LEARN_SPELLING,
    EWK_CONTEXT_MENU_ITEM_TAG_OTHER,
    EWK_CONTEXT_MENU_ITEM_TAG_SEARCH_IN_SPOTLIGHT,
    EWK_CONTEXT_MENU_ITEM_TAG_SEARCH_WEB,
    EWK_CONTEXT_MENU_ITEM_TAG_LOOK_UP_IN_DICTIONARY,
    EWK_CONTEXT_MENU_ITEM_TAG_OPEN_WITH_DEFAULT_APPLICATION,
    EWK_CONTEXT_MENU_ITEM_PDFACTUAL_SIZE,
    EWK_CONTEXT_MENU_ITEM_PDFZOOM_IN,
    EWK_CONTEXT_MENU_ITEM_PDFZOOM_OUT,
    EWK_CONTEXT_MENU_ITEM_PDFAUTO_SIZE,
    EWK_CONTEXT_MENU_ITEM_PDFSINGLE_PAGE,
    EWK_CONTEXT_MENU_ITEM_PDFFACING_PAGES,
    EWK_CONTEXT_MENU_ITEM_PDFCONTINUOUS,
    EWK_CONTEXT_MENU_ITEM_PDFNEXT_PAGE,
    EWK_CONTEXT_MENU_ITEM_PDFPREVIOUS_PAGE,
    EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK = 2000,
    EWK_CONTEXT_MENU_ITEM_TAG_IGNORE_GRAMMAR,
    EWK_CONTEXT_MENU_ITEM_TAG_SPELLING_MENU, /**< spelling or spelling/grammar sub-menu */
    EWK_CONTEXT_MENU_ITEM_TAG_SHOW_SPELLING_PANEL,
    EWK_CONTEXT_MENU_ITEM_TAG_CHECK_SPELLING,
    EWK_CONTEXT_MENU_ITEM_TAG_CHECK_SPELLING_WHILE_TYPING,
    EWK_CONTEXT_MENU_ITEM_TAG_CHECK_GRAMMAR_WITH_SPELLING,
    EWK_CONTEXT_MENU_ITEM_TAG_FONT_MENU, /**< font sub-menu */
    EWK_CONTEXT_MENU_ITEM_TAG_SHOW_FONTS,
    EWK_CONTEXT_MENU_ITEM_TAG_BOLD,
    EWK_CONTEXT_MENU_ITEM_TAG_ITALIC,
    EWK_CONTEXT_MENU_ITEM_TAG_UNDERLINE,
    EWK_CONTEXT_MENU_ITEM_TAG_OUTLINE,
    EWK_CONTEXT_MENU_ITEM_TAG_STYLES,
    EWK_CONTEXT_MENU_ITEM_TAG_SHOW_COLORS,
    EWK_CONTEXT_MENU_ITEM_TAG_SPEECH_MENU, /**< speech sub-menu */
    EWK_CONTEXT_MENU_ITEM_TAG_START_SPEAKING,
    EWK_CONTEXT_MENU_ITEM_TAG_STOP_SPEAKING,
    EWK_CONTEXT_MENU_ITEM_TAG_WRITING_DIRECTION_MENU, /**< writing direction sub-menu */
    EWK_CONTEXT_MENU_ITEM_TAG_DEFAULT_DIRECTION,
    EWK_CONTEXT_MENU_ITEM_TAG_LEFT_TO_RIGHT,
    EWK_CONTEXT_MENU_ITEM_TAG_RIGHT_TO_LEFT,
    EWK_CONTEXT_MENU_ITEM_TAG_PDFSINGLE_PAGE_SCROLLING,
    EWK_CONTEXT_MENU_ITEM_TAG_PDFFACING_PAGES_SCROLLING,
    EWK_CONTEXT_MENU_ITEM_TAG_INSPECT_ELEMENT,
    EWK_CONTEXT_MENU_ITEM_TAG_TEXT_DIRECTION_MENU, /**< text direction sub-menu */
    EWK_CONTEXT_MENU_ITEM_TAG_TEXT_DIRECTION_DEFAULT,
    EWK_CONTEXT_MENU_ITEM_TAG_TEXT_DIRECTION_LEFT_TO_RIGHT,
    EWK_CONTEXT_MENU_ITEM_TAG_TEXT_DIRECTION_RIGHT_TO_LEFT,
    EWK_CONTEXT_MENU_ITEM_OPEN_MEDIA_IN_NEW_WINDOW,
    EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_MEDIA_TO_DISK, 
    EWK_CONTEXT_MENU_ITEM_TAG_COPY_MEDIA_LINK_TO_CLIPBOARD,
    EWK_CONTEXT_MENU_ITEM_TAG_TOGGLE_MEDIA_CONTROLS,
    EWK_CONTEXT_MENU_ITEM_TAG_TOGGLE_MEDIA_LOOP,
    EWK_CONTEXT_MENU_ITEM_TAG_ENTER_VIDEO_FULLSCREEN,
    EWK_CONTEXT_MENU_ITEM_TAG_MEDIA_PLAY_PAUSE,
    EWK_CONTEXT_MENU_ITEM_TAG_MEDIA_MUTE,
    EWK_CONTEXT_MENU_ITEM_BASE_APPLICATION_TAG = 10000
} Ewk_Context_Menu_Item_Action;

/**
 * Creates a new item of the context menu.
 *
 * @param type specifies a type of the item
 * @param action specifies a action of the item
 * @param title specifies a title of the item
 * @param checked @c EINA_TRUE if the item should be toggled or @c EINA_FALSE if not
 * @param enabled @c EINA_TRUE to enable the item or @c EINA_FALSE to disable
 * @return the pointer to the new item
 *
 * @see ewk_context_menu_item_new_with_submenu
 */
EAPI Ewk_Context_Menu_Item *ewk_context_menu_item_new(Ewk_Context_Menu_Item_Type type, Ewk_Context_Menu_Item_Action action, const char *title, Eina_Bool checked, Eina_Bool enabled);

/**
 * Creates a new sub menu type item of the context menu.
 *
 * @param action specifies a action of the item
 * @param title specifies a title of the item
 * @param enabled @c EINA_TRUE to enable the item or @c EINA_FALSE to disable
 * @param submenu specifies a submenu of the item
 * @return the pointer to the new item
 *
 * @see ewk_context_menu_item_new
 */
EAPI Ewk_Context_Menu_Item *ewk_context_menu_item_new_with_submenu(Ewk_Context_Menu_Item_Action action, const char *title, Eina_Bool enabled, Ewk_Context_Menu *submenu);

/**
 * Gets type of the item.
 *
 * @param o the item to get the type
 * @return type of the item on success or @c EWK_ACTION_TYPE on failure
 *
 * @see ewk_context_menu_item_type_set
 */
EAPI Ewk_Context_Menu_Item_Type ewk_context_menu_item_type_get(const Ewk_Context_Menu_Item *o);

/**
 * Sets the type of item.
 *
 * @param o the item to set the type
 * @param type a new type for the item object
 * @return @c EINA_TRUE on success, or @c EINA_FALSE on failure
 *
 * @see ewk_context_menu_item_type_get
 */
EAPI Eina_Bool ewk_context_menu_item_type_set(Ewk_Context_Menu_Item *o, Ewk_Context_Menu_Item_Type type);

/**
 * Gets an action of the item.
 *
 * @param o the item to get the action
 * @return an action of the item on success or @c EWK_CONTEXT_MENU_ITEM_TAG_NO_ACTION on failure
 *
 * @see ewk_context_menu_item_action_set
 */
EAPI Ewk_Context_Menu_Item_Action ewk_context_menu_item_action_get(const Ewk_Context_Menu_Item *o);

/**
 * Sets an action of the item.
 *
 * @param o the item to set the action
 * @param action a new action for the item object
 * @return @c EINA_TRUE on success, or @c EINA_FALSE on failure
 *
 * @see ewk_context_menu_item_action_get
 */
EAPI Eina_Bool ewk_context_menu_item_action_set(Ewk_Context_Menu_Item *o, Ewk_Context_Menu_Item_Action action);

/**
 * Gets a title of the item.
 *
 * @param o the item to get the title
 * @return a title of the item on success, or @c NULL on failure
 *
 * @see ewk_context_menu_item_title_set
 */
EAPI const char *ewk_context_menu_item_title_get(const Ewk_Context_Menu_Item *o);

/**
 * Sets a title of the item.
 *
 * @param o the item to set the title
 * @param title a new title for the item object
 * @return @c EINA_TRUE on success, or @c EINA_FALSE on failure
 *
 * @see ewk_context_menu_item_title_get
 */
EAPI Eina_Bool ewk_context_menu_item_title_set(Ewk_Context_Menu_Item *o, const char *title);

/**
 * Queries if the item is toggled.
 *
 * @param o the item to query if the item is toggled
 * @return @c EINA_TRUE if the item is toggled or @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_context_menu_item_checked_get(const Ewk_Context_Menu_Item *o);

/**
 * Sets if the item should be toggled.
 *
 * @param o the item to be toggled
 * @param checked @c EINA_TRUE if the item should be toggled or @c EINA_FALSE if not
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_menu_item_checked_set(Ewk_Context_Menu_Item *o, Eina_Bool checked);

/**
 * Gets if the item is enabled.
 *
 * @param o the item to get enabled state
 * @return @c EINA_TRUE if it's enabled, @c EINA_FALSE if not or on failure
 *
 * @see ewk_context_menu_item_enabled_set
 */
EAPI Eina_Bool ewk_context_menu_item_enabled_get(const Ewk_Context_Menu_Item *o);

/**
 * Enables/disables the item.
 *
 * @param o the item to enable/disable
 * @param enabled @c EINA_TRUE to enable the item or @c EINA_FALSE to disable
 * @return @c EINA_TRUE on success, or @c EINA_FALSE on failure
 *
 * @see ewk_context_menu_item_enabled_get
 */
EAPI Eina_Bool ewk_context_menu_item_enabled_set(Ewk_Context_Menu_Item *o, Eina_Bool enabled);

/**
 * Gets the parent menu for the item.
 *
 * @param o item to get the parent
 *
 * @return the pointer to parent menu on success or @c NULL on failure
 */
EAPI Ewk_Context_Menu *ewk_context_menu_item_parent_menu_get(const Ewk_Context_Menu_Item *o);

/**
 * Gets the submenu for the item.
 *
 * @param o item to get the submenu
 *
 * @return the pointer to submenu on success or @c NULL on failure
 */
EAPI Ewk_Context_Menu *ewk_context_menu_item_submenu_get(const Ewk_Context_Menu_Item *o);

#ifdef __cplusplus
}
#endif

#endif /* ewk_context_menu_item_h */
