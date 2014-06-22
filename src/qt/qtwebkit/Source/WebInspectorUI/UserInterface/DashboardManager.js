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

WebInspector.DashboardManager = function() {
    WebInspector.Object.call(this);

    this.toolbarItem = new WebInspector.NavigationItem("dashboard", "group", WebInspector.UIString("Activity Viewer"));
    this._view = new WebInspector.DashboardView(this.toolbarItem.element);

    this._waitingForFirstMainResourceToStartTrackingSize = true;

    // Necessary event required to track page load time and resource sizes.
    WebInspector.Frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainResourceDidChange, this);
    WebInspector.timelineManager.addEventListener(WebInspector.TimelineManager.Event.RecordingStopped, this._recordingStopped, this);

    // Necessary events required to track load of resources.
    WebInspector.Frame.addEventListener(WebInspector.Frame.Event.ResourceWasAdded, this._resourceWasAdded, this);
    WebInspector.Frame.addEventListener(WebInspector.Frame.Event.AllResourcesRemoved, this._resourcesWereCleared, this);
    WebInspector.frameResourceManager.addEventListener(WebInspector.FrameResourceManager.Event.FrameWasAdded, this._frameWasAdded, this);

    // Necessary events required to track console messages.
    var logManager = WebInspector.logManager;
    logManager.addEventListener(WebInspector.LogManager.Event.Cleared, this._consoleWasCleared, this);
    logManager.addEventListener(WebInspector.LogManager.Event.ActiveLogCleared, this._consoleWasCleared, this);
    logManager.addEventListener(WebInspector.LogManager.Event.MessageAdded, this._consoleMessageAdded, this);
    logManager.addEventListener(WebInspector.LogManager.Event.PreviousMessageRepeatCountUpdated, this._consoleMessageWasRepeated, this);
};

WebInspector.DashboardManager.prototype = {
    constructor: WebInspector.DashboardManager,

    _mainResourceDidChange: function(event)
    {
        console.assert(event.target instanceof WebInspector.Frame);

        if (!event.target.isMainFrame())
            return;

        // Only update the time if we are recording the timeline.
        if (!WebInspector.timelineManager.recording) {
            this._view.time = 0;
            return;
        }

        // We should only track resource sizes on fresh loads.
        if (this._waitingForFirstMainResourceToStartTrackingSize) {
            delete this._waitingForFirstMainResourceToStartTrackingSize;
            WebInspector.Resource.addEventListener(WebInspector.Resource.Event.SizeDidChange, this._resourceSizeDidChange, this);
        }

        this._startUpdatingTime();
    },

    _recordingStopped: function(event)
    {
        // If recording stops, we should stop the timer if it hasn't stopped already.
        this._stopUpdatingTime();
    },

    _resourceWasAdded: function(event)
    {
        ++this._view.resourcesCount;
    },

    _resourcesWereCleared: function(event)
    {
        this._view.resourcesCount = 1;
        this._view.resourcesSize = WebInspector.frameResourceManager.mainFrame.mainResource.size || 0;
    },

    _frameWasAdded: function(event)
    {
        ++this._view.resourcesCount;
    },

    _resourceSizeDidChange: function(event)
    {
        this._view.resourcesSize += event.target.size - event.data.previousSize;
    },

    _startUpdatingTime: function()
    {
        this._stopUpdatingTime();

        this._view.time = 0;

        this._timelineBaseTime = Date.now();
        this._timeIntervalDelay = 50;
        this._timeIntervalIdentifier = setInterval(this._updateTime.bind(this), this._timeIntervalDelay);
    },

    _stopUpdatingTime: function()
    {
        if (!this._timeIntervalIdentifier)
            return;

        clearInterval(this._timeIntervalIdentifier);
        delete this._timeIntervalIdentifier;
    },

    _updateTime: function()
    {
        var duration = Date.now() - this._timelineBaseTime;

        var timeIntervalDelay = this._timeIntervalDelay;
        if (duration >= 1000) // 1 second
            timeIntervalDelay = 100;
        else if (duration >= 60000) // 60 seconds
            timeIntervalDelay = 1000;
        else if (duration >= 3600000) // 1 minute
            timeIntervalDelay = 10000;

        if (timeIntervalDelay !== this._timeIntervalDelay) {
            this._timeIntervalDelay = timeIntervalDelay;

            clearInterval(this._timeIntervalIdentifier);
            this._timeIntervalIdentifier = setInterval(this._updateTime.bind(this), this._timeIntervalDelay);
        }

        var timelineManager = WebInspector.timelineManager;
        var mainFrameLoadEventTime = timelineManager.mainFrameLoadEventTime;

        if (isNaN(mainFrameLoadEventTime)) {
            this._view.time = (Date.now() - this._timelineBaseTime) / 1000;
            return;
        }

        var networkRecords = timelineManager.recordsWithType(WebInspector.TimelineRecord.Type.Network);
        this._view.time = mainFrameLoadEventTime - networkRecords[0].startTime;

        this._stopUpdatingTime();
    },

    _consoleMessageAdded: function(event)
    {
        var message = event.data.message;
        this._lastConsoleMessageType = message.level;
        this._incrementConsoleMessageType(message.level, message.totalRepeatCount);
    },

    _consoleMessageWasRepeated: function(event)
    {
        this._incrementConsoleMessageType(this._lastConsoleMessageType, 1);
    },

    _incrementConsoleMessageType: function(type, increment)
    {
        switch (type) {
        case "log":
            this._view.logs += increment;
            break;
        case "warning":
            this._view.issues += increment;
            break;
        case "error":
            this._view.errors += increment;
            break;
        }
    },

    _consoleWasCleared: function(event)
    {
        this._view.logs = 0;
        this._view.issues = 0;
        this._view.errors = 0;
    }
};

WebInspector.DashboardManager.prototype.__proto__ = WebInspector.Object.prototype;
