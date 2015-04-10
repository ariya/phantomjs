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
 * @param {WebInspector.NetworkManager} networkManager
 */
WebInspector.ResourceTreeModel = function(networkManager)
{
    networkManager.addEventListener(WebInspector.NetworkManager.EventTypes.RequestFinished, this._onRequestFinished, this);
    networkManager.addEventListener(WebInspector.NetworkManager.EventTypes.RequestUpdateDropped, this._onRequestUpdateDropped, this);

    WebInspector.console.addEventListener(WebInspector.ConsoleModel.Events.MessageAdded, this._consoleMessageAdded, this);
    WebInspector.console.addEventListener(WebInspector.ConsoleModel.Events.RepeatCountUpdated, this._consoleMessageAdded, this);
    WebInspector.console.addEventListener(WebInspector.ConsoleModel.Events.ConsoleCleared, this._consoleCleared, this);

    PageAgent.enable();

    NetworkAgent.enable();
    this._fetchResourceTree();

    InspectorBackend.registerPageDispatcher(new WebInspector.PageDispatcher(this));

    this._pendingConsoleMessages = {};
    this._securityOriginFrameCount = {};
}

WebInspector.ResourceTreeModel.EventTypes = {
    FrameAdded: "FrameAdded",
    FrameNavigated: "FrameNavigated",
    FrameDetached: "FrameDetached",
    MainFrameNavigated: "MainFrameNavigated",
    MainFrameCreatedOrNavigated: "MainFrameCreatedOrNavigated",
    ResourceAdded: "ResourceAdded",
    WillLoadCachedResources: "WillLoadCachedResources",
    CachedResourcesLoaded: "CachedResourcesLoaded",
    DOMContentLoaded: "DOMContentLoaded",
    OnLoad: "OnLoad",
    InspectedURLChanged: "InspectedURLChanged",
    SecurityOriginAdded: "SecurityOriginAdded",
    SecurityOriginRemoved: "SecurityOriginRemoved"
}

WebInspector.ResourceTreeModel.prototype = {
    _fetchResourceTree: function()
    {
        this._frames = {};
        delete this._cachedResourcesProcessed;
        PageAgent.getResourceTree(this._processCachedResources.bind(this));
    },

    _processCachedResources: function(error, mainFramePayload)
    {
        if (error) {
            console.error(JSON.stringify(error));
            return;
        }

        this.dispatchEventToListeners(WebInspector.ResourceTreeModel.EventTypes.WillLoadCachedResources);
        WebInspector.inspectedPageURL = mainFramePayload.frame.url;
        this._addFramesRecursively(null, mainFramePayload);
        this._dispatchInspectedURLChanged();
        this.dispatchEventToListeners(WebInspector.ResourceTreeModel.EventTypes.CachedResourcesLoaded);
        this._cachedResourcesProcessed = true;
    },

    cachedResourcesLoaded: function()
    {
        return this._cachedResourcesProcessed;
    },

    _dispatchInspectedURLChanged: function()
    {
        InspectorFrontendHost.inspectedURLChanged(WebInspector.inspectedPageURL);
        this.dispatchEventToListeners(WebInspector.ResourceTreeModel.EventTypes.InspectedURLChanged, WebInspector.inspectedPageURL);
    },

    /**
     * @param {WebInspector.ResourceTreeFrame} frame
     * @param {boolean=} aboutToNavigate
     */
    _addFrame: function(frame, aboutToNavigate)
    {
        this._frames[frame.id] = frame;
        if (frame.isMainFrame())
            this.mainFrame = frame;
        this.dispatchEventToListeners(WebInspector.ResourceTreeModel.EventTypes.FrameAdded, frame);
        if (!aboutToNavigate)
            this._addSecurityOrigin(frame.securityOrigin);
        if (frame.isMainFrame())
            this.dispatchEventToListeners(WebInspector.ResourceTreeModel.EventTypes.MainFrameCreatedOrNavigated, frame);
    },

    /**
     * @param {string} securityOrigin
     */
    _addSecurityOrigin: function(securityOrigin)
    {
        if (!this._securityOriginFrameCount[securityOrigin]) {
            this._securityOriginFrameCount[securityOrigin] = 1;
            this.dispatchEventToListeners(WebInspector.ResourceTreeModel.EventTypes.SecurityOriginAdded, securityOrigin);
            return;
        }
        this._securityOriginFrameCount[securityOrigin] += 1;
    },

    /**
     * @param {string} securityOrigin
     */
    _removeSecurityOrigin: function(securityOrigin)
    {
        console.assert(this._securityOriginFrameCount[securityOrigin]);
        if (this._securityOriginFrameCount[securityOrigin] === 1) {
            delete this._securityOriginFrameCount[securityOrigin];
            this.dispatchEventToListeners(WebInspector.ResourceTreeModel.EventTypes.SecurityOriginRemoved, securityOrigin);
            return;
        }
        this._securityOriginFrameCount[securityOrigin] -= 1;
    },

    /**
     * @return {Array.<string>}
     */
    securityOrigins: function()
    {
        return Object.keys(this._securityOriginFrameCount);
    },

    /**
     * @param {WebInspector.ResourceTreeFrame} mainFrame
     */
    _handleMainFrameDetached: function(mainFrame)
    {
        /**
         * @param {WebInspector.ResourceTreeFrame} frame
         */
        function removeOriginForFrame(frame)
        {
            for (var i = 0; i < frame.childFrames.length; ++i)
                removeOriginForFrame.call(this, frame.childFrames[i]);
            if (!frame.isMainFrame())
                this._removeSecurityOrigin(frame.securityOrigin);
        }
        removeOriginForFrame.call(this, WebInspector.resourceTreeModel.mainFrame);
    },

    /**
     * @param {PageAgent.Frame} framePayload
     */
    _frameNavigated: function(framePayload)
    {
        // Do nothing unless cached resource tree is processed - it will overwrite everything.
        if (!this._cachedResourcesProcessed)
            return;
        var frame = this._frames[framePayload.id];
        var addedOrigin;
        if (frame) {
            // Navigation within existing frame.
            this._removeSecurityOrigin(frame.securityOrigin);
            frame._navigate(framePayload);
            addedOrigin = frame.securityOrigin;
        } else {
            // Either a new frame or a main frame navigation to the new backend process. 
            var parentFrame = this._frames[framePayload.parentId];
            frame = new WebInspector.ResourceTreeFrame(this, parentFrame, framePayload);
            if (frame.isMainFrame() && this.mainFrame) {
                this._handleMainFrameDetached(this.mainFrame);
                // Definitely a navigation to the new backend process.
                this._frameDetached(this.mainFrame.id);
            }
            this._addFrame(frame, true);
            addedOrigin = frame.securityOrigin;
        }

        if (frame.isMainFrame())
            WebInspector.inspectedPageURL = frame.url;

        this.dispatchEventToListeners(WebInspector.ResourceTreeModel.EventTypes.FrameNavigated, frame);
        if (frame.isMainFrame()) {
            this.dispatchEventToListeners(WebInspector.ResourceTreeModel.EventTypes.MainFrameNavigated, frame);
            this.dispatchEventToListeners(WebInspector.ResourceTreeModel.EventTypes.MainFrameCreatedOrNavigated, frame);
        }
        if (addedOrigin)
            this._addSecurityOrigin(addedOrigin);

        // Fill frame with retained resources (the ones loaded using new loader).
        var resources = frame.resources();
        for (var i = 0; i < resources.length; ++i)
            this.dispatchEventToListeners(WebInspector.ResourceTreeModel.EventTypes.ResourceAdded, resources[i]);

        if (frame.isMainFrame())
            this._dispatchInspectedURLChanged();
    },

    /**
     * @param {NetworkAgent.FrameId} frameId
     */
    _frameDetached: function(frameId)
    {
        // Do nothing unless cached resource tree is processed - it will overwrite everything.
        if (!this._cachedResourcesProcessed)
            return;

        var frame = this._frames[frameId];
        if (!frame)
            return;

        this._removeSecurityOrigin(frame.securityOrigin);
        if (frame.parentFrame)
            frame.parentFrame._removeChildFrame(frame);
        else
            frame._remove();
    },

    /**
     * @param {WebInspector.Event} event
     */
    _onRequestFinished: function(event)
    {
        if (!this._cachedResourcesProcessed)
            return;

        var request = /** @type {WebInspector.NetworkRequest} */ (event.data);
        if (request.failed || request.type === WebInspector.resourceTypes.XHR)
            return;

        var frame = this._frames[request.frameId];
        if (frame) {
            var resource = frame._addRequest(request);
            this._addPendingConsoleMessagesToResource(resource);
        }
    },

    /**
     * @param {WebInspector.Event} event
     */
    _onRequestUpdateDropped: function(event)
    {
        if (!this._cachedResourcesProcessed)
            return;

        var frameId = event.data.frameId;
        var frame = this._frames[frameId];
        if (!frame)
            return;

        var url = event.data.url;
        if (frame._resourcesMap[url])
            return;

        var resource = new WebInspector.Resource(null, url, frame.url, frameId, event.data.loaderId, WebInspector.resourceTypes[event.data.resourceType], event.data.mimeType);
        frame.addResource(resource);
    },

    /**
     * @param {NetworkAgent.FrameId} frameId
     * @return {WebInspector.ResourceTreeFrame}
     */
    frameForId: function(frameId)
    {
        return this._frames[frameId];
    },

    /**
     * @param {function(WebInspector.Resource)} callback
     * @return {boolean}
     */
    forAllResources: function(callback)
    {
        if (this.mainFrame)
            return this.mainFrame._callForFrameResources(callback);
        return false;
    },

    /**
     * @return {Array.<WebInspector.ResourceTreeFrame>}
     */
    frames: function() 
    {
        return Object.values(this._frames);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _consoleMessageAdded: function(event)
    {
        var msg = /** @type {WebInspector.ConsoleMessage} */ (event.data);
        var resource = msg.url ? this.resourceForURL(msg.url) : null;
        if (resource)
            this._addConsoleMessageToResource(msg, resource);
        else
            this._addPendingConsoleMessage(msg);
    },

    /**
     * @param {WebInspector.ConsoleMessage} msg
     */
    _addPendingConsoleMessage: function(msg)
    {
        if (!msg.url)
            return;
        if (!this._pendingConsoleMessages[msg.url])
            this._pendingConsoleMessages[msg.url] = [];
        this._pendingConsoleMessages[msg.url].push(msg);
    },

    /**
     * @param {WebInspector.Resource} resource
     */
    _addPendingConsoleMessagesToResource: function(resource)
    {
        var messages = this._pendingConsoleMessages[resource.url];
        if (messages) {
            for (var i = 0; i < messages.length; i++)
                this._addConsoleMessageToResource(messages[i], resource);
            delete this._pendingConsoleMessages[resource.url];
        }
    },

    /**
     * @param {WebInspector.ConsoleMessage} msg
     * @param {WebInspector.Resource} resource
     */
    _addConsoleMessageToResource: function(msg, resource)
    {
        switch (msg.level) {
        case WebInspector.ConsoleMessage.MessageLevel.Warning:
            resource.warnings += msg.repeatDelta;
            break;
        case WebInspector.ConsoleMessage.MessageLevel.Error:
            resource.errors += msg.repeatDelta;
            break;
        }
        resource.addMessage(msg);
    },

    _consoleCleared: function()
    {
        function callback(resource)
        {
            resource.clearErrorsAndWarnings();
        }

        this._pendingConsoleMessages = {};
        this.forAllResources(callback);
    },

    /**
     * @param {string} url
     * @return {WebInspector.Resource}
     */
    resourceForURL: function(url)
    {
        // Workers call into this with no frames available.
        return this.mainFrame ? this.mainFrame.resourceForURL(url) : null;
    },

    /**
     * @param {WebInspector.ResourceTreeFrame} parentFrame
     * @param {PageAgent.FrameResourceTree} frameTreePayload
     */
    _addFramesRecursively: function(parentFrame, frameTreePayload)
    {
        var framePayload = frameTreePayload.frame;
        var frame = new WebInspector.ResourceTreeFrame(this, parentFrame, framePayload);
        this._addFrame(frame);

        var frameResource = this._createResourceFromFramePayload(framePayload, framePayload.url, WebInspector.resourceTypes.Document, framePayload.mimeType);
        if (frame.isMainFrame())
            WebInspector.inspectedPageURL = frameResource.url;
        frame.addResource(frameResource);

        for (var i = 0; frameTreePayload.childFrames && i < frameTreePayload.childFrames.length; ++i)
            this._addFramesRecursively(frame, frameTreePayload.childFrames[i]);

        for (var i = 0; i < frameTreePayload.resources.length; ++i) {
            var subresource = frameTreePayload.resources[i];
            var resource = this._createResourceFromFramePayload(framePayload, subresource.url, WebInspector.resourceTypes[subresource.type], subresource.mimeType);
            frame.addResource(resource);
        }
    },

    /**
     * @param {PageAgent.Frame} frame
     * @param {string} url
     * @param {WebInspector.ResourceType} type
     * @param {string} mimeType
     * @return {WebInspector.Resource}
     */
    _createResourceFromFramePayload: function(frame, url, type, mimeType)
    {
        return new WebInspector.Resource(null, url, frame.url, frame.id, frame.loaderId, type, mimeType);
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 * @param {WebInspector.ResourceTreeModel} model
 * @param {?WebInspector.ResourceTreeFrame} parentFrame
 * @param {PageAgent.Frame} payload
 */
WebInspector.ResourceTreeFrame = function(model, parentFrame, payload)
{
    this._model = model;
    this._parentFrame = parentFrame;

    this._id = payload.id;
    this._loaderId = payload.loaderId;
    this._name = payload.name;
    this._url = payload.url;
    this._securityOrigin = payload.securityOrigin;
    this._mimeType = payload.mimeType;

    /**
     * @type {Array.<WebInspector.ResourceTreeFrame>}
     */
    this._childFrames = [];

    /**
     * @type {Object.<string, WebInspector.Resource>}
     */
    this._resourcesMap = {};

    if (this._parentFrame)
        this._parentFrame._childFrames.push(this);
}

WebInspector.ResourceTreeFrame.prototype = {
    /**
     * @return {string}
     */
    get id()
    {
        return this._id;
    },

    /**
     * @return {string}
     */
    get name()
    {
        return this._name || "";
    },

    /**
     * @return {string}
     */
    get url()
    {
        return this._url;
    },

    /**
     * @return {string}
     */
    get securityOrigin()
    {
        return this._securityOrigin;
    },

    /**
     * @return {string}
     */
    get loaderId()
    {
        return this._loaderId;
    },

    /**
     * @return {WebInspector.ResourceTreeFrame}
     */
    get parentFrame()
    {
        return this._parentFrame;
    },

    /**
     * @return {Array.<WebInspector.ResourceTreeFrame>}
     */
    get childFrames()
    {
        return this._childFrames;
    },

    /**
     * @return {boolean}
     */
    isMainFrame: function()
    {
        return !this._parentFrame;
    },

    /**
     * @param {PageAgent.Frame} framePayload
     */
    _navigate: function(framePayload)
    {
        this._loaderId = framePayload.loaderId;
        this._name = framePayload.name;
        this._url = framePayload.url;
        this._securityOrigin = framePayload.securityOrigin;
        this._mimeType = framePayload.mimeType;

        var mainResource = this._resourcesMap[this._url];
        this._resourcesMap = {};
        this._removeChildFrames();
        if (mainResource && mainResource.loaderId === this._loaderId)
            this.addResource(mainResource);
    },

    /**
     * @return {WebInspector.Resource}
     */
    get mainResource()
    {
        return this._resourcesMap[this._url];
    },

    /**
     * @param {WebInspector.ResourceTreeFrame} frame
     */
    _removeChildFrame: function(frame)
    {
        this._childFrames.remove(frame);
        frame._remove();
    },

    _removeChildFrames: function()
    {
        var copy = this._childFrames.slice();
        for (var i = 0; i < copy.length; ++i)
            this._removeChildFrame(copy[i]); 
    },

    _remove: function()
    {
        this._removeChildFrames();
        delete this._model._frames[this.id];
        this._model.dispatchEventToListeners(WebInspector.ResourceTreeModel.EventTypes.FrameDetached, this);
    },

    /**
     * @param {WebInspector.Resource} resource
     */
    addResource: function(resource)
    {
        if (this._resourcesMap[resource.url] === resource) {
            // Already in the tree, we just got an extra update.
            return;
        }
        this._resourcesMap[resource.url] = resource;
        this._model.dispatchEventToListeners(WebInspector.ResourceTreeModel.EventTypes.ResourceAdded, resource);
    },

    /**
     * @param {WebInspector.NetworkRequest} request
     * @return {WebInspector.Resource}
     */
    _addRequest: function(request)
    {
        var resource = this._resourcesMap[request.url];
        if (resource && resource.request === request) {
            // Already in the tree, we just got an extra update.
            return resource;
        }
        resource = new WebInspector.Resource(request, request.url, request.documentURL, request.frameId, request.loaderId, request.type, request.mimeType);
        this._resourcesMap[resource.url] = resource;
        this._model.dispatchEventToListeners(WebInspector.ResourceTreeModel.EventTypes.ResourceAdded, resource);
        return resource;
    },

    /**
     * @return {Array.<WebInspector.Resource>}
     */
    resources: function()
    {
        var result = [];
        for (var url in this._resourcesMap)
            result.push(this._resourcesMap[url]);
        return result;
    },

    /**
     * @param {string} url
     * @return {?WebInspector.Resource}
     */
    resourceForURL: function(url)
    {
        var result;
        function filter(resource)
        {
            if (resource.url === url) {
                result = resource;
                return true;
            }
        }
        this._callForFrameResources(filter);
        return result;
    },

    /**
     * @param {function(WebInspector.Resource)} callback
     * @return {boolean}
     */
    _callForFrameResources: function(callback)
    {
        for (var url in this._resourcesMap) {
            if (callback(this._resourcesMap[url]))
                return true;
        }

        for (var i = 0; i < this._childFrames.length; ++i) {
            if (this._childFrames[i]._callForFrameResources(callback))
                return true;
        }
        return false;
    }
}

/**
 * @constructor
 * @implements {PageAgent.Dispatcher}
 */
WebInspector.PageDispatcher = function(resourceTreeModel)
{
    this._resourceTreeModel = resourceTreeModel;
}

WebInspector.PageDispatcher.prototype = {
    domContentEventFired: function(time)
    {
        this._resourceTreeModel.dispatchEventToListeners(WebInspector.ResourceTreeModel.EventTypes.DOMContentLoaded, time);
    },

    loadEventFired: function(time)
    {
        this._resourceTreeModel.dispatchEventToListeners(WebInspector.ResourceTreeModel.EventTypes.OnLoad, time);
    },

    frameNavigated: function(frame)
    {
        this._resourceTreeModel._frameNavigated(frame);
    },

    frameDetached: function(frameId)
    {
        this._resourceTreeModel._frameDetached(frameId);
    },

    frameStartedLoading: function(frameId)
    {
    },

    frameStoppedLoading: function(frameId)
    {
    },

    frameScheduledNavigation: function(frameId, delay)
    {
    },

    frameClearedScheduledNavigation: function(frameId)
    {
    },

    javascriptDialogOpening: function(message)
    {
    },

    javascriptDialogClosed: function()
    {
    },

    scriptsEnabled: function(isEnabled)
    {
        WebInspector.settings.javaScriptDisabled.set(!isEnabled);
    }
}

/**
 * @type {WebInspector.ResourceTreeModel}
 */
WebInspector.resourceTreeModel = null;
