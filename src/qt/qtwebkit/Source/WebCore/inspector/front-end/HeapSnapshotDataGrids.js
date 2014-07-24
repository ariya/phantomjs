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
 * @extends {WebInspector.DataGrid}
 */
WebInspector.HeapSnapshotSortableDataGrid = function(columns)
{
    WebInspector.DataGrid.call(this, columns);

    /**
     * @type {number}
     */
    this._recursiveSortingDepth = 0;
    /**
     * @type {WebInspector.HeapSnapshotGridNode}
     */
    this._highlightedNode = null;
    /**
     * @type {boolean}
     */
    this._populatedAndSorted = false;
    this.addEventListener("sorting complete", this._sortingComplete, this);
    this.addEventListener(WebInspector.DataGrid.Events.SortingChanged, this.sortingChanged, this);
}

WebInspector.HeapSnapshotSortableDataGrid.Events = {
    ContentShown: "ContentShown"
}

WebInspector.HeapSnapshotSortableDataGrid.prototype = {
    /**
     * @return {number}
     */
    defaultPopulateCount: function()
    {
        return 100;
    },

    dispose: function()
    {
        var children = this.topLevelNodes();
        for (var i = 0, l = children.length; i < l; ++i)
            children[i].dispose();
    },

    /**
     * @override
     */
    wasShown: function()
    {
        if (this._populatedAndSorted)
            this.dispatchEventToListeners(WebInspector.HeapSnapshotSortableDataGrid.Events.ContentShown, this);
    },

    _sortingComplete: function()
    {
        this.removeEventListener("sorting complete", this._sortingComplete, this);
        this._populatedAndSorted = true;
        this.dispatchEventToListeners(WebInspector.HeapSnapshotSortableDataGrid.Events.ContentShown, this);
    },

    /**
     * @override
     */
    willHide: function()
    {
        this._clearCurrentHighlight();
    },

    /**
     * @param {WebInspector.ProfilesPanel} profilesPanel
     * @param {WebInspector.ContextMenu} contextMenu
     * @param {Event} event
     */
    populateContextMenu: function(profilesPanel, contextMenu, event)
    {
        var td = event.target.enclosingNodeOrSelfWithNodeName("td");
        if (!td)
            return;
        var node = td.heapSnapshotNode;
        if (node instanceof WebInspector.HeapSnapshotInstanceNode || node instanceof WebInspector.HeapSnapshotObjectNode) {
            function revealInDominatorsView()
            {
                profilesPanel.showObject(node.snapshotNodeId, "Dominators");
            }
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Reveal in Dominators view" : "Reveal in Dominators View"), revealInDominatorsView.bind(this));
        } else if (node instanceof WebInspector.HeapSnapshotDominatorObjectNode) {
            function revealInSummaryView()
            {
                profilesPanel.showObject(node.snapshotNodeId, "Summary");
            }
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Reveal in Summary view" : "Reveal in Summary View"), revealInSummaryView.bind(this));
        }
    },

    resetSortingCache: function()
    {
        delete this._lastSortColumnIdentifier;
        delete this._lastSortAscending;
    },

    topLevelNodes: function()
    {
        return this.rootNode().children;
    },

    /**
     * @param {HeapProfilerAgent.HeapSnapshotObjectId} heapSnapshotObjectId
     */
    highlightObjectByHeapSnapshotId: function(heapSnapshotObjectId)
    {
    },

    /**
     * @param {WebInspector.HeapSnapshotGridNode} node
     */
    highlightNode: function(node)
    {
        var prevNode = this._highlightedNode;
        this._clearCurrentHighlight();
        this._highlightedNode = node;
        this._highlightedNode.element.addStyleClass("highlighted-row");
        // If highlighted node hasn't changed reinsert it to make the highlight animation restart.
        if (node === prevNode) {
            var element = node.element;
            var parent = element.parentElement;
            var nextSibling = element.nextSibling;
            parent.removeChild(element);
            parent.insertBefore(element, nextSibling);
        }
    },

    nodeWasDetached: function(node)
    {
        if (this._highlightedNode === node)
            this._clearCurrentHighlight();
    },

    _clearCurrentHighlight: function()
    {
        if (!this._highlightedNode)
            return
        this._highlightedNode.element.removeStyleClass("highlighted-row");
        this._highlightedNode = null;
    },

    changeNameFilter: function(filter)
    {
        filter = filter.toLowerCase();
        var children = this.topLevelNodes();
        for (var i = 0, l = children.length; i < l; ++i) {
            var node = children[i];
            if (node.depth === 0)
                node.revealed = node._name.toLowerCase().indexOf(filter) !== -1;
        }
        this.updateVisibleNodes();
    },

    sortingChanged: function()
    {
        var sortAscending = this.isSortOrderAscending();
        var sortColumnIdentifier = this.sortColumnIdentifier();
        if (this._lastSortColumnIdentifier === sortColumnIdentifier && this._lastSortAscending === sortAscending)
            return;
        this._lastSortColumnIdentifier = sortColumnIdentifier;
        this._lastSortAscending = sortAscending;
        var sortFields = this._sortFields(sortColumnIdentifier, sortAscending);

        function SortByTwoFields(nodeA, nodeB)
        {
            var field1 = nodeA[sortFields[0]];
            var field2 = nodeB[sortFields[0]];
            var result = field1 < field2 ? -1 : (field1 > field2 ? 1 : 0);
            if (!sortFields[1])
                result = -result;
            if (result !== 0)
                return result;
            field1 = nodeA[sortFields[2]];
            field2 = nodeB[sortFields[2]];
            result = field1 < field2 ? -1 : (field1 > field2 ? 1 : 0);
            if (!sortFields[3])
                result = -result;
            return result;
        }
        this._performSorting(SortByTwoFields);
    },

    _performSorting: function(sortFunction)
    {
        this.recursiveSortingEnter();
        var children = this._topLevelNodes;
        this.rootNode().removeChildren();
        children.sort(sortFunction);
        for (var i = 0, l = children.length; i < l; ++i) {
            var child = children[i];
            this.appendChildAfterSorting(child);
            if (child.expanded)
                child.sort();
        }
        this.updateVisibleNodes();
        this.recursiveSortingLeave();
    },

    appendChildAfterSorting: function(child)
    {
        var revealed = child.revealed;
        this.rootNode().appendChild(child);
        child.revealed = revealed;
    },

    updateVisibleNodes: function()
    {
    },

    recursiveSortingEnter: function()
    {
        ++this._recursiveSortingDepth;
    },

    recursiveSortingLeave: function()
    {
        if (!this._recursiveSortingDepth)
            return;
        if (!--this._recursiveSortingDepth)
            this.dispatchEventToListeners("sorting complete");
    },

    __proto__: WebInspector.DataGrid.prototype
}



/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotSortableDataGrid}
 */
WebInspector.HeapSnapshotViewportDataGrid = function(columns)
{
    WebInspector.HeapSnapshotSortableDataGrid.call(this, columns);
    this.scrollContainer.addEventListener("scroll", this._onScroll.bind(this), true);
    this._topLevelNodes = [];
    this._topPadding = new WebInspector.HeapSnapshotPaddingNode();
    this._bottomPadding = new WebInspector.HeapSnapshotPaddingNode();
    /**
     * @type {WebInspector.HeapSnapshotGridNode}
     */
    this._nodeToHighlightAfterScroll = null;
}

WebInspector.HeapSnapshotViewportDataGrid.prototype = {
    topLevelNodes: function()
    {
        return this._topLevelNodes;
    },

    appendChildAfterSorting: function(child)
    {
        // Do nothing here, it will be added in updateVisibleNodes.
    },

    updateVisibleNodes: function()
    {
        var scrollTop = this.scrollContainer.scrollTop;

        var viewPortHeight = this.scrollContainer.offsetHeight;

        this._removePaddingRows();

        var children = this._topLevelNodes;

        var i = 0;
        var topPadding = 0;
        while (i < children.length) {
            if (children[i].revealed) {
                var newTop = topPadding + children[i].nodeHeight();
                if (newTop > scrollTop)
                    break;
                topPadding = newTop;
            }
            ++i;
        }

        this.rootNode().removeChildren();
        // The height of the view port + invisible top part.
        var heightToFill = viewPortHeight + (scrollTop - topPadding);
        var filledHeight = 0;
        while (i < children.length && filledHeight < heightToFill) {
            if (children[i].revealed) {
                this.rootNode().appendChild(children[i]);
                filledHeight += children[i].nodeHeight();
            }
            ++i;
        }

        var bottomPadding = 0;
        while (i < children.length) {
            bottomPadding += children[i].nodeHeight();
            ++i;
        }

        this._addPaddingRows(topPadding, bottomPadding);
    },

    appendTopLevelNode: function(node)
    {
        this._topLevelNodes.push(node);
    },

    removeTopLevelNodes: function()
    {
        this.rootNode().removeChildren();
        this._topLevelNodes = [];
    },

    /**
     * @override
     * @param {WebInspector.HeapSnapshotGridNode} node
     */
    highlightNode: function(node)
    {
        if (this._isScrolledIntoView(node.element))
            WebInspector.HeapSnapshotSortableDataGrid.prototype.highlightNode.call(this, node);
        else {
            node.element.scrollIntoViewIfNeeded(true);
            this._nodeToHighlightAfterScroll = node;
        }
    },

    _isScrolledIntoView: function(element)
    {
        var viewportTop = this.scrollContainer.scrollTop;
        var viewportBottom = viewportTop + this.scrollContainer.clientHeight;
        var elemTop = element.offsetTop
        var elemBottom = elemTop + element.offsetHeight;
        return elemBottom <= viewportBottom && elemTop >= viewportTop;
    },

    _addPaddingRows: function(top, bottom)
    {
        if (this._topPadding.element.parentNode !== this.dataTableBody)
            this.dataTableBody.insertBefore(this._topPadding.element, this.dataTableBody.firstChild);
        if (this._bottomPadding.element.parentNode !== this.dataTableBody)
            this.dataTableBody.insertBefore(this._bottomPadding.element, this.dataTableBody.lastChild);
        this._topPadding.setHeight(top);
        this._bottomPadding.setHeight(bottom);
    },

    _removePaddingRows: function()
    {
        this._bottomPadding.removeFromTable();
        this._topPadding.removeFromTable();
    },

    onResize: function()
    {
        WebInspector.HeapSnapshotSortableDataGrid.prototype.onResize.call(this);
        this.updateVisibleNodes();
    },

    _onScroll: function(event)
    {
        this.updateVisibleNodes();

        if (this._nodeToHighlightAfterScroll) {
            WebInspector.HeapSnapshotSortableDataGrid.prototype.highlightNode.call(this, this._nodeToHighlightAfterScroll);
            this._nodeToHighlightAfterScroll = null;
        }
    },

    __proto__: WebInspector.HeapSnapshotSortableDataGrid.prototype
}

/**
 * @constructor
 */
WebInspector.HeapSnapshotPaddingNode = function()
{
    this.element = document.createElement("tr");
    this.element.addStyleClass("revealed");
}

WebInspector.HeapSnapshotPaddingNode.prototype = {
   setHeight: function(height)
   {
       this.element.style.height = height + "px";
   },
   removeFromTable: function()
   {
        var parent = this.element.parentNode;
        if (parent)
            parent.removeChild(this.element);
   }
}


/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotSortableDataGrid}
 * @param {Array.<!WebInspector.DataGrid.ColumnDescriptor>=} columns
 */
WebInspector.HeapSnapshotContainmentDataGrid = function(columns)
{
    columns = columns || [
        {id: "object", title: WebInspector.UIString("Object"), disclosure: true, sortable: true},
        {id: "shallowSize", title: WebInspector.UIString("Shallow Size"), width: "120px", sortable: true},
        {id: "retainedSize", title: WebInspector.UIString("Retained Size"), width: "120px", sortable: true, sort: WebInspector.DataGrid.Order.Descending}
    ];
    WebInspector.HeapSnapshotSortableDataGrid.call(this, columns);
}

WebInspector.HeapSnapshotContainmentDataGrid.prototype = {
    setDataSource: function(snapshot, nodeIndex)
    {
        this.snapshot = snapshot;
        var node = new WebInspector.HeapSnapshotNode(snapshot, nodeIndex || snapshot.rootNodeIndex);
        var fakeEdge = { node: node };
        this.setRootNode(new WebInspector.HeapSnapshotObjectNode(this, false, fakeEdge, null));
        this.rootNode().sort();
    },

    sortingChanged: function()
    {
        this.rootNode().sort();
    },

    __proto__: WebInspector.HeapSnapshotSortableDataGrid.prototype
}


/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotContainmentDataGrid}
 */
WebInspector.HeapSnapshotRetainmentDataGrid = function()
{
    this.showRetainingEdges = true;
    var columns = [
        {id: "object", title: WebInspector.UIString("Object"), disclosure: true, sortable: true},
        {id: "shallowSize", title: WebInspector.UIString("Shallow Size"), width: "120px", sortable: true},
        {id: "retainedSize", title: WebInspector.UIString("Retained Size"), width: "120px", sortable: true},
        {id: "distance", title: WebInspector.UIString("Distance"), width: "80px", sortable: true, sort: WebInspector.DataGrid.Order.Ascending}
    ];
    WebInspector.HeapSnapshotContainmentDataGrid.call(this, columns);
}

WebInspector.HeapSnapshotRetainmentDataGrid.Events = {
    ExpandRetainersComplete: "ExpandRetainersComplete"
}

WebInspector.HeapSnapshotRetainmentDataGrid.prototype = {
    _sortFields: function(sortColumn, sortAscending)
    {
        return {
            object: ["_name", sortAscending, "_count", false],
            count: ["_count", sortAscending, "_name", true],
            shallowSize: ["_shallowSize", sortAscending, "_name", true],
            retainedSize: ["_retainedSize", sortAscending, "_name", true],
            distance: ["_distance", sortAscending, "_name", true]
        }[sortColumn];
    },

    reset: function()
    {
        this.rootNode().removeChildren();
        this.resetSortingCache();
    },

    /**
     * @param {!WebInspector.HeapSnapshotProxy} snapshot
     * @param {number} nodeIndex
     */
    setDataSource: function(snapshot, nodeIndex)
    {
        WebInspector.HeapSnapshotContainmentDataGrid.prototype.setDataSource.call(this, snapshot, nodeIndex);

        var dataGrid = this;
        var maxExpandLevels = 20;
        /**
         * @this {!WebInspector.HeapSnapshotObjectNode}
         */
        function populateComplete()
        {
            this.removeEventListener(WebInspector.HeapSnapshotGridNode.Events.PopulateComplete, populateComplete, this);
            this.expand();
            if (this.children.length === 1 && --maxExpandLevels > 0) {
                var child = this.children[0];
                child.addEventListener(WebInspector.HeapSnapshotGridNode.Events.PopulateComplete, populateComplete, child);
                child.populate();
            } else
                dataGrid.dispatchEventToListeners(WebInspector.HeapSnapshotRetainmentDataGrid.Events.ExpandRetainersComplete);
        }
        this.rootNode().addEventListener(WebInspector.HeapSnapshotGridNode.Events.PopulateComplete, populateComplete, this.rootNode());
    },

    __proto__: WebInspector.HeapSnapshotContainmentDataGrid.prototype
}


/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotViewportDataGrid}
 */
WebInspector.HeapSnapshotConstructorsDataGrid = function()
{
    var columns = [
        {id: "object", title: WebInspector.UIString("Constructor"), disclosure: true, sortable: true},
        {id: "distance", title: WebInspector.UIString("Distance"), width: "90px", sortable: true},
        {id: "count", title: WebInspector.UIString("Objects Count"), width: "90px", sortable: true},
        {id: "shallowSize", title: WebInspector.UIString("Shallow Size"), width: "120px", sortable: true},
        {id: "retainedSize", title: WebInspector.UIString("Retained Size"), width: "120px", sort: WebInspector.DataGrid.Order.Descending, sortable: true}
    ];
    WebInspector.HeapSnapshotViewportDataGrid.call(this, columns);
    this._profileIndex = -1;
    this._topLevelNodes = [];

    this._objectIdToSelect = null;
}

WebInspector.HeapSnapshotConstructorsDataGrid.prototype = {
    _sortFields: function(sortColumn, sortAscending)
    {
        return {
            object: ["_name", sortAscending, "_count", false],
            distance: ["_distance", sortAscending, "_retainedSize", true],
            count: ["_count", sortAscending, "_name", true],
            shallowSize: ["_shallowSize", sortAscending, "_name", true],
            retainedSize: ["_retainedSize", sortAscending, "_name", true]
        }[sortColumn];
    },

    /**
     * @override
     * @param {HeapProfilerAgent.HeapSnapshotObjectId} id
     */
    highlightObjectByHeapSnapshotId: function(id)
    {
        if (!this.snapshot) {
            this._objectIdToSelect = id;
            return;
        }

        function didGetClassName(className)
        {
            var constructorNodes = this.topLevelNodes();
            for (var i = 0; i < constructorNodes.length; i++) {
                var parent = constructorNodes[i];
                if (parent._name === className) {
                    parent.revealNodeBySnapshotObjectId(parseInt(id, 10));
                    return;
                }
            }
        }
        this.snapshot.nodeClassName(parseInt(id, 10), didGetClassName.bind(this));
    },

    setDataSource: function(snapshot)
    {
        this.snapshot = snapshot;
        if (this._profileIndex === -1)
            this._populateChildren();

        if (this._objectIdToSelect) {
            this.highlightObjectByHeapSnapshotId(this._objectIdToSelect);
            this._objectIdToSelect = null;
        }
    },

    _aggregatesReceived: function(key, aggregates)
    {
        for (var constructor in aggregates)
            this.appendTopLevelNode(new WebInspector.HeapSnapshotConstructorNode(this, constructor, aggregates[constructor], key));
        this.sortingChanged();
    },

    _populateChildren: function()
    {

        this.dispose();
        this.removeTopLevelNodes();
        this.resetSortingCache();

        var key = this._profileIndex === -1 ? "allObjects" : this._minNodeId + ".." + this._maxNodeId;
        var filter = this._profileIndex === -1 ? null : "function(node) { var id = node.id(); return id > " + this._minNodeId + " && id <= " + this._maxNodeId + "; }";

        this.snapshot.aggregates(false, key, filter, this._aggregatesReceived.bind(this, key));
    },

    filterSelectIndexChanged: function(profiles, profileIndex)
    {
        this._profileIndex = profileIndex;

        delete this._maxNodeId;
        delete this._minNodeId;

        if (this._profileIndex !== -1) {
            this._minNodeId = profileIndex > 0 ? profiles[profileIndex - 1].maxJSObjectId : 0;
            this._maxNodeId = profiles[profileIndex].maxJSObjectId;
        }

        this._populateChildren();
    },

    __proto__: WebInspector.HeapSnapshotViewportDataGrid.prototype
}


/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotViewportDataGrid}
 */
WebInspector.HeapSnapshotDiffDataGrid = function()
{
    var columns = [
        {id: "object", title: WebInspector.UIString("Constructor"), disclosure: true, sortable: true},
        {id: "addedCount", title: WebInspector.UIString("# New"), width: "72px", sortable: true},
        {id: "removedCount", title: WebInspector.UIString("# Deleted"), width: "72px", sortable: true},
        {id: "countDelta", title: "# Delta", width: "64px", sortable: true},
        {id: "addedSize", title: WebInspector.UIString("Alloc. Size"), width: "72px", sortable: true, sort: WebInspector.DataGrid.Order.Descending},
        {id: "removedSize", title: WebInspector.UIString("Freed Size"), width: "72px", sortable: true},
        {id: "sizeDelta", title: "Size Delta", width: "72px", sortable: true}
    ];
    WebInspector.HeapSnapshotViewportDataGrid.call(this, columns);
}

WebInspector.HeapSnapshotDiffDataGrid.prototype = {
    /**
     * @override
     * @return {number}
     */
    defaultPopulateCount: function()
    {
        return 50;
    },

    _sortFields: function(sortColumn, sortAscending)
    {
        return {
            object: ["_name", sortAscending, "_count", false],
            addedCount: ["_addedCount", sortAscending, "_name", true],
            removedCount: ["_removedCount", sortAscending, "_name", true],
            countDelta: ["_countDelta", sortAscending, "_name", true],
            addedSize: ["_addedSize", sortAscending, "_name", true],
            removedSize: ["_removedSize", sortAscending, "_name", true],
            sizeDelta: ["_sizeDelta", sortAscending, "_name", true]
        }[sortColumn];
    },

    setDataSource: function(snapshot)
    {
        this.snapshot = snapshot;
    },

    /**
     * @param {WebInspector.HeapSnapshotProxy} baseSnapshot
     */
    setBaseDataSource: function(baseSnapshot)
    {
        this.baseSnapshot = baseSnapshot;
        this.dispose();
        this.removeTopLevelNodes();
        this.resetSortingCache();
        if (this.baseSnapshot === this.snapshot) {
            this.dispatchEventToListeners("sorting complete");
            return;
        }
        this._populateChildren();
    },

    _populateChildren: function()
    {
        function aggregatesForDiffReceived(aggregatesForDiff)
        {
            this.snapshot.calculateSnapshotDiff(this.baseSnapshot.uid, aggregatesForDiff, didCalculateSnapshotDiff.bind(this));
            function didCalculateSnapshotDiff(diffByClassName)
            {
                for (var className in diffByClassName) {
                    var diff = diffByClassName[className];
                    this.appendTopLevelNode(new WebInspector.HeapSnapshotDiffNode(this, className, diff));
                }
                this.sortingChanged();
            }
        }
        // Two snapshots live in different workers isolated from each other. That is why
        // we first need to collect information about the nodes in the first snapshot and
        // then pass it to the second snapshot to calclulate the diff.
        this.baseSnapshot.aggregatesForDiff(aggregatesForDiffReceived.bind(this));
    },

    __proto__: WebInspector.HeapSnapshotViewportDataGrid.prototype
}


/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotSortableDataGrid}
 */
WebInspector.HeapSnapshotDominatorsDataGrid = function()
{
    var columns = [
        {id: "object", title: WebInspector.UIString("Object"), disclosure: true, sortable: true},
        {id: "shallowSize", title: WebInspector.UIString("Shallow Size"), width: "120px", sortable: true},
        {id: "retainedSize", title: WebInspector.UIString("Retained Size"), width: "120px", sort: WebInspector.DataGrid.Order.Descending, sortable: true}
    ];
    WebInspector.HeapSnapshotSortableDataGrid.call(this, columns);
    this._objectIdToSelect = null;
}

WebInspector.HeapSnapshotDominatorsDataGrid.prototype = {
    /**
     * @override
     * @return {number}
     */
    defaultPopulateCount: function()
    {
        return 25;
    },

    setDataSource: function(snapshot)
    {
        this.snapshot = snapshot;

        var fakeNode = { nodeIndex: this.snapshot.rootNodeIndex };
        this.setRootNode(new WebInspector.HeapSnapshotDominatorObjectNode(this, fakeNode));
        this.rootNode().sort();

        if (this._objectIdToSelect) {
            this.highlightObjectByHeapSnapshotId(this._objectIdToSelect);
            this._objectIdToSelect = null;
        }
    },

    sortingChanged: function()
    {
        this.rootNode().sort();
    },

    /**
     * @override
     * @param {HeapProfilerAgent.HeapSnapshotObjectId} id
     */
    highlightObjectByHeapSnapshotId: function(id)
    {
        if (!this.snapshot) {
            this._objectIdToSelect = id;
            return;
        }

        function didGetDominators(dominatorIds)
        {
            if (!dominatorIds) {
                WebInspector.log(WebInspector.UIString("Cannot find corresponding heap snapshot node"));
                return;
            }
            var dominatorNode = this.rootNode();
            expandNextDominator.call(this, dominatorIds, dominatorNode);
        }

        function expandNextDominator(dominatorIds, dominatorNode)
        {
            if (!dominatorNode) {
                console.error("Cannot find dominator node");
                return;
            }
            if (!dominatorIds.length) {
                this.highlightNode(dominatorNode);
                dominatorNode.element.scrollIntoViewIfNeeded(true);
                return;
            }
            var snapshotObjectId = dominatorIds.pop();
            dominatorNode.retrieveChildBySnapshotObjectId(snapshotObjectId, expandNextDominator.bind(this, dominatorIds));
        }

        this.snapshot.dominatorIdsForNode(parseInt(id, 10), didGetDominators.bind(this));
    },

    __proto__: WebInspector.HeapSnapshotSortableDataGrid.prototype
}

