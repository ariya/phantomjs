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
 * @extends {WebInspector.SidebarPane}
 */
WebInspector.CallStackSidebarPane = function()
{
    WebInspector.SidebarPane.call(this, WebInspector.UIString("Call Stack"));
    this._model = WebInspector.debuggerModel;

    this.bodyElement.addEventListener("keydown", this._keyDown.bind(this), true);
    this.bodyElement.tabIndex = 0;
}

WebInspector.CallStackSidebarPane.prototype = {
    update: function(callFrames)
    {
        this.bodyElement.removeChildren();
        delete this._statusMessageElement;
        this.placards = [];

        if (!callFrames) {
            var infoElement = document.createElement("div");
            infoElement.className = "info";
            infoElement.textContent = WebInspector.UIString("Not Paused");
            this.bodyElement.appendChild(infoElement);
            return;
        }

        for (var i = 0; i < callFrames.length; ++i) {
            var callFrame = callFrames[i];
            var placard = new WebInspector.CallStackSidebarPane.Placard(callFrame, this);
            placard.element.addEventListener("click", this._placardSelected.bind(this, placard), false);
            this.placards.push(placard);
            this.bodyElement.appendChild(placard.element);
        }
    },

    setSelectedCallFrame: function(x)
    {
        for (var i = 0; i < this.placards.length; ++i) {
            var placard = this.placards[i];
            placard.selected = (placard._callFrame === x);
        }
    },

    /**
     * @param {Event=} event
     * @return {boolean}
     */
    _selectNextCallFrameOnStack: function(event)
    {
        var index = this._selectedCallFrameIndex();
        if (index == -1)
            return true;
        this._selectedPlacardByIndex(index + 1);
        return true;
    },

    /**
     * @param {Event=} event
     * @return {boolean}
     */
    _selectPreviousCallFrameOnStack: function(event)
    {
        var index = this._selectedCallFrameIndex();
        if (index == -1)
            return true;
        this._selectedPlacardByIndex(index - 1);
        return true;
    },

    /**
     * @param {number} index
     */
    _selectedPlacardByIndex: function(index)
    {
        if (index < 0 || index >= this.placards.length)
            return;
        this._placardSelected(this.placards[index])
    },

    /**
     * @return {number}
     */
    _selectedCallFrameIndex: function()
    {
        if (!this._model.selectedCallFrame())
            return -1;
        for (var i = 0; i < this.placards.length; ++i) {
            var placard = this.placards[i];
            if (placard._callFrame === this._model.selectedCallFrame())
                return i;
        }
        return -1;
    },

    _placardSelected: function(placard)
    {
        this._model.setSelectedCallFrame(placard._callFrame);
    },

    _copyStackTrace: function()
    {
        var text = "";
        for (var i = 0; i < this.placards.length; ++i)
            text += this.placards[i].title + " (" + this.placards[i].subtitle + ")\n";
        InspectorFrontendHost.copyText(text);
    },

    /**
     * @param {function(!Array.<!WebInspector.KeyboardShortcut.Descriptor>, function(Event=):boolean)} registerShortcutDelegate
     */
    registerShortcuts: function(registerShortcutDelegate)
    {
        registerShortcutDelegate(WebInspector.ScriptsPanelDescriptor.ShortcutKeys.NextCallFrame, this._selectNextCallFrameOnStack.bind(this));
        registerShortcutDelegate(WebInspector.ScriptsPanelDescriptor.ShortcutKeys.PrevCallFrame, this._selectPreviousCallFrameOnStack.bind(this));
    },

    setStatus: function(status)
    {
        if (!this._statusMessageElement) {
            this._statusMessageElement = document.createElement("div");
            this._statusMessageElement.className = "info";
            this.bodyElement.appendChild(this._statusMessageElement);
        }
        if (typeof status === "string")
            this._statusMessageElement.textContent = status;
        else {
            this._statusMessageElement.removeChildren();
            this._statusMessageElement.appendChild(status);
        }
    },

    _keyDown: function(event)
    {
        if (event.altKey || event.shiftKey || event.metaKey || event.ctrlKey)
            return;

        if (event.keyIdentifier === "Up") {
            this._selectPreviousCallFrameOnStack();
            event.consume();
        } else if (event.keyIdentifier === "Down") {
            this._selectNextCallFrameOnStack();
            event.consume();
        }
    },

    __proto__: WebInspector.SidebarPane.prototype
}

/**
 * @constructor
 * @extends {WebInspector.Placard}
 * @param {WebInspector.DebuggerModel.CallFrame} callFrame
 * @param {WebInspector.CallStackSidebarPane} pane
 */
WebInspector.CallStackSidebarPane.Placard = function(callFrame, pane)
{
    WebInspector.Placard.call(this, callFrame.functionName || WebInspector.UIString("(anonymous function)"), "");
    callFrame.createLiveLocation(this._update.bind(this));
    this.element.addEventListener("contextmenu", this._placardContextMenu.bind(this), true);
    this._callFrame = callFrame;
    this._pane = pane;
}

WebInspector.CallStackSidebarPane.Placard.prototype = {
    _update: function(uiLocation)
    {
        this.subtitle = WebInspector.formatLinkText(uiLocation.uiSourceCode.originURL(), uiLocation.lineNumber).centerEllipsizedToLength(100);
    },

    _placardContextMenu: function(event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);

        if (WebInspector.debuggerModel.canSetScriptSource()) {
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Restart frame" : "Restart Frame"), this._restartFrame.bind(this));
            contextMenu.appendSeparator();
        }
        contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Copy stack trace" : "Copy Stack Trace"), this._pane._copyStackTrace.bind(this._pane));

        contextMenu.show();
    },

    _restartFrame: function()
    {
        this._callFrame.restart(undefined);
    },

    __proto__: WebInspector.Placard.prototype
}
