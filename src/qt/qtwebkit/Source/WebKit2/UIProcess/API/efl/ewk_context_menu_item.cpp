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

#include "config.h"
#include "ewk_context_menu_item.h"

#include "ewk_context_menu_item_private.h"
#include "ewk_context_menu_private.h"
#include "ewk_private.h"
#include <WebKit2/WKContextMenuItemTypes.h>
#include <wtf/text/CString.h>

using namespace WebKit;

static Ewk_Context_Menu_Item_Action getEwkActionFromWKTag(WKContextMenuItemTag action);

EwkContextMenuItem::EwkContextMenuItem(WKContextMenuItemRef item, EwkContextMenu* parentMenu)
    : m_type(static_cast<Ewk_Context_Menu_Item_Type>(WKContextMenuItemGetType(item)))
    , m_action(getEwkActionFromWKTag((WKContextMenuItemGetTag(item))))
    , m_title(WKEinaSharedString(AdoptWK, WKContextMenuItemCopyTitle(item)))
    , m_isChecked(WKContextMenuItemGetChecked(item))
    , m_isEnabled(WKContextMenuItemGetEnabled(item))
    , m_parentMenu(parentMenu)
{
    if (WKContextMenuItemGetType(item) == kWKContextMenuItemTypeSubmenu) {
        WKRetainPtr<WKArrayRef> menuItems = adoptWK(WKContextMenuCopySubmenuItems(item));
        m_subMenu = EwkContextMenu::create(m_parentMenu->ewkView(), menuItems.get());
    }
}

EwkContextMenuItem::EwkContextMenuItem(Ewk_Context_Menu_Item_Type type, Ewk_Context_Menu_Item_Action action, const char* title, Eina_Bool checked, Eina_Bool enabled, PassRefPtr<EwkContextMenu> subMenu, EwkContextMenu* parentMenu)
    : m_type(type)
    , m_action(action)
    , m_title(title)
    , m_isChecked(checked)
    , m_isEnabled(enabled)
    , m_parentMenu(parentMenu)
    , m_subMenu(subMenu)
{
}

Ewk_Context_Menu_Item* ewk_context_menu_item_new(Ewk_Context_Menu_Item_Type type, Ewk_Context_Menu_Item_Action action, const char* title, Eina_Bool checked, Eina_Bool enabled)
{
    return Ewk_Context_Menu_Item::create(type, action, title, checked, enabled).leakPtr();
}

Ewk_Context_Menu_Item* ewk_context_menu_item_new_with_submenu(Ewk_Context_Menu_Item_Action action, const char* title, Eina_Bool enabled, Ewk_Context_Menu* subMenu)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContextMenu, subMenu, subMenuImpl, 0);

    return Ewk_Context_Menu_Item::create(EWK_SUBMENU_TYPE, action, title, false, enabled, subMenuImpl).leakPtr();
}

Ewk_Context_Menu_Item_Type ewk_context_menu_item_type_get(const Ewk_Context_Menu_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, EWK_ACTION_TYPE);

    return item->type();
}

Eina_Bool ewk_context_menu_item_type_set(Ewk_Context_Menu_Item* item, Ewk_Context_Menu_Item_Type type)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, false);

    item->setType(type);

    return true;
}

Ewk_Context_Menu_Item_Action ewk_context_menu_item_action_get(const Ewk_Context_Menu_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, EWK_CONTEXT_MENU_ITEM_TAG_NO_ACTION);

    return item->action();
}

Eina_Bool ewk_context_menu_item_action_set(Ewk_Context_Menu_Item* item, Ewk_Context_Menu_Item_Action action)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, false);

    item->setAction(action);

    return true;
}

const char* ewk_context_menu_item_title_get(const Ewk_Context_Menu_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, 0);

    return item->title();
}

Eina_Bool ewk_context_menu_item_title_set(Ewk_Context_Menu_Item* item, const char* title)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, false);

    item->setTitle(title);

    return true;
}

Eina_Bool ewk_context_menu_item_checked_get(const Ewk_Context_Menu_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, false);

    return item->checked();
}

Eina_Bool ewk_context_menu_item_checked_set(Ewk_Context_Menu_Item* item, Eina_Bool checked)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, false);

    item->setChecked(checked);

    return true;
}

Eina_Bool ewk_context_menu_item_enabled_get(const Ewk_Context_Menu_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, false);
    
    return item->enabled();
}

Eina_Bool ewk_context_menu_item_enabled_set(Ewk_Context_Menu_Item* item, Eina_Bool enabled)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, false);

    item->setEnabled(enabled);

    return true;
}

Ewk_Context_Menu* ewk_context_menu_item_parent_menu_get(const Ewk_Context_Menu_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, 0);

    return item->parentMenu();
}

Ewk_Context_Menu* ewk_context_menu_item_submenu_get(const Ewk_Context_Menu_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, 0);

    return item->subMenu();
}

static Ewk_Context_Menu_Item_Action getEwkActionFromWKTag(WKContextMenuItemTag action)
{
    switch (action) {
    case kWKContextMenuItemTagNoAction:
        return EWK_CONTEXT_MENU_ITEM_TAG_NO_ACTION;
    case kWKContextMenuItemTagOpenLinkInNewWindow:
        return EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK_IN_NEW_WINDOW;
    case kWKContextMenuItemTagDownloadLinkToDisk:
        return EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_LINK_TO_DISK;
    case kWKContextMenuItemTagCopyLinkToClipboard:
        return EWK_CONTEXT_MENU_ITEM_TAG_COPY_LINK_TO_CLIPBOARD;
    case kWKContextMenuItemTagOpenImageInNewWindow:
        return EWK_CONTEXT_MENU_ITEM_TAG_OPEN_IMAGE_IN_NEW_WINDOW;
    case kWKContextMenuItemTagDownloadImageToDisk:
        return EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_IMAGE_TO_DISK;
    case kWKContextMenuItemTagCopyImageToClipboard:
        return EWK_CONTEXT_MENU_ITEM_TAG_COPY_IMAGE_TO_CLIPBOARD;
    case kWKContextMenuItemTagCopyImageUrlToClipboard:
        return EWK_CONTEXT_MENU_ITEM_TAG_COPY_IMAGE_URL_TO_CLIPBOARD;
    case kWKContextMenuItemTagOpenFrameInNewWindow:
        return EWK_CONTEXT_MENU_ITEM_TAG_OPEN_FRAME_IN_NEW_WINDOW;
    case kWKContextMenuItemTagCopy:
        return EWK_CONTEXT_MENU_ITEM_TAG_COPY;
    case kWKContextMenuItemTagGoBack:
        return EWK_CONTEXT_MENU_ITEM_TAG_GO_BACK;
    case kWKContextMenuItemTagGoForward:
        return EWK_CONTEXT_MENU_ITEM_TAG_GO_FORWARD;
    case kWKContextMenuItemTagStop:
        return EWK_CONTEXT_MENU_ITEM_TAG_STOP;
    case kWKContextMenuItemTagReload:
        return EWK_CONTEXT_MENU_ITEM_TAG_RELOAD;
    case kWKContextMenuItemTagCut:
        return EWK_CONTEXT_MENU_ITEM_TAG_CUT;
    case kWKContextMenuItemTagPaste:
        return EWK_CONTEXT_MENU_ITEM_TAG_PASTE;
    case kWKContextMenuItemTagSelectAll:
        return EWK_CONTEXT_MENU_ITEM_TAG_SELECT_ALL;
    case kWKContextMenuItemTagSpellingGuess:
        return EWK_CONTEXT_MENU_ITEM_TAG_SPELLING_GUESS;
    case kWKContextMenuItemTagNoGuessesFound:
        return EWK_CONTEXT_MENU_ITEM_TAG_NO_GUESSES_FOUND;
    case kWKContextMenuItemTagIgnoreSpelling:
        return EWK_CONTEXT_MENU_ITEM_TAG_IGNORE_SPELLING;
    case kWKContextMenuItemTagLearnSpelling:
        return EWK_CONTEXT_MENU_ITEM_TAG_LEARN_SPELLING;
    case kWKContextMenuItemTagOther:
        return EWK_CONTEXT_MENU_ITEM_TAG_OTHER;
    case kWKContextMenuItemTagSearchInSpotlight:
        return EWK_CONTEXT_MENU_ITEM_TAG_SEARCH_IN_SPOTLIGHT;
    case kWKContextMenuItemTagSearchWeb:
        return EWK_CONTEXT_MENU_ITEM_TAG_SEARCH_WEB;
    case kWKContextMenuItemTagLookUpInDictionary:
        return EWK_CONTEXT_MENU_ITEM_TAG_LOOK_UP_IN_DICTIONARY;
    case kWKContextMenuItemTagOpenWithDefaultApplication:
        return EWK_CONTEXT_MENU_ITEM_TAG_OPEN_WITH_DEFAULT_APPLICATION;
    case kWKContextMenuItemTagPDFActualSize:
        return EWK_CONTEXT_MENU_ITEM_PDFACTUAL_SIZE;
    case kWKContextMenuItemTagPDFZoomIn:
        return EWK_CONTEXT_MENU_ITEM_PDFZOOM_IN;
    case kWKContextMenuItemTagPDFZoomOut:
        return EWK_CONTEXT_MENU_ITEM_PDFZOOM_OUT;
    case kWKContextMenuItemTagPDFAutoSize:
        return EWK_CONTEXT_MENU_ITEM_PDFAUTO_SIZE;
    case kWKContextMenuItemTagPDFSinglePage:
        return EWK_CONTEXT_MENU_ITEM_PDFSINGLE_PAGE;
    case kWKContextMenuItemTagPDFFacingPages:
        return EWK_CONTEXT_MENU_ITEM_PDFFACING_PAGES;
    case kWKContextMenuItemTagPDFContinuous:
        return EWK_CONTEXT_MENU_ITEM_PDFCONTINUOUS;
    case kWKContextMenuItemTagPDFNextPage:
        return EWK_CONTEXT_MENU_ITEM_PDFNEXT_PAGE;
    case kWKContextMenuItemTagPDFPreviousPage:
        return EWK_CONTEXT_MENU_ITEM_PDFPREVIOUS_PAGE;
    case kWKContextMenuItemTagOpenLink:
        return EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK;
    case kWKContextMenuItemTagIgnoreGrammar:
        return EWK_CONTEXT_MENU_ITEM_TAG_IGNORE_GRAMMAR;
    case kWKContextMenuItemTagSpellingMenu:
        return EWK_CONTEXT_MENU_ITEM_TAG_SPELLING_MENU;
    case kWKContextMenuItemTagShowSpellingPanel:
        return EWK_CONTEXT_MENU_ITEM_TAG_SHOW_SPELLING_PANEL;
    case kWKContextMenuItemTagCheckSpelling:
        return EWK_CONTEXT_MENU_ITEM_TAG_CHECK_SPELLING;
    case kWKContextMenuItemTagCheckSpellingWhileTyping:
        return EWK_CONTEXT_MENU_ITEM_TAG_CHECK_SPELLING_WHILE_TYPING;
    case kWKContextMenuItemTagCheckGrammarWithSpelling:
        return EWK_CONTEXT_MENU_ITEM_TAG_CHECK_GRAMMAR_WITH_SPELLING;
    case kWKContextMenuItemTagFontMenu:
        return EWK_CONTEXT_MENU_ITEM_TAG_FONT_MENU;
    case kWKContextMenuItemTagShowFonts:
        return EWK_CONTEXT_MENU_ITEM_TAG_SHOW_FONTS;
    case kWKContextMenuItemTagBold:
        return EWK_CONTEXT_MENU_ITEM_TAG_BOLD;
    case kWKContextMenuItemTagItalic:
        return EWK_CONTEXT_MENU_ITEM_TAG_ITALIC;
    case kWKContextMenuItemTagUnderline:
        return EWK_CONTEXT_MENU_ITEM_TAG_UNDERLINE;
    case kWKContextMenuItemTagOutline:
        return EWK_CONTEXT_MENU_ITEM_TAG_OUTLINE;
    case kWKContextMenuItemTagStyles:
        return EWK_CONTEXT_MENU_ITEM_TAG_STYLES;
    case kWKContextMenuItemTagShowColors:
        return EWK_CONTEXT_MENU_ITEM_TAG_SHOW_COLORS;
    case kWKContextMenuItemTagSpeechMenu:
        return EWK_CONTEXT_MENU_ITEM_TAG_SPEECH_MENU;
    case kWKContextMenuItemTagStartSpeaking:
        return EWK_CONTEXT_MENU_ITEM_TAG_START_SPEAKING;
    case kWKContextMenuItemTagStopSpeaking:
        return EWK_CONTEXT_MENU_ITEM_TAG_STOP_SPEAKING;
    case kWKContextMenuItemTagWritingDirectionMenu:
        return EWK_CONTEXT_MENU_ITEM_TAG_WRITING_DIRECTION_MENU;
    case kWKContextMenuItemTagDefaultDirection:
        return EWK_CONTEXT_MENU_ITEM_TAG_DEFAULT_DIRECTION;
    case kWKContextMenuItemTagLeftToRight:
        return EWK_CONTEXT_MENU_ITEM_TAG_LEFT_TO_RIGHT;
    case kWKContextMenuItemTagRightToLeft:
        return EWK_CONTEXT_MENU_ITEM_TAG_RIGHT_TO_LEFT;
    case kWKContextMenuItemTagPDFSinglePageScrolling:
        return EWK_CONTEXT_MENU_ITEM_TAG_PDFSINGLE_PAGE_SCROLLING;
    case kWKContextMenuItemTagPDFFacingPagesScrolling:
        return EWK_CONTEXT_MENU_ITEM_TAG_PDFFACING_PAGES_SCROLLING;
    case kWKContextMenuItemTagInspectElement:
        return EWK_CONTEXT_MENU_ITEM_TAG_INSPECT_ELEMENT;
    case kWKContextMenuItemTagTextDirectionMenu:
        return EWK_CONTEXT_MENU_ITEM_TAG_TEXT_DIRECTION_MENU;
    case kWKContextMenuItemTagTextDirectionDefault:
        return EWK_CONTEXT_MENU_ITEM_TAG_TEXT_DIRECTION_DEFAULT;
    case kWKContextMenuItemTagTextDirectionLeftToRight:
        return EWK_CONTEXT_MENU_ITEM_TAG_TEXT_DIRECTION_LEFT_TO_RIGHT;
    case kWKContextMenuItemTagTextDirectionRightToLeft:
        return EWK_CONTEXT_MENU_ITEM_TAG_TEXT_DIRECTION_RIGHT_TO_LEFT;
    case kWKContextMenuItemTagOpenMediaInNewWindow:
        return EWK_CONTEXT_MENU_ITEM_OPEN_MEDIA_IN_NEW_WINDOW;
    case kWKContextMenuItemTagDownloadMediaToDisk:
        return EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_MEDIA_TO_DISK;
    case kWKContextMenuItemTagCopyMediaLinkToClipboard:
        return EWK_CONTEXT_MENU_ITEM_TAG_COPY_MEDIA_LINK_TO_CLIPBOARD;
    case kWKContextMenuItemTagToggleMediaControls:
        return EWK_CONTEXT_MENU_ITEM_TAG_TOGGLE_MEDIA_CONTROLS;
    case kWKContextMenuItemTagToggleMediaLoop:
        return EWK_CONTEXT_MENU_ITEM_TAG_TOGGLE_MEDIA_LOOP;
    case kWKContextMenuItemTagEnterVideoFullscreen:
        return EWK_CONTEXT_MENU_ITEM_TAG_ENTER_VIDEO_FULLSCREEN;
    case kWKContextMenuItemTagMediaPlayPause:
        return EWK_CONTEXT_MENU_ITEM_TAG_MEDIA_PLAY_PAUSE;
    case kWKContextMenuItemTagMediaMute:
        return EWK_CONTEXT_MENU_ITEM_TAG_MEDIA_MUTE;
    case kWKContextMenuItemBaseApplicationTag:
        return EWK_CONTEXT_MENU_ITEM_BASE_APPLICATION_TAG;
    default:
        return static_cast<Ewk_Context_Menu_Item_Action>(action);
    }
}
