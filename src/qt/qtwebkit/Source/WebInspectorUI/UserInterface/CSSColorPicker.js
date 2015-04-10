/*
 * Copyright (C) 2011 Brian Grinstead All rights reserved.
 * Copyright (C) 2013 Matt Holden <jftholden@yahoo.com>
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


WebInspector.CSSColorPicker = function()
{
    WebInspector.Object.call(this);

    this._element = document.createElement("div");
    this._element.className = "colorpicker-container";

    var topElement = this._element.createChild("div", "colorpicker-top");
    topElement.createChild("div", "colorpicker-fill");

    var topInnerElement = topElement.createChild("div", "colorpicker-top-inner colorpicker-fill-parent");
    this._draggerElement = topInnerElement.createChild("div", "colorpicker-color");
    this._dragHelperElement = this._draggerElement
        .createChild("div", "colorpicker-saturation colorpicker-fill-parent")
        .createChild("div", "colorpicker-value colorpicker-fill-parent")
        .createChild("div", "colorpicker-dragger");

    this._sliderElement = topInnerElement.createChild("div", "colorpicker-hue");
    this._slideHelper = this._sliderElement.createChild("div", "colorpicker-slider");

    var rangeContainer = this._element.createChild("div", "colorpicker-range-container");
    var alphaLabel = rangeContainer.createChild("label", "colorpicker-text");
    alphaLabel.textContent = WebInspector.UIString("Opacity:");

    this._alphaElement = rangeContainer.createChild("input", "colorpicker-range");
    this._alphaElement.setAttribute("type", "range");
    this._alphaElement.setAttribute("min", "0");
    this._alphaElement.setAttribute("max", "100");
    this._alphaElement.addEventListener("change", alphaDrag.bind(this), false);

    var displayContainer = this._element.createChild("div", "colorpicker-bottom");

    var swatchElement = displayContainer.createChild("span", "swatch");
    this._swatchInnerElement = swatchElement.createChild("span", "swatch-inner");
    
    swatchElement.addEventListener("click", colorSwatchClicked.bind(this), false);

    this._displayElement = displayContainer.createChild("span", "colorpicker-text colorpicker-color-text-value");

    makeDraggable(this._sliderElement, hueDrag.bind(this));
    makeDraggable(this._draggerElement, colorDrag.bind(this), colorDragStart.bind(this));

    function makeDraggable(element, onmove, onstart, onstop)
    {
        var doc = document;
        var dragging;
        var offset;
        var scrollOffset;
        var maxHeight;
        var maxWidth;

        function consume(e)
        {
            event.preventDefault();
        }

        function move(e)
        {
            if (!dragging || !onmove)
                return;

            var dragX = Math.max(0, Math.min(e.pageX - offset.left + scrollOffset.left, maxWidth));
            var dragY = Math.max(0, Math.min(e.pageY - offset.top + scrollOffset.top, maxHeight));

            onmove(element, dragX, dragY, e);
        }

        function start(e)
        {
            if (e.button !== 0 || e.ctrlKey)
                return;

            if (dragging)
                return;

            if (onstart)
                onstart(element, e)

            dragging = true;
            maxHeight = element.clientHeight;
            maxWidth = element.clientWidth;

            scrollOffset = {left: 0, top: 0};
            var boundingRect = element.getBoundingClientRect();
            offset = {left: boundingRect.left, top: boundingRect.top};

            doc.addEventListener("selectstart", consume, false);
            doc.addEventListener("dragstart", consume, false);
            doc.addEventListener("mousemove", move, false);
            doc.addEventListener("mouseup", stop, false);

            move(e);
            consume(e);
        }

        function stop(e)
        {
            if (!dragging)
                return;

            doc.removeEventListener("selectstart", consume, false);
            doc.removeEventListener("dragstart", consume, false);
            doc.removeEventListener("mousemove", move, false);
            doc.removeEventListener("mouseup", stop, false);

            if (onstop)
                onstop(element, e);

            dragging = false;
        }

        element.addEventListener("mousedown", start, false);
    }

    function hueDrag(element, dragX, dragY)
    {
        this.hsv[0] = dragY / this.slideHeight;

        this._updateDisplay(true);
    }

    var initialHelperOffset;

    function colorDragStart(element, dragX, dragY)
    {
        initialHelperOffset = {x: this._dragHelperElement.offsetLeft, y: this._dragHelperElement.offsetTop};
    }

    function colorDrag(element, dragX, dragY, event)
    {
        if (event.shiftKey) {
            if (Math.abs(dragX - initialHelperOffset.x) >= Math.abs(dragY - initialHelperOffset.y))
                dragY = initialHelperOffset.y;
            else
                dragX = initialHelperOffset.x;
        }

        this.hsv[1] = dragX / this.dragWidth;
        this.hsv[2] = (this.dragHeight - dragY) / this.dragHeight;

        this._updateDisplay(true);
    }

    function alphaDrag()
    {
        this.hsv[3] = this._alphaElement.value / 100;

        this._updateDisplay(true);
    }

    function colorSwatchClicked()
    {
        // By changing this value, the new color format
        // will be propagated down to any listeners after the UI is updated.
        this._originalFormat = this.color.nextFormat();
        this._updateDisplay(true);
    }
};

WebInspector.Object.addConstructorFunctions(WebInspector.CSSColorPicker);

WebInspector.CSSColorPicker.Event = {
    ColorChanged: "css-color-picker-color-changed"
};

WebInspector.CSSColorPicker.prototype = {
    contructor: WebInspector.CSSColorPicker,

    get element()
    {
        return this._element;
    },

    set color(color)
    {
        if (!this._originalFormat) {
            this._originalFormat = color.format;
            if (color.format === WebInspector.Color.Format.ShortHEX)
                this._originalFormat = WebInspector.Color.Format.HEX;
        }

        var rgba = (color.rgba || color.rgb).slice(0);

        if (rgba.length === 3)
            rgba[3] = 1;

        this.hsv = WebInspector.CSSColorPicker.rgbaToHSVA(rgba[0], rgba[1], rgba[2], rgba[3]);
    },

    get color()
   {
       var rgba = WebInspector.CSSColorPicker.hsvaToRGBA(this.hsv[0], this.hsv[1], this.hsv[2], this.hsv[3]);
       var color;
       if (rgba[3] === 1)
           color = WebInspector.Color.fromRGB(rgba[0], rgba[1], rgba[2]);
       else
           color = WebInspector.Color.fromRGBA(rgba[0], rgba[1], rgba[2], rgba[3]);
       var colorValue = color.toString(this.outputColorFormat);
       if (!colorValue)
           colorValue = color.toString(); // this.outputColorFormat can be invalid for current color (e.g. "nickname").
       return new WebInspector.Color(colorValue);
    },

    get outputColorFormat()
    {
        var format = this._originalFormat;

        if (this.hsv[3] === 1) {
            // Simplify transparent formats, and don't allow ShortHex.
            if (format === WebInspector.Color.Format.RGBA)
                format = WebInspector.Color.Format.RGB;
            else if (format === WebInspector.Color.Format.HSLA)
                format = WebInspector.Color.Format.HSL;
            else if (format === WebInspector.Color.Format.ShortHEX)
                format = WebInspector.Color.Format.HEX;
        } else {
            // Everything except HSL(A) should be returned as RGBA if transparency is involved.
            var printableTransparentFormats = [
                WebInspector.Color.Format.HSLA,
                WebInspector.Color.Format.RGBA
            ];

            if (printableTransparentFormats.indexOf(format) === -1)
                format = WebInspector.Color.Format.RGBA;
        }

        return format;
    },

    get colorHueOnly()
    {
        var rgba = WebInspector.CSSColorPicker.hsvaToRGBA(this.hsv[0], 1, 1, 1);
        return WebInspector.Color.fromRGBA(rgba[0], rgba[1], rgba[2], rgba[3]);
    },

    set displayText(text)
    {
        this._displayElement.textContent = text;
    },

    shown: function()
    {
        this.slideHeight = this._sliderElement.offsetHeight;
        this.dragWidth = this._draggerElement.offsetWidth;
        this.dragHeight = this._draggerElement.offsetHeight;
        this._dragHelperElementHeight = this._dragHelperElement.offsetHeight / 2;
        this.slideHelperHeight = this._slideHelper.offsetHeight / 2;
        this._updateDisplay();
    },

    // Private

    _updateHelperLocations: function()
    {
        var h = this.hsv[0];
        var s = this.hsv[1];
        var v = this.hsv[2];

        // Where to show the little circle that displays your current selected color.
        var dragX = s * this.dragWidth;
        var dragY = this.dragHeight - (v * this.dragHeight);

        dragX = Math.max(-this._dragHelperElementHeight,
            Math.min(this.dragWidth - this._dragHelperElementHeight, dragX - this._dragHelperElementHeight));
        dragY = Math.max(-this._dragHelperElementHeight,
            Math.min(this.dragHeight - this._dragHelperElementHeight, dragY - this._dragHelperElementHeight));

        this._dragHelperElement.positionAt(dragX, dragY);

        // Where to show the bar that displays your current selected hue.
        var slideY = (h * this.slideHeight) - this.slideHelperHeight;
        this._slideHelper.style.top = slideY + "px";

        this._alphaElement.value = this.hsv[3] * 100;
    },

    _updateDisplay: function(colorWasChanged)
    {
        this._updateHelperLocations();

        var rgb = (this.color.rgba || this.color.rgb).slice(0);

        if (rgb.length === 3)
            rgb[3] = 1;

        var rgbHueOnly = this.colorHueOnly.rgb;

        var flatColor = "rgb(" + rgbHueOnly[0] + ", " + rgbHueOnly[1] + ", " + rgbHueOnly[2] + ")";
        var fullColor = "rgba(" + rgb[0] + ", " + rgb[1] + ", " + rgb[2] + ", " + rgb[3] + ")";

        this._draggerElement.style.backgroundColor = flatColor;
        this._swatchInnerElement.style.backgroundColor = fullColor;

        this._alphaElement.value = this.hsv[3] * 100;

        var colorValue = this.color.toString(this.outputColorFormat);
        if (!colorValue)
            colorValue = this.color.toString(); // this.outputColorFormat can be invalid for current color (e.g. "nickname").

        this.displayText = colorValue;

        if (colorWasChanged)
            this.dispatchEventToListeners(WebInspector.CSSColorPicker.Event.ColorChanged, {color: this.color});
    }
};

WebInspector.CSSColorPicker.hsvaToRGBA = function(h, s, v, a)
{
    var r, g, b;

    var i = Math.floor(h * 6);
    var f = h * 6 - i;
    var p = v * (1 - s);
    var q = v * (1 - f * s);
    var t = v * (1 - (1 - f) * s);

    switch(i % 6) {
    case 0:
        r = v, g = t, b = p;
        break;
    case 1:
        r = q, g = v, b = p;
        break;
    case 2:
        r = p, g = v, b = t;
        break;
    case 3:
        r = p, g = q, b = v;
        break;
    case 4:
        r = t, g = p, b = v;
        break;
    case 5:
        r = v, g = p, b = q;
        break;
    }

    return [Math.round(r * 255), Math.round(g * 255), Math.round(b * 255), a];
};

WebInspector.CSSColorPicker.rgbaToHSVA = function(r, g, b, a)
{
    r = r / 255;
    g = g / 255;
    b = b / 255;

    var max = Math.max(r, g, b);
    var min = Math.min(r, g, b);
    var h;
    var s;
    var v = max;

    var d = max - min;
    s = max ? d / max : 0;

    if(max === min) {
        // Achromatic.
        h = 0;
    } else {
        switch(max) {
        case r:
            h = (g - b) / d + (g < b ? 6 : 0);
            break;
        case g:
            h = (b - r) / d + 2;
            break;
        case b:
            h = (r - g) / d + 4;
            break;
        }
        h /= 6;
    }
    return [h, s, v, a];
};

WebInspector.CSSColorPicker.prototype.__proto__ = WebInspector.Object.prototype;
