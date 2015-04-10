/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.Object}
 * @implements {WebInspector.ContentProvider}
 * @param {string} scriptId
 * @param {string} sourceURL
 * @param {number} startLine
 * @param {number} startColumn
 * @param {number} endLine
 * @param {number} endColumn
 * @param {boolean} isContentScript
 * @param {string=} sourceMapURL
 * @param {boolean=} hasSourceURL
 */
WebInspector.Script = function(scriptId, sourceURL, startLine, startColumn, endLine, endColumn, isContentScript, sourceMapURL, hasSourceURL)
{
    this.scriptId = scriptId;
    this.sourceURL = sourceURL;
    this.lineOffset = startLine;
    this.columnOffset = startColumn;
    this.endLine = endLine;
    this.endColumn = endColumn;
    this.isContentScript = isContentScript;
    this.sourceMapURL = sourceMapURL;
    this.hasSourceURL = hasSourceURL;
    this._locations = new Set();
    this._sourceMappings = [];
}

WebInspector.Script.Events = {
    ScriptEdited: "ScriptEdited",
}

WebInspector.Script.snippetSourceURLPrefix = "snippets:///";

WebInspector.Script.prototype = {
    /**
     * @return {string}
     */
    contentURL: function()
    {
        return this.sourceURL;
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
        if (this._source) {
            callback(this._source, false, "text/javascript");
            return;
        }

        /**
         * @this {WebInspector.Script}
         * @param {?Protocol.Error} error
         * @param {string} source
         */
        function didGetScriptSource(error, source)
        {
            this._source = error ? "" : source;
            callback(this._source, false, "text/javascript");
        }
        if (this.scriptId) {
            // Script failed to parse.
            DebuggerAgent.getScriptSource(this.scriptId, didGetScriptSource.bind(this));
        } else
            callback("", false, "text/javascript");
    },

    /**
     * @param {string} query
     * @param {boolean} caseSensitive
     * @param {boolean} isRegex
     * @param {function(Array.<PageAgent.SearchMatch>)} callback
     */
    searchInContent: function(query, caseSensitive, isRegex, callback)
    {
        /**
         * @this {WebInspector.Script}
         * @param {?Protocol.Error} error
         * @param {Array.<PageAgent.SearchMatch>} searchMatches
         */
        function innerCallback(error, searchMatches)
        {
            if (error)
                console.error(error);
            var result = [];
            for (var i = 0; i < searchMatches.length; ++i) {
                var searchMatch = new WebInspector.ContentProvider.SearchMatch(searchMatches[i].lineNumber, searchMatches[i].lineContent);
                result.push(searchMatch);
            }
            callback(result || []);
        }

        if (this.scriptId) {
            // Script failed to parse.
            DebuggerAgent.searchInContent(this.scriptId, query, caseSensitive, isRegex, innerCallback.bind(this));
        } else
            callback([]);
    },

    /**
     * @param {string} newSource
     * @param {function(?Protocol.Error, Array.<DebuggerAgent.CallFrame>=)} callback
     */
    editSource: function(newSource, callback)
    {
        /**
         * @this {WebInspector.Script}
         * @param {?Protocol.Error} error
         * @param {Array.<DebuggerAgent.CallFrame>=} callFrames
         * @param {Object=} debugData
         */
        function didEditScriptSource(error, callFrames, debugData)
        {
            // FIXME: support debugData.stack_update_needs_step_in flag by calling WebInspector.debugger_model.callStackModified
            if (!error)
                this._source = newSource;
            callback(error, callFrames);
            if (!error)
                this.dispatchEventToListeners(WebInspector.Script.Events.ScriptEdited, newSource);
        }
        if (this.scriptId) {
            // Script failed to parse.
            DebuggerAgent.setScriptSource(this.scriptId, newSource, undefined, didEditScriptSource.bind(this));
        } else
            callback("Script failed to parse");
    },

    /**
     * @return {boolean}
     */
    isInlineScript: function()
    {
        var startsAtZero = !this.lineOffset && !this.columnOffset;
        return !!this.sourceURL && !startsAtZero;
    },

    /**
     * @return {boolean}
     */
    isAnonymousScript: function()
    {
        return !this.sourceURL;
    },

    /**
     * @param {boolean} isDynamicScript
     */
    setIsDynamicScript: function(isDynamicScript)
    {
        this._isDynamicScript = isDynamicScript;
    },

    /**
     * @return {boolean}
     */
    isDynamicScript: function()
    {
        return !!this._isDynamicScript;
    },

    /**
     * @return {boolean}
     */
    isSnippet: function()
    {
        return this.sourceURL && this.sourceURL.startsWith(WebInspector.Script.snippetSourceURLPrefix);
    },

    /**
     * @param {number} lineNumber
     * @param {number=} columnNumber
     * @return {WebInspector.UILocation}
     */
    rawLocationToUILocation: function(lineNumber, columnNumber)
    {
        var uiLocation;
        var rawLocation = new WebInspector.DebuggerModel.Location(this.scriptId, lineNumber, columnNumber || 0);
        for (var i = this._sourceMappings.length - 1; !uiLocation && i >= 0; --i)
            uiLocation = this._sourceMappings[i].rawLocationToUILocation(rawLocation);
        console.assert(uiLocation, "Script raw location can not be mapped to any ui location.");
        return uiLocation.uiSourceCode.overrideLocation(uiLocation);
    },

    /**
     * @param {WebInspector.SourceMapping} sourceMapping
     */
    pushSourceMapping: function(sourceMapping)
    {
        this._sourceMappings.push(sourceMapping);
        this.updateLocations();
    },

    popSourceMapping: function()
    {
        this._sourceMappings.pop();
        this.updateLocations();
    },

    updateLocations: function()
    {
        var items = this._locations.items();
        for (var i = 0; i < items.length; ++i)
            items[i].update();
    },

    /**
     * @param {WebInspector.DebuggerModel.Location} rawLocation
     * @param {function(WebInspector.UILocation):(boolean|undefined)} updateDelegate
     * @return {WebInspector.Script.Location}
     */
    createLiveLocation: function(rawLocation, updateDelegate)
    {
        console.assert(rawLocation.scriptId === this.scriptId);
        var location = new WebInspector.Script.Location(this, rawLocation, updateDelegate);
        this._locations.add(location);
        location.update();
        return location;
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 * @extends {WebInspector.LiveLocation}
 * @param {WebInspector.Script} script
 * @param {WebInspector.DebuggerModel.Location} rawLocation
 * @param {function(WebInspector.UILocation):(boolean|undefined)} updateDelegate
 */
WebInspector.Script.Location = function(script, rawLocation, updateDelegate)
{
    WebInspector.LiveLocation.call(this, rawLocation, updateDelegate);
    this._script = script;
}

WebInspector.Script.Location.prototype = {
    /**
     * @return {WebInspector.UILocation}
     */
    uiLocation: function()
    {
        var debuggerModelLocation = /** @type {WebInspector.DebuggerModel.Location} */ (this.rawLocation());
        return this._script.rawLocationToUILocation(debuggerModelLocation.lineNumber, debuggerModelLocation.columnNumber);
    },

    dispose: function()
    {
        WebInspector.LiveLocation.prototype.dispose.call(this);
        this._script._locations.remove(this);
    },

    __proto__: WebInspector.LiveLocation.prototype
}
