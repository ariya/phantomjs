/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.SidebarView}
 */
WebInspector.CSSNamedFlowCollectionsView = function()
{
    WebInspector.SidebarView.call(this, WebInspector.SidebarView.SidebarPosition.Start);
    this.registerRequiredCSS("cssNamedFlows.css");

    this._namedFlows = {};
    this._contentNodes = {};
    this._regionNodes = {};

    this.element.addStyleClass("css-named-flow-collections-view");
    this.element.addStyleClass("fill");

    this._statusElement = document.createElement("span");
    this._statusElement.textContent = WebInspector.UIString("CSS Named Flows");

    var sidebarHeader = this.firstElement().createChild("div", "tabbed-pane-header selected sidebar-header");
    var tab = sidebarHeader.createChild("div", "tabbed-pane-header-tab");
    tab.createChild("span", "tabbed-pane-header-tab-title").textContent = WebInspector.UIString("CSS Named Flows");

    this._sidebarContentElement = this.firstElement().createChild("div", "sidebar-content outline-disclosure");
    this._flowListElement = this._sidebarContentElement.createChild("ol");
    this._flowTree = new TreeOutline(this._flowListElement);

    this._emptyElement = document.createElement("div");
    this._emptyElement.addStyleClass("info");
    this._emptyElement.textContent = WebInspector.UIString("No CSS Named Flows");

    this._tabbedPane = new WebInspector.TabbedPane();
    this._tabbedPane.closeableTabs = true;
    this._tabbedPane.show(this.secondElement());
}

WebInspector.CSSNamedFlowCollectionsView.prototype = {
    showInDrawer: function()
    {
        WebInspector.showViewInDrawer(this._statusElement, this);
    },

    reset: function()
    {
        if (!this._document)
            return;

        WebInspector.cssModel.getNamedFlowCollectionAsync(this._document.id, this._resetNamedFlows.bind(this));
    },

    /**
     * @param {WebInspector.DOMDocument} document
     */
    _setDocument: function(document)
    {
        this._document = document;
        this.reset();
    },

    /**
     * @param {WebInspector.Event} event
     */
    _documentUpdated: function(event)
    {
        var document = /** @type {WebInspector.DOMDocument} */ (event.data);
        this._setDocument(document);
    },

    /**
     * @param {boolean} hasContent
     */
    _setSidebarHasContent: function(hasContent)
    {
        if (hasContent) {
            if (!this._emptyElement.parentNode)
                return;

            this._sidebarContentElement.removeChild(this._emptyElement);
            this._sidebarContentElement.appendChild(this._flowListElement);
        } else {
            if (!this._flowListElement.parentNode)
                return;

            this._sidebarContentElement.removeChild(this._flowListElement);
            this._sidebarContentElement.appendChild(this._emptyElement);
        }
    },

    /**
     * @param {WebInspector.NamedFlow} flow
     */
    _appendNamedFlow: function(flow)
    {
        var flowHash = this._hashNamedFlow(flow.documentNodeId, flow.name);
        var flowContainer = { flow: flow, flowHash: flowHash };

        for (var i = 0; i < flow.content.length; ++i)
            this._contentNodes[flow.content[i]] = flowHash;
        for (var i = 0; i < flow.regions.length; ++i)
            this._regionNodes[flow.regions[i].nodeId] = flowHash;

        var flowTreeItem = new WebInspector.FlowTreeElement(flowContainer);
        flowTreeItem.onselect = this._selectNamedFlowTab.bind(this, flowHash);

        flowContainer.flowTreeItem = flowTreeItem;
        this._namedFlows[flowHash] = flowContainer;

        if (!this._flowTree.children.length)
            this._setSidebarHasContent(true);
        this._flowTree.appendChild(flowTreeItem);
    },

    /**
     * @param {string} flowHash
     */
    _removeNamedFlow: function(flowHash)
    {
        var flowContainer = this._namedFlows[flowHash];

        if (this._tabbedPane._tabsById[flowHash])
            this._tabbedPane.closeTab(flowHash);
        this._flowTree.removeChild(flowContainer.flowTreeItem);

        var flow = flowContainer.flow;
        for (var i = 0; i < flow.content.length; ++i)
            delete this._contentNodes[flow.content[i]];
        for (var i = 0; i < flow.regions.length; ++i)
            delete this._regionNodes[flow.regions[i].nodeId];

        delete this._namedFlows[flowHash];

        if (!this._flowTree.children.length)
            this._setSidebarHasContent(false);
    },

    /**
     * @param {WebInspector.NamedFlow} flow
     */
    _updateNamedFlow: function(flow)
    {
        var flowHash = this._hashNamedFlow(flow.documentNodeId, flow.name);
        var flowContainer = this._namedFlows[flowHash];

        if (!flowContainer)
            return;

        var oldFlow = flowContainer.flow;
        flowContainer.flow = flow;

        for (var i = 0; i < oldFlow.content.length; ++i)
            delete this._contentNodes[oldFlow.content[i]];
        for (var i = 0; i < oldFlow.regions.length; ++i)
            delete this._regionNodes[oldFlow.regions[i].nodeId];

        for (var i = 0; i < flow.content.length; ++i)
            this._contentNodes[flow.content[i]] = flowHash;
        for (var i = 0; i < flow.regions.length; ++i)
            this._regionNodes[flow.regions[i].nodeId] = flowHash;

        flowContainer.flowTreeItem.setOverset(flow.overset);

        if (flowContainer.flowView)
            flowContainer.flowView.flow = flow;
    },

    /**
     * @param {WebInspector.NamedFlowCollection} namedFlowCollection
     */
    _resetNamedFlows: function(namedFlowCollection)
    {
        for (var flowHash in this._namedFlows)
            this._removeNamedFlow(flowHash);

        var namedFlows = namedFlowCollection.namedFlowMap;
        for (var flowName in namedFlows)
            this._appendNamedFlow(namedFlows[flowName]);

        if (!this._flowTree.children.length)
            this._setSidebarHasContent(false);
        else
            this._showNamedFlowForNode(WebInspector.panel("elements").treeOutline.selectedDOMNode());
    },

    /**
     * @param {WebInspector.Event} event
     */
    _namedFlowCreated: function(event)
    {
        // FIXME: We only have support for Named Flows in the main document.
        if (event.data.documentNodeId !== this._document.id)
            return;

        var flow = /** @type {WebInspector.NamedFlow} */ (event.data);
        this._appendNamedFlow(flow);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _namedFlowRemoved: function(event)
    {
        // FIXME: We only have support for Named Flows in the main document.
        if (event.data.documentNodeId !== this._document.id)
            return;

        this._removeNamedFlow(this._hashNamedFlow(event.data.documentNodeId, event.data.flowName));
    },

    /**
     * @param {WebInspector.Event} event
     */
    _regionLayoutUpdated: function(event)
    {
        // FIXME: We only have support for Named Flows in the main document.
        if (event.data.documentNodeId !== this._document.id)
            return;

        var flow = /** @type {WebInspector.NamedFlow} */ (event.data);
        this._updateNamedFlow(flow);
    },
    
    /**
     * @param {WebInspector.Event} event
     */
    _regionOversetChanged: function(event)
    {
        // FIXME: We only have support for Named Flows in the main document.
        if (event.data.documentNodeId !== this._document.id)
            return;
        
        var flow = /** @type {WebInspector.NamedFlow} */ (event.data);
        this._updateNamedFlow(flow);
    },

    /**
     * @param {DOMAgent.NodeId} documentNodeId
     * @param {string} flowName
     */
    _hashNamedFlow: function(documentNodeId, flowName)
    {
        return documentNodeId + "|" + flowName;
    },

    /**
     * @param {string} flowHash
     */
    _showNamedFlow: function(flowHash)
    {
        this._selectNamedFlowInSidebar(flowHash);
        this._selectNamedFlowTab(flowHash);
    },

    /**
     * @param {string} flowHash
     */
    _selectNamedFlowInSidebar: function(flowHash)
    {
        this._namedFlows[flowHash].flowTreeItem.select(true);
    },

    /**
     * @param {string} flowHash
     */
    _selectNamedFlowTab: function(flowHash)
    {
        var flowContainer = this._namedFlows[flowHash];

        if (this._tabbedPane.selectedTabId === flowHash)
            return;

        if (!this._tabbedPane.selectTab(flowHash)) {
            if (!flowContainer.flowView)
                flowContainer.flowView = new WebInspector.CSSNamedFlowView(flowContainer.flow);

            this._tabbedPane.appendTab(flowHash, flowContainer.flow.name, flowContainer.flowView);
            this._tabbedPane.selectTab(flowHash);
        }
    },

    /**
     * @param {WebInspector.Event} event
     */
    _selectedNodeChanged: function(event)
    {
        var node = /** @type {WebInspector.DOMNode} */ (event.data);
        this._showNamedFlowForNode(node);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _tabSelected: function(event)
    {
        this._selectNamedFlowInSidebar(event.data.tabId);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _tabClosed: function(event)
    {
        this._namedFlows[event.data.tabId].flowTreeItem.deselect();
    },

    /**
     * @param {?WebInspector.DOMNode} node
     */
    _showNamedFlowForNode: function(node)
    {
        if (!node)
            return;

        if (this._regionNodes[node.id]) {
            this._showNamedFlow(this._regionNodes[node.id]);
            return;
        }

        while (node) {
            if (this._contentNodes[node.id]) {
                this._showNamedFlow(this._contentNodes[node.id]);
                return;
            }

            node = node.parentNode;
        }
    },

    wasShown: function()
    {
        WebInspector.SidebarView.prototype.wasShown.call(this);

        WebInspector.domAgent.requestDocument(this._setDocument.bind(this));

        WebInspector.domAgent.addEventListener(WebInspector.DOMAgent.Events.DocumentUpdated, this._documentUpdated, this);

        WebInspector.cssModel.addEventListener(WebInspector.CSSStyleModel.Events.NamedFlowCreated, this._namedFlowCreated, this);
        WebInspector.cssModel.addEventListener(WebInspector.CSSStyleModel.Events.NamedFlowRemoved, this._namedFlowRemoved, this);
        WebInspector.cssModel.addEventListener(WebInspector.CSSStyleModel.Events.RegionLayoutUpdated, this._regionLayoutUpdated, this);
        WebInspector.cssModel.addEventListener(WebInspector.CSSStyleModel.Events.RegionOversetChanged, this._regionOversetChanged, this);

        WebInspector.panel("elements").treeOutline.addEventListener(WebInspector.ElementsTreeOutline.Events.SelectedNodeChanged, this._selectedNodeChanged, this);

        this._tabbedPane.addEventListener(WebInspector.TabbedPane.EventTypes.TabSelected, this._tabSelected, this);
        this._tabbedPane.addEventListener(WebInspector.TabbedPane.EventTypes.TabClosed, this._tabClosed, this);
    },

    willHide: function()
    {
        WebInspector.domAgent.removeEventListener(WebInspector.DOMAgent.Events.DocumentUpdated, this._documentUpdated, this);

        WebInspector.cssModel.removeEventListener(WebInspector.CSSStyleModel.Events.NamedFlowCreated, this._namedFlowCreated, this);
        WebInspector.cssModel.removeEventListener(WebInspector.CSSStyleModel.Events.NamedFlowRemoved, this._namedFlowRemoved, this);
        WebInspector.cssModel.removeEventListener(WebInspector.CSSStyleModel.Events.RegionLayoutUpdated, this._regionLayoutUpdated, this);
        WebInspector.cssModel.removeEventListener(WebInspector.CSSStyleModel.Events.RegionOversetChanged, this._regionOversetChanged, this);

        WebInspector.panel("elements").treeOutline.removeEventListener(WebInspector.ElementsTreeOutline.Events.SelectedNodeChanged, this._selectedNodeChanged, this);

        this._tabbedPane.removeEventListener(WebInspector.TabbedPane.EventTypes.TabSelected, this._tabSelected, this);
        this._tabbedPane.removeEventListener(WebInspector.TabbedPane.EventTypes.TabClosed, this._tabClosed, this);
    },

    __proto__: WebInspector.SidebarView.prototype
}

/**
 * @constructor
 * @extends {TreeElement}
 */
WebInspector.FlowTreeElement = function(flowContainer)
{
    var container = document.createElement("div");
    container.createChild("div", "selection");
    container.createChild("span", "title").createChild("span").textContent = flowContainer.flow.name;

    TreeElement.call(this, container, flowContainer, false);

    this._overset = false;
    this.setOverset(flowContainer.flow.overset);
}

WebInspector.FlowTreeElement.prototype = {
    /**
     * @param {boolean} newOverset
     */
    setOverset: function(newOverset)
    {
        if (this._overset === newOverset)
            return;

        if (newOverset) {
            this.title.addStyleClass("named-flow-overflow");
            this.tooltip = WebInspector.UIString("Overflows.");
        } else {
            this.title.removeStyleClass("named-flow-overflow");
            this.tooltip = "";
        }

        this._overset = newOverset;
    },

    __proto__: TreeElement.prototype
}
