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

WebInspector.DebuggerManager = function()
{
    WebInspector.Object.call(this);

    DebuggerAgent.enable();

    WebInspector.Breakpoint.addEventListener(WebInspector.Breakpoint.Event.DisplayLocationDidChange, this._breakpointDisplayLocationDidChange, this);
    WebInspector.Breakpoint.addEventListener(WebInspector.Breakpoint.Event.DisabledStateDidChange, this._breakpointDisabledStateDidChange, this);
    WebInspector.Breakpoint.addEventListener(WebInspector.Breakpoint.Event.ConditionDidChange, this._breakpointConditionDidChange, this);

    window.addEventListener("pagehide", this._inspectorClosing.bind(this));

    this._allExceptionsBreakpointEnabledSetting = new WebInspector.Setting("break-on-all-exceptions", false);
    this._allUncaughtExceptionsBreakpointEnabledSetting = new WebInspector.Setting("break-on-all-uncaught-exceptions", false);

    var specialBreakpointLocation = new WebInspector.SourceCodeLocation(null, Infinity, Infinity);

    this._allExceptionsBreakpoint = new WebInspector.Breakpoint(specialBreakpointLocation, !this._allExceptionsBreakpointEnabledSetting.value);
    this._allExceptionsBreakpoint.resolved = true;

    this._allUncaughtExceptionsBreakpoint = new WebInspector.Breakpoint(specialBreakpointLocation, !this._allUncaughtExceptionsBreakpointEnabledSetting.value);

    this._breakpoints = [];
    this._breakpointURLMap = {};
    this._breakpointScriptIdentifierMap = {};
    this._breakpointIdMap = {};

    this._scriptIdMap = {};
    this._scriptURLMap = {};

    this._breakpointsSetting = new WebInspector.Setting("breakpoints", []);
    this._breakpointsEnabledSetting = new WebInspector.Setting("breakpoints-enabled", true);

    DebuggerAgent.setBreakpointsActive(this._breakpointsEnabledSetting.value);

    this._updateBreakOnExceptionsState();

    var savedBreakpoints = this._breakpointsSetting.value;
    for (var i = 0; i < savedBreakpoints.length; ++i) {
        var breakpoint = new WebInspector.Breakpoint(savedBreakpoints[i]);
        this.addBreakpoint(breakpoint, true);
    }
};

WebInspector.DebuggerManager.Event = {
    BreakpointAdded: "debugger-manager-breakpoint-added",
    BreakpointRemoved: "debugger-manager-breakpoint-removed",
    BreakpointMoved: "debugger-manager-breakpoint-moved",
    Paused: "debugger-manager-paused",
    Resumed: "debugger-manager-resumed",
    CallFramesDidChange: "debugger-manager-call-frames-did-change",
    ActiveCallFrameDidChange: "debugger-manager-active-call-frame-did-change",
    ScriptAdded: "debugger-manager-script-added",
    ScriptsCleared: "debugger-manager-scripts-cleared"
};

WebInspector.DebuggerManager.prototype = {
    constructor: WebInspector.DebuggerManager,

    // Public

    get breakpointsEnabled()
    {
        return this._breakpointsEnabledSetting.value;
    },

    set breakpointsEnabled(enabled)
    {
        if (this._breakpointsEnabled === enabled)
            return;

        this._breakpointsEnabledSetting.value = enabled;

        this._allExceptionsBreakpoint.dispatchEventToListeners(WebInspector.Breakpoint.Event.ResolvedStateDidChange);
        this._allUncaughtExceptionsBreakpoint.dispatchEventToListeners(WebInspector.Breakpoint.Event.ResolvedStateDidChange);

        for (var i = 0; i < this._breakpoints.length; ++i)
            this._breakpoints[i].dispatchEventToListeners(WebInspector.Breakpoint.Event.ResolvedStateDidChange);

        DebuggerAgent.setBreakpointsActive(enabled);

        this._updateBreakOnExceptionsState();
    },

    get paused()
    {
        return this._paused;
    },

    get callFrames()
    {
        return this._callFrames;
    },

    get activeCallFrame()
    {
        return this._activeCallFrame;
    },

    set activeCallFrame(callFrame)
    {
        if (callFrame === this._activeCallFrame)
            return;

        this._activeCallFrame = callFrame || null;

        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange);
    },

    pause: function()
    {
        DebuggerAgent.pause();
    },

    resume: function()
    {
        DebuggerAgent.resume();
    },

    stepOver: function()
    {
        DebuggerAgent.stepOver();
    },

    stepInto: function()
    {
        DebuggerAgent.stepInto();
    },

    stepOut: function()
    {
        DebuggerAgent.stepOut();
    },

    get allExceptionsBreakpoint()
    {
        return this._allExceptionsBreakpoint;
    },

    get allUncaughtExceptionsBreakpoint()
    {
        return this._allUncaughtExceptionsBreakpoint;
    },

    get breakpoints()
    {
        return this._breakpoints;
    },

    breakpointsForSourceCode: function(sourceCode)
    {
        console.assert(sourceCode instanceof WebInspector.Resource || sourceCode instanceof WebInspector.Script);

        if (sourceCode instanceof WebInspector.SourceMapResource) {
            var mappedResourceBreakpoints = [];
            var originalSourceCodeBreakpoints = this.breakpointsForSourceCode(sourceCode.sourceMap.originalSourceCode);
            return originalSourceCodeBreakpoints.filter(function(breakpoint) {
                return breakpoint.sourceCodeLocation.displaySourceCode === sourceCode;
            });
        }

        if (sourceCode.url in this._breakpointURLMap) {
            var urlBreakpoint = this._breakpointURLMap[sourceCode.url] || [];
            this._associateBreakpointsWithSourceCode(urlBreakpoint, sourceCode);
            return urlBreakpoint;
        }

        if (sourceCode instanceof WebInspector.Script && sourceCode.id in this._breakpointScriptIdentifierMap) {
            var scriptIdentifierBreakpoints = this._breakpointScriptIdentifierMap[sourceCode.id] || [];
            this._associateBreakpointsWithSourceCode(scriptIdentifierBreakpoints, sourceCode);
            return scriptIdentifierBreakpoints;
        }

        return [];
    },

    scriptForIdentifier: function(id)
    {
        return this._scriptIdMap[id] || null;
    },

    scriptsForURL: function(url)
    {
        // FIXME: This may not be safe. A Resource's URL may differ from a Script's URL.
        return this._scriptURLMap[url] || [];
    },

    addBreakpoint: function(breakpoint, skipEventDispatch)
    {
        console.assert(breakpoint);
        if (!breakpoint)
            return;

        if (breakpoint.url) {
            var urlBreakpoints = this._breakpointURLMap[breakpoint.url];
            if (!urlBreakpoints)
                urlBreakpoints = this._breakpointURLMap[breakpoint.url] = [];
            urlBreakpoints.push(breakpoint);
        }

        if (breakpoint.scriptIdentifier) {
            var scriptIdentifierBreakpoints = this._breakpointScriptIdentifierMap[breakpoint.scriptIdentifier];
            if (!scriptIdentifierBreakpoints)
                scriptIdentifierBreakpoints = this._breakpointScriptIdentifierMap[breakpoint.scriptIdentifier] = [];
            scriptIdentifierBreakpoints.push(breakpoint);
        }

        this._breakpoints.push(breakpoint);

        if (!breakpoint.disabled)
            this._setBreakpoint(breakpoint);

        if (!skipEventDispatch)
            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.BreakpointAdded, {breakpoint: breakpoint});
    },

    removeBreakpoint: function(breakpoint)
    {
        console.assert(breakpoint);
        if (!breakpoint)
            return;

        console.assert(this.isBreakpointRemovable(breakpoint));
        if (!this.isBreakpointRemovable(breakpoint))
            return;

        this._breakpoints.remove(breakpoint);

        if (breakpoint.id)
            this._removeBreakpoint(breakpoint);

        if (breakpoint.url) {
            var urlBreakpoints = this._breakpointURLMap[breakpoint.url];
            if (urlBreakpoints) {
                urlBreakpoints.remove(breakpoint);
                if (!urlBreakpoints.length)
                    delete this._breakpointURLMap[breakpoint.url];
            }
        }

        if (breakpoint.scriptIdentifier) {
            var scriptIdentifierBreakpoints = this._breakpointScriptIdentifierMap[breakpoint.scriptIdentifier];
            if (scriptIdentifierBreakpoints) {
                scriptIdentifierBreakpoints.remove(breakpoint);
                if (!scriptIdentifierBreakpoints.length)
                    delete this._breakpointScriptIdentifierMap[breakpoint.scriptIdentifier];
            }
        }

        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.BreakpointRemoved, {breakpoint: breakpoint});
    },

    breakpointResolved: function(breakpointIdentifier, location)
    {
        // Called from WebInspector.DebuggerObserver.

        var breakpoint = this._breakpointIdMap[breakpointIdentifier];
        console.assert(breakpoint);
        if (!breakpoint)
            return;

        console.assert(breakpoint.id === breakpointIdentifier);

        breakpoint.resolved = true;
    },

    reset: function()
    {
        // Called from WebInspector.DebuggerObserver.

        var wasPaused = this._paused;

        WebInspector.Script.resetUniqueDisplayNameNumbers();

        this._paused = false;
        this._scriptIdMap = {};
        this._scriptURLMap = {};

        this._ignoreBreakpointDisplayLocationDidChangeEvent = true;

        // Mark all the breakpoints as unresolved. They will be reported as resolved when
        // breakpointResolved is called as the page loads.
        for (var i = 0; i < this._breakpoints.length; ++i) {
            var breakpoint = this._breakpoints[i];
            breakpoint.resolved = false;
            if (breakpoint.sourceCodeLocation.sourceCode)
                breakpoint.sourceCodeLocation.sourceCode = null;
        }

        delete this._ignoreBreakpointDisplayLocationDidChangeEvent;

        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.ScriptsCleared);

        if (wasPaused)
            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.Resumed);
    },

    debuggerDidPause: function(callFramesPayload)
    {
        // Called from WebInspector.DebuggerObserver.

        if (this._delayedResumeTimeout) {
            clearTimeout(this._delayedResumeTimeout);
            delete this._delayedResumeTimeout;
        }

        var wasStillPaused = this._paused;

        this._paused = true;
        this._callFrames = [];

        for (var i = 0; i < callFramesPayload.length; ++i) {
            var callFramePayload = callFramesPayload[i];
            var sourceCodeLocation = this._sourceCodeLocationFromPayload(callFramePayload.location);
            if (!sourceCodeLocation)
                continue;
            var thisObject = WebInspector.RemoteObject.fromPayload(callFramePayload.this);
            var scopeChain = this._scopeChainFromPayload(callFramePayload.scopeChain);
            var callFrame = new WebInspector.CallFrame(callFramePayload.callFrameId, sourceCodeLocation, callFramePayload.functionName, thisObject, scopeChain);
            this._callFrames.push(callFrame);
        }

        this._activeCallFrame = this._callFrames[0];

        if (!wasStillPaused)
            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.Paused);
        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.CallFramesDidChange);
        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange);
    },

    debuggerDidResume: function()
    {
        // Called from WebInspector.DebuggerObserver.

        function delayedWork()
        {
            delete this._delayedResumeTimeout;

            this._paused = false;
            this._callFrames = null;
            this._activeCallFrame = null;

            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.Resumed);
            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.CallFramesDidChange);
            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange);
        }

        // We delay clearing the state and firing events so the user interface does not flash
        // between brief steps or successive breakpoints.
        this._delayedResumeTimeout = setTimeout(delayedWork.bind(this), 50);
    },

    scriptDidParse: function(scriptIdentifier, url, isContentScript, startLine, startColumn, endLine, endColumn, sourceMapURL)
    {
        // Don't add the script again if it is already known.
        if (this._scriptIdMap[scriptIdentifier]) {
            console.assert(this._scriptIdMap[scriptIdentifier].url === url);
            console.assert(this._scriptIdMap[scriptIdentifier].range.startLine === startLine);
            console.assert(this._scriptIdMap[scriptIdentifier].range.startColumn === startColumn);
            console.assert(this._scriptIdMap[scriptIdentifier].range.endLine === endLine);
            console.assert(this._scriptIdMap[scriptIdentifier].range.endColumn === endColumn);
            return;
        }

        var script = new WebInspector.Script(scriptIdentifier, new WebInspector.TextRange(startLine, startColumn, endLine, endColumn), url, isContentScript, sourceMapURL);

        this._scriptIdMap[scriptIdentifier] = script;

        if (script.url) {
            var scripts = this._scriptURLMap[script.url];
            if (!scripts)
                scripts = this._scriptURLMap[script.url] = [];
            scripts.push(script);
        }

        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.ScriptAdded, {script: script});
    },

    isBreakpointRemovable: function(breakpoint)
    {
        return breakpoint !== this._allExceptionsBreakpoint && breakpoint !== this._allUncaughtExceptionsBreakpoint;
    },

    isBreakpointEditable: function(breakpoint)
    {
        return this.isBreakpointRemovable(breakpoint);
    },

    // Private

    _sourceCodeLocationFromPayload: function(payload)
    {
        var script = this._scriptIdMap[payload.scriptId];
        console.assert(script);
        if (!script)
            return null;

        return script.createSourceCodeLocation(payload.lineNumber, payload.columnNumber);
    },

    _scopeChainFromPayload: function(payload)
    {
        var scopeChain = [];
        for (var i = 0; i < payload.length; ++i)
            scopeChain.push(this._scopeChainNodeFromPayload(payload[i]));
        return scopeChain;
    },

    _scopeChainNodeFromPayload: function(payload)
    {
        var type = null;
        switch (payload.type) {
        case "local":
            type = WebInspector.ScopeChainNode.Type.Local;
            break;
        case "global":
            type = WebInspector.ScopeChainNode.Type.Global;
            break;
        case "with":
            type = WebInspector.ScopeChainNode.Type.With;
            break;
        case "closure":
            type = WebInspector.ScopeChainNode.Type.Closure;
            break;
        case "catch":
            type = WebInspector.ScopeChainNode.Type.Catch;
            break;
        default:
            console.error("Unknown type: " + payload.type);
        }

        var object = WebInspector.RemoteObject.fromPayload(payload.object);
        return new WebInspector.ScopeChainNode(type, object);
    },

    _setBreakpoint: function(breakpoint, callback)
    {
        console.assert(!breakpoint.id);
        console.assert(!breakpoint.disabled);

        if (breakpoint.id || breakpoint.disabled)
            return;

        function didSetBreakpoint(error, breakpointIdentifier)
        {
            if (error)
                return;

            this._breakpointIdMap[breakpointIdentifier] = breakpoint;

            breakpoint.id = breakpointIdentifier;
            breakpoint.resolved = true;

            if (typeof callback === "function")
                callback();
        }

        // The breakpoint will be resolved again by calling DebuggerAgent, so mark it as unresolved.
        // If something goes wrong it will stay unresolved and show up as such in the user interface.
        breakpoint.resolved = false;

        if (breakpoint.url)
            DebuggerAgent.setBreakpointByUrl(breakpoint.sourceCodeLocation.lineNumber, breakpoint.url, undefined, breakpoint.sourceCodeLocation.columnNumber, breakpoint.condition, didSetBreakpoint.bind(this));
        else if (breakpoint.scriptIdentifier)
            DebuggerAgent.setBreakpoint({scriptId: breakpoint.scriptIdentifier, lineNumber: breakpoint.sourceCodeLocation.lineNumber, columnNumber: breakpoint.sourceCodeLocation.columnNumber}, breakpoint.condition, didSetBreakpoint.bind(this));
    },

    _removeBreakpoint: function(breakpoint, callback)
    {
        console.assert(breakpoint.id);

        if (!breakpoint.id)
            return;

        function didRemoveBreakpoint(error)
        {
            delete this._breakpointIdMap[breakpoint.id];

            breakpoint.id = null;

            // Don't reset resolved here since we want to keep disabled breakpoints looking like they
            // are resolved in the user interface. They will get marked as unresolved in reset.

            if (typeof callback === "function")
                callback();
        }

        DebuggerAgent.removeBreakpoint(breakpoint.id, didRemoveBreakpoint.bind(this));
    },

    _breakpointDisplayLocationDidChange: function(event)
    {
        if (this._ignoreBreakpointDisplayLocationDidChangeEvent)
            return;

        var breakpoint = event.target;
        if (!breakpoint.id || breakpoint.disabled)
            return;

        // Remove the breakpoint with its old id.
        this._removeBreakpoint(breakpoint, breakpointRemoved.bind(this));

        function breakpointRemoved()
        {
            // Add the breakpoint at its new lineNumber and get a new id.
            this._setBreakpoint(breakpoint);

            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.BreakpointMoved, {breakpoint: breakpoint});
        }
    },

    _breakpointDisabledStateDidChange: function(event)
    {
        var breakpoint = event.target;

        if (breakpoint === this._allExceptionsBreakpoint) {
            this._allExceptionsBreakpointEnabledSetting.value = !breakpoint.disabled;
            this._updateBreakOnExceptionsState();
            return;
        }

        if (breakpoint === this._allUncaughtExceptionsBreakpoint) {
            this._allUncaughtExceptionsBreakpointEnabledSetting.value = !breakpoint.disabled;
            this._updateBreakOnExceptionsState();
            return;
        }

        if (breakpoint.disabled)
            this._removeBreakpoint(breakpoint);
        else
            this._setBreakpoint(breakpoint);
    },

    _breakpointConditionDidChange: function(event)
    {
        var breakpoint = event.target;
        if (breakpoint.disabled)
            return;

        console.assert(this.isBreakpointEditable(breakpoint));
        if (!this.isBreakpointEditable(breakpoint))
            return;

        // Remove the breakpoint with its old id.
        this._removeBreakpoint(breakpoint, breakpointRemoved.bind(this));

        function breakpointRemoved()
        {
            // Add the breakpoint with its new condition and get a new id.
            this._setBreakpoint(breakpoint);
        }
    },

    _updateBreakOnExceptionsState: function()
    {
        var state = "none";

        if (this._breakpointsEnabledSetting.value) {
            if (!this._allExceptionsBreakpoint.disabled)
                state = "all";
            else if (!this._allUncaughtExceptionsBreakpoint.disabled)
                state = "uncaught";
        }

        switch (state) {
        case "all":
            // Mark the uncaught breakpoint as unresolved since "all" includes "uncaught".
            // That way it is clear in the user interface that the breakpoint is ignored.
            this._allUncaughtExceptionsBreakpoint.resolved = false;
            break;
        case "uncaught":
        case "none":
            // Mark the uncaught breakpoint as resolved again.
            this._allUncaughtExceptionsBreakpoint.resolved = true;
            break;
        }

        DebuggerAgent.setPauseOnExceptions(state);
    },

    _inspectorClosing: function(event)
    {
        this._saveBreakpoints();
    },

    _saveBreakpoints: function()
    {
        var savedBreakpoints = [];

        for (var i = 0; i < this._breakpoints.length; ++i) {
            var breakpoint = this._breakpoints[i];

            // Only breakpoints with URLs can be saved. Breakpoints for transient scripts can't.
            if (!breakpoint.url)
                continue;

            savedBreakpoints.push(breakpoint.info);
        }

        this._breakpointsSetting.value = savedBreakpoints;
    },

    _associateBreakpointsWithSourceCode: function(breakpoints, sourceCode)
    {
        this._ignoreBreakpointDisplayLocationDidChangeEvent = true;

        for (var i = 0; i < breakpoints.length; ++i) {
            var breakpoint = breakpoints[i];
            if (breakpoint.sourceCodeLocation.sourceCode === null)
                breakpoint.sourceCodeLocation.sourceCode = sourceCode;
            // SourceCodes can be unequal if the SourceCodeLocation is associated with a Script and we are looking at the Resource.
            console.assert(breakpoint.sourceCodeLocation.sourceCode === sourceCode || breakpoint.sourceCodeLocation.sourceCode.url === sourceCode.url);
        }

        delete this._ignoreBreakpointDisplayLocationDidChangeEvent;
    }
};

WebInspector.DebuggerManager.prototype.__proto__ = WebInspector.Object.prototype;
