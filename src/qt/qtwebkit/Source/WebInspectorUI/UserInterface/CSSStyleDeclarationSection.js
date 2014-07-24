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

WebInspector.CSSStyleDeclarationSection = function(style)
{
    WebInspector.Object.call(this);

    console.assert(style);
    this._style = style || null;

    this._element = document.createElement("div");
    this._element.className = WebInspector.CSSStyleDeclarationSection.StyleClassName;

    this._headerElement = document.createElement("div");
    this._headerElement.className = WebInspector.CSSStyleDeclarationSection.HeaderElementStyleClassName;

    this._iconElement = document.createElement("img");
    this._iconElement.className = WebInspector.CSSStyleDeclarationSection.IconElementStyleClassName;
    this._headerElement.appendChild(this._iconElement);

    this._selectorElement = document.createElement("span");
    this._selectorElement.className = WebInspector.CSSStyleDeclarationSection.SelectorElementStyleClassName;
    this._selectorElement.setAttribute("spellcheck", "false");
    this._headerElement.appendChild(this._selectorElement);

    this._originElement = document.createElement("span");
    this._originElement.className = WebInspector.CSSStyleDeclarationSection.OriginElementStyleClassName;
    this._headerElement.appendChild(this._originElement);

    this._propertiesElement = document.createElement("div");
    this._propertiesElement.className = WebInspector.CSSStyleDeclarationSection.PropertiesElementStyleClassName;

    this._propertiesTextEditor = new WebInspector.CSSStyleDeclarationTextEditor(this, style);
    this._propertiesElement.appendChild(this._propertiesTextEditor.element);

    this._element.appendChild(this._headerElement);
    this._element.appendChild(this._propertiesElement);

    var iconClassName;
    switch (style.type) {
    case WebInspector.CSSStyleDeclaration.Type.Rule:
        console.assert(style.ownerRule);

        if (style.inherited)
            iconClassName = WebInspector.CSSStyleDeclarationSection.InheritedStyleRuleIconStyleClassName;
        else if (style.ownerRule.type === WebInspector.CSSRule.Type.Author)
            iconClassName = WebInspector.CSSStyleDeclarationSection.AuthorStyleRuleIconStyleClassName;
        else if (style.ownerRule.type === WebInspector.CSSRule.Type.User)
            iconClassName = WebInspector.CSSStyleDeclarationSection.UserStyleRuleIconStyleClassName;
        else if (style.ownerRule.type === WebInspector.CSSRule.Type.UserAgent)
            iconClassName = WebInspector.CSSStyleDeclarationSection.UserAgentStyleRuleIconStyleClassName;
        else if (style.ownerRule.type === WebInspector.CSSRule.Type.Inspector)
            iconClassName = WebInspector.CSSStyleDeclarationSection.InspectorStyleRuleIconStyleClassName;
        break;

    case WebInspector.CSSStyleDeclaration.Type.Inline:
    case WebInspector.CSSStyleDeclaration.Type.Attribute:
        if (style.inherited)
            iconClassName = WebInspector.CSSStyleDeclarationSection.InheritedElementStyleRuleIconStyleClassName;
        else
            iconClassName = WebInspector.DOMTreeElementPathComponent.DOMElementIconStyleClassName;
        break;
    }

    console.assert(iconClassName);
    this._element.classList.add(iconClassName);

    if (!style.editable)
        this._element.classList.add(WebInspector.CSSStyleDeclarationSection.LockedStyleClassName);
    else if (style.ownerRule) {
        this._commitSelectorKeyboardShortcut = new WebInspector.KeyboardShortcut(null, WebInspector.KeyboardShortcut.Key.Enter, this._commitSelector.bind(this), this._selectorElement);
        this._selectorElement.addEventListener("blur", this._commitSelector.bind(this));
    } else
        this._element.classList.add(WebInspector.CSSStyleDeclarationSection.SelectorLockedStyleClassName);

    if (!WebInspector.CSSStyleDeclarationSection._generatedLockImages) {
        WebInspector.CSSStyleDeclarationSection._generatedLockImages = true;

        var specifications = {"style-lock-normal": {fillColor: [0, 0, 0, 0.5]}};
        generateColoredImagesForCSS("Images/Locked.pdf", specifications, 8, 10);
    }

    this.refresh();
};

WebInspector.CSSStyleDeclarationSection.StyleClassName = "style-declaration-section";
WebInspector.CSSStyleDeclarationSection.LockedStyleClassName = "locked";
WebInspector.CSSStyleDeclarationSection.SelectorLockedStyleClassName = "selector-locked";
WebInspector.CSSStyleDeclarationSection.LastInGroupStyleClassName = "last-in-group";
WebInspector.CSSStyleDeclarationSection.HeaderElementStyleClassName = "header";
WebInspector.CSSStyleDeclarationSection.IconElementStyleClassName = "icon";
WebInspector.CSSStyleDeclarationSection.SelectorElementStyleClassName = "selector";
WebInspector.CSSStyleDeclarationSection.OriginElementStyleClassName = "origin";
WebInspector.CSSStyleDeclarationSection.PropertiesElementStyleClassName = "properties";
WebInspector.CSSStyleDeclarationSection.MatchedSelectorElementStyleClassName = "matched";

WebInspector.CSSStyleDeclarationSection.AuthorStyleRuleIconStyleClassName = "author-style-rule-icon";
WebInspector.CSSStyleDeclarationSection.UserStyleRuleIconStyleClassName = "user-style-rule-icon";
WebInspector.CSSStyleDeclarationSection.UserAgentStyleRuleIconStyleClassName = "user-agent-style-rule-icon";
WebInspector.CSSStyleDeclarationSection.InspectorStyleRuleIconStyleClassName = "inspector-style-rule-icon";
WebInspector.CSSStyleDeclarationSection.InheritedStyleRuleIconStyleClassName = "inherited-style-rule-icon";
WebInspector.CSSStyleDeclarationSection.InheritedElementStyleRuleIconStyleClassName = "inherited-element-style-rule-icon";

WebInspector.CSSStyleDeclarationSection.prototype = {
    constructor: WebInspector.CSSStyleDeclarationSection,

    // Public

    get element()
    {
        return this._element;
    },

    get style()
    {
        return this._style;
    },

    get lastInGroup()
    {
        return this._element.classList.contains(WebInspector.CSSStyleDeclarationSection.LastInGroupStyleClassName);
    },

    set lastInGroup(last)
    {
        if (last)
            this._element.classList.add(WebInspector.CSSStyleDeclarationSection.LastInGroupStyleClassName);
        else
            this._element.classList.remove(WebInspector.CSSStyleDeclarationSection.LastInGroupStyleClassName);
    },

    get focused()
    {
        return this._propertiesTextEditor.focused;
    },

    focus: function()
    {
        this._propertiesTextEditor.focus();
    },

    refresh: function()
    {
        this._selectorElement.removeChildren();
        this._originElement.removeChildren();

        this._originElement.appendChild(document.createTextNode(" \u2014 "));

        function appendSelector(selectorText, matched)
        {
            var selectorElement = document.createElement("span");
            if (matched)
                selectorElement.className = WebInspector.CSSStyleDeclarationSection.MatchedSelectorElementStyleClassName;
            selectorElement.textContent = selectorText;
            this._selectorElement.appendChild(selectorElement);
        }

        switch (this._style.type) {
        case WebInspector.CSSStyleDeclaration.Type.Rule:
            console.assert(this._style.ownerRule);

            var selectors = this._style.ownerRule.selectors;
            var matchedSelectorIndices = this._style.ownerRule.matchedSelectorIndices;
            if (selectors.length && matchedSelectorIndices.length) {
                for (var i = 0; i < selectors.length; ++i) {
                    appendSelector.call(this, selectors[i], matchedSelectorIndices.contains(i));
                    if (i < selectors.length - 1)
                        this._selectorElement.appendChild(document.createTextNode(", "));
                }
            } else
                appendSelector.call(this, this._style.ownerRule.selectorText, true);

            if (this._style.ownerRule.sourceCodeLocation) {
                var sourceCodeLink = WebInspector.createSourceCodeLocationLink(this._style.ownerRule.sourceCodeLocation, true);
                this._originElement.appendChild(sourceCodeLink);
            } else {
                var originString;
                switch (this._style.ownerRule.type) {
                case WebInspector.CSSRule.Type.Author:
                    originString = WebInspector.UIString("Author Stylesheet");
                    break;

                case WebInspector.CSSRule.Type.User:
                    originString = WebInspector.UIString("User Stylesheet");
                    break;

                case WebInspector.CSSRule.Type.UserAgent:
                    originString = WebInspector.UIString("User Agent Stylesheet");
                    break;

                case WebInspector.CSSRule.Type.Inspector:
                    originString = WebInspector.UIString("Web Inspector");
                    break;
                }

                console.assert(originString);
                if (originString)
                    this._originElement.appendChild(document.createTextNode(originString));
            }

            break;

        case WebInspector.CSSStyleDeclaration.Type.Inline:
            appendSelector.call(this, WebInspector.displayNameForNode(this._style.node), true);
            this._originElement.appendChild(document.createTextNode(WebInspector.UIString("Style Attribute")));
            break;

        case WebInspector.CSSStyleDeclaration.Type.Attribute:
            appendSelector.call(this, WebInspector.displayNameForNode(this._style.node), true);
            this._originElement.appendChild(document.createTextNode(WebInspector.UIString("HTML Attributes")));
            break;
        }
    },

    updateLayout: function()
    {
        this._propertiesTextEditor.updateLayout();
    },

    // Private

    _commitSelector: function(mutations)
    {
        console.assert(this._style.ownerRule);
        if (!this._style.ownerRule)
            return;

        var newSelectorText = this._selectorElement.textContent.trim();
        if (!newSelectorText) {
            // Revert to the current selector (by doing a refresh) since the new selector is empty.
            this.refresh();
            return;
        }

        this._style.ownerRule.selectorText = newSelectorText;
    }
};

WebInspector.CSSStyleDeclarationSection.prototype.__proto__ = WebInspector.StyleDetailsPanel.prototype;
