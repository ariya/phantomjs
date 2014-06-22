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

WebInspector.HierarchicalPathComponent = function(displayName, styleClassNames, representedObject, textOnly, showSelectorArrows)
{
    WebInspector.Object.call(this);

    console.assert(displayName);
    console.assert(styleClassNames);

    this._representedObject = representedObject || null;

    this._element = document.createElement("div");
    this._element.className = WebInspector.HierarchicalPathComponent.StyleClassName;

    if (!(styleClassNames instanceof Array))
        styleClassNames = [styleClassNames];

    for (var i = 0; i < styleClassNames.length; ++i) {
        if (!styleClassNames[i])
            continue;
        this._element.classList.add(styleClassNames[i]);
    }

    if (!textOnly) {
        this._iconElement = document.createElement("img");
        this._iconElement.className = WebInspector.HierarchicalPathComponent.IconElementStyleClassName;
        this._element.appendChild(this._iconElement);
    } else
        this._element.classList.add(WebInspector.HierarchicalPathComponent.TextOnlyStyleClassName);

    this._titleElement = document.createElement("div");
    this._titleElement.className = WebInspector.HierarchicalPathComponent.TitleElementStyleClassName;
    this._element.appendChild(this._titleElement);

    this._titleContentElement = document.createElement("div");
    this._titleContentElement.className = WebInspector.HierarchicalPathComponent.TitleContentElementStyleClassName;
    this._titleElement.appendChild(this._titleContentElement);

    this._separatorElement = document.createElement("div");
    this._separatorElement.className = WebInspector.HierarchicalPathComponent.SeparatorElementStyleClassName;
    this._element.appendChild(this._separatorElement);

    this._selectElement = document.createElement("select");
    this._selectElement.addEventListener("mouseover", this._selectElementMouseOver.bind(this));
    this._selectElement.addEventListener("mouseout", this._selectElementMouseOut.bind(this));
    this._selectElement.addEventListener("mousedown", this._selectElementMouseDown.bind(this));
    this._selectElement.addEventListener("mouseup", this._selectElementMouseUp.bind(this));
    this._selectElement.addEventListener("change", this._selectElementSelectionChanged.bind(this));
    this._element.appendChild(this._selectElement);

    this._previousSibling = null;
    this._nextSibling = null;

    this._truncatedDisplayNameLength = 0;

    this.selectorArrows = showSelectorArrows;
    this.displayName = displayName;
};

WebInspector.HierarchicalPathComponent.StyleClassName = "hierarchical-path-component";
WebInspector.HierarchicalPathComponent.HiddenStyleClassName = "hidden";
WebInspector.HierarchicalPathComponent.CollapsedStyleClassName = "collapsed";
WebInspector.HierarchicalPathComponent.IconElementStyleClassName = "icon";
WebInspector.HierarchicalPathComponent.TextOnlyStyleClassName = "text-only";
WebInspector.HierarchicalPathComponent.ShowSelectorArrowsStyleClassName = "show-selector-arrows";
WebInspector.HierarchicalPathComponent.TitleElementStyleClassName = "title";
WebInspector.HierarchicalPathComponent.TitleContentElementStyleClassName = "content";
WebInspector.HierarchicalPathComponent.SelectorArrowsElementStyleClassName = "selector-arrows";
WebInspector.HierarchicalPathComponent.SeparatorElementStyleClassName = "separator";

WebInspector.HierarchicalPathComponent.MinimumWidth = 32;
WebInspector.HierarchicalPathComponent.MinimumWidthCollapsed = 24;
WebInspector.HierarchicalPathComponent.MinimumWidthForOneCharacterTruncatedTitle = 54;
WebInspector.HierarchicalPathComponent.SelectorArrowsWidth = 12;

WebInspector.HierarchicalPathComponent.Event = {
    SiblingWasSelected: "hierarchical-path-component-sibling-was-selected",
    Clicked: "hierarchical-path-component-clicked"
};

WebInspector.HierarchicalPathComponent.prototype = {
    constructor: WebInspector.HierarchicalPathComponent,

    // Public

    get element()
    {
        return this._element;
    },

    get representedObject()
    {
        return this._representedObject;
    },

    get displayName()
    {
        return this._displayName;
    },

    set displayName(newDisplayName)
    {
        console.assert(newDisplayName);
        if (newDisplayName === this._displayName)
            return;

        this._displayName = newDisplayName;

        this._updateElementTitleAndText();
    },

    get truncatedDisplayNameLength()
    {
        return this._truncatedDisplayNameLength;
    },

    set truncatedDisplayNameLength(truncatedDisplayNameLength)
    {
        truncatedDisplayNameLength = truncatedDisplayNameLength || 0;

        if (truncatedDisplayNameLength === this._truncatedDisplayNameLength)
            return;

        this._truncatedDisplayNameLength = truncatedDisplayNameLength;

        this._updateElementTitleAndText();
    },

    get minimumWidth()
    {
        if (this.collapsed)
            return WebInspector.HierarchicalPathComponent.MinimumWidthCollapsed;
        if (this.selectorArrows)
            return WebInspector.HierarchicalPathComponent.MinimumWidth + WebInspector.HierarchicalPathComponent.SelectorArrowsWidth;
        return WebInspector.HierarchicalPathComponent.MinimumWidth;
    },

    get forcedWidth()
    {
        var maxWidth = this._element.style.getProperty("width");
        if (typeof maxWidth === "string")
            return parseInt(maxWidth);
        return null;
    },

    set forcedWidth(width)
    {
        if (typeof width === "number") {
            var minimumWidthForOneCharacterTruncatedTitle = WebInspector.HierarchicalPathComponent.MinimumWidthForOneCharacterTruncatedTitle;
            if (this.selectorArrows)
                minimumWidthForOneCharacterTruncatedTitle += WebInspector.HierarchicalPathComponent.SelectorArrowsWidth;

            // If the width is less than the minimum width required to show a single character and ellipsis, then
            // just collapse down to the bare minimum to show only the icon.
            if (width < minimumWidthForOneCharacterTruncatedTitle)
                width = 0;

            // Ensure the width does not go less than 1px. If the width is 0 the layout gets funky. There is a min-width
            // in the CSS too, so as long the width is less than min-width we get the desired effect of only showing the icon.
            this._element.style.setProperty("width", Math.max(1, width) + "px");
        } else
            this._element.style.removeProperty("width");
    },

    get hidden()
    {
        return this._element.classList.contains(WebInspector.HierarchicalPathComponent.HiddenStyleClassName);
    },

    set hidden(flag)
    {
        if (flag)
            this._element.classList.add(WebInspector.HierarchicalPathComponent.HiddenStyleClassName);
        else
            this._element.classList.remove(WebInspector.HierarchicalPathComponent.HiddenStyleClassName);
    },

    get collapsed()
    {
        return this._element.classList.contains(WebInspector.HierarchicalPathComponent.CollapsedStyleClassName);
    },

    set collapsed(flag)
    {
        if (flag)
            this._element.classList.add(WebInspector.HierarchicalPathComponent.CollapsedStyleClassName);
        else
            this._element.classList.remove(WebInspector.HierarchicalPathComponent.CollapsedStyleClassName);
    },

    get selectorArrows()
    {
        return this._element.classList.contains(WebInspector.HierarchicalPathComponent.ShowSelectorArrowsStyleClassName);
    },

    set selectorArrows(flag)
    {
        if (flag) {
            this._selectorArrowsElement = document.createElement("img");
            this._selectorArrowsElement.className = WebInspector.HierarchicalPathComponent.SelectorArrowsElementStyleClassName;
            this._element.insertBefore(this._selectorArrowsElement, this._separatorElement);

            this._element.classList.add(WebInspector.HierarchicalPathComponent.ShowSelectorArrowsStyleClassName);
        } else {
            if (this._selectorArrowsElement) {
                this._selectorArrowsElement.remove();
                delete this._selectorArrowsElement;
            }

            this._element.classList.remove(WebInspector.HierarchicalPathComponent.ShowSelectorArrowsStyleClassName);
        }
    },

    get previousSibling()
    {
        return this._previousSibling;
    },

    set previousSibling(newSlibling)
    {
        this._previousSibling = newSlibling || null;
    },

    get nextSibling()
    {
        return this._nextSibling;
    },

    set nextSibling(newSlibling)
    {
        this._nextSibling = newSlibling || null;
    },

    // Private

    _updateElementTitleAndText: function()
    {
        var truncatedDisplayName = this._displayName;
        if (this._truncatedDisplayNameLength && truncatedDisplayName.length > this._truncatedDisplayNameLength)
            truncatedDisplayName = truncatedDisplayName.substring(0, this._truncatedDisplayNameLength) + "\u2026";

        this._element.title = this._displayName;
        this._titleContentElement.textContent = truncatedDisplayName;
    },

    _updateSelectElement: function()
    {
        this._selectElement.removeChildren();

        function createOption(component)
        {
            var optionElement = document.createElement("option");
            const maxPopupMenuLength = 130; // <rdar://problem/13445374> <select> with very long option has clipped text and popup menu is still very wide
            optionElement.textContent = component.displayName.length <= maxPopupMenuLength ? component.displayName : component.displayName.substring(0, maxPopupMenuLength) + "\u2026";
            optionElement._pathComponent = component;
            return optionElement;
        }

        var previousSiblingCount = 0;
        var sibling = this.previousSibling;
        while (sibling) {
            this._selectElement.insertBefore(createOption(sibling), this._selectElement.firstChild);
            sibling = sibling.previousSibling;
            ++previousSiblingCount;
        }

        this._selectElement.appendChild(createOption(this));

        sibling = this.nextSibling;
        while (sibling) {
            this._selectElement.appendChild(createOption(sibling));
            sibling = sibling.nextSibling;
        }

        // Since the change event only fires when the selection actually changes we are
        // stuck with either not showing the current selection in the menu or accepting that
        // the user can't select what is already selected again. Selecting the same item
        // again can be desired (for selecting the main resource while looking at an image).
        // So if there is only one option, don't make it be selected by default. This lets
        // you select the top level item which usually has no siblings to go back.
        // FIXME: Make this work when there are multiple options with a selectedIndex.
        if (this._selectElement.options.length === 1)
            this._selectElement.selectedIndex = -1;
        else
            this._selectElement.selectedIndex = previousSiblingCount;
    },

    _selectElementMouseOver: function(event)
    {
        if (typeof this.mouseOver === "function")
            this.mouseOver();
    },

    _selectElementMouseOut: function(event)
    {
        if (typeof this.mouseOut === "function")
            this.mouseOut();
    },

    _selectElementMouseDown: function(event)
    {
        this._updateSelectElement();
    },

    _selectElementMouseUp: function(event)
    {
        this.dispatchEventToListeners(WebInspector.HierarchicalPathComponent.Event.Clicked);
    },

    _selectElementSelectionChanged: function(event)
    {
        this.dispatchEventToListeners(WebInspector.HierarchicalPathComponent.Event.SiblingWasSelected, {pathComponent: this._selectElement[this._selectElement.selectedIndex]._pathComponent});
    }
};

WebInspector.HierarchicalPathComponent.prototype.__proto__ = WebInspector.Object.prototype;
