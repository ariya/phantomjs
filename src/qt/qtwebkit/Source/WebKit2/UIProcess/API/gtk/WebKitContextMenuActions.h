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

#if !defined(__WEBKIT2_H_INSIDE__) && !defined(WEBKIT2_COMPILATION)
#error "Only <webkit2/webkit2.h> can be included directly."
#endif

#include <glib.h>

#ifndef WebKitContextMenuActions_h
#define WebKitContextMenuActions_h

G_BEGIN_DECLS

/**
 * WebKitContextMenuAction:
 * @WEBKIT_CONTEXT_MENU_ACTION_NO_ACTION: No action, used by separator menu items.
 * @WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK: Open current link.
 * @WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK_IN_NEW_WINDOW: Open current link in a new window.
 * @WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_LINK_TO_DISK: Download link destination.
 * @WEBKIT_CONTEXT_MENU_ACTION_COPY_LINK_TO_CLIPBOARD: Copy link location to the clipboard.
 * @WEBKIT_CONTEXT_MENU_ACTION_OPEN_IMAGE_IN_NEW_WINDOW: Open current image in a new window.
 * @WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_IMAGE_TO_DISK: Download current image.
 * @WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_TO_CLIPBOARD: Copy current image to the clipboard.
 * @WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_URL_TO_CLIPBOARD: Copy curent image location to the clipboard.
 * @WEBKIT_CONTEXT_MENU_ACTION_OPEN_FRAME_IN_NEW_WINDOW: Open current frame in a new window.
 * @WEBKIT_CONTEXT_MENU_ACTION_GO_BACK: Load the previous history item.
 * @WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD: Load the next history item.
 * @WEBKIT_CONTEXT_MENU_ACTION_STOP: Stop any ongoing loading operation.
 * @WEBKIT_CONTEXT_MENU_ACTION_RELOAD: Reload the conents of current view.
 * @WEBKIT_CONTEXT_MENU_ACTION_COPY: Copy current selection the clipboard.
 * @WEBKIT_CONTEXT_MENU_ACTION_CUT: Cut current selection to the clipboard.
 * @WEBKIT_CONTEXT_MENU_ACTION_PASTE: Paste clipboard contents.
 * @WEBKIT_CONTEXT_MENU_ACTION_DELETE: Delete current selection.
 * @WEBKIT_CONTEXT_MENU_ACTION_SELECT_ALL: Select all text.
 * @WEBKIT_CONTEXT_MENU_ACTION_INPUT_METHODS: Input methods menu.
 * @WEBKIT_CONTEXT_MENU_ACTION_UNICODE: Unicode menu.
 * @WEBKIT_CONTEXT_MENU_ACTION_SPELLING_GUESS: A proposed replacement for a misspelled word.
 * @WEBKIT_CONTEXT_MENU_ACTION_NO_GUESSES_FOUND: An indicator that spellchecking found no proposed replacements.
 * @WEBKIT_CONTEXT_MENU_ACTION_IGNORE_SPELLING: Causes the spellchecker to ignore the word for this session.
 * @WEBKIT_CONTEXT_MENU_ACTION_LEARN_SPELLING: Causes the spellchecker to add the word to the dictionary.
 * @WEBKIT_CONTEXT_MENU_ACTION_IGNORE_GRAMMAR: Ignore grammar.
 * @WEBKIT_CONTEXT_MENU_ACTION_FONT_MENU: Font options menu.
 * @WEBKIT_CONTEXT_MENU_ACTION_BOLD: Bold.
 * @WEBKIT_CONTEXT_MENU_ACTION_ITALIC: Italic.
 * @WEBKIT_CONTEXT_MENU_ACTION_UNDERLINE: Underline.
 * @WEBKIT_CONTEXT_MENU_ACTION_OUTLINE: Outline.
 * @WEBKIT_CONTEXT_MENU_ACTION_INSPECT_ELEMENT: Open current element in the inspector.
 * @WEBKIT_CONTEXT_MENU_ACTION_OPEN_VIDEO_IN_NEW_WINDOW: Open current video element in a new window.
 * @WEBKIT_CONTEXT_MENU_ACTION_OPEN_AUDIO_IN_NEW_WINDOW: Open current audio element in a new window.
 * @WEBKIT_CONTEXT_MENU_ACTION_COPY_VIDEO_LINK_TO_CLIPBOARD: Copy video link location in to the clipboard.
 * @WEBKIT_CONTEXT_MENU_ACTION_COPY_AUDIO_LINK_TO_CLIPBOARD: Copy audio link location in to the clipboard.
 * @WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_CONTROLS: Enable or disable media controls.
 * @WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_LOOP: Enable or disable media loop.
 * @WEBKIT_CONTEXT_MENU_ACTION_ENTER_VIDEO_FULLSCREEN: Show current video element in fullscreen mode.
 * @WEBKIT_CONTEXT_MENU_ACTION_MEDIA_PLAY: Play current media element.
 * @WEBKIT_CONTEXT_MENU_ACTION_MEDIA_PAUSE: Pause current media element.
 * @WEBKIT_CONTEXT_MENU_ACTION_MEDIA_MUTE: Mute current media element.
 * @WEBKIT_CONTEXT_MENU_ACTION_CUSTOM: Custom action defined by applications.
 *
 * Enum values used to denote the stock actions for
 * #WebKitContextMenuItem<!-- -->s
 */
typedef enum {
    WEBKIT_CONTEXT_MENU_ACTION_NO_ACTION = 0,

    WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK,
    WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK_IN_NEW_WINDOW,
    WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_LINK_TO_DISK,
    WEBKIT_CONTEXT_MENU_ACTION_COPY_LINK_TO_CLIPBOARD,
    WEBKIT_CONTEXT_MENU_ACTION_OPEN_IMAGE_IN_NEW_WINDOW,
    WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_IMAGE_TO_DISK,
    WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_TO_CLIPBOARD,
    WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_URL_TO_CLIPBOARD,
    WEBKIT_CONTEXT_MENU_ACTION_OPEN_FRAME_IN_NEW_WINDOW,
    WEBKIT_CONTEXT_MENU_ACTION_GO_BACK,
    WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD,
    WEBKIT_CONTEXT_MENU_ACTION_STOP,
    WEBKIT_CONTEXT_MENU_ACTION_RELOAD,
    WEBKIT_CONTEXT_MENU_ACTION_COPY,
    WEBKIT_CONTEXT_MENU_ACTION_CUT,
    WEBKIT_CONTEXT_MENU_ACTION_PASTE,
    WEBKIT_CONTEXT_MENU_ACTION_DELETE,
    WEBKIT_CONTEXT_MENU_ACTION_SELECT_ALL,
    WEBKIT_CONTEXT_MENU_ACTION_INPUT_METHODS,
    WEBKIT_CONTEXT_MENU_ACTION_UNICODE,
    WEBKIT_CONTEXT_MENU_ACTION_SPELLING_GUESS,
    WEBKIT_CONTEXT_MENU_ACTION_NO_GUESSES_FOUND,
    WEBKIT_CONTEXT_MENU_ACTION_IGNORE_SPELLING,
    WEBKIT_CONTEXT_MENU_ACTION_LEARN_SPELLING,
    WEBKIT_CONTEXT_MENU_ACTION_IGNORE_GRAMMAR,
    WEBKIT_CONTEXT_MENU_ACTION_FONT_MENU,
    WEBKIT_CONTEXT_MENU_ACTION_BOLD,
    WEBKIT_CONTEXT_MENU_ACTION_ITALIC,
    WEBKIT_CONTEXT_MENU_ACTION_UNDERLINE,
    WEBKIT_CONTEXT_MENU_ACTION_OUTLINE,
    WEBKIT_CONTEXT_MENU_ACTION_INSPECT_ELEMENT,
    WEBKIT_CONTEXT_MENU_ACTION_OPEN_VIDEO_IN_NEW_WINDOW,
    WEBKIT_CONTEXT_MENU_ACTION_OPEN_AUDIO_IN_NEW_WINDOW,
    WEBKIT_CONTEXT_MENU_ACTION_COPY_VIDEO_LINK_TO_CLIPBOARD,
    WEBKIT_CONTEXT_MENU_ACTION_COPY_AUDIO_LINK_TO_CLIPBOARD,
    WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_CONTROLS,
    WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_LOOP,
    WEBKIT_CONTEXT_MENU_ACTION_ENTER_VIDEO_FULLSCREEN,
    WEBKIT_CONTEXT_MENU_ACTION_MEDIA_PLAY,
    WEBKIT_CONTEXT_MENU_ACTION_MEDIA_PAUSE,
    WEBKIT_CONTEXT_MENU_ACTION_MEDIA_MUTE,

    WEBKIT_CONTEXT_MENU_ACTION_CUSTOM = 10000
} WebKitContextMenuAction;

G_END_DECLS

#endif
