/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL GOOGLE INC.
 * OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 */
WebInspector.InspectElementModeController = function()
{
    this.toggleSearchButton = new WebInspector.StatusBarButton(WebInspector.UIString("Select an element in the page to inspect it."), "node-search-status-bar-item");
    this.toggleSearchButton.addEventListener("click", this.toggleSearch, this);
    this._shortcut = WebInspector.InspectElementModeController.createShortcut();
}

WebInspector.InspectElementModeController.createShortcut = function()
{
    return WebInspector.KeyboardShortcut.makeDescriptor("c", WebInspector.KeyboardShortcut.Modifiers.CtrlOrMeta | WebInspector.KeyboardShortcut.Modifiers.Shift);
}

WebInspector.InspectElementModeController.prototype = {
    enabled: function()
    {
        return this.toggleSearchButton.toggled;
    },

    disable: function()
    {
        if (this.enabled())
            this.toggleSearch();
    },

    toggleSearch: function()
    {
        var enabled = !this.enabled();
        /**
         * @param {?Protocol.Error} error
         */
        function callback(error)
        {
            if (!error)
                this.toggleSearchButton.toggled = enabled;
        }
        WebInspector.domAgent.setInspectModeEnabled(enabled, callback.bind(this));
    },

    /**
     * @param {KeyboardEvent} event
     * @return {boolean}
     */
    handleShortcut: function(event)
    {
        if (WebInspector.KeyboardShortcut.makeKeyFromEvent(event) !== this._shortcut.key)
            return false;
        this.toggleSearch();
        event.consume(true);
        return true;
    }
}
