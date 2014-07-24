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

WebInspector.CSSStyleSheet = function(id, url, parentFrame)
{
    WebInspector.SourceCode.call(this);

    console.assert(id);

    this._id = id || null;

    this.updateInfo(url, parentFrame);
};

WebInspector.Object.addConstructorFunctions(WebInspector.CSSStyleSheet);

WebInspector.CSSStyleSheet.resetUniqueDisplayNameNumbers = function()
{
    WebInspector.CSSStyleSheet._nextUniqueDisplayNameNumber = 1;
}

WebInspector.CSSStyleSheet._nextUniqueDisplayNameNumber = 1;

WebInspector.CSSStyleSheet.Event = {
    ContentDidChange: "stylesheet-content-did-change"
};

WebInspector.CSSStyleSheet.prototype = {
    constructor: WebInspector.CSSStyleSheet,

    // Public

    get id()
    {
        return this._id;
    },

    get parentFrame()
    {
        return this._parentFrame;
    },

    get url()
    {
        return this._url;
    },

    get urlComponents()
    {
        if (!this._urlComponents)
            this._urlComponents = parseURL(this._url);
        return this._urlComponents;
    },

    get displayName()
    {
        if (this._url)
            return WebInspector.displayNameForURL(this._url, this.urlComponents);

        // Assign a unique number to the StyleSheet object so it will stay the same.
        if (!this._uniqueDisplayNameNumber)
            this._uniqueDisplayNameNumber = this.constructor._nextUniqueDisplayNameNumber++;

        return WebInspector.UIString("Anonymous StyleSheet %d").format(this._uniqueDisplayNameNumber);
    },

    // Protected

    updateInfo: function(url, parentFrame)
    {
        this._url = url || null;
        delete this._urlComponents;

        this._parentFrame = parentFrame || null;
    },

    get revisionForRequestedContent()
    {
        return this.currentRevision;
    },

    handleCurrentRevisionContentChange: function()
    {
        if (!this._id)
            return;

        function contentDidChange(error)
        {
            if (error)
                return;

            DOMAgent.markUndoableState();

            this.dispatchEventToListeners(WebInspector.CSSStyleSheet.Event.ContentDidChange);
        }

        this._ignoreNextContentDidChangeNotification = true;

        CSSAgent.setStyleSheetText(this._id, this.currentRevision.content, contentDidChange.bind(this));
    },

    canRequestContentFromBackend: function()
    {
        // We can request content if we have an id.
        return !!this._id;
    },

    requestContentFromBackend: function(callback)
    {
        if (!this._id) {
            // There is no identifier to request content with. Return false to cause the
            // pending callbacks to get null content.
            return false;
        }

        CSSAgent.getStyleSheetText(this._id, callback);
        return true;
    },

    noteContentDidChange: function()
    {
        if (this._ignoreNextContentDidChangeNotification) {
            delete this._ignoreNextContentDidChangeNotification;
            return false;
        }

        this.markContentAsStale();
        this.dispatchEventToListeners(WebInspector.CSSStyleSheet.Event.ContentDidChange);
        return true;
    }
};

WebInspector.CSSStyleSheet.prototype.__proto__ = WebInspector.SourceCode.prototype;
