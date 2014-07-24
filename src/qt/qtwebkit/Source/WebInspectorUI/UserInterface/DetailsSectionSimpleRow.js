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

WebInspector.DetailsSectionSimpleRow = function(label, value) {
    WebInspector.DetailsSectionRow.call(this);

    this.element.classList.add(WebInspector.DetailsSectionSimpleRow.StyleClassName);

    this._labelElement = document.createElement("div");
    this._labelElement.className = WebInspector.DetailsSectionSimpleRow.LabelElementStyleClassName;
    this.element.appendChild(this._labelElement);

    this._valueElement = document.createElement("div");
    this._valueElement.className = WebInspector.DetailsSectionSimpleRow.ValueElementStyleClassName;
    this.element.appendChild(this._valueElement);

    // Workaround for <rdar://problem/12668870> Triple-clicking text within a
    // <div> set to "display: table-cell" selects text outside the cell.
    //
    // On triple-click, adjust the selection range to include only the value
    // element if the selection extends beyond it.
    var valueElementClicked = function(event) {
        event.stopPropagation();

        if (event.detail < 3)
            return;

        var currentSelection = window.getSelection();
        if (!currentSelection)
            return;

        var currentRange = currentSelection.getRangeAt(0);
        if (!currentRange || currentRange.startContainer == currentRange.endContainer)
            return;

        var correctedRange = document.createRange();
        correctedRange.selectNodeContents(event.currentTarget);
        currentSelection.removeAllRanges();
        currentSelection.addRange(correctedRange);
    };
    this._valueElement.addEventListener("click", valueElementClicked);

    this.label = label;
    this.value = value;
};

WebInspector.DetailsSectionSimpleRow.StyleClassName = "simple";
WebInspector.DetailsSectionSimpleRow.DataStyleClassName = "data";
WebInspector.DetailsSectionSimpleRow.EmptyStyleClassName = "empty";
WebInspector.DetailsSectionSimpleRow.LabelElementStyleClassName = "label";
WebInspector.DetailsSectionSimpleRow.ValueElementStyleClassName = "value";

WebInspector.DetailsSectionSimpleRow.prototype = {
    constructor: WebInspector.DetailsSectionSimpleRow,

    // Public

    get label()
    {
        return this._labelElement.textContent;
    },

    set label(label)
    {
        this._labelElement.textContent = label;
    },

    get value()
    {
        return this._value;
    },

    set value(value)
    {
        this._value = value || "";

        if (this._value) {
            this.element.classList.remove(WebInspector.DetailsSectionSimpleRow.EmptyStyleClassName);

            // If the value has space characters that cause word wrapping then we don't need the data class.
            if (/[\s\u200b]/.test(this._value))
                this.element.classList.remove(WebInspector.DetailsSectionSimpleRow.DataStyleClassName);
            else
                this.element.classList.add(WebInspector.DetailsSectionSimpleRow.DataStyleClassName);
        } else {
            this.element.classList.add(WebInspector.DetailsSectionSimpleRow.EmptyStyleClassName);
            this.element.classList.remove(WebInspector.DetailsSectionSimpleRow.DataStyleClassName);
        }

        if (value instanceof Node) {
            this._valueElement.removeChildren();
            this._valueElement.appendChild(this._value);
        } else
            this._valueElement.textContent = this._value;
    }
};

WebInspector.DetailsSectionSimpleRow.prototype.__proto__ = WebInspector.DetailsSectionRow.prototype;
