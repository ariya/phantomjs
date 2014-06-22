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

WebInspector.ScopeBar = function(identifier, items, defaultItem) {
    WebInspector.NavigationItem.call(this, identifier);

    this._element.classList.add(WebInspector.ScopeBar.StyleClassName);

    this._items = items;
    this._defaultItem = defaultItem;

    this._itemsById = [];
    this._populate();
};

WebInspector.ScopeBar.StyleClassName = "scope-bar";
WebInspector.ScopeBar.Event = {
    SelectionChanged: "scopebar-selection-did-change"
};

WebInspector.ScopeBar.prototype = {
    constructor: WebInspector.ScopeBar,

    // Public

    get defaultItem()
    {
        return this._defaultItem;
    },

    item: function(id)
    {
        return this._itemsById[id];
    },

    get selectedItems()
    {
        return this._items.filter(function(item) {
            return item.selected;
        });
    },

    updateLayout: function(expandOnly)
    {
        if (expandOnly)
            return;

        for (var i = 0; i < this._items.length; ++i) {
            var item = this._items[i];
            var isSelected = item.selected;

            if (!isSelected)
                item.element.classList.add(WebInspector.ScopeBarItem.SelectedStyleClassName);

            var selectedWidth = item.element.offsetWidth;
            if (selectedWidth)
                item.element.style.minWidth = selectedWidth + "px";

            if (!isSelected)
                item.element.classList.remove(WebInspector.ScopeBarItem.SelectedStyleClassName);
        }
    },

    // Private
    
    _populate: function()
    {
        var item;
        for (var i = 0; i < this._items.length; ++i) {
            item = this._items[i];
            this._itemsById[item.id] = item;
            this._element.appendChild(item.element);

            item.addEventListener(WebInspector.ScopeBarItem.Event.SelectionChanged, this._itemSelectionDidChange, this);
        }

        if (!this.selectedItems.length && this._defaultItem)
            this._defaultItem.selected = true;
    },
    
    _itemSelectionDidChange: function(event)
    {
        var sender = event.target;
        var item;

        // An exclusive item was selected, unselect everything else.
        if (sender.isExclusive && sender.selected) {
            for (var i = 0; i < this._items.length; ++i) {
                item = this._items[i];
                if (item !== sender)
                    item.selected = false;
            }
        } else {
            var replacesCurrentSelection = !event.data.withModifier;
            for (var i = 0; i < this._items.length; ++i) {
                item = this._items[i];
                if (item.isExclusive && item !== sender && sender.selected)
                    item.selected = false;
                else if (sender.selected && replacesCurrentSelection && sender !== item)
                    item.selected = false;
            }
        }

        // If nothing is selected anymore, select the default item.
        if (!this.selectedItems.length && this._defaultItem)
            this._defaultItem.selected = true;

        this.dispatchEventToListeners(WebInspector.ScopeBar.Event.SelectionChanged);
    }
};

WebInspector.ScopeBar.prototype.__proto__ = WebInspector.NavigationItem.prototype;
