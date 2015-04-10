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

WebInspector.ApplicationCacheDetailsSidebarPanel = function() {
    WebInspector.DetailsSidebarPanel.call(this, "application-cache-details", WebInspector.UIString("Storage"), WebInspector.UIString("Storage"), "Images/NavigationItemStorage.pdf");

    this.element.classList.add(WebInspector.ApplicationCacheDetailsSidebarPanel.StyleClassName);

    this._applicationCacheFrame = null;

    this._locationManifestURLRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Manifest URL"));
    this._locationFrameURLRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Frame URL"));

    this._locationGroup = new WebInspector.DetailsSectionGroup([this._locationManifestURLRow, this._locationFrameURLRow]);

    this._locationSection = new WebInspector.DetailsSection("application-cache-location", WebInspector.UIString("Location"), [this._locationGroup]);

    this._onlineRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Online"));
    this._statusRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Status"));

    this._statusGroup = new WebInspector.DetailsSectionGroup([this._onlineRow, this._statusRow]);

    this._statusSection = new WebInspector.DetailsSection("application-cache-status", WebInspector.UIString("Status"), [this._statusGroup]);

    this.element.appendChild(this._locationSection.element);
    this.element.appendChild(this._statusSection.element);

    WebInspector.applicationCacheManager.addEventListener(WebInspector.ApplicationCacheManager.Event.NetworkStateUpdated, this._networkStateUpdated, this);
    WebInspector.applicationCacheManager.addEventListener(WebInspector.ApplicationCacheManager.Event.FrameManifestStatusChanged, this._frameManifestStatusChanged, this);
};

WebInspector.ApplicationCacheDetailsSidebarPanel.StyleClassName = "application-cache";

// This needs to be kept in sync with ApplicationCacheManager.js.
WebInspector.ApplicationCacheDetailsSidebarPanel.Status = {
    0: "Uncached",
    1: "Idle",
    2: "Checking",
    3: "Downloading",
    4: "UpdateReady",
    5: "Obsolete"
};

WebInspector.ApplicationCacheDetailsSidebarPanel.prototype = {
    constructor: WebInspector.ApplicationCacheDetailsSidebarPanel,

    // Public

    inspect: function(objects)
    {
        // Convert to a single item array if needed.
        if (!(objects instanceof Array))
            objects = [objects];

        var applicationCacheFrameToInspect = null;

        // Iterate over the objects to find a WebInspector.ApplicationCacheFrame to inspect.
        for (var i = 0; i < objects.length; ++i) {
            if (objects[i] instanceof WebInspector.ApplicationCacheFrame) {
                applicationCacheFrameToInspect = objects[i];
                break;
            }
        }

        this.applicationCacheFrame = applicationCacheFrameToInspect;

        return !!this.applicationCacheFrame;
    },
    
    get applicationCacheFrame()
    {
        return this._applicationCacheFrame;
    },
    
    set applicationCacheFrame(applicationCacheFrame)
    {
        if (this._applicationCacheFrame === applicationCacheFrame)
            return;
        
        this._applicationCacheFrame = applicationCacheFrame;

        this.needsRefresh();
    },

    refresh: function()
    {
        if (!this.applicationCacheFrame)
            return;

        this._locationFrameURLRow.value = this.applicationCacheFrame.frame.url;
        this._locationManifestURLRow.value = this.applicationCacheFrame.manifest.manifestURL;
        
        this._refreshOnlineRow();
        this._refreshStatusRow();
    },
    
    // Private
    
    _networkStateUpdated: function(event)
    {
        if (!this.applicationCacheFrame)
            return;
        
        this._refreshOnlineRow();
    },
    
    _frameManifestStatusChanged: function(event)
    {
        if (!this.applicationCacheFrame)
            return;

        console.assert(event.data.frameManifest instanceof WebInspector.ApplicationCacheFrame);
        if (event.data.frameManifest !== this.applicationCacheFrame)
            return;

        this._refreshStatusRow();
    },
    
    _refreshOnlineRow: function()
    {
        this._onlineRow.value = WebInspector.applicationCacheManager.online ? WebInspector.UIString("Yes") : WebInspector.UIString("No");
    },
    
    _refreshStatusRow: function()
    {
        this._statusRow.value = WebInspector.ApplicationCacheDetailsSidebarPanel.Status[this.applicationCacheFrame.status];
    }
};

WebInspector.ApplicationCacheDetailsSidebarPanel.prototype.__proto__ = WebInspector.DetailsSidebarPanel.prototype;
