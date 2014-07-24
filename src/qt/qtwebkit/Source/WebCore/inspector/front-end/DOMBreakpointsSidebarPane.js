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
 * @extends {WebInspector.NativeBreakpointsSidebarPane}
 */
WebInspector.DOMBreakpointsSidebarPane = function()
{
    WebInspector.NativeBreakpointsSidebarPane.call(this, WebInspector.UIString("DOM Breakpoints"));

    this._breakpointElements = {};

    this._breakpointTypes = {
        SubtreeModified: "subtree-modified",
        AttributeModified: "attribute-modified",
        NodeRemoved: "node-removed"
    };
    this._breakpointTypeLabels = {};
    this._breakpointTypeLabels[this._breakpointTypes.SubtreeModified] = WebInspector.UIString("Subtree Modified");
    this._breakpointTypeLabels[this._breakpointTypes.AttributeModified] = WebInspector.UIString("Attribute Modified");
    this._breakpointTypeLabels[this._breakpointTypes.NodeRemoved] = WebInspector.UIString("Node Removed");

    this._contextMenuLabels = {};
    this._contextMenuLabels[this._breakpointTypes.SubtreeModified] = WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Subtree modifications" : "Subtree Modifications");
    this._contextMenuLabels[this._breakpointTypes.AttributeModified] = WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Attributes modifications" : "Attributes Modifications");
    this._contextMenuLabels[this._breakpointTypes.NodeRemoved] = WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Node removal" : "Node Removal");

    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.InspectedURLChanged, this._inspectedURLChanged, this);
    WebInspector.domAgent.addEventListener(WebInspector.DOMAgent.Events.NodeRemoved, this._nodeRemoved, this);
}

WebInspector.DOMBreakpointsSidebarPane.prototype = {
    _inspectedURLChanged: function(event)
    {
        this._breakpointElements = {};
        this._reset();
        var url = event.data;
        this._inspectedURL = url.removeURLFragment();
    },

    populateNodeContextMenu: function(node, contextMenu)
    {
        var nodeBreakpoints = {};
        for (var id in this._breakpointElements) {
            var element = this._breakpointElements[id];
            if (element._node === node)
                nodeBreakpoints[element._type] = true;
        }

        function toggleBreakpoint(type)
        {
            if (!nodeBreakpoints[type])
                this._setBreakpoint(node, type, true);
            else
                this._removeBreakpoint(node, type);
            this._saveBreakpoints();
        }

        var breakPointSubMenu = contextMenu.appendSubMenuItem(WebInspector.UIString("Break on..."));
        for (var key in this._breakpointTypes) {
            var type = this._breakpointTypes[key];
            var label = this._contextMenuLabels[type];
            breakPointSubMenu.appendCheckboxItem(label, toggleBreakpoint.bind(this, type), nodeBreakpoints[type]);
        }
    },

    createBreakpointHitStatusMessage: function(auxData, callback)
    {
        if (auxData.type === this._breakpointTypes.SubtreeModified) {
            var targetNodeObject = WebInspector.RemoteObject.fromPayload(auxData["targetNode"]);
            function didPushNodeToFrontend(targetNodeId)
            {
                if (targetNodeId)
                    targetNodeObject.release();
                this._doCreateBreakpointHitStatusMessage(auxData, targetNodeId, callback);
            }
            targetNodeObject.pushNodeToFrontend(didPushNodeToFrontend.bind(this));
        } else
            this._doCreateBreakpointHitStatusMessage(auxData, null, callback);
    },

    _doCreateBreakpointHitStatusMessage: function (auxData, targetNodeId, callback)
    {
        var message;
        var typeLabel = this._breakpointTypeLabels[auxData.type];
        var linkifiedNode = WebInspector.DOMPresentationUtils.linkifyNodeById(auxData.nodeId);
        var substitutions = [typeLabel, linkifiedNode];
        var targetNode = "";
        if (targetNodeId)
            targetNode = WebInspector.DOMPresentationUtils.linkifyNodeById(targetNodeId);

        if (auxData.type === this._breakpointTypes.SubtreeModified) {
            if (auxData.insertion) {
                if (targetNodeId !== auxData.nodeId) {
                    message = "Paused on a \"%s\" breakpoint set on %s, because a new child was added to its descendant %s.";
                    substitutions.push(targetNode);
                } else
                    message = "Paused on a \"%s\" breakpoint set on %s, because a new child was added to that node.";
            } else {
                message = "Paused on a \"%s\" breakpoint set on %s, because its descendant %s was removed.";
                substitutions.push(targetNode);
            }
        } else
            message = "Paused on a \"%s\" breakpoint set on %s.";

        var element = document.createElement("span");
        var formatters = {
            s: function(substitution)
            {
                return substitution;
            }
        };
        function append(a, b)
        {
            if (typeof b === "string")
                b = document.createTextNode(b);
            element.appendChild(b);
        }
        WebInspector.formatLocalized(message, substitutions, formatters, "", append);

        callback(element);
    },

    _nodeRemoved: function(event)
    {
        var node = event.data.node;
        this._removeBreakpointsForNode(event.data.node);
        if (!node.children)
            return;
        for (var i = 0; i < node.children.length; ++i)
            this._removeBreakpointsForNode(node.children[i]);
        this._saveBreakpoints();
    },

    _removeBreakpointsForNode: function(node)
    {
        for (var id in this._breakpointElements) {
            var element = this._breakpointElements[id];
            if (element._node === node)
                this._removeBreakpoint(element._node, element._type);
        }
    },

    _setBreakpoint: function(node, type, enabled)
    {
        var breakpointId = this._createBreakpointId(node.id, type);
        if (breakpointId in this._breakpointElements)
            return;

        var element = document.createElement("li");
        element._node = node;
        element._type = type;
        element.addEventListener("contextmenu", this._contextMenu.bind(this, node, type), true);

        var checkboxElement = document.createElement("input");
        checkboxElement.className = "checkbox-elem";
        checkboxElement.type = "checkbox";
        checkboxElement.checked = enabled;
        checkboxElement.addEventListener("click", this._checkboxClicked.bind(this, node, type), false);
        element._checkboxElement = checkboxElement;
        element.appendChild(checkboxElement);

        var labelElement = document.createElement("span");
        element.appendChild(labelElement);

        var linkifiedNode = WebInspector.DOMPresentationUtils.linkifyNodeById(node.id);
        linkifiedNode.addStyleClass("monospace");
        labelElement.appendChild(linkifiedNode);

        var description = document.createElement("div");
        description.className = "source-text";
        description.textContent = this._breakpointTypeLabels[type];
        labelElement.appendChild(description);

        var currentElement = this.listElement.firstChild;
        while (currentElement) {
            if (currentElement._type && currentElement._type < element._type)
                break;
            currentElement = currentElement.nextSibling;
        }
        this._addListElement(element, currentElement);
        this._breakpointElements[breakpointId] = element;
        if (enabled)
            DOMDebuggerAgent.setDOMBreakpoint(node.id, type);
    },

    _removeAllBreakpoints: function()
    {
        for (var id in this._breakpointElements) {
            var element = this._breakpointElements[id];
            this._removeBreakpoint(element._node, element._type);
        }
        this._saveBreakpoints();
    },

    _removeBreakpoint: function(node, type)
    {
        var breakpointId = this._createBreakpointId(node.id, type);
        var element = this._breakpointElements[breakpointId];
        if (!element)
            return;

        this._removeListElement(element);
        delete this._breakpointElements[breakpointId];
        if (element._checkboxElement.checked)
            DOMDebuggerAgent.removeDOMBreakpoint(node.id, type);
    },

    _contextMenu: function(node, type, event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);
        function removeBreakpoint()
        {
            this._removeBreakpoint(node, type);
            this._saveBreakpoints();
        }
        contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Remove breakpoint" : "Remove Breakpoint"), removeBreakpoint.bind(this));
        contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Remove all DOM breakpoints" : "Remove All DOM Breakpoints"), this._removeAllBreakpoints.bind(this));
        contextMenu.show();
    },

    _checkboxClicked: function(node, type, event)
    {
        if (event.target.checked)
            DOMDebuggerAgent.setDOMBreakpoint(node.id, type);
        else
            DOMDebuggerAgent.removeDOMBreakpoint(node.id, type);
        this._saveBreakpoints();
    },

    highlightBreakpoint: function(auxData)
    {
        var breakpointId = this._createBreakpointId(auxData.nodeId, auxData.type);
        var element = this._breakpointElements[breakpointId];
        if (!element)
            return;
        this.expand();
        element.addStyleClass("breakpoint-hit");
        this._highlightedElement = element;
    },

    clearBreakpointHighlight: function()
    {
        if (this._highlightedElement) {
            this._highlightedElement.removeStyleClass("breakpoint-hit");
            delete this._highlightedElement;
        }
    },

    _createBreakpointId: function(nodeId, type)
    {
        return nodeId + ":" + type;
    },

    _saveBreakpoints: function()
    {
        var breakpoints = [];
        var storedBreakpoints = WebInspector.settings.domBreakpoints.get();
        for (var i = 0; i < storedBreakpoints.length; ++i) {
            var breakpoint = storedBreakpoints[i];
            if (breakpoint.url !== this._inspectedURL)
                breakpoints.push(breakpoint);
        }
        for (var id in this._breakpointElements) {
            var element = this._breakpointElements[id];
            breakpoints.push({ url: this._inspectedURL, path: element._node.path(), type: element._type, enabled: element._checkboxElement.checked });
        }
        WebInspector.settings.domBreakpoints.set(breakpoints);
    },

    restoreBreakpoints: function()
    {
        var pathToBreakpoints = {};

        function didPushNodeByPathToFrontend(path, nodeId)
        {
            var node = WebInspector.domAgent.nodeForId(nodeId);
            if (!node)
                return;

            var breakpoints = pathToBreakpoints[path];
            for (var i = 0; i < breakpoints.length; ++i)
                this._setBreakpoint(node, breakpoints[i].type, breakpoints[i].enabled);
        }

        var breakpoints = WebInspector.settings.domBreakpoints.get();
        for (var i = 0; i < breakpoints.length; ++i) {
            var breakpoint = breakpoints[i];
            if (breakpoint.url !== this._inspectedURL)
                continue;
            var path = breakpoint.path;
            if (!pathToBreakpoints[path]) {
                pathToBreakpoints[path] = [];
                WebInspector.domAgent.pushNodeByPathToFrontend(path, didPushNodeByPathToFrontend.bind(this, path));
            }
            pathToBreakpoints[path].push(breakpoint);
        }
    },

    /**
     * @param {WebInspector.Panel} panel
     */
    createProxy: function(panel)
    {
        var proxy = new WebInspector.DOMBreakpointsSidebarPane.Proxy(this, panel);
        if (!this._proxies)
            this._proxies = [];
        this._proxies.push(proxy);
        return proxy;
    },

    onContentReady: function()
    {
        for (var i = 0; i != this._proxies.length; i++)
            this._proxies[i].onContentReady();
    },

    __proto__: WebInspector.NativeBreakpointsSidebarPane.prototype
}

/**
 * @constructor
 * @extends {WebInspector.SidebarPane}
 * @param {WebInspector.DOMBreakpointsSidebarPane} pane
 * @param {WebInspector.Panel} panel
 */
WebInspector.DOMBreakpointsSidebarPane.Proxy = function(pane, panel)
{
    WebInspector.View._assert(!pane.titleElement.firstChild, "Cannot create proxy for a sidebar pane with a toolbar");

    WebInspector.SidebarPane.call(this, pane.title());
    this.registerRequiredCSS("breakpointsList.css");

    this._wrappedPane = pane;
    this._panel = panel;

    this.bodyElement.removeSelf();
    this.bodyElement = this._wrappedPane.bodyElement;
}

WebInspector.DOMBreakpointsSidebarPane.Proxy.prototype = {
    expand: function()
    {
        this._wrappedPane.expand();
    },

    onContentReady: function()
    {
        if (!this._panel.isShowing())
            return;

        this._reattachBody();
        WebInspector.SidebarPane.prototype.onContentReady.call(this);
    },

    wasShown: function()
    {
        WebInspector.SidebarPane.prototype.wasShown.call(this);
        this._reattachBody();
    },

    _reattachBody: function()
    {
        if (this.bodyElement.parentNode !== this.element)
            this.element.appendChild(this.bodyElement);
    },

    __proto__: WebInspector.SidebarPane.prototype
}

/**
 * @type {?WebInspector.DOMBreakpointsSidebarPane}
 */
WebInspector.domBreakpointsSidebarPane = null;
