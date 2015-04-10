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

WebInspector.ResourceTimelineRecord = function(resource)
{
    WebInspector.TimelineRecord.call(this, WebInspector.TimelineRecord.Type.Network);

    this._resource = resource;
    this._resource.addEventListener(WebInspector.Resource.Event.TimestampsDidChange, this._dispatchUpdatedEvent, this);
};

WebInspector.ResourceTimelineRecord.prototype = {
    constructor: WebInspector.ResourceTimelineRecord,

    // Public

    get resource()
    {
        return this._resource;
    },

    get startTime()
    {
        return this._resource.firstTimestamp;
    },

    get endTime()
    {
        return this._resource.lastTimestamp;
    },

    get waitingDuration()
    {
        return this._resource.latency;
    },

    get activeDuration()
    {
        return this._resource.receiveDuration;
    },

    // Private

    _dispatchUpdatedEvent: function()
    {
        this.dispatchEventToListeners(WebInspector.TimelineRecord.Event.Updated);
    }
};

WebInspector.ResourceTimelineRecord.prototype.__proto__ = WebInspector.TimelineRecord.prototype;
