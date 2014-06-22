/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

WebInspector.DetailsSidebarPanel = function(identifier, displayName, singularDisplayName, image, keyboardShortcutKey, element) {
    if (keyboardShortcutKey)
        this._keyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Control | WebInspector.KeyboardShortcut.Modifier.Shift, keyboardShortcutKey, this.toggle.bind(this));

    if (this._keyboardShortcut) {
        var showToolTip = WebInspector.UIString("Show the %s details sidebar (%s)").format(singularDisplayName, this._keyboardShortcut.displayName);
        var hideToolTip = WebInspector.UIString("Hide the %s details sidebar (%s)").format(singularDisplayName, this._keyboardShortcut.displayName);
    } else {
        var showToolTip = WebInspector.UIString("Show the %s details sidebar").format(singularDisplayName);
        var hideToolTip = WebInspector.UIString("Hide the %s details sidebar").format(singularDisplayName);
    }

    WebInspector.SidebarPanel.call(this, identifier, displayName, showToolTip, hideToolTip, image, element);

    this.element.classList.add(WebInspector.DetailsSidebarPanel.StyleClassName);
};

WebInspector.DetailsSidebarPanel.StyleClassName = "details";

WebInspector.DetailsSidebarPanel.prototype = {
    constructor: WebInspector.DetailsSidebarPanel,

    // Public

    inspect: function(objects)
    {
        // Implemented by subclasses.
        return false;
    },

    shown: function()
    {
        if (this._needsRefresh) {
            delete this._needsRefresh;
            this.refresh();
        }
    },

    needsRefresh: function()
    {
        if (!this.selected) {
            this._needsRefresh = true;
            return;
        }

        this.refresh();
    },

    refresh: function()
    {
        // Implemented by subclasses.
    }
};

WebInspector.DetailsSidebarPanel.prototype.__proto__ = WebInspector.SidebarPanel.prototype;
