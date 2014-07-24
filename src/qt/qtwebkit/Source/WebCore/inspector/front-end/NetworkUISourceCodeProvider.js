/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 * @param {WebInspector.SimpleWorkspaceProvider} networkWorkspaceProvider
 * @param {WebInspector.Workspace} workspace
 */
WebInspector.NetworkUISourceCodeProvider = function(networkWorkspaceProvider, workspace)
{
    this._networkWorkspaceProvider = networkWorkspaceProvider;
    this._workspace = workspace;
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.ResourceAdded, this._resourceAdded, this);
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.MainFrameNavigated, this._mainFrameNavigated, this);
    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.ParsedScriptSource, this._parsedScriptSource, this);

    this._processedURLs = {};
    this._lastDynamicAnonymousScriptIndexForURL = {};
}

WebInspector.NetworkUISourceCodeProvider.prototype = {
    _populate: function()
    {
        function populateFrame(frame)
        {
            for (var i = 0; i < frame.childFrames.length; ++i)
                populateFrame.call(this, frame.childFrames[i]);

            var resources = frame.resources();
            for (var i = 0; i < resources.length; ++i)
                this._resourceAdded({data:resources[i]});
        }

        populateFrame.call(this, WebInspector.resourceTreeModel.mainFrame);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _parsedScriptSource: function(event)
    {
        var script = /** @type {WebInspector.Script} */ (event.data);
        if (!script.sourceURL || script.isInlineScript())
            return;
        if (WebInspector.experimentsSettings.snippetsSupport.isEnabled() && script.isSnippet())
            return;
        var isDynamicAnonymousScript;
        // Only add uiSourceCodes for
        // - content scripts;
        // - scripts with explicit sourceURL comment;
        // - dynamic scripts (script elements with src attribute) when inspector is opened after the script was loaded.
        if (!script.hasSourceURL && !script.isContentScript) {
            var requestURL = script.sourceURL.replace(/#.*/, "");
            if (WebInspector.resourceForURL(requestURL) || WebInspector.networkLog.requestForURL(requestURL))
                return;
        }
        // Filter out embedder injected content scripts.
        if (script.isContentScript && !script.hasSourceURL) {
            var parsedURL = new WebInspector.ParsedURL(script.sourceURL);
            if (!parsedURL.host)
                return;
        }
        this._addFile(script.sourceURL, script, script.isContentScript);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _resourceAdded: function(event)
    {
        var resource = /** @type {WebInspector.Resource} */ (event.data);
        this._addFile(resource.url, resource);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _mainFrameNavigated: function(event)
    {
        this._reset();
    },

    /**
     * @param {string} url
     * @param {WebInspector.ContentProvider} contentProvider
     * @param {boolean=} isContentScript
     */
    _addFile: function(url, contentProvider, isContentScript)
    {
        if (this._workspace.hasMappingForURL(url))
            return;

        var type = contentProvider.contentType();
        if (type !== WebInspector.resourceTypes.Stylesheet && type !== WebInspector.resourceTypes.Document && type !== WebInspector.resourceTypes.Script)
            return;
        if (this._processedURLs[url])
            return;
        this._processedURLs[url] = true;
        var isEditable = type !== WebInspector.resourceTypes.Document;
        this._networkWorkspaceProvider.addFileForURL(url, contentProvider, isEditable, isContentScript);
    },

    _reset: function()
    {
        this._processedURLs = {};
        this._lastDynamicAnonymousScriptIndexForURL = {};
        this._networkWorkspaceProvider.reset();
        this._populate();
    }
}

/**
 * @type {?WebInspector.SimpleWorkspaceProvider}
 */
WebInspector.networkWorkspaceProvider = null;
