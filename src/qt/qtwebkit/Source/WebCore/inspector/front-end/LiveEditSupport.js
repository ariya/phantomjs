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
 * @param {WebInspector.Workspace} workspace
 */
WebInspector.LiveEditSupport = function(workspace)
{
    this._workspaceProvider = new WebInspector.SimpleWorkspaceProvider(workspace, WebInspector.projectTypes.LiveEdit);
    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.GlobalObjectCleared, this._debuggerReset, this);
    this._debuggerReset();
}

WebInspector.LiveEditSupport.prototype = {
    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @return {WebInspector.UISourceCode}
     */
    uiSourceCodeForLiveEdit: function(uiSourceCode)
    {
        var rawLocation = uiSourceCode.uiLocationToRawLocation(0, 0);
        var debuggerModelLocation = /** @type {WebInspector.DebuggerModel.Location} */ (rawLocation);
        var script = WebInspector.debuggerModel.scriptForId(debuggerModelLocation.scriptId);
        var uiLocation = script.rawLocationToUILocation(0, 0);

        // FIXME: Support live editing of scripts mapped to some file.
        if (uiLocation.uiSourceCode !== uiSourceCode)
            return uiLocation.uiSourceCode;
        if (this._uiSourceCodeForScriptId[script.scriptId])
            return this._uiSourceCodeForScriptId[script.scriptId];

        console.assert(!script.isInlineScript());
        var liveEditUISourceCode = this._workspaceProvider.addUniqueFileForURL(script.sourceURL, script, true, script.isContentScript);

        liveEditUISourceCode.setScriptFile(new WebInspector.LiveEditScriptFile(uiSourceCode, liveEditUISourceCode, script.scriptId));
        this._uiSourceCodeForScriptId[script.scriptId] = liveEditUISourceCode;
        this._scriptIdForUISourceCode.put(liveEditUISourceCode, script.scriptId);
        return liveEditUISourceCode;
    },

    _debuggerReset: function()
    {
        /** @type {Object.<string, WebInspector.UISourceCode>} */
        this._uiSourceCodeForScriptId = {};
        this._scriptIdForUISourceCode = new Map();
        this._workspaceProvider.reset();
    },
}

/**
 * @constructor
 * @implements {WebInspector.ScriptFile}
 * @extends {WebInspector.Object}
 * @param {WebInspector.UISourceCode} uiSourceCode
 * @param {WebInspector.UISourceCode} liveEditUISourceCode
 * @param {string} scriptId
 */
WebInspector.LiveEditScriptFile = function(uiSourceCode, liveEditUISourceCode, scriptId)
{
    WebInspector.ScriptFile.call(this);
    this._uiSourceCode = uiSourceCode;
    this._liveEditUISourceCode = liveEditUISourceCode;
    this._scriptId = scriptId;
    this._liveEditUISourceCode.addEventListener(WebInspector.UISourceCode.Events.WorkingCopyCommitted, this._workingCopyCommitted, this);
}

WebInspector.LiveEditScriptFile.prototype = {
    _workingCopyCommitted: function(event)
    {
        /**
         * @param {?string} error
         */
        function innerCallback(error)
        {
            if (error) {
                WebInspector.showErrorMessage(error);
                return;
            }
        }

        var script = WebInspector.debuggerModel.scriptForId(this._scriptId);
        WebInspector.debuggerModel.setScriptSource(script.scriptId, this._liveEditUISourceCode.workingCopy(), innerCallback.bind(this));
    },

    /**
     * @return {boolean}
     */
    hasDivergedFromVM: function()
    {
        return true;
    },

    /**
     * @return {boolean}
     */
    isDivergingFromVM: function()
    {
        return false;
    },

    /**
     * @return {boolean}
     */
    isMergingToVM: function()
    {
        return false;
    },

    checkMapping: function()
    {
    },

    __proto__: WebInspector.Object.prototype
}

/** @type {WebInspector.LiveEditSupport} */
WebInspector.liveEditSupport = null;
