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
 */
WebInspector.NetworkLog = function()
{
    this._requests = [];
    this._requestForId = {};
    WebInspector.networkManager.addEventListener(WebInspector.NetworkManager.EventTypes.RequestStarted, this._onRequestStarted, this);
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.MainFrameNavigated, this._onMainFrameNavigated, this);
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.OnLoad, this._onLoad, this);
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.DOMContentLoaded, this._onDOMContentLoaded, this);
}

WebInspector.NetworkLog.prototype = {
    /**
     * @return {Array.<WebInspector.NetworkRequest>}
     */
    get requests()
    {
        return this._requests;
    },

    /**
     * @param {string} url
     * @return {WebInspector.NetworkRequest}
     */
    requestForURL: function(url)
    {
        for (var i = 0; i < this._requests.length; ++i) {
            if (this._requests[i].url === url)
                return this._requests[i];
        }
        return null;
    },

    /**
     * @param {WebInspector.NetworkRequest} request
     * @return {WebInspector.PageLoad}
     */
    pageLoadForRequest: function(request)
    {
        return request.__page;
    },

    /**
     * @param {WebInspector.Event} event
     */
    _onMainFrameNavigated: function(event)
    {
        var mainFrame = /** type {WebInspector.ResourceTreeFrame} */ event.data;
        // Preserve requests from the new session.
        this._currentPageLoad = null;
        var oldRequests = this._requests.splice(0, this._requests.length);
        for (var i = 0; i < oldRequests.length; ++i) {
            var request = oldRequests[i];
            if (request.loaderId === mainFrame.loaderId) {
                if (!this._currentPageLoad)
                    this._currentPageLoad = new WebInspector.PageLoad(request);
                this._requests.push(request);
                request.__page = this._currentPageLoad;
            }
        }
    },

    /**
     * @param {WebInspector.Event} event
     */
    _onRequestStarted: function(event)
    {
        var request = /** @type {WebInspector.NetworkRequest} */ (event.data);
        this._requests.push(request);
        this._requestForId[request.requestId] = request;
        request.__page = this._currentPageLoad;
    },

    /**
     * @param {WebInspector.Event} event
     */
    _onDOMContentLoaded: function(event)
    {
        if (this._currentPageLoad)
            this._currentPageLoad.contentLoadTime = event.data;
    },

    /**
     * @param {WebInspector.Event} event
     */
    _onLoad: function(event)
    {
        if (this._currentPageLoad)
            this._currentPageLoad.loadTime = event.data;
    },

    /**
     * @param {NetworkAgent.RequestId} requestId
     * @return {?WebInspector.NetworkRequest}
     */
    requestForId: function(requestId)
    {
        return this._requestForId[requestId];
    }
}

/**
 * @type {WebInspector.NetworkLog}
 */
WebInspector.networkLog = null;

/**
 * @constructor
 * @param {WebInspector.NetworkRequest} mainRequest
 */
WebInspector.PageLoad = function(mainRequest)
{
    this.id = ++WebInspector.PageLoad._lastIdentifier;
    this.url = mainRequest.url;
    this.startTime = mainRequest.startTime;
}

WebInspector.PageLoad._lastIdentifier = 0;
