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

WebInspector.Notification = {
    GlobalModifierKeysDidChange: "global-modifiers-did-change"
};

WebInspector.loaded = function()
{
    // Tell the InspectorFrontendHost we loaded first to establish communication with InspectorBackend.
    InspectorFrontendHost.loaded();

    // Initialize WebSocket to communication
    this._initializeWebSocketIfNeeded();

    // Register observers for events from the InspectorBackend.
    InspectorBackend.registerInspectorDispatcher(new WebInspector.InspectorObserver);
    InspectorBackend.registerPageDispatcher(new WebInspector.PageObserver);
    InspectorBackend.registerConsoleDispatcher(new WebInspector.ConsoleObserver);
    InspectorBackend.registerNetworkDispatcher(new WebInspector.NetworkObserver);
    InspectorBackend.registerDOMDispatcher(new WebInspector.DOMObserver);
    InspectorBackend.registerDebuggerDispatcher(new WebInspector.DebuggerObserver);
    InspectorBackend.registerDatabaseDispatcher(new WebInspector.DatabaseObserver);
    InspectorBackend.registerDOMStorageDispatcher(new WebInspector.DOMStorageObserver);
    InspectorBackend.registerApplicationCacheDispatcher(new WebInspector.ApplicationCacheObserver);
    InspectorBackend.registerTimelineDispatcher(new WebInspector.TimelineObserver);
    InspectorBackend.registerProfilerDispatcher(new WebInspector.ProfilerObserver);
    InspectorBackend.registerCSSDispatcher(new WebInspector.CSSObserver);
    if (InspectorBackend.registerLayerTreeDispatcher)
        InspectorBackend.registerLayerTreeDispatcher(new WebInspector.LayerTreeObserver);
    if (InspectorBackend.registerRuntimeDispatcher)
        InspectorBackend.registerRuntimeDispatcher(new WebInspector.RuntimeObserver);

    // Enable agents.
    InspectorAgent.enable();

    // Perform one-time tasks.
    WebInspector.CSSCompletions.requestCSSNameCompletions();
    this._generateDisclosureTriangleImages();

    // Create the singleton managers next, before the user interface elements, so the user interface can register
    // as event listeners on these managers.
    this.branchManager = new WebInspector.BranchManager;
    this.frameResourceManager = new WebInspector.FrameResourceManager;
    this.storageManager = new WebInspector.StorageManager;
    this.domTreeManager = new WebInspector.DOMTreeManager;
    this.cssStyleManager = new WebInspector.CSSStyleManager;
    this.logManager = new WebInspector.LogManager;
    this.issueManager = new WebInspector.IssueManager;
    this.applicationCacheManager = new WebInspector.ApplicationCacheManager;
    this.timelineManager = new WebInspector.TimelineManager;
    this.profileManager = new WebInspector.ProfileManager;
    this.debuggerManager = new WebInspector.DebuggerManager;
    this.sourceMapManager = new WebInspector.SourceMapManager;
    this.layerTreeManager = new WebInspector.LayerTreeManager;
    this.dashboardManager = new WebInspector.DashboardManager;

    // Enable the Console Agent after creating the singleton managers.
    ConsoleAgent.enable();

    // Enable the RuntimeAgent to receive notification of execution contexts.
    if (RuntimeAgent.enable)
        RuntimeAgent.enable();

    // Register for events.
    this.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.Paused, this._debuggerDidPause, this);
    this.domTreeManager.addEventListener(WebInspector.DOMTreeManager.Event.InspectModeStateChanged, this._inspectModeStateChanged, this);
    this.domTreeManager.addEventListener(WebInspector.DOMTreeManager.Event.DOMNodeWasInspected, this._domNodeWasInspected, this);
    this.frameResourceManager.addEventListener(WebInspector.FrameResourceManager.Event.MainFrameDidChange, this._mainFrameDidChange, this);

    WebInspector.Frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainResourceDidChange, this);

    document.addEventListener("DOMContentLoaded", this.contentLoaded.bind(this));

    document.addEventListener("beforecopy", this._beforecopy.bind(this));
    document.addEventListener("copy", this._copy.bind(this));

    document.addEventListener("click", this._mouseWasClicked.bind(this));
    document.addEventListener("dragover", this._dragOver.bind(this));
    document.addEventListener("focus", WebInspector._focusChanged.bind(this), true);

    window.addEventListener("focus", this._windowFocused.bind(this));
    window.addEventListener("blur", this._windowBlurred.bind(this));
    window.addEventListener("resize", this._windowResized.bind(this));
    window.addEventListener("keydown", this._windowKeyDown.bind(this));
    window.addEventListener("keyup", this._windowKeyUp.bind(this));

    // Create settings.
    this._lastSelectedNavigationSidebarPanelSetting = new WebInspector.Setting("last-selected-navigation-sidebar-panel", "resource");
    this._navigationSidebarCollapsedSetting = new WebInspector.Setting("navigation-sidebar-collapsed", false);
    this._navigationSidebarWidthSetting = new WebInspector.Setting("navigation-sidebar-width", null);

    this._lastSelectedDetailsSidebarPanelSetting = new WebInspector.Setting("last-selected-details-sidebar-panel", null);
    this._detailsSidebarCollapsedSetting = new WebInspector.Setting("details-sidebar-collapsed", true);
    this._detailsSidebarWidthSetting = new WebInspector.Setting("details-sidebar-width", null);

    this._lastContentViewResponsibleSidebarPanelSetting = new WebInspector.Setting("last-content-view-responsible-sidebar-panel", "resource");
    this._lastContentCookieSetting = new WebInspector.Setting("last-content-view-cookie", {});

    this._toolbarDockedRightDisplayModeSetting = new WebInspector.Setting("toolbar-docked-right-display-mode", WebInspector.Toolbar.DisplayMode.IconAndLabelVertical);
    this._toolbarDockedRightSizeModeSetting = new WebInspector.Setting("toolbar-docked-right-size-mode",WebInspector.Toolbar.SizeMode.Normal);

    this._toolbarDockedBottomDisplayModeSetting = new WebInspector.Setting("toolbar-docked-display-mode", WebInspector.Toolbar.DisplayMode.IconAndLabelHorizontal);
    this._toolbarDockedBottomSizeModeSetting = new WebInspector.Setting("toolbar-docked-size-mode",WebInspector.Toolbar.SizeMode.Small);

    this._toolbarUndockedDisplayModeSetting = new WebInspector.Setting("toolbar-undocked-display-mode", WebInspector.Toolbar.DisplayMode.IconAndLabelVertical);
    this._toolbarUndockedSizeModeSetting = new WebInspector.Setting("toolbar-undocked-size-mode",WebInspector.Toolbar.SizeMode.Normal);

    this._showingSplitConsoleSetting = new WebInspector.Setting("showing-split-console", false);
    this._splitConsoleHeightSetting = new WebInspector.Setting("split-console-height", 150);

    this._dockButtonToggledSetting = new WebInspector.Setting("dock-button-toggled", false);

    this.showShadowDOMSetting = new WebInspector.Setting("show-shadow-dom", false);
}

WebInspector.contentLoaded = function()
{
    // Check for a nightly build by looking for a plus in the version number and a small number of stylesheets (indicating combined resources).
    var versionMatch = / AppleWebKit\/([^ ]+)/.exec(navigator.userAgent);
    if (versionMatch && versionMatch[1].indexOf("+") !== -1 && document.styleSheets.length < 10)
        document.body.classList.add("nightly-build");

    // Create the user interface elements.
    this.toolbar = new WebInspector.Toolbar(document.getElementById("toolbar"));
    this.toolbar.addEventListener(WebInspector.Toolbar.Event.DisplayModeDidChange, this._toolbarDisplayModeDidChange, this);
    this.toolbar.addEventListener(WebInspector.Toolbar.Event.SizeModeDidChange, this._toolbarSizeModeDidChange, this);
    
    var contentElement = document.getElementById("content");
    contentElement.setAttribute("role", "main");
    contentElement.setAttribute("aria-label", WebInspector.UIString("Content"));

    this.contentBrowser = new WebInspector.ContentBrowser(document.getElementById("content-browser"), this);
    this.contentBrowser.addEventListener(WebInspector.ContentBrowser.Event.CurrentRepresentedObjectsDidChange, this._contentBrowserRepresentedObjectsDidChange, this);
    this.contentBrowser.addEventListener(WebInspector.ContentBrowser.Event.CurrentContentViewDidChange, this._contentBrowserCurrentContentViewDidChange, this);

    this.splitContentBrowser = new WebInspector.ContentBrowser(document.getElementById("split-content-browser"), this, true);
    this.splitContentBrowser.navigationBar.element.addEventListener("mousedown", this._consoleResizerMouseDown.bind(this));

    this.quickConsole = new WebInspector.QuickConsole(document.getElementById("quick-console"));
    this.quickConsole.addEventListener(WebInspector.QuickConsole.Event.DidResize, this._quickConsoleDidResize, this);

    this._consoleRepresentedObject = new WebInspector.LogObject;
    this._consoleTreeElement = new WebInspector.LogTreeElement(this._consoleRepresentedObject);
    this.consoleContentView = WebInspector.contentBrowser.contentViewForRepresentedObject(this._consoleRepresentedObject);

    // FIXME: The sidebars should be flipped in RTL languages.
    this.leftSidebar = this.navigationSidebar = new WebInspector.Sidebar(document.getElementById("navigation-sidebar"), WebInspector.Sidebar.Sides.Left);
    this.navigationSidebar.addEventListener(WebInspector.Sidebar.Event.CollapsedStateDidChange, this._sidebarCollapsedStateDidChange, this);
    this.navigationSidebar.addEventListener(WebInspector.Sidebar.Event.WidthDidChange, this._sidebarWidthDidChange, this);
    this.navigationSidebar.addEventListener(WebInspector.Sidebar.Event.SidebarPanelSelected, this._navigationSidebarPanelSelected, this);

    this.rightSidebar = this.detailsSidebar = new WebInspector.Sidebar(document.getElementById("details-sidebar"), WebInspector.Sidebar.Sides.Right, null, null, WebInspector.UIString("Details"));
    this.detailsSidebar.addEventListener(WebInspector.Sidebar.Event.CollapsedStateDidChange, this._sidebarCollapsedStateDidChange, this);
    this.detailsSidebar.addEventListener(WebInspector.Sidebar.Event.WidthDidChange, this._sidebarWidthDidChange, this);
    this.detailsSidebar.addEventListener(WebInspector.Sidebar.Event.SidebarPanelSelected, this._detailsSidebarPanelSelected, this);

    this._reloadPageKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Command, "R", this._reloadPage.bind(this));
    this._reloadPageIgnoringCacheKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Command | WebInspector.KeyboardShortcut.Modifier.Shift, "R", this._reloadPageIgnoringCache.bind(this));

    this._inspectModeKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Command | WebInspector.KeyboardShortcut.Modifier.Shift, "C", this._toggleInspectMode.bind(this));

    this._undoKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Command, "Z", this._undoKeyboardShortcut.bind(this));
    this._redoKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Command | WebInspector.KeyboardShortcut.Modifier.Shift, "Z", this._redoKeyboardShortcut.bind(this));
    this._undoKeyboardShortcut.implicitlyPreventsDefault = this._redoKeyboardShortcut.implicitlyPreventsDefault = false;

    this.undockButtonNavigationItem = new WebInspector.ToggleControlToolbarItem("undock", WebInspector.UIString("Detach into separate window"), "", "Images/Undock.pdf", "", 16, 14);
    this.undockButtonNavigationItem.addEventListener(WebInspector.ButtonNavigationItem.Event.Clicked, this._undock, this);

    this.closeButtonNavigationItem = new WebInspector.ControlToolbarItem("dock-close", WebInspector.UIString("Close"), "Images/Close.pdf", 16, 14);
    this.closeButtonNavigationItem.addEventListener(WebInspector.ButtonNavigationItem.Event.Clicked, this.close, this);

    this.toolbar.addToolbarItem(this.closeButtonNavigationItem, WebInspector.Toolbar.Section.Control);
    this.toolbar.addToolbarItem(this.undockButtonNavigationItem, WebInspector.Toolbar.Section.Control);

    this.resourceSidebarPanel = new WebInspector.ResourceSidebarPanel;
    this.instrumentSidebarPanel = new WebInspector.InstrumentSidebarPanel;
    this.debuggerSidebarPanel = new WebInspector.DebuggerSidebarPanel;

    this.navigationSidebar.addSidebarPanel(this.resourceSidebarPanel);
    this.navigationSidebar.addSidebarPanel(this.instrumentSidebarPanel);
    this.navigationSidebar.addSidebarPanel(this.debuggerSidebarPanel);

    this.toolbar.addToolbarItem(this.resourceSidebarPanel.toolbarItem, WebInspector.Toolbar.Section.Left);
    this.toolbar.addToolbarItem(this.instrumentSidebarPanel.toolbarItem, WebInspector.Toolbar.Section.Left);
    this.toolbar.addToolbarItem(this.debuggerSidebarPanel.toolbarItem, WebInspector.Toolbar.Section.Left);

    // The toolbar button for the console.
    const consoleKeyboardShortcut = "\u2325\u2318C"; // Option-Command-C
    var toolTip = WebInspector.UIString("Show console (%s)").format(consoleKeyboardShortcut);
    var activatedToolTip = WebInspector.UIString("Hide console");
    this._consoleToolbarButton = new WebInspector.ActivateButtonToolbarItem("console", toolTip, activatedToolTip, WebInspector.UIString("Console"), "Images/NavigationItemLog.pdf");
    this._consoleToolbarButton.addEventListener(WebInspector.ButtonNavigationItem.Event.Clicked, this.toggleConsoleView, this);
    this.toolbar.addToolbarItem(this._consoleToolbarButton, WebInspector.Toolbar.Section.Center);

    this.toolbar.addToolbarItem(this.dashboardManager.toolbarItem, WebInspector.Toolbar.Section.Center);

    // The toolbar button for node inspection.
    var toolTip = WebInspector.UIString("Enable point to inspect mode (%s)").format(WebInspector._inspectModeKeyboardShortcut.displayName);
    var activatedToolTip = WebInspector.UIString("Disable point to inspect mode (%s)").format(WebInspector._inspectModeKeyboardShortcut.displayName);
    this._inspectModeToolbarButton = new WebInspector.ActivateButtonToolbarItem("inspect", toolTip, activatedToolTip, WebInspector.UIString("Inspect"), "Images/Crosshair.pdf");
    this._inspectModeToolbarButton.addEventListener(WebInspector.ButtonNavigationItem.Event.Clicked, this._toggleInspectMode, this);
    this.toolbar.addToolbarItem(this._inspectModeToolbarButton, WebInspector.Toolbar.Section.Center);

    this.resourceDetailsSidebarPanel = new WebInspector.ResourceDetailsSidebarPanel;
    this.domNodeDetailsSidebarPanel = new WebInspector.DOMNodeDetailsSidebarPanel;
    this.cssStyleDetailsSidebarPanel = new WebInspector.CSSStyleDetailsSidebarPanel;
    this.applicationCacheDetailsSidebarPanel = new WebInspector.ApplicationCacheDetailsSidebarPanel;
    this.scopeChainDetailsSidebarPanel = new WebInspector.ScopeChainDetailsSidebarPanel;

    this.detailsSidebarPanels = [this.resourceDetailsSidebarPanel, this.applicationCacheDetailsSidebarPanel, this.scopeChainDetailsSidebarPanel,
        this.domNodeDetailsSidebarPanel, this.cssStyleDetailsSidebarPanel];

    if (window.LayerTreeAgent) {
        this.layerTreeSidebarPanel = new WebInspector.LayerTreeSidebarPanel;
        this.detailsSidebarPanels.splice(this.detailsSidebarPanels.length - 1, 0, this.layerTreeSidebarPanel);
    }

    this.modifierKeys = {altKey: false, metaKey: false, shiftKey: false};

    // Add the items in reverse order since the last items appear and disappear the least. So they
    // will not cause the other buttons to visually shift around, keeping things more stable.
    for (var i = this.detailsSidebarPanels.length - 1; i >= 0; --i) {
        var toolbarItem = this.detailsSidebarPanels[i].toolbarItem;
        toolbarItem.hidden = true;
        this.toolbar.addToolbarItem(toolbarItem, WebInspector.Toolbar.Section.Right);
    }

    this.toolbar.element.addEventListener("mousedown", this._toolbarMouseDown.bind(this));
    document.getElementById("docked-resizer").addEventListener("mousedown", this._dockedResizerMouseDown.bind(this));

    this._updateToolbarHeight();

    this.navigationSidebar.selectedSidebarPanel = this._lastSelectedNavigationSidebarPanelSetting.value;

    if (this._navigationSidebarWidthSetting.value)
        this.navigationSidebar.width = this._navigationSidebarWidthSetting.value;

    if (this._detailsSidebarWidthSetting.value)
        this.detailsSidebar.width = this._detailsSidebarWidthSetting.value;

    // Update the docked state based on the query string passed when the Web Inspector was loaded.
    this.updateDockedState(parseLocationQueryParameters().dockSide || "undocked");

    // Tell the frontend API we are loaded so any pending frontend commands can be dispatched.
    InspectorFrontendAPI.loadCompleted();

    // Set collapsed after loading the pending frontend commands are dispatched so only the final
    // selected sidebar panel gets shown and has a say in what content view gets shown.
    this.navigationSidebar.collapsed = this._navigationSidebarCollapsedSetting.value;

    // If InspectorFrontendAPI didn't show a content view, then try to show the last content view.
    if (!this.contentBrowser.currentContentView && !this.ignoreLastContentCookie) {
        if (this._lastContentCookieSetting.value === "console") {
            // The console does not have a sidebar, so handle its special cookie here.
            this.showFullHeightConsole();
        } else {
            var responsibleSidebarPanel = this.navigationSidebar.findSidebarPanel(this._lastContentViewResponsibleSidebarPanelSetting.value);
            if (responsibleSidebarPanel)
                responsibleSidebarPanel.showContentViewForCookie(this._lastContentCookieSetting.value);
        }
    }

    this._updateSplitConsoleHeight(this._splitConsoleHeightSetting.value);

    if (this._showingSplitConsoleSetting.value)
        this.showSplitConsole();
}

WebInspector.messagesToDispatch = [];

WebInspector.dispatchNextQueuedMessageFromBackend = function()
{
    for (var i = 0; i < this.messagesToDispatch.length; ++i)
        InspectorBackend.dispatch(this.messagesToDispatch[i]);

    this.messagesToDispatch = [];

    this._dispatchTimeout = null;
}

WebInspector.dispatchMessageFromBackend = function(message)
{
    // Enforce asynchronous interaction between the backend and the frontend by queueing messages.
    // The messages are dequeued on a zero delay timeout.

    this.messagesToDispatch.push(message);

    if (this._dispatchTimeout)
        return;

    this._dispatchTimeout = setTimeout(this.dispatchNextQueuedMessageFromBackend.bind(this), 0);
}

WebInspector.sidebarPanelForCurrentContentView = function()
{
    var currentContentView = this.contentBrowser.currentContentView;
    if (!currentContentView)
        return null;
    return this.sidebarPanelForRepresentedObject(currentContentView.representedObject);
}

WebInspector.sidebarPanelForRepresentedObject = function(representedObject)
{
    if (representedObject instanceof WebInspector.Frame || representedObject instanceof WebInspector.Resource ||
        representedObject instanceof WebInspector.Script)
        return this.resourceSidebarPanel;

    if (representedObject instanceof WebInspector.DOMStorageObject || representedObject instanceof WebInspector.CookieStorageObject ||
        representedObject instanceof WebInspector.DatabaseTableObject || representedObject instanceof WebInspector.DatabaseObject ||
        representedObject instanceof WebInspector.ApplicationCacheFrame)
        return this.resourceSidebarPanel;

    // The console does not have a sidebar.
    if (representedObject instanceof WebInspector.LogObject)
        return null;

    if (representedObject instanceof WebInspector.TimelinesObject || representedObject instanceof WebInspector.ProfileObject)
        return this.instrumentSidebarPanel;

    console.error("Unknown representedObject: ", representedObject);
    return null;
}

WebInspector.contentBrowserTreeElementForRepresentedObject = function(contentBrowser, representedObject)
{
    // The console does not have a sidebar, so return a tree element here so something is shown.
    if (representedObject instanceof WebInspector.LogObject)
        return this._consoleTreeElement;

    var sidebarPanel = this.sidebarPanelForRepresentedObject(representedObject);
    if (sidebarPanel)
        return sidebarPanel.treeElementForRepresentedObject(representedObject);
    return null;
}

WebInspector.displayNameForURL = function(url, urlComponents)
{
    if (!urlComponents)
        urlComponents = parseURL(url);

    var displayName;
    try {
        displayName = decodeURIComponent(urlComponents.lastPathComponent || "");
    } catch (e) {
        displayName = urlComponents.lastPathComponent;
    }

    return displayName || WebInspector.displayNameForHost(urlComponents.host) || url;
}

WebInspector.displayNameForHost = function(host)
{
    // FIXME <rdar://problem/11237413>: This should decode punycode hostnames.
    return host;
}

WebInspector.updateWindowTitle = function()
{
    var mainFrame = this.frameResourceManager.mainFrame;
    console.assert(mainFrame);

    var urlComponents = mainFrame.mainResource.urlComponents;

    var lastPathComponent;
    try {
        lastPathComponent = decodeURIComponent(urlComponents.lastPathComponent || "");
    } catch (e) {
        lastPathComponent = urlComponents.lastPathComponent;
    }

    // Build a title based on the URL components.
    if (urlComponents.host && lastPathComponent)
        var title = this.displayNameForHost(urlComponents.host) + " \u2014 " + lastPathComponent;
    else if (urlComponents.host)
        var title = this.displayNameForHost(urlComponents.host);
    else if (lastPathComponent)
        var title = lastPathComponent;
    else
        var title = mainFrame.url;

    // The name "inspectedURLChanged" sounds like the whole URL is required, however this is only
    // used for updating the window title and it can be any string.
    InspectorFrontendHost.inspectedURLChanged(title);
}

WebInspector.updateDockedState = function(side)
{
    if (this._dockSide === side)
        return;

    this._dockSide = side;

    this.docked = side !== "undocked";

    this._ignoreToolbarModeDidChangeEvents = true;

    if (side === "bottom") {
        document.body.classList.add("docked");
        document.body.classList.add("bottom");

        document.body.classList.remove("window-inactive");
        document.body.classList.remove("right");

        this.toolbar.displayMode = this._toolbarDockedBottomDisplayModeSetting.value;
        this.toolbar.sizeMode = this._toolbarDockedBottomSizeModeSetting.value;
    } else if (side === "right") {
        document.body.classList.add("docked");
        document.body.classList.add("right");

        document.body.classList.remove("window-inactive");
        document.body.classList.remove("bottom");

        this.toolbar.displayMode = this._toolbarDockedRightDisplayModeSetting.value;
        this.toolbar.sizeMode = this._toolbarDockedRightSizeModeSetting.value;
    } else {
        document.body.classList.remove("docked");
        document.body.classList.remove("right");
        document.body.classList.remove("bottom");

        this.toolbar.displayMode = this._toolbarUndockedDisplayModeSetting.value;
        this.toolbar.sizeMode = this._toolbarUndockedSizeModeSetting.value;
    }

    this._ignoreToolbarModeDidChangeEvents = false;

    this._updateDockNavigationItems();
    this._updateToolbarHeight();
}

WebInspector.handlePossibleLinkClick = function(event, frame, alwaysOpenExternally)
{
    var anchorElement = event.target.enclosingNodeOrSelfWithNodeName("a");
    if (!anchorElement || !anchorElement.href)
        return false;

    if (WebInspector.isBeingEdited(anchorElement)) {
        // Don't follow the link when it is being edited.
        return false;
    }

    // Prevent the link from navigating, since we don't do any navigation by following links normally.
    event.preventDefault();
    event.stopPropagation();

    this.openURL(anchorElement.href, frame, false, anchorElement.lineNumber);

    return true;
}

WebInspector.openURL = function(url, frame, alwaysOpenExternally, lineNumber)
{
    console.assert(url);
    if (!url)
        return;

    // If alwaysOpenExternally is not defined, base it off the command/meta key for the current event.
    if (alwaysOpenExternally === undefined || alwaysOpenExternally === null)
        alwaysOpenExternally = window.event ? window.event.metaKey : false;

    if (alwaysOpenExternally) {
        InspectorFrontendHost.openInNewTab(url);
        return;
    }
    
    var parsedURL = parseURL(url);
    if (parsedURL.scheme === WebInspector.ProfileType.ProfileScheme) {
        var profileType = parsedURL.host.toUpperCase();
        var profileTitle = parsedURL.path;
        
        // The path of of the profile URL starts with a slash, remove it, so 
        // we can get the actual title.
        console.assert(profileTitle[0] === '/');
        profileTitle = profileTitle.substring(1);

        this.instrumentSidebarPanel.showProfile(profileType, profileTitle);
        return;
    }

    var searchChildFrames = false;
    if (!frame) {
        frame = this.frameResourceManager.mainFrame;
        searchChildFrames = true;
    }

    console.assert(frame);

    // WebInspector.Frame.resourceForURL does not check the main resource, only sub-resources. So check both.
    var resource = frame.url === url ? frame.mainResource : frame.resourceForURL(url, searchChildFrames);
    if (resource) {
        this.resourceSidebarPanel.showSourceCode(resource, lineNumber);
        return;
    }

    InspectorFrontendHost.openInNewTab(url);
}

WebInspector.close = function()
{
    if (this._isClosing)
        return;

    this._isClosing = true;

    InspectorFrontendHost.closeWindow();
}

WebInspector.isConsoleFocused = function()
{
    return this.quickConsole.prompt.focused;
}

WebInspector.isShowingSplitConsole = function()
{
    return !this.splitContentBrowser.element.classList.contains("hidden");
}

WebInspector.currentViewSupportsSplitContentBrowser = function()
{
    var currentContentView = this.contentBrowser.currentContentView;
    return !currentContentView || currentContentView.supportsSplitContentBrowser;
}

WebInspector.toggleSplitConsole = function()
{
    if (!this.currentViewSupportsSplitContentBrowser()) {
        this.toggleConsoleView();
        return;
    }

    if (this.isShowingSplitConsole())
        this.hideSplitConsole();
    else
        this.showSplitConsole();
}

WebInspector.showSplitConsole = function()
{
    if (!this.currentViewSupportsSplitContentBrowser()) {
        this.showFullHeightConsole();
        return;
    }

    this.splitContentBrowser.element.classList.remove("hidden");

    this._showingSplitConsoleSetting.value = true;

    if (this.splitContentBrowser.currentContentView !== this.consoleContentView) {
        // Be sure to close any existing log view in the main content browser before showing it in the
        // split content browser. We can only show a content view in one browser at a time.
        this.contentBrowser.contentViewContainer.closeAllContentViewsOfPrototype(WebInspector.LogContentView);
        this.splitContentBrowser.showContentView(this.consoleContentView);
    } else {
        // This causes the view to know it was shown and focus the prompt.
        this.splitContentBrowser.contentViewContainer.shown();
    }

    if (this._wasShowingNavigationSidebarBeforeFullHeightConsole)
        this.navigationSidebar.collapsed = false;

    this.quickConsole.consoleLogVisibilityChanged(true);
}

WebInspector.hideSplitConsole = function()
{
    this.splitContentBrowser.element.classList.add("hidden");

    this._showingSplitConsoleSetting.value = false;

    // This causes the view to know it was hidden.
    this.splitContentBrowser.contentViewContainer.hidden();

    this.quickConsole.consoleLogVisibilityChanged(false);
}

WebInspector.showFullHeightConsole = function(scope)
{
    this.splitContentBrowser.element.classList.add("hidden");

    this._showingSplitConsoleSetting.value = false;

    scope = scope || WebInspector.LogContentView.Scopes.All;

    // If the requested scope is already selected and the console is showing, then switch back to All.
    if (this.isShowingConsoleView() && this.consoleContentView.scopeBar.item(scope).selected)
        scope = WebInspector.LogContentView.Scopes.All;

    this.consoleContentView.scopeBar.item(scope).selected = true;

    if (this.contentBrowser.currentContentView !== this.consoleContentView) {
        this._wasShowingNavigationSidebarBeforeFullHeightConsole = !this.navigationSidebar.collapsed;

        // Collapse the sidebar before showing the console view, so the check for the collapsed state in
        // _revealAndSelectRepresentedObjectInNavigationSidebar returns early and does not deselect any
        // tree elements in the current sidebar.
        this.navigationSidebar.collapsed = true;

        // Be sure to close any existing log view in the split content browser before showing it in the
        // main content browser. We can only show a content view in one browser at a time.
        this.splitContentBrowser.contentViewContainer.closeAllContentViewsOfPrototype(WebInspector.LogContentView);
        this.contentBrowser.showContentView(this.consoleContentView);
    }

    console.assert(this.isShowingConsoleView());
    console.assert(this._consoleToolbarButton.activated);

    this.quickConsole.consoleLogVisibilityChanged(true);
}

WebInspector.isShowingConsoleView = function()
{
    return this.contentBrowser.currentContentView instanceof WebInspector.LogContentView;
}

WebInspector.showConsoleView = function(scope)
{
    this.showFullHeightConsole(scope);
}

WebInspector.toggleConsoleView = function()
{
    if (this.isShowingConsoleView()) {
        if (this.contentBrowser.canGoBack())
            this.contentBrowser.goBack();
        else
            this.resourceSidebarPanel.showMainFrameSourceCode();

        if (this._wasShowingNavigationSidebarBeforeFullHeightConsole)
            this.navigationSidebar.collapsed = false;
    } else
        this.showFullHeightConsole();
}

WebInspector.UIString = function(string, vararg)
{
    if (window.localizedStrings && string in window.localizedStrings)
        return window.localizedStrings[string];

    if (!this._missingLocalizedStrings)
        this._missingLocalizedStrings = {};

    if (!(string in this._missingLocalizedStrings)) {
        console.error("Localized string \"" + string + "\" was not found.");
        this._missingLocalizedStrings[string] = true;
    }

    return "LOCALIZED STRING NOT FOUND";
}

WebInspector.restoreFocusFromElement = function(element)
{
    if (element && element.isSelfOrAncestor(this.currentFocusElement))
        this.previousFocusElement.focus();
}

WebInspector._focusChanged = function(event)
{
    // Make a caret selection inside the focused element if there isn't a range selection and there isn't already
    // a caret selection inside. This is needed (at least) to remove caret from console when focus is moved.
    // The selection change should not apply to text fields and text areas either.

    if (WebInspector.isEventTargetAnEditableField(event))
        return;

    var selection = window.getSelection();
    if (!selection.isCollapsed)
        return;

    var element = event.target;

    if (element !== this.currentFocusElement) {
        this.previousFocusElement = this.currentFocusElement;
        this.currentFocusElement = element;
    }

    if (element.isInsertionCaretInside())
        return;

    var selectionRange = element.ownerDocument.createRange();
    selectionRange.setStart(element, 0);
    selectionRange.setEnd(element, 0);

    selection.removeAllRanges();
    selection.addRange(selectionRange);
}

WebInspector._mouseWasClicked = function(event)
{
    this.handlePossibleLinkClick(event);
}

WebInspector._dragOver = function(event)
{
    // Do nothing if another event listener handled the event already.
    if (event.defaultPrevented)
        return;

    // Allow dropping into editable areas.
    if (WebInspector.isEventTargetAnEditableField(event))
        return;

    // Prevent the drop from being accepted.
    event.dataTransfer.dropEffect = "none";
    event.preventDefault();
}

WebInspector._debuggerDidPause = function(event)
{
    this.debuggerSidebarPanel.show();

    // Since the Scope Chain details sidebar panel might not be in the sidebar yet,
    // set a flag to select and show it when it does become available.
    this._selectAndShowScopeChainDetailsSidebarPanelWhenAvailable = true;

    InspectorFrontendHost.bringToFront();
}

WebInspector._mainFrameDidChange = function(event)
{
    this.updateWindowTitle();
}

WebInspector._mainResourceDidChange = function(event)
{
    if (!event.target.isMainFrame())
        return;
    this.updateWindowTitle();
}

WebInspector._windowFocused = function(event)
{
    if (event.target.document.nodeType !== Node.DOCUMENT_NODE || this.docked)
        return;

    // FIXME: We should use the :window-inactive pseudo class once https://webkit.org/b/38927 is fixed.
    document.body.classList.remove("window-inactive");
}

WebInspector._windowBlurred = function(event)
{
    if (event.target.document.nodeType !== Node.DOCUMENT_NODE || this.docked)
        return;

    // FIXME: We should use the :window-inactive pseudo class once https://webkit.org/b/38927 is fixed.
    document.body.classList.add("window-inactive");
}

WebInspector._windowResized = function(event)
{
    this.toolbar.updateLayout();

    this._contentBrowserSizeDidChange(event);
}

WebInspector._updateModifierKeys = function(event)
{
    var didChange = this.modifierKeys.altKey !== event.altKey || this.modifierKeys.metaKey !== event.metaKey || this.modifierKeys.shiftKey !== event.shiftKey;

    this.modifierKeys = {altKey: event.altKey, metaKey: event.metaKey, shiftKey: event.shiftKey};

    if (didChange)
        this.notifications.dispatchEventToListeners(WebInspector.Notification.GlobalModifierKeysDidChange, event);
}

WebInspector._windowKeyDown = function(event)
{
    this._updateModifierKeys(event);

    var opposite = !this._dockButtonToggledSetting.value;
    this.undockButtonNavigationItem.toggled = (event.altKey && !event.metaKey && !event.shiftKey) ? opposite : !opposite;
}

WebInspector._windowKeyUp = function(event)
{
    this._updateModifierKeys(event);

    var opposite = !this._dockButtonToggledSetting.value;
    this.undockButtonNavigationItem.toggled = (event.altKey && !event.metaKey && !event.shiftKey) ? opposite : !opposite;
}

WebInspector._undock = function(event)
{
    this._dockButtonToggledSetting.value = this.undockButtonNavigationItem.toggled;

    if (this.undockButtonNavigationItem.toggled)
        InspectorFrontendHost.requestSetDockSide(this._dockSide === "bottom" ? "right" : "bottom");
    else
        InspectorFrontendHost.requestSetDockSide("undocked");
}

WebInspector._updateDockNavigationItems = function()
{
    // The close and undock buttons are only available when docked.
    var docked = this.docked;
    this.closeButtonNavigationItem.hidden = !docked;
    this.undockButtonNavigationItem.hidden = !docked;

    if (docked) {
        this.undockButtonNavigationItem.alternateImage = this._dockSide === "bottom" ? "Images/DockRight.pdf" : "Images/DockBottom.pdf";
        this.undockButtonNavigationItem.alternateToolTip = this._dockSide === "bottom" ? WebInspector.UIString("Dock to right of window") : WebInspector.UIString("Dock to bottom of window");
    }

    this.undockButtonNavigationItem.toggled = this._dockButtonToggledSetting.value;
}

WebInspector._sidebarCollapsedStateDidChange = function(event)
{
    if (event.target === this.navigationSidebar) {
        this._navigationSidebarCollapsedSetting.value = this.navigationSidebar.collapsed;
        this._updateNavigationSidebarForCurrentContentView();
    } else if (event.target === this.detailsSidebar) {
        if (!this._ignoreDetailsSidebarPanelCollapsedEvent)
            this._detailsSidebarCollapsedSetting.value = this.detailsSidebar.collapsed;
    }
}

WebInspector._detailsSidebarPanelSelected = function(event)
{
    if (!this.detailsSidebar.selectedSidebarPanel || this._ignoreDetailsSidebarPanelSelectedEvent)
        return;

    this._lastSelectedDetailsSidebarPanelSetting.value = this.detailsSidebar.selectedSidebarPanel.identifier;
}

WebInspector._revealAndSelectRepresentedObjectInNavigationSidebar = function(representedObject)
{
    if (this.navigationSidebar.collapsed)
        return;

    var selectedSidebarPanel = this.navigationSidebar.selectedSidebarPanel;

    // If the tree outline is processing a selection currently then we can assume the selection does not
    // need to be changed. This is needed to allow breakpoints tree elements to be selected without jumping
    // back to selecting the resource tree element.
    if (selectedSidebarPanel.contentTreeOutline.processingSelectionChange)
        return;

    var treeElement = selectedSidebarPanel.treeElementForRepresentedObject(representedObject);
    if (treeElement)
        treeElement.revealAndSelect(true, false, true, true);
    else if (selectedSidebarPanel.contentTreeOutline.selectedTreeElement)
        selectedSidebarPanel.contentTreeOutline.selectedTreeElement.deselect(true);
}

WebInspector._updateNavigationSidebarForCurrentContentView = function()
{
    if (this.navigationSidebar.collapsed)
        return;

    var selectedSidebarPanel = this.navigationSidebar.selectedSidebarPanel;
    if (!selectedSidebarPanel)
        return;

    var currentContentView = this.contentBrowser.currentContentView;
    if (!currentContentView)
        return;

    // Ensure the navigation sidebar panel is allowed by the current content view, if not ask the sidebar panel
    // to show the content view for the current selection.
    var allowedNavigationSidebarPanels = currentContentView.allowedNavigationSidebarPanels;
    if (!allowedNavigationSidebarPanels.contains(selectedSidebarPanel.identifier)) {
        selectedSidebarPanel.showContentViewForCurrentSelection();

        // Fetch the current content view again, since it likely changed.
        currentContentView = this.contentBrowser.currentContentView;
    }

    if (!allowedNavigationSidebarPanels.length || allowedNavigationSidebarPanels.contains(selectedSidebarPanel.identifier))
        currentContentView.__lastNavigationSidebarPanelIdentifer = selectedSidebarPanel.identifier;

    this._revealAndSelectRepresentedObjectInNavigationSidebar(currentContentView.representedObject);
}

WebInspector._navigationSidebarPanelSelected = function(event)
{
    var selectedSidebarPanel = this.navigationSidebar.selectedSidebarPanel;
    if (!selectedSidebarPanel)
        return;

    this._lastSelectedNavigationSidebarPanelSetting.value = selectedSidebarPanel.identifier;

    this._updateNavigationSidebarForCurrentContentView();
}

WebInspector._domNodeWasInspected = function(event)
{
    WebInspector.domTreeManager.highlightDOMNodeForTwoSeconds(event.data.node.id);

    // Select the Style details sidebar panel if one of the DOM details sidebar panels isn't already selected.
    if (!(this.detailsSidebar.selectedSidebarPanel instanceof WebInspector.DOMDetailsSidebarPanel))
        this.detailsSidebar.selectedSidebarPanel = this.cssStyleDetailsSidebarPanel;

    InspectorFrontendHost.bringToFront();
}

WebInspector._contentBrowserSizeDidChange = function(event)
{
    this.contentBrowser.updateLayout();
    this.splitContentBrowser.updateLayout();
    this.quickConsole.updateLayout();
}

WebInspector._quickConsoleDidResize = function(event)
{
    this.contentBrowser.updateLayout();
}

WebInspector._sidebarWidthDidChange = function(event)
{
    if (!event.target.collapsed) {
        if (event.target === this.navigationSidebar)
            this._navigationSidebarWidthSetting.value = this.navigationSidebar.width;
        else if (event.target === this.detailsSidebar)
            this._detailsSidebarWidthSetting.value = this.detailsSidebar.width;
    }

    this._contentBrowserSizeDidChange(event);
}

WebInspector._updateToolbarHeight = function()
{
    InspectorFrontendHost.setToolbarHeight(this.toolbar.element.offsetHeight);
}

WebInspector._toolbarDisplayModeDidChange = function(event)
{
    if (this._ignoreToolbarModeDidChangeEvents)
        return;

    if (this._dockSide === "bottom")
        this._toolbarDockedBottomDisplayModeSetting.value = this.toolbar.displayMode;
    else if (this._dockSide === "right")
        this._toolbarDockedRightDisplayModeSetting.value = this.toolbar.displayMode;
    else
        this._toolbarUndockedDisplayModeSetting.value = this.toolbar.displayMode;

    this._updateToolbarHeight();
}

WebInspector._toolbarSizeModeDidChange = function(event)
{
    if (this._ignoreToolbarModeDidChangeEvents)
        return;

    if (this._dockSide === "bottom")
        this._toolbarDockedBottomSizeModeSetting.value = this.toolbar.sizeMode;
    else if (this._dockSide === "right")
        this._toolbarDockedRightSizeModeSetting.value = this.toolbar.sizeMode;
    else
        this._toolbarUndockedSizeModeSetting.value = this.toolbar.sizeMode;

    this._updateToolbarHeight();
}

WebInspector._updateCurrentContentViewCookie = function()
{
    var currentContentView = this.contentBrowser.currentContentView;
    if (!currentContentView)
        return;

    // The console does not have a sidebar, so create a cookie here.
    if (currentContentView.representedObject instanceof WebInspector.LogObject) {
        this._lastContentViewResponsibleSidebarPanelSetting.value = null;
        this._lastContentCookieSetting.value = "console";
        return;
    }

    var responsibleSidebarPanel = this.sidebarPanelForRepresentedObject(currentContentView.representedObject);
    if (!responsibleSidebarPanel)
        return;

    var cookie = responsibleSidebarPanel.cookieForContentView(currentContentView);

    this._lastContentViewResponsibleSidebarPanelSetting.value = responsibleSidebarPanel.identifier;
    this._lastContentCookieSetting.value = cookie;
}

WebInspector._contentBrowserCurrentContentViewDidChange = function(event)
{
    var consoleViewShowing = this.isShowingConsoleView();
    this._consoleToolbarButton.activated = consoleViewShowing;

    if (!this.isShowingSplitConsole())
        this.quickConsole.consoleLogVisibilityChanged(consoleViewShowing);

    if (!this.currentViewSupportsSplitContentBrowser())
        this.hideSplitConsole();

    var currentContentView = this.contentBrowser.currentContentView;
    if (!currentContentView)
        return;

    // Ensure the navigation sidebar panel is allowed by the current content view, if not change the navigation sidebar panel
    // to the last navigation sidebar panel used with the content view or the first one allowed.
    var selectedSidebarPanelIdentifier = this.navigationSidebar.selectedSidebarPanel.identifier;

    var allowedNavigationSidebarPanels = currentContentView.allowedNavigationSidebarPanels;
    if (allowedNavigationSidebarPanels.length && !allowedNavigationSidebarPanels.contains(selectedSidebarPanelIdentifier)) {
        console.assert(!currentContentView.__lastNavigationSidebarPanelIdentifer || allowedNavigationSidebarPanels.contains(currentContentView.__lastNavigationSidebarPanelIdentifer));
        this.navigationSidebar.selectedSidebarPanel = currentContentView.__lastNavigationSidebarPanelIdentifer || allowedNavigationSidebarPanels[0];
    }

    if (!allowedNavigationSidebarPanels.length || allowedNavigationSidebarPanels.contains(selectedSidebarPanelIdentifier))
        currentContentView.__lastNavigationSidebarPanelIdentifer = selectedSidebarPanelIdentifier;

    this._revealAndSelectRepresentedObjectInNavigationSidebar(currentContentView.representedObject);
}

WebInspector._contentBrowserRepresentedObjectsDidChange = function(event)
{
    var currentRepresentedObjects = this.contentBrowser.currentRepresentedObjects;
    var currentSidebarPanels = this.detailsSidebar.sidebarPanels;
    var wasSidebarEmpty = !currentSidebarPanels.length;

    // Ignore any changes to the selected sidebar panel during this function so only user initiated
    // changes are recorded in _lastSelectedDetailsSidebarPanelSetting.
    this._ignoreDetailsSidebarPanelSelectedEvent = true;

    for (var i = 0; i < this.detailsSidebarPanels.length; ++i) {
        var sidebarPanel = this.detailsSidebarPanels[i];
        if (sidebarPanel.inspect(currentRepresentedObjects)) {
            var currentSidebarPanelIndex = currentSidebarPanels.indexOf(sidebarPanel);
            if (currentSidebarPanelIndex !== -1) {
                // Already showing the panel.
                continue;
            }

            // The sidebar panel was not previously showing, so add the panel and show the toolbar item.
            this.detailsSidebar.addSidebarPanel(sidebarPanel);
            sidebarPanel.toolbarItem.hidden = false;

            if (this._selectAndShowScopeChainDetailsSidebarPanelWhenAvailable && sidebarPanel === this.scopeChainDetailsSidebarPanel) {
                // Select the scope chain sidebar panel since it needs to be shown after pausing in the debugger.
                delete this._selectAndShowScopeChainDetailsSidebarPanelWhenAvailable;
                this.detailsSidebar.selectedSidebarPanel = this.scopeChainDetailsSidebarPanel;

                this._ignoreDetailsSidebarPanelCollapsedEvent = true;
                this.detailsSidebar.collapsed = false;
                delete this._ignoreDetailsSidebarPanelCollapsedEvent;
            } else if (this._lastSelectedDetailsSidebarPanelSetting.value === sidebarPanel.identifier) {
                // Restore the sidebar panel selection if this sidebar panel was the last one selected by the user.
                this.detailsSidebar.selectedSidebarPanel = sidebarPanel;
            }
        } else {
            // The sidebar panel can't inspect the current represented objects, so remove the panel and hide the toolbar item.
            this.detailsSidebar.removeSidebarPanel(sidebarPanel);
            sidebarPanel.toolbarItem.hidden = true;
        }
    }

    if (!this.detailsSidebar.selectedSidebarPanel && currentSidebarPanels.length)
        this.detailsSidebar.selectedSidebarPanel = currentSidebarPanels[0];

    this._ignoreDetailsSidebarPanelCollapsedEvent = true;

    if (!this.detailsSidebar.sidebarPanels.length)
        this.detailsSidebar.collapsed = true;
    else if (wasSidebarEmpty)
        this.detailsSidebar.collapsed = this._detailsSidebarCollapsedSetting.value;

    delete this._ignoreDetailsSidebarPanelCollapsedEvent;

    // Stop ignoring the sidebar panel selected event.
    delete this._ignoreDetailsSidebarPanelSelectedEvent;

    this._updateCurrentContentViewCookie(event);
}

WebInspector._initializeWebSocketIfNeeded = function()
{
    var ws;
    var queryParams = parseLocationQueryParameters();

    if ("ws" in queryParams)
        ws = "ws://" + queryParams.ws;
    else if ("page" in queryParams) {
        var page = queryParams.page;
        var host = "host" in queryParams ? queryParams.host : window.location.host;
        ws = "ws://" + host + "/devtools/page/" + page;
    }

    if (!ws)
        return;

    var socket = new WebSocket(ws);
    socket.addEventListener("open", createSocket);

    function createSocket()
    {
        WebInspector.socket = socket;
        WebInspector.socket.addEventListener("message", function(message) { InspectorBackend.dispatch(message.data); });
        WebInspector.socket.addEventListener("error", function(error) { console.error(error); });
    }
}

WebInspector._updateSplitConsoleHeight = function(height)
{
    const minimumHeight = 64;
    const maximumHeight = window.innerHeight * 0.55;

    height = Math.max(minimumHeight, Math.min(height, maximumHeight));

    this.splitContentBrowser.element.style.height = height + "px";
}

WebInspector._consoleResizerMouseDown = function(event)
{
    if (event.button !== 0 || event.ctrlKey)
        return;

    // Only start dragging if the target is one of the elements that we expect.
    if (!event.target.classList.contains("navigation-bar") && !event.target.classList.contains("flexible-space"))
        return;

    var resizerElement = event.target;
    var mouseOffset = resizerElement.offsetHeight - (event.pageY - resizerElement.totalOffsetTop);

    function dockedResizerDrag(event)
    {
        if (event.button !== 0)
            return;

        var height = window.innerHeight - event.pageY - mouseOffset;

        this._splitConsoleHeightSetting.value = height;

        this._updateSplitConsoleHeight(height);
    }

    function dockedResizerDragEnd(event)
    {
        if (event.button !== 0)
            return;

        this.elementDragEnd(event);
    }

    this.elementDragStart(resizerElement, dockedResizerDrag.bind(this), dockedResizerDragEnd.bind(this), event, "row-resize");
}

WebInspector._toolbarMouseDown = function(event)
{
    if (event.ctrlKey)
        return;

    if (this._dockSide === "right")
        return;

    if (this.docked)
        this._dockedResizerMouseDown(event);
    else
        this._moveWindowMouseDown(event);
}

WebInspector._dockedResizerMouseDown = function(event)
{
    if (event.button !== 0 || event.ctrlKey)
        return;

    if (!this.docked)
        return;

    // Only start dragging if the target is one of the elements that we expect.
    if (event.target.id !== "docked-resizer" && !event.target.classList.contains("toolbar") &&
        !event.target.classList.contains("flexible-space") && !event.target.classList.contains("item-section"))
        return;

    var windowProperty = this._dockSide === "bottom" ? "innerHeight" : "innerWidth";
    var eventProperty = this._dockSide === "bottom" ? "screenY" : "screenX";

    var resizerElement = event.target;
    var lastScreenPosition = event[eventProperty];

    function dockedResizerDrag(event)
    {
        if (event.button !== 0)
            return;

        var position = event[eventProperty];
        var dimension = window[windowProperty] - (position - lastScreenPosition);

        if (this._dockSide === "bottom")
            InspectorFrontendHost.setAttachedWindowHeight(dimension);
        else
            InspectorFrontendHost.setAttachedWindowWidth(dimension);

        lastScreenPosition = position;
    }

    function dockedResizerDragEnd(event)
    {
        if (event.button !== 0)
            return;

        WebInspector.elementDragEnd(event);
    }

    WebInspector.elementDragStart(resizerElement, dockedResizerDrag.bind(this), dockedResizerDragEnd.bind(this), event, this._dockSide === "bottom" ? "row-resize" : "col-resize");
}

WebInspector._moveWindowMouseDown = function(event)
{
    console.assert(!this.docked);

    if (event.button !== 0 || event.ctrlKey)
        return;

    // Only start dragging if the target is one of the elements that we expect.
    if (!event.target.classList.contains("toolbar") && !event.target.classList.contains("flexible-space") &&
        !event.target.classList.contains("item-section"))
        return;

    var lastScreenX = event.screenX;
    var lastScreenY = event.screenY;

    function toolbarDrag(event)
    {
        if (event.button !== 0)
            return;

        var x = event.screenX - lastScreenX;
        var y = event.screenY - lastScreenY;

        InspectorFrontendHost.moveWindowBy(x, y);

        lastScreenX = event.screenX;
        lastScreenY = event.screenY;
    }

    function toolbarDragEnd(event)
    {
        if (event.button !== 0)
            return;

        WebInspector.elementDragEnd(event);
    }

    WebInspector.elementDragStart(event.target, toolbarDrag, toolbarDragEnd, event, "default");
}

WebInspector._inspectModeStateChanged = function(event)
{
    this._inspectModeToolbarButton.activated = WebInspector.domTreeManager.inspectModeEnabled;
}

WebInspector._toggleInspectMode = function(event)
{
    WebInspector.domTreeManager.inspectModeEnabled = !WebInspector.domTreeManager.inspectModeEnabled;
}

WebInspector._reloadPage = function(event)
{
    PageAgent.reload();
}

WebInspector._reloadPageIgnoringCache = function(event)
{
    PageAgent.reload(true);
}

WebInspector._toggleInspectMode = function(event)
{
    this.domTreeManager.inspectModeEnabled = !this.domTreeManager.inspectModeEnabled;
}

WebInspector._focusedContentView = function()
{
    if (this.contentBrowser.element.isSelfOrAncestor(this.currentFocusElement))
        return this.contentBrowser.currentContentView;
    if (this.splitContentBrowser.element.isSelfOrAncestor(this.currentFocusElement))
        return  this.splitContentBrowser.currentContentView;
    return null;
}

WebInspector._beforecopy = function(event)
{
    var selection = window.getSelection();

    // If there is no selection, see if the focused element or focused ContentView can handle the copy event.
    if (selection.isCollapsed && !WebInspector.isEventTargetAnEditableField(event)) {
        var focusedCopyHandler = this.currentFocusElement && this.currentFocusElement.copyHandler;
        if (focusedCopyHandler && typeof focusedCopyHandler.handleBeforeCopyEvent === "function") {
            focusedCopyHandler.handleBeforeCopyEvent(event);
            if (event.defaultPrevented)
                return;
        }

        var focusedContentView = this._focusedContentView();
        if (focusedContentView && typeof focusedContentView.handleCopyEvent === "function") {
            event.preventDefault();
            return;
        }

        return;
    }

    if (selection.isCollapsed)
        return;

    // Say we can handle it (by preventing default) to remove word break characters.
    event.preventDefault();
}

WebInspector._copy = function(event)
{
    var selection = window.getSelection();

    // If there is no selection, pass the copy event on to the focused element or focused ContentView.
    if (selection.isCollapsed && !WebInspector.isEventTargetAnEditableField(event)) {
        var focusedCopyHandler = this.currentFocusElement && this.currentFocusElement.copyHandler;
        if (focusedCopyHandler && typeof focusedCopyHandler.handleCopyEvent === "function") {
            focusedCopyHandler.handleCopyEvent(event);
            if (event.defaultPrevented)
                return;
        }

        var focusedContentView = this._focusedContentView();
        if (focusedContentView && typeof focusedContentView.handleCopyEvent === "function") {
            focusedContentView.handleCopyEvent(event);
            return;
        }

        return;
    }

    if (selection.isCollapsed)
        return;

    // Remove word break characters from the selection before putting it on the pasteboard.
    var selectionString = selection.toString().removeWordBreakCharacters();
    event.clipboardData.setData("text/plain", selectionString);
    event.preventDefault();
}

WebInspector._generateDisclosureTriangleImages = function()
{
    var specifications = {};
    specifications["normal"] = {fillColor: [0, 0, 0, 0.5]};
    specifications["normal-active"] = {fillColor: [0, 0, 0, 0.7]};
    specifications["selected"] = {fillColor: [255, 255, 255, 0.8]};
    specifications["selected-active"] = {fillColor: [255, 255, 255, 1]};

    generateColoredImagesForCSS("Images/DisclosureTriangleSmallOpen.pdf", specifications, 13, 13, "disclosure-triangle-small-open-");
    generateColoredImagesForCSS("Images/DisclosureTriangleSmallClosed.pdf", specifications, 13, 13, "disclosure-triangle-small-closed-");

    generateColoredImagesForCSS("Images/DisclosureTriangleTinyOpen.pdf", specifications, 8, 8, "disclosure-triangle-tiny-open-");
    generateColoredImagesForCSS("Images/DisclosureTriangleTinyClosed.pdf", specifications, 8, 8, "disclosure-triangle-tiny-closed-");
}

WebInspector.elementDragStart = function(element, dividerDrag, elementDragEnd, event, cursor, eventTarget)
{
    if (WebInspector._elementDraggingEventListener || WebInspector._elementEndDraggingEventListener)
        WebInspector.elementDragEnd(event);
    
    if (element) {
        // Install glass pane
        if (WebInspector._elementDraggingGlassPane)
            WebInspector._elementDraggingGlassPane.parentElement.removeChild(WebInspector._elementDraggingGlassPane);
        
        var glassPane = document.createElement("div");
        glassPane.style.cssText = "position:absolute;top:0;bottom:0;left:0;right:0;opacity:0;z-index:1";
        glassPane.id = "glass-pane-for-drag";
        element.ownerDocument.body.appendChild(glassPane);
        WebInspector._elementDraggingGlassPane = glassPane;
    }
    
    WebInspector._elementDraggingEventListener = dividerDrag;
    WebInspector._elementEndDraggingEventListener = elementDragEnd;
    
    var targetDocument = event.target.ownerDocument;

    WebInspector._elementDraggingEventTarget = eventTarget || targetDocument;
    WebInspector._elementDraggingEventTarget.addEventListener("mousemove", dividerDrag, true);
    WebInspector._elementDraggingEventTarget.addEventListener("mouseup", elementDragEnd, true);
    
    targetDocument.body.style.cursor = cursor;
    
    event.preventDefault();
}

WebInspector.elementDragEnd = function(event)
{
    WebInspector._elementDraggingEventTarget.removeEventListener("mousemove", WebInspector._elementDraggingEventListener, true);
    WebInspector._elementDraggingEventTarget.removeEventListener("mouseup", WebInspector._elementEndDraggingEventListener, true);
    
    event.target.ownerDocument.body.style.removeProperty("cursor");
    
    if (WebInspector._elementDraggingGlassPane)
        WebInspector._elementDraggingGlassPane.parentElement.removeChild(WebInspector._elementDraggingGlassPane);
    
    delete WebInspector._elementDraggingGlassPane;
    delete WebInspector._elementDraggingEventTarget;
    delete WebInspector._elementDraggingEventListener;
    delete WebInspector._elementEndDraggingEventListener;
    
    event.preventDefault();
}

WebInspector.createMessageTextView = function(message, isError)
{
    var messageElement = document.createElement("div");
    messageElement.className = "message-text-view";
    if (isError)
        messageElement.classList.add("error");

    messageElement.textContent = message;
    
    return messageElement;
}

WebInspector.createGoToArrowButton = function()
{
    if (!WebInspector._generatedGoToArrowButtonImages) {
        WebInspector._generatedGoToArrowButtonImages = true;

        var specifications = {};
        specifications["go-to-arrow-normal"] = {fillColor: [0, 0, 0, 0.5]};
        specifications["go-to-arrow-normal-active"] = {fillColor: [0, 0, 0, 0.7]};
        specifications["go-to-arrow-selected"] = {fillColor: [255, 255, 255, 0.8]};
        specifications["go-to-arrow-selected-active"] = {fillColor: [255, 255, 255, 1]};

        generateColoredImagesForCSS("Images/GoToArrow.pdf", specifications, 10, 10);
    }

    function stopPropagation(event)
    {
        event.stopPropagation()
    }

    var button = document.createElement("button");
    button.addEventListener("mousedown", stopPropagation, true);
    button.className = "go-to-arrow";
    button.tabIndex = -1;
    return button;
}

WebInspector.createSourceCodeLocationLink = function(sourceCodeLocation, dontFloat, useGoToArrowButton)
{
    console.assert(sourceCodeLocation);
    if (!sourceCodeLocation)
        return null;

    function showSourceCodeLocation(event)
    {
        event.stopPropagation();
        event.preventDefault();

        if (event.metaKey)
            this.resourceSidebarPanel.showOriginalUnformattedSourceCodeLocation(sourceCodeLocation);
        else
            this.resourceSidebarPanel.showSourceCodeLocation(sourceCodeLocation);
    }

    var linkElement = document.createElement("a");
    linkElement.className = "go-to-link";
    linkElement.addEventListener("click", showSourceCodeLocation.bind(this));
    sourceCodeLocation.populateLiveDisplayLocationTooltip(linkElement);

    if (useGoToArrowButton)
        linkElement.appendChild(WebInspector.createGoToArrowButton());
    else
        sourceCodeLocation.populateLiveDisplayLocationString(linkElement, "textContent");

    if (dontFloat)
        linkElement.classList.add("dont-float");

    return linkElement;
}

WebInspector.linkifyLocation = function(url, lineNumber, columnNumber, className)
{
    var sourceCode = WebInspector.frameResourceManager.resourceForURL(url);
    if (!sourceCode) {
        sourceCode = WebInspector.debuggerManager.scriptsForURL(url)[0];
        if (sourceCode)
            sourceCode = sourceCode.resource || sourceCode;
    }

    if (!sourceCode) {
        var anchor = document.createElement("a");
        anchor.href  = url;
        anchor.lineNumber = lineNumber;
        if (className)
            anchor.className = className;
        anchor.appendChild(document.createTextNode(WebInspector.displayNameForURL(url) + ":" + lineNumber));
        return anchor;
    }

    var sourceCodeLocation = sourceCode.createSourceCodeLocation(lineNumber, columnNumber);
    var linkElement = WebInspector.createSourceCodeLocationLink(sourceCodeLocation, true);
    if (className)
        linkElement.classList.add(className);
    return linkElement;
}

WebInspector.linkifyURLAsNode = function(url, linkText, classes, tooltipText)
{
    if (!linkText)
        linkText = url;

    classes = (classes ? classes + " " : "");

    var a = document.createElement("a");
    a.href = url;
    a.className = classes;

    if (typeof tooltipText === "undefined")
        a.title = url;
    else if (typeof tooltipText !== "string" || tooltipText.length)
        a.title = tooltipText;

    a.textContent = linkText;
    a.style.maxWidth = "100%";

    return a;
}

WebInspector.linkifyStringAsFragmentWithCustomLinkifier = function(string, linkifier)
{
    var container = document.createDocumentFragment();
    var linkStringRegEx = /(?:[a-zA-Z][a-zA-Z0-9+.-]{2,}:\/\/|www\.)[\w$\-_+*'=\|\/\\(){}[\]%@&#~,:;.!?]{2,}[\w$\-_+*=\|\/\\({%@&#~]/;
    var lineColumnRegEx = /:(\d+)(:(\d+))?$/;

    while (string) {
        var linkString = linkStringRegEx.exec(string);
        if (!linkString)
            break;

        linkString = linkString[0];
        var linkIndex = string.indexOf(linkString);
        var nonLink = string.substring(0, linkIndex);
        container.appendChild(document.createTextNode(nonLink));

        var title = linkString;
        var realURL = (linkString.startsWith("www.") ? "http://" + linkString : linkString);
        var lineColumnMatch = lineColumnRegEx.exec(realURL);
        if (lineColumnMatch)
            realURL = realURL.substring(0, realURL.length - lineColumnMatch[0].length);

        var linkNode = linkifier(title, realURL, lineColumnMatch ? lineColumnMatch[1] : undefined);
        container.appendChild(linkNode);
        string = string.substring(linkIndex + linkString.length, string.length);
    }

    if (string)
        container.appendChild(document.createTextNode(string));

    return container;
}

WebInspector.linkifyStringAsFragment = function(string)
{
    function linkifier(title, url, lineNumber)
    {
        var urlNode = WebInspector.linkifyURLAsNode(url, title, undefined);
        if (typeof(lineNumber) !== "undefined")
            urlNode.lineNumber = lineNumber;

        return urlNode; 
    }
    
    return WebInspector.linkifyStringAsFragmentWithCustomLinkifier(string, linkifier);
}

WebInspector._undoKeyboardShortcut = function(event)
{
    if (!this.isEditingAnyField() && !this.isEventTargetAnEditableField(event)) {
        this.undo();
        event.preventDefault();
    }
}

WebInspector._redoKeyboardShortcut = function(event)
{
    if (!this.isEditingAnyField() && !this.isEventTargetAnEditableField(event)) {
        this.redo();
        event.preventDefault();
    }
}

WebInspector.undo = function()
{
    DOMAgent.undo();
}

WebInspector.redo = function()
{
    DOMAgent.redo();
}

/**
 * @param {Element} element
 * @param {Array.<Object>} resultRanges
 * @param {string} styleClass
 * @param {Array.<Object>=} changes
 */
WebInspector.highlightRangesWithStyleClass = function(element, resultRanges, styleClass, changes)
{
    changes = changes || [];
    var highlightNodes = [];
    var lineText = element.textContent;
    var ownerDocument = element.ownerDocument;
    var textNodeSnapshot = ownerDocument.evaluate(".//text()", element, null, XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null);

    var snapshotLength = textNodeSnapshot.snapshotLength;
    if (snapshotLength === 0)
        return highlightNodes;

    var nodeRanges = [];
    var rangeEndOffset = 0;
    for (var i = 0; i < snapshotLength; ++i) {
        var range = {};
        range.offset = rangeEndOffset;
        range.length = textNodeSnapshot.snapshotItem(i).textContent.length;
        rangeEndOffset = range.offset + range.length;
        nodeRanges.push(range);
    }

    var startIndex = 0;
    for (var i = 0; i < resultRanges.length; ++i) {
        var startOffset = resultRanges[i].offset;
        var endOffset = startOffset + resultRanges[i].length;

        while (startIndex < snapshotLength && nodeRanges[startIndex].offset + nodeRanges[startIndex].length <= startOffset)
            startIndex++;
        var endIndex = startIndex;
        while (endIndex < snapshotLength && nodeRanges[endIndex].offset + nodeRanges[endIndex].length < endOffset)
            endIndex++;
        if (endIndex === snapshotLength)
            break;

        var highlightNode = ownerDocument.createElement("span");
        highlightNode.className = styleClass;
        highlightNode.textContent = lineText.substring(startOffset, endOffset);

        var lastTextNode = textNodeSnapshot.snapshotItem(endIndex);
        var lastText = lastTextNode.textContent;
        lastTextNode.textContent = lastText.substring(endOffset - nodeRanges[endIndex].offset);
        changes.push({ node: lastTextNode, type: "changed", oldText: lastText, newText: lastTextNode.textContent });

        if (startIndex === endIndex) {
            lastTextNode.parentElement.insertBefore(highlightNode, lastTextNode);
            changes.push({ node: highlightNode, type: "added", nextSibling: lastTextNode, parent: lastTextNode.parentElement });
            highlightNodes.push(highlightNode);

            var prefixNode = ownerDocument.createTextNode(lastText.substring(0, startOffset - nodeRanges[startIndex].offset));
            lastTextNode.parentElement.insertBefore(prefixNode, highlightNode);
            changes.push({ node: prefixNode, type: "added", nextSibling: highlightNode, parent: lastTextNode.parentElement });
        } else {
            var firstTextNode = textNodeSnapshot.snapshotItem(startIndex);
            var firstText = firstTextNode.textContent;
            var anchorElement = firstTextNode.nextSibling;

            firstTextNode.parentElement.insertBefore(highlightNode, anchorElement);
            changes.push({ node: highlightNode, type: "added", nextSibling: anchorElement, parent: firstTextNode.parentElement });
            highlightNodes.push(highlightNode);

            firstTextNode.textContent = firstText.substring(0, startOffset - nodeRanges[startIndex].offset);
            changes.push({ node: firstTextNode, type: "changed", oldText: firstText, newText: firstTextNode.textContent });

            for (var j = startIndex + 1; j < endIndex; j++) {
                var textNode = textNodeSnapshot.snapshotItem(j);
                var text = textNode.textContent;
                textNode.textContent = "";
                changes.push({ node: textNode, type: "changed", oldText: text, newText: textNode.textContent });
            }
        }
        startIndex = endIndex;
        nodeRanges[startIndex].offset = endOffset;
        nodeRanges[startIndex].length = lastTextNode.textContent.length;

    }
    return highlightNodes;
}

WebInspector.revertDomChanges = function(domChanges)
{
    for (var i = domChanges.length - 1; i >= 0; --i) {
        var entry = domChanges[i];
        switch (entry.type) {
        case "added":
            if (entry.node.parentElement)
                entry.node.parentElement.removeChild(entry.node);
            break;
        case "changed":
            entry.node.textContent = entry.oldText;
            break;
        }
    }
}
