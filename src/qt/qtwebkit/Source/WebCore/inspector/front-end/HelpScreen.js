/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @param {string=} title
 * @extends {WebInspector.View}
 */
WebInspector.HelpScreen = function(title)
{
    WebInspector.View.call(this);
    this.markAsRoot();
    this.registerRequiredCSS("helpScreen.css");

    this.element.className = "help-window-outer";
    this.element.addEventListener("keydown", this._onKeyDown.bind(this), false);
    this.element.tabIndex = 0;
    this.element.addEventListener("focus", this._onBlur.bind(this), false);

    if (title) {
        var mainWindow = this.element.createChild("div", "help-window-main");
        var captionWindow = mainWindow.createChild("div", "help-window-caption");
        captionWindow.appendChild(this._createCloseButton());
        this.contentElement = mainWindow.createChild("div", "help-content");
        captionWindow.createChild("h1", "help-window-title").textContent = title;
    }
}

/**
 * @type {WebInspector.HelpScreen}
 */
WebInspector.HelpScreen._visibleScreen = null;

WebInspector.HelpScreen.prototype = {
    _createCloseButton: function()
    {
        var closeButton = document.createElement("button");
        closeButton.className = "help-close-button";
        closeButton.textContent = "\u2716"; // Code stands for HEAVY MULTIPLICATION X.
        closeButton.addEventListener("click", this.hide.bind(this), false);
        return closeButton;
    },

    showModal: function()
    {
        var visibleHelpScreen = WebInspector.HelpScreen._visibleScreen;
        if (visibleHelpScreen === this)
            return;

        if (visibleHelpScreen)
            visibleHelpScreen.hide();
        WebInspector.HelpScreen._visibleScreen = this;
        this.show(document.body);
        this.focus();
    },

    hide: function()
    {
        if (!this.isShowing())
            return;

        WebInspector.HelpScreen._visibleScreen = null;

        WebInspector.restoreFocusFromElement(this.element);
        this.detach();
    },

    /**
     * @param {number} keyCode
     * @return {boolean}
     */
    isClosingKey: function(keyCode)
    {
        return [
            WebInspector.KeyboardShortcut.Keys.Enter.code,
            WebInspector.KeyboardShortcut.Keys.Esc.code,
            WebInspector.KeyboardShortcut.Keys.Space.code,
        ].indexOf(keyCode) >= 0;
    },

    _onKeyDown: function(event)
    {
        if (this.isShowing() && this.isClosingKey(event.keyCode)) {
            this.hide();
            event.consume();
        }
    },

    _onBlur: function(event)
    {
        // Pretend we're modal, grab focus back if we're still shown.
        if (this.isShowing() && !this.element.isSelfOrAncestor(event.target))
            WebInspector.setCurrentFocusElement(this.element);
    },

    __proto__: WebInspector.View.prototype
}

/**
 * @constructor
 * @param {string=} title
 * @param {string=} message
 * @extends {WebInspector.HelpScreen}
 */
WebInspector.HelpScreenUntilReload = function(title, message)
{
    WebInspector.HelpScreen.call(this, title);
    var p = this.contentElement.createChild("p");
    p.addStyleClass("help-section");
    p.textContent = message;
    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.GlobalObjectCleared, this.hide, this);
}

WebInspector.HelpScreenUntilReload.prototype = {
    /**
     * @override
     */
    willHide: function()
    {
        WebInspector.debuggerModel.removeEventListener(WebInspector.DebuggerModel.Events.GlobalObjectCleared, this.hide, this);
        WebInspector.HelpScreen.prototype.willHide.call(this);
    },

    __proto__: WebInspector.HelpScreen.prototype
}

