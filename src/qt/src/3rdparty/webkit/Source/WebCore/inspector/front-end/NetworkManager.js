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

WebInspector.NetworkManager = function()
{
    WebInspector.Object.call(this);
    this._dispatcher = new WebInspector.NetworkDispatcher(this);
    NetworkAgent.enable();
}

WebInspector.NetworkManager.EventTypes = {
    ResourceStarted: "ResourceStarted",
    ResourceUpdated: "ResourceUpdated",
    ResourceFinished: "ResourceFinished"
}

WebInspector.NetworkManager.prototype = {
    frontendReused: function()
    {
        NetworkAgent.enable();
    },

    requestContent: function(resource, base64Encode, callback)
    {
        function callbackWrapper(error, content)
        {
            callback(!error ? content : null);
        }
        PageAgent.getResourceContent(resource.frameId, resource.url, base64Encode, callbackWrapper);
    },

    inflightResourceForURL: function(url)
    {
        return this._dispatcher._inflightResourcesByURL[url];
    }
}

WebInspector.NetworkManager.prototype.__proto__ = WebInspector.Object.prototype;

WebInspector.NetworkDispatcher = function(manager)
{
    this._manager = manager;
    this._inflightResourcesById = {};
    this._inflightResourcesByURL = {};
    this._lastIdentifierForCachedResource = 0;
    InspectorBackend.registerDomainDispatcher("Network", this);
}

WebInspector.NetworkDispatcher.prototype = {
    _updateResourceWithRequest: function(resource, request)
    {
        resource.requestMethod = request.method;
        resource.requestHeaders = request.headers;
        resource.requestFormData = request.postData;
    },

    _updateResourceWithResponse: function(resource, response)
    {
        if (!response)
            return;

        resource.mimeType = response.mimeType;
        resource.statusCode = response.status;
        resource.statusText = response.statusText;
        resource.responseHeaders = response.headers;
        if (response.headersText)
            resource.responseHeadersText = response.headersText;
        if (response.requestHeaders)
            resource.requestHeaders = response.requestHeaders;
        if (response.requestHeadersText)
            resource.requestHeadersText = response.requestHeadersText;

        resource.connectionReused = response.connectionReused;
        resource.connectionID = response.connectionID;

        if (response.fromDiskCache)
            resource.cached = true;
        else
            resource.timing = response.timing;
    },

    _updateResourceWithCachedResource: function(resource, cachedResource)
    {
        resource.type = WebInspector.Resource.Type[cachedResource.type];
        resource.resourceSize = cachedResource.bodySize;
        this._updateResourceWithResponse(resource, cachedResource.response);
    },

    requestWillBeSent: function(identifier, frameId, loaderId, documentURL, request, time, stackTrace, redirectResponse)
    {
        var resource = this._inflightResourcesById[identifier];
        if (resource) {
            this.responseReceived(identifier, time, "Other", redirectResponse);
            resource = this._appendRedirect(identifier, time, request.url);
        } else
            resource = this._createResource(identifier, frameId, loaderId, request.url, documentURL, stackTrace);
        this._updateResourceWithRequest(resource, request);
        resource.startTime = time;

        this._startResource(resource);
    },

    resourceMarkedAsCached: function(identifier)
    {
        var resource = this._inflightResourcesById[identifier];
        if (!resource)
            return;

        resource.cached = true;
        this._updateResource(resource);
    },

    responseReceived: function(identifier, time, resourceType, response)
    {
        var resource = this._inflightResourcesById[identifier];
        if (!resource)
            return;

        resource.responseReceivedTime = time;
        resource.type = WebInspector.Resource.Type[resourceType];

        this._updateResourceWithResponse(resource, response);

        this._updateResource(resource);
    },

    dataReceived: function(identifier, time, dataLength, encodedDataLength)
    {
        var resource = this._inflightResourcesById[identifier];
        if (!resource)
            return;

        resource.resourceSize += dataLength;
        if (encodedDataLength != -1)
            resource.increaseTransferSize(encodedDataLength);
        resource.endTime = time;

        this._updateResource(resource);
    },

    loadingFinished: function(identifier, finishTime)
    {
        var resource = this._inflightResourcesById[identifier];
        if (!resource)
            return;

        this._finishResource(resource, finishTime);
    },

    loadingFailed: function(identifier, time, localizedDescription, canceled)
    {
        var resource = this._inflightResourcesById[identifier];
        if (!resource)
            return;

        resource.failed = true;
        resource.canceled = canceled;
        resource.localizedFailDescription = localizedDescription;
        this._finishResource(resource, time);
    },

    resourceLoadedFromMemoryCache: function(frameId, loaderId, documentURL, time, cachedResource)
    {
        var resource = this._createResource("cached:" + ++this._lastIdentifierForCachedResource, frameId, loaderId, cachedResource.url, documentURL);
        this._updateResourceWithCachedResource(resource, cachedResource);
        resource.cached = true;
        resource.requestMethod = "GET";
        this._startResource(resource);
        resource.startTime = resource.responseReceivedTime = time;
        this._finishResource(resource, time);
    },

    initialContentSet: function(identifier, sourceString, type)
    {
        var resource = WebInspector.networkResourceById(identifier);
        if (!resource)
            return;

        resource.type = WebInspector.Resource.Type[type];
        resource.setInitialContent(sourceString);
        this._updateResource(resource);
    },

    webSocketCreated: function(identifier, requestURL)
    {
        var resource = this._createResource(identifier, null, null, requestURL);
        resource.type = WebInspector.Resource.Type.WebSocket;
        this._startResource(resource);
    },

    webSocketWillSendHandshakeRequest: function(identifier, time, request)
    {
        var resource = this._inflightResourcesById[identifier];
        if (!resource)
            return;

        resource.requestMethod = "GET";
        resource.requestHeaders = request.headers;
        resource.webSocketRequestKey3 = request.requestKey3;
        resource.startTime = time;

        this._updateResource(resource);
    },

    webSocketHandshakeResponseReceived: function(identifier, time, response)
    {
        var resource = this._inflightResourcesById[identifier];
        if (!resource)
            return;

        resource.statusCode = response.status;
        resource.statusText = response.statusText;
        resource.responseHeaders = response.headers;
        resource.webSocketChallengeResponse = response.challengeResponse;
        resource.responseReceivedTime = time;

        this._updateResource(resource);
    },

    webSocketClosed: function(identifier, time)
    {
        var resource = this._inflightResourcesById[identifier];
        if (!resource)
            return;
        this._finishResource(resource, time);
    },

    _appendRedirect: function(identifier, time, redirectURL)
    {
        var originalResource = this._inflightResourcesById[identifier];
        var previousRedirects = originalResource.redirects || [];
        originalResource.identifier = "redirected:" + identifier + "." + previousRedirects.length;
        delete originalResource.redirects;
        this._finishResource(originalResource, time);
        var newResource = this._createResource(identifier, originalResource.frameId, originalResource.loaderId,
             redirectURL, originalResource.documentURL, originalResource.stackTrace);
        newResource.redirects = previousRedirects.concat(originalResource);
        return newResource;
    },

    _startResource: function(resource)
    {
        this._inflightResourcesById[resource.identifier] = resource;
        this._inflightResourcesByURL[resource.url] = resource;
        this._dispatchEventToListeners(WebInspector.NetworkManager.EventTypes.ResourceStarted, resource);
    },

    _updateResource: function(resource)
    {
        this._dispatchEventToListeners(WebInspector.NetworkManager.EventTypes.ResourceUpdated, resource);
    },

    _finishResource: function(resource, finishTime)
    {
        resource.endTime = finishTime;
        resource.finished = true;
        this._dispatchEventToListeners(WebInspector.NetworkManager.EventTypes.ResourceFinished, resource);
        delete this._inflightResourcesById[resource.identifier];
        delete this._inflightResourcesByURL[resource.url];
    },

    _dispatchEventToListeners: function(eventType, resource)
    {
        this._manager.dispatchEventToListeners(eventType, resource);
    },

    _createResource: function(identifier, frameId, loaderId, url, documentURL, stackTrace)
    {
        var resource = new WebInspector.Resource(identifier, url, loaderId);
        resource.documentURL = documentURL;
        resource.frameId = frameId;
        resource.stackTrace = stackTrace;
        return resource;
    }
}
