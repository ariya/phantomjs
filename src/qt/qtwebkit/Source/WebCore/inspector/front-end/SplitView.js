/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL GOOGLE INC.
 * OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.View}
 * @param {boolean} isVertical
 * @param {string=} sidebarSizeSettingName
 * @param {number=} defaultSidebarWidth
 * @param {number=} defaultSidebarHeight
 */
WebInspector.SplitView = function(isVertical, sidebarSizeSettingName, defaultSidebarWidth, defaultSidebarHeight)
{
    WebInspector.View.call(this);

    this.registerRequiredCSS("splitView.css");

    this.element.className = "split-view";

    this._firstElement = this.element.createChild("div", "split-view-contents scroll-target split-view-contents-first");
    this._secondElement = this.element.createChild("div", "split-view-contents scroll-target split-view-contents-second");

    this._resizerElement = this.element.createChild("div", "split-view-resizer");
    this.installResizer(this._resizerElement);
    this._resizable = true;

    this._savedSidebarWidth = defaultSidebarWidth || 200;
    this._savedSidebarHeight = defaultSidebarHeight || this._savedSidebarWidth;

    if (0 < this._savedSidebarWidth && this._savedSidebarWidth < 1 &&
        0 < this._savedSidebarHeight && this._savedSidebarHeight < 1)
        this._useFraction = true;
    
    this._sidebarSizeSettingName = sidebarSizeSettingName;

    this.setSecondIsSidebar(true);

    this._innerSetVertical(isVertical);
}

WebInspector.SplitView.prototype = {
    /**
     * @return {boolean}
     */
    isVertical: function()
    {
        return this._isVertical;
    },

    /**
     * @param {boolean} isVertical
     */
    setVertical: function(isVertical)
    {
        if (this._isVertical === isVertical)
            return;

        this._innerSetVertical(isVertical);

        if (this.isShowing())
            this._updateLayout();
    },

    /**
     * @param {boolean} isVertical
     */
    _innerSetVertical: function(isVertical)
    {
        this.element.removeStyleClass(this._isVertical ? "split-view-vertical" : "split-view-horizontal");
        this._isVertical = isVertical;
        this.element.addStyleClass(this._isVertical ? "split-view-vertical" : "split-view-horizontal");
    },
  
    _updateLayout: function()
    {
        this._updateTotalSize();

        delete this._sidebarSize;  // Force update.
        this.setSidebarSize(this._lastSidebarSize());
    },

    /**
     * @return {Element}
     */
    firstElement: function()
    {
        return this._firstElement;
    },

    /**
     * @return {Element}
     */
    secondElement: function()
    {
        return this._secondElement;
    },

    /**
     * @return {Element}
     */
    get mainElement()
    {
        return this.isSidebarSecond() ? this.firstElement() : this.secondElement();
    },

    /**
     * @return {Element}
     */
    get sidebarElement()
    {
        return this.isSidebarSecond() ? this.secondElement() : this.firstElement();
    },

    /**
     * @return {boolean}
     */
    isSidebarSecond: function()
    {
        return this._secondIsSidebar;
    },

    /**
     * @param {boolean} secondIsSidebar
     */
    setSecondIsSidebar: function(secondIsSidebar)
    {
        this.sidebarElement.removeStyleClass("split-view-sidebar");
        this._secondIsSidebar = secondIsSidebar;
        this.sidebarElement.addStyleClass("split-view-sidebar");
    },

    /**
     * @return {Element}
     */
    resizerElement: function()
    {
        return this._resizerElement;
    },

    showOnlyFirst: function()
    {
        this._showOnly(this._firstElement, this._secondElement);
    },

    showOnlySecond: function()
    {
        this._showOnly(this._secondElement, this._firstElement);
    },

    /**
     * @param {Element} sideA
     * @param {Element} sideB
     */
    _showOnly: function(sideA, sideB)
    {
        sideA.removeStyleClass("hidden");
        sideA.addStyleClass("maximized");
        sideB.addStyleClass("hidden");
        sideB.removeStyleClass("maximized");
        this._removeAllLayoutProperties();

        this._isShowingOne = true;
        this.setResizable(false);
        this.doResize();
    },

    _removeAllLayoutProperties: function()
    {
        this._firstElement.style.removeProperty("right");
        this._firstElement.style.removeProperty("bottom");
        this._firstElement.style.removeProperty("width");
        this._firstElement.style.removeProperty("height");

        this._secondElement.style.removeProperty("left");
        this._secondElement.style.removeProperty("top");
        this._secondElement.style.removeProperty("width");
        this._secondElement.style.removeProperty("height");

        this._resizerElement.style.removeProperty("left");
        this._resizerElement.style.removeProperty("right");
        this._resizerElement.style.removeProperty("top");
        this._resizerElement.style.removeProperty("bottom");

        this._resizerElement.style.removeProperty("margin-left");
        this._resizerElement.style.removeProperty("margin-right");
        this._resizerElement.style.removeProperty("margin-top");
        this._resizerElement.style.removeProperty("margin-bottom");
    },

    showBoth: function()
    {
        this._isShowingOne = false;
        this._firstElement.removeStyleClass("hidden");
        this._firstElement.removeStyleClass("maximized");
        this._secondElement.removeStyleClass("hidden");
        this._secondElement.removeStyleClass("maximized");

        this._updateLayout();

        this.setResizable(true);
        this.doResize();
    },

    /**
     * @param {boolean} resizable
     */
    setResizable: function(resizable)
    {
        if (this._resizable === resizable)
            return;
        this._resizable = resizable;
        if (resizable)
            this._resizerElement.removeStyleClass("hidden");
        else
            this._resizerElement.addStyleClass("hidden");
    },

    /**
     * @param {number} size
     */
    setSidebarSize: function(size)
    {
        if (this._sidebarSize === size)
            return;

        size = this.applyConstraints(size);
        this._innerSetSidebarSize(size);
        this._saveSidebarSize(size);
    },

    /**
     * @return {number}
     */
    sidebarSize: function()
    {
        return this._sidebarSize;
    },

    /**
     * @return {number}
     */
    totalSize: function()
    {
        return this._totalSize;
    },

    _updateTotalSize: function()
    {
        this._totalSize = this._isVertical ? this.element.offsetWidth : this.element.offsetHeight;
        if (this._useFraction)
            this._sidebarSize = this._lastSidebarSize();
    },

    /**
     * @param {number} size
     */
    _innerSetSidebarSize: function(size)
    {
        if (this._isShowingOne)
            return;

        this._removeAllLayoutProperties();

        var sizeValue;
        if (this._useFraction)
            sizeValue = (size / this._totalSize) * 100 + "%";
        else
            sizeValue = size + "px";

        if (this._isVertical) {
            var resizerWidth = this._resizerElement.offsetWidth;
            if (this._secondIsSidebar) {
                this._firstElement.style.right = sizeValue;
                this._secondElement.style.width = sizeValue;
                this._resizerElement.style.right = sizeValue;
                this._resizerElement.style.marginRight = -resizerWidth / 2 + "px";
            } else {
                this._firstElement.style.width = sizeValue;
                this._secondElement.style.left = sizeValue;
                this._resizerElement.style.left = sizeValue;
                this._resizerElement.style.marginLeft = -resizerWidth / 2 + "px";
            }
        } else {
            var resizerHeight = this._resizerElement.offsetHeight;

            if (this._secondIsSidebar) {
                this._firstElement.style.bottom = sizeValue;
                this._secondElement.style.height = sizeValue;
                this._resizerElement.style.bottom = sizeValue;
                this._resizerElement.style.marginBottom = -resizerHeight / 2 + "px";
            } else {
                this._firstElement.style.height = sizeValue;
                this._secondElement.style.top = sizeValue;
                this._resizerElement.style.top = sizeValue;
                this._resizerElement.style.marginTop = -resizerHeight / 2 + "px";
            }
        }

        this._sidebarSize = size;
        this.doResize();
    },

    /**
     * @param {number} size
     * @return {number}
     */
    applyConstraints: function(size)
    {
        const minSize = 20;
        size = Math.max(size, minSize);
        if (this._totalSize - size < minSize)
            size = this._totalSize - minSize;
        return size;
    },

    wasShown: function()
    {
        this._updateLayout();
    },

    onResize: function()
    {
        this._updateTotalSize();
    },

    /**
     * @param {Event} event
     * @return {boolean}
     */
    _startResizerDragging: function(event)
    {
        if (!this._resizable)
            return false;

        this._dragOffset = (this._secondIsSidebar ? this._totalSize - this._sidebarSize : this._sidebarSize) - (this._isVertical ? event.pageX : event.pageY);
        return true;
    },

    /**
     * @param {Event} event
     */
    _resizerDragging: function(event)
    {
        var newOffset = (this._isVertical ? event.pageX : event.pageY) + this._dragOffset;
        var newSize = (this._secondIsSidebar ? this._totalSize - newOffset : newOffset);
        this.setSidebarSize(newSize);
        event.preventDefault();
    },

    /**
     * @param {Event} event
     */
    _endResizerDragging: function(event)
    {
        delete this._dragOffset;
    },

    /**
     * @param {Element} resizerElement
     */
    installResizer: function(resizerElement)
    {
        resizerElement.addEventListener("mousedown", this._onDragStart.bind(this), false);
    },

    /**
     *
     * @param {Event} event
     */
    _onDragStart: function(event)
    {
        WebInspector._elementDragStart(this._startResizerDragging.bind(this), this._resizerDragging.bind(this), this._endResizerDragging.bind(this), this._isVertical ? "ew-resize" : "ns-resize", event);
    },

    /**
     * @return {WebInspector.Setting}
     */
    _sizeSetting: function()
    {
        if (!this._sidebarSizeSettingName)
            return null;

        var settingName = this._sidebarSizeSettingName + (this._isVertical ? "" : "H");
        if (!WebInspector.settings[settingName])
            WebInspector.settings[settingName] = WebInspector.settings.createSetting(settingName, undefined);

        return WebInspector.settings[settingName];
    },

    /**
     * @return {number}
     */
    _lastSidebarSize: function()
    {
        var sizeSetting = this._sizeSetting();
        var size = sizeSetting ? sizeSetting.get() : 0;
        if (!size)
             size = this._isVertical ? this._savedSidebarWidth : this._savedSidebarHeight;
        if (this._useFraction)
            size *= this._totalSize;
        return size;
    },

    /**
     * @param {number} size
     */
    _saveSidebarSize: function(size)
    {
        if (this._useFraction)
            size /= this._totalSize;

        if (this._isVertical)
            this._savedSidebarWidth = size;
        else
            this._savedSidebarHeight = size;

        var sizeSetting = this._sizeSetting();
        if (sizeSetting)
            sizeSetting.set(size);
    },

    __proto__: WebInspector.View.prototype
}
