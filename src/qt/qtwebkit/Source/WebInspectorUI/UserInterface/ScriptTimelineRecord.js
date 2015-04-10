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

WebInspector.ScriptTimelineRecord = function(eventType, startTime, endTime, details, resource, lineNumber, callFrames)
{
    WebInspector.TimelineRecord.call(this, WebInspector.TimelineRecord.Type.Script, startTime, endTime);

    console.assert(eventType);

    if (eventType in WebInspector.ScriptTimelineRecord.EventType)
        eventType = WebInspector.ScriptTimelineRecord.EventType[eventType];

    this._eventType = eventType;
    this._details = details || "";
    this._resource = resource || null;
    this._lineNumber = lineNumber || NaN;
    this._callFrames = callFrames || null;
};

WebInspector.ScriptTimelineRecord.EventType = {
    ScriptEvaluated: "script-timeline-record-script-evaluated",
    EventDispatched: "script-timeline-record-event-dispatch",
    TimerFired: "script-timeline-record-timer-fired",
    TimerInstalled: "script-timeline-record-timer-installed",
    TimerRemoved: "script-timeline-record-timer-removed",
    AnimationFrameFired: "script-timeline-record-animation-frame-fired"
};

WebInspector.ScriptTimelineRecord.EventType.displayName = function(eventType)
{
    switch(eventType) {
    case WebInspector.ScriptTimelineRecord.EventType.ScriptEvaluated:
        return WebInspector.UIString("Script Evaluated");
    case WebInspector.ScriptTimelineRecord.EventType.EventDispatched:
        return WebInspector.UIString("Event Dispatched");
    case WebInspector.ScriptTimelineRecord.EventType.TimerFired:
        return WebInspector.UIString("Timer Fired");
    case WebInspector.ScriptTimelineRecord.EventType.TimerInstalled:
        return WebInspector.UIString("Timer Installed");
    case WebInspector.ScriptTimelineRecord.EventType.TimerRemoved:
        return WebInspector.UIString("Timer Removed");
    case WebInspector.ScriptTimelineRecord.EventType.AnimationFrameFired:
        return WebInspector.UIString("Animation Frame Fired");
    }
};

WebInspector.ScriptTimelineRecord.prototype = {
    constructor: WebInspector.ScriptTimelineRecord,

    // Public

    get eventType()
    {
        return this._eventType;
    },

    get details()
    {
        return this._details;
    },

    get resource()
    {
        return this._resource;
    },

    get lineNumber()
    {
        return this._lineNumber;
    },

    get callFrames()
    {
        return this._callFrames;
    },

    get sourceCodeLocation()
    {
        if ("_sourceCodeLocation" in this)
            return this._sourceCodeLocation;

        if (!this._resource) {
            this._sourceCodeLocation = null;
            return this._sourceCodeLocation;
        }

        // FIXME: Script Timeline Events with a location should always contain a call stack
        // or a complete (url:line:column) triplet.

        if (this._callFrames) {
            for (var i = 0; i < this._callFrames.length; ++i) {
                var callFrame = this._callFrames[i];
                if (callFrame.nativeCode)
                    continue;

                if (!callFrame.sourceCodeLocation)
                    break;

                this._sourceCodeLocation = callFrame.sourceCodeLocation;
                return this._sourceCodeLocation;
            }
        }

        var lineNumber = isNaN(this._lineNumber) ? 0 : this._lineNumber;
        this._sourceCodeLocation = this._resource.createSourceCodeLocation(lineNumber, 0);
        return this._sourceCodeLocation;
    }
};

WebInspector.ScriptTimelineRecord.prototype.__proto__ = WebInspector.TimelineRecord.prototype;
