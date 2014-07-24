/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebKitContextMenuActions.h"

#include "WebKitContextMenuActionsPrivate.h"
#include <WebCore/LocalizedStrings.h>

using namespace WebCore;

bool webkitContextMenuActionIsCheckable(WebKitContextMenuAction action)
{
    switch (action) {
    case WEBKIT_CONTEXT_MENU_ACTION_BOLD:
    case WEBKIT_CONTEXT_MENU_ACTION_ITALIC:
    case WEBKIT_CONTEXT_MENU_ACTION_UNDERLINE:
    case WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_CONTROLS:
    case WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_LOOP:
        return true;
    default:
        return false;
    }
}

ContextMenuAction webkitContextMenuActionGetActionTag(WebKitContextMenuAction action)
{
    switch (action) {
    case WEBKIT_CONTEXT_MENU_ACTION_NO_ACTION:
        return ContextMenuItemTagNoAction;
    case WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK:
        return ContextMenuItemTagOpenLink;
    case WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK_IN_NEW_WINDOW:
        return ContextMenuItemTagOpenLinkInNewWindow;
    case WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_LINK_TO_DISK:
        return ContextMenuItemTagDownloadLinkToDisk;
    case WEBKIT_CONTEXT_MENU_ACTION_COPY_LINK_TO_CLIPBOARD:
        return ContextMenuItemTagCopyLinkToClipboard;
    case WEBKIT_CONTEXT_MENU_ACTION_OPEN_IMAGE_IN_NEW_WINDOW:
        return ContextMenuItemTagOpenImageInNewWindow;
    case WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_IMAGE_TO_DISK:
        return ContextMenuItemTagDownloadImageToDisk;
    case WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_TO_CLIPBOARD:
        return ContextMenuItemTagCopyImageToClipboard;
    case WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_URL_TO_CLIPBOARD:
        return ContextMenuItemTagCopyImageUrlToClipboard;
    case WEBKIT_CONTEXT_MENU_ACTION_OPEN_FRAME_IN_NEW_WINDOW:
        return ContextMenuItemTagOpenFrameInNewWindow;
    case WEBKIT_CONTEXT_MENU_ACTION_GO_BACK:
        return ContextMenuItemTagGoBack;
    case WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD:
        return ContextMenuItemTagGoForward;
    case WEBKIT_CONTEXT_MENU_ACTION_STOP:
        return ContextMenuItemTagStop;
    case WEBKIT_CONTEXT_MENU_ACTION_RELOAD:
        return ContextMenuItemTagReload;
    case WEBKIT_CONTEXT_MENU_ACTION_COPY:
        return ContextMenuItemTagCopy;
    case WEBKIT_CONTEXT_MENU_ACTION_CUT:
        return ContextMenuItemTagCut;
    case WEBKIT_CONTEXT_MENU_ACTION_PASTE:
        return ContextMenuItemTagPaste;
    case WEBKIT_CONTEXT_MENU_ACTION_DELETE:
        return ContextMenuItemTagDelete;
    case WEBKIT_CONTEXT_MENU_ACTION_SELECT_ALL:
        return ContextMenuItemTagSelectAll;
    case WEBKIT_CONTEXT_MENU_ACTION_INPUT_METHODS:
        return ContextMenuItemTagInputMethods;
    case WEBKIT_CONTEXT_MENU_ACTION_UNICODE:
        return ContextMenuItemTagUnicode;
    case WEBKIT_CONTEXT_MENU_ACTION_SPELLING_GUESS:
        return ContextMenuItemTagSpellingGuess;
    case WEBKIT_CONTEXT_MENU_ACTION_NO_GUESSES_FOUND:
        return ContextMenuItemTagNoGuessesFound;
    case WEBKIT_CONTEXT_MENU_ACTION_IGNORE_SPELLING:
        return ContextMenuItemTagIgnoreSpelling;
    case WEBKIT_CONTEXT_MENU_ACTION_LEARN_SPELLING:
        return ContextMenuItemTagLearnSpelling;
    case WEBKIT_CONTEXT_MENU_ACTION_IGNORE_GRAMMAR:
        return ContextMenuItemTagIgnoreGrammar;
    case WEBKIT_CONTEXT_MENU_ACTION_FONT_MENU:
        return ContextMenuItemTagFontMenu;
    case WEBKIT_CONTEXT_MENU_ACTION_BOLD:
        return ContextMenuItemTagBold;
    case WEBKIT_CONTEXT_MENU_ACTION_ITALIC:
        return ContextMenuItemTagItalic;
    case WEBKIT_CONTEXT_MENU_ACTION_UNDERLINE:
        return ContextMenuItemTagUnderline;
    case WEBKIT_CONTEXT_MENU_ACTION_OUTLINE:
        return ContextMenuItemTagOutline;
    case WEBKIT_CONTEXT_MENU_ACTION_INSPECT_ELEMENT:
        return ContextMenuItemTagInspectElement;
    case WEBKIT_CONTEXT_MENU_ACTION_OPEN_VIDEO_IN_NEW_WINDOW:
    case WEBKIT_CONTEXT_MENU_ACTION_OPEN_AUDIO_IN_NEW_WINDOW:
        return ContextMenuItemTagOpenMediaInNewWindow;
    case WEBKIT_CONTEXT_MENU_ACTION_COPY_VIDEO_LINK_TO_CLIPBOARD:
    case WEBKIT_CONTEXT_MENU_ACTION_COPY_AUDIO_LINK_TO_CLIPBOARD:
        return ContextMenuItemTagCopyMediaLinkToClipboard;
    case WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_CONTROLS:
        return ContextMenuItemTagToggleMediaControls;
    case WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_LOOP:
        return ContextMenuItemTagToggleMediaLoop;
    case WEBKIT_CONTEXT_MENU_ACTION_ENTER_VIDEO_FULLSCREEN:
        return ContextMenuItemTagEnterVideoFullscreen;
    case WEBKIT_CONTEXT_MENU_ACTION_MEDIA_PLAY:
    case WEBKIT_CONTEXT_MENU_ACTION_MEDIA_PAUSE:
        return ContextMenuItemTagMediaPlayPause;
    case WEBKIT_CONTEXT_MENU_ACTION_MEDIA_MUTE:
        return ContextMenuItemTagMediaMute;
    case WEBKIT_CONTEXT_MENU_ACTION_CUSTOM:
        return ContextMenuItemBaseApplicationTag;
    default:
        ASSERT_NOT_REACHED();
    }

    return ContextMenuItemBaseApplicationTag;
}

WebKitContextMenuAction webkitContextMenuActionGetForContextMenuItem(ContextMenuItem* menuItem)
{
    switch (menuItem->action()) {
    case ContextMenuItemTagNoAction:
        return WEBKIT_CONTEXT_MENU_ACTION_NO_ACTION;
    case ContextMenuItemTagOpenLink:
        return WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK;
    case ContextMenuItemTagOpenLinkInNewWindow:
        return WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK_IN_NEW_WINDOW;
    case ContextMenuItemTagDownloadLinkToDisk:
        return WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_LINK_TO_DISK;
    case ContextMenuItemTagCopyLinkToClipboard:
        return WEBKIT_CONTEXT_MENU_ACTION_COPY_LINK_TO_CLIPBOARD;
    case ContextMenuItemTagOpenImageInNewWindow:
        return WEBKIT_CONTEXT_MENU_ACTION_OPEN_IMAGE_IN_NEW_WINDOW;
    case ContextMenuItemTagDownloadImageToDisk:
        return WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_IMAGE_TO_DISK;
    case ContextMenuItemTagCopyImageToClipboard:
        return WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_TO_CLIPBOARD;
    case ContextMenuItemTagCopyImageUrlToClipboard:
        return WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_URL_TO_CLIPBOARD;
    case ContextMenuItemTagOpenFrameInNewWindow:
        return WEBKIT_CONTEXT_MENU_ACTION_OPEN_FRAME_IN_NEW_WINDOW;
    case ContextMenuItemTagGoBack:
        return WEBKIT_CONTEXT_MENU_ACTION_GO_BACK;
    case ContextMenuItemTagGoForward:
        return WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD;
    case ContextMenuItemTagStop:
        return WEBKIT_CONTEXT_MENU_ACTION_STOP;
    case ContextMenuItemTagReload:
        return WEBKIT_CONTEXT_MENU_ACTION_RELOAD;
    case ContextMenuItemTagCopy:
        return WEBKIT_CONTEXT_MENU_ACTION_COPY;
    case ContextMenuItemTagCut:
        return WEBKIT_CONTEXT_MENU_ACTION_CUT;
    case ContextMenuItemTagPaste:
        return WEBKIT_CONTEXT_MENU_ACTION_PASTE;
    case ContextMenuItemTagDelete:
        return WEBKIT_CONTEXT_MENU_ACTION_DELETE;
    case ContextMenuItemTagSelectAll:
        return WEBKIT_CONTEXT_MENU_ACTION_SELECT_ALL;
    case ContextMenuItemTagInputMethods:
        return WEBKIT_CONTEXT_MENU_ACTION_INPUT_METHODS;
    case ContextMenuItemTagUnicode:
        return WEBKIT_CONTEXT_MENU_ACTION_UNICODE;
    case ContextMenuItemTagSpellingGuess:
        return WEBKIT_CONTEXT_MENU_ACTION_SPELLING_GUESS;
    case ContextMenuItemTagIgnoreSpelling:
        return WEBKIT_CONTEXT_MENU_ACTION_IGNORE_SPELLING;
    case ContextMenuItemTagLearnSpelling:
        return WEBKIT_CONTEXT_MENU_ACTION_LEARN_SPELLING;
    case ContextMenuItemTagIgnoreGrammar:
        return WEBKIT_CONTEXT_MENU_ACTION_IGNORE_GRAMMAR;
    case ContextMenuItemTagFontMenu:
        return WEBKIT_CONTEXT_MENU_ACTION_FONT_MENU;
    case ContextMenuItemTagBold:
        return WEBKIT_CONTEXT_MENU_ACTION_BOLD;
    case ContextMenuItemTagItalic:
        return WEBKIT_CONTEXT_MENU_ACTION_ITALIC;
    case ContextMenuItemTagUnderline:
        return WEBKIT_CONTEXT_MENU_ACTION_UNDERLINE;
    case ContextMenuItemTagOutline:
        return WEBKIT_CONTEXT_MENU_ACTION_OUTLINE;
    case ContextMenuItemTagInspectElement:
        return WEBKIT_CONTEXT_MENU_ACTION_INSPECT_ELEMENT;
    case ContextMenuItemTagOpenMediaInNewWindow:
        return menuItem->title() == contextMenuItemTagOpenVideoInNewWindow() ?
            WEBKIT_CONTEXT_MENU_ACTION_OPEN_VIDEO_IN_NEW_WINDOW : WEBKIT_CONTEXT_MENU_ACTION_OPEN_AUDIO_IN_NEW_WINDOW;
    case ContextMenuItemTagCopyMediaLinkToClipboard:
        return menuItem->title() == contextMenuItemTagCopyVideoLinkToClipboard() ?
            WEBKIT_CONTEXT_MENU_ACTION_COPY_VIDEO_LINK_TO_CLIPBOARD : WEBKIT_CONTEXT_MENU_ACTION_COPY_AUDIO_LINK_TO_CLIPBOARD;
    case ContextMenuItemTagToggleMediaControls:
        return WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_CONTROLS;
    case ContextMenuItemTagToggleMediaLoop:
        return WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_LOOP;
    case ContextMenuItemTagEnterVideoFullscreen:
        return WEBKIT_CONTEXT_MENU_ACTION_ENTER_VIDEO_FULLSCREEN;
    case ContextMenuItemTagMediaPlayPause:
        return menuItem->title() == contextMenuItemTagMediaPlay() ?
            WEBKIT_CONTEXT_MENU_ACTION_MEDIA_PLAY : WEBKIT_CONTEXT_MENU_ACTION_MEDIA_PAUSE;
    case ContextMenuItemTagMediaMute:
        return WEBKIT_CONTEXT_MENU_ACTION_MEDIA_MUTE;
    case ContextMenuItemBaseApplicationTag:
        return WEBKIT_CONTEXT_MENU_ACTION_CUSTOM;
    default:
        ASSERT_NOT_REACHED();
    }

    return WEBKIT_CONTEXT_MENU_ACTION_CUSTOM;
}

String webkitContextMenuActionGetLabel(WebKitContextMenuAction action)
{
    switch (action) {
    case WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK:
        return contextMenuItemTagOpenLink();
    case WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK_IN_NEW_WINDOW:
        return contextMenuItemTagOpenLinkInNewWindow();
    case WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_LINK_TO_DISK:
        return contextMenuItemTagDownloadLinkToDisk();
    case WEBKIT_CONTEXT_MENU_ACTION_COPY_LINK_TO_CLIPBOARD:
        return contextMenuItemTagCopyLinkToClipboard();
    case WEBKIT_CONTEXT_MENU_ACTION_OPEN_IMAGE_IN_NEW_WINDOW:
        return contextMenuItemTagOpenImageInNewWindow();
    case WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_IMAGE_TO_DISK:
        return contextMenuItemTagDownloadImageToDisk();
    case WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_TO_CLIPBOARD:
        return contextMenuItemTagCopyImageToClipboard();
    case WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_URL_TO_CLIPBOARD:
        return contextMenuItemTagCopyImageUrlToClipboard();
    case WEBKIT_CONTEXT_MENU_ACTION_OPEN_FRAME_IN_NEW_WINDOW:
        return contextMenuItemTagOpenFrameInNewWindow();
    case WEBKIT_CONTEXT_MENU_ACTION_GO_BACK:
        return contextMenuItemTagGoBack();
    case WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD:
        return contextMenuItemTagGoForward();
    case WEBKIT_CONTEXT_MENU_ACTION_STOP:
        return contextMenuItemTagStop();
    case WEBKIT_CONTEXT_MENU_ACTION_RELOAD:
        return contextMenuItemTagReload();
    case WEBKIT_CONTEXT_MENU_ACTION_COPY:
        return contextMenuItemTagCopy();
    case WEBKIT_CONTEXT_MENU_ACTION_CUT:
        return contextMenuItemTagCut();
    case WEBKIT_CONTEXT_MENU_ACTION_PASTE:
        return contextMenuItemTagPaste();
    case WEBKIT_CONTEXT_MENU_ACTION_DELETE:
        return contextMenuItemTagDelete();
    case WEBKIT_CONTEXT_MENU_ACTION_SELECT_ALL:
        return contextMenuItemTagSelectAll();
    case WEBKIT_CONTEXT_MENU_ACTION_INPUT_METHODS:
        return contextMenuItemTagInputMethods();
    case WEBKIT_CONTEXT_MENU_ACTION_UNICODE:
        return contextMenuItemTagUnicode();
    case WEBKIT_CONTEXT_MENU_ACTION_NO_GUESSES_FOUND:
        return contextMenuItemTagNoGuessesFound();
    case WEBKIT_CONTEXT_MENU_ACTION_IGNORE_SPELLING:
        return contextMenuItemTagIgnoreSpelling();
    case WEBKIT_CONTEXT_MENU_ACTION_LEARN_SPELLING:
        return contextMenuItemTagLearnSpelling();
    case WEBKIT_CONTEXT_MENU_ACTION_IGNORE_GRAMMAR:
        return contextMenuItemTagIgnoreGrammar();
    case WEBKIT_CONTEXT_MENU_ACTION_FONT_MENU:
        return contextMenuItemTagFontMenu();
    case WEBKIT_CONTEXT_MENU_ACTION_BOLD:
        return contextMenuItemTagBold();
    case WEBKIT_CONTEXT_MENU_ACTION_ITALIC:
        return contextMenuItemTagItalic();
    case WEBKIT_CONTEXT_MENU_ACTION_UNDERLINE:
        return contextMenuItemTagUnderline();
    case WEBKIT_CONTEXT_MENU_ACTION_OUTLINE:
        return contextMenuItemTagOutline();
    case WEBKIT_CONTEXT_MENU_ACTION_INSPECT_ELEMENT:
        return contextMenuItemTagInspectElement();
    case WEBKIT_CONTEXT_MENU_ACTION_OPEN_VIDEO_IN_NEW_WINDOW:
        return contextMenuItemTagOpenVideoInNewWindow();
    case WEBKIT_CONTEXT_MENU_ACTION_OPEN_AUDIO_IN_NEW_WINDOW:
        return contextMenuItemTagOpenAudioInNewWindow();
    case WEBKIT_CONTEXT_MENU_ACTION_COPY_VIDEO_LINK_TO_CLIPBOARD:
        return contextMenuItemTagCopyVideoLinkToClipboard();
    case WEBKIT_CONTEXT_MENU_ACTION_COPY_AUDIO_LINK_TO_CLIPBOARD:
        return contextMenuItemTagCopyAudioLinkToClipboard();
    case WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_CONTROLS:
        return contextMenuItemTagToggleMediaControls();
    case WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_LOOP:
        return contextMenuItemTagToggleMediaLoop();
    case WEBKIT_CONTEXT_MENU_ACTION_ENTER_VIDEO_FULLSCREEN:
        return contextMenuItemTagEnterVideoFullscreen();
    case WEBKIT_CONTEXT_MENU_ACTION_MEDIA_PLAY:
        return contextMenuItemTagMediaPlay();
    case WEBKIT_CONTEXT_MENU_ACTION_MEDIA_PAUSE:
        return contextMenuItemTagMediaPause();
    case WEBKIT_CONTEXT_MENU_ACTION_MEDIA_MUTE:
        return contextMenuItemTagMediaMute();
    case WEBKIT_CONTEXT_MENU_ACTION_NO_ACTION:
    case WEBKIT_CONTEXT_MENU_ACTION_CUSTOM:
    case WEBKIT_CONTEXT_MENU_ACTION_SPELLING_GUESS:
        return String();
    default:
        ASSERT_NOT_REACHED();
    }

    return String();
}
