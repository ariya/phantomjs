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

WebInspector.SourceCode = function()
{
    WebInspector.Object.call(this);

    this._pendingContentRequestCallbacks = [];

    this._originalRevision = new WebInspector.SourceCodeRevision(this, null, false);
    this._currentRevision = this._originalRevision;

    this._sourceMaps = null;
    this._formatterSourceMap = null;
};

WebInspector.SourceCode.Event = {
    ContentDidChange: "source-code-content-did-change",
    SourceMapAdded: "source-code-source-map-added",
    FormatterDidChange: "source-code-formatter-did-change"
};

WebInspector.SourceCode.prototype = {
    constructor: WebInspector.SourceCode,

    // Public

    get displayName()
    {
        // Implemented by subclasses.
        console.error("Needs to be implemented by a subclass.");
        return "";
    },

    get originalRevision()
    {
        return this._originalRevision;
    },

    get currentRevision()
    {
        return this._currentRevision;
    },

    set currentRevision(revision)
    {
        console.assert(revision instanceof WebInspector.SourceCodeRevision);
        if (!(revision instanceof WebInspector.SourceCodeRevision))
            return;

        console.assert(revision.sourceCode === this);
        if (revision.sourceCode !== this)
            return;

        this._currentRevision = revision;

        this.dispatchEventToListeners(WebInspector.SourceCode.Event.ContentDidChange);
    },

    get content()
    {
        return this._currentRevision.content;
    },

    get contentIsBase64Encoded()
    {
        return this._currentRevision.contentIsBase64Encoded;
    },

    get sourceMaps()
    {
        return this._sourceMaps || [];
    },

    addSourceMap: function(sourceMap)
    {
        console.assert(sourceMap instanceof WebInspector.SourceMap);

        if (!this._sourceMaps)
            this._sourceMaps = [];

        this._sourceMaps.push(sourceMap);

        this.dispatchEventToListeners(WebInspector.SourceCode.Event.SourceMapAdded);
    },

    get formatterSourceMap()
    {
        return this._formatterSourceMap;
    },

    set formatterSourceMap(formatterSourceMap)
    {
        console.assert(this._formatterSourceMap === null || formatterSourceMap === null);
        console.assert(formatterSourceMap === null || formatterSourceMap instanceof WebInspector.FormatterSourceMap);

        this._formatterSourceMap = formatterSourceMap;

        this.dispatchEventToListeners(WebInspector.SourceCode.Event.FormatterDidChange);
    },

    requestContent: function(callback)
    {
        console.assert(typeof callback === "function");
        if (typeof callback !== "function")
            return;

        this._pendingContentRequestCallbacks.push(callback);

        if (this._contentReceived) {
            // Call _servicePendingContentRequests on a timeout to force callbacks to be asynchronous.
            if (!this._servicePendingContentRequestsTimeoutIdentifier)
                this._servicePendingContentRequestsTimeoutIdentifier = setTimeout(this.servicePendingContentRequests.bind(this), 0);
        } else if (this.canRequestContentFromBackend())
            this.requestContentFromBackendIfNeeded();
    },

    createSourceCodeLocation: function(lineNumber, columnNumber)
    {
        return new WebInspector.SourceCodeLocation(this, lineNumber, columnNumber);
    },

    createSourceCodeTextRange: function(textRange)
    {
        return new WebInspector.SourceCodeTextRange(this, textRange);
    },

    // Protected

    revisionContentDidChange: function(revision)
    {
        if (this._ignoreRevisionContentDidChangeEvent)
            return;

        if (revision !== this._currentRevision)
            return;

        this.handleCurrentRevisionContentChange();

        this.dispatchEventToListeners(WebInspector.SourceCode.Event.ContentDidChange);
    },

    handleCurrentRevisionContentChange: function()
    {
        // Implemented by subclasses if needed.
    },

    get revisionForRequestedContent()
    {
        // Implemented by subclasses if needed.
        return this._originalRevision;
    },

    markContentAsStale: function()
    {
        this._contentReceived = false;
    },

    canRequestContentFromBackend: function()
    {
        // Implemented by subclasses.
        console.error("Needs to be implemented by a subclass.");
        return false;
    },

    requestContentFromBackend: function(callback)
    {
        // Implemented by subclasses.
        console.error("Needs to be implemented by a subclass.");
    },

    requestContentFromBackendIfNeeded: function()
    {
        console.assert(this.canRequestContentFromBackend());
        if (!this.canRequestContentFromBackend())
            return;

        if (!this._pendingContentRequestCallbacks.length)
            return;

        if (this._contentRequestResponsePending)
            return;

        this._contentRequestResponsePending = true;

        if (this.requestContentFromBackend(this._processContent.bind(this)))
            return;

        // Since requestContentFromBackend returned false, just call _processContent,
        // which will cause the pending callbacks to get null content.
        this._processContent();
    },

    servicePendingContentRequests: function(force)
    {
        if (this._servicePendingContentRequestsTimeoutIdentifier) {
            clearTimeout(this._servicePendingContentRequestsTimeoutIdentifier);
            delete this._servicePendingContentRequestsTimeoutIdentifier;
        }

        // Force the content requests to be sent. To do this correctly we also force
        // _contentReceived to be true so future calls to requestContent go through.
        if (force)
            this._contentReceived = true;

        console.assert(this._contentReceived);
        if (!this._contentReceived)
            return;

        // Move the callbacks into a local and clear _pendingContentRequestCallbacks so
        // callbacks that might call requestContent again will not modify the array.
        var callbacks = this._pendingContentRequestCallbacks;
        this._pendingContentRequestCallbacks = [];

        for (var i = 0; i < callbacks.length; ++i)
            callbacks[i](this, this.content, this.contentIsBase64Encoded);
    },

    // Private

    _processContent: function(error, content, base64Encoded)
    {
        if (error)
            console.error(error);

        this._contentRequestResponsePending = false;
        this._contentReceived = true;

        var revision = this.revisionForRequestedContent;

        this._ignoreRevisionContentDidChangeEvent = true;
        revision.content = content || null;
        revision.contentIsBase64Encoded = base64Encoded || false;
        delete this._ignoreRevisionContentDidChangeEvent;

        this.servicePendingContentRequests();
    }
};

WebInspector.SourceCode.prototype.__proto__ = WebInspector.Object.prototype;
