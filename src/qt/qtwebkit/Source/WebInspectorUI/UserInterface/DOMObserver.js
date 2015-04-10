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

WebInspector.DOMObserver = function()
{
    WebInspector.Object.call(this);
};

WebInspector.DOMObserver.prototype = {
    constructor: WebInspector.DOMObserver,

    // Events defined by the "DOM" domain (see WebCore/inspector/Inspector.json).

    documentUpdated: function()
    {
        WebInspector.domTreeManager._documentUpdated();
    },

    setChildNodes: function(parentId, payloads)
    {
        WebInspector.domTreeManager._setChildNodes(parentId, payloads);
    },

    attributeModified: function(nodeId, name, value)
    {
        WebInspector.domTreeManager._attributeModified(nodeId, name, value);
    },

    attributeRemoved: function(nodeId, name)
    {
        WebInspector.domTreeManager._attributeRemoved(nodeId, name);
    },

    inlineStyleInvalidated: function(nodeIds)
    {
        WebInspector.domTreeManager._inlineStyleInvalidated(nodeIds);
    },

    characterDataModified: function(nodeId, characterData)
    {
        WebInspector.domTreeManager._characterDataModified(nodeId, characterData);
    },

    childNodeCountUpdated: function(nodeId, childNodeCount)
    {
        WebInspector.domTreeManager._childNodeCountUpdated(nodeId, childNodeCount);
    },

    childNodeInserted: function(parentNodeId, previousNodeId, payload)
    {
        WebInspector.domTreeManager._childNodeInserted(parentNodeId, previousNodeId, payload);
    },

    childNodeRemoved: function(parentNodeId, nodeId)
    {
        WebInspector.domTreeManager._childNodeRemoved(parentNodeId, nodeId);
    },

    shadowRootPushed: function(parentNodeId, nodeId)
    {
        WebInspector.domTreeManager._childNodeInserted(parentNodeId, 0, nodeId);
    },

    shadowRootPopped: function(parentNodeId, nodeId)
    {
        WebInspector.domTreeManager._childNodeRemoved(parentNodeId, nodeId);
    }
};

WebInspector.DOMObserver.prototype.__proto__ = WebInspector.Object.prototype;
