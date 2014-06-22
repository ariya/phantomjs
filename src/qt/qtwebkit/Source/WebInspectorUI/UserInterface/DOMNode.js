/*
 * Copyright (C) 2009, 2010 Google Inc. All rights reserved.
 * Copyright (C) 2009 Joseph Pecoraro
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * @param {?WebInspector.DOMNode} doc
 * @param {boolean} isInShadowTree
 * @param {DOMAgent.Node} payload
 */
WebInspector.DOMNode = function(domAgent, doc, isInShadowTree, payload) {
    WebInspector.Object.call(this);

    this._domAgent = domAgent;
    this._isInShadowTree = isInShadowTree;

    this.id = payload.nodeId;
    domAgent._idToDOMNode[this.id] = this;

    this._nodeType = payload.nodeType;
    this._nodeName = payload.nodeName;
    this._localName = payload.localName;
    this._nodeValue = payload.nodeValue;

    if (this._nodeType === Node.DOCUMENT_NODE)
        this.ownerDocument = this;
    else
        this.ownerDocument = doc;

    this._attributes = [];
    this._attributesMap = {};
    if (payload.attributes)
        this._setAttributesPayload(payload.attributes);

    this._childNodeCount = payload.childNodeCount;
    this._children = null;
    this._filteredChildren = null;
    this._filteredChildrenNeedsUpdating = true;

    this._nextSibling = null;
    this._previousSibling = null;
    this.parentNode = null;

    this._enabledPseudoClasses = [];

    this._shadowRoots = [];
    if (payload.shadowRoots) {
        for (var i = 0; i < payload.shadowRoots.length; ++i) {
            var root = payload.shadowRoots[i];
            var node = new WebInspector.DOMNode(this._domAgent, this.ownerDocument, true, root);
            this._shadowRoots.push(node);
        }
    }

    if (payload.children)
        this._setChildrenPayload(payload.children);

    if (payload.contentDocument) {
        this._contentDocument = new WebInspector.DOMNode(domAgent, null, false, payload.contentDocument);
        this._children = [this._contentDocument];
        this._renumber();
    }

    if (this._nodeType === Node.ELEMENT_NODE) {
        // HTML and BODY from internal iframes should not overwrite top-level ones.
        if (this.ownerDocument && !this.ownerDocument.documentElement && this._nodeName === "HTML")
            this.ownerDocument.documentElement = this;
        if (this.ownerDocument && !this.ownerDocument.body && this._nodeName === "BODY")
            this.ownerDocument.body = this;
        if (payload.documentURL)
            this.documentURL = payload.documentURL;
    } else if (this._nodeType === Node.DOCUMENT_TYPE_NODE) {
        this.publicId = payload.publicId;
        this.systemId = payload.systemId;
        this.internalSubset = payload.internalSubset;
    } else if (this._nodeType === Node.DOCUMENT_NODE) {
        this.documentURL = payload.documentURL;
        this.xmlVersion = payload.xmlVersion;
    } else if (this._nodeType === Node.ATTRIBUTE_NODE) {
        this.name = payload.name;
        this.value = payload.value;
    }
}

WebInspector.Object.addConstructorFunctions(WebInspector.DOMNode);

WebInspector.DOMNode.Event = {
    EnabledPseudoClassesChanged: "dom-node-enabled-pseudo-classes-did-change",
    AttributeModified: "dom-node-attribute-modified",
    AttributeRemoved: "dom-node-attribute-removed"
};

WebInspector.DOMNode.prototype = {
    constructor: WebInspector.DOMNode,

    get children()
    {
        if (!this._children)
            return null;

        if (WebInspector.showShadowDOMSetting.value)
            return this._children;

        if (this._filteredChildrenNeedsUpdating) {
            this._filteredChildrenNeedsUpdating = false;
            this._filteredChildren = this._children.filter(function(node) {
                return !node._isInShadowTree;
            });
        }

        return this._filteredChildren;
    },

    get firstChild()
    {
        var children = this.children;

        if (children && children.length > 0)
            return children[0];

        return null;
    },

    get lastChild()
    {
        var children = this.children;

        if (children && children.length > 0)
            return children.lastValue;

        return null;
    },

    get nextSibling()
    {
        if (WebInspector.showShadowDOMSetting.value)
            return this._nextSibling;

        var node = this._nextSibling;
        while (node) {
            if (!node._isInShadowTree)
                return node;
            node = node._nextSibling;
        }
        return null;
    },

    get previousSibling()
    {
        if (WebInspector.showShadowDOMSetting.value)
            return this._previousSibling;

        var node = this._previousSibling;
        while (node) {
            if (!node._isInShadowTree)
                return node;
            node = node._previousSibling;
        }
        return null;
    },

    get childNodeCount()
    {
        var children = this.children;
        if (children)
            return children.length;

        if (WebInspector.showShadowDOMSetting.value)
            return this._childNodeCount + this._shadowRoots.length;

        return this._childNodeCount;
    },

    set childNodeCount(count)
    {
        this._childNodeCount = count;
    },
    
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
        return this.childNodeCount > 0;
    },

    /**
     * @return {boolean}
     */
    hasShadowRoots: function()
    {
        return !!this._shadowRoots.length;
    },

    /**
     * @return {boolean}
     */
    isInShadowTree: function()
    {
        return this._isInShadowTree;
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
     * @return {string}
     */
    nodeNameInCorrectCase: function()
    {
        return this.isXMLNode() ? this.nodeName() : this.nodeName().toLowerCase();
    },

    /**
     * @param {string} name
     * @param {function()=} callback
     */
    setNodeName: function(name, callback)
    {
        DOMAgent.setNodeName(this.id, name, this._makeUndoableCallback(callback));
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
        DOMAgent.setNodeValue(this.id, value, this._makeUndoableCallback(callback));
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
     * @param {function()=} callback
     */
    setAttribute: function(name, text, callback)
    {
        DOMAgent.setAttributesAsText(this.id, text, name, this._makeUndoableCallback(callback));
    },

    /**
     * @param {string} name
     * @param {string} value
     * @param {function()=} callback
     */
    setAttributeValue: function(name, value, callback)
    {
        DOMAgent.setAttributeValue(this.id, name, value, this._makeUndoableCallback(callback));
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
     * @param {function()=} callback
     */
    removeAttribute: function(name, callback)
    {
        function mycallback(error, success)
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

            this._makeUndoableCallback(callback)(error);
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
        function mycallback(error) {
            if (!error && callback)
                callback(this.children);
        }

        DOMAgent.requestChildNodes(this.id, mycallback.bind(this));
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
        DOMAgent.setOuterHTML(this.id, html, this._makeUndoableCallback(callback));
    },

    /**
     * @param {function(?Protocol.Error)=} callback
     */
    removeNode: function(callback)
    {
        DOMAgent.removeNode(this.id, this._makeUndoableCallback(callback));
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
     * @param {function(?Protocol.Error)=} callback
     */
    eventListeners: function(callback)
    {
        DOMAgent.getEventListenersForNode(this.id, callback);
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
            if (/[\s'"]/.test(id)) {
                id = id.replace(/\\/g, "\\\\").replace(/\"/g, "\\\"");
                selector = lowerCaseName + "[id=\"" + id + "\"]";
            } else
                selector = "#" + id;
            return (justSelector ? selector : lowerCaseName + selector);
        }

        var className = this.getAttribute("class");
        if (className) {
            var selector = "." + className.trim().replace(/\s+/, ".");
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
     */
    _setAttributesPayload: function(attrs)
    {
        this._attributes = [];
        this._attributesMap = {};
        for (var i = 0; i < attrs.length; i += 2)
            this._addAttribute(attrs[i], attrs[i + 1]);
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
            if (!this._children) {
                // First node
                this._children = this._shadowRoots.concat([node]);
            } else
                this._children.unshift(node);
        } else
            this._children.splice(this._children.indexOf(prev) + 1, 0, node);
        this._renumber();
        return node;
    },

    /**
     * @param {WebInspector.DOMNode} node
     */
    _removeChild: function(node)
    {
        this._children.splice(this._children.indexOf(node), 1);
        node.parentNode = null;
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

        this._children = this._shadowRoots.slice();
        for (var i = 0; i < payloads.length; ++i) {
            var payload = payloads[i];
            var node = new WebInspector.DOMNode(this._domAgent, this.ownerDocument, this._isInShadowTree, payload);
            this._children.push(node);
        }
        this._renumber();
    },

    _renumber: function()
    {
        this._filteredChildrenNeedsUpdating = true;

        var childNodeCount = this._children.length;
        if (childNodeCount === 0)
            return;

        for (var i = 0; i < childNodeCount; ++i) {
            var child = this._children[i];
            child.index = i;
            child._nextSibling = i + 1 < childNodeCount ? this._children[i + 1] : null;
            child._previousSibling = i - 1 >= 0 ? this._children[i - 1] : null;
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
        DOMAgent.moveTo(this.id, targetNode.id, anchorNode ? anchorNode.id : undefined, this._makeUndoableCallback(callback));
    },

    /**
     * @return {boolean}
     */
    isXMLNode: function()
    {
        return !!this.ownerDocument && !!this.ownerDocument.xmlVersion;
    },

    get enabledPseudoClasses()
    {
        return this._enabledPseudoClasses;
    },
    
    setPseudoClassEnabled: function(pseudoClass, enabled)
    {
        var pseudoClasses = this._enabledPseudoClasses;
        if (enabled) {
            if (pseudoClasses.contains(pseudoClass))
                return;
            pseudoClasses.push(pseudoClass);
        } else {
            if (!pseudoClasses.contains(pseudoClass))
                return;
            pseudoClasses.remove(pseudoClass);
        }

        function changed(error)
        {
            if (!error)
                this.dispatchEventToListeners(WebInspector.DOMNode.Event.EnabledPseudoClassesChanged);
        }

        CSSAgent.forcePseudoState(this.id, pseudoClasses, changed.bind(this));
    },

    _makeUndoableCallback: function(callback)
    {
        return function(error)
        {
            if (!error)
                DOMAgent.markUndoableState();

            if (callback)
                callback.apply(null, arguments);
        };
    }
}

WebInspector.DOMNode.prototype.__proto__ = WebInspector.Object.prototype;
