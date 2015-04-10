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

WebInspector.StyleDetailsPanel = function(className, identifier, label)
{
    this._element = document.createElement("div");
    this._element.className = className;

    // Add this offset-sections class name so the sticky headers don't overlap the navigation bar.
    this.element.classList.add(WebInspector.StyleDetailsPanel.OffsetSectionsStyleClassName);

    this._navigationItem = new WebInspector.RadioButtonNavigationItem(identifier, label);

    this._nodeStyles = null;
    this._visible = false;
};

WebInspector.StyleDetailsPanel.OffsetSectionsStyleClassName = "offset-sections";

WebInspector.StyleDetailsPanel.prototype = {
    constructor: WebInspector.StyleDetailsPanel,

    // Public

    get element()
    {
        return this._element;
    },

    get navigationItem()
    {
        return this._navigationItem;
    },

    get nodeStyles()
    {
        return this._nodeStyles;
    },

    shown: function()
    {
        if (this._visible)
            return;

        this._visible = true;

        this._refreshNodeStyles();
    },

    hidden: function()
    {
        this._visible = false;
    },

    widthDidChange: function()
    {
        // Implemented by subclasses.
    },

    markAsNeedsRefresh: function(domNode)
    {
        console.assert(domNode);
        if (!domNode)
            return;

        if (!this._nodeStyles || this._nodeStyles.node !== domNode) {
            if (this._nodeStyles) {
                this._nodeStyles.removeEventListener(WebInspector.DOMNodeStyles.Event.Refreshed, this._nodeStylesRefreshed, this);
                this._nodeStyles.removeEventListener(WebInspector.DOMNodeStyles.Event.NeedsRefresh, this._nodeStylesNeedsRefreshed, this);
            }

            this._nodeStyles = WebInspector.cssStyleManager.stylesForNode(domNode);

            console.assert(this._nodeStyles);
            if (!this._nodeStyles)
                return;

            this._nodeStyles.addEventListener(WebInspector.DOMNodeStyles.Event.Refreshed, this._nodeStylesRefreshed, this);
            this._nodeStyles.addEventListener(WebInspector.DOMNodeStyles.Event.NeedsRefresh, this._nodeStylesNeedsRefreshed, this);

            this._forceSignificantChange = true;
        }

        if (this._visible)
            this._refreshNodeStyles();
    },

    refresh: function(significantChange)
    {
        // Implemented by subclasses.
    },

    // Private

    get _initialScrollOffset()
    {
        if (!WebInspector.cssStyleManager.canForcePseudoClasses())
            return 0;
        return this.nodeStyles.node.enabledPseudoClasses.length ? 0 : WebInspector.CSSStyleDetailsSidebarPanel.NoForcedPseudoClassesScrollOffset;
    },

    _refreshNodeStyles: function()
    {
        if (!this._nodeStyles)
            return;
        this._nodeStyles.refresh();
    },

    _refreshPreservingScrollPosition: function(significantChange)
    {
        significantChange = this._forceSignificantChange || significantChange || false;
        delete this._forceSignificantChange;

        var previousScrollTop = this._initialScrollOffset;

        // Only remember the scroll position if the previous node is the same as this one.
        if (this.element.parentNode && this._previousRefreshNodeIdentifier === this._nodeStyles.node.id)
            previousScrollTop = this.element.parentNode.scrollTop;

        this.refresh(significantChange);

        this._previousRefreshNodeIdentifier = this._nodeStyles.node.id;

        if (this.element.parentNode)
            this.element.parentNode.scrollTop = previousScrollTop;
    },

    _nodeStylesRefreshed: function(event)
    {
        if (this._visible)
            this._refreshPreservingScrollPosition(event.data.significantChange);
    },

    _nodeStylesNeedsRefreshed: function(event)
    {
        if (this._visible)
            this._refreshNodeStyles();
    }
};

WebInspector.StyleDetailsPanel.prototype.__proto__ = WebInspector.Object.prototype;
