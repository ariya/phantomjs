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

WebInspector.ButtonNavigationItem = function(identifier, toolTipOrLabel, image, imageWidth, imageHeight, suppressEmboss, role, label) {
    WebInspector.NavigationItem.call(this, identifier);

    console.assert(identifier);
    console.assert(toolTipOrLabel);

    this.toolTip = toolTipOrLabel;

    this._element.addEventListener("click", this._mouseClicked.bind(this));
    
    this._element.setAttribute("role", role || "button");
    
    if (label) 
        this._element.setAttribute("aria-label", label);

    this._imageWidth = imageWidth || 16;
    this._imageHeight = imageHeight || 16;
    this._suppressEmboss = suppressEmboss || false;

    if (suppressEmboss)
        this._element.classList.add(WebInspector.ButtonNavigationItem.SuppressEmbossStyleClassName);

    if (image)
        this.image = image;
    else
        this.label = toolTipOrLabel;
};

WebInspector.ButtonNavigationItem.StyleClassName = "button";
WebInspector.ButtonNavigationItem.DisabledStyleClassName = "disabled";
WebInspector.ButtonNavigationItem.SuppressBezelStyleClassName = "suppress-bezel";
WebInspector.ButtonNavigationItem.SuppressEmbossStyleClassName = "suppress-emboss";
WebInspector.ButtonNavigationItem.TextOnlyClassName = "text-only";

WebInspector.ButtonNavigationItem.States = {};
WebInspector.ButtonNavigationItem.States.Normal = "normal";
WebInspector.ButtonNavigationItem.States.Active = "active";
WebInspector.ButtonNavigationItem.States.Focus = "focus";
WebInspector.ButtonNavigationItem.States.ActiveFocus = "active-focus";

WebInspector.ButtonNavigationItem.Event = {
    Clicked: "button-navigation-item-clicked"
};

WebInspector.ButtonNavigationItem.prototype = {
    constructor: WebInspector.ButtonNavigationItem,

    // Public

    get toolTip()
    {
        return this._element.title;
    },

    set toolTip(newToolTip)
    {
        console.assert(newToolTip);
        if (!newToolTip)
            return;

        this._element.title = newToolTip;
    },

    get label()
    {
        return this._element.textContent;
    },

    set label(newLabel)
    {
        this._element.classList.add(WebInspector.ButtonNavigationItem.TextOnlyClassName);
        this._element.textContent = newLabel || "";
        if (this.parentNavigationBar)
            this.parentNavigationBar.updateLayout();
    },

    get image()
    {
        return this._image;
    },

    set image(newImage)
    {
        if (!newImage) {
            this._element.removeChildren();
            return;
        }

        this._element.removeChildren();
        this._element.classList.remove(WebInspector.ButtonNavigationItem.TextOnlyClassName);

        this._image = newImage;

        this._glyphElement = document.createElement("div");
        this._glyphElement.className = "glyph";
        this._element.appendChild(this._glyphElement);

        this._updateImage();
    },

    get enabled()
    {
        return !this._element.classList.contains(WebInspector.ButtonNavigationItem.DisabledStyleClassName);
    },

    set enabled(flag)
    {
        if (flag)
            this._element.classList.remove(WebInspector.ButtonNavigationItem.DisabledStyleClassName);
        else
            this._element.classList.add(WebInspector.ButtonNavigationItem.DisabledStyleClassName);
    },

    get suppressBezel()
    {
        return this._element.classList.contains(WebInspector.ButtonNavigationItem.SuppressBezelStyleClassName);
    },

    set suppressBezel(flag)
    {
        if (flag)
            this._element.classList.add(WebInspector.ButtonNavigationItem.SuppressBezelStyleClassName);
        else
            this._element.classList.remove(WebInspector.ButtonNavigationItem.SuppressBezelStyleClassName);
    },

    generateStyleText: function(parentSelector)
    {
        var classNames = this._classNames.join(".");

        if (this._suppressEmboss)
            var styleText = parentSelector + " ." + classNames + " > .glyph { background-size: " +  this._imageWidth + "px " + this._imageHeight + "px; }\n";
        else {
            // Default state.
            var styleText = parentSelector + " ." + classNames + " > .glyph { background-image: -webkit-canvas(" + this._canvasIdentifier() + "); background-size: " +  this._imageWidth + "px " + this._imageHeight + "px; }\n";

            // Pressed state.
            styleText += parentSelector + " ." + classNames + ":not(.disabled):active > .glyph { background-image: -webkit-canvas(" + this._canvasIdentifier(WebInspector.ButtonNavigationItem.States.Active) + "); }\n";

            // Focused state.
            styleText += parentSelector + " ." + classNames + ":not(.disabled):focus > .glyph { background-image: -webkit-canvas(" + this._canvasIdentifier(WebInspector.ButtonNavigationItem.States.Focus) + "); }\n";
        }

        return styleText;
    },

    // Private

    _additionalClassNames: [WebInspector.ButtonNavigationItem.StyleClassName],
    _embossedImageStates: WebInspector.ButtonNavigationItem.States,
    _imageCacheable: true,

    _mouseClicked: function(event)
    {
        if (!this.enabled)
            return;
        this.dispatchEventToListeners(WebInspector.ButtonNavigationItem.Event.Clicked);
    },

    _canvasIdentifier: function(state)
    {
        console.assert(!this._suppressEmboss);
        return "navigation-item-" + this._identifier + "-" + (state || WebInspector.ButtonNavigationItem.States.Normal);
    },

    _updateImage: function()
    {
        if (this._suppressEmboss)
            this._glyphElement.style.backgroundImage = "url(" + this._image + ")";
        else
            this._generateImages();
    },

    _generateImages: function()
    {
        console.assert(!this._suppressEmboss);
        if (this._suppressEmboss)
            return;
        generateEmbossedImages(this.image, this._imageWidth, this._imageHeight, this._embossedImageStates, this._canvasIdentifier.bind(this), !this._imageCacheable);
    }
};

WebInspector.ButtonNavigationItem.prototype.__proto__ = WebInspector.NavigationItem.prototype;
