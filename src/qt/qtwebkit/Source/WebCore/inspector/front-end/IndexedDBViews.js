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
 * @extends {WebInspector.View}
 * @param {WebInspector.IndexedDBModel.Database} database
 */
WebInspector.IDBDatabaseView = function(database)
{
    WebInspector.View.call(this);
    this.registerRequiredCSS("indexedDBViews.css");

    this.element.addStyleClass("fill");
    this.element.addStyleClass("indexed-db-database-view");

    this._headersListElement = this.element.createChild("ol", "outline-disclosure");
    this._headersTreeOutline = new TreeOutline(this._headersListElement);
    this._headersTreeOutline.expandTreeElementsWhenArrowing = true;

    this._securityOriginTreeElement = new TreeElement("", null, false);
    this._securityOriginTreeElement.selectable = false;
    this._headersTreeOutline.appendChild(this._securityOriginTreeElement);

    this._nameTreeElement = new TreeElement("", null, false);
    this._nameTreeElement.selectable = false;
    this._headersTreeOutline.appendChild(this._nameTreeElement);

    this._intVersionTreeElement = new TreeElement("", null, false);
    this._intVersionTreeElement.selectable = false;
    this._headersTreeOutline.appendChild(this._intVersionTreeElement);

    this._stringVersionTreeElement = new TreeElement("", null, false);
    this._stringVersionTreeElement.selectable = false;
    this._headersTreeOutline.appendChild(this._stringVersionTreeElement);

    this.update(database);
}

WebInspector.IDBDatabaseView.prototype = {
    /**
     * @param {string} name
     * @param {string} value
     */
    _formatHeader: function(name, value)
    {
        var fragment = document.createDocumentFragment();
        fragment.createChild("div", "attribute-name").textContent = name + ":";
        fragment.createChild("div", "attribute-value source-code").textContent = value;

        return fragment;
    },

    _refreshDatabase: function()
    {
        this._securityOriginTreeElement.title = this._formatHeader(WebInspector.UIString("Security origin"), this._database.databaseId.securityOrigin);
        this._nameTreeElement.title = this._formatHeader(WebInspector.UIString("Name"), this._database.databaseId.name);
        this._stringVersionTreeElement.title = this._formatHeader(WebInspector.UIString("String Version"), this._database.version);
        this._intVersionTreeElement.title = this._formatHeader(WebInspector.UIString("Integer Version"), this._database.intVersion);
    },

    /**
     * @param {WebInspector.IndexedDBModel.Database} database
     */
    update: function(database)
    {
        this._database = database;
        this._refreshDatabase();
    },

    __proto__: WebInspector.View.prototype
}


/**
 * @constructor
 * @extends {WebInspector.View}
 * @param {WebInspector.IndexedDBModel} model
 * @param {WebInspector.IndexedDBModel.DatabaseId} databaseId
 * @param {WebInspector.IndexedDBModel.ObjectStore} objectStore
 * @param {WebInspector.IndexedDBModel.Index} index
 */
WebInspector.IDBDataView = function(model, databaseId, objectStore, index)
{
    WebInspector.View.call(this);
    this.registerRequiredCSS("indexedDBViews.css");

    this._model = model;
    this._databaseId = databaseId;
    this._isIndex = !!index;

    this.element.addStyleClass("indexed-db-data-view");

    var editorToolbar = this._createEditorToolbar();
    this.element.appendChild(editorToolbar);

    this._dataGridContainer = this.element.createChild("div", "fill");
    this._dataGridContainer.addStyleClass("data-grid-container");

    this._refreshButton = new WebInspector.StatusBarButton(WebInspector.UIString("Refresh"), "refresh-storage-status-bar-item");
    this._refreshButton.addEventListener("click", this._refreshButtonClicked, this);

    this._clearButton = new WebInspector.StatusBarButton(WebInspector.UIString("Clear object store"), "clear-storage-status-bar-item");
    this._clearButton.addEventListener("click", this._clearButtonClicked, this);

    this._pageSize = 50;
    this._skipCount = 0;

    this.update(objectStore, index);
    this._entries = [];
}

WebInspector.IDBDataView.prototype = {
    /**
     * @return {WebInspector.DataGrid}
     */
    _createDataGrid: function()
    {
        var keyPath = this._isIndex ? this._index.keyPath : this._objectStore.keyPath;

        var columns = [];
        columns.push({id: "number", title: WebInspector.UIString("#"), width: "50px"});
        columns.push({id: "key", titleDOMFragment: this._keyColumnHeaderFragment(WebInspector.UIString("Key"), keyPath)});
        if (this._isIndex)
            columns.push({id: "primaryKey", titleDOMFragment: this._keyColumnHeaderFragment(WebInspector.UIString("Primary key"), this._objectStore.keyPath)});
        columns.push({id: "value", title: WebInspector.UIString("Value")});

        var dataGrid = new WebInspector.DataGrid(columns);
        return dataGrid;
    },

    /**
     * @param {string} prefix
     * @param {*} keyPath
     * @return {DocumentFragment}
     */
    _keyColumnHeaderFragment: function(prefix, keyPath)
    {
        var keyColumnHeaderFragment = document.createDocumentFragment();
        keyColumnHeaderFragment.appendChild(document.createTextNode(prefix));
        if (keyPath === null)
            return keyColumnHeaderFragment;

        keyColumnHeaderFragment.appendChild(document.createTextNode(" (" + WebInspector.UIString("Key path: ")));
        if (keyPath instanceof Array) {
            keyColumnHeaderFragment.appendChild(document.createTextNode("["));
            for (var i = 0; i < keyPath.length; ++i) {
                if (i != 0)
                    keyColumnHeaderFragment.appendChild(document.createTextNode(", "));
                keyColumnHeaderFragment.appendChild(this._keyPathStringFragment(keyPath[i]));
            }
            keyColumnHeaderFragment.appendChild(document.createTextNode("]"));
        } else {
            var keyPathString = /** @type {string} */ (keyPath);
            keyColumnHeaderFragment.appendChild(this._keyPathStringFragment(keyPathString));
        }
        keyColumnHeaderFragment.appendChild(document.createTextNode(")"));
        return keyColumnHeaderFragment;
    },

    /**
     * @param {string} keyPathString
     * @return {DocumentFragment}
     */
    _keyPathStringFragment: function(keyPathString)
    {
        var keyPathStringFragment = document.createDocumentFragment();
        keyPathStringFragment.appendChild(document.createTextNode("\""));
        var keyPathSpan = keyPathStringFragment.createChild("span", "source-code console-formatted-string");
        keyPathSpan.textContent = keyPathString;
        keyPathStringFragment.appendChild(document.createTextNode("\""));
        return keyPathStringFragment;
    },

    /**
     * @return {Element}
     */
    _createEditorToolbar: function()
    {
        var editorToolbar = document.createElement("div");
        editorToolbar.addStyleClass("status-bar");
        editorToolbar.addStyleClass("data-view-toolbar");

        this._pageBackButton = editorToolbar.createChild("button", "back-button");
        this._pageBackButton.addStyleClass("status-bar-item");
        this._pageBackButton.title = WebInspector.UIString("Show previous page.");
        this._pageBackButton.disabled = true;
        this._pageBackButton.appendChild(document.createElement("img"));
        this._pageBackButton.addEventListener("click", this._pageBackButtonClicked.bind(this), false);
        editorToolbar.appendChild(this._pageBackButton);

        this._pageForwardButton = editorToolbar.createChild("button", "forward-button");
        this._pageForwardButton.addStyleClass("status-bar-item");
        this._pageForwardButton.title = WebInspector.UIString("Show next page.");
        this._pageForwardButton.disabled = true;
        this._pageForwardButton.appendChild(document.createElement("img"));
        this._pageForwardButton.addEventListener("click", this._pageForwardButtonClicked.bind(this), false);
        editorToolbar.appendChild(this._pageForwardButton);

        this._keyInputElement = editorToolbar.createChild("input", "key-input");
        this._keyInputElement.placeholder = WebInspector.UIString("Start from key");
        this._keyInputElement.addEventListener("paste", this._keyInputChanged.bind(this));
        this._keyInputElement.addEventListener("cut", this._keyInputChanged.bind(this));
        this._keyInputElement.addEventListener("keypress", this._keyInputChanged.bind(this));
        this._keyInputElement.addEventListener("keydown", this._keyInputChanged.bind(this));

        return editorToolbar;
    },

    _pageBackButtonClicked: function()
    {
        this._skipCount = Math.max(0, this._skipCount - this._pageSize);
        this._updateData(false);
    },

    _pageForwardButtonClicked: function()
    {
        this._skipCount = this._skipCount + this._pageSize;
        this._updateData(false);
    },

    _keyInputChanged: function()
    {
        window.setTimeout(this._updateData.bind(this, false), 0);
    },

    /**
     * @param {WebInspector.IndexedDBModel.ObjectStore} objectStore
     * @param {WebInspector.IndexedDBModel.Index} index
     */
    update: function(objectStore, index)
    {
        this._objectStore = objectStore;
        this._index = index;

        if (this._dataGrid)
            this._dataGrid.detach();
        this._dataGrid = this._createDataGrid();
        this._dataGrid.show(this._dataGridContainer);

        this._skipCount = 0;
        this._updateData(true);
    },

    /**
     * @param {string} keyString
     */
    _parseKey: function(keyString)
    {
        var result;
        try {
            result = JSON.parse(keyString);
        } catch (e) {
            result = keyString;
        }
        return result;
    },

    /**
     * @return {string}
     */
    _stringifyKey: function(key)
    {
        if (typeof(key) === "string")
            return key;
        return JSON.stringify(key);
    },

    /**
     * @param {boolean} force
     */
    _updateData: function(force)
    {
        var key = this._parseKey(this._keyInputElement.value);
        var pageSize = this._pageSize;
        var skipCount = this._skipCount;
        this._refreshButton.setEnabled(false);
        this._clearButton.setEnabled(!this._isIndex);

        if (!force && this._lastKey === key && this._lastPageSize === pageSize && this._lastSkipCount === skipCount)
            return;

        if (this._lastKey !== key || this._lastPageSize !== pageSize) {
            skipCount = 0;
            this._skipCount = 0;
        }
        this._lastKey = key;
        this._lastPageSize = pageSize;
        this._lastSkipCount = skipCount;

        /**
         * @param {Array.<WebInspector.IndexedDBModel.Entry>} entries
         * @param {boolean} hasMore
         */
        function callback(entries, hasMore)
        {
            this._refreshButton.setEnabled(true);
            this.clear();
            this._entries = entries;
            for (var i = 0; i < entries.length; ++i) {
                var data = {};
                data["number"] = i + skipCount;
                data["key"] = entries[i].key;
                data["primaryKey"] = entries[i].primaryKey;
                data["value"] = entries[i].value;

                var primaryKey = JSON.stringify(this._isIndex ? entries[i].primaryKey : entries[i].key);
                var node = new WebInspector.IDBDataGridNode(data);
                this._dataGrid.rootNode().appendChild(node);
            }

            this._pageBackButton.disabled = skipCount === 0;
            this._pageForwardButton.disabled = !hasMore;
        }

        var idbKeyRange = key ? window.webkitIDBKeyRange.lowerBound(key) : null;
        if (this._isIndex)
            this._model.loadIndexData(this._databaseId, this._objectStore.name, this._index.name, idbKeyRange, skipCount, pageSize, callback.bind(this));
        else
            this._model.loadObjectStoreData(this._databaseId, this._objectStore.name, idbKeyRange, skipCount, pageSize, callback.bind(this));
    },

    _refreshButtonClicked: function(event)
    {
        this._updateData(true);
    },

    _clearButtonClicked: function(event)
    {
        function cleared() {
            this._clearButton.setEnabled(true);
            this._updateData(true);
        }
        this._clearButton.setEnabled(false);
        this._model.clearObjectStore(this._databaseId, this._objectStore.name, cleared.bind(this));
    },

    statusBarItems: function()
    {
        return [this._refreshButton.element, this._clearButton.element];
    },

    clear: function()
    {
        this._dataGrid.rootNode().removeChildren();
        for (var i = 0; i < this._entries.length; ++i) {
            this._entries[i].key.release();
            this._entries[i].primaryKey.release();
            this._entries[i].value.release();
        }
        this._entries = [];
    },

    __proto__: WebInspector.View.prototype
}

/**
 * @constructor
 * @extends {WebInspector.DataGridNode}
 * @param {*} data
 */
WebInspector.IDBDataGridNode = function(data)
{
    WebInspector.DataGridNode.call(this, data, false);
    this.selectable = false;
}

WebInspector.IDBDataGridNode.prototype = {
    /**
     * @return {Element}
     */
    createCell: function(columnIdentifier)
    {
        var cell = WebInspector.DataGridNode.prototype.createCell.call(this, columnIdentifier);
        var value = this.data[columnIdentifier];

        switch (columnIdentifier) {
        case "value":
        case "key":
        case "primaryKey":
            cell.removeChildren();
            this._formatValue(cell, value);
            break;
        default:
        }

        return cell;
    },

    _formatValue: function(cell, value)
    {
        var type = value.subtype || value.type;
        var contents = cell.createChild("div", "source-code console-formatted-" + type);

        switch (type) {
        case "object":
        case "array":
            var section = new WebInspector.ObjectPropertiesSection(value, value.description)
            section.editable = false;
            section.skipProto = true;
            contents.appendChild(section.element);
            break;
        case "string":
            contents.addStyleClass("primitive-value");
            contents.appendChild(document.createTextNode("\"" + value.description + "\""));
            break;
        default:
            contents.addStyleClass("primitive-value");
            contents.appendChild(document.createTextNode(value.description));
        }
    },

    __proto__: WebInspector.DataGridNode.prototype
}

