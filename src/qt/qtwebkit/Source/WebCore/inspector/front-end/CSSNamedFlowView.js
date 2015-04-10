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
 * @extends {WebInspector.View}
 * @param {WebInspector.NamedFlow} flow
 */
WebInspector.CSSNamedFlowView = function(flow)
{
    WebInspector.View.call(this);
    this.element.addStyleClass("css-named-flow");
    this.element.addStyleClass("outline-disclosure");

    this._treeOutline = new TreeOutline(this.element.createChild("ol"), true);

    this._contentTreeItem = new TreeElement(WebInspector.UIString("content"), null, true);
    this._treeOutline.appendChild(this._contentTreeItem);

    this._regionsTreeItem = new TreeElement(WebInspector.UIString("region chain"), null, true);
    this._regionsTreeItem.expand();
    this._treeOutline.appendChild(this._regionsTreeItem);

    this._flow = flow;

    var content = flow.content;
    for (var i = 0; i < content.length; ++i)
        this._insertContentNode(content[i]);

    var regions = flow.regions;
    for (var i = 0; i < regions.length; ++i)
        this._insertRegion(regions[i]);
}

WebInspector.CSSNamedFlowView.OversetTypeMessageMap = {
    empty: "empty",
    fit: "fit",
    overset: "overset"
}

WebInspector.CSSNamedFlowView.prototype = {
    /**
     * @param {WebInspector.DOMNode=} rootDOMNode
     * @return {?WebInspector.ElementsTreeOutline}
     */
    _createFlowTreeOutline: function(rootDOMNode)
    {
        if (!rootDOMNode)
            return null;

        var treeOutline = new WebInspector.ElementsTreeOutline(false, false, true);
        treeOutline.element.addStyleClass("named-flow-element");
        treeOutline.setVisible(true);
        treeOutline.rootDOMNode = rootDOMNode;
        treeOutline.wireToDomAgent();
        WebInspector.domAgent.removeEventListener(WebInspector.DOMAgent.Events.DocumentUpdated, treeOutline._elementsTreeUpdater._documentUpdated, treeOutline._elementsTreeUpdater);

        return treeOutline;
    },

    /**
     * @param {DOMAgent.NodeId} contentNodeId
     * @param {number=} index
     */
    _insertContentNode: function(contentNodeId, index)
    {
        var treeOutline = this._createFlowTreeOutline(WebInspector.domAgent.nodeForId(contentNodeId));
        var treeItem = new TreeElement(treeOutline.element, treeOutline);

        if (index === undefined) {
            this._contentTreeItem.appendChild(treeItem);
            return;
        }

        this._contentTreeItem.insertChild(treeItem, index);
    },

    /**
     * @param {CSSAgent.Region} region
     * @param {number=} index
     */
    _insertRegion: function(region, index)
    {
        var treeOutline = this._createFlowTreeOutline(WebInspector.domAgent.nodeForId(region.nodeId));
        treeOutline.element.addStyleClass("region-" + region.regionOverset);

        var treeItem = new TreeElement(treeOutline.element, treeOutline);
        var oversetText = WebInspector.UIString(WebInspector.CSSNamedFlowView.OversetTypeMessageMap[region.regionOverset]);
        treeItem.tooltip = WebInspector.UIString("Region is %s.", oversetText);

        if (index === undefined) {
            this._regionsTreeItem.appendChild(treeItem);
            return;
        }

        this._regionsTreeItem.insertChild(treeItem, index);
    },

    get flow()
    {
        return this._flow;
    },

    set flow(newFlow)
    {
        this._update(newFlow);
    },

    /**
     * @param {TreeElement} regionTreeItem
     * @param {string} newRegionOverset
     * @param {string} oldRegionOverset
     */
    _updateRegionOverset: function(regionTreeItem, newRegionOverset, oldRegionOverset)
    {
        var element = regionTreeItem.representedObject.element;
        element.removeStyleClass("region-" + oldRegionOverset);
        element.addStyleClass("region-" + newRegionOverset);

        var oversetText = WebInspector.UIString(WebInspector.CSSNamedFlowView.OversetTypeMessageMap[newRegionOverset]);
        regionTreeItem.tooltip = WebInspector.UIString("Region is %s." , oversetText);
    },

    /**
     * @param {Array.<DOMAgent.NodeId>} oldContent
     * @param {Array.<DOMAgent.NodeId>} newContent
     */
    _mergeContentNodes: function(oldContent, newContent)
    {
        var nodeIdSet = {};
        for (var i = 0; i < newContent.length; ++i)
            nodeIdSet[newContent[i]] = true;

        var oldContentIndex = 0;
        var newContentIndex = 0;
        var contentTreeChildIndex = 0;

        while(oldContentIndex < oldContent.length || newContentIndex < newContent.length) {
            if (oldContentIndex === oldContent.length) {
                this._insertContentNode(newContent[newContentIndex]);
                ++newContentIndex;
                continue;
            }

            if (newContentIndex === newContent.length) {
                this._contentTreeItem.removeChildAtIndex(contentTreeChildIndex);
                ++oldContentIndex;
                continue;
            }

            if (oldContent[oldContentIndex] === newContent[newContentIndex]) {
                ++oldContentIndex;
                ++newContentIndex;
                ++contentTreeChildIndex;
                continue;
            }

            if (nodeIdSet[oldContent[oldContentIndex]]) {
                this._insertContentNode(newContent[newContentIndex], contentTreeChildIndex);
                ++newContentIndex;
                ++contentTreeChildIndex;
                continue;
            }

            this._contentTreeItem.removeChildAtIndex(contentTreeChildIndex);
            ++oldContentIndex;
        }
    },

    /**
     * @param {Array.<CSSAgent.Region>} oldRegions
     * @param {Array.<CSSAgent.Region>} newRegions
     */
    _mergeRegions: function(oldRegions, newRegions)
    {
        var nodeIdSet = {};
        for (var i = 0; i < newRegions.length; ++i)
            nodeIdSet[newRegions[i].nodeId] = true;

        var oldRegionsIndex = 0;
        var newRegionsIndex = 0;
        var regionsTreeChildIndex = 0;

        while(oldRegionsIndex < oldRegions.length || newRegionsIndex < newRegions.length) {
            if (oldRegionsIndex === oldRegions.length) {
                this._insertRegion(newRegions[newRegionsIndex]);
                ++newRegionsIndex;
                continue;
            }

            if (newRegionsIndex === newRegions.length) {
                this._regionsTreeItem.removeChildAtIndex(regionsTreeChildIndex);
                ++oldRegionsIndex;
                continue;
            }

            if (oldRegions[oldRegionsIndex].nodeId === newRegions[newRegionsIndex].nodeId) {
                if (oldRegions[oldRegionsIndex].regionOverset !== newRegions[newRegionsIndex].regionOverset)
                    this._updateRegionOverset(this._regionsTreeItem.children[regionsTreeChildIndex], newRegions[newRegionsIndex].regionOverset, oldRegions[oldRegionsIndex].regionOverset);
                ++oldRegionsIndex;
                ++newRegionsIndex;
                ++regionsTreeChildIndex;
                continue;
            }

            if (nodeIdSet[oldRegions[oldRegionsIndex].nodeId]) {
                this._insertRegion(newRegions[newRegionsIndex], regionsTreeChildIndex);
                ++newRegionsIndex;
                ++regionsTreeChildIndex;
                continue;
            }

            this._regionsTreeItem.removeChildAtIndex(regionsTreeChildIndex);
            ++oldRegionsIndex;
        }
    },

    /**
     * @param {WebInspector.NamedFlow} newFlow
     */
    _update: function(newFlow)
    {
        this._mergeContentNodes(this._flow.content, newFlow.content);
        this._mergeRegions(this._flow.regions, newFlow.regions);

        this._flow = newFlow;
    },

    __proto__: WebInspector.View.prototype
}
