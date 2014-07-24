/*
    Copyright (C) 2010 ProFUSION embedded systems
    Copyright (C) 2010 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

/**
 * @file    ewk_contextmenu.h
 * @brief   Describes the context menu API.
 *
 * The following signals (see evas_object_smart_callback_add()) are emitted:
 *
 *  - "contextmenu,customize", Eina_List *: customize context menu is taken
 *    and it gives a list with items of context menu as an argument.
 *  - "contextmenu,free", Ewk_Context_Menu *: a context menu is freed.
 *  - "contextmenu,item,appended", Ewk_Context_Menu *: a new item was added to
 *    the context menu.
 *  - "contextmenu,new", Ewk_Context_Menu *: a new context menu was created
 *    and it gives the context menu as an argument.
 *  - "contextmenu,show", Ewk_Context_Menu *: a context menu is shown.
 */

#ifndef ewk_contextmenu_h
#define ewk_contextmenu_h

#include <Eina.h>
#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \enum    _Ewk_Context_Menu_Action
 * @brief   Provides the actions of items for the context menu.
 * @info    Keep this in sync with ContextMenuItem.h
 */
enum _Ewk_Context_Menu_Action {
    EWK_CONTEXT_MENU_ITEM_TAG_NO_ACTION = 0, // this item is not actually in web_uidelegate.h
    EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK_IN_NEW_WINDOW = 1,
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
    EWK_CONTEXT_MENU_ITEM_BASE_CUSTOM_TAG = 5000,
    EWK_CONTEXT_MENU_ITEM_CUSTOM_TAG_NO_ACTION = 5998,
    EWK_CONTEXT_MENU_ITEM_LAST_CUSTOM_TAG = 5999,
    EWK_CONTEXT_MENU_ITEM_BASE_APPLICATION_TAG = 10000
};
/** Creates a type name for _Ewk_Context_Menu_Action */
typedef enum _Ewk_Context_Menu_Action Ewk_Context_Menu_Action;

/**
 * \enum    _Ewk_Context_Menu_Item_Type
 * @brief   Defines the types of the items for the context menu.
 * @info    Keep this in sync with ContextMenuItem.h
 */
enum _Ewk_Context_Menu_Item_Type {
    EWK_ACTION_TYPE,
    EWK_CHECKABLE_ACTION_TYPE,
    EWK_SEPARATOR_TYPE,
    EWK_SUBMENU_TYPE
};
/** Creates a type name for _Ewk_Context_Menu_Item_Type */
typedef enum _Ewk_Context_Menu_Item_Type    Ewk_Context_Menu_Item_Type;

/** Creates a type name for _Ewk_Context_Menu */
typedef struct _Ewk_Context_Menu            Ewk_Context_Menu;

/** Creates a type name for _Ewk_Context_Menu_Item */
typedef struct _Ewk_Context_Menu_Item       Ewk_Context_Menu_Item;



/************************** Exported functions ***********************/

/**
 * Increases the reference count of the given object.
 *
 * @param menu the context menu object to increase the reference count
 */
EAPI void                        ewk_context_menu_ref(Ewk_Context_Menu *menu);

/**
 * Decreases the reference count of the given object, possibly freeing it.
 *
 * When the reference count reaches 0, the menu with all its items are freed.
 *
 * @param menu the context menu object to decrease the reference count
 */
EAPI void                        ewk_context_menu_unref(Ewk_Context_Menu *menu);

/**
 * Destroys the context menu object.
 *
 * @param menu the context menu object to destroy
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 *
 * @see ewk_context_menu_item_free
 */
EAPI Eina_Bool                   ewk_context_menu_destroy(Ewk_Context_Menu *menu);

/**
 * Gets the list of items.
 *
 * @param o the context menu object to get list of the items
 * @return the list of the items on success or @c NULL on failure
 */
EAPI const Eina_List            *ewk_context_menu_item_list_get(const Ewk_Context_Menu *o);

/**
 * Creates a new item of the context menu.
 *
 * @param type specifies a type of the item
 * @param action specifies a action of the item
 * @param parent_menu specifies a parent menu of the item
 * @param submenu specifies a submenu of the item
 * @param title specifies a title of the item
 * @param checked @c EINA_TRUE if the item should be toggled or @c EINA_FALSE if not
 * @param enabled @c EINA_TRUE to enable the item or @c EINA_FALSE to disable
 * @return the pointer to the new item on success or @c NULL on failure
 *
 * @note The return value @b should @b be freed after use.
 */
EAPI Ewk_Context_Menu_Item      *ewk_context_menu_item_new(Ewk_Context_Menu_Item_Type type, Ewk_Context_Menu_Action action, Ewk_Context_Menu *parent_menu, Ewk_Context_Menu *submenu, const char *title, Eina_Bool checked, Eina_Bool enabled);

/**
 * Destroys the item of the context menu object.
 *
 * @param item the item to destroy
 *
 * @see ewk_context_menu_destroy
 * @see ewk_context_menu_unref
 */
EAPI void                        ewk_context_menu_item_free(Ewk_Context_Menu_Item *item);

/**
 * Selects the item from the context menu object.
 *
 * @param menu the context menu object
 * @param item the item is selected
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool                   ewk_context_menu_item_select(Ewk_Context_Menu *menu, Ewk_Context_Menu_Item *item);

/**
 * Gets type of the item.
 *
 * @param o the item to get the type
 * @return type of the item on success or @c EWK_ACTION_TYPE on failure
 *
 * @see ewk_context_menu_item_type_set
 */
EAPI Ewk_Context_Menu_Item_Type  ewk_context_menu_item_type_get(const Ewk_Context_Menu_Item *o);

/**
 * Sets the type of item.
 *
 * @param o the item to set the type
 * @param type a new type for the item object
 * @return @c EINA_TRUE on success, or @c EINA_FALSE on failure
 *
 * @see ewk_context_menu_item_type_get
 */
EAPI Eina_Bool                   ewk_context_menu_item_type_set(Ewk_Context_Menu_Item *o, Ewk_Context_Menu_Item_Type type);

/**
 * Gets an action of the item.
 *
 * @param o the item to get the action
 * @return an action of the item on success or @c EWK_CONTEXT_MENU_ITEM_TAG_NO_ACTION on failure
 *
 * @see ewk_context_menu_item_action_set
 */
EAPI Ewk_Context_Menu_Action     ewk_context_menu_item_action_get(const Ewk_Context_Menu_Item *o);

/**
 * Sets an action of the item.
 *
 * @param o the item to set the action
 * @param action a new action for the item object
 * @return @c EINA_TRUE on success, or @c EINA_FALSE on failure
 *
 * @see ewk_context_menu_item_action_get
 */
EAPI Eina_Bool                   ewk_context_menu_item_action_set(Ewk_Context_Menu_Item *o, Ewk_Context_Menu_Action action);

/**
 * Gets a title of the item.
 *
 * @param o the item to get the title
 * @return a title of the item on success, or @c NULL on failure
 *
 * @see ewk_context_menu_item_title_set
 */
EAPI const char                 *ewk_context_menu_item_title_get(const Ewk_Context_Menu_Item *o);

/**
 * Sets a title of the item.
 *
 * @param o the item to set the title
 * @param title a new title for the item object
 * @return a new title of the item on success or @c NULL on failure
 *
 * @see ewk_context_menu_item_title_get
 */
EAPI const char                 *ewk_context_menu_item_title_set(Ewk_Context_Menu_Item *o, const char *title);

/**
 * Queries if the item is toggled.
 *
 * @param o the item to query if the item is toggled
 * @return @c EINA_TRUE if the item is toggled or @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool                   ewk_context_menu_item_checked_get(const Ewk_Context_Menu_Item *o);

/**
 * Sets if the item should be toggled.
 *
 * @param o the item to be toggled
 * @param checked @c EINA_TRUE if the item should be toggled or @c EINA_FALSE if not
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool                   ewk_context_menu_item_checked_set(Ewk_Context_Menu_Item *o, Eina_Bool checked);

/**
 * Gets if the item is enabled.
 *
 * @param o the item to get enabled state
 * @return @c EINA_TRUE if it's enabled, @c EINA_FALSE if not or on failure
 *
 * @see ewk_context_menu_item_enabled_set
 */
EAPI Eina_Bool                   ewk_context_menu_item_enabled_get(const Ewk_Context_Menu_Item *o);

/**
 * Enables/disables the item.
 *
 * @param o the item to enable/disable
 * @param enabled @c EINA_TRUE to enable the item or @c EINA_FALSE to disable
 * @return @c EINA_TRUE on success, or @c EINA_FALSE on failure
 *
 * @see ewk_context_menu_item_enabled_get
 */
EAPI Eina_Bool                   ewk_context_menu_item_enabled_set(Ewk_Context_Menu_Item *o, Eina_Bool enabled);

/**
 * Gets the parent menu for context menu item.
 *
 * @param o the context menu item object
 * @return a context menu object on success or @c NULL on failure
 */
EAPI Ewk_Context_Menu           *ewk_context_menu_item_parent_get(const Ewk_Context_Menu_Item *o);

#ifdef __cplusplus
}
#endif
#endif // ewk_contextmenu_h
