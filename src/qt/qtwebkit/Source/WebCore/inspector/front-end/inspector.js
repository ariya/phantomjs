/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Matt Lilek (pewtermoose@gmail.com).
 * Copyright (C) 2009 Joseph Pecoraro
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

var WebInspector = {
    _panelDescriptors: function()
    {
        this.panels = {};
        WebInspector.inspectorView = new WebInspector.InspectorView();
        var parentElement = document.getElementById("main");
        WebInspector.inspectorView.show(parentElement);
        WebInspector.inspectorView.addEventListener(WebInspector.InspectorView.Events.PanelSelected, this._panelSelected, this);

        var elements = new WebInspector.ElementsPanelDescriptor();
        var resources = new WebInspector.PanelDescriptor("resources", WebInspector.UIString("Resources"), "ResourcesPanel", "ResourcesPanel.js");
        var network = new WebInspector.NetworkPanelDescriptor();
        var scripts = new WebInspector.ScriptsPanelDescriptor();
        var timeline = new WebInspector.TimelinePanelDescriptor();
        var profiles = new WebInspector.ProfilesPanelDescriptor();
        var audits = new WebInspector.PanelDescriptor("audits", WebInspector.UIString("Audits"), "AuditsPanel", "AuditsPanel.js");
        var console = new WebInspector.PanelDescriptor("console", WebInspector.UIString("Console"), "ConsolePanel");
        var allDescriptors = [elements, resources, network, scripts, timeline, profiles, audits, console];
        var allProfilers = [profiles];
        if (WebInspector.experimentsSettings.separateProfilers.isEnabled()) {
            allProfilers = [];
            allProfilers.push(new WebInspector.PanelDescriptor("cpu-profiler", WebInspector.UIString("CPU Profiler"), "CPUProfilerPanel", "ProfilesPanel.js"));
            if (!WebInspector.WorkerManager.isWorkerFrontend())
                allProfilers.push(new WebInspector.PanelDescriptor("css-profiler", WebInspector.UIString("CSS Profiler"), "CSSSelectorProfilerPanel", "ProfilesPanel.js"));
            if (Capabilities.heapProfilerPresent)
                allProfilers.push(new WebInspector.PanelDescriptor("heap-profiler", WebInspector.UIString("Heap Profiler"), "HeapProfilerPanel", "ProfilesPanel.js"));
            if (!WebInspector.WorkerManager.isWorkerFrontend() && WebInspector.experimentsSettings.canvasInspection.isEnabled())
                allProfilers.push(new WebInspector.PanelDescriptor("canvas-profiler", WebInspector.UIString("Canvas Profiler"), "CanvasProfilerPanel", "ProfilesPanel.js"));
            Array.prototype.splice.bind(allDescriptors, allDescriptors.indexOf(profiles), 1).apply(null, allProfilers);
        }

        var panelDescriptors = [];
        if (WebInspector.WorkerManager.isWorkerFrontend()) {
            panelDescriptors.push(scripts);
            panelDescriptors.push(timeline);
            panelDescriptors = panelDescriptors.concat(allProfilers);
            panelDescriptors.push(console);
            return panelDescriptors;
        }
        for (var i = 0; i < allDescriptors.length; ++i)
            panelDescriptors.push(allDescriptors[i]);
        return panelDescriptors;
    },

    _panelSelected: function()
    {
        this._toggleConsoleButton.setEnabled(WebInspector.inspectorView.currentPanel().name !== "console");
    },

    _createGlobalStatusBarItems: function()
    {
        var bottomStatusBarContainer = document.getElementById("bottom-status-bar-container");

        // Create main dock button and options.
        var mainStatusBar = document.getElementById("main-status-bar");
        mainStatusBar.insertBefore(this.dockController.element, bottomStatusBarContainer);

        this._toggleConsoleButton = new WebInspector.StatusBarButton(WebInspector.UIString("Show console."), "console-status-bar-item");
        this._toggleConsoleButton.addEventListener("click", this._toggleConsoleButtonClicked.bind(this), false);
        mainStatusBar.insertBefore(this._toggleConsoleButton.element, bottomStatusBarContainer);

        if (this.inspectElementModeController)
            mainStatusBar.insertBefore(this.inspectElementModeController.toggleSearchButton.element, bottomStatusBarContainer);

        mainStatusBar.appendChild(this.settingsController.statusBarItem);
    },

    _toggleConsoleButtonClicked: function()
    {
        if (!this._toggleConsoleButton.enabled())
            return;

        this._toggleConsoleButton.toggled = !this._toggleConsoleButton.toggled;

        var animationType = window.event && window.event.shiftKey ? WebInspector.Drawer.AnimationType.Slow : WebInspector.Drawer.AnimationType.Normal;
        if (this._toggleConsoleButton.toggled) {
            this._toggleConsoleButton.title = WebInspector.UIString("Hide console.");
            this.drawer.show(this.consoleView, animationType);
            this._consoleWasShown = true;
        } else {
            this._toggleConsoleButton.title = WebInspector.UIString("Show console.");
            this.drawer.hide(animationType);
            delete this._consoleWasShown;
        }
    },

    /**
     * @param {Element} statusBarElement
     * @param {WebInspector.View} view
     * @param {function()=} onclose
     */
    showViewInDrawer: function(statusBarElement, view, onclose)
    {
        this._toggleConsoleButton.title = WebInspector.UIString("Hide console.");
        this._toggleConsoleButton.toggled = false;
        this._closePreviousDrawerView();

        var drawerStatusBarHeader = document.createElement("div");
        drawerStatusBarHeader.className = "drawer-header status-bar-item";
        drawerStatusBarHeader.appendChild(statusBarElement);
        drawerStatusBarHeader.onclose = onclose;

        var closeButton = drawerStatusBarHeader.createChild("span");
        closeButton.textContent = WebInspector.UIString("\u00D7");
        closeButton.addStyleClass("drawer-header-close-button");
        closeButton.addEventListener("click", this.closeViewInDrawer.bind(this), false);

        var panelStatusBar = document.getElementById("panel-status-bar");
        var drawerViewAnchor = document.getElementById("drawer-view-anchor");
        panelStatusBar.insertBefore(drawerStatusBarHeader, drawerViewAnchor);
        this._drawerStatusBarHeader = drawerStatusBarHeader;
        this.drawer.show(view, WebInspector.Drawer.AnimationType.Immediately);
    },

    closeViewInDrawer: function()
    {
        if (this._drawerStatusBarHeader) {
            this._closePreviousDrawerView();

            // Once drawer is closed console should be shown if it was shown before current view replaced it in drawer. 
            if (!this._consoleWasShown)
                this.drawer.hide(WebInspector.Drawer.AnimationType.Immediately);
            else
                this._toggleConsoleButtonClicked();
        }
    },

    _closePreviousDrawerView: function()
    {
        if (this._drawerStatusBarHeader) {
            this._drawerStatusBarHeader.parentElement.removeChild(this._drawerStatusBarHeader);
            if (this._drawerStatusBarHeader.onclose)
                this._drawerStatusBarHeader.onclose();
            delete this._drawerStatusBarHeader;
        }
    },

    _updateErrorAndWarningCounts: function()
    {
        var errorWarningElement = document.getElementById("error-warning-count");
        if (!errorWarningElement)
            return;

        var errors = WebInspector.console.errors;
        var warnings = WebInspector.console.warnings;
        if (!errors && !warnings) {
            errorWarningElement.addStyleClass("hidden");
            return;
        }

        errorWarningElement.removeStyleClass("hidden");

        errorWarningElement.removeChildren();

        if (errors) {
            var errorImageElement = document.createElement("img");
            errorImageElement.id = "error-count-img";
            errorWarningElement.appendChild(errorImageElement);
            var errorElement = document.createElement("span");
            errorElement.id = "error-count";
            errorElement.textContent = errors;
            errorWarningElement.appendChild(errorElement);
        }

        if (warnings) {
            var warningsImageElement = document.createElement("img");
            warningsImageElement.id = "warning-count-img";
            errorWarningElement.appendChild(warningsImageElement);
            var warningsElement = document.createElement("span");
            warningsElement.id = "warning-count";
            warningsElement.textContent = warnings;
            errorWarningElement.appendChild(warningsElement);
        }

        if (errors) {
            if (warnings) {
                if (errors == 1) {
                    if (warnings == 1)
                        errorWarningElement.title = WebInspector.UIString("%d error, %d warning", errors, warnings);
                    else
                        errorWarningElement.title = WebInspector.UIString("%d error, %d warnings", errors, warnings);
                } else if (warnings == 1)
                    errorWarningElement.title = WebInspector.UIString("%d errors, %d warning", errors, warnings);
                else
                    errorWarningElement.title = WebInspector.UIString("%d errors, %d warnings", errors, warnings);
            } else if (errors == 1)
                errorWarningElement.title = WebInspector.UIString("%d error", errors);
            else
                errorWarningElement.title = WebInspector.UIString("%d errors", errors);
        } else if (warnings == 1)
            errorWarningElement.title = WebInspector.UIString("%d warning", warnings);
        else if (warnings)
            errorWarningElement.title = WebInspector.UIString("%d warnings", warnings);
        else
            errorWarningElement.title = null;
    },

    get inspectedPageDomain()
    {
        var parsedURL = WebInspector.inspectedPageURL && WebInspector.inspectedPageURL.asParsedURL();
        return parsedURL ? parsedURL.host : "";
    },

    _initializeCapability: function(name, callback, error, result)
    {
        Capabilities[name] = result;
        if (callback)
            callback();
    },

    _zoomIn: function()
    {
        this._zoomLevel = Math.min(this._zoomLevel + 1, WebInspector.Zoom.Table.length - WebInspector.Zoom.DefaultOffset - 1);
        this._requestZoom();
    },

    _zoomOut: function()
    {
        this._zoomLevel = Math.max(this._zoomLevel - 1, -WebInspector.Zoom.DefaultOffset);
        this._requestZoom();
    },

    _resetZoom: function()
    {
        this._zoomLevel = 0;
        this._requestZoom();
    },

    _requestZoom: function()
    {
        WebInspector.settings.zoomLevel.set(this._zoomLevel);
        // For backwards compatibility, zoomLevel takes integers (with 0 being default zoom).
        var index = this._zoomLevel + WebInspector.Zoom.DefaultOffset;
        index = Math.min(WebInspector.Zoom.Table.length - 1, index);
        index = Math.max(0, index);
        InspectorFrontendHost.setZoomFactor(WebInspector.Zoom.Table[index]);
    },

    _debuggerPaused: function()
    {
        // Create scripts panel upon demand.
        WebInspector.panel("scripts");
    }
}

WebInspector.Events = {
    InspectorLoaded: "InspectorLoaded",
    InspectorClosing: "InspectorClosing"
}

{(function parseQueryParameters()
{
    WebInspector.queryParamsObject = {};
    var queryParams = window.location.search;
    if (!queryParams)
        return;
    var params = queryParams.substring(1).split("&");
    for (var i = 0; i < params.length; ++i) {
        var pair = params[i].split("=");
        WebInspector.queryParamsObject[pair[0]] = pair[1];
    }
})();}

WebInspector.suggestReload = function()
{
    if (window.confirm(WebInspector.UIString("It is recommended to restart inspector after making these changes. Would you like to restart it?")))
        this.reload();
}

WebInspector.reload = function()
{
    var queryParams = window.location.search;
    var url = window.location.href;
    url = url.substring(0, url.length - queryParams.length);
    var queryParamsObject = {};
    for (var name in WebInspector.queryParamsObject)
        queryParamsObject[name] = WebInspector.queryParamsObject[name];
    if (this.dockController)
        queryParamsObject["dockSide"] = this.dockController.dockSide();
    var names = Object.keys(queryParamsObject);
    for (var i = 0; i < names.length; ++i)
        url += (i ? "&" : "?") + names[i] + "=" + queryParamsObject[names[i]];

    InspectorBackend.disconnect();
    document.location = url;
}

WebInspector.loaded = function()
{
    InspectorBackend.loadFromJSONIfNeeded("../Inspector.json");
    WebInspector.dockController = new WebInspector.DockController();

    if (WebInspector.WorkerManager.isDedicatedWorkerFrontend()) {
        // Do not create socket for the worker front-end.
        WebInspector.doLoadedDone();
        return;
    }

    var ws;
    if ("ws" in WebInspector.queryParamsObject)
        ws = "ws://" + WebInspector.queryParamsObject.ws;
    else if ("page" in WebInspector.queryParamsObject) {
        var page = WebInspector.queryParamsObject.page;
        var host = "host" in WebInspector.queryParamsObject ? WebInspector.queryParamsObject.host : window.location.host;
        ws = "ws://" + host + "/devtools/page/" + page;
    }

    if (ws) {
        WebInspector.socket = new WebSocket(ws);
        WebInspector.socket.onmessage = function(message) { InspectorBackend.dispatch(message.data); }
        WebInspector.socket.onerror = function(error) { console.error(error); }
        WebInspector.socket.onopen = function() {
            InspectorFrontendHost.sendMessageToBackend = WebInspector.socket.send.bind(WebInspector.socket);
            WebInspector.doLoadedDone();
        }
        WebInspector.socket.onclose = function() {
            if (!WebInspector.socket._detachReason)
                (new WebInspector.RemoteDebuggingTerminatedScreen("websocket_closed")).showModal();
        }
        return;
    }

    WebInspector.doLoadedDone();

    // In case of loading as a web page with no bindings / harness, kick off initialization manually.
    if (InspectorFrontendHost.isStub) {
        InspectorFrontendAPI.dispatchQueryParameters();
        WebInspector._doLoadedDoneWithCapabilities();
    }
}

WebInspector.doLoadedDone = function()
{
    // Install styles and themes
    WebInspector.installPortStyles();
    if (WebInspector.socket)
        document.body.addStyleClass("remote");

    if (WebInspector.queryParamsObject.toolbarColor && WebInspector.queryParamsObject.textColor)
        WebInspector.setToolbarColors(WebInspector.queryParamsObject.toolbarColor, WebInspector.queryParamsObject.textColor);

    InspectorFrontendHost.loaded();
    WebInspector.WorkerManager.loaded();

    DebuggerAgent.causesRecompilation(WebInspector._initializeCapability.bind(WebInspector, "debuggerCausesRecompilation", null));
    DebuggerAgent.supportsSeparateScriptCompilationAndExecution(WebInspector._initializeCapability.bind(WebInspector, "separateScriptCompilationAndExecutionEnabled", null));
    ProfilerAgent.causesRecompilation(WebInspector._initializeCapability.bind(WebInspector, "profilerCausesRecompilation", null));
    ProfilerAgent.isSampling(WebInspector._initializeCapability.bind(WebInspector, "samplingCPUProfiler", null));
    HeapProfilerAgent.hasHeapProfiler(WebInspector._initializeCapability.bind(WebInspector, "heapProfilerPresent", null));
    TimelineAgent.supportsFrameInstrumentation(WebInspector._initializeCapability.bind(WebInspector, "timelineSupportsFrameInstrumentation", null));
    TimelineAgent.canMonitorMainThread(WebInspector._initializeCapability.bind(WebInspector, "timelineCanMonitorMainThread", null));
    PageAgent.canShowDebugBorders(WebInspector._initializeCapability.bind(WebInspector, "canShowDebugBorders", null));
    PageAgent.canShowFPSCounter(WebInspector._initializeCapability.bind(WebInspector, "canShowFPSCounter", null));
    PageAgent.canContinuouslyPaint(WebInspector._initializeCapability.bind(WebInspector, "canContinuouslyPaint", null));
    PageAgent.canOverrideDeviceMetrics(WebInspector._initializeCapability.bind(WebInspector, "canOverrideDeviceMetrics", null));
    PageAgent.canOverrideGeolocation(WebInspector._initializeCapability.bind(WebInspector, "canOverrideGeolocation", null));
    WorkerAgent.canInspectWorkers(WebInspector._initializeCapability.bind(WebInspector, "canInspectWorkers", null));
    PageAgent.canOverrideDeviceOrientation(WebInspector._initializeCapability.bind(WebInspector, "canOverrideDeviceOrientation", WebInspector._doLoadedDoneWithCapabilities.bind(WebInspector)));
}

WebInspector._doLoadedDoneWithCapabilities = function()
{
    new WebInspector.VersionController().updateVersion();

    WebInspector.shortcutsScreen = new WebInspector.ShortcutsScreen();
    this._registerShortcuts();

    // set order of some sections explicitly
    WebInspector.shortcutsScreen.section(WebInspector.UIString("Console"));
    WebInspector.shortcutsScreen.section(WebInspector.UIString("Elements Panel"));

    var panelDescriptors = this._panelDescriptors();
    for (var i = 0; i < panelDescriptors.length; ++i)
        panelDescriptors[i].registerShortcuts();

    this.console = new WebInspector.ConsoleModel();
    this.console.addEventListener(WebInspector.ConsoleModel.Events.ConsoleCleared, this._updateErrorAndWarningCounts, this);
    this.console.addEventListener(WebInspector.ConsoleModel.Events.MessageAdded, this._updateErrorAndWarningCounts, this);
    this.console.addEventListener(WebInspector.ConsoleModel.Events.RepeatCountUpdated, this._updateErrorAndWarningCounts, this);

    WebInspector.CSSMetadata.requestCSSShorthandData();

    this.drawer = new WebInspector.Drawer();

    this.networkManager = new WebInspector.NetworkManager();
    this.resourceTreeModel = new WebInspector.ResourceTreeModel(this.networkManager);
    this.debuggerModel = new WebInspector.DebuggerModel();
    this.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.DebuggerPaused, this._debuggerPaused, this);
    this.networkLog = new WebInspector.NetworkLog();
    this.domAgent = new WebInspector.DOMAgent();
    this.runtimeModel = new WebInspector.RuntimeModel(this.resourceTreeModel);

    this.consoleView = new WebInspector.ConsoleView(WebInspector.WorkerManager.isWorkerFrontend());

    InspectorBackend.registerInspectorDispatcher(this);

    this.isolatedFileSystemManager = new WebInspector.IsolatedFileSystemManager();
    this.isolatedFileSystemDispatcher = new WebInspector.IsolatedFileSystemDispatcher(this.isolatedFileSystemManager);
    this.fileMapping = new WebInspector.FileMapping();
    this.workspace = new WebInspector.Workspace(this.fileMapping, this.isolatedFileSystemManager.mapping());

    this.cssModel = new WebInspector.CSSStyleModel(this.workspace);
    this.timelineManager = new WebInspector.TimelineManager();
    this.userAgentSupport = new WebInspector.UserAgentSupport();

    this.searchController = new WebInspector.SearchController();
    this.advancedSearchController = new WebInspector.AdvancedSearchController();
    if (!WebInspector.WorkerManager.isWorkerFrontend())
        this.inspectElementModeController = new WebInspector.InspectElementModeController();

    this.settingsController = new WebInspector.SettingsController();

    this.domBreakpointsSidebarPane = new WebInspector.DOMBreakpointsSidebarPane();

    this._zoomLevel = WebInspector.settings.zoomLevel.get();
    if (this._zoomLevel)
        this._requestZoom();

    var autoselectPanel = WebInspector.UIString("a panel chosen automatically");
    var openAnchorLocationSetting = WebInspector.settings.createSetting("openLinkHandler", autoselectPanel);
    this.openAnchorLocationRegistry = new WebInspector.HandlerRegistry(openAnchorLocationSetting);
    this.openAnchorLocationRegistry.registerHandler(autoselectPanel, function() { return false; });

    this.workspaceController = new WebInspector.WorkspaceController(this.workspace);

    this.fileSystemWorkspaceProvider = new WebInspector.FileSystemWorkspaceProvider(this.isolatedFileSystemManager, this.workspace);

    this.networkWorkspaceProvider = new WebInspector.SimpleWorkspaceProvider(this.workspace, WebInspector.projectTypes.Network);
    new WebInspector.NetworkUISourceCodeProvider(this.networkWorkspaceProvider, this.workspace);

    this.breakpointManager = new WebInspector.BreakpointManager(WebInspector.settings.breakpoints, this.debuggerModel, this.workspace);

    this.scriptSnippetModel = new WebInspector.ScriptSnippetModel(this.workspace);

    new WebInspector.DebuggerScriptMapping(this.workspace, this.networkWorkspaceProvider);
    this.liveEditSupport = new WebInspector.LiveEditSupport(this.workspace);
    this.styleContentBinding = new WebInspector.StyleContentBinding(this.cssModel, this.workspace);
    new WebInspector.StylesSourceMapping(this.cssModel, this.workspace);
    if (WebInspector.experimentsSettings.sass.isEnabled())
        new WebInspector.SASSSourceMapping(this.cssModel, this.workspace, this.networkWorkspaceProvider);

    new WebInspector.PresentationConsoleMessageHelper(this.workspace);

    this._createGlobalStatusBarItems();

    this.toolbar = new WebInspector.Toolbar();
    WebInspector.startBatchUpdate();
    for (var i = 0; i < panelDescriptors.length; ++i)
        WebInspector.inspectorView.addPanel(panelDescriptors[i]);
    WebInspector.endBatchUpdate();

    this.addMainEventListeners(document);

    window.addEventListener("resize", this.windowResize.bind(this), true);

    var errorWarningCount = document.getElementById("error-warning-count");
    errorWarningCount.addEventListener("click", this.showConsole.bind(this), false);
    this._updateErrorAndWarningCounts();

    this.extensionServer.initExtensions();

    this.console.enableAgent();

    function showInitialPanel()
    {
        if (!WebInspector.inspectorView.currentPanel())
            WebInspector.showPanel(WebInspector.settings.lastActivePanel.get());
    }

    InspectorAgent.enable(showInitialPanel);
    this.databaseModel = new WebInspector.DatabaseModel();
    this.domStorageModel = new WebInspector.DOMStorageModel();

    if (!Capabilities.profilerCausesRecompilation || WebInspector.settings.profilerEnabled.get())
        ProfilerAgent.enable();

    if (WebInspector.settings.showPaintRects.get())
        PageAgent.setShowPaintRects(true);

    if (WebInspector.settings.showDebugBorders.get())
        PageAgent.setShowDebugBorders(true);

    if (WebInspector.settings.continuousPainting.get())
        PageAgent.setContinuousPaintingEnabled(true);

    if (WebInspector.settings.javaScriptDisabled.get())
        PageAgent.setScriptExecutionDisabled(true);

    if (WebInspector.settings.showFPSCounter.get())
        PageAgent.setShowFPSCounter(true);

    this.domAgent._emulateTouchEventsChanged();

    WebInspector.WorkerManager.loadCompleted();
    InspectorFrontendAPI.loadCompleted();

    WebInspector.notifications.dispatchEventToListeners(WebInspector.Events.InspectorLoaded);
}

var windowLoaded = function()
{
    var localizedStringsURL = InspectorFrontendHost.localizedStringsURL();
    if (localizedStringsURL) {
        var localizedStringsScriptElement = document.createElement("script");
        localizedStringsScriptElement.addEventListener("load", WebInspector.loaded.bind(WebInspector), false);
        localizedStringsScriptElement.type = "text/javascript";
        localizedStringsScriptElement.src = localizedStringsURL;
        document.head.appendChild(localizedStringsScriptElement);
    } else
        WebInspector.loaded();

    window.removeEventListener("DOMContentLoaded", windowLoaded, false);
    delete windowLoaded;
};

window.addEventListener("DOMContentLoaded", windowLoaded, false);

// We'd like to enforce asynchronous interaction between the inspector controller and the frontend.
// It is needed to prevent re-entering the backend code.
// Also, native dispatches do not guarantee setTimeouts to be serialized, so we
// enforce serialization using 'messagesToDispatch' queue. It is also important that JSC debugger
// tests require that each command was dispatch within individual timeout callback, so we don't batch them.

var messagesToDispatch = [];

WebInspector.dispatchQueueIsEmpty = function() {
    return messagesToDispatch.length == 0;
}

WebInspector.dispatch = function(message) {
    messagesToDispatch.push(message);
    setTimeout(function() {
        InspectorBackend.dispatch(messagesToDispatch.shift());
    }, 0);
}

WebInspector.windowResize = function(event)
{
    if (WebInspector.inspectorView)
        WebInspector.inspectorView.doResize();
    if (WebInspector.drawer)
        WebInspector.drawer.resize();
    if (WebInspector.toolbar)
        WebInspector.toolbar.resize();
    if (WebInspector.settingsController)
        WebInspector.settingsController.resize();
}

WebInspector.setDockingUnavailable = function(unavailable)
{
    if (this.dockController)
        this.dockController.setDockingUnavailable(unavailable);
}

WebInspector.close = function(event)
{
    if (this._isClosing)
        return;
    this._isClosing = true;
    this.notifications.dispatchEventToListeners(WebInspector.Events.InspectorClosing);
    InspectorFrontendHost.closeWindow();
}

WebInspector.documentClick = function(event)
{
    var anchor = event.target.enclosingNodeOrSelfWithNodeName("a");
    if (!anchor || (anchor.target === "_blank"))
        return;

    // Prevent the link from navigating, since we don't do any navigation by following links normally.
    event.consume(true);

    function followLink()
    {
        if (WebInspector.isBeingEdited(event.target) || WebInspector._showAnchorLocation(anchor))
            return;

        const profileMatch = WebInspector.ProfilesPanelDescriptor.ProfileURLRegExp.exec(anchor.href);
        if (profileMatch) {
            WebInspector.showPanel("profiles").showProfile(profileMatch[1], profileMatch[2]);
            return;
        }

        var parsedURL = anchor.href.asParsedURL();
        if (parsedURL && parsedURL.scheme === "webkit-link-action") {
            if (parsedURL.host === "show-panel") {
                var panel = parsedURL.path.substring(1);
                if (WebInspector.panel(panel))
                    WebInspector.showPanel(panel);
            }
            return;
        }

        InspectorFrontendHost.openInNewTab(anchor.href);
    }

    if (WebInspector.followLinkTimeout)
        clearTimeout(WebInspector.followLinkTimeout);

    if (anchor.preventFollowOnDoubleClick) {
        // Start a timeout if this is the first click, if the timeout is canceled
        // before it fires, then a double clicked happened or another link was clicked.
        if (event.detail === 1)
            WebInspector.followLinkTimeout = setTimeout(followLink, 333);
        return;
    }

    followLink();
}

WebInspector.openResource = function(resourceURL, inResourcesPanel)
{
    var resource = WebInspector.resourceForURL(resourceURL);
    if (inResourcesPanel && resource)
        WebInspector.showPanel("resources").showResource(resource);
    else
        InspectorFrontendHost.openInNewTab(resourceURL);
}

WebInspector._registerShortcuts = function()
{
    var shortcut = WebInspector.KeyboardShortcut;
    var section = WebInspector.shortcutsScreen.section(WebInspector.UIString("All Panels"));
    var keys = [
        shortcut.makeDescriptor("[", shortcut.Modifiers.CtrlOrMeta),
        shortcut.makeDescriptor("]", shortcut.Modifiers.CtrlOrMeta)
    ];
    section.addRelatedKeys(keys, WebInspector.UIString("Go to the panel to the left/right"));

    keys = [
        shortcut.makeDescriptor("[", shortcut.Modifiers.CtrlOrMeta | shortcut.Modifiers.Alt),
        shortcut.makeDescriptor("]", shortcut.Modifiers.CtrlOrMeta | shortcut.Modifiers.Alt)
    ];
    section.addRelatedKeys(keys, WebInspector.UIString("Go back/forward in panel history"));

    section.addKey(shortcut.makeDescriptor(shortcut.Keys.Esc), WebInspector.UIString("Toggle console"));
    section.addKey(shortcut.makeDescriptor("f", shortcut.Modifiers.CtrlOrMeta), WebInspector.UIString("Search"));

    var advancedSearchShortcut = WebInspector.AdvancedSearchController.createShortcut();
    section.addKey(advancedSearchShortcut, WebInspector.UIString("Search across all sources"));

    var inspectElementModeShortcut = WebInspector.InspectElementModeController.createShortcut();
    section.addKey(inspectElementModeShortcut, WebInspector.UIString("Select node to inspect"));

    var openResourceShortcut = WebInspector.KeyboardShortcut.makeDescriptor("o", WebInspector.KeyboardShortcut.Modifiers.CtrlOrMeta);
    section.addKey(openResourceShortcut, WebInspector.UIString("Go to source"));

    if (WebInspector.isMac()) {
        keys = [
            shortcut.makeDescriptor("g", shortcut.Modifiers.Meta),
            shortcut.makeDescriptor("g", shortcut.Modifiers.Meta | shortcut.Modifiers.Shift)
        ];
        section.addRelatedKeys(keys, WebInspector.UIString("Find next/previous"));
    }

    var goToShortcut = WebInspector.GoToLineDialog.createShortcut();
    section.addKey(goToShortcut, WebInspector.UIString("Go to line"));

    keys = [
        shortcut.Keys.F1,
        shortcut.makeDescriptor("?")
    ];
    section.addAlternateKeys(keys, WebInspector.UIString("Show keyboard shortcuts"));
}

/**
 * @param {KeyboardEvent} event
 */
WebInspector.documentKeyDown = function(event)
{
    const helpKey = WebInspector.isMac() ? "U+003F" : "U+00BF"; // "?" for both platforms

    if (event.keyIdentifier === "F1" ||
        (event.keyIdentifier === helpKey && event.shiftKey && (!WebInspector.isBeingEdited(event.target) || event.metaKey))) {
        this.settingsController.showSettingsScreen(WebInspector.SettingsScreen.Tabs.Shortcuts);
        event.consume(true);
        return;
    }

    if (WebInspector.currentFocusElement() && WebInspector.currentFocusElement().handleKeyEvent) {
        WebInspector.currentFocusElement().handleKeyEvent(event);
        if (event.handled) {
            event.consume(true);
            return;
        }
    }

    if (WebInspector.inspectorView.currentPanel()) {
        WebInspector.inspectorView.currentPanel().handleShortcut(event);
        if (event.handled) {
            event.consume(true);
            return;
        }
    }

    if (WebInspector.searchController.handleShortcut(event))
        return;
    if (WebInspector.advancedSearchController.handleShortcut(event))
        return;
    if (WebInspector.inspectElementModeController && WebInspector.inspectElementModeController.handleShortcut(event))
        return;

    switch (event.keyIdentifier) {
        case "U+004F": // O key
            if (!event.shiftKey && !event.altKey && WebInspector.KeyboardShortcut.eventHasCtrlOrMeta(event)) {
                WebInspector.showPanel("scripts").showGoToSourceDialog();
                event.consume(true);
            }
            break;
        case "U+0052": // R key
            if (WebInspector.KeyboardShortcut.eventHasCtrlOrMeta(event)) {
                PageAgent.reload(event.shiftKey);
                event.consume(true);
            }
            if (window.DEBUG && event.altKey) {
                WebInspector.reload();
                return;
            }
            break;
        case "F5":
            if (!WebInspector.isMac()) {
                PageAgent.reload(event.ctrlKey || event.shiftKey);
                event.consume(true);
            }
            break;
    }

    var isValidZoomShortcut = WebInspector.KeyboardShortcut.eventHasCtrlOrMeta(event) &&
        !event.altKey &&
        !InspectorFrontendHost.isStub;
    switch (event.keyCode) {
        case 107: // +
        case 187: // +
            if (isValidZoomShortcut) {
                WebInspector._zoomIn();
                event.consume(true);
            }
            break;
        case 109: // -
        case 189: // -
            if (isValidZoomShortcut) {
                WebInspector._zoomOut();
                event.consume(true);
            }
            break;
        case 48: // 0
            // Zoom reset shortcut does not allow "Shift" when handled by the browser.
            if (isValidZoomShortcut && !event.shiftKey) {
                WebInspector._resetZoom();
                event.consume(true);
            }
            break;
    }
}

WebInspector.postDocumentKeyDown = function(event)
{
    if (event.handled)
        return;

    if (event.keyCode === WebInspector.KeyboardShortcut.Keys.Esc.code) {
        // If drawer is open with some view other than console then close it.
        if (!this._toggleConsoleButton.toggled && WebInspector.drawer.visible)
            this.closeViewInDrawer();
        else
            this._toggleConsoleButtonClicked();
    }
}

WebInspector.documentCanCopy = function(event)
{
    if (WebInspector.inspectorView.currentPanel() && WebInspector.inspectorView.currentPanel().handleCopyEvent)
        event.preventDefault();
}

WebInspector.documentCopy = function(event)
{
    if (WebInspector.inspectorView.currentPanel() && WebInspector.inspectorView.currentPanel().handleCopyEvent)
        WebInspector.inspectorView.currentPanel().handleCopyEvent(event);
    WebInspector.documentCopyEventFired(event);
}

WebInspector.documentCopyEventFired = function(event)
{
}

WebInspector.contextMenuEventFired = function(event)
{
    if (event.handled || event.target.hasStyleClass("popup-glasspane"))
        event.preventDefault();
}

WebInspector.showConsole = function()
{
    if (WebInspector._toggleConsoleButton && !WebInspector._toggleConsoleButton.toggled) {
        if (WebInspector.drawer.visible)
            this._closePreviousDrawerView();
        WebInspector._toggleConsoleButtonClicked();
    }
}

WebInspector.showPanel = function(panel)
{
    return WebInspector.inspectorView.showPanel(panel);
}

WebInspector.panel = function(panel)
{
    return WebInspector.inspectorView.panel(panel);
}

WebInspector.bringToFront = function()
{
    InspectorFrontendHost.bringToFront();
}

/**
 * @param {string=} messageLevel
 * @param {boolean=} showConsole
 */
WebInspector.log = function(message, messageLevel, showConsole)
{
    // remember 'this' for setInterval() callback
    var self = this;

    // return indication if we can actually log a message
    function isLogAvailable()
    {
        return WebInspector.ConsoleMessage && WebInspector.RemoteObject && self.console;
    }

    // flush the queue of pending messages
    function flushQueue()
    {
        var queued = WebInspector.log.queued;
        if (!queued)
            return;

        for (var i = 0; i < queued.length; ++i)
            logMessage(queued[i]);

        delete WebInspector.log.queued;
    }

    // flush the queue if it console is available
    // - this function is run on an interval
    function flushQueueIfAvailable()
    {
        if (!isLogAvailable())
            return;

        clearInterval(WebInspector.log.interval);
        delete WebInspector.log.interval;

        flushQueue();
    }

    // actually log the message
    function logMessage(message)
    {
        // post the message
        var msg = WebInspector.ConsoleMessage.create(
            WebInspector.ConsoleMessage.MessageSource.Other,
            messageLevel || WebInspector.ConsoleMessage.MessageLevel.Debug,
            message);

        self.console.addMessage(msg);
        if (showConsole)
            WebInspector.showConsole();
    }

    // if we can't log the message, queue it
    if (!isLogAvailable()) {
        if (!WebInspector.log.queued)
            WebInspector.log.queued = [];

        WebInspector.log.queued.push(message);

        if (!WebInspector.log.interval)
            WebInspector.log.interval = setInterval(flushQueueIfAvailable, 1000);

        return;
    }

    // flush the pending queue if any
    flushQueue();

    // log the message
    logMessage(message);
}

WebInspector.showErrorMessage = function(error)
{
    WebInspector.log(error, WebInspector.ConsoleMessage.MessageLevel.Error, true);
}

// Inspector.inspect protocol event
WebInspector.inspect = function(payload, hints)
{
    var object = WebInspector.RemoteObject.fromPayload(payload);
    if (object.subtype === "node") {
        function callback(nodeId)
        {
            WebInspector._updateFocusedNode(nodeId);
            object.release();
        }
        object.pushNodeToFrontend(callback);
        return;
    }

    if (hints.databaseId)
        WebInspector.showPanel("resources").selectDatabase(WebInspector.databaseModel.databaseForId(hints.databaseId));
    else if (hints.domStorageId)
        WebInspector.showPanel("resources").selectDOMStorage(WebInspector.domStorageModel.storageForId(hints.domStorageId));

    object.release();
}

// Inspector.detached protocol event
WebInspector.detached = function(reason)
{
    WebInspector.socket._detachReason = reason;
    (new WebInspector.RemoteDebuggingTerminatedScreen(reason)).showModal();
}

WebInspector.targetCrashed = function()
{
    (new WebInspector.HelpScreenUntilReload(
        WebInspector.UIString("Inspected target crashed"),
        WebInspector.UIString("Inspected target has crashed. Once it reloads we will attach to it automatically."))).showModal();
}

WebInspector._updateFocusedNode = function(nodeId)
{
    if (WebInspector.inspectElementModeController && WebInspector.inspectElementModeController.enabled()) {
        InspectorFrontendHost.bringToFront();
        WebInspector.inspectElementModeController.disable();
    }
    WebInspector.showPanel("elements").revealAndSelectNode(nodeId);
}

WebInspector._showAnchorLocation = function(anchor)
{
    if (WebInspector.openAnchorLocationRegistry.dispatch({ url: anchor.href, lineNumber: anchor.lineNumber}))
        return true;
    var preferredPanel = this.panels[anchor.preferredPanel];
    if (preferredPanel && WebInspector._showAnchorLocationInPanel(anchor, preferredPanel))
        return true;
    if (WebInspector._showAnchorLocationInPanel(anchor, this.panel("scripts")))
        return true;
    if (WebInspector._showAnchorLocationInPanel(anchor, this.panel("resources")))
        return true;
    if (WebInspector._showAnchorLocationInPanel(anchor, this.panel("network")))
        return true;
    return false;
}

WebInspector._showAnchorLocationInPanel = function(anchor, panel)
{
    if (!panel || !panel.canShowAnchorLocation(anchor))
        return false;

    // FIXME: support webkit-html-external-link links here.
    if (anchor.hasStyleClass("webkit-html-external-link")) {
        anchor.removeStyleClass("webkit-html-external-link");
        anchor.addStyleClass("webkit-html-resource-link");
    }

    WebInspector.inspectorView.showPanelForAnchorNavigation(panel);
    panel.showAnchorLocation(anchor);
    return true;
}

WebInspector.evaluateInConsole = function(expression, showResultOnly)
{
    this.showConsole();
    this.consoleView.evaluateUsingTextPrompt(expression, showResultOnly);
}

WebInspector.addMainEventListeners = function(doc)
{
    doc.addEventListener("keydown", this.documentKeyDown.bind(this), true);
    doc.addEventListener("keydown", this.postDocumentKeyDown.bind(this), false);
    doc.addEventListener("beforecopy", this.documentCanCopy.bind(this), true);
    doc.addEventListener("copy", this.documentCopy.bind(this), false);
    doc.addEventListener("contextmenu", this.contextMenuEventFired.bind(this), true);
    doc.addEventListener("click", this.documentClick.bind(this), true);
}

WebInspector.Zoom = {
    Table: [0.25, 0.33, 0.5, 0.66, 0.75, 0.9, 1, 1.1, 1.25, 1.5, 1.75, 2, 2.5, 3, 4, 5],
    DefaultOffset: 6
}
