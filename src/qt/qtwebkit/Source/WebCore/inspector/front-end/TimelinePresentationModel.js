/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2012 Intel Inc. All rights reserved.
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
 * @extends {WebInspector.Object}
 */
WebInspector.TimelinePresentationModel = function()
{
    this._linkifier = new WebInspector.Linkifier();
    this._glueRecords = false;
    this._filters = [];
    this.reset();
}

WebInspector.TimelinePresentationModel.categories = function()
{
    if (WebInspector.TimelinePresentationModel._categories)
        return WebInspector.TimelinePresentationModel._categories;
    WebInspector.TimelinePresentationModel._categories = {
        program: new WebInspector.TimelineCategory("program", WebInspector.UIString("Program"), -1, "#BBBBBB", "#DDDDDD", "#FFFFFF"),
        loading: new WebInspector.TimelineCategory("loading", WebInspector.UIString("Loading"), 0, "#5A8BCC", "#8EB6E9", "#70A2E3"),
        scripting: new WebInspector.TimelineCategory("scripting", WebInspector.UIString("Scripting"), 1, "#D8AA34", "#F3D07A", "#F1C453"),
        rendering: new WebInspector.TimelineCategory("rendering", WebInspector.UIString("Rendering"), 2, "#8266CC", "#AF9AEB", "#9A7EE6"),
        painting: new WebInspector.TimelineCategory("painting", WebInspector.UIString("Painting"), 2, "#5FA050", "#8DC286", "#71B363")
    };
    return WebInspector.TimelinePresentationModel._categories;
};

/**
 * @return {!Object.<string, {title: string, category}>}
 */
WebInspector.TimelinePresentationModel._initRecordStyles = function()
{
    if (WebInspector.TimelinePresentationModel._recordStylesMap)
        return WebInspector.TimelinePresentationModel._recordStylesMap;

    var recordTypes = WebInspector.TimelineModel.RecordType;
    var categories = WebInspector.TimelinePresentationModel.categories();

    var recordStyles = {};
    recordStyles[recordTypes.Root] = { title: "#root", category: categories["loading"] };
    recordStyles[recordTypes.Program] = { title: WebInspector.UIString("Program"), category: categories["program"] };
    recordStyles[recordTypes.EventDispatch] = { title: WebInspector.UIString("Event"), category: categories["scripting"] };
    recordStyles[recordTypes.BeginFrame] = { title: WebInspector.UIString("Frame Start"), category: categories["rendering"] };
    recordStyles[recordTypes.ScheduleStyleRecalculation] = { title: WebInspector.UIString("Schedule Style Recalculation"), category: categories["rendering"] };
    recordStyles[recordTypes.RecalculateStyles] = { title: WebInspector.UIString("Recalculate Style"), category: categories["rendering"] };
    recordStyles[recordTypes.InvalidateLayout] = { title: WebInspector.UIString("Invalidate Layout"), category: categories["rendering"] };
    recordStyles[recordTypes.Layout] = { title: WebInspector.UIString("Layout"), category: categories["rendering"] };
    recordStyles[recordTypes.Paint] = { title: WebInspector.UIString("Paint"), category: categories["painting"] };
    recordStyles[recordTypes.Rasterize] = { title: WebInspector.UIString("Rasterize"), category: categories["painting"] };
    recordStyles[recordTypes.ScrollLayer] = { title: WebInspector.UIString("Scroll"), category: categories["rendering"] };
    recordStyles[recordTypes.DecodeImage] = { title: WebInspector.UIString("Image Decode"), category: categories["painting"] };
    recordStyles[recordTypes.ResizeImage] = { title: WebInspector.UIString("Image Resize"), category: categories["painting"] };
    recordStyles[recordTypes.CompositeLayers] = { title: WebInspector.UIString("Composite Layers"), category: categories["painting"] };
    recordStyles[recordTypes.ParseHTML] = { title: WebInspector.UIString("Parse HTML"), category: categories["loading"] };
    recordStyles[recordTypes.TimerInstall] = { title: WebInspector.UIString("Install Timer"), category: categories["scripting"] };
    recordStyles[recordTypes.TimerRemove] = { title: WebInspector.UIString("Remove Timer"), category: categories["scripting"] };
    recordStyles[recordTypes.TimerFire] = { title: WebInspector.UIString("Timer Fired"), category: categories["scripting"] };
    recordStyles[recordTypes.XHRReadyStateChange] = { title: WebInspector.UIString("XHR Ready State Change"), category: categories["scripting"] };
    recordStyles[recordTypes.XHRLoad] = { title: WebInspector.UIString("XHR Load"), category: categories["scripting"] };
    recordStyles[recordTypes.EvaluateScript] = { title: WebInspector.UIString("Evaluate Script"), category: categories["scripting"] };
    recordStyles[recordTypes.ResourceSendRequest] = { title: WebInspector.UIString("Send Request"), category: categories["loading"] };
    recordStyles[recordTypes.ResourceReceiveResponse] = { title: WebInspector.UIString("Receive Response"), category: categories["loading"] };
    recordStyles[recordTypes.ResourceFinish] = { title: WebInspector.UIString("Finish Loading"), category: categories["loading"] };
    recordStyles[recordTypes.FunctionCall] = { title: WebInspector.UIString("Function Call"), category: categories["scripting"] };
    recordStyles[recordTypes.ResourceReceivedData] = { title: WebInspector.UIString("Receive Data"), category: categories["loading"] };
    recordStyles[recordTypes.GCEvent] = { title: WebInspector.UIString("GC Event"), category: categories["scripting"] };
    recordStyles[recordTypes.MarkDOMContent] = { title: WebInspector.UIString("DOMContentLoaded event"), category: categories["scripting"] };
    recordStyles[recordTypes.MarkLoad] = { title: WebInspector.UIString("Load event"), category: categories["scripting"] };
    recordStyles[recordTypes.TimeStamp] = { title: WebInspector.UIString("Stamp"), category: categories["scripting"] };
    recordStyles[recordTypes.Time] = { title: WebInspector.UIString("Time"), category: categories["scripting"] };
    recordStyles[recordTypes.TimeEnd] = { title: WebInspector.UIString("Time End"), category: categories["scripting"] };
    recordStyles[recordTypes.ScheduleResourceRequest] = { title: WebInspector.UIString("Schedule Request"), category: categories["loading"] };
    recordStyles[recordTypes.RequestAnimationFrame] = { title: WebInspector.UIString("Request Animation Frame"), category: categories["scripting"] };
    recordStyles[recordTypes.CancelAnimationFrame] = { title: WebInspector.UIString("Cancel Animation Frame"), category: categories["scripting"] };
    recordStyles[recordTypes.FireAnimationFrame] = { title: WebInspector.UIString("Animation Frame Fired"), category: categories["scripting"] };
    recordStyles[recordTypes.WebSocketCreate] = { title: WebInspector.UIString("Create WebSocket"), category: categories["scripting"] };
    recordStyles[recordTypes.WebSocketSendHandshakeRequest] = { title: WebInspector.UIString("Send WebSocket Handshake"), category: categories["scripting"] };
    recordStyles[recordTypes.WebSocketReceiveHandshakeResponse] = { title: WebInspector.UIString("Receive WebSocket Handshake"), category: categories["scripting"] };
    recordStyles[recordTypes.WebSocketDestroy] = { title: WebInspector.UIString("Destroy WebSocket"), category: categories["scripting"] };

    WebInspector.TimelinePresentationModel._recordStylesMap = recordStyles;
    return recordStyles;
}

/**
 * @param {Object} record
 */
WebInspector.TimelinePresentationModel.recordStyle = function(record)
{
    var recordStyles = WebInspector.TimelinePresentationModel._initRecordStyles();
    var result = recordStyles[record.type];
    if (!result) {
        result = {
            title: WebInspector.UIString("Unknown: %s", record.type),
            category: WebInspector.TimelinePresentationModel.categories()["program"]
        };
        recordStyles[record.type] = result;
    }
    return result;
}

WebInspector.TimelinePresentationModel.categoryForRecord = function(record)
{
    return WebInspector.TimelinePresentationModel.recordStyle(record).category;
}

WebInspector.TimelinePresentationModel.isEventDivider = function(record)
{
    var recordTypes = WebInspector.TimelineModel.RecordType;
    if (record.type === recordTypes.TimeStamp)
        return true;
    if (record.type === recordTypes.MarkDOMContent || record.type === recordTypes.MarkLoad) {
        if (record.data && ((typeof record.data.isMainFrame) === "boolean"))
            return record.data.isMainFrame;
    }
    return false;
}

/**
 * @param {Array} recordsArray
 * @param {?function(*)} preOrderCallback
 * @param {function(*)=} postOrderCallback
 */
WebInspector.TimelinePresentationModel.forAllRecords = function(recordsArray, preOrderCallback, postOrderCallback)
{
    if (!recordsArray)
        return;
    var stack = [{array: recordsArray, index: 0}];
    while (stack.length) {
        var entry = stack[stack.length - 1];
        var records = entry.array;
        if (entry.index < records.length) {
             var record = records[entry.index];
             if (preOrderCallback && preOrderCallback(record))
                 return;
             if (record.children)
                 stack.push({array: record.children, index: 0, record: record});
             else if (postOrderCallback && postOrderCallback(record))
                return;
             ++entry.index;
        } else {
            if (entry.record && postOrderCallback && postOrderCallback(entry.record))
                return;
            stack.pop();
        }
    }
}

/**
 * @param {string=} recordType
 * @return {boolean}
 */
WebInspector.TimelinePresentationModel.needsPreviewElement = function(recordType)
{
    if (!recordType)
        return false;
    const recordTypes = WebInspector.TimelineModel.RecordType;
    switch (recordType) {
    case recordTypes.ScheduleResourceRequest:
    case recordTypes.ResourceSendRequest:
    case recordTypes.ResourceReceiveResponse:
    case recordTypes.ResourceReceivedData:
    case recordTypes.ResourceFinish:
        return true;
    default:
        return false;
    }
}

/**
 * @param {string} recordType
 * @param {string=} title
 */
WebInspector.TimelinePresentationModel.createEventDivider = function(recordType, title)
{
    var eventDivider = document.createElement("div");
    eventDivider.className = "resources-event-divider";
    var recordTypes = WebInspector.TimelineModel.RecordType;

    if (recordType === recordTypes.MarkDOMContent)
        eventDivider.className += " resources-blue-divider";
    else if (recordType === recordTypes.MarkLoad)
        eventDivider.className += " resources-red-divider";
    else if (recordType === recordTypes.TimeStamp)
        eventDivider.className += " resources-orange-divider";
    else if (recordType === recordTypes.BeginFrame)
        eventDivider.className += " timeline-frame-divider";

    if (title)
        eventDivider.title = title;

    return eventDivider;
}

WebInspector.TimelinePresentationModel._hiddenRecords = { }
WebInspector.TimelinePresentationModel._hiddenRecords[WebInspector.TimelineModel.RecordType.MarkDOMContent] = 1;
WebInspector.TimelinePresentationModel._hiddenRecords[WebInspector.TimelineModel.RecordType.MarkLoad] = 1;
WebInspector.TimelinePresentationModel._hiddenRecords[WebInspector.TimelineModel.RecordType.ScheduleStyleRecalculation] = 1;
WebInspector.TimelinePresentationModel._hiddenRecords[WebInspector.TimelineModel.RecordType.InvalidateLayout] = 1;

WebInspector.TimelinePresentationModel.prototype = {
    /**
     * @param {!WebInspector.TimelinePresentationModel.Filter} filter
     */
    addFilter: function(filter)
    {
        this._filters.push(filter);
    },

    /**
     * @param {!WebInspector.TimelinePresentationModel.Filter} filter
     */
    removeFilter: function(filter)
    {
        var index = this._filters.indexOf(filter);
        if (index !== -1)
            this._filters.splice(index, 1);
    },

    rootRecord: function()
    {
        return this._rootRecord;
    },

    frames: function()
    {
        return this._frames;
    },

    reset: function()
    {
        this._linkifier.reset();
        this._rootRecord = new WebInspector.TimelinePresentationModel.Record(this, { type: WebInspector.TimelineModel.RecordType.Root }, null, null, null, false);
        this._sendRequestRecords = {};
        this._scheduledResourceRequests = {};
        this._timerRecords = {};
        this._requestAnimationFrameRecords = {};
        this._eventDividerRecords = [];
        this._timeRecords = {};
        this._timeRecordStack = [];
        this._frames = [];
        this._minimumRecordTime = -1;
        this._layoutInvalidateStack = {};
        this._lastScheduleStyleRecalculation = {};
        this._webSocketCreateRecords = {};
        this._coalescingBuckets = {};
    },

    addFrame: function(frame)
    {
        this._frames.push(frame);
    },

    addRecord: function(record)
    {
        if (this._minimumRecordTime === -1 || record.startTime < this._minimumRecordTime)
            this._minimumRecordTime = WebInspector.TimelineModel.startTimeInSeconds(record);

        var records;
        if (record.type === WebInspector.TimelineModel.RecordType.Program)
            records = record.children;
        else
            records = [record];

        var formattedRecords = [];
        var recordsCount = records.length;
        for (var i = 0; i < recordsCount; ++i)
            formattedRecords.push(this._innerAddRecord(records[i], this._rootRecord));
        return formattedRecords;
    },

    _innerAddRecord: function(record, parentRecord)
    {
        const recordTypes = WebInspector.TimelineModel.RecordType;
        var isHiddenRecord = record.type in WebInspector.TimelinePresentationModel._hiddenRecords;
        var origin;
        var coalescingBucket;

        if (!isHiddenRecord) {
            var newParentRecord = this._findParentRecord(record);
            if (newParentRecord) {
                origin = parentRecord;
                parentRecord = newParentRecord;
            }
            if (parentRecord === this._rootRecord) {
                // On main thread, only coalesce if the last event is of same type.
                coalescingBucket = record.thread ? record.type : "mainThread";
                var coalescedRecord = this._findCoalescedParent(record, coalescingBucket);
                if (coalescedRecord) {
                    if (!origin)
                        origin = parentRecord;
                    parentRecord = coalescedRecord;
                }
            }
        }

        var children = record.children;
        var scriptDetails;
        if (record.data && record.data["scriptName"]) {
            scriptDetails = {
                scriptName: record.data["scriptName"],
                scriptLine: record.data["scriptLine"]
            }
        };

        if ((record.type === recordTypes.TimerFire || record.type === recordTypes.FireAnimationFrame) && children && children.length) {
            var childRecord = children[0];
            if (childRecord.type === recordTypes.FunctionCall) {
                scriptDetails = {
                    scriptName: childRecord.data["scriptName"],
                    scriptLine: childRecord.data["scriptLine"]
                };
                children = childRecord.children.concat(children.slice(1));
            }
        }

        var formattedRecord = new WebInspector.TimelinePresentationModel.Record(this, record, parentRecord, origin, scriptDetails, isHiddenRecord);

        if (WebInspector.TimelinePresentationModel.isEventDivider(formattedRecord))
            this._eventDividerRecords.push(formattedRecord);

        if (isHiddenRecord)
            return formattedRecord;

        formattedRecord.collapsed = parentRecord === this._rootRecord;
        if (coalescingBucket)
            this._coalescingBuckets[coalescingBucket] = formattedRecord;

        var childrenCount = children ? children.length : 0;
        for (var i = 0; i < childrenCount; ++i)
            this._innerAddRecord(children[i], formattedRecord);

        formattedRecord.calculateAggregatedStats();

        if (origin)
            this._updateAncestorStats(formattedRecord);

        if (parentRecord.coalesced && parentRecord.startTime > formattedRecord.startTime)
            parentRecord._record.startTime = record.startTime;

        origin = formattedRecord.origin();
        if (!origin.isRoot() && !origin.coalesced)
            origin.selfTime -= formattedRecord.endTime - formattedRecord.startTime;
        return formattedRecord;
    },

    /**
     * @param {WebInspector.TimelinePresentationModel.Record} record
     */
    _updateAncestorStats: function(record)
    {
        var lastChildEndTime = record.lastChildEndTime;
        var aggregatedStats = record.aggregatedStats;
        for (var currentRecord = record.parent; currentRecord && !currentRecord.isRoot(); currentRecord = currentRecord.parent) {
            currentRecord._cpuTime += record._cpuTime;
            if (currentRecord.lastChildEndTime < lastChildEndTime)
                currentRecord.lastChildEndTime = lastChildEndTime;
            for (var category in aggregatedStats)
                currentRecord.aggregatedStats[category] += aggregatedStats[category];
        }
    },

    /**
     * @param {Object} record
     * @param {String} bucket
     * @return {WebInspector.TimelinePresentationModel.Record?}
     */
    _findCoalescedParent: function(record, bucket)
    {
        const coalescingThresholdSeconds = 0.001;

        var lastRecord = this._coalescingBuckets[bucket];
        var startTime = WebInspector.TimelineModel.startTimeInSeconds(record);
        var endTime = WebInspector.TimelineModel.endTimeInSeconds(record);
        if (!lastRecord || lastRecord.type !== record.type)
            return null;
        if (lastRecord.endTime + coalescingThresholdSeconds < startTime)
            return null;
        if (endTime + coalescingThresholdSeconds < lastRecord.startTime)
            return null;
        if (lastRecord.parent.coalesced)
            return lastRecord.parent;
        // Do not aggregate records that were reparented.
        if (lastRecord.parent !== this._rootRecord)
            return null;
        return this._replaceWithCoalescedRecord(lastRecord);
    },

    /**
     * @param {WebInspector.TimelinePresentationModel.Record} record
     * @return {WebInspector.TimelinePresentationModel.Record}
     */
    _replaceWithCoalescedRecord: function(record)
    {
        var rawRecord = {
            type: record._record.type,
            startTime: record._record.startTime,
            endTime: record._record.endTime,
            data: { }
        };
        if (record._record.thread)
            rawRecord.thread = "aggregated";
        var coalescedRecord = new WebInspector.TimelinePresentationModel.Record(this, rawRecord, null, null, null, false);
        var parent = record.parent;

        coalescedRecord.coalesced = true;
        coalescedRecord.collapsed = true;
        coalescedRecord._children.push(record);
        record.parent = coalescedRecord;
        coalescedRecord.calculateAggregatedStats();
        if (record.hasWarning || record.childHasWarning)
            coalescedRecord.childHasWarning = true;

        coalescedRecord.parent = parent;
        parent._children[parent._children.indexOf(record)] = coalescedRecord;
        return coalescedRecord;
    },

    _findParentRecord: function(record)
    {
        if (!this._glueRecords)
            return null;
        var recordTypes = WebInspector.TimelineModel.RecordType;

        switch (record.type) {
        case recordTypes.ResourceReceiveResponse:
        case recordTypes.ResourceFinish:
        case recordTypes.ResourceReceivedData:
            return this._sendRequestRecords[record.data["requestId"]];

        case recordTypes.ResourceSendRequest:
            return this._rootRecord;

        case recordTypes.TimerFire:
            return this._timerRecords[record.data["timerId"]];

        case recordTypes.ResourceSendRequest:
            return this._scheduledResourceRequests[record.data["url"]];

        case recordTypes.FireAnimationFrame:
            return this._requestAnimationFrameRecords[record.data["id"]];

        case recordTypes.Time:
            return this._rootRecord;

        case recordTypes.TimeEnd:
            return this._timeRecords[record.data["message"]];
        }
    },

    setGlueRecords: function(glue)
    {
        this._glueRecords = glue;
    },

    invalidateFilteredRecords: function()
    {
        delete this._filteredRecords;
    },

    filteredRecords: function()
    {
        if (this._filteredRecords)
            return this._filteredRecords;

        var recordsInWindow = [];

        var stack = [{children: this._rootRecord.children, index: 0, parentIsCollapsed: false}];
        while (stack.length) {
            var entry = stack[stack.length - 1];
            var records = entry.children;
            if (records && entry.index < records.length) {
                 var record = records[entry.index];
                 ++entry.index;

                 if (this.isVisible(record)) {
                     ++record.parent._invisibleChildrenCount;
                     if (!entry.parentIsCollapsed)
                         recordsInWindow.push(record);
                 }

                 record._invisibleChildrenCount = 0;

                 stack.push({children: record.children,
                             index: 0,
                             parentIsCollapsed: (entry.parentIsCollapsed || record.collapsed),
                             parentRecord: record,
                             windowLengthBeforeChildrenTraversal: recordsInWindow.length});
            } else {
                stack.pop();
                if (entry.parentRecord)
                    entry.parentRecord._visibleChildrenCount = recordsInWindow.length - entry.windowLengthBeforeChildrenTraversal;
            }
        }

        this._filteredRecords = recordsInWindow;
        return recordsInWindow;
    },

    filteredFrames: function(startTime, endTime)
    {
        function compareStartTime(value, object)
        {
            return value - object.startTime;
        }
        function compareEndTime(value, object)
        {
            return value - object.endTime;
        }
        var firstFrame = insertionIndexForObjectInListSortedByFunction(startTime, this._frames, compareStartTime);
        var lastFrame = insertionIndexForObjectInListSortedByFunction(endTime, this._frames, compareEndTime);
        while (lastFrame < this._frames.length && this._frames[lastFrame].endTime <= endTime)
            ++lastFrame;
        return this._frames.slice(firstFrame, lastFrame);
    },

    eventDividerRecords: function()
    {
        return this._eventDividerRecords;
    },

    isVisible: function(record)
    {
        for (var i = 0; i < this._filters.length; ++i) {
            if (!this._filters[i].accept(record))
                return false;
        }
        return true;
    },

    /**
     * @param {{tasks: !Array.<{startTime: number, endTime: number}>, firstTaskIndex: number, lastTaskIndex: number}} info
     * @return {!Element}
     */
    generateMainThreadBarPopupContent: function(info)
    {
        var firstTaskIndex = info.firstTaskIndex;
        var lastTaskIndex = info.lastTaskIndex;
        var tasks = info.tasks;
        var messageCount = lastTaskIndex - firstTaskIndex + 1;
        var cpuTime = 0;

        for (var i = firstTaskIndex; i <= lastTaskIndex; ++i) {
            var task = tasks[i];
            cpuTime += task.endTime - task.startTime;
        }
        var startTime = tasks[firstTaskIndex].startTime;
        var endTime = tasks[lastTaskIndex].endTime;
        var duration = endTime - startTime;
        var offset = this._minimumRecordTime;

        var contentHelper = new WebInspector.PopoverContentHelper(WebInspector.UIString("CPU"));
        var durationText = WebInspector.UIString("%s (at %s)", Number.secondsToString(duration, true),
            Number.secondsToString(startTime - offset, true));
        contentHelper.appendTextRow(WebInspector.UIString("Duration"), durationText);
        contentHelper.appendTextRow(WebInspector.UIString("CPU time"), Number.secondsToString(cpuTime, true));
        contentHelper.appendTextRow(WebInspector.UIString("Message Count"), messageCount);
        return contentHelper.contentTable();
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 * @param {WebInspector.TimelinePresentationModel} presentationModel
 * @param {Object} record
 * @param {WebInspector.TimelinePresentationModel.Record} parentRecord
 * @param {WebInspector.TimelinePresentationModel.Record} origin
 * @param {Object|undefined} scriptDetails
 * @param {boolean} hidden
 */
WebInspector.TimelinePresentationModel.Record = function(presentationModel, record, parentRecord, origin, scriptDetails, hidden)
{
    this._linkifier = presentationModel._linkifier;
    this._aggregatedStats = {};
    this._record = record;
    this._children = [];
    if (!hidden && parentRecord) {
        this.parent = parentRecord;
        if (this.isBackground)
            WebInspector.TimelinePresentationModel.insertRetrospectiveRecord(parentRecord, this);
        else
            parentRecord.children.push(this);
    }
    if (origin)
        this._origin = origin;

    this._selfTime = this.endTime - this.startTime;
    this._lastChildEndTime = this.endTime;
    this._startTimeOffset = this.startTime - presentationModel._minimumRecordTime;

    if (record.data && record.data["url"])
        this.url = record.data["url"];
    if (scriptDetails) {
        this.scriptName = scriptDetails.scriptName;
        this.scriptLine = scriptDetails.scriptLine;
    }
    if (parentRecord && parentRecord.callSiteStackTrace)
        this.callSiteStackTrace = parentRecord.callSiteStackTrace;

    var recordTypes = WebInspector.TimelineModel.RecordType;
    switch (record.type) {
    case recordTypes.ResourceSendRequest:
        // Make resource receive record last since request was sent; make finish record last since response received.
        presentationModel._sendRequestRecords[record.data["requestId"]] = this;
        break;

    case recordTypes.ScheduleResourceRequest:
        presentationModel._scheduledResourceRequests[record.data["url"]] = this;
        break;

    case recordTypes.ResourceReceiveResponse:
        var sendRequestRecord = presentationModel._sendRequestRecords[record.data["requestId"]];
        if (sendRequestRecord) { // False if we started instrumentation in the middle of request.
            this.url = sendRequestRecord.url;
            // Now that we have resource in the collection, recalculate details in order to display short url.
            sendRequestRecord._refreshDetails();
            if (sendRequestRecord.parent !== presentationModel._rootRecord && sendRequestRecord.parent.type === recordTypes.ScheduleResourceRequest)
                sendRequestRecord.parent._refreshDetails();
        }
        break;

    case recordTypes.ResourceReceivedData:
    case recordTypes.ResourceFinish:
        var sendRequestRecord = presentationModel._sendRequestRecords[record.data["requestId"]];
        if (sendRequestRecord) // False for main resource.
            this.url = sendRequestRecord.url;
        break;

    case recordTypes.TimerInstall:
        this.timeout = record.data["timeout"];
        this.singleShot = record.data["singleShot"];
        presentationModel._timerRecords[record.data["timerId"]] = this;
        break;

    case recordTypes.TimerFire:
        var timerInstalledRecord = presentationModel._timerRecords[record.data["timerId"]];
        if (timerInstalledRecord) {
            this.callSiteStackTrace = timerInstalledRecord.stackTrace;
            this.timeout = timerInstalledRecord.timeout;
            this.singleShot = timerInstalledRecord.singleShot;
        }
        break;

    case recordTypes.RequestAnimationFrame:
        presentationModel._requestAnimationFrameRecords[record.data["id"]] = this;
        break;

    case recordTypes.FireAnimationFrame:
        var requestAnimationRecord = presentationModel._requestAnimationFrameRecords[record.data["id"]];
        if (requestAnimationRecord)
            this.callSiteStackTrace = requestAnimationRecord.stackTrace;
        break;

    case recordTypes.Time:
        var message = record.data["message"];
        var oldReference = presentationModel._timeRecords[message];
        if (oldReference)
            break;
        presentationModel._timeRecords[message] = this;
        if (origin)
            presentationModel._timeRecordStack.push(this);
        break;

    case recordTypes.TimeEnd:
        var message = record.data["message"];
        var timeRecord = presentationModel._timeRecords[message];
        delete presentationModel._timeRecords[message];
        if (timeRecord) {
            this.timeRecord = timeRecord;
            timeRecord.timeEndRecord = this;
            var intervalDuration = this.startTime - timeRecord.startTime;
            this.intervalDuration = intervalDuration;
            timeRecord.intervalDuration = intervalDuration;
            if (!origin)
                break;
            var recordStack = presentationModel._timeRecordStack;
            recordStack.splice(recordStack.indexOf(timeRecord), 1);
            for (var index = recordStack.length; index; --index) {
                var openRecord = recordStack[index - 1];
                if (openRecord.startTime > timeRecord.startTime)
                    continue;
                WebInspector.TimelinePresentationModel.adoptRecord(openRecord, timeRecord);
                break;
            }
        }
        break;

    case recordTypes.ScheduleStyleRecalculation:
        presentationModel._lastScheduleStyleRecalculation[this.frameId] = this;
        break;

    case recordTypes.RecalculateStyles:
        var scheduleStyleRecalculationRecord = presentationModel._lastScheduleStyleRecalculation[this.frameId];
        if (!scheduleStyleRecalculationRecord)
            break;
        this.callSiteStackTrace = scheduleStyleRecalculationRecord.stackTrace;
        break;

    case recordTypes.InvalidateLayout:
        // Consider style recalculation as a reason for layout invalidation,
        // but only if we had no earlier layout invalidation records.
        var styleRecalcStack;
        if (!presentationModel._layoutInvalidateStack[this.frameId]) {
            for (var outerRecord = parentRecord; outerRecord; outerRecord = record.parent) {
                if (outerRecord.type === recordTypes.RecalculateStyles) {
                    styleRecalcStack = outerRecord.callSiteStackTrace;
                    break;
                }
            }
        }
        presentationModel._layoutInvalidateStack[this.frameId] = styleRecalcStack || this.stackTrace;
        break;

    case recordTypes.Layout:
        var layoutInvalidateStack = presentationModel._layoutInvalidateStack[this.frameId];
        if (layoutInvalidateStack)
            this.callSiteStackTrace = layoutInvalidateStack;
        if (this.stackTrace)
            this.setHasWarning();
        presentationModel._layoutInvalidateStack[this.frameId] = null;
        this.highlightQuad = record.data.root;
        break;

    case recordTypes.Paint:
        this.highlightQuad = record.data.clip;
        break;

    case recordTypes.WebSocketCreate:
        this.webSocketURL = record.data["url"];
        if (typeof record.data["webSocketProtocol"] !== "undefined")
            this.webSocketProtocol = record.data["webSocketProtocol"];
        presentationModel._webSocketCreateRecords[record.data["identifier"]] = this;
        break;
   
    case recordTypes.WebSocketSendHandshakeRequest:
    case recordTypes.WebSocketReceiveHandshakeResponse:
    case recordTypes.WebSocketDestroy:
        var webSocketCreateRecord = presentationModel._webSocketCreateRecords[record.data["identifier"]];
        if (webSocketCreateRecord) { // False if we started instrumentation in the middle of request.
            this.webSocketURL = webSocketCreateRecord.webSocketURL;
            if (typeof webSocketCreateRecord.webSocketProtocol !== "undefined")
                this.webSocketProtocol = webSocketCreateRecord.webSocketProtocol;
        }
        break;
    }
}

WebInspector.TimelinePresentationModel.adoptRecord = function(newParent, record)
{
    record.parent.children.splice(record.parent.children.indexOf(record));
    WebInspector.TimelinePresentationModel.insertRetrospectiveRecord(newParent, record);
    record.parent = newParent;
}

WebInspector.TimelinePresentationModel.insertRetrospectiveRecord = function(parent, record)
{
    function compareStartTime(value, record)
    {
        return value < record.startTime ? -1 : 1;
    }
    
    parent.children.splice(insertionIndexForObjectInListSortedByFunction(record.startTime, parent.children, compareStartTime), 0, record);
}

WebInspector.TimelinePresentationModel.Record.prototype = {
    get lastChildEndTime()
    {
        return this._lastChildEndTime;
    },

    set lastChildEndTime(time)
    {
        this._lastChildEndTime = time;
    },

    get selfTime()
    {
        return this.coalesced ? this._lastChildEndTime - this.startTime : this._selfTime;
    },

    set selfTime(time)
    {
        this._selfTime = time;
    },

    get cpuTime()
    {
        return this._cpuTime;
    },

    /**
     * @return {boolean}
     */
    isRoot: function()
    {
        return this.type === WebInspector.TimelineModel.RecordType.Root;
    },

    /**
     * @return {WebInspector.TimelinePresentationModel.Record}
     */
    origin: function()
    {
        return this._origin || this.parent;
    },

    /**
     * @return {Array.<WebInspector.TimelinePresentationModel.Record>}
     */
    get children()
    {
        return this._children;
    },

    /**
     * @return {number}
     */
    get visibleChildrenCount()
    {
        return this._visibleChildrenCount || 0;
    },

    /**
     * @return {number}
     */
    get invisibleChildrenCount()
    {
        return this._invisibleChildrenCount || 0;
    },

    /**
     * @return {WebInspector.TimelineCategory}
     */
    get category()
    {
        return WebInspector.TimelinePresentationModel.recordStyle(this._record).category
    },

    /**
     * @return {string}
     */
    get title()
    {
        return this.type === WebInspector.TimelineModel.RecordType.TimeStamp ? this._record.data["message"] :
            WebInspector.TimelinePresentationModel.recordStyle(this._record).title;
    },

    /**
     * @return {number}
     */
    get startTime()
    {
        return WebInspector.TimelineModel.startTimeInSeconds(this._record);
    },

    /**
     * @return {number}
     */
    get endTime()
    {
        return WebInspector.TimelineModel.endTimeInSeconds(this._record);
    },

    /**
     * @return {boolean}
     */
    get isBackground()
    {
        return !!this._record.thread;
    },

    /**
     * @return {Object}
     */
    get data()
    {
        return this._record.data;
    },

    /**
     * @return {string}
     */
    get type()
    {
        return this._record.type;
    },

    /**
     * @return {string}
     */
    get frameId()
    {
        return this._record.frameId;
    },

    /**
     * @return {number}
     */
    get usedHeapSizeDelta()
    {
        return this._record.usedHeapSizeDelta || 0;
    },

    /**
     * @return {number}
     */
    get usedHeapSize()
    {
        return this._record.usedHeapSize;
    },

    /**
     * @return {Array.<DebuggerAgent.CallFrame>?}
     */
    get stackTrace()
    {
        if (this._record.stackTrace && this._record.stackTrace.length)
            return this._record.stackTrace;
        return null;
    },

    containsTime: function(time)
    {
        return this.startTime <= time && time <= this.endTime;
    },

    /**
     * @param {function(Element)} callback
     */
    generatePopupContent: function(callback)
    {
        if (WebInspector.TimelinePresentationModel.needsPreviewElement(this.type))
            WebInspector.DOMPresentationUtils.buildImagePreviewContents(this.url, false, this._generatePopupContentWithImagePreview.bind(this, callback));
        else
            this._generatePopupContentWithImagePreview(callback);
    },

    /**
     * @param {function(Element)} callback
     * @param {Element=} previewElement
     */
    _generatePopupContentWithImagePreview: function(callback, previewElement)
    {
        var contentHelper = new WebInspector.PopoverContentHelper(this.title);
        var text = WebInspector.UIString("%s (at %s)", Number.secondsToString(this._lastChildEndTime - this.startTime, true),
            Number.secondsToString(this._startTimeOffset));
        contentHelper.appendTextRow(WebInspector.UIString("Duration"), text);

        if (this._children.length) {
            if (!this.coalesced)
                contentHelper.appendTextRow(WebInspector.UIString("Self Time"), Number.secondsToString(this._selfTime, true));
            contentHelper.appendTextRow(WebInspector.UIString("CPU Time"), Number.secondsToString(this._cpuTime, true));
            contentHelper.appendElementRow(WebInspector.UIString("Aggregated Time"),
                WebInspector.TimelinePresentationModel._generateAggregatedInfo(this._aggregatedStats));
        }

        if (this.coalesced) {
            callback(contentHelper.contentTable());
            return;
        }
        const recordTypes = WebInspector.TimelineModel.RecordType;

        // The messages may vary per record type;
        var callSiteStackTraceLabel;
        var callStackLabel;

        switch (this.type) {
            case recordTypes.GCEvent:
                contentHelper.appendTextRow(WebInspector.UIString("Collected"), Number.bytesToString(this.data["usedHeapSizeDelta"]));
                break;
            case recordTypes.TimerInstall:
            case recordTypes.TimerFire:
            case recordTypes.TimerRemove:
                contentHelper.appendTextRow(WebInspector.UIString("Timer ID"), this.data["timerId"]);
                if (typeof this.timeout === "number") {
                    contentHelper.appendTextRow(WebInspector.UIString("Timeout"), Number.secondsToString(this.timeout / 1000));
                    contentHelper.appendTextRow(WebInspector.UIString("Repeats"), !this.singleShot);
                }
                break;
            case recordTypes.FireAnimationFrame:
                contentHelper.appendTextRow(WebInspector.UIString("Callback ID"), this.data["id"]);
                break;
            case recordTypes.FunctionCall:
                contentHelper.appendElementRow(WebInspector.UIString("Location"), this._linkifyScriptLocation());
                break;
            case recordTypes.ScheduleResourceRequest:
            case recordTypes.ResourceSendRequest:
            case recordTypes.ResourceReceiveResponse:
            case recordTypes.ResourceReceivedData:
            case recordTypes.ResourceFinish:
                contentHelper.appendElementRow(WebInspector.UIString("Resource"), WebInspector.linkifyResourceAsNode(this.url));
                if (previewElement)
                    contentHelper.appendElementRow(WebInspector.UIString("Preview"), previewElement);
                if (this.data["requestMethod"])
                    contentHelper.appendTextRow(WebInspector.UIString("Request Method"), this.data["requestMethod"]);
                if (typeof this.data["statusCode"] === "number")
                    contentHelper.appendTextRow(WebInspector.UIString("Status Code"), this.data["statusCode"]);
                if (this.data["mimeType"])
                    contentHelper.appendTextRow(WebInspector.UIString("MIME Type"), this.data["mimeType"]);
                if (this.data["encodedDataLength"])
                    contentHelper.appendTextRow(WebInspector.UIString("Encoded Data Length"), WebInspector.UIString("%d Bytes", this.data["encodedDataLength"]));
                break;
            case recordTypes.EvaluateScript:
                if (this.data && this.url)
                    contentHelper.appendElementRow(WebInspector.UIString("Script"), this._linkifyLocation(this.url, this.data["lineNumber"]));
                break;
            case recordTypes.Paint:
                contentHelper.appendTextRow(WebInspector.UIString("Location"), WebInspector.UIString("(%d, %d)", this.data.clip[0], this.data.clip[1]));
                var clipWidth = WebInspector.TimelinePresentationModel.quadWidth(this.data.clip);
                var clipHeight = WebInspector.TimelinePresentationModel.quadHeight(this.data.clip);
                contentHelper.appendTextRow(WebInspector.UIString("Dimensions"), WebInspector.UIString("%d × %d", clipWidth, clipHeight));
                break;
            case recordTypes.RecalculateStyles: // We don't want to see default details.
                callSiteStackTraceLabel = WebInspector.UIString("Styles invalidated");
                callStackLabel = WebInspector.UIString("Styles recalculation forced");
                break;
            case recordTypes.Layout:
                if (this.data["dirtyObjects"])
                    contentHelper.appendTextRow(WebInspector.UIString("Nodes that need layout"), this.data["dirtyObjects"]);
                if (this.data["totalObjects"])
                    contentHelper.appendTextRow(WebInspector.UIString("Layout tree size"), this.data["totalObjects"]);
                if (typeof this.data["partialLayout"] === "boolean") {
                    contentHelper.appendTextRow(WebInspector.UIString("Layout scope"),
                       this.data["partialLayout"] ? WebInspector.UIString("Partial") : WebInspector.UIString("Whole document"));
                }
                callSiteStackTraceLabel = WebInspector.UIString("Layout invalidated");
                if (this.stackTrace) {
                    callStackLabel = WebInspector.UIString("Layout forced");
                    contentHelper.appendTextRow(WebInspector.UIString("Note"), WebInspector.UIString("Forced synchronous layout is a possible performance bottleneck."));
                }
                break;
            case recordTypes.Time:
            case recordTypes.TimeEnd:
                contentHelper.appendTextRow(WebInspector.UIString("Message"), this.data["message"]);
                if (typeof this.intervalDuration === "number")
                    contentHelper.appendTextRow(WebInspector.UIString("Interval Duration"), Number.secondsToString(this.intervalDuration, true));
                break;
            case recordTypes.WebSocketCreate:
            case recordTypes.WebSocketSendHandshakeRequest:
            case recordTypes.WebSocketReceiveHandshakeResponse:
            case recordTypes.WebSocketDestroy:
                if (typeof this.webSocketURL !== "undefined")
                    contentHelper.appendTextRow(WebInspector.UIString("URL"), this.webSocketURL);
                if (typeof this.webSocketProtocol !== "undefined")
                    contentHelper.appendTextRow(WebInspector.UIString("WebSocket Protocol"), this.webSocketProtocol);
                if (typeof this.data["message"] !== "undefined")
                    contentHelper.appendTextRow(WebInspector.UIString("Message"), this.data["message"])
                    break;
            default:
                if (this.detailsNode())
                    contentHelper.appendElementRow(WebInspector.UIString("Details"), this.detailsNode().childNodes[1].cloneNode());
                break;
        }

        if (this.scriptName && this.type !== recordTypes.FunctionCall)
            contentHelper.appendElementRow(WebInspector.UIString("Function Call"), this._linkifyScriptLocation());

        if (this.usedHeapSize) {
            if (this.usedHeapSizeDelta) {
                var sign = this.usedHeapSizeDelta > 0 ? "+" : "-";
                contentHelper.appendTextRow(WebInspector.UIString("Used Heap Size"),
                    WebInspector.UIString("%s (%s%s)", Number.bytesToString(this.usedHeapSize), sign, Number.bytesToString(this.usedHeapSizeDelta)));
            } else if (this.category === WebInspector.TimelinePresentationModel.categories().scripting)
                contentHelper.appendTextRow(WebInspector.UIString("Used Heap Size"), Number.bytesToString(this.usedHeapSize));
        }

        if (this.callSiteStackTrace)
            contentHelper.appendStackTrace(callSiteStackTraceLabel || WebInspector.UIString("Call Site stack"), this.callSiteStackTrace, this._linkifyCallFrame.bind(this));

        if (this.stackTrace)
            contentHelper.appendStackTrace(callStackLabel || WebInspector.UIString("Call Stack"), this.stackTrace, this._linkifyCallFrame.bind(this));

        callback(contentHelper.contentTable());
    },

    _refreshDetails: function()
    {
        delete this._detailsNode;
    },

    /**
     * @return {?Node}
     */
    detailsNode: function()
    {
        if (typeof this._detailsNode === "undefined") {
            this._detailsNode = this._getRecordDetails();

            if (this._detailsNode) {
                this._detailsNode.insertBefore(document.createTextNode("("), this._detailsNode.firstChild);
                this._detailsNode.appendChild(document.createTextNode(")"));
            }
        }
        return this._detailsNode;
    },

    _createSpanWithText: function(textContent)
    {
        var node = document.createElement("span");
        node.textContent = textContent;
        return node;
    },

    /**
     * @return {?Node}
     */
    _getRecordDetails: function()
    {
        var details;
        if (this.coalesced)
            return this._createSpanWithText(WebInspector.UIString("× %d", this.children.length));

        switch (this.type) {
        case WebInspector.TimelineModel.RecordType.GCEvent:
            details = WebInspector.UIString("%s collected", Number.bytesToString(this.data["usedHeapSizeDelta"]));
            break;
        case WebInspector.TimelineModel.RecordType.TimerFire:
            details = this._linkifyScriptLocation(this.data["timerId"]);
            break;
        case WebInspector.TimelineModel.RecordType.FunctionCall:
            details = this._linkifyScriptLocation();
            break;
        case WebInspector.TimelineModel.RecordType.FireAnimationFrame:
            details = this._linkifyScriptLocation(this.data["id"]);
            break;
        case WebInspector.TimelineModel.RecordType.EventDispatch:
            details = this.data ? this.data["type"] : null;
            break;
        case WebInspector.TimelineModel.RecordType.Paint:
            details = WebInspector.TimelinePresentationModel.quadWidth(this.data.clip)  + "\u2009\u00d7\u2009" + WebInspector.TimelinePresentationModel.quadHeight(this.data.clip);
            break;
        case WebInspector.TimelineModel.RecordType.DecodeImage:
            details = this.data["imageType"];
            break;
        case WebInspector.TimelineModel.RecordType.ResizeImage:
            details = this.data["cached"] ? WebInspector.UIString("cached") : WebInspector.UIString("non-cached");
            break;
        case WebInspector.TimelineModel.RecordType.TimerInstall:
        case WebInspector.TimelineModel.RecordType.TimerRemove:
            details = this._linkifyTopCallFrame(this.data["timerId"]);
            break;
        case WebInspector.TimelineModel.RecordType.RequestAnimationFrame:
        case WebInspector.TimelineModel.RecordType.CancelAnimationFrame:
            details = this._linkifyTopCallFrame(this.data["id"]);
            break;
        case WebInspector.TimelineModel.RecordType.ParseHTML:
        case WebInspector.TimelineModel.RecordType.RecalculateStyles:
            details = this._linkifyTopCallFrame();
            break;
        case WebInspector.TimelineModel.RecordType.EvaluateScript:
            details = this.url ? this._linkifyLocation(this.url, this.data["lineNumber"], 0) : null;
            break;
        case WebInspector.TimelineModel.RecordType.XHRReadyStateChange:
        case WebInspector.TimelineModel.RecordType.XHRLoad:
        case WebInspector.TimelineModel.RecordType.ScheduleResourceRequest:
        case WebInspector.TimelineModel.RecordType.ResourceSendRequest:
        case WebInspector.TimelineModel.RecordType.ResourceReceivedData:
        case WebInspector.TimelineModel.RecordType.ResourceReceiveResponse:
        case WebInspector.TimelineModel.RecordType.ResourceFinish:
            details = WebInspector.displayNameForURL(this.url);
            break;
        case WebInspector.TimelineModel.RecordType.Time:
        case WebInspector.TimelineModel.RecordType.TimeEnd:
        case WebInspector.TimelineModel.RecordType.TimeStamp:
            details = this.data["message"];
            break;
        default:
            details = this._linkifyScriptLocation() || this._linkifyTopCallFrame() || null;
            break;
        }

        if (details && !(details instanceof Node))
            return this._createSpanWithText("" + details);

        return details || null;
    },

    /**
     * @param {string} url
     * @param {number} lineNumber
     * @param {number=} columnNumber
     */
    _linkifyLocation: function(url, lineNumber, columnNumber)
    {
        // FIXME(62725): stack trace line/column numbers are one-based.
        columnNumber = columnNumber ? columnNumber - 1 : 0;
        return this._linkifier.linkifyLocation(url, lineNumber - 1, columnNumber, "timeline-details");
    },

    _linkifyCallFrame: function(callFrame)
    {
        return this._linkifyLocation(callFrame.url, callFrame.lineNumber, callFrame.columnNumber);
    },

    /**
     * @param {string=} defaultValue
     */
    _linkifyTopCallFrame: function(defaultValue)
    {
        if (this.stackTrace)
            return this._linkifyCallFrame(this.stackTrace[0]);
        if (this.callSiteStackTrace)
            return this._linkifyCallFrame(this.callSiteStackTrace[0]);
        return defaultValue;
    },

    /**
     * @param {*=} defaultValue
     * @return {Element|string}
     */
    _linkifyScriptLocation: function(defaultValue)
    {
        if (this.scriptName)
            return this._linkifyLocation(this.scriptName, this.scriptLine, 0);
        else
            return defaultValue ? "" + defaultValue : null;
    },

    calculateAggregatedStats: function()
    {
        this._aggregatedStats = {};
        this._cpuTime = this._selfTime;

        for (var index = this._children.length; index; --index) {
            var child = this._children[index - 1];
            for (var category in child._aggregatedStats)
                this._aggregatedStats[category] = (this._aggregatedStats[category] || 0) + child._aggregatedStats[category];
        }
        for (var category in this._aggregatedStats)
            this._cpuTime += this._aggregatedStats[category];
        this._aggregatedStats[this.category.name] = (this._aggregatedStats[this.category.name] || 0) + this._selfTime;
    },

    get aggregatedStats()
    {
        return this._aggregatedStats;
    },

    setHasWarning: function()
    {
        this.hasWarning = true;
        for (var parent = this.parent; parent && !parent.childHasWarning; parent = parent.parent)
            parent.childHasWarning = true;
    }
}

/**
 * @param {Object} aggregatedStats
 */
WebInspector.TimelinePresentationModel._generateAggregatedInfo = function(aggregatedStats)
{
    var cell = document.createElement("span");
    cell.className = "timeline-aggregated-info";
    for (var index in aggregatedStats) {
        var label = document.createElement("div");
        label.className = "timeline-aggregated-category timeline-" + index;
        cell.appendChild(label);
        var text = document.createElement("span");
        text.textContent = Number.secondsToString(aggregatedStats[index], true);
        cell.appendChild(text);
    }
    return cell;
}

WebInspector.TimelinePresentationModel.generatePopupContentForFrame = function(frame)
{
    var contentHelper = new WebInspector.PopoverContentHelper(WebInspector.UIString("Frame"));
    var durationInSeconds = frame.endTime - frame.startTime;
    var durationText = WebInspector.UIString("%s (at %s)", Number.secondsToString(frame.endTime - frame.startTime, true),
        Number.secondsToString(frame.startTimeOffset, true));
    contentHelper.appendTextRow(WebInspector.UIString("Duration"), durationText);
    contentHelper.appendTextRow(WebInspector.UIString("FPS"), Math.floor(1 / durationInSeconds));
    contentHelper.appendTextRow(WebInspector.UIString("CPU time"), Number.secondsToString(frame.cpuTime, true));
    contentHelper.appendElementRow(WebInspector.UIString("Aggregated Time"),
        WebInspector.TimelinePresentationModel._generateAggregatedInfo(frame.timeByCategory));

    return contentHelper.contentTable();
}

/**
 * @param {WebInspector.FrameStatistics} statistics
 */
WebInspector.TimelinePresentationModel.generatePopupContentForFrameStatistics = function(statistics)
{
    /**
     * @param {number} time
     */
    function formatTimeAndFPS(time)
    {
        return WebInspector.UIString("%s (%.0f FPS)", Number.secondsToString(time, true), 1 / time);
    }

    var contentHelper = new WebInspector.PopoverContentHelper(WebInspector.UIString("Selected Range"));

    contentHelper.appendTextRow(WebInspector.UIString("Selected range"), WebInspector.UIString("%s\u2013%s (%d frames)",
        Number.secondsToString(statistics.startOffset, true), Number.secondsToString(statistics.endOffset, true), statistics.frameCount));
    contentHelper.appendTextRow(WebInspector.UIString("Minimum Time"), formatTimeAndFPS(statistics.minDuration));
    contentHelper.appendTextRow(WebInspector.UIString("Average Time"), formatTimeAndFPS(statistics.average));
    contentHelper.appendTextRow(WebInspector.UIString("Maximum Time"), formatTimeAndFPS(statistics.maxDuration));
    contentHelper.appendTextRow(WebInspector.UIString("Standard Deviation"), Number.secondsToString(statistics.stddev, true));
    contentHelper.appendElementRow(WebInspector.UIString("Time by category"),
        WebInspector.TimelinePresentationModel._generateAggregatedInfo(statistics.timeByCategory));

    return contentHelper.contentTable();
}

/**
 * @param {CanvasRenderingContext2D} context
 * @param {number} width
 * @param {number} height
 * @param {string} color0
 * @param {string} color1
 * @param {string} color2
 */
WebInspector.TimelinePresentationModel.createFillStyle = function(context, width, height, color0, color1, color2)
{
    var gradient = context.createLinearGradient(0, 0, width, height);
    gradient.addColorStop(0, color0);
    gradient.addColorStop(0.25, color1);
    gradient.addColorStop(0.75, color1);
    gradient.addColorStop(1, color2);
    return gradient;
}

/**
 * @param {CanvasRenderingContext2D} context
 * @param {number} width
 * @param {number} height
 * @param {WebInspector.TimelineCategory} category
 */
WebInspector.TimelinePresentationModel.createFillStyleForCategory = function(context, width, height, category)
{
    return WebInspector.TimelinePresentationModel.createFillStyle(context, width, height, category.fillColorStop0, category.fillColorStop1, category.borderColor);
}

/**
 * @param {WebInspector.TimelineCategory} category
 */
WebInspector.TimelinePresentationModel.createStyleRuleForCategory = function(category)
{
    var selector = ".timeline-category-" + category.name + " .timeline-graph-bar, " +
        ".timeline-category-statusbar-item.timeline-category-" + category.name + " .timeline-category-checkbox, " +
        ".popover .timeline-" + category.name + ", " +
        ".timeline-category-" + category.name + " .timeline-tree-icon"

    return selector + " { background-image: -webkit-linear-gradient(" +
       category.fillColorStop0 + ", " + category.fillColorStop1 + " 25%, " + category.fillColorStop1 + " 75%, " + category.borderColor + ");" +
       " border-color: " + category.borderColor +
       "}";
}

/**
 * @param {Array.<number>} quad
 * @return {number}
 */
WebInspector.TimelinePresentationModel.quadWidth = function(quad)
{
    return Math.round(Math.sqrt(Math.pow(quad[0] - quad[2], 2) + Math.pow(quad[1] - quad[3], 2)));
}

/**
 * @param {Array.<number>} quad
 * @return {number}
 */
WebInspector.TimelinePresentationModel.quadHeight = function(quad)
{
    return Math.round(Math.sqrt(Math.pow(quad[0] - quad[6], 2) + Math.pow(quad[1] - quad[7], 2)));
}

/**
 * @interface
 */
WebInspector.TimelinePresentationModel.Filter = function()
{
}

WebInspector.TimelinePresentationModel.Filter.prototype = {
    /**
     * @param {!WebInspector.TimelinePresentationModel.Record} record
     * @return {boolean}
     */
    accept: function(record) { return false; }
}

/**
 * @constructor
 * @extends {WebInspector.Object}
 * @param {string} name
 * @param {string} title
 * @param {number} overviewStripGroupIndex
 * @param {string} borderColor
 * @param {string} fillColorStop0
 * @param {string} fillColorStop1
 */
WebInspector.TimelineCategory = function(name, title, overviewStripGroupIndex, borderColor, fillColorStop0, fillColorStop1)
{
    this.name = name;
    this.title = title;
    this.overviewStripGroupIndex = overviewStripGroupIndex;
    this.borderColor = borderColor;
    this.fillColorStop0 = fillColorStop0;
    this.fillColorStop1 = fillColorStop1;
    this.hidden = false;
}

WebInspector.TimelineCategory.Events = {
    VisibilityChanged: "VisibilityChanged"
};

WebInspector.TimelineCategory.prototype = {
    /**
     * @return {boolean}
     */
    get hidden()
    {
        return this._hidden;
    },

    set hidden(hidden)
    {
        this._hidden = hidden;
        this.dispatchEventToListeners(WebInspector.TimelineCategory.Events.VisibilityChanged, this);
    },

    __proto__: WebInspector.Object.prototype
}

//@ sourceURL=http://localhost/inspector/front-end/TimelinePresentationModel.js
