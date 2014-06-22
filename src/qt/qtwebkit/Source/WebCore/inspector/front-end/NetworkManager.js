/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * @extends {WebInspector.Object}
 */
WebInspector.NetworkManager = function()
{
    WebInspector.Object.call(this);
    this._dispatcher = new WebInspector.NetworkDispatcher(this);
    if (WebInspector.settings.cacheDisabled.get())
        NetworkAgent.setCacheDisabled(true);

    NetworkAgent.enable();

    WebInspector.settings.cacheDisabled.addChangeListener(this._cacheDisabledSettingChanged, this);
}

WebInspector.NetworkManager.EventTypes = {
    RequestStarted: "RequestStarted",
    RequestUpdated: "RequestUpdated",
    RequestFinished: "RequestFinished",
    RequestUpdateDropped: "RequestUpdateDropped"
}

WebInspector.NetworkManager._MIMETypes = {
    "text/html":                   {"document": true},
    "text/xml":                    {"document": true},
    "text/plain":                  {"document": true},
    "application/xhtml+xml":       {"document": true},
    "text/css":                    {"stylesheet": true},
    "text/xsl":                    {"stylesheet": true},
    "image/jpg":                   {"image": true},
    "image/jpeg":                  {"image": true},
    "image/pjpeg":                 {"image": true},
    "image/png":                   {"image": true},
    "image/gif":                   {"image": true},
    "image/bmp":                   {"image": true},
    "image/svg+xml":               {"image": true},
    "image/vnd.microsoft.icon":    {"image": true},
    "image/webp":                  {"image": true},
    "image/x-icon":                {"image": true},
    "image/x-xbitmap":             {"image": true},
    "font/ttf":                    {"font": true},
    "font/opentype":               {"font": true},
    "application/font-woff":       {"font": true},
    "application/x-font-type1":    {"font": true},
    "application/x-font-ttf":      {"font": true},
    "application/x-truetype-font": {"font": true},
    "text/javascript":             {"script": true},
    "text/ecmascript":             {"script": true},
    "application/javascript":      {"script": true},
    "application/ecmascript":      {"script": true},
    "application/x-javascript":    {"script": true},
    "application/json":            {"script": true},
    "text/javascript1.1":          {"script": true},
    "text/javascript1.2":          {"script": true},
    "text/javascript1.3":          {"script": true},
    "text/jscript":                {"script": true},
    "text/livescript":             {"script": true},
}

WebInspector.NetworkManager.prototype = {
    /**
     * @param {string} url
     * @return {WebInspector.NetworkRequest}
     */
    inflightRequestForURL: function(url)
    {
        return this._dispatcher._inflightRequestsByURL[url];
    },

    /**
     * @param {WebInspector.Event} event
     */
    _cacheDisabledSettingChanged: function(event)
    {
        var enabled = /** @type {boolean} */ (event.data);
        NetworkAgent.setCacheDisabled(enabled);
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 * @implements {NetworkAgent.Dispatcher}
 */
WebInspector.NetworkDispatcher = function(manager)
{
    this._manager = manager;
    this._inflightRequestsById = {};
    this._inflightRequestsByURL = {};
    InspectorBackend.registerNetworkDispatcher(this);
}

WebInspector.NetworkDispatcher.prototype = {
    /**
     * @param {NetworkAgent.Headers} headersMap
     * @return {!Array.<!WebInspector.NetworkRequest.NameValue>}
     */
    _headersMapToHeadersArray: function(headersMap)
    {
        var result = [];
        for (var name in headersMap) {
            var values = headersMap[name].split("\n");
            for (var i = 0; i < values.length; ++i)
                result.push({name: name, value: values[i]});
        }
        return result;
    },

    /**
     * @param {WebInspector.NetworkRequest} networkRequest
     * @param {NetworkAgent.Request} request
     */
    _updateNetworkRequestWithRequest: function(networkRequest, request)
    {
        networkRequest.requestMethod = request.method;
        networkRequest.requestHeaders = this._headersMapToHeadersArray(request.headers);
        networkRequest.requestFormData = request.postData;
    },

    /**
     * @param {WebInspector.NetworkRequest} networkRequest
     * @param {NetworkAgent.Response=} response
     */
    _updateNetworkRequestWithResponse: function(networkRequest, response)
    {
        if (!response)
            return;

        if (response.url && networkRequest.url !== response.url)
            networkRequest.url = response.url;
        networkRequest.mimeType = response.mimeType;
        networkRequest.statusCode = response.status;
        networkRequest.statusText = response.statusText;
        networkRequest.responseHeaders = this._headersMapToHeadersArray(response.headers);
        if (response.headersText)
            networkRequest.responseHeadersText = response.headersText;
        if (response.requestHeaders)
            networkRequest.requestHeaders = this._headersMapToHeadersArray(response.requestHeaders);
        if (response.requestHeadersText)
            networkRequest.requestHeadersText = response.requestHeadersText;

        networkRequest.connectionReused = response.connectionReused;
        networkRequest.connectionId = response.connectionId;

        if (response.fromDiskCache)
            networkRequest.cached = true;
        else
            networkRequest.timing = response.timing;

        if (!this._mimeTypeIsConsistentWithType(networkRequest)) {
            WebInspector.console.addMessage(WebInspector.ConsoleMessage.create(WebInspector.ConsoleMessage.MessageSource.Network,
                WebInspector.ConsoleMessage.MessageLevel.Log,
                WebInspector.UIString("Resource interpreted as %s but transferred with MIME type %s: \"%s\".", networkRequest.type.title(), networkRequest.mimeType, networkRequest.url),
                WebInspector.ConsoleMessage.MessageType.Log,
                "",
                0,
                1,
                [],
                null,
                networkRequest.requestId));
        }
    },

    /**
     * @param {WebInspector.NetworkRequest} networkRequest
     * @return {boolean}
     */
    _mimeTypeIsConsistentWithType: function(networkRequest)
    {
        // If status is an error, content is likely to be of an inconsistent type,
        // as it's going to be an error message. We do not want to emit a warning
        // for this, though, as this will already be reported as resource loading failure.
        // Also, if a URL like http://localhost/wiki/load.php?debug=true&lang=en produces text/css and gets reloaded,
        // it is 304 Not Modified and its guessed mime-type is text/php, which is wrong.
        // Don't check for mime-types in 304-resources.
        if (networkRequest.hasErrorStatusCode() || networkRequest.statusCode === 304 || networkRequest.statusCode === 204)
            return true;

        if (typeof networkRequest.type === "undefined"
            || networkRequest.type === WebInspector.resourceTypes.Other
            || networkRequest.type === WebInspector.resourceTypes.XHR
            || networkRequest.type === WebInspector.resourceTypes.WebSocket)
            return true;

        if (!networkRequest.mimeType)
            return true; // Might be not known for cached resources with null responses.

        if (networkRequest.mimeType in WebInspector.NetworkManager._MIMETypes)
            return networkRequest.type.name() in WebInspector.NetworkManager._MIMETypes[networkRequest.mimeType];

        return false;
    },

    /**
     * @param {WebInspector.NetworkRequest} networkRequest
     * @param {?NetworkAgent.CachedResource} cachedResource
     */
    _updateNetworkRequestWithCachedResource: function(networkRequest, cachedResource)
    {
        networkRequest.type = WebInspector.resourceTypes[cachedResource.type];
        networkRequest.resourceSize = cachedResource.bodySize;
        this._updateNetworkRequestWithResponse(networkRequest, cachedResource.response);
    },

    /**
     * @param {NetworkAgent.Response} response
     * @return {boolean}
     */
    _isNull: function(response)
    {
        if (!response)
            return true;
        return !response.status && !response.mimeType && (!response.headers || !Object.keys(response.headers).length);
    },

    /**
     * @param {NetworkAgent.RequestId} requestId
     * @param {NetworkAgent.FrameId} frameId
     * @param {NetworkAgent.LoaderId} loaderId
     * @param {string} documentURL
     * @param {NetworkAgent.Request} request
     * @param {NetworkAgent.Timestamp} time
     * @param {NetworkAgent.Initiator} initiator
     * @param {NetworkAgent.Response=} redirectResponse
     */
    requestWillBeSent: function(requestId, frameId, loaderId, documentURL, request, time, initiator, redirectResponse)
    {
        var networkRequest = this._inflightRequestsById[requestId];
        if (networkRequest) {
            // FIXME: move this check to the backend.
            if (!redirectResponse)
                return;
            this.responseReceived(requestId, frameId, loaderId, time, PageAgent.ResourceType.Other, redirectResponse);
            networkRequest = this._appendRedirect(requestId, time, request.url);
        } else
            networkRequest = this._createNetworkRequest(requestId, frameId, loaderId, request.url, documentURL, initiator);
        networkRequest.hasNetworkData = true;
        this._updateNetworkRequestWithRequest(networkRequest, request);
        networkRequest.startTime = time;

        this._startNetworkRequest(networkRequest);
    },

    /**
     * @param {NetworkAgent.RequestId} requestId
     */
    requestServedFromCache: function(requestId)
    {
        var networkRequest = this._inflightRequestsById[requestId];
        if (!networkRequest)
            return;

        networkRequest.cached = true;
    },

    /**
     * @param {NetworkAgent.RequestId} requestId
     * @param {NetworkAgent.FrameId} frameId
     * @param {NetworkAgent.LoaderId} loaderId
     * @param {NetworkAgent.Timestamp} time
     * @param {PageAgent.ResourceType} resourceType
     * @param {NetworkAgent.Response} response
     */
    responseReceived: function(requestId, frameId, loaderId, time, resourceType, response)
    {
        // FIXME: move this check to the backend.
        if (this._isNull(response))
            return;

        var networkRequest = this._inflightRequestsById[requestId];
        if (!networkRequest) {
            // We missed the requestWillBeSent.
            var eventData = {};
            eventData.url = response.url;
            eventData.frameId = frameId;
            eventData.loaderId = loaderId;
            eventData.resourceType = resourceType;
            eventData.mimeType = response.mimeType;
            this._manager.dispatchEventToListeners(WebInspector.NetworkManager.EventTypes.RequestUpdateDropped, eventData);
            return;
        }

        networkRequest.responseReceivedTime = time;
        networkRequest.type = WebInspector.resourceTypes[resourceType];

        this._updateNetworkRequestWithResponse(networkRequest, response);

        this._updateNetworkRequest(networkRequest);
    },

    /**
     * @param {NetworkAgent.RequestId} requestId
     * @param {NetworkAgent.Timestamp} time
     * @param {number} dataLength
     * @param {number} encodedDataLength
     */
    dataReceived: function(requestId, time, dataLength, encodedDataLength)
    {
        var networkRequest = this._inflightRequestsById[requestId];
        if (!networkRequest)
            return;

        networkRequest.resourceSize += dataLength;
        if (encodedDataLength != -1)
            networkRequest.increaseTransferSize(encodedDataLength);
        networkRequest.endTime = time;

        this._updateNetworkRequest(networkRequest);
    },

    /**
     * @param {NetworkAgent.RequestId} requestId
     * @param {NetworkAgent.Timestamp} finishTime
     */
    loadingFinished: function(requestId, finishTime)
    {
        var networkRequest = this._inflightRequestsById[requestId];
        if (!networkRequest)
            return;
        this._finishNetworkRequest(networkRequest, finishTime);
    },

    /**
     * @param {NetworkAgent.RequestId} requestId
     * @param {NetworkAgent.Timestamp} time
     * @param {string} localizedDescription
     * @param {boolean=} canceled
     */
    loadingFailed: function(requestId, time, localizedDescription, canceled)
    {
        var networkRequest = this._inflightRequestsById[requestId];
        if (!networkRequest)
            return;

        networkRequest.failed = true;
        networkRequest.canceled = canceled;
        networkRequest.localizedFailDescription = localizedDescription;
        this._finishNetworkRequest(networkRequest, time);
    },

    /**
     * @param {NetworkAgent.RequestId} requestId
     * @param {NetworkAgent.FrameId} frameId
     * @param {NetworkAgent.LoaderId} loaderId
     * @param {string} documentURL
     * @param {NetworkAgent.Timestamp} time
     * @param {NetworkAgent.Initiator} initiator
     * @param {NetworkAgent.CachedResource} cachedResource
     */
    requestServedFromMemoryCache: function(requestId, frameId, loaderId, documentURL, time, initiator, cachedResource)
    {
        var networkRequest = this._createNetworkRequest(requestId, frameId, loaderId, cachedResource.url, documentURL, initiator);
        this._updateNetworkRequestWithCachedResource(networkRequest, cachedResource);
        networkRequest.cached = true;
        networkRequest.requestMethod = "GET";
        this._startNetworkRequest(networkRequest);
        networkRequest.startTime = networkRequest.responseReceivedTime = time;
        this._finishNetworkRequest(networkRequest, time);
    },

    /**
     * @param {NetworkAgent.RequestId} requestId
     * @param {string} requestURL
     */
    webSocketCreated: function(requestId, requestURL)
    {
        var networkRequest = new WebInspector.NetworkRequest(requestId, requestURL, "", "", "");
        networkRequest.type = WebInspector.resourceTypes.WebSocket;
        this._startNetworkRequest(networkRequest);
    },

    /**
     * @param {NetworkAgent.RequestId} requestId
     * @param {NetworkAgent.Timestamp} time
     * @param {NetworkAgent.WebSocketRequest} request
     */
    webSocketWillSendHandshakeRequest: function(requestId, time, request)
    {
        var networkRequest = this._inflightRequestsById[requestId];
        if (!networkRequest)
            return;

        networkRequest.requestMethod = "GET";
        networkRequest.requestHeaders = this._headersMapToHeadersArray(request.headers);
        networkRequest.startTime = time;

        this._updateNetworkRequest(networkRequest);
    },

    /**
     * @param {NetworkAgent.RequestId} requestId
     * @param {NetworkAgent.Timestamp} time
     * @param {NetworkAgent.WebSocketResponse} response
     */
    webSocketHandshakeResponseReceived: function(requestId, time, response)
    {
        var networkRequest = this._inflightRequestsById[requestId];
        if (!networkRequest)
            return;

        networkRequest.statusCode = response.status;
        networkRequest.statusText = response.statusText;
        networkRequest.responseHeaders = this._headersMapToHeadersArray(response.headers);
        networkRequest.responseReceivedTime = time;

        this._updateNetworkRequest(networkRequest);
    },

    /**
     * @param {NetworkAgent.RequestId} requestId
     * @param {NetworkAgent.Timestamp} time
     * @param {NetworkAgent.WebSocketFrame} response
     */
    webSocketFrameReceived: function(requestId, time, response)
    {
        var networkRequest = this._inflightRequestsById[requestId];
        if (!networkRequest)
            return;

        networkRequest.addFrame(response, time);
        networkRequest.responseReceivedTime = time;

        this._updateNetworkRequest(networkRequest);
    },

    /**
     * @param {NetworkAgent.RequestId} requestId
     * @param {NetworkAgent.Timestamp} time
     * @param {NetworkAgent.WebSocketFrame} response
     */
    webSocketFrameSent: function(requestId, time, response)
    {
        var networkRequest = this._inflightRequestsById[requestId];
        if (!networkRequest)
            return;

        networkRequest.addFrame(response, time, true);
        networkRequest.responseReceivedTime = time;

        this._updateNetworkRequest(networkRequest);
    },

    /**
     * @param {NetworkAgent.RequestId} requestId
     * @param {NetworkAgent.Timestamp} time
     * @param {string} errorMessage
     */
    webSocketFrameError: function(requestId, time, errorMessage)
    {
        var networkRequest = this._inflightRequestsById[requestId];
        if (!networkRequest)
            return;

        networkRequest.addFrameError(errorMessage, time);
        networkRequest.responseReceivedTime = time;

        this._updateNetworkRequest(networkRequest);
    },

    /**
     * @param {NetworkAgent.RequestId} requestId
     * @param {NetworkAgent.Timestamp} time
     */
    webSocketClosed: function(requestId, time)
    {
        var networkRequest = this._inflightRequestsById[requestId];
        if (!networkRequest)
            return;
        this._finishNetworkRequest(networkRequest, time);
    },

    /**
     * @param {NetworkAgent.RequestId} requestId
     * @param {NetworkAgent.Timestamp} time
     * @param {string} redirectURL
     * @return {WebInspector.NetworkRequest}
     */
    _appendRedirect: function(requestId, time, redirectURL)
    {
        var originalNetworkRequest = this._inflightRequestsById[requestId];
        var previousRedirects = originalNetworkRequest.redirects || [];
        originalNetworkRequest.requestId = "redirected:" + requestId + "." + previousRedirects.length;
        delete originalNetworkRequest.redirects;
        if (previousRedirects.length > 0)
            originalNetworkRequest.redirectSource = previousRedirects[previousRedirects.length - 1];
        this._finishNetworkRequest(originalNetworkRequest, time);
        var newNetworkRequest = this._createNetworkRequest(requestId, originalNetworkRequest.frameId, originalNetworkRequest.loaderId,
             redirectURL, originalNetworkRequest.documentURL, originalNetworkRequest.initiator);
        newNetworkRequest.redirects = previousRedirects.concat(originalNetworkRequest);
        return newNetworkRequest;
    },

    /**
     * @param {WebInspector.NetworkRequest} networkRequest
     */
    _startNetworkRequest: function(networkRequest)
    {
        this._inflightRequestsById[networkRequest.requestId] = networkRequest;
        this._inflightRequestsByURL[networkRequest.url] = networkRequest;
        this._dispatchEventToListeners(WebInspector.NetworkManager.EventTypes.RequestStarted, networkRequest);
    },

    /**
     * @param {WebInspector.NetworkRequest} networkRequest
     */
    _updateNetworkRequest: function(networkRequest)
    {
        this._dispatchEventToListeners(WebInspector.NetworkManager.EventTypes.RequestUpdated, networkRequest);
    },

    /**
     * @param {WebInspector.NetworkRequest} networkRequest
     * @param {NetworkAgent.Timestamp} finishTime
     */
    _finishNetworkRequest: function(networkRequest, finishTime)
    {
        networkRequest.endTime = finishTime;
        networkRequest.finished = true;
        this._dispatchEventToListeners(WebInspector.NetworkManager.EventTypes.RequestFinished, networkRequest);
        delete this._inflightRequestsById[networkRequest.requestId];
        delete this._inflightRequestsByURL[networkRequest.url];
    },

    /**
     * @param {string} eventType
     * @param {WebInspector.NetworkRequest} networkRequest
     */
    _dispatchEventToListeners: function(eventType, networkRequest)
    {
        this._manager.dispatchEventToListeners(eventType, networkRequest);
    },

    /**
     * @param {NetworkAgent.RequestId} requestId
     * @param {string} frameId
     * @param {NetworkAgent.LoaderId} loaderId
     * @param {string} url
     * @param {string} documentURL
     * @param {NetworkAgent.Initiator} initiator
     */
    _createNetworkRequest: function(requestId, frameId, loaderId, url, documentURL, initiator)
    {
        var networkRequest = new WebInspector.NetworkRequest(requestId, url, documentURL, frameId, loaderId);
        networkRequest.initiator = initiator;
        return networkRequest;
    }
}

/**
 * @type {?WebInspector.NetworkManager}
 */
WebInspector.networkManager = null;
