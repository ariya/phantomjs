/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyrightdd
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
WebInspector.HeapSnapshotWorkerWrapper = function()
{
}

WebInspector.HeapSnapshotWorkerWrapper.prototype =  {
    postMessage: function(message)
    {
    },
    terminate: function()
    {
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotWorkerWrapper}
 */
WebInspector.HeapSnapshotRealWorker = function()
{
    this._worker = new Worker("HeapSnapshotWorker.js");
    this._worker.addEventListener("message", this._messageReceived.bind(this), false);
}

WebInspector.HeapSnapshotRealWorker.prototype = {
    _messageReceived: function(event)
    {
        var message = event.data;
        if ("callId" in message)
            this.dispatchEventToListeners("message", message);
        else {
            if (message.object !== "console") {
                console.log(WebInspector.UIString("Worker asks to call a method '%s' on an unsupported object '%s'.", message.method, message.object));
                return;
            }
            if (message.method !== "log" && message.method !== "info" && message.method !== "error") {
                console.log(WebInspector.UIString("Worker asks to call an unsupported method '%s' on the console object.", message.method));
                return;
            }
            console[message.method].apply(window[message.object], message.arguments);
        }
    },

    postMessage: function(message)
    {
        this._worker.postMessage(message);
    },

    terminate: function()
    {
        this._worker.terminate();
    },

    __proto__: WebInspector.HeapSnapshotWorkerWrapper.prototype
}


/**
 * @constructor
 */
WebInspector.AsyncTaskQueue = function()
{
    this._queue = [];
    this._isTimerSheduled = false;
}

WebInspector.AsyncTaskQueue.prototype = {
    /**
     * @param {function()} task
     */
    addTask: function(task)
    {
        this._queue.push(task);
        this._scheduleTimer();
    },

    _onTimeout: function()
    {
        this._isTimerSheduled = false;
        var queue = this._queue;
        this._queue = [];
        for (var i = 0; i < queue.length; i++) {
            try {
                queue[i]();
            } catch (e) {
                console.error("Exception while running task: " + e.stack);
            }
        }
        this._scheduleTimer();
    },

    _scheduleTimer: function()
    {
        if (this._queue.length && !this._isTimerSheduled) {
            setTimeout(this._onTimeout.bind(this), 0);
            this._isTimerSheduled = true;
        }
    }
}

/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotWorkerWrapper}
 */
WebInspector.HeapSnapshotFakeWorker = function()
{
    this._dispatcher = new WebInspector.HeapSnapshotWorkerDispatcher(window, this._postMessageFromWorker.bind(this));
    this._asyncTaskQueue = new WebInspector.AsyncTaskQueue();
}

WebInspector.HeapSnapshotFakeWorker.prototype = {
    postMessage: function(message)
    {
        function dispatch()
        {
            if (this._dispatcher)
                this._dispatcher.dispatchMessage({data: message});
        }
        this._asyncTaskQueue.addTask(dispatch.bind(this));
    },

    terminate: function()
    {
        this._dispatcher = null;
    },

    _postMessageFromWorker: function(message)
    {
        function send()
        {
            this.dispatchEventToListeners("message", message);
        }
        this._asyncTaskQueue.addTask(send.bind(this));
    },

    __proto__: WebInspector.HeapSnapshotWorkerWrapper.prototype
}


/**
 * @constructor
 * @extends {WebInspector.Object}
 */
WebInspector.HeapSnapshotWorker = function()
{
    this._nextObjectId = 1;
    this._nextCallId = 1;
    this._callbacks = [];
    this._previousCallbacks = [];
    // There is no support for workers in Chromium DRT.
    this._worker = typeof InspectorTest === "undefined" ? new WebInspector.HeapSnapshotRealWorker() : new WebInspector.HeapSnapshotFakeWorker();
    this._worker.addEventListener("message", this._messageReceived, this);
}

WebInspector.HeapSnapshotWorker.prototype = {
    createLoader: function(snapshotConstructorName, proxyConstructor)
    {
        var objectId = this._nextObjectId++;
        var proxy = new WebInspector.HeapSnapshotLoaderProxy(this, objectId, snapshotConstructorName, proxyConstructor);
        this._postMessage({callId: this._nextCallId++, disposition: "create", objectId: objectId, methodName: "WebInspector.HeapSnapshotLoader"});
        return proxy;
    },

    dispose: function()
    {
        this._worker.terminate();
        if (this._interval)
            clearInterval(this._interval);
    },

    disposeObject: function(objectId)
    {
        this._postMessage({callId: this._nextCallId++, disposition: "dispose", objectId: objectId});
    },

    callGetter: function(callback, objectId, getterName)
    {
        var callId = this._nextCallId++;
        this._callbacks[callId] = callback;
        this._postMessage({callId: callId, disposition: "getter", objectId: objectId, methodName: getterName});
    },

    callFactoryMethod: function(callback, objectId, methodName, proxyConstructor)
    {
        var callId = this._nextCallId++;
        var methodArguments = Array.prototype.slice.call(arguments, 4);
        var newObjectId = this._nextObjectId++;
        if (callback) {
            function wrapCallback(remoteResult)
            {
                callback(remoteResult ? new proxyConstructor(this, newObjectId) : null);
            }
            this._callbacks[callId] = wrapCallback.bind(this);
            this._postMessage({callId: callId, disposition: "factory", objectId: objectId, methodName: methodName, methodArguments: methodArguments, newObjectId: newObjectId});
            return null;
        } else {
            this._postMessage({callId: callId, disposition: "factory", objectId: objectId, methodName: methodName, methodArguments: methodArguments, newObjectId: newObjectId});
            return new proxyConstructor(this, newObjectId);
        }
    },

    callMethod: function(callback, objectId, methodName)
    {
        var callId = this._nextCallId++;
        var methodArguments = Array.prototype.slice.call(arguments, 3);
        if (callback)
            this._callbacks[callId] = callback;
        this._postMessage({callId: callId, disposition: "method", objectId: objectId, methodName: methodName, methodArguments: methodArguments});
    },

    startCheckingForLongRunningCalls: function()
    {
        if (this._interval)
            return;
        this._checkLongRunningCalls();
        this._interval = setInterval(this._checkLongRunningCalls.bind(this), 300);
    },

    _checkLongRunningCalls: function()
    {
        for (var callId in this._previousCallbacks)
            if (!(callId in this._callbacks))
                delete this._previousCallbacks[callId];
        var hasLongRunningCalls = false;
        for (callId in this._previousCallbacks) {
            hasLongRunningCalls = true;
            break;
        }
        this.dispatchEventToListeners("wait", hasLongRunningCalls);
        for (callId in this._callbacks)
            this._previousCallbacks[callId] = true;
    },

    _findFunction: function(name)
    {
        var path = name.split(".");
        var result = window;
        for (var i = 0; i < path.length; ++i)
            result = result[path[i]];
        return result;
    },

    _messageReceived: function(event)
    {
        var data = event.data;
        if (event.data.error) {
            if (event.data.errorMethodName)
                WebInspector.log(WebInspector.UIString("An error happened when a call for method '%s' was requested", event.data.errorMethodName));
            WebInspector.log(event.data.errorCallStack);
            delete this._callbacks[data.callId];
            return;
        }
        if (!this._callbacks[data.callId])
            return;
        var callback = this._callbacks[data.callId];
        delete this._callbacks[data.callId];
        callback(data.result);
    },

    _postMessage: function(message)
    {
        this._worker.postMessage(message);
    },

    __proto__: WebInspector.Object.prototype
}


/**
 * @constructor
 */
WebInspector.HeapSnapshotProxyObject = function(worker, objectId)
{
    this._worker = worker;
    this._objectId = objectId;
}

WebInspector.HeapSnapshotProxyObject.prototype = {
    _callWorker: function(workerMethodName, args)
    {
        args.splice(1, 0, this._objectId);
        return this._worker[workerMethodName].apply(this._worker, args);
    },

    dispose: function()
    {
        this._worker.disposeObject(this._objectId);
    },

    disposeWorker: function()
    {
        this._worker.dispose();
    },

    /**
     * @param {...*} var_args
     */
    callFactoryMethod: function(callback, methodName, proxyConstructor, var_args)
    {
        return this._callWorker("callFactoryMethod", Array.prototype.slice.call(arguments, 0));
    },

    callGetter: function(callback, getterName)
    {
        return this._callWorker("callGetter", Array.prototype.slice.call(arguments, 0));
    },

    /**
     * @param {...*} var_args
     */
    callMethod: function(callback, methodName, var_args)
    {
        return this._callWorker("callMethod", Array.prototype.slice.call(arguments, 0));
    },

    get worker() {
        return this._worker;
    }
};

/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotProxyObject}
 * @implements {WebInspector.OutputStream}
 */
WebInspector.HeapSnapshotLoaderProxy = function(worker, objectId, snapshotConstructorName, proxyConstructor)
{
    WebInspector.HeapSnapshotProxyObject.call(this, worker, objectId);
    this._snapshotConstructorName = snapshotConstructorName;
    this._proxyConstructor = proxyConstructor;
    this._pendingSnapshotConsumers = [];
}

WebInspector.HeapSnapshotLoaderProxy.prototype = {
    /**
     * @param {function(WebInspector.HeapSnapshotProxy)} callback
     */
    addConsumer: function(callback)
    {
        this._pendingSnapshotConsumers.push(callback);
    },

    /**
     * @param {string} chunk
     * @param {function(WebInspector.OutputStream)=} callback
     */
    write: function(chunk, callback)
    {
        this.callMethod(callback, "write", chunk);
    },

    close: function()
    {
        function buildSnapshot()
        {
            this.callFactoryMethod(updateStaticData.bind(this), "buildSnapshot", this._proxyConstructor, this._snapshotConstructorName);
        }
        function updateStaticData(snapshotProxy)
        {
            this.dispose();
            snapshotProxy.updateStaticData(notifyPendingConsumers.bind(this));
        }
        function notifyPendingConsumers(snapshotProxy)
        {
            for (var i = 0; i < this._pendingSnapshotConsumers.length; ++i)
                this._pendingSnapshotConsumers[i](snapshotProxy);
            this._pendingSnapshotConsumers = [];
        }
        this.callMethod(buildSnapshot.bind(this), "close");
    },

    __proto__: WebInspector.HeapSnapshotProxyObject.prototype
}


/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotProxyObject}
 */
WebInspector.HeapSnapshotProxy = function(worker, objectId)
{
    WebInspector.HeapSnapshotProxyObject.call(this, worker, objectId);
}

WebInspector.HeapSnapshotProxy.prototype = {
    aggregates: function(sortedIndexes, key, filter, callback)
    {
        this.callMethod(callback, "aggregates", sortedIndexes, key, filter);
    },

    aggregatesForDiff: function(callback)
    {
        this.callMethod(callback, "aggregatesForDiff");
    },

    calculateSnapshotDiff: function(baseSnapshotId, baseSnapshotAggregates, callback)
    {
        this.callMethod(callback, "calculateSnapshotDiff", baseSnapshotId, baseSnapshotAggregates);
    },

    nodeClassName: function(snapshotObjectId, callback)
    {
        this.callMethod(callback, "nodeClassName", snapshotObjectId);
    },

    dominatorIdsForNode: function(nodeIndex, callback)
    {
        this.callMethod(callback, "dominatorIdsForNode", nodeIndex);
    },

    createEdgesProvider: function(nodeIndex, showHiddenData)
    {
        return this.callFactoryMethod(null, "createEdgesProvider", WebInspector.HeapSnapshotProviderProxy, nodeIndex, showHiddenData);
    },

    createRetainingEdgesProvider: function(nodeIndex, showHiddenData)
    {
        return this.callFactoryMethod(null, "createRetainingEdgesProvider", WebInspector.HeapSnapshotProviderProxy, nodeIndex, showHiddenData);
    },

    createAddedNodesProvider: function(baseSnapshotId, className)
    {
        return this.callFactoryMethod(null, "createAddedNodesProvider", WebInspector.HeapSnapshotProviderProxy, baseSnapshotId, className);
    },

    createDeletedNodesProvider: function(nodeIndexes)
    {
        return this.callFactoryMethod(null, "createDeletedNodesProvider", WebInspector.HeapSnapshotProviderProxy, nodeIndexes);
    },

    createNodesProvider: function(filter)
    {
        return this.callFactoryMethod(null, "createNodesProvider", WebInspector.HeapSnapshotProviderProxy, filter);
    },

    createNodesProviderForClass: function(className, aggregatesKey)
    {
        return this.callFactoryMethod(null, "createNodesProviderForClass", WebInspector.HeapSnapshotProviderProxy, className, aggregatesKey);
    },

    createNodesProviderForDominator: function(nodeIndex)
    {
        return this.callFactoryMethod(null, "createNodesProviderForDominator", WebInspector.HeapSnapshotProviderProxy, nodeIndex);
    },

    dispose: function()
    {
        this.disposeWorker();
    },

    get nodeCount()
    {
        return this._staticData.nodeCount;
    },

    get rootNodeIndex()
    {
        return this._staticData.rootNodeIndex;
    },

    updateStaticData: function(callback)
    {
        function dataReceived(staticData)
        {
            this._staticData = staticData;
            callback(this);
        }
        this.callMethod(dataReceived.bind(this), "updateStaticData");
    },

    get totalSize()
    {
        return this._staticData.totalSize;
    },

    get uid()
    {
        return this._staticData.uid;
    },

    __proto__: WebInspector.HeapSnapshotProxyObject.prototype
}


/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotProxy}
 */
WebInspector.NativeHeapSnapshotProxy = function(worker, objectId)
{
    WebInspector.HeapSnapshotProxy.call(this, worker, objectId);
}

WebInspector.NativeHeapSnapshotProxy.prototype = {
    images: function(callback)
    {
        this.callMethod(callback, "images");
    },

    __proto__: WebInspector.HeapSnapshotProxy.prototype
}

/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotProxyObject}
 */
WebInspector.HeapSnapshotProviderProxy = function(worker, objectId)
{
    WebInspector.HeapSnapshotProxyObject.call(this, worker, objectId);
}

WebInspector.HeapSnapshotProviderProxy.prototype = {
    nodePosition: function(snapshotObjectId, callback)
    {
        this.callMethod(callback, "nodePosition", snapshotObjectId);
    },

    isEmpty: function(callback)
    {
        this.callMethod(callback, "isEmpty");
    },

    serializeItemsRange: function(startPosition, endPosition, callback)
    {
        this.callMethod(callback, "serializeItemsRange", startPosition, endPosition);
    },

    sortAndRewind: function(comparator, callback)
    {
        this.callMethod(callback, "sortAndRewind", comparator);
    },

    __proto__: WebInspector.HeapSnapshotProxyObject.prototype
}

