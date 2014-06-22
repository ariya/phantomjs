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

WebInspector.ResourceTreeElement = function(resource, representedObject)
{
    console.assert(resource instanceof WebInspector.Resource);

    WebInspector.SourceCodeTreeElement.call(this, resource, [WebInspector.ResourceTreeElement.StyleClassName, WebInspector.ResourceTreeElement.ResourceIconStyleClassName, resource.type], "", "", representedObject || resource, false);

    this._updateResource(resource);
};

WebInspector.ResourceTreeElement.StyleClassName = "resource";
WebInspector.ResourceTreeElement.ResourceIconStyleClassName = "resource-icon";
WebInspector.ResourceTreeElement.FailedStyleClassName = "failed";

WebInspector.ResourceTreeElement.compareResourceTreeElements = function(a, b)
{
    // Compare by type first to keep resources grouped by type when not sorted into folders.
    var comparisonResult = a.resource.type.localeCompare(b.resource.type);
    if (comparisonResult !== 0)
        return comparisonResult;

    // Compare async resource types by their first timestamp so they are in chronological order.
    if (a.resource.type === WebInspector.Resource.Type.XHR || a.resource.type === WebInspector.Resource.Type.WebSocket)
        return a.resource.firstTimestamp - b.resource.firstTimestamp || 0;

    // Compare by subtitle when the types are the same. The subtitle is used to show the
    // domain of the resource. This causes resources to group by domain. If the resource
    // is on the same domain as the frame it will have an empty subtitle. This is good
    // because empty string sorts first, so those will appear before external resources.
    comparisonResult = a.subtitle.localeCompare(b.subtitle);
    if (comparisonResult !== 0)
        return comparisonResult;

    // Compare by title when the subtitles are the same.
    return a.mainTitle.localeCompare(b.mainTitle);
}

WebInspector.ResourceTreeElement.compareFolderAndResourceTreeElements = function(a, b)
{
    var aIsFolder = a instanceof WebInspector.FolderTreeElement;
    var bIsFolder = b instanceof WebInspector.FolderTreeElement;

    if (aIsFolder && !bIsFolder)
        return -1;
    if (!aIsFolder && bIsFolder)
        return 1;
    if (aIsFolder && bIsFolder)
        return a.mainTitle.localeCompare(b.mainTitle);

    return WebInspector.ResourceTreeElement.compareResourceTreeElements(a, b);
}

WebInspector.ResourceTreeElement.prototype = {
    constructor: WebInspector.ResourceTreeElement,

    // Public

    get resource()
    {
        return this._resource;
    },

    get filterableData()
    {
        return {text: this._resource.url};
    },

    get reloadButton()
    {
        if (!this._reloadButton) {
            this._reloadButton = document.createElement("img");
            this._reloadButton.className = "reload-button";
            this._reloadButton.title = WebInspector.UIString("Reload page (%s)\nReload ignoring cache (%s)").format(WebInspector._reloadPageKeyboardShortcut.displayName, WebInspector._reloadPageIgnoringCacheKeyboardShortcut.displayName);
            this._reloadButton.addEventListener("click", this._reloadPageClicked.bind(this));
        }
        return this._reloadButton;
    },

    ondblclick: function()
    {
        InspectorFrontendHost.openInNewTab(this._resource.url);
    },

    // Protected (Used by FrameTreeElement)

    _updateResource: function(resource)
    {
        console.assert(resource instanceof WebInspector.Resource);

        // This method is for subclasses like FrameTreeElement who don't use a resource as the representedObject.
        // This method should only be called once if the representedObject is a resource, since changing the resource
        // without changing the representedObject is bad. If you need to change the resource, make a new ResourceTreeElement.
        console.assert(!this._resource || !(this.representedObject instanceof WebInspector.Resource));

        if (this._resource) {
            this._resource.removeEventListener(WebInspector.Resource.Event.URLDidChange, this._urlDidChange, this);
            this._resource.removeEventListener(WebInspector.Resource.Event.TypeDidChange, this._typeDidChange, this);
            this._resource.removeEventListener(WebInspector.Resource.Event.LoadingDidFinish, this._updateStatus, this);
            this._resource.removeEventListener(WebInspector.Resource.Event.LoadingDidFail, this._updateStatus, this);
        }

        this._updateSourceCode(resource);

        this._resource = resource;

        resource.addEventListener(WebInspector.Resource.Event.URLDidChange, this._urlDidChange, this);
        resource.addEventListener(WebInspector.Resource.Event.TypeDidChange, this._typeDidChange, this);
        resource.addEventListener(WebInspector.Resource.Event.LoadingDidFinish, this._updateStatus, this);
        resource.addEventListener(WebInspector.Resource.Event.LoadingDidFail, this._updateStatus, this);

        this._updateTitles();
        this._updateStatus();
        this._updateToolTip();
    },

    // Protected

    _updateTitles: function()
    {
        var frame = this._resource.parentFrame;
        var isMainResource = this._resource.isMainResource();
        if (isMainResource && frame) {
            // When the resource is a main resource, get the host from the current frame's parent frame instead of the current frame.
            var parentResourceHost = frame.parentFrame ? frame.parentFrame.mainResource.urlComponents.host : null;
        } else if (frame) {
            // When the resource is a normal sub-resource, get the host from the current frame's main resource.
            var parentResourceHost = frame.mainResource.urlComponents.host;
        }

        var urlComponents = this._resource.urlComponents;

        var oldMainTitle = this.mainTitle;
        this.mainTitle = WebInspector.displayNameForURL(this._resource.url, urlComponents);

        // Show the host as the subtitle if it is different from the main resource or if this is the main frame's main resource.
        var subtitle = parentResourceHost !== urlComponents.host || frame.isMainFrame() && isMainResource ? WebInspector.displayNameForHost(urlComponents.host) : null;
        this.subtitle = this.mainTitle !== subtitle ? subtitle : null;

        if (oldMainTitle !== this.mainTitle)
            this.callFirstAncestorFunction("descendantResourceTreeElementMainTitleDidChange", [this, oldMainTitle]);
    },

    // Private

    _updateStatus: function()
    {
        if (this._resource.failed)
            this.addClassName(WebInspector.ResourceTreeElement.FailedStyleClassName);
        else
            this.removeClassName(WebInspector.ResourceTreeElement.FailedStyleClassName);

        if (this._resource.finished || this._resource.failed) {
            // Remove the spinner and replace with a reload button in case it's the main frame's main resource.
            var frame = this._resource.parentFrame;
            this.status = this._resource.isMainResource() && frame && frame.isMainFrame() ? this.reloadButton : null;
        } else {
            var spinner = new WebInspector.IndeterminateProgressSpinner;
            this.status = spinner.element;
        }
    },

    _updateToolTip: function()
    {
        this.tooltip = this._resource.url;
    },

    _reloadPageClicked: function(event)
    {
        event.stopPropagation();

        // Ignore cache when the shift key is pressed.
        PageAgent.reload(event.shiftKey);
    },

    _urlDidChange: function(event)
    {
        this._updateTitles();
        this._updateToolTip();
    },

    _typeDidChange: function(event)
    {
        this.removeClassName(event.data.oldType);
        this.addClassName(this._resource.type);

        this.callFirstAncestorFunction("descendantResourceTreeElementTypeDidChange", [this, event.data.oldType]);
    }
};

WebInspector.ResourceTreeElement.prototype.__proto__ = WebInspector.SourceCodeTreeElement.prototype;
