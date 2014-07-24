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

WebInspector.SourceMapResourceTreeElement = function(sourceMapResource, representedObject)
{
    console.assert(sourceMapResource instanceof WebInspector.SourceMapResource);

    WebInspector.ResourceTreeElement.call(this, sourceMapResource);

    console.assert(this.resource === sourceMapResource);

    this.addClassName(WebInspector.SourceMapResourceTreeElement.StyleClassName);
};

WebInspector.SourceMapResourceTreeElement.StyleClassName = "source-map-resource";

WebInspector.SourceMapResourceTreeElement.prototype = {
    constructor: WebInspector.SourceMapResourceTreeElement,

    // Protected

    onattach: function()
    {
        WebInspector.ResourceTreeElement.prototype.onattach.call(this);

        // SourceMap resources must be loaded by the frontend, and only
        // then do they get their type information. So force a load as
        // soon as they are attached to the sidebar.
        this.resource.requestContent(function() {});
    },

    _updateTitles: function()
    {
        var oldMainTitle = this.mainTitle;
        this.mainTitle = this.resource.displayName;

        // Show the host as the subtitle if it is different from the originalSourceCode's host.
        var sourceMapHost = this.resource.urlComponents.host;
        var originalHost = this.resource.sourceMap.originalSourceCode.urlComponents.host;
        var subtitle = sourceMapHost !== originalHost ? WebInspector.displayNameForHost(sourceMapHost) : null;
        this.subtitle = this.mainTitle !== subtitle ? subtitle : null;

        if (oldMainTitle !== this.mainTitle)
            this.callFirstAncestorFunction("descendantResourceTreeElementMainTitleDidChange", [this, oldMainTitle]);
    }
};

WebInspector.SourceMapResourceTreeElement.prototype.__proto__ = WebInspector.ResourceTreeElement.prototype;
