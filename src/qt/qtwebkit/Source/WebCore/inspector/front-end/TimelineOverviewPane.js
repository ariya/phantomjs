/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
 * @param {WebInspector.TimelineModel} model
 */
WebInspector.TimelineOverviewPane = function(model)
{
    WebInspector.View.call(this);
    this.element.id = "timeline-overview-panel";

    this._windowStartTime = 0;
    this._windowEndTime = Infinity;
    this._eventDividers = [];

    this._model = model;

    this._topPaneSidebarElement = document.createElement("div");
    this._topPaneSidebarElement.id = "timeline-overview-sidebar";

    var overviewTreeElement = document.createElement("ol");
    overviewTreeElement.className = "sidebar-tree";
    this._topPaneSidebarElement.appendChild(overviewTreeElement);
    this.element.appendChild(this._topPaneSidebarElement);

    var topPaneSidebarTree = new TreeOutline(overviewTreeElement);

    this._overviewItems = {};
    this._overviewItems[WebInspector.TimelineOverviewPane.Mode.Events] = new WebInspector.SidebarTreeElement("timeline-overview-sidebar-events",
        WebInspector.UIString("Events"));
    if (Capabilities.timelineSupportsFrameInstrumentation) {
        this._overviewItems[WebInspector.TimelineOverviewPane.Mode.Frames] = new WebInspector.SidebarTreeElement("timeline-overview-sidebar-frames",
            WebInspector.UIString("Frames"));
    }
    this._overviewItems[WebInspector.TimelineOverviewPane.Mode.Memory] = new WebInspector.SidebarTreeElement("timeline-overview-sidebar-memory",
        WebInspector.UIString("Memory"));

    for (var mode in this._overviewItems) {
        var item = this._overviewItems[mode];
        item.onselect = this.setMode.bind(this, mode);
        topPaneSidebarTree.appendChild(item);
    }
    
    this._overviewGrid = new WebInspector.OverviewGrid("timeline");
    this.element.appendChild(this._overviewGrid.element);

    var separatorElement = document.createElement("div");
    separatorElement.id = "timeline-overview-separator";
    this.element.appendChild(separatorElement);

    this._innerSetMode(WebInspector.TimelineOverviewPane.Mode.Events);

    var categories = WebInspector.TimelinePresentationModel.categories();
    for (var category in categories)
        categories[category].addEventListener(WebInspector.TimelineCategory.Events.VisibilityChanged, this._onCategoryVisibilityChanged, this);

    this._overviewCalculator = new WebInspector.TimelineOverviewCalculator();

    model.addEventListener(WebInspector.TimelineModel.Events.RecordAdded, this._onRecordAdded, this);
    model.addEventListener(WebInspector.TimelineModel.Events.RecordsCleared, this._reset, this);
    this._overviewGrid.addEventListener(WebInspector.OverviewGrid.Events.WindowChanged, this._onWindowChanged, this);
}

WebInspector.TimelineOverviewPane.Mode = {
    Events: "Events",
    Frames: "Frames",
    Memory: "Memory"
};

WebInspector.TimelineOverviewPane.Events = {
    ModeChanged: "ModeChanged",
    WindowChanged: "WindowChanged"
};

WebInspector.TimelineOverviewPane.prototype = {
    wasShown: function()
    {
        this._update();
    },

    onResize: function()
    {
        this._update();
    },

    setMode: function(newMode)
    {
        if (this._currentMode === newMode)
            return;
        this._innerSetMode(newMode);
        this.dispatchEventToListeners(WebInspector.TimelineOverviewPane.Events.ModeChanged, this._currentMode);
        this._update();
    },

    _innerSetMode: function(newMode)
    {
        if (this._overviewControl)
            this._overviewControl.detach();

        this._currentMode = newMode;
        this._overviewControl = this._createOverviewControl();
        this._overviewControl.show(this._overviewGrid.element);
        this._overviewItems[this._currentMode].revealAndSelect(false);
    },

    /**
     * @return {WebInspector.TimelineOverviewBase|null}
     */
    _createOverviewControl: function()
    {
        switch (this._currentMode) {
        case WebInspector.TimelineOverviewPane.Mode.Events:
            return new WebInspector.TimelineEventOverview(this._model);
        case WebInspector.TimelineOverviewPane.Mode.Frames:
            return new WebInspector.TimelineFrameOverview(this._model);
        case WebInspector.TimelineOverviewPane.Mode.Memory:
            return new WebInspector.TimelineMemoryOverview(this._model);
        }
        throw new Error("Invalid overview mode: " + this._currentMode);
    },

    _onCategoryVisibilityChanged: function(event)
    {
        this._overviewControl.categoryVisibilityChanged();
    },

    _update: function()
    {
        delete this._refreshTimeout;

        this._updateWindow();
        this._overviewCalculator.setWindow(this._model.minimumRecordTime(), this._model.maximumRecordTime());
        this._overviewCalculator.setDisplayWindow(0, this._overviewGrid.clientWidth());

        this._overviewControl.update();
        this._overviewGrid.updateDividers(this._overviewCalculator);
        this._updateEventDividers();
    },

    _updateEventDividers: function()
    {
        var records = this._eventDividers;
        this._overviewGrid.removeEventDividers();
        var dividers = [];
        for (var i = 0; i < records.length; ++i) {
            var record = records[i];
            var positions = this._overviewCalculator.computeBarGraphPercentages(record);
            var dividerPosition = Math.round(positions.start * 10);
            if (dividers[dividerPosition])
                continue;
            var divider = WebInspector.TimelinePresentationModel.createEventDivider(record.type);
            divider.style.left = positions.start + "%";
            dividers[dividerPosition] = divider;
        }
        this._overviewGrid.addEventDividers(dividers);
    },

    /**
     * @param {number} width
     */
    sidebarResized: function(width)
    {
        this._overviewGrid.element.style.left = width + "px";
        this._topPaneSidebarElement.style.width = width + "px";
        this._update();
    },

    /**
     * @param {WebInspector.TimelineFrame} frame
     */
    addFrame: function(frame)
    {
        this._overviewControl.addFrame(frame);
        this._scheduleRefresh();
    },

    /**
     * @param {WebInspector.TimelineFrame} frame
     */
    zoomToFrame: function(frame)
    {
        var frameOverview = /** @type WebInspector.TimelineFrameOverview */ (this._overviewControl);
        var window = frameOverview.framePosition(frame);
        if (!window)
            return;

        this._overviewGrid.setWindowPosition(window.start, window.end);
    },

    _onRecordAdded: function(event)
    {
        var record = event.data;
        var eventDividers = this._eventDividers;
        function addEventDividers(record)
        {
            if (WebInspector.TimelinePresentationModel.isEventDivider(record))
                eventDividers.push(record);
        }
        WebInspector.TimelinePresentationModel.forAllRecords([record], addEventDividers);
        this._scheduleRefresh();
    },

    _reset: function()
    {
        this._windowStartTime = 0;
        this._windowEndTime = Infinity;
        this._overviewCalculator.reset();
        this._overviewGrid.reset();
        this._eventDividers = [];
        this._overviewGrid.updateDividers(this._overviewCalculator);
        this._overviewControl.reset();
        this._update();
    },

    windowStartTime: function()
    {
        return this._windowStartTime || this._model.minimumRecordTime();
    },

    windowEndTime: function()
    {
        return this._windowEndTime < Infinity ? this._windowEndTime : this._model.maximumRecordTime();
    },

    windowLeft: function()
    {
        return this._overviewGrid.windowLeft();
    },

    windowRight: function()
    {
        return this._overviewGrid.windowRight();
    },

    _onWindowChanged: function()
    {
        if (this._ignoreWindowChangedEvent)
            return;
        var times = this._overviewControl.windowTimes(this.windowLeft(), this.windowRight());
        this._windowStartTime = times.startTime;
        this._windowEndTime = times.endTime;
        this.dispatchEventToListeners(WebInspector.TimelineOverviewPane.Events.WindowChanged);
    },

    /**
     * @param {Number} left
     * @param {Number} right
     */
    setWindowTimes: function(left, right)
    {
        this._windowStartTime = left;
        this._windowEndTime = right;
        this._updateWindow();
    },

    _updateWindow: function()
    {
        var offset = this._model.minimumRecordTime();
        var timeSpan = this._model.maximumRecordTime() - offset;
        var left = this._windowStartTime ? (this._windowStartTime - offset) / timeSpan : 0;
        var right = this._windowEndTime < Infinity ? (this._windowEndTime - offset) / timeSpan : 1;
        this._ignoreWindowChangedEvent = true;
        this._overviewGrid.setWindow(left, right);
        this._ignoreWindowChangedEvent = false;
    },

    _scheduleRefresh: function()
    {
        if (this._refreshTimeout)
            return;
        if (!this.isShowing())
            return;
        this._refreshTimeout = setTimeout(this._update.bind(this), 300);
    },

    __proto__: WebInspector.View.prototype
}

/**
 * @constructor
 * @implements {WebInspector.TimelineGrid.Calculator}
 */
WebInspector.TimelineOverviewCalculator = function()
{
}

WebInspector.TimelineOverviewCalculator.prototype = {
    /**
     * @param {number} time
     */
    computePosition: function(time)
    {
        return (time - this._minimumBoundary) / this.boundarySpan() * this._workingArea + this.paddingLeft;
    },

    computeBarGraphPercentages: function(record)
    {
        var start = (WebInspector.TimelineModel.startTimeInSeconds(record) - this._minimumBoundary) / this.boundarySpan() * 100;
        var end = (WebInspector.TimelineModel.endTimeInSeconds(record) - this._minimumBoundary) / this.boundarySpan() * 100;
        return {start: start, end: end};
    },

    /**
     * @param {number=} minimum
     * @param {number=} maximum
     */
    setWindow: function(minimum, maximum)
    {
        this._minimumBoundary = minimum >= 0 ? minimum : undefined;
        this._maximumBoundary = maximum >= 0 ? maximum : undefined;
    },

    /**
     * @param {number} paddingLeft
     * @param {number} clientWidth
     */
    setDisplayWindow: function(paddingLeft, clientWidth)
    {
        this._workingArea = clientWidth - paddingLeft;
        this.paddingLeft = paddingLeft;
    },

    reset: function()
    {
        this.setWindow();
    },

    formatTime: function(value)
    {
        return Number.secondsToString(value);
    },

    maximumBoundary: function()
    {
        return this._maximumBoundary;
    },

    minimumBoundary: function()
    {
        return this._minimumBoundary;
    },

    zeroTime: function()
    {
        return this._minimumBoundary;
    },

    boundarySpan: function()
    {
        return this._maximumBoundary - this._minimumBoundary;
    }
}

/**
 * @constructor
 * @extends {WebInspector.View}
 * @param {WebInspector.TimelineModel} model
 */
WebInspector.TimelineOverviewBase = function(model)
{
    WebInspector.View.call(this);
    this._model = model;
    this._canvas = this.element.createChild("canvas", "fill");
}

WebInspector.TimelineOverviewBase.prototype = {
    update: function() { },
    reset: function() { },

    categoryVisibilityChanged: function() { },

    /**
     * @param {WebInspector.TimelineFrame} frame
     */
    addFrame: function(frame) { },

    /**
     * @param {number} windowLeft
     * @param {number} windowRight
     */
    windowTimes: function(windowLeft, windowRight)
    {
        var absoluteMin = this._model.minimumRecordTime();
        var absoluteMax = this._model.maximumRecordTime();
        return {
            startTime: absoluteMin + (absoluteMax - absoluteMin) * windowLeft,
            endTime: absoluteMin + (absoluteMax - absoluteMin) * windowRight
        };
    },

    __proto__: WebInspector.View.prototype
}

/**
 * @constructor
 * @extends {WebInspector.TimelineOverviewBase}
 * @param {WebInspector.TimelineModel} model
 */
WebInspector.TimelineMemoryOverview = function(model)
{
    WebInspector.TimelineOverviewBase.call(this, model);
    this.element.id = "timeline-overview-memory";
    this.element.classList.add("fill");

    this._maxHeapSizeLabel = this.element.createChild("div", "max memory-graph-label");
    this._minHeapSizeLabel = this.element.createChild("div", "min memory-graph-label");
}

WebInspector.TimelineMemoryOverview.prototype = {
    reset: function()
    {
        this._maxHeapSizeLabel.textContent = "";
        this._minHeapSizeLabel.textContent = "";
    },

    update: function()
    {
        const yPadding = 5;
        this._canvas.width = this.element.clientWidth;
        this._canvas.height = this.element.clientHeight - yPadding;

        var records = this._model.records;
        if (!records.length)
            return;

        const lowerOffset = 3;
        var maxUsedHeapSize = 0;
        var minUsedHeapSize = 100000000000;
        var minTime = this._model.minimumRecordTime();
        var maxTime = this._model.maximumRecordTime();;
        WebInspector.TimelinePresentationModel.forAllRecords(records, function(r) {
            maxUsedHeapSize = Math.max(maxUsedHeapSize, r.usedHeapSize || maxUsedHeapSize);
            minUsedHeapSize = Math.min(minUsedHeapSize, r.usedHeapSize || minUsedHeapSize);
        });
        minUsedHeapSize = Math.min(minUsedHeapSize, maxUsedHeapSize);

        var width = this._canvas.width;
        var height = this._canvas.height - lowerOffset;
        var xFactor = width / (maxTime - minTime);
        var yFactor = height / (maxUsedHeapSize - minUsedHeapSize);

        var histogram = new Array(width);
        WebInspector.TimelinePresentationModel.forAllRecords(records, function(r) {
            if (!r.usedHeapSize)
                return;
            var x = Math.round((WebInspector.TimelineModel.endTimeInSeconds(r) - minTime) * xFactor);
            var y = Math.round((r.usedHeapSize - minUsedHeapSize) * yFactor);
            histogram[x] = Math.max(histogram[x] || 0, y);
        });

        var ctx = this._canvas.getContext("2d");
        this._clear(ctx);

        // +1 so that the border always fit into the canvas area.
        height = height + 1;

        ctx.beginPath();
        var initialY = 0;
        for (var k = 0; k < histogram.length; k++) {
            if (histogram[k]) {
                initialY = histogram[k];
                break;
            }
        }
        ctx.moveTo(0, height - initialY);

        for (var x = 0; x < histogram.length; x++) {
             if (!histogram[x])
                 continue;
             ctx.lineTo(x, height - histogram[x]);
        }

        ctx.lineWidth = 0.5;
        ctx.strokeStyle = "rgba(20,0,0,0.8)";
        ctx.stroke();

        ctx.fillStyle = "rgba(214,225,254, 0.8);";
        ctx.lineTo(width, 60);
        ctx.lineTo(0, 60);
        ctx.lineTo(0, height - initialY);
        ctx.fill();
        ctx.closePath();

        this._maxHeapSizeLabel.textContent = Number.bytesToString(maxUsedHeapSize);
        this._minHeapSizeLabel.textContent = Number.bytesToString(minUsedHeapSize);
    },

    _clear: function(ctx)
    {
        ctx.fillStyle = "rgba(255,255,255,0.8)";
        ctx.fillRect(0, 0, this._canvas.width, this._canvas.height);
    },

    __proto__: WebInspector.TimelineOverviewBase.prototype
}

/**
 * @constructor
 * @extends {WebInspector.TimelineOverviewBase}
 * @param {WebInspector.TimelineModel} model
 */
WebInspector.TimelineEventOverview = function(model)
{
    WebInspector.TimelineOverviewBase.call(this, model);

    this.element.id = "timeline-overview-events";
    this._context = this._canvas.getContext("2d");

    this._fillStyles = {};
    var categories = WebInspector.TimelinePresentationModel.categories();
    for (var category in categories)
        this._fillStyles[category] = WebInspector.TimelinePresentationModel.createFillStyleForCategory(this._context, 0, WebInspector.TimelineEventOverview._innerStripHeight, categories[category]);

    this._disabledCategoryFillStyle = WebInspector.TimelinePresentationModel.createFillStyle(this._context, 0, WebInspector.TimelineEventOverview._innerStripHeight,
        "rgb(218, 218, 218)", "rgb(170, 170, 170)", "rgb(143, 143, 143)");

    this._disabledCategoryBorderStyle = "rgb(143, 143, 143)";
}

/** @const */
WebInspector.TimelineEventOverview._canvasHeight = 60;
/** @const */
WebInspector.TimelineEventOverview._numberOfStrips = 3;
/** @const */
WebInspector.TimelineEventOverview._stripHeight = Math.round(WebInspector.TimelineEventOverview._canvasHeight  / WebInspector.TimelineEventOverview._numberOfStrips);
/** @const */
WebInspector.TimelineEventOverview._stripPadding = 4;
/** @const */
WebInspector.TimelineEventOverview._innerStripHeight = WebInspector.TimelineEventOverview._stripHeight - 2 * WebInspector.TimelineEventOverview._stripPadding;

WebInspector.TimelineEventOverview.prototype = {
    update: function()
    {
        // Use real world, 1:1 coordinates in canvas. This will also take care of clearing it.
        this._canvas.width = this.element.parentElement.clientWidth;
        this._canvas.height = WebInspector.TimelineEventOverview._canvasHeight;

        var timeOffset = this._model.minimumRecordTime();
        var timeSpan = this._model.maximumRecordTime() - timeOffset;
        var scale = this._canvas.width / timeSpan;

        var lastBarByGroup = [];

        this._context.fillStyle = "rgba(0, 0, 0, 0.05)";
        for (var i = 1; i < WebInspector.TimelineEventOverview._numberOfStrips; i += 2)
            this._context.fillRect(0.5, i * WebInspector.TimelineEventOverview._stripHeight + 0.5, this._canvas.width, WebInspector.TimelineEventOverview._stripHeight);

        function appendRecord(record)
        {
            if (record.type === WebInspector.TimelineModel.RecordType.BeginFrame)
                return;
            var recordStart = Math.floor((WebInspector.TimelineModel.startTimeInSeconds(record) - timeOffset) * scale);
            var recordEnd = Math.ceil((WebInspector.TimelineModel.endTimeInSeconds(record) - timeOffset) * scale);
            var category = WebInspector.TimelinePresentationModel.categoryForRecord(record);
            if (category.overviewStripGroupIndex < 0)
                return;
            var bar = lastBarByGroup[category.overviewStripGroupIndex];
            // This bar may be merged with previous -- so just adjust the previous bar.
            const barsMergeThreshold = 2;
            if (bar && bar.category === category && bar.end + barsMergeThreshold >= recordStart) {
                if (recordEnd > bar.end)
                    bar.end = recordEnd;
                return;
            }
            if (bar)
                this._renderBar(bar.start, bar.end, bar.category);
            lastBarByGroup[category.overviewStripGroupIndex] = { start: recordStart, end: recordEnd, category: category };
        }
        WebInspector.TimelinePresentationModel.forAllRecords(this._model.records, appendRecord.bind(this));
        for (var i = 0; i < lastBarByGroup.length; ++i) {
            if (lastBarByGroup[i])
                this._renderBar(lastBarByGroup[i].start, lastBarByGroup[i].end, lastBarByGroup[i].category);
        }
    },

    categoryVisibilityChanged: function()
    {
        this.update();
    },

    _renderBar: function(begin, end, category)
    {
        var x = begin + 0.5;
        var y = category.overviewStripGroupIndex * WebInspector.TimelineEventOverview._stripHeight + WebInspector.TimelineEventOverview._stripPadding + 0.5;
        var width = Math.max(end - begin, 1);

        this._context.save();
        this._context.translate(x, y);
        this._context.fillStyle = category.hidden ? this._disabledCategoryFillStyle : this._fillStyles[category.name];
        this._context.fillRect(0, 0, width, WebInspector.TimelineEventOverview._innerStripHeight);
        this._context.strokeStyle = category.hidden ? this._disabledCategoryBorderStyle : category.borderColor;
        this._context.strokeRect(0, 0, width, WebInspector.TimelineEventOverview._innerStripHeight);
        this._context.restore();
    },

    __proto__: WebInspector.TimelineOverviewBase.prototype
}

/**
 * @constructor
 * @extends {WebInspector.TimelineOverviewBase}
 * @param {WebInspector.TimelineModel} model
 */
WebInspector.TimelineFrameOverview = function(model)
{
    WebInspector.TimelineOverviewBase.call(this, model);
    this._canvas.classList.add("timeline-frame-overview-bars");
    this.reset();

    this._outerPadding = 4;
    this._maxInnerBarWidth = 10;

    // The below two are really computed by update() -- but let's have something so that windowTimes() is happy.
    this._actualPadding = 5;
    this._actualOuterBarWidth = this._maxInnerBarWidth + this._actualPadding;

    this._context = this._canvas.getContext("2d");

    this._fillStyles = {};
    var categories = WebInspector.TimelinePresentationModel.categories();
    for (var category in categories)
        this._fillStyles[category] = WebInspector.TimelinePresentationModel.createFillStyleForCategory(this._context, this._maxInnerBarWidth, 0, categories[category]);
}

WebInspector.TimelineFrameOverview.prototype = {
    reset: function()
    {
        this._recordsPerBar = 1;
        this._barTimes = [];
        this._frames = [];
    },

    update: function()
    {
        const minBarWidth = 4;
        this._framesPerBar = Math.max(1, this._frames.length * minBarWidth / this.element.clientWidth);
        this._barTimes = [];
        var visibleFrames = this._aggregateFrames(this._framesPerBar);

        const paddingTop = 4;

        // Optimize appearance for 30fps. However, if at least half frames won't fit at this scale,
        // fall back to using autoscale.
        const targetFPS = 30;
        var fullBarLength = 1.0 / targetFPS;
        if (fullBarLength < this._medianFrameLength)
            fullBarLength = Math.min(this._medianFrameLength * 2, this._maxFrameLength);

        var scale = (this._canvas.clientHeight - paddingTop) / fullBarLength;
        this._renderBars(visibleFrames, scale);
    },

    /**
     * @param {WebInspector.TimelineFrame} frame
     */
    addFrame: function(frame)
    {
        this._frames.push(frame);
    },

    framePosition: function(frame)
    {
        var frameNumber = this._frames.indexOf(frame);
        if (frameNumber < 0)
            return;
        var barNumber = Math.floor(frameNumber / this._framesPerBar);
        var firstBar = this._framesPerBar > 1 ? barNumber : Math.max(barNumber - 1, 0);
        var lastBar = this._framesPerBar > 1 ? barNumber : Math.min(barNumber + 1, this._barTimes.length - 1);
        return {
            start: Math.ceil(this._barNumberToScreenPosition(firstBar) - this._actualPadding / 2),
            end: Math.floor(this._barNumberToScreenPosition(lastBar + 1) - this._actualPadding / 2)
        }
    },

    /**
     * @param {number} framesPerBar
     */
    _aggregateFrames: function(framesPerBar)
    {
        var visibleFrames = [];
        var durations = [];

        this._maxFrameLength = 0;

        for (var barNumber = 0, currentFrame = 0; currentFrame < this._frames.length; ++barNumber) {
            var barStartTime = this._frames[currentFrame].startTime;
            var longestFrame = null;

            for (var lastFrame = Math.min(Math.floor((barNumber + 1) * framesPerBar), this._frames.length);
                 currentFrame < lastFrame; ++currentFrame) {
                if (!longestFrame || longestFrame.duration < this._frames[currentFrame].duration)
                    longestFrame = this._frames[currentFrame];
            }
            var barEndTime = this._frames[currentFrame - 1].endTime;
            if (longestFrame) {
                this._maxFrameLength = Math.max(this._maxFrameLength, longestFrame.duration);
                visibleFrames.push(longestFrame);
                this._barTimes.push({ startTime: barStartTime, endTime: barEndTime });
                durations.push(longestFrame.duration);
            }
        }
        this._medianFrameLength = durations.qselect(Math.floor(durations.length / 2));
        return visibleFrames;
    },

    /**
     * @param {Array.<WebInspector.TimelineFrame>} frames
     * @param {number} scale
     */
    _renderBars: function(frames, scale)
    {
        // Use real world, 1:1 coordinates in canvas. This will also take care of clearing it.
        this._canvas.width = this._canvas.clientWidth;
        this._canvas.height = this._canvas.clientHeight;

        const maxPadding = 5;
        this._actualOuterBarWidth = Math.min((this._canvas.width - 2 * this._outerPadding) / frames.length, this._maxInnerBarWidth + maxPadding);
        this._actualPadding = Math.min(Math.floor(this._actualOuterBarWidth / 3), maxPadding);

        var barWidth = this._actualOuterBarWidth - this._actualPadding;
        for (var i = 0; i < frames.length; ++i)
            this._renderBar(this._barNumberToScreenPosition(i), barWidth, frames[i], scale);

        this._drawFPSMarks(scale);
    },

    /**
     * @param {number} n
     */
    _barNumberToScreenPosition: function(n)
    {
        return this._outerPadding + this._actualOuterBarWidth * n;
    },

    /**
     * @param {number} scale
     */
    _drawFPSMarks: function(scale)
    {
        const fpsMarks = [30, 60];

        this._context.save();
        this._context.beginPath();
        this._context.font = "9px monospace";
        this._context.textAlign = "right";
        this._context.textBaseline = "top";

        const labelPadding = 2;
        var lineHeight = 12;
        var labelTopMargin = 0;

        for (var i = 0; i < fpsMarks.length; ++i) {
            var fps = fpsMarks[i];
            // Draw lines one pixel above they need to be, so 60pfs line does not cross most of the frames tops.
            var y = this._canvas.height - Math.floor(1.0 / fps * scale) - 0.5;
            var label = fps + " FPS ";
            var labelWidth = this._context.measureText(label).width;
            var labelX = this._canvas.width;
            var labelY;

            if (labelTopMargin < y - lineHeight)
                labelY = y - lineHeight;
            else if (y + lineHeight < this._canvas.height)
                labelY = y;
            else
                break; // No space for the label, so no line as well.

            this._context.moveTo(0, y);
            this._context.lineTo(this._canvas.width, y);

            this._context.fillStyle = "rgba(255, 255, 255, 0.75)";
            this._context.fillRect(labelX - labelWidth - labelPadding, labelY, labelWidth + 2 * labelPadding, lineHeight);
            this._context.fillStyle = "rgb(0, 0, 0)";
            this._context.fillText(label, labelX, labelY);
            labelTopMargin = labelY + lineHeight;
        }
        this._context.strokeStyle = "rgb(51, 51, 51)";
        this._context.stroke();
        this._context.restore();
    },

    _renderBar: function(left, width, frame, scale)
    {
        var categories = Object.keys(WebInspector.TimelinePresentationModel.categories());
        if (!categories.length)
            return;
        var x = Math.floor(left) + 0.5;
        width = Math.floor(width);

        for (var i = 0, bottomOffset = this._canvas.height; i < categories.length; ++i) {
            var category = categories[i];
            var duration = frame.timeByCategory[category];

            if (!duration)
                continue;
            var height = duration * scale;
            var y = Math.floor(bottomOffset - height) + 0.5;

            this._context.save();
            this._context.translate(x, 0);
            this._context.scale(width / this._maxInnerBarWidth, 1);
            this._context.fillStyle = this._fillStyles[category];
            this._context.fillRect(0, y, this._maxInnerBarWidth, Math.floor(height));
            this._context.restore();

            this._context.strokeStyle = WebInspector.TimelinePresentationModel.categories()[category].borderColor;
            this._context.strokeRect(x, y, width, Math.floor(height));
            bottomOffset -= height - 1;
        }
        // Draw a contour for the rest of frame time that we did not instrument.
        var nonCPUTime = frame.duration - frame.cpuTime;
        var y0 = Math.floor(bottomOffset - nonCPUTime * scale) + 0.5;
        var y1 = Math.floor(bottomOffset) + 0.5;

        this._context.strokeStyle = "rgb(90, 90, 90)";
        this._context.beginPath();
        this._context.moveTo(x, y1);
        this._context.lineTo(x, y0);
        this._context.lineTo(x + width, y0);
        this._context.lineTo(x + width, y1);
        this._context.stroke();
    },

    windowTimes: function(windowLeft, windowRight)
    {
        var windowSpan = this.element.clientWidth;
        var leftOffset = windowLeft * windowSpan - this._outerPadding + this._actualPadding;
        var rightOffset = windowRight * windowSpan - this._outerPadding;
        var bars = this.element.children;
        var firstBar = Math.floor(Math.max(leftOffset, 0) / this._actualOuterBarWidth);
        var lastBar = Math.min(Math.floor(rightOffset / this._actualOuterBarWidth), this._barTimes.length - 1);
        const snapToRightTolerancePixels = 3;
        return {
            startTime: firstBar >= this._barTimes.length ? Infinity : this._barTimes[firstBar].startTime,
            endTime: rightOffset + snapToRightTolerancePixels > windowSpan ? Infinity : this._barTimes[lastBar].endTime
        }
    },

    __proto__: WebInspector.TimelineOverviewBase.prototype
}

/**
 * @param {WebInspector.TimelineOverviewPane} pane
 * @constructor
 * @implements {WebInspector.TimelinePresentationModel.Filter}
 */
WebInspector.TimelineWindowFilter = function(pane)
{
    this._pane = pane;
}

WebInspector.TimelineWindowFilter.prototype = {
    /**
     * @param {!WebInspector.TimelinePresentationModel.Record} record
     * @return {boolean}
     */
    accept: function(record)
    {
        return record.lastChildEndTime >= this._pane._windowStartTime && record.startTime <= this._pane._windowEndTime;
    }
}
