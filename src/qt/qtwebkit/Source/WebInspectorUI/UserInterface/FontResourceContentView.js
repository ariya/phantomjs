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

WebInspector.FontResourceContentView = function(resource)
{
    WebInspector.ResourceContentView.call(this, resource, WebInspector.FontResourceContentView.StyleClassName);

    this._styleElement = null;
    this._previewElement = null;
};

WebInspector.FontResourceContentView._uniqueFontIdentifier = 0;

WebInspector.FontResourceContentView.StyleClassName = "font";
WebInspector.FontResourceContentView.PreviewElementStyleClassName = "preview";
WebInspector.FontResourceContentView.LineElementStyleClassName = "line";
WebInspector.FontResourceContentView.ContentElementStyleClassName = "content";
WebInspector.FontResourceContentView.MetricElementStyleClassName = "metric";
WebInspector.FontResourceContentView.TopMetricElementStyleClassName = "top";
WebInspector.FontResourceContentView.XHeightMetricElementStyleClassName = "xheight";
WebInspector.FontResourceContentView.MiddleMetricElementStyleClassName = "middle";
WebInspector.FontResourceContentView.BaselineMetricElementStyleClassName = "baseline";
WebInspector.FontResourceContentView.BottomMetricElementStyleClassName = "bottom";
WebInspector.FontResourceContentView.PreviewLines = ["ABCDEFGHIJKLM", "NOPQRSTUVWXYZ", "abcdefghijklm", "nopqrstuvwxyz", "1234567890"];

WebInspector.FontResourceContentView.MaximumFontSize = 72;
WebInspector.FontResourceContentView.MinimumFontSize = 12;

WebInspector.FontResourceContentView.prototype = {
    constructor: WebInspector.FontResourceContentView,

    // Public

    get previewElement()
    {
        return this._previewElement;
    },

    sizeToFit: function()
    {
        if (!this._previewElement)
            return;

        // Start at the maximum size and try progressively smaller font sizes until minimum is reached or the preview element is not as wide as the main element.
        for (var fontSize = WebInspector.FontResourceContentView.MaximumFontSize; fontSize >= WebInspector.FontResourceContentView.MinimumFontSize; fontSize -= 5) {
            this._previewElement.style.fontSize = fontSize + "px";
            if (this._previewElement.offsetWidth <= this.element.offsetWidth)
                break;
        }
    },

    contentAvailable: function(content, base64Encoded)
    {
        this.element.removeChildren();

        const uniqueFontName = "WebInspectorFontPreview" + (++WebInspector.FontResourceContentView._uniqueFontIdentifier);

        var format = "";

        // We need to specify a format when loading SVG fonts to make them work.
        if (this.resource.mimeType === "image/svg+xml")
            format = " format(\"svg\")";

        if (this._styleElement && this._styleElement.parentNode)
            this._styleElement.parentNode.removeChild(this._styleElement);

        this._styleElement = document.createElement("style");
        this._styleElement.textContent = "@font-face { font-family: \"" + uniqueFontName + "\"; src: url(" + this.resource.contentURL + ")" + format + "; }";

        // The style element will be added when shown later if we are not visible now.
        if (this.visible)
            document.head.appendChild(this._styleElement);

        this._previewElement = document.createElement("div");
        this._previewElement.className = WebInspector.FontResourceContentView.PreviewElementStyleClassName;
        this._previewElement.style.fontFamily = uniqueFontName;

        function createMetricElement(className)
        {
            var metricElement = document.createElement("div");
            metricElement.className = WebInspector.FontResourceContentView.MetricElementStyleClassName + " " + className;
            return metricElement;
        }

        var lines = WebInspector.FontResourceContentView.PreviewLines;
        for (var i = 0; i < lines.length; ++i) {
            var lineElement = document.createElement("div");
            lineElement.className = WebInspector.FontResourceContentView.LineElementStyleClassName;

            lineElement.appendChild(createMetricElement(WebInspector.FontResourceContentView.TopMetricElementStyleClassName));
            lineElement.appendChild(createMetricElement(WebInspector.FontResourceContentView.XHeightMetricElementStyleClassName));
            lineElement.appendChild(createMetricElement(WebInspector.FontResourceContentView.MiddleMetricElementStyleClassName));
            lineElement.appendChild(createMetricElement(WebInspector.FontResourceContentView.BaselineMetricElementStyleClassName));
            lineElement.appendChild(createMetricElement(WebInspector.FontResourceContentView.BottomMetricElementStyleClassName));

            var contentElement = document.createElement("div");
            contentElement.className = WebInspector.FontResourceContentView.ContentElementStyleClassName;
            contentElement.textContent = lines[i];
            lineElement.appendChild(contentElement);

            this._previewElement.appendChild(lineElement);
        }

        this.element.appendChild(this._previewElement);

        this.sizeToFit();
    },

    updateLayout: function()
    {
        this.sizeToFit();
    },

    shown: function()
    {
        // Add the style element since it is removed when hidden.
        if (this._styleElement)
            document.head.appendChild(this._styleElement);
    },

    hidden: function()
    {
        // Remove the style element so it will not stick around when this content view is destroyed.
        if (this._styleElement && this._styleElement.parentNode)
            this._styleElement.parentNode.removeChild(this._styleElement);
    }
};

WebInspector.FontResourceContentView.prototype.__proto__ = WebInspector.ResourceContentView.prototype;
