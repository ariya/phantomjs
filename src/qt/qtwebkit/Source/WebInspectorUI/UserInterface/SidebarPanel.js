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

WebInspector.SidebarPanel = function(identifier, displayName, showToolTip, hideToolTip, image, element, role, label) {
    WebInspector.Object.call(this);

    this._identifier = identifier;

    this._toolbarItem = new WebInspector.ActivateButtonToolbarItem(identifier, showToolTip, hideToolTip, displayName, image);
    this._toolbarItem.addEventListener(WebInspector.ButtonNavigationItem.Event.Clicked, this.toggle, this);
    this._toolbarItem.enabled = false;

    this._element = element || document.createElement("div");
    this._element.classList.add(WebInspector.SidebarPanel.StyleClassName);
    this._element.classList.add(identifier);

    this._element.setAttribute("role", role || "group");
    this._element.setAttribute("aria-label", label || displayName);

};

WebInspector.SidebarPanel.StyleClassName = "panel";
WebInspector.SidebarPanel.SelectedStyleClassName = "selected";

WebInspector.SidebarPanel.prototype = {
    constructor: WebInspector.SidebarPanel,

    // Public

    get identifier()
    {
        return this._identifier;
    },

    get toolbarItem()
    {
        return this._toolbarItem;
    },

    get element()
    {
        return this._element;
    },

    get visible()
    {
        return this.selected && this._parentSidebar && !this._parentSidebar.collapsed;
    },

    get selected()
    {
        return this._element.classList.contains(WebInspector.SidebarPanel.SelectedStyleClassName);
    },

    set selected(flag)
    {
        if (flag)
            this._element.classList.add(WebInspector.SidebarPanel.SelectedStyleClassName);
        else
            this._element.classList.remove(WebInspector.SidebarPanel.SelectedStyleClassName);
    },

    get parentSidebar()
    {
        return this._parentSidebar;
    },

    show: function()
    {
        if (!this._parentSidebar)
            return;

        this._parentSidebar.collapsed = false;
        this._parentSidebar.selectedSidebarPanel = this;
    },

    hide: function()
    {
        if (!this._parentSidebar)
            return;

        this._parentSidebar.collapsed = true;
        this._parentSidebar.selectedSidebarPanel = null;
    },

    toggle: function()
    {
        if (this.visible)
            this.hide();
        else
            this.show();
    },

    added: function()
    {
        console.assert(this._parentSidebar);
        this._toolbarItem.enabled = true;
        this._toolbarItem.activated = this.visible;
    },

    removed: function()
    {
        console.assert(!this._parentSidebar);
        this._toolbarItem.enabled = false;
        this._toolbarItem.activated = false;
    },

    willRemove: function()
    {
        // Implemented by subclasses.
    },

    shown: function()
    {
        // Implemented by subclasses.
    },

    hidden: function()
    {
        // Implemented by subclasses.
    },

    widthDidChange: function()
    {
        // Implemented by subclasses.
    },

    visibilityDidChange: function()
    {
        this._toolbarItem.activated = this.visible;
    }
};

WebInspector.SidebarPanel.prototype.__proto__ = WebInspector.Object.prototype;
