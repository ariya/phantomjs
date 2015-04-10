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
 * @param {string} prefix
 */
WebInspector.OverviewGrid = function(prefix)
{
    this.element = document.createElement("div");
    this.element.className = "fill";
    this.element.id = prefix + "-overview-container";

    this._grid = new WebInspector.TimelineGrid();
    this._grid.element.id = prefix + "-overview-grid";
    this._grid.setScrollAndDividerTop(0, 0);

    this.element.appendChild(this._grid.element);

    this._window = new WebInspector.OverviewGrid.Window(this.element, this._grid.dividersLabelBarElement);
}

WebInspector.OverviewGrid.prototype = {
    /**
     * @return {number}
     */
    clientWidth: function()
    {
        return this.element.clientWidth;
    },

    /**
     * @param {!WebInspector.TimelineGrid.Calculator} calculator
     */
    updateDividers: function(calculator)
    {
        this._grid.updateDividers(calculator);
    },

    /**
     * @param {!Array.<Element>} dividers
     */
    addEventDividers: function(dividers)
    {
        this._grid.addEventDividers(dividers);
    },

    removeEventDividers: function()
    {
        this._grid.removeEventDividers();
    },

    /**
     * @param {?number} start
     * @param {?number} end
     */
    setWindowPosition: function(start, end)
    {
        this._window._setWindowPosition(start, end);
    },

    reset: function()
    {
        this._window.reset();
    },

    /**
     * @return {number}
     */
    windowLeft: function()
    {
        return this._window.windowLeft;
    },

    /**
     * @return {number}
     */
    windowRight: function()
    {
        return this._window.windowRight;
    },

    /**
     * @param {number} left
     * @param {number} right
     */
    setWindow: function(left, right)
    {
        this._window._setWindow(left, right);
    },

    /**
     * @param {string} eventType
     * @param {function(WebInspector.Event)} listener
     * @param {Object=} thisObject
     */
    addEventListener: function(eventType, listener, thisObject)
    {
        this._window.addEventListener(eventType, listener, thisObject);
    },

    /**
     * @param {!number} zoomFactor
     * @param {!number} referencePoint
     */
    zoom: function(zoomFactor, referencePoint)
    {
        this._window._zoom(zoomFactor, referencePoint);
    }
}


WebInspector.OverviewGrid.MinSelectableSize = 12;

WebInspector.OverviewGrid.WindowScrollSpeedFactor = .3;

WebInspector.OverviewGrid.ResizerOffset = 3.5; // half pixel because offset values are not rounded but ceiled

/**
 * @constructor
 * @extends {WebInspector.Object}
 * @param {Element} parentElement
 * @param {Element} dividersLabelBarElement
 */
WebInspector.OverviewGrid.Window = function(parentElement, dividersLabelBarElement)
{
    this._parentElement = parentElement;
    this._dividersLabelBarElement = dividersLabelBarElement;

    WebInspector.installDragHandle(this._parentElement, this._startWindowSelectorDragging.bind(this), this._windowSelectorDragging.bind(this), this._endWindowSelectorDragging.bind(this), "ew-resize");
    WebInspector.installDragHandle(this._dividersLabelBarElement, this._startWindowDragging.bind(this), this._windowDragging.bind(this), null, "move");

    this.windowLeft = 0.0;
    this.windowRight = 1.0;

    this._parentElement.addEventListener("mousewheel", this._onMouseWheel.bind(this), true);
    this._parentElement.addEventListener("dblclick", this._resizeWindowMaximum.bind(this), true);

    this._overviewWindowElement = document.createElement("div");
    this._overviewWindowElement.className = "overview-grid-window";
    parentElement.appendChild(this._overviewWindowElement);

    this._overviewWindowBordersElement = document.createElement("div");
    this._overviewWindowBordersElement.className = "overview-grid-window-rulers";
    parentElement.appendChild(this._overviewWindowBordersElement);

    var overviewDividersBackground = document.createElement("div");
    overviewDividersBackground.className = "overview-grid-dividers-background";
    parentElement.appendChild(overviewDividersBackground);

    this._leftResizeElement = document.createElement("div");
    this._leftResizeElement.className = "overview-grid-window-resizer";
    this._leftResizeElement.style.left = 0;
    parentElement.appendChild(this._leftResizeElement);
    WebInspector.installDragHandle(this._leftResizeElement, this._resizerElementStartDragging.bind(this), this._leftResizeElementDragging.bind(this), null, "ew-resize");

    this._rightResizeElement = document.createElement("div");
    this._rightResizeElement.className = "overview-grid-window-resizer overview-grid-window-resizer-right";
    this._rightResizeElement.style.right = 0;
    parentElement.appendChild(this._rightResizeElement);
    WebInspector.installDragHandle(this._rightResizeElement, this._resizerElementStartDragging.bind(this), this._rightResizeElementDragging.bind(this), null, "ew-resize");
}

WebInspector.OverviewGrid.Events = {
    WindowChanged: "WindowChanged"
}

WebInspector.OverviewGrid.Window.prototype = {
    reset: function()
    {
        this.windowLeft = 0.0;
        this.windowRight = 1.0;

        this._overviewWindowElement.style.left = "0%";
        this._overviewWindowElement.style.width = "100%";
        this._overviewWindowBordersElement.style.left = "0%";
        this._overviewWindowBordersElement.style.right = "0%";
        this._leftResizeElement.style.left = "0%";
        this._rightResizeElement.style.left = "100%";
    },

    /**
     * @param {Event} event
     */
    _resizerElementStartDragging: function(event)
    {
        this._resizerParentOffsetLeft = event.pageX - event.offsetX - event.target.offsetLeft;
        event.preventDefault();
        return true;
    },

    /**
     * @param {Event} event
     */
    _leftResizeElementDragging: function(event)
    {
        this._resizeWindowLeft(event.pageX - this._resizerParentOffsetLeft);
        event.preventDefault();
    },

    /**
     * @param {Event} event
     */
    _rightResizeElementDragging: function(event)
    {
        this._resizeWindowRight(event.pageX - this._resizerParentOffsetLeft);
        event.preventDefault();
    },

    /**
     * @param {Event} event
     * @return {boolean}
     */
    _startWindowSelectorDragging: function(event)
    {
        this._offsetLeft = event.pageX - event.offsetX;
        var position = event.pageX - this._offsetLeft;
        this._overviewWindowSelector = new WebInspector.OverviewGrid.WindowSelector(this._parentElement, position);
        return true;
    },

    /**
     * @param {Event} event
     */
    _windowSelectorDragging: function(event)
    {
        this._overviewWindowSelector._updatePosition(event.pageX - this._offsetLeft);
        event.preventDefault();
    },

    /**
     * @param {Event} event
     */
    _endWindowSelectorDragging: function(event)
    {
        var window = this._overviewWindowSelector._close(event.pageX - this._offsetLeft);
        delete this._overviewWindowSelector;
        if (window.end === window.start) { // Click, not drag.
            var middle = window.end;
            window.start = Math.max(0, middle - WebInspector.OverviewGrid.MinSelectableSize / 2);
            window.end = Math.min(this._parentElement.clientWidth, middle + WebInspector.OverviewGrid.MinSelectableSize / 2);
        } else if (window.end - window.start < WebInspector.OverviewGrid.MinSelectableSize) {
            if (this._parentElement.clientWidth - window.end > WebInspector.OverviewGrid.MinSelectableSize)
                window.end = window.start + WebInspector.OverviewGrid.MinSelectableSize;
            else
                window.start = window.end - WebInspector.OverviewGrid.MinSelectableSize;
        }
        this._setWindowPosition(window.start, window.end);
    },

    /**
     * @param {Event} event
     * @return {boolean}
     */
    _startWindowDragging: function(event)
    {
        this._dragStartPoint = event.pageX;
        this._dragStartLeft = this.windowLeft;
        this._dragStartRight = this.windowRight;
        return true;
    },

    /**
     * @param {Event} event
     */
    _windowDragging: function(event)
    {
        event.preventDefault();
        var delta = (event.pageX - this._dragStartPoint) / this._parentElement.clientWidth;
        if (this._dragStartLeft + delta < 0)
            delta = -this._dragStartLeft;

        if (this._dragStartRight + delta > 1)
            delta = 1 - this._dragStartRight;

        this._setWindow(this._dragStartLeft + delta, this._dragStartRight + delta);
    },

    /**
     * @param {number} start
     */
    _resizeWindowLeft: function(start)
    {
        // Glue to edge.
        if (start < 10)
            start = 0;
        else if (start > this._rightResizeElement.offsetLeft -  4)
            start = this._rightResizeElement.offsetLeft - 4;
        this._setWindowPosition(start, null);
    },

    /**
     * @param {number} end
     */
    _resizeWindowRight: function(end)
    {
        // Glue to edge.
        if (end > this._parentElement.clientWidth - 10)
            end = this._parentElement.clientWidth;
        else if (end < this._leftResizeElement.offsetLeft + WebInspector.OverviewGrid.MinSelectableSize)
            end = this._leftResizeElement.offsetLeft + WebInspector.OverviewGrid.MinSelectableSize;
        this._setWindowPosition(null, end);
    },

    _resizeWindowMaximum: function()
    {
        this._setWindowPosition(0, this._parentElement.clientWidth);
    },

    /**
     * @param {number} left
     * @param {number} right
     */
    _setWindow: function(left, right)
    {
        var clientWidth = this._parentElement.clientWidth;
        this._setWindowPosition(left * clientWidth, right * clientWidth);
    },

    /**
     * @param {?number} start
     * @param {?number} end
     */
    _setWindowPosition: function(start, end)
    {
        var clientWidth = this._parentElement.clientWidth;
        const rulerAdjustment = 1 / clientWidth;
        if (typeof start === "number") {
            this.windowLeft = start / clientWidth;
            this._leftResizeElement.style.left = this.windowLeft * 100 + "%";
            this._overviewWindowElement.style.left = this.windowLeft * 100 + "%";
            this._overviewWindowBordersElement.style.left = (this.windowLeft - rulerAdjustment) * 100 + "%";
        }
        if (typeof end === "number") {
            this.windowRight = end / clientWidth;
            this._rightResizeElement.style.left = this.windowRight * 100 + "%";
        }
        this._overviewWindowElement.style.width = (this.windowRight - this.windowLeft) * 100 + "%";
        this._overviewWindowBordersElement.style.right = (1 - this.windowRight + 2 * rulerAdjustment) * 100 + "%";
        this.dispatchEventToListeners(WebInspector.OverviewGrid.Events.WindowChanged);
    },

    /**
     * @param {Event} event
     */
    _onMouseWheel: function(event)
    {
        const zoomFactor = 1.1;
        const mouseWheelZoomSpeed = 1 / 120;

        if (typeof event.wheelDeltaY === "number" && event.wheelDeltaY) {
            var referencePoint = event.offsetX;
            this._zoom(Math.pow(zoomFactor, -event.wheelDeltaY * mouseWheelZoomSpeed), referencePoint);
        }
        if (typeof event.wheelDeltaX === "number" && event.wheelDeltaX) {
            var offset = Math.round(event.wheelDeltaX * WebInspector.OverviewGrid.WindowScrollSpeedFactor);
            var windowLeft = this._leftResizeElement.offsetLeft + WebInspector.OverviewGrid.ResizerOffset;
            var windowRight = this._rightResizeElement.offsetLeft + WebInspector.OverviewGrid.ResizerOffset;

            if (windowLeft - offset < 0)
                offset = windowLeft;

            if (windowRight - offset > this._parentElement.clientWidth)
                offset = windowRight - this._parentElement.clientWidth;

            this._setWindowPosition(windowLeft - offset, windowRight - offset);

            event.preventDefault();
        }
    },

    /**
     * @param {number} factor
     * @param {number} referencePoint
     */
    _zoom: function(factor, referencePoint)
    {
        var left = this._leftResizeElement.offsetLeft + WebInspector.OverviewGrid.ResizerOffset;
        var right = this._rightResizeElement.offsetLeft + WebInspector.OverviewGrid.ResizerOffset;

        var delta = factor * (right - left);
        if (factor < 1 && delta < WebInspector.OverviewGrid.MinSelectableSize)
            return;
        var max = this._parentElement.clientWidth;
        if (typeof referencePoint !== "number")
            referencePoint = (right + left) / 2;
        left = Math.max(0, Math.min(max - delta, referencePoint + (left - referencePoint) * factor));
        right = Math.min(max, left + delta);
        this._setWindowPosition(left, right);
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 */
WebInspector.OverviewGrid.WindowSelector = function(parent, position)
{
    this._startPosition = position;
    this._width = parent.offsetWidth;
    this._windowSelector = document.createElement("div");
    this._windowSelector.className = "overview-grid-window-selector";
    this._windowSelector.style.left = this._startPosition + "px";
    this._windowSelector.style.right = this._width - this._startPosition + "px";
    parent.appendChild(this._windowSelector);
}

WebInspector.OverviewGrid.WindowSelector.prototype = {
    _createSelectorElement: function(parent, left, width, height)
    {
        var selectorElement = document.createElement("div");
        selectorElement.className = "overview-grid-window-selector";
        selectorElement.style.left = left + "px";
        selectorElement.style.width = width + "px";
        selectorElement.style.top = "0px";
        selectorElement.style.height = height + "px";
        parent.appendChild(selectorElement);
        return selectorElement;
    },

    _close: function(position)
    {
        position = Math.max(0, Math.min(position, this._width));
        this._windowSelector.parentNode.removeChild(this._windowSelector);
        return this._startPosition < position ? {start: this._startPosition, end: position} : {start: position, end: this._startPosition};
    },

    _updatePosition: function(position)
    {
        position = Math.max(0, Math.min(position, this._width));
        if (position < this._startPosition) {
            this._windowSelector.style.left = position + "px";
            this._windowSelector.style.right = this._width - this._startPosition + "px";
        } else {
            this._windowSelector.style.left = this._startPosition + "px";
            this._windowSelector.style.right = this._width - position + "px";
        }
    }
}
