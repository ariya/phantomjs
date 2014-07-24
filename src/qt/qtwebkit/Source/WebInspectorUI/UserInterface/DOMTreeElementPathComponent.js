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

WebInspector.DOMTreeElementPathComponent = function(domTreeElement, representedObject) {
    var node = domTreeElement.representedObject;

    var title = null;
    var className = null;

    switch (node.nodeType()) {
    case Node.ELEMENT_NODE:
        className = WebInspector.DOMTreeElementPathComponent.DOMElementIconStyleClassName;
        title = WebInspector.displayNameForNode(node);
        break;

    case Node.TEXT_NODE:
        className = WebInspector.DOMTreeElementPathComponent.DOMTextNodeIconStyleClassName;
        title = "\"" + node.nodeValue().trimEnd(32) + "\"";
        break

    case Node.COMMENT_NODE:
        className = WebInspector.DOMTreeElementPathComponent.DOMCommentIconStyleClassName;
        title = "<!--" + node.nodeValue().trimEnd(32) + "-->";
        break;

    case Node.DOCUMENT_TYPE_NODE:
        className = WebInspector.DOMTreeElementPathComponent.DOMDocumentTypeIconStyleClassName;
        title = "<!DOCTYPE>";
        break;

    case Node.DOCUMENT_NODE:
        className = WebInspector.DOMTreeElementPathComponent.DOMDocumentIconStyleClassName;
        title = node.nodeNameInCorrectCase();
        break;

    case Node.CDATA_SECTION_NODE:
        className = WebInspector.DOMTreeElementPathComponent.DOMCharacterDataIconStyleClassName;
        title = "<![CDATA[" + node.trimEnd(32) + "]]>";
        break;

    case Node.DOCUMENT_FRAGMENT_NODE:
        // FIXME: At some point we might want a different icon for this.
        // <rdar://problem/12800950> Need icon for DOCUMENT_FRAGMENT_NODE
        className = WebInspector.DOMTreeElementPathComponent.DOMDocumentTypeIconStyleClassName;
        if (node.isInShadowTree())
            title = WebInspector.UIString("Shadow Content");
        else
            title = WebInspector.displayNameForNode(node);
        break;

    default:
        console.error("Unknown DOM node type: ", node.nodeType());
        className = WebInspector.DOMTreeElementPathComponent.DOMNodeIconStyleClassName;
        title = node.nodeNameInCorrectCase();
    }

    WebInspector.HierarchicalPathComponent.call(this, title, className, representedObject || domTreeElement.representedObject);

    this._domTreeElement = domTreeElement;
};

WebInspector.DOMTreeElementPathComponent.DOMElementIconStyleClassName = "dom-element-icon";
WebInspector.DOMTreeElementPathComponent.DOMTextNodeIconStyleClassName = "dom-text-node-icon";
WebInspector.DOMTreeElementPathComponent.DOMCommentIconStyleClassName = "dom-comment-icon";
WebInspector.DOMTreeElementPathComponent.DOMDocumentTypeIconStyleClassName = "dom-document-type-icon";
WebInspector.DOMTreeElementPathComponent.DOMDocumentIconStyleClassName = "dom-document-icon";
WebInspector.DOMTreeElementPathComponent.DOMCharacterDataIconStyleClassName = "dom-character-data-icon";
WebInspector.DOMTreeElementPathComponent.DOMNodeIconStyleClassName = "dom-node-icon";

WebInspector.DOMTreeElementPathComponent.prototype = {
    constructor: WebInspector.DOMTreeElementPathComponent,

    // Public

    get domTreeElement()
    {
        return this._domTreeElement;
    },

    get previousSibling()
    {
        if (!this._domTreeElement.previousSibling)
            return null;
        return new WebInspector.DOMTreeElementPathComponent(this._domTreeElement.previousSibling);
    },

    get nextSibling()
    {
        if (!this._domTreeElement.nextSibling)
            return null;
        if (this._domTreeElement.nextSibling.isCloseTag())
            return null;
        return new WebInspector.DOMTreeElementPathComponent(this._domTreeElement.nextSibling);
    },

    // Protected

    mouseOver: function()
    {
        var nodeId = this._domTreeElement.representedObject.id;
        WebInspector.domTreeManager.highlightDOMNode(nodeId);
    },

    mouseOut: function()
    {
        WebInspector.domTreeManager.hideDOMNodeHighlight();
    }
};

WebInspector.DOMTreeElementPathComponent.prototype.__proto__ = WebInspector.HierarchicalPathComponent.prototype;
