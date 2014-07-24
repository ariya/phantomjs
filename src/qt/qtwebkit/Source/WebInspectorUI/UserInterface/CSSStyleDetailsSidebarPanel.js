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

WebInspector.CSSStyleDetailsSidebarPanel = function()
{
    WebInspector.DOMDetailsSidebarPanel.call(this, "css-style", WebInspector.UIString("Styles"), WebInspector.UIString("Style"), "Images/NavigationItemBrushAndRuler.pdf", "4");

    this._selectedPanel = null;

    this._navigationBar = new WebInspector.NavigationBar(null, null, "tablist");
    this._navigationBar.addEventListener(WebInspector.NavigationBar.Event.NavigationItemSelected, this._navigationItemSelected, this);
    this.element.appendChild(this._navigationBar.element);

    this._contentElement = document.createElement("div");
    this._contentElement.className = WebInspector.CSSStyleDetailsSidebarPanel.ContentStyleClassName;

    this._forcedPseudoClassCheckboxes = {};

    if (WebInspector.cssStyleManager.canForcePseudoClasses()) {
        this._forcedPseudoClassContainer = document.createElement("div");
        this._forcedPseudoClassContainer.className = WebInspector.CSSStyleDetailsSidebarPanel.PseudoClassesElementStyleClassName;

        var groupElement = null;

        WebInspector.CSSStyleManager.ForceablePseudoClasses.forEach(function(pseudoClass) {
            // We don't localize the label since it is a CSS pseudo-class from the CSS standard.
            var label = pseudoClass.capitalize();

            var labelElement = document.createElement("label");

            var checkboxElement = document.createElement("input");
            checkboxElement.addEventListener("change", this._forcedPseudoClassCheckboxChanged.bind(this, pseudoClass));
            checkboxElement.type = "checkbox";

            this._forcedPseudoClassCheckboxes[pseudoClass] = checkboxElement;

            labelElement.appendChild(checkboxElement);
            labelElement.appendChild(document.createTextNode(label));

            if (!groupElement || groupElement.children.length === 2) {
                groupElement = document.createElement("div");
                groupElement.className = WebInspector.CSSStyleDetailsSidebarPanel.PseudoClassesGroupElementStyleClassName;
                this._forcedPseudoClassContainer.appendChild(groupElement);
            }

            groupElement.appendChild(labelElement);
        }.bind(this));

        this._contentElement.appendChild(this._forcedPseudoClassContainer);
    }

    this.element.appendChild(this._contentElement);

    this._computedStyleDetailsPanel = new WebInspector.ComputedStyleDetailsPanel;
    this._rulesStyleDetailsPanel = new WebInspector.RulesStyleDetailsPanel;
    this._metricsStyleDetailsPanel = new WebInspector.MetricsStyleDetailsPanel;

    this._panels = [this._computedStyleDetailsPanel, this._rulesStyleDetailsPanel, this._metricsStyleDetailsPanel];

    this._navigationBar.addNavigationItem(this._computedStyleDetailsPanel.navigationItem);
    this._navigationBar.addNavigationItem(this._rulesStyleDetailsPanel.navigationItem);
    this._navigationBar.addNavigationItem(this._metricsStyleDetailsPanel.navigationItem);

    this._lastSelectedSectionSetting = new WebInspector.Setting("last-selected-style-details-panel", this._rulesStyleDetailsPanel.navigationItem.identifier);

    // This will cause the selected panel to be set in _navigationItemSelected.
    this._navigationBar.selectedNavigationItem = this._lastSelectedSectionSetting.value;
};

WebInspector.CSSStyleDetailsSidebarPanel.ContentStyleClassName = "content";
WebInspector.CSSStyleDetailsSidebarPanel.PseudoClassesElementStyleClassName = "pseudo-classes";
WebInspector.CSSStyleDetailsSidebarPanel.PseudoClassesGroupElementStyleClassName = "group";
WebInspector.CSSStyleDetailsSidebarPanel.NoForcedPseudoClassesScrollOffset = 38; // Default height of the forced pseudo classes container. Updated in widthDidChange.

WebInspector.CSSStyleDetailsSidebarPanel.prototype = {
    constructor: WebInspector.CSSStyleDetailsSidebarPanel,

    // Public

    supportsDOMNode: function(nodeToInspect)
    {
        return nodeToInspect.nodeType() === Node.ELEMENT_NODE;
    },

    refresh: function()
    {
        var domNode = this.domNode;
        if (!domNode)
            return;

        this._contentElement.scrollTop = this._initialScrollOffset;

        for (var i = 0; i < this._panels.length; ++i) {
            delete this._panels[i].element._savedScrollTop;
            this._panels[i].markAsNeedsRefresh(domNode);
        }

        this._updatePseudoClassCheckboxes();
    },

    visibilityDidChange: function()
    {
        WebInspector.SidebarPanel.prototype.visibilityDidChange.call(this);

        if (!this._selectedPanel)
            return;

        if (!this.visible) {
            this._selectedPanel.hidden();
            return;
        }

        this._navigationBar.updateLayout();

        this._updateNoForcedPseudoClassesScrollOffset();

        this._selectedPanel.shown();
        this._selectedPanel.markAsNeedsRefresh(this.domNode);
    },

    widthDidChange: function()
    {
        this._updateNoForcedPseudoClassesScrollOffset();

        if (this._selectedPanel)
            this._selectedPanel.widthDidChange();
    },

    // Protected

    addEventListeners: function()
    {
        this.domNode.addEventListener(WebInspector.DOMNode.Event.EnabledPseudoClassesChanged, this._updatePseudoClassCheckboxes, this);
    },

    removeEventListeners: function()
    {
        this.domNode.removeEventListener(null, null, this);
    },

    // Private

    get _initialScrollOffset()
    {
        if (!WebInspector.cssStyleManager.canForcePseudoClasses())
            return 0;
        return this.domNode && this.domNode.enabledPseudoClasses.length ? 0 : WebInspector.CSSStyleDetailsSidebarPanel.NoForcedPseudoClassesScrollOffset;
    },

    _updateNoForcedPseudoClassesScrollOffset: function()
    {
        if (this._forcedPseudoClassContainer)
            WebInspector.CSSStyleDetailsSidebarPanel.NoForcedPseudoClassesScrollOffset = this._forcedPseudoClassContainer.offsetHeight;
    },

    _navigationItemSelected: function(event)
    {
        console.assert(event.target.selectedNavigationItem);
        if (!event.target.selectedNavigationItem)
            return;

        var selectedNavigationItem = event.target.selectedNavigationItem;

        var selectedPanel = null;
        for (var i = 0; i < this._panels.length; ++i) {
            if (this._panels[i].navigationItem !== selectedNavigationItem)
                continue;
            selectedPanel = this._panels[i];
            break;
        }

        console.assert(selectedPanel);

        if (this._selectedPanel) {
            this._selectedPanel.hidden();
            this._selectedPanel.element._savedScrollTop = this._contentElement.scrollTop;
            this._selectedPanel.element.remove();
        }

        this._selectedPanel = selectedPanel;

        if (this._selectedPanel) {
            this._contentElement.appendChild(this._selectedPanel.element);

            if (typeof this._selectedPanel.element._savedScrollTop === "number")
                this._contentElement.scrollTop = this._selectedPanel.element._savedScrollTop;
            else
                this._contentElement.scrollTop = this._initialScrollOffset;

            this._selectedPanel.shown();
        }

        this._lastSelectedSectionSetting.value = selectedNavigationItem.identifier;
    },

    _forcedPseudoClassCheckboxChanged: function(pseudoClass, event)
    {
        if (!this.domNode)
            return;

        this.domNode.setPseudoClassEnabled(pseudoClass, event.target.checked);
    },

    _updatePseudoClassCheckboxes: function()
    {
        if (!this.domNode)
            return;

        var enabledPseudoClasses = this.domNode.enabledPseudoClasses;

        for (var pseudoClass in this._forcedPseudoClassCheckboxes) {
            var checkboxElement = this._forcedPseudoClassCheckboxes[pseudoClass];
            checkboxElement.checked = enabledPseudoClasses.contains(pseudoClass);
        }
    }
};

WebInspector.CSSStyleDetailsSidebarPanel.prototype.__proto__ = WebInspector.DOMDetailsSidebarPanel.prototype;
