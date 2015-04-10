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

WebInspector.FrameResourceManager = function()
{
    WebInspector.Object.call(this);

    PageAgent.enable();
    NetworkAgent.enable();

    this.initialize();
};

WebInspector.Object.addConstructorFunctions(WebInspector.FrameResourceManager);

WebInspector.FrameResourceManager.Event = {
    FrameWasAdded: "frame-resource-manager-frame-was-added",
    FrameWasRemoved: "frame-resource-manager-frame-was-removed",
    MainFrameDidChange: "frame-resource-manager-main-frame-did-change"
};

WebInspector.FrameResourceManager.prototype = {
    constructor: WebInspector.FrameResourceManager,

    // Public

    initialize: function()
    {
        var oldMainFrame = this._mainFrame;

        this._frameIdentifierMap = {};
        this._mainFrame = null;
        this._resourceRequestIdentifierMap = {};

        if (this._mainFrame !== oldMainFrame)
            this._mainFrameDidChange(oldMainFrame);

        this._waitingForMainFrameResourceTreePayload = true;
        PageAgent.getResourceTree(this._processMainFrameResourceTreePayload.bind(this));
    },

    get mainFrame()
    {
        return this._mainFrame;
    },

    get frames()
    {
        var frames = [];
        for (var key in this._frameIdentifierMap)
            frames.push(this._frameIdentifierMap[key]);

        return frames;
    },

    frameForIdentifier: function(frameId)
    {
        return this._frameIdentifierMap[frameId] || null;
    },

    frameDidNavigate: function(framePayload)
    {
        // Called from WebInspector.PageObserver.

        // Ignore this while waiting for the whole frame/resource tree.
        if (this._waitingForMainFrameResourceTreePayload)
            return;

        var frameWasLoadedInstantly = false;

        var frame = this.frameForIdentifier(framePayload.id);
        if (!frame) {
            // If the frame wasn't known before now, then the main resource was loaded instantly (about:blank, etc.)
            // Make a new resource (which will make the frame). Mark will mark it as loaded at the end too since we
            // don't expect any more events about the load finishing for these frames.
            var frameResource = this._addNewResourceToFrame(null, framePayload.id, framePayload.loaderId, framePayload.url, null, null, null, null, null, framePayload.name, framePayload.securityOrigin);
            var frame = frameResource.parentFrame;
            frameWasLoadedInstantly = true;

            console.assert(frame);
            if (!frame)
                return;
        }

        if (framePayload.loaderId === frame.provisionalLoaderIdentifier) {
            // There was a provisional load in progress, commit it.
            frame.commitProvisionalLoad(framePayload.securityOrigin);
        } else {
            if (frame.mainResource.url !== framePayload.url || frame.loaderIdentifier !== framePayload.loaderId) {
                // Navigations like back/forward do not have provisional loads, so create a new main resource here.
                var mainResource = new WebInspector.Resource(framePayload.url, framePayload.mimeType, null, framePayload.loaderId);
            } else {
                // The main resource is already correct, so reuse it.
                var mainResource = frame.mainResource;
            }

            frame.initialize(framePayload.name, framePayload.securityOrigin, framePayload.loaderId, mainResource);
        }

        var oldMainFrame = this._mainFrame;

        if (framePayload.parentId) {
            var parentFrame = this.frameForIdentifier(framePayload.parentId);
            console.assert(parentFrame);

            if (frame === this._mainFrame)
                this._mainFrame = null

            if (frame.parentFrame !== parentFrame)
                parentFrame.addChildFrame(frame);
        } else {
            if (frame.parentFrame)
                frame.parentFrame.removeChildFrame(frame);
            this._mainFrame = frame;
        }

        if (this._mainFrame !== oldMainFrame)
            this._mainFrameDidChange(oldMainFrame);

        if (frameWasLoadedInstantly)
            frame.mainResource.markAsFinished();
    },

    frameDidDetach: function(frameId)
    {
        // Called from WebInspector.PageObserver.

        // Ignore this while waiting for the whole frame/resource tree.
        if (this._waitingForMainFrameResourceTreePayload)
            return;

        var frame = this.frameForIdentifier(frameId);
        if (!frame)
            return;

        if (frame.parentFrame)
            frame.parentFrame.removeChildFrame(frame);

        delete this._frameIdentifierMap[frame.id];

        var oldMainFrame = this._mainFrame;

        if (frame === this._mainFrame)
            this._mainFrame = null;

        frame.clearExecutionContexts();

        this.dispatchEventToListeners(WebInspector.FrameResourceManager.Event.FrameWasRemoved, {frame: frame});

        if (this._mainFrame !== oldMainFrame)
            this._mainFrameDidChange(oldMainFrame);
    },

    resourceRequestWillBeSent: function(requestIdentifier, frameIdentifier, loaderIdentifier, request, type, redirectResponse, timestamp)
    {
        // Called from WebInspector.NetworkObserver.

        // Ignore this while waiting for the whole frame/resource tree.
        if (this._waitingForMainFrameResourceTreePayload)
            return;

        var resource = this._resourceRequestIdentifierMap[requestIdentifier];
        if (resource) {
            // This is an existing request which is being redirected, update the resource.
            console.assert(redirectResponse);
            resource.updateForRedirectResponse(request.url, request.headers, timestamp);
            return;
        }

        // This is a new request, make a new resource and add it to the right frame.
        resource = this._addNewResourceToFrame(requestIdentifier, frameIdentifier, loaderIdentifier, request.url, type, request.method, request.headers, request.postData, timestamp, null, null);

        // Associate the resource with the requestIdentifier so it can be found in future loading events.
        this._resourceRequestIdentifierMap[requestIdentifier] = resource;
    },

    markResourceRequestAsServedFromMemoryCache: function(requestIdentifier)
    {
        // Called from WebInspector.NetworkObserver.

        // Ignore this while waiting for the whole frame/resource tree.
        if (this._waitingForMainFrameResourceTreePayload)
            return;

        var resource = this._resourceRequestIdentifierMap[requestIdentifier];

        // We might not have a resource if the inspector was opened during the page load (after resourceRequestWillBeSent is called).
        // We don't want to assert in this case since we do likely have the resource, via PageAgent.getResourceTree. The Resource
        // just doesn't have a requestIdentifier for us to look it up.
        if (!resource)
            return;

        resource.markAsCached();
    },

    resourceRequestWasServedFromMemoryCache: function(requestIdentifier, frameIdentifier, loaderIdentifier, cachedResourcePayload, timestamp)
    {
        // Called from WebInspector.NetworkObserver.

        // Ignore this while waiting for the whole frame/resource tree.
        if (this._waitingForMainFrameResourceTreePayload)
            return;

        console.assert(!(requestIdentifier in this._resourceRequestIdentifierMap));

        var response = cachedResourcePayload.response;
        var resource = this._addNewResourceToFrame(requestIdentifier, frameIdentifier, loaderIdentifier, cachedResourcePayload.url, cachedResourcePayload.type, null, null, timestamp, null, null);
        resource.markAsCached();
        resource.updateForResponse(cachedResourcePayload.url, response.mimeType, cachedResourcePayload.type, response.headers, response.status, response.statusText, timestamp);
        resource.markAsFinished(timestamp);

        if (cachedResourcePayload.sourceMapURL)
            WebInspector.sourceMapManager.downloadSourceMap(cachedResourcePayload.sourceMapURL, resource.url, resource);

        // No need to associate the resource with the requestIdentifier, since this is the only event
        // sent for memory cache resource loads.
    },

    resourceRequestDidReceiveResponse: function(requestIdentifier, frameIdentifier, loaderIdentifier, type, response, timestamp)
    {
        // Called from WebInspector.NetworkObserver.

        // Ignore this while waiting for the whole frame/resource tree.
        if (this._waitingForMainFrameResourceTreePayload)
            return;

        var resource = this._resourceRequestIdentifierMap[requestIdentifier];

        // We might not have a resource if the inspector was opened during the page load (after resourceRequestWillBeSent is called).
        // We don't want to assert in this case since we do likely have the resource, via PageAgent.getResourceTree. The Resource
        // just doesn't have a requestIdentifier for us to look it up, but we can try to look it up by its URL.
        if (!resource) {
            var frame = this.frameForIdentifier(frameIdentifier);
            if (frame)
                resource = frame.resourceForURL(response.url);

            // If we find the resource this way we had marked it earlier as finished via PageAgent.getResourceTree.
            // Associate the resource with the requestIdentifier so it can be found in future loading events.
            // and roll it back to an unfinished state, we know now it is still loading.
            if (resource) {
                this._resourceRequestIdentifierMap[requestIdentifier] = resource;
                resource.revertMarkAsFinished();
            }
        }

        // If we haven't found an existing Resource by now, then it is a resource that was loading when the inspector
        // opened and we just missed the resourceRequestWillBeSent for it. So make a new resource and add it.
        if (!resource) {
            resource = this._addNewResourceToFrame(requestIdentifier, frameIdentifier, loaderIdentifier, response.url, type, null, response.requestHeaders, timestamp, null, null);

            // Associate the resource with the requestIdentifier so it can be found in future loading events.
            this._resourceRequestIdentifierMap[requestIdentifier] = resource;
        }

        if (response.fromDiskCache)
            resource.markAsCached();

        resource.updateForResponse(response.url, response.mimeType, type, response.headers, response.status, response.statusText, timestamp);
    },

    resourceRequestDidReceiveData: function(requestIdentifier, dataLength, encodedDataLength, timestamp)
    {
        // Called from WebInspector.NetworkObserver.

        // Ignore this while waiting for the whole frame/resource tree.
        if (this._waitingForMainFrameResourceTreePayload)
            return;

        var resource = this._resourceRequestIdentifierMap[requestIdentifier];

        // We might not have a resource if the inspector was opened during the page load (after resourceRequestWillBeSent is called).
        // We don't want to assert in this case since we do likely have the resource, via PageAgent.getResourceTree. The Resource
        // just doesn't have a requestIdentifier for us to look it up.
        if (!resource)
            return;

        resource.increaseSize(dataLength, timestamp);

        if (encodedDataLength !== -1)
            resource.increaseTransferSize(encodedDataLength);
    },

    resourceRequestDidFinishLoading: function(requestIdentifier, timestamp, sourceMapURL)
    {
        // Called from WebInspector.NetworkObserver.

        // Ignore this while waiting for the whole frame/resource tree.
        if (this._waitingForMainFrameResourceTreePayload)
            return;

        // By now we should always have the Resource. Either it was fetched when the inspector first opened with
        // PageAgent.getResourceTree, or it was a currently loading resource that we learned about in resourceRequestDidReceiveResponse.
        var resource = this._resourceRequestIdentifierMap[requestIdentifier];
        console.assert(resource);
        if (!resource)
            return;

        resource.markAsFinished(timestamp);

        if (sourceMapURL)
            WebInspector.sourceMapManager.downloadSourceMap(sourceMapURL, resource.url, resource);

        delete this._resourceRequestIdentifierMap[requestIdentifier];
    },

    resourceRequestDidFailLoading: function(requestIdentifier, canceled, timestamp)
    {
        // Called from WebInspector.NetworkObserver.

        // Ignore this while waiting for the whole frame/resource tree.
        if (this._waitingForMainFrameResourceTreePayload)
            return;

        // By now we should always have the Resource. Either it was fetched when the inspector first opened with
        // PageAgent.getResourceTree, or it was a currently loading resource that we learned about in resourceRequestDidReceiveResponse.
        var resource = this._resourceRequestIdentifierMap[requestIdentifier];
        console.assert(resource);
        if (!resource)
            return;

        resource.markAsFailed(canceled, timestamp);

        if (resource === resource.parentFrame.provisionalMainResource)
            resource.parentFrame.clearProvisionalLoad();

        delete this._resourceRequestIdentifierMap[requestIdentifier];
    },

    executionContextCreated: function(contextPayload)
    {
        // Called from WebInspector.RuntimeObserver.

        var frame = this.frameForIdentifier(contextPayload.frameId);
        console.assert(frame);
        if (!frame)
            return;

        var displayName = contextPayload.name || frame.mainResource.displayName;
        var executionContext = new WebInspector.ExecutionContext(contextPayload.id, displayName, contextPayload.isPageContext, frame);
        frame.addExecutionContext(executionContext);
    },

    resourceForURL: function(url)
    {
        if (!this._mainFrame)
            return null;

        if (this._mainFrame.mainResource.url === url)
            return this._mainFrame.mainResource;

        return this._mainFrame.resourceForURL(url, true);
    },

    // Private

    _addNewResourceToFrame: function(requestIdentifier, frameIdentifier, loaderIdentifier, url, type, requestMethod, requestHeaders, requestData, timestamp, frameName, frameSecurityOrigin)
    {
        console.assert(!this._waitingForMainFrameResourceTreePayload);
        if (this._waitingForMainFrameResourceTreePayload)
            return;

        var resource = null;

        var frame = this.frameForIdentifier(frameIdentifier);
        if (frame) {
            // This is a new request for an existing frame, which might be the main resource or a new resource.
            if (frame.mainResource.url === url && frame.loaderIdentifier === loaderIdentifier)
                resource = frame.mainResource;
            else if (frame.provisionalMainResource && frame.provisionalMainResource.url === url && frame.provisionalLoaderIdentifier === loaderIdentifier)
                resource = frame.provisionalMainResource;
            else {
                resource = new WebInspector.Resource(url, null, type, loaderIdentifier, requestIdentifier, requestMethod, requestHeaders, requestData, timestamp);
                this._addResourceToFrame(frame, resource);
            }
        } else {
            // This is a new request for a new frame, which is always the main resource.
            resource = new WebInspector.Resource(url, null, type, loaderIdentifier, requestIdentifier, requestMethod, requestHeaders, requestData, timestamp);
            frame = new WebInspector.Frame(frameIdentifier, frameName, frameSecurityOrigin, loaderIdentifier, resource);
            this._frameIdentifierMap[frame.id] = frame;

            // If we don't have a main frame, assume this is it. This can change later in
            // frameDidNavigate when the parent frame is known.
            if (!this._mainFrame) {
                this._mainFrame = frame;
                this._mainFrameDidChange(null);
            }

            this._dispatchFrameWasAddedEvent(frame);
        }

        console.assert(resource);

        return resource;
    },

    _addResourceToFrame: function(frame, resource)
    {
        console.assert(!this._waitingForMainFrameResourceTreePayload);
        if (this._waitingForMainFrameResourceTreePayload)
            return;

        console.assert(frame);
        console.assert(resource);

        if (resource.loaderIdentifier !== frame.loaderIdentifier && !frame.provisionalLoaderIdentifier) {
            // This is the start of a provisional load which happens before frameDidNavigate is called.
            // This resource will be the new mainResource if frameDidNavigate is called.
            frame.startProvisionalLoad(resource);
            return;
        }

        // This is just another resource, either for the main loader or the provisional loader.
        console.assert(resource.loaderIdentifier === frame.loaderIdentifier || resource.loaderIdentifier === frame.provisionalLoaderIdentifier);
        frame.addResource(resource);
    },

    _processMainFrameResourceTreePayload: function(error, mainFramePayload)
    {
        console.assert(this._waitingForMainFrameResourceTreePayload);
        delete this._waitingForMainFrameResourceTreePayload;

        if (error) {
            console.error(JSON.stringify(error));
            return;
        }

        console.assert(mainFramePayload);
        console.assert(mainFramePayload.frame);

        this._resourceRequestIdentifierMap = {};
        this._frameIdentifierMap = {};

        var oldMainFrame = this._mainFrame;

        this._mainFrame = this._addFrameTreeFromFrameResourceTreePayload(mainFramePayload, true);

        if (this._mainFrame !== oldMainFrame)
            this._mainFrameDidChange(oldMainFrame);
    },

    _createFrame: function(payload)
    {
        // If payload.url is missing or empty then this page is likely the special empty page. In that case
        // we will just say it is "about:blank" so we have a URL, which is required for resources.
        var mainResource = new WebInspector.Resource(payload.url || "about:blank", payload.mimeType, null, payload.loaderId);
        var frame = new WebInspector.Frame(payload.id, payload.name, payload.securityOrigin, payload.loaderId, mainResource);

        this._frameIdentifierMap[frame.id] = frame;

        mainResource.markAsFinished();

        return frame;
    },

    _createResource: function(payload, framePayload)
    {
        var resource = new WebInspector.Resource(payload.url, payload.mimeType, payload.type, framePayload.loaderId);

        if (payload.sourceMapURL)
            WebInspector.sourceMapManager.downloadSourceMap(payload.sourceMapURL, resource.url, resource);

        return resource;
    },

    _addFrameTreeFromFrameResourceTreePayload: function(payload, isMainFrame)
    {
        var frame = this._createFrame(payload.frame);
        if (isMainFrame)
            frame.markAsMainFrame();

        for (var i = 0; payload.childFrames && i < payload.childFrames.length; ++i)
            frame.addChildFrame(this._addFrameTreeFromFrameResourceTreePayload(payload.childFrames[i], false));

        for (var i = 0; payload.resources && i < payload.resources.length; ++i) {
            var resourcePayload = payload.resources[i];

            // The main resource is included as a resource. We can skip it since we already created
            // a main resource when we created the Frame. The resource payload does not include anything
            // didn't already get from the frame payload.
            if (resourcePayload.type === "Document" && resourcePayload.url === payload.frame.url)
                continue;

            var resource = this._createResource(resourcePayload, payload);
            frame.addResource(resource);

            if (resourcePayload.failed || resourcePayload.canceled)
                resource.markAsFailed(resourcePayload.canceled);
            else
                resource.markAsFinished();
        }

        this._dispatchFrameWasAddedEvent(frame);

        return frame;
    },

    _dispatchFrameWasAddedEvent: function(frame)
    {
        this.dispatchEventToListeners(WebInspector.FrameResourceManager.Event.FrameWasAdded, {frame: frame});
    },

    _mainFrameDidChange: function(oldMainFrame)
    {
        if (oldMainFrame)
            oldMainFrame.unmarkAsMainFrame();
        if (this._mainFrame)
            this._mainFrame.markAsMainFrame();
        this.dispatchEventToListeners(WebInspector.FrameResourceManager.Event.MainFrameDidChange, {oldMainFrame: oldMainFrame});
    }
};

WebInspector.FrameResourceManager.prototype.__proto__ = WebInspector.Object.prototype;
