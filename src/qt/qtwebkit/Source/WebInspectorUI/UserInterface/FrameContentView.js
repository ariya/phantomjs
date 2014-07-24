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

WebInspector.FrameContentView = function(frame)
{
    WebInspector.ClusterContentView.call(this, frame);

    this._frame = frame;

    function createPathComponent(displayName, className, identifier)
    {
        var pathComponent = new WebInspector.HierarchicalPathComponent(displayName, className, identifier, false, true);
        pathComponent.addEventListener(WebInspector.HierarchicalPathComponent.Event.SiblingWasSelected, this._pathComponentSelected, this);
        return pathComponent;
    }

    this._sourceCodePathComponent = createPathComponent.call(this, WebInspector.UIString("Source Code"), WebInspector.FrameContentView.SourceCodeIconStyleClassName, WebInspector.FrameContentView.SourceCodeIdentifier);
    this._domTreePathComponent = createPathComponent.call(this, WebInspector.UIString("DOM Tree"), WebInspector.FrameContentView.DOMTreeIconStyleClassName, WebInspector.FrameContentView.DOMTreeIdentifier);

    this._sourceCodePathComponent.nextSibling = this._domTreePathComponent;
    this._domTreePathComponent.previousSibling = this._sourceCodePathComponent;

    this.element.classList.add(WebInspector.FrameContentView.StyleClassName);

    this._currentContentViewSetting = new WebInspector.Setting("frame-current-view-" + this._frame.url.hash, WebInspector.FrameContentView.DOMTreeIdentifier);
};

WebInspector.FrameContentView.StyleClassName = "frame";
WebInspector.FrameContentView.SourceCodeIconStyleClassName = "source-code-icon";
WebInspector.FrameContentView.SourceCodeIdentifier = "source-code";
WebInspector.FrameContentView.DOMTreeIconStyleClassName = "dom-tree-icon";
WebInspector.FrameContentView.DOMTreeIdentifier = "dom-tree";

WebInspector.FrameContentView.prototype = {
    constructor: WebInspector.FrameContentView,

    // Public

    get frame()
    {
        return this._frame;
    },

    get allowedNavigationSidebarPanels()
    {
        return ["resource", "debugger"];
    },

    get selectionPathComponents()
    {
        if (!this._contentViewContainer.currentContentView)
            return [];

        // Append the current view's path components to the path component representing the current view.
        var components = [this._pathComponentForContentView(this._contentViewContainer.currentContentView)];
        return components.concat(this._contentViewContainer.currentContentView.selectionPathComponents);
    },

    shown: function()
    {
        WebInspector.ClusterContentView.prototype.shown.call(this);

        if (this._shownInitialContent)
            return;

        this._showContentViewForIdentifier(this._currentContentViewSetting.value);
    },

    closed: function()
    {
        WebInspector.ClusterContentView.prototype.closed.call(this);

        this._shownInitialContent = false;
    },

    showResource: function()
    {
        this._shownInitialContent = true;

        return this._showContentViewForIdentifier(WebInspector.FrameContentView.SourceCodeIdentifier);
    },

    showSourceCode: function(positionToReveal, textRangeToSelect, forceUnformatted)
    {
        var resourceContentView = this.showResource();
        console.assert(resourceContentView instanceof WebInspector.ResourceClusterContentView);
        if (!resourceContentView)
            return null;

        var responseContentView = resourceContentView.showResponse();
        if (typeof responseContentView.revealPosition === "function")
            responseContentView.revealPosition(positionToReveal, textRangeToSelect, forceUnformatted);

        return resourceContentView;
    },

    showDOMTree: function(domNodeToSelect, preventFocusChange)
    {
        this._shownInitialContent = true;

        var domTreeContentView = this._showContentViewForIdentifier(WebInspector.FrameContentView.DOMTreeIdentifier);
        console.assert(domTreeContentView);
        if (!domTreeContentView || !domNodeToSelect)
            return null;

        domTreeContentView.selectAndRevealDOMNode(domNodeToSelect, preventFocusChange);

        return domTreeContentView;
    },

    // Private

    _pathComponentForContentView: function(contentView)
    {
        console.assert(contentView);
        if (!contentView)
            return null;
        if (contentView.representedObject instanceof WebInspector.Resource)
            return this._sourceCodePathComponent;
        if (contentView.representedObject instanceof WebInspector.DOMTree)
            return this._domTreePathComponent;
        console.error("Unknown contentView.");
        return null;
    },

    _identifierForContentView: function(contentView)
    {
        console.assert(contentView);
        if (!contentView)
            return null;
        if (contentView.representedObject instanceof WebInspector.Resource)
            return WebInspector.FrameContentView.SourceCodeIdentifier;
        if (contentView.representedObject instanceof WebInspector.DOMTree)
            return WebInspector.FrameContentView.DOMTreeIdentifier;
        console.error("Unknown contentView.");
        return null;
    },

    _showContentViewForIdentifier: function(identifier)
    {
        var representedObjectToShow = null;

        switch (identifier) {
        case WebInspector.FrameContentView.SourceCodeIdentifier:
            representedObjectToShow = this._frame.mainResource;
            break;
        case WebInspector.FrameContentView.DOMTreeIdentifier:
            representedObjectToShow = this._frame.domTree;
            break;
        }

        console.assert(representedObjectToShow);
        if (!representedObjectToShow)
            return;

        this._currentContentViewSetting.value = identifier;

        return this.contentViewContainer.showContentViewForRepresentedObject(representedObjectToShow);
    },

    _pathComponentSelected: function(event)
    {
        this._showContentViewForIdentifier(event.data.pathComponent.representedObject);
    }
};

WebInspector.FrameContentView.prototype.__proto__ = WebInspector.ClusterContentView.prototype;
