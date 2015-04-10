/*
 * Copyright (C) 2007, 2008, 2013 Apple Inc.  All rights reserved.
 * Copyright (C) 2008, 2009 Anthony Ricaud <rik@webkit.org>
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.NetworkTimeCalculator = function(startAtZero)
{
    this.startAtZero = startAtZero;
    this.reset();
}

WebInspector.NetworkTimeCalculator.prototype = {
    get boundarySpan()
    {
        return this.maximumBoundary - this.minimumBoundary;
    },

    reset: function()
    {
        this.minimumBoundary = NaN;
        this.maximumBoundary = NaN;
    },

    computeBarGraphPercentages: function(resource)
    {
        if (!isNaN(resource.requestSentTimestamp))
            var start = ((resource.requestSentTimestamp - this.minimumBoundary) / this.boundarySpan) * 100;
        else
            var start = 0;

        if (!isNaN(resource.responseReceivedTimestamp))
            var middle = ((resource.responseReceivedTimestamp - this.minimumBoundary) / this.boundarySpan) * 100;
        else if (!isNaN(resource.lastTimestamp))
            var middle = ((resource.lastTimestamp - this.minimumBoundary) / this.boundarySpan) * 100;
        else
            var middle = (this.startAtZero ? start : 100);

        if (!isNaN(resource.lastTimestamp))
            var end = ((resource.lastTimestamp - this.minimumBoundary) / this.boundarySpan) * 100;
        else
            var end = (this.startAtZero ? middle : 100);

        if (this.startAtZero) {
            end -= start;
            middle -= start;
            start = 0;
        }

        return {start: start, middle: middle, end: end};
    },

    computeBarGraphLabels: function(resource)
    {
        var rightLabel = "";
        if (!isNaN(resource.responseReceivedTimestamp) && !isNaN(resource.lastTimestamp))
            rightLabel = this.formatValue(resource.lastTimestamp - resource.responseReceivedTimestamp);

        var hasLatency = resource.latency > 0;
        if (hasLatency)
            var leftLabel = this.formatValue(resource.latency);
        else
            var leftLabel = rightLabel;

        if (hasLatency && rightLabel) {
            var total = this.formatValue(resource.duration);
            var tooltip = WebInspector.UIString("%s latency, %s download (%s total)").format(leftLabel, rightLabel, total);
        } else if (hasLatency)
            var tooltip = WebInspector.UIString("%s latency").format(leftLabel);
        else if (rightLabel)
            var tooltip = WebInspector.UIString("%s download").format(rightLabel);

        if (resource.cached)
            tooltip = WebInspector.UIString("%s (from cache)").format(tooltip);
        return {left: leftLabel, right: rightLabel, tooltip: tooltip};
    },

    updateBoundaries: function(resource)
    {
        var didChange = false;

        if (this.startAtZero)
            var lowerBound = 0;
        else
            var lowerBound = this._lowerBound(resource);

        if (!isNaN(lowerBound) && (isNaN(this.minimumBoundary) || lowerBound < this.minimumBoundary)) {
            this.minimumBoundary = lowerBound;
            didChange = true;
        }

        var upperBound = this._upperBound(resource);
        if (!isNaN(upperBound) && (isNaN(this.maximumBoundary) || upperBound > this.maximumBoundary)) {
            this.maximumBoundary = upperBound;
            didChange = true;
        }

        return didChange;
    },

    updateBoundariesForEventMarker: function(eventMarker)
    {
        var didChange = false;
        var timestamp = eventMarker.timestamp;

        if (!this.startAtZero) {
            if (isNaN(this.minimumBoundary) || timestamp < this.minimumBoundary) {
                this.minimumBoundary = timestamp;
                didChange = true;
            }
        }

        // Don't extend the bounds if the timestamp is more than 10% past the current maximum boundary. This prevents dead space.
        const allowedPercentPastEndForTimestamp = 0.10;
        // If the timestamp is within 3% of the maximum boundary, extend the boundary anyways to give room for the marker.
        const allowedPercentNearEndForTimestampToExtendEnd = 0.03;
        // If the timestamp will cause a new maximum boundary, give 3% extra padding, to ensure we show the marker.
        const extendPercentPastEnd = 0.03;

        if (isNaN(this.maximumBoundary))
            this.maximumBoundary = timestamp;
        else {
            var boundary = this.maximumBoundary - this.minimumBoundary;
            console.assert(!isNaN(boundary));

            if (timestamp > this.maximumBoundary) {
                var percentPastEnd = (timestamp - this.maximumBoundary) / boundary;
                if (!boundary || percentPastEnd < allowedPercentPastEndForTimestamp) {
                    var newBoundary = timestamp - this.minimumBoundary;
                    this.maximumBoundary = timestamp + (newBoundary * extendPercentPastEnd);
                    didChange = true;
                }
            } else {
                var percentNearEnd = (this.maximumBoundary - timestamp) / boundary;
                if (percentNearEnd < allowedPercentNearEndForTimestampToExtendEnd) {
                    this.maximumBoundary = this.maximumBoundary + (boundary * extendPercentPastEnd);
                    didChange = true;
                }
            }
        }

        return didChange;
    },

    formatValue: function(value)
    {
        return Number.secondsToString(value);
    },

    _lowerBound: function(resource)
    {
        // Implemented by subclasses if not startAtZero.
        return 0;
    },

    _upperBound: function(resource)
    {
        // Implemented by subclasses.
        return 0;
    }
}

WebInspector.NetworkTimeCalculator.prototype.__proto__ = WebInspector.Object.prototype;


/**
 * @constructor
 * @extends {WebInspector.NetworkTimeCalculator}
 */
WebInspector.NetworkTransferTimeCalculator = function()
{
    WebInspector.NetworkTimeCalculator.call(this, false);
}

WebInspector.NetworkTransferTimeCalculator.prototype = {
    _lowerBound: function(resource)
    {
        return resource.firstTimestamp || NaN;
    },

    _upperBound: function(resource)
    {
        return resource.lastTimestamp || NaN;
    }
}

WebInspector.NetworkTransferTimeCalculator.prototype.__proto__ = WebInspector.NetworkTimeCalculator.prototype;


/**
 * @constructor
 * @extends {WebInspector.NetworkTimeCalculator}
 */
WebInspector.NetworkTransferDurationCalculator = function()
{
    WebInspector.NetworkTimeCalculator.call(this, true);
}

WebInspector.NetworkTransferDurationCalculator.prototype = {
    _upperBound: function(resource)
    {
        return resource.duration || 0;
    }
}

WebInspector.NetworkTransferDurationCalculator.prototype.__proto__ = WebInspector.NetworkTimeCalculator.prototype;
