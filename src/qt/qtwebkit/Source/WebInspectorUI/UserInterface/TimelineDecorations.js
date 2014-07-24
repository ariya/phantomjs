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

WebInspector.TimelineDecorations = function()
{
    WebInspector.Object.call(this);

    this._element = document.createElement("div");
    this._element.className = WebInspector.TimelineDecorations.StyleClassName;

    this._headerElement = document.createElement("div");
    this._headerElement.className = WebInspector.TimelineDecorations.HeaderElementStyleClassName;
    this._element.appendChild(this._headerElement);

    this._markersElement = document.createElement("div");
    this._markersElement.className = WebInspector.TimelineDecorations.EventMarkersElementStyleClassName
    this._element.appendChild(this._markersElement);

    this.clear();
}

WebInspector.TimelineDecorations.StyleClassName = "timeline-decorations";
WebInspector.TimelineDecorations.HeaderElementStyleClassName = "header";
WebInspector.TimelineDecorations.DividerElementStyleClassName = "divider";
WebInspector.TimelineDecorations.DividerLabelElementStyleClassName = "label";

WebInspector.TimelineDecorations.EventMarkersElementStyleClassName = "event-markers";
WebInspector.TimelineDecorations.EventMarkerTooltipElementStyleClassName = "event-marker-tooltip";
WebInspector.TimelineDecorations.BaseEventMarkerElementStyleClassName = "event-marker";

WebInspector.TimelineDecorations.prototype = {
    constructor: WebInspector.TimelineDecorations,

    // Public

    get element()
    {
        return this._element;
    },

    get headerElement()
    {
        return this._headerElement
    },

    clear: function()
    {
        this._eventMarkers = [];
        this._markersElement.removeChildren();
    },

    updateHeaderTimes: function(boundarySpan, leftPadding, rightPadding, force)
    {
        if (!this.isShowingHeaderDividers())
            return;

        if (isNaN(boundarySpan))
            return;

        leftPadding = leftPadding || 0;
        rightPadding = rightPadding || 0;

        var clientWidth = this._headerElement.clientWidth;
        var leftVisibleEdge = leftPadding;
        var rightVisibleEdge = clientWidth - rightPadding;
        var visibleWidth = rightVisibleEdge - leftVisibleEdge;
        if (visibleWidth <= 0)
            return;

        var dividerCount = Math.round(visibleWidth / 64);
        var slice = boundarySpan / dividerCount;
        if (!force && this._currentDividerSlice === slice)
            return;

        this._currentDividerSlice = slice;

        var dividerElement = this._headerElement.firstChild;
        for (var i = 0; i <= dividerCount; ++i) {
            if (!dividerElement) {
                dividerElement = document.createElement("div");
                dividerElement.className = WebInspector.TimelineDecorations.DividerElementStyleClassName;
                this._headerElement.appendChild(dividerElement);

                var labelElement = document.createElement("div");
                labelElement.className = WebInspector.TimelineDecorations.DividerLabelElementStyleClassName;
                dividerElement._labelElement = labelElement;
                dividerElement.appendChild(labelElement);
            }

            var left = visibleWidth * (i / dividerCount);
            var totalLeft = left + leftVisibleEdge;
            var fractionLeft = totalLeft / clientWidth;
            var percentLeft = 100 * fractionLeft;
            var time = fractionLeft * boundarySpan;

            var currentPercentLeft = parseFloat(dividerElement.style.left);
            if (isNaN(currentPercentLeft) || Math.abs(currentPercentLeft - percentLeft) >= 0.1)
                dividerElement.style.left = percentLeft + "%";

            dividerElement._labelElement.textContent = isNaN(slice) ? "" : Number.secondsToString(time);
            dividerElement = dividerElement.nextSibling;
        }

        // Remove extra dividers.
        while (dividerElement) {
            var nextDividerElement = dividerElement.nextSibling;
            this._headerElement.removeChild(dividerElement);
            dividerElement = nextDividerElement;
        }
    },

    updateEventMarkers: function(minimumBoundary, maximumBoundary)
    {
        this._markersElement.removeChildren();

        if (!this._eventMarkers.length)
            return;

        var boundarySpan = maximumBoundary - minimumBoundary;
        if (isNaN(boundarySpan))
            return;

        function toolTipForEventMarker(eventMarker, time)
        {
            switch (eventMarker.type) {
            case WebInspector.TimelineEventMarker.Type.LoadEvent:
                return WebInspector.UIString("Load event fired at %s").format(Number.secondsToString(time));
            case WebInspector.TimelineEventMarker.Type.DOMContentEvent:
                return WebInspector.UIString("DOMContent event fired at %s").format(Number.secondsToString(time));
            case WebInspector.TimelineEventMarker.Type.TimeStamp:
                return WebInspector.UIString("Timestamp marker at %s").format(Number.secondsToString(time));
            default:
                console.assert(false);
                return "";
            }
        }

        for (var i = 0; i < this._eventMarkers.length; ++i) {
            var eventMarker = this._eventMarkers[i];
            if (eventMarker.timestamp < minimumBoundary || eventMarker.timestamp > maximumBoundary)
                continue;

            var time = eventMarker.timestamp - minimumBoundary;
            var percentLeft = (100 * time) / boundarySpan;

            var tooltipElement = document.createElement("div");
            tooltipElement.className = WebInspector.TimelineDecorations.EventMarkerTooltipElementStyleClassName;
            tooltipElement.title = toolTipForEventMarker(eventMarker, time);
            tooltipElement.style.left = percentLeft + "%";

            var markerElement = document.createElement("div");
            markerElement.className = WebInspector.TimelineDecorations.BaseEventMarkerElementStyleClassName;
            markerElement.classList.add(eventMarker.type);
            markerElement.style.left = percentLeft + "%";

            this._markersElement.appendChild(markerElement);
            this._markersElement.appendChild(tooltipElement);
        }
    },

    isShowingHeaderDividers: function()
    {
        return !this._headerElement.classList.contains("hidden");
    },

    showHeaderDividers: function()
    {
        this._headerElement.classList.remove("hidden");
    },

    hideHeaderDividers: function()
    {
        this._headerElement.classList.add("hidden");
    },

    addTimelineEventMarker: function(eventMarker)
    {
        this._eventMarkers.push(eventMarker);
    },
}

WebInspector.TimelineDecorations.prototype.__proto__ = WebInspector.Object.prototype;
