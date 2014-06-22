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

WebInspector.DetailsSection = function(identifier, title, groups, optionsElement, defaultCollapsedSettingValue) {
    WebInspector.Object.call(this);

    console.assert(identifier);

    this._element = document.createElement("div");
    this._element.className = WebInspector.DetailsSection.StyleClassName;

    this._headerElement = document.createElement("div");
    this._headerElement.addEventListener("click", this._headerElementClicked.bind(this));
    this._headerElement.className = WebInspector.DetailsSection.HeaderElementStyleClassName;
    this._element.appendChild(this._headerElement);

    if (optionsElement instanceof HTMLElement) {
        this._optionsElement = optionsElement;
        this._optionsElement.addEventListener("mousedown", this._optionsElementMouseDown.bind(this));
        this._optionsElement.addEventListener("mouseup", this._optionsElementMouseUp.bind(this));
        this._headerElement.appendChild(this._optionsElement);
    }

    this._titleElement = document.createElement("span");
    this._headerElement.appendChild(this._titleElement);

    this._contentElement = document.createElement("div");
    this._contentElement.className = WebInspector.DetailsSection.ContentElementStyleClassName;
    this._element.appendChild(this._contentElement);

    this._generateDisclosureTrianglesIfNeeded();

    this._identifier = identifier;
    this.title = title;
    this.groups = groups || [new WebInspector.DetailsSectionGroup];

    this._collapsedSetting = new WebInspector.Setting(identifier + "-details-section-collapsed", !!defaultCollapsedSettingValue);
    this.collapsed = this._collapsedSetting.value;
};

WebInspector.DetailsSection.StyleClassName = "details-section";
WebInspector.DetailsSection.HeaderElementStyleClassName = "header";
WebInspector.DetailsSection.TitleElementStyleClassName = "title";
WebInspector.DetailsSection.ContentElementStyleClassName = "content";
WebInspector.DetailsSection.CollapsedStyleClassName = "collapsed";
WebInspector.DetailsSection.MouseOverOptionsElementStyleClassName = "mouse-over-options-element";
WebInspector.DetailsSection.DisclosureTriangleOpenCanvasIdentifier = "details-section-disclosure-triangle-open";
WebInspector.DetailsSection.DisclosureTriangleClosedCanvasIdentifier = "details-section-disclosure-triangle-closed";
WebInspector.DetailsSection.DisclosureTriangleNormalCanvasIdentifierSuffix = "-normal";
WebInspector.DetailsSection.DisclosureTriangleActiveCanvasIdentifierSuffix = "-active";

WebInspector.DetailsSection.prototype = {
    constructor: WebInspector.DetailsSection,

    // Public

    get element()
    {
        return this._element;
    },

    get identifier()
    {
        return this._identifier;
    },

    get title()
    {
        return this._titleElement.textContent;
    },

    set title(title)
    {
        this._titleElement.textContent = title;
    },

    get collapsed()
    {
        return this._element.classList.contains(WebInspector.DetailsSection.CollapsedStyleClassName);
    },

    set collapsed(flag)
    {
        if (flag)
            this._element.classList.add(WebInspector.DetailsSection.CollapsedStyleClassName);
        else
            this._element.classList.remove(WebInspector.DetailsSection.CollapsedStyleClassName);

        this._collapsedSetting.value = flag || false;
    },

    get groups()
    {
        return this._groups;
    },

    set groups(groups)
    {
        this._contentElement.removeChildren();

        this._groups = groups || [];

        for (var i = 0; i < this._groups.length; ++i)
            this._contentElement.appendChild(this._groups[i].element);
    },

    // Private

    _headerElementClicked: function(event)
    {
        if (event.target.isSelfOrDescendant(this._optionsElement))
            return;

        this.collapsed = !this.collapsed;

        this._element.scrollIntoViewIfNeeded(false);
    },

    _optionsElementMouseDown: function(event)
    {
        this._headerElement.classList.add(WebInspector.DetailsSection.MouseOverOptionsElementStyleClassName);
    },

    _optionsElementMouseUp: function(event)
    {
        this._headerElement.classList.remove(WebInspector.DetailsSection.MouseOverOptionsElementStyleClassName);
    },

    _generateDisclosureTrianglesIfNeeded: function()
    {
        if (WebInspector.DetailsSection._generatedDisclosureTriangles)
            return;

        // Set this early instead of in _generateDisclosureTriangle because we don't want multiple sections that are
        // created at the same time to duplicate the work (even though it would be harmless.)
        WebInspector.DetailsSection._generatedDisclosureTriangles = true;

        var specifications = {};
        specifications[WebInspector.DetailsSection.DisclosureTriangleNormalCanvasIdentifierSuffix] = {
            fillColor: [134, 134, 134]
        };

        specifications[WebInspector.DetailsSection.DisclosureTriangleActiveCanvasIdentifierSuffix] = {
            fillColor: [57, 57, 57]
        };

        generateColoredImagesForCSS("Images/DisclosureTriangleSmallOpen.pdf", specifications, 13, 13, WebInspector.DetailsSection.DisclosureTriangleOpenCanvasIdentifier);
        generateColoredImagesForCSS("Images/DisclosureTriangleSmallClosed.pdf", specifications, 13, 13, WebInspector.DetailsSection.DisclosureTriangleClosedCanvasIdentifier);
    }
};

WebInspector.DetailsSection.prototype.__proto__ = WebInspector.Object.prototype;
