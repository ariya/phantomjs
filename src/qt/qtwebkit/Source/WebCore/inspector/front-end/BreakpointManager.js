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
 * @extends {WebInspector.Object}
 * @param {WebInspector.Setting} breakpointStorage
 * @param {WebInspector.DebuggerModel} debuggerModel
 * @param {WebInspector.Workspace} workspace
 */
WebInspector.BreakpointManager = function(breakpointStorage, debuggerModel, workspace)
{
    this._storage = new WebInspector.BreakpointManager.Storage(this, breakpointStorage);
    this._debuggerModel = debuggerModel;
    this._workspace = workspace;

    this._breakpoints = new Map();
    this._breakpointForDebuggerId = {};
    this._breakpointsForUISourceCode = new Map();
    this._sourceFilesWithRestoredBreakpoints = {};

    this._debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.BreakpointResolved, this._breakpointResolved, this);
    this._workspace.addEventListener(WebInspector.Workspace.Events.ProjectWillReset, this._projectWillReset, this);
    this._workspace.addEventListener(WebInspector.UISourceCodeProvider.Events.UISourceCodeAdded, this._uiSourceCodeAdded, this);
}

WebInspector.BreakpointManager.Events = {
    BreakpointAdded: "breakpoint-added",
    BreakpointRemoved: "breakpoint-removed"
}

WebInspector.BreakpointManager.sourceFileId = function(uiSourceCode)
{
    if (!uiSourceCode.url)
        return "";
    var deobfuscatedPrefix = uiSourceCode.formatted() ? "deobfuscated:" : "";
    return deobfuscatedPrefix + uiSourceCode.uri();
}

WebInspector.BreakpointManager.prototype = {
    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    _restoreBreakpoints: function(uiSourceCode)
    {
        var sourceFileId = WebInspector.BreakpointManager.sourceFileId(uiSourceCode);
        if (!sourceFileId || this._sourceFilesWithRestoredBreakpoints[sourceFileId])
            return;
        this._sourceFilesWithRestoredBreakpoints[sourceFileId] = true;

        // Erase provisional breakpoints prior to restoring them.
        for (var debuggerId in this._breakpointForDebuggerId) {
            var breakpoint = this._breakpointForDebuggerId[debuggerId];
            if (breakpoint._sourceFileId !== sourceFileId)
                continue;
            breakpoint.remove(true);
        }
        this._storage._restoreBreakpoints(uiSourceCode);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _uiSourceCodeAdded: function(event)
    {
        var uiSourceCode = /** @type {WebInspector.UISourceCode} */ (event.data);
        this._restoreBreakpoints(uiSourceCode);
        if (uiSourceCode.contentType() === WebInspector.resourceTypes.Script || uiSourceCode.contentType() === WebInspector.resourceTypes.Document) {
            uiSourceCode.addEventListener(WebInspector.UISourceCode.Events.SourceMappingChanged, this._uiSourceCodeMappingChanged, this);
            uiSourceCode.addEventListener(WebInspector.UISourceCode.Events.FormattedChanged, this._uiSourceCodeFormatted, this);
        }
    },

    /**
     * @param {WebInspector.Event} event
     */
    _uiSourceCodeFormatted: function(event)
    {
        var uiSourceCode = /** @type {WebInspector.UISourceCode} */ (event.target);
        this._restoreBreakpoints(uiSourceCode);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    _resetBreakpoints: function(uiSourceCode)
    {
        var sourceFileId = WebInspector.BreakpointManager.sourceFileId(uiSourceCode);
        var breakpoints = this._breakpoints.keys();
        for (var i = 0; i < breakpoints.length; ++i) {
            var breakpoint = breakpoints[i];
            if (breakpoint._sourceFileId !== sourceFileId)
                return;
            if (breakpoint.enabled()) {
                breakpoint._removeFromDebugger();
                breakpoint._setInDebugger();
            }
        }
    },

    /**
     * @param {WebInspector.Event} event
     */
    _uiSourceCodeMappingChanged: function(event)
    {
        var identityHasChanged = /** @type {boolean} */ (event.data.identityHasChanged);
        if (!identityHasChanged)
            return;
        var uiSourceCode = /** @type {WebInspector.UISourceCode} */ (event.target);
        this._resetBreakpoints(uiSourceCode);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     * @param {string} condition
     * @param {boolean} enabled
     * @return {WebInspector.BreakpointManager.Breakpoint}
     */
    setBreakpoint: function(uiSourceCode, lineNumber, condition, enabled)
    {
        this._debuggerModel.setBreakpointsActive(true);
        return this._innerSetBreakpoint(uiSourceCode, lineNumber, condition, enabled);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     * @param {string} condition
     * @param {boolean} enabled
     * @return {WebInspector.BreakpointManager.Breakpoint}
     */
    _innerSetBreakpoint: function(uiSourceCode, lineNumber, condition, enabled)
    {
        var breakpoint = this.findBreakpoint(uiSourceCode, lineNumber);
        if (breakpoint) {
            breakpoint._updateBreakpoint(condition, enabled);
            return breakpoint;
        }
        breakpoint = new WebInspector.BreakpointManager.Breakpoint(this, uiSourceCode, lineNumber, condition, enabled);
        this._breakpoints.put(breakpoint);
        return breakpoint;
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     * @return {?WebInspector.BreakpointManager.Breakpoint}
     */
    findBreakpoint: function(uiSourceCode, lineNumber)
    {
        var breakpoints = this._breakpointsForUISourceCode.get(uiSourceCode);
        var lineBreakpoints = breakpoints ? breakpoints[lineNumber] : null;
        return lineBreakpoints ? lineBreakpoints[0] : null;
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @return {Array.<WebInspector.BreakpointManager.Breakpoint>}
     */
    breakpointsForUISourceCode: function(uiSourceCode)
    {
        var result = [];
        var breakpoints = /** @type {Array.<WebInspector.BreakpointManager.Breakpoint>} */(this._breakpoints.keys());
        for (var i = 0; i < breakpoints.length; ++i) {
            var breakpoint = breakpoints[i];
            var uiLocation = breakpoint._primaryUILocation;
            if (uiLocation.uiSourceCode === uiSourceCode)
                result.push(breakpoint);
        }
        return result;
    },

    /**
     * @return {Array.<WebInspector.BreakpointManager.Breakpoint>}
     */
    allBreakpoints: function()
    {
        var result = [];
        var breakpoints = /** @type {Array.<WebInspector.BreakpointManager.Breakpoint>} */(this._breakpoints.keys());
        return breakpoints;
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @return {Array.<{breakpoint: WebInspector.BreakpointManager.Breakpoint, uiLocation: WebInspector.UILocation}>}
     */
    breakpointLocationsForUISourceCode: function(uiSourceCode)
    {
        var result = [];
        var breakpoints = /** @type {Array.<WebInspector.BreakpointManager.Breakpoint>} */(this._breakpoints.keys());
        for (var i = 0; i < breakpoints.length; ++i) {
            var breakpoint = breakpoints[i];
            var uiLocations = Object.values(breakpoint._uiLocations);
            for (var j = 0; j < uiLocations.length; ++j) {
                var uiLocation = uiLocations[j];
                if (uiLocation.uiSourceCode === uiSourceCode)
                    result.push({breakpoint: breakpoint, uiLocation: uiLocations[j]});
            }
        }
        return result;
    },

    /**
     * @return {Array.<{breakpoint: WebInspector.BreakpointManager.Breakpoint, uiLocation: WebInspector.UILocation}>}
     */
    allBreakpointLocations: function()
    {
        var result = [];
        var breakpoints = /** @type {Array.<WebInspector.BreakpointManager.Breakpoint>} */(this._breakpoints.keys());
        for (var i = 0; i < breakpoints.length; ++i) {
            var breakpoint = breakpoints[i];
            var uiLocations = Object.values(breakpoint._uiLocations);
            for (var j = 0; j < uiLocations.length; ++j)
                result.push({breakpoint: breakpoint, uiLocation: uiLocations[j]});
        }
        return result;
    },

    /**
     * @param {boolean} toggleState
     */
    toggleAllBreakpoints: function(toggleState)
    {
        var breakpoints = /** @type {Array.<WebInspector.BreakpointManager.Breakpoint>} */(this._breakpoints.keys());
        for (var i = 0; i < breakpoints.length; ++i) {
            var breakpoint = breakpoints[i];
            if (breakpoint.enabled() != toggleState)
                breakpoint.setEnabled(toggleState);
        }
    },

    removeAllBreakpoints: function()
    {
        var breakpoints = /** @type {Array.<WebInspector.BreakpointManager.Breakpoint>} */(this._breakpoints.keys());
        for (var i = 0; i < breakpoints.length; ++i)
            breakpoints[i].remove();
    },

    reset: function()
    {
        // Remove all breakpoints from UI and debugger, do not update storage.
        this._storage._muted = true;
        this.removeAllBreakpoints();
        delete this._storage._muted;

        // Remove all provisional breakpoints from the debugger.
        for (var debuggerId in this._breakpointForDebuggerId)
            this._debuggerModel.removeBreakpoint(debuggerId);
        this._breakpointForDebuggerId = {};
        this._sourceFilesWithRestoredBreakpoints = {};
    },

    _projectWillReset: function(event)
    {
        var project = /** @type {WebInspector.Project} */ (event.data);
        var uiSourceCodes = project.uiSourceCodes();
        for (var i = 0; i < uiSourceCodes.length; ++i) {
            var uiSourceCode = uiSourceCodes[i];
            var breakpoints = this._breakpointsForUISourceCode.get(uiSourceCode) || [];
            for (var lineNumber in breakpoints) {
                var lineBreakpoints = breakpoints[lineNumber];
                for (var j = 0; j < lineBreakpoints.length; ++j) {
                    var breakpoint = lineBreakpoints[j];
                    breakpoint._resetLocations();
                }
            }
            this._breakpointsForUISourceCode.remove(uiSourceCode);

            breakpoints = this.breakpointsForUISourceCode(uiSourceCode);
            for (var j = 0; j < breakpoints.length; ++j) { 
                var breakpoint = breakpoints[j];
                this._breakpoints.remove(breakpoint);
                delete breakpoint._primaryUILocation;
            }

            var sourceFileId = WebInspector.BreakpointManager.sourceFileId(uiSourceCode);
            delete this._sourceFilesWithRestoredBreakpoints[sourceFileId];
        }
    },

    _breakpointResolved: function(event)
    {
        var breakpointId = /** @type {DebuggerAgent.BreakpointId} */ (event.data.breakpointId);
        var location = /** @type {WebInspector.DebuggerModel.Location} */ (event.data.location);
        var breakpoint = this._breakpointForDebuggerId[breakpointId];
        if (!breakpoint)
            return;
        if (!this._breakpoints.contains(breakpoint))
            this._breakpoints.put(breakpoint);
        breakpoint._addResolvedLocation(location);
    },

    /**
     * @param {WebInspector.BreakpointManager.Breakpoint} breakpoint
     * @param {boolean} removeFromStorage
     */
    _removeBreakpoint: function(breakpoint, removeFromStorage)
    {
        console.assert(!breakpoint._debuggerId)
        this._breakpoints.remove(breakpoint);
        if (removeFromStorage)
            this._storage._removeBreakpoint(breakpoint);
    },

    /**
     * @param {WebInspector.BreakpointManager.Breakpoint} breakpoint
     * @param {WebInspector.UILocation} uiLocation
     */
    _uiLocationAdded: function(breakpoint, uiLocation)
    {
        var breakpoints = this._breakpointsForUISourceCode.get(uiLocation.uiSourceCode);
        if (!breakpoints) {
            breakpoints = {};
            this._breakpointsForUISourceCode.put(uiLocation.uiSourceCode, breakpoints);
        }

        var lineBreakpoints = breakpoints[uiLocation.lineNumber];
        if (!lineBreakpoints) {
            lineBreakpoints = [];
            breakpoints[uiLocation.lineNumber] = lineBreakpoints;
        }

        lineBreakpoints.push(breakpoint);
        this.dispatchEventToListeners(WebInspector.BreakpointManager.Events.BreakpointAdded, {breakpoint: breakpoint, uiLocation: uiLocation});
    },

    /**
     * @param {WebInspector.BreakpointManager.Breakpoint} breakpoint
     * @param {WebInspector.UILocation} uiLocation
     */
    _uiLocationRemoved: function(breakpoint, uiLocation)
    {
      var breakpoints = this._breakpointsForUISourceCode.get(uiLocation.uiSourceCode);
        if (!breakpoints)
            return;

        var lineBreakpoints = breakpoints[uiLocation.lineNumber];
        if (!lineBreakpoints)
            return;

        lineBreakpoints.remove(breakpoint);
        if (!lineBreakpoints.length)
            delete breakpoints[uiLocation.lineNumber];
        this.dispatchEventToListeners(WebInspector.BreakpointManager.Events.BreakpointRemoved, {breakpoint: breakpoint, uiLocation: uiLocation});
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 * @param {WebInspector.BreakpointManager} breakpointManager
 * @param {WebInspector.UISourceCode} uiSourceCode
 * @param {number} lineNumber
 * @param {string} condition
 * @param {boolean} enabled
 */
WebInspector.BreakpointManager.Breakpoint = function(breakpointManager, uiSourceCode, lineNumber, condition, enabled)
{
    this._breakpointManager = breakpointManager;
    this._primaryUILocation = new WebInspector.UILocation(uiSourceCode, lineNumber, 0);
    this._sourceFileId = WebInspector.BreakpointManager.sourceFileId(uiSourceCode);
    /** @type {Array.<WebInspector.Script.Location>} */
    this._liveLocations = [];
    /** @type {Object.<string, WebInspector.UILocation>} */
    this._uiLocations = {};

    // Force breakpoint update.
    /** @type {string} */ this._condition;
    /** @type {boolean} */ this._enabled;
    this._updateBreakpoint(condition, enabled);
}

WebInspector.BreakpointManager.Breakpoint.prototype = {
    /**
     * @return {WebInspector.UILocation}
     */
    primaryUILocation: function()
    {
        return this._primaryUILocation;
    },

    /**
     * @param {WebInspector.DebuggerModel.Location} location
     */
    _addResolvedLocation: function(location)
    {
        this._liveLocations.push(this._breakpointManager._debuggerModel.createLiveLocation(location, this._locationUpdated.bind(this, location)));
    },

    /**
     * @param {WebInspector.DebuggerModel.Location} location
     * @param {WebInspector.UILocation} uiLocation
     */
    _locationUpdated: function(location, uiLocation)
    {
        var stringifiedLocation = location.scriptId + ":" + location.lineNumber + ":" + location.columnNumber;
        var oldUILocation = /** @type {WebInspector.UILocation} */ (this._uiLocations[stringifiedLocation]);
        if (oldUILocation)
            this._breakpointManager._uiLocationRemoved(this, oldUILocation);
        if (this._uiLocations[""]) {
            delete this._uiLocations[""];
            this._breakpointManager._uiLocationRemoved(this, this._primaryUILocation);
        }
        this._uiLocations[stringifiedLocation] = uiLocation;
        this._breakpointManager._uiLocationAdded(this, uiLocation);
    },

    /**
     * @return {boolean}
     */
    enabled: function()
    {
        return this._enabled;
    },

    /**
     * @param {boolean} enabled
     */
    setEnabled: function(enabled)
    {
        this._updateBreakpoint(this._condition, enabled);
    },

    /**
     * @return {string}
     */
    condition: function()
    {
        return this._condition;
    },

    /**
     * @param {string} condition
     */
    setCondition: function(condition)
    {
        this._updateBreakpoint(condition, this._enabled);
    },

    /**
     * @param {string} condition
     * @param {boolean} enabled
     */
    _updateBreakpoint: function(condition, enabled)
    {
        if (this._enabled === enabled && this._condition === condition)
            return;

        if (this._enabled)
            this._removeFromDebugger();

        this._enabled = enabled;
        this._condition = condition;
        this._breakpointManager._storage._updateBreakpoint(this);

        var scriptFile = this._primaryUILocation.uiSourceCode.scriptFile();
        if (this._enabled && !(scriptFile && scriptFile.hasDivergedFromVM())) {
            this._setInDebugger();
            return;
        }

        this._fakeBreakpointAtPrimaryLocation();
    },

    /**
     * @param {boolean=} keepInStorage
     */
    remove: function(keepInStorage)
    {
        var removeFromStorage = !keepInStorage;
        this._resetLocations();
        this._removeFromDebugger();
        this._breakpointManager._removeBreakpoint(this, removeFromStorage);
    },

    _setInDebugger: function()
    {
        console.assert(!this._debuggerId);
        var rawLocation = this._primaryUILocation.uiLocationToRawLocation();
        var debuggerModelLocation = /** @type {WebInspector.DebuggerModel.Location} */ (rawLocation);
        if (debuggerModelLocation)
            this._breakpointManager._debuggerModel.setBreakpointByScriptLocation(debuggerModelLocation, this._condition, didSetBreakpoint.bind(this));
        else
            this._breakpointManager._debuggerModel.setBreakpointByURL(this._primaryUILocation.uiSourceCode.url, this._primaryUILocation.lineNumber, 0, this._condition, didSetBreakpoint.bind(this));

        /**
         * @this {WebInspector.BreakpointManager.Breakpoint}
         * @param {?DebuggerAgent.BreakpointId} breakpointId
         * @param {Array.<WebInspector.DebuggerModel.Location>} locations
         */
        function didSetBreakpoint(breakpointId, locations)
        {
            if (!breakpointId) {
                this._resetLocations();
                this._breakpointManager._removeBreakpoint(this, false);
                return;
            }

            this._debuggerId = breakpointId;
            this._breakpointManager._breakpointForDebuggerId[breakpointId] = this;

            if (!locations.length) {
                this._fakeBreakpointAtPrimaryLocation();
                return;
            }

            this._resetLocations();
            for (var i = 0; i < locations.length; ++i) {
                var script = this._breakpointManager._debuggerModel.scriptForId(locations[i].scriptId);
                var uiLocation = script.rawLocationToUILocation(locations[i].lineNumber, locations[i].columnNumber);
                if (this._breakpointManager.findBreakpoint(uiLocation.uiSourceCode, uiLocation.lineNumber)) {
                    // location clash
                    this.remove();
                    return;
                }
            }

            for (var i = 0; i < locations.length; ++i)
                this._addResolvedLocation(locations[i]);
        }
    },

    _removeFromDebugger: function()
    {
        if (this._debuggerId) {
            this._breakpointManager._debuggerModel.removeBreakpoint(this._debuggerId);
            delete this._breakpointManager._breakpointForDebuggerId[this._debuggerId];
            delete this._debuggerId;
        }
    },

    _resetLocations: function()
    {
        for (var stringifiedLocation in this._uiLocations)
            this._breakpointManager._uiLocationRemoved(this, this._uiLocations[stringifiedLocation]);

        for (var i = 0; i < this._liveLocations.length; ++i)
            this._liveLocations[i].dispose();
        this._liveLocations = [];

        this._uiLocations = {};
    },

    /**
     * @return {string}
     */
    _breakpointStorageId: function()
    {
        if (!this._sourceFileId)
            return "";
        return this._sourceFileId + ":" + this._primaryUILocation.lineNumber;
    },

    _fakeBreakpointAtPrimaryLocation: function()
    {
        this._resetLocations();
        this._uiLocations[""] = this._primaryUILocation;
        this._breakpointManager._uiLocationAdded(this, this._primaryUILocation);
    }
}

/**
 * @constructor
 * @param {WebInspector.BreakpointManager} breakpointManager
 * @param {WebInspector.Setting} setting
 */
WebInspector.BreakpointManager.Storage = function(breakpointManager, setting)
{
    this._breakpointManager = breakpointManager;
    this._setting = setting;
    var breakpoints = this._setting.get();
    /** @type {Object.<string,WebInspector.BreakpointManager.Storage.Item>} */
    this._breakpoints = {};
    for (var i = 0; i < breakpoints.length; ++i) {
        var breakpoint = /** @type {WebInspector.BreakpointManager.Storage.Item} */ (breakpoints[i]);
        this._breakpoints[breakpoint.sourceFileId + ":" + breakpoint.lineNumber] = breakpoint;
    }
}

WebInspector.BreakpointManager.Storage.prototype = {
    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    _restoreBreakpoints: function(uiSourceCode)
    {
        this._muted = true;
        var sourceFileId = WebInspector.BreakpointManager.sourceFileId(uiSourceCode);
        for (var id in this._breakpoints) {
            var breakpoint = this._breakpoints[id];
            if (breakpoint.sourceFileId === sourceFileId)
                this._breakpointManager._innerSetBreakpoint(uiSourceCode, breakpoint.lineNumber, breakpoint.condition, breakpoint.enabled);
        }
        delete this._muted;
    },

    /**
     * @param {WebInspector.BreakpointManager.Breakpoint} breakpoint
     */
    _updateBreakpoint: function(breakpoint)
    {
        if (this._muted || !breakpoint._breakpointStorageId())
            return;
        this._breakpoints[breakpoint._breakpointStorageId()] = new WebInspector.BreakpointManager.Storage.Item(breakpoint);
        this._save();
    },

    /**
     * @param {WebInspector.BreakpointManager.Breakpoint} breakpoint
     */
    _removeBreakpoint: function(breakpoint)
    {
        if (this._muted)
            return;
        delete this._breakpoints[breakpoint._breakpointStorageId()];
        this._save();
    },

    _save: function()
    {
        var breakpointsArray = [];
        for (var id in this._breakpoints)
            breakpointsArray.push(this._breakpoints[id]);
        this._setting.set(breakpointsArray);
    }
}

/**
 * @constructor
 * @param {WebInspector.BreakpointManager.Breakpoint} breakpoint
 */
WebInspector.BreakpointManager.Storage.Item = function(breakpoint)
{
    var primaryUILocation = breakpoint.primaryUILocation();
    this.sourceFileId = breakpoint._sourceFileId;
    this.lineNumber = primaryUILocation.lineNumber;
    this.condition = breakpoint.condition();
    this.enabled = breakpoint.enabled();
}

/** @type {WebInspector.BreakpointManager} */
WebInspector.breakpointManager = null;
