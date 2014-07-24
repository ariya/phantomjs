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

WebInspector.DOMTree = function(frame)
{
    WebInspector.Object.call(this);

    this._frame = frame;

    this._rootDOMNode = null;
    this._requestIdentifier = 0;

    this._frame.addEventListener(WebInspector.Frame.Event.PageExecutionContextChanged, this._framePageExecutionContextChanged, this);

    WebInspector.domTreeManager.addEventListener(WebInspector.DOMTreeManager.Event.DocumentUpdated, this._documentUpdated, this);

    // Only add extra event listeners when not the main frame. Since DocumentUpdated is enough for the main frame.
    if (!this._frame.isMainFrame()) {
        WebInspector.domTreeManager.addEventListener(WebInspector.DOMTreeManager.Event.NodeRemoved, this._nodeRemoved, this);
        this._frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._frameMainResourceDidChange, this);
    }
};

WebInspector.Object.addConstructorFunctions(WebInspector.DOMTree);

WebInspector.DOMTree.Event = {
    RootDOMNodeInvalidated: "dom-tree-root-dom-node-invalidated"
};

WebInspector.DOMTree.prototype = {
    constructor: WebInspector.DOMTree,

    // Public

    get frame()
    {
        return this._frame;
    },

    invalidate: function()
    {
        // Set to null so it is fetched again next time requestRootDOMNode is called.
        this._rootDOMNode = null;

        // Clear the pending callbacks. It is the responsibility of the client to listen for
        // the RootDOMNodeInvalidated event and request the root DOM node again.
        delete this._pendingRootDOMNodeRequests;

        if (this._invalidateTimeoutIdentifier)
            return;

        function performInvalidate()
        {
            delete this._invalidateTimeoutIdentifier;

            this.dispatchEventToListeners(WebInspector.DOMTree.Event.RootDOMNodeInvalidated);
        }

        // Delay the invalidation on a timeout to coalesce multiple calls to invalidate.
        this._invalidateTimeoutIdentifier = setTimeout(performInvalidate.bind(this), 0);
    },

    requestRootDOMNode: function(callback)
    {
        console.assert(typeof callback === "function");
        if (typeof callback !== "function")
            return;

        if (this._rootDOMNode) {
            callback(this._rootDOMNode);
            return;
        }

        if (!this._frame.isMainFrame() && WebInspector.ExecutionContext.supported() && !this._frame.pageExecutionContext) {
            this._rootDOMNodeRequestWaitingForExecutionContext = true;
            if (!this._pendingRootDOMNodeRequests)
                this._pendingRootDOMNodeRequests = [];
            this._pendingRootDOMNodeRequests.push(callback);
            return;
        }

        if (this._pendingRootDOMNodeRequests) {
            this._pendingRootDOMNodeRequests.push(callback);
            return;
        }

        this._pendingRootDOMNodeRequests = [callback];
        this._requestRootDOMNode();
    },

    // Private

    _requestRootDOMNode: function()
    {
        console.assert(this._frame.isMainFrame() || !WebInspector.ExecutionContext.supported() || this._frame.pageExecutionContext);
        console.assert(this._pendingRootDOMNodeRequests.length);

        // Bump the request identifier. This prevents pending callbacks for previous requests from completing.
        var requestIdentifier = ++this._requestIdentifier;

        function rootObjectAvailable(error, result)
        {
            // Check to see if we have been invalidated (if the callbacks were cleared).
            if (!this._pendingRootDOMNodeRequests || requestIdentifier != this._requestIdentifier)
                return;

            if (error) {
                console.error(JSON.stringify(error));

                this._rootDOMNode = null;
                dispatchCallbacks.call(this);
                return;
            }

            // Convert the RemoteObject to a DOMNode by asking the backend to push it to us.
            var remoteObject = WebInspector.RemoteObject.fromPayload(result);
            remoteObject.pushNodeToFrontend(rootDOMNodeAvailable.bind(this, remoteObject));
        }

        function rootDOMNodeAvailable(remoteObject, nodeId)
        {
            remoteObject.release();

            // Check to see if we have been invalidated (if the callbacks were cleared).
            if (!this._pendingRootDOMNodeRequests || requestIdentifier != this._requestIdentifier)
                return;

            if (!nodeId) {
                this._rootDOMNode = null;
                dispatchCallbacks.call(this);
                return;
            }

            this._rootDOMNode = WebInspector.domTreeManager.nodeForId(nodeId);

            console.assert(this._rootDOMNode);
            if (!this._rootDOMNode) {
                dispatchCallbacks.call(this);
                return;
            }

            // Request the child nodes since the root node is often not shown in the UI,
            // and the child nodes will be needed immediately.
            this._rootDOMNode.getChildNodes(dispatchCallbacks.bind(this));
        }

        function mainDocumentAvailable(document)
        {
            this._rootDOMNode = document;

            dispatchCallbacks.call(this);
        }

        function dispatchCallbacks()
        {
            // Check to see if we have been invalidated (if the callbacks were cleared).
            if (!this._pendingRootDOMNodeRequests || requestIdentifier != this._requestIdentifier)
                return;

            for (var i = 0; i < this._pendingRootDOMNodeRequests.length; ++i)
                this._pendingRootDOMNodeRequests[i](this._rootDOMNode);
            delete this._pendingRootDOMNodeRequests;
        }

        // For the main frame we can use the more straight forward requestDocument function. For
        // child frames we need to do a more roundabout approach since the protocol does not include
        // a specific way to request a document given a frame identifier. The child frame approach
        // involves evaluating the JavaScript "document" and resolving that into a DOMNode.
        if (this._frame.isMainFrame())
            WebInspector.domTreeManager.requestDocument(mainDocumentAvailable.bind(this));
        else {
            // COMPATIBILITY (iOS 6): Execution context identifiers (contextId) did not exist
            // in iOS 6. Fallback to including the frame identifier (frameId).
            var contextId = this._frame.pageExecutionContext ? this._frame.pageExecutionContext.id : undefined;
            RuntimeAgent.evaluate.invoke({expression: "document", objectGroup: "", includeCommandLineAPI: false, doNotPauseOnExceptionsAndMuteConsole: true, contextId: contextId, frameId: this._frame.id, returnByValue: false, generatePreview: false}, rootObjectAvailable.bind(this));
        }
    },

    _nodeRemoved: function(event)
    {
        console.assert(!this._frame.isMainFrame());

        if (event.data.node !== this._rootDOMNode)
            return;

        this.invalidate();
    },

    _documentUpdated: function(event)
    {
        this.invalidate();
    },

    _frameMainResourceDidChange: function(event)
    {
        console.assert(!this._frame.isMainFrame());

        this.invalidate();
    },

    _framePageExecutionContextChanged: function(event)
    {
        if (this._rootDOMNodeRequestWaitingForExecutionContext) {
            console.assert(this._frame.pageExecutionContext);
            console.assert(this._pendingRootDOMNodeRequests && this._pendingRootDOMNodeRequests.length);

            delete this._rootDOMNodeRequestWaitingForExecutionContext;

            this._requestRootDOMNode();
        }
    }
};

WebInspector.DOMTree.prototype.__proto__ = WebInspector.Object.prototype;
