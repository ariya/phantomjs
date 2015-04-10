/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 */
WebInspector.DebuggerModel = function()
{
    InspectorBackend.registerDebuggerDispatcher(new WebInspector.DebuggerDispatcher(this));

    this._debuggerPausedDetails = null;
    /**
     * @type {Object.<string, WebInspector.Script>}
     */
    this._scripts = {};
    this._scriptsBySourceURL = {};

    this._canSetScriptSource = false;
    this._breakpointsActive = true;

    WebInspector.settings.pauseOnExceptionStateString = WebInspector.settings.createSetting("pauseOnExceptionStateString", WebInspector.DebuggerModel.PauseOnExceptionsState.DontPauseOnExceptions);
    WebInspector.settings.pauseOnExceptionStateString.addChangeListener(this._pauseOnExceptionStateChanged, this);

    if (!Capabilities.debuggerCausesRecompilation || WebInspector.settings.debuggerEnabled.get())
        this.enableDebugger();
}

// Keep these in sync with WebCore::ScriptDebugServer
WebInspector.DebuggerModel.PauseOnExceptionsState = {
    DontPauseOnExceptions : "none",
    PauseOnAllExceptions : "all",
    PauseOnUncaughtExceptions: "uncaught"
};

/**
 * @constructor
 * @implements {WebInspector.RawLocation}
 * @param {string} scriptId
 * @param {number} lineNumber
 * @param {number} columnNumber
 */
WebInspector.DebuggerModel.Location = function(scriptId, lineNumber, columnNumber)
{
    this.scriptId = scriptId;
    this.lineNumber = lineNumber;
    this.columnNumber = columnNumber;
}

WebInspector.DebuggerModel.Events = {
    DebuggerWasEnabled: "DebuggerWasEnabled",
    DebuggerWasDisabled: "DebuggerWasDisabled",
    DebuggerPaused: "DebuggerPaused",
    DebuggerResumed: "DebuggerResumed",
    ParsedScriptSource: "ParsedScriptSource",
    FailedToParseScriptSource: "FailedToParseScriptSource",
    BreakpointResolved: "BreakpointResolved",
    GlobalObjectCleared: "GlobalObjectCleared",
    CallFrameSelected: "CallFrameSelected",
    ExecutionLineChanged: "ExecutionLineChanged",
    ConsoleCommandEvaluatedInSelectedCallFrame: "ConsoleCommandEvaluatedInSelectedCallFrame",
    BreakpointsActiveStateChanged: "BreakpointsActiveStateChanged"
}

WebInspector.DebuggerModel.BreakReason = {
    DOM: "DOM",
    EventListener: "EventListener",
    XHR: "XHR",
    Exception: "exception",
    Assert: "assert",
    CSPViolation: "CSPViolation"
}

WebInspector.DebuggerModel.prototype = {
    /**
     * @return {boolean}
     */
    debuggerEnabled: function()
    {
        return !!this._debuggerEnabled;
    },

    enableDebugger: function()
    {
        if (this._debuggerEnabled)
            return;

        function callback(error, result)
        {
            this._canSetScriptSource = result;
        }
        DebuggerAgent.canSetScriptSource(callback.bind(this));
        DebuggerAgent.enable(this._debuggerWasEnabled.bind(this));
    },

    disableDebugger: function()
    {
        if (!this._debuggerEnabled)
            return;

        DebuggerAgent.disable(this._debuggerWasDisabled.bind(this));
    },

    /**
     * @return {boolean}
     */
    canSetScriptSource: function()
    {
        return this._canSetScriptSource;
    },

    _debuggerWasEnabled: function()
    {
        this._debuggerEnabled = true;
        this._pauseOnExceptionStateChanged();
        this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.DebuggerWasEnabled);
    },

    _pauseOnExceptionStateChanged: function()
    {
        DebuggerAgent.setPauseOnExceptions(WebInspector.settings.pauseOnExceptionStateString.get());
    },

    _debuggerWasDisabled: function()
    {
        this._debuggerEnabled = false;
        this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.DebuggerWasDisabled);
    },

    /**
     * @param {WebInspector.DebuggerModel.Location} rawLocation
     */
    continueToLocation: function(rawLocation)
    {
        DebuggerAgent.continueToLocation(rawLocation);
    },

    /**
     * @param {WebInspector.DebuggerModel.Location} rawLocation
     * @param {string} condition
     * @param {function(?DebuggerAgent.BreakpointId, Array.<WebInspector.DebuggerModel.Location>):void=} callback
     */
    setBreakpointByScriptLocation: function(rawLocation, condition, callback)
    {
        var script = this.scriptForId(rawLocation.scriptId);
        if (script.sourceURL)
            this.setBreakpointByURL(script.sourceURL, rawLocation.lineNumber, rawLocation.columnNumber, condition, callback);
        else
            this.setBreakpointBySourceId(rawLocation, condition, callback);
    },

    /**
     * @param {string} url
     * @param {number} lineNumber
     * @param {number=} columnNumber
     * @param {string=} condition
     * @param {function(?DebuggerAgent.BreakpointId, Array.<WebInspector.DebuggerModel.Location>)=} callback
     */
    setBreakpointByURL: function(url, lineNumber, columnNumber, condition, callback)
    {
        // Adjust column if needed.
        var minColumnNumber = 0;
        var scripts = this._scriptsBySourceURL[url] || [];
        for (var i = 0, l = scripts.length; i < l; ++i) {
            var script = scripts[i];
            if (lineNumber === script.lineOffset)
                minColumnNumber = minColumnNumber ? Math.min(minColumnNumber, script.columnOffset) : script.columnOffset;
        }
        columnNumber = Math.max(columnNumber, minColumnNumber);

        /**
         * @this {WebInspector.DebuggerModel}
         * @param {?Protocol.Error} error
         * @param {DebuggerAgent.BreakpointId} breakpointId
         * @param {Array.<DebuggerAgent.Location>} locations
         */
        function didSetBreakpoint(error, breakpointId, locations)
        {
            if (callback) {
                var rawLocations = /** @type {Array.<WebInspector.DebuggerModel.Location>} */ (locations);
                callback(error ? null : breakpointId, rawLocations);
            }
        }
        DebuggerAgent.setBreakpointByUrl(lineNumber, url, undefined, columnNumber, condition, didSetBreakpoint.bind(this));
        WebInspector.userMetrics.ScriptsBreakpointSet.record();
    },

    /**
     * @param {WebInspector.DebuggerModel.Location} rawLocation
     * @param {string} condition
     * @param {function(?DebuggerAgent.BreakpointId, Array.<WebInspector.DebuggerModel.Location>)=} callback
     */
    setBreakpointBySourceId: function(rawLocation, condition, callback)
    {
        /**
         * @this {WebInspector.DebuggerModel}
         * @param {?Protocol.Error} error
         * @param {DebuggerAgent.BreakpointId} breakpointId
         * @param {DebuggerAgent.Location} actualLocation
         */
        function didSetBreakpoint(error, breakpointId, actualLocation)
        {
            if (callback) {
                var rawLocation = /** @type {WebInspector.DebuggerModel.Location} */ (actualLocation);
                callback(error ? null : breakpointId, [rawLocation]);
            }
        }
        DebuggerAgent.setBreakpoint(rawLocation, condition, didSetBreakpoint.bind(this));
        WebInspector.userMetrics.ScriptsBreakpointSet.record();
    },

    /**
     * @param {DebuggerAgent.BreakpointId} breakpointId
     * @param {function(?Protocol.Error)=} callback
     */
    removeBreakpoint: function(breakpointId, callback)
    {
        DebuggerAgent.removeBreakpoint(breakpointId, callback);
    },

    /**
     * @param {DebuggerAgent.BreakpointId} breakpointId
     * @param {DebuggerAgent.Location} location
     */
    _breakpointResolved: function(breakpointId, location)
    {
        this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.BreakpointResolved, {breakpointId: breakpointId, location: location});
    },

    _globalObjectCleared: function()
    {
        this._setDebuggerPausedDetails(null);
        this._reset();
        this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.GlobalObjectCleared);
    },

    _reset: function()
    {
        this._scripts = {};
        this._scriptsBySourceURL = {};
    },

    /**
     * @return {Object.<string, WebInspector.Script>}
     */
    get scripts()
    {
        return this._scripts;
    },

    /**
     * @param {DebuggerAgent.ScriptId} scriptId
     * @return {WebInspector.Script}
     */
    scriptForId: function(scriptId)
    {
        return this._scripts[scriptId] || null;
    },

    /**
     * @param {DebuggerAgent.ScriptId} scriptId
     * @param {string} newSource
     * @param {function(?Protocol.Error)} callback
     */
    setScriptSource: function(scriptId, newSource, callback)
    {
        this._scripts[scriptId].editSource(newSource, this._didEditScriptSource.bind(this, scriptId, newSource, callback));
    },

    /**
     * @param {DebuggerAgent.ScriptId} scriptId
     * @param {string} newSource
     * @param {function(?Protocol.Error)} callback
     * @param {?Protocol.Error} error
     * @param {Array.<DebuggerAgent.CallFrame>=} callFrames
     */
    _didEditScriptSource: function(scriptId, newSource, callback, error, callFrames)
    {
        callback(error);
        if (!error && callFrames && callFrames.length)
            this._pausedScript(callFrames, this._debuggerPausedDetails.reason, this._debuggerPausedDetails.auxData);
    },

    /**
     * @return {Array.<DebuggerAgent.CallFrame>}
     */
    get callFrames()
    {
        return this._debuggerPausedDetails ? this._debuggerPausedDetails.callFrames : null;
    },

    /**
     * @return {?WebInspector.DebuggerPausedDetails}
     */
    debuggerPausedDetails: function()
    {
        return this._debuggerPausedDetails;
    },

    /**
     * @param {?WebInspector.DebuggerPausedDetails} debuggerPausedDetails
     */
    _setDebuggerPausedDetails: function(debuggerPausedDetails)
    {
        if (this._debuggerPausedDetails)
            this._debuggerPausedDetails.dispose();
        this._debuggerPausedDetails = debuggerPausedDetails;
        if (this._debuggerPausedDetails)
            this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.DebuggerPaused, this._debuggerPausedDetails);
        if (debuggerPausedDetails) {
            this.setSelectedCallFrame(debuggerPausedDetails.callFrames[0]);
            DebuggerAgent.setOverlayMessage(WebInspector.UIString("Paused in debugger"));
        } else {
            this.setSelectedCallFrame(null);
            DebuggerAgent.setOverlayMessage();
        }
    },

    /**
     * @param {Array.<DebuggerAgent.CallFrame>} callFrames
     * @param {string} reason
     * @param {*} auxData
     */
    _pausedScript: function(callFrames, reason, auxData)
    {
        this._setDebuggerPausedDetails(new WebInspector.DebuggerPausedDetails(this, callFrames, reason, auxData));
    },

    _resumedScript: function()
    {
        this._setDebuggerPausedDetails(null);
        if (this._executionLineLiveLocation)
            this._executionLineLiveLocation.dispose();
        this._executionLineLiveLocation = null;
        this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.DebuggerResumed);
    },

    /**
     * @param {DebuggerAgent.ScriptId} scriptId
     * @param {string} sourceURL
     * @param {number} startLine
     * @param {number} startColumn
     * @param {number} endLine
     * @param {number} endColumn
     * @param {boolean} isContentScript
     * @param {string=} sourceMapURL
     * @param {boolean=} hasSourceURL
     */
    _parsedScriptSource: function(scriptId, sourceURL, startLine, startColumn, endLine, endColumn, isContentScript, sourceMapURL, hasSourceURL)
    {
        var script = new WebInspector.Script(scriptId, sourceURL, startLine, startColumn, endLine, endColumn, isContentScript, sourceMapURL, hasSourceURL);
        if (!script.isAnonymousScript() && !script.isInlineScript()) {
            var existingScripts = this._scriptsBySourceURL[script.sourceURL] || [];
            for (var i = 0; i < existingScripts.length; ++i) {
                if (existingScripts[i].isInlineScript()) {
                    script.setIsDynamicScript(true); 
                    break;
                }
            }
        }
        this._registerScript(script);
        this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.ParsedScriptSource, script);
    },

    /**
     * @param {WebInspector.Script} script
     */
    _registerScript: function(script)
    {
        this._scripts[script.scriptId] = script;
        if (script.sourceURL) {
            var scripts = this._scriptsBySourceURL[script.sourceURL];
            if (!scripts) {
                scripts = [];
                this._scriptsBySourceURL[script.sourceURL] = scripts;
            }
            scripts.push(script);
        }
    },

    /**
     * @param {WebInspector.Script} script
     * @param {number} lineNumber
     * @param {number} columnNumber
     * @return {WebInspector.DebuggerModel.Location}
     */
    createRawLocation: function(script, lineNumber, columnNumber)
    {
        if (script.sourceURL)
            return this.createRawLocationByURL(script.sourceURL, lineNumber, columnNumber)
        return new WebInspector.DebuggerModel.Location(script.scriptId, lineNumber, columnNumber);
    },

    /**
     * @param {string} sourceURL
     * @param {number} lineNumber
     * @param {number} columnNumber
     * @return {WebInspector.DebuggerModel.Location}
     */
    createRawLocationByURL: function(sourceURL, lineNumber, columnNumber)
    {
        var closestScript = null;
        var scripts = this._scriptsBySourceURL[sourceURL] || [];
        for (var i = 0, l = scripts.length; i < l; ++i) {
            var script = scripts[i];
            if (!closestScript)
                closestScript = script;
            if (script.lineOffset > lineNumber || (script.lineOffset === lineNumber && script.columnOffset > columnNumber))
                continue;
            if (script.endLine < lineNumber || (script.endLine === lineNumber && script.endColumn <= columnNumber))
                continue;
            closestScript = script;
            break;
        }
        return closestScript ? new WebInspector.DebuggerModel.Location(closestScript.scriptId, lineNumber, columnNumber) : null;
    },

    /**
     * @return {boolean}
     */
    isPaused: function()
    {
        return !!this.debuggerPausedDetails();
    },

    /**
     * @param {?WebInspector.DebuggerModel.CallFrame} callFrame
     */
    setSelectedCallFrame: function(callFrame)
    {
        if (this._executionLineLiveLocation)
            this._executionLineLiveLocation.dispose();
        delete this._executionLineLiveLocation;

        this._selectedCallFrame = callFrame;
        if (!this._selectedCallFrame)
            return;

        this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.CallFrameSelected, callFrame);

        function updateExecutionLine(uiLocation)
        {
            this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.ExecutionLineChanged, uiLocation);
        }
        this._executionLineLiveLocation = callFrame.script.createLiveLocation(callFrame.location, updateExecutionLine.bind(this));
    },

    /**
     * @return {?WebInspector.DebuggerModel.CallFrame}
     */
    selectedCallFrame: function()
    {
        return this._selectedCallFrame;
    },

    /**
     * @param {string} code
     * @param {string} objectGroup
     * @param {boolean} includeCommandLineAPI
     * @param {boolean} doNotPauseOnExceptionsAndMuteConsole
     * @param {boolean} returnByValue
     * @param {boolean} generatePreview
     * @param {function(?WebInspector.RemoteObject, boolean, RuntimeAgent.RemoteObject=)} callback
     */
    evaluateOnSelectedCallFrame: function(code, objectGroup, includeCommandLineAPI, doNotPauseOnExceptionsAndMuteConsole, returnByValue, generatePreview, callback)
    {
        /**
         * @param {?RuntimeAgent.RemoteObject} result
         * @param {boolean=} wasThrown
         */
        function didEvaluate(result, wasThrown)
        {
            if (returnByValue)
                callback(null, !!wasThrown, wasThrown ? null : result);
            else
                callback(WebInspector.RemoteObject.fromPayload(result), !!wasThrown);

            if (objectGroup === "console")
                this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.ConsoleCommandEvaluatedInSelectedCallFrame);
        }

        this.selectedCallFrame().evaluate(code, objectGroup, includeCommandLineAPI, doNotPauseOnExceptionsAndMuteConsole, returnByValue, generatePreview, didEvaluate.bind(this));
    },

    /**
     * @param {function(Object)} callback
     */
    getSelectedCallFrameVariables: function(callback)
    {
        var result = { this: true };

        var selectedCallFrame = this._selectedCallFrame;
        if (!selectedCallFrame)
            callback(result);

        var pendingRequests = 0;

        function propertiesCollected(properties)
        {
            for (var i = 0; properties && i < properties.length; ++i)
                result[properties[i].name] = true;
            if (--pendingRequests == 0)
                callback(result);
        }

        for (var i = 0; i < selectedCallFrame.scopeChain.length; ++i) {
            var scope = selectedCallFrame.scopeChain[i];
            var object = WebInspector.RemoteObject.fromPayload(scope.object);
            pendingRequests++;
            object.getAllProperties(propertiesCollected);
        }
    },

    /**
     * @param {boolean} active
     */
    setBreakpointsActive: function(active)
    {
        if (this._breakpointsActive === active)
            return;
        this._breakpointsActive = active;
        DebuggerAgent.setBreakpointsActive(active);
        this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.BreakpointsActiveStateChanged, active);
    },

    /**
     * @return {boolean}
     */
    breakpointsActive: function()
    {
        return this._breakpointsActive;
    },

    /**
     * @param {WebInspector.DebuggerModel.Location} rawLocation
     * @param {function(WebInspector.UILocation):(boolean|undefined)} updateDelegate
     * @return {WebInspector.Script.Location}
     */
    createLiveLocation: function(rawLocation, updateDelegate)
    {
        var script = this._scripts[rawLocation.scriptId];
        return script.createLiveLocation(rawLocation, updateDelegate);
    },

    /**
     * @param {WebInspector.DebuggerModel.Location} rawLocation
     * @return {?WebInspector.UILocation}
     */
    rawLocationToUILocation: function(rawLocation)
    {
        var script = this._scripts[rawLocation.scriptId];
        if (!script)
            return null;
        return script.rawLocationToUILocation(rawLocation.lineNumber, rawLocation.columnNumber);
    },

    /**
     * Handles notification from JavaScript VM about updated stack (liveedit or frame restart action).
     * @this {WebInspector.DebuggerModel}
     * @param {Array.<DebuggerAgent.CallFrame>=} newCallFrames
     * @param {Object=} details
     */
    callStackModified: function(newCallFrames, details)
    {
        // FIXME: declare this property in protocol and in JavaScript.
        if (details && details["stack_update_needs_step_in"])
            DebuggerAgent.stepInto();
        else {
            if (newCallFrames && newCallFrames.length)
                this._pausedScript(newCallFrames, this._debuggerPausedDetails.reason, this._debuggerPausedDetails.auxData);

        }
    },

    __proto__: WebInspector.Object.prototype
}

WebInspector.DebuggerEventTypes = {
    JavaScriptPause: 0,
    JavaScriptBreakpoint: 1,
    NativeBreakpoint: 2
};

/**
 * @constructor
 * @implements {DebuggerAgent.Dispatcher}
 * @param {WebInspector.DebuggerModel} debuggerModel
 */
WebInspector.DebuggerDispatcher = function(debuggerModel)
{
    this._debuggerModel = debuggerModel;
}

WebInspector.DebuggerDispatcher.prototype = {
    /**
     * @param {Array.<DebuggerAgent.CallFrame>} callFrames
     * @param {string} reason
     * @param {Object=} auxData
     */
    paused: function(callFrames, reason, auxData)
    {
        this._debuggerModel._pausedScript(callFrames, reason, auxData);
    },

    resumed: function()
    {
        this._debuggerModel._resumedScript();
    },

    globalObjectCleared: function()
    {
        this._debuggerModel._globalObjectCleared();
    },

    /**
     * @param {DebuggerAgent.ScriptId} scriptId
     * @param {string} sourceURL
     * @param {number} startLine
     * @param {number} startColumn
     * @param {number} endLine
     * @param {number} endColumn
     * @param {boolean=} isContentScript
     * @param {string=} sourceMapURL
     * @param {boolean=} hasSourceURL
     */
    scriptParsed: function(scriptId, sourceURL, startLine, startColumn, endLine, endColumn, isContentScript, sourceMapURL, hasSourceURL)
    {
        this._debuggerModel._parsedScriptSource(scriptId, sourceURL, startLine, startColumn, endLine, endColumn, !!isContentScript, sourceMapURL, hasSourceURL);
    },

    /**
     * @param {string} sourceURL
     * @param {string} source
     * @param {number} startingLine
     * @param {number} errorLine
     * @param {string} errorMessage
     */
    scriptFailedToParse: function(sourceURL, source, startingLine, errorLine, errorMessage)
    {
    },

    /**
    * @param {DebuggerAgent.BreakpointId} breakpointId
    * @param {DebuggerAgent.Location} location
     */
    breakpointResolved: function(breakpointId, location)
    {
        this._debuggerModel._breakpointResolved(breakpointId, location);
    }
}

/**
 * @constructor
 * @param {WebInspector.Script} script
 * @param {DebuggerAgent.CallFrame} payload
 */
WebInspector.DebuggerModel.CallFrame = function(script, payload)
{
    this._script = script;
    this._payload = payload;
    this._locations = [];
}

WebInspector.DebuggerModel.CallFrame.prototype = {
    /**
     * @return {WebInspector.Script}
     */
    get script()
    {
        return this._script;
    },

    /**
     * @return {string}
     */
    get type()
    {
        return this._payload.type;
    },

    /**
     * @return {string}
     */
    get id()
    {
        return this._payload.callFrameId;
    },

    /**
     * @return {Array.<DebuggerAgent.Scope>}
     */
    get scopeChain()
    {
        return this._payload.scopeChain;
    },

    /**
     * @return {RuntimeAgent.RemoteObject}
     */
    get this()
    {
        return this._payload.this;
    },

    /**
     * @return {string}
     */
    get functionName()
    {
        return this._payload.functionName;
    },

    /**
     * @return {WebInspector.DebuggerModel.Location}
     */
    get location()
    {
        var rawLocation = /** @type {WebInspector.DebuggerModel.Location} */ (this._payload.location);
        return rawLocation;
    },

    /**
     * @param {string} code
     * @param {string} objectGroup
     * @param {boolean} includeCommandLineAPI
     * @param {boolean} doNotPauseOnExceptionsAndMuteConsole
     * @param {boolean} returnByValue
     * @param {boolean} generatePreview
     * @param {function(?RuntimeAgent.RemoteObject, boolean=)=} callback
     */
    evaluate: function(code, objectGroup, includeCommandLineAPI, doNotPauseOnExceptionsAndMuteConsole, returnByValue, generatePreview, callback)
    {
        /**
         * @this {WebInspector.DebuggerModel.CallFrame}
         * @param {?Protocol.Error} error
         * @param {RuntimeAgent.RemoteObject} result
         * @param {boolean=} wasThrown
         */
        function didEvaluateOnCallFrame(error, result, wasThrown)
        {
            if (error) {
                console.error(error);
                callback(null, false);
                return;
            }
            callback(result, wasThrown);
        }
        DebuggerAgent.evaluateOnCallFrame(this._payload.callFrameId, code, objectGroup, includeCommandLineAPI, doNotPauseOnExceptionsAndMuteConsole, returnByValue, generatePreview, didEvaluateOnCallFrame.bind(this));
    },

    /**
     * @param {function(?Protocol.Error=)=} callback
     */
    restart: function(callback)
    {
        /**
         * @this {WebInspector.DebuggerModel.CallFrame}
         * @param {?Protocol.Error} error
         * @param {Array.<DebuggerAgent.CallFrame>=} callFrames
         * @param {Object=} details
         */
        function protocolCallback(error, callFrames, details)
        {
            if (!error)
                WebInspector.debuggerModel.callStackModified(callFrames, details);
            if (callback)
                callback(error);
        }
        DebuggerAgent.restartFrame(this._payload.callFrameId, protocolCallback);
    },

    /**
     * @param {function(WebInspector.UILocation):(boolean|undefined)} updateDelegate
     */
    createLiveLocation: function(updateDelegate)
    {
        var location = this._script.createLiveLocation(this.location, updateDelegate);
        this._locations.push(location);
        return location;
    },

    dispose: function(updateDelegate)
    {
        for (var i = 0; i < this._locations.length; ++i)
            this._locations[i].dispose();
        this._locations = [];
    }
}

/**
 * @constructor
 * @param {WebInspector.DebuggerModel} model
 * @param {Array.<DebuggerAgent.CallFrame>} callFrames
 * @param {string} reason
 * @param {*} auxData
 */
WebInspector.DebuggerPausedDetails = function(model, callFrames, reason, auxData)
{
    this.callFrames = [];
    for (var i = 0; i < callFrames.length; ++i) {
        var callFrame = callFrames[i];
        var script = model.scriptForId(callFrame.location.scriptId);
        if (script)
            this.callFrames.push(new WebInspector.DebuggerModel.CallFrame(script, callFrame));
    }
    this.reason = reason;
    this.auxData = auxData;
}

WebInspector.DebuggerPausedDetails.prototype = {
    dispose: function()
    {
        for (var i = 0; i < this.callFrames.length; ++i) {
            var callFrame = this.callFrames[i];
            callFrame.dispose();
        }
    }
}

/**
 * @type {?WebInspector.DebuggerModel}
 */
WebInspector.debuggerModel = null;
