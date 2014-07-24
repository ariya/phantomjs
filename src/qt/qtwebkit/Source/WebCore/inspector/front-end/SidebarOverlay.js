/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 * @param {WebInspector.View} view
 * @param {string} widthSettingName
 * @param {number} minimalWidth
 */
WebInspector.SidebarOverlay = function(view, widthSettingName, minimalWidth)
{
    this.element = document.createElement("div");
    this.element.className = "sidebar-overlay";

    this._view = view;
    this._widthSettingName = widthSettingName;
    this._minimalWidth = minimalWidth;
    this._savedWidth = minimalWidth || 300;
    
    if (this._widthSettingName)
        WebInspector.settings[this._widthSettingName] = WebInspector.settings.createSetting(this._widthSettingName, undefined);
    
    this._resizerElement = document.createElement("div");
    this._resizerElement.className = "sidebar-overlay-resizer";
    this._installResizer(this._resizerElement);
}

WebInspector.SidebarOverlay.prototype = {
    /**
     * @param {Element} relativeToElement
     */
    show: function(relativeToElement)
    {
        relativeToElement.appendChild(this.element);
        relativeToElement.addStyleClass("sidebar-overlay-shown");
        this._view.show(this.element);
        this.element.appendChild(this._resizerElement);
        if (this._resizerWidgetElement)
            this.element.appendChild(this._resizerWidgetElement);
        this.position(relativeToElement);
    },

    /**
     * @param {Element} relativeToElement
     */
    position: function(relativeToElement)
    {
        this._totalWidth = relativeToElement.offsetWidth;
        this._setWidth(this._preferredWidth());
    },

    focus: function()
    {
        WebInspector.setCurrentFocusElement(this._view.element);
    },

    hide: function()
    {
        var element = this.element.parentElement;
        if (!element)
            return;

        this._view.detach();
        element.removeChild(this.element);
        element.removeStyleClass("sidebar-overlay-shown");
        this.element.removeChild(this._resizerElement);
        if (this._resizerWidgetElement)
            this.element.removeChild(this._resizerWidgetElement);
    },
    
    /**
     * @param {number} newWidth
     */
    _setWidth: function(newWidth)
    {
        var width = Number.constrain(newWidth, this._minimalWidth, this._totalWidth);
        
        if (this._width === width)
            return;

        this.element.style.width = width + "px";
        this._resizerElement.style.left = (width - 3) + "px";
        this._width = width;
        this._view.doResize();
        this._saveWidth();
    },

    /**
     * @return {number}
     */
    _preferredWidth: function()
    {
        if (!this._widthSettingName)
            return this._savedWidth;

        return WebInspector.settings[this._widthSettingName].get() || this._savedWidth;
    },
    
    _saveWidth: function()
    {
        this._savedWidth = this._width;
        if (!this._widthSettingName)
            return;

        WebInspector.settings[this._widthSettingName].set(this._width);
    },

    /**
     * @param {Event} event
     * @return {boolean}
     */
    _startResizerDragging: function(event)
    {
        var width = this._width;
        this._dragOffset = width - event.pageX;
        return true;
    },

    /**
     * @param {Event} event
     */
    _resizerDragging: function(event)
    {
        var width = event.pageX + this._dragOffset;
        this._setWidth(width);
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
    _installResizer: function(resizerElement)
    {
        WebInspector.installDragHandle(resizerElement, this._startResizerDragging.bind(this), this._resizerDragging.bind(this), this._endResizerDragging.bind(this), "ew-resize");
    },

    /**
     * @type {Element}
     */
    set resizerWidgetElement(resizerWidgetElement)
    {
        this._resizerWidgetElement = resizerWidgetElement;
        this._installResizer(resizerWidgetElement);
    }
}
