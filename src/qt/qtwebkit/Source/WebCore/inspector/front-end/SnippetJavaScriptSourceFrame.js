/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 * @extends {WebInspector.JavaScriptSourceFrame}
 * @param {WebInspector.ScriptsPanel} scriptsPanel
 * @param {WebInspector.UISourceCode} uiSourceCode
 */
WebInspector.SnippetJavaScriptSourceFrame = function(scriptsPanel, uiSourceCode)
{
    WebInspector.JavaScriptSourceFrame.call(this, scriptsPanel, uiSourceCode);
    
    this._uiSourceCode = uiSourceCode;
    this._runButton = new WebInspector.StatusBarButton(WebInspector.UIString("Run"), "evaluate-snippet-status-bar-item");
    this._runButton.addEventListener("click", this._runButtonClicked, this);
    this.textEditor.element.addEventListener("keydown", this._onKeyDown.bind(this), true);
    this._snippetsShortcuts = {};
    var runSnippetShortcutDescriptor = WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Enter, WebInspector.KeyboardShortcut.Modifiers.CtrlOrMeta)
    this._snippetsShortcuts[runSnippetShortcutDescriptor.key] = this._runSnippet.bind(this);
}

WebInspector.SnippetJavaScriptSourceFrame.prototype = {
    /**
     * @return {Array.<Element>}
     */
    statusBarItems: function()
    {
        return [this._runButton.element].concat(WebInspector.JavaScriptSourceFrame.prototype.statusBarItems.call(this));
    },

    _runButtonClicked: function()
    {
        this._runSnippet();
    },

    _runSnippet: function()
    {
        WebInspector.scriptSnippetModel.evaluateScriptSnippet(this._uiSourceCode);
    },

    /**
     * @param {KeyboardEvent} event
     */
    _onKeyDown: function(event)
    {
        var shortcutKey = WebInspector.KeyboardShortcut.makeKeyFromEvent(event);
        var handler = this._snippetsShortcuts[shortcutKey];
        if (handler) {
            handler(event);
            event.handled = true;
        }
    },

    __proto__: WebInspector.JavaScriptSourceFrame.prototype
}
