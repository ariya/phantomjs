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

WebInspector.TimelineDataGrid = function(columns, editCallback, deleteCallback)
{
    WebInspector.DataGrid.call(this, columns, editCallback, deleteCallback);

    this.filterableColumns = [];

    // Check if any of the cells can be filtered.
    for (var identifier in columns) {
        var scopeBar = columns[identifier].scopeBar;
        if (!scopeBar)
            continue;
        this.filterableColumns.push(identifier);
        scopeBar.columnIdenfifier = identifier;
        scopeBar.addEventListener(WebInspector.ScopeBar.Event.SelectionChanged, this._scopeBarSelectedItemsDidChange, this);
    }

    if (this.filterableColumns.length > 1) {
        console.error("Creating a TimelineDataGrid with more than one filterable column is not yet supported.");
        return;
    }

    var items = [new WebInspector.FlexibleSpaceNavigationItem, this.columns[this.filterableColumns[0]].scopeBar];
    this._navigationBar = new WebInspector.NavigationBar(null, items);
    var container = this.element.appendChild(document.createElement("div"));
    container.className = "navigation-bar-container";
    container.appendChild(this._navigationBar.element);

    this.addEventListener(WebInspector.DataGrid.Event.SelectedNodeChanged, this._dataGridSelectedNodeChanged, this);
    window.addEventListener("resize", this._windowResized.bind(this));
}

WebInspector.TimelineDataGrid.DelayedPopoverShowTimeout = 250;
WebInspector.TimelineDataGrid.DelayedPopoverHideContentClearTimeout = 500;

WebInspector.TimelineDataGrid.Event = {
    FiltersDidChange: "timelinedatagrid-filters-did-change"
};

WebInspector.TimelineDataGrid.prototype = {
    constructor: WebInspector.TimelineDataGrid,

    // Public

    get currentCalculator()
    {
        // Implemented by subclasses if they have a graph.
        return null;
    },

    updateCalculatorBoundariesWithRecord: function(record)
    {
        // Implemented by subclasses if they have a graph.
    },

    updateCalculatorBoundariesWithDataGridNode: function(node)
    {
        // Implemented by subclasses if they have a graph.
    },

    updateCalculatorBoundariesWithEventMarker: function(eventMarker)
    {
        // Implemented by subclasses if they have a graph.
    },

    reset: function()
    {
        // May be overridden by subclasses. If so, they should call the superclass.

        this._hidePopover();
    },

    shown: function()
    {
        // Implemented by subclasses.
    },

    hidden: function()
    {
        // May be overridden by subclasses. If so, they should call the superclass.

        this._hidePopover();
    },

    update: function()
    {
        // Implemented by subclasses.
    },

    addTimelineEventMarker: function(eventMarker)
    {
        // Implemented by subclasses.
    },

    callFramePopoverAnchorElement: function()
    {
        // Implemented by subclasses.
        return null;
    },

    updateLayout: function()
    {
        WebInspector.DataGrid.prototype.updateLayout.call(this);

        this._navigationBar.updateLayout();
    },

    // Private

    _scopeBarSelectedItemsDidChange: function(event)
    {
        var columnIdentifier = event.target.columnIdenfifier;
        this.dispatchEventToListeners(WebInspector.TimelineDataGrid.Event.FiltersDidChange, {columnIdentifier: columnIdentifier});
    },

    _dataGridSelectedNodeChanged: function(event)
    {
        if (!this.selectedNode) {
            this._hidePopover();
            return;
        }

        var record = this.selectedNode.record;
        if (!record || !record.callFrames || !record.callFrames.length) {
            this._hidePopover();
            return;
        }

        this._showPopoverForSelectedNodeSoon();
    },

    _windowResized: function(event)
    {
        if (this._popover && this._popover.visible)
            this._updatePopoverForSelectedNode(false);
    },

    _showPopoverForSelectedNodeSoon: function()
    {
        if (this._showPopoverTimeout)
            return;

        function delayedWork()
        {
            if (!this._popover)
                this._popover = new WebInspector.Popover;

            this._updatePopoverForSelectedNode(true);
        }

        this._showPopoverTimeout = setTimeout(delayedWork.bind(this), WebInspector.TimelineDataGrid.DelayedPopoverShowTimeout);
    },

    _hidePopover: function()
    {
        if (this._showPopoverTimeout) {
            clearTimeout(this._showPopoverTimeout);
            delete this._showPopoverTimeout;
        }

        if (this._popover)
            this._popover.dismiss();

        function delayedWork()
        {
            if (this._popoverCallStackTreeOutline)
                this._popoverCallStackTreeOutline.removeChildren();
        }

        if (this._hidePopoverContentClearTimeout)
            clearTimeout(this._hidePopoverContentClearTimeout);
        this._hidePopoverContentClearTimeout = setTimeout(delayedWork.bind(this), WebInspector.TimelineDataGrid.DelayedPopoverHideContentClearTimeout);
    },

    _updatePopoverForSelectedNode: function(updateContent)
    {
        if (!this._popover || !this.selectedNode)
            return;

        var targetPopoverElement = this.callFramePopoverAnchorElement();
        console.assert(targetPopoverElement, "TimelineDataGrid subclass should always return a valid element from callFramePopoverAnchorElement.");
        if (!targetPopoverElement)
            return;

        var targetFrame = WebInspector.Rect.rectFromClientRect(targetPopoverElement.getBoundingClientRect());

        // The element might be hidden if it does not have a width and height.
        if (!targetFrame.size.width && !targetFrame.size.height)
            return;

        if (this._hidePopoverContentClearTimeout) {
            clearTimeout(this._hidePopoverContentClearTimeout);
            delete this._hidePopoverContentClearTimeout;
        }

        const padding = 2;
        targetFrame.origin.x -= padding;
        targetFrame.origin.y -= padding;
        targetFrame.size.width += padding * 2;
        targetFrame.size.height += padding * 2;

        if (updateContent)
            this._popover.content = this._createPopoverContent();

        this._popover.present(targetFrame, [WebInspector.RectEdge.MAX_Y, WebInspector.RectEdge.MIN_Y, WebInspector.RectEdge.MAX_X]);
    },

    _createPopoverContent: function()
    {
        if (!this._popoverCallStackTreeOutline) {
            var contentElement = document.createElement("ol");
            contentElement.classList.add("timeline-data-grid-tree-outline");
            this._popoverCallStackTreeOutline = new TreeOutline(contentElement);
            this._popoverCallStackTreeOutline.onselect = this._popoverCallStackTreeElementSelected.bind(this);
        } else
            this._popoverCallStackTreeOutline.removeChildren();

        var callFrames = this.selectedNode.record.callFrames;
        for (var i = 0 ; i < callFrames.length; ++i) {
            var callFrameTreeElement = new WebInspector.CallFrameTreeElement(callFrames[i]);
            this._popoverCallStackTreeOutline.appendChild(callFrameTreeElement);
        }

        var content = document.createElement("div");
        content.className = "timeline-data-grid-popover";
        content.appendChild(this._popoverCallStackTreeOutline.element);
        return content;
    },

    _popoverCallStackTreeElementSelected: function(treeElement, selectedByUser)
    {
        this._popover.dismiss();

        console.assert(treeElement instanceof WebInspector.CallFrameTreeElement, "TreeElements in TimelineDataGrid popover should always be CallFrameTreeElements");
        var callFrame = treeElement.callFrame;
        if (!callFrame.sourceCodeLocation)
            return;

        WebInspector.resourceSidebarPanel.showSourceCodeLocation(callFrame.sourceCodeLocation);
    }
}

WebInspector.TimelineDataGrid.prototype.__proto__ = WebInspector.DataGrid.prototype;
