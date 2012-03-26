/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.ScriptsPanel = function()
{
    WebInspector.Panel.call(this, "scripts");

    this._presentationModel = new WebInspector.DebuggerPresentationModel();

    this.topStatusBar = document.createElement("div");
    this.topStatusBar.className = "status-bar";
    this.topStatusBar.id = "scripts-status-bar";
    this.element.appendChild(this.topStatusBar);

    this.backButton = document.createElement("button");
    this.backButton.className = "status-bar-item";
    this.backButton.id = "scripts-back";
    this.backButton.title = WebInspector.UIString("Show the previous script resource.");
    this.backButton.disabled = true;
    this.backButton.appendChild(document.createElement("img"));
    this.backButton.addEventListener("click", this._goBack.bind(this), false);
    this.topStatusBar.appendChild(this.backButton);

    this.forwardButton = document.createElement("button");
    this.forwardButton.className = "status-bar-item";
    this.forwardButton.id = "scripts-forward";
    this.forwardButton.title = WebInspector.UIString("Show the next script resource.");
    this.forwardButton.disabled = true;
    this.forwardButton.appendChild(document.createElement("img"));
    this.forwardButton.addEventListener("click", this._goForward.bind(this), false);
    this.topStatusBar.appendChild(this.forwardButton);

    this._filesSelectElement = document.createElement("select");
    this._filesSelectElement.className = "status-bar-item";
    this._filesSelectElement.id = "scripts-files";
    this._filesSelectElement.addEventListener("change", this._filesSelectChanged.bind(this), false);
    this.topStatusBar.appendChild(this._filesSelectElement);

    this.functionsSelectElement = document.createElement("select");
    this.functionsSelectElement.className = "status-bar-item";
    this.functionsSelectElement.id = "scripts-functions";

    // FIXME: append the functions select element to the top status bar when it is implemented.
    // this.topStatusBar.appendChild(this.functionsSelectElement);

    this.sidebarButtonsElement = document.createElement("div");
    this.sidebarButtonsElement.id = "scripts-sidebar-buttons";
    this.topStatusBar.appendChild(this.sidebarButtonsElement);

    this.pauseButton = document.createElement("button");
    this.pauseButton.className = "status-bar-item";
    this.pauseButton.id = "scripts-pause";
    this.pauseButton.title = WebInspector.UIString("Pause script execution.");
    this.pauseButton.disabled = true;
    this.pauseButton.appendChild(document.createElement("img"));
    this.pauseButton.addEventListener("click", this._togglePause.bind(this), false);
    this.sidebarButtonsElement.appendChild(this.pauseButton);

    this.stepOverButton = document.createElement("button");
    this.stepOverButton.className = "status-bar-item";
    this.stepOverButton.id = "scripts-step-over";
    this.stepOverButton.title = WebInspector.UIString("Step over next function call.");
    this.stepOverButton.disabled = true;
    this.stepOverButton.addEventListener("click", this._stepOverClicked.bind(this), false);
    this.stepOverButton.appendChild(document.createElement("img"));
    this.sidebarButtonsElement.appendChild(this.stepOverButton);

    this.stepIntoButton = document.createElement("button");
    this.stepIntoButton.className = "status-bar-item";
    this.stepIntoButton.id = "scripts-step-into";
    this.stepIntoButton.title = WebInspector.UIString("Step into next function call.");
    this.stepIntoButton.disabled = true;
    this.stepIntoButton.addEventListener("click", this._stepIntoClicked.bind(this), false);
    this.stepIntoButton.appendChild(document.createElement("img"));
    this.sidebarButtonsElement.appendChild(this.stepIntoButton);

    this.stepOutButton = document.createElement("button");
    this.stepOutButton.className = "status-bar-item";
    this.stepOutButton.id = "scripts-step-out";
    this.stepOutButton.title = WebInspector.UIString("Step out of current function.");
    this.stepOutButton.disabled = true;
    this.stepOutButton.addEventListener("click", this._stepOutClicked.bind(this), false);
    this.stepOutButton.appendChild(document.createElement("img"));
    this.sidebarButtonsElement.appendChild(this.stepOutButton);

    this.toggleBreakpointsButton = new WebInspector.StatusBarButton(WebInspector.UIString("Deactivate all breakpoints."), "toggle-breakpoints");
    this.toggleBreakpointsButton.toggled = true;
    this.toggleBreakpointsButton.addEventListener("click", this.toggleBreakpointsClicked.bind(this), false);
    this.sidebarButtonsElement.appendChild(this.toggleBreakpointsButton.element);

    this.debuggerStatusElement = document.createElement("div");
    this.debuggerStatusElement.id = "scripts-debugger-status";
    this.sidebarButtonsElement.appendChild(this.debuggerStatusElement);

    this.viewsContainerElement = document.createElement("div");
    this.viewsContainerElement.id = "script-resource-views";

    this.sidebarElement = document.createElement("div");
    this.sidebarElement.id = "scripts-sidebar";

    this.sidebarResizeElement = document.createElement("div");
    this.sidebarResizeElement.className = "sidebar-resizer-vertical";
    this.sidebarResizeElement.addEventListener("mousedown", this._startSidebarResizeDrag.bind(this), false);

    this.sidebarResizeWidgetElement = document.createElement("div");
    this.sidebarResizeWidgetElement.id = "scripts-sidebar-resizer-widget";
    this.sidebarResizeWidgetElement.addEventListener("mousedown", this._startSidebarResizeDrag.bind(this), false);
    this.topStatusBar.appendChild(this.sidebarResizeWidgetElement);

    this.sidebarPanes = {};
    this.sidebarPanes.watchExpressions = new WebInspector.WatchExpressionsSidebarPane();
    this.sidebarPanes.callstack = new WebInspector.CallStackSidebarPane(this._presentationModel);
    this.sidebarPanes.scopechain = new WebInspector.ScopeChainSidebarPane();
    this.sidebarPanes.jsBreakpoints = new WebInspector.JavaScriptBreakpointsSidebarPane(this._presentationModel, this._showSourceLine.bind(this));
    if (Preferences.nativeInstrumentationEnabled) {
        this.sidebarPanes.domBreakpoints = WebInspector.domBreakpointsSidebarPane;
        this.sidebarPanes.xhrBreakpoints = new WebInspector.XHRBreakpointsSidebarPane();
        this.sidebarPanes.eventListenerBreakpoints = new WebInspector.EventListenerBreakpointsSidebarPane();
    }

    this.sidebarPanes.workers = new WebInspector.WorkersSidebarPane();

    for (var pane in this.sidebarPanes)
        this.sidebarElement.appendChild(this.sidebarPanes[pane].element);

    this.sidebarPanes.callstack.expanded = true;

    this.sidebarPanes.scopechain.expanded = true;
    this.sidebarPanes.jsBreakpoints.expanded = true;

    var panelEnablerHeading = WebInspector.UIString("You need to enable debugging before you can use the Scripts panel.");
    var panelEnablerDisclaimer = WebInspector.UIString("Enabling debugging will make scripts run slower.");
    var panelEnablerButton = WebInspector.UIString("Enable Debugging");

    this.panelEnablerView = new WebInspector.PanelEnablerView("scripts", panelEnablerHeading, panelEnablerDisclaimer, panelEnablerButton);
    this.panelEnablerView.addEventListener("enable clicked", this._enableDebugging, this);

    this.element.appendChild(this.panelEnablerView.element);
    this.element.appendChild(this.viewsContainerElement);
    this.element.appendChild(this.sidebarElement);
    this.element.appendChild(this.sidebarResizeElement);

    this.enableToggleButton = new WebInspector.StatusBarButton("", "enable-toggle-status-bar-item");
    this.enableToggleButton.addEventListener("click", this._toggleDebugging.bind(this), false);
    if (Preferences.debuggerAlwaysEnabled)
        this.enableToggleButton.element.addStyleClass("hidden");

    this._pauseOnExceptionButton = new WebInspector.StatusBarButton("", "scripts-pause-on-exceptions-status-bar-item", 3);
    this._pauseOnExceptionButton.addEventListener("click", this._togglePauseOnExceptions.bind(this), false);

    this._toggleFormatSourceFilesButton = new WebInspector.StatusBarButton(WebInspector.UIString("Pretty print"), "scripts-toggle-pretty-print-status-bar-item");
    this._toggleFormatSourceFilesButton.toggled = false;
    this._toggleFormatSourceFilesButton.addEventListener("click", this._toggleFormatSourceFiles.bind(this), false);

    this._registerShortcuts();

    this._debuggerEnabled = Preferences.debuggerAlwaysEnabled;

    this.reset();

    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.DebuggerWasEnabled, this._debuggerWasEnabled, this);
    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.DebuggerWasDisabled, this._debuggerWasDisabled, this);

    this._presentationModel.addEventListener(WebInspector.DebuggerPresentationModel.Events.SourceFileAdded, this._sourceFileAdded, this)
    this._presentationModel.addEventListener(WebInspector.DebuggerPresentationModel.Events.SourceFileChanged, this._sourceFileChanged, this);
    this._presentationModel.addEventListener(WebInspector.DebuggerPresentationModel.Events.ConsoleMessageAdded, this._consoleMessageAdded, this);
    this._presentationModel.addEventListener(WebInspector.DebuggerPresentationModel.Events.BreakpointAdded, this._breakpointAdded, this);
    this._presentationModel.addEventListener(WebInspector.DebuggerPresentationModel.Events.BreakpointRemoved, this._breakpointRemoved, this);
    this._presentationModel.addEventListener(WebInspector.DebuggerPresentationModel.Events.DebuggerPaused, this._debuggerPaused, this);
    this._presentationModel.addEventListener(WebInspector.DebuggerPresentationModel.Events.DebuggerResumed, this._debuggerResumed, this);
    this._presentationModel.addEventListener(WebInspector.DebuggerPresentationModel.Events.CallFrameSelected, this._callFrameSelected, this);

    var enableDebugger = Preferences.debuggerAlwaysEnabled || WebInspector.settings.debuggerEnabled;
    if (enableDebugger || InspectorFrontendHost.loadSessionSetting("debugger-enabled") === "true")
        WebInspector.debuggerModel.enableDebugger();
}

// Keep these in sync with WebCore::ScriptDebugServer
WebInspector.ScriptsPanel.PauseOnExceptionsState = {
    DontPauseOnExceptions : "none",
    PauseOnAllExceptions : "all",
    PauseOnUncaughtExceptions: "uncaught"
};

WebInspector.ScriptsPanel.BrowserBreakpointTypes = {
    DOM: "DOM",
    EventListener: "EventListener",
    XHR: "XHR"
}

WebInspector.ScriptsPanel.prototype = {
    get toolbarItemLabel()
    {
        return WebInspector.UIString("Scripts");
    },

    get statusBarItems()
    {
        return [this.enableToggleButton.element, this._pauseOnExceptionButton.element, this._toggleFormatSourceFilesButton.element];
    },

    get defaultFocusedElement()
    {
        return this._filesSelectElement;
    },

    get paused()
    {
        return this._paused;
    },

    show: function()
    {
        WebInspector.Panel.prototype.show.call(this);
        this.sidebarResizeElement.style.right = (this.sidebarElement.offsetWidth - 3) + "px";
        if (Preferences.nativeInstrumentationEnabled)
            this.sidebarElement.insertBefore(this.sidebarPanes.domBreakpoints.element, this.sidebarPanes.xhrBreakpoints.element);

        if (this.visibleView)
            this.visibleView.show(this.viewsContainerElement);
    },

    hide: function()
    {
        if (this.visibleView)
            this.visibleView.hide();
        WebInspector.Panel.prototype.hide.call(this);
    },

    get breakpointsActivated()
    {
        return this.toggleBreakpointsButton.toggled;
    },

    _sourceFileAdded: function(event)
    {
        var sourceFile = event.data;

        if (!sourceFile.url) {
            // Anonymous sources are shown only when stepping.
            return;
        }

        this._addOptionToFilesSelect(sourceFile.id);

        var lastViewedURL = WebInspector.settings.lastViewedScriptFile;
        if (this._filesSelectElement.length === 1) {
            // Option we just added is the only option in files select.
            // We have to show corresponding source frame immediately.
            this._showSourceFrameAndAddToHistory(sourceFile.id);
            // Restore original value of lastViewedScriptFile because
            // source frame was shown as a result of initial load.
            WebInspector.settings.lastViewedScriptFile = lastViewedURL;
        } else if (sourceFile.url === lastViewedURL)
            this._showSourceFrameAndAddToHistory(sourceFile.id);
    },

    _addOptionToFilesSelect: function(sourceFileId)
    {
        var sourceFile = this._presentationModel.sourceFile(sourceFileId);
        var select = this._filesSelectElement;
        var option = document.createElement("option");
        option.text = sourceFile.displayName;
        option.isContentScript = sourceFile.isContentScript;
        if (sourceFile.isContentScript)
            option.addStyleClass("extension-script");
        function optionCompare(a, b)
        {
            if (a === select.contentScriptSection)
                return b.isContentScript ? -1 : 1;
            if (b === select.contentScriptSection)
                return a.isContentScript ? 1 : -1;

            if (a.isContentScript && !b.isContentScript)
                return 1;
            if (!a.isContentScript && b.isContentScript)
                return -1;

            if (a.text === b.text)
                return 0;
            return a.text < b.text ? -1 : 1;
        }

        var insertionIndex = insertionIndexForObjectInListSortedByFunction(option, select.childNodes, optionCompare);
        select.insertBefore(option, insertionIndex < 0 ? null : select.childNodes.item(insertionIndex));

        if (sourceFile.isContentScript && !select.contentScriptSection) {
            var contentScriptSection = document.createElement("option");
            contentScriptSection.text = WebInspector.UIString("Content scripts");
            contentScriptSection.disabled = true;
            select.contentScriptSection = contentScriptSection;

            var insertionIndex = insertionIndexForObjectInListSortedByFunction(contentScriptSection, select.childNodes, optionCompare);
            select.insertBefore(contentScriptSection, insertionIndex < 0 ? null : select.childNodes.item(insertionIndex));
        }
        option._sourceFileId = sourceFileId;
        this._sourceFileIdToFilesSelectOption[sourceFileId] = option;
    },

    setScriptSourceIsBeingEdited: function(sourceFileId, inEditMode)
    {
        var option = this._sourceFileIdToFilesSelectOption[sourceFileId];
        if (!option)
            return;
        if (inEditMode)
            option.text = option.text.replace(/[^*]$/, "$&*");
        else
            option.text = option.text.replace(/[*]$/, "");
    },

    addConsoleMessage: function(message)
    {
        if (message.isErrorOrWarning() && message.message)
            this._presentationModel.addConsoleMessage(message);
    },

    clearConsoleMessages: function()
    {
        this._presentationModel.clearConsoleMessages();
        for (var sourceFileId in this._sourceFileIdToSourceFrame)
            this._sourceFileIdToSourceFrame[sourceFileId].clearMessages();
    },

    _consoleMessageAdded: function(event)
    {
        var message = event.data;

        var sourceFrame = this._sourceFileIdToSourceFrame[message.sourceFileId];
        if (sourceFrame && sourceFrame.loaded)
            sourceFrame.addMessageToSource(message.lineNumber, message.originalMessage);
    },

    _breakpointAdded: function(event)
    {
        var breakpoint = event.data;

        var sourceFrame = this._sourceFileIdToSourceFrame[breakpoint.sourceFileId];
        if (sourceFrame && sourceFrame.loaded)
            sourceFrame.addBreakpoint(breakpoint.lineNumber, breakpoint.resolved, breakpoint.condition, breakpoint.enabled);

        this.sidebarPanes.jsBreakpoints.addBreakpoint(breakpoint);
    },

    _breakpointRemoved: function(event)
    {
        var breakpoint = event.data;

        var sourceFrame = this._sourceFileIdToSourceFrame[breakpoint.sourceFileId];
        if (sourceFrame && sourceFrame.loaded)
            sourceFrame.removeBreakpoint(breakpoint.lineNumber);

        this.sidebarPanes.jsBreakpoints.removeBreakpoint(breakpoint.sourceFileId, breakpoint.lineNumber);
    },

    evaluateInSelectedCallFrame: function(code, objectGroup, includeCommandLineAPI, callback)
    {
        var selectedCallFrame = this._presentationModel.selectedCallFrame;
        selectedCallFrame.evaluate(code, objectGroup, includeCommandLineAPI, callback);
    },

    _debuggerPaused: function(event)
    {
        var callFrames = event.data.callFrames;
        var details = event.data.details;

        this._paused = true;
        this._waitingToPause = false;
        this._stepping = false;

        this._updateDebuggerButtons();

        WebInspector.currentPanel = this;

        this.sidebarPanes.callstack.update(callFrames, details);
        this.sidebarPanes.callstack.selectedCallFrame = this._presentationModel.selectedCallFrame;

        if (details.eventType === WebInspector.DebuggerEventTypes.NativeBreakpoint) {
            if (details.eventData.breakpointType === WebInspector.ScriptsPanel.BrowserBreakpointTypes.DOM) {
                this.sidebarPanes.domBreakpoints.highlightBreakpoint(details.eventData);
                function didCreateBreakpointHitStatusMessage(element)
                {
                    this.sidebarPanes.callstack.setStatus(element);
                }
                this.sidebarPanes.domBreakpoints.createBreakpointHitStatusMessage(details.eventData, didCreateBreakpointHitStatusMessage.bind(this));
            } else if (details.eventData.breakpointType === WebInspector.ScriptsPanel.BrowserBreakpointTypes.EventListener) {
                var eventName = details.eventData.eventName;
                this.sidebarPanes.eventListenerBreakpoints.highlightBreakpoint(details.eventData.eventName);
                var eventNameForUI = WebInspector.EventListenerBreakpointsSidebarPane.eventNameForUI(eventName);
                this.sidebarPanes.callstack.setStatus(WebInspector.UIString("Paused on a \"%s\" Event Listener.", eventNameForUI));
            } else if (details.eventData.breakpointType === WebInspector.ScriptsPanel.BrowserBreakpointTypes.XHR) {
                this.sidebarPanes.xhrBreakpoints.highlightBreakpoint(details.eventData.breakpointURL);
                this.sidebarPanes.callstack.setStatus(WebInspector.UIString("Paused on a XMLHttpRequest."));
            }
        } else {
            function didGetSourceLocation(sourceFileId, lineNumber)
            {
                var exception = WebInspector.debuggerModel.debuggerPausedDetails.exception;
                if (exception) {
                    this.sidebarPanes.callstack.setStatus(WebInspector.UIString("Paused on exception: '%s'.", exception.description));
                    return;
                }

                if (!sourceFileId || !this._presentationModel.findBreakpoint(sourceFileId, lineNumber))
                    return;
                this.sidebarPanes.jsBreakpoints.highlightBreakpoint(sourceFileId, lineNumber);
                this.sidebarPanes.callstack.setStatus(WebInspector.UIString("Paused on a JavaScript breakpoint."));
            }
            callFrames[0].sourceLine(didGetSourceLocation.bind(this));
        }

        window.focus();
        InspectorFrontendHost.bringToFront();
    },

    _debuggerResumed: function()
    {
        this._paused = false;
        this._waitingToPause = false;
        this._stepping = false;

        this._clearInterface();
    },

    _debuggerWasEnabled: function()
    {
        this._setPauseOnExceptions(WebInspector.settings.pauseOnExceptionStateString);

        if (this._debuggerEnabled)
            return;

        InspectorFrontendHost.saveSessionSetting("debugger-enabled", "true");
        this._debuggerEnabled = true;
        this.reset(true);
    },

    _debuggerWasDisabled: function()
    {
        if (!this._debuggerEnabled)
            return;

        InspectorFrontendHost.saveSessionSetting("debugger-enabled", "false");
        this._debuggerEnabled = false;
        this.reset(true);
    },

    reset: function(preserveItems)
    {
        this.visibleView = null;

        delete this.currentQuery;
        this.searchCanceled();

        this._debuggerResumed();

        this._backForwardList = [];
        this._currentBackForwardIndex = -1;
        this._updateBackAndForwardButtons();

        for (var id in this._sourceFileIdToSourceFrame) {
            var sourceFrame = this._sourceFileIdToSourceFrame[id];
            sourceFrame.removeEventListener(WebInspector.SourceFrame.Events.Loaded, this._sourceFrameLoaded, this);
        }

        this._sourceFileIdToSourceFrame = {};
        this._sourceFileIdToFilesSelectOption = {};
        this._filesSelectElement.removeChildren();
        this.functionsSelectElement.removeChildren();
        this.viewsContainerElement.removeChildren();

        this.sidebarPanes.jsBreakpoints.reset();
        this.sidebarPanes.watchExpressions.refreshExpressions();
        if (!preserveItems)
            this.sidebarPanes.workers.reset();
    },

    get visibleView()
    {
        return this._visibleView;
    },

    set visibleView(x)
    {
        if (this._visibleView === x)
            return;

        if (this._visibleView)
            this._visibleView.hide();

        this._visibleView = x;

        if (x)
            x.show(this.viewsContainerElement);
    },

    canShowAnchorLocation: function(anchor)
    {
        return this._debuggerEnabled && this._presentationModel.sourceFileForScriptURL(anchor.href);
    },

    showAnchorLocation: function(anchor)
    {
        function didRequestSourceMapping(mapping)
        {
            var lineNumber = mapping.scriptLocationToSourceLine({lineNumber:anchor.getAttribute("line_number") - 1, columnNumber:0});
            this._showSourceLine(sourceFile.id, lineNumber);
        }
        var sourceFile = this._presentationModel.sourceFileForScriptURL(anchor.href);
        sourceFile.requestSourceMapping(didRequestSourceMapping.bind(this));
    },

    _showSourceLine: function(sourceFileId, lineNumber)
    {
        var sourceFrame = this._showSourceFrameAndAddToHistory(sourceFileId);
        sourceFrame.highlightLine(lineNumber);
    },

    _showSourceFrameAndAddToHistory: function(sourceFileId)
    {
        var sourceFrame = this._showSourceFrame(sourceFileId);

        var oldIndex = this._currentBackForwardIndex;
        if (oldIndex >= 0)
            this._backForwardList.splice(oldIndex + 1, this._backForwardList.length - oldIndex);

        // Check for a previous entry of the same object in _backForwardList.
        // If one is found, remove it.
        var previousEntryIndex = this._backForwardList.indexOf(sourceFileId);
        if (previousEntryIndex !== -1)
            this._backForwardList.splice(previousEntryIndex, 1);

        this._backForwardList.push(sourceFileId);
        this._currentBackForwardIndex = this._backForwardList.length - 1;

        this._updateBackAndForwardButtons();

        return sourceFrame;
    },

    _showSourceFrame: function(sourceFileId)
    {
        var index = this._sourceFileIdToFilesSelectOption[sourceFileId].index;
        this._filesSelectElement.selectedIndex = index;

        var sourceFrame = this._sourceFrameForSourceFileId(sourceFileId);
        this.visibleView = sourceFrame;

        var sourceFile = this._presentationModel.sourceFile(sourceFileId);
        if (sourceFile.url)
            WebInspector.settings.lastViewedScriptFile = sourceFile.url;

        return sourceFrame;
    },

    _sourceFrameForSourceFileId: function(sourceFileId)
    {
        var sourceFrame = this._sourceFileIdToSourceFrame[sourceFileId];
        return sourceFrame || this._createSourceFrame(sourceFileId);
    },

    _createSourceFrame: function(sourceFileId)
    {
        var sourceFile = this._presentationModel.sourceFile(sourceFileId);
        var delegate = new WebInspector.SourceFrameDelegateForScriptsPanel(this._presentationModel, sourceFileId, sourceFile.displayName);
        var sourceFrame = new WebInspector.SourceFrame(delegate, sourceFile.url);
        sourceFrame._sourceFileId = sourceFileId;
        sourceFrame.addEventListener(WebInspector.SourceFrame.Events.Loaded, this._sourceFrameLoaded, this);
        this._sourceFileIdToSourceFrame[sourceFileId] = sourceFrame;
        return sourceFrame;
    },

    _sourceFileChanged: function(event)
    {
        var sourceFileId = event.data.id;

        var oldSourceFrame = this._sourceFileIdToSourceFrame[sourceFileId];
        if (!oldSourceFrame)
            return;
        oldSourceFrame.removeEventListener(WebInspector.SourceFrame.Events.Loaded, this._sourceFrameLoaded, this);
        delete this._sourceFileIdToSourceFrame[sourceFileId];
        if (this.visibleView !== oldSourceFrame)
            return;

        var newSourceFrame = this._createSourceFrame(sourceFileId)
        newSourceFrame.scrollTop = oldSourceFrame.scrollTop;
        this.visibleView = newSourceFrame;
    },

    _sourceFrameLoaded: function(event)
    {
        var sourceFrame = event.target;
        var sourceFileId = sourceFrame._sourceFileId;
        var sourceFile = this._presentationModel.sourceFile(sourceFileId);

        var messages = sourceFile.messages;
        for (var i = 0; i < messages.length; ++i) {
            var message = messages[i];
            sourceFrame.addMessageToSource(message.lineNumber, message.originalMessage);
        }

        var breakpoints = this._presentationModel.breakpointsForSourceFileId(sourceFileId);
        for (var i = 0; i < breakpoints.length; ++i) {
            var breakpoint = breakpoints[i];
            sourceFrame.addBreakpoint(breakpoint.lineNumber, breakpoint.resolved, breakpoint.condition, breakpoint.enabled);
        }
    },

    _clearCurrentExecutionLine: function()
    {
        if (this._executionSourceFrame)
            this._executionSourceFrame.clearExecutionLine();
        delete this._executionSourceFrame;
    },

    _callFrameSelected: function(event)
    {
        var callFrame = event.data;

        this._clearCurrentExecutionLine();

        if (!callFrame)
            return;

        this.sidebarPanes.scopechain.update(callFrame);
        this.sidebarPanes.watchExpressions.refreshExpressions();
        this.sidebarPanes.callstack.selectedCallFrame = this._presentationModel.selectedCallFrame;

        function didGetSourceLocation(sourceFileId, lineNumber)
        {
            if (!sourceFileId)
                return;

            if (!(sourceFileId in this._sourceFileIdToFilesSelectOption)) {
                // Anonymous scripts are not added to files select by default.
                this._addOptionToFilesSelect(sourceFileId);
            }
            var sourceFrame = this._showSourceFrameAndAddToHistory(sourceFileId);
            sourceFrame.setExecutionLine(lineNumber);
            this._executionSourceFrame = sourceFrame;
        }
        callFrame.sourceLine(didGetSourceLocation.bind(this));
    },

    _filesSelectChanged: function()
    {
        var sourceFileId = this._filesSelectElement[this._filesSelectElement.selectedIndex]._sourceFileId;
        this._showSourceFrameAndAddToHistory(sourceFileId);
    },

    _startSidebarResizeDrag: function(event)
    {
        WebInspector.elementDragStart(this.sidebarElement, this._sidebarResizeDrag.bind(this), this._endSidebarResizeDrag.bind(this), event, "col-resize");

        if (event.target === this.sidebarResizeWidgetElement)
            this._dragOffset = (event.target.offsetWidth - (event.pageX - event.target.totalOffsetLeft));
        else
            this._dragOffset = 0;
    },

    _endSidebarResizeDrag: function(event)
    {
        WebInspector.elementDragEnd(event);
        delete this._dragOffset;
        this.saveSidebarWidth();
    },

    _sidebarResizeDrag: function(event)
    {
        var x = event.pageX + this._dragOffset;
        var newWidth = Number.constrain(window.innerWidth - x, Preferences.minScriptsSidebarWidth, window.innerWidth * 0.66);
        this.setSidebarWidth(newWidth);
        event.preventDefault();
    },

    setSidebarWidth: function(newWidth)
    {
        this.sidebarElement.style.width = newWidth + "px";
        this.sidebarButtonsElement.style.width = newWidth + "px";
        this.viewsContainerElement.style.right = newWidth + "px";
        this.sidebarResizeWidgetElement.style.right = newWidth + "px";
        this.sidebarResizeElement.style.right = (newWidth - 3) + "px";

        this.resize();
    },

    _setPauseOnExceptions: function(pauseOnExceptionsState)
    {
        pauseOnExceptionsState = pauseOnExceptionsState || WebInspector.ScriptsPanel.PauseOnExceptionsState.DontPauseOnExceptions;
        function callback(error)
        {
            if (error)
                return;
            if (pauseOnExceptionsState == WebInspector.ScriptsPanel.PauseOnExceptionsState.DontPauseOnExceptions)
                this._pauseOnExceptionButton.title = WebInspector.UIString("Don't pause on exceptions.\nClick to Pause on all exceptions.");
            else if (pauseOnExceptionsState == WebInspector.ScriptsPanel.PauseOnExceptionsState.PauseOnAllExceptions)
                this._pauseOnExceptionButton.title = WebInspector.UIString("Pause on all exceptions.\nClick to Pause on uncaught exceptions.");
            else if (pauseOnExceptionsState == WebInspector.ScriptsPanel.PauseOnExceptionsState.PauseOnUncaughtExceptions)
                this._pauseOnExceptionButton.title = WebInspector.UIString("Pause on uncaught exceptions.\nClick to Not pause on exceptions.");

            this._pauseOnExceptionButton.state = pauseOnExceptionsState;
            WebInspector.settings.pauseOnExceptionStateString = pauseOnExceptionsState;
        }
        DebuggerAgent.setPauseOnExceptions(pauseOnExceptionsState, callback.bind(this));
    },

    _updateDebuggerButtons: function()
    {
        if (this._debuggerEnabled) {
            this.enableToggleButton.title = WebInspector.UIString("Debugging enabled. Click to disable.");
            this.enableToggleButton.toggled = true;
            this._pauseOnExceptionButton.visible = true;
            this.panelEnablerView.visible = false;
        } else {
            this.enableToggleButton.title = WebInspector.UIString("Debugging disabled. Click to enable.");
            this.enableToggleButton.toggled = false;
            this._pauseOnExceptionButton.visible = false;
            this.panelEnablerView.visible = true;
        }

        if (this._paused) {
            this.pauseButton.addStyleClass("paused");

            this.pauseButton.disabled = false;
            this.stepOverButton.disabled = false;
            this.stepIntoButton.disabled = false;
            this.stepOutButton.disabled = false;

            this.debuggerStatusElement.textContent = WebInspector.UIString("Paused");
        } else {
            this.pauseButton.removeStyleClass("paused");

            this.pauseButton.disabled = this._waitingToPause;
            this.stepOverButton.disabled = true;
            this.stepIntoButton.disabled = true;
            this.stepOutButton.disabled = true;

            if (this._waitingToPause)
                this.debuggerStatusElement.textContent = WebInspector.UIString("Pausing");
            else if (this._stepping)
                this.debuggerStatusElement.textContent = WebInspector.UIString("Stepping");
            else
                this.debuggerStatusElement.textContent = "";
        }
    },

    _updateBackAndForwardButtons: function()
    {
        this.backButton.disabled = this._currentBackForwardIndex <= 0;
        this.forwardButton.disabled = this._currentBackForwardIndex >= (this._backForwardList.length - 1);
    },

    _clearInterface: function()
    {
        this.sidebarPanes.callstack.update(null);
        this.sidebarPanes.scopechain.update(null);
        this.sidebarPanes.jsBreakpoints.clearBreakpointHighlight();
        if (Preferences.nativeInstrumentationEnabled) {
            this.sidebarPanes.domBreakpoints.clearBreakpointHighlight();
            this.sidebarPanes.eventListenerBreakpoints.clearBreakpointHighlight();
            this.sidebarPanes.xhrBreakpoints.clearBreakpointHighlight();
        }

        this._clearCurrentExecutionLine();
        this._updateDebuggerButtons();
    },

    _goBack: function()
    {
        if (this._currentBackForwardIndex <= 0) {
            console.error("Can't go back from index " + this._currentBackForwardIndex);
            return;
        }

        this._showSourceFrame(this._backForwardList[--this._currentBackForwardIndex]);
        this._updateBackAndForwardButtons();
    },

    _goForward: function()
    {
        if (this._currentBackForwardIndex >= this._backForwardList.length - 1) {
            console.error("Can't go forward from index " + this._currentBackForwardIndex);
            return;
        }

        this._showSourceFrame(this._backForwardList[++this._currentBackForwardIndex]);
        this._updateBackAndForwardButtons();
    },

    _enableDebugging: function()
    {
        if (this._debuggerEnabled)
            return;
        this._toggleDebugging(this.panelEnablerView.alwaysEnabled);
    },

    _toggleDebugging: function(optionalAlways)
    {
        this._paused = false;
        this._waitingToPause = false;
        this._stepping = false;

        if (this._debuggerEnabled) {
            WebInspector.settings.debuggerEnabled = false;
            WebInspector.debuggerModel.disableDebugger();
        } else {
            WebInspector.settings.debuggerEnabled = !!optionalAlways;
            WebInspector.debuggerModel.enableDebugger();
        }
    },

    _togglePauseOnExceptions: function()
    {
        var nextStateMap = {};
        var stateEnum = WebInspector.ScriptsPanel.PauseOnExceptionsState;
        nextStateMap[stateEnum.DontPauseOnExceptions] = stateEnum.PauseOnAllExceptions;
        nextStateMap[stateEnum.PauseOnAllExceptions] = stateEnum.PauseOnUncaughtExceptions;
        nextStateMap[stateEnum.PauseOnUncaughtExceptions] = stateEnum.DontPauseOnExceptions;
        this._setPauseOnExceptions(nextStateMap[this._pauseOnExceptionButton.state]);
    },

    _togglePause: function()
    {
        if (this._paused) {
            this._paused = false;
            this._waitingToPause = false;
            DebuggerAgent.resume();
        } else {
            this._stepping = false;
            this._waitingToPause = true;
            DebuggerAgent.pause();
        }

        this._clearInterface();
    },

    _stepOverClicked: function()
    {
        this._paused = false;
        this._stepping = true;

        this._clearInterface();

        DebuggerAgent.stepOver();
    },

    _stepIntoClicked: function()
    {
        this._paused = false;
        this._stepping = true;

        this._clearInterface();

        DebuggerAgent.stepInto();
    },

    _stepOutClicked: function()
    {
        this._paused = false;
        this._stepping = true;

        this._clearInterface();

        DebuggerAgent.stepOut();
    },

    toggleBreakpointsClicked: function()
    {
        this.toggleBreakpointsButton.toggled = !this.toggleBreakpointsButton.toggled;
        if (this.toggleBreakpointsButton.toggled) {
            DebuggerAgent.setBreakpointsActive(true);
            this.toggleBreakpointsButton.title = WebInspector.UIString("Deactivate all breakpoints.");
            document.getElementById("main-panels").removeStyleClass("breakpoints-deactivated");
        } else {
            DebuggerAgent.setBreakpointsActive(false);
            this.toggleBreakpointsButton.title = WebInspector.UIString("Activate all breakpoints.");
            document.getElementById("main-panels").addStyleClass("breakpoints-deactivated");
        }
    },

    elementsToRestoreScrollPositionsFor: function()
    {
        return [ this.sidebarElement ];
    },

    _registerShortcuts: function()
    {
        var section = WebInspector.shortcutsHelp.section(WebInspector.UIString("Scripts Panel"));
        var handler, shortcut1, shortcut2;
        var platformSpecificModifier = WebInspector.KeyboardShortcut.Modifiers.CtrlOrMeta;

        var shortcuts = {};

        // Continue.
        handler = this.pauseButton.click.bind(this.pauseButton);
        shortcut1 = WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.F8);
        shortcuts[shortcut1.key] = handler;
        shortcut2 = WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Slash, platformSpecificModifier);
        shortcuts[shortcut2.key] = handler;
        section.addAlternateKeys([ shortcut1.name, shortcut2.name ], WebInspector.UIString("Pause/Continue"));

        // Step over.
        handler = this.stepOverButton.click.bind(this.stepOverButton);
        shortcut1 = WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.F10);
        shortcuts[shortcut1.key] = handler;
        shortcut2 = WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.SingleQuote, platformSpecificModifier);
        shortcuts[shortcut2.key] = handler;
        section.addAlternateKeys([ shortcut1.name, shortcut2.name ], WebInspector.UIString("Step over"));

        // Step into.
        handler = this.stepIntoButton.click.bind(this.stepIntoButton);
        shortcut1 = WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.F11);
        shortcuts[shortcut1.key] = handler;
        shortcut2 = WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Semicolon, platformSpecificModifier);
        shortcuts[shortcut2.key] = handler;
        section.addAlternateKeys([ shortcut1.name, shortcut2.name ], WebInspector.UIString("Step into"));

        // Step out.
        handler = this.stepOutButton.click.bind(this.stepOutButton);
        shortcut1 = WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.F11, WebInspector.KeyboardShortcut.Modifiers.Shift);
        shortcuts[shortcut1.key] = handler;
        shortcut2 = WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Semicolon, WebInspector.KeyboardShortcut.Modifiers.Shift, platformSpecificModifier);
        shortcuts[shortcut2.key] = handler;
        section.addAlternateKeys([ shortcut1.name, shortcut2.name ], WebInspector.UIString("Step out"));

        this.sidebarPanes.callstack.registerShortcuts(section, shortcuts);
        this.registerShortcuts(shortcuts);
    },

    searchCanceled: function()
    {
        if (this._searchView)
            this._searchView.searchCanceled();

        delete this._searchView;
        delete this._searchQuery;
    },

    performSearch: function(query)
    {
        WebInspector.searchController.updateSearchMatchesCount(0, this);

        if (!this.visibleView)
            return;

        // Call searchCanceled since it will reset everything we need before doing a new search.
        this.searchCanceled();

        this._searchView = this.visibleView;
        this._searchQuery = query;

        function finishedCallback(view, searchMatches)
        {
            if (!searchMatches)
                return;

            WebInspector.searchController.updateSearchMatchesCount(searchMatches, this);
            view.jumpToFirstSearchResult();
        }

        this._searchView.performSearch(query, finishedCallback.bind(this));
    },

    jumpToNextSearchResult: function()
    {
        if (!this._searchView)
            return;

        if (this._searchView !== this.visibleView) {
            this.performSearch(this._searchQuery);
            return;
        }

        if (this._searchView.showingLastSearchResult())
            this._searchView.jumpToFirstSearchResult();
        else
            this._searchView.jumpToNextSearchResult();
    },

    jumpToPreviousSearchResult: function()
    {
        if (!this._searchView)
            return;

        if (this._searchView !== this.visibleView) {
            this.performSearch(this._searchQuery);
            if (this._searchView)
                this._searchView.jumpToLastSearchResult();
            return;
        }

        if (this._searchView.showingFirstSearchResult())
            this._searchView.jumpToLastSearchResult();
        else
            this._searchView.jumpToPreviousSearchResult();
    },

    _toggleFormatSourceFiles: function()
    {
        WebInspector.panels.scripts.reset();
        this._toggleFormatSourceFilesButton.toggled = !this._toggleFormatSourceFilesButton.toggled;
        this._presentationModel.setFormatSourceFiles(this._toggleFormatSourceFilesButton.toggled);
    }
}

WebInspector.ScriptsPanel.prototype.__proto__ = WebInspector.Panel.prototype;


WebInspector.SourceFrameDelegateForScriptsPanel = function(model, sourceFileId, scriptName)
{
    WebInspector.SourceFrameDelegate.call(this);
    this._model = model;
    this._sourceFileId = sourceFileId;
    this._popoverObjectGroup = "popover";
    this._scriptName = scriptName;
}

WebInspector.SourceFrameDelegateForScriptsPanel.prototype = {
    requestContent: function(callback)
    {
        this._model.requestSourceFileContent(this._sourceFileId, callback);
    },

    debuggingSupported: function()
    {
        return true;
    },

    setBreakpoint: function(lineNumber, condition, enabled)
    {
        this._model.setBreakpoint(this._sourceFileId, lineNumber, condition, enabled);

        if (!WebInspector.panels.scripts.breakpointsActivated)
            WebInspector.panels.scripts.toggleBreakpointsClicked();
    },

    updateBreakpoint: function(lineNumber, condition, enabled)
    {
        this._model.updateBreakpoint(this._sourceFileId, lineNumber, condition, enabled);
    },

    removeBreakpoint: function(lineNumber)
    {
        this._model.removeBreakpoint(this._sourceFileId, lineNumber);
    },

    findBreakpoint: function(lineNumber)
    {
        return this._model.findBreakpoint(this._sourceFileId, lineNumber);
    },

    continueToLine: function(lineNumber)
    {
        this._model.continueToLine(this._sourceFileId, lineNumber);
    },

    canEditScriptSource: function()
    {
        return this._model.canEditScriptSource(this._sourceFileId);
    },

    editScriptSource: function(text, callback)
    {
        this._model.editScriptSource(this._sourceFileId, text, callback);
    },

    setScriptSourceIsBeingEdited: function(inEditMode)
    {
        WebInspector.panels.scripts.setScriptSourceIsBeingEdited(this._sourceFileId, inEditMode);
    },

    debuggerPaused: function()
    {
        return WebInspector.panels.scripts.paused;
    },

    evaluateInSelectedCallFrame: function(string, callback)
    {
        WebInspector.panels.scripts.evaluateInSelectedCallFrame(string, this._popoverObjectGroup, false, callback);
    },

    releaseEvaluationResult: function()
    {
        RuntimeAgent.releaseObjectGroup(this._popoverObjectGroup);
    },

    suggestedFileName: function()
    {
        return this._scriptName;
    }
}

WebInspector.SourceFrameDelegateForScriptsPanel.prototype.__proto__ = WebInspector.SourceFrameDelegate.prototype;
