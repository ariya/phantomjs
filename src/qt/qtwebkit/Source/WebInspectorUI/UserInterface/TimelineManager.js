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

WebInspector.TimelineManager = function()
{
    WebInspector.Object.call(this);

    WebInspector.Frame.addEventListener(WebInspector.Frame.Event.ProvisionalLoadStarted, this._startAutoRecording, this);
    WebInspector.Frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainResourceDidChange, this);
    WebInspector.Frame.addEventListener(WebInspector.Frame.Event.ResourceWasAdded, this._resourceWasAdded, this);

    this._recording = false;
    this._records = [];
    this._typeRecords = {};
    this._loadEventTime = NaN;
    this._eventMarkers = [];
};

WebInspector.TimelineManager.Event = {
    RecordsCleared: "timeline-manager-records-cleared",
    RecordingStarted: "timeline-manager-recording-started",
    RecordingStopped: "timeline-manager-recording-stopped",
    RecordAdded: "timeline-manager-record-added",
    RecordedEventMarker: "timeline-manager-recorded-event-marker",
};

WebInspector.TimelineManager.MaximumAutoRecordDuration = 90000; // 90 seconds
WebInspector.TimelineManager.MaximumAutoRecordDurationAfterLoadEvent = 10000; // 10 seconds
WebInspector.TimelineManager.DeadTimeRequiredToStopAutoRecordingEarly = 2000; // 2 seconds

WebInspector.TimelineManager.prototype = {
    constructor: WebInspector.TimelineManager,

    // Public

    get recording()
    {
        return this._recording;
    },

    get mainFrameLoadEventTime()
    {
        return this._loadEventTime;
    },

    get timelineEventMarkers()
    {
        return this._eventMarkers;
    },

    get records()
    {
        return this._records;
    },

    recordsWithType: function(type)
    {
        return this._typeRecords[type] || [];
    },

    startRecording: function()
    {
        if (this._recording)
            return;

        this._clear();

        this._recording = true;

        TimelineAgent.start();

        this.dispatchEventToListeners(WebInspector.TimelineManager.Event.RecordingStarted);
    },

    stopRecording: function()
    {
        if (!this._recording)
            return;

        if (this._stopRecordingTimeout) {
            clearTimeout(this._stopRecordingTimeout);
            delete this._stopRecordingTimeout;
        }

        if (this._deadTimeTimeout) {
            clearTimeout(this._deadTimeTimeout);
            delete this._deadTimeTimeout;
        }

        TimelineAgent.stop();

        this._recording = false;
        this._autoRecordingMainResource = null;

        this.dispatchEventToListeners(WebInspector.TimelineManager.Event.RecordingStopped);
    },

    eventRecorded: function(originalRecord)
    {
        // Called from WebInspector.TimelineObserver.

        if (!this._recording)
            return;

        function processRecord(record, parentRecord)
        {
            // Convert the timestamps to seconds to match the resource timestamps.
            var startTime = record.startTime / 1000;
            var endTime = record.endTime / 1000;

            var callFrames = this._callFramesFromPayload(record.stackTrace);

            switch (record.type) {
            case "MarkLoad":
                var mainFrame = WebInspector.frameResourceManager.mainFrame;
                console.assert(mainFrame);

                if (!mainFrame || record.frameId !== mainFrame.id)
                    break;

                this._loadEventTime = startTime;

                var eventMarker = new WebInspector.TimelineEventMarker(startTime, WebInspector.TimelineEventMarker.Type.LoadEvent);
                this._eventMarkers.push(eventMarker);
                this.dispatchEventToListeners(WebInspector.TimelineManager.Event.RecordedEventMarker, {eventMarker: eventMarker});
                this._stopAutoRecordingSoon();
                break;
            case "MarkDOMContent":
                var mainFrame = WebInspector.frameResourceManager.mainFrame;
                console.assert(mainFrame);

                if (!mainFrame || record.frameId !== mainFrame.id)
                    break;

                var eventMarker = new WebInspector.TimelineEventMarker(startTime, WebInspector.TimelineEventMarker.Type.DOMContentEvent);
                this._eventMarkers.push(eventMarker);
                this.dispatchEventToListeners(WebInspector.TimelineManager.Event.RecordedEventMarker, {eventMarker: eventMarker});
                break;
            case "ScheduleStyleRecalculation":
                console.assert(isNaN(endTime));
                // Pass the startTime as the endTime since this record type has no duration.
                this._addRecord(new WebInspector.LayoutTimelineRecord(WebInspector.LayoutTimelineRecord.EventType.InvalidateStyles, startTime, startTime, callFrames));
                break;
            case "RecalculateStyles":
                this._addRecord(new WebInspector.LayoutTimelineRecord(WebInspector.LayoutTimelineRecord.EventType.RecalculateStyles, startTime, endTime, callFrames));
                break;
            case "InvalidateLayout":
                console.assert(isNaN(endTime));
                // Pass the startTime as the endTime since this record type has no duration.
                this._addRecord(new WebInspector.LayoutTimelineRecord(WebInspector.LayoutTimelineRecord.EventType.InvalidateLayout, startTime, startTime, callFrames));
                break;
            case "Layout":
                // COMPATIBILITY (iOS 6): Layout records did not contain area properties. This is not exposed via a quad "root".
                var quad = record.data.root ? new WebInspector.Quad(record.data.root) : null;
                if (quad)
                    this._addRecord(new WebInspector.LayoutTimelineRecord(WebInspector.LayoutTimelineRecord.EventType.Layout, startTime, endTime, callFrames, quad.points[0].x, quad.points[0].y, quad.width, quad.height, quad));
                else
                    this._addRecord(new WebInspector.LayoutTimelineRecord(WebInspector.LayoutTimelineRecord.EventType.Layout, startTime, endTime, callFrames));
                break;
            case "Paint":
                // COMPATIBILITY (iOS 6): Paint records data contained x, y, width, height properties. This became a quad "clip".
                var quad = record.data.clip ? new WebInspector.Quad(record.data.clip) : null;
                if (quad)
                    this._addRecord(new WebInspector.LayoutTimelineRecord(WebInspector.LayoutTimelineRecord.EventType.Paint, startTime, endTime, callFrames, null, null, quad.width, quad.height, quad));
                else
                    this._addRecord(new WebInspector.LayoutTimelineRecord(WebInspector.LayoutTimelineRecord.EventType.Paint, startTime, endTime, callFrames, record.data.x, record.data.y, record.data.width, record.data.height));
                break;
            case "EvaluateScript":
                var resource = WebInspector.frameResourceManager.resourceForURL(record.data.url);

                // The lineNumber is 1-based, but we expect 0-based.
                var lineNumber = record.data.lineNumber - 1;

                switch (parent ? parent.type : null) {
                case "TimerFire":
                    this._addRecord(new WebInspector.ScriptTimelineRecord(WebInspector.ScriptTimelineRecord.EventType.TimerFired, startTime, endTime, parent.data.timerId, resource, lineNumber, callFrames));
                    break;
                default:
                    this._addRecord(new WebInspector.ScriptTimelineRecord(WebInspector.ScriptTimelineRecord.EventType.ScriptEvaluated, startTime, endTime, null, resource, lineNumber, callFrames));
                    break;
                }

                break;
            case "TimeStamp":
                var eventMarker = new WebInspector.TimelineEventMarker(startTime, WebInspector.TimelineEventMarker.Type.TimeStamp);
                this._eventMarkers.push(eventMarker);
                this.dispatchEventToListeners(WebInspector.TimelineManager.Event.RecordedEventMarker, {eventMarker: eventMarker});
                break;
            case "FunctionCall":
                // FunctionCall always happens as a child of another record, and since the FunctionCall record
                // has useful info we just make the timeline record here (combining the data from both records).
                if (!parent)
                    break;

                var resource = WebInspector.frameResourceManager.resourceForURL(record.data.scriptName);

                // The scriptLine is 1-based, but we expect 0-based.
                var lineNumber = record.data.scriptLine - 1;

                switch (parent.type) {
                case "TimerFire":
                    this._addRecord(new WebInspector.ScriptTimelineRecord(WebInspector.ScriptTimelineRecord.EventType.TimerFired, startTime, endTime, parent.data.timerId, resource, lineNumber, callFrames));
                    break;
                case "EventDispatch":
                    this._addRecord(new WebInspector.ScriptTimelineRecord(WebInspector.ScriptTimelineRecord.EventType.EventDispatched, startTime, endTime, parent.data.type, resource, lineNumber, callFrames));
                    break;
                case "XHRLoad":
                    this._addRecord(new WebInspector.ScriptTimelineRecord(WebInspector.ScriptTimelineRecord.EventType.EventDispatched, startTime, endTime, "load", resource, lineNumber, callFrames));
                    break;
                case "XHRReadyStateChange":
                    this._addRecord(new WebInspector.ScriptTimelineRecord(WebInspector.ScriptTimelineRecord.EventType.EventDispatched, startTime, endTime, "readystatechange", resource, lineNumber, callFrames));
                    break;
                case "FireAnimationFrame":
                    this._addRecord(new WebInspector.ScriptTimelineRecord(WebInspector.ScriptTimelineRecord.EventType.AnimationFrameFired, startTime, endTime, parent.data.id, resource, lineNumber, callFrames));
                    break;
                }

                break;
            case "TimerInstall":
            case "TimerRemove":
                // COMPATIBILITY (iOS 6): TimerInstall and TimerRemove did not have a stack trace.
                var callFrame = null;
                if (callFrames) {
                    for (var i = 0; i < callFrames.length; ++i) {
                        if (callFrames[i].nativeCode)
                            continue;
                        callFrame = callFrames[i];
                        break;
                    }
                }

                var sourceCodeLocation = callFrame && callFrame.sourceCodeLocation;
                var resource = sourceCodeLocation ? sourceCodeLocation.sourceCode : null;
                var lineNumber = sourceCodeLocation ? sourceCodeLocation.lineNumber : null;

                if (record.type === "TimerInstall")
                    this._addRecord(new WebInspector.ScriptTimelineRecord(WebInspector.ScriptTimelineRecord.EventType.TimerInstalled, startTime, endTime, record.data.timerId, resource, lineNumber, callFrames));
                else
                    this._addRecord(new WebInspector.ScriptTimelineRecord(WebInspector.ScriptTimelineRecord.EventType.TimerRemoved, startTime, endTime, record.data.timerId, resource, lineNumber, callFrames));

                break;
            }

            return null;
        }

        // Iterate over the records tree using a stack. Doing this recursively has
        // been known to cause a call stack overflow. https://webkit.org/b/79106
        var stack = [{array: [originalRecord], parent: null, index: 0}];
        while (stack.length) {
            var entry = stack.lastValue;
            var records = entry.array;
            var parent = entry.parent;

            if (entry.index < records.length) {
                var record = records[entry.index];

                processRecord.call(this, record, parent);

                if (record.children)
                    stack.push({array: record.children, parent: record, index: 0});
                ++entry.index;
            } else
                stack.pop();
        }
    },

    pageDidLoad: function(timestamp)
    {
        if (isNaN(this._loadEventTime))
            this._loadEventTime = timestamp;
    },

    // Private

    _clear: function()
    {
        this._records = [];
        this._typeRecords = {};
        this._loadEventTime = NaN;
        this._eventMarkers = [];

        this.dispatchEventToListeners(WebInspector.TimelineManager.Event.RecordsCleared);
    },

    _callFramesFromPayload: function(payload)
    {
        if (!payload)
            return null;

        function createCallFrame(payload)
        {
            var url = payload.url;
            var nativeCode = false;

            if (url === "[native code]") {
                nativeCode = true;
                url = null;
            }

            var sourceCode = WebInspector.frameResourceManager.resourceForURL(url);
            if (!sourceCode)
                sourceCode = WebInspector.debuggerManager.scriptsForURL(url)[0];

            // The lineNumber is 1-based, but we expect 0-based.
            var lineNumber = payload.lineNumber - 1;

            var sourceCodeLocation = sourceCode ? sourceCode.createSourceCodeLocation(lineNumber, payload.columnNumber) : null;
            var functionName = payload.functionName !== "global code" ? payload.functionName : null;

            return new WebInspector.CallFrame(null, sourceCodeLocation, functionName, null, null, nativeCode);
        }

        return payload.map(createCallFrame);
    },

    _addRecord: function(record)
    {
        this._records.push(record);

        if (!this._typeRecords[record.type])
            this._typeRecords[record.type] = [];
        this._typeRecords[record.type].push(record);

        this.dispatchEventToListeners(WebInspector.TimelineManager.Event.RecordAdded, {record: record});

        // Only worry about dead time after the load event.
        if (!isNaN(this._loadEventTime))
            this._resetAutoRecordingDeadTimeTimeout();
    },

    _startAutoRecording: function(event)
    {
        if (!event.target.isMainFrame() || (this._recording && !this._autoRecordingMainResource))
            return false;

        var mainResource = event.target.provisionalMainResource || event.target.mainResource;
        if (mainResource === this._autoRecordingMainResource)
            return false;

        this.stopRecording();

        this._autoRecordingMainResource = mainResource;

        this.startRecording();

        this._addRecord(new WebInspector.ResourceTimelineRecord(mainResource));

        if (this._stopRecordingTimeout)
            clearTimeout(this._stopRecordingTimeout);
        this._stopRecordingTimeout = setTimeout(this.stopRecording.bind(this), WebInspector.TimelineManager.MaximumAutoRecordDuration);

        return true;
    },

    _stopAutoRecordingSoon: function()
    {
        // Only auto stop when auto recording.
        if (!this._recording || !this._autoRecordingMainResource)
            return;

        if (this._stopRecordingTimeout)
            clearTimeout(this._stopRecordingTimeout);
        this._stopRecordingTimeout = setTimeout(this.stopRecording.bind(this), WebInspector.TimelineManager.MaximumAutoRecordDurationAfterLoadEvent);
    },

    _resetAutoRecordingDeadTimeTimeout: function()
    {
        // Only monitor dead time when auto recording.
        if (!this._recording || !this._autoRecordingMainResource)
            return;

        if (this._deadTimeTimeout)
            clearTimeout(this._deadTimeTimeout);
        this._deadTimeTimeout = setTimeout(this.stopRecording.bind(this), WebInspector.TimelineManager.DeadTimeRequiredToStopAutoRecordingEarly);
    },

    _mainResourceDidChange: function(event)
    {
        // Ignore resource events when there isn't a main frame yet. Those events are triggered by
        // loading the cached resources when the inspector opens, and they do not have timing information.
        if (!WebInspector.frameResourceManager.mainFrame)
            return;

        if (this._startAutoRecording(event))
            return;

        if (!this._recording)
            return;

        var mainResource = event.target.mainResource;
        if (mainResource === this._autoRecordingMainResource)
            return;

        this._addRecord(new WebInspector.ResourceTimelineRecord(mainResource));
    },

    _resourceWasAdded: function(event)
    {
        // Ignore resource events when there isn't a main frame yet. Those events are triggered by
        // loading the cached resources when the inspector opens, and they do not have timing information.
        if (!WebInspector.frameResourceManager.mainFrame)
            return;

        if (!this._recording)
            return;

        this._addRecord(new WebInspector.ResourceTimelineRecord(event.data.resource));
    }
};

WebInspector.TimelineManager.prototype.__proto__ = WebInspector.Object.prototype;
