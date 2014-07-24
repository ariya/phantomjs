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

var InspectorFrontendAPI = {
    _pendingCommands: [],

    isDebuggingEnabled: function()
    {
        return WebInspector.debuggerModel.debuggerEnabled();
    },

    setDebuggingEnabled: function(enabled)
    {
        if (enabled) {
            WebInspector.debuggerModel.enableDebugger();
            WebInspector.showPanel("scripts");
        } else
            WebInspector.debuggerModel.disableDebugger();
    },

    isTimelineProfilingEnabled: function()
    {
        return WebInspector.panels.timeline && WebInspector.panels.timeline.timelineProfilingEnabled;
    },

    setTimelineProfilingEnabled: function(enabled)
    {
        WebInspector.showPanel("timeline").setTimelineProfilingEnabled(enabled);
    },

    isProfilingJavaScript: function()
    {
        return WebInspector.panels.profiles && WebInspector.CPUProfileType.instance && WebInspector.CPUProfileType.instance.isRecordingProfile();
    },

    startProfilingJavaScript: function()
    {
        WebInspector.showPanel("profiles").enableProfiler();
        if (WebInspector.CPUProfileType.instance)
            WebInspector.CPUProfileType.instance.startRecordingProfile();
    },

    stopProfilingJavaScript: function()
    {
        WebInspector.showPanel("profiles");
        if (WebInspector.CPUProfileType.instance)
            WebInspector.CPUProfileType.instance.stopRecordingProfile();
    },

    setAttachedWindow: function(side)
    {
      
    },

    setDockSide: function(side)
    {
        if (WebInspector.dockController)
            WebInspector.dockController.setDockSide(side);
    },

    showConsole: function()
    {
        WebInspector.showPanel("console");
    },

    showMainResourceForFrame: function(frameId)
    {
        // FIXME: Implement this to show the source code for the main resource of a given frame.
    },

    showResources: function()
    {
        WebInspector.showPanel("resources");
    },

    setDockingUnavailable: function(unavailable)
    {
        WebInspector.setDockingUnavailable(unavailable);
    },

    enterInspectElementMode: function()
    {
        if (WebInspector.inspectElementModeController)
            WebInspector.inspectElementModeController.toggleSearch();
    },

    fileSystemsLoaded: function(fileSystems)
    {
        WebInspector.isolatedFileSystemDispatcher.fileSystemsLoaded(fileSystems);
    },

    fileSystemRemoved: function(fileSystemPath)
    {
        WebInspector.isolatedFileSystemDispatcher.fileSystemRemoved(fileSystemPath);
    },

    fileSystemAdded: function(errorMessage, fileSystem)
    {
        WebInspector.isolatedFileSystemDispatcher.fileSystemAdded(errorMessage, fileSystem);
    },

    savedURL: function(url)
    {
        WebInspector.fileManager.savedURL(url);
    },

    appendedToURL: function(url)
    {
        WebInspector.fileManager.appendedToURL(url);
    },

    setToolbarColors: function(backgroundColor, color)
    {
        WebInspector.setToolbarColors(backgroundColor, color);
    },

    evaluateForTest: function(callId, script)
    {
        WebInspector.evaluateForTestInFrontend(callId, script);
    },

    dispatch: function(signature)
    {
        if (InspectorFrontendAPI._isLoaded) {
            var methodName = signature.shift();
            return InspectorFrontendAPI[methodName].apply(InspectorFrontendAPI, signature);
        }
        InspectorFrontendAPI._pendingCommands.push(signature);
    },

    dispatchQueryParameters: function()
    {
        if ("dispatch" in WebInspector.queryParamsObject)
            InspectorFrontendAPI.dispatch(JSON.parse(window.decodeURI(WebInspector.queryParamsObject["dispatch"])));
    },

    /**
     * @param {string} url
     */
    loadTimelineFromURL: function(url) 
    {
        /** @type {WebInspector.TimelinePanel} */ (WebInspector.showPanel("timeline")).loadFromURL(url);
    },

    loadCompleted: function()
    {
        InspectorFrontendAPI._isLoaded = true;
        for (var i = 0; i < InspectorFrontendAPI._pendingCommands.length; ++i)
            InspectorFrontendAPI.dispatch(InspectorFrontendAPI._pendingCommands[i]);
        InspectorFrontendAPI._pendingCommands = [];
        if (window.opener)
            window.opener.postMessage(["loadCompleted"], "*");
    },

    contextMenuItemSelected: function(id)
    {
        WebInspector.contextMenuItemSelected(id);
    },

    contextMenuCleared: function()
    {
        WebInspector.contextMenuCleared();
    },

    dispatchMessageAsync: function(messageObject)
    {
        WebInspector.dispatch(messageObject);
    },

    dispatchMessage: function(messageObject)
    {
        InspectorBackend.dispatch(messageObject);
    }
}

if (window.opener && window.dispatchStandaloneTestRunnerMessages) {
    function onMessageFromOpener(event)
    {
        if (event.source === window.opener)
            InspectorFrontendAPI.dispatch(event.data);
    }
    window.addEventListener("message", onMessageFromOpener, true);
}
