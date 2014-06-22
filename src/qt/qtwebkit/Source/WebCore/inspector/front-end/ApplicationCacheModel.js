/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * @extends {WebInspector.Object}
 */
WebInspector.ApplicationCacheModel = function()
{
    ApplicationCacheAgent.enable();
    InspectorBackend.registerApplicationCacheDispatcher(new WebInspector.ApplicationCacheDispatcher(this));
    
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.FrameNavigated, this._frameNavigated, this);
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.FrameDetached, this._frameDetached, this);
    
    this._statuses = {};
    this._manifestURLsByFrame = {};

    this._mainFrameNavigated();
    
    this._onLine = true;
}

WebInspector.ApplicationCacheModel.EventTypes = {
    FrameManifestStatusUpdated: "FrameManifestStatusUpdated",
    FrameManifestAdded: "FrameManifestAdded",
    FrameManifestRemoved: "FrameManifestRemoved",
    NetworkStateChanged: "NetworkStateChanged"
}

WebInspector.ApplicationCacheModel.prototype = {
    _frameNavigated: function(event)
    {
        var frame = /** @type {WebInspector.ResourceTreeFrame} */ (event.data);
        if (frame.isMainFrame()) {
            this._mainFrameNavigated();
            return;
        }

        ApplicationCacheAgent.getManifestForFrame(frame.id, this._manifestForFrameLoaded.bind(this, frame.id));
    },
    
    /**
     * @param {WebInspector.Event} event
     */
    _frameDetached: function(event)
    {
        var frame = /** @type {WebInspector.ResourceTreeFrame} */ (event.data);
        this._frameManifestRemoved(frame.id);
    },
    
    _mainFrameNavigated: function()
    {
        ApplicationCacheAgent.getFramesWithManifests(this._framesWithManifestsLoaded.bind(this));
    },

    /**
     * @param {string} frameId
     * @param {?Protocol.Error} error
     * @param {string} manifestURL
     */
    _manifestForFrameLoaded: function(frameId, error, manifestURL)
    {
        if (error) {
            console.error(error);
            return;
        }
        
        if (!manifestURL)
            this._frameManifestRemoved(frameId);
    },
    
    /**
     * @param {?Protocol.Error} error
     * @param {Array.<ApplicationCacheAgent.FrameWithManifest>} framesWithManifests
     */
    _framesWithManifestsLoaded: function(error, framesWithManifests)
    {
        if (error) {
            console.error(error);
            return;
        }

        for (var i = 0; i < framesWithManifests.length; ++i)
            this._frameManifestUpdated(framesWithManifests[i].frameId, framesWithManifests[i].manifestURL, framesWithManifests[i].status);
    },
    
    /**
     * @param {string} frameId
     * @param {string} manifestURL
     * @param {number} status
     */
    _frameManifestUpdated: function(frameId, manifestURL, status)
    {
        if (status === applicationCache.UNCACHED) {
            this._frameManifestRemoved(frameId);
            return;
        }
            
        if (!manifestURL)
            return;
            
        if (this._manifestURLsByFrame[frameId] && manifestURL !== this._manifestURLsByFrame[frameId])
            this._frameManifestRemoved(frameId);
        
        var statusChanged = this._statuses[frameId] !== status;
        this._statuses[frameId] = status;
        
        if (!this._manifestURLsByFrame[frameId]) {
            this._manifestURLsByFrame[frameId] = manifestURL;
            this.dispatchEventToListeners(WebInspector.ApplicationCacheModel.EventTypes.FrameManifestAdded, frameId);
        }
            
        if (statusChanged)
            this.dispatchEventToListeners(WebInspector.ApplicationCacheModel.EventTypes.FrameManifestStatusUpdated, frameId);
    },
    
    /**
     * @param {string} frameId
     */
    _frameManifestRemoved: function(frameId)
    {
        if (!this._manifestURLsByFrame[frameId])
            return;

        var manifestURL = this._manifestURLsByFrame[frameId];
        delete this._manifestURLsByFrame[frameId];
        delete this._statuses[frameId];
        
        this.dispatchEventToListeners(WebInspector.ApplicationCacheModel.EventTypes.FrameManifestRemoved, frameId);
    },
    
    /**
     * @param {string} frameId
     * @return {string}
     */
    frameManifestURL: function(frameId)
    {
        return this._manifestURLsByFrame[frameId] || "";
    },
    
    /**
     * @param {string} frameId
     * @return {number}
     */
    frameManifestStatus: function(frameId)
    {
        return this._statuses[frameId] || applicationCache.UNCACHED;
    },
    
    /**
     * @return {boolean}
     */
    get onLine()
    {
        return this._onLine;
    },
    
    /**
     * @param {string} frameId
     * @param {string} manifestURL
     * @param {number} status
     */
    _statusUpdated: function(frameId, manifestURL, status)
    {
        this._frameManifestUpdated(frameId, manifestURL, status);
    },
    
    /**
     * @param {string} frameId
     * @param {function(Object)} callback
     */
    requestApplicationCache: function(frameId, callback)
    {
        function callbackWrapper(error, applicationCache)
        {
            if (error) {
                console.error(error);
                callback(null);
                return;
            }
            
            callback(applicationCache);
        }
        
        ApplicationCacheAgent.getApplicationCacheForFrame(frameId, callbackWrapper.bind(this));
    },
    
    /**
     * @param {boolean} isNowOnline
     */
    _networkStateUpdated: function(isNowOnline)
    {
        this._onLine = isNowOnline;
        this.dispatchEventToListeners(WebInspector.ApplicationCacheModel.EventTypes.NetworkStateChanged, isNowOnline);
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 * @implements {ApplicationCacheAgent.Dispatcher}
 */
WebInspector.ApplicationCacheDispatcher = function(applicationCacheModel)
{
    this._applicationCacheModel = applicationCacheModel;
}

WebInspector.ApplicationCacheDispatcher.prototype = {
    /**
     * @param {string} frameId
     * @param {string} manifestURL
     * @param {number} status
     */
    applicationCacheStatusUpdated: function(frameId, manifestURL, status)
    {
        this._applicationCacheModel._statusUpdated(frameId, manifestURL, status);
    },
    
    /**
     * @param {boolean} isNowOnline
     */
    networkStateUpdated: function(isNowOnline)
    {
        this._applicationCacheModel._networkStateUpdated(isNowOnline);
    }
}
