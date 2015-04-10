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

WebInspector.NavigationBar = function(element, navigationItems, role, label) {
    WebInspector.Object.call(this);

    this._element = element || document.createElement("div");
    this._element.classList.add(this.constructor.StyleClassName || WebInspector.NavigationBar.StyleClassName);
    this._element.tabIndex = 0;
    
    if (role)
        this._element.setAttribute("role", role);
    if (label)
        this._element.setAttribute("aria-label", label);

    this._element.addEventListener("focus", this._focus.bind(this), false);
    this._element.addEventListener("blur", this._blur.bind(this), false);
    this._element.addEventListener("keydown", this._keyDown.bind(this), false);
    this._element.addEventListener("mousedown", this._mouseDown.bind(this), false);

    document.addEventListener("load", this.updateLayout.bind(this), false);

    this._styleElement = document.createElement("style");

    this._navigationItems = [];

    if (navigationItems) {
        for (var i = 0; i < navigationItems.length; ++i)
            this.addNavigationItem(navigationItems[i]);
    }

    document.head.appendChild(this._styleElement);
};

WebInspector.Object.addConstructorFunctions(WebInspector.NavigationBar);

WebInspector.NavigationBar.StyleClassName = "navigation-bar";
WebInspector.NavigationBar.CollapsedStyleClassName = "collapsed";

WebInspector.NavigationBar.Event = {
    NavigationItemSelected: "navigation-bar-navigation-item-selected"
};

WebInspector.NavigationBar.prototype = {
    constructor: WebInspector.NavigationBar,

    // Public

    addNavigationItem: function(navigationItem, parentElement)
    {
        return this.insertNavigationItem(navigationItem, this._navigationItems.length, parentElement);
    },

    insertNavigationItem: function(navigationItem, index, parentElement)
    {
        console.assert(navigationItem instanceof WebInspector.NavigationItem);
        if (!(navigationItem instanceof WebInspector.NavigationItem))
            return;

        if (navigationItem.parentNavigationBar)
            navigationItem.parentNavigationBar.removeNavigationItem(navigationItem);

        navigationItem._parentNavigationBar = this;

        console.assert(index >= 0 && index <= this._navigationItems.length);
        index = Math.max(0, Math.min(index, this._navigationItems.length));

        this._navigationItems.splice(index, 0, navigationItem);

        if (!parentElement)
            parentElement = this._element;

        var nextSibling = this._navigationItems[index + 1];
        var nextSiblingElement = nextSibling ? nextSibling.element : null;
        if (nextSiblingElement && nextSiblingElement.parentNode !== parentElement)
            nextSiblingElement = null;

        parentElement.insertBefore(navigationItem.element, nextSiblingElement);

        this._minimumWidthNeedsRecalculation = true;
        this._needsStyleUpdated = true;

        this.updateLayoutSoon();

        return navigationItem;
    },

    removeNavigationItem: function(navigationItemOrIdentifierOrIndex, index)
    {
        var navigationItem = this._findNavigationItem(navigationItemOrIdentifierOrIndex);
        if (!navigationItem)
            return;

        navigationItem._parentNavigationBar = null;

        if (this._selectedNavigationItem === navigationItem)
            this.selectedNavigationItem = null;

        this._navigationItems.remove(navigationItem);
        navigationItem.element.remove();

        this._minimumWidthNeedsRecalculation = true;
        this._needsStyleUpdated = true;

        this.updateLayoutSoon();

        return navigationItem;
    },

    updateLayoutSoon: function()
    {
        if (this._updateLayoutTimeout)
            return;

        this._needsLayout = true;

        function update()
        {
            delete this._updateLayoutTimeout;

            if (this._needsLayout || this._needsStyleUpdated)
                this.updateLayout();
        }

        this._updateLayoutTimeout = setTimeout(update.bind(this), 0);
    },

    updateLayout: function()
    {
        if (this._updateLayoutTimeout) {
            clearTimeout(this._updateLayoutTimeout);
            delete this._updateLayoutTimeout;
        }

        if (this._needsStyleUpdated)
            this._updateStyle();

        this._needsLayout = false;

        if (typeof this.customUpdateLayout === "function") {
            this.customUpdateLayout();
            return;
        }

        // Remove the collapsed style class to test if the items can fit at full width.
        this._element.classList.remove(WebInspector.NavigationBar.CollapsedStyleClassName);

        // Tell each navigation item to update to full width if needed.
        for (var i = 0; i < this._navigationItems.length; ++i)
            this._navigationItems[i].updateLayout(true);

        var totalItemWidth = 0;
        for (var i = 0; i < this._navigationItems.length; ++i) {
            // Skip flexible space items since they can take up no space at the minimum width.
            if (this._navigationItems[i] instanceof WebInspector.FlexibleSpaceNavigationItem)
                continue;

            totalItemWidth += this._navigationItems[i].element.offsetWidth;
        }

        var barWidth = this._element.offsetWidth;

        // Add the collapsed class back if the items are wider than the bar.
        if (totalItemWidth > barWidth)
            this._element.classList.add(WebInspector.NavigationBar.CollapsedStyleClassName);

        // Give each navigation item the opportunity to collapse further.
        for (var i = 0; i < this._navigationItems.length; ++i)
            this._navigationItems[i].updateLayout();
    },

    get selectedNavigationItem()
    {
        return this._selectedNavigationItem || null;
    },

    set selectedNavigationItem(navigationItemOrIdentifierOrIndex)
    {
        var navigationItem = this._findNavigationItem(navigationItemOrIdentifierOrIndex);

        // Only radio navigation items can be selected.
        if (!(navigationItem instanceof WebInspector.RadioButtonNavigationItem))
            navigationItem = null;

        if (this._selectedNavigationItem === navigationItem)
            return;

        if (this._selectedNavigationItem)
            this._selectedNavigationItem.selected = false;

        this._selectedNavigationItem = navigationItem || null;

        if (this._selectedNavigationItem)
            this._selectedNavigationItem.selected = true;

        // When the mouse is down don't dispatch the selected event, it will be dispatched on mouse up.
        // This prevents sending the event while the user is scrubbing the bar.
        if (!this._mouseIsDown)
            this.dispatchEventToListeners(WebInspector.NavigationBar.Event.NavigationItemSelected);
    },

    get navigationItems()
    {
        return this._navigationItems;
    },

    get element()
    {
        return this._element;
    },

    get minimumWidth()
    {
        if (typeof this._minimumWidth === "undefined" || this._minimumWidthNeedsRecalculation) {
            this._minimumWidth = this._calculateMinimumWidth();
            delete this._minimumWidthNeedsRecalculation;
        }

        return this._minimumWidth;
    },

    get sizesToFit()
    {
        // Can be overriden by subclasses.
        return false;
    },

    // Private

    _findNavigationItem: function(navigationItemOrIdentifierOrIndex)
    {
        var navigationItem = null;

        if (navigationItemOrIdentifierOrIndex instanceof WebInspector.NavigationItem) {
            if (this._navigationItems.contains(navigationItemOrIdentifierOrIndex))
                navigationItem = navigationItemOrIdentifierOrIndex;
        } else if (typeof navigationItemOrIdentifierOrIndex === "number") {
            navigationItem = this._navigationItems[navigationItemOrIdentifierOrIndex];
        } else if (typeof navigationItemOrIdentifierOrIndex === "string") {
            for (var i = 0; i < this._navigationItems.length; ++i) {
                if (this._navigationItems[i].identifier === navigationItemOrIdentifierOrIndex) {
                    navigationItem = this._navigationItems[i];
                    break;
                }
            }
        }

        return navigationItem;
    },

    _mouseDown: function(event)
    {
        // Only handle left mouse clicks.
        if (event.button !== 0)
            return;

        // Remove the tabIndex so clicking the navigation bar does not give it focus.
        // Only keep the tabIndex if already focused from keyboard navigation. This matches Xcode.
        if (!this._focused)
            this._element.removeAttribute("tabindex");

        var itemElement = event.target.enclosingNodeOrSelfWithClass(WebInspector.RadioButtonNavigationItem.StyleClassName);
        if (!itemElement || !itemElement.navigationItem)
            return;

        this._previousSelectedNavigationItem = this.selectedNavigationItem;
        this.selectedNavigationItem = itemElement.navigationItem;

        this._mouseIsDown = true;

        this._mouseMovedEventListener = this._mouseMoved.bind(this);
        this._mouseUpEventListener = this._mouseUp.bind(this);

        // Register these listeners on the document so we can track the mouse if it leaves the resizer.
        document.addEventListener("mousemove", this._mouseMovedEventListener, false);
        document.addEventListener("mouseup", this._mouseUpEventListener, false);

        event.preventDefault();
        event.stopPropagation();
    },

    _mouseMoved: function(event)
    {
        console.assert(event.button === 0);
        console.assert(this._mouseIsDown);
        if (!this._mouseIsDown)
            return;

        event.preventDefault();
        event.stopPropagation();

        var itemElement = event.target.enclosingNodeOrSelfWithClass(WebInspector.RadioButtonNavigationItem.StyleClassName);
        if (!itemElement || !itemElement.navigationItem) {
            // Find the element that is at the X position of the mouse, even when the mouse is no longer
            // vertically in the navigation bar.
            var element = document.elementFromPoint(event.pageX, this._element.totalOffsetTop);
            if (!element)
                return;

            itemElement = element.enclosingNodeOrSelfWithClass(WebInspector.RadioButtonNavigationItem.StyleClassName);
            if (!itemElement || !itemElement.navigationItem)
                return;
        }

        if (this.selectedNavigationItem)
            this.selectedNavigationItem.active = false;

        this.selectedNavigationItem = itemElement.navigationItem;

        this.selectedNavigationItem.active = true;
    },

    _mouseUp: function(event)
    {
        console.assert(event.button === 0);
        console.assert(this._mouseIsDown);
        if (!this._mouseIsDown)
            return;

        if (this.selectedNavigationItem)
            this.selectedNavigationItem.active = false;

        this._mouseIsDown = false;

        document.removeEventListener("mousemove", this._mouseMovedEventListener, false);
        document.removeEventListener("mouseup", this._mouseUpEventListener, false);

        delete this._mouseMovedEventListener;
        delete this._mouseUpEventListener;

        // Restore the tabIndex so the navigation bar can be in the keyboard tab loop.
        this._element.tabIndex = 0;

        // Dispatch the selected event here since the selectedNavigationItem setter surpresses it
        // while the mouse is down to prevent sending it while scrubbing the bar.
        if (this._previousSelectedNavigationItem !== this.selectedNavigationItem)
            this.dispatchEventToListeners(WebInspector.NavigationBar.Event.NavigationItemSelected);

        delete this._previousSelectedNavigationItem;

        event.preventDefault();
        event.stopPropagation();
    },

    _keyDown: function(event)
    {
        if (!this._focused)
            return;

        if (event.keyIdentifier !== "Left" && event.keyIdentifier !== "Right")
            return;

        event.preventDefault();
        event.stopPropagation();

        var selectedNavigationItemIndex = this._navigationItems.indexOf(this._selectedNavigationItem);

        if (event.keyIdentifier === "Left") {
            if (selectedNavigationItemIndex === -1)
                selectedNavigationItemIndex = this._navigationItems.length;

            do {
                selectedNavigationItemIndex = Math.max(0, selectedNavigationItemIndex - 1);
            } while (selectedNavigationItemIndex && !(this._navigationItems[selectedNavigationItemIndex] instanceof WebInspector.RadioButtonNavigationItem));
        } else if (event.keyIdentifier === "Right") {
            do {
                selectedNavigationItemIndex = Math.min(selectedNavigationItemIndex + 1, this._navigationItems.length - 1);
            } while (selectedNavigationItemIndex < this._navigationItems.length - 1 && !(this._navigationItems[selectedNavigationItemIndex] instanceof WebInspector.RadioButtonNavigationItem));
        }

        if (!(this._navigationItems[selectedNavigationItemIndex] instanceof WebInspector.RadioButtonNavigationItem))
            return;

        this.selectedNavigationItem = this._navigationItems[selectedNavigationItemIndex];
    },

    _focus: function(event)
    {
        this._focused = true;
    },

    _blur: function(event)
    {
        this._focused = false;
    },

    _updateStyle: function()
    {
        this._needsStyleUpdated = false;

        var parentSelector = "." + (this.constructor.StyleClassName || WebInspector.NavigationBar.StyleClassName);

        var styleText = "";
        for (var i = 0; i < this._navigationItems.length; ++i) {
            if (!this._navigationItems[i].generateStyleText)
                continue;
            if (styleText)
                styleText += "\n";
            styleText += this._navigationItems[i].generateStyleText(parentSelector);
        }

        this._styleElement.textContent = styleText;
    },

    _calculateMinimumWidth: function()
    {
        var wasCollapsed = this._element.classList.contains(WebInspector.NavigationBar.CollapsedStyleClassName)

        // Add the collapsed style class to calculate the width of the items when they are collapsed.
        if (!wasCollapsed)
            this._element.classList.add(WebInspector.NavigationBar.CollapsedStyleClassName);

        var totalItemWidth = 0;
        for (var i = 0; i < this._navigationItems.length; ++i) {
            // Skip flexible space items since they can take up no space at the minimum width.
            if (this._navigationItems[i] instanceof WebInspector.FlexibleSpaceNavigationItem)
                continue;
            totalItemWidth += this._navigationItems[i].element.offsetWidth;
        }

        // Remove the collapsed style class if we were not collapsed before.
        if (!wasCollapsed)
            this._element.classList.remove(WebInspector.NavigationBar.CollapsedStyleClassName);

        return totalItemWidth;
    }
};

WebInspector.NavigationBar.prototype.__proto__ = WebInspector.Object.prototype;
