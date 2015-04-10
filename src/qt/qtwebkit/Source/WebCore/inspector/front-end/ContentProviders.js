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
 * @implements {WebInspector.ContentProvider}
 * @param {Array.<WebInspector.Script>} scripts
 */
WebInspector.ConcatenatedScriptsContentProvider = function(scripts)
{
    this._mimeType = "text/html";
    this._scripts = scripts;
}

WebInspector.ConcatenatedScriptsContentProvider.scriptOpenTag = "<script>";
WebInspector.ConcatenatedScriptsContentProvider.scriptCloseTag = "</script>";

WebInspector.ConcatenatedScriptsContentProvider.prototype = {
    /**
     * @return {Array.<WebInspector.Script>}
     */
    _sortedScripts: function()
    {
        if (this._sortedScriptsArray)
            return this._sortedScriptsArray;

        this._sortedScriptsArray = [];
        
        var scripts = this._scripts.slice();
        scripts.sort(function(x, y) { return x.lineOffset - y.lineOffset || x.columnOffset - y.columnOffset; });
        
        var scriptOpenTagLength = WebInspector.ConcatenatedScriptsContentProvider.scriptOpenTag.length;
        var scriptCloseTagLength = WebInspector.ConcatenatedScriptsContentProvider.scriptCloseTag.length;
        
        this._sortedScriptsArray.push(scripts[0]);
        for (var i = 1; i < scripts.length; ++i) {
            var previousScript = this._sortedScriptsArray[this._sortedScriptsArray.length - 1];
            
            var lineNumber = previousScript.endLine;
            var columnNumber = previousScript.endColumn + scriptCloseTagLength + scriptOpenTagLength;
            
            if (lineNumber < scripts[i].lineOffset || (lineNumber === scripts[i].lineOffset && columnNumber <= scripts[i].columnOffset))
                this._sortedScriptsArray.push(scripts[i]);
        }
        return this._sortedScriptsArray;
    },

    /**
     * @return {string}
     */
    contentURL: function()
    {
        return "";
    },

    /**
     * @return {WebInspector.ResourceType}
     */
    contentType: function()
    {
        return WebInspector.resourceTypes.Document;
    },
    
    /**
     * @param {function(?string,boolean,string)} callback
     */
    requestContent: function(callback)
    {
        var scripts = this._sortedScripts();
        var sources = [];
        function didRequestSource(content, contentEncoded, mimeType)
        {
            sources.push(content);
            if (sources.length == scripts.length)
                callback(this._concatenateScriptsContent(scripts, sources), false, this._mimeType);
        }
        for (var i = 0; i < scripts.length; ++i)
            scripts[i].requestContent(didRequestSource.bind(this));
    },

    /**
     * @param {string} query
     * @param {boolean} caseSensitive
     * @param {boolean} isRegex
     * @param {function(Array.<WebInspector.ContentProvider.SearchMatch>)} callback
     */
    searchInContent: function(query, caseSensitive, isRegex, callback)
    {
        var results = {};
        var scripts = this._sortedScripts();
        var scriptsLeft = scripts.length;

        function maybeCallback()
        {
            if (scriptsLeft)
                return;

            var result = [];
            for (var i = 0; i < scripts.length; ++i)
                result = result.concat(results[scripts[i].scriptId]);
            callback(result);
        }

        /**
         * @param {WebInspector.Script} script
         * @param {Array.<PageAgent.SearchMatch>} searchMatches
         */
        function searchCallback(script, searchMatches)
        {
            results[script.scriptId] = [];
            for (var i = 0; i < searchMatches.length; ++i) {
                var searchMatch = new WebInspector.ContentProvider.SearchMatch(searchMatches[i].lineNumber + script.lineOffset, searchMatches[i].lineContent);
                results[script.scriptId].push(searchMatch);
            }
            scriptsLeft--;
            maybeCallback.call(this);
        }

        maybeCallback();
        for (var i = 0; i < scripts.length; ++i)
            scripts[i].searchInContent(query, caseSensitive, isRegex, searchCallback.bind(this, scripts[i]));
    },

    /**
     * @return {string}
     */
    _concatenateScriptsContent: function(scripts, sources)
    {
        var content = "";
        var lineNumber = 0;
        var columnNumber = 0;

        var scriptOpenTag = WebInspector.ConcatenatedScriptsContentProvider.scriptOpenTag;
        var scriptCloseTag = WebInspector.ConcatenatedScriptsContentProvider.scriptCloseTag;
        for (var i = 0; i < scripts.length; ++i) {
            // Fill the gap with whitespace characters.
            for (var newLinesCount = scripts[i].lineOffset - lineNumber; newLinesCount > 0; --newLinesCount) {
                columnNumber = 0;
                content += "\n";
            }
            for (var spacesCount = scripts[i].columnOffset - columnNumber - scriptOpenTag.length; spacesCount > 0; --spacesCount)
                content += " ";

            // Add script tag.
            content += scriptOpenTag;
            content += sources[i];
            content += scriptCloseTag;
            lineNumber = scripts[i].endLine;
            columnNumber = scripts[i].endColumn + scriptCloseTag.length;
        }

        return content;
    },

    __proto__: WebInspector.ContentProvider.prototype
}

/**
 * @constructor
 * @param {string} sourceURL
 * @implements {WebInspector.ContentProvider}
 */
WebInspector.CompilerSourceMappingContentProvider = function(sourceURL)
{
    this._sourceURL = sourceURL;
}

WebInspector.CompilerSourceMappingContentProvider.prototype = {
    /**
     * @return {string}
     */
    contentURL: function()
    {
        return this._sourceURL;
    },

    /**
     * @return {WebInspector.ResourceType}
     */
    contentType: function()
    {
        return WebInspector.resourceTypes.Script;
    },
    
    /**
     * @param {function(?string,boolean,string)} callback
     */
    requestContent: function(callback)
    {
        var sourceCode = "";
        try {
            // FIXME: make sendRequest async.
            sourceCode = InspectorFrontendHost.loadResourceSynchronously(this._sourceURL);
        } catch(e) {
            console.error(e.message);
        }
        callback(sourceCode, false, "text/javascript");
    },

    /**
     * @param {string} query
     * @param {boolean} caseSensitive
     * @param {boolean} isRegex
     * @param {function(Array.<WebInspector.ContentProvider.SearchMatch>)} callback
     */
    searchInContent: function(query, caseSensitive, isRegex, callback)
    {
        callback([]);
    },

    __proto__: WebInspector.ContentProvider.prototype
}

/**
 * @constructor
 * @implements {WebInspector.ContentProvider}
 * @param {WebInspector.ResourceType} contentType 
 * @param {string} content
 * @param {string=} mimeType
 */
WebInspector.StaticContentProvider = function(contentType, content, mimeType)
{
    this._content = content;
    this._contentType = contentType;
    this._mimeType = mimeType;
}

WebInspector.StaticContentProvider.prototype = {
    /**
     * @return {string}
     */
    contentURL: function()
    {
        return "";
    },

    /**
     * @return {WebInspector.ResourceType}
     */
    contentType: function()
    {
        return this._contentType;
    },

    /**
     * @param {function(?string,boolean,string)} callback
     */
    requestContent: function(callback)
    {
        callback(this._content, false, this._mimeType || this._contentType.canonicalMimeType());
    },

    /**
     * @param {string} query
     * @param {boolean} caseSensitive
     * @param {boolean} isRegex
     * @param {function(Array.<WebInspector.ContentProvider.SearchMatch>)} callback
     */
    searchInContent: function(query, caseSensitive, isRegex, callback)
    {
        function performSearch()
        {
            callback(WebInspector.ContentProvider.performSearchInContent(this._content, query, caseSensitive, isRegex));
        }

        // searchInContent should call back later.
        window.setTimeout(performSearch.bind(this), 0);
    },

    __proto__: WebInspector.ContentProvider.prototype
}
