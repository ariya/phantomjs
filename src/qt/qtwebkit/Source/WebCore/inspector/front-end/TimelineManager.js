/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
WebInspector.TimelineManager = function()
{
    WebInspector.Object.call(this);
    this._dispatcher = new WebInspector.TimelineDispatcher(this);
    this._enablementCount = 0;
}

WebInspector.TimelineManager.EventTypes = {
    TimelineStarted: "TimelineStarted",
    TimelineStopped: "TimelineStopped",
    TimelineEventRecorded: "TimelineEventRecorded"
}

WebInspector.TimelineManager.prototype = {
    /**
     * @param {number=} maxCallStackDepth
     * @param {boolean=} includeDomCounters
     * @param {boolean=} includeNativeMemoryStatistics
     */
    start: function(maxCallStackDepth, includeDomCounters, includeNativeMemoryStatistics)
    {
        this._enablementCount++;
        if (this._enablementCount === 1)
            TimelineAgent.start(maxCallStackDepth, includeDomCounters, includeNativeMemoryStatistics, this._started.bind(this));
    },

    stop: function()
    {
        if (!this._enablementCount) {
            console.error("WebInspector.TimelineManager start/stop calls are unbalanced");
            return;
        }
        this._enablementCount--;
        if (!this._enablementCount)
            TimelineAgent.stop(this._stopped.bind(this));
    },

    _started: function()
    {
        this.dispatchEventToListeners(WebInspector.TimelineManager.EventTypes.TimelineStarted);
    },

    _stopped: function()
    {
        this.dispatchEventToListeners(WebInspector.TimelineManager.EventTypes.TimelineStopped);
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 * @implements {TimelineAgent.Dispatcher}
 */
WebInspector.TimelineDispatcher = function(manager)
{
    this._manager = manager;
    InspectorBackend.registerTimelineDispatcher(this);
}

WebInspector.TimelineDispatcher.prototype = {
    eventRecorded: function(record)
    {
        this._manager.dispatchEventToListeners(WebInspector.TimelineManager.EventTypes.TimelineEventRecorded, record);
    }
}

/**
 * @type {WebInspector.TimelineManager}
 */
WebInspector.timelineManager;
