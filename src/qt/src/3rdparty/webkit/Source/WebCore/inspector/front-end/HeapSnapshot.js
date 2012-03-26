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

WebInspector.HeapSnapshotLoader = function()
{
    this._json = "";
    this._state = "find-snapshot-info";
    this._snapshot = {};
}

WebInspector.HeapSnapshotLoader.prototype = {
    _findBalancedCurlyBrackets: function()
    {
        var counter = 0;
        var openingBracket = "{".charCodeAt(0), closingBracket = "}".charCodeAt(0);
        for (var i = 0, l = this._json.length; i < l; ++i) {
            var character = this._json.charCodeAt(i);
            if (character === openingBracket)
                ++counter;
            else if (character === closingBracket) {
                if (--counter === 0)
                    return i + 1;
            }
        }
        return -1;
    },

    finishLoading: function()
    {
        if (!this._json)
            return null;
        this._parseStringsArray();
        this._json = "";
        var result = new WebInspector.HeapSnapshot(this._snapshot);
        this._json = "";
        this._snapshot = {};
        return result;
    },

    _parseNodes: function()
    {
        var index = 0;
        var char0 = "0".charCodeAt(0), char9 = "9".charCodeAt(0), closingBracket = "]".charCodeAt(0);
        var length = this._json.length;
        while (true) {
            while (index < length) {
                var code = this._json.charCodeAt(index);
                if (char0 <= code && code <= char9)
                    break;
                else if (code === closingBracket) {
                    this._json = this._json.slice(index + 1);
                    // Shave off provisionally allocated space.
                    this._snapshot.nodes = this._snapshot.nodes.slice(0);
                    return false;
                }
                ++index;
            }
            if (index === length) {
                this._json = "";
                return true;
            }
            var startIndex = index;
            while (index < length) {
                var code = this._json.charCodeAt(index);
                if (char0 > code || code > char9)
                    break;
                ++index;
            }
            if (index === length) {
                this._json = this._json.slice(startIndex);
                return true;
            }
            this._snapshot.nodes.push(parseInt(this._json.slice(startIndex, index)));
        }
    },

    _parseStringsArray: function()
    {
        var closingBracketIndex = this._json.lastIndexOf("]");
        if (closingBracketIndex === -1)
            throw new Error("Incomplete JSON");
        this._json = this._json.slice(0, closingBracketIndex + 1);
        this._snapshot.strings = JSON.parse(this._json);
    },

    pushJSONChunk: function(chunk)
    {
        this._json += chunk;
        switch (this._state) {
        case "find-snapshot-info": {
            var snapshotToken = "\"snapshot\"";
            var snapshotTokenIndex = this._json.indexOf(snapshotToken);
            if (snapshotTokenIndex === -1)
                throw new Error("Snapshot token not found");
            this._json = this._json.slice(snapshotTokenIndex + snapshotToken.length + 1);
            this._state = "parse-snapshot-info";
            this.pushJSONChunk("");
            break;
        }
        case "parse-snapshot-info": {
            var closingBracketIndex = this._findBalancedCurlyBrackets();
            if (closingBracketIndex === -1)
                return;
            this._snapshot.snapshot = JSON.parse(this._json.slice(0, closingBracketIndex));
            this._json = this._json.slice(closingBracketIndex);
            this._state = "find-nodes";
            this.pushJSONChunk("");
            break;
        }
        case "find-nodes": {
            var nodesToken = "\"nodes\"";
            var nodesTokenIndex = this._json.indexOf(nodesToken);
            if (nodesTokenIndex === -1)
                return;
            var bracketIndex = this._json.indexOf("[", nodesTokenIndex);
            if (bracketIndex === -1)
                return;
            this._json = this._json.slice(bracketIndex + 1);
            this._state = "parse-nodes-meta-info";
            this.pushJSONChunk("");
            break;
        }
        case "parse-nodes-meta-info": {
            var closingBracketIndex = this._findBalancedCurlyBrackets();
            if (closingBracketIndex === -1)
                return;
            this._snapshot.nodes = [JSON.parse(this._json.slice(0, closingBracketIndex))];
            this._json = this._json.slice(closingBracketIndex);
            this._state = "parse-nodes";
            this.pushJSONChunk("");
            break;          
        }
        case "parse-nodes": {
            if (this._parseNodes())
                return;
            this._state = "find-strings";
            this.pushJSONChunk("");
            break;
        }
        case "find-strings": {
            var stringsToken = "\"strings\"";
            var stringsTokenIndex = this._json.indexOf(stringsToken);
            if (stringsTokenIndex === -1)
                return;
            var bracketIndex = this._json.indexOf("[", stringsTokenIndex);
            if (bracketIndex === -1)
                return;
            this._json = this._json.slice(bracketIndex);
            this._state = "accumulate-strings";
            break;
        }
        case "accumulate-strings":
            break;
        }
    }
};

WebInspector.HeapSnapshotArraySlice = function(snapshot, arrayName, start, end)
{
    // Note: we don't reference snapshot contents directly to avoid
    // holding references to big chunks of data.
    this._snapshot = snapshot;
    this._arrayName = arrayName;
    this._start = start;
    this.length = end - start;
}

WebInspector.HeapSnapshotArraySlice.prototype = {
    item: function(index)
    {
        return this._snapshot[this._arrayName][this._start + index];
    }
}

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

    get hasStringName()
    {
        if (!this.isShortcut)
            return this._hasStringName;
        return isNaN(parseInt(this._name, 10));
    },

    get isElement()
    {
        return this._type() === this._snapshot._edgeElementType;
    },

    get isHidden()
    {
        return this._type() === this._snapshot._edgeHiddenType;
    },

    get isInternal()
    {
        return this._type() === this._snapshot._edgeInternalType;
    },

    get isInvisible()
    {
        return this._type() === this._snapshot._edgeInvisibleType;
    },

    get isShortcut()
    {
        return this._type() === this._snapshot._edgeShortcutType;
    },

    get name()
    {
        if (!this.isShortcut)
            return this._name;
        var numName = parseInt(this._name, 10);
        return isNaN(numName) ? this._name : numName;
    },

    get node()
    {
        return new WebInspector.HeapSnapshotNode(this._snapshot, this.nodeIndex);
    },

    get nodeIndex()
    {
        return this._edges.item(this.edgeIndex + this._snapshot._edgeToNodeOffset);
    },

    get rawEdges()
    {
        return this._edges;
    },

    toString: function()
    {
        switch (this.type) {
        case "context": return "->" + this.name;
        case "element": return "[" + this.name + "]";
        case "property":
            return this.name.indexOf(" ") === -1 ? "." + this.name : "[\"" + this.name + "\"]";
        case "shortcut":
            var name = this.name;
            if (typeof name === "string")
                return this.name.indexOf(" ") === -1 ? "." + this.name : "[\"" + this.name + "\"]";
            else
                return "[" + this.name + "]";
        case "internal":
        case "hidden":
        case "invisible":
            return "{" + this.name + "}";
        };
        return "?" + this.name + "?";
    },

    get type()
    {
        return this._snapshot._edgeTypes[this._type()];
    },

    get _hasStringName()
    {
        return !this.isElement && !this.isHidden;
    },

    get _name()
    {
        return this._hasStringName ? this._snapshot._strings[this._nameOrIndex] : this._nameOrIndex;
    },

    get _nameOrIndex()
    {
        return this._edges.item(this.edgeIndex + this._snapshot._edgeNameOffset);
    },

    _type: function()
    {
        return this._edges.item(this.edgeIndex + this._snapshot._edgeTypeOffset);
    }
};

WebInspector.HeapSnapshotEdgeIterator = function(edge)
{
    this.edge = edge;
}

WebInspector.HeapSnapshotEdgeIterator.prototype = {
    first: function()
    {
        this.edge.edgeIndex = 0;
    },

    hasNext: function()
    {
        return this.edge.edgeIndex < this.edge._edges.length;
    },

    get index()
    {
        return this.edge.edgeIndex;
    },

    set index(newIndex)
    {
        this.edge.edgeIndex = newIndex;
    },

    get item()
    {
        return this.edge;
    },

    next: function()
    {
        this.edge.edgeIndex += this.edge._snapshot._edgeFieldsCount;
    }
};

WebInspector.HeapSnapshotRetainerEdge = function(snapshot, retainers, retainerIndex)
{
    this._snapshot = snapshot;
    this._retainers = retainers;
    this.retainerIndex = retainerIndex || 0;
}

WebInspector.HeapSnapshotRetainerEdge.prototype = {
    clone: function()
    {
        return new WebInspector.HeapSnapshotRetainerEdge(this._snapshot, this._retainers, this.retainerIndex);
    },

    get hasStringName()
    {
        return this._edge.hasStringName;
    },

    get isElement()
    {
        return this._edge.isElement;
    },

    get isHidden()
    {
        return this._edge.isHidden;
    },

    get isInternal()
    {
        return this._edge.isInternal;
    },

    get isInvisible()
    {
        return this._edge.isInvisible;
    },

    get isShortcut()
    {
        return this._edge.isShortcut;
    },

    get name()
    {
        return this._edge.name;
    },

    get node()
    {
        return this._node;
    },

    get nodeIndex()
    {
        return this._nodeIndex;
    },

    get retainerIndex()
    {
        return this._retainerIndex;
    },

    set retainerIndex(newIndex)
    {
        if (newIndex !== this._retainerIndex) {
            this._retainerIndex = newIndex;
            this._setupEdge();
        }
    },

    _setupEdge: function()
    {
        var globalEdgeIndex = this._retainers.item(this._retainerIndex);
        this._nodeIndex = this._snapshot._findNearestNodeIndex(globalEdgeIndex);
        this._node = new WebInspector.HeapSnapshotNode(this._snapshot, this._nodeIndex);
        var edgeIndex = globalEdgeIndex - this._nodeIndex - this._snapshot._firstEdgeOffset;
        this._edge = new WebInspector.HeapSnapshotEdge(this._snapshot, this._node.rawEdges, edgeIndex);
    },

    toString: function()
    {
        return this._edge.toString();
    },

    get type()
    {
        return this._edge.type;
    }
}

WebInspector.HeapSnapshotRetainerEdgeIterator = function(retainer)
{
    this.retainer = retainer;
}

WebInspector.HeapSnapshotRetainerEdgeIterator.prototype = {
    first: function()
    {
        this.retainer.retainerIndex = 0;
    },

    hasNext: function()
    {
        return this.retainer.retainerIndex < this.retainer._retainers.length;
    },

    get index()
    {
        return this.retainer.retainerIndex;
    },

    set index(newIndex)
    {
        this.retainer.retainerIndex = newIndex;
    },

    get item()
    {
        return this.retainer;
    },

    next: function()
    {
        ++this.retainer.retainerIndex;
    }
};

WebInspector.HeapSnapshotNode = function(snapshot, nodeIndex)
{
    this._snapshot = snapshot;
    this._firstNodeIndex = nodeIndex;
    this.nodeIndex = nodeIndex;
}

WebInspector.HeapSnapshotNode.prototype = {
    get className()
    {
        switch (this.type) {
        case "hidden":
            return WebInspector.UIString("(system)");
        case "object":
            return this.name;
        case "native": {
            var entitiesCountPos = this.name.indexOf("/");
            return entitiesCountPos !== -1 ? this.name.substring(0, entitiesCountPos).trimRight() : this.name;
        }
        case "code":
            return WebInspector.UIString("(compiled code)");
        default:
            return "(" + this.type + ")";
        }
    },

    get dominatorIndex()
    {
        return this._nodes[this.nodeIndex + this._snapshot._dominatorOffset];
    },

    get edges()
    {
        return new WebInspector.HeapSnapshotEdgeIterator(new WebInspector.HeapSnapshotEdge(this._snapshot, this.rawEdges));
    },

    get edgesCount()
    {
        return this._nodes[this.nodeIndex + this._snapshot._edgesCountOffset];
    },

    get id()
    {
        return this._nodes[this.nodeIndex + this._snapshot._nodeIdOffset];
    },

    get instancesCount()
    {
        return this._nodes[this.nodeIndex + this._snapshot._nodeInstancesCountOffset];
    },

    get isHidden()
    {
        return this._type() === this._snapshot._nodeHiddenType;
    },

    get isRoot()
    {
        return this.nodeIndex === this._snapshot._rootNodeIndex;
    },

    get name()
    {
        return this._snapshot._strings[this._name()];
    },

    get rawEdges()
    {
        var firstEdgeIndex = this._firstEdgeIndex();
        return new WebInspector.HeapSnapshotArraySlice(this._snapshot, "_nodes", firstEdgeIndex, firstEdgeIndex + this.edgesCount * this._snapshot._edgeFieldsCount);
    },

    get retainedSize()
    {
        return this._nodes[this.nodeIndex + this._snapshot._nodeRetainedSizeOffset];
    },

    get retainers()
    {
        return new WebInspector.HeapSnapshotRetainerEdgeIterator(new WebInspector.HeapSnapshotRetainerEdge(this._snapshot, this._snapshot._retainersForNode(this)));
    },

    get selfSize()
    {
        return this._nodes[this.nodeIndex + this._snapshot._nodeSelfSizeOffset];
    },

    get type()
    {
        return this._snapshot._nodeTypes[this._type()];
    },

    _name: function()
    {
        return this._nodes[this.nodeIndex + this._snapshot._nodeNameOffset];
    },

    get _nodes()
    {
        return this._snapshot._nodes;
    },

    _firstEdgeIndex: function()
    {
        return this.nodeIndex + this._snapshot._firstEdgeOffset;
    },

    get _nextNodeIndex()
    {
        return this._firstEdgeIndex() + this.edgesCount * this._snapshot._edgeFieldsCount;
    },

    _type: function()
    {
        return this._nodes[this.nodeIndex + this._snapshot._nodeTypeOffset];
    }
};

WebInspector.HeapSnapshotNodeIterator = function(node)
{
    this.node = node;
}

WebInspector.HeapSnapshotNodeIterator.prototype = {
    first: function()
    {
        this.node.nodeIndex = this.node._firstNodeIndex;
    },

    hasNext: function()
    {
        return this.node.nodeIndex < this.node._nodes.length;
    },

    get index()
    {
        return this.node.nodeIndex;
    },

    set index(newIndex)
    {
        this.node.nodeIndex = newIndex;
    },

    get item()
    {
        return this.node;
    },

    next: function()
    {
        this.node.nodeIndex = this.node._nextNodeIndex;
    }
}

WebInspector.HeapSnapshot = function(profile)
{
    this.uid = profile.snapshot.uid;
    this._nodes = profile.nodes;
    this._strings = profile.strings;

    this._init();
}

WebInspector.HeapSnapshot.prototype = {
    _init: function()
    {
        this._metaNodeIndex = 0;
        this._rootNodeIndex = 1;
        var meta = this._nodes[this._metaNodeIndex];
        this._nodeTypeOffset = meta.fields.indexOf("type");
        this._nodeNameOffset = meta.fields.indexOf("name");
        this._nodeIdOffset = meta.fields.indexOf("id");
        this._nodeInstancesCountOffset = this._nodeIdOffset;
        this._nodeSelfSizeOffset = meta.fields.indexOf("self_size");
        this._nodeRetainedSizeOffset = meta.fields.indexOf("retained_size");
        this._dominatorOffset = meta.fields.indexOf("dominator");
        this._edgesCountOffset = meta.fields.indexOf("children_count");
        this._firstEdgeOffset = meta.fields.indexOf("children");
        this._nodeTypes = meta.types[this._nodeTypeOffset];
        this._nodeHiddenType = this._nodeTypes.indexOf("hidden");
        var edgesMeta = meta.types[this._firstEdgeOffset];
        this._edgeFieldsCount = edgesMeta.fields.length;
        this._edgeTypeOffset = edgesMeta.fields.indexOf("type");
        this._edgeNameOffset = edgesMeta.fields.indexOf("name_or_index");
        this._edgeToNodeOffset = edgesMeta.fields.indexOf("to_node");
        this._edgeTypes = edgesMeta.types[this._edgeTypeOffset];
        this._edgeElementType = this._edgeTypes.indexOf("element");
        this._edgeHiddenType = this._edgeTypes.indexOf("hidden");
        this._edgeInternalType = this._edgeTypes.indexOf("internal");
        this._edgeShortcutType = this._edgeTypes.indexOf("shortcut");
        this._edgeInvisibleType = this._edgeTypes.length;
        this._edgeTypes.push("invisible");

        this._markInvisibleEdges();
    },

    dispose: function()
    {
        delete this._nodes;
        delete this._strings;
        delete this._retainers;
        delete this._retainerIndex;
        delete this._nodeIndex;
        if (this._aggregates) {
            delete this._aggregates;
            this._aggregatesIndexesSorted = false;
        }
        delete this._baseNodeIds;
    },

    get _allNodes()
    {
        return new WebInspector.HeapSnapshotNodeIterator(this.rootNode);
    },

    get nodeCount()
    {
        if (this._nodeCount)
            return this._nodeCount;

        this._nodeCount = 0;
        for (var iter = this._allNodes; iter.hasNext(); iter.next())
            ++this._nodeCount;
        return this._nodeCount;
    },

    nodeFieldValuesByIndex: function(fieldName, indexes)
    {
        var node = new WebInspector.HeapSnapshotNode(this);
        var result = new Array(indexes.length);
        for (var i = 0, l = indexes.length; i < l; ++i) {
            node.nodeIndex = indexes[i];
            result[i] = node[fieldName];
        }
        return result;
    },

    get rootNode()
    {
        return new WebInspector.HeapSnapshotNode(this, this._rootNodeIndex);
    },

    get rootNodeIndex()
    {
        return this._rootNodeIndex;
    },

    get totalSize()
    {
        return this.rootNode.retainedSize;
    },

    _retainersForNode: function(node)
    {
        if (!this._retainers)
            this._buildRetainers();

        var retIndexFrom = this._getRetainerIndex(node.nodeIndex);
        var retIndexTo = this._getRetainerIndex(node._nextNodeIndex);
        return new WebInspector.HeapSnapshotArraySlice(this, "_retainers", retIndexFrom, retIndexTo);
    },

    aggregates: function(sortedIndexes)
    {
        if (!this._aggregates)
            this._buildAggregates();
        if (sortedIndexes && !this._aggregatesIndexesSorted)
            this._sortAggregateIndexes();
        return this._aggregates;
    },

    _buildRetainers: function()
    {
        if (!this._nodeIndex)
            this._buildNodeIndex();

        this._retainerIndex = new Array(this._nodeIndex.length);
        for (var i = 0, l = this._retainerIndex.length; i < l; ++i)
            this._retainerIndex[i] = 0;
        for (var nodesIter = this._allNodes; nodesIter.hasNext(); nodesIter.next()) {
            var node = nodesIter.node;
            for (var edgesIter = node.edges; edgesIter.hasNext(); edgesIter.next()) {
                var edge = edgesIter.edge;
                var nodeIndex = edge.nodeIndex;
                var position = this._findNodePositionInIndex(nodeIndex);
                ++this._retainerIndex[position];
            }
        }
        var retainerCount = 0;
        for (i = 0, l = this._retainerIndex.length; i < l; ++i)
            retainerCount += this._retainerIndex[i];
        this._retainers = new Array(retainerCount + 1);
        var retainerPosition = 0;
        for (i = 0, l = this._retainerIndex.length; i < l; ++i) {
            retainerCount = this._retainers[retainerPosition] = this._retainerIndex[i];
            this._retainerIndex[i] = retainerPosition;
            retainerPosition += retainerCount;
        }
        for (nodesIter = this._allNodes; nodesIter.hasNext(); nodesIter.next()) {
            var node = nodesIter.node;
            for (var edgesIter = node.edges; edgesIter.hasNext(); edgesIter.next()) {
                var edge = edgesIter.edge;
                var nodeIndex = edge.nodeIndex;
                var retIndex = this._getRetainerIndex(nodeIndex);
                var idx = retIndex + (--this._retainers[retIndex]);
                this._retainers[idx] = node.nodeIndex + this._firstEdgeOffset + edge.edgeIndex;
            }
        }
    },

    _buildAggregates: function()
    {
        this._aggregates = {};
        for (var iter = this._allNodes; iter.hasNext(); iter.next()) {
            var node = iter.node;
            var className = node.className;
            var nameMatters = node.type === "object" || node.type === "native";
            if (node.type !== "native" && node.selfSize === 0)
                continue;
            if (!(className in this._aggregates))
                this._aggregates[className] = { count: 0, self: 0, maxRet: 0, type: node.type, name: nameMatters ? node.name : null, idxs: [] };
            var clss = this._aggregates[className];
            ++clss.count;
            clss.self += node.selfSize;
            if (node.retainedSize > clss.maxRet)
                clss.maxRet = node.retainedSize;
            clss.idxs.push(node.nodeIndex);
        }
        // Shave off provisionally allocated space.
        for (var className in this._aggregates)
            this._aggregates[className].idxs = this._aggregates[className].idxs.slice(0);
    },

    _sortAggregateIndexes: function()
    {
        var nodeA = new WebInspector.HeapSnapshotNode(this);
        var nodeB = new WebInspector.HeapSnapshotNode(this);
        for (var clss in this._aggregates)
            this._aggregates[clss].idxs.sort(
                function(idxA, idxB) {
                    nodeA.nodeIndex = idxA;
                    nodeB.nodeIndex = idxB;
                    return nodeA.id < nodeB.id ? -1 : 1;
                });

        this._aggregatesIndexesSorted = true;
    },

    _buildNodeIndex: function()
    {
        var count = 0;
        for (var nodesIter = this._allNodes; nodesIter.hasNext(); nodesIter.next(), ++count);
        this._nodeIndex = new Array(count + 1);
        count = 0;
        for (nodesIter = this._allNodes; nodesIter.hasNext(); nodesIter.next(), ++count)
            this._nodeIndex[count] = nodesIter.index;
        this._nodeIndex[count] = this._nodes.length;
    },

    _findNodePositionInIndex: function(index)
    {
        return binarySearch(index, this._nodeIndex, this._numbersComparator);
    },

    _findNearestNodeIndex: function(index)
    {
        var result = this._findNodePositionInIndex(index);
        if (result < 0) {
            result = -result - 1;
            nodeIndex = this._nodeIndex[result];
            // Binary search can return either maximum lower value, or minimum higher value.
            if (nodeIndex > index)
                nodeIndex = this._nodeIndex[result - 1];
        } else
            var nodeIndex = this._nodeIndex[result];
        return nodeIndex;
    },

    _getRetainerIndex: function(nodeIndex)
    {
        var nodePosition = this._findNodePositionInIndex(nodeIndex);
        return this._retainerIndex[nodePosition];
    },
  
    _markInvisibleEdges: function()
    {
        // Mark hidden edges of global objects as invisible.
        // FIXME: This is a temporary measure. Normally, we should
        // really hide all hidden nodes.
        for (var iter = this.rootNode.edges; iter.hasNext(); iter.next()) {
            var edge = iter.edge;
            if (!edge.isShortcut)
                continue;
            var node = edge.node;
            var propNames = {};
            for (var innerIter = node.edges; innerIter.hasNext(); innerIter.next()) {
                var globalObjEdge = innerIter.edge;
                if (globalObjEdge.isShortcut)
                    propNames[globalObjEdge._nameOrIndex] = true;
            }
            for (innerIter.first(); innerIter.hasNext(); innerIter.next()) {
                var globalObjEdge = innerIter.edge;
                if (!globalObjEdge.isShortcut
                    && globalObjEdge.node.isHidden
                    && globalObjEdge._hasStringName
                    && (globalObjEdge._nameOrIndex in propNames))
                    this._nodes[globalObjEdge._edges._start + globalObjEdge.edgeIndex + this._edgeTypeOffset] = this._edgeInvisibleType;
            }
        }
    },

    _numbersComparator: function(a, b)
    {
        return a < b ? -1 : (a > b ? 1 : 0);
    },

    baseSnapshotHasNode: function(baseSnapshotId, className, nodeId)
    {
        return this._baseNodeIds[baseSnapshotId][className].binaryIndexOf(nodeId, this._numbersComparator) !== -1;
    },

    pushBaseIds: function(baseSnapshotId, className, nodeIds)
    {
        if (!this._baseNodeIds)
            this._baseNodeIds = [];
        if (!this._baseNodeIds[baseSnapshotId])
            this._baseNodeIds[baseSnapshotId] = {};
        this._baseNodeIds[baseSnapshotId][className] = nodeIds;
    },

    createDiff: function(className)
    {
        return new WebInspector.HeapSnapshotsDiff(this, className);
    },

    _parseFilter: function(filter)
    {
        if (!filter)
            return null;
        var parsedFilter = eval("(function(){return " + filter + "})()");
        return parsedFilter.bind(this);
    },

    createEdgesProvider: function(nodeIndex, filter)
    {
        return new WebInspector.HeapSnapshotEdgesProvider(this, nodeIndex, this._parseFilter(filter));
    },

    createNodesProvider: function(filter)
    {
        return new WebInspector.HeapSnapshotNodesProvider(this, this._parseFilter(filter));
    },

    createNodesProviderForClass: function(className)
    {
        return new WebInspector.HeapSnapshotNodesProvider(this, null, className);
    },

    createPathFinder: function(targetNodeIndex, skipHidden)
    {
        return new WebInspector.HeapSnapshotPathFinder(this, targetNodeIndex, skipHidden);
    },

    updateStaticData: function()
    {
        return {nodeCount: this.nodeCount, rootNodeIndex: this._rootNodeIndex, totalSize: this.totalSize, uid: this.uid};
    }
};

WebInspector.HeapSnapshotFilteredOrderedIterator = function(iterator, filter, iterationOrder)
{
    this._filter = filter;
    this._iterator = iterator;
    this._iterationOrder = iterationOrder ? iterationOrder.slice(0) : null;
    this._position = 0;
    this._lastComparator = null;
}

WebInspector.HeapSnapshotFilteredOrderedIterator.prototype = {
    _createIterationOrder: function()
    {
        this._iterationOrder = [];
        var iterator = this._iterator;
        if (!this._filter) {
            for (iterator.first(); iterator.hasNext(); iterator.next())
                this._iterationOrder.push(iterator.index);
        } else {
            for (iterator.first(); iterator.hasNext(); iterator.next()) {
                if (this._filter(iterator.item))
                    this._iterationOrder.push(iterator.index);
            }
        }
    },

    first: function()
    {
        this._position = 0;
    },

    hasNext: function()
    {
        return this._position < this._iterationOrder.length;
    },

    get isEmpty()
    {
        if (this._iterationOrder)
            return !this._iterationOrder.length;
        var iterator = this._iterator;
        if (!this._filter) {
            iterator.first();
            return !iterator.hasNext();
        }
        for (iterator.first(); iterator.hasNext(); iterator.next())
            if (this._filter(iterator.item)) return false;
        return true;
    },

    get item()
    {
        this._iterator.index = this._iterationOrder[this._position];
        return this._iterator.item;
    },

    get length()
    {
        if (!this._iterationOrder)
            this._createIterationOrder();
        return this._iterationOrder.length;
    },

    next: function()
    {
        ++this._position;
    },

    serializeNextItems: function(count)
    {
        var result = new Array(count);
        for (var i = 0 ; i < count && this.hasNext(); ++i, this.next())
            result[i] = this._serialize(this.item);
        result.length = i;
        result.hasNext = this.hasNext();
        result.totalLength = this.length;
        return result;
    },

    sortAndRewind: function(comparator)
    {
        var result = this.sort(comparator);
        if (result)
            this.first();
        return result;
    }
}

WebInspector.HeapSnapshotFilteredOrderedIterator.prototype.createComparator = function(fieldNames)
{
    return {fieldName1:fieldNames[0], ascending1:fieldNames[1], fieldName2:fieldNames[2], ascending2:fieldNames[3]};
}

WebInspector.HeapSnapshotEdgesProvider = function(snapshot, nodeIndex, filter)
{
    this.snapshot = snapshot;
    var node = new WebInspector.HeapSnapshotNode(snapshot, nodeIndex);
    WebInspector.HeapSnapshotFilteredOrderedIterator.call(this, new WebInspector.HeapSnapshotEdgeIterator(new WebInspector.HeapSnapshotEdge(snapshot, node.rawEdges)), filter);
}

WebInspector.HeapSnapshotEdgesProvider.prototype = {
    _serialize: function(edge)
    {
        return {name: edge.name, node: WebInspector.HeapSnapshotNodesProvider.prototype._serialize(edge.node), nodeIndex: edge.nodeIndex, type: edge.type};
    },

    sort: function(comparator)
    {
        if (this._lastComparator === comparator)
            return false;
        this._lastComparator = comparator;
        var fieldName1 = comparator.fieldName1;
        var fieldName2 = comparator.fieldName2;
        var ascending1 = comparator.ascending1;
        var ascending2 = comparator.ascending2;

        var edgeA = this._iterator.item.clone();
        var edgeB = edgeA.clone();
        var nodeA = new WebInspector.HeapSnapshotNode(this.snapshot);
        var nodeB = new WebInspector.HeapSnapshotNode(this.snapshot);

        function sortByEdgeFieldName(ascending, indexA, indexB)
        {
            edgeA.edgeIndex = indexA;
            edgeB.edgeIndex = indexB;
            if (edgeB.name === "__proto__") return -1;
            if (edgeA.name === "__proto__") return 1;
            var result =
                edgeA.hasStringName === edgeB.hasStringName ?
                (edgeA.name < edgeB.name ? -1 : (edgeA.name > edgeB.name ? 1 : 0)) :
                (edgeA.hasStringName ? -1 : 1);
            return ascending ? result : -result;
        }

        function sortByNodeField(fieldName, ascending, indexA, indexB)
        {
            edgeA.edgeIndex = indexA;
            edgeB.edgeIndex = indexB;
            nodeA.nodeIndex = edgeA.nodeIndex;
            nodeB.nodeIndex = edgeB.nodeIndex;
            var valueA = nodeA[fieldName];
            var valueB = nodeB[fieldName];
            var result = valueA < valueB ? -1 : (valueA > valueB ? 1 : 0);
            return ascending ? result : -result;
        }

        if (!this._iterationOrder)
            this._createIterationOrder();

        function sortByEdgeAndNode(indexA, indexB) {
            var result = sortByEdgeFieldName(ascending1, indexA, indexB);
            if (result === 0)
                result = sortByNodeField(fieldName2, ascending2, indexA, indexB);
            return result;
        }

        function sortByNodeAndEdge(indexA, indexB) {
            var result = sortByNodeField(fieldName1, ascending1, indexA, indexB);
            if (result === 0)
                result = sortByEdgeFieldName(ascending2, indexA, indexB);
            return result;
        }

        function sortByNodeAndNode(indexA, indexB) {
            var result = sortByNodeField(fieldName1, ascending1, indexA, indexB);
            if (result === 0)
                result = sortByNodeField(fieldName2, ascending2, indexA, indexB);
            return result;
        }

        if (fieldName1 === "!edgeName")
            this._iterationOrder.sort(sortByEdgeAndNode);
        else if (fieldName2 === "!edgeName")
            this._iterationOrder.sort(sortByNodeAndEdge);
        else
            this._iterationOrder.sort(sortByNodeAndNode);
        return true;
    }
};

WebInspector.HeapSnapshotEdgesProvider.prototype.__proto__ = WebInspector.HeapSnapshotFilteredOrderedIterator.prototype;

WebInspector.HeapSnapshotNodesProvider = function(snapshot, filter, className)
{
    this.snapshot = snapshot;
    if (!className)
        WebInspector.HeapSnapshotFilteredOrderedIterator.call(this, snapshot._allNodes, filter);
    else
        WebInspector.HeapSnapshotFilteredOrderedIterator.call(this, snapshot._allNodes, null, snapshot.aggregates(false)[className].idxs);
}

WebInspector.HeapSnapshotNodesProvider.prototype = {
    _serialize: function(node)
    {
        return {id: node.id, name: node.name, nodeIndex: node.nodeIndex, retainedSize: node.retainedSize, selfSize: node.selfSize, type: node.type};
    },

    sort: function(comparator)
    {
        if (this._lastComparator === comparator)
            return false;
        this._lastComparator = comparator;
        var fieldName1 = comparator.fieldName1;
        var fieldName2 = comparator.fieldName2;
        var ascending1 = comparator.ascending1;
        var ascending2 = comparator.ascending2;

        var nodeA = new WebInspector.HeapSnapshotNode(this.snapshot);
        var nodeB = new WebInspector.HeapSnapshotNode(this.snapshot);

        function sortByNodeField(fieldName, ascending, indexA, indexB)
        {
            nodeA.nodeIndex = indexA;
            nodeB.nodeIndex = indexB;
            var valueA = nodeA[fieldName];
            var valueB = nodeB[fieldName];
            var result = valueA < valueB ? -1 : (valueA > valueB ? 1 : 0);
            return ascending ? result : -result;
        }

        if (!this._iterationOrder)
            this._createIterationOrder();

        function sortByComparator(indexA, indexB) {
            var result = sortByNodeField(fieldName1, ascending1, indexA, indexB);
            if (result === 0)
                result = sortByNodeField(fieldName2, ascending2, indexA, indexB);
            return result;
        }

        this._iterationOrder.sort(sortByComparator);
        return true;
    }
};

WebInspector.HeapSnapshotNodesProvider.prototype.__proto__ = WebInspector.HeapSnapshotFilteredOrderedIterator.prototype;

WebInspector.HeapSnapshotPathFinder = function(snapshot, targetNodeIndex, skipHidden)
{
    this._snapshot = snapshot;
    this._maxLength = 1;
    this._lengthLimit = 15;
    this._targetNodeIndex = targetNodeIndex;
    this._currentPath = null;
    this._skipHidden = skipHidden;
    this._rootChildren = this._fillRootChildren();
}

WebInspector.HeapSnapshotPathFinder.prototype = {
    findNext: function()
    {
        for (var i = 0; i < 100000; ++i) {
            if (!this._buildNextPath()) {
                if (++this._maxLength >= this._lengthLimit)
                    return null;
                this._currentPath = null;
                if (!this._buildNextPath())
                    return null;
            }
            if (this._isPathFound())
                return {path:this._pathToString(this._currentPath), route:this._pathToRoute(this._currentPath), len:this._currentPath.length};
        }

        return false;
    },

    updateRoots: function(filter)
    {
        if (filter)
            filter = eval("(function(){return " + filter + "})()");
        this._rootChildren = this._fillRootChildren(filter);  
        this._reset();
    },

    _reset: function()
    {
        this._maxLength = 1;
        this._currentPath = null;
    },

    _fillRootChildren: function(filter)
    {
        var result = [];
        for (var iter = this._snapshot.rootNode.edges; iter.hasNext(); iter.next()) {
            if (!filter || filter(iter.edge.node))
                result[iter.edge.nodeIndex] = true;
        }
        return result;
    },

    _appendToCurrentPath: function(iter)
    {
        this._currentPath._cache[this._lastEdge.nodeIndex] = true;
        this._currentPath.push(iter);
    },

    _removeLastFromCurrentPath: function()
    {
        this._currentPath.pop();
        delete this._currentPath._cache[this._lastEdge.nodeIndex];
    },

    _hasInPath: function(nodeIndex)
    {
        return this._targetNodeIndex === nodeIndex
            || !!this._currentPath._cache[nodeIndex];
    },

    _isPathFound: function()
    {
        return this._currentPath.length === this._maxLength
            && this._lastEdge.nodeIndex in this._rootChildren;
    },

    get _lastEdgeIter()
    {
        return this._currentPath[this._currentPath.length - 1];
    },

    get _lastEdge()
    {
        return this._lastEdgeIter.item;
    },

    _skipEdge: function(edge)
    {
        return edge.isInvisible
            || (this._skipHidden && (edge.isHidden || edge.node.isHidden))
            || this._hasInPath(edge.nodeIndex);
    },

    _nextEdgeIter: function()
    {
        var iter = this._lastEdgeIter;
        while (iter.hasNext() && this._skipEdge(iter.item))
            iter.next();
        return iter;
    },

    _buildNextPath: function()
    {
        if (this._currentPath !== null) {
            var iter = this._lastEdgeIter;
            while (true) {
                iter.next();
                iter = this._nextEdgeIter();
                if (iter.hasNext())
                    return true;
                while (true) {
                    if (this._currentPath.length > 1) {
                        this._removeLastFromCurrentPath();
                        iter = this._lastEdgeIter;
                        iter.next();
                        iter = this._nextEdgeIter();
                        if (iter.hasNext()) {
                            while (this._currentPath.length < this._maxLength) {
                                iter = this._nextEdgeIter();
                                if (iter.hasNext())
                                    this._appendToCurrentPath(iter.item.node.retainers);
                                else
                                    return true;
                            }
                            return true;
                        }
                    } else
                        return false;
                }
            }
        } else {
            var node = new WebInspector.HeapSnapshotNode(this._snapshot, this._targetNodeIndex);
            this._currentPath = [node.retainers];
            this._currentPath._cache = {};
            while (this._currentPath.length < this._maxLength) {
                var iter = this._nextEdgeIter();
                if (iter.hasNext())
                    this._appendToCurrentPath(iter.item.node.retainers);
                else
                    break;
            }
            return true;
        }
    },

    _nodeToString: function(node)
    {
        if (node.id === 1)
            return node.name;
        else
            return node.name + "@" + node.id;
    },

    _pathToString: function(path)
    {
        if (!path)
            return "";
        var sPath = [];
        for (var j = 0; j < path.length; ++j)
            sPath.push(path[j].item.toString());
        sPath.push(this._nodeToString(path[path.length - 1].item.node));
        sPath.reverse();
        return sPath.join("");
    },

    _pathToRoute: function(path)
    {
        if (!path)
           return [];
        var route = [];
        route.push(this._targetNodeIndex);
        for (var i = 0; i < path.length; ++i)
            route.push(path[i].item.nodeIndex);
        route.reverse();
        return route;
    }
};

WebInspector.HeapSnapshotsDiff = function(snapshot, className)
{
    this._snapshot = snapshot;
    this._className = className;
};

WebInspector.HeapSnapshotsDiff.prototype = {
    calculate: function()
    {
        var aggregates = this._snapshot.aggregates(true)[this._className];
        var indexes = aggregates ? aggregates.idxs : [];
        var i = 0, l = this._baseIds.length;
        var j = 0, m = indexes.length;
        var diff = { addedCount: 0, removedCount: 0, addedSize: 0, removedSize: 0 };
           
        var nodeB = new WebInspector.HeapSnapshotNode(this._snapshot, indexes[j]);
        while (i < l && j < m) {
            var nodeAId = this._baseIds[i];
            if (nodeAId < nodeB.id) {
                diff.removedCount++;
                diff.removedSize += this._baseSelfSizes[i];
                ++i;
            } else if (nodeAId > nodeB.id) {
                diff.addedCount++;
                diff.addedSize += nodeB.selfSize;
                nodeB.nodeIndex = indexes[++j];                
            } else {
                ++i;
                nodeB.nodeIndex = indexes[++j];                
            }
        }
        while (i < l) {
            diff.removedCount++;
            diff.removedSize += this._baseSelfSizes[i];
            ++i;
        }
        while (j < m) {
            diff.addedCount++;
            diff.addedSize += nodeB.selfSize;
            nodeB.nodeIndex = indexes[++j];
        }
        diff.countDelta = diff.addedCount - diff.removedCount;
        diff.sizeDelta = diff.addedSize - diff.removedSize;
        return diff;
    },

    pushBaseIds: function(baseIds)
    {
        this._baseIds = baseIds;
    },

    pushBaseSelfSizes: function(baseSelfSizes)
    {
        this._baseSelfSizes = baseSelfSizes;
    }
};
