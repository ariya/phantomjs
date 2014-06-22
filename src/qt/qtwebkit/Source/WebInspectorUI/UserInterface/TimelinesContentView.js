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

WebInspector.TimelinesContentView = function(representedObject)
{
    WebInspector.ContentView.call(this, representedObject);

    this.element.classList.add(WebInspector.TimelinesContentView.StyleClassName);

    this._timelineOverview = new WebInspector.TimelineOverview(this, [WebInspector.TimelineRecord.Type.Network, WebInspector.TimelineRecord.Type.Layout, WebInspector.TimelineRecord.Type.Script]);
    this.element.appendChild(this._timelineOverview.element);

    function createPathComponent(displayName, className, representedObject)
    {
        var pathComponent = new WebInspector.HierarchicalPathComponent(displayName, className, representedObject);
        pathComponent.addEventListener(WebInspector.HierarchicalPathComponent.Event.SiblingWasSelected, this._pathComponentSelected, this);
        return pathComponent;
    }

    var networkPathComponent = createPathComponent.call(this, WebInspector.UIString("Network Requests"), WebInspector.InstrumentSidebarPanel.NetworkIconStyleClass, WebInspector.TimelineRecord.Type.Network);
    var layoutPathComponent = createPathComponent.call(this, WebInspector.UIString("Layout & Rendering"), WebInspector.InstrumentSidebarPanel.ColorsIconStyleClass, WebInspector.TimelineRecord.Type.Layout);
    var scriptPathComponent = createPathComponent.call(this, WebInspector.UIString("JavaScript & Events"), WebInspector.InstrumentSidebarPanel.ScriptIconStyleClass, WebInspector.TimelineRecord.Type.Script);

    networkPathComponent.nextSibling = layoutPathComponent;
    layoutPathComponent.previousSibling = networkPathComponent;
    layoutPathComponent.nextSibling = scriptPathComponent;
    scriptPathComponent.previousSibling = layoutPathComponent;

    this._currentRecordType = null;
    this._currentRecordTypeSetting = new WebInspector.Setting("timeline-view-current-record-type", WebInspector.TimelineRecord.Type.Network);
    this._currentDataGrid = null;

    var networkDataGridColumns = {name: {}, domain: {}, type: {}, statusCode: {}, cached: {}, size: {}, transferSize: {}, latency: {}, duration: {}, timeline: {}};

    networkDataGridColumns.name.title = WebInspector.UIString("Name");
    networkDataGridColumns.name.width = "15%";

    networkDataGridColumns.domain.title = WebInspector.UIString("Domain");
    networkDataGridColumns.domain.width = "10%";
    networkDataGridColumns.domain.group = WebInspector.NetworkDataGrid.DetailsColumnGroup;

    networkDataGridColumns.type.title = WebInspector.UIString("Type");
    networkDataGridColumns.type.width = "8%";
    networkDataGridColumns.type.group = WebInspector.NetworkDataGrid.DetailsColumnGroup;
    networkDataGridColumns.type.scopeBar = this._makeColumnScopeBar("network", WebInspector.Resource.Type);

    networkDataGridColumns.statusCode.title = WebInspector.UIString("Status");
    networkDataGridColumns.statusCode.width = "6%";
    networkDataGridColumns.statusCode.group = WebInspector.NetworkDataGrid.DetailsColumnGroup;

    networkDataGridColumns.cached.title = WebInspector.UIString("Cached");
    networkDataGridColumns.cached.width = "7%";
    networkDataGridColumns.cached.group = WebInspector.NetworkDataGrid.DetailsColumnGroup;

    networkDataGridColumns.size.title = WebInspector.UIString("Size");
    networkDataGridColumns.size.width = "8%";
    networkDataGridColumns.size.aligned = "right";
    networkDataGridColumns.size.group = WebInspector.NetworkDataGrid.DetailsColumnGroup;

    networkDataGridColumns.transferSize.title = WebInspector.UIString("Transferred");
    networkDataGridColumns.transferSize.width = "8%";
    networkDataGridColumns.transferSize.aligned = "right";
    networkDataGridColumns.transferSize.group = WebInspector.NetworkDataGrid.DetailsColumnGroup;

    networkDataGridColumns.latency.title = WebInspector.UIString("Latency");
    networkDataGridColumns.latency.width = "9%";
    networkDataGridColumns.latency.aligned = "right";
    networkDataGridColumns.latency.group = WebInspector.NetworkDataGrid.DetailsColumnGroup;

    networkDataGridColumns.duration.title = WebInspector.UIString("Duration");
    networkDataGridColumns.duration.width = "9%";
    networkDataGridColumns.duration.aligned = "right";
    networkDataGridColumns.duration.group = WebInspector.NetworkDataGrid.DetailsColumnGroup;

    networkDataGridColumns.timeline.title = WebInspector.UIString("Timeline");
    networkDataGridColumns.timeline.width = "20%";
    networkDataGridColumns.timeline.sort = "ascending";
    networkDataGridColumns.timeline.collapsesGroup = WebInspector.NetworkDataGrid.DetailsColumnGroup;

    var layoutDataGridColumns = {eventType: {}, initiatorCallFrame: {}, width: {}, height: {}, area: {}, startTime: {}, duration: {}};

    layoutDataGridColumns.eventType.title = WebInspector.UIString("Type");
    layoutDataGridColumns.eventType.width = "15%";
    layoutDataGridColumns.eventType.scopeBar = this._makeColumnScopeBar("layout", WebInspector.LayoutTimelineRecord.EventType);

    layoutDataGridColumns.initiatorCallFrame.title = WebInspector.UIString("Initiator");
    layoutDataGridColumns.initiatorCallFrame.width = "25%";

    layoutDataGridColumns.width.title = WebInspector.UIString("Width");
    layoutDataGridColumns.width.width = "8%";

    layoutDataGridColumns.height.title = WebInspector.UIString("Height");
    layoutDataGridColumns.height.width = "8%";

    layoutDataGridColumns.area.title = WebInspector.UIString("Area");
    layoutDataGridColumns.area.width = "12%";

    layoutDataGridColumns.startTime.title = WebInspector.UIString("Start Time");
    layoutDataGridColumns.startTime.width = "8%";
    layoutDataGridColumns.startTime.aligned = "right";
    layoutDataGridColumns.startTime.sort = "ascending";

    layoutDataGridColumns.duration.title = WebInspector.UIString("Duration");
    layoutDataGridColumns.duration.width = "8%";
    layoutDataGridColumns.duration.aligned = "right";

    var scriptDataGridColumns = {eventType: {}, details: {}, resource: {}, startTime: {}, duration: {}};

    scriptDataGridColumns.eventType.title = WebInspector.UIString("Type");
    scriptDataGridColumns.eventType.width = "15%";
    scriptDataGridColumns.eventType.scopeBar = this._makeColumnScopeBar("script", WebInspector.ScriptTimelineRecord.EventType);

    scriptDataGridColumns.details.title = WebInspector.UIString("Details");
    scriptDataGridColumns.details.width = "15%";

    scriptDataGridColumns.resource.title = WebInspector.UIString("Location");
    scriptDataGridColumns.resource.width = "15%";

    scriptDataGridColumns.startTime.title = WebInspector.UIString("Start Time");
    scriptDataGridColumns.startTime.width = "10%";
    scriptDataGridColumns.startTime.aligned = "right";
    scriptDataGridColumns.startTime.sort = "ascending";

    scriptDataGridColumns.duration.title = WebInspector.UIString("Duration");
    scriptDataGridColumns.duration.width = "10%";
    scriptDataGridColumns.duration.aligned = "right";

    for (var column in networkDataGridColumns)
        networkDataGridColumns[column].sortable = true;

    for (var column in layoutDataGridColumns)
        layoutDataGridColumns[column].sortable = true;

    for (var column in scriptDataGridColumns)
        scriptDataGridColumns[column].sortable = true;

    var networkDataGrid = new WebInspector.NetworkDataGrid(networkDataGridColumns);
    var layoutDataGrid = new WebInspector.LayoutTimelineDataGrid(layoutDataGridColumns);
    var scriptDataGrid = new WebInspector.ScriptTimelineDataGrid(scriptDataGridColumns);

    networkDataGrid.addEventListener(WebInspector.DataGrid.Event.SelectedNodeChanged, this._selectedNodeChanged, this);

    this._pathComponentMap = {};
    this._pathComponentMap[WebInspector.TimelineRecord.Type.Network] = networkPathComponent;
    this._pathComponentMap[WebInspector.TimelineRecord.Type.Layout] = layoutPathComponent;
    this._pathComponentMap[WebInspector.TimelineRecord.Type.Script] = scriptPathComponent;

    this._dataGridMap = {};
    this._dataGridMap[WebInspector.TimelineRecord.Type.Network] = networkDataGrid;
    this._dataGridMap[WebInspector.TimelineRecord.Type.Layout] = layoutDataGrid;
    this._dataGridMap[WebInspector.TimelineRecord.Type.Script] = scriptDataGrid;

    for (var type in this._dataGridMap) {
        var dataGrid = this._dataGridMap[type];
        dataGrid.addEventListener(WebInspector.DataGrid.Event.SortChanged, this._sortCurrentDataGrid, this);
        dataGrid.addEventListener(WebInspector.TimelineDataGrid.Event.FiltersDidChange, this._dataGridFiltersDidChange, this);
        dataGrid.scrollContainer.addEventListener("scroll", this._updateOffscreenRows.bind(this));
    }

    this._pendingRecords = {};

    for (var typeName in WebInspector.TimelineRecord.Type) {
        var type = WebInspector.TimelineRecord.Type[typeName];
        this._pendingRecords[type] = WebInspector.timelineManager.recordsWithType(type);
    }

    this._pendingRefreshGridNodes = {};
    this._pendingEventMarkers = {};

    WebInspector.ResourceTimelineDataGridNode.addEventListener(WebInspector.ResourceTimelineDataGridNode.Event.NeedsRefresh, this._scheduleGridNodeForRefresh, this);
    WebInspector.timelineManager.addEventListener(WebInspector.TimelineManager.Event.RecordsCleared, this._recordsCleared, this);
    WebInspector.timelineManager.addEventListener(WebInspector.TimelineManager.Event.RecordAdded, this._recordAdded, this);
    WebInspector.timelineManager.addEventListener(WebInspector.TimelineManager.Event.RecordedEventMarker, this._recordedEventMarker, this);

    WebInspector.TimelinesContentView.generateEmbossedCollapseImages();
};

WebInspector.TimelinesContentView.StyleClassName = "timelines";
WebInspector.TimelinesContentView.OffscreenDataGridRowStyleClassName = "offscreen";
WebInspector.TimelinesContentView.UpdateInterval = 500; // 0.5 seconds

WebInspector.TimelinesContentView.CollapseButton = {};
WebInspector.TimelinesContentView.CollapseButton.More = "more";
WebInspector.TimelinesContentView.CollapseButton.Less = "less";

WebInspector.TimelinesContentView.CollapseButton.States = {};
WebInspector.TimelinesContentView.CollapseButton.States.Normal = "normal";
WebInspector.TimelinesContentView.CollapseButton.States.Active = "active";

WebInspector.TimelinesContentView.generateEmbossedCollapseImages = function()
{
    if (WebInspector.TimelinesContentView._generatedImages)
        return;
    WebInspector.TimelinesContentView._generatedImages = true;

    generateEmbossedImages("Images/MoreColumns.pdf", 15, 13, WebInspector.TimelinesContentView.CollapseButton.States, canvasIdentifier.bind(this, WebInspector.TimelinesContentView.CollapseButton.More));
    generateEmbossedImages("Images/LessColumns.pdf", 15, 13, WebInspector.TimelinesContentView.CollapseButton.States, canvasIdentifier.bind(this, WebInspector.TimelinesContentView.CollapseButton.Less));

    function canvasIdentifier(type, state) {
        return "timeline-datagrid-collapse-button-" + type + "-" + state;
    }
}

WebInspector.TimelinesContentView.prototype = {
    constructor: WebInspector.TimelinesContentView,

    // Public

    get allowedNavigationSidebarPanels()
    {
        return ["instrument"];
    },

    showTimelineForRecordType: function(type)
    {
        if (this._currentRecordType === type)
            return;

        this._currentRecordType = type;
        this._currentRecordTypeSetting.value = type;

        if (this._currentDataGrid) {
            // Save scroll positon before removing from the document.
            if (this._currentDataGrid.isScrolledToLastRow()) {
                this._currentDataGrid._savedIsScrolledToLastRow = true;
                delete this._currentDataGrid._savedScrollTop;
            } else {
                this._currentDataGrid._savedScrollTop = this._currentDataGrid.scrollContainer.scrollTop;
                delete this._currentDataGrid._savedIsScrolledToLastRow;
            }

            this.element.removeChild(this._currentDataGrid.element);
            this._currentDataGrid.hidden();
        }

        this._currentDataGrid = this._dataGridMap[type];
        console.assert(this._currentDataGrid);

        this.element.appendChild(this._currentDataGrid.element);
        this._currentDataGrid.updateLayout();
        this._currentDataGrid.shown();

        // Restore scroll positon now that we are back in the document.
        if (this._currentDataGrid._savedIsScrolledToLastRow)
            this._currentDataGrid.scrollToLastRow()
        else if (this._currentDataGrid._savedScrollTop)
            this._currentDataGrid.scrollContainer.scrollTop = this._currentDataGrid._savedScrollTop;

        this._updatePendingRecords();

        this.dispatchEventToListeners(WebInspector.ContentView.Event.SelectionPathComponentsDidChange);
        WebInspector.instrumentSidebarPanel.showTimelineForRecordType(type);
    },

    get supportsSplitContentBrowser()
    {
        // The layout of the overview and split content browser don't work well.
        return false;
    },

    get selectionPathComponents()
    {
        var pathComponents = [this._pathComponentMap[this._currentRecordType]] || [];

        if (this._currentDataGrid) {
            var selectedNode = this._currentDataGrid.selectedNode;
            if (selectedNode instanceof WebInspector.ResourceTimelineDataGridNode) {
                var pathComponent = new WebInspector.ResourceTimelineDataGridNodePathComponent(selectedNode);
                pathComponent.addEventListener(WebInspector.HierarchicalPathComponent.Event.SiblingWasSelected, this._dataGridNodePathComponentSelected, this);
                pathComponents.push(pathComponent);
            }
        }

        return pathComponents;
    },

    updateLayout: function()
    {
        if (this._currentDataGrid)
            this._currentDataGrid.updateLayout();
        this._timelineOverview.updateLayout();
        this._updateOffscreenRows();
    },

    get scrollableElements()
    {
        if (!this._currentDataGrid)
            return null;
        return [this._currentDataGrid.scrollContainer];
    },

    get shouldKeepElementsScrolledToBottom()
    {
        return true;
    },

    shown: function()
    {
        if (this._currentRecordType) {
            this._updatePendingRecords();
            return;
        }

        if (this._currentDataGrid)
            this._currentDataGrid.shown();

        this.showTimelineForRecordType(this._currentRecordTypeSetting.value);
    },

    hidden: function()
    {
        if (this._currentDataGrid)
            this._currentDataGrid.hidden();
    },

    timelineOverviewRecordsWithType: function(type)
    {
        return this._filterRecordsWithType(WebInspector.timelineManager.recordsWithType(type), type);
    },

    // Private

    _pathComponentSelected: function(event)
    {
        this.showTimelineForRecordType(event.data.pathComponent.representedObject);
    },

    _dataGridNodePathComponentSelected: function(event)
    {
        console.assert(event.data.pathComponent instanceof WebInspector.ResourceTimelineDataGridNodePathComponent);
        if (!(event.data.pathComponent instanceof WebInspector.ResourceTimelineDataGridNodePathComponent))
            return;

        event.data.pathComponent.resourceTimelineDataGridNode.revealAndSelect();
        event.data.pathComponent.resourceTimelineDataGridNode.dataGrid.element.focus();
    },

    _selectedNodeChanged: function(event)
    {
        if (this._ignoreSelectionEvent)
            return;

        this.dispatchEventToListeners(WebInspector.ContentView.Event.SelectionPathComponentsDidChange);
    },

    _recordsCleared: function(event)
    {
        this._pendingRecords = {};
        this._pendingRefreshGridNodes = {};
        this._pendingEventMarkers = {};

        for (var type in this._dataGridMap) {
            var dataGrid = this._dataGridMap[type];
            delete dataGrid._savedIsScrolledToLastRow;
            delete dataGrid._savedScrollTop;
            dataGrid.removeChildren();
            dataGrid.reset();
        }

        this._timelineOverview.clear();
    },

    _recordAdded: function(event)
    {
        this._addRecordToDataGrid(event.data.record);
    },

    _recordedEventMarker: function(event)
    {
        var eventMarker = event.data.eventMarker;
        this._timelineOverview.addTimelineEventMarker(eventMarker);

        for (var type in this._dataGridMap) {
            if (!this._pendingEventMarkers[type])
                this._pendingEventMarkers[type] = [];
            this._pendingEventMarkers[type].push(eventMarker);

            this._dataGridMap[type].addTimelineEventMarker(eventMarker);
        }

        this._updatePendingRecordsSoon();
    },

    _addRecordToDataGrid: function(record)
    {
        if (!this._pendingRecords[record.type])
            this._pendingRecords[record.type] = [];
        this._pendingRecords[record.type].push(record);

        this._updatePendingRecordsSoon();
    },

    _updatePendingRecordsSoon: function()
    {
        if (!this.visible || this._updatePendingRecordsTimeout)
            return;

        this._updatePendingRecordsTimeout = setTimeout(this._updatePendingRecords.bind(this), WebInspector.TimelinesContentView.UpdateInterval);
    },

    _updatePendingRecords: function()
    {
        if (this._updatePendingRecordsTimeout) {
            clearTimeout(this._updatePendingRecordsTimeout);
            delete this._updatePendingRecordsTimeout;
        }

        this._timelineOverview.update();

        console.assert(this._dataGridMap[this._currentRecordType] === this._currentDataGrid);
        if (this._dataGridMap[this._currentRecordType] !== this._currentDataGrid)
            return;

        var isScrolledToLastRow = this._currentDataGrid.isScrolledToLastRow();

        // If the data grid has a timeline calculator, pass through all the pending records first
        // to update the timeline bounds. If the bounds change we need to refresh all nodes
        // in the data grid, otherwise we can just proceed and update the pending records.
        var wasBoundsChange = this._updateCalculatorBoundsForPendingRecordsAndEventMarkers();
        this._updatePendingRecordsWithNewBounds(wasBoundsChange);

        this._updateOffscreenRows();

        this._currentDataGrid.update();
        if (isScrolledToLastRow)
            this._currentDataGrid.scrollToLastRow();
    },
    
    _updateCalculatorBoundsForPendingRecordsAndEventMarkers: function()
    {
        var currentDataGrid = this._currentDataGrid;
        if (!currentDataGrid.currentCalculator)
            return false;

        var wasBoundsChange = false;

        var pendingRecords = this._pendingRecords[this._currentRecordType];
        if (pendingRecords) {
            for (var i = 0; i < pendingRecords.length; ++i) {
                if (currentDataGrid.updateCalculatorBoundariesWithRecord(pendingRecords[i]))
                    wasBoundsChange = true;
            }
        }

        var pendingRefreshGridNodes = this._pendingRefreshGridNodes[this._currentRecordType];
        if (pendingRefreshGridNodes) {
            for (var i = 0; i < pendingRefreshGridNodes.length; ++i) {
                if (currentDataGrid.updateCalculatorBoundariesWithDataGridNode(pendingRefreshGridNodes[i]))
                    wasBoundsChange = true;
            }
        }

        var pendingEventMarkers = this._pendingEventMarkers[this._currentRecordType];
        if (pendingEventMarkers) {
            delete this._pendingEventMarkers[this._currentRecordType];

            for (var i = 0; i < pendingEventMarkers.length; ++i) {
                if (currentDataGrid.updateCalculatorBoundariesWithEventMarker(pendingEventMarkers[i]))
                    wasBoundsChange = true;
            }
        }

        return wasBoundsChange;
    },

    _updatePendingRecordsWithNewBounds: function(refreshAllNodes)
    {
        var sortComparator = this._sortComparator.bind(this);

        var pendingRecords = this._pendingRecords[this._currentRecordType];
        if (pendingRecords) {
            delete this._pendingRecords[this._currentRecordType];
            pendingRecords = this._filterRecordsWithType(pendingRecords, this._currentRecordType);

            for (var i = 0; i < pendingRecords.length; ++i) {
                var dataGridNode = this._createDataGridNodeForRecord(pendingRecords[i]);
                if (this._currentDataGrid.sortColumnIdentifier) {
                    var insertionIndex = insertionIndexForObjectInListSortedByFunction(dataGridNode, this._currentDataGrid.children, sortComparator);
                    this._currentDataGrid.insertChild(dataGridNode, insertionIndex);
                } else
                    this._currentDataGrid.appendChild(dataGridNode);
            }
        }

        var pendingRefreshGridNodes = refreshAllNodes ? this._currentDataGrid.children.slice() : this._pendingRefreshGridNodes[this._currentRecordType];
        if (pendingRefreshGridNodes) {
            delete this._pendingRefreshGridNodes[this._currentRecordType];

            var selectedNode = this._currentDataGrid.selectedNode;

            for (var i = 0; i < pendingRefreshGridNodes.length; ++i) {
                var dataGridNode = pendingRefreshGridNodes[i];
                delete dataGridNode._pendingRefresh;
                dataGridNode.refresh();

                if (!this._currentDataGrid.sortColumnIdentifier)
                    continue;

                if (dataGridNode === selectedNode)
                    this._ignoreSelectionEvent = true;

                // Remove the data grid node so we can find the right sorted location to reinsert it. We need to
                // remove it first so insertionIndexForObjectInListSortedByFunction does not return the current index.
                this._currentDataGrid.removeChild(dataGridNode);

                var insertionIndex = insertionIndexForObjectInListSortedByFunction(dataGridNode, this._currentDataGrid.children, sortComparator);
                this._currentDataGrid.insertChild(dataGridNode, insertionIndex);

                if (dataGridNode === selectedNode) {
                    selectedNode.revealAndSelect();
                    delete this._ignoreSelectionEvent;
                }
            }
        }
    },

    _updateOffscreenRows: function()
    {
        var dataTableBody = this._currentDataGrid.dataTableBody;
        var rows = dataTableBody.children;
        var recordsCount = rows.length;
        if (recordsCount < 2)
            return;  // Filler row only.

        const overflowPadding = 100;
        var visibleTop = this._currentDataGrid.scrollContainer.scrollTop - overflowPadding;
        var visibleBottom = visibleTop + this._currentDataGrid.scrollContainer.offsetHeight + overflowPadding;

        var rowHeight = 0;

        // Filler is at recordsCount - 1.
        for (var i = 0; i < recordsCount - 1; ++i) {
            var row = rows[i];
            if (!rowHeight)
                rowHeight = row.offsetHeight;

            var rowIsVisible = (i * rowHeight) < visibleBottom && ((i + 1) * rowHeight) > visibleTop;
            if (rowIsVisible !== row.rowIsVisible) {
                if (rowIsVisible)
                    row.classList.remove(WebInspector.TimelinesContentView.OffscreenDataGridRowStyleClassName);
                else
                    row.classList.add(WebInspector.TimelinesContentView.OffscreenDataGridRowStyleClassName);
                row.rowIsVisible = rowIsVisible;
            }
        }
    },

    _sortCurrentDataGrid: function()
    {
        console.assert(this._currentDataGrid);
        if (!this._currentDataGrid)
            return;

        var sortColumnIdentifier = this._currentDataGrid.sortColumnIdentifier;
        if (!sortColumnIdentifier)
            return;

        var selectedNode = this._currentDataGrid.selectedNode;
        this._ignoreSelectionEvent = true;

        var nodes = this._currentDataGrid.children.slice();
        nodes.sort(this._sortComparator.bind(this));

        this._currentDataGrid.removeChildren();
        for (var i = 0; i < nodes.length; ++i)
            this._currentDataGrid.appendChild(nodes[i]);

        if (selectedNode)
            selectedNode.revealAndSelect();

        delete this._ignoreSelectionEvent;

        this._updateOffscreenRows();
    },

    _sortComparator: function(node1, node2)
    {
        var sortColumnIdentifier = this._currentDataGrid.sortColumnIdentifier;
        if (!sortColumnIdentifier)
            return 0;

        var sortDirection = this._currentDataGrid.sortOrder === "ascending" ? 1 : -1;

        var value1 = node1.data[sortColumnIdentifier];
        var value2 = node2.data[sortColumnIdentifier];

        if (typeof value1 === "number" && typeof value2 === "number") {
            if (isNaN(value1) && isNaN(value2))
                return 0;
            if (isNaN(value1))
                return sortDirection * -1;
            if (isNaN(value2))
                return sortDirection * 1;
            return (value1 - value2) * sortDirection;
        }

        if (typeof value1 === "string" && typeof value2 === "string")
            return value1.localeCompare(value2) * sortDirection;

        if (value1 instanceof WebInspector.CallFrame || value2 instanceof WebInspector.CallFrame) {
            // Sort by function name if available, then fall back to the source code object.
            value1 = value1 && value1.functionName ? value1.functionName : (value1 && value1.sourceCodeLocation ? value1.sourceCodeLocation.sourceCode : "");
            value2 = value2 && value2.functionName ? value2.functionName : (value2 && value2.sourceCodeLocation ? value2.sourceCodeLocation.sourceCode : "");
        }

        if (value1 instanceof WebInspector.SourceCode || value2 instanceof WebInspector.SourceCode) {
            value1 = value1 ? value1.displayName || "" : "";
            value2 = value2 ? value2.displayName || "" : "";
        }

        // For everything else (mostly booleans).
        return (value1 < value2 ? -1 : (value1 > value2 ? 1 : 0)) * sortDirection;
    },

    _createDataGridNodeForRecord: function(record)
    {
        var baseStartTime = WebInspector.timelineManager.records[0].startTime;

        switch (record.type) {
        case WebInspector.TimelineRecord.Type.Network:
            return new WebInspector.ResourceTimelineDataGridNode(record);
        case WebInspector.TimelineRecord.Type.Layout:
            return new WebInspector.LayoutTimelineDataGridNode(record, baseStartTime);
        case WebInspector.TimelineRecord.Type.Script:
            return new WebInspector.ScriptTimelineDataGridNode(record, baseStartTime);
        }

        console.error("Unknown record type: " + record.type);
        return null;
    },

    _scheduleGridNodeForRefresh: function(event)
    {
        var gridNode = event.target;
        var record = gridNode.record;

        if (gridNode._pendingRefresh)
            return;
        gridNode._pendingRefresh = true;

        if (!this._pendingRefreshGridNodes[record.type])
            this._pendingRefreshGridNodes[record.type] = [];
        this._pendingRefreshGridNodes[record.type].push(gridNode);

        this._updatePendingRecordsSoon();
    },

    _makeColumnScopeBar: function(prefix, dictionary)
    {
        prefix = "timeline-" + prefix + "-data-grid-";

        var keys = Object.keys(dictionary).filter(function(key) {
            return typeof dictionary[key] === "string" || dictionary[key] instanceof String;
        });

        var scopeBarItems = keys.map(function(key) {
            var value = dictionary[key];
            var id = prefix + value;
            var label = dictionary.displayName(value, true);
            var item = new WebInspector.ScopeBarItem(id, label);
            item.value = value;
            return item;
        });

        scopeBarItems.unshift(new WebInspector.ScopeBarItem(prefix + "type-all", WebInspector.UIString("All"), true));
        
        return new WebInspector.ScopeBar(prefix + "scope-bar", scopeBarItems, scopeBarItems[0]);
    },

    _dataGridFiltersDidChange: function(event)
    {
        var dataGrid = event.target;
        if (this._currentDataGrid === dataGrid)
            this._refreshAllRecordsForCurrentDataGrid();

        this._timelineOverview.update();
    },

    _refreshAllRecordsForCurrentDataGrid: function()
    {
        var dataGrid = this._currentDataGrid;
        var recordType = this._currentRecordType;
        var records = this.timelineOverviewRecordsWithType(recordType);

        var nodes = records.map(function(record) {
            return this._createDataGridNodeForRecord(record);
        }, this);
        
        dataGrid.removeChildren();
        nodes.sort(this._sortComparator.bind(this)).forEach(function(node) {
            dataGrid.appendChild(node);
        });

        delete this._pendingRefreshGridNodes[recordType];
        
        this._updateOffscreenRows();
    },

    _filterRecordsWithType: function(records, type)
    {
        var dataGrid = this._dataGridMap[type];
        var identifiers = dataGrid.filterableColumns;

        function filterableValueForRecordAndIdentifier(record, identifier)
        {
            switch (type) {
            case WebInspector.TimelineRecord.Type.Network:
                return record.resource[identifier];
            case WebInspector.TimelineRecord.Type.Layout:
                return record[identifier];
            case WebInspector.TimelineRecord.Type.Script:
                return record[identifier];
            }
        }

        return records.filter(function(record) {
            for (var i = 0; i < identifiers.length; i++) {
                var identifier = identifiers[i];
                var scopeBar = dataGrid.columns[identifier].scopeBar;
                if (!scopeBar || scopeBar.defaultItem.selected)
                    continue;
                var value = filterableValueForRecordAndIdentifier(record, identifier);
                var matchesFilter = scopeBar.selectedItems.some(function(scopeBarItem) {
                    return (scopeBarItem.value === value);
                });
                if (!matchesFilter)
                    return false;
            }
            return true;
        });
    }
};

WebInspector.TimelinesContentView.prototype.__proto__ = WebInspector.ContentView.prototype;
