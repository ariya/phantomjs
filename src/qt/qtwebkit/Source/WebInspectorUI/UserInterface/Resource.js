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

WebInspector.Resource = function(url, mimeType, type, loaderIdentifier, requestIdentifier, requestMethod, requestHeaders, requestData, requestSentTimestamp)
{
    WebInspector.SourceCode.call(this);

    console.assert(url);

    if (type in WebInspector.Resource.Type)
        type = WebInspector.Resource.Type[type];

    this._url = url;
    this._mimeType = mimeType;
    this._type = type || WebInspector.Resource.Type.fromMIMEType(mimeType);
    this._loaderIdentifier = loaderIdentifier || null;
    this._requestIdentifier = requestIdentifier || null;
    this._requestMethod = requestMethod || null;
    this._requestData = requestData || null;
    this._requestHeaders = requestHeaders || {};
    this._responseHeaders = {};
    this._parentFrame = null;
    this._requestSentTimestamp = requestSentTimestamp || NaN;
    this._responseReceivedTimestamp = NaN;
    this._lastRedirectReceivedTimestamp = NaN;
    this._lastDataReceivedTimestamp = NaN;
    this._finishedOrFailedTimestamp = NaN;
    this._size = NaN;
    this._transferSize = NaN;
    this._cached = false;
};

WebInspector.Object.addConstructorFunctions(WebInspector.Resource);

WebInspector.Resource.Event = {
    URLDidChange: "resource-url-did-change",
    MIMETypeDidChange: "resource-mime-type-did-change",
    TypeDidChange: "resource-type-did-change",
    RequestHeadersDidChange: "resource-request-headers-did-change",
    ResponseReceived: "resource-response-received",
    LoadingDidFinish: "resource-loading-did-finish",
    LoadingDidFail: "resource-loading-did-fail",
    TimestampsDidChange: "resource-timestamps-did-change",
    SizeDidChange: "resource-size-did-change",
    TransferSizeDidChange: "resource-transfer-size-did-change",
    CacheStatusDidChange: "resource-cached-did-change"
};

// Keep these in sync with the "ResourceType" enum defined by the "Page" domain (see WebCore/inspector/Inspector.json).
WebInspector.Resource.Type = {
    Document: "resource-type-document",
    Stylesheet: "resource-type-stylesheet",
    Image: "resource-type-image",
    Font: "resource-type-font",
    Script: "resource-type-script",
    XHR: "resource-type-xhr",
    WebSocket: "resource-type-websocket",
    Other: "resource-type-other"
};

// This MIME Type map is private, use WebInspector.Resource.Type.fromMIMEType().
WebInspector.Resource.Type._mimeTypeMap = {
    "text/html": WebInspector.Resource.Type.Document,
    "text/xml": WebInspector.Resource.Type.Document,
    "text/plain": WebInspector.Resource.Type.Document,
    "application/xhtml+xml": WebInspector.Resource.Type.Document,
    "image/svg+xml": WebInspector.Resource.Type.Document,

    "text/css": WebInspector.Resource.Type.Stylesheet,
    "text/xsl": WebInspector.Resource.Type.Stylesheet,
    "text/x-less": WebInspector.Resource.Type.Stylesheet,
    "text/x-sass": WebInspector.Resource.Type.Stylesheet,
    "text/x-scss": WebInspector.Resource.Type.Stylesheet,

    "application/pdf": WebInspector.Resource.Type.Image,

    "application/x-font-type1": WebInspector.Resource.Type.Font,
    "application/x-font-ttf": WebInspector.Resource.Type.Font,
    "application/x-font-woff": WebInspector.Resource.Type.Font,
    "application/x-truetype-font": WebInspector.Resource.Type.Font,

    "text/javascript": WebInspector.Resource.Type.Script,
    "text/ecmascript": WebInspector.Resource.Type.Script,
    "application/javascript": WebInspector.Resource.Type.Script,
    "application/ecmascript": WebInspector.Resource.Type.Script,
    "application/x-javascript": WebInspector.Resource.Type.Script,
    "application/json": WebInspector.Resource.Type.Script,
    "application/x-json": WebInspector.Resource.Type.Script,
    "text/x-javascript": WebInspector.Resource.Type.Script,
    "text/x-json": WebInspector.Resource.Type.Script,
    "text/javascript1.1": WebInspector.Resource.Type.Script,
    "text/javascript1.2": WebInspector.Resource.Type.Script,
    "text/javascript1.3": WebInspector.Resource.Type.Script,
    "text/jscript": WebInspector.Resource.Type.Script,
    "text/livescript": WebInspector.Resource.Type.Script,
    "text/x-livescript": WebInspector.Resource.Type.Script,
    "text/typescript": WebInspector.Resource.Type.Script,
    "text/x-clojure": WebInspector.Resource.Type.Script,
    "text/x-coffeescript": WebInspector.Resource.Type.Script
};

WebInspector.Resource.Type.fromMIMEType = function(mimeType)
{
    if (!mimeType)
        return WebInspector.Resource.Type.Other;

    if (mimeType in WebInspector.Resource.Type._mimeTypeMap)
        return WebInspector.Resource.Type._mimeTypeMap[mimeType];

    if (mimeType.startsWith("image/"))
        return WebInspector.Resource.Type.Image;

    if (mimeType.startsWith("font/"))
        return WebInspector.Resource.Type.Font;

    return WebInspector.Resource.Type.Other;
};

WebInspector.Resource.Type.displayName = function(type, plural)
{
    switch(type) {
    case WebInspector.Resource.Type.Document:
        if (plural)
            return WebInspector.UIString("Documents");
        return WebInspector.UIString("Document");
    case WebInspector.Resource.Type.Stylesheet:
        if (plural)
            return WebInspector.UIString("Stylesheets");
        return WebInspector.UIString("Stylesheet");
    case WebInspector.Resource.Type.Image:
        if (plural)
            return WebInspector.UIString("Images");
        return WebInspector.UIString("Image");
    case WebInspector.Resource.Type.Font:
        if (plural)
            return WebInspector.UIString("Fonts");
        return WebInspector.UIString("Font");
    case WebInspector.Resource.Type.Script:
        if (plural)
            return WebInspector.UIString("Scripts");
        return WebInspector.UIString("Script");
    case WebInspector.Resource.Type.XHR:
        if (plural)
            return WebInspector.UIString("XHRs");
        return WebInspector.UIString("XHR");
    case WebInspector.Resource.Type.WebSocket:
        if (plural)
            return WebInspector.UIString("Sockets");
        return WebInspector.UIString("Socket");
    case WebInspector.Resource.Type.Other:
        return WebInspector.UIString("Other");
    default:
        console.error("Unknown resource type: ", type);
        return null;
    }
};

WebInspector.Resource.prototype = {
    constructor: WebInspector.Resource,

    // Public

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
        return WebInspector.displayNameForURL(this._url, this.urlComponents);
    },

    get type()
    {
        return this._type;
    },

    get mimeType()
    {
        return this._mimeType;
    },

    get syntheticMIMEType()
    {
        // Resources are often transferred with a MIME-type that doesn't match the purpose the
        // resource was loaded for, which is what WebInspector.Resource.Type represents.
        // This getter generates a MIME-type, if needed, that matches the resource type.

        // If the type matches the Resource.Type of the MIME-type, then return the actual MIME-type.
        if (this._type === WebInspector.Resource.Type.fromMIMEType(this._mimeType))
            return this._mimeType;

        // Return the default MIME-types for the Resource.Type, since the current MIME-type
        // does not match what is expected for the Resource.Type.
        switch (this._type) {
        case WebInspector.Resource.Type.Document:
            return "text/html";
        case WebInspector.Resource.Type.Stylesheet:
            return "text/css";
        case WebInspector.Resource.Type.Script:
            return "text/javascript";
        }

        // Return the actual MIME-type since we don't have a better synthesized one to return.
        return this._mimeType;
    },

    get contentURL()
    {
        const maximumDataURLSize = 1024 * 1024; // 1 MiB

        // If content is not available or won't fit a data URL, fallback to using original URL.
        var content = this.content;
        if (content === null || content.length > maximumDataURLSize)
            return this._url;

        return "data:" + this._mimeType + (this.contentIsBase64Encoded ? ";base64," + content : "," + encodeURIComponent(content));
    },

    isMainResource: function()
    {
        return this._parentFrame ? this._parentFrame.mainResource === this : false;
    },

    get parentFrame()
    {
        return this._parentFrame;
    },

    get loaderIdentifier()
    {
        return this._loaderIdentifier;
    },

    get requestIdentifier()
    {
        return this._requestIdentifier;
    },

    get finished()
    {
        return this._finished;
    },

    get failed()
    {
        return this._failed;
    },

    get canceled()
    {
        return this._canceled;
    },

    get requestMethod()
    {
        return this._requestMethod;
    },

    get requestData()
    {
        return this._requestData;
    },

    get requestDataContentType()
    {
        return this._requestHeaders.valueForCaseInsensitiveKey("Content-Type") || null;
    },

    get requestHeaders()
    {
        return this._requestHeaders;
    },

    get responseHeaders()
    {
        return this._responseHeaders;
    },

    get requestSentTimestamp()
    {
        return this._requestSentTimestamp;
    },

    get lastRedirectReceivedTimestamp()
    {
        return this._lastRedirectReceivedTimestamp;
    },

    get responseReceivedTimestamp()
    {
        return this._responseReceivedTimestamp;
    },

    get lastDataReceivedTimestamp()
    {
        return this._lastDataReceivedTimestamp;
    },

    get finishedOrFailedTimestamp()
    {
        return this._finishedOrFailedTimestamp;
    },

    get firstTimestamp()
    {
        return this.requestSentTimestamp || this.lastRedirectReceivedTimestamp || this.responseReceivedTimestamp || this.lastDataReceivedTimestamp || this.finishedOrFailedTimestamp;
    },

    get lastTimestamp()
    {
        return this.finishedOrFailedTimestamp || this.lastDataReceivedTimestamp || this.responseReceivedTimestamp || this.lastRedirectReceivedTimestamp || this.requestSentTimestamp;
    },

    get duration()
    {
        return this._finishedOrFailedTimestamp - this._requestSentTimestamp;
    },

    get latency()
    {
        return this._responseReceivedTimestamp - this._requestSentTimestamp;
    },

    get receiveDuration()
    {
        return this._finishedOrFailedTimestamp - this._responseReceivedTimestamp;
    },

    get cached()
    {
        return this._cached;
    },

    get statusCode()
    {
        return this._statusCode;
    },

    get statusText()
    {
        return this._statusText;
    },

    get size()
    {
        return this._size;
    },

    get encodedSize()
    {
        if (!isNaN(this._transferSize))
            return this._transferSize;

        // If we did not receive actual transfer size from network
        // stack, we prefer using Content-Length over resourceSize as
        // resourceSize may differ from actual transfer size if platform's
        // network stack performed decoding (e.g. gzip decompression).
        // The Content-Length, though, is expected to come from raw
        // response headers and will reflect actual transfer length.
        // This won't work for chunked content encoding, so fall back to
        // resourceSize when we don't have Content-Length. This still won't
        // work for chunks with non-trivial encodings. We need a way to
        // get actual transfer size from the network stack.

        return Number(this._responseHeaders.valueForCaseInsensitiveKey("Content-Length") || this._size);
    },

    get transferSize()
    {
        if (this.statusCode === 304) // Not modified
            return this._responseHeadersSize;

        if (this._cached)
            return 0;

        return this._responseHeadersSize + this.encodedSize;
    },

    get compressed()
    {
        var contentEncoding = this._responseHeaders.valueForCaseInsensitiveKey("Content-Encoding");
        return contentEncoding && /\b(?:gzip|deflate)\b/.test(contentEncoding);
    },

    get scripts()
    {
        return this._scripts || [];
    },

    updateForRedirectResponse: function(url, requestHeaders, timestamp)
    {
        console.assert(!this._finished);
        console.assert(!this._failed);
        console.assert(!this._canceled);

        var oldURL = this._url;

        this._url = url;
        this._requestHeaders = requestHeaders || {};
        this._lastRedirectReceivedTimestamp = timestamp || NaN;

        if (oldURL !== url) {
            // Delete the URL components so the URL is re-parsed the next time it is requested.
            delete this._urlComponents;

            this.dispatchEventToListeners(WebInspector.Resource.Event.URLDidChange, {oldURL: oldURL});
        }

        this.dispatchEventToListeners(WebInspector.Resource.Event.RequestHeadersDidChange);
        this.dispatchEventToListeners(WebInspector.Resource.Event.TimestampsDidChange);
    },

    updateForResponse: function(url, mimeType, type, responseHeaders, statusCode, statusText, timestamp)
    {
        console.assert(!this._finished);
        console.assert(!this._failed);
        console.assert(!this._canceled);

        var oldURL = this._url;
        var oldMIMEType = this._mimeType;
        var oldType = this._type;

        if (type in WebInspector.Resource.Type)
            type = WebInspector.Resource.Type[type];

        this._url = url;
        this._mimeType = mimeType;
        this._type = type || WebInspector.Resource.Type.fromMIMEType(mimeType);
        this._statusCode = statusCode;
        this._statusText = statusText;
        this._responseHeaders = responseHeaders || {};
        this._responseReceivedTimestamp = timestamp || NaN;

        this._responseHeadersSize = String(this._statusCode).length + this._statusText.length + 12; // Extra length is for "HTTP/1.1 ", " ", and "\r\n".
        for (var name in this._responseHeaders)
            this._responseHeadersSize += name.length + this._responseHeaders[name].length + 4; // Extra length is for ": ", and "\r\n".

        if (statusCode === 304 && !this._cached)
            this.markAsCached();

        if (oldURL !== url) {
            // Delete the URL components so the URL is re-parsed the next time it is requested.
            delete this._urlComponents;

            this.dispatchEventToListeners(WebInspector.Resource.Event.URLDidChange, {oldURL: oldURL});
        }

        if (oldMIMEType !== mimeType)
            this.dispatchEventToListeners(WebInspector.Resource.Event.MIMETypeDidChange, {oldMIMEType: oldMIMEType});

        if (oldType !== type)
            this.dispatchEventToListeners(WebInspector.Resource.Event.TypeDidChange, {oldType: oldType});

        console.assert(isNaN(this._size));
        console.assert(isNaN(this._transferSize));

        // The transferSize becomes 0 when status is 304 or Content-Length is available, so
        // notify listeners of that change.
        if (statusCode === 304 || this._responseHeaders.valueForCaseInsensitiveKey("Content-Length"))
            this.dispatchEventToListeners(WebInspector.Resource.Event.TransferSizeDidChange);

        this.dispatchEventToListeners(WebInspector.Resource.Event.ResponseReceived);
        this.dispatchEventToListeners(WebInspector.Resource.Event.TimestampsDidChange);
    },

    canRequestContentFromBackend: function()
    {
        return this._finished;
    },

    requestContentFromBackend: function(callback)
    {
        // If we have the requestIdentifier we can get the actual response for this specific resource.
        // Otherwise the content will be cached resource data, which might not exist anymore.
        if (this._requestIdentifier) {
            NetworkAgent.getResponseBody(this._requestIdentifier, callback);
            return true;
        }

        if (this._parentFrame) {
            PageAgent.getResourceContent(this._parentFrame.id, this._url, callback);
            return true;
        }

        // There is no request identifier or frame to request content from. Return false to cause the
        // pending callbacks to get null content.
        return false;
    },

    increaseSize: function(dataLength, timestamp)
    {
        console.assert(dataLength >= 0);

        if (isNaN(this._size))
            this._size = 0;

        var previousSize = this._size;

        this._size += dataLength;

        this._lastDataReceivedTimestamp = timestamp || NaN;

        this.dispatchEventToListeners(WebInspector.Resource.Event.SizeDidChange, {previousSize: previousSize});

        // The transferSize is based off of size when status is not 304 or Content-Length is missing.
        if (isNaN(this._transferSize) && this._statusCode !== 304 && !this._responseHeaders.valueForCaseInsensitiveKey("Content-Length"))
            this.dispatchEventToListeners(WebInspector.Resource.Event.TransferSizeDidChange);
    },

    increaseTransferSize: function(encodedDataLength)
    {
        console.assert(encodedDataLength >= 0);

        if (isNaN(this._transferSize))
            this._transferSize = 0;
        this._transferSize += encodedDataLength;

        this.dispatchEventToListeners(WebInspector.Resource.Event.TransferSizeDidChange);
    },

    markAsCached: function()
    {
        this._cached = true;

        this.dispatchEventToListeners(WebInspector.Resource.Event.CacheStatusDidChange);

        // The transferSize is starts returning 0 when cached is true, unless status is 304.
        if (this._statusCode !== 304)
            this.dispatchEventToListeners(WebInspector.Resource.Event.TransferSizeDidChange);
    },

    markAsFinished: function(timestamp)
    {
        console.assert(!this._failed);
        console.assert(!this._canceled);

        this._finished = true;
        this._finishedOrFailedTimestamp = timestamp || NaN;

        this.dispatchEventToListeners(WebInspector.Resource.Event.LoadingDidFinish);
        this.dispatchEventToListeners(WebInspector.Resource.Event.TimestampsDidChange);

        if (this.canRequestContentFromBackend())
            this.requestContentFromBackendIfNeeded();
    },

    markAsFailed: function(canceled, timestamp)
    {
        console.assert(!this._finished);

        this._failed = true;
        this._canceled = canceled;
        this._finishedOrFailedTimestamp = timestamp || NaN;

        this.dispatchEventToListeners(WebInspector.Resource.Event.LoadingDidFail);
        this.dispatchEventToListeners(WebInspector.Resource.Event.TimestampsDidChange);

        // Force the content requests to be serviced. They will get null as the content.
        this.servicePendingContentRequests(true);
    },

    revertMarkAsFinished: function(timestamp)
    {
        console.assert(!this._failed);
        console.assert(!this._canceled);
        console.assert(this._finished);

        this._finished = false;
        this._finishedOrFailedTimestamp = NaN;
    },

    getImageSize: function(callback)
    {
        // Throw an error in the case this resource is not an image.
        if (this.type !== WebInspector.Resource.Type.Image)
            throw "Resource is not an image.";

        // See if we've already computed and cached the image size,
        // in which case we can provide them directly.
        if (this._imageSize) {
            callback(this._imageSize);
            return;
        }

        // Event handler for the image "load" event.
        function imageDidLoad()
        {
            // Cache the image metrics.
            this._imageSize = {
                width: image.width,
                height: image.height
            };
            
            callback(this._imageSize);
        };

        // Create an <img> element that we'll use to load the image resource
        // so that we can query its intrinsic size.
        var image = new Image;
        image.addEventListener("load", imageDidLoad.bind(this), false);

        // Set the image source once we've obtained the base64-encoded URL for it.
        this.requestContent(function() {
            image.src = this.contentURL;
        }.bind(this));
    },

    associateWithScript: function(script)
    {
        if (!this._scripts)
            this._scripts = []

        this._scripts.push(script);

        // COMPATIBILITY (iOS 6): Resources did not know their type until a response
        // was received. We can set the Resource type to be Script here.
        if (this._type === WebInspector.Resource.Type.Other) {
            var oldType = this._type;
            this._type = WebInspector.Resource.Type.Script;
            this.dispatchEventToListeners(WebInspector.Resource.Event.TypeDidChange, {oldType: oldType});
        }
    }
};

WebInspector.Resource.prototype.__proto__ = WebInspector.SourceCode.prototype;
