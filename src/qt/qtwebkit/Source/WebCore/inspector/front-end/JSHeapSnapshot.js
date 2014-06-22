/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 * @extends {WebInspector.HeapSnapshot}
 */
WebInspector.JSHeapSnapshot = function(profile)
{
    this._nodeFlags = { // bit flags
        canBeQueried: 1,
        detachedDOMTreeNode: 2,
        pageObject: 4, // The idea is to track separately the objects owned by the page and the objects owned by debugger.

        visitedMarkerMask: 0x0ffff, // bits: 0,1111,1111,1111,1111
        visitedMarker:     0x10000  // bits: 1,0000,0000,0000,0000
    };
    WebInspector.HeapSnapshot.call(this, profile);
}

WebInspector.JSHeapSnapshot.prototype = {
    createNode: function(nodeIndex)
    {
        return new WebInspector.JSHeapSnapshotNode(this, nodeIndex);
    },

    createEdge: function(edges, edgeIndex)
    {
        return new WebInspector.JSHeapSnapshotEdge(this, edges, edgeIndex);
    },

    createRetainingEdge: function(retainedNodeIndex, retainerIndex)
    {
        return new WebInspector.JSHeapSnapshotRetainerEdge(this, retainedNodeIndex, retainerIndex);
    },

    classNodesFilter: function()
    {
        function filter(node)
        {
            return node.isUserObject();
        }
        return filter;
    },

    containmentEdgesFilter: function(showHiddenData)
    {
        function filter(edge) {
            if (edge.isInvisible())
                return false;
            if (showHiddenData)
                return true;
            return !edge.isHidden() && !edge.node().isHidden();
        }
        return filter;
    },

    retainingEdgesFilter: function(showHiddenData)
    {
        var containmentEdgesFilter = this.containmentEdgesFilter(showHiddenData);
        function filter(edge) {
            if (!containmentEdgesFilter(edge))
                return false;
            return edge.node().id() !== 1 && !edge.node().isSynthetic() && !edge.isWeak();
        }
        return filter;
    },

    dispose: function()
    {
        WebInspector.HeapSnapshot.prototype.dispose.call(this);
        delete this._flags;
    },

    _markInvisibleEdges: function()
    {
        // Mark hidden edges of global objects as invisible.
        // FIXME: This is a temporary measure. Normally, we should
        // really hide all hidden nodes.
        for (var iter = this.rootNode().edges(); iter.hasNext(); iter.next()) {
            var edge = iter.edge;
            if (!edge.isShortcut())
                continue;
            var node = edge.node();
            var propNames = {};
            for (var innerIter = node.edges(); innerIter.hasNext(); innerIter.next()) {
                var globalObjEdge = innerIter.edge;
                if (globalObjEdge.isShortcut())
                    propNames[globalObjEdge._nameOrIndex()] = true;
            }
            for (innerIter.rewind(); innerIter.hasNext(); innerIter.next()) {
                var globalObjEdge = innerIter.edge;
                if (!globalObjEdge.isShortcut()
                    && globalObjEdge.node().isHidden()
                    && globalObjEdge._hasStringName()
                    && (globalObjEdge._nameOrIndex() in propNames))
                    this._containmentEdges[globalObjEdge._edges._start + globalObjEdge.edgeIndex + this._edgeTypeOffset] = this._edgeInvisibleType;
            }
        }
    },

    _calculateFlags: function()
    {
        this._flags = new Uint32Array(this.nodeCount);
        this._markDetachedDOMTreeNodes();
        this._markQueriableHeapObjects();
        this._markPageOwnedNodes();
    },

    distanceForUserRoot: function(node)
    {
        if (node.isWindow())
            return 1;
        if (node.isDocumentDOMTreesRoot())
            return 0;
        return -1;
    },

    userObjectsMapAndFlag: function()
    {
        return {
            map: this._flags,
            flag: this._nodeFlags.pageObject
        };
    },

    _flagsOfNode: function(node)
    {
        return this._flags[node.nodeIndex / this._nodeFieldCount];
    },

    _markDetachedDOMTreeNodes: function()
    {
        var flag = this._nodeFlags.detachedDOMTreeNode;
        var detachedDOMTreesRoot;
        for (var iter = this.rootNode().edges(); iter.hasNext(); iter.next()) {
            var node = iter.edge.node();
            if (node.name() === "(Detached DOM trees)") {
                detachedDOMTreesRoot = node;
                break;
            }
        }

        if (!detachedDOMTreesRoot)
            return;

        var detachedDOMTreeRE = /^Detached DOM tree/;
        for (var iter = detachedDOMTreesRoot.edges(); iter.hasNext(); iter.next()) {
            var node = iter.edge.node();
            if (detachedDOMTreeRE.test(node.className())) {
                for (var edgesIter = node.edges(); edgesIter.hasNext(); edgesIter.next())
                    this._flags[edgesIter.edge.node().nodeIndex / this._nodeFieldCount] |= flag;
            }
        }
    },

    _markQueriableHeapObjects: function()
    {
        // Allow runtime properties query for objects accessible from Window objects
        // via regular properties, and for DOM wrappers. Trying to access random objects
        // can cause a crash due to insonsistent state of internal properties of wrappers.
        var flag = this._nodeFlags.canBeQueried;
        var hiddenEdgeType = this._edgeHiddenType;
        var internalEdgeType = this._edgeInternalType;
        var invisibleEdgeType = this._edgeInvisibleType;
        var weakEdgeType = this._edgeWeakType;
        var edgeToNodeOffset = this._edgeToNodeOffset;
        var edgeTypeOffset = this._edgeTypeOffset;
        var edgeFieldsCount = this._edgeFieldsCount;
        var containmentEdges = this._containmentEdges;
        var nodes = this._nodes;
        var nodeCount = this.nodeCount;
        var nodeFieldCount = this._nodeFieldCount;
        var firstEdgeIndexes = this._firstEdgeIndexes;

        var flags = this._flags;
        var list = [];

        for (var iter = this.rootNode().edges(); iter.hasNext(); iter.next()) {
            if (iter.edge.node().isWindow())
                list.push(iter.edge.node().nodeIndex / nodeFieldCount);
        }

        while (list.length) {
            var nodeOrdinal = list.pop();
            if (flags[nodeOrdinal] & flag)
                continue;
            flags[nodeOrdinal] |= flag;
            var beginEdgeIndex = firstEdgeIndexes[nodeOrdinal];
            var endEdgeIndex = firstEdgeIndexes[nodeOrdinal + 1];
            for (var edgeIndex = beginEdgeIndex; edgeIndex < endEdgeIndex; edgeIndex += edgeFieldsCount) {
                var childNodeIndex = containmentEdges[edgeIndex + edgeToNodeOffset];
                var childNodeOrdinal = childNodeIndex / nodeFieldCount;
                if (flags[childNodeOrdinal] & flag)
                    continue;
                var type = containmentEdges[edgeIndex + edgeTypeOffset];
                if (type === hiddenEdgeType || type === invisibleEdgeType || type === internalEdgeType || type === weakEdgeType)
                    continue;
                list.push(childNodeOrdinal);
            }
        }
    },

    _markPageOwnedNodes: function()
    {
        var edgeShortcutType = this._edgeShortcutType;
        var edgeElementType = this._edgeElementType;
        var edgeToNodeOffset = this._edgeToNodeOffset;
        var edgeTypeOffset = this._edgeTypeOffset;
        var edgeFieldsCount = this._edgeFieldsCount;
        var edgeWeakType = this._edgeWeakType;
        var firstEdgeIndexes = this._firstEdgeIndexes;
        var containmentEdges = this._containmentEdges;
        var containmentEdgesLength = containmentEdges.length;
        var nodes = this._nodes;
        var nodeFieldCount = this._nodeFieldCount;
        var nodesCount = this.nodeCount;

        var flags = this._flags;
        var flag = this._nodeFlags.pageObject;
        var visitedMarker = this._nodeFlags.visitedMarker;
        var visitedMarkerMask = this._nodeFlags.visitedMarkerMask;
        var markerAndFlag = visitedMarker | flag;

        var nodesToVisit = new Uint32Array(nodesCount);
        var nodesToVisitLength = 0;

        var rootNodeOrdinal = this._rootNodeIndex / nodeFieldCount;
        var node = this.rootNode();
        for (var edgeIndex = firstEdgeIndexes[rootNodeOrdinal], endEdgeIndex = firstEdgeIndexes[rootNodeOrdinal + 1];
             edgeIndex < endEdgeIndex;
             edgeIndex += edgeFieldsCount) {
            var edgeType = containmentEdges[edgeIndex + edgeTypeOffset];
            var nodeIndex = containmentEdges[edgeIndex + edgeToNodeOffset];
            if (edgeType === edgeElementType) {
                node.nodeIndex = nodeIndex;
                if (!node.isDocumentDOMTreesRoot())
                    continue;
            } else if (edgeType !== edgeShortcutType)
                continue;
            var nodeOrdinal = nodeIndex / nodeFieldCount;
            nodesToVisit[nodesToVisitLength++] = nodeOrdinal;
            flags[nodeOrdinal] |= visitedMarker;
        }

        while (nodesToVisitLength) {
            var nodeOrdinal = nodesToVisit[--nodesToVisitLength];
            flags[nodeOrdinal] |= flag;
            flags[nodeOrdinal] &= visitedMarkerMask;
            var beginEdgeIndex = firstEdgeIndexes[nodeOrdinal];
            var endEdgeIndex = firstEdgeIndexes[nodeOrdinal + 1];
            for (var edgeIndex = beginEdgeIndex; edgeIndex < endEdgeIndex; edgeIndex += edgeFieldsCount) {
                var childNodeIndex = containmentEdges[edgeIndex + edgeToNodeOffset];
                var childNodeOrdinal = childNodeIndex / nodeFieldCount;
                if (flags[childNodeOrdinal] & markerAndFlag)
                    continue;
                var type = containmentEdges[edgeIndex + edgeTypeOffset];
                if (type === edgeWeakType)
                    continue;
                nodesToVisit[nodesToVisitLength++] = childNodeOrdinal;
                flags[childNodeOrdinal] |= visitedMarker;
            }
        }
    },

    __proto__: WebInspector.HeapSnapshot.prototype
};

/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotNode}
 * @param {WebInspector.JSHeapSnapshot} snapshot
 * @param {number=} nodeIndex
 */
WebInspector.JSHeapSnapshotNode = function(snapshot, nodeIndex)
{
    WebInspector.HeapSnapshotNode.call(this, snapshot, nodeIndex)
}

WebInspector.JSHeapSnapshotNode.prototype = {
    canBeQueried: function()
    {
        var flags = this._snapshot._flagsOfNode(this);
        return !!(flags & this._snapshot._nodeFlags.canBeQueried);
    },

    isUserObject: function()
    {
        var flags = this._snapshot._flagsOfNode(this);
        return !!(flags & this._snapshot._nodeFlags.pageObject);
    },

    className: function()
    {
        var type = this.type();
        switch (type) {
        case "hidden":
            return "(system)";
        case "object":
        case "native":
            return this.name();
        case "code":
            return "(compiled code)";
        default:
            return "(" + type + ")";
        }
    },

    classIndex: function()
    {
        var snapshot = this._snapshot;
        var nodes = snapshot._nodes;
        var type = nodes[this.nodeIndex + snapshot._nodeTypeOffset];;
        if (type === snapshot._nodeObjectType || type === snapshot._nodeNativeType)
            return nodes[this.nodeIndex + snapshot._nodeNameOffset];
        return -1 - type;
    },

    id: function()
    {
        var snapshot = this._snapshot;
        return snapshot._nodes[this.nodeIndex + snapshot._nodeIdOffset];
    },

    isHidden: function()
    {
        return this._type() === this._snapshot._nodeHiddenType;
    },

    isSynthetic: function()
    {
        return this._type() === this._snapshot._nodeSyntheticType;
    },

    isWindow: function()
    {
        const windowRE = /^Window/;
        return windowRE.test(this.name());
    },

    isDocumentDOMTreesRoot: function()
    {
        return this.isSynthetic() && this.name() === "(Document DOM trees)";
    },

    serialize: function()
    {
        var result = WebInspector.HeapSnapshotNode.prototype.serialize.call(this);
        var flags = this._snapshot._flagsOfNode(this);
        if (flags & this._snapshot._nodeFlags.canBeQueried)
            result.canBeQueried = true;
        if (flags & this._snapshot._nodeFlags.detachedDOMTreeNode)
            result.detachedDOMTreeNode = true;
        return result;
    },

    __proto__: WebInspector.HeapSnapshotNode.prototype
};

/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotEdge}
 * @param {WebInspector.JSHeapSnapshot} snapshot
 * @param {Array.<number>} edges
 * @param {number=} edgeIndex
 */
WebInspector.JSHeapSnapshotEdge = function(snapshot, edges, edgeIndex)
{
    WebInspector.HeapSnapshotEdge.call(this, snapshot, edges, edgeIndex);
}

WebInspector.JSHeapSnapshotEdge.prototype = {
    clone: function()
    {
        return new WebInspector.JSHeapSnapshotEdge(this._snapshot, this._edges, this.edgeIndex);
    },

    hasStringName: function()
    {
        if (!this.isShortcut())
            return this._hasStringName();
        return isNaN(parseInt(this._name(), 10));
    },

    isElement: function()
    {
        return this._type() === this._snapshot._edgeElementType;
    },

    isHidden: function()
    {
        return this._type() === this._snapshot._edgeHiddenType;
    },

    isWeak: function()
    {
        return this._type() === this._snapshot._edgeWeakType;
    },

    isInternal: function()
    {
        return this._type() === this._snapshot._edgeInternalType;
    },

    isInvisible: function()
    {
        return this._type() === this._snapshot._edgeInvisibleType;
    },

    isShortcut: function()
    {
        return this._type() === this._snapshot._edgeShortcutType;
    },

    name: function()
    {
        if (!this.isShortcut())
            return this._name();
        var numName = parseInt(this._name(), 10);
        return isNaN(numName) ? this._name() : numName;
    },

    toString: function()
    {
        var name = this.name();
        switch (this.type()) {
        case "context": return "->" + name;
        case "element": return "[" + name + "]";
        case "weak": return "[[" + name + "]]";
        case "property":
            return name.indexOf(" ") === -1 ? "." + name : "[\"" + name + "\"]";
        case "shortcut":
            if (typeof name === "string")
                return name.indexOf(" ") === -1 ? "." + name : "[\"" + name + "\"]";
            else
                return "[" + name + "]";
        case "internal":
        case "hidden":
        case "invisible":
            return "{" + name + "}";
        };
        return "?" + name + "?";
    },

    _hasStringName: function()
    {
        return !this.isElement() && !this.isHidden() && !this.isWeak();
    },

    _name: function()
    {
        return this._hasStringName() ? this._snapshot._strings[this._nameOrIndex()] : this._nameOrIndex();
    },

    _nameOrIndex: function()
    {
        return this._edges.item(this.edgeIndex + this._snapshot._edgeNameOffset);
    },

    _type: function()
    {
        return this._edges.item(this.edgeIndex + this._snapshot._edgeTypeOffset);
    },

    __proto__: WebInspector.HeapSnapshotEdge.prototype
};


/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotRetainerEdge}
 * @param {WebInspector.JSHeapSnapshot} snapshot
 */
WebInspector.JSHeapSnapshotRetainerEdge = function(snapshot, retainedNodeIndex, retainerIndex)
{
    WebInspector.HeapSnapshotRetainerEdge.call(this, snapshot, retainedNodeIndex, retainerIndex);
}

WebInspector.JSHeapSnapshotRetainerEdge.prototype = {
    clone: function()
    {
        return new WebInspector.JSHeapSnapshotRetainerEdge(this._snapshot, this._retainedNodeIndex, this.retainerIndex());
    },

    isHidden: function()
    {
        return this._edge().isHidden();
    },

    isInternal: function()
    {
        return this._edge().isInternal();
    },

    isInvisible: function()
    {
        return this._edge().isInvisible();
    },

    isShortcut: function()
    {
        return this._edge().isShortcut();
    },

    isWeak: function()
    {
        return this._edge().isWeak();
    },

    __proto__: WebInspector.HeapSnapshotRetainerEdge.prototype
}

