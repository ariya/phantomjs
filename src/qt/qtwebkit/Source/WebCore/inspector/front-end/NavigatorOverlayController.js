/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL GOOGLE INC.
 * OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @param {WebInspector.SidebarView} parentSidebarView
 * @param {WebInspector.View} navigatorView
 * @param {WebInspector.View} editorView
 */
WebInspector.NavigatorOverlayController = function(parentSidebarView, navigatorView, editorView)
{
    this._parentSidebarView = parentSidebarView;
    this._navigatorView = navigatorView;
    this._editorView = editorView;

    this._navigatorSidebarResizeWidgetElement = document.createElement("div");
    this._navigatorSidebarResizeWidgetElement.addStyleClass("scripts-navigator-resizer-widget");
    this._parentSidebarView.installResizer(this._navigatorSidebarResizeWidgetElement);
    this._navigatorView.element.appendChild(this._navigatorSidebarResizeWidgetElement);

    this._navigatorShowHideButton = new WebInspector.StatusBarButton(WebInspector.UIString("Hide navigator"), "scripts-navigator-show-hide-button", 3);
    this._navigatorShowHideButton.state = "pinned";
    this._navigatorShowHideButton.addEventListener("click", this._toggleNavigator, this);
    this._editorView.element.appendChild(this._navigatorShowHideButton.element);

    WebInspector.settings.navigatorHidden = WebInspector.settings.createSetting("navigatorHidden", true);
    if (WebInspector.settings.navigatorHidden.get())
        this._toggleNavigator();
}

WebInspector.NavigatorOverlayController.prototype = {
    wasShown: function()
    {
        window.setTimeout(this._maybeShowNavigatorOverlay.bind(this), 0);
    },

    _maybeShowNavigatorOverlay: function()
    {
        if (WebInspector.settings.navigatorHidden.get() && !WebInspector.settings.navigatorWasOnceHidden.get())
            this.showNavigatorOverlay();
    },

    _toggleNavigator: function()
    {
        if (this._navigatorShowHideButton.state === "overlay")
            this._pinNavigator();
        else if (this._navigatorShowHideButton.state === "hidden")
            this.showNavigatorOverlay();
        else
            this._hidePinnedNavigator();
    },

    _hidePinnedNavigator: function()
    {
        this._navigatorShowHideButton.state = "hidden";
        this._navigatorShowHideButton.title = WebInspector.UIString("Show navigator");
        this._parentSidebarView.element.appendChild(this._navigatorShowHideButton.element);

        this._editorView.element.addStyleClass("navigator-hidden");
        this._navigatorSidebarResizeWidgetElement.addStyleClass("hidden");

        this._parentSidebarView.hideSidebarElement();
        this._navigatorView.detach();
        this._editorView.focus();

        WebInspector.settings.navigatorWasOnceHidden.set(true);
        WebInspector.settings.navigatorHidden.set(true);
    },

    _pinNavigator: function()
    {
        this._navigatorShowHideButton.state = "pinned";
        this._navigatorShowHideButton.title = WebInspector.UIString("Hide navigator");

        this._editorView.element.removeStyleClass("navigator-hidden");
        this._navigatorSidebarResizeWidgetElement.removeStyleClass("hidden");
        this._editorView.element.appendChild(this._navigatorShowHideButton.element);

        this._innerHideNavigatorOverlay();
        this._parentSidebarView.showSidebarElement();
        this._navigatorView.show(this._parentSidebarView.sidebarElement);
        this._navigatorView.focus();
        WebInspector.settings.navigatorHidden.set(false);
    },

    showNavigatorOverlay: function()
    {
        if (this._navigatorShowHideButton.state === "overlay")
            return;

        this._navigatorShowHideButton.state = "overlay";
        this._navigatorShowHideButton.title = WebInspector.UIString("Pin navigator");

        this._sidebarOverlay = new WebInspector.SidebarOverlay(this._navigatorView, "scriptsPanelNavigatorOverlayWidth", Preferences.minScriptsSidebarWidth);
        this._boundKeyDown = this._keyDown.bind(this);
        this._sidebarOverlay.element.addEventListener("keydown", this._boundKeyDown, false);
        var navigatorOverlayResizeWidgetElement = document.createElement("div");
        navigatorOverlayResizeWidgetElement.addStyleClass("scripts-navigator-resizer-widget");
        this._sidebarOverlay.resizerWidgetElement = navigatorOverlayResizeWidgetElement;

        this._navigatorView.element.appendChild(this._navigatorShowHideButton.element);
        this._boundContainingElementFocused = this._containingElementFocused.bind(this);
        this._parentSidebarView.element.addEventListener("mousedown", this._boundContainingElementFocused, false);

        this._sidebarOverlay.show(this._parentSidebarView.element);
        this._navigatorView.focus();
    },

    _keyDown: function(event)
    {
        if (event.handled)
            return;

        if (event.keyCode === WebInspector.KeyboardShortcut.Keys.Esc.code) {
            this.hideNavigatorOverlay();
            event.consume(true);
        }
    },

    hideNavigatorOverlay: function()
    {
        if (this._navigatorShowHideButton.state !== "overlay")
            return;

        this._navigatorShowHideButton.state = "hidden";
        this._navigatorShowHideButton.title = WebInspector.UIString("Show navigator");
        this._parentSidebarView.element.appendChild(this._navigatorShowHideButton.element);

        this._innerHideNavigatorOverlay();
        this._editorView.focus();
    },

    _innerHideNavigatorOverlay: function()
    {
        this._parentSidebarView.element.removeEventListener("mousedown", this._boundContainingElementFocused, false);
        this._sidebarOverlay.element.removeEventListener("keydown", this._boundKeyDown, false);
        this._sidebarOverlay.hide();
    },

    _containingElementFocused: function(event)
    {
        if (!event.target.isSelfOrDescendant(this._sidebarOverlay.element))
            this.hideNavigatorOverlay();
    },
    
    isNavigatorPinned: function()
    {
        return this._navigatorShowHideButton.state === "pinned";
    },
    
    isNavigatorHidden: function()
    {
        return this._navigatorShowHideButton.state === "hidden";
    }
}
