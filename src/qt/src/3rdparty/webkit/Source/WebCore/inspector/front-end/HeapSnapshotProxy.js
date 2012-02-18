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

WebInspector.HeapSnapshotRealWorker = function()
{
    this._worker = new Worker("HeapSnapshotWorker.js");
    this._worker.addEventListener("message", this._messageReceived.bind(this), false);
}

WebInspector.HeapSnapshotRealWorker.prototype = {
    _messageReceived: function(event)
    {
        this.dispatchEventToListeners("message", event.data);
    },

    postMessage: function(message)
    {
        this._worker.postMessage(message);
    },

    terminate: function()
    {
        this._worker.terminate();
    }
};

WebInspector.HeapSnapshotRealWorker.prototype.__proto__ = WebInspector.Object.prototype;

WebInspector.HeapSnapshotFakeWorker = function()
{
    this._dispatcher = new WebInspector.HeapSnapshotWorkerDispatcher(window, this._postMessageFromWorker.bind(this));
}

WebInspector.HeapSnapshotFakeWorker.prototype = {
    postMessage: function(message)
    {
        function dispatch()
        {
            if (this._dispatcher)
                this._dispatcher.dispatchMessage({data: message});
        }
        setTimeout(dispatch.bind(this), 0);
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
        setTimeout(send.bind(this), 0);
    }
};

WebInspector.HeapSnapshotFakeWorker.prototype.__proto__ = WebInspector.Object.prototype;

WebInspector.HeapSnapshotWorker = function()
{
    this._nextObjectId = 1;
    this._nextCallId = 1;
    this._callbacks = [];
    // There is no support for workers in Chromium DRT.
    this._worker = typeof InspectorTest === "undefined" ? new WebInspector.HeapSnapshotRealWorker() : new WebInspector.HeapSnapshotFakeWorker();
    this._worker.addEventListener("message", this._messageReceived.bind(this), false);
}

WebInspector.HeapSnapshotWorker.prototype = {
    createObject: function(constructorName)
    {
        var proxyConstructorFunction = this._findFunction(constructorName + "Proxy");
        var objectId = this._nextObjectId++;
        var proxy = new proxyConstructorFunction(this, objectId);
        this._postMessage({callId: this._nextCallId++, disposition: "create", objectId: objectId, methodName: constructorName});
        return proxy;
    },

    dispose: function()
    {
        this._worker.terminate();
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

    callFactoryMethod: function(callback, objectId, methodName, proxyConstructorName)
    {
        var callId = this._nextCallId++;
        var methodArguments = Array.prototype.slice.call(arguments, 4);
        var newObjectId = this._nextObjectId++;
        var proxyConstructorFunction = this._findFunction(proxyConstructorName);
        if (callback) {
            function wrapCallback(remoteResult)
            {
                callback(remoteResult ? new proxyConstructorFunction(this, newObjectId) : null);
            }
            this._callbacks[callId] = wrapCallback.bind(this);
            this._postMessage({callId: callId, disposition: "factory", objectId: objectId, methodName: methodName, methodArguments: methodArguments, newObjectId: newObjectId});
            return null;
        } else {
            this._postMessage({callId: callId, disposition: "factory", objectId: objectId, methodName: methodName, methodArguments: methodArguments, newObjectId: newObjectId});
            return new proxyConstructorFunction(this, newObjectId);
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
        if (!this._callbacks[data.callId])
            return;
        var callback = this._callbacks[data.callId];
        delete this._callbacks[data.callId];
        callback(data.result);
    },

    _postMessage: function(message)
    {
        this._worker.postMessage(message);      
    }
};

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

    callFactoryMethod: function(callback, methodName, proxyConstructorName)
    {
        return this._callWorker("callFactoryMethod", Array.prototype.slice.call(arguments, 0));
    },

    callGetter: function(callback, getterName)
    {
        return this._callWorker("callGetter", Array.prototype.slice.call(arguments, 0));
    },

    callMethod: function(callback, methodName)
    {
        return this._callWorker("callMethod", Array.prototype.slice.call(arguments, 0));
    }
};

WebInspector.HeapSnapshotLoaderProxy = function(worker, objectId)
{
    WebInspector.HeapSnapshotProxyObject.call(this, worker, objectId);
    this._loading = false;
    this._loaded = false;
}

WebInspector.HeapSnapshotLoaderProxy.prototype = {
    finishLoading: function(callback)
    {
        if (!this._loading)
            return false;
        var loadCallbacks = this._onLoadCallbacks;
        loadCallbacks.splice(0, 0, callback);
        delete this._onLoadCallbacks;
        this._loading = false;
        this._loaded = true;
        function callLoadCallbacks(snapshotProxy)
        {
            for (var i = 0; i < loadCallbacks.length; ++i)
                loadCallbacks[i](snapshotProxy);
        }
        function updateStaticData(snapshotProxy)
        {
            this.dispose();
            snapshotProxy.updateStaticData(callLoadCallbacks);
        }
        this.callFactoryMethod(updateStaticData.bind(this), "finishLoading", "WebInspector.HeapSnapshotProxy");
        return true;
    },

    get loaded()
    {
        return this._loaded;
    },

    startLoading: function(callback)
    {
        if (!this._loading) {
            this._onLoadCallbacks = [callback];
            this._loading = true;
            return true;
        } else {
            this._onLoadCallbacks.push(callback);
            return false;
        }
    },

    pushJSONChunk: function(chunk)
    {
        if (!this._loading)
            return;
        this.callMethod(null, "pushJSONChunk", chunk);
    }
};

WebInspector.HeapSnapshotLoaderProxy.prototype.__proto__ = WebInspector.HeapSnapshotProxyObject.prototype;

WebInspector.HeapSnapshotProxy = function(worker, objectId)
{
    WebInspector.HeapSnapshotProxyObject.call(this, worker, objectId);
}

WebInspector.HeapSnapshotProxy.prototype = {
    aggregates: function(sortedIndexes, callback)
    {
        this.callMethod(callback, "aggregates", sortedIndexes);
    },

    createDiff: function(className)
    {
        return this.callFactoryMethod(null, "createDiff",  "WebInspector.HeapSnapshotsDiffProxy", className);
    },

    createEdgesProvider: function(nodeIndex, filter)
    {
        return this.callFactoryMethod(null, "createEdgesProvider", "WebInspector.HeapSnapshotProviderProxy", nodeIndex, filter);
    },

    createNodesProvider: function(filter)
    {
        return this.callFactoryMethod(null, "createNodesProvider", "WebInspector.HeapSnapshotProviderProxy", filter);
    },

    createNodesProviderForClass: function(className)
    {
        return this.callFactoryMethod(null, "createNodesProviderForClass", "WebInspector.HeapSnapshotProviderProxy", className);
    },

    createPathFinder: function(targetNodeIndex)
    {
        return this.callFactoryMethod(null, "createPathFinder", "WebInspector.HeapSnapshotPathFinderProxy", targetNodeIndex);
    },

    dispose: function()
    {
        this.disposeWorker();
    },

    finishLoading: function()
    {
        return false;
    },

    get loaded()
    {
        return !!this._objectId;
    },

    get nodeCount()
    {
        return this._staticData.nodeCount;
    },

    nodeFieldValuesByIndex: function(fieldName, indexes, callback)
    {
        this.callMethod(callback, "nodeFieldValuesByIndex", fieldName, indexes);
    },

    pushBaseIds: function(snapshotId, className, nodeIds)
    {
        this.callMethod(null, "pushBaseIds", snapshotId, className, nodeIds);
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

    startLoading: function(callback)
    {
        setTimeout(callback, 0);
        return false;
    },

    get totalSize()
    {
        return this._staticData.totalSize;
    },

    get uid()
    {
        return this._staticData.uid;
    }
};

WebInspector.HeapSnapshotProxy.prototype.__proto__ = WebInspector.HeapSnapshotProxyObject.prototype;

WebInspector.HeapSnapshotProviderProxy = function(worker, objectId)
{
    WebInspector.HeapSnapshotProxyObject.call(this, worker, objectId);
}

WebInspector.HeapSnapshotProviderProxy.prototype = {
    isEmpty: function(callback)
    {
        this.callGetter(callback, "isEmpty");
    },

    serializeNextItems: function(count, callback)
    {
        this.callMethod(callback, "serializeNextItems", count);
    },

    sortAndRewind: function(comparator, callback)
    {
        this.callMethod(callback, "sortAndRewind", comparator);
    }
};

WebInspector.HeapSnapshotProviderProxy.prototype.__proto__ = WebInspector.HeapSnapshotProxyObject.prototype;

WebInspector.HeapSnapshotPathFinderProxy = function(worker, objectId)
{
    WebInspector.HeapSnapshotProxyObject.call(this, worker, objectId);
}

WebInspector.HeapSnapshotPathFinderProxy.prototype = {
    findNext: function(callback)
    {
        this.callMethod(callback, "findNext");
    },

    updateRoots: function(filter)
    {
        this.callMethod(null, "updateRoots", filter);
    }
};

WebInspector.HeapSnapshotPathFinderProxy.prototype.__proto__ = WebInspector.HeapSnapshotProxyObject.prototype;

WebInspector.HeapSnapshotsDiffProxy = function(worker, objectId)
{
    WebInspector.HeapSnapshotProxyObject.call(this, worker, objectId);
}

WebInspector.HeapSnapshotsDiffProxy.prototype = {
    calculate: function(callback)
    {
        this.callMethod(callback, "calculate");
    },

    pushBaseIds: function(baseIds)
    {
        this.callMethod(null, "pushBaseIds", baseIds);
    },

    pushBaseSelfSizes: function(baseSelfSizes)
    {
        this.callMethod(null, "pushBaseSelfSizes", baseSelfSizes);
    }
};

WebInspector.HeapSnapshotsDiffProxy.prototype.__proto__ = WebInspector.HeapSnapshotProxyObject.prototype;
