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

WebInspector.ResourceTimelineDataGridNode = function(resourceTimelineRecord)
{
    WebInspector.DataGridNode.call(this, {});

    this._record = resourceTimelineRecord;

    this._record.addEventListener(WebInspector.TimelineRecord.Event.Updated, this._needsRefresh, this);
    this._record.resource.addEventListener(WebInspector.Resource.Event.URLDidChange, this._needsRefresh, this);
    this._record.resource.addEventListener(WebInspector.Resource.Event.TypeDidChange, this._needsRefresh, this);
    this._record.resource.addEventListener(WebInspector.Resource.Event.LoadingDidFinish, this._needsRefresh, this);
    this._record.resource.addEventListener(WebInspector.Resource.Event.LoadingDidFail, this._needsRefresh, this);
    this._record.resource.addEventListener(WebInspector.Resource.Event.SizeDidChange, this._needsRefresh, this);
    this._record.resource.addEventListener(WebInspector.Resource.Event.TransferSizeDidChange, this._needsRefresh, this);
};

WebInspector.Object.addConstructorFunctions(WebInspector.ResourceTimelineDataGridNode);

WebInspector.ResourceTimelineDataGridNode.IconStyleClassName = "icon";
WebInspector.ResourceTimelineDataGridNode.ErrorStyleClassName = "error";

WebInspector.ResourceTimelineDataGridNode.Event = {
    NeedsRefresh: "resource-timeline-data-grid-node-needs-refresh"
};

WebInspector.ResourceTimelineDataGridNode.prototype = {
    constructor: WebInspector.ResourceTimelineDataGridNode,

    // Public

    get record()
    {
        return this._record;
    },

    get data()
    {
        if (this._cachedData)
            return this._cachedData;

        var resource = this._record.resource;

        var data = {};
        data.name = WebInspector.displayNameForURL(resource.url, resource.urlComponents);
        data.domain = WebInspector.displayNameForHost(resource.urlComponents.host);
        data.type = resource.type;
        data.statusCode = resource.statusCode;
        data.cached = resource.cached;
        data.size = resource.size;
        data.transferSize = resource.transferSize;
        data.duration = resource.receiveDuration;
        data.latency = resource.latency;
        data.timeline = resource.responseReceivedTimestamp || resource.requestSentTimestamp;

        this._cachedData = data;
        return data;
    },

    createCellContent: function(columnIdentifier, cell)
    {
        var resource = this._record.resource;

        if (resource.failed || resource.canceled || resource.statusCode >= 400)
            cell.classList.add(WebInspector.ResourceTimelineDataGridNode.ErrorStyleClassName);

        const emptyValuePlaceholderString = "\u2014";
        var value = this.data[columnIdentifier];

        switch (columnIdentifier) {
        case "name":
            cell.classList.add(WebInspector.ResourceTreeElement.ResourceIconStyleClassName);
            cell.classList.add(resource.type);

            var fragment = document.createDocumentFragment();

            var gotToButton = WebInspector.createGoToArrowButton();
            gotToButton.addEventListener("click", this._goToResource.bind(this));
            fragment.appendChild(gotToButton);

            var icon = document.createElement("div");
            icon.className = WebInspector.ResourceTimelineDataGridNode.IconStyleClassName;
            fragment.appendChild(icon);

            var text = document.createTextNode(value);
            fragment.appendChild(text);

            cell.title = resource.url;

            return fragment;

        case "type":
            return WebInspector.Resource.Type.displayName(value);

        case "statusCode":
            cell.title = resource.statusText || "";
            return value || emptyValuePlaceholderString;

        case "cached":
            return value ? WebInspector.UIString("Yes") : WebInspector.UIString("No");

        case "domain":
            return value || emptyValuePlaceholderString;

        case "size":
        case "transferSize":
            return isNaN(value) ? emptyValuePlaceholderString : Number.bytesToString(value);

        case "latency":
        case "duration":
            return isNaN(value) ? emptyValuePlaceholderString : Number.secondsToString(value);

        case "timeline":
            return this._createGraphElement();
        }

        return WebInspector.DataGridNode.prototype.createCellContent.call(this, columnIdentifier);
    },

    refresh: function()
    {
        delete this._cachedData;

        WebInspector.DataGridNode.prototype.refresh.call(this);
    },

    updateLayout: function()
    {
        this._refreshLabelPositions();
    },

    select: function(supressSelectedEvent)
    {
        if (this.element.classList.contains(WebInspector.TimelinesContentView.OffscreenDataGridRowStyleClassName))
            this.element.classList.remove(WebInspector.TimelinesContentView.OffscreenDataGridRowStyleClassName);

        WebInspector.DataGridNode.prototype.select.call(this, supressSelectedEvent);

        this._refreshLabelPositions();
    },

    // Private

    _needsRefresh: function()
    {
        this.dispatchEventToListeners(WebInspector.ResourceTimelineDataGridNode.Event.NeedsRefresh);
    },

    _goToResource: function(event)
    {
        WebInspector.resourceSidebarPanel.showSourceCode(this._record.resource);
    },

    _createGraphElement: function(cell)
    {
        if (!this._graphElement) {
            this._graphElement = document.createElement("div");
            this._graphElement.className = "network-graph-side";

            this._barAreaElement = document.createElement("div");
            this._barAreaElement.className = "network-graph-bar-area";
            this._barAreaElement.resource = this._record.resource;
            this._graphElement.appendChild(this._barAreaElement);

            this._barLeftElement = document.createElement("div");
            this._barLeftElement.className = "network-graph-bar waiting";
            this._barAreaElement.appendChild(this._barLeftElement);

            this._barRightElement = document.createElement("div");
            this._barRightElement.className = "network-graph-bar";
            this._barAreaElement.appendChild(this._barRightElement);

            this._labelLeftElement = document.createElement("div");
            this._labelLeftElement.className = "network-graph-label waiting";
            this._barAreaElement.appendChild(this._labelLeftElement);

            this._labelRightElement = document.createElement("div");
            this._labelRightElement.className = "network-graph-label";
            this._barAreaElement.appendChild(this._labelRightElement);

            this._graphElement.addEventListener("mouseover", this._refreshLabelPositions.bind(this));
        }

        this._refreshGraph();

        return this._graphElement;
    },

    _refreshGraph: function(c)
    {
        var resource = this._record.resource;
        if (resource.cached)
            this._graphElement.classList.add("resource-cached");

        var calculator = this.dataGrid.currentCalculator;
        var percentages = calculator.computeBarGraphPercentages(resource);
        this._percentages = percentages;

        if (!this._graphElement.classList.contains("network-" + resource.type)) {
            this._graphElement.removeMatchingStyleClasses("network-resource-type-\\w+");
            this._graphElement.classList.add("network-" + resource.type);
        }

        this._barLeftElement.style.setProperty("left", percentages.start + "%");
        this._barRightElement.style.setProperty("right", (100 - percentages.end) + "%");

        this._barLeftElement.style.setProperty("right", (100 - percentages.end) + "%");
        this._barRightElement.style.setProperty("left", percentages.middle + "%");

        var labels = calculator.computeBarGraphLabels(resource);
        this._labelLeftElement.textContent = labels.left;
        this._labelRightElement.textContent = labels.right;

        var tooltip = (labels.tooltip || "");
        this._barLeftElement.title = tooltip;
        this._labelLeftElement.title = tooltip;
        this._labelRightElement.title = tooltip;
        this._barRightElement.title = tooltip;
    },

    _refreshLabelPositions: function()
    {
        if (!this._percentages)
            return;

        this._labelLeftElement.style.removeProperty("left");
        this._labelLeftElement.style.removeProperty("right");
        this._labelLeftElement.classList.remove("before");
        this._labelLeftElement.classList.remove("hidden");

        this._labelRightElement.style.removeProperty("left");
        this._labelRightElement.style.removeProperty("right");
        this._labelRightElement.classList.remove("after");
        this._labelRightElement.classList.remove("hidden");

        const labelPadding = 10;
        const leftLabelWidth = this._labelLeftElement.offsetWidth + labelPadding;
        const rightLabelWidth = this._labelRightElement.offsetWidth + labelPadding;

        const barRightElementOffsetWidth = this._barRightElement.offsetWidth;
        const barLeftElementOffsetWidth = this._barLeftElement.offsetWidth;

        const leftBarWidth = barLeftElementOffsetWidth - barRightElementOffsetWidth;
        const rightBarWidth = barRightElementOffsetWidth;

        const leftCallout = (leftLabelWidth > leftBarWidth);
        const rightCallout = (rightLabelWidth > rightBarWidth);

        // Hide the left or right callout if there is not enough space.
        const graphElementOffsetWidth = this._graphElement.offsetWidth;
        if (leftCallout && (graphElementOffsetWidth * (this._percentages.start / 100)) < leftLabelWidth)
            var leftHidden = true;
        if (rightCallout && (graphElementOffsetWidth * ((100 - this._percentages.end) / 100)) < rightLabelWidth)
            var rightHidden = true;

        // The left/right label data are the same, so a before/after label can be replaced by a single on-bar label.
        if (barLeftElementOffsetWidth == barRightElementOffsetWidth) {
            if (leftCallout && !rightCallout)
                leftHidden = true;
            else if (rightCallout && !leftCallout)
                rightHidden = true;
        }

        if (leftCallout) {
            if (leftHidden)
                this._labelLeftElement.classList.add("hidden");
            this._labelLeftElement.style.setProperty("right", (100 - this._percentages.start) + "%");
            this._labelLeftElement.classList.add("before");
        } else {
            this._labelLeftElement.style.setProperty("left", this._percentages.start + "%");
            this._labelLeftElement.style.setProperty("right", (100 - this._percentages.middle) + "%");
        }

        if (rightCallout) {
            if (rightHidden)
                this._labelRightElement.classList.add("hidden");
            this._labelRightElement.style.setProperty("left", this._percentages.end + "%");
            this._labelRightElement.classList.add("after");
        } else {
            this._labelRightElement.style.setProperty("left", this._percentages.middle + "%");
            this._labelRightElement.style.setProperty("right", (100 - this._percentages.end) + "%");
        }
    }
}

WebInspector.ResourceTimelineDataGridNode.prototype.__proto__ = WebInspector.DataGridNode.prototype;
