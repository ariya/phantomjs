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

WebInspector.DetailsSectionGroup = function(rows) {
    WebInspector.Object.call(this);

    this._element = document.createElement("div");
    this._element.className = WebInspector.DetailsSectionGroup.StyleClassName;

    this.rows = rows;
};

WebInspector.DetailsSectionGroup.StyleClassName = "group";

WebInspector.DetailsSectionGroup.prototype = {
    constructor: WebInspector.DetailsSectionGroup,

    // Public

    get element()
    {
        return this._element;
    },

    get rows()
    {
        return this._rows;
    },

    set rows(rows)
    {
        this._element.removeChildren();

        this._rows = rows || [];

        for (var i = 0; i < this._rows.length; ++i)
            this._element.appendChild(this._rows[i].element);
    }
};

WebInspector.DetailsSectionGroup.prototype.__proto__ = WebInspector.Object.prototype;
