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
WebInspector.WorkerManager = function()
{
    this._workerIdToWindow = {};
    InspectorBackend.registerWorkerDispatcher(new WebInspector.WorkerDispatcher(this));
}

WebInspector.WorkerManager.isWorkerFrontend = function()
{
    return !!WebInspector.queryParamsObject["dedicatedWorkerId"] ||
           !!WebInspector.queryParamsObject["isSharedWorker"];
}

WebInspector.WorkerManager.isDedicatedWorkerFrontend = function()
{
    return !!WebInspector.queryParamsObject["dedicatedWorkerId"];
}

WebInspector.WorkerManager.loaded = function()
{
    var workerId = WebInspector.queryParamsObject["dedicatedWorkerId"];
    if (workerId)
        WebInspector.WorkerManager._initializeDedicatedWorkerFrontend(workerId);
    else
        WebInspector.workerManager = new WebInspector.WorkerManager();
}

WebInspector.WorkerManager.loadCompleted = function()
{
    // Make sure script execution of dedicated worker is resumed and then paused
    // on the first script statement in case we autoattached to it.
    if (WebInspector.queryParamsObject["workerPaused"]) {
        DebuggerAgent.pause();
        RuntimeAgent.run(calculateTitle);
    } else if (WebInspector.WorkerManager.isWorkerFrontend())
        calculateTitle();

    function calculateTitle()
    {
        WebInspector.WorkerManager._calculateWorkerInspectorTitle();
    }

    if (WebInspector.workerManager)
        WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.MainFrameNavigated, WebInspector.workerManager._mainFrameNavigated, WebInspector.workerManager);
}

WebInspector.WorkerManager._initializeDedicatedWorkerFrontend = function(workerId)
{
    function receiveMessage(event)
    {
        var message = event.data;
        InspectorBackend.dispatch(message);
    }
    window.addEventListener("message", receiveMessage, true);


    InspectorBackend.sendMessageObjectToBackend = function(message)
    {
        window.opener.postMessage({workerId: workerId, command: "sendMessageToBackend", message: message}, "*");
    }

    InspectorFrontendHost.loaded = function()
    {
        window.opener.postMessage({workerId: workerId, command: "loaded"}, "*");
    }
}

WebInspector.WorkerManager._calculateWorkerInspectorTitle = function()
{
    var expression = "location.href";
    if (WebInspector.queryParamsObject["isSharedWorker"])
        expression += " + (this.name ? ' (' + this.name + ')' : '')";
    RuntimeAgent.evaluate.invoke({expression:expression, doNotPauseOnExceptionsAndMuteConsole:true, returnByValue: true}, evalCallback.bind(this));
    
    /**
     * @param {?Protocol.Error} error
     * @param {RuntimeAgent.RemoteObject} result
     * @param {boolean=} wasThrown
     */
    function evalCallback(error, result, wasThrown)
    {
        if (error || wasThrown) {
            console.error(error);
            return;
        }
        InspectorFrontendHost.inspectedURLChanged(result.value);
    }
}

WebInspector.WorkerManager.Events = {
    WorkerAdded: "worker-added",
    WorkerRemoved: "worker-removed",
    WorkersCleared: "workers-cleared",
}

WebInspector.WorkerManager.prototype = {
    _workerCreated: function(workerId, url, inspectorConnected)
     {
        if (inspectorConnected)
            this._openInspectorWindow(workerId, true);
        this.dispatchEventToListeners(WebInspector.WorkerManager.Events.WorkerAdded, {workerId: workerId, url: url, inspectorConnected: inspectorConnected});
     },

    _workerTerminated: function(workerId)
     {
        this.closeWorkerInspector(workerId);
        this.dispatchEventToListeners(WebInspector.WorkerManager.Events.WorkerRemoved, workerId);
     },

    _sendMessageToWorkerInspector: function(workerId, message)
    {
        var workerInspectorWindow = this._workerIdToWindow[workerId];
        if (workerInspectorWindow)
            workerInspectorWindow.postMessage(message, "*");
    },

    openWorkerInspector: function(workerId)
    {
        var existingInspector = this._workerIdToWindow[workerId];
        if (existingInspector) {
            existingInspector.focus();
            return;
        }

        this._openInspectorWindow(workerId, false);
        WorkerAgent.connectToWorker(workerId);
    },

    _openInspectorWindow: function(workerId, workerIsPaused)
    {
        var search = window.location.search;
        var hash = window.location.hash;
        var url = window.location.href;
        // Make sure hash is in rear
        url = url.replace(hash, "");
        url += (search ? "&dedicatedWorkerId=" : "?dedicatedWorkerId=") + workerId;
        if (workerIsPaused)
            url += "&workerPaused=true";
        url = url.replace("docked=true&", "");
        url += hash;
        var width = WebInspector.settings.workerInspectorWidth.get();
        var height = WebInspector.settings.workerInspectorHeight.get();
        // Set location=0 just to make sure the front-end will be opened in a separate window, not in new tab.
        var workerInspectorWindow = window.open(url, undefined, "location=0,width=" + width + ",height=" + height);
        workerInspectorWindow.addEventListener("resize", this._onWorkerInspectorResize.bind(this, workerInspectorWindow), false);
        this._workerIdToWindow[workerId] = workerInspectorWindow;
        workerInspectorWindow.addEventListener("beforeunload", this._workerInspectorClosing.bind(this, workerId), true);

        // Listen to beforeunload in detached state and to the InspectorClosing event in case of attached inspector.
        window.addEventListener("beforeunload", this._pageInspectorClosing.bind(this), true);
        WebInspector.notifications.addEventListener(WebInspector.Events.InspectorClosing, this._pageInspectorClosing, this);
    },

    closeWorkerInspector: function(workerId)
    {
        var workerInspectorWindow = this._workerIdToWindow[workerId];
        if (workerInspectorWindow)
            workerInspectorWindow.close();
    },

    _mainFrameNavigated: function(event)
    {
        for (var workerId in this._workerIdToWindow)
            this.closeWorkerInspector(workerId);
        this.dispatchEventToListeners(WebInspector.WorkerManager.Events.WorkersCleared);
    },

    _pageInspectorClosing: function()
    {
        this._ignoreWorkerInspectorClosing = true;
        for (var workerId in this._workerIdToWindow) {
            this._workerIdToWindow[workerId].close();
            WorkerAgent.disconnectFromWorker(parseInt(workerId, 10));
        }
    },

    _onWorkerInspectorResize: function(workerInspectorWindow)
    {
        var doc = workerInspectorWindow.document;
        WebInspector.settings.workerInspectorWidth.set(doc.width);
        WebInspector.settings.workerInspectorHeight.set(doc.height);
    },

    _workerInspectorClosing: function(workerId, event)
    {
        if (event.target.location.href === "about:blank")
            return;
        if (this._ignoreWorkerInspectorClosing)
            return;
        delete this._workerIdToWindow[workerId];
        WorkerAgent.disconnectFromWorker(workerId);
    },

    _disconnectedFromWorker: function()
    {
        var screen = new WebInspector.WorkerTerminatedScreen();
        WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.GlobalObjectCleared, screen.hide, screen);
        screen.showModal();
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 * @implements {WorkerAgent.Dispatcher}
 */
WebInspector.WorkerDispatcher = function(workerManager)
{
    this._workerManager = workerManager;
    window.addEventListener("message", this._receiveMessage.bind(this), true);
}

WebInspector.WorkerDispatcher.prototype = {
    _receiveMessage: function(event)
    {
        var workerId = event.data["workerId"];
        workerId = parseInt(workerId, 10);
        var command = event.data.command;
        var message = event.data.message;

        if (command == "sendMessageToBackend")
            WorkerAgent.sendMessageToWorker(workerId, message);
    },

    workerCreated: function(workerId, url, inspectorConnected)
    {
        this._workerManager._workerCreated(workerId, url, inspectorConnected);
    },

    workerTerminated: function(workerId)
    {
        this._workerManager._workerTerminated(workerId);
    },

    dispatchMessageFromWorker: function(workerId, message)
    {
        this._workerManager._sendMessageToWorkerInspector(workerId, message);
    },

    disconnectedFromWorker: function()
    {
        this._workerManager._disconnectedFromWorker();
    }
}

/**
 * @constructor
 * @extends {WebInspector.HelpScreen}
 */
WebInspector.WorkerTerminatedScreen = function()
{
    WebInspector.HelpScreen.call(this, WebInspector.UIString("Inspected worker terminated"));
    var p = this.contentElement.createChild("p");
    p.addStyleClass("help-section");
    p.textContent = WebInspector.UIString("Inspected worker has terminated. Once it restarts we will attach to it automatically.");
}

WebInspector.WorkerTerminatedScreen.prototype = {
    /**
     * @override
     */
    willHide: function()
    {
        WebInspector.debuggerModel.removeEventListener(WebInspector.DebuggerModel.Events.GlobalObjectCleared, this.hide, this);
        WebInspector.HelpScreen.prototype.willHide.call(this);
    },

    __proto__: WebInspector.HelpScreen.prototype
}
