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

WebInspector.ButtonToolbarItem = function(identifier, toolTip, label, image, suppressEmboss)
{
    WebInspector.ButtonNavigationItem.call(this, identifier, toolTip, image, 32, 32, suppressEmboss);

    console.assert(label);

    this._labelElement = document.createElement("div");
    this._labelElement.className = WebInspector.ButtonToolbarItem.LabelStyleClassName;
    this._element.appendChild(this._labelElement);

    this.label = label;
};

WebInspector.ButtonToolbarItem.LabelStyleClassName = "label";

WebInspector.ButtonToolbarItem.prototype = {
    constructor: WebInspector.ButtonToolbarItem,

    // Public

    get label()
    {
        return this._labelElement.textContent;
    },

    set label(newLabel)
    {
        console.assert(newLabel);
        if (!newLabel)
            return;

        this._labelElement.textContent = newLabel;
    }
};

WebInspector.ButtonToolbarItem.prototype.__proto__ = WebInspector.ButtonNavigationItem.prototype;
