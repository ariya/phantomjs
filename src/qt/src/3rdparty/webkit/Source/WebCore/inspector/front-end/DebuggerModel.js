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

WebInspector.DebuggerModel = function()
{
    this._debuggerPausedDetails = {};
    this._scripts = {};

    InspectorBackend.registerDomainDispatcher("Debugger", new WebInspector.DebuggerDispatcher(this));
}

WebInspector.DebuggerModel.Events = {
    DebuggerWasEnabled: "debugger-was-enabled",
    DebuggerWasDisabled: "debugger-was-disabled",
    DebuggerPaused: "debugger-paused",
    DebuggerResumed: "debugger-resumed",
    ParsedScriptSource: "parsed-script-source",
    FailedToParseScriptSource: "failed-to-parse-script-source",
    BreakpointResolved: "breakpoint-resolved",
    Reset: "reset"
}

WebInspector.DebuggerModel.prototype = {
    enableDebugger: function()
    {
        DebuggerAgent.enable();
    },

    disableDebugger: function()
    {
        DebuggerAgent.disable();
    },

    _debuggerWasEnabled: function()
    {
        this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.DebuggerWasEnabled);
    },

    _debuggerWasDisabled: function()
    {
        this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.DebuggerWasDisabled);
    },

    continueToLocation: function(location)
    {
        DebuggerAgent.continueToLocation(location);
    },

    setBreakpoint: function(url, lineNumber, columnNumber, condition, callback)
    {
        // Adjust column if needed.
        var minColumnNumber = 0;
        for (var id in this._scripts) {
            var script = this._scripts[id];
            if (url === script.sourceURL && lineNumber === script.lineOffset)
                minColumnNumber = minColumnNumber ? Math.min(minColumnNumber, script.columnOffset) : script.columnOffset;
        }
        columnNumber = Math.max(columnNumber, minColumnNumber);

        function didSetBreakpoint(error, breakpointId, locations)
        {
            if (callback)
                callback(error ? null : breakpointId, locations);
        }
        DebuggerAgent.setBreakpointByUrl(url, lineNumber, columnNumber, condition, didSetBreakpoint.bind(this));
    },

    setBreakpointBySourceId: function(location, condition, callback)
    {
        function didSetBreakpoint(error, breakpointId, actualLocation)
        {
            if (callback)
                callback(error ? null : breakpointId, [actualLocation]);
        }
        DebuggerAgent.setBreakpoint(location, condition, didSetBreakpoint.bind(this));
    },

    removeBreakpoint: function(breakpointId, callback)
    {
        DebuggerAgent.removeBreakpoint(breakpointId, callback);
    },

    _breakpointResolved: function(breakpointId, location)
    {
        this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.BreakpointResolved, {breakpointId: breakpointId, location: location});
    },

    reset: function()
    {
        this._debuggerPausedDetails = {};
        this._scripts = {};
        this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.Reset);
    },

    get scripts()
    {
        return this._scripts;
    },

    scriptForSourceID: function(sourceID)
    {
        return this._scripts[sourceID];
    },

    scriptsForURL: function(url)
    {
        return this.queryScripts(function(s) { return s.sourceURL === url; });
    },

    queryScripts: function(filter)
    {
        var scripts = [];
        for (var sourceID in this._scripts) {
            var script = this._scripts[sourceID];
            if (filter(script))
                scripts.push(script);
        }
        return scripts;
    },

    editScriptSource: function(sourceID, newSource, callback)
    {
        this._scripts[sourceID].editSource(newSource, this._didEditScriptSource.bind(this, sourceID, newSource, callback));
    },

    _didEditScriptSource: function(sourceID, newSource, callback, error, callFrames)
    {
        if (!error && callFrames && callFrames.length)
            this._debuggerPausedDetails.callFrames = callFrames;
        callback(error);
    },

    get callFrames()
    {
        return this._debuggerPausedDetails.callFrames;
    },

    get debuggerPausedDetails()
    {
        return this._debuggerPausedDetails;
    },

    _pausedScript: function(details)
    {
        this._debuggerPausedDetails = details;
        this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.DebuggerPaused, details);
    },

    _resumedScript: function()
    {
        this._debuggerPausedDetails = {};
        this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.DebuggerResumed);
    },

    _parsedScriptSource: function(sourceID, sourceURL, startLine, startColumn, endLine, endColumn, isContentScript)
    {
        var script = new WebInspector.Script(sourceID, sourceURL, startLine, startColumn, endLine, endColumn, undefined, undefined, isContentScript);
        this._scripts[sourceID] = script;
        this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.ParsedScriptSource, script);
    },

    _failedToParseScriptSource: function(sourceURL, source, startingLine, errorLine, errorMessage)
    {
        var script = new WebInspector.Script(null, sourceURL, startingLine, 0, 0, 0, errorLine, errorMessage, undefined);
        this.dispatchEventToListeners(WebInspector.DebuggerModel.Events.FailedToParseScriptSource, script);
    }
}

WebInspector.DebuggerModel.prototype.__proto__ = WebInspector.Object.prototype;

WebInspector.DebuggerEventTypes = {
    JavaScriptPause: 0,
    JavaScriptBreakpoint: 1,
    NativeBreakpoint: 2
};

WebInspector.DebuggerDispatcher = function(debuggerModel)
{
    this._debuggerModel = debuggerModel;
}

WebInspector.DebuggerDispatcher.prototype = {
    paused: function(details)
    {
        this._debuggerModel._pausedScript(details);
    },

    resumed: function()
    {
        this._debuggerModel._resumedScript();
    },

    debuggerWasEnabled: function()
    {
        this._debuggerModel._debuggerWasEnabled();
    },

    debuggerWasDisabled: function()
    {
        this._debuggerModel._debuggerWasDisabled();
    },

    scriptParsed: function(sourceID, sourceURL, startLine, startColumn, endLine, endColumn, isContentScript)
    {
        this._debuggerModel._parsedScriptSource(sourceID, sourceURL, startLine, startColumn, endLine, endColumn, isContentScript);
    },

    scriptFailedToParse: function(sourceURL, source, startingLine, errorLine, errorMessage)
    {
        this._debuggerModel._failedToParseScriptSource(sourceURL, source, startingLine, errorLine, errorMessage);
    },

    breakpointResolved: function(breakpointId, sourceID, lineNumber, columnNumber)
    {
        this._debuggerModel._breakpointResolved(breakpointId, sourceID, lineNumber, columnNumber);
    }
}
