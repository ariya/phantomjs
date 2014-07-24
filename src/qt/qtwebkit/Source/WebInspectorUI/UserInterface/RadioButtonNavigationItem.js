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

WebInspector.RadioButtonNavigationItem = function(identifier, toolTip, image, imageWidth, imageHeight) {
    WebInspector.ButtonNavigationItem.call(this, identifier, toolTip, image, imageWidth, imageHeight, null, "tab");
};

WebInspector.RadioButtonNavigationItem.StyleClassName = "radio";
WebInspector.RadioButtonNavigationItem.ActiveStyleClassName = "active";
WebInspector.RadioButtonNavigationItem.SelectedStyleClassName = "selected";

WebInspector.RadioButtonNavigationItem.prototype = {
    constructor: WebInspector.RadioButtonNavigationItem,

    // Public

    get selected()
    {
        return this.element.classList.contains(WebInspector.RadioButtonNavigationItem.SelectedStyleClassName);
    },

    set selected(flag)
    {
        if (flag) {
            this.element.classList.add(WebInspector.RadioButtonNavigationItem.SelectedStyleClassName);
            this.element.setAttribute("aria-selected", "true");
        } else {
            this.element.classList.remove(WebInspector.RadioButtonNavigationItem.SelectedStyleClassName);
            this.element.setAttribute("aria-selected", "false");
        }
    },

    get active()
    {
        return this.element.classList.contains(WebInspector.RadioButtonNavigationItem.ActiveStyleClassName);
    },

    set active(flag)
    {
        if (flag)
            this.element.classList.add(WebInspector.RadioButtonNavigationItem.ActiveStyleClassName);
        else
            this.element.classList.remove(WebInspector.RadioButtonNavigationItem.ActiveStyleClassName);
    },

    generateStyleText: function(parentSelector)
    {
        var classNames = this._classNames.join(".");

        // Default state.
        var styleText = parentSelector + " ." + classNames + " > .glyph { background-image: -webkit-canvas(" + this._canvasIdentifier() + "); background-size: " +  this._imageWidth + "px " + this._imageHeight + "px; }\n";

        // Selected state.
        styleText += parentSelector + " ." + classNames + ".selected:not(.disabled) > .glyph { background-image: -webkit-canvas(" + this._canvasIdentifier(WebInspector.ButtonNavigationItem.States.Focus) + "); }\n";

        // Selected and pressed state.
        styleText += parentSelector + " ." + classNames + ".selected:not(.disabled):active > .glyph { background-image: -webkit-canvas(" + this._canvasIdentifier(WebInspector.ButtonNavigationItem.States.ActiveFocus) + "); }\n";

        return styleText;
    },

    updateLayout: function(expandOnly)
    {
        if (expandOnly)
            return;

        var isSelected = this.selected;

        if (!isSelected) {
            this.element.classList.add(WebInspector.RadioButtonNavigationItem.SelectedStyleClassName);
            this.element.setAttribute("aria-selected", "true");
        }

        var selectedWidth = this.element.offsetWidth;
        if (selectedWidth)
            this.element.style.minWidth = selectedWidth + "px";

        if (!isSelected) {
            this.element.classList.remove(WebInspector.RadioButtonNavigationItem.SelectedStyleClassName);
            this.element.setAttribute("aria-selected", "false");
        }
    },

    // Private

    _additionalClassNames: [WebInspector.RadioButtonNavigationItem.StyleClassName, WebInspector.ButtonNavigationItem.StyleClassName],
};

WebInspector.RadioButtonNavigationItem.prototype.__proto__ = WebInspector.ButtonNavigationItem.prototype;
