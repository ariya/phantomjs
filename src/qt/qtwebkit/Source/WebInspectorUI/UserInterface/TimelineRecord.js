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

WebInspector.TimelineRecord = function(type, startTime, endTime)
{
    WebInspector.Object.call(this);

    console.assert(type);

    if (type in WebInspector.TimelineRecord.Type)
        type = WebInspector.TimelineRecord.Type[type];

    this._type = type;
    this._startTime = startTime || NaN;
    this._endTime = endTime || NaN;
};

WebInspector.TimelineRecord.Event = {
    Updated: "timeline-record-updated"
};

WebInspector.TimelineRecord.Type = {
    Network: "timeline-record-type-network",
    Layout: "timeline-record-type-layout",
    Script: "timeline-record-type-script"
};

WebInspector.TimelineRecord.prototype = {
    constructor: WebInspector.TimelineRecord,

    // Public

    get type()
    {
        return this._type;
    },

    get startTime()
    {
        return this._startTime;
    },

    get endTime()
    {
        return this._endTime;
    },

    get duration()
    {
        // Use the getters instead of the properties so this works for subclasses that override the getters.
        return this.endTime - this.startTime;
    },

    get waitingDuration()
    {
        // Implemented by subclasses if needed.
        return NaN;
    },

    get activeDuration()
    {
        // Implemented by subclasses if needed.
        return this.duration;
    }
};

WebInspector.TimelineRecord.prototype.__proto__ = WebInspector.Object.prototype;
