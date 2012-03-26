/*
 * Copyright (C) 2007, 2008 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.FontView = function(resource)
{
    WebInspector.ResourceView.call(this, resource);

    this.element.addStyleClass("font");
}

WebInspector.FontView._fontInnerHTML = "ABCDEFGHIJKLM<br>NOPQRSTUVWXYZ<br>abcdefghijklm<br>nopqrstuvwxyz<br>1234567890";

WebInspector.FontView._fontId = 0;

WebInspector.FontView._measureFontSize = 50;

WebInspector.FontView.prototype = {
    hasContent: function()
    {
        return true;
    },

    _createContentIfNeeded: function()
    {
        if (this.fontPreviewElement)
            return;

        var uniqueFontName = "WebInspectorFontPreview" + (++WebInspector.FontView._fontId);

        this.fontStyleElement = document.createElement("style");
        this.fontStyleElement.textContent = "@font-face { font-family: \"" + uniqueFontName + "\"; src: url(" + this.resource.url + "); }";
        document.head.appendChild(this.fontStyleElement);

        this.fontPreviewElement = document.createElement("div");
        this.fontPreviewElement.innerHTML = WebInspector.FontView._fontInnerHTML;
        this.fontPreviewElement.style.setProperty("font-family", uniqueFontName, null);
        this.fontPreviewElement.style.setProperty("visibility", "hidden", null);

        this._dummyElement = document.createElement("div");
        this._dummyElement.style.visibility = "hidden";
        this._dummyElement.style.zIndex = "-1";
        this._dummyElement.style.display = "inline";
        this._dummyElement.style.position = "absolute";
        this._dummyElement.style.setProperty("font-family", uniqueFontName, null);
        this._dummyElement.style.setProperty("font-size", WebInspector.FontView._measureFontSize + "px", null);
        this._dummyElement.innerHTML = WebInspector.FontView._fontInnerHTML;

        this.element.appendChild(this.fontPreviewElement);
    },

    show: function(parentElement)
    {
        WebInspector.ResourceView.prototype.show.call(this, parentElement);
        this._createContentIfNeeded();

        this.updateFontPreviewSize();
    },

    resize: function()
    {
        if (this._inResize)
            return;

        this._inResize = true;
        try {
            this.updateFontPreviewSize();
        } finally {
            delete this._inResize;
        }
    },

    _measureElement: function()
    {
        this.element.appendChild(this._dummyElement);
        var result = { width: this._dummyElement.offsetWidth, height: this._dummyElement.offsetHeight };
        this.element.removeChild(this._dummyElement);

        return result;
    },

    updateFontPreviewSize: function()
    {
        if (!this.fontPreviewElement || !this.visible)
            return;

        this.fontPreviewElement.style.removeProperty("visibility");
        var dimension = this._measureElement();

        const height = dimension.height;
        const width = dimension.width;

        // Subtract some padding. This should match the paddings in the CSS plus room for the scrollbar.
        const containerWidth = this.element.offsetWidth - 50;
        const containerHeight = this.element.offsetHeight - 30;

        if (!height || !width || !containerWidth || !containerHeight) {
            this.fontPreviewElement.style.removeProperty("font-size");
            return;
        }

        var widthRatio = containerWidth / width;
        var heightRatio = containerHeight / height;
        var finalFontSize = Math.floor(WebInspector.FontView._measureFontSize * Math.min(widthRatio, heightRatio)) - 2;

        this.fontPreviewElement.style.setProperty("font-size", finalFontSize + "px", null);
    }
}

WebInspector.FontView.prototype.__proto__ = WebInspector.ResourceView.prototype;
