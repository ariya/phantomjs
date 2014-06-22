/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 * @extends {WebInspector.SourceFrame}
 * @param {WebInspector.UISourceCode} uiSourceCode
 */
WebInspector.UISourceCodeFrame = function(uiSourceCode)
{
    this._uiSourceCode = uiSourceCode;
    WebInspector.SourceFrame.call(this, this._uiSourceCode);
    this._uiSourceCode.addEventListener(WebInspector.UISourceCode.Events.FormattedChanged, this._onFormattedChanged, this);
    this._uiSourceCode.addEventListener(WebInspector.UISourceCode.Events.WorkingCopyChanged, this._onWorkingCopyChanged, this);
    this._uiSourceCode.addEventListener(WebInspector.UISourceCode.Events.WorkingCopyCommitted, this._onWorkingCopyCommitted, this);
}

WebInspector.UISourceCodeFrame.prototype = {
    wasShown: function()
    {
        WebInspector.SourceFrame.prototype.wasShown.call(this);
        this._boundWindowFocused = this._windowFocused.bind(this);
        window.addEventListener("focus", this._boundWindowFocused, false);
        this._checkContentUpdated();
    },

    willHide: function()
    {
        WebInspector.SourceFrame.prototype.willHide.call(this);
        window.removeEventListener("focus", this._boundWindowFocused, false);
        delete this._boundWindowFocused;
    },

    /**
     * @return {boolean}
     */
    canEditSource: function()
    {
        return this._uiSourceCode.isEditable();
    },

    _windowFocused: function(event)
    {
        this._checkContentUpdated();
    },

    _checkContentUpdated: function()
    {
        if (!this.loaded || !this.isShowing())
            return;
        this._uiSourceCode.checkContentUpdated();
    },

    /**
     * @param {string} text
     */
    commitEditing: function(text)
    {
        if (!this._uiSourceCode.isDirty())
            return;

        this._isCommittingEditing = true;
        this._uiSourceCode.commitWorkingCopy(this._didEditContent.bind(this));
        delete this._isCommittingEditing;
    },

    onTextChanged: function(oldRange, newRange)
    {
        WebInspector.SourceFrame.prototype.onTextChanged.call(this, oldRange, newRange);
        this._isSettingWorkingCopy = true;
        this._uiSourceCode.setWorkingCopy(this._textEditor.text());
        delete this._isSettingWorkingCopy;
    },

    _didEditContent: function(error)
    {
        if (error) {
            WebInspector.log(error, WebInspector.ConsoleMessage.MessageLevel.Error, true);
            return;
        }
    },

    /**
     * @param {WebInspector.Event} event
     */
    _onFormattedChanged: function(event)
    {
        var content = /** @type {string} */ (event.data.content);
        this._textEditor.setReadOnly(this._uiSourceCode.formatted());
        this.setContent(content, false, this._uiSourceCode.mimeType());
    },

    /**
     * @param {WebInspector.Event} event
     */
    _onWorkingCopyChanged: function(event)
    {
        this._innerSetContent(this._uiSourceCode.workingCopy());
    },

    /**
     * @param {WebInspector.Event} event
     */
    _onWorkingCopyCommitted: function(event)
    {
        this._innerSetContent(this._uiSourceCode.workingCopy());
    },

    /**
     * @param {string} content
     */
    onUISourceCodeContentChanged: function(content)
    {
        this.setContent(content, false, this._uiSourceCode.mimeType());
    },

    /**
     * @param {string} content
     */
    _innerSetContent: function(content)
    {
        if (this._isSettingWorkingCopy || this._isCommittingEditing)
            return;
        this.onUISourceCodeContentChanged(content);
    },

    populateTextAreaContextMenu: function(contextMenu, lineNumber)
    {
        WebInspector.SourceFrame.prototype.populateTextAreaContextMenu.call(this, contextMenu, lineNumber);
        contextMenu.appendApplicableItems(this._uiSourceCode);
        contextMenu.appendSeparator();
    },

    dispose: function()
    {
        this.detach();
    },

    __proto__: WebInspector.SourceFrame.prototype
}
