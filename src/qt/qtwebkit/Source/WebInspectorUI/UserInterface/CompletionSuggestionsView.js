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

WebInspector.CompletionSuggestionsView = function(delegate)
{
    WebInspector.Object.call(this);

    this._delegate = delegate || null;

    this._selectedIndex = NaN;

    this._element = document.createElement("div");
    this._element.className = WebInspector.CompletionSuggestionsView.StyleClassName;

    this._containerElement = document.createElement("div");
    this._containerElement.className = WebInspector.CompletionSuggestionsView.ContainerElementStyleClassName;
    this._containerElement.addEventListener("mousedown", this._mouseDown.bind(this));
    this._containerElement.addEventListener("mouseup", this._mouseUp.bind(this));
    this._containerElement.addEventListener("click", this._itemClicked.bind(this));
    this._element.appendChild(this._containerElement);
};

WebInspector.CompletionSuggestionsView.StyleClassName = "completion-suggestions";
WebInspector.CompletionSuggestionsView.ContainerElementStyleClassName = "completion-suggestions-container";
WebInspector.CompletionSuggestionsView.ItemElementStyleClassName = "item";
WebInspector.CompletionSuggestionsView.SelectedItemStyleClassName = "selected";

WebInspector.CompletionSuggestionsView.prototype = {
    constructor: WebInspector.CompletionSuggestionsView,

    // Public

    get delegate()
    {
        return this._delegate;
    },

    get visible()
    {
        return !!this._element.parentNode;
    },

    get selectedIndex()
    {
        return this._selectedIndex;
    },

    set selectedIndex(index)
    {
        var selectedItemElement = this._selectedItemElement;
        if (selectedItemElement)
            selectedItemElement.classList.remove(WebInspector.CompletionSuggestionsView.SelectedItemStyleClassName);

        this._selectedIndex = index;

        selectedItemElement = this._selectedItemElement;
        if (!selectedItemElement)
            return;

        selectedItemElement.classList.add(WebInspector.CompletionSuggestionsView.SelectedItemStyleClassName);
        selectedItemElement.scrollIntoViewIfNeeded(false);
    },

    selectNext: function()
    {
        var count = this._containerElement.children.length;

        if (isNaN(this._selectedIndex) || this._selectedIndex === count - 1)
            this.selectedIndex = 0;
        else
            ++this.selectedIndex;

        var selectedItemElement = this._selectedItemElement;
        if (selectedItemElement && this._delegate && typeof this._delegate.completionSuggestionsSelectedCompletion === "function")
            this._delegate.completionSuggestionsSelectedCompletion(this, selectedItemElement.textContent);
    },

    selectPrevious: function()
    {
        if (isNaN(this._selectedIndex) || this._selectedIndex === 0)
            this.selectedIndex = this._containerElement.children.length - 1;
        else
            --this.selectedIndex;

        var selectedItemElement = this._selectedItemElement;
        if (selectedItemElement && this._delegate && typeof this._delegate.completionSuggestionsSelectedCompletion === "function")
            this._delegate.completionSuggestionsSelectedCompletion(this, selectedItemElement.textContent);
    },

    isHandlingClickEvent: function()
    {
        return this._mouseIsDown;
    },

    show: function(anchorBounds)
    {
        // Measure the container so we can know the intrinsic size of the items.
        this._containerElement.style.position = "absolute";
        document.body.appendChild(this._containerElement);

        var containerWidth = this._containerElement.offsetWidth;
        var containerHeight = this._containerElement.offsetHeight;

        this._containerElement.removeAttribute("style");
        this._element.appendChild(this._containerElement);

        // Lay out the suggest-box relative to the anchorBounds.
        const margin = 10;
        const horizontalPadding = 22;
        const absoluteMaximumHeight = 160;

        var x = anchorBounds.origin.x;
        var y = anchorBounds.origin.y + anchorBounds.size.height;

        var maximumWidth = window.innerWidth - anchorBounds.origin.x - margin;
        var width = Math.min(containerWidth, maximumWidth - horizontalPadding) + horizontalPadding;
        var paddedWidth = containerWidth + horizontalPadding;

        if (width < paddedWidth) {
            // Shift the suggest box to the left to accommodate the content without trimming to the BODY edge.
            maximumWidth = window.innerWidth - margin;
            width = Math.min(containerWidth, maximumWidth - horizontalPadding) + horizontalPadding;
            x = document.body.offsetWidth - width;
        }

        var aboveHeight = anchorBounds.origin.y;
        var underHeight = window.innerHeight - anchorBounds.origin.y - anchorBounds.size.height;
        var maximumHeight = Math.min(absoluteMaximumHeight, Math.max(underHeight, aboveHeight) - margin);
        var height = Math.min(containerHeight, maximumHeight);

        // Position the sugesstions above the anchor if there is more room.
        // FIXME: This should always prefer positioning below until there is absolutely no room.
        if (aboveHeight > underHeight)
            y = anchorBounds.origin.y - height;

        this._element.style.left = x + "px";
        this._element.style.top = y + "px";
        this._element.style.width = width + "px";
        this._element.style.height = height + "px";

        document.body.appendChild(this._element);
    },

    hide: function()
    {
        this._element.remove();
    },

    update: function(completions, selectedIndex)
    {
        this._containerElement.removeChildren();

        if (typeof selectedIndex === "number")
            this._selectedIndex = selectedIndex;

        for (var i = 0; i < completions.length; ++i) {
            var itemElement = document.createElement("div");
            itemElement.className = WebInspector.CompletionSuggestionsView.ItemElementStyleClassName;
            itemElement.textContent = completions[i];
            if (i === this._selectedIndex)
                itemElement.classList.add(WebInspector.CompletionSuggestionsView.SelectedItemStyleClassName);
            this._containerElement.appendChild(itemElement);
        }
    },

    // Private

    get _selectedItemElement()
    {
        if (isNaN(this._selectedIndex))
            return null;

        var element = this._containerElement.children[this._selectedIndex] || null;
        console.assert(element);
        return element;
    },

    _mouseDown: function(event)
    {
        if (event.button !== 0)
            return;
        this._mouseIsDown = true;
    },

    _mouseUp: function(event)
    {
        if (event.button !== 0)
            return;
        this._mouseIsDown = false;
    },

    _itemClicked: function(event)
    {
        if (event.button !== 0)
            return;

        var itemElement = event.target.enclosingNodeOrSelfWithClass(WebInspector.CompletionSuggestionsView.ItemElementStyleClassName);
        console.assert(itemElement);
        if (!itemElement)
            return;

        if (this._delegate && typeof this._delegate.completionSuggestionsClickedCompletion === "function")
            this._delegate.completionSuggestionsClickedCompletion(this, itemElement.textContent);
    }
};

WebInspector.CompletionSuggestionsView.prototype.__proto__ = WebInspector.Object.prototype;
