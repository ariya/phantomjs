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

WebInspector.ResourceSearchMatchObject = function(resource, lineText, searchTerm, textRange)
{
    console.assert(resource instanceof WebInspector.Resource);

    WebInspector.Object.call(this);

    this._resource = resource;
    this._lineText = lineText;
    this._searchTerm = searchTerm;
    this._sourceCodeTextRange = resource.createSourceCodeTextRange(textRange);
};

WebInspector.ResourceSearchMatchObject.ResourceMatchIconStyleClassName = "resource-match-icon";

WebInspector.ResourceSearchMatchObject.prototype = {
    constructor: WebInspector.ResourceSearchMatchObject,

    get resource()
    {
        return this._resource;
    },

    get title()
    {
        return this._lineText;
    },

    get className()
    {
        return WebInspector.ResourceSearchMatchObject.ResourceMatchIconStyleClassName;
    },

    get searchTerm()
    {
        return this._searchTerm;
    },

    get sourceCodeTextRange()
    {
        return this._sourceCodeTextRange;
    }
};

WebInspector.ResourceSearchMatchObject.prototype.__proto__ = WebInspector.Object.prototype;
