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
 * @constructor
 * @param {WebInspector.ViewportControl.Provider} provider
 */
WebInspector.ViewportControl = function(provider)
{
    this.element = document.createElement("div");
    this.element.className = "fill";
    this.element.style.overflow = "auto";
    this._topGapElement = this.element.createChild("div");
    this._contentElement = this.element.createChild("div");
    this._bottomGapElement = this.element.createChild("div");

    this._provider = provider;
    this.element.addEventListener("scroll", this._onScroll.bind(this), false);
    this._firstVisibleIndex = 0;
    this._lastVisibleIndex = -1;
}

/**
 * @interface
 */
WebInspector.ViewportControl.Provider = function() 
{
}

WebInspector.ViewportControl.Provider.prototype = { 
    /**
     * @return {number}
     */
    itemCount: function() { return 0; },

    /**
     * @param {number} index
     * @return {Element}
     */
    itemElement: function(index) { return null; }
}

WebInspector.ViewportControl.prototype = {
    /**
     * @return {Element}
     */
    contentElement: function()
    {
        return this._contentElement;
    },

    refresh: function()
    {
        if (!this.element.clientHeight)
            return;  // Do nothing for invisible controls.

        var itemCount = this._provider.itemCount();
        if (!itemCount)
            return;

        if (!this._rowHeight) {
            var firstElement = this._provider.itemElement(0);
            this._rowHeight = firstElement.measurePreferredSize(this._contentElement).height;
        }

        var visibleFrom = this.element.scrollTop;
        var visibleTo = visibleFrom + this.element.clientHeight;

        this._firstVisibleIndex = Math.floor(visibleFrom / this._rowHeight);
        this._lastVisibleIndex = Math.min(Math.ceil(visibleTo / this._rowHeight), itemCount) - 1;

        this._topGapElement.style.height = (this._rowHeight * this._firstVisibleIndex) + "px";
        this._bottomGapElement.style.height = (this._rowHeight * (itemCount - this._lastVisibleIndex - 1)) + "px"; 

        this._contentElement.removeChildren();
        for (var i = this._firstVisibleIndex; i <= this._lastVisibleIndex; ++i)
            this._contentElement.appendChild(this._provider.itemElement(i));
    },

    /**
     * @param {Event} event
     */
    _onScroll: function(event)
    {
        this.refresh();
    },

    /**
     * @return {number}
     */
    rowsPerViewport: function()
    {
        return Math.floor(this.element.clientHeight / this._rowHeight);
    },

    /**
     * @return {number}
     */
    firstVisibleIndex: function()
    {
        return this._firstVisibleIndex;
    },

    /**
     * @return {number}
     */
    lastVisibleIndex: function()
    {
        return this._lastVisibleIndex;
    },

    /**
     * @return {?Element}
     */
    renderedElementAt: function(index)
    {
        if (index < this._firstVisibleIndex)
            return null;
        if (index > this._lastVisibleIndex)
            return null;
        return this._contentElement.childNodes[index - this._firstVisibleIndex];
    },

    /**
     * @param {number} index
     * @param {boolean=} makeLast
     */
    scrollItemIntoView: function(index, makeLast)
    {
        if (index > this._firstVisibleIndex && index < this._lastVisibleIndex)
            return;

        if (makeLast)
            this.element.scrollTop = this._rowHeight * (index + 1) - this.element.clientHeight;
        else
            this.element.scrollTop = this._rowHeight * index;
    }
}
