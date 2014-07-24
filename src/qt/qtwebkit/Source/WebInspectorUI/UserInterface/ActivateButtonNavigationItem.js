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

WebInspector.ActivateButtonNavigationItem = function(identifier, defaultToolTip, activatedToolTip, image, imageWidth, imageHeight, suppressEmboss)
{
    WebInspector.ButtonNavigationItem.call(this, identifier, defaultToolTip, image, imageWidth, imageHeight, suppressEmboss);

    this._defaultToolTip = defaultToolTip;
    this._activatedToolTip = activatedToolTip || defaultToolTip;
};

WebInspector.ActivateButtonNavigationItem.StyleClassName = "activate";
WebInspector.ActivateButtonNavigationItem.ActivatedStyleClassName = "activated";

WebInspector.ActivateButtonNavigationItem.prototype = {
    constructor: WebInspector.ActivateButtonNavigationItem,

    // Public

    get defaultToolTip()
    {
        return this._defaultToolTip;
    },

    get activatedToolTip()
    {
        return this._activatedToolTip;
    },

    get activated()
    {
        return this.element.classList.contains(WebInspector.ActivateButtonNavigationItem.ActivatedStyleClassName);
    },

    set activated(flag)
    {
        if (flag) {
            this.toolTip = this._activatedToolTip;
            this.element.classList.add(WebInspector.ActivateButtonNavigationItem.ActivatedStyleClassName);
        } else {
            this.toolTip = this._defaultToolTip;
            this.element.classList.remove(WebInspector.ActivateButtonNavigationItem.ActivatedStyleClassName);
        }
    },

    generateStyleText: function(parentSelector)
    {
        var classNames = this._classNames.join(".");

        if (this._suppressEmboss)
            var styleText = parentSelector + " ." + classNames + " > .glyph { background-size: " +  this._imageWidth + "px " + this._imageHeight + "px; }\n";
        else {
            var activatedClassName = "." + WebInspector.ActivateButtonNavigationItem.ActivatedStyleClassName;

            // Default state.
            var styleText = parentSelector + " ." + classNames + " > .glyph { background-image: -webkit-canvas(" + this._canvasIdentifier(WebInspector.ButtonNavigationItem.States.Normal) + "); background-size: " +  this._imageWidth + "px " + this._imageHeight + "px; }\n";

            // Pressed state.
            styleText += parentSelector + " ." + classNames + ":not(.disabled):active > .glyph { background-image: -webkit-canvas(" + this._canvasIdentifier(WebInspector.ButtonNavigationItem.States.Active) + "); }\n";

            // Activated state.
            styleText += parentSelector + " ." + classNames + activatedClassName + ":not(.disabled) > .glyph { background-image: -webkit-canvas(" + this._canvasIdentifier(WebInspector.ButtonNavigationItem.States.Focus) + "); }\n";

            // Activated and pressed state.
            styleText += parentSelector + " ." + classNames + activatedClassName + ":not(.disabled):active > .glyph { background-image: -webkit-canvas(" + this._canvasIdentifier(WebInspector.ButtonNavigationItem.States.ActiveFocus) + "); }\n";
        }

        return styleText;
    },

    // Private

    _additionalClassNames: [WebInspector.ActivateButtonNavigationItem.StyleClassName, WebInspector.ButtonNavigationItem.StyleClassName]
};

WebInspector.ActivateButtonNavigationItem.prototype.__proto__ = WebInspector.ButtonNavigationItem.prototype;
