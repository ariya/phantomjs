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

WebInspector.EventListenerSectionGroup = function(eventListener, nodeId)
{
    this._eventListener = eventListener;
    this._nodeId = nodeId;

    var rows = [];
    rows.push(new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Node"), this._nodeTextOrLink()));
    rows.push(new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Function"), this._functionTextOrLink()));
    rows.push(new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Type"), this._type()));

    WebInspector.DetailsSectionGroup.call(this, rows);
}

WebInspector.EventListenerSectionGroup.prototype = {
    constructor: WebInspector.EventListenerSectionGroup,

    _nodeTextOrLink: function()
    {
        var node = this._eventListener.node;
        if (!node)
            return;

        if (node.nodeType() === Node.DOCUMENT_NODE)
            return "document";

        return WebInspector.linkifyNodeReference(node);
    },

    _type: function()
    {
        if (this._eventListener.useCapture)
            return WebInspector.UIString("Capturing");

        if (this._eventListener.isAttribute)
            return WebInspector.UIString("Attribute");

        return WebInspector.UIString("Bubbling");
    },

    _functionTextOrLink: function()
    {
        var match = this._eventListener.handlerBody.match(/function ([^\(]+?)\(/);
        if (match) {
            var anonymous = false;
            var functionName = match[1];
        } else {
            var anonymous = true;
            var functionName = WebInspector.UIString("(anonymous function)");        
        }

        if (!this._eventListener.location)
            return functionName;

        // COMPATIBILITY (iOS 6): In the past scriptId could be a URL. Now it is always a script identifier.
        // So try looking up the resource by URL if a script can't be found directly.
        var scriptIdentifierOrURL = this._eventListener.location.scriptId;
        var sourceCode = WebInspector.debuggerManager.scriptForIdentifier(scriptIdentifierOrURL);
        if (!sourceCode)
            sourceCode = WebInspector.frameResourceManager.resourceForURL(scriptIdentifierOrURL);

        if (!sourceCode)
            return functionName;

        var sourceCodeLocation = sourceCode.createSourceCodeLocation(this._eventListener.location.lineNumber, this._eventListener.location.columnNumber || 0);
        var linkElement = WebInspector.createSourceCodeLocationLink(sourceCodeLocation, anonymous);
        if (anonymous)
            return linkElement;

        var fragment = document.createDocumentFragment();
        fragment.appendChild(linkElement);
        fragment.appendChild(document.createTextNode(functionName));
        return fragment;
    }
}

WebInspector.EventListenerSectionGroup.prototype.__proto__ = WebInspector.DetailsSectionGroup.prototype;
