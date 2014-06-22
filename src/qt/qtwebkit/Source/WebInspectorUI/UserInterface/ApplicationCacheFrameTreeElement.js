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

WebInspector.ApplicationCacheFrameTreeElement = function(representedObject)
{
    console.assert(representedObject instanceof WebInspector.ApplicationCacheFrame);

    WebInspector.GeneralTreeElement.call(this, WebInspector.ApplicationCacheFrameTreeElement.StyleClassName, "", "", representedObject, false);

    this.small = true;
    
    this.updateTitles();
};

WebInspector.ApplicationCacheFrameTreeElement.StyleClassName = "application-cache-frame";

WebInspector.ApplicationCacheFrameTreeElement.prototype = {
    constructor: WebInspector.ApplicationCacheFrameTreeElement,

    updateTitles: function()
    {
        var url = this.representedObject.frame.url;
        var parsedURL = parseURL(url);

        this.mainTitle = WebInspector.displayNameForURL(url, parsedURL);

        // Show the host as the subtitle only if it doesn't match the subtitle of the manifest tree element,
        // and it doesn't match the mainTitle.
        var subtitle = WebInspector.displayNameForHost(parsedURL.host);

        // FIXME: This is bad layering. We should not be calling a global object to get this.
        var manifestTreeElement = WebInspector.resourceSidebarPanel.treeElementForRepresentedObject(this.representedObject.manifest);

        var subtitleIsDuplicate = subtitle === this._mainTitle || subtitle === manifestTreeElement.subtitle;
        this.subtitle = subtitleIsDuplicate ? null : subtitle;
    }
};

WebInspector.ApplicationCacheFrameTreeElement.prototype.__proto__ = WebInspector.GeneralTreeElement.prototype;
