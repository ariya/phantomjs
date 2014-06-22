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

function defineCommonExtensionSymbols(apiPrivate)
{
    if (!apiPrivate.audits)
        apiPrivate.audits = {};
    apiPrivate.audits.Severity = {
        Info: "info",
        Warning: "warning",
        Severe: "severe"
    };

    if (!apiPrivate.console)
        apiPrivate.console = {};
    apiPrivate.console.Severity = {
        Debug: "debug",
        Log: "log",
        Warning: "warning",
        Error: "error"
    };

    if (!apiPrivate.panels)
        apiPrivate.panels = {};
    apiPrivate.panels.SearchAction = {
        CancelSearch: "cancelSearch",
        PerformSearch: "performSearch",
        NextSearchResult: "nextSearchResult",
        PreviousSearchResult: "previousSearchResult"
    };

    apiPrivate.Events = {
        AuditStarted: "audit-started-",
        ButtonClicked: "button-clicked-",
        ConsoleMessageAdded: "console-message-added",
        ElementsPanelObjectSelected: "panel-objectSelected-elements",
        NetworkRequestFinished: "network-request-finished",
        OpenResource: "open-resource",
        PanelSearch: "panel-search-",
        Reload: "Reload",
        ResourceAdded: "resource-added",
        ResourceContentCommitted: "resource-content-committed",
        TimelineEventRecorded: "timeline-event-recorded",
        ViewShown: "view-shown-",
        ViewHidden: "view-hidden-"
    };

    apiPrivate.Commands = {
        AddAuditCategory: "addAuditCategory",
        AddAuditResult: "addAuditResult",
        AddConsoleMessage: "addConsoleMessage",
        AddRequestHeaders: "addRequestHeaders",
        CreatePanel: "createPanel",
        CreateSidebarPane: "createSidebarPane",
        CreateStatusBarButton: "createStatusBarButton",
        EvaluateOnInspectedPage: "evaluateOnInspectedPage",
        GetConsoleMessages: "getConsoleMessages",
        GetHAR: "getHAR",
        GetPageResources: "getPageResources",
        GetRequestContent: "getRequestContent",
        GetResourceContent: "getResourceContent",
        Subscribe: "subscribe",
        SetOpenResourceHandler: "setOpenResourceHandler",
        SetResourceContent: "setResourceContent",
        SetSidebarContent: "setSidebarContent",
        SetSidebarHeight: "setSidebarHeight",
        SetSidebarPage: "setSidebarPage",
        ShowPanel: "showPanel",
        StopAuditCategoryRun: "stopAuditCategoryRun",
        Unsubscribe: "unsubscribe",
        UpdateAuditProgress: "updateAuditProgress",
        UpdateButton: "updateButton",
        InspectedURLChanged: "inspectedURLChanged"
    };
}

function injectedExtensionAPI(injectedScriptId)
{

var apiPrivate = {};

defineCommonExtensionSymbols(apiPrivate);

var commands = apiPrivate.Commands;
var events = apiPrivate.Events;
var userAction = false;

// Here and below, all constructors are private to API implementation.
// For a public type Foo, if internal fields are present, these are on
// a private FooImpl type, an instance of FooImpl is used in a closure
// by Foo consutrctor to re-bind publicly exported members to an instance
// of Foo.

/**
 * @constructor
 */
function EventSinkImpl(type, customDispatch)
{
    this._type = type;
    this._listeners = [];
    this._customDispatch = customDispatch;
}

EventSinkImpl.prototype = {
    addListener: function(callback)
    {
        if (typeof callback !== "function")
            throw "addListener: callback is not a function";
        if (this._listeners.length === 0)
            extensionServer.sendRequest({ command: commands.Subscribe, type: this._type });
        this._listeners.push(callback);
        extensionServer.registerHandler("notify-" + this._type, this._dispatch.bind(this));
    },

    removeListener: function(callback)
    {
        var listeners = this._listeners;

        for (var i = 0; i < listeners.length; ++i) {
            if (listeners[i] === callback) {
                listeners.splice(i, 1);
                break;
            }
        }
        if (this._listeners.length === 0)
            extensionServer.sendRequest({ command: commands.Unsubscribe, type: this._type });
    },

    _fire: function()
    {
        var listeners = this._listeners.slice();
        for (var i = 0; i < listeners.length; ++i)
            listeners[i].apply(null, arguments);
    },

    _dispatch: function(request)
    {
         if (this._customDispatch)
             this._customDispatch.call(this, request);
         else
             this._fire.apply(this, request.arguments);
    }
}

/**
 * @constructor
 */
function InspectorExtensionAPI()
{
    this.audits = new Audits();
    this.inspectedWindow = new InspectedWindow();
    this.panels = new Panels();
    this.network = new Network();
    defineDeprecatedProperty(this, "webInspector", "resources", "network");
    this.timeline = new Timeline();
    this.console = new ConsoleAPI();
}

/**
 * @constructor
 */
InspectorExtensionAPI.prototype = {
    log: function(message)
    {
        extensionServer.sendRequest({ command: commands.Log, message: message });
    }
}

/**
 * @constructor
 */
function ConsoleAPI()
{
    this.onMessageAdded = new EventSink(events.ConsoleMessageAdded);
}

ConsoleAPI.prototype = {
    getMessages: function(callback)
    {
        extensionServer.sendRequest({ command: commands.GetConsoleMessages }, callback);
    },

    addMessage: function(severity, text, url, line)
    {
        extensionServer.sendRequest({ command: commands.AddConsoleMessage, severity: severity, text: text, url: url, line: line });
    },

    get Severity()
    {
        return apiPrivate.console.Severity;
    }
}

/**
 * @constructor
 */
function Network()
{
    function dispatchRequestEvent(message)
    {
        var request = message.arguments[1];
        request.__proto__ = new Request(message.arguments[0]);
        this._fire(request);
    }
    this.onRequestFinished = new EventSink(events.NetworkRequestFinished, dispatchRequestEvent);
    defineDeprecatedProperty(this, "network", "onFinished", "onRequestFinished");
    this.onNavigated = new EventSink(events.InspectedURLChanged);
}

Network.prototype = {
    getHAR: function(callback)
    {
        function callbackWrapper(result)
        {
            var entries = (result && result.entries) || [];
            for (var i = 0; i < entries.length; ++i) {
                entries[i].__proto__ = new Request(entries[i]._requestId);
                delete entries[i]._requestId;
            }
            callback(result);
        }
        return extensionServer.sendRequest({ command: commands.GetHAR }, callback && callbackWrapper);
    },

    addRequestHeaders: function(headers)
    {
        return extensionServer.sendRequest({ command: commands.AddRequestHeaders, headers: headers, extensionId: window.location.hostname });
    }
}

/**
 * @constructor
 */
function RequestImpl(id)
{
    this._id = id;
}

RequestImpl.prototype = {
    getContent: function(callback)
    {
        function callbackWrapper(response)
        {
            callback(response.content, response.encoding);
        }
        extensionServer.sendRequest({ command: commands.GetRequestContent, id: this._id }, callback && callbackWrapper);
    }
}

/**
 * @constructor
 */
function Panels()
{
    var panels = {
        elements: new ElementsPanel()
    };

    function panelGetter(name)
    {
        return panels[name];
    }
    for (var panel in panels)
        this.__defineGetter__(panel, panelGetter.bind(null, panel));
}

Panels.prototype = {
    create: function(title, icon, page, callback)
    {
        var id = "extension-panel-" + extensionServer.nextObjectId();
        var request = {
            command: commands.CreatePanel,
            id: id,
            title: title,
            icon: icon,
            page: page
        };
        extensionServer.sendRequest(request, callback && callback.bind(this, new ExtensionPanel(id)));
    },

    setOpenResourceHandler: function(callback)
    {
        var hadHandler = extensionServer.hasHandler(events.OpenResource);

        if (!callback)
            extensionServer.unregisterHandler(events.OpenResource);
        else {
            function callbackWrapper(message)
            {
                // Allow the panel to show itself when handling the event.
                userAction = true;
                try {
                    callback.call(null, new Resource(message.resource), message.lineNumber);
                } finally {
                    userAction = false;
                }
            }
            extensionServer.registerHandler(events.OpenResource, callbackWrapper);
        }
        // Only send command if we either removed an existing handler or added handler and had none before.
        if (hadHandler === !callback)
            extensionServer.sendRequest({ command: commands.SetOpenResourceHandler, "handlerPresent": !!callback });
    },

    get SearchAction()
    {
        return apiPrivate.panels.SearchAction;
    }
}

/**
 * @constructor
 */
function ExtensionViewImpl(id)
{
    this._id = id;

    function dispatchShowEvent(message)
    {
        var frameIndex = message.arguments[0];
        this._fire(window.parent.frames[frameIndex]);
    }
    this.onShown = new EventSink(events.ViewShown + id, dispatchShowEvent);
    this.onHidden = new EventSink(events.ViewHidden + id);
}

/**
 * @constructor
 */
function PanelWithSidebarImpl(id)
{
    this._id = id;
}

PanelWithSidebarImpl.prototype = {
    createSidebarPane: function(title, callback)
    {
        var id = "extension-sidebar-" + extensionServer.nextObjectId();
        var request = {
            command: commands.CreateSidebarPane,
            panel: this._id,
            id: id,
            title: title
        };
        function callbackWrapper()
        {
            callback(new ExtensionSidebarPane(id));
        }
        extensionServer.sendRequest(request, callback && callbackWrapper);
    },

    __proto__: ExtensionViewImpl.prototype
}

/**
 * @constructor
 * @extends {PanelWithSidebar}
 */
function ElementsPanel()
{
    var id = "elements";
    PanelWithSidebar.call(this, id);
    this.onSelectionChanged = new EventSink(events.ElementsPanelObjectSelected);
}

/**
 * @constructor
 * @extends {ExtensionViewImpl}
 */
function ExtensionPanelImpl(id)
{
    ExtensionViewImpl.call(this, id);
    this.onSearch = new EventSink(events.PanelSearch + id);
}

ExtensionPanelImpl.prototype = {
    createStatusBarButton: function(iconPath, tooltipText, disabled)
    {
        var id = "button-" + extensionServer.nextObjectId();
        var request = {
            command: commands.CreateStatusBarButton,
            panel: this._id,
            id: id,
            icon: iconPath,
            tooltip: tooltipText,
            disabled: !!disabled
        };
        extensionServer.sendRequest(request);
        return new Button(id);
    },

    show: function()
    {
        if (!userAction)
            return;

        var request = {
            command: commands.ShowPanel,
            id: this._id
        };
        extensionServer.sendRequest(request);
    },

    __proto__: ExtensionViewImpl.prototype
}

/**
 * @constructor
 * @extends {ExtensionViewImpl}
 */
function ExtensionSidebarPaneImpl(id)
{
    ExtensionViewImpl.call(this, id);
}

ExtensionSidebarPaneImpl.prototype = {
    setHeight: function(height)
    {
        extensionServer.sendRequest({ command: commands.SetSidebarHeight, id: this._id, height: height });
    },

    setExpression: function(expression, rootTitle, evaluateOptions)
    {
        var request = {
            command: commands.SetSidebarContent,
            id: this._id,
            expression: expression,
            rootTitle: rootTitle,
            evaluateOnPage: true,
        };
        if (typeof evaluateOptions === "object")
            request.evaluateOptions = evaluateOptions;
        extensionServer.sendRequest(request, extractCallbackArgument(arguments));
    },

    setObject: function(jsonObject, rootTitle, callback)
    {
        extensionServer.sendRequest({ command: commands.SetSidebarContent, id: this._id, expression: jsonObject, rootTitle: rootTitle }, callback);
    },

    setPage: function(page)
    {
        extensionServer.sendRequest({ command: commands.SetSidebarPage, id: this._id, page: page });
    }
}

/**
 * @constructor
 */
function ButtonImpl(id)
{
    this._id = id;
    this.onClicked = new EventSink(events.ButtonClicked + id);
}

ButtonImpl.prototype = {
    update: function(iconPath, tooltipText, disabled)
    {
        var request = {
            command: commands.UpdateButton,
            id: this._id,
            icon: iconPath,
            tooltip: tooltipText,
            disabled: !!disabled
        };
        extensionServer.sendRequest(request);
    }
};

/**
 * @constructor
 */
function Audits()
{
}

Audits.prototype = {
    addCategory: function(displayName, resultCount)
    {
        var id = "extension-audit-category-" + extensionServer.nextObjectId();
        if (typeof resultCount !== "undefined")
            console.warn("Passing resultCount to audits.addCategory() is deprecated. Use AuditResult.updateProgress() instead.");
        extensionServer.sendRequest({ command: commands.AddAuditCategory, id: id, displayName: displayName, resultCount: resultCount });
        return new AuditCategory(id);
    }
}

/**
 * @constructor
 */
function AuditCategoryImpl(id)
{
    function dispatchAuditEvent(request)
    {
        var auditResult = new AuditResult(request.arguments[0]);
        try {
            this._fire(auditResult);
        } catch (e) {
            console.error("Uncaught exception in extension audit event handler: " + e);
            auditResult.done();
        }
    }
    this._id = id;
    this.onAuditStarted = new EventSink(events.AuditStarted + id, dispatchAuditEvent);
}

/**
 * @constructor
 */
function AuditResultImpl(id)
{
    this._id = id;

    this.createURL = this._nodeFactory.bind(null, "url");
    this.createSnippet = this._nodeFactory.bind(null, "snippet");
    this.createText = this._nodeFactory.bind(null, "text");
    this.createObject = this._nodeFactory.bind(null, "object");
    this.createNode = this._nodeFactory.bind(null, "node");
}

AuditResultImpl.prototype = {
    addResult: function(displayName, description, severity, details)
    {
        // shorthand for specifying details directly in addResult().
        if (details && !(details instanceof AuditResultNode))
            details = new AuditResultNode(details instanceof Array ? details : [details]);

        var request = {
            command: commands.AddAuditResult,
            resultId: this._id,
            displayName: displayName,
            description: description,
            severity: severity,
            details: details
        };
        extensionServer.sendRequest(request);
    },

    createResult: function()
    {
        return new AuditResultNode(Array.prototype.slice.call(arguments));
    },

    updateProgress: function(worked, totalWork)
    {
        extensionServer.sendRequest({ command: commands.UpdateAuditProgress, resultId: this._id, progress: worked / totalWork });
    },

    done: function()
    {
        extensionServer.sendRequest({ command: commands.StopAuditCategoryRun, resultId: this._id });
    },

    get Severity()
    {
        return apiPrivate.audits.Severity;
    },

    createResourceLink: function(url, lineNumber)
    {
        return {
            type: "resourceLink",
            arguments: [url, lineNumber && lineNumber - 1]
        };
    },

    _nodeFactory: function(type)
    {
        return {
            type: type,
            arguments: Array.prototype.slice.call(arguments, 1)
        };
    }
}

/**
 * @constructor
 */
function AuditResultNode(contents)
{
    this.contents = contents;
    this.children = [];
    this.expanded = false;
}

AuditResultNode.prototype = {
    addChild: function()
    {
        var node = new AuditResultNode(Array.prototype.slice.call(arguments));
        this.children.push(node);
        return node;
    }
};

/**
 * @constructor
 */
function InspectedWindow()
{
    function dispatchResourceEvent(message)
    {
        this._fire(new Resource(message.arguments[0]));
    }
    function dispatchResourceContentEvent(message)
    {
        this._fire(new Resource(message.arguments[0]), message.arguments[1]);
    }
    this.onResourceAdded = new EventSink(events.ResourceAdded, dispatchResourceEvent);
    this.onResourceContentCommitted = new EventSink(events.ResourceContentCommitted, dispatchResourceContentEvent);
}

InspectedWindow.prototype = {
    reload: function(optionsOrUserAgent)
    {
        var options = null;
        if (typeof optionsOrUserAgent === "object")
            options = optionsOrUserAgent;
        else if (typeof optionsOrUserAgent === "string") {
            options = { userAgent: optionsOrUserAgent };
            console.warn("Passing userAgent as string parameter to inspectedWindow.reload() is deprecated. " +
                         "Use inspectedWindow.reload({ userAgent: value}) instead.");
        }
        return extensionServer.sendRequest({ command: commands.Reload, options: options });
    },

    eval: function(expression, evaluateOptions)
    {
        var callback = extractCallbackArgument(arguments);
        function callbackWrapper(result)
        {
            if (result.isError || result.isException)
                callback(undefined, result);
            else
                callback(result.value);
        }
        var request = {
            command: commands.EvaluateOnInspectedPage,
            expression: expression
        };
        if (typeof evaluateOptions === "object")
            request.evaluateOptions = evaluateOptions;
        return extensionServer.sendRequest(request, callback && callbackWrapper);
    },

    getResources: function(callback)
    {
        function wrapResource(resourceData)
        {
            return new Resource(resourceData);
        }
        function callbackWrapper(resources)
        {
            callback(resources.map(wrapResource));
        }
        return extensionServer.sendRequest({ command: commands.GetPageResources }, callback && callbackWrapper);
    }
}

/**
 * @constructor
 */
function ResourceImpl(resourceData)
{
    this._url = resourceData.url
    this._type = resourceData.type;
}

ResourceImpl.prototype = {
    get url()
    {
        return this._url;
    },

    get type()
    {
        return this._type;
    },

    getContent: function(callback)
    {
        function callbackWrapper(response)
        {
            callback(response.content, response.encoding);
        }

        return extensionServer.sendRequest({ command: commands.GetResourceContent, url: this._url }, callback && callbackWrapper);
    },

    setContent: function(content, commit, callback)
    {
        return extensionServer.sendRequest({ command: commands.SetResourceContent, url: this._url, content: content, commit: commit }, callback);
    }
}

/**
 * @constructor
 */
function TimelineImpl()
{
    this.onEventRecorded = new EventSink(events.TimelineEventRecorded);
}

/**
 * @constructor
 */
function ExtensionServerClient()
{
    this._callbacks = {};
    this._handlers = {};
    this._lastRequestId = 0;
    this._lastObjectId = 0;

    this.registerHandler("callback", this._onCallback.bind(this));

    var channel = new MessageChannel();
    this._port = channel.port1;
    this._port.addEventListener("message", this._onMessage.bind(this), false);
    this._port.start();

    window.parent.postMessage("registerExtension", [ channel.port2 ], "*");
}

ExtensionServerClient.prototype = {
    /**
     * @param {function()=} callback
     */
    sendRequest: function(message, callback)
    {
        if (typeof callback === "function")
            message.requestId = this._registerCallback(callback);
        return this._port.postMessage(message);
    },

    hasHandler: function(command)
    {
        return !!this._handlers[command];
    },

    registerHandler: function(command, handler)
    {
        this._handlers[command] = handler;
    },

    unregisterHandler: function(command)
    {
        delete this._handlers[command];
    },

    nextObjectId: function()
    {
        return injectedScriptId + "_" + ++this._lastObjectId;
    },

    _registerCallback: function(callback)
    {
        var id = ++this._lastRequestId;
        this._callbacks[id] = callback;
        return id;
    },

    _onCallback: function(request)
    {
        if (request.requestId in this._callbacks) {
            var callback = this._callbacks[request.requestId];
            delete this._callbacks[request.requestId];
            callback(request.result);
        }
    },

    _onMessage: function(event)
    {
        var request = event.data;
        var handler = this._handlers[request.command];
        if (handler)
            handler.call(this, request);
    }
}

function populateInterfaceClass(interface, implementation)
{
    for (var member in implementation) {
        if (member.charAt(0) === "_")
            continue;
        var descriptor = null;
        // Traverse prototype chain until we find the owner.
        for (var owner = implementation; owner && !descriptor; owner = owner.__proto__)
            descriptor = Object.getOwnPropertyDescriptor(owner, member);
        if (!descriptor)
            continue;
        if (typeof descriptor.value === "function")
            interface[member] = descriptor.value.bind(implementation);
        else if (typeof descriptor.get === "function")
            interface.__defineGetter__(member, descriptor.get.bind(implementation));
        else
            Object.defineProperty(interface, member, descriptor);
    }
}

function declareInterfaceClass(implConstructor)
{
    return function()
    {
        var impl = { __proto__: implConstructor.prototype };
        implConstructor.apply(impl, arguments);
        populateInterfaceClass(this, impl);
    }
}

function defineDeprecatedProperty(object, className, oldName, newName)
{
    var warningGiven = false;
    function getter()
    {
        if (!warningGiven) {
            console.warn(className + "." + oldName + " is deprecated. Use " + className + "." + newName + " instead");
            warningGiven = true;
        }
        return object[newName];
    }
    object.__defineGetter__(oldName, getter);
}

function extractCallbackArgument(args)
{
    var lastArgument = args[args.length - 1];
    return typeof lastArgument === "function" ? lastArgument : undefined;
}

var AuditCategory = declareInterfaceClass(AuditCategoryImpl);
var AuditResult = declareInterfaceClass(AuditResultImpl);
var Button = declareInterfaceClass(ButtonImpl);
var EventSink = declareInterfaceClass(EventSinkImpl);
var ExtensionPanel = declareInterfaceClass(ExtensionPanelImpl);
var ExtensionSidebarPane = declareInterfaceClass(ExtensionSidebarPaneImpl);
var PanelWithSidebar = declareInterfaceClass(PanelWithSidebarImpl);
var Request = declareInterfaceClass(RequestImpl);
var Resource = declareInterfaceClass(ResourceImpl);
var Timeline = declareInterfaceClass(TimelineImpl);

// extensionServer is a closure variable defined by the glue below -- make sure we fail if it's not there.
if (!extensionServer)
    extensionServer = new ExtensionServerClient();

return new InspectorExtensionAPI();
}

// Default implementation; platforms will override.
function buildPlatformExtensionAPI(extensionInfo)
{
    function platformExtensionAPI(coreAPI)
    {
        window.webInspector = coreAPI;
    }
    return platformExtensionAPI.toString();
}


function buildExtensionAPIInjectedScript(extensionInfo)
{
    return "(function(injectedScriptId){ " +
        "var extensionServer;" +
        defineCommonExtensionSymbols.toString() + ";" +
        injectedExtensionAPI.toString() + ";" +
        buildPlatformExtensionAPI(extensionInfo) + ";" +
        "platformExtensionAPI(injectedExtensionAPI(injectedScriptId));" +
        "return {};" +
        "})";
}
