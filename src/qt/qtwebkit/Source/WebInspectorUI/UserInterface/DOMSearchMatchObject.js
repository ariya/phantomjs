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

WebInspector.DOMSearchMatchObject = function(resource, domNode, title, searchTerm, textRange)
{
    console.assert(resource instanceof WebInspector.Resource);
    console.assert(domNode instanceof WebInspector.DOMNode);

    WebInspector.Object.call(this);

    this._resource = resource;
    this._domNode = domNode;
    this._title = title;
    this._searchTerm = searchTerm;
    this._sourceCodeTextRange = resource.createSourceCodeTextRange(textRange);
};

WebInspector.DOMSearchMatchObject.DOMMatchElementIconStyleClassName = "dom-match-element-icon";
WebInspector.DOMSearchMatchObject.DOMMatchTextNodeIconStyleClassName = "dom-match-text-node-icon";
WebInspector.DOMSearchMatchObject.DOMMatchCommentIconStyleClassName = "dom-match-comment-icon";
WebInspector.DOMSearchMatchObject.DOMMatchDocumentTypeIconStyleClassName = "dom-match-document-type-icon";
WebInspector.DOMSearchMatchObject.DOMMatchCharacterDataIconStyleClassName = "dom-match-character-data-icon";
WebInspector.DOMSearchMatchObject.DOMMatchNodeIconStyleClassName = "dom-match-node-icon";

WebInspector.DOMSearchMatchObject.prototype = {
    constructor: WebInspector.DOMSearchMatchObject,

    // Public

    get domNode()
    {
        return this._domNode;
    },

    get title()
    {
        return this._title;     
    },

    get className()
    {
        if (!this._className)
            this._className = this._generateClassName();

        return this._className;
    },

    get searchTerm()
    {
        return this._searchTerm;
    },

    get sourceCodeTextRange()
    {
        return this._sourceCodeTextRange;
    },

    // Private
    
    _generateClassName: function()
    {
        switch (this._domNode.nodeType()) {
        case Node.ELEMENT_NODE:
            return WebInspector.DOMSearchMatchObject.DOMMatchElementIconStyleClassName;

        case Node.TEXT_NODE:
            return WebInspector.DOMSearchMatchObject.DOMMatchTextNodeIconStyleClassName;

        case Node.COMMENT_NODE:
            return WebInspector.DOMSearchMatchObject.DOMMatchCommentIconStyleClassName;

        case Node.DOCUMENT_TYPE_NODE:
            return WebInspector.DOMSearchMatchObject.DOMMatchDocumentTypeIconStyleClassName;

        case Node.CDATA_SECTION_NODE:
            return WebInspector.DOMSearchMatchObject.DOMMatchCharacterDataIconStyleClassName;

        default:
            console.error("Unknown DOM node type: ", node.nodeType());
            return WebInspector.DOMSearchMatchObject.DOMMatchNodeIconStyleClassName;
        }
    }
};

WebInspector.DOMSearchMatchObject.titleForDOMNode = function(domNode)
{
    switch (domNode.nodeType()) {
    case Node.ELEMENT_NODE:
        var title = "<" + domNode.nodeNameInCorrectCase();
        
        for (var i = 0; i < domNode.attributes().length; ++i) {
            title += " " + domNode.attributes()[i].name;
            if (domNode.attributes()[i].value.length)
                title += "=\"" + domNode.attributes()[i].value + "\"";
        }

        return title + ">";

    case Node.TEXT_NODE:
        return "\"" + domNode.nodeValue() + "\"";

    case Node.COMMENT_NODE:
        return "<!--" + domNode.nodeValue() + "-->";

    case Node.DOCUMENT_TYPE_NODE:
        var title = "<!DOCTYPE " + domNode.nodeName();
        if (domNode.publicId) {
            title += " PUBLIC \"" + domNode.publicId + "\"";
            if (node.systemId)
                title += " \"" + domNode.systemId + "\"";
        } else if (domNode.systemId)
            title += " SYSTEM \"" + domNode.systemId + "\"";

        if (domNode.internalSubset)
            title += " [" + domNode.internalSubset + "]";

        return title + ">";

    case Node.CDATA_SECTION_NODE:
        return "<![CDATA[" + domNode + "]]>";

    default:
        console.error("Unknown DOM node type: ", domNode.nodeType());
        return domNode.nodeNameInCorrectCase();
    }
}

WebInspector.DOMSearchMatchObject.prototype.__proto__ = WebInspector.Object.prototype;
