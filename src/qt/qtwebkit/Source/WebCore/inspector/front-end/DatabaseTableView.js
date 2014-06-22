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
 * @extends {WebInspector.View}
 */
WebInspector.DatabaseTableView = function(database, tableName)
{
    WebInspector.View.call(this);

    this.database = database;
    this.tableName = tableName;

    this.element.addStyleClass("storage-view");
    this.element.addStyleClass("table");

    this.refreshButton = new WebInspector.StatusBarButton(WebInspector.UIString("Refresh"), "refresh-storage-status-bar-item");
    this.refreshButton.addEventListener("click", this._refreshButtonClicked, this);
}

WebInspector.DatabaseTableView.prototype = {
    wasShown: function()
    {
        this.update();
    },

    statusBarItems: function()
    {
        return [this.refreshButton.element];
    },

    /**
     * @param {string} tableName
     * @return {string}
     */
    _escapeTableName: function(tableName)
    {
        return tableName.replace(/\"/g, "\"\"");
    },
    
    update: function()
    {
        this.database.executeSql("SELECT * FROM \"" + this._escapeTableName(this.tableName) + "\"", this._queryFinished.bind(this), this._queryError.bind(this));
    },

    _queryFinished: function(columnNames, values)
    {
        this.detachChildViews();
        this.element.removeChildren();

        var dataGrid = WebInspector.DataGrid.createSortableDataGrid(columnNames, values);
        if (!dataGrid) {
            this._emptyView = new WebInspector.EmptyView(WebInspector.UIString("The “%s”\ntable is empty.", this.tableName));
            this._emptyView.show(this.element);
            return;
        }
        dataGrid.show(this.element);
        dataGrid.autoSizeColumns(5);
    },

    _queryError: function(error)
    {
        this.detachChildViews();
        this.element.removeChildren();

        var errorMsgElement = document.createElement("div");
        errorMsgElement.className = "storage-table-error";
        errorMsgElement.textContent = WebInspector.UIString("An error occurred trying to\nread the “%s” table.", this.tableName);
        this.element.appendChild(errorMsgElement);
    },

    _refreshButtonClicked: function(event)
    {
        this.update();
    },

    __proto__: WebInspector.View.prototype
}
