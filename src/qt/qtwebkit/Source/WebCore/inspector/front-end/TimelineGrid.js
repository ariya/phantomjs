/*
 * Copyright (C) 2007, 2008 Apple Inc.  All rights reserved.
 * Copyright (C) 2008, 2009 Anthony Ricaud <rik@webkit.org>
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

/**
 * @constructor
 */
WebInspector.TimelineGrid = function()
{
    this.element = document.createElement("div");

    this._itemsGraphsElement = document.createElement("div");
    this._itemsGraphsElement.id = "resources-graphs";
    this.element.appendChild(this._itemsGraphsElement);

    this._dividersElement = this.element.createChild("div", "resources-dividers");

    this._gridHeaderElement = document.createElement("div"); 
    this._eventDividersElement = this._gridHeaderElement.createChild("div", "resources-event-dividers");
    this._dividersLabelBarElement = this._gridHeaderElement.createChild("div", "resources-dividers-label-bar");
    this.element.appendChild(this._gridHeaderElement);

    this._leftCurtainElement = this.element.createChild("div", "timeline-cpu-curtain-left");
    this._rightCurtainElement = this.element.createChild("div", "timeline-cpu-curtain-right");
}

WebInspector.TimelineGrid.prototype = {
    get itemsGraphsElement()
    {
        return this._itemsGraphsElement;
    },

    get dividersElement()
    {
        return this._dividersElement;
    },

    get dividersLabelBarElement()
    {
        return this._dividersLabelBarElement;
    },

    get gridHeaderElement()
    {
        return this._gridHeaderElement;
    },

    removeDividers: function()
    {
        this._dividersElement.removeChildren();
        this._dividersLabelBarElement.removeChildren();
    },

    updateDividers: function(calculator)
    {
        var dividersElementClientWidth = this._dividersElement.clientWidth;
        var dividerCount = Math.round(dividersElementClientWidth / 64);
        var slice = calculator.boundarySpan() / dividerCount;

        this._currentDividerSlice = slice;

        // Reuse divider elements and labels.
        var divider = this._dividersElement.firstChild;
        var dividerLabelBar = this._dividersLabelBarElement.firstChild;

        var sliceRemainder = (calculator.minimumBoundary() - calculator.zeroTime()) % slice;
        for (var i = 0; i <= dividerCount; ++i) {
            if (!divider) {
                divider = document.createElement("div");
                divider.className = "resources-divider";
                this._dividersElement.appendChild(divider);

                dividerLabelBar = document.createElement("div");
                dividerLabelBar.className = "resources-divider";
                var label = document.createElement("div");
                label.className = "resources-divider-label";
                dividerLabelBar._labelElement = label;
                dividerLabelBar.appendChild(label);
                this._dividersLabelBarElement.appendChild(dividerLabelBar);
            }

            if (!i) {
                divider.addStyleClass("first");
                dividerLabelBar.addStyleClass("first");
            } else {
                divider.removeStyleClass("first");
                dividerLabelBar.removeStyleClass("first");
            }

            if (i === dividerCount) {
                divider.addStyleClass("last");
                dividerLabelBar.addStyleClass("last");
            } else {
                divider.removeStyleClass("last");
                dividerLabelBar.removeStyleClass("last");
            }

            var left;
            if (!slice) {
                left = dividersElementClientWidth / dividerCount * i;
                dividerLabelBar._labelElement.textContent = "";
            } else {
                left = calculator.computePosition(calculator.minimumBoundary() + slice * i - sliceRemainder);
                dividerLabelBar._labelElement.textContent = calculator.formatTime(slice * i - sliceRemainder);
            }
            var percentLeft = 100 * left / dividersElementClientWidth;
            this._setDividerAndBarLeft(divider, dividerLabelBar, percentLeft);

            divider = divider.nextSibling;
            dividerLabelBar = dividerLabelBar.nextSibling;
        }

        // Remove extras.
        while (divider) {
            var nextDivider = divider.nextSibling;
            this._dividersElement.removeChild(divider);
            divider = nextDivider;
        }
        while (dividerLabelBar) {
            var nextDivider = dividerLabelBar.nextSibling;
            this._dividersLabelBarElement.removeChild(dividerLabelBar);
            dividerLabelBar = nextDivider;
        }
        return true;
    },

    _setDividerAndBarLeft: function(divider, dividerLabelBar, percentLeft)
    {
        var percentStyleLeft = parseFloat(divider.style.left);
        if (!isNaN(percentStyleLeft) && Math.abs(percentStyleLeft - percentLeft) < 0.1)
            return;
        divider.style.left = percentLeft + "%";
        dividerLabelBar.style.left = percentLeft + "%";
    },

    addEventDivider: function(divider)
    {
        this._eventDividersElement.appendChild(divider);
    },

    addEventDividers: function(dividers)
    {
        this._gridHeaderElement.removeChild(this._eventDividersElement);
        for (var i = 0; i < dividers.length; ++i) {
            if (dividers[i])
                this._eventDividersElement.appendChild(dividers[i]);
        }
        this._gridHeaderElement.appendChild(this._eventDividersElement);
    },

    removeEventDividers: function()
    {
        this._eventDividersElement.removeChildren();
    },

    hideEventDividers: function()
    {
        this._eventDividersElement.addStyleClass("hidden");
    },

    showEventDividers: function()
    {
        this._eventDividersElement.removeStyleClass("hidden");
    },

    hideCurtains: function()
    {
        this._leftCurtainElement.addStyleClass("hidden");
        this._rightCurtainElement.addStyleClass("hidden");
    },

    /**
     * @param {number} gapOffset
     * @param {number} gapWidth
     */
    showCurtains: function(gapOffset, gapWidth)
    {
        this._leftCurtainElement.style.width = gapOffset + "px";
        this._leftCurtainElement.removeStyleClass("hidden");
        this._rightCurtainElement.style.left = (gapOffset + gapWidth) + "px";
        this._rightCurtainElement.removeStyleClass("hidden");
    },

    setScrollAndDividerTop: function(scrollTop, dividersTop)
    {
        this._dividersElement.style.top = scrollTop + "px";
        this._leftCurtainElement.style.top = scrollTop + "px";
        this._rightCurtainElement.style.top = scrollTop + "px";
    }
}

/**
 * @interface
 */
WebInspector.TimelineGrid.Calculator = function() { }

WebInspector.TimelineGrid.Calculator.prototype = {
    /** @param {number} time */
    computePosition: function(time) { },

    /** @param {number} time */
    formatTime: function(time) { },

    /** @return {number} */
    minimumBoundary: function() { },

    /** @return {number} */
    zeroTime: function() { },

    /** @return {number} */
    maximumBoundary: function() { },

    /** @return {number} */
    boundarySpan: function() { }
}
