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
 */
WebInspector.HeapSnapshotArraySlice = function(array, start, end)
{
    this._array = array;
    this._start = start;
    this.length = end - start;
}

WebInspector.HeapSnapshotArraySlice.prototype = {
    item: function(index)
    {
        return this._array[this._start + index];
    },

    slice: function(start, end)
    {
        if (typeof end === "undefined")
            end = this.length;
        return this._array.subarray(this._start + start, this._start + end);
    }
}

/**
 * @constructor
 * @param {number=} edgeIndex
 */
WebInspector.HeapSnapshotEdge = function(snapshot, edges, edgeIndex)
{
    this._snapshot = snapshot;
    this._edges = edges;
    this.edgeIndex = edgeIndex || 0;
}

WebInspector.HeapSnapshotEdge.prototype = {
    clone: function()
    {
        return new WebInspector.HeapSnapshotEdge(this._snapshot, this._edges, this.edgeIndex);
    },

    hasStringName: function()
    {
        throw new Error("Not implemented");
    },

    name: function()
    {
        throw new Error("Not implemented");
    },

    node: function()
    {
        return this._snapshot.createNode(this.nodeIndex());
    },

    nodeIndex: function()
    {
        return this._edges.item(this.edgeIndex + this._snapshot._edgeToNodeOffset);
    },

    rawEdges: function()
    {
        return this._edges;
    },

    toString: function()
    {
        return "HeapSnapshotEdge: " + this.name();
    },

    type: function()
    {
        return this._snapshot._edgeTypes[this._type()];
    },

    serialize: function()
    {
        var node = this.node();
        return {
            name: this.name(),
            node: node.serialize(),
            nodeIndex: this.nodeIndex(),
            type: this.type(),
            distance: node.distance()
        };
    },

    _type: function()
    {
        return this._edges.item(this.edgeIndex + this._snapshot._edgeTypeOffset);
    }
};

/**
 * @constructor
 */
WebInspector.HeapSnapshotEdgeIterator = function(edge)
{
    this.edge = edge;
}

WebInspector.HeapSnapshotEdgeIterator.prototype = {
    rewind: function()
    {
        this.edge.edgeIndex = 0;
    },

    hasNext: function()
    {
        return this.edge.edgeIndex < this.edge._edges.length;
    },

    index: function()
    {
        return this.edge.edgeIndex;
    },

    setIndex: function(newIndex)
    {
        this.edge.edgeIndex = newIndex;
    },

    item: function()
    {
        return this.edge;
    },

    next: function()
    {
        this.edge.edgeIndex += this.edge._snapshot._edgeFieldsCount;
    }
};

/**
 * @constructor
 */
WebInspector.HeapSnapshotRetainerEdge = function(snapshot, retainedNodeIndex, retainerIndex)
{
    this._snapshot = snapshot;
    this._retainedNodeIndex = retainedNodeIndex;

    var retainedNodeOrdinal = retainedNodeIndex / snapshot._nodeFieldCount;
    this._firstRetainer = snapshot._firstRetainerIndex[retainedNodeOrdinal];
    this._retainersCount = snapshot._firstRetainerIndex[retainedNodeOrdinal + 1] - this._firstRetainer;

    this.setRetainerIndex(retainerIndex);
}

WebInspector.HeapSnapshotRetainerEdge.prototype = {
    clone: function()
    {
        return new WebInspector.HeapSnapshotRetainerEdge(this._snapshot, this._retainedNodeIndex, this.retainerIndex());
    },

    hasStringName: function()
    {
        return this._edge().hasStringName();
    },

    name: function()
    {
        return this._edge().name();
    },

    node: function()
    {
        return this._node();
    },

    nodeIndex: function()
    {
        return this._nodeIndex;
    },

    retainerIndex: function()
    {
        return this._retainerIndex;
    },

    setRetainerIndex: function(newIndex)
    {
        if (newIndex !== this._retainerIndex) {
            this._retainerIndex = newIndex;
            this.edgeIndex = newIndex;
        }
    },

    set edgeIndex(edgeIndex)
    {
        var retainerIndex = this._firstRetainer + edgeIndex;
        this._globalEdgeIndex = this._snapshot._retainingEdges[retainerIndex];
        this._nodeIndex = this._snapshot._retainingNodes[retainerIndex];
        delete this._edgeInstance;
        delete this._nodeInstance;
    },

    _node: function()
    {
        if (!this._nodeInstance)
            this._nodeInstance = this._snapshot.createNode(this._nodeIndex);
        return this._nodeInstance;
    },

    _edge: function()
    {
        if (!this._edgeInstance) {
            var edgeIndex = this._globalEdgeIndex - this._node()._edgeIndexesStart();
            this._edgeInstance = this._snapshot.createEdge(this._node().rawEdges(), edgeIndex);
        }
        return this._edgeInstance;
    },

    toString: function()
    {
        return this._edge().toString();
    },

    serialize: function()
    {
        var node = this.node();
        return {
            name: this.name(),
            node: node.serialize(),
            nodeIndex: this.nodeIndex(),
            type: this.type(),
            distance: node.distance()
        };
    },

    type: function()
    {
        return this._edge().type();
    }
}

/**
 * @constructor
 */
WebInspector.HeapSnapshotRetainerEdgeIterator = function(retainer)
{
    this.retainer = retainer;
}

WebInspector.HeapSnapshotRetainerEdgeIterator.prototype = {
    rewind: function()
    {
        this.retainer.setRetainerIndex(0);
    },

    hasNext: function()
    {
        return this.retainer.retainerIndex() < this.retainer._retainersCount;
    },

    index: function()
    {
        return this.retainer.retainerIndex();
    },

    setIndex: function(newIndex)
    {
        this.retainer.setRetainerIndex(newIndex);
    },

    item: function()
    {
        return this.retainer;
    },

    next: function()
    {
        this.retainer.setRetainerIndex(this.retainer.retainerIndex() + 1);
    }
};

/**
 * @constructor
 * @param {number=} nodeIndex
 */
WebInspector.HeapSnapshotNode = function(snapshot, nodeIndex)
{
    this._snapshot = snapshot;
    this._firstNodeIndex = nodeIndex;
    this.nodeIndex = nodeIndex;
}

WebInspector.HeapSnapshotNode.prototype = {
    distance: function()
    {
        return this._snapshot._nodeDistances[this.nodeIndex / this._snapshot._nodeFieldCount];
    },

    className: function()
    {
        throw new Error("Not implemented");
    },

    classIndex: function()
    {
        throw new Error("Not implemented");
    },

    dominatorIndex: function()
    {
        var nodeFieldCount = this._snapshot._nodeFieldCount;
        return this._snapshot._dominatorsTree[this.nodeIndex / this._snapshot._nodeFieldCount] * nodeFieldCount;
    },

    edges: function()
    {
        return new WebInspector.HeapSnapshotEdgeIterator(this._snapshot.createEdge(this.rawEdges(), 0));
    },

    edgesCount: function()
    {
        return (this._edgeIndexesEnd() - this._edgeIndexesStart()) / this._snapshot._edgeFieldsCount;
    },

    id: function()
    {
        throw new Error("Not implemented");
    },

    isRoot: function()
    {
        return this.nodeIndex === this._snapshot._rootNodeIndex;
    },

    name: function()
    {
        return this._snapshot._strings[this._name()];
    },

    rawEdges: function()
    {
        return new WebInspector.HeapSnapshotArraySlice(this._snapshot._containmentEdges, this._edgeIndexesStart(), this._edgeIndexesEnd());
    },

    retainedSize: function()
    {
        var snapshot = this._snapshot;
        return snapshot._nodes[this.nodeIndex + snapshot._nodeRetainedSizeOffset];
    },

    retainers: function()
    {
        return new WebInspector.HeapSnapshotRetainerEdgeIterator(this._snapshot.createRetainingEdge(this.nodeIndex, 0));
    },

    selfSize: function()
    {
        var snapshot = this._snapshot;
        return snapshot._nodes[this.nodeIndex + snapshot._nodeSelfSizeOffset];
    },

    type: function()
    {
        return this._snapshot._nodeTypes[this._type()];
    },

    serialize: function()
    {
        return {
            id: this.id(),
            name: this.name(),
            distance: this.distance(),
            nodeIndex: this.nodeIndex,
            retainedSize: this.retainedSize(),
            selfSize: this.selfSize(),
            type: this.type(),
        };
    },

    _name: function()
    {
        var snapshot = this._snapshot;
        return snapshot._nodes[this.nodeIndex + snapshot._nodeNameOffset];
    },

    _edgeIndexesStart: function()
    {
        return this._snapshot._firstEdgeIndexes[this._ordinal()];
    },

    _edgeIndexesEnd: function()
    {
        return this._snapshot._firstEdgeIndexes[this._ordinal() + 1];
    },

    _ordinal: function()
    {
        return this.nodeIndex / this._snapshot._nodeFieldCount;
    },

    _nextNodeIndex: function()
    {
        return this.nodeIndex + this._snapshot._nodeFieldCount;
    },

    _type: function()
    {
        var snapshot = this._snapshot;
        return snapshot._nodes[this.nodeIndex + snapshot._nodeTypeOffset];
    }
};

/**
 * @constructor
 */
WebInspector.HeapSnapshotNodeIterator = function(node)
{
    this.node = node;
    this._nodesLength = node._snapshot._nodes.length;
}

WebInspector.HeapSnapshotNodeIterator.prototype = {
    rewind: function()
    {
        this.node.nodeIndex = this.node._firstNodeIndex;
    },

    hasNext: function()
    {
        return this.node.nodeIndex < this._nodesLength;
    },

    index: function()
    {
        return this.node.nodeIndex;
    },

    setIndex: function(newIndex)
    {
        this.node.nodeIndex = newIndex;
    },

    item: function()
    {
        return this.node;
    },

    next: function()
    {
        this.node.nodeIndex = this.node._nextNodeIndex();
    }
}

/**
 * @constructor
 */
WebInspector.HeapSnapshot = function(profile)
{
    this.uid = profile.snapshot.uid;
    this._nodes = profile.nodes;
    this._containmentEdges = profile.edges;
    /** @type{HeapSnapshotMetainfo} */
    this._metaNode = profile.snapshot.meta;
    this._strings = profile.strings;

    this._rootNodeIndex = 0;
    if (profile.snapshot.root_index)
        this._rootNodeIndex = profile.snapshot.root_index;

    this._snapshotDiffs = {};
    this._aggregatesForDiff = null;

    this._init();
}

/**
 * @constructor
 */
function HeapSnapshotMetainfo()
{
    // New format.
    this.node_fields = [];
    this.node_types = [];
    this.edge_fields = [];
    this.edge_types = [];
    this.type_strings = {};

    // Old format.
    this.fields = [];
    this.types = [];
}

/**
 * @constructor
 */
function HeapSnapshotHeader()
{
    // New format.
    this.title = "";
    this.uid = 0;
    this.meta = new HeapSnapshotMetainfo();
    this.node_count = 0;
    this.edge_count = 0;
}

WebInspector.HeapSnapshot.prototype = {
    _init: function()
    {
        var meta = this._metaNode;

        this._nodeTypeOffset = meta.node_fields.indexOf("type");
        this._nodeNameOffset = meta.node_fields.indexOf("name");
        this._nodeIdOffset = meta.node_fields.indexOf("id");
        this._nodeSelfSizeOffset = meta.node_fields.indexOf("self_size");
        this._nodeEdgeCountOffset = meta.node_fields.indexOf("edge_count");
        this._nodeFieldCount = meta.node_fields.length;

        this._nodeTypes = meta.node_types[this._nodeTypeOffset];
        this._nodeHiddenType = this._nodeTypes.indexOf("hidden");
        this._nodeObjectType = this._nodeTypes.indexOf("object");
        this._nodeNativeType = this._nodeTypes.indexOf("native");
        this._nodeCodeType = this._nodeTypes.indexOf("code");
        this._nodeSyntheticType = this._nodeTypes.indexOf("synthetic");

        this._edgeFieldsCount = meta.edge_fields.length;
        this._edgeTypeOffset = meta.edge_fields.indexOf("type");
        this._edgeNameOffset = meta.edge_fields.indexOf("name_or_index");
        this._edgeToNodeOffset = meta.edge_fields.indexOf("to_node");

        this._edgeTypes = meta.edge_types[this._edgeTypeOffset];
        this._edgeTypes.push("invisible");
        this._edgeElementType = this._edgeTypes.indexOf("element");
        this._edgeHiddenType = this._edgeTypes.indexOf("hidden");
        this._edgeInternalType = this._edgeTypes.indexOf("internal");
        this._edgeShortcutType = this._edgeTypes.indexOf("shortcut");
        this._edgeWeakType = this._edgeTypes.indexOf("weak");
        this._edgeInvisibleType = this._edgeTypes.indexOf("invisible");

        this.nodeCount = this._nodes.length / this._nodeFieldCount;
        this._edgeCount = this._containmentEdges.length / this._edgeFieldsCount;

        this._buildEdgeIndexes();
        this._markInvisibleEdges();
        this._buildRetainers();
        this._calculateFlags();
        this._calculateDistances();
        var result = this._buildPostOrderIndex();
        // Actually it is array that maps node ordinal number to dominator node ordinal number.
        this._dominatorsTree = this._buildDominatorTree(result.postOrderIndex2NodeOrdinal, result.nodeOrdinal2PostOrderIndex);
        this._calculateRetainedSizes(result.postOrderIndex2NodeOrdinal);
        this._buildDominatedNodes();
    },

    _buildEdgeIndexes: function()
    {
        var nodes = this._nodes;
        var nodeCount = this.nodeCount;
        var firstEdgeIndexes = this._firstEdgeIndexes = new Uint32Array(nodeCount + 1);
        var nodeFieldCount = this._nodeFieldCount;
        var edgeFieldsCount = this._edgeFieldsCount;
        var nodeEdgeCountOffset = this._nodeEdgeCountOffset;
        firstEdgeIndexes[nodeCount] = this._containmentEdges.length;
        for (var nodeOrdinal = 0, edgeIndex = 0; nodeOrdinal < nodeCount; ++nodeOrdinal) {
            firstEdgeIndexes[nodeOrdinal] = edgeIndex;
            edgeIndex += nodes[nodeOrdinal * nodeFieldCount + nodeEdgeCountOffset] * edgeFieldsCount;
        }
    },

    _buildRetainers: function()
    {
        var retainingNodes = this._retainingNodes = new Uint32Array(this._edgeCount);
        var retainingEdges = this._retainingEdges = new Uint32Array(this._edgeCount);
        // Index of the first retainer in the _retainingNodes and _retainingEdges
        // arrays. Addressed by retained node index.
        var firstRetainerIndex = this._firstRetainerIndex = new Uint32Array(this.nodeCount + 1);

        var containmentEdges = this._containmentEdges;
        var edgeFieldsCount = this._edgeFieldsCount;
        var nodeFieldCount = this._nodeFieldCount;
        var edgeToNodeOffset = this._edgeToNodeOffset;
        var nodes = this._nodes;
        var firstEdgeIndexes = this._firstEdgeIndexes;
        var nodeCount = this.nodeCount;

        for (var toNodeFieldIndex = edgeToNodeOffset, l = containmentEdges.length; toNodeFieldIndex < l; toNodeFieldIndex += edgeFieldsCount) {
            var toNodeIndex = containmentEdges[toNodeFieldIndex];
            if (toNodeIndex % nodeFieldCount)
                throw new Error("Invalid toNodeIndex " + toNodeIndex);
            ++firstRetainerIndex[toNodeIndex / nodeFieldCount];
        }
        for (var i = 0, firstUnusedRetainerSlot = 0; i < nodeCount; i++) {
            var retainersCount = firstRetainerIndex[i];
            firstRetainerIndex[i] = firstUnusedRetainerSlot;
            retainingNodes[firstUnusedRetainerSlot] = retainersCount;
            firstUnusedRetainerSlot += retainersCount;
        }
        firstRetainerIndex[nodeCount] = retainingNodes.length;

        var nextNodeFirstEdgeIndex = firstEdgeIndexes[0];
        for (var srcNodeOrdinal = 0; srcNodeOrdinal < nodeCount; ++srcNodeOrdinal) {
            var firstEdgeIndex = nextNodeFirstEdgeIndex;
            nextNodeFirstEdgeIndex = firstEdgeIndexes[srcNodeOrdinal + 1];
            var srcNodeIndex = srcNodeOrdinal * nodeFieldCount;
            for (var edgeIndex = firstEdgeIndex; edgeIndex < nextNodeFirstEdgeIndex; edgeIndex += edgeFieldsCount) {
                var toNodeIndex = containmentEdges[edgeIndex + edgeToNodeOffset];
                if (toNodeIndex % nodeFieldCount)
                    throw new Error("Invalid toNodeIndex " + toNodeIndex);
                var firstRetainerSlotIndex = firstRetainerIndex[toNodeIndex / nodeFieldCount];
                var nextUnusedRetainerSlotIndex = firstRetainerSlotIndex + (--retainingNodes[firstRetainerSlotIndex]);
                retainingNodes[nextUnusedRetainerSlotIndex] = srcNodeIndex;
                retainingEdges[nextUnusedRetainerSlotIndex] = edgeIndex;
            }
        }
    },

    /**
     * @param {number=} nodeIndex
     */
    createNode: function(nodeIndex)
    {
        throw new Error("Not implemented");
    },

    createEdge: function(edges, edgeIndex)
    {
        throw new Error("Not implemented");
    },

    createRetainingEdge: function(retainedNodeIndex, retainerIndex)
    {
        throw new Error("Not implemented");
    },

    dispose: function()
    {
        delete this._nodes;
        delete this._strings;
        delete this._retainingEdges;
        delete this._retainingNodes;
        delete this._firstRetainerIndex;
        if (this._aggregates) {
            delete this._aggregates;
            delete this._aggregatesSortedFlags;
        }
        delete this._dominatedNodes;
        delete this._firstDominatedNodeIndex;
        delete this._nodeDistances;
        delete this._dominatorsTree;
    },

    _allNodes: function()
    {
        return new WebInspector.HeapSnapshotNodeIterator(this.rootNode());
    },

    rootNode: function()
    {
        return this.createNode(this._rootNodeIndex);
    },

    get rootNodeIndex()
    {
        return this._rootNodeIndex;
    },

    get totalSize()
    {
        return this.rootNode().retainedSize();
    },

    _getDominatedIndex: function(nodeIndex)
    {
        if (nodeIndex % this._nodeFieldCount)
            throw new Error("Invalid nodeIndex: " + nodeIndex);
        return this._firstDominatedNodeIndex[nodeIndex / this._nodeFieldCount];
    },

    _dominatedNodesOfNode: function(node)
    {
        var dominatedIndexFrom = this._getDominatedIndex(node.nodeIndex);
        var dominatedIndexTo = this._getDominatedIndex(node._nextNodeIndex());
        return new WebInspector.HeapSnapshotArraySlice(this._dominatedNodes, dominatedIndexFrom, dominatedIndexTo);
    },

    /**
     * @param {boolean} sortedIndexes
     * @param {string} key
     * @param {string=} filterString
     */
    aggregates: function(sortedIndexes, key, filterString)
    {
        if (!this._aggregates) {
            this._aggregates = {};
            this._aggregatesSortedFlags = {};
        }

        var aggregatesByClassName = this._aggregates[key];
        if (aggregatesByClassName) {
            if (sortedIndexes && !this._aggregatesSortedFlags[key]) {
                this._sortAggregateIndexes(aggregatesByClassName);
                this._aggregatesSortedFlags[key] = sortedIndexes;
            }
            return aggregatesByClassName;
        }

        var filter;
        if (filterString)
            filter = this._parseFilter(filterString);

        var aggregates = this._buildAggregates(filter);
        this._calculateClassesRetainedSize(aggregates.aggregatesByClassIndex, filter);
        aggregatesByClassName = aggregates.aggregatesByClassName;

        if (sortedIndexes)
            this._sortAggregateIndexes(aggregatesByClassName);

        this._aggregatesSortedFlags[key] = sortedIndexes;
        this._aggregates[key] = aggregatesByClassName;

        return aggregatesByClassName;
    },

    aggregatesForDiff: function()
    {
        if (this._aggregatesForDiff)
            return this._aggregatesForDiff;

        var aggregatesByClassName = this.aggregates(true, "allObjects");
        this._aggregatesForDiff  = {};

        var node = this.createNode();
        for (var className in aggregatesByClassName) {
            var aggregate = aggregatesByClassName[className];
            var indexes = aggregate.idxs;
            var ids = new Array(indexes.length);
            var selfSizes = new Array(indexes.length);
            for (var i = 0; i < indexes.length; i++) {
                node.nodeIndex = indexes[i];
                ids[i] = node.id();
                selfSizes[i] = node.selfSize();
            }

            this._aggregatesForDiff[className] = {
                indexes: indexes,
                ids: ids,
                selfSizes: selfSizes
            };
        }
        return this._aggregatesForDiff;
    },

    distanceForUserRoot: function(node)
    {
        return 1;
    },

    _calculateDistances: function()
    {
        var nodeFieldCount = this._nodeFieldCount;
        var distances = new Uint32Array(this.nodeCount);

        // bfs for Window roots
        var nodesToVisit = new Uint32Array(this.nodeCount);
        var nodesToVisitLength = 0;
        for (var iter = this.rootNode().edges(); iter.hasNext(); iter.next()) {
            var node = iter.edge.node();
            var distance = this.distanceForUserRoot(node);
            if (distance !== -1) {
                nodesToVisit[nodesToVisitLength++] = node.nodeIndex;
                distances[node.nodeIndex / nodeFieldCount] = distance;
            }
        }
        this._bfs(nodesToVisit, nodesToVisitLength, distances);

        // bfs for root
        nodesToVisitLength = 0;
        nodesToVisit[nodesToVisitLength++] = this._rootNodeIndex;
        distances[this._rootNodeIndex / nodeFieldCount] = 1;
        this._bfs(nodesToVisit, nodesToVisitLength, distances);
        this._nodeDistances = distances;
    },

    _bfs: function(nodesToVisit, nodesToVisitLength, distances)
    {
        // Peload fields into local variables for better performance.
        var edgeFieldsCount = this._edgeFieldsCount;
        var nodeFieldCount = this._nodeFieldCount;
        var containmentEdges = this._containmentEdges;
        var firstEdgeIndexes = this._firstEdgeIndexes;
        var edgeToNodeOffset = this._edgeToNodeOffset;
        var edgeTypeOffset = this._edgeTypeOffset;
        var nodes = this._nodes;
        var nodeCount = this.nodeCount;
        var containmentEdgesLength = containmentEdges.length;
        var edgeWeakType = this._edgeWeakType;

        var index = 0;
        while (index < nodesToVisitLength) {
            var nodeIndex = nodesToVisit[index++]; // shift generates too much garbage.
            var nodeOrdinal = nodeIndex / nodeFieldCount;
            var distance = distances[nodeOrdinal] + 1;
            var firstEdgeIndex = firstEdgeIndexes[nodeOrdinal];
            var edgesEnd = firstEdgeIndexes[nodeOrdinal + 1];
            for (var edgeIndex = firstEdgeIndex; edgeIndex < edgesEnd; edgeIndex += edgeFieldsCount) {
                var edgeType = containmentEdges[edgeIndex + edgeTypeOffset];
                if (edgeType == edgeWeakType)
                    continue;
                var childNodeIndex = containmentEdges[edgeIndex + edgeToNodeOffset];
                var childNodeOrdinal = childNodeIndex / nodeFieldCount;
                if (distances[childNodeOrdinal])
                    continue;
                distances[childNodeOrdinal] = distance;
                nodesToVisit[nodesToVisitLength++] = childNodeIndex;
            }
        }
        if (nodesToVisitLength > nodeCount)
            throw new Error("BFS failed. Nodes to visit (" + nodesToVisitLength + ") is more than nodes count (" + nodeCount + ")");
    },

    _buildAggregates: function(filter)
    {
        var aggregates = {};
        var aggregatesByClassName = {};
        var classIndexes = [];
        var nodes = this._nodes;
        var mapAndFlag = this.userObjectsMapAndFlag();
        var flags = mapAndFlag ? mapAndFlag.map : null;
        var flag = mapAndFlag ? mapAndFlag.flag : 0;
        var nodesLength = nodes.length;
        var nodeNativeType = this._nodeNativeType;
        var nodeFieldCount = this._nodeFieldCount;
        var selfSizeOffset = this._nodeSelfSizeOffset;
        var nodeTypeOffset = this._nodeTypeOffset;
        var node = this.rootNode();
        var nodeDistances = this._nodeDistances;

        for (var nodeIndex = 0; nodeIndex < nodesLength; nodeIndex += nodeFieldCount) {
            var nodeOrdinal = nodeIndex / nodeFieldCount;
            if (flags && !(flags[nodeOrdinal] & flag))
                continue;
            node.nodeIndex = nodeIndex;
            if (filter && !filter(node))
                continue;
            var selfSize = nodes[nodeIndex + selfSizeOffset];
            if (!selfSize && nodes[nodeIndex + nodeTypeOffset] !== nodeNativeType)
                continue;
            var classIndex = node.classIndex();
            if (!(classIndex in aggregates)) {
                var nodeType = node.type();
                var nameMatters = nodeType === "object" || nodeType === "native";
                var value = {
                    count: 1,
                    distance: nodeDistances[nodeOrdinal],
                    self: selfSize,
                    maxRet: 0,
                    type: nodeType,
                    name: nameMatters ? node.name() : null,
                    idxs: [nodeIndex]
                };
                aggregates[classIndex] = value;
                classIndexes.push(classIndex);
                aggregatesByClassName[node.className()] = value;
            } else {
                var clss = aggregates[classIndex];
                clss.distance = Math.min(clss.distance, nodeDistances[nodeOrdinal]);
                ++clss.count;
                clss.self += selfSize;
                clss.idxs.push(nodeIndex);
            }
        }

        // Shave off provisionally allocated space.
        for (var i = 0, l = classIndexes.length; i < l; ++i) {
            var classIndex = classIndexes[i];
            aggregates[classIndex].idxs = aggregates[classIndex].idxs.slice();
        }
        return {aggregatesByClassName: aggregatesByClassName, aggregatesByClassIndex: aggregates};
    },

    _calculateClassesRetainedSize: function(aggregates, filter)
    {
        var rootNodeIndex = this._rootNodeIndex;
        var node = this.createNode(rootNodeIndex);
        var list = [rootNodeIndex];
        var sizes = [-1];
        var classes = [];
        var seenClassNameIndexes = {};
        var nodeFieldCount = this._nodeFieldCount;
        var nodeTypeOffset = this._nodeTypeOffset;
        var nodeNativeType = this._nodeNativeType;
        var dominatedNodes = this._dominatedNodes;
        var nodes = this._nodes;
        var mapAndFlag = this.userObjectsMapAndFlag();
        var flags = mapAndFlag ? mapAndFlag.map : null;
        var flag = mapAndFlag ? mapAndFlag.flag : 0;
        var firstDominatedNodeIndex = this._firstDominatedNodeIndex;

        while (list.length) {
            var nodeIndex = list.pop();
            node.nodeIndex = nodeIndex;
            var classIndex = node.classIndex();
            var seen = !!seenClassNameIndexes[classIndex];
            var nodeOrdinal = nodeIndex / nodeFieldCount;
            var dominatedIndexFrom = firstDominatedNodeIndex[nodeOrdinal];
            var dominatedIndexTo = firstDominatedNodeIndex[nodeOrdinal + 1];

            if (!seen &&
                (!flags || (flags[nodeOrdinal] & flag)) &&
                (!filter || filter(node)) &&
                (node.selfSize() || nodes[nodeIndex + nodeTypeOffset] === nodeNativeType)
               ) {
                aggregates[classIndex].maxRet += node.retainedSize();
                if (dominatedIndexFrom !== dominatedIndexTo) {
                    seenClassNameIndexes[classIndex] = true;
                    sizes.push(list.length);
                    classes.push(classIndex);
                }
            }
            for (var i = dominatedIndexFrom; i < dominatedIndexTo; i++)
                list.push(dominatedNodes[i]);

            var l = list.length;
            while (sizes[sizes.length - 1] === l) {
                sizes.pop();
                classIndex = classes.pop();
                seenClassNameIndexes[classIndex] = false;
            }
        }
    },

    _sortAggregateIndexes: function(aggregates)
    {
        var nodeA = this.createNode();
        var nodeB = this.createNode();
        for (var clss in aggregates)
            aggregates[clss].idxs.sort(
                function(idxA, idxB) {
                    nodeA.nodeIndex = idxA;
                    nodeB.nodeIndex = idxB;
                    return nodeA.id() < nodeB.id() ? -1 : 1;
                });
    },

    _buildPostOrderIndex: function()
    {
        var nodeFieldCount = this._nodeFieldCount;
        var nodes = this._nodes;
        var nodeCount = this.nodeCount;
        var rootNodeOrdinal = this._rootNodeIndex / nodeFieldCount;

        var edgeFieldsCount = this._edgeFieldsCount;
        var edgeTypeOffset = this._edgeTypeOffset;
        var edgeToNodeOffset = this._edgeToNodeOffset;
        var edgeShortcutType = this._edgeShortcutType;
        var firstEdgeIndexes = this._firstEdgeIndexes;
        var containmentEdges = this._containmentEdges;
        var containmentEdgesLength = this._containmentEdges.length;

        var mapAndFlag = this.userObjectsMapAndFlag();
        var flags = mapAndFlag ? mapAndFlag.map : null;
        var flag = mapAndFlag ? mapAndFlag.flag : 0;

        var nodesToVisit = new Uint32Array(nodeCount);
        var postOrderIndex2NodeOrdinal = new Uint32Array(nodeCount);
        var nodeOrdinal2PostOrderIndex = new Uint32Array(nodeCount);
        var painted = new Uint8Array(nodeCount);
        var nodesToVisitLength = 0;
        var postOrderIndex = 0;
        var grey = 1;
        var black = 2;

        nodesToVisit[nodesToVisitLength++] = rootNodeOrdinal;
        painted[rootNodeOrdinal] = grey;

        while (nodesToVisitLength) {
            var nodeOrdinal = nodesToVisit[nodesToVisitLength - 1];

            if (painted[nodeOrdinal] === grey) {
                painted[nodeOrdinal] = black;
                var nodeFlag = !flags || (flags[nodeOrdinal] & flag);
                var beginEdgeIndex = firstEdgeIndexes[nodeOrdinal];
                var endEdgeIndex = firstEdgeIndexes[nodeOrdinal + 1];
                for (var edgeIndex = beginEdgeIndex; edgeIndex < endEdgeIndex; edgeIndex += edgeFieldsCount) {
                    if (nodeOrdinal !== rootNodeOrdinal && containmentEdges[edgeIndex + edgeTypeOffset] === edgeShortcutType)
                        continue;
                    var childNodeIndex = containmentEdges[edgeIndex + edgeToNodeOffset];
                    var childNodeOrdinal = childNodeIndex / nodeFieldCount;
                    var childNodeFlag = !flags || (flags[childNodeOrdinal] & flag);
                    // We are skipping the edges from non-page-owned nodes to page-owned nodes.
                    // Otherwise the dominators for the objects that also were retained by debugger would be affected.
                    if (nodeOrdinal !== rootNodeOrdinal && childNodeFlag && !nodeFlag)
                        continue;
                    if (!painted[childNodeOrdinal]) {
                        painted[childNodeOrdinal] = grey;
                        nodesToVisit[nodesToVisitLength++] = childNodeOrdinal;
                    }
                }
            } else {
                nodeOrdinal2PostOrderIndex[nodeOrdinal] = postOrderIndex;
                postOrderIndex2NodeOrdinal[postOrderIndex++] = nodeOrdinal;
                --nodesToVisitLength;
            }
        }

        if (postOrderIndex !== nodeCount) {
            var dumpNode = this.rootNode();
            for (var i = 0; i < nodeCount; ++i) {
                if (painted[i] !== black) {
                    dumpNode.nodeIndex = i * nodeFieldCount;
                    console.log(JSON.stringify(dumpNode.serialize()));
                    var retainers = dumpNode.retainers();
                    while (retainers) {
                        console.log("edgeName: " + retainers.item().name() + " nodeClassName: " + retainers.item().node().className());
                        retainers = retainers.item().node().retainers();
                    }
                }
            }
            throw new Error("Postordering failed. " + (nodeCount - postOrderIndex) + " hanging nodes");
        }

        return {postOrderIndex2NodeOrdinal: postOrderIndex2NodeOrdinal, nodeOrdinal2PostOrderIndex: nodeOrdinal2PostOrderIndex};
    },

    // The algorithm is based on the article:
    // K. Cooper, T. Harvey and K. Kennedy "A Simple, Fast Dominance Algorithm"
    // Softw. Pract. Exper. 4 (2001), pp. 1-10.
    /**
     * @param {Array.<number>} postOrderIndex2NodeOrdinal
     * @param {Array.<number>} nodeOrdinal2PostOrderIndex
     */
    _buildDominatorTree: function(postOrderIndex2NodeOrdinal, nodeOrdinal2PostOrderIndex)
    {
        var nodeFieldCount = this._nodeFieldCount;
        var nodes = this._nodes;
        var firstRetainerIndex = this._firstRetainerIndex;
        var retainingNodes = this._retainingNodes;
        var retainingEdges = this._retainingEdges;
        var edgeFieldsCount = this._edgeFieldsCount;
        var edgeTypeOffset = this._edgeTypeOffset;
        var edgeToNodeOffset = this._edgeToNodeOffset;
        var edgeShortcutType = this._edgeShortcutType;
        var firstEdgeIndexes = this._firstEdgeIndexes;
        var containmentEdges = this._containmentEdges;
        var containmentEdgesLength = this._containmentEdges.length;
        var rootNodeIndex = this._rootNodeIndex;

        var mapAndFlag = this.userObjectsMapAndFlag();
        var flags = mapAndFlag ? mapAndFlag.map : null;
        var flag = mapAndFlag ? mapAndFlag.flag : 0;

        var nodesCount = postOrderIndex2NodeOrdinal.length;
        var rootPostOrderedIndex = nodesCount - 1;
        var noEntry = nodesCount;
        var dominators = new Uint32Array(nodesCount);
        for (var i = 0; i < rootPostOrderedIndex; ++i)
            dominators[i] = noEntry;
        dominators[rootPostOrderedIndex] = rootPostOrderedIndex;

        // The affected array is used to mark entries which dominators
        // have to be racalculated because of changes in their retainers.
        var affected = new Uint8Array(nodesCount);
        var nodeOrdinal;

        { // Mark the root direct children as affected.
            nodeOrdinal = this._rootNodeIndex / nodeFieldCount;
            var beginEdgeToNodeFieldIndex = firstEdgeIndexes[nodeOrdinal] + edgeToNodeOffset;
            var endEdgeToNodeFieldIndex = firstEdgeIndexes[nodeOrdinal + 1];
            for (var toNodeFieldIndex = beginEdgeToNodeFieldIndex;
                 toNodeFieldIndex < endEdgeToNodeFieldIndex;
                 toNodeFieldIndex += edgeFieldsCount) {
                var childNodeOrdinal = containmentEdges[toNodeFieldIndex] / nodeFieldCount;
                affected[nodeOrdinal2PostOrderIndex[childNodeOrdinal]] = 1;
            }
        }

        var changed = true;
        while (changed) {
            changed = false;
            for (var postOrderIndex = rootPostOrderedIndex - 1; postOrderIndex >= 0; --postOrderIndex) {
                if (affected[postOrderIndex] === 0)
                    continue;
                affected[postOrderIndex] = 0;
                // If dominator of the entry has already been set to root,
                // then it can't propagate any further.
                if (dominators[postOrderIndex] === rootPostOrderedIndex)
                    continue;
                nodeOrdinal = postOrderIndex2NodeOrdinal[postOrderIndex];
                var nodeFlag = !flags || (flags[nodeOrdinal] & flag);
                var newDominatorIndex = noEntry;
                var beginRetainerIndex = firstRetainerIndex[nodeOrdinal];
                var endRetainerIndex = firstRetainerIndex[nodeOrdinal + 1];
                for (var retainerIndex = beginRetainerIndex; retainerIndex < endRetainerIndex; ++retainerIndex) {
                    var retainerEdgeIndex = retainingEdges[retainerIndex];
                    var retainerEdgeType = containmentEdges[retainerEdgeIndex + edgeTypeOffset];
                    var retainerNodeIndex = retainingNodes[retainerIndex];
                    if (retainerNodeIndex !== rootNodeIndex && retainerEdgeType === edgeShortcutType)
                        continue;
                    var retainerNodeOrdinal = retainerNodeIndex / nodeFieldCount;
                    var retainerNodeFlag = !flags || (flags[retainerNodeOrdinal] & flag);
                    // We are skipping the edges from non-page-owned nodes to page-owned nodes.
                    // Otherwise the dominators for the objects that also were retained by debugger would be affected.
                    if (retainerNodeIndex !== rootNodeIndex && nodeFlag && !retainerNodeFlag)
                        continue;
                    var retanerPostOrderIndex = nodeOrdinal2PostOrderIndex[retainerNodeOrdinal];
                    if (dominators[retanerPostOrderIndex] !== noEntry) {
                        if (newDominatorIndex === noEntry)
                            newDominatorIndex = retanerPostOrderIndex;
                        else {
                            while (retanerPostOrderIndex !== newDominatorIndex) {
                                while (retanerPostOrderIndex < newDominatorIndex)
                                    retanerPostOrderIndex = dominators[retanerPostOrderIndex];
                                while (newDominatorIndex < retanerPostOrderIndex)
                                    newDominatorIndex = dominators[newDominatorIndex];
                            }
                        }
                        // If idom has already reached the root, it doesn't make sense
                        // to check other retainers.
                        if (newDominatorIndex === rootPostOrderedIndex)
                            break;
                    }
                }
                if (newDominatorIndex !== noEntry && dominators[postOrderIndex] !== newDominatorIndex) {
                    dominators[postOrderIndex] = newDominatorIndex;
                    changed = true;
                    nodeOrdinal = postOrderIndex2NodeOrdinal[postOrderIndex];
                    beginEdgeToNodeFieldIndex = firstEdgeIndexes[nodeOrdinal] + edgeToNodeOffset;
                    endEdgeToNodeFieldIndex = firstEdgeIndexes[nodeOrdinal + 1];
                    for (var toNodeFieldIndex = beginEdgeToNodeFieldIndex;
                         toNodeFieldIndex < endEdgeToNodeFieldIndex;
                         toNodeFieldIndex += edgeFieldsCount) {
                        var childNodeOrdinal = containmentEdges[toNodeFieldIndex] / nodeFieldCount;
                        affected[nodeOrdinal2PostOrderIndex[childNodeOrdinal]] = 1;
                    }
                }
            }
        }

        var dominatorsTree = new Uint32Array(nodesCount);
        for (var postOrderIndex = 0, l = dominators.length; postOrderIndex < l; ++postOrderIndex) {
            nodeOrdinal = postOrderIndex2NodeOrdinal[postOrderIndex];
            dominatorsTree[nodeOrdinal] = postOrderIndex2NodeOrdinal[dominators[postOrderIndex]];
        }
        return dominatorsTree;
    },

    _calculateRetainedSizes: function(postOrderIndex2NodeOrdinal)
    {
        var nodeCount = this.nodeCount;
        var nodes = this._nodes;
        var nodeSelfSizeOffset = this._nodeSelfSizeOffset;
        var nodeFieldCount = this._nodeFieldCount;
        var dominatorsTree = this._dominatorsTree;
        // Reuse now unused edge_count field to store retained size.
        var nodeRetainedSizeOffset = this._nodeRetainedSizeOffset = this._nodeEdgeCountOffset;
        delete this._nodeEdgeCountOffset;

        for (var nodeIndex = 0, l = nodes.length; nodeIndex < l; nodeIndex += nodeFieldCount)
            nodes[nodeIndex + nodeRetainedSizeOffset] = nodes[nodeIndex + nodeSelfSizeOffset];

        // Propagate retained sizes for each node excluding root.
        for (var postOrderIndex = 0; postOrderIndex < nodeCount - 1; ++postOrderIndex) {
            var nodeOrdinal = postOrderIndex2NodeOrdinal[postOrderIndex];
            var nodeIndex = nodeOrdinal * nodeFieldCount;
            var dominatorIndex = dominatorsTree[nodeOrdinal] * nodeFieldCount;
            nodes[dominatorIndex + nodeRetainedSizeOffset] += nodes[nodeIndex + nodeRetainedSizeOffset];
        }
    },

    _buildDominatedNodes: function()
    {
        // Builds up two arrays:
        //  - "dominatedNodes" is a continuous array, where each node owns an
        //    interval (can be empty) with corresponding dominated nodes.
        //  - "indexArray" is an array of indexes in the "dominatedNodes"
        //    with the same positions as in the _nodeIndex.
        var indexArray = this._firstDominatedNodeIndex = new Uint32Array(this.nodeCount + 1);
        // All nodes except the root have dominators.
        var dominatedNodes = this._dominatedNodes = new Uint32Array(this.nodeCount - 1);

        // Count the number of dominated nodes for each node. Skip the root (node at
        // index 0) as it is the only node that dominates itself.
        var nodeFieldCount = this._nodeFieldCount;
        var dominatorsTree = this._dominatorsTree;

        var fromNodeOrdinal = 0;
        var toNodeOrdinal = this.nodeCount;
        var rootNodeOrdinal = this._rootNodeIndex / nodeFieldCount;
        if (rootNodeOrdinal === fromNodeOrdinal)
            fromNodeOrdinal = 1;
        else if (rootNodeOrdinal === toNodeOrdinal - 1)
            toNodeOrdinal = toNodeOrdinal - 1;
        else
            throw new Error("Root node is expected to be either first or last");
        for (var nodeOrdinal = fromNodeOrdinal; nodeOrdinal < toNodeOrdinal; ++nodeOrdinal)
            ++indexArray[dominatorsTree[nodeOrdinal]];
        // Put in the first slot of each dominatedNodes slice the count of entries
        // that will be filled.
        var firstDominatedNodeIndex = 0;
        for (var i = 0, l = this.nodeCount; i < l; ++i) {
            var dominatedCount = dominatedNodes[firstDominatedNodeIndex] = indexArray[i];
            indexArray[i] = firstDominatedNodeIndex;
            firstDominatedNodeIndex += dominatedCount;
        }
        indexArray[this.nodeCount] = dominatedNodes.length;
        // Fill up the dominatedNodes array with indexes of dominated nodes. Skip the root (node at
        // index 0) as it is the only node that dominates itself.
        for (var nodeOrdinal = fromNodeOrdinal; nodeOrdinal < toNodeOrdinal; ++nodeOrdinal) {
            var dominatorOrdinal = dominatorsTree[nodeOrdinal];
            var dominatedRefIndex = indexArray[dominatorOrdinal];
            dominatedRefIndex += (--dominatedNodes[dominatedRefIndex]);
            dominatedNodes[dominatedRefIndex] = nodeOrdinal * nodeFieldCount;
        }
    },

    _markInvisibleEdges: function()
    {
        throw new Error("Not implemented");
    },

    _numbersComparator: function(a, b)
    {
        return a < b ? -1 : (a > b ? 1 : 0);
    },

    _calculateFlags: function()
    {
        throw new Error("Not implemented");
    },

    userObjectsMapAndFlag: function()
    {
        throw new Error("Not implemented");
    },

    calculateSnapshotDiff: function(baseSnapshotId, baseSnapshotAggregates)
    {
        var snapshotDiff = this._snapshotDiffs[baseSnapshotId];
        if (snapshotDiff)
            return snapshotDiff;
        snapshotDiff = {};

        var aggregates = this.aggregates(true, "allObjects");
        for (var className in baseSnapshotAggregates) {
            var baseAggregate = baseSnapshotAggregates[className];
            var diff = this._calculateDiffForClass(baseAggregate, aggregates[className]);
            if (diff)
                snapshotDiff[className] = diff;
        }
        var emptyBaseAggregate = { ids: [], indexes: [], selfSizes: [] };
        for (var className in aggregates) {
            if (className in baseSnapshotAggregates)
                continue;
            snapshotDiff[className] = this._calculateDiffForClass(emptyBaseAggregate, aggregates[className]);
        }

        this._snapshotDiffs[baseSnapshotId] = snapshotDiff;
        return snapshotDiff;
    },

    _calculateDiffForClass: function(baseAggregate, aggregate)
    {
        var baseIds = baseAggregate.ids;
        var baseIndexes = baseAggregate.indexes;
        var baseSelfSizes = baseAggregate.selfSizes;

        var indexes = aggregate ? aggregate.idxs : [];

        var i = 0, l = baseIds.length;
        var j = 0, m = indexes.length;
        var diff = { addedCount: 0,
                     removedCount: 0,
                     addedSize: 0,
                     removedSize: 0,
                     deletedIndexes: [],
                     addedIndexes: [] };

        var nodeB = this.createNode(indexes[j]);
        while (i < l && j < m) {
            var nodeAId = baseIds[i];
            if (nodeAId < nodeB.id()) {
                diff.deletedIndexes.push(baseIndexes[i]);
                diff.removedCount++;
                diff.removedSize += baseSelfSizes[i];
                ++i;
            } else if (nodeAId > nodeB.id()) { // Native nodes(e.g. dom groups) may have ids less than max JS object id in the base snapshot
                diff.addedIndexes.push(indexes[j]);
                diff.addedCount++;
                diff.addedSize += nodeB.selfSize();
                nodeB.nodeIndex = indexes[++j];
            } else { // nodeAId === nodeB.id()
                ++i;
                nodeB.nodeIndex = indexes[++j];
            }
        }
        while (i < l) {
            diff.deletedIndexes.push(baseIndexes[i]);
            diff.removedCount++;
            diff.removedSize += baseSelfSizes[i];
            ++i;
        }
        while (j < m) {
            diff.addedIndexes.push(indexes[j]);
            diff.addedCount++;
            diff.addedSize += nodeB.selfSize();
            nodeB.nodeIndex = indexes[++j];
        }
        diff.countDelta = diff.addedCount - diff.removedCount;
        diff.sizeDelta = diff.addedSize - diff.removedSize;
        if (!diff.addedCount && !diff.removedCount)
            return null;
        return diff;
    },

    _nodeForSnapshotObjectId: function(snapshotObjectId)
    {
        for (var it = this._allNodes(); it.hasNext(); it.next()) {
            if (it.node.id() === snapshotObjectId)
                return it.node;
        }
        return null;
    },

    nodeClassName: function(snapshotObjectId)
    {
        var node = this._nodeForSnapshotObjectId(snapshotObjectId);
        if (node)
            return node.className();
        return null;
    },

    dominatorIdsForNode: function(snapshotObjectId)
    {
        var node = this._nodeForSnapshotObjectId(snapshotObjectId);
        if (!node)
            return null;
        var result = [];
        while (!node.isRoot()) {
            result.push(node.id());
            node.nodeIndex = node.dominatorIndex();
        }
        return result;
    },

    _parseFilter: function(filter)
    {
        if (!filter)
            return null;
        var parsedFilter = eval("(function(){return " + filter + "})()");
        return parsedFilter.bind(this);
    },

    createEdgesProvider: function(nodeIndex, showHiddenData)
    {
        var node = this.createNode(nodeIndex);
        var filter = this.containmentEdgesFilter(showHiddenData);
        return new WebInspector.HeapSnapshotEdgesProvider(this, filter, node.edges());
    },

    createEdgesProviderForTest: function(nodeIndex, filter)
    {
        var node = this.createNode(nodeIndex);
        return new WebInspector.HeapSnapshotEdgesProvider(this, filter, node.edges());
    },

    retainingEdgesFilter: function(showHiddenData)
    {
        return null;
    },

    containmentEdgesFilter: function(showHiddenData)
    {
        return null;
    },

    createRetainingEdgesProvider: function(nodeIndex, showHiddenData)
    {
        var node = this.createNode(nodeIndex);
        var filter = this.retainingEdgesFilter(showHiddenData);
        return new WebInspector.HeapSnapshotEdgesProvider(this, filter, node.retainers());
    },

    createAddedNodesProvider: function(baseSnapshotId, className)
    {
        var snapshotDiff = this._snapshotDiffs[baseSnapshotId];
        var diffForClass = snapshotDiff[className];
        return new WebInspector.HeapSnapshotNodesProvider(this, null, diffForClass.addedIndexes);
    },

    createDeletedNodesProvider: function(nodeIndexes)
    {
        return new WebInspector.HeapSnapshotNodesProvider(this, null, nodeIndexes);
    },

    classNodesFilter: function()
    {
        return null;
    },

    createNodesProviderForClass: function(className, aggregatesKey)
    {
        return new WebInspector.HeapSnapshotNodesProvider(this, this.classNodesFilter(), this.aggregates(false, aggregatesKey)[className].idxs);
    },

    createNodesProviderForDominator: function(nodeIndex)
    {
        var node = this.createNode(nodeIndex);
        return new WebInspector.HeapSnapshotNodesProvider(this, null, this._dominatedNodesOfNode(node));
    },

    updateStaticData: function()
    {
        return {nodeCount: this.nodeCount, rootNodeIndex: this._rootNodeIndex, totalSize: this.totalSize, uid: this.uid};
    }
};

/**
 * @constructor
 * @param {Array.<number>=} unfilteredIterationOrder
 */
WebInspector.HeapSnapshotFilteredOrderedIterator = function(iterator, filter, unfilteredIterationOrder)
{
    this._filter = filter;
    this._iterator = iterator;
    this._unfilteredIterationOrder = unfilteredIterationOrder;
    this._iterationOrder = null;
    this._position = 0;
    this._currentComparator = null;
    this._sortedPrefixLength = 0;
}

WebInspector.HeapSnapshotFilteredOrderedIterator.prototype = {
    _createIterationOrder: function()
    {
        if (this._iterationOrder)
            return;
        if (this._unfilteredIterationOrder && !this._filter) {
            this._iterationOrder = this._unfilteredIterationOrder.slice(0);
            this._unfilteredIterationOrder = null;
            return;
        }
        this._iterationOrder = [];
        var iterator = this._iterator;
        if (!this._unfilteredIterationOrder && !this._filter) {
            for (iterator.rewind(); iterator.hasNext(); iterator.next())
                this._iterationOrder.push(iterator.index());
        } else if (!this._unfilteredIterationOrder) {
            for (iterator.rewind(); iterator.hasNext(); iterator.next()) {
                if (this._filter(iterator.item()))
                    this._iterationOrder.push(iterator.index());
            }
        } else {
            var order = this._unfilteredIterationOrder.constructor === Array ?
                this._unfilteredIterationOrder : this._unfilteredIterationOrder.slice(0);
            for (var i = 0, l = order.length; i < l; ++i) {
                iterator.setIndex(order[i]);
                if (this._filter(iterator.item()))
                    this._iterationOrder.push(iterator.index());
            }
            this._unfilteredIterationOrder = null;
        }
    },

    rewind: function()
    {
        this._position = 0;
    },

    hasNext: function()
    {
        return this._position < this._iterationOrder.length;
    },

    isEmpty: function()
    {
        if (this._iterationOrder)
            return !this._iterationOrder.length;
        if (this._unfilteredIterationOrder && !this._filter)
            return !this._unfilteredIterationOrder.length;
        var iterator = this._iterator;
        if (!this._unfilteredIterationOrder && !this._filter) {
            iterator.rewind();
            return !iterator.hasNext();
        } else if (!this._unfilteredIterationOrder) {
            for (iterator.rewind(); iterator.hasNext(); iterator.next())
                if (this._filter(iterator.item()))
                    return false;
        } else {
            var order = this._unfilteredIterationOrder.constructor === Array ?
                this._unfilteredIterationOrder : this._unfilteredIterationOrder.slice(0);
            for (var i = 0, l = order.length; i < l; ++i) {
                iterator.setIndex(order[i]);
                if (this._filter(iterator.item()))
                    return false;
            }
        }
        return true;
    },

    item: function()
    {
        this._iterator.setIndex(this._iterationOrder[this._position]);
        return this._iterator.item();
    },

    get length()
    {
        this._createIterationOrder();
        return this._iterationOrder.length;
    },

    next: function()
    {
        ++this._position;
    },

    /**
     * @param {number} begin
     * @param {number} end
     */
    serializeItemsRange: function(begin, end)
    {
        this._createIterationOrder();
        if (begin > end)
            throw new Error("Start position > end position: " + begin + " > " + end);
        if (end >= this._iterationOrder.length)
            end = this._iterationOrder.length;
        if (this._sortedPrefixLength < end) {
            this.sort(this._currentComparator, this._sortedPrefixLength, this._iterationOrder.length - 1, end - this._sortedPrefixLength);
            this._sortedPrefixLength = end;
        }

        this._position = begin;
        var startPosition = this._position;
        var count = end - begin;
        var result = new Array(count);
        for (var i = 0 ; i < count && this.hasNext(); ++i, this.next())
            result[i] = this.item().serialize();
        result.length = i;
        result.totalLength = this._iterationOrder.length;

        result.startPosition = startPosition;
        result.endPosition = this._position;
        return result;
    },

    sortAll: function()
    {
        this._createIterationOrder();
        if (this._sortedPrefixLength === this._iterationOrder.length)
            return;
        this.sort(this._currentComparator, this._sortedPrefixLength, this._iterationOrder.length - 1, this._iterationOrder.length);
        this._sortedPrefixLength = this._iterationOrder.length;
    },

    sortAndRewind: function(comparator)
    {
        this._currentComparator = comparator;
        this._sortedPrefixLength = 0;
        this.rewind();
    }
}

WebInspector.HeapSnapshotFilteredOrderedIterator.prototype.createComparator = function(fieldNames)
{
    return {fieldName1: fieldNames[0], ascending1: fieldNames[1], fieldName2: fieldNames[2], ascending2: fieldNames[3]};
}

/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotFilteredOrderedIterator}
 */
WebInspector.HeapSnapshotEdgesProvider = function(snapshot, filter, edgesIter)
{
    this.snapshot = snapshot;
    WebInspector.HeapSnapshotFilteredOrderedIterator.call(this, edgesIter, filter);
}

WebInspector.HeapSnapshotEdgesProvider.prototype = {
    sort: function(comparator, leftBound, rightBound, count)
    {
        var fieldName1 = comparator.fieldName1;
        var fieldName2 = comparator.fieldName2;
        var ascending1 = comparator.ascending1;
        var ascending2 = comparator.ascending2;

        var edgeA = this._iterator.item().clone();
        var edgeB = edgeA.clone();
        var nodeA = this.snapshot.createNode();
        var nodeB = this.snapshot.createNode();

        function compareEdgeFieldName(ascending, indexA, indexB)
        {
            edgeA.edgeIndex = indexA;
            edgeB.edgeIndex = indexB;
            if (edgeB.name() === "__proto__") return -1;
            if (edgeA.name() === "__proto__") return 1;
            var result =
                edgeA.hasStringName() === edgeB.hasStringName() ?
                (edgeA.name() < edgeB.name() ? -1 : (edgeA.name() > edgeB.name() ? 1 : 0)) :
                (edgeA.hasStringName() ? -1 : 1);
            return ascending ? result : -result;
        }

        function compareNodeField(fieldName, ascending, indexA, indexB)
        {
            edgeA.edgeIndex = indexA;
            nodeA.nodeIndex = edgeA.nodeIndex();
            var valueA = nodeA[fieldName]();

            edgeB.edgeIndex = indexB;
            nodeB.nodeIndex = edgeB.nodeIndex();
            var valueB = nodeB[fieldName]();

            var result = valueA < valueB ? -1 : (valueA > valueB ? 1 : 0);
            return ascending ? result : -result;
        }

        function compareEdgeAndNode(indexA, indexB) {
            var result = compareEdgeFieldName(ascending1, indexA, indexB);
            if (result === 0)
                result = compareNodeField(fieldName2, ascending2, indexA, indexB);
            return result;
        }

        function compareNodeAndEdge(indexA, indexB) {
            var result = compareNodeField(fieldName1, ascending1, indexA, indexB);
            if (result === 0)
                result = compareEdgeFieldName(ascending2, indexA, indexB);
            return result;
        }

        function compareNodeAndNode(indexA, indexB) {
            var result = compareNodeField(fieldName1, ascending1, indexA, indexB);
            if (result === 0)
                result = compareNodeField(fieldName2, ascending2, indexA, indexB);
            return result;
        }

        if (fieldName1 === "!edgeName")
            this._iterationOrder.sortRange(compareEdgeAndNode, leftBound, rightBound, count);
        else if (fieldName2 === "!edgeName")
            this._iterationOrder.sortRange(compareNodeAndEdge, leftBound, rightBound, count);
        else
            this._iterationOrder.sortRange(compareNodeAndNode, leftBound, rightBound, count);
    },

    __proto__: WebInspector.HeapSnapshotFilteredOrderedIterator.prototype
}


/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotFilteredOrderedIterator}
 * @param {Array.<number>=} nodeIndexes
 */
WebInspector.HeapSnapshotNodesProvider = function(snapshot, filter, nodeIndexes)
{
    this.snapshot = snapshot;
    WebInspector.HeapSnapshotFilteredOrderedIterator.call(this, snapshot._allNodes(), filter, nodeIndexes);
}

WebInspector.HeapSnapshotNodesProvider.prototype = {
    nodePosition: function(snapshotObjectId)
    {
        this._createIterationOrder();
        if (this.isEmpty())
            return -1;
        this.sortAll();

        var node = this.snapshot.createNode();
        for (var i = 0; i < this._iterationOrder.length; i++) {
            node.nodeIndex = this._iterationOrder[i];
            if (node.id() === snapshotObjectId)
                return i;
        }
        return -1;
    },

    sort: function(comparator, leftBound, rightBound, count)
    {
        var fieldName1 = comparator.fieldName1;
        var fieldName2 = comparator.fieldName2;
        var ascending1 = comparator.ascending1;
        var ascending2 = comparator.ascending2;

        var nodeA = this.snapshot.createNode();
        var nodeB = this.snapshot.createNode();

        function sortByNodeField(fieldName, ascending)
        {
            var valueOrFunctionA = nodeA[fieldName];
            var valueA = typeof valueOrFunctionA !== "function" ? valueOrFunctionA : valueOrFunctionA.call(nodeA);
            var valueOrFunctionB = nodeB[fieldName];
            var valueB = typeof valueOrFunctionB !== "function" ? valueOrFunctionB : valueOrFunctionB.call(nodeB);
            var result = valueA < valueB ? -1 : (valueA > valueB ? 1 : 0);
            return ascending ? result : -result;
        }

        function sortByComparator(indexA, indexB) {
            nodeA.nodeIndex = indexA;
            nodeB.nodeIndex = indexB;
            var result = sortByNodeField(fieldName1, ascending1);
            if (result === 0)
                result = sortByNodeField(fieldName2, ascending2);
            return result;
        }

        this._iterationOrder.sortRange(sortByComparator, leftBound, rightBound, count);
    },

    __proto__: WebInspector.HeapSnapshotFilteredOrderedIterator.prototype
}

