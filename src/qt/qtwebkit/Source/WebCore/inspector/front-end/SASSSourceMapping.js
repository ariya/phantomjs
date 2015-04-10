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
 * @implements {WebInspector.SourceMapping}
 * @param {WebInspector.CSSStyleModel} cssModel
 * @param {WebInspector.Workspace} workspace
 * @param {WebInspector.SimpleWorkspaceProvider} networkWorkspaceProvider
 */
WebInspector.SASSSourceMapping = function(cssModel, workspace, networkWorkspaceProvider)
{
    this._cssModel = cssModel;
    this._workspace = workspace;
    this._networkWorkspaceProvider = networkWorkspaceProvider;
    this._sourceMapByURL = {};
    this._sourceMapByStyleSheetURL = {};
    this._cssURLsForSASSURL = {};
    this._timeoutForURL = {};
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.ResourceAdded, this._resourceAdded, this);
    WebInspector.fileManager.addEventListener(WebInspector.FileManager.EventTypes.SavedURL, this._fileSaveFinished, this);
    this._cssModel.addEventListener(WebInspector.CSSStyleModel.Events.StyleSheetChanged, this._styleSheetChanged, this);
    this._workspace.addEventListener(WebInspector.Workspace.Events.ProjectWillReset, this._reset, this);
}

WebInspector.SASSSourceMapping.prototype = {
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
    _styleSheetChanged: function(event)
    {
        var isAddingRevision = this._isAddingRevision;
        delete this._isAddingRevision;

        if (isAddingRevision)
            return;
        this._cssModel.resourceBinding().requestResourceURLForStyleSheetId(event.data.styleSheetId, callback.bind(this));

        function callback(url)
        {
            if (!url)
                return;
            this._cssModel.setSourceMapping(url, null);
        }
    },

    /**
     * @param {WebInspector.Event} event
     */
    _fileSaveFinished: function(event)
    {
        // FIXME: add support for FileMapping.
        var sassURL = /** @type {string} */ (event.data);
        function callback()
        {
            delete this._timeoutForURL[sassURL];
            var cssURLs = this._cssURLsForSASSURL[sassURL];
            if (!cssURLs)
                return;
            for (var i = 0; i < cssURLs.length; ++i)
                this._reloadCSS(cssURLs[i]);
        }

        var timer = this._timeoutForURL[sassURL];
        if (timer) {
            clearTimeout(timer);
            delete this._timeoutForURL[sassURL];
        }
        if (!WebInspector.settings.cssReloadEnabled.get() || !this._cssURLsForSASSURL[sassURL])
            return;
        var timeout = WebInspector.settings.cssReloadTimeout.get();
        if (timeout && isFinite(timeout))
            this._timeoutForURL[sassURL] = setTimeout(callback.bind(this), Number(timeout));
    },

    _reloadCSS: function(url)
    {
        var uiSourceCode = this._workspace.uiSourceCodeForURL(url);
        if (!uiSourceCode)
            return;
        var newContent = InspectorFrontendHost.loadResourceSynchronously(url);
        this._isAddingRevision = true;
        uiSourceCode.addRevision(newContent);
        // this._isAddingRevision will be deleted in this._styleSheetChanged().
        this._loadAndProcessSourceMap(newContent, url, true);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _resourceAdded: function(event)
    {
        var resource = /** @type {WebInspector.Resource} */ (event.data);
        if (resource.type !== WebInspector.resourceTypes.Stylesheet)
            return;

        /**
         * @param {?string} content
         * @param {boolean} contentEncoded
         * @param {string} mimeType
         */
        function didRequestContent(content, contentEncoded, mimeType)
        {
            this._loadAndProcessSourceMap(content, resource.url);
        }
        resource.requestContent(didRequestContent.bind(this));
    },

    /**
     * @param {?string} content
     * @param {string} cssURL
     * @param {boolean=} forceRebind
     */
    _loadAndProcessSourceMap: function(content, cssURL, forceRebind)
    {
        if (!content)
            return;
        var lines = content.split(/\r?\n/);
        if (!lines.length)
            return;

        const sourceMapRegex = /^\/\*[#@] sourceMappingURL=([^\s]+)\s*\*\/$/;
        var lastLine = lines[lines.length - 1];
        var match = lastLine.match(sourceMapRegex);
        if (!match)
            return;

        if (!forceRebind && this._sourceMapByStyleSheetURL[cssURL])
            return;
        var sourceMap = this.loadSourceMapForStyleSheet(match[1], cssURL, forceRebind);

        if (!sourceMap)
            return;
        this._sourceMapByStyleSheetURL[cssURL] = sourceMap;
        this._bindUISourceCode(cssURL, sourceMap);
    },

    /**
     * @param {string} cssURL
     * @param {string} sassURL
     */
    _addCSSURLforSASSURL: function(cssURL, sassURL)
    {
        var cssURLs;
        if (this._cssURLsForSASSURL.hasOwnProperty(sassURL))
            cssURLs = this._cssURLsForSASSURL[sassURL];
        else {
            cssURLs = [];
            this._cssURLsForSASSURL[sassURL] = cssURLs;
        }
        if (cssURLs.indexOf(cssURL) === -1)
            cssURLs.push(cssURL);
    },

    /**
     * @param {string} sourceMapURL
     * @param {string} styleSheetURL
     * @param {boolean=} forceReload
     * @return {WebInspector.SourceMap}
     */
    loadSourceMapForStyleSheet: function(sourceMapURL, styleSheetURL, forceReload)
    {
        var completeStyleSheetURL = WebInspector.ParsedURL.completeURL(WebInspector.inspectedPageURL, styleSheetURL);
        if (!completeStyleSheetURL)
            return null;
        var completeSourceMapURL = WebInspector.ParsedURL.completeURL(completeStyleSheetURL, sourceMapURL);
        if (!completeSourceMapURL)
            return null;
        var sourceMap = this._sourceMapByURL[completeSourceMapURL];
        if (sourceMap && !forceReload)
            return sourceMap;
        sourceMap = WebInspector.SourceMap.load(completeSourceMapURL, completeStyleSheetURL);
        if (!sourceMap) {
            delete this._sourceMapByURL[completeSourceMapURL];
            return null;
        }
        this._sourceMapByURL[completeSourceMapURL] = sourceMap;
        return sourceMap;
    },

    /**
     * @param {string} rawURL
     * @param {WebInspector.SourceMap} sourceMap
     */
    _bindUISourceCode: function(rawURL, sourceMap)
    {
        this._cssModel.setSourceMapping(rawURL, this);
        var sources = sourceMap.sources();
        for (var i = 0; i < sources.length; ++i) {
            var url = sources[i];
            if (!this._workspace.hasMappingForURL(url) && !this._workspace.uiSourceCodeForURL(url)) {
                var content = InspectorFrontendHost.loadResourceSynchronously(url);
                var contentProvider = new WebInspector.StaticContentProvider(WebInspector.resourceTypes.Stylesheet, content, "text/x-scss");
                var uiSourceCode = this._networkWorkspaceProvider.addFileForURL(url, contentProvider, true);
                this._addCSSURLforSASSURL(rawURL, url);
                uiSourceCode.setSourceMapping(this);
            }
        }
    },

    /**
     * @param {WebInspector.RawLocation} rawLocation
     * @return {WebInspector.UILocation}
     */
    rawLocationToUILocation: function(rawLocation)
    {
        var location = /** @type WebInspector.CSSLocation */ (rawLocation);
        var entry;
        var sourceMap = this._sourceMapByStyleSheetURL[location.url];
        if (!sourceMap)
            return null;
        entry = sourceMap.findEntry(location.lineNumber, location.columnNumber);
        if (!entry || entry.length === 2)
            return null;
        var uiSourceCode = this._workspace.uiSourceCodeForURL(entry[2]);
        if (!uiSourceCode)
            return null;
        return new WebInspector.UILocation(uiSourceCode, entry[3], entry[4]);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     * @param {number} columnNumber
     * @return {WebInspector.RawLocation}
     */
    uiLocationToRawLocation: function(uiSourceCode, lineNumber, columnNumber)
    {
        // FIXME: Implement this when ui -> raw mapping has clients.
        return new WebInspector.CSSLocation(uiSourceCode.url || "", lineNumber, columnNumber);
    },

    /**
     * @return {boolean}
     */
    isIdentity: function()
    {
        return false;
    },

    _reset: function()
    {
        this._sourceMapByURL = {};
        this._sourceMapByStyleSheetURL = {};
        this._populate();
    }
}
