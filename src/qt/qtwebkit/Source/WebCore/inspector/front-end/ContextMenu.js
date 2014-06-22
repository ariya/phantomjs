/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
 * @param {WebInspector.ContextSubMenuItem} topLevelMenu
 * @param {string} type
 * @param {string=} label
 * @param {boolean=} disabled
 * @param {boolean=} checked
 */
WebInspector.ContextMenuItem = function(topLevelMenu, type, label, disabled, checked)
{
    this._type = type;
    this._label = label;
    this._disabled = disabled;
    this._checked = checked;
    this._contextMenu = topLevelMenu;
    if (type === "item" || type === "checkbox")
        this._id = topLevelMenu.nextId();
}

WebInspector.ContextMenuItem.prototype = {
    id: function()
    {
        return this._id;
    },

    type: function()
    {
        return this._type;
    },

    /**
     * @return {boolean}
     */
    isEnabled: function()
    {
        return !this._disabled;
    },

    /**
     * @param {boolean} enabled
     */
    setEnabled: function(enabled)
    {
        this._disabled = !enabled;
    },

    _buildDescriptor: function()
    {
        switch (this._type) {
        case "item":
            return { type: "item", id: this._id, label: this._label, enabled: !this._disabled };
        case "separator":
            return { type: "separator" };
        case "checkbox":
            return { type: "checkbox", id: this._id, label: this._label, checked: !!this._checked, enabled: !this._disabled };
        }
    }
}

/**
 * @constructor
 * @extends {WebInspector.ContextMenuItem}
 * @param topLevelMenu
 * @param {string=} label
 * @param {boolean=} disabled
 */
WebInspector.ContextSubMenuItem = function(topLevelMenu, label, disabled)
{
    WebInspector.ContextMenuItem.call(this, topLevelMenu, "subMenu", label, disabled);
    /** @type {!Array.<!WebInspector.ContextMenuItem>} */
    this._items = [];
}

WebInspector.ContextSubMenuItem.prototype = {
    /**
     * @param {string} label
     * @param {function(?)} handler
     * @param {boolean=} disabled
     * @return {WebInspector.ContextMenuItem}
     */
    appendItem: function(label, handler, disabled)
    {
        var item = new WebInspector.ContextMenuItem(this._contextMenu, "item", label, disabled);
        this._pushItem(item);
        this._contextMenu._setHandler(item.id(), handler);
        return item;
    },

    /**
     * @param {string} label
     * @param {boolean=} disabled
     * @return {WebInspector.ContextMenuItem}
     */
    appendSubMenuItem: function(label, disabled)
    {
        var item = new WebInspector.ContextSubMenuItem(this._contextMenu, label, disabled);
        this._pushItem(item);
        return item;
    },

    /**
     * @param {boolean=} disabled
     */
    appendCheckboxItem: function(label, handler, checked, disabled)
    {
        var item = new WebInspector.ContextMenuItem(this._contextMenu, "checkbox", label, disabled, checked);
        this._pushItem(item);
        this._contextMenu._setHandler(item.id(), handler);
        return item;
    },

    appendSeparator: function()
    {
        if (this._items.length)
            this._pendingSeparator = true;
    },

    /**
     * @param {!WebInspector.ContextMenuItem} item
     */
    _pushItem: function(item)
    {
        if (this._pendingSeparator) {
            this._items.push(new WebInspector.ContextMenuItem(this._contextMenu, "separator"));
            delete this._pendingSeparator;
        }
        this._items.push(item);
    },

    /**
     * @return {boolean}
     */
    isEmpty: function()
    {
        return !this._items.length;
    },

    _buildDescriptor: function()
    {
        var result = { type: "subMenu", label: this._label, enabled: !this._disabled, subItems: [] };
        for (var i = 0; i < this._items.length; ++i)
            result.subItems.push(this._items[i]._buildDescriptor());
        return result;
    },

    __proto__: WebInspector.ContextMenuItem.prototype
}

/**
 * @constructor
 * @extends {WebInspector.ContextSubMenuItem}
 */
WebInspector.ContextMenu = function(event) {
    WebInspector.ContextSubMenuItem.call(this, this, "");
    this._event = event;
    this._handlers = {};
    this._id = 0;
}

WebInspector.ContextMenu.prototype = {
    nextId: function()
    {
        return this._id++;
    },

    show: function()
    {
        var menuObject = this._buildDescriptor();

        if (menuObject.length) {
            WebInspector._contextMenu = this;
            InspectorFrontendHost.showContextMenu(this._event, menuObject);
            this._event.consume();
        }
    },

    showSoftMenu: function()
    {
        var menuObject = this._buildDescriptor();

        if (menuObject.length) {
            WebInspector._contextMenu = this;
            var softMenu = new WebInspector.SoftContextMenu(menuObject);
            softMenu.show(this._event, true);
        }
        this._event.consume();
    },

    _setHandler: function(id, handler)
    {
        if (handler)
            this._handlers[id] = handler;
    },

    _buildDescriptor: function()
    {
        var result = [];
        for (var i = 0; i < this._items.length; ++i)
            result.push(this._items[i]._buildDescriptor());
        return result;
    },

    _itemSelected: function(id)
    {
        if (this._handlers[id])
            this._handlers[id].call(this);
    },

    /**
     * @param {Object} target
     */
    appendApplicableItems: function(target)
    {
        for (var i = 0; i < WebInspector.ContextMenu._providers.length; ++i) {
            var provider = WebInspector.ContextMenu._providers[i];
            this.appendSeparator();
            provider.appendApplicableItems(this._event, this, target);
            this.appendSeparator();
        }
    },

    __proto__: WebInspector.ContextSubMenuItem.prototype
}

/**
 * @interface
 */
WebInspector.ContextMenu.Provider = function() { 
}

WebInspector.ContextMenu.Provider.prototype = {
    /** 
     * @param {WebInspector.ContextMenu} contextMenu
     * @param {Object} target
     */
    appendApplicableItems: function(event, contextMenu, target) { }
}

/**
 * @param {WebInspector.ContextMenu.Provider} provider
 */
WebInspector.ContextMenu.registerProvider = function(provider)
{
    WebInspector.ContextMenu._providers.push(provider);
}

WebInspector.ContextMenu._providers = [];

WebInspector.contextMenuItemSelected = function(id)
{
    if (WebInspector._contextMenu)
        WebInspector._contextMenu._itemSelected(id);
}

WebInspector.contextMenuCleared = function()
{
    // FIXME: Unfortunately, contextMenuCleared is invoked between show and item selected
    // so we can't delete last menu object from WebInspector. Fix the contract.
}
