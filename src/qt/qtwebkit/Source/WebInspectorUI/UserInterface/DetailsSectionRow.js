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

WebInspector.DetailsSectionRow = function(emptyMessage) {
    WebInspector.Object.call(this);

    this._element = document.createElement("div");
    this._element.className = WebInspector.DetailsSectionRow.StyleClassName;

    this._emptyMessage = emptyMessage || "";
};

WebInspector.DetailsSectionRow.StyleClassName = "row";
WebInspector.DetailsSectionRow.EmptyStyleClassName = "empty";

WebInspector.DetailsSectionRow.prototype = {
    constructor: WebInspector.DetailsSectionRow,

    // Public

    get element()
    {
        return this._element;
    },

    get emptyMessage()
    {
        return this._emptyMessage;
    },

    set emptyMessage(emptyMessage)
    {
        this._emptyMessage = emptyMessage || "";

        if (!this.childNodes.length)
            this.showEmptyMessage();
    },

    showEmptyMessage: function()
    {
        this.element.classList.add(WebInspector.DetailsSectionRow.EmptyStyleClassName);

        if (this._emptyMessage instanceof Node) {
            this.element.removeChildren();
            this.element.appendChild(this._emptyMessage);
        } else
            this.element.textContent = this._emptyMessage;
    },

    hideEmptyMessage: function()
    {
        this.element.classList.remove(WebInspector.DetailsSectionRow.EmptyStyleClassName);
        this.element.removeChildren();
    }
};

WebInspector.DetailsSectionRow.prototype.__proto__ = WebInspector.Object.prototype;
