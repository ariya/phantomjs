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
 * @extends {WebInspector.Object}
 * @param {WebInspector.Workspace} workspace
 */
WebInspector.ScriptSnippetModel = function(workspace)
{
    this._workspace = workspace;
    /** {Object.<string, WebInspector.UISourceCode>} */
    this._uiSourceCodeForScriptId = {};
    this._scriptForUISourceCode = new Map();
    /** {Object.<string, WebInspector.UISourceCode>} */
    this._uiSourceCodeForSnippetId = {};
    this._snippetIdForUISourceCode = new Map();
    
    this._snippetStorage = new WebInspector.SnippetStorage("script", "Script snippet #");
    this._lastSnippetEvaluationIndexSetting = WebInspector.settings.createSetting("lastSnippetEvaluationIndex", 0);
    this._snippetScriptMapping = new WebInspector.SnippetScriptMapping(this);
    this._workspaceProvider = new WebInspector.SimpleWorkspaceProvider(this._workspace, WebInspector.projectTypes.Snippets);
    this.reset();
    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.GlobalObjectCleared, this._debuggerReset, this);
}

WebInspector.ScriptSnippetModel.prototype = {
    /**
     * @return {WebInspector.SnippetScriptMapping}
     */
    get scriptMapping()
    {
        return this._snippetScriptMapping;
    },

    _loadSnippets: function()
    {
        var snippets = this._snippetStorage.snippets();
        for (var i = 0; i < snippets.length; ++i)
            this._addScriptSnippet(snippets[i]);
    },

    /**
     * @return {WebInspector.UISourceCode}
     */
    createScriptSnippet: function()
    {
        var snippet = this._snippetStorage.createSnippet();
        return this._addScriptSnippet(snippet);
    },

    /**
     * @param {WebInspector.Snippet} snippet
     * @return {WebInspector.UISourceCode}
     */
    _addScriptSnippet: function(snippet)
    {
        var uiSourceCode = this._workspaceProvider.addFileByName("", snippet.name, new WebInspector.SnippetContentProvider(snippet), true);
        var scriptFile = new WebInspector.SnippetScriptFile(this, uiSourceCode);
        uiSourceCode.setScriptFile(scriptFile);
        this._snippetIdForUISourceCode.put(uiSourceCode, snippet.id);
        uiSourceCode.setSourceMapping(this._snippetScriptMapping);
        this._uiSourceCodeForSnippetId[snippet.id] = uiSourceCode;
        return uiSourceCode;
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    deleteScriptSnippet: function(uiSourceCode)
    {
        var snippetId = this._snippetIdForUISourceCode.get(uiSourceCode);
        var snippet = this._snippetStorage.snippetForId(snippetId);
        this._snippetStorage.deleteSnippet(snippet);
        this._removeBreakpoints(uiSourceCode);
        this._releaseSnippetScript(uiSourceCode);
        delete this._uiSourceCodeForSnippetId[snippet.id];
        this._snippetIdForUISourceCode.remove(uiSourceCode);
        this._workspaceProvider.removeFileByName("", snippet.name);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {string} newName
     */
    renameScriptSnippet: function(uiSourceCode, newName)
    {
        var breakpointLocations = this._removeBreakpoints(uiSourceCode);
        var snippetId = this._snippetIdForUISourceCode.get(uiSourceCode);
        var snippet = this._snippetStorage.snippetForId(snippetId);
        if (!snippet || !newName || snippet.name === newName)
            return;
        snippet.name = newName;
        this._restoreBreakpoints(uiSourceCode, breakpointLocations);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {string} newContent
     */
    _setScriptSnippetContent: function(uiSourceCode, newContent)
    {
        var snippetId = this._snippetIdForUISourceCode.get(uiSourceCode);
        var snippet = this._snippetStorage.snippetForId(snippetId);
        snippet.content = newContent;
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    _scriptSnippetEdited: function(uiSourceCode)
    {
        var script = this._scriptForUISourceCode.get(uiSourceCode);
        if (!script)
            return;

        var breakpointLocations = this._removeBreakpoints(uiSourceCode);
        var scriptUISourceCode = this._releaseSnippetScript(uiSourceCode);
        this._restoreBreakpoints(uiSourceCode, breakpointLocations);
        if (scriptUISourceCode)
            this._restoreBreakpoints(scriptUISourceCode, breakpointLocations);
    },

    /**
     * @param {string} snippetId
     * @return {number}
     */
    _nextEvaluationIndex: function(snippetId)
    {
        var evaluationIndex = this._lastSnippetEvaluationIndexSetting.get() + 1;
        this._lastSnippetEvaluationIndexSetting.set(evaluationIndex);
        return evaluationIndex;
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    evaluateScriptSnippet: function(uiSourceCode)
    {
        var breakpointLocations = this._removeBreakpoints(uiSourceCode);
        this._releaseSnippetScript(uiSourceCode);
        this._restoreBreakpoints(uiSourceCode, breakpointLocations);
        var snippetId = this._snippetIdForUISourceCode.get(uiSourceCode);
        var evaluationIndex = this._nextEvaluationIndex(snippetId);
        uiSourceCode._evaluationIndex = evaluationIndex;
        var evaluationUrl = this._evaluationSourceURL(uiSourceCode);

        var expression = uiSourceCode.workingCopy();
        
        // In order to stop on the breakpoints during the snippet evaluation we need to compile and run it separately.
        // If separate compilation and execution is not supported by the port we fall back to evaluation in console.
        // In case we don't need that since debugger is already paused.
        // We do the same when we are stopped on the call frame  since debugger is already paused and can not stop on breakpoint anymore.
        if (WebInspector.debuggerModel.selectedCallFrame() || !Capabilities.separateScriptCompilationAndExecutionEnabled) {
            expression = uiSourceCode.workingCopy() + "\n//@ sourceURL=" + evaluationUrl + "\n";
            WebInspector.evaluateInConsole(expression, true);
            return;
        }
        
        WebInspector.showConsole();
        DebuggerAgent.compileScript(expression, evaluationUrl, compileCallback.bind(this));

        /**
         * @param {?string} error
         * @param {string=} scriptId
         * @param {string=} syntaxErrorMessage
         */
        function compileCallback(error, scriptId, syntaxErrorMessage)
        {
            if (!uiSourceCode || uiSourceCode._evaluationIndex !== evaluationIndex)
                return;

            if (error) {
                console.error(error);
                return;
            }

            if (!scriptId) {
                var consoleMessage = WebInspector.ConsoleMessage.create(
                        WebInspector.ConsoleMessage.MessageSource.JS,
                        WebInspector.ConsoleMessage.MessageLevel.Error,
                        syntaxErrorMessage || "");
                WebInspector.console.addMessage(consoleMessage);
                return;
            }

            var breakpointLocations = this._removeBreakpoints(uiSourceCode);
            this._restoreBreakpoints(uiSourceCode, breakpointLocations);

            this._runScript(scriptId);
        }
    },

    /**
     * @param {DebuggerAgent.ScriptId} scriptId
     */
    _runScript: function(scriptId)
    {
        var currentExecutionContext = WebInspector.runtimeModel.currentExecutionContext();
        DebuggerAgent.runScript(scriptId, currentExecutionContext ? currentExecutionContext.id : undefined, "console", false, runCallback.bind(this));

        /**
         * @param {?string} error
         * @param {?RuntimeAgent.RemoteObject} result
         * @param {boolean=} wasThrown
         */
        function runCallback(error, result, wasThrown)
        {
            if (error) {
                console.error(error);
                return;
            }

            this._printRunScriptResult(result, wasThrown);
        }
    },

    /**
     * @param {?RuntimeAgent.RemoteObject} result
     * @param {boolean=} wasThrown
     */
    _printRunScriptResult: function(result, wasThrown)
    {
        var level = (wasThrown ? WebInspector.ConsoleMessage.MessageLevel.Error : WebInspector.ConsoleMessage.MessageLevel.Log);
        var message = WebInspector.ConsoleMessage.create(WebInspector.ConsoleMessage.MessageSource.JS, level, "", undefined, undefined, undefined, undefined, [result]);
        WebInspector.console.addMessage(message)
    },

    /**
     * @param {WebInspector.DebuggerModel.Location} rawLocation
     * @return {WebInspector.UILocation}
     */
    _rawLocationToUILocation: function(rawLocation)
    {
        var uiSourceCode = this._uiSourceCodeForScriptId[rawLocation.scriptId];
        return new WebInspector.UILocation(uiSourceCode, rawLocation.lineNumber, rawLocation.columnNumber || 0);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     * @param {number} columnNumber
     * @return {WebInspector.DebuggerModel.Location}
     */
    _uiLocationToRawLocation: function(uiSourceCode, lineNumber, columnNumber)
    {
        var script = this._scriptForUISourceCode.get(uiSourceCode);
        if (!script)
            return null;

        return WebInspector.debuggerModel.createRawLocation(script, lineNumber, columnNumber);
    },

    /**
     * @param {WebInspector.Script} script
     */
    _addScript: function(script)
    {
        var snippetId = this._snippetIdForSourceURL(script.sourceURL);
        if (!snippetId)
            return;
        var uiSourceCode = this._uiSourceCodeForSnippetId[snippetId];

        if (!uiSourceCode || this._evaluationSourceURL(uiSourceCode) !== script.sourceURL)
            return;

        console.assert(!this._scriptForUISourceCode.get(uiSourceCode));
        this._uiSourceCodeForScriptId[script.scriptId] = uiSourceCode;
        this._scriptForUISourceCode.put(uiSourceCode, script);
        uiSourceCode.scriptFile().setHasDivergedFromVM(false);
        script.pushSourceMapping(this._snippetScriptMapping);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @return {Array.<Object>}
     */
    _removeBreakpoints: function(uiSourceCode)
    {
        var breakpointLocations = WebInspector.breakpointManager.breakpointLocationsForUISourceCode(uiSourceCode);
        for (var i = 0; i < breakpointLocations.length; ++i)
            breakpointLocations[i].breakpoint.remove();
        return breakpointLocations;
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {Array.<Object>} breakpointLocations
     */
    _restoreBreakpoints: function(uiSourceCode, breakpointLocations)
    {
        for (var i = 0; i < breakpointLocations.length; ++i) {
            var uiLocation = breakpointLocations[i].uiLocation;
            var breakpoint = breakpointLocations[i].breakpoint;
            WebInspector.breakpointManager.setBreakpoint(uiSourceCode, uiLocation.lineNumber, breakpoint.condition(), breakpoint.enabled());
        }
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @return {WebInspector.UISourceCode}
     */
    _releaseSnippetScript: function(uiSourceCode)
    {
        var script = this._scriptForUISourceCode.get(uiSourceCode);
        if (!script)
            return null;

        uiSourceCode.scriptFile().setIsDivergingFromVM(true);
        uiSourceCode.scriptFile().setHasDivergedFromVM(true);
        delete this._uiSourceCodeForScriptId[script.scriptId];
        this._scriptForUISourceCode.remove(uiSourceCode);
        delete uiSourceCode._evaluationIndex;
        script.popSourceMapping(this._snippetScriptMapping);
        uiSourceCode.scriptFile().setIsDivergingFromVM(false);
        return script.rawLocationToUILocation(0, 0).uiSourceCode;
    },

    _debuggerReset: function()
    {
        for (var snippetId in this._uiSourceCodeForSnippetId) {
            var uiSourceCode = this._uiSourceCodeForSnippetId[snippetId];
            this._releaseSnippetScript(uiSourceCode);
        }
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @return {string}
     */
    _evaluationSourceURL: function(uiSourceCode)
    {
        var evaluationSuffix = "_" + uiSourceCode._evaluationIndex;
        var snippetId = this._snippetIdForUISourceCode.get(uiSourceCode);
        return WebInspector.Script.snippetSourceURLPrefix + snippetId + evaluationSuffix;
    },

    /**
     * @param {string} sourceURL
     * @return {string|null}
     */
    _snippetIdForSourceURL: function(sourceURL)
    {
        var snippetPrefix = WebInspector.Script.snippetSourceURLPrefix;
        if (!sourceURL.startsWith(snippetPrefix))
            return null;
        var splittedURL = sourceURL.substring(snippetPrefix.length).split("_");
        var snippetId = splittedURL[0];
        return snippetId;
    },

    reset: function()
    {
        /** @type {!Object.<string, WebInspector.UISourceCode>} */
        this._uiSourceCodeForScriptId = {};
        this._scriptForUISourceCode = new Map();
        /** @type {!Object.<string, WebInspector.UISourceCode>} */
        this._uiSourceCodeForSnippetId = {};
        this._snippetIdForUISourceCode = new Map();
        this._workspaceProvider.reset();
        this._loadSnippets();
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 * @implements {WebInspector.ScriptFile}
 * @extends {WebInspector.Object}
 * @param {WebInspector.ScriptSnippetModel} scriptSnippetModel
 * @param {WebInspector.UISourceCode} uiSourceCode
 */
WebInspector.SnippetScriptFile = function(scriptSnippetModel, uiSourceCode)
{
    WebInspector.ScriptFile.call(this);
    this._scriptSnippetModel = scriptSnippetModel;
    this._uiSourceCode = uiSourceCode;
    this._hasDivergedFromVM = true;
    this._uiSourceCode.addEventListener(WebInspector.UISourceCode.Events.WorkingCopyCommitted, this._workingCopyCommitted, this);
    this._uiSourceCode.addEventListener(WebInspector.UISourceCode.Events.WorkingCopyChanged, this._workingCopyChanged, this);
}

WebInspector.SnippetScriptFile.prototype = {
    /**
     * @return {boolean}
     */
    hasDivergedFromVM: function()
    {
        return this._hasDivergedFromVM;
    },

    /**
     * @param {boolean} hasDivergedFromVM
     */
    setHasDivergedFromVM: function(hasDivergedFromVM)
    {
        this._hasDivergedFromVM = hasDivergedFromVM;
    },

    /**
     * @return {boolean}
     */
    isDivergingFromVM: function()
    {
        return this._isDivergingFromVM;
    },

    checkMapping: function()
    {
    },

    /**
     * @return {boolean}
     */
    isMergingToVM: function()
    {
        return false;
    },

    /**
     * @param {boolean} isDivergingFromVM
     */
    setIsDivergingFromVM: function(isDivergingFromVM)
    {
        this._isDivergingFromVM = isDivergingFromVM;
    },

    _workingCopyCommitted: function()
    {
        this._scriptSnippetModel._setScriptSnippetContent(this._uiSourceCode, this._uiSourceCode.workingCopy());
    },

    _workingCopyChanged: function()
    {
        this._scriptSnippetModel._scriptSnippetEdited(this._uiSourceCode);
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 * @implements {WebInspector.ScriptSourceMapping}
 * @param {WebInspector.ScriptSnippetModel} scriptSnippetModel
 */
WebInspector.SnippetScriptMapping = function(scriptSnippetModel)
{
    this._scriptSnippetModel = scriptSnippetModel;
}

WebInspector.SnippetScriptMapping.prototype = {
    /**
     * @param {WebInspector.RawLocation} rawLocation
     * @return {WebInspector.UILocation}
     */
    rawLocationToUILocation: function(rawLocation)
    {
        var debuggerModelLocation = /** @type {WebInspector.DebuggerModel.Location} */(rawLocation);
        return this._scriptSnippetModel._rawLocationToUILocation(debuggerModelLocation);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     * @param {number} columnNumber
     * @return {WebInspector.DebuggerModel.Location}
     */
    uiLocationToRawLocation: function(uiSourceCode, lineNumber, columnNumber)
    {
        return this._scriptSnippetModel._uiLocationToRawLocation(uiSourceCode, lineNumber, columnNumber);
    },

    /**
     * @return {boolean}
     */
    isIdentity: function()
    {
        return true;
    },

    /**
     * @param {string} sourceURL
     * @return {string|null}
     */
    snippetIdForSourceURL: function(sourceURL)
    {
        return this._scriptSnippetModel._snippetIdForSourceURL(sourceURL);
    },

    /**
     * @param {WebInspector.Script} script
     */
    addScript: function(script)
    {
        this._scriptSnippetModel._addScript(script);
    }
}

/**
 * @constructor
 * @extends {WebInspector.StaticContentProvider}
 * @param {WebInspector.Snippet} snippet
 */
WebInspector.SnippetContentProvider = function(snippet)
{
    WebInspector.StaticContentProvider.call(this, WebInspector.resourceTypes.Script, snippet.content);
}

WebInspector.SnippetContentProvider.prototype = {
    __proto__: WebInspector.StaticContentProvider.prototype
}

/**
 * @type {?WebInspector.ScriptSnippetModel}
 */
WebInspector.scriptSnippetModel = null;
