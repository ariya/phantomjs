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

WebInspector.CSSRule = function(nodeStyles, ownerStyleSheet, id, type, sourceCodeLocation, selectorText, selectors, matchedSelectorIndices, style, mediaList)
{
    WebInspector.Object.call(this);

    console.assert(nodeStyles);
    this._nodeStyles = nodeStyles;

    this._ownerStyleSheet = ownerStyleSheet || null;
    this._id = id || null;
    this._type = type || null;

    this.update(sourceCodeLocation, selectorText, selectors, matchedSelectorIndices, style, mediaList, true);
};

WebInspector.Object.addConstructorFunctions(WebInspector.CSSRule);

WebInspector.CSSRule.Event = {
    Changed: "css-rule-changed"
};

WebInspector.CSSRule.Type = {
    Author: "css-rule-type-author",
    User: "css-rule-type-user",
    UserAgent: "css-rule-type-user-agent",
    Inspector: "css-rule-type-inspector"
};

WebInspector.CSSRule.prototype = {
    constructor: WebInspector.CSSRule,

    // Public

    get id()
    {
        return this._id;
    },

    get ownerStyleSheet()
    {
        return this._ownerStyleSheet;
    },

    get editable()
    {
        return !!this._id && (this._type === WebInspector.CSSRule.Type.Author || this._type === WebInspector.CSSRule.Type.Inspector);
    },

    update: function(sourceCodeLocation, selectorText, selectors, matchedSelectorIndices, style, mediaList, dontFireEvents)
    {
        sourceCodeLocation = sourceCodeLocation || null;
        selectorText = selectorText || "";
        selectors = selectors || [];
        matchedSelectorIndices = matchedSelectorIndices || [];
        style = style || null;
        mediaList = mediaList || [];

        var changed = false;
        if (!dontFireEvents) {
            changed = this._selectorText !== selectorText || !Object.shallowEqual(this._selectors, selectors) ||
                !Object.shallowEqual(this._matchedSelectorIndices, matchedSelectorIndices) || this._style !== style ||
                !!this._sourceCodeLocation !== !!sourceCodeLocation || this._mediaList.length !== mediaList.length;
            // FIXME: Look for differences in the media list arrays.
        }

        if (this._style)
            this._style.ownerRule = null;

        this._sourceCodeLocation = sourceCodeLocation;
        this._selectorText = selectorText;
        this._selectors = selectors;
        this._matchedSelectorIndices = matchedSelectorIndices;
        this._style = style;
        this._mediaList = mediaList;

        delete this._matchedSelectors;
        delete this._matchedSelectorText;

        if (this._style)
            this._style.ownerRule = this;

        if (changed)
            this.dispatchEventToListeners(WebInspector.CSSRule.Event.Changed);
    },

    get type()
    {
        return this._type;
    },

    get sourceCodeLocation()
    {
        return this._sourceCodeLocation;
    },

    get selectorText()
    {
        return this._selectorText;
    },

    set selectorText(selectorText)
    {
        console.assert(this.editable);
        if (!this.editable)
            return;

        if (this._selectorText === selectorText)
            return;

        this._nodeStyles.changeRuleSelector(this, selectorText);
    },

    get selectors()
    {
        return this._selectors;
    },

    set selectors(selectors)
    {
        this.selectorText = (selectors || []).join(", ");
    },

    get matchedSelectorIndices()
    {
        return this._matchedSelectorIndices;
    },

    get matchedSelectors()
    {
        // COMPATIBILITY (iOS 6): The selectors array is always empty, so just return an empty array.
        if (!this._selectors.length) {
            console.assert(!this._matchedSelectorIndices.length);
            return [];
        }

        if (this._matchedSelectors)
            return this._matchedSelectors;

        this._matchedSelectors = this._selectors.filter(function(element, index) {
            return this._matchedSelectorIndices.contains(index);
        }, this);

        return this._matchedSelectors;
    },

    get matchedSelectorText()
    {
        // COMPATIBILITY (iOS 6): The selectors array is always empty, so just return the whole selector.
        if (!this._selectors.length) {
            console.assert(!this._matchedSelectorIndices.length);
            return this._selectorText;
        }

        if ("_matchedSelectorText" in this)
            return this._matchedSelectorText;

        this._matchedSelectorText = this.matchedSelectors.join(", ");

        return this._matchedSelectorText;
    },

    get style()
    {
        return this._style;
    },

    get mediaList()
    {
        return this._mediaList;
    },

    // Protected

    get nodeStyles()
    {
        return this._nodeStyles;
    }
};

WebInspector.CSSRule.prototype.__proto__ = WebInspector.Object.prototype;
