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

WebInspector.DOMDetailsSidebarPanel = function(identifier, displayName, singularDisplayName, image, keyboardShortcutKey, element) {
    WebInspector.DetailsSidebarPanel.call(this, identifier, displayName, singularDisplayName, image, keyboardShortcutKey, element);

    this.element.addEventListener("click", this._mouseWasClicked.bind(this), true);

    this._domNode = null;
};

WebInspector.DOMDetailsSidebarPanel.prototype = {
    constructor: WebInspector.DOMDetailsSidebarPanel,

    // Public

    inspect: function(objects)
    {
        // Convert to a single item array if needed.
        if (!(objects instanceof Array))
            objects = [objects];

        var nodeToInspect = null;

        // Iterate over the objects to find a WebInspector.DOMNode to inspect.
        for (var i = 0; i < objects.length; ++i) {
            if (objects[i] instanceof WebInspector.DOMNode) {
                nodeToInspect = objects[i];
                break;
            }
        }

        if (nodeToInspect && !this.supportsDOMNode(nodeToInspect))
            nodeToInspect = null;

        this.domNode = nodeToInspect;

        return !!this._domNode;
    },

    get domNode()
    {
        return this._domNode;
    },

    set domNode(domNode)
    {
        if (domNode === this._domNode)
            return;

        if (this._domNode)
            this.removeEventListeners();

        this._domNode = domNode;

        if (this._domNode)
            this.addEventListeners();

        this.needsRefresh();
    },

    supportsDOMNode: function(nodeToInspect)
    {
        // Implemented by subclasses.
        return true;
    },

    addEventListeners: function()
    {
        // Implemented by subclasses.
    },

    removeEventListeners: function()
    {
        // Implemented by subclasses.
    },

    // Private

    _mouseWasClicked: function(event)
    {
        if (this._domNode && this._domNode.ownerDocument) {
            var mainResource = WebInspector.frameResourceManager.resourceForURL(this._domNode.ownerDocument.documentURL);
            if (mainResource)
                var parentFrame = mainResource.parentFrame;
        }

        WebInspector.handlePossibleLinkClick(event, parentFrame);
    }
};

WebInspector.DOMDetailsSidebarPanel.prototype.__proto__ = WebInspector.DetailsSidebarPanel.prototype;
