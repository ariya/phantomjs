/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 * @extends {WebInspector.TabbedPane}
 * @param {WebInspector.NetworkRequest} request
 */
WebInspector.NetworkItemView = function(request)
{
    WebInspector.TabbedPane.call(this);
    this.element.addStyleClass("network-item-view");

    var headersView = new WebInspector.RequestHeadersView(request);
    this.appendTab("headers", WebInspector.UIString("Headers"), headersView);

    this.addEventListener(WebInspector.TabbedPane.EventTypes.TabSelected, this._tabSelected, this);

    if (request.type === WebInspector.resourceTypes.WebSocket) {
        var frameView = new WebInspector.ResourceWebSocketFrameView(request);
        this.appendTab("webSocketFrames", WebInspector.UIString("Frames"), frameView);
    } else {
        var responseView = new WebInspector.RequestResponseView(request);
        var previewView = new WebInspector.RequestPreviewView(request, responseView);
        this.appendTab("preview", WebInspector.UIString("Preview"), previewView);
        this.appendTab("response", WebInspector.UIString("Response"), responseView);
    }

    if (request.requestCookies || request.responseCookies) {
        this._cookiesView = new WebInspector.RequestCookiesView(request);
        this.appendTab("cookies", WebInspector.UIString("Cookies"), this._cookiesView);
    }

    if (request.timing) {
        var timingView = new WebInspector.RequestTimingView(request);
        this.appendTab("timing", WebInspector.UIString("Timing"), timingView);
    }
    this._request = request;
}

WebInspector.NetworkItemView.prototype = {
    wasShown: function()
    {
        WebInspector.TabbedPane.prototype.wasShown.call(this);
        this._selectTab();
    },

    /**
     * @param {string=} tabId
     */
    _selectTab: function(tabId)
    {
        if (!tabId)
            tabId = WebInspector.settings.resourceViewTab.get();

        if (!this.selectTab(tabId)) {
            this._isInFallbackSelection = true;
            this.selectTab("headers");
            delete this._isInFallbackSelection;
        }
    },

    _tabSelected: function(event)
    {
        if (!event.data.isUserGesture)
            return;

        WebInspector.settings.resourceViewTab.set(event.data.tabId);

        WebInspector.notifications.dispatchEventToListeners(WebInspector.UserMetrics.UserAction, {
            action: WebInspector.UserMetrics.UserActionNames.NetworkRequestTabSelected,
            tab: event.data.tabId,
            url: this._request.url
        });
    },

    /**
      * @return {WebInspector.NetworkRequest}
      */
    request: function()
    {
        return this._request;
    },

    __proto__: WebInspector.TabbedPane.prototype
}

/**
 * @constructor
 * @extends {WebInspector.RequestView}
 * @param {WebInspector.NetworkRequest} request
 */
WebInspector.RequestContentView = function(request)
{
    WebInspector.RequestView.call(this, request);
}

WebInspector.RequestContentView.prototype = {
    hasContent: function()
    {
        return true;
    },

    get innerView()
    {
        return this._innerView;
    },

    set innerView(innerView)
    {
        this._innerView = innerView;
    },

    wasShown: function()
    {
        this._ensureInnerViewShown();
    },

    _ensureInnerViewShown: function()
    {
        if (this._innerViewShowRequested)
            return;
        this._innerViewShowRequested = true;

        /**
         * @param {?string} content
         * @param {boolean} contentEncoded
         * @param {string} mimeType
         */
        function callback(content, contentEncoded, mimeType)
        {
            this._innerViewShowRequested = false;
            this.contentLoaded();
        }

        this.request.requestContent(callback.bind(this));
    },

    contentLoaded: function()
    {
        // Should be implemented by subclasses.
    },

    canHighlightLine: function()
    {
        return this._innerView && this._innerView.canHighlightLine();
    },

    highlightLine: function(line)
    {
        if (this.canHighlightLine())
            this._innerView.highlightLine(line);
    },

    __proto__: WebInspector.RequestView.prototype
}
