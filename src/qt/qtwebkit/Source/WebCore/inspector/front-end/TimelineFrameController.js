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
 * @param {WebInspector.TimelineModel} model
 * @param {WebInspector.TimelineOverviewPane} overviewPane
 * @param {WebInspector.TimelinePresentationModel} presentationModel
 */
WebInspector.TimelineFrameController = function(model, overviewPane, presentationModel)
{
    this._lastFrame = null;
    this._model = model;
    this._overviewPane = overviewPane;
    this._presentationModel = presentationModel;
    this._model.addEventListener(WebInspector.TimelineModel.Events.RecordAdded, this._onRecordAdded, this);
    this._model.addEventListener(WebInspector.TimelineModel.Events.RecordsCleared, this._onRecordsCleared, this);

    var records = model.records;
    for (var i = 0; i < records.length; ++i)
        this._addRecord(records[i]);
}

WebInspector.TimelineFrameController.prototype = {
    _onRecordAdded: function(event)
    {
        this._addRecord(event.data);
    },

    _onRecordsCleared: function()
    {
        this._lastFrame = null;
    },

    _addRecord: function(record)
    {
        var records;
        if (record.isBackground)
            return;
        if (record.type === WebInspector.TimelineModel.RecordType.Program)
            records = record["children"] || [];
        else
            records = [record];
        records.forEach(this._innerAddRecord, this);
    },

    _innerAddRecord: function(record)
    {
        if (record.type === WebInspector.TimelineModel.RecordType.BeginFrame && this._lastFrame)
            this._flushFrame(record);
        else {
            if (!this._lastFrame)
                this._lastFrame = this._createFrame(record);
            if (!record.thread)
                WebInspector.TimelineModel.aggregateTimeForRecord(this._lastFrame.timeByCategory, record);
            this._lastFrame.cpuTime += WebInspector.TimelineModel.durationInSeconds(record);
        }
    },

    _flushFrame: function(record)
    {
        this._lastFrame.endTime = WebInspector.TimelineModel.startTimeInSeconds(record);
        this._lastFrame.duration = this._lastFrame.endTime - this._lastFrame.startTime;
        this._overviewPane.addFrame(this._lastFrame);
        this._presentationModel.addFrame(this._lastFrame);
        this._lastFrame = this._createFrame(record);
    },

    _createFrame: function(record)
    {
        var frame = new WebInspector.TimelineFrame();
        frame.startTime = WebInspector.TimelineModel.startTimeInSeconds(record);
        frame.startTimeOffset = this._model.recordOffsetInSeconds(record);
        return frame;
    },

    dispose: function()
    {
        this._model.removeEventListener(WebInspector.TimelineModel.Events.RecordAdded, this._onRecordAdded, this);
        this._model.removeEventListener(WebInspector.TimelineModel.Events.RecordsCleared, this._onRecordsCleared, this);
    }
}

/**
 * @constructor
 * @param {Array.<WebInspector.TimelineFrame>} frames
 */
WebInspector.FrameStatistics = function(frames)
{
    this.frameCount = frames.length;
    this.minDuration = Infinity;
    this.maxDuration = 0;
    this.timeByCategory = {};
    this.startOffset = frames[0].startTimeOffset;
    var lastFrame = frames[this.frameCount - 1];
    this.endOffset = lastFrame.startTimeOffset + lastFrame.duration;

    var totalDuration = 0;
    var sumOfSquares = 0;
    for (var i = 0; i < this.frameCount; ++i) {
        var duration = frames[i].duration;
        totalDuration += duration;
        sumOfSquares += duration * duration;
        this.minDuration = Math.min(this.minDuration, duration);
        this.maxDuration = Math.max(this.maxDuration, duration);
        WebInspector.TimelineModel.aggregateTimeByCategory(this.timeByCategory, frames[i].timeByCategory);
    }
    this.average = totalDuration / this.frameCount;
    var variance = sumOfSquares / this.frameCount - this.average * this.average;
    this.stddev = Math.sqrt(variance);
}

/**
 * @constructor
 */
WebInspector.TimelineFrame = function()
{
    this.timeByCategory = {};
    this.cpuTime = 0;
}
