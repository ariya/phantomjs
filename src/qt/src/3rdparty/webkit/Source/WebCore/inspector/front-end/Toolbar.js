 /*
 * Copyright (C) 2006, 2007, 2008 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Matt Lilek (pewtermoose@gmail.com).
 * Copyright (C) 2009 Joseph Pecoraro
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.Toolbar = function()
{
    this.element = document.getElementById("toolbar");
    this.element.addEventListener("mousedown", this._toolbarDragStart.bind(this), true);

    this._dropdownButton = document.getElementById("toolbar-dropdown-arrow");
    this._dropdownButton.addEventListener("click", this._toggleDropdown.bind(this), false);

    document.getElementById("close-button-left").addEventListener("click", this._onClose, true);
    document.getElementById("close-button-right").addEventListener("click", this._onClose, true);
}

WebInspector.Toolbar.prototype = {
    set attached(attached)
    {
        if (attached)
            this.element.addStyleClass("toolbar-small");
        else
            this.element.removeStyleClass("toolbar-small");
        this._updateDropdownButtonAndHideDropdown();
    },

    resize: function()
    {
        this._updateDropdownButtonAndHideDropdown();
    },

    addPanel: function(panel)
    {
        this.element.appendChild(panel.toolbarItem);
        this.resize();
    },

    _toolbarDragStart: function(event)
    {
        if ((!WebInspector.attached && WebInspector.platformFlavor !== WebInspector.PlatformFlavor.MacLeopard && WebInspector.platformFlavor !== WebInspector.PlatformFlavor.MacSnowLeopard) || WebInspector.port == "qt")
            return;

        var target = event.target;
        if (target.hasStyleClass("toolbar-item") && target.hasStyleClass("toggleable"))
            return;

        if (target !== this.element && !target.hasStyleClass("toolbar-item"))
            return;

        this.element.lastScreenX = event.screenX;
        this.element.lastScreenY = event.screenY;

        WebInspector.elementDragStart(this.element, this._toolbarDrag.bind(this), this._toolbarDragEnd.bind(this), event, (WebInspector.attached ? "row-resize" : "default"));
    },

    _toolbarDragEnd: function(event)
    {
        WebInspector.elementDragEnd(event);

        delete this.element.lastScreenX;
        delete this.element.lastScreenY;
    },

    _toolbarDrag: function(event)
    {
        if (WebInspector.attached) {
            var height = window.innerHeight - (event.screenY - this.element.lastScreenY);

            InspectorFrontendHost.setAttachedWindowHeight(height);
        } else {
            var x = event.screenX - this.element.lastScreenX;
            var y = event.screenY - this.element.lastScreenY;

            // We cannot call window.moveBy here because it restricts the movement
            // of the window at the edges.
            InspectorFrontendHost.moveWindowBy(x, y);
        }

        this.element.lastScreenX = event.screenX;
        this.element.lastScreenY = event.screenY;

        event.preventDefault();
    },

    _onClose: function()
    {
        WebInspector.close();
    },

    _setDropdownVisible: function(visible)
    {
        if (!this._dropdown) {
            if (!visible)
                return;
            this._dropdown = new WebInspector.ToolbarDropdown();
        }
        if (visible)
            this._dropdown.show();
        else
            this._dropdown.hide();
    },

    _toggleDropdown: function()
    {
        this._setDropdownVisible(!this._dropdown || !this._dropdown.visible);
    },

    _updateDropdownButtonAndHideDropdown: function()
    {
        this._setDropdownVisible(false);

        var toolbar = document.getElementById("toolbar");
        if (this.element.scrollHeight > this.element.clientHeight)
            this._dropdownButton.removeStyleClass("hidden");
        else
            this._dropdownButton.addStyleClass("hidden");
    }
};

WebInspector.Toolbar.createPanelToolbarItem = function(panel)
{
    var toolbarItem = document.createElement("button");
    toolbarItem.className = "toolbar-item toggleable";
    toolbarItem.panel = panel;
    toolbarItem.addStyleClass(panel._panelName);
    function onToolbarItemClicked()
    {
        WebInspector.toolbar._updateDropdownButtonAndHideDropdown();
        WebInspector.currentPanel = panel;
    }
    toolbarItem.addEventListener("click", onToolbarItemClicked);

    var iconElement = toolbarItem.createChild("div", "toolbar-icon");

    if ("toolbarItemLabel" in panel)
        toolbarItem.createChild("div", "toolbar-label").textContent = panel.toolbarItemLabel;

    if (panel === WebInspector.currentPanel)
        toolbarItem.addStyleClass("toggled-on");

    return toolbarItem;
}

WebInspector.ToolbarDropdown = function()
{
    this._toolbar = document.getElementById("toolbar");
    this._arrow = document.getElementById("toolbar-dropdown-arrow");
    this.element = document.createElement("div");
    this.element.id = "toolbar-dropdown";
    this.element.className = "toolbar-small";
    this._contentElement = this.element.createChild("div", "scrollable-content");
    this._contentElement.tabIndex = 0;
    this._contentElement.addEventListener("keydown", this._onKeyDown.bind(this), true);
}

WebInspector.ToolbarDropdown.prototype = {
    show: function()
    {
        if (this.visible)
            return;
        var style = this.element.style;
        this._populate();
        var top = this._arrow.totalOffsetTop + this._arrow.clientHeight;
        this._arrow.addStyleClass("dropdown-visible");
        this.element.style.top = top + "px";
        this.element.style.left = this._arrow.totalOffsetLeft + "px";
        this._contentElement.style.maxHeight = window.innerHeight - top - 20 + "px";
        this._toolbar.appendChild(this.element);
        WebInspector.currentFocusElement = this.contentElement;
    },

    hide: function()
    {
        if (!this.visible)
            return;
        this._arrow.removeStyleClass("dropdown-visible");
        this.element.parentNode.removeChild(this.element);
        this._contentElement.removeChildren();
    },

    get visible()
    {
        return !!this.element.parentNode;
    },

    _populate: function()
    {
        var toolbarItems = this._toolbar.querySelectorAll(".toolbar-item.toggleable");

        for (var i = 0; i < toolbarItems.length; ++i) {
            if (toolbarItems[i].offsetTop > 0)
                this._contentElement.appendChild(WebInspector.Toolbar.createPanelToolbarItem(toolbarItems[i].panel));
        }
    },

    _onKeyDown: function(event)
    {
        if (event.keyCode !== WebInspector.KeyboardShortcut.Keys.Esc.code)
            return;
        event.stopPropagation();
        this.hide();
    }
};
