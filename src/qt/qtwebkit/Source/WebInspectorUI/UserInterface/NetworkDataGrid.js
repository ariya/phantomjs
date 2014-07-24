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

WebInspector.NetworkDataGrid = function(columns)
{
    WebInspector.TimelineDataGrid.call(this, columns);
    this.element.classList.add(WebInspector.NetworkDataGrid.NetworkDataGridStyleClassName);

    this.addEventListener(WebInspector.DataGrid.Event.SortChanged, this._dataGridSortChanged, this);
    this.addEventListener(WebInspector.DataGrid.Event.DidLayout, this._dataGridDidLayout, this);

    this._timelineDecorations = new WebInspector.TimelineDecorations;
    this._timelineDecorations.element.classList.add(WebInspector.NetworkDataGrid.NetworkTimelineDecorationsStyleClassName);
    this._timelineDecorations.hideHeaderDividers();
    this.element.appendChild(this._timelineDecorations.element);

    // Network calculators all deal with WebInspector.Resources.
    this._timeCalculator = new WebInspector.NetworkTransferTimeCalculator;

    this._detailsColumnGroupCollapsedSetting = new WebInspector.Setting("network-data-grid-" + WebInspector.NetworkDataGrid.DetailsColumnGroup + "-collapsed", false);
    if (this._detailsColumnGroupCollapsedSetting.value)
        this.collapseColumnGroup(WebInspector.NetworkDataGrid.DetailsColumnGroup);
}

WebInspector.NetworkDataGrid.NetworkDataGridStyleClassName = "network-datagrid";
WebInspector.NetworkDataGrid.NetworkTimelineDecorationsStyleClassName = "network-timeline-decorations";
WebInspector.NetworkDataGrid.NetworkTimelineCollapsedColumnsStyleClassName = "collapsed-details";

WebInspector.NetworkDataGrid.DetailsColumnGroup = "details";

WebInspector.NetworkDataGrid.prototype = {
    constructor: WebInspector.NetworkDataGrid,

    // Public

    get currentCalculator()
    {
        return this._timeCalculator;
    },

    updateCalculatorBoundariesWithRecord: function(record)
    {
        return this.currentCalculator.updateBoundaries(record.resource);
    },

    updateCalculatorBoundariesWithDataGridNode: function(node)
    {
        return this.currentCalculator.updateBoundaries(node.record.resource);
    },

    updateCalculatorBoundariesWithEventMarker: function(eventMarker)
    {
        return this.currentCalculator.updateBoundariesForEventMarker(eventMarker);
    },

    reset: function()
    {
        WebInspector.TimelineDataGrid.prototype.reset.call(this);

        this.currentCalculator.reset();

        this._timelineDecorations.clear();
        this._updateDecorations();
    },

    update: function()
    {
        this._updateDecorations();
    },

    // Protected

    willToggleColumnGroup: function(columnGroup, willCollapse)
    {
        console.assert(columnGroup === "details");

        if (willCollapse) {
            console.assert(!this._savedExpandedNetworkColumnWidths);
            this._savedExpandedNetworkColumnWidths = this.columnWidthsMap();
        }
    },

    didToggleColumnGroup: function(columnGroup, didCollapse)
    {
        console.assert(columnGroup === "details");
        console.assert(this._savedExpandedNetworkColumnWidths);

        if (didCollapse) {
            var newWidths = {};
            newWidths.name = this._savedExpandedNetworkColumnWidths.name;
            newWidths.timeline = (100 - newWidths.name);

            this.element.classList.add(WebInspector.NetworkDataGrid.NetworkTimelineCollapsedColumnsStyleClassName);
            this._timelineDecorations.showHeaderDividers();
        } else {
            var newWidths = this._savedExpandedNetworkColumnWidths;
            delete this._savedExpandedNetworkColumnWidths;

            this.element.classList.remove(WebInspector.NetworkDataGrid.NetworkTimelineCollapsedColumnsStyleClassName);
            this._timelineDecorations.hideHeaderDividers();
        }

        this.applyColumnWidthsMap(newWidths);

        this._detailsColumnGroupCollapsedSetting.value = didCollapse;
    },

    addTimelineEventMarker: function(eventMarker)
    {
        this._timelineDecorations.addTimelineEventMarker(eventMarker);
    },

    // Private

    _dataGridSortChanged: function()
    {
        this._updateDecorations(true);
    },

    _dataGridDidLayout: function()
    {
        if (this.selectedNode)
            this.selectedNode.updateLayout();

        this._updateDecorations();
    },

    _localizedTimelineHeaderWidth: function()
    {
        if (WebInspector.NetworkDataGrid._localizedTimelineWidth !== undefined)
            return WebInspector.NetworkDataGrid._localizedTimelineWidth;

        var div = document.createElement("div");
        div.style.position = "absolute";
        div.style.top = 0;
        div.style.left = 0;
        div.style.fontSize = "11px";
        div.style.fontFamily = "\"Lucida Grande\", sans-serif";
        div.style.visibility = "hidden";
        div.textContent = WebInspector.UIString("Timeline");

        document.body.appendChild(div);
        WebInspector.NetworkDataGrid._localizedTimelineWidth = div.offsetWidth + 20 /* padding */;
        document.body.removeChild(div);

        return WebInspector.NetworkDataGrid._localizedTimelineWidth;
    },

    _updateDecorations: function(force)
    {
        this._updateDecorationsPosition();
        this._updateHeaderTimes(force);
        this._updateEventMarkers();
    },

    _updateDecorationsPosition: function()
    {
        var lastVisibleResizer = null;
        for (var i = this.resizers.length - 1; i >= 0; --i) {
            if (this.resizers[i].style.display !== "none") {
                lastVisibleResizer = this.resizers[i];
                break;
            }
        }

        if (!lastVisibleResizer)
            return;

        this._timelineDecorations.element.style.left = lastVisibleResizer.style.left;
    },

    _updateHeaderTimes: function(force)
    {
        var calculator = this.currentCalculator;
        var boundarySpan = calculator.boundarySpan;
        var isTimelineColumnSorted = this.isColumnSortColumn("timeline");

        if (isTimelineColumnSorted)
            this._timelineDecorations.element.classList.add("sort-active");
        else
            this._timelineDecorations.element.classList.remove("sort-active");

        var leftPadding = this._localizedTimelineHeaderWidth();
        var rightPadding = isTimelineColumnSorted ? 50 /* Collapse Button and Sort Indicator */ : 27 /* Collapse Button */;
        this._timelineDecorations.updateHeaderTimes(boundarySpan, leftPadding, rightPadding, force);
    },

    _updateEventMarkers: function()
    {
        var calculator = this.currentCalculator;
        this._timelineDecorations.updateEventMarkers(calculator.minimumBoundary, calculator.maximumBoundary);
    },
}

WebInspector.NetworkDataGrid.prototype.__proto__ = WebInspector.TimelineDataGrid.prototype;
