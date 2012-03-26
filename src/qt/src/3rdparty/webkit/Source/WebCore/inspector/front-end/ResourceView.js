/*
 * Copyright (C) 2007, 2008 Apple Inc.  All rights reserved.
 * Copyright (C) IBM Corp. 2009  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.ResourceView = function(resource)
{
    WebInspector.View.call(this);
    this.element.addStyleClass("resource-view");
    this.resource = resource;
}

WebInspector.ResourceView.prototype = {
    hasContent: function()
    {
        return false;
    }
}

WebInspector.ResourceView.prototype.__proto__ = WebInspector.View.prototype;

WebInspector.ResourceView.createResourceView = function(resource)
{
    switch (resource.category) {
    case WebInspector.resourceCategories.documents:
    case WebInspector.resourceCategories.scripts:
    case WebInspector.resourceCategories.xhr:
    case WebInspector.resourceCategories.stylesheets:
        return new WebInspector.ResourceSourceFrame(resource);
    case WebInspector.resourceCategories.images:
        return new WebInspector.ImageView(resource);
    case WebInspector.resourceCategories.fonts:
        return new WebInspector.FontView(resource);
    default:
        return new WebInspector.ResourceView(resource);
    }
}

WebInspector.ResourceView.resourceViewTypeMatchesResource = function(resource)
{
    var resourceView = resource._resourceView;
    switch (resource.category) {
    case WebInspector.resourceCategories.documents:
    case WebInspector.resourceCategories.scripts:
    case WebInspector.resourceCategories.xhr:
    case WebInspector.resourceCategories.stylesheets:
        return resourceView.__proto__ === WebInspector.ResourceSourceFrame.prototype;
    case WebInspector.resourceCategories.images:
        return resourceView.__proto__ === WebInspector.ImageView.prototype;
    case WebInspector.resourceCategories.fonts:
        return resourceView.__proto__ === WebInspector.FontView.prototype;
    default:
        return resourceView.__proto__ === WebInspector.ResourceView.prototype;
    }
}

WebInspector.ResourceView.resourceViewForResource = function(resource)
{
    if (!resource)
        return null;
    if (!resource._resourceView)
        resource._resourceView = WebInspector.ResourceView.createResourceView(resource);
    return resource._resourceView;
}

WebInspector.ResourceView.recreateResourceView = function(resource)
{
    var newView = WebInspector.ResourceView.createResourceView(resource);

    var oldView = resource._resourceView;
    var oldViewParentNode = oldView.visible ? oldView.element.parentNode : null;
    var scrollTop = oldView.scrollTop;

    resource._resourceView.detach();
    delete resource._resourceView;

    resource._resourceView = newView;

    if (oldViewParentNode)
        newView.show(oldViewParentNode);
    if (scrollTop)
        newView.scrollTop = scrollTop;

    return newView;
}

WebInspector.ResourceView.existingResourceViewForResource = function(resource)
{
    if (!resource)
        return null;
    return resource._resourceView;
}


WebInspector.ResourceSourceFrame = function(resource)
{
    WebInspector.SourceFrame.call(this, new WebInspector.SourceFrameDelegate(), resource.url);
    this._resource = resource;
}

//This is a map from resource.type to mime types
//found in WebInspector.SourceTokenizer.Registry.
WebInspector.ResourceSourceFrame.DefaultMIMETypeForResourceType = {
    0: "text/html",
    1: "text/css",
    4: "text/javascript"
}

WebInspector.ResourceSourceFrame.prototype = {
    get resource()
    {
        return this._resource;
    },

    isContentEditable: function()
    {
        return this._resource.isEditable();
    },

    editContent: function(newText, callback)
    {
        this._clearIncrementalUpdateTimer();
        var majorChange = true;
        this._resource.setContent(newText, majorChange, callback);
    },

    endEditing: function(oldRange, newRange)
    {
        function commitIncrementalEdit()
        {
            var majorChange = false;
            this._resource.setContent(this._textModel.text, majorChange, function() {});
        }
        const updateTimeout = 200;
        this._incrementalUpdateTimer = setTimeout(commitIncrementalEdit.bind(this), updateTimeout);
    },

    _clearIncrementalUpdateTimer: function()
    {
        if (this._incrementalUpdateTimer)
            clearTimeout(this._incrementalUpdateTimer);
        delete this._incrementalUpdateTimer;
    },

    requestContent: function(callback)
    {
        function contentLoaded(text)
        {
            var mimeType = WebInspector.ResourceSourceFrame.DefaultMIMETypeForResourceType[this._resource.type] || this._resource.mimeType;
            callback(mimeType, text);
        }
        this._resource.requestContent(contentLoaded.bind(this));
    },

    suggestedFileName: function()
    {
        return this._resource.displayName;
    }
}

WebInspector.ResourceSourceFrame.prototype.__proto__ = WebInspector.SourceFrame.prototype;

WebInspector.RevisionSourceFrame = function(revision)
{
    WebInspector.SourceFrame.call(this, new WebInspector.SourceFrameDelegate(), revision.resource.url);
    this._revision = revision;
}

WebInspector.RevisionSourceFrame.prototype = {
    get resource()
    {
        return this._revision.resource;
    },

    isContentEditable: function()
    {
        return false;
    },

    requestContent: function(callback)
    {
        function contentLoaded(text)
        {
            var mimeType = WebInspector.ResourceSourceFrame.DefaultMIMETypeForResourceType[this._revision.resource.type] || this._revision.resource.mimeType;
            callback(mimeType, text);
        }
        this._revision.requestContent(contentLoaded.bind(this));
    },

    suggestedFileName: function()
    {
        return this._revision.resource.displayName;
    }
}

WebInspector.RevisionSourceFrame.prototype.__proto__ = WebInspector.SourceFrame.prototype;
