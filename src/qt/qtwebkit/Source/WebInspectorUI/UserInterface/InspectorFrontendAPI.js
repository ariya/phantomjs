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

InspectorFrontendAPI = {
    _loaded: false,

    _pendingCommands: [],

    savedURL: function(url)
    {
        // FIXME: Not implemented.
    },

    appendedToURL: function(url)
    {
        // FIXME: Not implemented.
    },

    isDebuggingEnabled: function()
    {
        // FIXME: Not implemented.
        return false;
    },

    setDebuggingEnabled: function(enabled)
    {
        // FIXME: Not implemented.
    },

    isTimelineProfilingEnabled: function()
    {
        return WebInspector.timelineManager.recording;
    },

    setTimelineProfilingEnabled: function(enabled)
    {
        if (enabled) {
            WebInspector.navigationSidebar.selectedSidebarPanel = WebInspector.instrumentSidebarPanel;
            WebInspector.instrumentSidebarPanel.showTimeline();
            WebInspector.timelineManager.startRecording();
        } else {
            WebInspector.timelineManager.stopRecording();
        }
    },

    isProfilingJavaScript: function()
    {
        return WebInspector.profileManager.isProfilingJavaScript();
    },

    startProfilingJavaScript: function()
    {
        WebInspector.profileManager.startProfilingJavaScript();
    },

    stopProfilingJavaScript: function()
    {
        WebInspector.instrumentSidebarPanel.show();
        WebInspector.profileManager.stopProfilingJavaScript();
    },

    setDockSide: function(side)
    {
        WebInspector.updateDockedState(side);
    },

    showConsole: function()
    {
        WebInspector.showConsoleView();

        WebInspector.quickConsole.prompt.focus();

        // If the page is still loading, focus the quick console again after tabindex autofocus.
        if (document.readyState !== "complete")
            document.addEventListener("readystatechange", this);
    },

    handleEvent: function(event)
    {
        console.assert(event.type === "readystatechange");

        if (document.readyState === "complete") {
            WebInspector.quickConsole.prompt.focus();
            document.removeEventListener("readystatechange", this);
        }
    },

    showResources: function()
    {
        WebInspector.ignoreLastContentCookie = true;
        WebInspector.navigationSidebar.selectedSidebarPanel = WebInspector.resourceSidebarPanel;
        WebInspector.navigationSidebar.collapsed = false;
    },

    showMainResourceForFrame: function(frameIdentifier)
    {
        WebInspector.ignoreLastContentCookie = true;
        WebInspector.navigationSidebar.selectedSidebarPanel = WebInspector.resourceSidebarPanel;
        WebInspector.resourceSidebarPanel.showSourceCodeForFrame(frameIdentifier, true);
    },

    setDockingUnavailable: function(unavailable)
    {
        // Not used.
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
        WebInspector.dispatchMessageFromBackend(messageObject);
    },

    dispatchMessage: function(messageObject)
    {
        InspectorBackend.dispatch(messageObject);
    },

    dispatch: function(signature)
    {
        if (!InspectorFrontendAPI._loaded) {
            InspectorFrontendAPI._pendingCommands.push(signature);
            return null;
        }

        var methodName = signature.shift();
        return InspectorFrontendAPI[methodName].apply(InspectorFrontendAPI, signature);
    },

    loadCompleted: function()
    {
        InspectorFrontendAPI._loaded = true;

        for (var i = 0; i < InspectorFrontendAPI._pendingCommands.length; ++i)
            InspectorFrontendAPI.dispatch(InspectorFrontendAPI._pendingCommands[i]);

        InspectorFrontendAPI._pendingCommands = [];
    }
};
