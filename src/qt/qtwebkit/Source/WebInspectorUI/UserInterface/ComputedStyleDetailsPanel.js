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

WebInspector.ComputedStyleDetailsPanel = function()
{
    WebInspector.StyleDetailsPanel.call(this, WebInspector.ComputedStyleDetailsPanel.StyleClassName, "computed", WebInspector.UIString("Computed"));

    this._computedStyleShowAllSetting = new WebInspector.Setting("computed-style-show-all", false);

    var computedStyleShowAllLabel = document.createElement("label");
    computedStyleShowAllLabel.textContent = WebInspector.UIString("Show All");

    this._computedStyleShowAllCheckbox = document.createElement("input");
    this._computedStyleShowAllCheckbox.type = "checkbox";
    this._computedStyleShowAllCheckbox.checked = this._computedStyleShowAllSetting.value;
    this._computedStyleShowAllCheckbox.addEventListener("change", this._computedStyleShowAllCheckboxValueChanged.bind(this));
    computedStyleShowAllLabel.appendChild(this._computedStyleShowAllCheckbox);

    this._propertiesTextEditor = new WebInspector.CSSStyleDeclarationTextEditor(this);
    this._propertiesTextEditor.showsImplicitProperties = this._computedStyleShowAllSetting.value;
    this._propertiesTextEditor.alwaysShowPropertyNames = ["display", "width", "height"];
    this._propertiesTextEditor.sortProperties = true;

    var propertiesRow = new WebInspector.DetailsSectionRow;
    var propertiesGroup = new WebInspector.DetailsSectionGroup([propertiesRow]);
    var propertiesSection = new WebInspector.DetailsSection("computed-style-properties", WebInspector.UIString("Properties"), [propertiesGroup], computedStyleShowAllLabel);

    propertiesRow.element.appendChild(this._propertiesTextEditor.element);

    this.element.appendChild(propertiesSection.element);
};

WebInspector.ComputedStyleDetailsPanel.StyleClassName = "computed";

WebInspector.ComputedStyleDetailsPanel.prototype = {
    constructor: WebInspector.ComputedStyleDetailsPanel,

    // Public

    refresh: function()
    {
        this._propertiesTextEditor.style = this.nodeStyles.computedStyle;
    },

    // Protected

    shown: function()
    {
        WebInspector.StyleDetailsPanel.prototype.shown.call(this);

        this._propertiesTextEditor.updateLayout();
    },

    widthDidChange: function()
    {
        this._propertiesTextEditor.updateLayout();
    },

    // Private

    _computedStyleShowAllCheckboxValueChanged: function(event)
    {
        var checked = this._computedStyleShowAllCheckbox.checked;
        this._computedStyleShowAllSetting.value = checked;
        this._propertiesTextEditor.showsImplicitProperties = checked;
    }
};

WebInspector.ComputedStyleDetailsPanel.prototype.__proto__ = WebInspector.StyleDetailsPanel.prototype;
