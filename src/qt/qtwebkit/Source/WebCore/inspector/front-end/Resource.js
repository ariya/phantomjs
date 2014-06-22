/*
 * Copyright (C) 2007, 2008 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.Object}
 * @implements {WebInspector.ContentProvider}
 * @param {?WebInspector.NetworkRequest} request
 * @param {string} url
 * @param {string} documentURL
 * @param {NetworkAgent.FrameId} frameId
 * @param {NetworkAgent.LoaderId} loaderId
 * @param {WebInspector.ResourceType} type
 * @param {string} mimeType
 * @param {boolean=} isHidden
 */
WebInspector.Resource = function(request, url, documentURL, frameId, loaderId, type, mimeType, isHidden)
{
    this._request = request;
    this.url = url;
    this._documentURL = documentURL;
    this._frameId = frameId;
    this._loaderId = loaderId;
    this._type = type || WebInspector.resourceTypes.Other;
    this._mimeType = mimeType;
    this._isHidden = isHidden;

    /** @type {?string} */ this._content;
    /** @type {boolean} */ this._contentEncoded;
    this._pendingContentCallbacks = [];
    if (this._request && !this._request.finished)
        this._request.addEventListener(WebInspector.NetworkRequest.Events.FinishedLoading, this._requestFinished, this);
}

WebInspector.Resource.Events = {
    MessageAdded: "message-added",
    MessagesCleared: "messages-cleared",
}

WebInspector.Resource.prototype = {
    /**
     * @return {?WebInspector.NetworkRequest}
     */
    get request()
    {
        return this._request;
    },

    /**
     * @return {string}
     */
    get url()
    {
        return this._url;
    },

    set url(x)
    {
        this._url = x;
        this._parsedURL = new WebInspector.ParsedURL(x);
    },

    get parsedURL()
    {
        return this._parsedURL;
    },

    /**
     * @return {string}
     */
    get documentURL()
    {
        return this._documentURL;
    },

    /**
     * @return {NetworkAgent.FrameId}
     */
    get frameId()
    {
        return this._frameId;
    },

    /**
     * @return {NetworkAgent.LoaderId}
     */
    get loaderId()
    {
        return this._loaderId;
    },

    /**
     * @return {string}
     */
    get displayName()
    {
        return this._parsedURL.displayName;
    },

    /**
     * @return {WebInspector.ResourceType}
     */
    get type()
    {
        return this._request ? this._request.type : this._type;
    },

    /**
     * @return {string}
     */
    get mimeType()
    {
        return this._request ? this._request.mimeType : this._mimeType;
    },

    /**
     * @return {Array.<WebInspector.ConsoleMessage>}
     */
    get messages()
    {
        return this._messages || [];
    },

    /**
     * @param {WebInspector.ConsoleMessage} msg
     */
    addMessage: function(msg)
    {
        if (!msg.isErrorOrWarning() || !msg.message)
            return;

        if (!this._messages)
            this._messages = [];
        this._messages.push(msg);
        this.dispatchEventToListeners(WebInspector.Resource.Events.MessageAdded, msg);
    },

    /**
     * @return {number}
     */
    get errors()
    {
        return this._errors || 0;
    },

    set errors(x)
    {
        this._errors = x;
    },

    /**
     * @return {number}
     */
    get warnings()
    {
        return this._warnings || 0;
    },

    set warnings(x)
    {
        this._warnings = x;
    },

    clearErrorsAndWarnings: function()
    {
        this._messages = [];
        this._warnings = 0;
        this._errors = 0;
        this.dispatchEventToListeners(WebInspector.Resource.Events.MessagesCleared);
    },

    /**
     * @return {?string}
     */
    get content()
    {
        return this._content;
    },

    /**
     * @return {boolean}
     */
    get contentEncoded()
    {
        return this._contentEncoded;
    },

    /**
     * @return {string}
     */
    contentURL: function()
    {
        return this._url;
    },

    /**
     * @return {WebInspector.ResourceType}
     */
    contentType: function()
    {
        return this.type;
    },

    /**
     * @param {function(?string, boolean, string)} callback
     */
    requestContent: function(callback)
    {
        if (typeof this._content !== "undefined") {
            callback(this._content, !!this._contentEncoded, this.canonicalMimeType());
            return;
        }

        this._pendingContentCallbacks.push(callback);
        if (!this._request || this._request.finished)
            this._innerRequestContent();
    },

    canonicalMimeType: function()
    {
        return this.type.canonicalMimeType() || this.mimeType;
    },

    /**
     * @param {string} query
     * @param {boolean} caseSensitive
     * @param {boolean} isRegex
     * @param {function(Array.<WebInspector.ContentProvider.SearchMatch>)} callback
     */
    searchInContent: function(query, caseSensitive, isRegex, callback)
    {
        /**
         * @param {?Protocol.Error} error
         * @param {Array.<PageAgent.SearchMatch>} searchMatches
         */
        function callbackWrapper(error, searchMatches)
        {
            callback(searchMatches || []);
        }

        if (this.frameId)
            PageAgent.searchInResource(this.frameId, this.url, query, caseSensitive, isRegex, callbackWrapper);
        else
            callback([]);
    },

    /**
     * @param {Element} image
     */
    populateImageSource: function(image)
    {
        function onResourceContent()
        {
            var imageSrc = WebInspector.contentAsDataURL(this._content, this.mimeType, this._contentEncoded);
            if (imageSrc === null)
                imageSrc = this.url;
            image.src = imageSrc;
        }

        this.requestContent(onResourceContent.bind(this));
    },

    _requestFinished: function()
    {
        this._request.removeEventListener(WebInspector.NetworkRequest.Events.FinishedLoading, this._requestFinished, this);
        if (this._pendingContentCallbacks.length)
            this._innerRequestContent();
    },


    _innerRequestContent: function()
    {
        if (this._contentRequested)
            return;
        this._contentRequested = true;

        /**
         * @param {?string} content
         * @param {boolean} contentEncoded
         */
        function contentLoaded(content, contentEncoded)
        {
            this._content = content;
            this._contentEncoded = contentEncoded;
            var callbacks = this._pendingContentCallbacks.slice();
            for (var i = 0; i < callbacks.length; ++i)
                callbacks[i](this._content, this._contentEncoded, this.canonicalMimeType());
            this._pendingContentCallbacks.length = 0;
            delete this._contentRequested;
        }

        /**
         * @param {?Protocol.Error} error
         * @param {string} content
         * @param {boolean} contentEncoded
         */
        function resourceContentLoaded(error, content, contentEncoded)
        {
            if (error)
                console.error("Resource content request failed: " + error);
            contentLoaded.call(this, error ? null : content, contentEncoded);
        }
        
        if (this.request) {
            /**
             * @param {?string} content
             * @param {boolean} contentEncoded
             * @param {string} mimeType
             */
            function requestContentLoaded(content, contentEncoded, mimeType)
            {
                contentLoaded.call(this, content, contentEncoded);
            }
            
            this.request.requestContent(requestContentLoaded.bind(this));
            return;
        }
        PageAgent.getResourceContent(this.frameId, this.url, resourceContentLoaded.bind(this));
    },

    /**
     * @return {boolean}
     */
    isHidden: function()
    {
        return !!this._isHidden;
    },

    __proto__: WebInspector.Object.prototype
}

