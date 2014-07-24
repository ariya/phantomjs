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

WebInspector.TimelineOverview = function(dataSource, recordTypes)
{
    WebInspector.Object.call(this);

    console.assert(dataSource);
    console.assert(recordTypes instanceof Array);

    this._element = document.createElement("div");
    this._element.className = WebInspector.TimelineOverview.StyleClassName;

    this._recordTypes = recordTypes;

    this._timelineDecorations = new WebInspector.TimelineDecorations;

    this._timelineElements = [];
    for (var i = 0; i < this._recordTypes.length; ++i) {
        var timelineElement = document.createElement("div");
        timelineElement.classList.add(WebInspector.TimelineOverview.TimelineElementStyleClassName);
        timelineElement.classList.add(this._recordTypes[i]);
        this._element.appendChild(timelineElement);

        var barContainerElement = document.createElement("div");
        barContainerElement.className = WebInspector.TimelineOverview.BarContainerElementStyleClassName;
        timelineElement.appendChild(barContainerElement);
        timelineElement._barContainerElement = barContainerElement;

        this._timelineElements.push(timelineElement);
    }

    this._element.appendChild(this._timelineDecorations.element);

    this._dataSource = dataSource;
};

WebInspector.TimelineOverview.StyleClassName = "timeline-overview";
WebInspector.TimelineOverview.TimelineElementStyleClassName = "timeline";
WebInspector.TimelineOverview.BarContainerElementStyleClassName = "bar-container";
WebInspector.TimelineOverview.BarElementStyleClassName = "bar";
WebInspector.TimelineOverview.WaitingBarElementStyleClassName = "waiting";

WebInspector.TimelineOverview.prototype = {
    constructor: WebInspector.TimelineOverview,

    // Public

    get element()
    {
        return this._element;
    },

    get recordTypes()
    {
        return this._recordTypes;
    },

    clear: function()
    {
        this._minimumBoundary = NaN;
        this._maximumBoundary = NaN;
        this._boundarySpan = NaN;

        this._timelineDecorations.clear();
        this._timelineDecorations.updateHeaderTimes(this._boundarySpan);

        for (var i = 0; i < this._timelineElements.length; ++i)
            this._timelineElements[i]._barContainerElement.removeChildren();
    },

    update: function()
    {
        var minimumBoundary = NaN;
        var maximumBoundary = NaN;

        var records = [];
        for (var i = 0; i < this._recordTypes.length; ++i)
            records.push(this._dataSource.timelineOverviewRecordsWithType(this._recordTypes[i]));

        for (var i = 0; i < records.length; ++i) {
            var currentRecords = records[i];
            if (!currentRecords.length)
                continue;

            // The first record can be assumed to be the oldest start time.
            var firstRecord = currentRecords[0];
            if (isNaN(minimumBoundary) || firstRecord.startTime < minimumBoundary)
                minimumBoundary = firstRecord.startTime;

            if (this._recordTypes[i] === WebInspector.TimelineRecord.Type.Network) {
                // For the Network timeline we need to look at all the records since
                // long loading resources can be anywhere, not just at the end. The other
                // timelines are static, so the last record is always the newest.
                for (var j = currentRecords.length - 1; j >= 0; --j) {
                    var record = currentRecords[j];
                    if (isNaN(maximumBoundary) || record.endTime > maximumBoundary)
                        maximumBoundary = record.endTime;
                }
            } else {
                // The newest record will always be at the end.
                var lastRecord = currentRecords.lastValue;
                if (isNaN(maximumBoundary) || lastRecord.endTime > maximumBoundary)
                    maximumBoundary = lastRecord.endTime;
            }
        }

        this._minimumBoundary = minimumBoundary;
        this._maximumBoundary = maximumBoundary;
        this._boundarySpan = maximumBoundary - minimumBoundary;

        this._timelineDecorations.updateHeaderTimes(this._boundarySpan);
        this._timelineDecorations.updateEventMarkers(this._minimumBoundary, this._maximumBoundary);

        for (var i = 0; i < records.length; ++i)
            this._updateTimelineBars(this._timelineElements[i], this._recordTypes[i], records[i]);
    },

    updateLayout: function()
    {
        this._timelineDecorations.updateHeaderTimes(this._boundarySpan);
    },

    addTimelineEventMarker: function(eventMarker)
    {
        this._timelineDecorations.addTimelineEventMarker(eventMarker);
    },

    // Private

    _updateTimelineBars: function(timelineElement, type, records)
    {
        function computeActivePercentages(record)
        {
            // Compute percentages for endTime and the start of activeDuration.

            var activeStartTime = record.endTime - record.activeDuration;
            if (isNaN(activeStartTime))
                activeStartTime = record.endTime;

            if (isNaN(activeStartTime))
                var start = 0;
            else
                var start = ((activeStartTime - this._minimumBoundary) / this._boundarySpan) * 100;

            if (isNaN(record.endTime))
                var end = 100;
            else
                var end = ((record.endTime - this._minimumBoundary) / this._boundarySpan) * 100;

            return {start: start, end: end};
        }

        function computePercentages(record)
        {
            // Compute percentages for startTime and endTime.

            if (isNaN(record.startTime))
                var start = 0;
            else
                var start = ((record.startTime - this._minimumBoundary) / this._boundarySpan) * 100;

            if (isNaN(record.endTime))
                var end = 100;
            else
                var end = ((record.endTime - this._minimumBoundary) / this._boundarySpan) * 100;

            return {start: start, end: end};
        }

        function addBar(startPercentage, endPercentage, extraBarStyleClass)
        {
            var barElement = document.createElement("div");
            barElement.className = WebInspector.TimelineOverview.BarElementStyleClassName;
            if (extraBarStyleClass)
                barElement.classList.add(extraBarStyleClass);

            if (startPercentage === 0 || startPercentage !== endPercentage)
                barElement.style.left = startPercentage + "%";
            barElement.style.right = (100 - endPercentage) + "%";

            timelineElement._barContainerElement.appendChild(barElement);
        }

        function createBars(percentageCalculator, extraBarStyleClass)
        {
            // Iterate over the records and find the percentage distribution on the graph.
            // The percentages are recorded in a sparse array from 0-100.

            var timelineSlots = new Array(101);
            for (var i = 0; i < records.length; ++i) {
                var percentages = percentageCalculator.call(this, records[i]);
                var end = Math.round(percentages.end);
                for (var j = Math.round(percentages.start); j <= end; ++j)
                    timelineSlots[j] = true;
            }

            var barStart = NaN;
            for (var i = 0; i < timelineSlots.length; ++i) {
                if (timelineSlots[i]) {
                    if (isNaN(barStart))
                        barStart = i;
                } else {
                    if (!isNaN(barStart)) {
                        addBar.call(this, barStart, i, extraBarStyleClass);
                        barStart = NaN;
                    }
                }
            }

            if (!isNaN(barStart)) {
                addBar.call(this, barStart, 100, extraBarStyleClass);
                barStart = NaN;
            }
        }

        timelineElement._barContainerElement.removeChildren();

        // The Network timeline shows two bars. The first is the total time (waiting for the resource until it finishes).
        // The second bar is the active download time (first first response to finish.)
        if (type === WebInspector.TimelineRecord.Type.Network) {
            createBars.call(this, computePercentages, WebInspector.TimelineOverview.WaitingBarElementStyleClassName);
            createBars.call(this, computeActivePercentages);
        } else
            createBars.call(this, computePercentages);
    }
};

WebInspector.TimelineOverview.prototype.__proto__ = WebInspector.Object.prototype;
