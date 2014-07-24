/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
WebInspector.TimelineModel = function()
{
    this._records = [];
    this._stringPool = new StringPool();
    this._minimumRecordTime = -1;
    this._maximumRecordTime = -1;
    this._collectionEnabled = false;

    WebInspector.timelineManager.addEventListener(WebInspector.TimelineManager.EventTypes.TimelineEventRecorded, this._onRecordAdded, this);
}

WebInspector.TimelineModel.TransferChunkLengthBytes = 5000000;

WebInspector.TimelineModel.RecordType = {
    Root: "Root",
    Program: "Program",
    EventDispatch: "EventDispatch",

    BeginFrame: "BeginFrame",
    ScheduleStyleRecalculation: "ScheduleStyleRecalculation",
    RecalculateStyles: "RecalculateStyles",
    InvalidateLayout: "InvalidateLayout",
    Layout: "Layout",
    Paint: "Paint",
    Rasterize: "Rasterize",
    ScrollLayer: "ScrollLayer",
    DecodeImage: "DecodeImage",
    ResizeImage: "ResizeImage",
    CompositeLayers: "CompositeLayers",

    ParseHTML: "ParseHTML",

    TimerInstall: "TimerInstall",
    TimerRemove: "TimerRemove",
    TimerFire: "TimerFire",

    XHRReadyStateChange: "XHRReadyStateChange",
    XHRLoad: "XHRLoad",
    EvaluateScript: "EvaluateScript",

    MarkLoad: "MarkLoad",
    MarkDOMContent: "MarkDOMContent",

    TimeStamp: "TimeStamp",
    Time: "Time",
    TimeEnd: "TimeEnd",

    ScheduleResourceRequest: "ScheduleResourceRequest",
    ResourceSendRequest: "ResourceSendRequest",
    ResourceReceiveResponse: "ResourceReceiveResponse",
    ResourceReceivedData: "ResourceReceivedData",
    ResourceFinish: "ResourceFinish",

    FunctionCall: "FunctionCall",
    GCEvent: "GCEvent",

    RequestAnimationFrame: "RequestAnimationFrame",
    CancelAnimationFrame: "CancelAnimationFrame",
    FireAnimationFrame: "FireAnimationFrame",

    WebSocketCreate : "WebSocketCreate",
    WebSocketSendHandshakeRequest : "WebSocketSendHandshakeRequest",
    WebSocketReceiveHandshakeResponse : "WebSocketReceiveHandshakeResponse",
    WebSocketDestroy : "WebSocketDestroy",
}

WebInspector.TimelineModel.Events = {
    RecordAdded: "RecordAdded",
    RecordsCleared: "RecordsCleared"
}

WebInspector.TimelineModel.startTimeInSeconds = function(record)
{
    return record.startTime / 1000;
}

WebInspector.TimelineModel.endTimeInSeconds = function(record)
{
    return (typeof record.endTime === "undefined" ? record.startTime : record.endTime) / 1000;
}

WebInspector.TimelineModel.durationInSeconds = function(record)
{
    return WebInspector.TimelineModel.endTimeInSeconds(record) - WebInspector.TimelineModel.startTimeInSeconds(record);
}

/**
 * @param {Object} total
 * @param {Object} rawRecord
 */
WebInspector.TimelineModel.aggregateTimeForRecord = function(total, rawRecord)
{
    var childrenTime = 0;
    var children = rawRecord["children"] || [];
    for (var i = 0; i < children.length; ++i) {
        WebInspector.TimelineModel.aggregateTimeForRecord(total, children[i]);
        childrenTime += WebInspector.TimelineModel.durationInSeconds(children[i]);
    }
    var categoryName = WebInspector.TimelinePresentationModel.recordStyle(rawRecord).category.name;
    var ownTime = WebInspector.TimelineModel.durationInSeconds(rawRecord) - childrenTime;
    total[categoryName] = (total[categoryName] || 0) + ownTime;
}

/**
 * @param {Object} total
 * @param {Object} addend
 */
WebInspector.TimelineModel.aggregateTimeByCategory = function(total, addend)
{
    for (var category in addend)
        total[category] = (total[category] || 0) + addend[category];
}

WebInspector.TimelineModel.prototype = {
    /**
     * @param {boolean=} includeDomCounters
     * @param {boolean=} includeNativeMemoryStatistics
     */
    startRecord: function(includeDomCounters, includeNativeMemoryStatistics)
    {
        if (this._collectionEnabled)
            return;
        this.reset();
        var maxStackFrames = WebInspector.settings.timelineLimitStackFramesFlag.get() ? WebInspector.settings.timelineStackFramesToCapture.get() : 30;
        WebInspector.timelineManager.start(maxStackFrames, includeDomCounters, includeNativeMemoryStatistics);
        this._collectionEnabled = true;
    },

    stopRecord: function()
    {
        if (!this._collectionEnabled)
            return;
        WebInspector.timelineManager.stop();
        this._collectionEnabled = false;
    },

    get records()
    {
        return this._records;
    },

    _onRecordAdded: function(event)
    {
        if (this._collectionEnabled)
            this._addRecord(event.data);
    },

    _addRecord: function(record)
    {
        this._stringPool.internObjectStrings(record);
        this._records.push(record);
        this._updateBoundaries(record);
        this.dispatchEventToListeners(WebInspector.TimelineModel.Events.RecordAdded, record);
    },

    /**
     * @param {!Blob} file
     * @param {!WebInspector.Progress} progress
     */
    loadFromFile: function(file, progress)
    {
        var delegate = new WebInspector.TimelineModelLoadFromFileDelegate(this, progress);
        var fileReader = this._createFileReader(file, delegate);
        var loader = new WebInspector.TimelineModelLoader(this, fileReader, progress);
        fileReader.start(loader);
    },

    /**
     * @param {string} url
     */
    loadFromURL: function(url, progress)
    {
        var delegate = new WebInspector.TimelineModelLoadFromFileDelegate(this, progress);
        var urlReader = new WebInspector.ChunkedXHRReader(url, delegate);
        var loader = new WebInspector.TimelineModelLoader(this, urlReader, progress);
        urlReader.start(loader);
    },

    _createFileReader: function(file, delegate)
    {
        return new WebInspector.ChunkedFileReader(file, WebInspector.TimelineModel.TransferChunkLengthBytes, delegate);
    },

    _createFileWriter: function(fileName, callback)
    {
        var stream = new WebInspector.FileOutputStream();
        stream.open(fileName, callback);
    },

    saveToFile: function()
    {
        var now = new Date();
        var fileName = "TimelineRawData-" + now.toISO8601Compact() + ".json";
        function callback(stream)
        {
            var saver = new WebInspector.TimelineSaver(stream);
            saver.save(this._records, window.navigator.appVersion);
        }
        this._createFileWriter(fileName, callback.bind(this));
    },

    reset: function()
    {
        this._records = [];
        this._stringPool.reset();
        this._minimumRecordTime = -1;
        this._maximumRecordTime = -1;
        this.dispatchEventToListeners(WebInspector.TimelineModel.Events.RecordsCleared);
    },

    minimumRecordTime: function()
    {
        return this._minimumRecordTime;
    },

    maximumRecordTime: function()
    {
        return this._maximumRecordTime;
    },

    _updateBoundaries: function(record)
    {
        var startTime = WebInspector.TimelineModel.startTimeInSeconds(record);
        var endTime = WebInspector.TimelineModel.endTimeInSeconds(record);

        if (this._minimumRecordTime === -1 || startTime < this._minimumRecordTime)
            this._minimumRecordTime = startTime;
        if (this._maximumRecordTime === -1 || endTime > this._maximumRecordTime)
            this._maximumRecordTime = endTime;
    },

    /**
     * @param {Object} rawRecord
     */
    recordOffsetInSeconds: function(rawRecord)
    {
        return WebInspector.TimelineModel.startTimeInSeconds(rawRecord) - this._minimumRecordTime;
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 * @implements {WebInspector.OutputStream}
 * @param {!WebInspector.TimelineModel} model
 * @param {!{cancel: function()}} reader
 * @param {!WebInspector.Progress} progress
 */
WebInspector.TimelineModelLoader = function(model, reader, progress)
{
    this._model = model;
    this._reader = reader;
    this._progress = progress;
    this._buffer = "";
    this._firstChunk = true;
}

WebInspector.TimelineModelLoader.prototype = {
    /**
     * @param {string} chunk
     */
    write: function(chunk)
    {
        var data = this._buffer + chunk;
        var lastIndex = 0;
        var index;
        do {
            index = lastIndex;
            lastIndex = WebInspector.findBalancedCurlyBrackets(data, index);
        } while (lastIndex !== -1)

        var json = data.slice(0, index) + "]";
        this._buffer = data.slice(index);

        if (!index)
            return;

        // Prepending "0" to turn string into valid JSON.
        if (!this._firstChunk)
            json = "[0" + json;

        var items;
        try {
            items = /** @type {Array} */ (JSON.parse(json));
        } catch (e) {
            WebInspector.showErrorMessage("Malformed timeline data.");
            this._model.reset();
            this._reader.cancel();
            this._progress.done();
            return;
        }

        if (this._firstChunk) {
            this._version = items[0];
            this._firstChunk = false;
            this._model.reset();
        }

        // Skip 0-th element - it is either version or 0.
        for (var i = 1, size = items.length; i < size; ++i)
            this._model._addRecord(items[i]);
    },

    close: function() { }
}

/**
 * @constructor
 * @implements {WebInspector.OutputStreamDelegate}
 * @param {!WebInspector.TimelineModel} model
 * @param {!WebInspector.Progress} progress
 */
WebInspector.TimelineModelLoadFromFileDelegate = function(model, progress)
{
    this._model = model;
    this._progress = progress;
}

WebInspector.TimelineModelLoadFromFileDelegate.prototype = {
    onTransferStarted: function()
    {
        this._progress.setTitle(WebInspector.UIString("Loading\u2026"));
    },

    /**
     * @param {WebInspector.ChunkedReader} reader
     */
    onChunkTransferred: function(reader)
    {
        if (this._progress.isCanceled()) {
            reader.cancel();
            this._progress.done();
            this._model.reset();
            return;
        }

        var totalSize = reader.fileSize();
        if (totalSize) {
            this._progress.setTotalWork(totalSize);
            this._progress.setWorked(reader.loadedSize());
        }
    },

    onTransferFinished: function()
    {
        this._progress.done();
    },

    /**
     * @param {WebInspector.ChunkedReader} reader
     */
    onError: function(reader, event)
    {
        this._progress.done();
        this._model.reset();
        switch (event.target.error.code) {
        case FileError.NOT_FOUND_ERR:
            WebInspector.showErrorMessage(WebInspector.UIString("File \"%s\" not found.", reader.fileName()));
            break;
        case FileError.NOT_READABLE_ERR:
            WebInspector.showErrorMessage(WebInspector.UIString("File \"%s\" is not readable", reader.fileName()));
            break;
        case FileError.ABORT_ERR:
            break;
        default:
            WebInspector.showErrorMessage(WebInspector.UIString("An error occurred while reading the file \"%s\"", reader.fileName()));
        }
    }
}

/**
 * @constructor
 */
WebInspector.TimelineSaver = function(stream)
{
    this._stream = stream;
}

WebInspector.TimelineSaver.prototype = {
    /**
     * @param {Array} records
     * @param {string} version
     */
    save: function(records, version)
    {
        this._records = records;
        this._recordIndex = 0;
        this._prologue = "[" + JSON.stringify(version);

        this._writeNextChunk(this._stream);
    },

    _writeNextChunk: function(stream)
    {
        const separator = ",\n";
        var data = [];
        var length = 0;

        if (this._prologue) {
            data.push(this._prologue);
            length += this._prologue.length;
            delete this._prologue;
        } else {
            if (this._recordIndex === this._records.length) {
                stream.close();
                return;
            }
            data.push("");
        }
        while (this._recordIndex < this._records.length) {
            var item = JSON.stringify(this._records[this._recordIndex]);
            var itemLength = item.length + separator.length;
            if (length + itemLength > WebInspector.TimelineModel.TransferChunkLengthBytes)
                break;
            length += itemLength;
            data.push(item);
            ++this._recordIndex;
        }
        if (this._recordIndex === this._records.length)
            data.push(data.pop() + "]");
        stream.write(data.join(separator), this._writeNextChunk.bind(this));
    }
}
