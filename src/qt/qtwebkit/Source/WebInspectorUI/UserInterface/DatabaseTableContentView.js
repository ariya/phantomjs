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

WebInspector.DatabaseTableContentView = function(representedObject)
{
    WebInspector.ContentView.call(this, representedObject);
    
    this.element.classList.add(WebInspector.DatabaseTableContentView.StyleClassName);
    
    this.update();
};

WebInspector.DatabaseTableContentView.StyleClassName = "database-table";

WebInspector.DatabaseTableContentView.prototype = {
    constructor: WebInspector.DatabaseTableContentView,

    // Public

    update: function()
    {
        this.representedObject.database.executeSQL("SELECT * FROM \"" + this._escapeTableName(this.representedObject.name) + "\"", this._queryFinished.bind(this), this._queryError.bind(this));
    },

    updateLayout: function()
    {
        if (this._dataGrid)
            this._dataGrid.updateLayout();
    },

    get scrollableElements()
    {
        if (!this._dataGrid)
            return [];
        return [this._dataGrid.scrollContainer];
    },

    // Private

    _escapeTableName: function(name)
    {
        return name.replace(/\"/g, "\"\"");
    },

    _queryFinished: function(columnNames, values)
    {
        // It would be nice to do better than creating a new data grid each time the table is updated, but the table updating
        // doesn't happen very frequently. Additionally, using DataGrid's createSortableDataGrid makes our code much cleaner and it knows
        // how to sort arbitrary columns.
        this.element.removeChildren();

        this._dataGrid = new WebInspector.DataGrid.createSortableDataGrid(columnNames, values);
        if (!this._dataGrid || !this._dataGrid.element) {
            // If the DataGrid is empty, then we were returned a table with no columns. This can happen when a table has
            // no data, the SELECT query only returns column names when there is data.
            this.element.removeChildren();
            this.element.appendChild(WebInspector.createMessageTextView(WebInspector.UIString("The “%s”\ntable is empty.").format(this.representedObject.name), false));
            return;
        }

        this.element.appendChild(this._dataGrid.element);
        this._dataGrid.updateLayout();
    },

    _queryError: function(error)
    {
        this.element.removeChildren();
        this.element.appendChild(WebInspector.createMessageTextView(WebInspector.UIString("An error occured trying to\nread the “%s” table.").format(this.representedObject.name), true));
    }
};

WebInspector.DatabaseTableContentView.prototype.__proto__ = WebInspector.ContentView.prototype;
