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
WebInspector.ResourceScriptMapping = function(workspace)
{
    this._workspace = workspace;
    this._workspace.addEventListener(WebInspector.UISourceCodeProvider.Events.UISourceCodeAdded, this._uiSourceCodeAddedToWorkspace, this);

    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.GlobalObjectCleared, this._debuggerReset, this);
    this._initialize();
}

WebInspector.ResourceScriptMapping.prototype = {
    /**
     * @param {WebInspector.RawLocation} rawLocation
     * @return {WebInspector.UILocation}
     */
    rawLocationToUILocation: function(rawLocation)
    {
        var debuggerModelLocation = /** @type {WebInspector.DebuggerModel.Location} */ (rawLocation);
        var script = WebInspector.debuggerModel.scriptForId(debuggerModelLocation.scriptId);
        var uiSourceCode = this._workspaceUISourceCodeForScript(script);
        if (!uiSourceCode)
            return null;
        var scriptFile = uiSourceCode.scriptFile();
        if (scriptFile && ((scriptFile.hasDivergedFromVM() && !scriptFile.isMergingToVM()) || scriptFile.isDivergingFromVM()))
            return null;
        return new WebInspector.UILocation(uiSourceCode, debuggerModelLocation.lineNumber, debuggerModelLocation.columnNumber || 0);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     * @param {number} columnNumber
     * @return {WebInspector.DebuggerModel.Location}
     */
    uiLocationToRawLocation: function(uiSourceCode, lineNumber, columnNumber)
    {
        var scripts = this._scriptsForUISourceCode(uiSourceCode);
        console.assert(scripts.length);
        return WebInspector.debuggerModel.createRawLocation(scripts[0], lineNumber, columnNumber);
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
        if (script.isAnonymousScript() || script.isDynamicScript())
            return;
        script.pushSourceMapping(this);
        
        var scriptsForSourceURL = script.isInlineScript() ? this._inlineScriptsForSourceURL : this._nonInlineScriptsForSourceURL;
        scriptsForSourceURL[script.sourceURL] = scriptsForSourceURL[script.sourceURL] || [];
        scriptsForSourceURL[script.sourceURL].push(script);

        var uiSourceCode = this._workspaceUISourceCodeForScript(script);
        if (!uiSourceCode)
            return;

        this._bindUISourceCodeToScripts(uiSourceCode, [script]);
    },

    _uiSourceCodeAddedToWorkspace: function(event)
    {
        var uiSourceCode = /** @type {WebInspector.UISourceCode} */ (event.data);
        if (!uiSourceCode.url)
            return;

        var scripts = this._scriptsForUISourceCode(uiSourceCode);
        if (!scripts.length)
            return;

        this._bindUISourceCodeToScripts(uiSourceCode, scripts);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    _hasMergedToVM: function(uiSourceCode)
    {
        var scripts = this._scriptsForUISourceCode(uiSourceCode);
        if (!scripts.length)
            return;
        for (var i = 0; i < scripts.length; ++i)
            scripts[i].updateLocations();
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    _hasDivergedFromVM: function(uiSourceCode)
    {
        var scripts = this._scriptsForUISourceCode(uiSourceCode);
        if (!scripts.length)
            return;
        for (var i = 0; i < scripts.length; ++i)
            scripts[i].updateLocations();
    },

    /**
     * @param {WebInspector.Script} script
     * @return {WebInspector.UISourceCode}
     */
    _workspaceUISourceCodeForScript: function(script)
    {
        if (script.isAnonymousScript() || script.isDynamicScript())
            return null;
        // FIXME: workaround for script.isDynamicScript() being unreliable.
        if (!script.isInlineScript() && this._inlineScriptsForSourceURL[script.sourceURL])
            return null;
        return this._workspace.uiSourceCodeForURL(script.sourceURL);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @return {Array.<WebInspector.Script>}
     */
    _scriptsForUISourceCode: function(uiSourceCode)
    {
        var isInlineScript;
        switch (uiSourceCode.contentType()) {
        case WebInspector.resourceTypes.Document:
            isInlineScript = true;
            break;
        case WebInspector.resourceTypes.Script:
            isInlineScript = false;
            break;
        default:
            return [];
        }
        if (!uiSourceCode.url)
            return [];
        var scriptsForSourceURL = isInlineScript ? this._inlineScriptsForSourceURL : this._nonInlineScriptsForSourceURL;
        return scriptsForSourceURL[uiSourceCode.url] || [];
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {Array.<WebInspector.Script>} scripts
     */
    _bindUISourceCodeToScripts: function(uiSourceCode, scripts)
    {
        console.assert(scripts.length);
        var scriptFile = new WebInspector.ResourceScriptFile(this, uiSourceCode, scripts);
        uiSourceCode.setScriptFile(scriptFile);
        for (var i = 0; i < scripts.length; ++i)
            scripts[i].updateLocations();
        uiSourceCode.setSourceMapping(this);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {Array.<WebInspector.Script>} scripts
     */
    _unbindUISourceCodeFromScripts: function(uiSourceCode, scripts)
    {
        console.assert(scripts.length);
        var scriptFile = /** @type {WebInspector.ResourceScriptFile} */ (uiSourceCode.scriptFile());
        scriptFile.dispose();
        uiSourceCode.setScriptFile(null);
        uiSourceCode.setSourceMapping(null);
    },

    _initialize: function()
    {
        /** @type {!Object.<string, !Array.<!WebInspector.UISourceCode>>} */
        this._inlineScriptsForSourceURL = {};
        /** @type {!Object.<string, !Array.<!WebInspector.UISourceCode>>} */
        this._nonInlineScriptsForSourceURL = {};
    },

    _debuggerReset: function()
    {
        /**
         * @param {!Object.<string, !Array.<!WebInspector.UISourceCode>>} scriptsForSourceURL
         */
        function unbindUISourceCodes(scriptsForSourceURL)
        {
            for (var sourceURL in scriptsForSourceURL) {
                var scripts = scriptsForSourceURL[sourceURL];
                if (!scripts.length)
                    continue;
                var uiSourceCode = this._workspaceUISourceCodeForScript(scripts[0]);
                if (!uiSourceCode)
                    continue;
                this._unbindUISourceCodeFromScripts(uiSourceCode, scripts);
            }
        }

        unbindUISourceCodes.call(this, this._inlineScriptsForSourceURL);
        unbindUISourceCodes.call(this, this._nonInlineScriptsForSourceURL);
        this._initialize();
    },
}

/**
 * @interface
 * @extends {WebInspector.EventTarget}
 */
WebInspector.ScriptFile = function()
{
}

WebInspector.ScriptFile.Events = {
    DidMergeToVM: "DidMergeToVM",
    DidDivergeFromVM: "DidDivergeFromVM",
}

WebInspector.ScriptFile.prototype = {
    /**
     * @return {boolean}
     */
    hasDivergedFromVM: function() { return false; },

    /**
     * @return {boolean}
     */
    isDivergingFromVM: function() { return false; },

    /**
     * @return {boolean}
     */
    isMergingToVM: function() { return false; },

    checkMapping: function() { },
}

/**
 * @constructor
 * @implements {WebInspector.ScriptFile}
 * @extends {WebInspector.Object}
 * @param {WebInspector.ResourceScriptMapping} resourceScriptMapping
 * @param {WebInspector.UISourceCode} uiSourceCode
 */
WebInspector.ResourceScriptFile = function(resourceScriptMapping, uiSourceCode, scripts)
{
    console.assert(scripts.length);

    WebInspector.ScriptFile.call(this);
    this._resourceScriptMapping = resourceScriptMapping;
    this._uiSourceCode = uiSourceCode;

    if (this._uiSourceCode.contentType() === WebInspector.resourceTypes.Script)
        this._script = scripts[0];

    this._uiSourceCode.addEventListener(WebInspector.UISourceCode.Events.WorkingCopyCommitted, this._workingCopyCommitted, this);
    this._uiSourceCode.addEventListener(WebInspector.UISourceCode.Events.WorkingCopyChanged, this._workingCopyChanged, this);
    this._update();
}

WebInspector.ResourceScriptFile.prototype = {
    _workingCopyCommitted: function(event)
    {
        /**
         * @param {?string} error
         */
        function innerCallback(error)
        {
            if (error) {
                this._update();
                WebInspector.showErrorMessage(error);
                return;
            }

            this._scriptSource = source;
            this._update();
        }
        if (!this._script)
            return;
        var source = this._uiSourceCode.workingCopy();
        if (this._script.hasSourceURL && !this._sourceEndsWithSourceURL(source))
            source += "\n //@ sourceURL=" + this._script.sourceURL;
        WebInspector.debuggerModel.setScriptSource(this._script.scriptId, source, innerCallback.bind(this));
    },

    /**
     * @return {boolean}
     */
    _isDiverged: function()
    {
        if (this._uiSourceCode.isDirty())
            return true;
        if (!this._script)
            return false;
        if (typeof this._scriptSource === "undefined")
            return false;
        return !this._sourceMatchesScriptSource(this._uiSourceCode.workingCopy(), this._scriptSource);
    },

    /**
     * @param {string} source
     * @param {string} scriptSource
     * @return {boolean}
     */
    _sourceMatchesScriptSource: function(source, scriptSource)
    {
        if (!scriptSource.startsWith(source))
            return false;
        var scriptSourceTail = scriptSource.substr(source.length).trim();
        return !scriptSourceTail || !!scriptSourceTail.match(/^\/\/@\ssourceURL=\s*(\S*?)\s*$/m);
    },

    /**
     * @param {string} source
     * @return {boolean}
     */
    _sourceEndsWithSourceURL: function(source)
    {
        return !!source.match(/\/\/@\ssourceURL=\s*(\S*?)\s*$/m);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _workingCopyChanged: function(event)
    {
        this._update();
    },

    _update: function()
    {
        if (this._isDiverged() && !this._hasDivergedFromVM)
            this._divergeFromVM();
        else if (!this._isDiverged() && this._hasDivergedFromVM)
            this._mergeToVM();
    },

    _divergeFromVM: function()
    {
        this._isDivergingFromVM = true;
        this._resourceScriptMapping._hasDivergedFromVM(this._uiSourceCode);
        delete this._isDivergingFromVM;
        this._hasDivergedFromVM = true;
        this.dispatchEventToListeners(WebInspector.ScriptFile.Events.DidDivergeFromVM, this._uiSourceCode);
    },

    _mergeToVM: function()
    {
        delete this._hasDivergedFromVM;
        this._isMergingToVM = true;
        this._resourceScriptMapping._hasMergedToVM(this._uiSourceCode);
        delete this._isMergingToVM;
        this.dispatchEventToListeners(WebInspector.ScriptFile.Events.DidMergeToVM, this._uiSourceCode);
    },

    /**
     * @return {boolean}
     */
    hasDivergedFromVM: function()
    {
        return this._hasDivergedFromVM;
    },

    /**
     * @return {boolean}
     */
    isDivergingFromVM: function()
    {
        return this._isDivergingFromVM;
    },

    /**
     * @return {boolean}
     */
    isMergingToVM: function()
    {
        return this._isMergingToVM;
    },

    checkMapping: function()
    {
        if (!this._script)
            return;
        if (typeof this._scriptSource !== "undefined")
            return;
        this._script.requestContent(callback.bind(this));

        /**
         * @param {?string} source
         * @param {boolean} encoded
         * @param {string} contentType
         */
        function callback(source, encoded, contentType)
        {
            this._scriptSource = source;
            this._update();
        }
    },

    dispose: function()
    {
        this._uiSourceCode.removeEventListener(WebInspector.UISourceCode.Events.WorkingCopyCommitted, this._workingCopyCommitted, this);
        this._uiSourceCode.removeEventListener(WebInspector.UISourceCode.Events.WorkingCopyChanged, this._workingCopyChanged, this);
    },

    __proto__: WebInspector.Object.prototype
}
