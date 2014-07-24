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

WebInspector.FilterBar = function(element) {
    WebInspector.Object.call(this);

    this._element = element || document.createElement("div");
    this._element.classList.add(WebInspector.FilterBar.StyleClassName);

    this._inputField = document.createElement("input");
    this._inputField.type = "search";
    this._inputField.spellcheck = false;
    this._inputField.incremental = true;
    this._inputField.addEventListener("search", this._inputFieldChanged.bind(this), false);
    this._element.appendChild(this._inputField);
};

WebInspector.Object.addConstructorFunctions(WebInspector.FilterBar);

WebInspector.FilterBar.StyleClassName = "filter-bar";

WebInspector.FilterBar.Event = {
    TextFilterDidChange: "filter-bar-text-filter-did-change"
};

WebInspector.FilterBar.prototype = {
    constructor: WebInspector.FilterBar,

    // Public

    get element()
    {
        return this._element;
    },

    get placeholder()
    {
        return this._inputField.getAttribute("placeholder");
    },

    set placeholder(text)
    {
        this._inputField.setAttribute("placeholder", text);
    },

    get inputField()
    {
        return this._inputField;
    },

    get filters()
    {
        return {text: this._inputField.value};
    },

    set filters(filters)
    {
        filters = filters || {};

        var oldTextValue = this._inputField.value;
        this._inputField.value = filters.text || "";
        if (oldTextValue !== this._inputField.value)
            this._inputFieldChanged();
    },

    hasActiveFilters: function()
    {
        if (this._inputField.value)
            return true;
        return false;
    },

    // Private

    _inputFieldChanged: function(event)
    {
        this.dispatchEventToListeners(WebInspector.FilterBar.Event.TextFilterDidChange);
    }
};

WebInspector.FilterBar.prototype.__proto__ = WebInspector.Object.prototype;
