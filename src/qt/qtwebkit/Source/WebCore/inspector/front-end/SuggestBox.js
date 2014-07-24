/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @interface
 */
WebInspector.SuggestBoxDelegate = function()
{
}

WebInspector.SuggestBoxDelegate.prototype = {
    /**
     * @param {string} suggestion
     * @param {boolean=} isIntermediateSuggestion
     */
    applySuggestion: function(suggestion, isIntermediateSuggestion) { },

    /**
     * acceptSuggestion will be always called after call to applySuggestion with isIntermediateSuggestion being equal to false.
     */
    acceptSuggestion: function() { },
}

/**
 * @constructor
 * @param {WebInspector.SuggestBoxDelegate} suggestBoxDelegate
 * @param {Element} anchorElement
 * @param {string=} className
 */
WebInspector.SuggestBox = function(suggestBoxDelegate, anchorElement, className)
{
    this._suggestBoxDelegate = suggestBoxDelegate;
    this._anchorElement = anchorElement;
    this._length = 0;
    this._selectedIndex = -1;
    this._selectedElement = null;
    this._boundOnScroll = this._onScrollOrResize.bind(this, true);
    this._boundOnResize = this._onScrollOrResize.bind(this, false);
    window.addEventListener("scroll", this._boundOnScroll, true);
    window.addEventListener("resize", this._boundOnResize, true);

    this._bodyElement = anchorElement.ownerDocument.body;
    this._element = anchorElement.ownerDocument.createElement("div");
    this._element.className = "suggest-box " + (className || "");
    this._element.addEventListener("mousedown", this._onBoxMouseDown.bind(this), true);
    this.containerElement = this._element.createChild("div", "container");
    this.contentElement = this.containerElement.createChild("div", "content");
}

WebInspector.SuggestBox.prototype = {
    /**
     * @return {boolean}
     */
    visible: function()
    {
        return !!this._element.parentElement;
    },

    /**
     * @param {boolean} isScroll
     * @param {Event} event
     */
    _onScrollOrResize: function(isScroll, event)
    {
        if (isScroll && this._element.isAncestor(event.target) || !this.visible())
            return;
        this._updateBoxPosition(this._anchorBox);
    },

    /**
     * @param {AnchorBox} anchorBox
     */
    _updateBoxPosition: function(anchorBox)
    {
        this._anchorBox = anchorBox;

        // Measure the content element box.
        this.contentElement.style.display = "inline-block";
        document.body.appendChild(this.contentElement);
        this.contentElement.positionAt(0, 0);
        var contentWidth = this.contentElement.offsetWidth;
        var contentHeight = this.contentElement.offsetHeight;
        this.contentElement.style.display = "block";
        this.containerElement.appendChild(this.contentElement);

        const spacer = 6;
        const suggestBoxPaddingX = 21;
        const suggestBoxPaddingY = 2;

        var maxWidth = document.body.offsetWidth - anchorBox.x - spacer;
        var width = Math.min(contentWidth, maxWidth - suggestBoxPaddingX) + suggestBoxPaddingX;
        var paddedWidth = contentWidth + suggestBoxPaddingX;
        var boxX = anchorBox.x;
        if (width < paddedWidth) {
            // Shift the suggest box to the left to accommodate the content without trimming to the BODY edge.
            maxWidth = document.body.offsetWidth - spacer;
            width = Math.min(contentWidth, maxWidth - suggestBoxPaddingX) + suggestBoxPaddingX;
            boxX = document.body.offsetWidth - width;
        }

        var boxY;
        var aboveHeight = anchorBox.y;
        var underHeight = document.body.offsetHeight - anchorBox.y - anchorBox.height;
        var maxHeight = Math.max(underHeight, aboveHeight) - spacer;
        var height = Math.min(contentHeight, maxHeight - suggestBoxPaddingY) + suggestBoxPaddingY;
        if (underHeight >= aboveHeight) {
            // Locate the suggest box under the anchorBox.
            boxY = anchorBox.y + anchorBox.height;
            this._element.removeStyleClass("above-anchor");
            this._element.addStyleClass("under-anchor");
        } else {
            // Locate the suggest box above the anchorBox.
            boxY = anchorBox.y - height;
            this._element.removeStyleClass("under-anchor");
            this._element.addStyleClass("above-anchor");
        }

        this._element.positionAt(boxX, boxY);
        this._element.style.width = width + "px";
        this._element.style.height = height + "px";
    },

    /**
     * @param {Event} event
     */
    _onBoxMouseDown: function(event)
    {
        event.preventDefault();
    },

    hide: function()
    {
        if (!this.visible())
            return;

        this._element.parentElement.removeChild(this._element);
        delete this._selectedElement;
    },

    removeFromElement: function()
    {
        window.removeEventListener("scroll", this._boundOnScroll, true);
        window.removeEventListener("resize", this._boundOnResize, true);
        this.hide();
    },

    /**
     * @param {string=} text
     * @param {boolean=} isIntermediateSuggestion
     */
    _applySuggestion: function(text, isIntermediateSuggestion)
    {
        if (!this.visible() || !(text || this._selectedElement))
            return false;

        var suggestion = text || this._selectedElement.textContent;
        if (!suggestion)
            return false;

        this._suggestBoxDelegate.applySuggestion(suggestion, isIntermediateSuggestion);
        return true;
    },

    /**
     * @param {string=} text
     */
    acceptSuggestion: function(text)
    {
        var result = this._applySuggestion(text, false);
        this.hide();
        if (!result)
            return false;

        this._suggestBoxDelegate.acceptSuggestion();

        return true;
    },

    /**
     * @param {number} shift
     * @param {boolean=} isCircular
     * @return {boolean} is changed
     */
    _selectClosest: function(shift, isCircular)
    {
        if (!this._length)
            return false;

        var index = this._selectedIndex + shift;

        if (isCircular)
            index = (this._length + index) % this._length;
        else
            index = Number.constrain(index, 0, this._length - 1);

        this._selectItem(index);
        this._applySuggestion(undefined, true);
        return true;
    },

    /**
     * @param {string} text
     * @param {Event} event
     */
    _onItemMouseDown: function(text, event)
    {
        this.acceptSuggestion(text);
        event.consume(true);
    },

    /**
     * @param {string} prefix
     * @param {string} text
     */
    _createItemElement: function(prefix, text)
    {
        var element = document.createElement("div");
        element.className = "suggest-box-content-item source-code";
        element.tabIndex = -1;
        if (prefix && prefix.length && !text.indexOf(prefix)) {
            var prefixElement = element.createChild("span", "prefix");
            prefixElement.textContent = prefix;
            var suffixElement = element.createChild("span", "suffix");
            suffixElement.textContent = text.substring(prefix.length);
        } else {
            var suffixElement = element.createChild("span", "suffix");
            suffixElement.textContent = text;
        }
        element.addEventListener("mousedown", this._onItemMouseDown.bind(this, text), false);
        return element;
    },

    /**
     * @param {!Array.<string>} items
     * @param {number} selectedIndex
     * @param {string} userEnteredText
     */
    _updateItems: function(items, selectedIndex, userEnteredText)
    {
        this._length = items.length;
        this.contentElement.removeChildren();

        for (var i = 0; i < items.length; ++i) {
            var item = items[i];
            var currentItemElement = this._createItemElement(userEnteredText, item);
            this.contentElement.appendChild(currentItemElement);
        }

        this._selectedElement = null;
        if (typeof selectedIndex === "number")
            this._selectItem(selectedIndex);
    },

    /**
     * @param {number} index
     */
    _selectItem: function(index)
    {
        if (this._selectedElement)
            this._selectedElement.classList.remove("selected");

        this._selectedIndex = index;
        this._selectedElement = this.contentElement.children[index];
        this._selectedElement.classList.add("selected");

        this._selectedElement.scrollIntoViewIfNeeded(false);
    },

    /**
     * @param {!Array.<string>} completions
     * @param {boolean} canShowForSingleItem
     * @param {string} userEnteredText
     */
    _canShowBox: function(completions, canShowForSingleItem, userEnteredText)
    {
        if (!completions || !completions.length)
            return false;

        if (completions.length > 1)
            return true;

        // Do not show a single suggestion if it is the same as user-entered prefix, even if allowed to show single-item suggest boxes.
        return canShowForSingleItem && completions[0] !== userEnteredText;
    },

    _rememberRowCountPerViewport: function()
    {
        if (!this.contentElement.firstChild)
            return;

        this._rowCountPerViewport = Math.floor(this.containerElement.offsetHeight / this.contentElement.firstChild.offsetHeight);
    },

    /**
     * @param {AnchorBox} anchorBox
     * @param {!Array.<string>} completions
     * @param {number} selectedIndex
     * @param {boolean} canShowForSingleItem
     * @param {string} userEnteredText
     */
    updateSuggestions: function(anchorBox, completions, selectedIndex, canShowForSingleItem, userEnteredText)
    {
        if (this._canShowBox(completions, canShowForSingleItem, userEnteredText)) {
            this._updateItems(completions, selectedIndex, userEnteredText);
            this._updateBoxPosition(anchorBox);
            if (!this.visible())
                this._bodyElement.appendChild(this._element);
            this._rememberRowCountPerViewport();
        } else
            this.hide();
    },

    /**
     * @return {boolean}
     */
    upKeyPressed: function()
    {
        return this._selectClosest(-1, true);
    },

    /**
     * @return {boolean}
     */
    downKeyPressed: function()
    {
        return this._selectClosest(1, true);
    },

    /**
     * @return {boolean}
     */
    pageUpKeyPressed: function()
    {
        return this._selectClosest(-this._rowCountPerViewport, false);
    },

    /**
     * @return {boolean}
     */
    pageDownKeyPressed: function()
    {
        return this._selectClosest(this._rowCountPerViewport, false);
    },

    /**
     * @return {boolean}
     */
    enterKeyPressed: function()
    {
        var hasSelectedItem = !!this._selectedElement;
        this.acceptSuggestion();

        // Report the event as non-handled if there is no selected item,
        // to commit the input or handle it otherwise.
        return hasSelectedItem;
    },

    /**
     * @return {boolean}
     */
    tabKeyPressed: function()
    {
        return this.enterKeyPressed();
    }
}
