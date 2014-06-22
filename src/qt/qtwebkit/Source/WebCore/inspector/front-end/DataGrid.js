/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * @extends {WebInspector.View}
 * @param {!Array.<!WebInspector.DataGrid.ColumnDescriptor>} columnsArray
 * @param {function(WebInspector.DataGridNode, string, string, string)=} editCallback
 * @param {function(WebInspector.DataGridNode)=} deleteCallback
 * @param {function()=} refreshCallback
 * @param {function(!WebInspector.ContextMenu, WebInspector.DataGridNode)=} contextMenuCallback
 */
WebInspector.DataGrid = function(columnsArray, editCallback, deleteCallback, refreshCallback, contextMenuCallback)
{
    WebInspector.View.call(this);
    this.registerRequiredCSS("dataGrid.css");

    this.element.className = "data-grid";
    this.element.tabIndex = 0;
    this.element.addEventListener("keydown", this._keyDown.bind(this), false);

    this._headerTable = document.createElement("table");
    this._headerTable.className = "header";
    this._headerTableHeaders = {};

    this._dataTable = document.createElement("table");
    this._dataTable.className = "data";

    this._dataTable.addEventListener("mousedown", this._mouseDownInDataTable.bind(this), true);
    this._dataTable.addEventListener("click", this._clickInDataTable.bind(this), true);

    this._dataTable.addEventListener("contextmenu", this._contextMenuInDataTable.bind(this), true);

    // FIXME: Add a createCallback which is different from editCallback and has different
    // behavior when creating a new node.
    if (editCallback)
        this._dataTable.addEventListener("dblclick", this._ondblclick.bind(this), false);
    this._editCallback = editCallback;
    this._deleteCallback = deleteCallback;
    this._refreshCallback = refreshCallback;
    this._contextMenuCallback = contextMenuCallback;

    this._scrollContainer = document.createElement("div");
    this._scrollContainer.className = "data-container";
    this._scrollContainer.appendChild(this._dataTable);

    this.element.appendChild(this._headerTable);
    this.element.appendChild(this._scrollContainer);

    var headerRow = document.createElement("tr");
    var columnGroup = document.createElement("colgroup");
    columnGroup.span = columnsArray.length;

    var fillerRow = document.createElement("tr");
    fillerRow.className = "filler";

    this._columnsArray = columnsArray;
    this.columns = {};

    for (var i = 0; i < columnsArray.length; ++i) {
        var column = columnsArray[i];
        column.ordinal = i;
        var columnIdentifier = column.identifier = column.id || i;
        this.columns[columnIdentifier] = column;
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
            cell.addStyleClass("sort-" + column.sort);
            this._sortColumnCell = cell;
        }

        if (column.sortable) {
            cell.addEventListener("click", this._clickInHeaderCell.bind(this), false);
            cell.addStyleClass("sortable");
        }

        headerRow.appendChild(cell);
        fillerRow.createChild("td", columnIdentifier + "-column");
    }

    headerRow.createChild("th", "corner");
    fillerRow.createChild("td", "corner");

    this._headerTableColumnGroup = columnGroup;
    this._headerTable.appendChild(this._headerTableColumnGroup);
    this.headerTableBody.appendChild(headerRow);

    this._dataTableColumnGroup = columnGroup.cloneNode(true);
    this._dataTable.appendChild(this._dataTableColumnGroup);
    this.dataTableBody.appendChild(fillerRow);

    this.selectedNode = null;
    this.expandNodesWhenArrowing = false;
    this.setRootNode(new WebInspector.DataGridNode());
    this.indentWidth = 15;
    this.resizers = [];
    this._columnWidthsInitialized = false;
}

/** @typedef {{id: ?string, editable: boolean, sort: WebInspector.DataGrid.Order, sortable: boolean, align: WebInspector.DataGrid.Align}} */
WebInspector.DataGrid.ColumnDescriptor;

WebInspector.DataGrid.Events = {
    SelectedNode: "SelectedNode",
    DeselectedNode: "DeselectedNode",
    SortingChanged: "SortingChanged",
    ColumnsResized: "ColumnsResized"
}

/** @enum {string} */
WebInspector.DataGrid.Order = {
    Ascending: "ascending",
    Descending: "descending"
}

/** @enum {string} */
WebInspector.DataGrid.Align = {
    Center: "center",
    Right: "right"
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

    var columns = [];
    for (var i = 0; i < columnNames.length; ++i)
        columns.push({title: columnNames[i], width: columnNames[i].length, sortable: true});

    var nodes = [];
    for (var i = 0; i < values.length / numColumns; ++i) {
        var data = {};
        for (var j = 0; j < columnNames.length; ++j)
            data[j] = values[numColumns * i + j];

        var node = new WebInspector.DataGridNode(data, false);
        node.selectable = false;
        nodes.push(node);
    }

    var dataGrid = new WebInspector.DataGrid(columns);
    var length = nodes.length;
    for (var i = 0; i < length; ++i)
        dataGrid.rootNode().appendChild(nodes[i]);

    dataGrid.addEventListener(WebInspector.DataGrid.Events.SortingChanged, sortDataGrid, this);

    function sortDataGrid()
    {
        var nodes = dataGrid._rootNode.children.slice();
        var sortColumnIdentifier = dataGrid.sortColumnIdentifier();
        var sortDirection = dataGrid.isSortOrderAscending() ? 1 : -1;
        var columnIsNumeric = true;

        for (var i = 0; i < nodes.length; i++) {
            if (isNaN(Number(nodes[i].data[sortColumnIdentifier])))
                columnIsNumeric = false;
        }

        function comparator(dataGridNode1, dataGridNode2)
        {
            var item1 = dataGridNode1.data[sortColumnIdentifier];
            var item2 = dataGridNode2.data[sortColumnIdentifier];
            item1 = item1 instanceof Node ? item1.textContent : String(item1);
            item2 = item2 instanceof Node ? item2.textContent : String(item2);

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
        dataGrid.rootNode().removeChildren();
        for (var i = 0; i < nodes.length; i++)
            dataGrid._rootNode.appendChild(nodes[i]);
    }
    return dataGrid;
}

WebInspector.DataGrid.prototype = {
    /**
     * @param {!WebInspector.DataGridNode} rootNode
     */
    setRootNode: function(rootNode)
    {
        if (this._rootNode) {
            this._rootNode.removeChildren();
            this._rootNode.dataGrid = null;
            this._rootNode._isRoot = false;
        }
        /** @type {!WebInspector.DataGridNode} */
        this._rootNode = rootNode;
        rootNode._isRoot = true;
        rootNode.hasChildren = false;
        rootNode._expanded = true;
        rootNode._revealed = true;
        rootNode.dataGrid = this;
    },

    /**
     * @return {!WebInspector.DataGridNode}
     */
    rootNode: function()
    {
        return this._rootNode;
    },

    _ondblclick: function(event)
    {
        if (this._editing || this._editingNode)
            return;

        var columnIdentifier = this.columnIdentifierFromNode(event.target);
        if (!columnIdentifier || !this.columns[columnIdentifier].editable)
            return;
        this._startEditing(event.target);
    },

    /**
     * @param {!WebInspector.DataGridNode} node
     * @param {number} columnOrdinal
     */
    _startEditingColumnOfDataGridNode: function(node, columnOrdinal)
    {
        this._editing = true;
        /** @type {WebInspector.DataGridNode} */
        this._editingNode = node;
        this._editingNode.select();

        var element = this._editingNode._element.children[columnOrdinal];
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
            return this._startEditingColumnOfDataGridNode(this._editingNode, this._nextEditableColumn(-1));

        this._editing = true;
        WebInspector.startEditing(element, this._startEditingConfig(element));

        window.getSelection().setBaseAndExtent(element, 0, element, 1);
    },

    renderInline: function()
    {
        this.element.addStyleClass("inline");
    },

    _startEditingConfig: function(element)
    {
        return new WebInspector.EditingConfig(this._editingCommitted.bind(this), this._editingCancelled.bind(this), element.textContent);
    },

    _editingCommitted: function(element, newText, oldText, context, moveDirection)
    {
        var columnIdentifier = this.columnIdentifierFromNode(element);
        if (!columnIdentifier) {
            this._editingCancelled(element);
            return;
        }
        var columnOrdinal = this.columns[columnIdentifier].ordinal;
        var textBeforeEditing = this._editingNode.data[columnIdentifier];
        var currentEditingNode = this._editingNode;

        function moveToNextIfNeeded(wasChange) {
            if (!moveDirection)
                return;

            if (moveDirection === "forward") {
            var firstEditableColumn = this._nextEditableColumn(-1);
                if (currentEditingNode.isCreationNode && columnOrdinal === firstEditableColumn && !wasChange)
                    return;

                var nextEditableColumn = this._nextEditableColumn(columnOrdinal);
                if (nextEditableColumn !== -1)
                    return this._startEditingColumnOfDataGridNode(currentEditingNode, nextEditableColumn);

                var nextDataGridNode = currentEditingNode.traverseNextNode(true, null, true);
                if (nextDataGridNode)
                    return this._startEditingColumnOfDataGridNode(nextDataGridNode, firstEditableColumn);
                if (currentEditingNode.isCreationNode && wasChange) {
                    this.addCreationNode(false);
                    return this._startEditingColumnOfDataGridNode(this.creationNode, firstEditableColumn);
                }
                return;
            }

            if (moveDirection === "backward") {
                var prevEditableColumn = this._nextEditableColumn(columnOrdinal, true);
                if (prevEditableColumn !== -1)
                    return this._startEditingColumnOfDataGridNode(currentEditingNode, prevEditableColumn);

                var lastEditableColumn = this._nextEditableColumn(this._columnsArray.length, true);
                var nextDataGridNode = currentEditingNode.traversePreviousNode(true, true);
                if (nextDataGridNode)
                    return this._startEditingColumnOfDataGridNode(nextDataGridNode, lastEditableColumn);
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

    /**
     * @param {number} columnOrdinal
     * @param {boolean=} moveBackward
     * @return {number}
     */
    _nextEditableColumn: function(columnOrdinal, moveBackward)
    {
        var increment = moveBackward ? -1 : 1;
        var columns = this._columnsArray;
        for (var i = columnOrdinal + increment; (i >= 0) && (i < columns.length); i += increment) {
            if (columns[i].editable)
                return i;
        }
        return -1;
    },

    /**
     * @return {?string}
     */
    sortColumnIdentifier: function()
    {
        if (!this._sortColumnCell)
            return null;
        return this._sortColumnCell.columnIdentifier;
    },

    /**
     * @return {?string}
     */
    sortOrder: function()
    {
        if (!this._sortColumnCell || this._sortColumnCell.hasStyleClass("sort-ascending"))
            return WebInspector.DataGrid.Order.Ascending;
        if (this._sortColumnCell.hasStyleClass("sort-descending"))
            return WebInspector.DataGrid.Order.Descending;
        return null;
    },

    /**
     * @return {boolean}
     */
    isSortOrderAscending: function()
    {
        return !this._sortColumnCell || this._sortColumnCell.hasStyleClass("sort-ascending");
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
     * @param {Array.<number>} widths
     * @param {number} minPercent
     * @param {number=} maxPercent
     */
    _autoSizeWidths: function(widths, minPercent, maxPercent)
    {
        if (minPercent)
            minPercent = Math.min(minPercent, Math.floor(100 / widths.length));
        var totalWidth = 0;
        for (var i = 0; i < widths.length; ++i)
            totalWidth += widths[i];
        var totalPercentWidth = 0;
        for (var i = 0; i < widths.length; ++i) {
            var width = Math.round(100 * widths[i] / totalWidth);
            if (minPercent && width < minPercent)
                width = minPercent;
            else if (maxPercent && width > maxPercent)
                width = maxPercent;
            totalPercentWidth += width;
            widths[i] = width;
        }
        var recoupPercent = totalPercentWidth - 100;

        while (minPercent && recoupPercent > 0) {
            for (var i = 0; i < widths.length; ++i) {
                if (widths[i] > minPercent) {
                    --widths[i];
                    --recoupPercent;
                    if (!recoupPercent)
                        break;
                }
            }
        }

        while (maxPercent && recoupPercent < 0) {
            for (var i = 0; i < widths.length; ++i) {
                if (widths[i] < maxPercent) {
                    ++widths[i];
                    ++recoupPercent;
                    if (!recoupPercent)
                        break;
                }
            }
        }

        return widths;
    },

    /**
     * @param {number} minPercent
     * @param {number=} maxPercent
     * @param {number=} maxDescentLevel
     */
    autoSizeColumns: function(minPercent, maxPercent, maxDescentLevel)
    {
        var widths = [];
        for (var i = 0; i < this._columnsArray.length; ++i)
            widths.push((this._columnsArray[i].title || "").length);

        maxDescentLevel = maxDescentLevel || 0;
        var children = this._enumerateChildren(this._rootNode, [], maxDescentLevel + 1);
        for (var i = 0; i < children.length; ++i) {
            var node = children[i];
            for (var j = 0; j < this._columnsArray.length; ++j) {
                var text = node.data[this._columnsArray[j].identifier] || "";
                if (text.length > widths[j])
                    widths[j] = text.length;
            }
        }

        widths = this._autoSizeWidths(widths, minPercent, maxPercent);

        for (var i = 0; i < this._columnsArray.length; ++i)
            this._columnsArray[i].element.style.width = widths[i] + "%";
        this._columnWidthsInitialized = false;
        this.updateWidths();
    },

    _enumerateChildren: function(rootNode, result, maxLevel)
    {
        if (!rootNode._isRoot)
            result.push(rootNode);
        if (!maxLevel)
            return;
        for (var i = 0; i < rootNode.children.length; ++i)
            this._enumerateChildren(rootNode.children[i], result, maxLevel - 1);
        return result;
    },

    onResize: function()
    {
        this.updateWidths();
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
    updateWidths: function()
    {
        var headerTableColumns = this._headerTableColumnGroup.children;

        var tableWidth = this._dataTable.offsetWidth;
        var numColumns = headerTableColumns.length;

        // Do not attempt to use offsetes if we're not attached to the document tree yet.
        if (!this._columnWidthsInitialized && this.element.offsetWidth) {
            // Give all the columns initial widths now so that during a resize,
            // when the two columns that get resized get a percent value for
            // their widths, all the other columns already have percent values
            // for their widths.
            for (var i = 0; i < numColumns; i++) {
                var columnWidth = this.headerTableBody.rows[0].cells[i].offsetWidth;
                var percentWidth = ((columnWidth / tableWidth) * 100) + "%";
                this._headerTableColumnGroup.children[i].style.width = percentWidth;
                this._dataTableColumnGroup.children[i].style.width = percentWidth;
            }
            this._columnWidthsInitialized = true;
        }
        this._positionResizers();
        this.dispatchEventToListeners(WebInspector.DataGrid.Events.ColumnsResized);
    },

    applyColumnWeights: function()
    {
        var sumOfWeights = 0.0;
        for (var i = 0; i < this._columnsArray.length; ++i) {
            var column = this._columnsArray[i];
            if (this.isColumnVisible(column))
                sumOfWeights += column.weight;
        }
        var factor = 100 / sumOfWeights;

        for (var i = 0; i < this._columnsArray.length; ++i) {
            var column = this._columnsArray[i];
            var width = this.isColumnVisible(column) ? ((factor * column.weight) + "%"): "0%";
            this._headerTableColumnGroup.children[i].style.width = width;
            this._dataTableColumnGroup.children[i].style.width = width;
        }

        this._positionResizers();
        this.dispatchEventToListeners(WebInspector.DataGrid.Events.ColumnsResized);
    },

    /**
     * @param {!WebInspector.DataGrid.ColumnDescriptor} column
     * @return {boolean}
     */
    isColumnVisible: function(column)
    {
        return !column.hidden;
    },

    /**
     * @param {string} columnIdentifier
     * @param {boolean} visible
     */
    setColumnVisible: function(columnIdentifier, visible)
    {
        if (visible === !this.columns[columnIdentifier].hidden)
            return;

        this.columns[columnIdentifier].hidden = !visible;
        this.element.enableStyleClass("hide-" + columnIdentifier + "-column", !visible);
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
                resizer.addStyleClass("data-grid-resizer");
                // This resizer is associated with the column to its right.
                WebInspector.installDragHandle(resizer, this._startResizerDragging.bind(this), this._resizerDragging.bind(this), this._endResizerDragging.bind(this), "col-resize");
                this.element.appendChild(resizer);
                this.resizers[i] = resizer;
            }

            // Get the width of the cell in the first (and only) row of the
            // header table in order to determine the width of the column, since
            // it is not possible to query a column for its width.
            left += this.headerTableBody.rows[0].cells[i].offsetWidth;

            if (!this._columnsArray[i].hidden) {
                resizer.style.removeProperty("display");
                if (resizer._position !== left) {
                    resizer._position = left;
                    resizer.style.left = left + "px";
                }
                resizer.leftNeighboringColumnIndex = i;
                if (previousResizer)
                    previousResizer.rightNeighboringColumnIndex = i;
                previousResizer = resizer;
            } else {
                if (previousResizer && previousResizer._position !== left) {
                    previousResizer._position = left;
                    previousResizer.style.left = left + "px";
                }
                resizer.style.setProperty("display", "none");
                resizer.leftNeighboringColumnIndex = 0;
                resizer.rightNeighboringColumnIndex = 0;
            }
        }
        if (previousResizer)
            previousResizer.rightNeighboringColumnIndex = numColumns - 1;
    },

    addCreationNode: function(hasChildren)
    {
        if (this.creationNode)
            this.creationNode.makeNormal();

        var emptyData = {};
        for (var column in this.columns)
            emptyData[column] = '';
        this.creationNode = new WebInspector.CreationDataGridNode(emptyData, hasChildren);
        this.rootNode().appendChild(this.creationNode);
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
        var fillerRow = childNodes[childNodes.length - 1];

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
            } else if (this.selectedNode.parent && !this.selectedNode.parent._isRoot) {
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
                this.changeNodeAfterDeletion();
            }
        } else if (isEnterKey(event)) {
            if (this._editCallback) {
                handled = true;
                this._startEditing(this.selectedNode._element.children[this._nextEditableColumn(-1)]);
            }
        }

        if (nextSelectedNode) {
            nextSelectedNode.reveal();
            nextSelectedNode.select();
        }

        if (handled)
            event.consume(true);
    },

    changeNodeAfterDeletion: function()
    {
        var nextSelectedNode = this.selectedNode.traverseNextNode(true);
        while (nextSelectedNode && !nextSelectedNode.selectable)
            nextSelectedNode = nextSelectedNode.traverseNextNode(true);

        if (!nextSelectedNode || nextSelectedNode.isCreationNode) {
            nextSelectedNode = this.selectedNode.traversePreviousNode(true);
            while (nextSelectedNode && !nextSelectedNode.selectable)
                nextSelectedNode = nextSelectedNode.traversePreviousNode(true);
        }

        if (nextSelectedNode) {
            nextSelectedNode.reveal();
            nextSelectedNode.select();
        }
    },

    /**
     * @param {!Node} target
     * @return {?WebInspector.DataGridNode}
     */
    dataGridNodeFromNode: function(target)
    {
        var rowElement = target.enclosingNodeOrSelfWithNodeName("tr");
        return rowElement && rowElement._dataGridNode;
    },

    /**
     * @param {!Node} target
     * @return {?string}
     */
    columnIdentifierFromNode: function(target)
    {
        var cellElement = target.enclosingNodeOrSelfWithNodeName("td");
        return cellElement && cellElement.columnIdentifier_;
    },

    _clickInHeaderCell: function(event)
    {
        var cell = event.target.enclosingNodeOrSelfWithNodeName("th");
        if (!cell || !cell.columnIdentifier || !cell.hasStyleClass("sortable"))
            return;

        var sortOrder = WebInspector.DataGrid.Order.Ascending;
        if ((cell === this._sortColumnCell) && this.isSortOrderAscending())
            sortOrder = WebInspector.DataGrid.Order.Descending;

        if (this._sortColumnCell)
            this._sortColumnCell.removeMatchingStyleClasses("sort-\\w+");
        this._sortColumnCell = cell;

        cell.addStyleClass("sort-" + sortOrder);

        this.dispatchEventToListeners(WebInspector.DataGrid.Events.SortingChanged);
    },

    /**
     * @param {string} columnIdentifier
     * @param {!WebInspector.DataGrid.Order} sortOrder
     */
    markColumnAsSortedBy: function(columnIdentifier, sortOrder)
    {
        if (this._sortColumnCell)
            this._sortColumnCell.removeMatchingStyleClasses("sort-\\w+");
        this._sortColumnCell = this._headerTableHeaders[columnIdentifier];
        this._sortColumnCell.addStyleClass("sort-" + sortOrder);
    },

    headerTableHeader: function(columnIdentifier)
    {
        return this._headerTableHeaders[columnIdentifier];
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
        if (this._refreshCallback && (!gridNode || gridNode !== this.creationNode))
            contextMenu.appendItem(WebInspector.UIString("Refresh"), this._refreshCallback.bind(this));

        if (gridNode && gridNode.selectable && !gridNode.isEventWithinDisclosureTriangle(event)) {
            if (this._editCallback) {
                if (gridNode === this.creationNode)
                    contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Add new" : "Add New"), this._startEditing.bind(this, event.target));
                else {
                    var columnIdentifier = this.columnIdentifierFromNode(event.target);
                    if (columnIdentifier && this.columns[columnIdentifier].editable) {
                        var columnTitle = columnIdentifier[0].toUpperCase() + columnIdentifier.slice(1);
                        contextMenu.appendItem(WebInspector.UIString("Edit “%s”", columnTitle), this._startEditing.bind(this, event.target));
                    }
                }
            }
            if (this._deleteCallback && gridNode !== this.creationNode)
                contextMenu.appendItem(WebInspector.UIString("Delete"), this._deleteCallback.bind(this, gridNode));
            if (this._contextMenuCallback)
                this._contextMenuCallback(contextMenu, gridNode);
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

    /**
     * @return {boolean}
     */
    _startResizerDragging: function(event)
    {
        this._currentResizer = event.target;
        return !!this._currentResizer.rightNeighboringColumnIndex
    },

    _resizerDragging: function(event)
    {
        var resizer = this._currentResizer;
        if (!resizer)
            return;

        var tableWidth = this._dataTable.offsetWidth; // Cache it early, before we invalidate layout.

        // Constrain the dragpoint to be within the containing div of the
        // datagrid.
        var dragPoint = event.clientX - this.element.totalOffsetLeft();
        // Constrain the dragpoint to be within the space made up by the
        // column directly to the left and the column directly to the right.
        var leftCellIndex = resizer.leftNeighboringColumnIndex;
        var rightCellIndex = resizer.rightNeighboringColumnIndex;
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

        var percentLeftColumn = (((dragPoint - leftEdgeOfPreviousColumn) / tableWidth) * 100) + "%";
        this._headerTableColumnGroup.children[leftCellIndex].style.width = percentLeftColumn;
        this._dataTableColumnGroup.children[leftCellIndex].style.width = percentLeftColumn;

        var percentRightColumn = (((rightEdgeOfNextColumn - dragPoint) / tableWidth) * 100) + "%";
        this._headerTableColumnGroup.children[rightCellIndex].style.width =  percentRightColumn;
        this._dataTableColumnGroup.children[rightCellIndex].style.width = percentRightColumn;

        var leftColumn = this._columnsArray[leftCellIndex];
        var rightColumn = this._columnsArray[rightCellIndex];
        if (leftColumn.weight || rightColumn.weight) {
            var sumOfWeights = leftColumn.weight + rightColumn.weight;
            var delta = rightEdgeOfNextColumn - leftEdgeOfPreviousColumn;
            leftColumn.weight = (dragPoint - leftEdgeOfPreviousColumn) * sumOfWeights / delta;
            rightColumn.weight = (rightEdgeOfNextColumn - dragPoint) * sumOfWeights / delta;
        }

        this._positionResizers();
        event.preventDefault();
        this.dispatchEventToListeners(WebInspector.DataGrid.Events.ColumnsResized);
    },

    _endResizerDragging: function(event)
    {
        this._currentResizer = null;
        this.dispatchEventToListeners(WebInspector.DataGrid.Events.ColumnsResized);
    },

    ColumnResizePadding: 10,

    CenterResizerOverBorderAdjustment: 3,

    __proto__: WebInspector.View.prototype
}

WebInspector.DataGrid.ResizeMethod = {
    Nearest: "nearest",
    First: "first",
    Last: "last"
}

/**
 * @constructor
 * @extends {WebInspector.Object}
 * @param {*=} data
 * @param {boolean=} hasChildren
 */
WebInspector.DataGridNode = function(data, hasChildren)
{
    this._expanded = false;
    this._selected = false;
    this._shouldRefreshChildren = true;
    this._data = data || {};
    this.hasChildren = hasChildren || false;
    /** @type {!Array.<WebInspector.DataGridNode>} */
    this.children = [];
    this.dataGrid = null;
    this.parent = null;
    /** @type {WebInspector.DataGridNode} */
    this.previousSibling = null;
    /** @type {WebInspector.DataGridNode} */
    this.nextSibling = null;
    this.disclosureToggleWidth = 10;
}

WebInspector.DataGridNode.prototype = {
    /** @type {boolean} */
    selectable: true,

    /** @type {boolean} */
    _isRoot: false,

    get element()
    {
        if (this._element)
            return this._element;

        if (!this.dataGrid)
            return null;

        this._element = document.createElement("tr");
        this._element._dataGridNode = this;

        if (this.hasChildren)
            this._element.addStyleClass("parent");
        if (this.expanded)
            this._element.addStyleClass("expanded");
        if (this.selected)
            this._element.addStyleClass("selected");
        if (this.revealed)
            this._element.addStyleClass("revealed");

        this.createCells();
        this._element.createChild("td", "corner");

        return this._element;
    },

    createCells: function()
    {
        var columnsArray = this.dataGrid._columnsArray;
        for (var i = 0; i < columnsArray.length; ++i) {
            var cell = this.createCell(columnsArray[i].identifier);
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
        while (currentAncestor && !currentAncestor._isRoot) {
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

        this._element.enableStyleClass("parent", this._hasChildren);
        this._element.enableStyleClass("expanded", this._hasChildren && this.expanded);
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

        if (this._element)
            this._element.enableStyleClass("revealed", this._revealed);

        for (var i = 0; i < this.children.length; ++i)
            this.children[i].revealed = x && this.expanded;
    },

    get depth()
    {
        if ("_depth" in this)
            return this._depth;
        if (this.parent && !this.parent._isRoot)
            this._depth = this.parent.depth + 1;
        else
            this._depth = 0;
        return this._depth;
    },

    get leftPadding()
    {
        if (typeof this._leftPadding === "number")
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
        this._element.createChild("td", "corner");
    },

    /**
     * @param {string} columnIdentifier
     * @return {!Element}
     */
    createTD: function(columnIdentifier)
    {
        var cell = document.createElement("td");
        cell.className = columnIdentifier + "-column";
        cell.columnIdentifier_ = columnIdentifier;

        var alignment = this.dataGrid.columns[columnIdentifier].align;
        if (alignment)
            cell.addStyleClass(alignment);

        return cell;
    },

    /**
     * @param {string} columnIdentifier
     * @return {!Element}
     */
    createCell: function(columnIdentifier)
    {
        var cell = this.createTD(columnIdentifier);

        var data = this.data[columnIdentifier];
        var div = document.createElement("div");
        if (data instanceof Node)
            div.appendChild(data);
        else
            div.textContent = data;
        cell.appendChild(div);

        if (columnIdentifier === this.dataGrid.disclosureColumnIdentifier) {
            cell.addStyleClass("disclosure");
            if (this.leftPadding)
                cell.style.setProperty("padding-left", this.leftPadding + "px");
        }

        return cell;
    },

    /**
     * @return {number}
     */
    nodeHeight: function()
    {
        var rowHeight = 16;
        if (!this.revealed)
            return 0;
        if (!this.expanded)
            return rowHeight;
        var result = rowHeight;
        for (var i = 0; i < this.children.length; i++)
            result += this.children[i].nodeHeight();
        return result;
    },

    /**
     * @param {WebInspector.DataGridNode} child
     */
    appendChild: function(child)
    {
        this.insertChild(child, this.children.length);
    },

    /**
     * @param {WebInspector.DataGridNode} child
     * @param {number} index
     */
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
        if (!this.revealed)
            child.revealed = false;
    },

    /**
     * @param {WebInspector.DataGridNode} child
     */
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
        if (this._isRoot)
            return;
        if (this._element)
            this._element.removeStyleClass("expanded");

        this._expanded = false;

        for (var i = 0; i < this.children.length; ++i)
            this.children[i].revealed = false;
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

    populate: function() { },

    expand: function()
    {
        if (!this.hasChildren || this.expanded)
            return;
        if (this._isRoot)
            return;

        if (this.revealed && !this._shouldRefreshChildren)
            for (var i = 0; i < this.children.length; ++i)
                this.children[i].revealed = true;

        if (this._shouldRefreshChildren) {
            for (var i = 0; i < this.children.length; ++i)
                this.children[i]._detach();

            this.populate();

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
            this._element.addStyleClass("expanded");

        this._expanded = true;
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
        if (this._isRoot)
            return;
        var currentAncestor = this.parent;
        while (currentAncestor && !currentAncestor._isRoot) {
            if (!currentAncestor.expanded)
                currentAncestor.expand();
            currentAncestor = currentAncestor.parent;
        }

        this.element.scrollIntoViewIfNeeded(false);
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
            this._element.addStyleClass("selected");

        if (!supressSelectedEvent)
            this.dataGrid.dispatchEventToListeners(WebInspector.DataGrid.Events.SelectedNode);
    },

    revealAndSelect: function()
    {
        if (this._isRoot)
            return;
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
            this._element.removeStyleClass("selected");

        if (!supressDeselectedEvent)
            this.dataGrid.dispatchEventToListeners(WebInspector.DataGrid.Events.DeselectedNode);
    },

    /**
     * @param {boolean} skipHidden
     * @param {WebInspector.DataGridNode=} stayWithin
     * @param {boolean=} dontPopulate
     * @param {Object=} info
     * @return {WebInspector.DataGridNode}
     */
    traverseNextNode: function(skipHidden, stayWithin, dontPopulate, info)
    {
        if (!dontPopulate && this.hasChildren)
            this.populate();

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
        while (node && !node._isRoot && !((!skipHidden || node.revealed) ? node.nextSibling : null) && node.parent !== stayWithin) {
            if (info)
                info.depthChange -= 1;
            node = node.parent;
        }

        if (!node)
            return null;

        return (!skipHidden || node.revealed) ? node.nextSibling : null;
    },

    /**
     * @param {boolean} skipHidden
     * @param {boolean=} dontPopulate
     * @return {WebInspector.DataGridNode}
     */
    traversePreviousNode: function(skipHidden, dontPopulate)
    {
        var node = (!skipHidden || this.revealed) ? this.previousSibling : null;
        if (!dontPopulate && node && node.hasChildren)
            node.populate();

        while (node && ((!skipHidden || (node.revealed && node.expanded)) ? node.children[node.children.length - 1] : null)) {
            if (!dontPopulate && node.hasChildren)
                node.populate();
            node = ((!skipHidden || (node.revealed && node.expanded)) ? node.children[node.children.length - 1] : null);
        }

        if (node)
            return node;

        if (!this.parent || this.parent._isRoot)
            return null;

        return this.parent;
    },

    /**
     * @return {boolean}
     */
    isEventWithinDisclosureTriangle: function(event)
    {
        if (!this.hasChildren)
            return false;
        var cell = event.target.enclosingNodeOrSelfWithNodeName("td");
        if (!cell.hasStyleClass("disclosure"))
            return false;

        var left = cell.totalOffsetLeft() + this.leftPadding;
        return event.pageX >= left && event.pageX <= left + this.disclosureToggleWidth;
    },

    _attach: function()
    {
        if (!this.dataGrid || this._attached)
            return;

        this._attached = true;

        var nextNode = null;
        var previousNode = this.traversePreviousNode(true, true);
        if (previousNode && previousNode.element.parentNode && previousNode.element.nextSibling)
            nextNode = previousNode.element.nextSibling;
        if (!nextNode)
            nextNode = this.dataGrid.dataTableBody.firstChild;
        this.dataGrid.dataTableBody.insertBefore(this.element, nextNode);

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

        this.wasDetached();
    },

    wasDetached: function()
    {
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
    },

    __proto__: WebInspector.Object.prototype
}

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
    },

    __proto__: WebInspector.DataGridNode.prototype
}
