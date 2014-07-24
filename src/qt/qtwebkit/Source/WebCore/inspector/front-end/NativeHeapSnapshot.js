/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
WebInspector.NativeHeapSnapshot = function(profile)
{
    WebInspector.HeapSnapshot.call(this, profile);
    this._nodeObjectType = this._metaNode.type_strings["object"];
    this._edgeWeakType = this._metaNode.type_strings["weak"];
    this._edgeElementType = this._metaNode.type_strings["property"];
}

WebInspector.NativeHeapSnapshot.prototype = {
    createNode: function(nodeIndex)
    {
        return new WebInspector.NativeHeapSnapshotNode(this, nodeIndex);
    },

    createEdge: function(edges, edgeIndex)
    {
        return new WebInspector.NativeHeapSnapshotEdge(this, edges, edgeIndex);
    },

    createRetainingEdge: function(retainedNodeIndex, retainerIndex)
    {
        return new WebInspector.NativeHeapSnapshotRetainerEdge(this, retainedNodeIndex, retainerIndex);
    },

    _markInvisibleEdges: function()
    {
    },

    _calculateFlags: function()
    {
    },

    userObjectsMapAndFlag: function()
    {
        return null;
    },

    images: function()
    {
        var aggregatesByClassName = this.aggregates(false, "allObjects");
        var result = [];
        var cachedImages = aggregatesByClassName["WebCore::CachedImage"];
        function getImageName(node)
        {
            return node.name();
        }
        this._addNodes(cachedImages, getImageName, result);

        var canvases = aggregatesByClassName["WebCore::HTMLCanvasElement"];
        function getCanvasName(node)
        {
            return "HTMLCanvasElement";
        }
        this._addNodes(canvases, getCanvasName, result);
        return result;
    },

    _addNodes: function(classData, nameResolver, result)
    {
        if (!classData)
            return;
        var node = this.rootNode();
        for (var i = 0; i < classData.idxs.length; i++) {
            node.nodeIndex = classData.idxs[i];
            result.push({
                name: nameResolver(node),
                size: node.retainedSize(),
            });
        }
    },

    __proto__: WebInspector.HeapSnapshot.prototype
};

/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotNode}
 * @param {WebInspector.NativeHeapSnapshot} snapshot
 * @param {number=} nodeIndex
 */
WebInspector.NativeHeapSnapshotNode = function(snapshot, nodeIndex)
{
    WebInspector.HeapSnapshotNode.call(this, snapshot, nodeIndex)
}

WebInspector.NativeHeapSnapshotNode.prototype = {
    className: function()
    {
        return this._snapshot._strings[this.classIndex()];
    },

    classIndex: function()
    {
        return this._snapshot._nodes[this.nodeIndex + this._snapshot._nodeTypeOffset];
    },

    id: function()
    {
        return this._snapshot._nodes[this.nodeIndex + this._snapshot._nodeIdOffset];
    },

    name: function()
    {
        return this._snapshot._strings[this._snapshot._nodes[this.nodeIndex + this._snapshot._nodeNameOffset]];;
    },

    serialize: function()
    {
        return {
            id: this.id(),
            name: this.className(),
            displayName: this.name(),
            distance: this.distance(),
            nodeIndex: this.nodeIndex,
            retainedSize: this.retainedSize(),
            selfSize: this.selfSize(),
            type: this._snapshot._nodeObjectType
       };
    },

    isHidden: function()
    {
        return false;
    },

    isSynthetic: function()
    {
        return false;
    },

    __proto__: WebInspector.HeapSnapshotNode.prototype
};

/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotEdge}
 * @param {WebInspector.NativeHeapSnapshot} snapshot
 * @param {Array.<number>} edges
 * @param {number=} edgeIndex
 */
WebInspector.NativeHeapSnapshotEdge = function(snapshot, edges, edgeIndex)
{
    WebInspector.HeapSnapshotEdge.call(this, snapshot, edges, edgeIndex);
}

WebInspector.NativeHeapSnapshotEdge.prototype = {
    clone: function()
    {
        return new WebInspector.NativeHeapSnapshotEdge(this._snapshot, this._edges, this.edgeIndex);
    },

    hasStringName: function()
    {
        return true;
    },

    isHidden: function()
    {
        return false;
    },

    isWeak: function()
    {
        return false;
    },

    isInternal: function()
    {
        return false;
    },

    isInvisible: function()
    {
        return false;
    },

    isShortcut: function()
    {
        return false;
    },

    name: function()
    {
        return this._snapshot._strings[this._nameOrIndex()];
    },

    toString: function()
    {
        return  "NativeHeapSnapshotEdge: " + this.name();
    },

    _nameOrIndex: function()
    {
        return this._edges.item(this.edgeIndex + this._snapshot._edgeNameOffset);
    },

    __proto__: WebInspector.HeapSnapshotEdge.prototype
};


/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotRetainerEdge}
 * @param {WebInspector.NativeHeapSnapshot} snapshot
 */
WebInspector.NativeHeapSnapshotRetainerEdge = function(snapshot, retainedNodeIndex, retainerIndex)
{
    WebInspector.HeapSnapshotRetainerEdge.call(this, snapshot, retainedNodeIndex, retainerIndex);
}

WebInspector.NativeHeapSnapshotRetainerEdge.prototype = {
    clone: function()
    {
        return new WebInspector.NativeHeapSnapshotRetainerEdge(this._snapshot, this._retainedNodeIndex, this.retainerIndex());
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

