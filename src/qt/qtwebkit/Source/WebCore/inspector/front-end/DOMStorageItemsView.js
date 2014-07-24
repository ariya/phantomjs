/*
 * Copyright (C) 2008 Nokia Inc.  All rights reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY
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
 * @extends {WebInspector.View}
 */
WebInspector.DOMStorageItemsView = function(domStorage, domStorageModel)
{
    WebInspector.View.call(this);

    this.domStorage = domStorage;
    this.domStorageModel = domStorageModel;

    this.element.addStyleClass("storage-view");
    this.element.addStyleClass("table");

    this.deleteButton = new WebInspector.StatusBarButton(WebInspector.UIString("Delete"), "delete-storage-status-bar-item");
    this.deleteButton.visible = false;
    this.deleteButton.addEventListener("click", this._deleteButtonClicked, this);

    this.refreshButton = new WebInspector.StatusBarButton(WebInspector.UIString("Refresh"), "refresh-storage-status-bar-item");
    this.refreshButton.addEventListener("click", this._refreshButtonClicked, this);

    this.domStorageModel.addEventListener(WebInspector.DOMStorageModel.Events.DOMStorageItemsCleared, this._domStorageItemsCleared, this);
    this.domStorageModel.addEventListener(WebInspector.DOMStorageModel.Events.DOMStorageItemRemoved, this._domStorageItemRemoved, this);
    this.domStorageModel.addEventListener(WebInspector.DOMStorageModel.Events.DOMStorageItemAdded, this._domStorageItemAdded, this);
    this.domStorageModel.addEventListener(WebInspector.DOMStorageModel.Events.DOMStorageItemUpdated, this._domStorageItemUpdated, this);
}

WebInspector.DOMStorageItemsView.prototype = {
    statusBarItems: function()
    {
        return [this.refreshButton.element, this.deleteButton.element];
    },

    wasShown: function()
    {
        this._update();
    },

    willHide: function()
    {
        this.deleteButton.visible = false;
    },

    /**
     * @param {WebInspector.Event} event
     */
    _domStorageItemsCleared: function(event)
    {
        if (!this.isShowing())
            return;

        this._dataGrid.rootNode().removeChildren();
        this._dataGrid.addCreationNode(false);
        this.deleteButton.visible = false;
        event.consume(true);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _domStorageItemRemoved: function(event)
    {
        if (!this.isShowing())
            return;

        var storageData = event.data;
        var rootNode = this._dataGrid.rootNode();
        var children = rootNode.children;

        event.consume(true);

        for (var i = 0; i < children.length; ++i) {
            var childNode = children[i];
            if (childNode.data.key === storageData.key) {
                rootNode.removeChild(childNode);
                this.deleteButton.visible = (children.length > 1);
                return;
            }
        }
    },

    /**
     * @param {WebInspector.Event} event
     */
    _domStorageItemAdded: function(event)
    {
        if (!this.isShowing())
            return;

        var storageData = event.data;
        var rootNode = this._dataGrid.rootNode();
        var children = rootNode.children;

        event.consume(true);
        this.deleteButton.visible = true;

        for (var i = 0; i < children.length; ++i)
            if (children[i].data.key === storageData.key)
                return;

        var childNode = new WebInspector.DataGridNode({key: storageData.key, value: storageData.newValue}, false);
        rootNode.insertChild(childNode, children.length - 1);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _domStorageItemUpdated: function(event)
    {
        if (!this.isShowing())
            return;

        var storageData = event.data;
        var rootNode = this._dataGrid.rootNode();
        var children = rootNode.children;

        event.consume(true);

        var keyFound = false;
        for (var i = 0; i < children.length; ++i) {
            var childNode = children[i];
            if (childNode.data.key === storageData.key) {
                if (keyFound) {
                    rootNode.removeChild(childNode);
                    return;
                }
                keyFound = true;
                if (childNode.data.value !== storageData.newValue) {
                    childNode.data.value = storageData.newValue;
                    childNode.refresh();
                    childNode.select();
                    childNode.reveal();
                }
                this.deleteButton.visible = true;
            }
        }
    },

    _update: function()
    {
        this.detachChildViews();
        this.domStorage.getItems(this._showDOMStorageItems.bind(this));
    },

    _showDOMStorageItems: function(error, items)
    {
        if (error)
            return;

        this._dataGrid = this._dataGridForDOMStorageItems(items);
        this._dataGrid.show(this.element);
        this._dataGrid.autoSizeColumns(10);
        this.deleteButton.visible = (this._dataGrid.rootNode().children.length > 1);
    },

    _dataGridForDOMStorageItems: function(items)
    {
        var columns = [
            {id: "key", title: WebInspector.UIString("Key"), editable: true},
            {id: "value", title: WebInspector.UIString("Value"), editable: true}
        ];

        var nodes = [];

        var keys = [];
        var length = items.length;
        for (var i = 0; i < items.length; i++) {
            var key = items[i][0];
            var value = items[i][1];
            var node = new WebInspector.DataGridNode({key: key, value: value}, false);
            node.selectable = true;
            nodes.push(node);
            keys.push(key);
        }

        var dataGrid = new WebInspector.DataGrid(columns, this._editingCallback.bind(this), this._deleteCallback.bind(this));
        length = nodes.length;
        for (var i = 0; i < length; ++i)
            dataGrid.rootNode().appendChild(nodes[i]);
        dataGrid.addCreationNode(false);
        if (length > 0)
            nodes[0].selected = true;
        return dataGrid;
    },

    _deleteButtonClicked: function(event)
    {
        if (!this._dataGrid || !this._dataGrid.selectedNode)
            return;

        this._deleteCallback(this._dataGrid.selectedNode);
        this._dataGrid.changeNodeAfterDeletion();
    },

    _refreshButtonClicked: function(event)
    {
        this._update();
    },

    _editingCallback: function(editingNode, columnIdentifier, oldText, newText)
    {
        var domStorage = this.domStorage;
        if ("key" === columnIdentifier) {
            if (oldText)
                domStorage.removeItem(oldText);
            domStorage.setItem(newText, editingNode.data.value);
            this._removeDupes(editingNode);
        } else
            domStorage.setItem(editingNode.data.key, newText);
    },

    /**
     * @param {!WebInspector.DataGridNode} masterNode
     */
    _removeDupes: function(masterNode)
    {
        var rootNode = this._dataGrid.rootNode();
        var children = rootNode.children;
        for (var i = children.length - 1; i >= 0; --i) {
            var childNode = children[i];
            if ((childNode.data.key === masterNode.data.key) && (masterNode !== childNode))
                rootNode.removeChild(childNode);
        }
    },

    _deleteCallback: function(node)
    {
        if (!node || node.isCreationNode)
            return;

        if (this.domStorage)
            this.domStorage.removeItem(node.data.key);
    },

    __proto__: WebInspector.View.prototype
}
