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

WebInspector.LayerTreeManager = function() {
    WebInspector.Object.call(this);

    this._supported = !!window.LayerTreeAgent;

    if (this._supported)
        LayerTreeAgent.enable();
};

WebInspector.LayerTreeManager.Event = {
    LayerTreeDidChange: "layer-tree-did-change"
};

WebInspector.LayerTreeManager.prototype = {
    constructor: WebInspector.LayerTreeManager,

    // Public

    get supported()
    {
        return this._supported;
    },

    layerTreeMutations: function(previousLayers, newLayers)
    {
        console.assert(this.supported);

        if (isEmptyObject(previousLayers)) {
            return {
                preserved: [],
                additions: newLayers,
                removals: []
            };
        }

        function nodeIdForLayer(layer)
        {
            return layer.isGeneratedContent ? layer.pseudoElementId : layer.nodeId;
        }

        var layerIdsInPreviousLayers = [];
        var nodeIdsInPreviousLayers = [];
        var nodeIdsForReflectionsInPreviousLayers = [];

        previousLayers.forEach(function(layer) {
            layerIdsInPreviousLayers.push(layer.layerId);

            var nodeId = nodeIdForLayer(layer);
            if (!nodeId)
                return;

            if (layer.isReflection)
                nodeIdsForReflectionsInPreviousLayers.push(nodeId);
            else
                nodeIdsInPreviousLayers.push(nodeId);
        });

        var preserved = [];
        var additions = [];

        var layerIdsInNewLayers = [];
        var nodeIdsInNewLayers = [];
        var nodeIdsForReflectionsInNewLayers = [];

        newLayers.forEach(function(layer) {
            layerIdsInNewLayers.push(layer.layerId);

            var existed = layerIdsInPreviousLayers.contains(layer.layerId);

            var nodeId = nodeIdForLayer(layer);
            if (!nodeId)
                return;

            if (layer.isReflection) {
                nodeIdsForReflectionsInNewLayers.push(nodeId);
                existed = existed || nodeIdsForReflectionsInPreviousLayers.contains(nodeId);
            } else {
                nodeIdsInNewLayers.push(nodeId);
                existed = existed || nodeIdsInPreviousLayers.contains(nodeId);
            }

            if (existed)
                preserved.push(layer);
            else
                additions.push(layer);
        });

        var removals = previousLayers.filter(function(layer) {
            var nodeId = nodeIdForLayer(layer);

            if (layer.isReflection)
                return !nodeIdsForReflectionsInNewLayers.contains(nodeId);
            else
                return !nodeIdsInNewLayers.contains(nodeId) && !layerIdsInNewLayers.contains(layer.layerId);
        });

        return {
            preserved: preserved,
            additions: additions,
            removals: removals
        };
    },

    layersForNode: function(node, callback)
    {
        console.assert(this.supported);

        LayerTreeAgent.layersForNode(node.id, function(error, layers) {
            if (error || isEmptyObject(layers)) {
                callback(null, []);
                return;
            }

            var firstLayer = layers[0];
            var layerForNode = firstLayer.nodeId === node.id && !firstLayer.isGeneratedContent ? layers.shift() : null;
            callback(layerForNode, layers);
        }.bind(this));
    },

    reasonsForCompositingLayer: function(layer, callback)
    {
        console.assert(this.supported);

        LayerTreeAgent.reasonsForCompositingLayer(layer.layerId, function(error, reasons) {
            callback(error ? 0 : reasons);
        });
    },

    layerTreeDidChange: function()
    {
        this.dispatchEventToListeners(WebInspector.LayerTreeManager.Event.LayerTreeDidChange);
    }
};

WebInspector.LayerTreeManager.prototype.__proto__ = WebInspector.Object.prototype;
