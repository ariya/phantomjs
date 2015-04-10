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
 * @param {WebInspector.TimelinePanel} timelinePanel
 * @param {WebInspector.TimelineModel} model
 * @param {number} sidebarWidth
 * @constructor
 * @extends {WebInspector.MemoryStatistics}
 */
WebInspector.NativeMemoryGraph = function(timelinePanel, model, sidebarWidth)
{
    WebInspector.MemoryStatistics.call(this, timelinePanel, model, sidebarWidth);
}

/**
 * @constructor
 * @extends {WebInspector.MemoryStatistics.Counter}
 */
WebInspector.NativeMemoryGraph.Counter = function(time, nativeCounters)
{
    WebInspector.MemoryStatistics.Counter.call(this, time);
    this.nativeCounters = nativeCounters;
}

/**
 * @constructor
 * @extends {WebInspector.CounterUIBase}
 * @param {WebInspector.NativeMemoryGraph} memoryCountersPane
 * @param {string} title
 * @param {Array.<number>} hsl
 * @param {function(WebInspector.NativeMemoryGraph.Counter):number} valueGetter
 */
WebInspector.NativeMemoryCounterUI = function(memoryCountersPane, title, hsl, valueGetter)
{
    var swatchColor = this._hslToString(hsl);
    WebInspector.CounterUIBase.call(this, memoryCountersPane, title, swatchColor, valueGetter);
    this._value = this._swatch.element.createChild("span", "memory-category-value");

    const borderLightnessDifference = 3;
    hsl[2] -= borderLightnessDifference;
    this.strokeColor = this._hslToString(hsl);
    this.graphYValues = [];
}

WebInspector.NativeMemoryCounterUI.prototype = {
    _hslToString: function(hsl)
    {
        return "hsl(" + hsl[0] + "," + hsl[1] + "%," + hsl[2] + "%)";
    },

    updateCurrentValue: function(countersEntry)
    {
        var bytes = this.valueGetter(countersEntry);
        var megabytes =  bytes / (1024 * 1024);
        this._value.textContent = WebInspector.UIString("%.1f\u2009MB", megabytes);
    },

    clearCurrentValueAndMarker: function(ctx)
    {
        this._value.textContent = "";
    },

    __proto__: WebInspector.CounterUIBase.prototype
}


WebInspector.NativeMemoryGraph.prototype = {
    _createCurrentValuesBar: function()
    {
    },

    _createCounterUIList: function()
    {
        var nativeCounters = [
            "JSExternalResources",
            "CSS",
            "GlyphCache",
            "Image",
            "Resources",
            "DOM",
            "Rendering",
            "Audio",
            "WebInspector",
            "JSHeap.Used",
            "JSHeap.Unused",
            "MallocWaste",
            "Other",
            "PrivateBytes",
        ];

        /**
         * @param {string} name
         * @return {number}
         */
        function getCounterValue(name, entry)
        {
            return (entry.nativeCounters && entry.nativeCounters[name]) || 0;
        }

        var list = [];
        for (var i = nativeCounters.length - 1; i >= 0; i--) {
            var name = nativeCounters[i];
            if ("PrivateBytes" === name) {
                var counterUI = new WebInspector.NativeMemoryCounterUI(this, "Total", [0, 0, 0], getCounterValue.bind(this, name))
                this._privateBytesCounter = counterUI;
            } else {
                var counterUI = new WebInspector.NativeMemoryCounterUI(this, name, [i * 20, 65, 63], getCounterValue.bind(this, name))
                list.push(counterUI);
            }
        }
        return list.reverse();
    },

    _canvasHeight: function()
    {
        return this._canvasContainer.offsetHeight;
    },

    /**
     * @param {WebInspector.Event} event
     */
    _onRecordAdded: function(event)
    {
        var statistics = this._counters;
        function addStatistics(record)
        {
            var nativeCounters = record["nativeHeapStatistics"];
            if (!nativeCounters)
                return;

            var knownSize = 0;
            for (var name in nativeCounters) {
                if (name === "PrivateBytes")
                    continue;
                knownSize += nativeCounters[name];
            }
            nativeCounters["Other"] = nativeCounters["PrivateBytes"] - knownSize;

            statistics.push(new WebInspector.NativeMemoryGraph.Counter(
                record.endTime || record.startTime,
                nativeCounters
            ));
        }
        WebInspector.TimelinePresentationModel.forAllRecords([event.data], null, addStatistics);
    },

    _draw: function()
    {
        WebInspector.MemoryStatistics.prototype._draw.call(this);

        var maxValue = this._maxCounterValue();
        this._resetTotalValues();

        var previousCounterUI;
        for (var i = 0; i < this._counterUI.length; i++) {
            this._drawGraph(this._counterUI[i], previousCounterUI, maxValue);
            if (this._counterUI[i].visible)
                previousCounterUI = this._counterUI[i];
        }
    },

    /**
     * @param {CanvasRenderingContext2D} ctx
     */
    _clearCurrentValueAndMarker: function(ctx)
    {
        WebInspector.MemoryStatistics.prototype._clearCurrentValueAndMarker.call(this, ctx);
        this._privateBytesCounter.clearCurrentValueAndMarker(ctx);
    },

    _updateCurrentValue: function(counterEntry)
    {
        WebInspector.MemoryStatistics.prototype._updateCurrentValue.call(this, counterEntry);
        this._privateBytesCounter.updateCurrentValue(counterEntry);
    },

    /**
     * @param {CanvasRenderingContext2D} ctx
     */
    _restoreImageUnderMarker: function(ctx)
    {
        if (this._imageUnderMarker)
            ctx.putImageData(this._imageUnderMarker.imageData, this._imageUnderMarker.x, this._imageUnderMarker.y);
        this._discardImageUnderMarker();
    },

    /**
     * @param {CanvasRenderingContext2D} ctx
     * @param {number} left
     * @param {number} top
     * @param {number} right
     * @param {number} bottom
     */
    _saveImageUnderMarker: function(ctx, left, top, right, bottom)
    {
        var imageData = ctx.getImageData(left, top, right, bottom);
        this._imageUnderMarker = {
            x: left,
            y: top,
            imageData: imageData
        };
    },

    /**
     * @param {CanvasRenderingContext2D} ctx
     * @param {number} x
     * @param {number} index
     */
    _drawMarker: function(ctx, x, index)
    {
        var left = this._counters[index].x;
        var right = index + 1 < this._counters.length ? this._counters[index + 1].x : left;
        var top = this._originY;
        top = 0;
        var bottom = top + this._clippedHeight;
        bottom += this._originY;

        this._saveImageUnderMarker(ctx, left, top, right, bottom);

        ctx.beginPath();
        ctx.moveTo(left, top);
        ctx.lineTo(right, top);
        ctx.lineTo(right, bottom);
        ctx.lineTo(left, bottom);
        ctx.lineWidth = 1;
        ctx.closePath();
        ctx.fillStyle = "rgba(220,220,220,0.3)";
        ctx.fill();
    },

    /**
     * @return {number}
     */
    _maxCounterValue: function()
    {
        if (!this._counters.length)
            return 0;

        var valueGetter = this._privateBytesCounter.valueGetter;
        var result = 0;
        for (var i = this._minimumIndex; i < this._maximumIndex; i++) {
            var counter = this._counters[i];
            var value = valueGetter(counter);
            if (value > result)
                result = value;
        }
        return result;
    },

    _resetTotalValues: function()
    {
        for (var i = this._minimumIndex; i <= this._maximumIndex; i++) {
            var counter = this._counters[i];
            counter.total = 0;
        }
    },

    /**
     * @param {WebInspector.CounterUIBase} counterUI
     * @param {WebInspector.CounterUIBase} previousCounterUI
     * @param {number} maxTotalValue
     */
    _drawGraph: function(counterUI, previousCounterUI, maxTotalValue)
    {
        var canvas = this._canvas;
        var ctx = canvas.getContext("2d");
        var width = canvas.width;
        var height = this._clippedHeight;
        var originY = this._originY;
        var valueGetter = counterUI.valueGetter;

        if (!this._counters.length)
            return;

        if (!counterUI.visible)
            return;

        for (var i = this._minimumIndex; i <= this._maximumIndex; i++) {
            var counter = this._counters[i];
            var value = valueGetter(counter);
            counter.total += value;
        }

        var yValues = counterUI.graphYValues;
        yValues.length = this._counters.length;

        var maxYRange =  maxTotalValue;
        var yFactor = maxYRange ? height / (maxYRange) : 1;

        ctx.beginPath();
        if (previousCounterUI) {
            var prevYValues = previousCounterUI.graphYValues;
            var currentY = prevYValues[this._maximumIndex];
            ctx.moveTo(width, currentY);
            var currentX = width;
            for (var i = this._maximumIndex - 1; i >= this._minimumIndex; i--) {
                currentY = prevYValues[i];
                currentX = this._counters[i].x;
                ctx.lineTo(currentX, currentY);
            }
        } else {
            var lastY = originY + height;
            ctx.moveTo(width, lastY);
            ctx.lineTo(0, lastY);
        }

        var currentY = originY + (height - this._counters[this._minimumIndex].total * yFactor);
        ctx.lineTo(0, currentY);
        for (var i = this._minimumIndex; i <= this._maximumIndex; i++) {
             var counter = this._counters[i];
             var x = counter.x;
             currentY = originY + (height - counter.total * yFactor);
             ctx.lineTo(x, currentY);

             yValues[i] = currentY;
        }
        ctx.lineTo(width, currentY);
        ctx.closePath();
        ctx.lineWidth = 1;

        ctx.strokeStyle = counterUI.strokeColor;
        ctx.fillStyle = counterUI.graphColor;
        ctx.fill();
        ctx.stroke();
    },

    _discardImageUnderMarker: function()
    {
        delete this._imageUnderMarker;
    },

    __proto__: WebInspector.MemoryStatistics.prototype
}

