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

WebInspector.Popover = function(delegate) {
    WebInspector.Object.call(this);

    this.delegate = delegate;
    this._edge = null;
    this._frame = new WebInspector.Rect;
    this._content = null;
    this._targetFrame = new WebInspector.Rect;
    this._preferredEdges = null;

    this._contentNeedsUpdate = false;

    this._element = document.createElement("div");
    this._element.className = WebInspector.Popover.StyleClassName;
    this._canvasId = "popover-" + (WebInspector.Popover.canvasId++);
    this._element.style.backgroundImage = "-webkit-canvas(" + this._canvasId + ")";
    this._element.addEventListener("webkitTransitionEnd", this, true);
    
    this._container = this._element.appendChild(document.createElement("div"));
    this._container.className = "container";
};

WebInspector.Popover.StyleClassName = "popover";
WebInspector.Popover.FadeOutClassName = "fade-out";
WebInspector.Popover.canvasId = 0;
WebInspector.Popover.CornerRadius = 5;
WebInspector.Popover.MinWidth = 40;
WebInspector.Popover.MinHeight = 40;
WebInspector.Popover.ShadowPadding = 5;
WebInspector.Popover.ContentPadding = 5;
WebInspector.Popover.AnchorSize = new WebInspector.Size(22, 11);
WebInspector.Popover.ShadowEdgeInsets = new WebInspector.EdgeInsets(WebInspector.Popover.ShadowPadding);

WebInspector.Popover.prototype = {
    constructor: WebInspector.Popover,

    // Public

    get element()
    {
        return this._element;
    },

    get frame()
    {
        return this._frame;
    },

    get visible()
    {
        return this._element.parentNode === document.body && !this._element.classList.contains(WebInspector.Popover.FadeOutClassName);
    },

    set frame(frame)
    {
        this._element.style.left = frame.origin.x + "px";
        this._element.style.top = frame.origin.y + "px";
        this._element.style.width = frame.size.width + "px";
        this._element.style.height = frame.size.height + "px";
        this._element.style.backgroundSize = frame.size.width + "px " + frame.size.height + "px";
        this._frame = frame;
    },

    set content(content)
    {
        if (content === this._content)
            return;

        this._content = content;

        this._contentNeedsUpdate = true;

        if (this.visible)
            this._update();
    },

    /**
     * @param {WebInspector.Rect} targetFrame
     * @param {WebInspector.RectEdge}[] preferredEdges
     */
    present: function(targetFrame, preferredEdges)
    {
        this._targetFrame = targetFrame;
        this._preferredEdges = preferredEdges;

        if (!this._content)
            return;

        window.addEventListener("mousedown", this, true);
        window.addEventListener("scroll", this, true);

        this._update();
    },
    
    dismiss: function()
    {
        if (this._element.parentNode !== document.body)
            return;

        window.removeEventListener("mousedown", this, true);
        window.removeEventListener("scroll", this, true);

        this._element.classList.add(WebInspector.Popover.FadeOutClassName);

        if (this.delegate && typeof this.delegate.willDismissPopover === "function")
            this.delegate.willDismissPopover(this);
    },

    handleEvent: function(event)
    {
        switch (event.type) {
        case "mousedown":
        case "scroll":
            if (!this._element.contains(event.target))
                this.dismiss();
            break;
        case "webkitTransitionEnd":
            document.body.removeChild(this._element);
            this._element.classList.remove(WebInspector.Popover.FadeOutClassName);
            this._container.textContent = "";
            if (this.delegate && typeof this.delegate.didDismissPopover === "function")
                this.delegate.didDismissPopover(this);
            break;
        }
    },

    // Private

    _update: function(replaceContent)
    {
        var targetFrame = this._targetFrame;
        var preferredEdges = this._preferredEdges;

        // Ensure our element is on display so that its metrics can be resolved
        // or interrupt any pending transition to remove it from display.
        if (this._element.parentNode !== document.body)
            document.body.appendChild(this._element);
        else
            this._element.classList.remove(WebInspector.Popover.FadeOutClassName);

        if (this._contentNeedsUpdate) {
            // Reset CSS properties on element so that the element may be sized to fit its content.
            this._element.style.removeProperty("left");
            this._element.style.removeProperty("top");
            this._element.style.removeProperty("width");
            this._element.style.removeProperty("height");
            if (this._edge !== null)
                this._element.classList.remove(this._cssClassNameForEdge());

            // Add the content in place of the wrapper to get the raw metrics.
            this._element.replaceChild(this._content, this._container);

            // Get the ideal size for the popover to fit its content.
            var popoverBounds = this._element.getBoundingClientRect();
            this._preferredSize = new WebInspector.Size(Math.ceil(popoverBounds.width), Math.ceil(popoverBounds.height));
        }

        // The frame of the window with a little inset to make sure we have room for shadows.
        var containerFrame = new WebInspector.Rect(0, 0, window.innerWidth, window.innerHeight);
        containerFrame = containerFrame.inset(WebInspector.Popover.ShadowEdgeInsets);

        // Work out the metrics for all edges.
        var metrics = new Array(preferredEdges.length);
        for (var edgeName in WebInspector.RectEdge) {
            var edge = WebInspector.RectEdge[edgeName];
            var item = {
                edge: edge,
                metrics: this._bestMetricsForEdge(this._preferredSize, targetFrame, containerFrame, edge)
            };
            var preferredIndex = preferredEdges.indexOf(edge);
            if (preferredIndex !== -1)
                metrics[preferredIndex] = item;
            else
                metrics.push(item);
        }

        function area(size)
        {
            return size.width * size.height;
        }

        // Find if any of those fit better than the frame for the preferred edge.
        var bestEdge = metrics[0].edge;
        var bestMetrics = metrics[0].metrics;
        for (var i = 1; i < metrics.length; i++) {
            var itemMetrics = metrics[i].metrics;
            if (area(itemMetrics.contentSize) > area(bestMetrics.contentSize)) {
                bestEdge = metrics[i].edge;
                bestMetrics = itemMetrics;
            }
        }

        var anchorPoint;
        var bestFrame = bestMetrics.frame;

        var needsToDrawBackground = !this._frame.size.equals(bestFrame.size) || this._edge !== bestEdge;

        this.frame = bestFrame;
        this._edge = bestEdge;

        if (this.frame === WebInspector.Rect.ZERO_RECT) {
            // The target for the popover is offscreen.
            this.dismiss();
        } else {
            switch (bestEdge) {
            case WebInspector.RectEdge.MIN_X: // Displayed on the left of the target, arrow points right.
                anchorPoint = new WebInspector.Point(bestFrame.size.width - WebInspector.Popover.ShadowPadding, targetFrame.midY() - bestFrame.minY());
                break;
            case WebInspector.RectEdge.MAX_X: // Displayed on the right of the target, arrow points left.
                anchorPoint = new WebInspector.Point(WebInspector.Popover.ShadowPadding, targetFrame.midY() - bestFrame.minY());
                break;
            case WebInspector.RectEdge.MIN_Y: // Displayed above the target, arrow points down.
                anchorPoint = new WebInspector.Point(targetFrame.midX() - bestFrame.minX(), bestFrame.size.height - WebInspector.Popover.ShadowPadding);
                break;
            case WebInspector.RectEdge.MAX_Y: // Displayed below the target, arrow points up.
                anchorPoint = new WebInspector.Point(targetFrame.midX() - bestFrame.minX(), WebInspector.Popover.ShadowPadding);
                break;
            }

            this._element.classList.add(this._cssClassNameForEdge());

            if (needsToDrawBackground)
                this._drawBackground(bestEdge, anchorPoint);

            // Make sure content is centered in case either of the dimension is smaller than the minimal bounds.
            if (this._preferredSize.width < WebInspector.Popover.MinWidth || this._preferredSize.height < WebInspector.Popover.MinHeight)
                this._container.classList.add("center");
            else
                this._container.classList.remove("center");
        }

        // Wrap the content in the container so that it's located correctly.
        if (this._contentNeedsUpdate) {
            this._container.textContent = "";
            this._element.replaceChild(this._container, this._content);
            this._container.appendChild(this._content);
        }

        this._contentNeedsUpdate = false;
    },

    _cssClassNameForEdge: function()
    {
        switch (this._edge) {
        case WebInspector.RectEdge.MIN_X: // Displayed on the left of the target, arrow points right.
            return "arrow-right";
        case WebInspector.RectEdge.MAX_X: // Displayed on the right of the target, arrow points left.
            return "arrow-left";
        case WebInspector.RectEdge.MIN_Y: // Displayed above the target, arrow points down.
            return "arrow-down";
        case WebInspector.RectEdge.MAX_Y: // Displayed below the target, arrow points up.
            return "arrow-up";
        }
        console.error("Unknown edge.");
        return "arrow-up";
    },

    _drawBackground: function(edge, anchorPoint)
    {
        var scaleFactor = window.devicePixelRatio;

        var width = this._frame.size.width;
        var height = this._frame.size.height;
        var scaledWidth = width * scaleFactor;
        var scaledHeight = height * scaleFactor;

        // Create a scratch canvas so we can draw the popover that will later be drawn into
        // the final context with a shadow.
        var scratchCanvas = document.createElement("canvas");
        scratchCanvas.width = scaledWidth;
        scratchCanvas.height = scaledHeight;

        var ctx = scratchCanvas.getContext("2d");
        ctx.scale(scaleFactor, scaleFactor);

        // Bounds of the path don't take into account the arrow, but really only the tight bounding box
        // of the content contained within the frame.
        var bounds;
        var arrowHeight = WebInspector.Popover.AnchorSize.height;
        switch (edge) {
        case WebInspector.RectEdge.MIN_X: // Displayed on the left of the target, arrow points right.
            bounds = new WebInspector.Rect(0, 0, width - arrowHeight, height);
            break;
        case WebInspector.RectEdge.MAX_X: // Displayed on the right of the target, arrow points left.
            bounds = new WebInspector.Rect(arrowHeight, 0, width - arrowHeight, height);
            break;
        case WebInspector.RectEdge.MIN_Y: // Displayed above the target, arrow points down.
            bounds = new WebInspector.Rect(0, 0, width, height - arrowHeight);
            break;
        case WebInspector.RectEdge.MAX_Y: // Displayed below the target, arrow points up.
            bounds = new WebInspector.Rect(0, arrowHeight, width, height - arrowHeight);
            break;
        }

        bounds = bounds.inset(WebInspector.Popover.ShadowEdgeInsets);

        // Clip the frame.
        ctx.fillStyle = "black";
        this._drawFrame(ctx, bounds, edge, anchorPoint);
        ctx.clip();

        // Gradient fill, top-to-bottom.
        var fillGradient = ctx.createLinearGradient(0, 0, 0, height);
        fillGradient.addColorStop(0, "rgba(255, 255, 255, 0.95)");
        fillGradient.addColorStop(1, "rgba(235, 235, 235, 0.95)");
        ctx.fillStyle = fillGradient;
        ctx.fillRect(0, 0, width, height);

        // Stroke.
        ctx.strokeStyle = "rgba(0, 0, 0, 0.25)";
        ctx.lineWidth = 2;
        this._drawFrame(ctx, bounds, edge, anchorPoint);
        ctx.stroke();

        // Draw the popover into the final context with a drop shadow.
        var finalContext = document.getCSSCanvasContext("2d", this._canvasId, scaledWidth, scaledHeight);

        finalContext.clearRect(0, 0, scaledWidth, scaledHeight);

        finalContext.shadowOffsetX = 1;
        finalContext.shadowOffsetY = 1;
        finalContext.shadowBlur = 5;
        finalContext.shadowColor = "rgba(0, 0, 0, 0.5)";

        finalContext.drawImage(scratchCanvas, 0, 0, scaledWidth, scaledHeight);
    },
    
    _bestMetricsForEdge: function(preferredSize, targetFrame, containerFrame, edge)
    {
        var x, y;
        var width = preferredSize.width + (WebInspector.Popover.ShadowPadding * 2) + (WebInspector.Popover.ContentPadding * 2);
        var height = preferredSize.height + (WebInspector.Popover.ShadowPadding * 2) + (WebInspector.Popover.ContentPadding * 2);
        var arrowLength = WebInspector.Popover.AnchorSize.height;

        switch (edge) {
        case WebInspector.RectEdge.MIN_X: // Displayed on the left of the target, arrow points right.
            width += arrowLength;
            x = targetFrame.origin.x - width + WebInspector.Popover.ShadowPadding;
            y = targetFrame.origin.y - (height - targetFrame.size.height) / 2;
            break;
        case WebInspector.RectEdge.MAX_X: // Displayed on the right of the target, arrow points left.
            width += arrowLength;
            x = targetFrame.origin.x + targetFrame.size.width - WebInspector.Popover.ShadowPadding;
            y = targetFrame.origin.y - (height - targetFrame.size.height) / 2;
            break;
        case WebInspector.RectEdge.MIN_Y: // Displayed above the target, arrow points down.
            height += arrowLength;
            x = targetFrame.origin.x - (width - targetFrame.size.width) / 2;
            y = targetFrame.origin.y - height + WebInspector.Popover.ShadowPadding;
            break;
        case WebInspector.RectEdge.MAX_Y: // Displayed below the target, arrow points up.
            height += arrowLength;
            x = targetFrame.origin.x - (width - targetFrame.size.width) / 2;
            y = targetFrame.origin.y + targetFrame.size.height - WebInspector.Popover.ShadowPadding;
            break;
        }

        var preferredFrame = new WebInspector.Rect(x, y, width, height);
        var bestFrame = preferredFrame.intersectionWithRect(containerFrame);

        width = bestFrame.size.width - (WebInspector.Popover.ShadowPadding * 2);
        height = bestFrame.size.height - (WebInspector.Popover.ShadowPadding * 2);

        switch (edge) {
        case WebInspector.RectEdge.MIN_X: // Displayed on the left of the target, arrow points right.
        case WebInspector.RectEdge.MAX_X: // Displayed on the right of the target, arrow points left.
            width -= arrowLength;
            break;
        case WebInspector.RectEdge.MIN_Y: // Displayed above the target, arrow points down.
        case WebInspector.RectEdge.MAX_Y: // Displayed below the target, arrow points up.
            height -= arrowLength;
            break;
        }

        return {
            frame: bestFrame,
            contentSize: new WebInspector.Size(width, height)
        };
    },

    _drawFrame: function(ctx, bounds, anchorEdge, anchorPoint)
    {
        var r = WebInspector.Popover.CornerRadius;
        var arrowHalfLength = WebInspector.Popover.AnchorSize.width / 2;

        ctx.beginPath();
        switch (anchorEdge) {
        case WebInspector.RectEdge.MIN_X: // Displayed on the left of the target, arrow points right.
            ctx.moveTo(bounds.maxX(), bounds.minY() + r);
            ctx.lineTo(bounds.maxX(), anchorPoint.y - arrowHalfLength);
            ctx.lineTo(anchorPoint.x, anchorPoint.y);
            ctx.lineTo(bounds.maxX(), anchorPoint.y + arrowHalfLength);
            ctx.arcTo(bounds.maxX(), bounds.maxY(), bounds.minX(), bounds.maxY(), r);
            ctx.arcTo(bounds.minX(), bounds.maxY(), bounds.minX(), bounds.minY(), r);
            ctx.arcTo(bounds.minX(), bounds.minY(), bounds.maxX(), bounds.minY(), r);
            ctx.arcTo(bounds.maxX(), bounds.minY(), bounds.maxX(), bounds.maxY(), r);
            break;
        case WebInspector.RectEdge.MAX_X: // Displayed on the right of the target, arrow points left.
            ctx.moveTo(bounds.minX(), bounds.maxY() - r);
            ctx.lineTo(bounds.minX(), anchorPoint.y + arrowHalfLength);
            ctx.lineTo(anchorPoint.x, anchorPoint.y);
            ctx.lineTo(bounds.minX(), anchorPoint.y - arrowHalfLength);
            ctx.arcTo(bounds.minX(), bounds.minY(), bounds.maxX(), bounds.minY(), r);
            ctx.arcTo(bounds.maxX(), bounds.minY(), bounds.maxX(), bounds.maxY(), r);
            ctx.arcTo(bounds.maxX(), bounds.maxY(), bounds.minX(), bounds.maxY(), r);
            ctx.arcTo(bounds.minX(), bounds.maxY(), bounds.minX(), bounds.minY(), r);
            break;
        case WebInspector.RectEdge.MIN_Y: // Displayed above the target, arrow points down.
            ctx.moveTo(bounds.maxX() - r, bounds.maxY());
            ctx.lineTo(anchorPoint.x + arrowHalfLength, bounds.maxY());
            ctx.lineTo(anchorPoint.x, anchorPoint.y);
            ctx.lineTo(anchorPoint.x - arrowHalfLength, bounds.maxY());
            ctx.arcTo(bounds.minX(), bounds.maxY(), bounds.minX(), bounds.minY(), r);
            ctx.arcTo(bounds.minX(), bounds.minY(), bounds.maxX(), bounds.minY(), r);
            ctx.arcTo(bounds.maxX(), bounds.minY(), bounds.maxX(), bounds.maxY(), r);
            ctx.arcTo(bounds.maxX(), bounds.maxY(), bounds.minX(), bounds.maxY(), r);
            break;
        case WebInspector.RectEdge.MAX_Y: // Displayed below the target, arrow points up.
            ctx.moveTo(bounds.minX() + r, bounds.minY());
            ctx.lineTo(anchorPoint.x - arrowHalfLength, bounds.minY());
            ctx.lineTo(anchorPoint.x, anchorPoint.y);
            ctx.lineTo(anchorPoint.x + arrowHalfLength, bounds.minY());
            ctx.arcTo(bounds.maxX(), bounds.minY(), bounds.maxX(), bounds.maxY(), r);
            ctx.arcTo(bounds.maxX(), bounds.maxY(), bounds.minX(), bounds.maxY(), r);
            ctx.arcTo(bounds.minX(), bounds.maxY(), bounds.minX(), bounds.minY(), r);
            ctx.arcTo(bounds.minX(), bounds.minY(), bounds.maxX(), bounds.minY(), r);
            break;
        }
        ctx.closePath();
    }
    
};

WebInspector.Popover.prototype.__proto__ = WebInspector.Object.prototype;
