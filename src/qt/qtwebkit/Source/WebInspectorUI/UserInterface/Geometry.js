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

WebInspector.Point = function(x, y)
{
    this.x = x || 0;
    this.y = y || 0;
};

WebInspector.Point.fromEvent = function(event)
{
    return new WebInspector.Point(event.pageX, event.pageY);
};

WebInspector.Point.fromEventInElement = function(event, element)
{
    var wkPoint = window.webkitConvertPointFromPageToNode(element, new WebKitPoint(event.pageX, event.pageY));
    return new WebInspector.Point(wkPoint.x, wkPoint.y);
};

WebInspector.Point.prototype = {
    constructor: WebInspector.Point,

    toString : function()
    {
        return "WebInspector.Point[" + this.x + "," + this.y + "]";
    },

    copy: function()
    {
        return new WebInspector.Point(this.x, this.y);
    },

    equals: function(anotherPoint)
    {
        return (this.x === anotherPoint.x && this.y === anotherPoint.y);
    }
};

WebInspector.Size = function(width, height)
{
    this.width = width || 0;
    this.height = height || 0;
};

WebInspector.Size.prototype = {
    constructor: WebInspector.Size,

    toString: function()
    {
        return "WebInspector.Size[" + this.width + "," + this.height + "]";
    },

    copy: function()
    {
        return new WebInspector.Size(this.width, this.height);
    },

    equals: function(anotherSize)
    {
        return (this.width === anotherSize.width && this.height === anotherSize.height);
    }
};

WebInspector.Size.ZERO_SIZE = new WebInspector.Size(0, 0);


WebInspector.Rect = function(x, y, width, height)
{
    this.origin = new WebInspector.Point(x || 0, y || 0);
    this.size = new WebInspector.Size(width || 0, height || 0);
};

WebInspector.Rect.rectFromClientRect = function(clientRect)
{
    return new WebInspector.Rect(clientRect.left, clientRect.top, clientRect.width, clientRect.height);
};

WebInspector.Rect.prototype = {
    constructor: WebInspector.Rect,

    toString: function()
    {
        return "WebInspector.Rect[" + [this.origin.x, this.origin.y, this.size.width, this.size.height].join(", ") + "]";
    },

    copy: function()
    {
        return new WebInspector.Rect(this.origin.x, this.origin.y, this.size.width, this.size.height);
    },

    equals: function(anotherRect)
    {
        return (this.origin.equals(anotherRect.origin) && this.size.equals(anotherRect.size));
    },

    inset: function(insets)
    {
        return new WebInspector.Rect(
            this.origin.x + insets.left,
            this.origin.y + insets.top,
            this.size.width - insets.left - insets.right,
            this.size.height - insets.top - insets.bottom
        );
    },

    minX: function()
    {
        return this.origin.x;
    },

    minY: function()
    {
        return this.origin.y;
    },

    midX: function()
    {
        return this.origin.x + (this.size.width / 2);
    },

    midY: function()
    {
        return this.origin.y + (this.size.height / 2);
    },

    maxX: function()
    {
        return this.origin.x + this.size.width;
    },

    maxY: function()
    {
        return this.origin.y + this.size.height;
    },

    intersectionWithRect: function(rect)
    {
        var x1 = Math.max(this.minX(), rect.minX());
        var x2 = Math.min(this.maxX(), rect.maxX());
        if (x1 > x2)
            return WebInspector.Rect.ZERO_RECT;
        var intersection = new WebInspector.Rect;
        intersection.origin.x = x1;
        intersection.size.width = x2 - x1;
        var y1 = Math.max(this.minY(), rect.minY());
        var y2 = Math.min(this.maxY(), rect.maxY());
        if (y1 > y2)
            return WebInspector.Rect.ZERO_RECT;
        intersection.origin.y = y1;
        intersection.size.height = y2 - y1;
        return intersection;
    }
};

WebInspector.Rect.ZERO_RECT = new WebInspector.Rect(0, 0, 0, 0);


WebInspector.EdgeInsets = function(top, right, bottom, left)
{
    console.assert(arguments.length === 1 || arguments.length === 4);

    if (arguments.length === 1) {
        this.top = top;
        this.right = top;
        this.bottom = top;
        this.left = top;
    } else if (arguments.length === 4) {
        this.top = top;
        this.right = right;
        this.bottom = bottom;
        this.left = left;
    }
};

WebInspector.EdgeInsets.prototype = {
    constructor: WebInspector.EdgeInsets,

    equals: function(anotherInset)
    {
        return (this.top === anotherInset.top && this.right === anotherInset.right &&
                this.bottom === anotherInset.bottom && this.left === anotherInset.left);
    },

    copy: function()
    {
        return new WebInspector.EdgeInsets(this.top, this.right, this.bottom, this.left);
    }
};

WebInspector.RectEdge = {
    MIN_X : 0,
    MIN_Y : 1,
    MAX_X : 2,
    MAX_Y : 3
};

WebInspector.Quad = function(quad)
{
    this.points = [
        new WebInspector.Point(quad[0], quad[1]), // top left
        new WebInspector.Point(quad[2], quad[3]), // top right
        new WebInspector.Point(quad[4], quad[5]), // bottom right
        new WebInspector.Point(quad[6], quad[7])  // bottom left
    ];

    this.width = Math.round(Math.sqrt(Math.pow(quad[0] - quad[2], 2) + Math.pow(quad[1] - quad[3], 2)));
    this.height = Math.round(Math.sqrt(Math.pow(quad[0] - quad[6], 2) + Math.pow(quad[1] - quad[7], 2)));
};

WebInspector.Quad.prototype = {
    constructor: WebInspector.Quad,

    toProtocol: function()
    {
        return [
            this.points[0].x, this.points[0].y,
            this.points[1].x, this.points[1].y,
            this.points[2].x, this.points[2].y,
            this.points[3].x, this.points[3].y
        ];
    }
};
