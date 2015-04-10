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

WebInspector.LayoutTimelineRecord = function(eventType, startTime, endTime, callFrames, x, y, width, height, quad)
{
    WebInspector.TimelineRecord.call(this, WebInspector.TimelineRecord.Type.Layout, startTime, endTime);

    console.assert(eventType);

    if (eventType in WebInspector.LayoutTimelineRecord.EventType)
        eventType = WebInspector.LayoutTimelineRecord.EventType[eventType];

    this._eventType = eventType;
    this._callFrames = callFrames || [];
    this._x = typeof x === "number" ? x : NaN;
    this._y = typeof y === "number" ? y : NaN;
    this._width = typeof width === "number" ? width : NaN;
    this._height = typeof height === "number" ? height : NaN;
    this._quad = quad instanceof WebInspector.Quad ? quad : null;
};

WebInspector.LayoutTimelineRecord.EventType = {
    InvalidateStyles: "layout-timeline-record-invalidate-styles",
    RecalculateStyles: "layout-timeline-record-recalculate-styles",
    InvalidateLayout: "layout-timeline-record-invalidate-layout",
    Layout: "layout-timeline-record-layout",
    Paint: "layout-timeline-record-paint"
};

WebInspector.LayoutTimelineRecord.EventType.displayName = function(eventType)
{
    switch(eventType) {
    case WebInspector.LayoutTimelineRecord.EventType.InvalidateStyles:
        return WebInspector.UIString("Invalidate Styles");
    case WebInspector.LayoutTimelineRecord.EventType.RecalculateStyles:
        return WebInspector.UIString("Recalculate Styles");
    case WebInspector.LayoutTimelineRecord.EventType.InvalidateLayout:
        return WebInspector.UIString("Invalidate Layout");
    case WebInspector.LayoutTimelineRecord.EventType.Layout:
        return WebInspector.UIString("Layout");
    case WebInspector.LayoutTimelineRecord.EventType.Paint:
        return WebInspector.UIString("Paint");
    }
};

WebInspector.LayoutTimelineRecord.prototype = {
    constructor: WebInspector.LayoutTimelineRecord,

    // Public

    get eventType()
    {
        return this._eventType;
    },

    get callFrames()
    {
        return this._callFrames;
    },

    get initiatorCallFrame()
    {
        if (!this._callFrames || !this._callFrames.length)
            return null;

        // Return the first non-native code call frame as the initiator.
        for (var i = 0; i < this._callFrames.length; ++i) {
            if (this._callFrames[i].nativeCode)
                continue;
            return this._callFrames[i];
        }

        return null;
    },

    get x()
    {
        return this._x;
    },

    get y()
    {
        return this._y;
    },

    get width()
    {
        return this._width;
    },

    get height()
    {
        return this._height;
    },

    get area()
    {
        return this._width * this._height;
    },

    get rect()
    {
        if (!isNaN(this._x) && !isNaN(this._y))
            return new WebInspector.Rect(this._x, this._y, this._width, this._height);
        return null;
    },

    get quad()
    {
        return this._quad;
    }
};

WebInspector.LayoutTimelineRecord.prototype.__proto__ = WebInspector.TimelineRecord.prototype;
