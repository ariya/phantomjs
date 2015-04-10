/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * @extends {WebInspector.SplitView}
 * @param {string=} sidebarPosition
 * @param {string=} sidebarWidthSettingName
 * @param {number=} defaultSidebarWidth
 * @param {number=} defaultSidebarHeight
 */
WebInspector.SidebarView = function(sidebarPosition, sidebarWidthSettingName, defaultSidebarWidth, defaultSidebarHeight)
{
    WebInspector.SplitView.call(this, true, sidebarWidthSettingName, defaultSidebarWidth, defaultSidebarHeight);

    this._minimumSidebarWidth = Preferences.minSidebarWidth;
    this._minimumMainWidthPercent = 50;

    this._minimumSidebarHeight = Preferences.minSidebarHeight;
    this._minimumMainHeightPercent = 50;

    this._sidebarPosition = sidebarPosition || WebInspector.SidebarView.SidebarPosition.Start;
    this.setSecondIsSidebar(this._sidebarPosition === WebInspector.SidebarView.SidebarPosition.End);
}

WebInspector.SidebarView.EventTypes = {
    Resized: "Resized"
}

/**
 * @enum {string}
 */
WebInspector.SidebarView.SidebarPosition = {
    Start: "Start",
    End: "End"
}

WebInspector.SidebarView.prototype = {
    /**
     * @param {number} width
     */
    setMinimumSidebarWidth: function(width)
    {
        this._minimumSidebarWidth = width;
    },

    /**
     * @param {number} height
     */
    setMinimumSidebarHeight: function(height)
    {
        this._minimumSidebarHeight = height;
    },

    /**
     * @param {number} widthPercent
     */
    setMinimumMainWidthPercent: function(widthPercent)
    {
        this._minimumMainWidthPercent = widthPercent;
    },

    /**
     * @param {number} heightPercent
     */
    setMinimumMainHeightPercent: function(heightPercent)
    {
        this._minimumMainHeightPercent = heightPercent;
    },

    /**
     * @param {number} width
     */
    setSidebarWidth: function(width)
    {
        this.setSidebarSize(width);
    },

    /**
     * @return {number}
     */
    sidebarWidth: function()
    {
        return this.sidebarSize();
    },

    onResize: function()
    {
        WebInspector.SplitView.prototype.onResize.call(this);
        this.dispatchEventToListeners(WebInspector.SidebarView.EventTypes.Resized, this.sidebarWidth());
    },

    /**
     * @param {number} size
     */
    applyConstraints: function(size)
    {
        var minSidebarSize = this.isVertical() ? this._minimumSidebarWidth : this._minimumSidebarHeight;
        var minMainSizePercent = this.isVertical() ? this._minimumMainWidthPercent : this._minimumMainHeightPercent;
        return Number.constrain(size, minSidebarSize, this.totalSize() * (100 - minMainSizePercent) / 100);
    },

    hideMainElement: function()
    {
        if (this.isSidebarSecond())
            this.showOnlySecond();
        else
            this.showOnlyFirst();
    },

    showMainElement: function()
    {
        this.showBoth();
    },

    hideSidebarElement: function()
    {
        if (this.isSidebarSecond())
            this.showOnlyFirst();
        else
            this.showOnlySecond();
    },

    showSidebarElement: function()
    {
        this.showBoth();
    },

    /**
     * @return {Array.<Element>}
     */
    elementsToRestoreScrollPositionsFor: function()
    {
        return [ this.mainElement, this.sidebarElement ];
    },

    __proto__: WebInspector.SplitView.prototype
}
