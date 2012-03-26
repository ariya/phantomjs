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
    resources: {},
    missingLocalizedStrings: {},
    pendingDispatches: 0,

    get platform()
    {
        if (!("_platform" in this))
            this._platform = InspectorFrontendHost.platform();

        return this._platform;
    },

    get platformFlavor()
    {
        if (!("_platformFlavor" in this))
            this._platformFlavor = this._detectPlatformFlavor();

        return this._platformFlavor;
    },

    _detectPlatformFlavor: function()
    {
        const userAgent = navigator.userAgent;

        if (this.platform === "windows") {
            var match = userAgent.match(/Windows NT (\d+)\.(?:\d+)/);
            if (match && match[1] >= 6)
                return WebInspector.PlatformFlavor.WindowsVista;
            return null;
        } else if (this.platform === "mac") {
            var match = userAgent.match(/Mac OS X\s*(?:(\d+)_(\d+))?/);
            if (!match || match[1] != 10)
                return WebInspector.PlatformFlavor.MacSnowLeopard;
            switch (Number(match[2])) {
                case 4:
                    return WebInspector.PlatformFlavor.MacTiger;
                case 5:
                    return WebInspector.PlatformFlavor.MacLeopard;
                case 6:
                default:
                    return WebInspector.PlatformFlavor.MacSnowLeopard;
            }
        }

        return null;
    },

    get port()
    {
        if (!("_port" in this))
            this._port = InspectorFrontendHost.port();

        return this._port;
    },

    get previousFocusElement()
    {
        return this._previousFocusElement;
    },

    get currentFocusElement()
    {
        return this._currentFocusElement;
    },

    set currentFocusElement(x)
    {
        if (this._currentFocusElement !== x)
            this._previousFocusElement = this._currentFocusElement;
        this._currentFocusElement = x;

        if (this._currentFocusElement) {
            this._currentFocusElement.focus();

            // Make a caret selection inside the new element if there isn't a range selection and
            // there isn't already a caret selection inside.
            var selection = window.getSelection();
            if (selection.isCollapsed && !this._currentFocusElement.isInsertionCaretInside()) {
                var selectionRange = this._currentFocusElement.ownerDocument.createRange();
                selectionRange.setStart(this._currentFocusElement, 0);
                selectionRange.setEnd(this._currentFocusElement, 0);

                selection.removeAllRanges();
                selection.addRange(selectionRange);
            }
        } else if (this._previousFocusElement)
            this._previousFocusElement.blur();
    },

    resetFocusElement: function()
    {
        this.currentFocusElement = null;
        this._previousFocusElement = null;
    },

    get currentPanel()
    {
        return this._currentPanel;
    },

    set currentPanel(x)
    {
        if (this._currentPanel === x)
            return;

        if (this._currentPanel)
            this._currentPanel.hide();

        this._currentPanel = x;

        if (x) {
            x.show();
            WebInspector.searchController.activePanelChanged();
        }
        for (var panelName in WebInspector.panels) {
            if (WebInspector.panels[panelName] === x) {
                WebInspector.settings.lastActivePanel = panelName;
                this._panelHistory.setPanel(panelName);
            }
        }
    },

    _createPanels: function()
    {
        if (WebInspector.WorkerManager.isWorkerFrontend()) {
            this.panels.scripts = new WebInspector.ScriptsPanel();
            this.panels.console = new WebInspector.ConsolePanel();
            return;
        }
        var hiddenPanels = (InspectorFrontendHost.hiddenPanels() || "").split(',');
        if (hiddenPanels.indexOf("elements") === -1)
            this.panels.elements = new WebInspector.ElementsPanel();
        if (hiddenPanels.indexOf("resources") === -1)
            this.panels.resources = new WebInspector.ResourcesPanel();
        if (hiddenPanels.indexOf("network") === -1)
            this.panels.network = new WebInspector.NetworkPanel();
        if (hiddenPanels.indexOf("scripts") === -1)
            this.panels.scripts = new WebInspector.ScriptsPanel();
        if (hiddenPanels.indexOf("timeline") === -1)
            this.panels.timeline = new WebInspector.TimelinePanel();
        if (hiddenPanels.indexOf("profiles") === -1)
            this.panels.profiles = new WebInspector.ProfilesPanel();
        if (hiddenPanels.indexOf("audits") === -1)
            this.panels.audits = new WebInspector.AuditsPanel();
        if (hiddenPanels.indexOf("console") === -1)
            this.panels.console = new WebInspector.ConsolePanel();
    },

    get attached()
    {
        return this._attached;
    },

    set attached(x)
    {
        if (this._attached === x)
            return;

        this._attached = x;

        var dockToggleButton = document.getElementById("dock-status-bar-item");
        var body = document.body;

        if (x) {
            body.removeStyleClass("detached");
            body.addStyleClass("attached");
            dockToggleButton.title = WebInspector.UIString("Undock into separate window.");
        } else {
            body.removeStyleClass("attached");
            body.addStyleClass("detached");
            dockToggleButton.title = WebInspector.UIString("Dock to main window.");
        }

        // This may be called before onLoadedDone, hence the bulk of inspector objects may
        // not be created yet.
        if (WebInspector.toolbar)
            WebInspector.toolbar.attached = x;

        if (WebInspector.searchController)
            WebInspector.searchController.updateSearchLabel();
    },

    get errors()
    {
        return this._errors || 0;
    },

    set errors(x)
    {
        x = Math.max(x, 0);

        if (this._errors === x)
            return;
        this._errors = x;
        this._updateErrorAndWarningCounts();
    },

    get warnings()
    {
        return this._warnings || 0;
    },

    set warnings(x)
    {
        x = Math.max(x, 0);

        if (this._warnings === x)
            return;
        this._warnings = x;
        this._updateErrorAndWarningCounts();
    },

    _updateErrorAndWarningCounts: function()
    {
        var errorWarningElement = document.getElementById("error-warning-count");
        if (!errorWarningElement)
            return;

        if (!this.errors && !this.warnings) {
            errorWarningElement.addStyleClass("hidden");
            return;
        }

        errorWarningElement.removeStyleClass("hidden");

        errorWarningElement.removeChildren();

        if (this.errors) {
            var errorElement = document.createElement("span");
            errorElement.id = "error-count";
            errorElement.textContent = this.errors;
            errorWarningElement.appendChild(errorElement);
        }

        if (this.warnings) {
            var warningsElement = document.createElement("span");
            warningsElement.id = "warning-count";
            warningsElement.textContent = this.warnings;
            errorWarningElement.appendChild(warningsElement);
        }

        if (this.errors) {
            if (this.warnings) {
                if (this.errors == 1) {
                    if (this.warnings == 1)
                        errorWarningElement.title = WebInspector.UIString("%d error, %d warning", this.errors, this.warnings);
                    else
                        errorWarningElement.title = WebInspector.UIString("%d error, %d warnings", this.errors, this.warnings);
                } else if (this.warnings == 1)
                    errorWarningElement.title = WebInspector.UIString("%d errors, %d warning", this.errors, this.warnings);
                else
                    errorWarningElement.title = WebInspector.UIString("%d errors, %d warnings", this.errors, this.warnings);
            } else if (this.errors == 1)
                errorWarningElement.title = WebInspector.UIString("%d error", this.errors);
            else
                errorWarningElement.title = WebInspector.UIString("%d errors", this.errors);
        } else if (this.warnings == 1)
            errorWarningElement.title = WebInspector.UIString("%d warning", this.warnings);
        else if (this.warnings)
            errorWarningElement.title = WebInspector.UIString("%d warnings", this.warnings);
        else
            errorWarningElement.title = null;
    },

    highlightDOMNode: function(nodeId, mode)
    {
        if ("_hideDOMNodeHighlightTimeout" in this) {
            clearTimeout(this._hideDOMNodeHighlightTimeout);
            delete this._hideDOMNodeHighlightTimeout;
        }

        if (this._highlightedDOMNodeId === nodeId)
            return;

        this._highlightedDOMNodeId = nodeId;
        if (nodeId)
            DOMAgent.highlightNode(nodeId, mode || "all");
        else
            DOMAgent.hideNodeHighlight();
    },

    highlightDOMNodeForTwoSeconds: function(nodeId)
    {
        this.highlightDOMNode(nodeId);
        this._hideDOMNodeHighlightTimeout = setTimeout(this.highlightDOMNode.bind(this, 0), 2000);
    },

    wireElementWithDOMNode: function(element, nodeId)
    {
        element.addEventListener("click", this._updateFocusedNode.bind(this, nodeId), false);
        element.addEventListener("mouseover", this.highlightDOMNode.bind(this, nodeId), false);
        element.addEventListener("mouseout", this.highlightDOMNode.bind(this, 0), false);
    },

    _updateFocusedNode: function(nodeId)
    {
        this.currentPanel = this.panels.elements;
        this.panels.elements.updateFocusedNode(nodeId);
    },

    get networkResources()
    {
        return this.panels.network.resources;
    },

    networkResourceById: function(id)
    {
        return this.panels.network.resourceById(id);
    },

    forAllResources: function(callback)
    {
        WebInspector.resourceTreeModel.forAllResources(callback);
    },

    resourceForURL: function(url)
    {
        return this.resourceTreeModel.resourceForURL(url);
    },

    openLinkExternallyLabel: function()
    {
        return WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Open link in new tab" : "Open Link in New Tab");
    }
}

WebInspector.PlatformFlavor = {
    WindowsVista: "windows-vista",
    MacTiger: "mac-tiger",
    MacLeopard: "mac-leopard",
    MacSnowLeopard: "mac-snowleopard"
};

(function parseQueryParameters()
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
})();

WebInspector.loaded = function()
{
    if ("page" in WebInspector.queryParamsObject) {
        var page = WebInspector.queryParamsObject.page;
        var host = "host" in WebInspector.queryParamsObject ? WebInspector.queryParamsObject.host : window.location.host;
        WebInspector.socket = new WebSocket("ws://" + host + "/devtools/page/" + page);
        WebInspector.socket.onmessage = function(message) { InspectorBackend.dispatch(message.data); }
        WebInspector.socket.onerror = function(error) { console.error(error); }
        WebInspector.socket.onopen = function() {
            InspectorFrontendHost.sendMessageToBackend = WebInspector.socket.send.bind(WebInspector.socket);
            InspectorFrontendHost.loaded = WebInspector.socket.send.bind(WebInspector.socket, "loaded");
            WebInspector.doLoadedDone();
        }
        return;
    }
    WebInspector.WorkerManager.loaded();
    WebInspector.doLoadedDone();
}

WebInspector.doLoadedDone = function()
{
    InspectorFrontendHost.loaded();

    var platform = WebInspector.platform;
    document.body.addStyleClass("platform-" + platform);
    var flavor = WebInspector.platformFlavor;
    if (flavor)
        document.body.addStyleClass("platform-" + flavor);
    var port = WebInspector.port;
    document.body.addStyleClass("port-" + port);
    if (WebInspector.socket)
        document.body.addStyleClass("remote");

    WebInspector.settings = new WebInspector.Settings();

    this._registerShortcuts();

    // set order of some sections explicitly
    WebInspector.shortcutsHelp.section(WebInspector.UIString("Console"));
    WebInspector.shortcutsHelp.section(WebInspector.UIString("Elements Panel"));

    this.drawer = new WebInspector.Drawer();
    this.console = new WebInspector.ConsoleView(this.drawer);
    this.drawer.visibleView = this.console;
    this.networkManager = new WebInspector.NetworkManager();
    this.resourceTreeModel = new WebInspector.ResourceTreeModel();
    this.domAgent = new WebInspector.DOMAgent();

    InspectorBackend.registerDomainDispatcher("Inspector", this);

    this.resourceCategories = {
        documents: new WebInspector.ResourceCategory("documents", WebInspector.UIString("Documents"), "rgb(47,102,236)"),
        stylesheets: new WebInspector.ResourceCategory("stylesheets", WebInspector.UIString("Stylesheets"), "rgb(157,231,119)"),
        images: new WebInspector.ResourceCategory("images", WebInspector.UIString("Images"), "rgb(164,60,255)"),
        scripts: new WebInspector.ResourceCategory("scripts", WebInspector.UIString("Scripts"), "rgb(255,121,0)"),
        xhr: new WebInspector.ResourceCategory("xhr", WebInspector.UIString("XHR"), "rgb(231,231,10)"),
        fonts: new WebInspector.ResourceCategory("fonts", WebInspector.UIString("Fonts"), "rgb(255,82,62)"),
        websockets: new WebInspector.ResourceCategory("websockets", WebInspector.UIString("WebSockets"), "rgb(186,186,186)"), // FIXME: Decide the color.
        other: new WebInspector.ResourceCategory("other", WebInspector.UIString("Other"), "rgb(186,186,186)")
    };

    this.cssModel = new WebInspector.CSSStyleModel();
    this.debuggerModel = new WebInspector.DebuggerModel();

    this.searchController = new WebInspector.SearchController();
    this.domBreakpointsSidebarPane = new WebInspector.DOMBreakpointsSidebarPane();

    this.panels = {};
    this._createPanels();
    this._panelHistory = new WebInspector.PanelHistory();
    this.toolbar = new WebInspector.Toolbar();
    this.toolbar.attached = WebInspector.attached;

    this.panelOrder = [];
    for (var panelName in this.panels)
        this.addPanel(this.panels[panelName]);

    this.Tips = {
        ResourceNotCompressed: {id: 0, message: WebInspector.UIString("You could save bandwidth by having your web server compress this transfer with gzip or zlib.")}
    };

    this.Warnings = {
        IncorrectMIMEType: {id: 0, message: WebInspector.UIString("Resource interpreted as %s but transferred with MIME type %s.")}
    };

    this.addMainEventListeners(document);

    window.addEventListener("resize", this.windowResize.bind(this), true);

    document.addEventListener("focus", this.focusChanged.bind(this), true);
    document.addEventListener("keydown", this.documentKeyDown.bind(this), false);
    document.addEventListener("beforecopy", this.documentCanCopy.bind(this), true);
    document.addEventListener("copy", this.documentCopy.bind(this), true);
    document.addEventListener("contextmenu", this.contextMenuEventFired.bind(this), true);

    var dockToggleButton = document.getElementById("dock-status-bar-item");
    dockToggleButton.addEventListener("click", this.toggleAttach.bind(this), false);

    if (this.attached)
        dockToggleButton.title = WebInspector.UIString("Undock into separate window.");
    else
        dockToggleButton.title = WebInspector.UIString("Dock to main window.");

    var errorWarningCount = document.getElementById("error-warning-count");
    errorWarningCount.addEventListener("click", this.showConsole.bind(this), false);
    this._updateErrorAndWarningCounts();

    this.extensionServer.initExtensions();

    if (WebInspector.settings.monitoringXHREnabled)
        ConsoleAgent.setMonitoringXHREnabled(true);
    ConsoleAgent.enable(this.console.setConsoleMessageExpiredCount.bind(this.console));

    DatabaseAgent.enable();
    DOMStorageAgent.enable();

    WebInspector.showPanel(WebInspector.settings.lastActivePanel);

    function propertyNamesCallback(error, names)
    {
        if (!error)
            WebInspector.cssNameCompletions = new WebInspector.CSSCompletions(names);
    }
    // As a DOMAgent method, this needs to happen after the frontend has loaded and the agent is available.
    CSSAgent.getSupportedCSSProperties(propertyNamesCallback);
}

WebInspector.addPanel = function(panel)
{
    this.panelOrder.push(panel);
    this.toolbar.addPanel(panel);
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

WebInspector.dispatch = function(message) {
    messagesToDispatch.push(message);
    setTimeout(function() {
        InspectorBackend.dispatch(messagesToDispatch.shift());
    }, 0);
}

WebInspector.dispatchMessageFromBackend = function(messageObject)
{
    WebInspector.dispatch(messageObject);
}

WebInspector.windowResize = function(event)
{
    if (this.currentPanel)
        this.currentPanel.resize();
    this.drawer.resize();
    this.toolbar.resize();
}

WebInspector.windowFocused = function(event)
{
    // Fires after blur, so when focusing on either the main inspector
    // or an <iframe> within the inspector we should always remove the
    // "inactive" class.
    if (event.target.document.nodeType === Node.DOCUMENT_NODE)
        document.body.removeStyleClass("inactive");
}

WebInspector.windowBlurred = function(event)
{
    // Leaving the main inspector or an <iframe> within the inspector.
    // We can add "inactive" now, and if we are moving the focus to another
    // part of the inspector then windowFocused will correct this.
    if (event.target.document.nodeType === Node.DOCUMENT_NODE)
        document.body.addStyleClass("inactive");
}

WebInspector.focusChanged = function(event)
{
    this.currentFocusElement = event.target;
}

WebInspector.setAttachedWindow = function(attached)
{
    this.attached = attached;
}

WebInspector.close = function(event)
{
    if (this._isClosing)
        return;
    this._isClosing = true;
    InspectorFrontendHost.closeWindow();
}

WebInspector.disconnectFromBackend = function()
{
    InspectorFrontendHost.disconnectFromBackend();
}

WebInspector.documentClick = function(event)
{
    var anchor = event.target.enclosingNodeOrSelfWithNodeName("a");
    if (!anchor || anchor.target === "_blank")
        return;

    // Prevent the link from navigating, since we don't do any navigation by following links normally.
    event.preventDefault();
    event.stopPropagation();

    function followLink()
    {
        if (WebInspector._showAnchorLocation(anchor))
            return;

        const profileMatch = WebInspector.ProfileType.URLRegExp.exec(anchor.href);
        if (profileMatch) {
            WebInspector.showProfileForURL(anchor.href);
            return;
        }

        var parsedURL = anchor.href.asParsedURL();
        if (parsedURL && parsedURL.scheme === "webkit-link-action") {
            if (parsedURL.host === "show-panel") {
                var panel = parsedURL.path.substring(1);
                if (WebInspector.panels[panel])
                    WebInspector.showPanel(panel);
            }
            return;
        }

        WebInspector.showPanel("resources");
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
    if (inResourcesPanel && resource) {
        WebInspector.panels.resources.showResource(resource);
        WebInspector.showPanel("resources");
    } else
        PageAgent.open(resourceURL, true);
}

WebInspector._registerShortcuts = function()
{
    var shortcut = WebInspector.KeyboardShortcut;
    var section = WebInspector.shortcutsHelp.section(WebInspector.UIString("All Panels"));
    var keys = [
        shortcut.shortcutToString("]", shortcut.Modifiers.CtrlOrMeta),
        shortcut.shortcutToString("[", shortcut.Modifiers.CtrlOrMeta)
    ];
    section.addRelatedKeys(keys, WebInspector.UIString("Next/previous panel"));
    section.addKey(shortcut.shortcutToString(shortcut.Keys.Esc), WebInspector.UIString("Toggle console"));
    section.addKey(shortcut.shortcutToString("f", shortcut.Modifiers.CtrlOrMeta), WebInspector.UIString("Search"));
    if (WebInspector.isMac()) {
        keys = [
            shortcut.shortcutToString("g", shortcut.Modifiers.Meta),
            shortcut.shortcutToString("g", shortcut.Modifiers.Meta | shortcut.Modifiers.Shift)
        ];
        section.addRelatedKeys(keys, WebInspector.UIString("Find next/previous"));
    }

    var goToShortcut = WebInspector.GoToLineDialog.createShortcut();
    section.addKey(goToShortcut.name, WebInspector.UIString("Go to Line"));
}

WebInspector.documentKeyDown = function(event)
{
    var isInputElement = event.target.nodeName === "INPUT";
    var isInEditMode = event.target.enclosingNodeOrSelfWithClass("text-prompt") || WebInspector.isEditingAnyField();
    const helpKey = WebInspector.isMac() ? "U+003F" : "U+00BF"; // "?" for both platforms

    if (event.keyIdentifier === "F1" ||
        (event.keyIdentifier === helpKey && event.shiftKey && (!isInEditMode && !isInputElement || event.metaKey))) {
        WebInspector.shortcutsHelp.show();
        event.stopPropagation();
        event.preventDefault();
        return;
    }

    if (WebInspector.isEditingAnyField())
        return;

    if (this.currentFocusElement && this.currentFocusElement.handleKeyEvent) {
        this.currentFocusElement.handleKeyEvent(event);
        if (event.handled) {
            event.preventDefault();
            return;
        }
    }

    if (this.currentPanel) {
        this.currentPanel.handleShortcut(event);
        if (event.handled) {
            event.preventDefault();
            return;
        }
    }

    WebInspector.searchController.handleShortcut(event);
    if (event.handled) {
        event.preventDefault();
        return;
    }

    var isMac = WebInspector.isMac();
    switch (event.keyIdentifier) {
        case "Left":
            var isBackKey = !isInEditMode && (isMac ? event.metaKey : event.ctrlKey);
            if (isBackKey && this._panelHistory.canGoBack()) {
                this._panelHistory.goBack();
                event.preventDefault();
            }
            break;

        case "Right":
            var isForwardKey = !isInEditMode && (isMac ? event.metaKey : event.ctrlKey);
            if (isForwardKey && this._panelHistory.canGoForward()) {
                this._panelHistory.goForward();
                event.preventDefault();
            }
            break;

        case "U+001B": // Escape key
            event.preventDefault();
            if (this.drawer.fullPanel)
                return;

            this.drawer.visible = !this.drawer.visible;
            break;

        // Windows and Mac have two different definitions of [, so accept both.
        case "U+005B":
        case "U+00DB": // [ key
            if (isMac)
                var isRotateLeft = event.metaKey && !event.shiftKey && !event.ctrlKey && !event.altKey;
            else
                var isRotateLeft = event.ctrlKey && !event.shiftKey && !event.metaKey && !event.altKey;

            if (isRotateLeft) {
                var index = this.panelOrder.indexOf(this.currentPanel);
                index = (index === 0) ? this.panelOrder.length - 1 : index - 1;
                this.panelOrder[index].toolbarItem.click();
                event.preventDefault();
            }

            break;

        // Windows and Mac have two different definitions of ], so accept both.
        case "U+005D":
        case "U+00DD":  // ] key
            if (isMac)
                var isRotateRight = event.metaKey && !event.shiftKey && !event.ctrlKey && !event.altKey;
            else
                var isRotateRight = event.ctrlKey && !event.shiftKey && !event.metaKey && !event.altKey;

            if (isRotateRight) {
                var index = this.panelOrder.indexOf(this.currentPanel);
                index = (index + 1) % this.panelOrder.length;
                this.panelOrder[index].toolbarItem.click();
                event.preventDefault();
            }

            break;

        case "U+0052": // R key
            if ((event.metaKey && isMac) || (event.ctrlKey && !isMac)) {
                PageAgent.reload(event.shiftKey);
                event.preventDefault();
            }
            break;
        case "F5":
            if (!isMac)
                PageAgent.reload(event.ctrlKey || event.shiftKey);
            break;
    }
}

WebInspector.documentCanCopy = function(event)
{
    if (this.currentPanel && this.currentPanel.handleCopyEvent)
        event.preventDefault();
}

WebInspector.documentCopy = function(event)
{
    if (this.currentPanel && this.currentPanel.handleCopyEvent)
        this.currentPanel.handleCopyEvent(event);
}

WebInspector.contextMenuEventFired = function(event)
{
    if (event.handled || event.target.hasStyleClass("popup-glasspane"))
        event.preventDefault();
}

WebInspector.animateStyle = function(animations, duration, callback)
{
    var interval;
    var complete = 0;
    var hasCompleted = false;

    const intervalDuration = (1000 / 30); // 30 frames per second.
    const animationsLength = animations.length;
    const propertyUnit = {opacity: ""};
    const defaultUnit = "px";

    function cubicInOut(t, b, c, d)
    {
        if ((t/=d/2) < 1) return c/2*t*t*t + b;
        return c/2*((t-=2)*t*t + 2) + b;
    }

    // Pre-process animations.
    for (var i = 0; i < animationsLength; ++i) {
        var animation = animations[i];
        var element = null, start = null, end = null, key = null;
        for (key in animation) {
            if (key === "element")
                element = animation[key];
            else if (key === "start")
                start = animation[key];
            else if (key === "end")
                end = animation[key];
        }

        if (!element || !end)
            continue;

        if (!start) {
            var computedStyle = element.ownerDocument.defaultView.getComputedStyle(element);
            start = {};
            for (key in end)
                start[key] = parseInt(computedStyle.getPropertyValue(key));
            animation.start = start;
        } else
            for (key in start)
                element.style.setProperty(key, start[key] + (key in propertyUnit ? propertyUnit[key] : defaultUnit));
    }

    function animateLoop()
    {
        // Advance forward.
        complete += intervalDuration;
        var next = complete + intervalDuration;

        // Make style changes.
        for (var i = 0; i < animationsLength; ++i) {
            var animation = animations[i];
            var element = animation.element;
            var start = animation.start;
            var end = animation.end;
            if (!element || !end)
                continue;

            var style = element.style;
            for (key in end) {
                var endValue = end[key];
                if (next < duration) {
                    var startValue = start[key];
                    var newValue = cubicInOut(complete, startValue, endValue - startValue, duration);
                    style.setProperty(key, newValue + (key in propertyUnit ? propertyUnit[key] : defaultUnit));
                } else
                    style.setProperty(key, endValue + (key in propertyUnit ? propertyUnit[key] : defaultUnit));
            }
        }

        // End condition.
        if (complete >= duration) {
            hasCompleted = true;
            clearInterval(interval);
            if (callback)
                callback();
        }
    }

    function forceComplete()
    {
        if (!hasCompleted) {
            complete = duration;
            animateLoop();
        }
    }

    function cancel()
    {
        hasCompleted = true;
        clearInterval(interval);
    }

    interval = setInterval(animateLoop, intervalDuration);
    return {
        cancel: cancel,
        forceComplete: forceComplete
    };
}

WebInspector.toggleAttach = function()
{
    if (!this.attached)
        InspectorFrontendHost.requestAttachWindow();
    else
        InspectorFrontendHost.requestDetachWindow();
}

WebInspector.elementDragStart = function(element, dividerDrag, elementDragEnd, event, cursor)
{
    if (this._elementDraggingEventListener || this._elementEndDraggingEventListener)
        this.elementDragEnd(event);

    this._elementDraggingEventListener = dividerDrag;
    this._elementEndDraggingEventListener = elementDragEnd;

    document.addEventListener("mousemove", dividerDrag, true);
    document.addEventListener("mouseup", elementDragEnd, true);

    document.body.style.cursor = cursor;

    event.preventDefault();
}

WebInspector.elementDragEnd = function(event)
{
    document.removeEventListener("mousemove", this._elementDraggingEventListener, true);
    document.removeEventListener("mouseup", this._elementEndDraggingEventListener, true);

    document.body.style.removeProperty("cursor");

    delete this._elementDraggingEventListener;
    delete this._elementEndDraggingEventListener;

    event.preventDefault();
}

WebInspector.toggleSearchingForNode = function()
{
    if (this.panels.elements) {
        this.showPanel("elements");
        this.panels.elements.toggleSearchingForNode();
    }
}

WebInspector.showConsole = function()
{
    this.drawer.showView(this.console);
}

WebInspector.showPanel = function(panel)
{
    if (!(panel in this.panels))
        panel = "elements";
    this.currentPanel = this.panels[panel];
}

WebInspector.startUserInitiatedDebugging = function()
{
    this.currentPanel = this.panels.scripts;
    WebInspector.debuggerModel.enableDebugger();
}

WebInspector.reset = function()
{
    this.debuggerModel.reset();

    for (var panelName in this.panels) {
        var panel = this.panels[panelName];
        if ("reset" in panel)
            panel.reset();
    }

    this.resources = {};
    this.highlightDOMNode(0);

    this.console.clearMessages();
    this.extensionServer.notifyInspectorReset();
}

WebInspector.bringToFront = function()
{
    InspectorFrontendHost.bringToFront();
}

WebInspector.didCreateWorker = function()
{
    var workersPane = WebInspector.panels.scripts.sidebarPanes.workers;
    workersPane.addWorker.apply(workersPane, arguments);
}

WebInspector.didDestroyWorker = function()
{
    var workersPane = WebInspector.panels.scripts.sidebarPanes.workers;
    workersPane.removeWorker.apply(workersPane, arguments);
}

WebInspector.log = function(message, messageLevel)
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
        var repeatCount = 1;
        if (message == WebInspector.log.lastMessage)
            repeatCount = WebInspector.log.repeatCount + 1;

        WebInspector.log.lastMessage = message;
        WebInspector.log.repeatCount = repeatCount;

        // ConsoleMessage expects a proxy object
        message = new WebInspector.RemoteObject.fromPrimitiveValue(message);

        // post the message
        var msg = new WebInspector.ConsoleMessage(
            WebInspector.ConsoleMessage.MessageSource.Other,
            WebInspector.ConsoleMessage.MessageType.Log,
            messageLevel || WebInspector.ConsoleMessage.MessageLevel.Debug,
            -1,
            null,
            repeatCount,
            null,
            [message],
            null);

        self.console.addMessage(msg);
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

WebInspector.drawLoadingPieChart = function(canvas, percent) {
    var g = canvas.getContext("2d");
    var darkColor = "rgb(122, 168, 218)";
    var lightColor = "rgb(228, 241, 251)";
    var cx = 8;
    var cy = 8;
    var r = 7;

    g.beginPath();
    g.arc(cx, cy, r, 0, Math.PI * 2, false);
    g.closePath();

    g.lineWidth = 1;
    g.strokeStyle = darkColor;
    g.fillStyle = lightColor;
    g.fill();
    g.stroke();

    var startangle = -Math.PI / 2;
    var endangle = startangle + (percent * Math.PI * 2);

    g.beginPath();
    g.moveTo(cx, cy);
    g.arc(cx, cy, r, startangle, endangle, false);
    g.closePath();

    g.fillStyle = darkColor;
    g.fill();
}

WebInspector.inspect = function(payload, hints)
{
    var object = WebInspector.RemoteObject.fromPayload(payload);
    if (object.type === "node") {
        // Request node from backend and focus it.
        object.pushNodeToFrontend(WebInspector.updateFocusedNode.bind(WebInspector), object.release.bind(object));
        return;
    }

    if (hints.databaseId) {
        WebInspector.currentPanel = WebInspector.panels.resources;
        WebInspector.panels.resources.selectDatabase(hints.databaseId);
    } else if (hints.domStorageId) {
        WebInspector.currentPanel = WebInspector.panels.resources;
        WebInspector.panels.resources.selectDOMStorage(hints.domStorageId);
    }

    object.release();
}

WebInspector.updateFocusedNode = function(nodeId)
{
    this._updateFocusedNode(nodeId);
    this.highlightDOMNodeForTwoSeconds(nodeId);
}

WebInspector.displayNameForURL = function(url)
{
    if (!url)
        return "";

    var resource = this.resourceForURL(url);
    if (resource)
        return resource.displayName;

    if (!WebInspector.mainResource)
        return url.trimURL("");

    var lastPathComponent = WebInspector.mainResource.lastPathComponent;
    var index = WebInspector.mainResource.url.indexOf(lastPathComponent);
    if (index !== -1 && index + lastPathComponent.length === WebInspector.mainResource.url.length) {
        var baseURL = WebInspector.mainResource.url.substring(0, index);
        if (url.indexOf(baseURL) === 0)
            return url.substring(index);
    }

    return url.trimURL(WebInspector.mainResource.domain);
}

WebInspector._showAnchorLocation = function(anchor)
{
    var preferedPanel = this.panels[anchor.getAttribute("preferred_panel") || "resources"];
    if (WebInspector._showAnchorLocationInPanel(anchor, preferedPanel))
        return true;
    if (preferedPanel !== this.panels.resources && WebInspector._showAnchorLocationInPanel(anchor, this.panels.resources))
        return true;
    return false;
}

WebInspector._showAnchorLocationInPanel = function(anchor, panel)
{
    if (!panel.canShowAnchorLocation(anchor))
        return false;

    // FIXME: support webkit-html-external-link links here.
    if (anchor.hasStyleClass("webkit-html-external-link")) {
        anchor.removeStyleClass("webkit-html-external-link");
        anchor.addStyleClass("webkit-html-resource-link");
    }

    this.currentPanel = panel;
    if (this.drawer)
        this.drawer.immediatelyFinishAnimation();
    this.currentPanel.showAnchorLocation(anchor);
    return true;
}

WebInspector.linkifyStringAsFragment = function(string)
{
    var container = document.createDocumentFragment();
    var linkStringRegEx = /(?:[a-zA-Z][a-zA-Z0-9+.-]{2,}:\/\/|www\.)[\w$\-_+*'=\|\/\\(){}[\]%@&#~,:;.!?]{2,}[\w$\-_+*=\|\/\\({%@&#~]/;
    var lineColumnRegEx = /:(\d+)(:(\d+))?$/;

    while (string) {
        var linkString = linkStringRegEx.exec(string);
        if (!linkString)
            break;

        linkString = linkString[0];
        var title = linkString;
        var linkIndex = string.indexOf(linkString);
        var nonLink = string.substring(0, linkIndex);
        container.appendChild(document.createTextNode(nonLink));

        var profileStringMatches = WebInspector.ProfileType.URLRegExp.exec(title);
        if (profileStringMatches)
            title = WebInspector.panels.profiles.displayTitleForProfileLink(profileStringMatches[2], profileStringMatches[1]);

        var realURL = (linkString.indexOf("www.") === 0 ? "http://" + linkString : linkString);
        var lineColumnMatch = lineColumnRegEx.exec(realURL);
        if (lineColumnMatch)
            realURL = realURL.substring(0, realURL.length - lineColumnMatch[0].length);

        var hasResourceWithURL = !!WebInspector.resourceForURL(realURL);
        var urlNode = WebInspector.linkifyURLAsNode(realURL, title, null, hasResourceWithURL);
        container.appendChild(urlNode);
        if (lineColumnMatch) {
            urlNode.setAttribute("line_number", lineColumnMatch[1]);
            urlNode.setAttribute("preferred_panel", "scripts");
        }
        string = string.substring(linkIndex + linkString.length, string.length);
    }

    if (string)
        container.appendChild(document.createTextNode(string));

    return container;
}

WebInspector.showProfileForURL = function(url)
{
    WebInspector.showPanel("profiles");
    WebInspector.panels.profiles.showProfileForURL(url);
}

WebInspector.linkifyURLAsNode = function(url, linkText, classes, isExternal, tooltipText)
{
    if (!linkText)
        linkText = url;
    classes = (classes ? classes + " " : "");
    classes += isExternal ? "webkit-html-external-link" : "webkit-html-resource-link";

    var a = document.createElement("a");
    a.href = url;
    a.className = classes;
    if (typeof tooltipText === "undefined")
        a.title = url;
    else if (typeof tooltipText !== "string" || tooltipText.length)
        a.title = tooltipText;
    a.textContent = linkText;
    a.style.maxWidth = "100%";
    if (isExternal)
        a.setAttribute("target", "_blank");

    return a;
}

WebInspector.linkifyURL = function(url, linkText, classes, isExternal, tooltipText)
{
    // Use the DOM version of this function so as to avoid needing to escape attributes.
    // FIXME:  Get rid of linkifyURL entirely.
    return WebInspector.linkifyURLAsNode(url, linkText, classes, isExternal, tooltipText).outerHTML;
}

WebInspector.linkifyResourceAsNode = function(url, preferredPanel, lineNumber, classes, tooltipText)
{
    var linkText = WebInspector.displayNameForURL(url);
    if (lineNumber)
        linkText += ":" + lineNumber;
    var node = WebInspector.linkifyURLAsNode(url, linkText, classes, false, tooltipText);
    node.setAttribute("line_number", lineNumber);
    node.setAttribute("preferred_panel", preferredPanel);
    return node;
}

WebInspector.resourceURLForRelatedNode = function(node, url)
{
    if (!url || url.indexOf("://") > 0)
        return url;

    for (var frameOwnerCandidate = node; frameOwnerCandidate; frameOwnerCandidate = frameOwnerCandidate.parentNode) {
        if (frameOwnerCandidate.documentURL) {
            var result = WebInspector.completeURL(frameOwnerCandidate.documentURL, url);
            if (result)
                return result;
            break;
        }
    }

    // documentURL not found or has bad value
    var resourceURL = url;
    function callback(resource)
    {
        if (resource.path === url) {
            resourceURL = resource.url;
            return true;
        }
    }
    WebInspector.forAllResources(callback);
    return resourceURL;
}

WebInspector.completeURL = function(baseURL, href)
{
    if (href) {
        // Return absolute URLs as-is.
        var parsedHref = href.asParsedURL();
        if (parsedHref && parsedHref.scheme)
            return href;
    }

    var parsedURL = baseURL.asParsedURL();
    if (parsedURL) {
        var path = href;
        if (path.charAt(0) !== "/") {
            var basePath = parsedURL.path;
            // A href of "?foo=bar" implies "basePath?foo=bar".
            // With "basePath?a=b" and "?foo=bar" we should get "basePath?foo=bar".
            var prefix;
            if (path.charAt(0) === "?") {
                var basePathCutIndex = basePath.indexOf("?");
                if (basePathCutIndex !== -1)
                    prefix = basePath.substring(0, basePathCutIndex);
                else
                    prefix = basePath;
            } else
                prefix = basePath.substring(0, basePath.lastIndexOf("/")) + "/";

            path = prefix + path;
        } else if (path.length > 1 && path.charAt(1) === "/") {
            // href starts with "//" which is a full URL with the protocol dropped (use the baseURL protocol).
            return parsedURL.scheme + ":" + path;
        }
        return parsedURL.scheme + "://" + parsedURL.host + (parsedURL.port ? (":" + parsedURL.port) : "") + path;
    }
    return null;
}

WebInspector.addMainEventListeners = function(doc)
{
    doc.defaultView.addEventListener("focus", this.windowFocused.bind(this), false);
    doc.defaultView.addEventListener("blur", this.windowBlurred.bind(this), false);
    doc.addEventListener("click", this.documentClick.bind(this), true);
}

WebInspector.frontendReused = function()
{
    ConsoleAgent.enable(this.console.setConsoleMessageExpiredCount.bind(this.console));
    DatabaseAgent.enable();
    DOMStorageAgent.enable();

    this.networkManager.frontendReused();
    this.resourceTreeModel.frontendReused();
    WebInspector.panels.network.clear();
    this.reset();
}

WebInspector.UIString = function(string)
{
    if (window.localizedStrings && string in window.localizedStrings)
        string = window.localizedStrings[string];
    else {
        if (!(string in WebInspector.missingLocalizedStrings)) {
            if (!WebInspector.InspectorBackendStub)
                console.warn("Localized string \"" + string + "\" not found.");
            WebInspector.missingLocalizedStrings[string] = true;
        }

        if (Preferences.showMissingLocalizedStrings)
            string += " (not localized)";
    }

    return String.vsprintf(string, Array.prototype.slice.call(arguments, 1));
}

WebInspector.formatLocalized = function(format, substitutions, formatters, initialValue, append)
{
    return String.format(WebInspector.UIString(format), substitutions, formatters, initialValue, append);
}

WebInspector.isMac = function()
{
    if (!("_isMac" in this))
        this._isMac = WebInspector.platform === "mac";

    return this._isMac;
}

WebInspector.useLowerCaseMenuTitles = function()
{
    return WebInspector.platform === "windows" && Preferences.useLowerCaseMenuTitlesOnWindows;
}

WebInspector.isBeingEdited = function(element)
{
    return element.__editing;
}

WebInspector.markBeingEdited = function(element, value)
{
    if (value) {
        if (element.__editing)
            return false;
        element.__editing = true;
        WebInspector.__editingCount = (WebInspector.__editingCount || 0) + 1;
    } else {
        if (!element.__editing)
            return false;
        delete element.__editing;
        --WebInspector.__editingCount;
    }
    return true;
}

WebInspector.isEditingAnyField = function()
{
    return !!WebInspector.__editingCount;
}

// Available config fields (all optional):
// context: Object - an arbitrary context object to be passed to the commit and cancel handlers
// commitHandler: Function - handles editing "commit" outcome
// cancelHandler: Function - handles editing "cancel" outcome
// customFinishHandler: Function - custom finish handler for the editing session (invoked on keydown)
// pasteHandler: Function - handles the "paste" event, return values are the same as those for customFinishHandler
// multiline: Boolean - whether the edited element is multiline
WebInspector.startEditing = function(element, config)
{
    if (!WebInspector.markBeingEdited(element, true))
        return;

    config = config || {};
    var committedCallback = config.commitHandler;
    var cancelledCallback = config.cancelHandler;
    var pasteCallback = config.pasteHandler;
    var context = config.context;
    var oldText = getContent(element);
    var moveDirection = "";

    element.addStyleClass("editing");

    var oldTabIndex = element.tabIndex;
    if (element.tabIndex < 0)
        element.tabIndex = 0;

    function blurEventListener() {
        editingCommitted.call(element);
    }

    function getContent(element) {
        if (element.tagName === "INPUT" && element.type === "text")
            return element.value;
        else
            return element.textContent;
    }

    function cleanUpAfterEditing() {
        WebInspector.markBeingEdited(element, false);

        this.removeStyleClass("editing");
        this.tabIndex = oldTabIndex;
        this.scrollTop = 0;
        this.scrollLeft = 0;

        element.removeEventListener("blur", blurEventListener, false);
        element.removeEventListener("keydown", keyDownEventListener, true);
        if (pasteCallback)
            element.removeEventListener("paste", pasteEventListener, true);

        if (element === WebInspector.currentFocusElement || element.isAncestor(WebInspector.currentFocusElement))
            WebInspector.currentFocusElement = WebInspector.previousFocusElement;
    }

    function editingCancelled() {
        if (this.tagName === "INPUT" && this.type === "text")
            this.value = oldText;
        else
            this.textContent = oldText;

        cleanUpAfterEditing.call(this);

        if (cancelledCallback)
            cancelledCallback(this, context);
    }

    function editingCommitted() {
        cleanUpAfterEditing.call(this);

        if (committedCallback)
            committedCallback(this, getContent(this), oldText, context, moveDirection);
    }

    function defaultFinishHandler(event)
    {
        var isMetaOrCtrl = WebInspector.isMac() ?
            event.metaKey && !event.shiftKey && !event.ctrlKey && !event.altKey :
            event.ctrlKey && !event.shiftKey && !event.metaKey && !event.altKey;
        if (isEnterKey(event) && (event.isMetaOrCtrlForTest || !config.multiline || isMetaOrCtrl))
            return "commit";
        else if (event.keyCode === WebInspector.KeyboardShortcut.Keys.Esc.code || event.keyIdentifier === "U+001B")
            return "cancel";
        else if (event.keyIdentifier === "U+0009") // Tab key
            return "move-" + (event.shiftKey ? "backward" : "forward");
    }

    function handleEditingResult(result, event)
    {
        if (result === "commit") {
            editingCommitted.call(element);
            event.preventDefault();
            event.stopPropagation();
        } else if (result === "cancel") {
            editingCancelled.call(element);
            event.preventDefault();
            event.stopPropagation();
        } else if (result && result.indexOf("move-") === 0) {
            moveDirection = result.substring(5);
            if (event.keyIdentifier !== "U+0009")
                blurEventListener();
        }
    }

    function pasteEventListener(event)
    {
        var result = pasteCallback(event);
        handleEditingResult(result, event);
    }

    function keyDownEventListener(event)
    {
        var handler = config.customFinishHandler || defaultFinishHandler;
        var result = handler(event);
        handleEditingResult(result, event);
    }

    element.addEventListener("blur", blurEventListener, false);
    element.addEventListener("keydown", keyDownEventListener, true);
    if (pasteCallback)
        element.addEventListener("paste", pasteEventListener, true);

    WebInspector.currentFocusElement = element;
    return {
        cancel: editingCancelled.bind(element),
        commit: editingCommitted.bind(element)
    };
}

WebInspector._toolbarItemClicked = function(event)
{
    var toolbarItem = event.currentTarget;
    this.currentPanel = toolbarItem.panel;
}

// This table maps MIME types to the Resource.Types which are valid for them.
// The following line:
//    "text/html":                {0: 1},
// means that text/html is a valid MIME type for resources that have type
// WebInspector.Resource.Type.Document (which has a value of 0).
WebInspector.MIMETypes = {
    "text/html":                   {0: true},
    "text/xml":                    {0: true},
    "text/plain":                  {0: true},
    "application/xhtml+xml":       {0: true},
    "text/css":                    {1: true},
    "text/xsl":                    {1: true},
    "image/jpeg":                  {2: true},
    "image/png":                   {2: true},
    "image/gif":                   {2: true},
    "image/bmp":                   {2: true},
    "image/vnd.microsoft.icon":    {2: true},
    "image/x-icon":                {2: true},
    "image/x-xbitmap":             {2: true},
    "font/ttf":                    {3: true},
    "font/opentype":               {3: true},
    "application/x-font-type1":    {3: true},
    "application/x-font-ttf":      {3: true},
    "application/x-font-woff":     {3: true},
    "application/x-truetype-font": {3: true},
    "text/javascript":             {4: true},
    "text/ecmascript":             {4: true},
    "application/javascript":      {4: true},
    "application/ecmascript":      {4: true},
    "application/x-javascript":    {4: true},
    "text/javascript1.1":          {4: true},
    "text/javascript1.2":          {4: true},
    "text/javascript1.3":          {4: true},
    "text/jscript":                {4: true},
    "text/livescript":             {4: true},
}

WebInspector.PanelHistory = function()
{
    this._history = [];
    this._historyIterator = -1;
}

WebInspector.PanelHistory.prototype = {
    canGoBack: function()
    {
        return this._historyIterator > 0;
    },

    goBack: function()
    {
        this._inHistory = true;
        WebInspector.currentPanel = WebInspector.panels[this._history[--this._historyIterator]];
        delete this._inHistory;
    },

    canGoForward: function()
    {
        return this._historyIterator < this._history.length - 1;
    },

    goForward: function()
    {
        this._inHistory = true;
        WebInspector.currentPanel = WebInspector.panels[this._history[++this._historyIterator]];
        delete this._inHistory;
    },

    setPanel: function(panelName)
    {
        if (this._inHistory)
            return;

        this._history.splice(this._historyIterator + 1, this._history.length - this._historyIterator - 1);
        if (!this._history.length || this._history[this._history.length - 1] !== panelName)
            this._history.push(panelName);
        this._historyIterator = this._history.length - 1;
    }
}
