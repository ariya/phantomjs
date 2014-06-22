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
 * @implements {WebInspector.ScriptSourceMapping}
 * @param {WebInspector.Workspace} workspace
 */
WebInspector.DefaultScriptMapping = function(workspace)
{
    this._projectDelegate = new WebInspector.DebuggerProjectDelegate();
    this._workspace = workspace;
    this._workspace.addProject(this._projectDelegate);
    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.GlobalObjectCleared, this._debuggerReset, this);
    this._debuggerReset();
}

WebInspector.DefaultScriptMapping.prototype = {
    /**
     * @param {WebInspector.RawLocation} rawLocation
     * @return {WebInspector.UILocation}
     */
    rawLocationToUILocation: function(rawLocation)
    {
        var debuggerModelLocation = /** @type {WebInspector.DebuggerModel.Location} */ (rawLocation);
        var script = WebInspector.debuggerModel.scriptForId(debuggerModelLocation.scriptId);
        var uiSourceCode = this._uiSourceCodeForScriptId[script.scriptId];
        var lineNumber = debuggerModelLocation.lineNumber;
        var columnNumber = debuggerModelLocation.columnNumber || 0;
        return new WebInspector.UILocation(uiSourceCode, lineNumber, columnNumber);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     * @param {number} columnNumber
     * @return {WebInspector.DebuggerModel.Location}
     */
    uiLocationToRawLocation: function(uiSourceCode, lineNumber, columnNumber)
    {
        var scriptId = this._scriptIdForUISourceCode.get(uiSourceCode);
        var script = WebInspector.debuggerModel.scriptForId(scriptId);
        return WebInspector.debuggerModel.createRawLocation(script, lineNumber, columnNumber);
    },

    /**
     * @return {boolean}
     */
    isIdentity: function()
    {
        return true;
    },

    /**
     * @param {WebInspector.Script} script
     */
    addScript: function(script)
    {
        var path = this._projectDelegate.addScript(script);
        var uiSourceCode = this._workspace.uiSourceCode(this._projectDelegate.id(), path);
        this._uiSourceCodeForScriptId[script.scriptId] = uiSourceCode;
        this._scriptIdForUISourceCode.put(uiSourceCode, script.scriptId);
        uiSourceCode.setSourceMapping(this);
        script.pushSourceMapping(this);
        script.addEventListener(WebInspector.Script.Events.ScriptEdited, this._scriptEdited.bind(this, script.scriptId));
        return uiSourceCode;
    },

    /**
     * @param {string} scriptId
     * @param {WebInspector.Event} event
     */
    _scriptEdited: function(scriptId, event)
    {
        var content = /** @type {string} */(event.data);
        this._uiSourceCodeForScriptId[scriptId].addRevision(content);
    },

    _debuggerReset: function()
    {
        /** @type {Object.<string, WebInspector.UISourceCode>} */
        this._uiSourceCodeForScriptId = {};
        this._scriptIdForUISourceCode = new Map();
        this._projectDelegate.reset();
    }
}

/**
 * @constructor
 * @extends {WebInspector.ContentProviderBasedProjectDelegate}
 */
WebInspector.DebuggerProjectDelegate = function()
{
    WebInspector.ContentProviderBasedProjectDelegate.call(this, WebInspector.projectTypes.Debugger);
}

WebInspector.DebuggerProjectDelegate.prototype = {
    /**
     * @return {string}
     */
    id: function()
    {
        return "debugger:";
    },

    /**
     * @return {string}
     */
    displayName: function()
    {
        return "debugger";
    },

    /**
     * @param {WebInspector.Script} script
     * @return {Array.<string>}
     */
    addScript: function(script)
    {
        var contentProvider = script.isInlineScript() ? new WebInspector.ConcatenatedScriptsContentProvider([script]) : script;
        var splittedURL = WebInspector.ParsedURL.splitURL(script.sourceURL);
        var name = splittedURL[splittedURL.length - 1];
        name = "[VM] " + name + " (" + script.scriptId + ")";
        return this.addContentProvider([name], script.sourceURL, contentProvider, false, script.isContentScript);
    },
    
    __proto__: WebInspector.ContentProviderBasedProjectDelegate.prototype
}
