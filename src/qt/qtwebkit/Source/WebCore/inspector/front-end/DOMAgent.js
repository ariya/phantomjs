/*
 * Copyright (C) 2009, 2010 Google Inc. All rights reserved.
 * Copyright (C) 2009 Joseph Pecoraro
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
 * @param {WebInspector.DOMAgent} domAgent
 * @param {?WebInspector.DOMDocument} doc
 * @param {boolean} isInShadowTree
 * @param {DOMAgent.Node} payload
 */
WebInspector.DOMNode = function(domAgent, doc, isInShadowTree, payload) {
    this._domAgent = domAgent;
    this.ownerDocument = doc;
    this._isInShadowTree = isInShadowTree;

    this.id = payload.nodeId;
    domAgent._idToDOMNode[this.id] = this;
    this._nodeType = payload.nodeType;
    this._nodeName = payload.nodeName;
    this._localName = payload.localName;
    this._nodeValue = payload.nodeValue;

    this._shadowRoots = [];

    this._attributes = [];
    this._attributesMap = {};
    if (payload.attributes)
        this._setAttributesPayload(payload.attributes);

    this._userProperties = {};
    this._descendantUserPropertyCounters = {};

    this._childNodeCount = payload.childNodeCount;
    this.children = null;

    this.nextSibling = null;
    this.previousSibling = null;
    this.firstChild = null;
    this.lastChild = null;
    this.parentNode = null;

    if (payload.shadowRoots && WebInspector.settings.showShadowDOM.get()) {
        for (var i = 0; i < payload.shadowRoots.length; ++i) {
            var root = payload.shadowRoots[i];
            var node = new WebInspector.DOMNode(this._domAgent, this.ownerDocument, true, root);
            this._shadowRoots.push(node);
        }
    }

    if (payload.templateContent)
        this._templateContent = new WebInspector.DOMNode(this._domAgent, this.ownerDocument, true, payload.templateContent);

    if (payload.children)
        this._setChildrenPayload(payload.children);

    if (payload.contentDocument) {
        this._contentDocument = new WebInspector.DOMDocument(domAgent, payload.contentDocument);
        this.children = [this._contentDocument];
        this._renumber();
    }

    if (this._nodeType === Node.ELEMENT_NODE) {
        // HTML and BODY from internal iframes should not overwrite top-level ones.
        if (this.ownerDocument && !this.ownerDocument.documentElement && this._nodeName === "HTML")
            this.ownerDocument.documentElement = this;
        if (this.ownerDocument && !this.ownerDocument.body && this._nodeName === "BODY")
            this.ownerDocument.body = this;
    } else if (this._nodeType === Node.DOCUMENT_TYPE_NODE) {
        this.publicId = payload.publicId;
        this.systemId = payload.systemId;
        this.internalSubset = payload.internalSubset;
    } else if (this._nodeType === Node.ATTRIBUTE_NODE) {
        this.name = payload.name;
        this.value = payload.value;
    }
}

/**
 * @constructor
 * @param {string} value
 * @param {boolean} optimized
 */
WebInspector.DOMNode.XPathStep = function(value, optimized)
{
    this.value = value;
    this.optimized = optimized;
}

WebInspector.DOMNode.XPathStep.prototype = {
    toString: function()
    {
        return this.value;
    }
}

WebInspector.DOMNode.prototype = {
    /**
     * @return {boolean}
     */
    hasAttributes: function()
    {
        return this._attributes.length > 0;
    },

    /**
     * @return {boolean}
     */
    hasChildNodes: function()
    {
        return this._childNodeCount > 0 || !!this._shadowRoots.length || !!this._templateContent;
    },

    /**
     * @return {boolean}
     */
    hasShadowRoots: function()
    {
        return !!this._shadowRoots.length;
    },

    /**
     * @return {number}
     */
    nodeType: function()
    {
        return this._nodeType;
    },

    /**
     * @return {string}
     */
    nodeName: function()
    {
        return this._nodeName;
    },

    /**
     * @return {boolean}
     */
    isInShadowTree: function()
    {
        return this._isInShadowTree;
    },

    /**
     * @return {string}
     */
    nodeNameInCorrectCase: function()
    {
        return this.isXMLNode() ? this.nodeName() : this.nodeName().toLowerCase();
    },

    /**
     * @param {string} name
     * @param {function(?Protocol.Error)=} callback
     */
    setNodeName: function(name, callback)
    {
        DOMAgent.setNodeName(this.id, name, WebInspector.domAgent._markRevision(this, callback));
    },

    /**
     * @return {string}
     */
    localName: function()
    {
        return this._localName;
    },

    /**
     * @return {string}
     */
    nodeValue: function()
    {
        return this._nodeValue;
    },

    /**
     * @param {string} value
     * @param {function(?Protocol.Error)=} callback
     */
    setNodeValue: function(value, callback)
    {
        DOMAgent.setNodeValue(this.id, value, WebInspector.domAgent._markRevision(this, callback));
    },

    /**
     * @param {string} name
     * @return {string}
     */
    getAttribute: function(name)
    {
        var attr = this._attributesMap[name];
        return attr ? attr.value : undefined;
    },

    /**
     * @param {string} name
     * @param {string} text
     * @param {function(?Protocol.Error)=} callback
     */
    setAttribute: function(name, text, callback)
    {
        DOMAgent.setAttributesAsText(this.id, text, name, WebInspector.domAgent._markRevision(this, callback));
    },

    /**
     * @param {string} name
     * @param {string} value
     * @param {function(?Protocol.Error)=} callback
     */
    setAttributeValue: function(name, value, callback)
    {
        DOMAgent.setAttributeValue(this.id, name, value, WebInspector.domAgent._markRevision(this, callback));
    },

    /**
     * @return {Object}
     */
    attributes: function()
    {
        return this._attributes;
    },

    /**
     * @param {string} name
     * @param {function(?Protocol.Error)=} callback
     */
    removeAttribute: function(name, callback)
    {
        /**
         *  @param {?Protocol.Error} error
         */
        function mycallback(error)
        {
            if (!error) {
                delete this._attributesMap[name];
                for (var i = 0;  i < this._attributes.length; ++i) {
                    if (this._attributes[i].name === name) {
                        this._attributes.splice(i, 1);
                        break;
                    }
                }
            }

            WebInspector.domAgent._markRevision(this, callback)(error);
        }
        DOMAgent.removeAttribute(this.id, name, mycallback.bind(this));
    },

    /**
     * @param {function(Array.<WebInspector.DOMNode>)=} callback
     */
    getChildNodes: function(callback)
    {
        if (this.children) {
            if (callback)
                callback(this.children);
            return;
        }

        /**
         * @this {WebInspector.DOMNode}
         * @param {?Protocol.Error} error
         */
        function mycallback(error)
        {
            if (!error && callback)
                callback(this.children);
        }

        DOMAgent.requestChildNodes(this.id, undefined, mycallback.bind(this));
    },

    /**
     * @param {number} depth
     * @param {function(Array.<WebInspector.DOMNode>)=} callback
     */
    getSubtree: function(depth, callback)
    {
        /**
         * @this {WebInspector.DOMNode}
         * @param {?Protocol.Error} error
         */
        function mycallback(error)
        {
            if (callback)
                callback(error ? null : this.children);                
        }

        DOMAgent.requestChildNodes(this.id, depth, mycallback.bind(this));
    },

    /**
     * @param {function(?Protocol.Error)=} callback
     */
    getOuterHTML: function(callback)
    {
        DOMAgent.getOuterHTML(this.id, callback);
    },

    /**
     * @param {string} html
     * @param {function(?Protocol.Error)=} callback
     */
    setOuterHTML: function(html, callback)
    {
        DOMAgent.setOuterHTML(this.id, html, WebInspector.domAgent._markRevision(this, callback));
    },

    /**
     * @param {function(?Protocol.Error)=} callback
     */
    removeNode: function(callback)
    {
        DOMAgent.removeNode(this.id, WebInspector.domAgent._markRevision(this, callback));
    },

    copyNode: function()
    {
        function copy(error, text)
        {
            if (!error)
                InspectorFrontendHost.copyText(text);
        }
        DOMAgent.getOuterHTML(this.id, copy);
    },

    /**
     * @param {boolean} optimized
     */
    copyXPath: function(optimized)
    {
        InspectorFrontendHost.copyText(this.xPath(optimized));
    },

    /**
     * @param {string} objectGroupId
     * @param {function(?Protocol.Error)=} callback
     */
    eventListeners: function(objectGroupId, callback)
    {
        DOMAgent.getEventListenersForNode(this.id, objectGroupId, callback);
    },

    /**
     * @return {string}
     */
    path: function()
    {
        var path = [];
        var node = this;
        while (node && "index" in node && node._nodeName.length) {
            path.push([node.index, node._nodeName]);
            node = node.parentNode;
        }
        path.reverse();
        return path.join(",");
    },

    /**
     * @param {boolean} justSelector
     * @return {string}
     */
    appropriateSelectorFor: function(justSelector)
    {
        var lowerCaseName = this.localName() || this.nodeName().toLowerCase();

        var id = this.getAttribute("id");
        if (id) {
            var selector = "#" + id;
            return (justSelector ? selector : lowerCaseName + selector);
        }

        var className = this.getAttribute("class");
        if (className) {
            var selector = "." + className.trim().replace(/\s+/g, ".");
            return (justSelector ? selector : lowerCaseName + selector);
        }

        if (lowerCaseName === "input" && this.getAttribute("type"))
            return lowerCaseName + "[type=\"" + this.getAttribute("type") + "\"]";

        return lowerCaseName;
    },

    /**
     * @param {WebInspector.DOMNode} node
     * @return {boolean}
     */
    isAncestor: function(node)
    {
        if (!node)
            return false;

        var currentNode = node.parentNode;
        while (currentNode) {
            if (this === currentNode)
                return true;
            currentNode = currentNode.parentNode;
        }
        return false;
    },

    /**
     * @param {WebInspector.DOMNode} descendant
     * @return {boolean}
     */
    isDescendant: function(descendant)
    {
        return descendant !== null && descendant.isAncestor(this);
    },

    /**
     * @param {Array.<string>} attrs
     * @return {boolean}
     */
    _setAttributesPayload: function(attrs)
    {
        var attributesChanged = !this._attributes || attrs.length !== this._attributes.length * 2;
        var oldAttributesMap = this._attributesMap || {};

        this._attributes = [];
        this._attributesMap = {};

        for (var i = 0; i < attrs.length; i += 2) {
            var name = attrs[i];
            var value = attrs[i + 1];
            this._addAttribute(name, value);

            if (attributesChanged)
                continue;

            if (!oldAttributesMap[name] || oldAttributesMap[name].value !== value)
              attributesChanged = true;
        }
        return attributesChanged;
    },

    /**
     * @param {WebInspector.DOMNode} prev
     * @param {DOMAgent.Node} payload
     * @return {WebInspector.DOMNode}
     */
    _insertChild: function(prev, payload)
    {
        var node = new WebInspector.DOMNode(this._domAgent, this.ownerDocument, this._isInShadowTree, payload);
        if (!prev) {
            if (!this.children) {
                // First node
                this.children = this._shadowRoots.slice();
                if (this._templateContent)
                    this.children.push(this._templateContent);

                this.children.push(node);
            } else
                this.children.unshift(node);
        } else
            this.children.splice(this.children.indexOf(prev) + 1, 0, node);
        this._renumber();
        return node;
    },

    /**
     * @param {WebInspector.DOMNode} node
     */
    _removeChild: function(node)
    {
        this.children.splice(this.children.indexOf(node), 1);
        node.parentNode = null;
        node._updateChildUserPropertyCountsOnRemoval(this);
        this._renumber();
    },

    /**
     * @param {Array.<DOMAgent.Node>} payloads
     */
    _setChildrenPayload: function(payloads)
    {
        // We set children in the constructor.
        if (this._contentDocument)
            return;

        this.children = this._shadowRoots.slice();
        if (this._templateContent)
            this.children.push(this._templateContent);

        for (var i = 0; i < payloads.length; ++i) {
            var payload = payloads[i];
            var node = new WebInspector.DOMNode(this._domAgent, this.ownerDocument, this._isInShadowTree, payload);
            this.children.push(node);
        }
        this._renumber();
    },

    _renumber: function()
    {
        this._childNodeCount = this.children.length;
        if (this._childNodeCount == 0) {
            this.firstChild = null;
            this.lastChild = null;
            return;
        }
        this.firstChild = this.children[0];
        this.lastChild = this.children[this._childNodeCount - 1];
        for (var i = 0; i < this._childNodeCount; ++i) {
            var child = this.children[i];
            child.index = i;
            child.nextSibling = i + 1 < this._childNodeCount ? this.children[i + 1] : null;
            child.previousSibling = i - 1 >= 0 ? this.children[i - 1] : null;
            child.parentNode = this;
        }
    },

    /**
     * @param {string} name
     * @param {string} value
     */
    _addAttribute: function(name, value)
    {
        var attr = {
            name: name,
            value: value,
            _node: this
        };
        this._attributesMap[name] = attr;
        this._attributes.push(attr);
    },

    /**
     * @param {string} name
     * @param {string} value
     */
    _setAttribute: function(name, value)
    {
        var attr = this._attributesMap[name];
        if (attr)
            attr.value = value;
        else
            this._addAttribute(name, value);
    },

    /**
     * @param {string} name
     */
    _removeAttribute: function(name)
    {
        var attr = this._attributesMap[name];
        if (attr) {
            this._attributes.remove(attr);
            delete this._attributesMap[name];
        }
    },

    /**
     * @param {WebInspector.DOMNode} targetNode
     * @param {?WebInspector.DOMNode} anchorNode
     * @param {function(?Protocol.Error)=} callback
     */
    moveTo: function(targetNode, anchorNode, callback)
    {
        DOMAgent.moveTo(this.id, targetNode.id, anchorNode ? anchorNode.id : undefined, WebInspector.domAgent._markRevision(this, callback));
    },

    /**
     * @return {boolean}
     */
    isXMLNode: function()
    {
        return !!this.ownerDocument && !!this.ownerDocument.xmlVersion;
    },

    /**
     * @param {boolean} optimized
     * @return {string}
     */
    xPath: function(optimized)
    {
        if (this._nodeType === Node.DOCUMENT_NODE)
            return "/";

        var steps = [];
        var contextNode = this;
        while (contextNode) {
            var step = contextNode._xPathValue(optimized);
            if (!step)
                break; // Error - bail out early.
            steps.push(step);
            if (step.optimized)
                break;
            contextNode = contextNode.parentNode;
        }

        steps.reverse();
        return (steps.length && steps[0].optimized ? "" : "/") + steps.join("/");
    },

    /**
     * @param {boolean} optimized
     * @return {WebInspector.DOMNode.XPathStep}
     */
    _xPathValue: function(optimized)
    {
        var ownValue;
        var ownIndex = this._xPathIndex();
        if (ownIndex === -1)
            return null; // Error.

        switch (this._nodeType) {
        case Node.ELEMENT_NODE:
            if (optimized && this.getAttribute("id"))
                return new WebInspector.DOMNode.XPathStep("//*[@id=\"" + this.getAttribute("id") + "\"]", true);
            ownValue = this._localName;
            break;
        case Node.ATTRIBUTE_NODE:
            ownValue = "@" + this._nodeName;
            break;
        case Node.TEXT_NODE:
        case Node.CDATA_SECTION_NODE:
            ownValue = "text()";
            break;
        case Node.PROCESSING_INSTRUCTION_NODE:
            ownValue = "processing-instruction()";
            break;
        case Node.COMMENT_NODE:
            ownValue = "comment()";
            break;
        case Node.DOCUMENT_NODE:
            ownValue = "";
            break;
        default:
            ownValue = "";
            break;
        }

        if (ownIndex > 0)
            ownValue += "[" + ownIndex + "]";

        return new WebInspector.DOMNode.XPathStep(ownValue, this._nodeType === Node.DOCUMENT_NODE);
    },

    /**
     * @return {number}
     */
    _xPathIndex: function()
    {
        // Returns -1 in case of error, 0 if no siblings matching the same expression, <XPath index among the same expression-matching sibling nodes> otherwise.
        function areNodesSimilar(left, right)
        {
            if (left === right)
                return true;

            if (left._nodeType === Node.ELEMENT_NODE && right._nodeType === Node.ELEMENT_NODE)
                return left._localName === right._localName;

            if (left._nodeType === right._nodeType)
                return true;

            // XPath treats CDATA as text nodes.
            var leftType = left._nodeType === Node.CDATA_SECTION_NODE ? Node.TEXT_NODE : left._nodeType;
            var rightType = right._nodeType === Node.CDATA_SECTION_NODE ? Node.TEXT_NODE : right._nodeType;
            return leftType === rightType;
        }

        var siblings = this.parentNode ? this.parentNode.children : null;
        if (!siblings)
            return 0; // Root node - no siblings.
        var hasSameNamedElements;
        for (var i = 0; i < siblings.length; ++i) {
            if (areNodesSimilar(this, siblings[i]) && siblings[i] !== this) {
                hasSameNamedElements = true;
                break;
            }
        }
        if (!hasSameNamedElements)
            return 0;
        var ownIndex = 1; // XPath indices start with 1.
        for (var i = 0; i < siblings.length; ++i) {
            if (areNodesSimilar(this, siblings[i])) {
                if (siblings[i] === this)
                    return ownIndex;
                ++ownIndex;
            }
        }
        return -1; // An error occurred: |this| not found in parent's children.
    },

    _updateChildUserPropertyCountsOnRemoval: function(parentNode)
    {
        var result = {};
        if (this._userProperties) {
            for (var name in this._userProperties)
                result[name] = (result[name] || 0) + 1;
        }

        if (this._descendantUserPropertyCounters) {
            for (var name in this._descendantUserPropertyCounters) {
                var counter = this._descendantUserPropertyCounters[name];
                result[name] = (result[name] || 0) + counter;
            }
        }

        for (var name in result)
            parentNode._updateDescendantUserPropertyCount(name, -result[name]);
    },

    _updateDescendantUserPropertyCount: function(name, delta)
    {
        if (!this._descendantUserPropertyCounters.hasOwnProperty(name))
            this._descendantUserPropertyCounters[name] = 0;
        this._descendantUserPropertyCounters[name] += delta;
        if (!this._descendantUserPropertyCounters[name])
            delete this._descendantUserPropertyCounters[name];
        if (this.parentNode)
            this.parentNode._updateDescendantUserPropertyCount(name, delta);
    },

    setUserProperty: function(name, value)
    {
        if (value === null) {
            this.removeUserProperty(name);
            return;
        }

        if (this.parentNode && !this._userProperties.hasOwnProperty(name))
            this.parentNode._updateDescendantUserPropertyCount(name, 1);

        this._userProperties[name] = value;
    },

    removeUserProperty: function(name)
    {
        if (!this._userProperties.hasOwnProperty(name))
            return;

        delete this._userProperties[name];
        if (this.parentNode)
            this.parentNode._updateDescendantUserPropertyCount(name, -1);
    },

    getUserProperty: function(name)
    {
        return this._userProperties ? this._userProperties[name] : null;
    },

    descendantUserPropertyCount: function(name)
    {
        return this._descendantUserPropertyCounters && this._descendantUserPropertyCounters[name] ? this._descendantUserPropertyCounters[name] : 0;
    },

    /**
     * @param {string} url
     * @return {?string}
     */
    resolveURL: function(url)
    {
        if (!url)
            return url;
        for (var frameOwnerCandidate = this; frameOwnerCandidate; frameOwnerCandidate = frameOwnerCandidate.parentNode) {
            if (frameOwnerCandidate.baseURL)
                return WebInspector.ParsedURL.completeURL(frameOwnerCandidate.baseURL, url);
        }
        return null;
    }
}

/**
 * @extends {WebInspector.DOMNode}
 * @constructor
 * @param {WebInspector.DOMAgent} domAgent
 * @param {DOMAgent.Node} payload
 */
WebInspector.DOMDocument = function(domAgent, payload)
{
    WebInspector.DOMNode.call(this, domAgent, this, false, payload);
    this.documentURL = payload.documentURL || "";
    this.baseURL = /** @type {string} */ (payload.baseURL);
    console.assert(this.baseURL);
    this.xmlVersion = payload.xmlVersion;
    this._listeners = {};
}

WebInspector.DOMDocument.prototype = {
    __proto__: WebInspector.DOMNode.prototype
}

/**
 * @extends {WebInspector.Object}
 * @constructor
 */
WebInspector.DOMAgent = function() {
    /** @type {Object|undefined} */
    this._idToDOMNode = {};
    this._document = null;
    this._attributeLoadNodeIds = {};
    InspectorBackend.registerDOMDispatcher(new WebInspector.DOMDispatcher(this));
    if (WebInspector.settings.emulateTouchEvents.get())
        this._emulateTouchEventsChanged();
    WebInspector.settings.emulateTouchEvents.addChangeListener(this._emulateTouchEventsChanged, this);
}

WebInspector.DOMAgent.Events = {
    AttrModified: "AttrModified",
    AttrRemoved: "AttrRemoved",
    CharacterDataModified: "CharacterDataModified",
    NodeInserted: "NodeInserted",
    NodeRemoved: "NodeRemoved",
    DocumentUpdated: "DocumentUpdated",
    ChildNodeCountUpdated: "ChildNodeCountUpdated",
    InspectElementRequested: "InspectElementRequested",
    UndoRedoRequested: "UndoRedoRequested",
    UndoRedoCompleted: "UndoRedoCompleted"
}

WebInspector.DOMAgent.prototype = {
    /**
     * @param {function(WebInspector.DOMDocument)=} callback
     */
    requestDocument: function(callback)
    {
        if (this._document) {
            if (callback)
                callback(this._document);
            return;
        }

        if (this._pendingDocumentRequestCallbacks) {
            this._pendingDocumentRequestCallbacks.push(callback);
            return;
        }

        this._pendingDocumentRequestCallbacks = [callback];

        /**
         * @this {WebInspector.DOMAgent}
         * @param {?Protocol.Error} error
         * @param {DOMAgent.Node} root
         */
        function onDocumentAvailable(error, root)
        {
            if (!error)
                this._setDocument(root);

            for (var i = 0; i < this._pendingDocumentRequestCallbacks.length; ++i) {
                var callback = this._pendingDocumentRequestCallbacks[i];
                if (callback)
                    callback(this._document);
            }
            delete this._pendingDocumentRequestCallbacks;
        }

        DOMAgent.getDocument(onDocumentAvailable.bind(this));
    },

    /**
     * @return {WebInspector.DOMDocument?}
     */
    existingDocument: function()
    {
        return this._document;
    },

    /**
     * @param {RuntimeAgent.RemoteObjectId} objectId
     * @param {function(?DOMAgent.NodeId)=} callback
     */
    pushNodeToFrontend: function(objectId, callback)
    {
        var callbackCast = /** @type {function(*)} */(callback);
        this._dispatchWhenDocumentAvailable(DOMAgent.requestNode.bind(DOMAgent, objectId), callbackCast);
    },

    /**
     * @param {string} path
     * @param {function(?WebInspector.DOMNode)=} callback
     */
    pushNodeByPathToFrontend: function(path, callback)
    {
        var callbackCast = /** @type {function(*)} */(callback);
        this._dispatchWhenDocumentAvailable(DOMAgent.pushNodeByPathToFrontend.bind(DOMAgent, path), callbackCast);
    },

    /**
     * @param {function(*)=} callback
     * @return {function(?Protocol.Error,*=)|undefined}
     */
    _wrapClientCallback: function(callback)
    {
        if (!callback)
            return;
        /**
         * @param {?Protocol.Error} error
         * @param {*=} result
         */
        return function(error, result)
        {
            // Caller is responsible for handling the actual error.
            callback(error ? null : result);
        }
    },

    /**
     * @param {function(function(?Protocol.Error, *=))} func
     * @param {function(*)=} callback
     */
    _dispatchWhenDocumentAvailable: function(func, callback)
    {
        var callbackWrapper = /** @type {function(?Protocol.Error, *=)} */(this._wrapClientCallback(callback));

        function onDocumentAvailable()
        {
            if (this._document)
                func(callbackWrapper);
            else {
                if (callbackWrapper)
                    callbackWrapper("No document");
            }
        }
        this.requestDocument(onDocumentAvailable.bind(this));
    },

    /**
     * @param {DOMAgent.NodeId} nodeId
     * @param {string} name
     * @param {string} value
     */
    _attributeModified: function(nodeId, name, value)
    {
        var node = this._idToDOMNode[nodeId];
        if (!node)
            return;

        node._setAttribute(name, value);
        this.dispatchEventToListeners(WebInspector.DOMAgent.Events.AttrModified, { node: node, name: name });
    },

    /**
     * @param {DOMAgent.NodeId} nodeId
     * @param {string} name
     */
    _attributeRemoved: function(nodeId, name)
    {
        var node = this._idToDOMNode[nodeId];
        if (!node)
            return;
        node._removeAttribute(name);
        this.dispatchEventToListeners(WebInspector.DOMAgent.Events.AttrRemoved, { node: node, name: name });
    },

    /**
     * @param {Array.<DOMAgent.NodeId>} nodeIds
     */
    _inlineStyleInvalidated: function(nodeIds)
    {
        for (var i = 0; i < nodeIds.length; ++i)
            this._attributeLoadNodeIds[nodeIds[i]] = true;
        if ("_loadNodeAttributesTimeout" in this)
            return;
        this._loadNodeAttributesTimeout = setTimeout(this._loadNodeAttributes.bind(this), 0);
    },

    _loadNodeAttributes: function()
    {
        /**
         * @this {WebInspector.DOMAgent}
         * @param {DOMAgent.NodeId} nodeId
         * @param {?Protocol.Error} error
         * @param {Array.<string>} attributes
         */
        function callback(nodeId, error, attributes)
        {
            if (error) {
                // We are calling _loadNodeAttributes asynchronously, it is ok if node is not found.
                return;
            }
            var node = this._idToDOMNode[nodeId];
            if (node) {
                if (node._setAttributesPayload(attributes))
                    this.dispatchEventToListeners(WebInspector.DOMAgent.Events.AttrModified, { node: node, name: "style" });
            }
        }

        delete this._loadNodeAttributesTimeout;

        for (var nodeId in this._attributeLoadNodeIds) {
            var nodeIdAsNumber = parseInt(nodeId, 10);
            DOMAgent.getAttributes(nodeIdAsNumber, callback.bind(this, nodeIdAsNumber));
        }
        this._attributeLoadNodeIds = {};
    },

    /**
     * @param {DOMAgent.NodeId} nodeId
     * @param {string} newValue
     */
    _characterDataModified: function(nodeId, newValue)
    {
        var node = this._idToDOMNode[nodeId];
        node._nodeValue = newValue;
        this.dispatchEventToListeners(WebInspector.DOMAgent.Events.CharacterDataModified, node);
    },

    /**
     * @param {DOMAgent.NodeId} nodeId
     * @return {WebInspector.DOMNode|undefined}
     */
    nodeForId: function(nodeId)
    {
        return this._idToDOMNode[nodeId];
    },

    _documentUpdated: function()
    {
        this._setDocument(null);
    },

    /**
     * @param {DOMAgent.Node} payload
     */
    _setDocument: function(payload)
    {
        this._idToDOMNode = {};
        if (payload && "nodeId" in payload)
            this._document = new WebInspector.DOMDocument(this, payload);
        else
            this._document = null;
        this.dispatchEventToListeners(WebInspector.DOMAgent.Events.DocumentUpdated, this._document);
    },

    /**
     * @param {DOMAgent.Node} payload
     */
    _setDetachedRoot: function(payload)
    {
        if (payload.nodeName === "#document")
            new WebInspector.DOMDocument(this, payload);
        else
            new WebInspector.DOMNode(this, null, false, payload);
    },

    /**
     * @param {DOMAgent.NodeId} parentId
     * @param {Array.<DOMAgent.Node>} payloads
     */
    _setChildNodes: function(parentId, payloads)
    {
        if (!parentId && payloads.length) {
            this._setDetachedRoot(payloads[0]);
            return;
        }

        var parent = this._idToDOMNode[parentId];
        parent._setChildrenPayload(payloads);
    },

    /**
     * @param {DOMAgent.NodeId} nodeId
     * @param {number} newValue
     */
    _childNodeCountUpdated: function(nodeId, newValue)
    {
        var node = this._idToDOMNode[nodeId];
        node._childNodeCount = newValue;
        this.dispatchEventToListeners(WebInspector.DOMAgent.Events.ChildNodeCountUpdated, node);
    },

    /**
     * @param {DOMAgent.NodeId} parentId
     * @param {DOMAgent.NodeId} prevId
     * @param {DOMAgent.Node} payload
     */
    _childNodeInserted: function(parentId, prevId, payload)
    {
        var parent = this._idToDOMNode[parentId];
        var prev = this._idToDOMNode[prevId];
        var node = parent._insertChild(prev, payload);
        this._idToDOMNode[node.id] = node;
        this.dispatchEventToListeners(WebInspector.DOMAgent.Events.NodeInserted, node);
    },

    /**
     * @param {DOMAgent.NodeId} parentId
     * @param {DOMAgent.NodeId} nodeId
     */
    _childNodeRemoved: function(parentId, nodeId)
    {
        var parent = this._idToDOMNode[parentId];
        var node = this._idToDOMNode[nodeId];
        parent._removeChild(node);
        this._unbind(node);
        this.dispatchEventToListeners(WebInspector.DOMAgent.Events.NodeRemoved, {node: node, parent: parent});
    },

    /**
     * @param {DOMAgent.NodeId} rootId
     */
    _shadowRootPopped: function(rootId)
    {
    },

    /**
     * @param {WebInspector.DOMNode} node
     */
    _unbind: function(node)
    {
        delete this._idToDOMNode[node.id];
        for (var i = 0; node.children && i < node.children.length; ++i)
            this._unbind(node.children[i]);
    },

    /**
     * @param {number} nodeId
     */
    inspectElement: function(nodeId)
    {
        var node = this._idToDOMNode[nodeId];
        if (node)
            this.dispatchEventToListeners(WebInspector.DOMAgent.Events.InspectElementRequested, node);
    },

    /**
     * @param {string} query
     * @param {function(number)} searchCallback
     */
    performSearch: function(query, searchCallback)
    {
        this.cancelSearch();

        /**
         * @param {?Protocol.Error} error
         * @param {string} searchId
         * @param {number} resultsCount
         */
        function callback(error, searchId, resultsCount)
        {
            this._searchId = searchId;
            searchCallback(resultsCount);
        }
        DOMAgent.performSearch(query, callback.bind(this));
    },

    /**
     * @param {number} index
     * @param {?function(DOMAgent.Node)} callback
     */
    searchResult: function(index, callback)
    {
        if (this._searchId) {
            /**
             * @param {?Protocol.Error} error
             * @param {Array.<number>} nodeIds
             */
            function mycallback(error, nodeIds)
            {
                if (error) {
                    console.error(error);
                    callback(null);
                    return;
                }
                if (nodeIds.length != 1)
                    return;

                callback(this._idToDOMNode[nodeIds[0]]);
            }
            DOMAgent.getSearchResults(this._searchId, index, index + 1, mycallback.bind(this));
        } else
            callback(null);
    },

    cancelSearch: function()
    {
        if (this._searchId) {
            DOMAgent.discardSearchResults(this._searchId);
            delete this._searchId;
        }
    },

    /**
     * @param {DOMAgent.NodeId} nodeId
     * @param {string} selectors
     * @param {function(?DOMAgent.NodeId)=} callback
     */
    querySelector: function(nodeId, selectors, callback)
    {
        var callbackCast = /** @type {function(*)|undefined} */(callback);
        DOMAgent.querySelector(nodeId, selectors, this._wrapClientCallback(callbackCast));
    },

    /**
     * @param {DOMAgent.NodeId} nodeId
     * @param {string} selectors
     * @param {function(?Array.<DOMAgent.NodeId>)=} callback
     */
    querySelectorAll: function(nodeId, selectors, callback)
    {
        var callbackCast = /** @type {function(*)|undefined} */(callback);
        DOMAgent.querySelectorAll(nodeId, selectors, this._wrapClientCallback(callbackCast));
    },

    /**
     * @param {DOMAgent.NodeId=} nodeId
     * @param {string=} mode
     * @param {RuntimeAgent.RemoteObjectId=} objectId
     */
    highlightDOMNode: function(nodeId, mode, objectId)
    {
        if (this._hideDOMNodeHighlightTimeout) {
            clearTimeout(this._hideDOMNodeHighlightTimeout);
            delete this._hideDOMNodeHighlightTimeout;
        }

        if (objectId || nodeId)
            DOMAgent.highlightNode(this._buildHighlightConfig(mode), objectId ? undefined : nodeId, objectId);
        else
            DOMAgent.hideHighlight();
    },

    hideDOMNodeHighlight: function()
    {
        this.highlightDOMNode(0);
    },

    /**
     * @param {DOMAgent.NodeId} nodeId
     */
    highlightDOMNodeForTwoSeconds: function(nodeId)
    {
        this.highlightDOMNode(nodeId);
        this._hideDOMNodeHighlightTimeout = setTimeout(this.hideDOMNodeHighlight.bind(this), 2000);
    },

    /**
     * @param {boolean} enabled
     * @param {function(?Protocol.Error)=} callback
     */
    setInspectModeEnabled: function(enabled, callback)
    {
        DOMAgent.setInspectModeEnabled(enabled, this._buildHighlightConfig(), callback);
    },

    /**
     * @param {string=} mode
     */
    _buildHighlightConfig: function(mode)
    {
        mode = mode || "all";
        var highlightConfig = { showInfo: mode === "all", showRulers: WebInspector.settings.showMetricsRulers.get() };
        if (mode === "all" || mode === "content")
            highlightConfig.contentColor = WebInspector.Color.PageHighlight.Content.toProtocolRGBA();

        if (mode === "all" || mode === "padding")
            highlightConfig.paddingColor = WebInspector.Color.PageHighlight.Padding.toProtocolRGBA();

        if (mode === "all" || mode === "border")
            highlightConfig.borderColor = WebInspector.Color.PageHighlight.Border.toProtocolRGBA();

        if (mode === "all" || mode === "margin")
            highlightConfig.marginColor = WebInspector.Color.PageHighlight.Margin.toProtocolRGBA();

        return highlightConfig;
    },

    /**
     * @param {WebInspector.DOMNode} node
     * @param {function(?Protocol.Error)=} callback
     * @return {function(?Protocol.Error)}
     */
    _markRevision: function(node, callback)
    {
        function wrapperFunction(error)
        {
            if (!error)
                this.markUndoableState();

            if (callback)
                callback.apply(this, arguments);
        }
        return wrapperFunction.bind(this);
    },

    _emulateTouchEventsChanged: function()
    {
        const injectedFunction = function() {
            const touchEvents = ["ontouchstart", "ontouchend", "ontouchmove", "ontouchcancel"];
            var recepients = [window.__proto__, document.__proto__];
            for (var i = 0; i < touchEvents.length; ++i) {
                for (var j = 0; j < recepients.length; ++j) {
                    if (!(touchEvents[i] in recepients[j]))
                        Object.defineProperty(recepients[j], touchEvents[i], { value: null, writable: true, configurable: true, enumerable: true });
                }
            }
        }

        var emulationEnabled = WebInspector.settings.emulateTouchEvents.get();
        if (emulationEnabled && !this._addTouchEventsScriptInjecting) {
            this._addTouchEventsScriptInjecting = true;
            PageAgent.addScriptToEvaluateOnLoad("(" + injectedFunction.toString() + ")()", scriptAddedCallback.bind(this));
        } else {
            if (typeof this._addTouchEventsScriptId !== "undefined") {
                PageAgent.removeScriptToEvaluateOnLoad(this._addTouchEventsScriptId);
                delete this._addTouchEventsScriptId;
            }
        }

        function scriptAddedCallback(error, scriptId)
        {
            delete this._addTouchEventsScriptInjecting;
            if (error)
                return;
            this._addTouchEventsScriptId = scriptId;
        }

        PageAgent.setTouchEmulationEnabled(emulationEnabled);
    },

    markUndoableState: function()
    {
        DOMAgent.markUndoableState();
    },

    /**
     * @param {function(?Protocol.Error)=} callback
     */
    undo: function(callback)
    {
        function mycallback(error)
        {
            this.dispatchEventToListeners(WebInspector.DOMAgent.Events.UndoRedoCompleted);
            callback(error);
        }

        this.dispatchEventToListeners(WebInspector.DOMAgent.Events.UndoRedoRequested);
        DOMAgent.undo(callback);
    },

    /**
     * @param {function(?Protocol.Error)=} callback
     */
    redo: function(callback)
    {
        function mycallback(error)
        {
            this.dispatchEventToListeners(WebInspector.DOMAgent.Events.UndoRedoCompleted);
            callback(error);
        }

        this.dispatchEventToListeners(WebInspector.DOMAgent.Events.UndoRedoRequested);
        DOMAgent.redo(callback);
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 * @implements {DOMAgent.Dispatcher}
 * @param {WebInspector.DOMAgent} domAgent
 */
WebInspector.DOMDispatcher = function(domAgent)
{
    this._domAgent = domAgent;
}

WebInspector.DOMDispatcher.prototype = {
    documentUpdated: function()
    {
        this._domAgent._documentUpdated();
    },

    /**
     * @param {DOMAgent.NodeId} nodeId
     * @param {string} name
     * @param {string} value
     */
    attributeModified: function(nodeId, name, value)
    {
        this._domAgent._attributeModified(nodeId, name, value);
    },

    /**
     * @param {DOMAgent.NodeId} nodeId
     * @param {string} name
     */
    attributeRemoved: function(nodeId, name)
    {
        this._domAgent._attributeRemoved(nodeId, name);
    },

    /**
     * @param {Array.<DOMAgent.NodeId>} nodeIds
     */
    inlineStyleInvalidated: function(nodeIds)
    {
        this._domAgent._inlineStyleInvalidated(nodeIds);
    },

    /**
     * @param {DOMAgent.NodeId} nodeId
     * @param {string} characterData
     */
    characterDataModified: function(nodeId, characterData)
    {
        this._domAgent._characterDataModified(nodeId, characterData);
    },

    /**
     * @param {DOMAgent.NodeId} parentId
     * @param {Array.<DOMAgent.Node>} payloads
     */
    setChildNodes: function(parentId, payloads)
    {
        this._domAgent._setChildNodes(parentId, payloads);
    },

    /**
     * @param {DOMAgent.NodeId} nodeId
     * @param {number} childNodeCount
     */
    childNodeCountUpdated: function(nodeId, childNodeCount)
    {
        this._domAgent._childNodeCountUpdated(nodeId, childNodeCount);
    },

    /**
     * @param {DOMAgent.NodeId} parentNodeId
     * @param {DOMAgent.NodeId} previousNodeId
     * @param {DOMAgent.Node} payload
     */
    childNodeInserted: function(parentNodeId, previousNodeId, payload)
    {
        this._domAgent._childNodeInserted(parentNodeId, previousNodeId, payload);
    },

    /**
     * @param {DOMAgent.NodeId} parentNodeId
     * @param {DOMAgent.NodeId} nodeId
     */
    childNodeRemoved: function(parentNodeId, nodeId)
    {
        this._domAgent._childNodeRemoved(parentNodeId, nodeId);
    },

    /**
     * @param {DOMAgent.NodeId} hostId
     * @param {DOMAgent.Node} root
     */
    shadowRootPushed: function(hostId, root)
    {
        this._domAgent._childNodeInserted(hostId, 0, root);
    },

    /**
     * @param {DOMAgent.NodeId} hostId
     * @param {DOMAgent.NodeId} rootId
     */
    shadowRootPopped: function(hostId, rootId)
    {
        this._domAgent._childNodeRemoved(hostId, rootId);
    }
}

/**
 * @type {?WebInspector.DOMAgent}
 */
WebInspector.domAgent = null;
