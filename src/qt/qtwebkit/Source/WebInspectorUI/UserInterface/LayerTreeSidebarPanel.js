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

WebInspector.LayerTreeSidebarPanel = function() {
    WebInspector.DOMDetailsSidebarPanel.call(this, "layer-tree", WebInspector.UIString("Layers"), WebInspector.UIString("Layer"), "Images/NavigationItemLayers.pdf", "3");

    this._dataGridNodesByLayerId = {};

    this.element.classList.add(WebInspector.LayerTreeSidebarPanel.StyleClassName);

    WebInspector.showShadowDOMSetting.addEventListener(WebInspector.Setting.Event.Changed, this._showShadowDOMSettingChanged, this);

    window.addEventListener("resize", this._windowResized.bind(this));

    // Add a container that will host all sections as a vertical stack.
    this._container = this.element.appendChild(document.createElement("div"));
    this._container.className = "box-container";

    this._buildLayerInfoSection();
    this._buildDataGridSection();
    this._buildBottomBar();
};

WebInspector.LayerTreeSidebarPanel.StyleClassName = "layer-tree";

WebInspector.LayerTreeSidebarPanel.prototype = {
    constructor: WebInspector.LayerTreeSidebarPanel,

    // DetailsSidebarPanel Overrides.

    shown: function()
    {
        WebInspector.layerTreeManager.addEventListener(WebInspector.LayerTreeManager.Event.LayerTreeDidChange, this._layerTreeDidChange, this);

        console.assert(this.parentSidebar);

        this.needsRefresh();

        WebInspector.DOMDetailsSidebarPanel.prototype.shown.call(this);
    },

    hidden: function()
    {
        WebInspector.layerTreeManager.removeEventListener(WebInspector.LayerTreeManager.Event.LayerTreeDidChange, this._layerTreeDidChange, this);

        WebInspector.DOMDetailsSidebarPanel.prototype.hidden.call(this);
    },

    refresh: function()
    {
        if (!this.domNode)
            return;

        WebInspector.layerTreeManager.layersForNode(this.domNode, function(layerForNode, childLayers) {
            this._unfilteredChildLayers = childLayers;
            this._updateDisplayWithLayers(layerForNode, childLayers);
        }.bind(this));
    },

    // DOMDetailsSidebarPanel Overrides

    supportsDOMNode: function(nodeToInspect)
    {
        return WebInspector.layerTreeManager.supported && nodeToInspect.nodeType() === Node.ELEMENT_NODE;
    },

    // Private

    _layerTreeDidChange: function(event)
    {
        this.needsRefresh();
    },

    _showShadowDOMSettingChanged: function(event)
    {
        if (this.selected)
            this._updateDisplayWithLayers(this._layerForNode, this._unfilteredChildLayers);
    },

    _windowResized: function(event)
    {
        if (this._popover && this._popover.visible)
            this._updatePopoverForSelectedNode();
    },

    _buildLayerInfoSection: function()
    {
        var rows = this._layerInfoRows = {};
        var rowsArray = [];

        rowsArray.push(rows["Width"] = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Width")));
        rowsArray.push(rows["Height"] = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Height")));
        rowsArray.push(rows["Paints"] = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Paints")));
        rowsArray.push(rows["Memory"] = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Memory")));

        this._layerInfoGroup = new WebInspector.DetailsSectionGroup(rowsArray);

        var emptyRow = new WebInspector.DetailsSectionRow(WebInspector.UIString("No Layer Available"));
        emptyRow.showEmptyMessage();
        this._noLayerInformationGroup = new WebInspector.DetailsSectionGroup([emptyRow]);

        this._layerInfoSection = new WebInspector.DetailsSection("layer-info", WebInspector.UIString("Layer Info"), [this._noLayerInformationGroup]);

        this._container.appendChild(this._layerInfoSection.element);
    },

    _buildDataGridSection: function()
    {
        this._dataGrid = new WebInspector.LayerTreeDataGrid;

        this._dataGrid.addEventListener(WebInspector.DataGrid.Event.SortChanged, this._sortDataGrid, this);
        this._dataGrid.addEventListener(WebInspector.DataGrid.Event.SelectedNodeChanged, this._selectedDataGridNodeChanged, this);

        var element = this._dataGrid.element;
        element.addEventListener("focus", this._dataGridGainedFocus.bind(this), false);
        element.addEventListener("blur", this._dataGridLostFocus.bind(this), false);
        element.addEventListener("click", this._dataGridWasClicked.bind(this), false);

        this._childLayersRow = new WebInspector.DetailsSectionDataGridRow(null, WebInspector.UIString("No Child Layers"));
        var group = new WebInspector.DetailsSectionGroup([this._childLayersRow]);
        var section = new WebInspector.DetailsSection("layer-children", WebInspector.UIString("Child Layers"), [group], null, true);

        // Display it in the container with a class name so we can easily style it differently to other sections,
        // this specific section is meant to scale to fill the space available vertically.
        var element = this._container.appendChild(section.element);
        element.classList.add(section.identifier);
    },

    _buildBottomBar: function()
    {
        var bottomBar = this._container.appendChild(document.createElement("div"));
        bottomBar.className = "bottom-bar";

        this._layersCountLabel = bottomBar.appendChild(document.createElement("div"));
        this._layersCountLabel.className = "layers-count-label";

        this._layersMemoryLabel = bottomBar.appendChild(document.createElement("div"));
        this._layersMemoryLabel.className = "layers-memory-label";
    },
    
    _sortDataGrid: function()
    {
        var dataGrid = this._dataGrid;

        var nodes = dataGrid.children.slice();
        var sortColumnIdentifier = dataGrid.sortColumnIdentifier;
        var sortDirection = dataGrid.sortOrder === "ascending" ? 1 : -1;

        function comparator(a, b)
        {
            var item1 = a.layer[sortColumnIdentifier] || 0;
            var item2 = b.layer[sortColumnIdentifier] || 0;
            return sortDirection * (item1 - item2);
        };

        nodes.sort(comparator);

        dataGrid.setChildren(nodes);
    },

    _selectedDataGridNodeChanged: function()
    {
        if (this._dataGrid.selectedNode) {
            this._highlightSelectedNode();
            this._showPopoverForSelectedNode();
        } else {
            WebInspector.domTreeManager.hideDOMNodeHighlight();
            this._hidePopover();
        }
    },

    _dataGridGainedFocus: function(event)
    {
        this._highlightSelectedNode();
        this._showPopoverForSelectedNode();
    },

    _dataGridLostFocus: function(event)
    {
        WebInspector.domTreeManager.hideDOMNodeHighlight();
        this._hidePopover();
    },

    _dataGridWasClicked: function(event)
    {
        if (this._dataGrid.selectedNode && event.target.parentNode.classList.contains("filler"))
            this._dataGrid.selectedNode.deselect();
    },

    _highlightSelectedNode: function()
    {
        var dataGridNode = this._dataGrid.selectedNode;
        if (!dataGridNode)
            return;

        var layer = dataGridNode.layer;
        if (layer.isGeneratedContent || layer.isReflection || layer.isAnonymous)
            WebInspector.domTreeManager.highlightRect(layer.bounds, true);
        else
            WebInspector.domTreeManager.highlightDOMNode(layer.nodeId);
    },

    _updateDisplayWithLayers: function(layerForNode, childLayers)
    {
        if (!WebInspector.showShadowDOMSetting.value) {
            childLayers = childLayers.filter(function(layer) {
                return !layer.isInShadowTree;
            });
        }

        this._updateLayerInfoSection(layerForNode);
        this._updateDataGrid(layerForNode, childLayers);
        this._updateMetrics(layerForNode, childLayers);

        this._layerForNode = layerForNode;
        this._childLayers = childLayers;
    },

    _updateLayerInfoSection: function(layer)
    {
        const emDash = "\u2014";

        this._layerInfoSection.groups = layer ? [this._layerInfoGroup] : [this._noLayerInformationGroup];

        if (!layer)
            return;

        this._layerInfoRows["Memory"].value = Number.bytesToString(layer.memory);
        this._layerInfoRows["Width"].value = layer.compositedBounds.width + "px";
        this._layerInfoRows["Height"].value = layer.compositedBounds.height + "px";
        this._layerInfoRows["Paints"].value = layer.paintCount + "";
    },

    _updateDataGrid: function(layerForNode, childLayers)
    {
        var dataGrid = this._dataGrid;

        var mutations = WebInspector.layerTreeManager.layerTreeMutations(this._childLayers, childLayers);

        mutations.removals.forEach(function(layer) {
            var node = this._dataGridNodesByLayerId[layer.layerId];
            if (node) {
                dataGrid.removeChild(node);
                delete this._dataGridNodesByLayerId[layer.layerId];
            }
        }.bind(this));

        mutations.additions.forEach(function(layer) {
            var node = this._dataGridNodeForLayer(layer);
            if (node)
                dataGrid.appendChild(node);
        }.bind(this));

        mutations.preserved.forEach(function(layer) {
            var node = this._dataGridNodesByLayerId[layer.layerId];
            if (node)
                node.layer = layer;
        }.bind(this));

        this._sortDataGrid();

        this._childLayersRow.dataGrid = !isEmptyObject(childLayers) ? this._dataGrid : null;
    },
    
    _dataGridNodeForLayer: function(layer)
    {
        var node = new WebInspector.LayerTreeDataGridNode(layer);

        this._dataGridNodesByLayerId[layer.layerId] = node;

        return node;
    },
    
    _updateMetrics: function(layerForNode, childLayers)
    {
        var layerCount = 0;
        var totalMemory = 0;

        if (layerForNode) {
            layerCount++;
            totalMemory += layerForNode.memory || 0;
        }

        childLayers.forEach(function(layer) {
            layerCount++;
            totalMemory += layer.memory || 0;
        });

        this._layersCountLabel.textContent = WebInspector.UIString("Layer count: %d").format(layerCount);
        this._layersMemoryLabel.textContent = WebInspector.UIString("Memory: %s").format(Number.bytesToString(totalMemory));
    },

    _showPopoverForSelectedNode: function()
    {
        var dataGridNode = this._dataGrid.selectedNode;
        if (!dataGridNode)
            return;

        this._contentForPopover(dataGridNode.layer, function(content) {
            if (dataGridNode === this._dataGrid.selectedNode)
                this._updatePopoverForSelectedNode(content);
        }.bind(this));
    },

    _updatePopoverForSelectedNode: function(content)
    {
        var dataGridNode = this._dataGrid.selectedNode;
        if (!dataGridNode)
            return;

        var popover = this._popover;
        if (!popover)
            popover = this._popover = new WebInspector.Popover;

        var targetFrame = WebInspector.Rect.rectFromClientRect(dataGridNode.element.getBoundingClientRect());
        const padding = 2;
        targetFrame.origin.x -= padding;
        targetFrame.origin.y -= padding;
        targetFrame.size.width += padding * 2;
        targetFrame.size.height += padding * 2;

        if (content)
            popover.content = content;

        popover.present(targetFrame, [WebInspector.RectEdge.MIN_X]);
    },

    _hidePopover: function()
    {
        if (this._popover)
            this._popover.dismiss();
    },

    _contentForPopover: function(layer, callback)
    {
        var content = document.createElement("div");
        content.className = "layer-tree-popover";
        
        content.appendChild(document.createElement("p")).textContent = WebInspector.UIString("Reasons for compositing:");

        var list = content.appendChild(document.createElement("ul"));

        WebInspector.layerTreeManager.reasonsForCompositingLayer(layer, function(compositingReasons) {
            if (isEmptyObject(compositingReasons)) {
                callback(content);
                return;
            }

            this._populateListOfCompositingReasons(list, compositingReasons);

            callback(content);
        }.bind(this));

        return content;
    },

    _populateListOfCompositingReasons: function(list, compositingReasons)
    {
        function addReason(reason)
        {
            list.appendChild(document.createElement("li")).textContent = reason;
        }

        if (compositingReasons.transform3D)
            addReason(WebInspector.UIString("Element has a 3D transform"));
        if (compositingReasons.video)
            addReason(WebInspector.UIString("Element is <video>"));
        if (compositingReasons.canvas)
            addReason(WebInspector.UIString("Element is <canvas>"));
        if (compositingReasons.plugin)
            addReason(WebInspector.UIString("Element is a plug-in"));
        if (compositingReasons.iFrame)
            addReason(WebInspector.UIString("Element is <iframe>"));
        if (compositingReasons.backfaceVisibilityHidden)
            addReason(WebInspector.UIString("Element has “backface-visibility: hidden” style"));
        if (compositingReasons.clipsCompositingDescendants)
            addReason(WebInspector.UIString("Element clips compositing descendants"));
        if (compositingReasons.animation)
            addReason(WebInspector.UIString("Element is animated"));
        if (compositingReasons.filters)
            addReason(WebInspector.UIString("Element has CSS filters applied"));
        if (compositingReasons.positionFixed)
            addReason(WebInspector.UIString("Element has “position: fixed” style"));
        if (compositingReasons.positionSticky)
            addReason(WebInspector.UIString("Element has “position: sticky” style"));
        if (compositingReasons.overflowScrollingTouch)
            addReason(WebInspector.UIString("Element has “-webkit-overflow-scrolling: touch” style"));
        if (compositingReasons.stacking)
            addReason(WebInspector.UIString("Element establishes a stacking context"));
        if (compositingReasons.overlap)
            addReason(WebInspector.UIString("Element overlaps other compositing element"));
        if (compositingReasons.negativeZIndexChildren)
            addReason(WebInspector.UIString("Element has children with a negative z-index"));
        if (compositingReasons.transformWithCompositedDescendants)
            addReason(WebInspector.UIString("Element has a 2D transform and composited descendants"));
        if (compositingReasons.opacityWithCompositedDescendants)
            addReason(WebInspector.UIString("Element has opacity applied and composited descendants"));
        if (compositingReasons.maskWithCompositedDescendants)
            addReason(WebInspector.UIString("Element is masked and composited descendants"));
        if (compositingReasons.reflectionWithCompositedDescendants)
            addReason(WebInspector.UIString("Element has a reflection and composited descendants"));
        if (compositingReasons.filterWithCompositedDescendants)
            addReason(WebInspector.UIString("Element has CSS filters applied and composited descendants"));
        if (compositingReasons.blendingWithCompositedDescendants)
            addReason(WebInspector.UIString("Element has CSS blending applied and composited descendants"));
        if (compositingReasons.perspective)
            addReason(WebInspector.UIString("Element has perspective applied"));
        if (compositingReasons.preserve3D)
            addReason(WebInspector.UIString("Element has “transform-style: preserve-3d” style"));
        if (compositingReasons.root)
            addReason(WebInspector.UIString("Element is the root element"));
    }
};

WebInspector.LayerTreeSidebarPanel.prototype.__proto__ = WebInspector.DOMDetailsSidebarPanel.prototype;
