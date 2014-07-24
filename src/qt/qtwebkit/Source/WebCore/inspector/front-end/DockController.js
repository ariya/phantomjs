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
 */
WebInspector.DockController = function()
{
    this._dockToggleButton = new WebInspector.StatusBarButton("", "dock-status-bar-item", 3);
    this._dockToggleButtonOption = new WebInspector.StatusBarButton("", "dock-status-bar-item", 3);
    this._dockToggleButton.addEventListener("click", this._toggleDockState, this);
    this._dockToggleButtonOption.addEventListener("click", this._toggleDockState, this);
    if (Preferences.showDockToRight)
        this._dockToggleButton.makeLongClickEnabled(this._createDockOptions.bind(this));

    this.setDockSide(WebInspector.queryParamsObject["dockSide"] || "bottom");
    WebInspector.settings.showToolbarIcons.addChangeListener(this._updateUI.bind(this));
}

WebInspector.DockController.State = {
    DockedToBottom: "bottom",
    DockedToRight: "right",
    Undocked: "undocked"
}

WebInspector.DockController.Events = {
    DockSideChanged: "DockSideChanged"
}

WebInspector.DockController.prototype = {
    /**
     * @return {Element}
     */
    get element()
    {
        return this._dockToggleButton.element;
    },

    /**
     * @return {string}
     */
    dockSide: function()
    {
        return this._dockSide;
    },

    /**
     * @param {string} dockSide
     */
    setDockSide: function(dockSide)
    {
        if (this._dockSide === dockSide)
            return;

        if (this._dockSide)
            WebInspector.settings.lastDockState.set(this._dockSide);

        this._dockSide = dockSide;
        if (dockSide === WebInspector.DockController.State.Undocked) 
            WebInspector.userMetrics.WindowDocked.record();
        else
            WebInspector.userMetrics.WindowUndocked.record();
        this._updateUI();
        this.dispatchEventToListeners(WebInspector.DockController.Events.DockSideChanged, this._dockSide);
    },

    /**
     * @param {boolean} unavailable
     */
    setDockingUnavailable: function(unavailable)
    {
        this._isDockingUnavailable = unavailable;
        this._updateUI();
    },

    _updateUI: function()
    {
        var body = document.body;
        switch (this._dockSide) {
        case WebInspector.DockController.State.DockedToBottom:
            body.removeStyleClass("undocked");
            body.removeStyleClass("dock-to-right");
            body.addStyleClass("dock-to-bottom");
            break;
        case WebInspector.DockController.State.DockedToRight: 
            body.removeStyleClass("undocked");
            body.addStyleClass("dock-to-right");
            body.removeStyleClass("dock-to-bottom");
            break;
        case WebInspector.DockController.State.Undocked: 
            body.addStyleClass("undocked");
            body.removeStyleClass("dock-to-right");
            body.removeStyleClass("dock-to-bottom");
            break;
        }

        if (WebInspector.settings.showToolbarIcons.get())
            document.body.addStyleClass("show-toolbar-icons");
        else
            document.body.removeStyleClass("show-toolbar-icons");

        if (this._isDockingUnavailable && this._dockSide === WebInspector.DockController.State.Undocked) {
            this._dockToggleButton.state = "undock";
            this._dockToggleButton.setEnabled(false);
            return;
        }

        this._dockToggleButton.setEnabled(true);

        // Choose different last state based on the current one if missing or if is the same.
        var sides = [WebInspector.DockController.State.DockedToBottom, WebInspector.DockController.State.Undocked, WebInspector.DockController.State.DockedToRight];
        sides.remove(this._dockSide);
        var lastState = WebInspector.settings.lastDockState.get();

        sides.remove(lastState);
        if (sides.length === 2) { // last state was not from the list of potential values
            lastState = sides[0];
            sides.remove(lastState);
        }
        this._decorateButtonForTargetState(this._dockToggleButton, lastState);
        this._decorateButtonForTargetState(this._dockToggleButtonOption, sides[0]);
    },

    /**
     * @param {WebInspector.StatusBarButton} button
     * @param {string} state
     */
    _decorateButtonForTargetState: function(button, state)
    {
        switch (state) {
        case WebInspector.DockController.State.DockedToBottom:
            button.title = WebInspector.UIString("Dock to main window.");
            button.state = "bottom";
            break;
        case WebInspector.DockController.State.DockedToRight:
            button.title = WebInspector.UIString("Dock to main window.");
            button.state = "right";
            break;
        case WebInspector.DockController.State.Undocked: 
            button.title = WebInspector.UIString("Undock into separate window.");
            button.state = "undock";
            break;
        }
    },

    _createDockOptions: function()
    {
        return [this._dockToggleButtonOption];
    },

    /**
     * @param {WebInspector.Event} e
     */
    _toggleDockState: function(e)
    {
        var action;
        switch (e.target.state) {
        case "bottom": action = "bottom"; break;
        case "right": action = "right"; break;
        case "undock": action = "undocked"; break;
        }
        InspectorFrontendHost.requestSetDockSide(action);
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @type {?WebInspector.DockController}
 */
WebInspector.dockController = null;
