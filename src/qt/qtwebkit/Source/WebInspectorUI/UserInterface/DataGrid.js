/*
 * Copyright (C) 2008, 2013 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.         IN NO EVENT SHALL APPLE INC. OR
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
 * @extends {WebInspector.Object}
 * @param {function(WebInspector.DataGridNode, number, string, string)=} editCallback
 * @param {function(WebInspector.DataGridNode)=} deleteCallback
 */
WebInspector.DataGrid = function(columns, editCallback, deleteCallback)
{
    this.element = document.createElement("div");
    this.element.className = "data-grid";
    this.element.tabIndex = 0;
    this.element.addEventListener("keydown", this._keyDown.bind(this), false);
    this.element.copyHandler = this;

    this._headerTable = document.createElement("table");
    this._headerTable.className = "header";
    this._headerTableHeaders = {};

    this._dataTable = document.createElement("table");
    this._dataTable.className = "data";

    this._dataTable.addEventListener("mousedown", this._mouseDownInDataTable.bind(this));
    this._dataTable.addEventListener("click", this._clickInDataTable.bind(this));

    this._dataTable.addEventListener("contextmenu", this._contextMenuInDataTable.bind(this), true);

    // FIXME: Add a createCallback which is different from editCallback and has different
    // behavior when creating a new node.
    if (editCallback) {
        this._dataTable.addEventListener("dblclick", this._ondblclick.bind(this), false);
        this._editCallback = editCallback;
    }
    if (deleteCallback)
        this._deleteCallback = deleteCallback;

    this.aligned = {};
    this.groups = {};
    this._hiddenColumns = {};

    this._scrollContainer = document.createElement("div");
    this._scrollContainer.className = "data-container";
    this._scrollContainer.appendChild(this._dataTable);

    this.element.appendChild(this._headerTable);
    this.element.appendChild(this._scrollContainer);

    var headerRow = document.createElement("tr");
    var columnGroup = document.createElement("colgroup");
    this._columnCount = 0;

    for (var columnIdentifier in columns) {
        var column = columns[columnIdentifier];
        if (column.disclosure)
            this.disclosureColumnIdentifier = columnIdentifier;

        var col = document.createElement("col");
        if (column.width)
            col.style.width = column.width;
        column.element = col;
        columnGroup.appendChild(col);

        var cell = document.createElement("th");
        cell.className = columnIdentifier + "-column";
        cell.columnIdentifier = columnIdentifier;
        this._headerTableHeaders[columnIdentifier] = cell;

        var div = document.createElement("div");
        if (column.titleDOMFragment)
            div.appendChild(column.titleDOMFragment);
        else
            div.textContent = column.title;
        cell.appendChild(div);

        if (column.sort) {
            cell.classList.add("sort-" + column.sort);
            this._sortColumnCell = cell;
        }

        if (column.sortable) {
            cell.addEventListener("click", this._clickInHeaderCell.bind(this), false);
            cell.classList.add("sortable");
        }

        if (column.aligned)
            this.aligned[columnIdentifier] = column.aligned;

        if (column.group) {
            this.groups[columnIdentifier] = column.group;
            cell.classList.add("column-group-" + column.group);
        }

        if (column.collapsesGroup) {
            console.assert(column.group !== column.collapsesGroup);

            var divider = document.createElement("div");
            divider.className = "divider";
            cell.appendChild(divider);

            var collapseDiv = document.createElement("div");
            collapseDiv.className = "collapser-button";
            collapseDiv.title = this._collapserButtonCollapseColumnsToolTip();
            collapseDiv.addEventListener("mouseover", this._mouseoverColumnCollapser.bind(this));
            collapseDiv.addEventListener("mouseout", this._mouseoutColumnCollapser.bind(this));
            collapseDiv.addEventListener("click", this._clickInColumnCollapser.bind(this));
            cell.appendChild(collapseDiv);

            cell.collapsesGroup = column.collapsesGroup;
            cell.classList.add("collapser");
        }

        headerRow.appendChild(cell);

        ++this._columnCount;
    }

    columnGroup.span = this._columnCount;

    this._headerTableColumnGroup = columnGroup;
    this._headerTable.appendChild(this._headerTableColumnGroup);
    this.headerTableBody.appendChild(headerRow);

    var fillerRow = document.createElement("tr");
    fillerRow.className = "filler";

    for (var columnIdentifier in columns) {
        var column = columns[columnIdentifier];
        var td = document.createElement("td");
        td.className = columnIdentifier + "-column";
        var group = this.groups[columnIdentifier];
        if (group)
            td.classList.add("column-group-" + group);
        fillerRow.appendChild(td);
    }

    this._dataTableColumnGroup = columnGroup.cloneNode(true);
    this._dataTable.appendChild(this._dataTableColumnGroup);
    this.dataTableBody.appendChild(fillerRow);

    this.columns = columns || {};
    this._columnsArray = [];
    for (var columnIdentifier in columns) {
        columns[columnIdentifier].ordinal = this._columnsArray.length;
        columns[columnIdentifier].identifier = columnIdentifier;
        this._columnsArray.push(columns[columnIdentifier]);
    }

    for (var i = 0; i < this._columnsArray.length; ++i)
        this._columnsArray[i].bodyElement = this._dataTableColumnGroup.children[i];

    this.children = [];
    this.selectedNode = null;
    this.expandNodesWhenArrowing = false;
    this.root = true;
    this.hasChildren = false;
    this.expanded = true;
    this.revealed = true;
    this.selected = false;
    this.dataGrid = this;
    this.indentWidth = 15;
    this.resizers = [];
    this._columnWidthsInitialized = false;

    this._generateSortIndicatorImagesIfNeeded();
}

WebInspector.DataGrid.Event = {
    DidLayout: "datagrid-did-layout",
    SortChanged: "datagrid-sort-changed",
    SelectedNodeChanged: "datagrid-selected-node-changed"
}

/**
 * @param {Array.<string>} columnNames
 * @param {Array.<string>} values
 */
WebInspector.DataGrid.createSortableDataGrid = function(columnNames, values)
{
    var numColumns = columnNames.length;
    if (!numColumns)
        return null;

    var columns = {};

    for (var i = 0; i < columnNames.length; ++i) {
        var column = {};
        column.width = columnNames[i].length;
        column.title = columnNames[i];
        column.sortable = true;

        columns[columnNames[i]] = column;
    }

    var nodes = [];
    for (var i = 0; i < values.length / numColumns; ++i) {
        var data = {};
        for (var j = 0; j < columnNames.length; ++j)
            data[columnNames[j]] = values[numColumns * i + j];

        var node = new WebInspector.DataGridNode(data, false);
        node.selectable = false;
        nodes.push(node);
    }

    var dataGrid = new WebInspector.DataGrid(columns);
    var length = nodes.length;
    for (var i = 0; i < length; ++i)
        dataGrid.appendChild(nodes[i]);

    dataGrid.addEventListener(WebInspector.DataGrid.Event.SortChanged, sortDataGrid, this);

    function sortDataGrid()
    {
        var nodes = dataGrid.children.slice();
        var sortColumnIdentifier = dataGrid.sortColumnIdentifier;
        var sortDirection = dataGrid.sortOrder === "ascending" ? 1 : -1;
        var columnIsNumeric = true;

        for (var i = 0; i < nodes.length; i++) {
            if (isNaN(Number(nodes[i].data[sortColumnIdentifier])))
                columnIsNumeric = false;
        }

        function comparator(dataGridNode1, dataGridNode2)
        {
            var item1 = dataGridNode1.data[sortColumnIdentifier];
            var item2 = dataGridNode2.data[sortColumnIdentifier];

            var comparison;
            if (columnIsNumeric) {
                // Sort numbers based on comparing their values rather than a lexicographical comparison.
                var number1 = parseFloat(item1);
                var number2 = parseFloat(item2);
                comparison = number1 < number2 ? -1 : (number1 > number2 ? 1 : 0);
            } else
                comparison = item1 < item2 ? -1 : (item1 > item2 ? 1 : 0);

            return sortDirection * comparison;
        }

        nodes.sort(comparator);
        dataGrid.removeChildren();
        for (var i = 0; i < nodes.length; i++)
            dataGrid.appendChild(nodes[i]);
    }
    return dataGrid;
}

WebInspector.DataGrid.prototype = {
    get refreshCallback()
    {
        return this._refreshCallback;
    },

    set refreshCallback(refreshCallback)
    {
        this._refreshCallback = refreshCallback;
    },

    _ondblclick: function(event)
    {
        if (this._editing || this._editingNode)
            return;

        this._startEditing(event.target);
    },

    _startEditingColumnOfDataGridNode: function(node, column)
    {
        this._editing = true;
        this._editingNode = node;
        this._editingNode.select();

        var element = this._editingNode._element.children[column];
        WebInspector.startEditing(element, this._startEditingConfig(element));
        window.getSelection().setBaseAndExtent(element, 0, element, 1);
    },

    _startEditing: function(target)
    {
        var element = target.enclosingNodeOrSelfWithNodeName("td");
        if (!element)
            return;

        this._editingNode = this.dataGridNodeFromNode(target);
        if (!this._editingNode) {
            if (!this.creationNode)
                return;
            this._editingNode = this.creationNode;
        }

        // Force editing the 1st column when editing the creation node
        if (this._editingNode.isCreationNode)
            return this._startEditingColumnOfDataGridNode(this._editingNode, 0);

        this._editing = true;
        WebInspector.startEditing(element, this._startEditingConfig(element));

        window.getSelection().setBaseAndExtent(element, 0, element, 1);
    },

    _startEditingConfig: function(element)
    {
        return new WebInspector.EditingConfig(this._editingCommitted.bind(this), this._editingCancelled.bind(this), element.textContent);
    },

    _editingCommitted: function(element, newText, oldText, context, moveDirection)
    {
        // FIXME: We need more column identifiers here throughout this function.
        // Not needed yet since only editable DataGrid is DOM Storage, which is Key - Value.

        // FIXME: Better way to do this than regular expressions?
        var columnIdentifier = parseInt(element.className.match(/\b(\d+)-column\b/)[1], 10);

        var textBeforeEditing = this._editingNode.data[columnIdentifier];
        var currentEditingNode = this._editingNode;

        function moveToNextIfNeeded(wasChange) {
            if (!moveDirection)
                return;

            if (moveDirection === "forward") {
                if (currentEditingNode.isCreationNode && columnIdentifier === 0 && !wasChange)
                    return;

                if (columnIdentifier === 0)
                    return this._startEditingColumnOfDataGridNode(currentEditingNode, 1);

                var nextDataGridNode = currentEditingNode.traverseNextNode(true, null, true);
                if (nextDataGridNode)
                    return this._startEditingColumnOfDataGridNode(nextDataGridNode, 0);
                if (currentEditingNode.isCreationNode && wasChange) {
                    this.addCreationNode(false);
                    return this._startEditingColumnOfDataGridNode(this.creationNode, 0);
                }
                return;
            }

            if (moveDirection === "backward") {
                if (columnIdentifier === 1)
                    return this._startEditingColumnOfDataGridNode(currentEditingNode, 0);
                    var nextDataGridNode = currentEditingNode.traversePreviousNode(true, null, true);

                if (nextDataGridNode)
                    return this._startEditingColumnOfDataGridNode(nextDataGridNode, 1);
                return;
            }
        }

        if (textBeforeEditing == newText) {
            this._editingCancelled(element);
            moveToNextIfNeeded.call(this, false);
            return;
        }

        // Update the text in the datagrid that we typed
        this._editingNode.data[columnIdentifier] = newText;

        // Make the callback - expects an editing node (table row), the column number that is being edited,
        // the text that used to be there, and the new text.
        this._editCallback(this._editingNode, columnIdentifier, textBeforeEditing, newText);

        if (this._editingNode.isCreationNode)
            this.addCreationNode(false);

        this._editingCancelled(element);
        moveToNextIfNeeded.call(this, true);
    },

    _editingCancelled: function(element)
    {
        delete this._editing;
        this._editingNode = null;
    },

    get sortColumnIdentifier()
    {
        if (!this._sortColumnCell)
            return null;
        return this._sortColumnCell.columnIdentifier;
    },

    get sortOrder()
    {
        if (!this._sortColumnCell || this._sortColumnCell.classList.contains("sort-ascending"))
            return "ascending";
        if (this._sortColumnCell.classList.contains("sort-descending"))
            return "descending";
        return null;
    },

    get headerTableBody()
    {
        if ("_headerTableBody" in this)
            return this._headerTableBody;

        this._headerTableBody = this._headerTable.getElementsByTagName("tbody")[0];
        if (!this._headerTableBody) {
            this._headerTableBody = this.element.ownerDocument.createElement("tbody");
            this._headerTable.insertBefore(this._headerTableBody, this._headerTable.tFoot);
        }

        return this._headerTableBody;
    },

    get dataTableBody()
    {
        if ("_dataTableBody" in this)
            return this._dataTableBody;

        this._dataTableBody = this._dataTable.getElementsByTagName("tbody")[0];
        if (!this._dataTableBody) {
            this._dataTableBody = this.element.ownerDocument.createElement("tbody");
            this._dataTable.insertBefore(this._dataTableBody, this._dataTable.tFoot);
        }

        return this._dataTableBody;
    },

    /**
     * @param {number=} maxDescentLevel
     */
    autoSizeColumns: function(minPercent, maxPercent, maxDescentLevel)
    {
        if (minPercent)
            minPercent = Math.min(minPercent, Math.floor(100 / this._columnCount));
        var widths = {};
        var columns = this.columns;
        for (var columnIdentifier in columns)
            widths[columnIdentifier] = (columns[columnIdentifier].title || "").length;

        var children = maxDescentLevel ? this._enumerateChildren(this, [], maxDescentLevel + 1) : this.children;
        for (var i = 0; i < children.length; ++i) {
            var node = children[i];
            for (var columnIdentifier in columns) {
                var text = node.data[columnIdentifier] || "";
                if (text.length > widths[columnIdentifier])
                    widths[columnIdentifier] = text.length;
            }
        }

        var totalColumnWidths = 0;
        for (var columnIdentifier in columns)
            totalColumnWidths += widths[columnIdentifier];

        var recoupPercent = 0;
        for (var columnIdentifier in columns) {
            var width = Math.round(100 * widths[columnIdentifier] / totalColumnWidths);
            if (minPercent && width < minPercent) {
                recoupPercent += (minPercent - width);
                width = minPercent;
            } else if (maxPercent && width > maxPercent) {
                recoupPercent -= (width - maxPercent);
                width = maxPercent;
            }
            widths[columnIdentifier] = width;
        }

        while (minPercent && recoupPercent > 0) {
            for (var columnIdentifier in columns) {
                if (widths[columnIdentifier] > minPercent) {
                    --widths[columnIdentifier];
                    --recoupPercent;
                    if (!recoupPercent)
                        break;
                }
            }
        }

        while (maxPercent && recoupPercent < 0) {
            for (var columnIdentifier in columns) {
                if (widths[columnIdentifier] < maxPercent) {
                    ++widths[columnIdentifier];
                    ++recoupPercent;
                    if (!recoupPercent)
                        break;
                }
            }
        }

        for (var columnIdentifier in columns)
            columns[columnIdentifier].element.style.width = widths[columnIdentifier] + "%";
        this._columnWidthsInitialized = false;
        this.updateLayout();
    },

    _enumerateChildren: function(rootNode, result, maxLevel)
    {
        if (!rootNode.root)
            result.push(rootNode);
        if (!maxLevel)
            return;
        for (var i = 0; i < rootNode.children.length; ++i)
            this._enumerateChildren(rootNode.children[i], result, maxLevel - 1);
        return result;
    },

    // Updates the widths of the table, including the positions of the column
    // resizers.
    //
    // IMPORTANT: This function MUST be called once after the element of the
    // DataGrid is attached to its parent element and every subsequent time the
    // width of the parent element is changed in order to make it possible to
    // resize the columns.
    //
    // If this function is not called after the DataGrid is attached to its
    // parent element, then the DataGrid's columns will not be resizable.
    updateLayout: function()
    {
        // Do not attempt to use offsetes if we're not attached to the document tree yet.
        if (!this._columnWidthsInitialized && this.element.offsetWidth) {
            // Give all the columns initial widths now so that during a resize,
            // when the two columns that get resized get a percent value for
            // their widths, all the other columns already have percent values
            // for their widths.
            var headerTableColumns = this._headerTableColumnGroup.children;
            var tableWidth = this._dataTable.offsetWidth;
            var numColumns = headerTableColumns.length;
            for (var i = 0; i < numColumns; i++) {
                var columnWidth = this.headerTableBody.rows[0].cells[i].offsetWidth;
                var percentWidth = ((columnWidth / tableWidth) * 100) + "%";
                this._headerTableColumnGroup.children[i].style.width = percentWidth;
                this._dataTableColumnGroup.children[i].style.width = percentWidth;
            }
            this._columnWidthsInitialized = true;
        }
        this._positionResizers();
        this.dispatchEventToListeners(WebInspector.DataGrid.Event.DidLayout);
    },

    columnWidthsMap: function()
    {
        var result = {};
        for (var columnIdentifier in this.columns) {
            var column = this.columns[columnIdentifier];
            var width = this._headerTableColumnGroup.children[column.ordinal].style.width;
            result[columnIdentifier] = parseFloat(width);
        }
        return result;
    },

    applyColumnWidthsMap: function(columnWidthsMap)
    {
        for (var columnIdentifier in this.columns) {
            var column = this.columns[columnIdentifier];
            var width = (columnWidthsMap[columnIdentifier] || 0) + "%";
            this._headerTableColumnGroup.children[column.ordinal].style.width = width;
            this._dataTableColumnGroup.children[column.ordinal].style.width = width;
        }

        this.updateLayout();
    },

    _isColumnVisible: function(columnIdentifier)
    {
        return !(columnIdentifier in this._hiddenColumns);
    },

    _showColumn: function(columnIdentifier)
    {
        delete this._hiddenColumns[columnIdentifier];
    },

    _hideColumn: function(columnIdentifier)
    {
        this._hiddenColumns[columnIdentifier] = true;

        var column = this.columns[columnIdentifier];
        var columnElement = column.element;
        columnElement.style.width = 0;

        var columnBodyElement = column.bodyElement;
        columnBodyElement.style.width = 0;

        this._columnWidthsInitialized = false;
    },

    get scrollContainer()
    {
        return this._scrollContainer;
    },

    isScrolledToLastRow: function()
    {
        return this._scrollContainer.isScrolledToBottom();
    },

    scrollToLastRow: function()
    {
        this._scrollContainer.scrollTop = this._scrollContainer.scrollHeight - this._scrollContainer.offsetHeight;
    },

    _positionResizers: function()
    {
        var headerTableColumns = this._headerTableColumnGroup.children;
        var numColumns = headerTableColumns.length;
        var left = 0;
        var previousResizer = null;

        // Make n - 1 resizers for n columns.
        for (var i = 0; i < numColumns - 1; i++) {
            var resizer = this.resizers[i];

            if (!resizer) {
                // This is the first call to updateWidth, so the resizers need
                // to be created.
                resizer = document.createElement("div");
                resizer.classList.add("data-grid-resizer");
                // This resizer is associated with the column to its right.
                resizer.addEventListener("mousedown", this._startResizerDragging.bind(this), false);
                this.element.appendChild(resizer);
                this.resizers[i] = resizer;
            }

            // Get the width of the cell in the first (and only) row of the
            // header table in order to determine the width of the column, since
            // it is not possible to query a column for its width.
            left += this.headerTableBody.rows[0].cells[i].offsetWidth;

            var columnIsVisible = this._isColumnVisible(this._columnsArray[i].identifier);
            if (columnIsVisible) {
                resizer.style.removeProperty("display");
                resizer.style.left = left + "px";
                resizer.leftNeighboringColumnID = i;
                if (previousResizer)
                    previousResizer.rightNeighboringColumnID = i;
                previousResizer = resizer;
            } else {
                resizer.style.setProperty("display", "none");
                resizer.leftNeighboringColumnID = 0;
                resizer.rightNeighboringColumnID = 0;
            }
        }
        if (previousResizer)
            previousResizer.rightNeighboringColumnID = numColumns - 1;
    },

    addCreationNode: function(hasChildren)
    {
        if (this.creationNode)
            this.creationNode.makeNormal();

        var emptyData = {};
        for (var column in this.columns)
            emptyData[column] = '';
        this.creationNode = new WebInspector.CreationDataGridNode(emptyData, hasChildren);
        this.appendChild(this.creationNode);
    },

    appendChild: function(child)
    {
        this.insertChild(child, this.children.length);
    },

    insertChild: function(child, index)
    {
        if (!child)
            throw("insertChild: Node can't be undefined or null.");
        if (child.parent === this)
            throw("insertChild: Node is already a child of this node.");

        if (child.parent)
            child.parent.removeChild(child);

        this.children.splice(index, 0, child);
        this.hasChildren = true;

        child.parent = this;
        child.dataGrid = this.dataGrid;
        child._recalculateSiblings(index);

        delete child._depth;
        delete child._revealed;
        delete child._attached;
        child._shouldRefreshChildren = true;

        var current = child.children[0];
        while (current) {
            current.dataGrid = this.dataGrid;
            delete current._depth;
            delete current._revealed;
            delete current._attached;
            current._shouldRefreshChildren = true;
            current = current.traverseNextNode(false, child, true);
        }

        if (this.expanded)
            child._attach();
    },

    removeChild: function(child)
    {
        if (!child)
            throw("removeChild: Node can't be undefined or null.");
        if (child.parent !== this)
            throw("removeChild: Node is not a child of this node.");

        child.deselect();
        child._detach();

        this.children.remove(child, true);

        if (child.previousSibling)
            child.previousSibling.nextSibling = child.nextSibling;
        if (child.nextSibling)
            child.nextSibling.previousSibling = child.previousSibling;

        child.dataGrid = null;
        child.parent = null;
        child.nextSibling = null;
        child.previousSibling = null;

        if (this.children.length <= 0)
            this.hasChildren = false;

        if (this.creationNode === child)
            delete this.creationNode;
    },

    removeChildren: function()
    {
        for (var i = 0; i < this.children.length; ++i) {
            var child = this.children[i];
            child.deselect();
            child._detach();

            child.dataGrid = null;
            child.parent = null;
            child.nextSibling = null;
            child.previousSibling = null;
        }

        this.children = [];
        this.hasChildren = false;
    },

    removeChildrenRecursive: function()
    {
        var childrenToRemove = this.children;

        var child = this.children[0];
        while (child) {
            if (child.children.length)
                childrenToRemove = childrenToRemove.concat(child.children);
            child = child.traverseNextNode(false, this, true);
        }

        for (var i = 0; i < childrenToRemove.length; ++i) {
            child = childrenToRemove[i];
            child.deselect();
            child._detach();

            child.children = [];
            child.dataGrid = null;
            child.parent = null;
            child.nextSibling = null;
            child.previousSibling = null;
        }

        this.children = [];
    },

    sortNodes: function(comparator, reverseMode)
    {
        function comparatorWrapper(a, b)
        {
            if (a._dataGridNode._data.summaryRow)
                return 1;
            if (b._dataGridNode._data.summaryRow)
                return -1;

            var aDataGirdNode = a._dataGridNode;
            var bDataGirdNode = b._dataGridNode;
            return reverseMode ? comparator(bDataGirdNode, aDataGirdNode) : comparator(aDataGirdNode, bDataGirdNode);
        }

        var tbody = this.dataTableBody;
        var tbodyParent = tbody.parentElement;
        tbodyParent.removeChild(tbody);

        var childNodes = tbody.childNodes;
        var fillerRow = childNodes.lastValue;

        var sortedRows = Array.prototype.slice.call(childNodes, 0, childNodes.length - 1);
        sortedRows.sort(comparatorWrapper);
        var sortedRowsLength = sortedRows.length;

        tbody.removeChildren();
        var previousSiblingNode = null;
        for (var i = 0; i < sortedRowsLength; ++i) {
            var row = sortedRows[i];
            var node = row._dataGridNode;
            node.previousSibling = previousSiblingNode;
            if (previousSiblingNode)
                previousSiblingNode.nextSibling = node;
            tbody.appendChild(row);
            previousSiblingNode = node;
        }
        if (previousSiblingNode)
            previousSiblingNode.nextSibling = null;

        tbody.appendChild(fillerRow);
        tbodyParent.appendChild(tbody);
    },

    _keyDown: function(event)
    {
        if (!this.selectedNode || event.shiftKey || event.metaKey || event.ctrlKey || this._editing)
            return;

        var handled = false;
        var nextSelectedNode;
        if (event.keyIdentifier === "Up" && !event.altKey) {
            nextSelectedNode = this.selectedNode.traversePreviousNode(true);
            while (nextSelectedNode && !nextSelectedNode.selectable)
                nextSelectedNode = nextSelectedNode.traversePreviousNode(true);
            handled = nextSelectedNode ? true : false;
        } else if (event.keyIdentifier === "Down" && !event.altKey) {
            nextSelectedNode = this.selectedNode.traverseNextNode(true);
            while (nextSelectedNode && !nextSelectedNode.selectable)
                nextSelectedNode = nextSelectedNode.traverseNextNode(true);
            handled = nextSelectedNode ? true : false;
        } else if (event.keyIdentifier === "Left") {
            if (this.selectedNode.expanded) {
                if (event.altKey)
                    this.selectedNode.collapseRecursively();
                else
                    this.selectedNode.collapse();
                handled = true;
            } else if (this.selectedNode.parent && !this.selectedNode.parent.root) {
                handled = true;
                if (this.selectedNode.parent.selectable) {
                    nextSelectedNode = this.selectedNode.parent;
                    handled = nextSelectedNode ? true : false;
                } else if (this.selectedNode.parent)
                    this.selectedNode.parent.collapse();
            }
        } else if (event.keyIdentifier === "Right") {
            if (!this.selectedNode.revealed) {
                this.selectedNode.reveal();
                handled = true;
            } else if (this.selectedNode.hasChildren) {
                handled = true;
                if (this.selectedNode.expanded) {
                    nextSelectedNode = this.selectedNode.children[0];
                    handled = nextSelectedNode ? true : false;
                } else {
                    if (event.altKey)
                        this.selectedNode.expandRecursively();
                    else
                        this.selectedNode.expand();
                }
            }
        } else if (event.keyCode === 8 || event.keyCode === 46) {
            if (this._deleteCallback) {
                handled = true;
                this._deleteCallback(this.selectedNode);
            }
        } else if (isEnterKey(event)) {
            if (this._editCallback) {
                handled = true;
                // The first child of the selected element is the <td class="0-column">,
                // and that's what we want to edit.
                this._startEditing(this.selectedNode._element.children[0]);
            }
        }

        if (nextSelectedNode) {
            nextSelectedNode.reveal();
            nextSelectedNode.select();
        }

        if (handled) {
            event.preventDefault();
            event.stopPropagation();
        }
    },

    expand: function()
    {
        // This is the root, do nothing.
    },

    collapse: function()
    {
        // This is the root, do nothing.
    },

    reveal: function()
    {
        // This is the root, do nothing.
    },

    revealAndSelect: function()
    {
        // This is the root, do nothing.
    },

    dataGridNodeFromNode: function(target)
    {
        var rowElement = target.enclosingNodeOrSelfWithNodeName("tr");
        return rowElement && rowElement._dataGridNode;
    },

    dataGridNodeFromPoint: function(x, y)
    {
        var node = this._dataTable.ownerDocument.elementFromPoint(x, y);
        var rowElement = node.enclosingNodeOrSelfWithNodeName("tr");
        return rowElement && rowElement._dataGridNode;
    },

    _clickInHeaderCell: function(event)
    {
        var cell = event.target.enclosingNodeOrSelfWithNodeName("th");
        if (!cell || !cell.columnIdentifier || !cell.classList.contains("sortable"))
            return;

        var sortOrder = this.sortOrder;

        if (this._sortColumnCell)
            this._sortColumnCell.removeMatchingStyleClasses("sort-\\w+");

        if (cell == this._sortColumnCell) {
            if (sortOrder === "ascending")
                sortOrder = "descending";
            else
                sortOrder = "ascending";
        }

        this._sortColumnCell = cell;

        cell.classList.add("sort-" + sortOrder);
    
        this.dispatchEventToListeners(WebInspector.DataGrid.Event.SortChanged);
    },

    _mouseoverColumnCollapser: function(event)
    {
        var cell = event.target.enclosingNodeOrSelfWithNodeName("th");
        if (!cell || !cell.collapsesGroup)
            return;

        cell.classList.add("mouse-over-collapser");
    },

    _mouseoutColumnCollapser: function(event)
    {
        var cell = event.target.enclosingNodeOrSelfWithNodeName("th");
        if (!cell || !cell.collapsesGroup)
            return;

        cell.classList.remove("mouse-over-collapser");
    },

    _clickInColumnCollapser: function(event)
    {
        var cell = event.target.enclosingNodeOrSelfWithNodeName("th");
        if (!cell || !cell.collapsesGroup)
            return;

        this._collapseColumnGroupWithCell(cell);

        event.stopPropagation();
        event.preventDefault();
    },

    collapseColumnGroup: function(columnGroup)
    {
        var collapserColumn = null;
        for (var columnIdentifier in this.columns) {
            var column = this.columns[columnIdentifier];
            if (column.collapsesGroup == columnGroup) {
                collapserColumn = column;
                break;
            }
        }

        console.assert(collapserColumn);
        if (!collapserColumn)
            return;

        var cell = this._headerTableHeaders[collapserColumn.identifier];
        this._collapseColumnGroupWithCell(cell);
    },

    _collapseColumnGroupWithCell: function(cell)
    {
        var columnsWillCollapse = cell.classList.toggle("collapsed");

        this.willToggleColumnGroup(cell.collapsesGroup, columnsWillCollapse);

        var showOrHide = columnsWillCollapse ? this._hideColumn : this._showColumn;
        for (var columnIdentifier in this.columns) {
            var column = this.columns[columnIdentifier];
            if (column.group === cell.collapsesGroup)
                showOrHide.call(this, columnIdentifier);
        }

        var collapserButton = cell.querySelector(".collapser-button");
        if (collapserButton)
            collapserButton.title = columnsWillCollapse ? this._collapserButtonExpandColumnsToolTip() : this._collapserButtonCollapseColumnsToolTip();

        this.didToggleColumnGroup(cell.collapsesGroup, columnsWillCollapse);
    },

    _collapserButtonCollapseColumnsToolTip: function()
    {
        return WebInspector.UIString("Collapse columns");
    },

    _collapserButtonExpandColumnsToolTip: function()
    {
        return WebInspector.UIString("Expand columns");
    },

    willToggleColumnGroup: function(columnGroup, willCollapse)
    {
        // Implemented by subclasses if needed.
    },

    didToggleColumnGroup: function(columnGroup, didCollapse)
    {
        // Implemented by subclasses if needed.
    },

    isColumnSortColumn: function(columnIdentifier)
    {
        return this._sortColumnCell === this._headerTableHeaders[columnIdentifier];
    },

    markColumnAsSortedBy: function(columnIdentifier, sortOrder)
    {
        if (this._sortColumnCell)
            this._sortColumnCell.removeMatchingStyleClasses("sort-\\w+");
        this._sortColumnCell = this._headerTableHeaders[columnIdentifier];
        this._sortColumnCell.classList.add("sort-" + sortOrder);
    },

    headerTableHeader: function(columnIdentifier)
    {
        return this._headerTableHeaders[columnIdentifier];
    },

    _generateSortIndicatorImagesIfNeeded: function()
    {
        if (WebInspector.DataGrid._generatedSortIndicatorImages)
            return;

        WebInspector.DataGrid._generatedSortIndicatorImages = true;

        var specifications = {};
        specifications["arrow"] = {
            fillColor: [81, 81, 81],
            shadowColor: [255, 255, 255, 0.5],
            shadowOffsetX: 0,
            shadowOffsetY: 1,
            shadowBlur: 0
        };

        generateColoredImagesForCSS("Images/SortIndicatorDownArrow.pdf", specifications, 9, 8, "data-grid-sort-indicator-down-");
        generateColoredImagesForCSS("Images/SortIndicatorUpArrow.pdf", specifications, 9, 8, "data-grid-sort-indicator-up-");
    },

    _mouseDownInDataTable: function(event)
    {
        var gridNode = this.dataGridNodeFromNode(event.target);
        if (!gridNode || !gridNode.selectable)
            return;

        if (gridNode.isEventWithinDisclosureTriangle(event))
            return;

        if (event.metaKey) {
            if (gridNode.selected)
                gridNode.deselect();
            else
                gridNode.select();
        } else
            gridNode.select();
    },

    _contextMenuInDataTable: function(event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);

        var gridNode = this.dataGridNodeFromNode(event.target);
        if (this.dataGrid._refreshCallback && (!gridNode || gridNode !== this.creationNode))
            contextMenu.appendItem(WebInspector.UIString("Refresh"), this._refreshCallback.bind(this));

        if (gridNode && gridNode.selectable && !gridNode.isEventWithinDisclosureTriangle(event)) {
            contextMenu.appendItem(WebInspector.UIString("Copy Row"), this._copyRow.bind(this, event.target));

            if (this.dataGrid._editCallback) {
                if (gridNode === this.creationNode)
                    contextMenu.appendItem(WebInspector.UIString("Add New"), this._startEditing.bind(this, event.target));
                else {
                    var element = event.target.enclosingNodeOrSelfWithNodeName("td");
                    var columnIdentifier = parseInt(element.className.match(/\b(\d+)-column\b/)[1], 10);
                    var columnTitle = this.dataGrid.columns[columnIdentifier].title;
                    contextMenu.appendItem(WebInspector.UIString("Edit “%s”").format(columnTitle), this._startEditing.bind(this, event.target));
                }
            }
            if (this.dataGrid._deleteCallback && gridNode !== this.creationNode)
                contextMenu.appendItem(WebInspector.UIString("Delete"), this._deleteCallback.bind(this, gridNode));
        }

        contextMenu.show();
    },

    _clickInDataTable: function(event)
    {
        var gridNode = this.dataGridNodeFromNode(event.target);
        if (!gridNode || !gridNode.hasChildren)
            return;

        if (!gridNode.isEventWithinDisclosureTriangle(event))
            return;

        if (gridNode.expanded) {
            if (event.altKey)
                gridNode.collapseRecursively();
            else
                gridNode.collapse();
        } else {
            if (event.altKey)
                gridNode.expandRecursively();
            else
                gridNode.expand();
        }
    },

    _copyTextForDataGridNode: function(node)
    {
        var fields = [];
        for (var columnIdentifier in node.dataGrid.columns)
            fields.push(node.data[columnIdentifier]);

        var tabSeparatedValues = fields.join("\t");
        return tabSeparatedValues;
    },

    handleBeforeCopyEvent: function(event)
    {
        if (this.selectedNode && window.getSelection().isCollapsed)
            event.preventDefault();
    },

    handleCopyEvent: function(event)
    {
        if (!this.selectedNode || !window.getSelection().isCollapsed)
            return;

        var copyText = this._copyTextForDataGridNode(this.selectedNode);
        event.clipboardData.setData("text/plain", copyText);
        event.stopPropagation();
        event.preventDefault();
    },

    _copyRow: function(target)
    {
        var gridNode = this.dataGridNodeFromNode(target);
        if (!gridNode)
            return;

        var copyText = this._copyTextForDataGridNode(gridNode);
        InspectorFrontendHost.copyText(copyText);
    },

    get resizeMethod()
    {
        if (typeof this._resizeMethod === "undefined")
            return WebInspector.DataGrid.ResizeMethod.Nearest;
        return this._resizeMethod;
    },

    set resizeMethod(method)
    {
        this._resizeMethod = method;
    },

    _startResizerDragging: function(event)
    {
        if (event.button !== 0 || event.ctrlKey)
            return;

        this._currentResizer = event.target;
        if (!this._currentResizer.rightNeighboringColumnID)
            return;

        WebInspector.elementDragStart(this._currentResizer, this._resizerDragging.bind(this),
            this._endResizerDragging.bind(this), event, "col-resize");
    },

    _resizerDragging: function(event)
    {
        if (event.button !== 0)
            return;

        var resizer = this._currentResizer;
        if (!resizer)
            return;

        // Constrain the dragpoint to be within the containing div of the
        // datagrid.
        var dragPoint = event.clientX - this.element.totalOffsetLeft;
        // Constrain the dragpoint to be within the space made up by the
        // column directly to the left and the column directly to the right.
        var leftCellIndex = resizer.leftNeighboringColumnID;
        var rightCellIndex = resizer.rightNeighboringColumnID;
        var firstRowCells = this.headerTableBody.rows[0].cells;
        var leftEdgeOfPreviousColumn = 0;
        for (var i = 0; i < leftCellIndex; i++)
            leftEdgeOfPreviousColumn += firstRowCells[i].offsetWidth;

        // Differences for other resize methods
        if (this.resizeMethod == WebInspector.DataGrid.ResizeMethod.Last) {
            rightCellIndex = this.resizers.length;
        } else if (this.resizeMethod == WebInspector.DataGrid.ResizeMethod.First) {
            leftEdgeOfPreviousColumn += firstRowCells[leftCellIndex].offsetWidth - firstRowCells[0].offsetWidth;
            leftCellIndex = 0;
        }

        var rightEdgeOfNextColumn = leftEdgeOfPreviousColumn + firstRowCells[leftCellIndex].offsetWidth + firstRowCells[rightCellIndex].offsetWidth;

        // Give each column some padding so that they don't disappear.
        var leftMinimum = leftEdgeOfPreviousColumn + this.ColumnResizePadding;
        var rightMaximum = rightEdgeOfNextColumn - this.ColumnResizePadding;

        dragPoint = Number.constrain(dragPoint, leftMinimum, rightMaximum);

        resizer.style.left = (dragPoint - this.CenterResizerOverBorderAdjustment) + "px";

        var percentLeftColumn = (((dragPoint - leftEdgeOfPreviousColumn) / this._dataTable.offsetWidth) * 100) + "%";
        this._headerTableColumnGroup.children[leftCellIndex].style.width = percentLeftColumn;
        this._dataTableColumnGroup.children[leftCellIndex].style.width = percentLeftColumn;

        var percentRightColumn = (((rightEdgeOfNextColumn - dragPoint) / this._dataTable.offsetWidth) * 100) + "%";
        this._headerTableColumnGroup.children[rightCellIndex].style.width =  percentRightColumn;
        this._dataTableColumnGroup.children[rightCellIndex].style.width = percentRightColumn;

        this._positionResizers();
        event.preventDefault();
        this.dispatchEventToListeners(WebInspector.DataGrid.Event.DidLayout);
    },

    _endResizerDragging: function(event)
    {
        if (event.button !== 0)
            return;

        WebInspector.elementDragEnd(event);
        this._currentResizer = null;
        this.dispatchEventToListeners(WebInspector.DataGrid.Event.DidLayout);
    },

    ColumnResizePadding: 10,

    CenterResizerOverBorderAdjustment: 3,
}

WebInspector.DataGrid.ResizeMethod = {
    Nearest: "nearest",
    First: "first",
    Last: "last"
}

WebInspector.DataGrid.prototype.__proto__ = WebInspector.Object.prototype;

/**
 * @constructor
 * @extends {WebInspector.Object}
 * @param {boolean=} hasChildren
 */
WebInspector.DataGridNode = function(data, hasChildren)
{
    this._expanded = false;
    this._selected = false;
    this._shouldRefreshChildren = true;
    this._data = data || {};
    this.hasChildren = hasChildren || false;
    this.children = [];
    this.dataGrid = null;
    this.parent = null;
    this.previousSibling = null;
    this.nextSibling = null;
    this.disclosureToggleWidth = 10;
}

WebInspector.DataGridNode.prototype = {
    selectable: true,

    get element()
    {
        if (this._element)
            return this._element;

        if (!this.dataGrid)
            return null;

        this._element = document.createElement("tr");
        this._element._dataGridNode = this;

        if (this.hasChildren)
            this._element.classList.add("parent");
        if (this.expanded)
            this._element.classList.add("expanded");
        if (this.selected)
            this._element.classList.add("selected");
        if (this.revealed)
            this._element.classList.add("revealed");

        this.createCells();
        return this._element;
    },

    createCells: function()
    {
        for (var columnIdentifier in this.dataGrid.columns) {
            var cell = this.createCell(columnIdentifier);
            this._element.appendChild(cell);
        }
    },

    get data()
    {
        return this._data;
    },

    set data(x)
    {
        this._data = x || {};
        this.refresh();
    },

    get revealed()
    {
        if ("_revealed" in this)
            return this._revealed;

        var currentAncestor = this.parent;
        while (currentAncestor && !currentAncestor.root) {
            if (!currentAncestor.expanded) {
                this._revealed = false;
                return false;
            }

            currentAncestor = currentAncestor.parent;
        }

        this._revealed = true;
        return true;
    },

    set hasChildren(x)
    {
        if (this._hasChildren === x)
            return;

        this._hasChildren = x;

        if (!this._element)
            return;

        if (this._hasChildren)
        {
            this._element.classList.add("parent");
            if (this.expanded)
                this._element.classList.add("expanded");
        }
        else
        {
            this._element.classList.remove("parent");
            this._element.classList.remove("expanded");
        }
    },

    get hasChildren()
    {
        return this._hasChildren;
    },

    set revealed(x)
    {
        if (this._revealed === x)
            return;

        this._revealed = x;

        if (this._element) {
            if (this._revealed)
                this._element.classList.add("revealed");
            else
                this._element.classList.remove("revealed");
        }

        for (var i = 0; i < this.children.length; ++i)
            this.children[i].revealed = x && this.expanded;
    },

    get depth()
    {
        if ("_depth" in this)
            return this._depth;
        if (this.parent && !this.parent.root)
            this._depth = this.parent.depth + 1;
        else
            this._depth = 0;
        return this._depth;
    },

    get leftPadding()
    {
        if (typeof(this._leftPadding) === "number")
            return this._leftPadding;
        
        this._leftPadding = this.depth * this.dataGrid.indentWidth;
        return this._leftPadding;
    },

    get shouldRefreshChildren()
    {
        return this._shouldRefreshChildren;
    },

    set shouldRefreshChildren(x)
    {
        this._shouldRefreshChildren = x;
        if (x && this.expanded)
            this.expand();
    },

    get selected()
    {
        return this._selected;
    },

    set selected(x)
    {
        if (x)
            this.select();
        else
            this.deselect();
    },

    get expanded()
    {
        return this._expanded;
    },

    set expanded(x)
    {
        if (x)
            this.expand();
        else
            this.collapse();
    },

    refresh: function()
    {
        if (!this._element || !this.dataGrid)
            return;

        this._element.removeChildren();
        this.createCells();
    },

    updateLayout: function()
    {
        // Implemented by subclasses if needed.
    },

    createCell: function(columnIdentifier)
    {
        var cell = document.createElement("td");
        cell.className = columnIdentifier + "-column";

        var alignment = this.dataGrid.aligned[columnIdentifier];
        if (alignment)
            cell.classList.add(alignment);

        var group = this.dataGrid.groups[columnIdentifier];
        if (group)
            cell.classList.add("column-group-" + group);

        var div = document.createElement("div");
        var content = this.createCellContent(columnIdentifier, cell);
        div.appendChild(content instanceof Node ? content : document.createTextNode(content));
        cell.appendChild(div);

        if (columnIdentifier === this.dataGrid.disclosureColumnIdentifier) {
            cell.classList.add("disclosure");
            if (this.leftPadding)
                cell.style.setProperty("padding-left", this.leftPadding + "px");
        }

        return cell;
    },

    createCellContent: function(columnIdentifier)
    {
        return this.data[columnIdentifier];
    },

    elementWithColumnIdentifier: function(columnIdentifier)
    {
        var index = Object.keys(this.dataGrid.columns).indexOf(columnIdentifier);
        if (index === -1)
            return null;

        return this._element.children[index];
    },

    // Share these functions with DataGrid. They are written to work with a DataGridNode this object.
    appendChild: WebInspector.DataGrid.prototype.appendChild,
    insertChild: WebInspector.DataGrid.prototype.insertChild,
    removeChild: WebInspector.DataGrid.prototype.removeChild,
    removeChildren: WebInspector.DataGrid.prototype.removeChildren,
    removeChildrenRecursive: WebInspector.DataGrid.prototype.removeChildrenRecursive,

    _recalculateSiblings: function(myIndex)
    {
        if (!this.parent)
            return;

        var previousChild = (myIndex > 0 ? this.parent.children[myIndex - 1] : null);

        if (previousChild) {
            previousChild.nextSibling = this;
            this.previousSibling = previousChild;
        } else
            this.previousSibling = null;

        var nextChild = this.parent.children[myIndex + 1];

        if (nextChild) {
            nextChild.previousSibling = this;
            this.nextSibling = nextChild;
        } else
            this.nextSibling = null;
    },

    collapse: function()
    {
        if (this._element)
            this._element.classList.remove("expanded");

        this._expanded = false;

        for (var i = 0; i < this.children.length; ++i)
            this.children[i].revealed = false;

        this.dispatchEventToListeners("collapsed");
    },

    collapseRecursively: function()
    {
        var item = this;
        while (item) {
            if (item.expanded)
                item.collapse();
            item = item.traverseNextNode(false, this, true);
        }
    },

    expand: function()
    {
        if (!this.hasChildren || this.expanded)
            return;

        if (this.revealed && !this._shouldRefreshChildren)
            for (var i = 0; i < this.children.length; ++i)
                this.children[i].revealed = true;

        if (this._shouldRefreshChildren) {
            for (var i = 0; i < this.children.length; ++i)
                this.children[i]._detach();

            this.dispatchEventToListeners("populate");

            if (this._attached) {
                for (var i = 0; i < this.children.length; ++i) {
                    var child = this.children[i];
                    if (this.revealed)
                        child.revealed = true;
                    child._attach();
                }
            }

            delete this._shouldRefreshChildren;
        }

        if (this._element)
            this._element.classList.add("expanded");

        this._expanded = true;

        this.dispatchEventToListeners("expanded");
    },

    expandRecursively: function()
    {
        var item = this;
        while (item) {
            item.expand();
            item = item.traverseNextNode(false, this);
        }
    },

    reveal: function()
    {
        var currentAncestor = this.parent;
        while (currentAncestor && !currentAncestor.root) {
            if (!currentAncestor.expanded)
                currentAncestor.expand();
            currentAncestor = currentAncestor.parent;
        }

        this.element.scrollIntoViewIfNeeded(false);

        this.dispatchEventToListeners("revealed");
    },

    /**
     * @param {boolean=} supressSelectedEvent
     */
    select: function(supressSelectedEvent)
    {
        if (!this.dataGrid || !this.selectable || this.selected)
            return;

        if (this.dataGrid.selectedNode)
            this.dataGrid.selectedNode.deselect();

        this._selected = true;
        this.dataGrid.selectedNode = this;

        if (this._element)
            this._element.classList.add("selected");

        if (!supressSelectedEvent)
            this.dataGrid.dispatchEventToListeners(WebInspector.DataGrid.Event.SelectedNodeChanged);
    },

    revealAndSelect: function()
    {
        this.reveal();
        this.select();
    },

    /**
     * @param {boolean=} supressDeselectedEvent
     */
    deselect: function(supressDeselectedEvent)
    {
        if (!this.dataGrid || this.dataGrid.selectedNode !== this || !this.selected)
            return;

        this._selected = false;
        this.dataGrid.selectedNode = null;

        if (this._element)
            this._element.classList.remove("selected");

        if (!supressDeselectedEvent)
            this.dataGrid.dispatchEventToListeners(WebInspector.DataGrid.Event.SelectedNodeChanged);
    },

    traverseNextNode: function(skipHidden, stayWithin, dontPopulate, info)
    {
        if (!dontPopulate && this.hasChildren)
            this.dispatchEventToListeners("populate");

        if (info)
            info.depthChange = 0;

        var node = (!skipHidden || this.revealed) ? this.children[0] : null;
        if (node && (!skipHidden || this.expanded)) {
            if (info)
                info.depthChange = 1;
            return node;
        }

        if (this === stayWithin)
            return null;

        node = (!skipHidden || this.revealed) ? this.nextSibling : null;
        if (node)
            return node;

        node = this;
        while (node && !node.root && !((!skipHidden || node.revealed) ? node.nextSibling : null) && node.parent !== stayWithin) {
            if (info)
                info.depthChange -= 1;
            node = node.parent;
        }

        if (!node)
            return null;

        return (!skipHidden || node.revealed) ? node.nextSibling : null;
    },

    traversePreviousNode: function(skipHidden, dontPopulate)
    {
        var node = (!skipHidden || this.revealed) ? this.previousSibling : null;
        if (!dontPopulate && node && node.hasChildren)
            node.dispatchEventToListeners("populate");

        while (node && ((!skipHidden || (node.revealed && node.expanded)) ? node.children.lastValue : null)) {
            if (!dontPopulate && node.hasChildren)
                node.dispatchEventToListeners("populate");
            node = ((!skipHidden || (node.revealed && node.expanded)) ? node.children.lastValue : null);
        }

        if (node)
            return node;

        if (!this.parent || this.parent.root)
            return null;

        return this.parent;
    },

    isEventWithinDisclosureTriangle: function(event)
    {
        if (!this.hasChildren)
            return false;
        var cell = event.target.enclosingNodeOrSelfWithNodeName("td");
        if (!cell.classList.contains("disclosure"))
            return false;
        
        var left = cell.totalOffsetLeft + this.leftPadding;
        return event.pageX >= left && event.pageX <= left + this.disclosureToggleWidth;
    },

    _attach: function()
    {
        if (!this.dataGrid || this._attached)
            return;

        this._attached = true;

        var nextElement = null;

        var previousGridNode = this.traversePreviousNode(true, true);
        if (previousGridNode && previousGridNode.element.parentNode)
            nextElement = previousGridNode.element.nextSibling;
        else if (!previousGridNode)
            nextElement = this.dataGrid.dataTableBody.firstChild;

        // If there is no next grid node, then append before the last child since the last child is the filler row.
        console.assert(this.dataGrid.dataTableBody.lastChild.classList.contains("filler"));
        if (!nextElement)
            nextElement = this.dataGrid.dataTableBody.lastChild;

        this.dataGrid.dataTableBody.insertBefore(this.element, nextElement);

        if (this.expanded)
            for (var i = 0; i < this.children.length; ++i)
                this.children[i]._attach();
    },

    _detach: function()
    {
        if (!this._attached)
            return;

        this._attached = false;

        if (this._element && this._element.parentNode)
            this._element.parentNode.removeChild(this._element);

        for (var i = 0; i < this.children.length; ++i)
            this.children[i]._detach();
    },

    savePosition: function()
    {
        if (this._savedPosition)
            return;

        if (!this.parent)
            throw("savePosition: Node must have a parent.");
        this._savedPosition = {
            parent: this.parent,
            index: this.parent.children.indexOf(this)
        };
    },

    restorePosition: function()
    {
        if (!this._savedPosition)
            return;

        if (this.parent !== this._savedPosition.parent)
            this._savedPosition.parent.insertChild(this, this._savedPosition.index);

        delete this._savedPosition;
    }
}

WebInspector.DataGridNode.prototype.__proto__ = WebInspector.Object.prototype;

/**
 * @constructor
 * @extends {WebInspector.DataGridNode}
 */
WebInspector.CreationDataGridNode = function(data, hasChildren)
{
    WebInspector.DataGridNode.call(this, data, hasChildren);
    this.isCreationNode = true;
}

WebInspector.CreationDataGridNode.prototype = {
    makeNormal: function()
    {
        delete this.isCreationNode;
        delete this.makeNormal;
    }
}

WebInspector.CreationDataGridNode.prototype.__proto__ = WebInspector.DataGridNode.prototype;
