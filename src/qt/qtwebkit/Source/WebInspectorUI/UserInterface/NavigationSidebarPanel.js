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

WebInspector.NavigationSidebarPanel = function(identifier, displayName, image, keyboardShortcutKey, autoPruneOldTopLevelResourceTreeElements, autoHideToolbarItemWhenEmpty, wantsTopOverflowShadow, element, role, label) {
    if (keyboardShortcutKey)
        this._keyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Control, keyboardShortcutKey, this.toggle.bind(this));

    if (this._keyboardShortcut) {
        var showToolTip = WebInspector.UIString("Show the %s navigation sidebar (%s)").format(displayName, this._keyboardShortcut.displayName);
        var hideToolTip = WebInspector.UIString("Hide the %s navigation sidebar (%s)").format(displayName, this._keyboardShortcut.displayName);
    } else {
        var showToolTip = WebInspector.UIString("Show the %s navigation sidebar").format(displayName);
        var hideToolTip = WebInspector.UIString("Hide the %s navigation sidebar").format(displayName);
    }

    WebInspector.SidebarPanel.call(this, identifier, displayName, showToolTip, hideToolTip, image, element, role, label || displayName);

    this.element.classList.add(WebInspector.NavigationSidebarPanel.StyleClassName);

    this._autoHideToolbarItemWhenEmpty = autoHideToolbarItemWhenEmpty || false;

    if (autoHideToolbarItemWhenEmpty)
        this.toolbarItem.hidden = true;

    this._contentElement = document.createElement("div");
    this._contentElement.className = WebInspector.NavigationSidebarPanel.ContentElementStyleClassName;
    this._contentElement.addEventListener("scroll", this._updateContentOverflowShadowVisibility.bind(this));
    this.element.appendChild(this._contentElement);

    this._contentTreeOutline = this.createContentTreeOutline(true);

    this._filterBar = new WebInspector.FilterBar();
    this._filterBar.addEventListener(WebInspector.FilterBar.Event.TextFilterDidChange, this._updateFilter, this);
    this.element.appendChild(this._filterBar.element);

    this._bottomOverflowShadowElement = document.createElement("div");
    this._bottomOverflowShadowElement.className = WebInspector.NavigationSidebarPanel.OverflowShadowElementStyleClassName;
    this.element.appendChild(this._bottomOverflowShadowElement);

    if (wantsTopOverflowShadow) {
        this._topOverflowShadowElement = document.createElement("div");
        this._topOverflowShadowElement.classList.add(WebInspector.NavigationSidebarPanel.OverflowShadowElementStyleClassName);
        this._topOverflowShadowElement.classList.add(WebInspector.NavigationSidebarPanel.TopOverflowShadowElementStyleClassName);
        this.element.appendChild(this._topOverflowShadowElement);
    }

    window.addEventListener("resize", this._updateContentOverflowShadowVisibility.bind(this));

    this._filtersSetting = new WebInspector.Setting(identifier + "-navigation-sidebar-filters", {});
    this._filterBar.filters = this._filtersSetting.value;

    this._emptyContentPlaceholderElement = document.createElement("div");
    this._emptyContentPlaceholderElement.className = WebInspector.NavigationSidebarPanel.EmptyContentPlaceholderElementStyleClassName;

    this._emptyContentPlaceholderMessageElement = document.createElement("div");
    this._emptyContentPlaceholderMessageElement.className = WebInspector.NavigationSidebarPanel.EmptyContentPlaceholderMessageElementStyleClassName;
    this._emptyContentPlaceholderElement.appendChild(this._emptyContentPlaceholderMessageElement);

    this._generateStyleRulesIfNeeded();
    this._generateDisclosureTrianglesIfNeeded();

    if (autoPruneOldTopLevelResourceTreeElements) {
        WebInspector.Frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._checkForOldResources, this);
        WebInspector.Frame.addEventListener(WebInspector.Frame.Event.ChildFrameWasRemoved, this._checkForOldResources, this);
        WebInspector.Frame.addEventListener(WebInspector.Frame.Event.ResourceWasRemoved, this._checkForOldResources, this);
    }
};

WebInspector.NavigationSidebarPanel.StyleClassName = "navigation";
WebInspector.NavigationSidebarPanel.OverflowShadowElementStyleClassName = "overflow-shadow";
WebInspector.NavigationSidebarPanel.TopOverflowShadowElementStyleClassName = "top";
WebInspector.NavigationSidebarPanel.ContentElementStyleClassName = "content";
WebInspector.NavigationSidebarPanel.ContentElementHiddenStyleClassName = "hidden";
WebInspector.NavigationSidebarPanel.ContentTreeOutlineElementHiddenStyleClassName = "hidden";
WebInspector.NavigationSidebarPanel.ContentTreeOutlineElementStyleClassName = "navigation-sidebar-panel-content-tree-outline";
WebInspector.NavigationSidebarPanel.HideDisclosureButtonsStyleClassName = "hide-disclosure-buttons";
WebInspector.NavigationSidebarPanel.EmptyContentPlaceholderElementStyleClassName = "empty-content-placeholder";
WebInspector.NavigationSidebarPanel.EmptyContentPlaceholderMessageElementStyleClassName = "message";
WebInspector.NavigationSidebarPanel.DisclosureTriangleOpenCanvasIdentifier = "navigation-sidebar-panel-disclosure-triangle-open";
WebInspector.NavigationSidebarPanel.DisclosureTriangleClosedCanvasIdentifier = "navigation-sidebar-panel-disclosure-triangle-closed";
WebInspector.NavigationSidebarPanel.DisclosureTriangleNormalCanvasIdentifierSuffix = "-normal";
WebInspector.NavigationSidebarPanel.DisclosureTriangleSelectedCanvasIdentifierSuffix = "-selected";

WebInspector.NavigationSidebarPanel.prototype = {
    constructor: WebInspector.NavigationSidebarPanel,

    // Public

    get contentElement()
    {
        return this._contentElement;
    },

    get contentTreeOutlineElement()
    {
        return this._contentTreeOutline.element;
    },

    get contentTreeOutline()
    {
        return this._contentTreeOutline;
    },

    set contentTreeOutline(newTreeOutline)
    {
        console.assert(newTreeOutline);
        if (!newTreeOutline)
            return;

        if (this._contentTreeOutline)
            this._contentTreeOutline.element.classList.add(WebInspector.NavigationSidebarPanel.ContentTreeOutlineElementHiddenStyleClassName);

        this._contentTreeOutline = newTreeOutline;
        this._contentTreeOutline.element.classList.remove(WebInspector.NavigationSidebarPanel.ContentTreeOutlineElementHiddenStyleClassName);

        this._updateFilter();
    },

    get contentTreeOutlineToAutoPrune()
    {
        return this._contentTreeOutline;
    },

    get filterBar()
    {
        return this._filterBar;
    },

    createContentTreeOutline: function(dontHideByDefault)
    {
        var contentTreeOutlineElement = document.createElement("ol");
        contentTreeOutlineElement.className = WebInspector.NavigationSidebarPanel.ContentTreeOutlineElementStyleClassName;
        if (!dontHideByDefault)
            contentTreeOutlineElement.classList.add(WebInspector.NavigationSidebarPanel.ContentTreeOutlineElementHiddenStyleClassName);
        this._contentElement.appendChild(contentTreeOutlineElement);

        var contentTreeOutline = new TreeOutline(contentTreeOutlineElement);
        contentTreeOutline.onadd = this._treeElementAddedOrChanged.bind(this);
        contentTreeOutline.onchange = this._treeElementAddedOrChanged.bind(this);
        contentTreeOutline.onexpand = this._treeElementExpandedOrCollapsed.bind(this);
        contentTreeOutline.oncollapse = this._treeElementExpandedOrCollapsed.bind(this);
        contentTreeOutline.allowsRepeatSelection = true;

        return contentTreeOutline;
    },

    treeElementForRepresentedObject: function(representedObject)
    {
        return this._contentTreeOutline.getCachedTreeElement(representedObject);
    },

    cookieForContentView: function(contentView)
    {
        // Implemented by subclasses.
        return null;
    },

    showContentViewForCookie: function(contentViewCookie)
    {
        // Implemented by subclasses.
    },

    showContentViewForCurrentSelection: function()
    {
        // Reselect the selected tree element to cause the content view to be shown as well. <rdar://problem/10854727>
        var selectedTreeElement = this._contentTreeOutline.selectedTreeElement;
        if (selectedTreeElement)
            selectedTreeElement.select();
    },

    showEmptyContentPlaceholder: function(message, hideToolbarItem)
    {
        console.assert(message);

        this._contentElement.classList.add(WebInspector.NavigationSidebarPanel.ContentElementHiddenStyleClassName);
        this._emptyContentPlaceholderMessageElement.textContent = message;
        this.element.appendChild(this._emptyContentPlaceholderElement);

        this._hideToolbarItemWhenEmpty = hideToolbarItem || false;
        this._updateToolbarItemVisibility();
        this._updateContentOverflowShadowVisibility();
    },

    hideEmptyContentPlaceholder: function()
    {
        this._contentElement.classList.remove(WebInspector.NavigationSidebarPanel.ContentElementHiddenStyleClassName);
        if (this._emptyContentPlaceholderElement.parentNode)
            this._emptyContentPlaceholderElement.parentNode.removeChild(this._emptyContentPlaceholderElement);

        this._hideToolbarItemWhenEmpty = false;
        this._updateToolbarItemVisibility();
        this._updateContentOverflowShadowVisibility();
    },

    updateEmptyContentPlaceholder: function(message)
    {
        this._updateToolbarItemVisibility();

        if (!this._contentTreeOutline.children.length) {
            // No tree elements, so no results.
            this.showEmptyContentPlaceholder(message);
        } else if (!this._emptyFilterResults) {
            // There are tree elements, and not all of them are hidden by the filter.
            this.hideEmptyContentPlaceholder();
        }
    },

    applyFiltersToTreeElement: function(treeElement)
    {
        if (!this._filterBar.hasActiveFilters()) {
            // No filters, so make everything visible.
            treeElement.hidden = false;

            // If this tree element was expanded during filtering, collapse it again.
            if (treeElement.expanded && treeElement.__wasExpandedDuringFiltering) {
                delete treeElement.__wasExpandedDuringFiltering;
                treeElement.collapse();
            }

            return;
        }

        // Get the filterable data from the tree element.
        var filterableData = treeElement.filterableData;
        if (!filterableData)
            return;

        var self = this;
        function matchTextFilter(input)
        {
            if (!self._textFilterRegex)
                return true;

            // Convert to a single item array if needed.
            if (!(input instanceof Array))
                input = [input];

            // Loop over all the inputs and try to match them.
            for (var i = 0; i < input.length; ++i) {
                if (!input[i])
                    continue;
                if (self._textFilterRegex.test(input[i]))
                    return true;
            }

            // No inputs matched.
            return false;
        }

        function makeVisible()
        {
            // Make this element visible.
            treeElement.hidden = false;

            // Make the ancestors visible and expand them.
            var currentAncestor = treeElement.parent;
            while (currentAncestor && !currentAncestor.root) {
                currentAncestor.hidden = false;

                if (!currentAncestor.expanded) {
                    currentAncestor.__wasExpandedDuringFiltering = true;
                    currentAncestor.expand();
                }

                currentAncestor = currentAncestor.parent;
            }
        }

        if (matchTextFilter(filterableData.text)) {
            // Make this element visible since it matches.
            makeVisible();
            return;
        }

        // Make this element invisible since it does not match.
        treeElement.hidden = true;
    },

    show: function()
    {
        if (!this.parentSidebar)
            return;

        WebInspector.SidebarPanel.prototype.show.call(this);

        this.contentTreeOutlineElement.focus();
    },

    shown: function()
    {
        WebInspector.SidebarPanel.prototype.shown.call(this);

        this._updateContentOverflowShadowVisibility();

        // Force the navigation item to be visible. This makes sure it is
        // always visible when the panel is shown.
        this.toolbarItem.hidden = false;
    },

    hidden: function()
    {
        WebInspector.SidebarPanel.prototype.hidden.call(this);

        this._updateToolbarItemVisibility();
    },

    // Private

    _updateContentOverflowShadowVisibility: function()
    {
        var scrollHeight = this._contentElement.scrollHeight;
        var offsetHeight = this._contentElement.offsetHeight;

        if (scrollHeight < offsetHeight) {
            if (this._topOverflowShadowElement)
                this._topOverflowShadowElement.style.opacity = 0;
            this._bottomOverflowShadowElement.style.opacity = 0;
            return;
        }

        const edgeThreshold = 10;
        var scrollTop = this._contentElement.scrollTop;

        var topCoverage = Math.min(scrollTop, edgeThreshold);
        var bottomCoverage = Math.max(0, (offsetHeight + scrollTop) - (scrollHeight - edgeThreshold))

        if (this._topOverflowShadowElement)
            this._topOverflowShadowElement.style.opacity = (topCoverage / edgeThreshold).toFixed(1);
        this._bottomOverflowShadowElement.style.opacity = (1 - (bottomCoverage / edgeThreshold)).toFixed(1);
    },

    _updateToolbarItemVisibility: function()
    {
        // Hide the navigation item if requested or auto-hiding and we are not visible and we are empty.
        var shouldHide = ((this._hideToolbarItemWhenEmpty || this._autoHideToolbarItemWhenEmpty) && !this.selected && !this._contentTreeOutline.children.length);
        this.toolbarItem.hidden = shouldHide;
    },

    _checkForEmptyFilterResults: function()
    {
        // No tree elements, so don't touch the empty content placeholder.
        if (!this._contentTreeOutline.children.length)
            return;

        // Iterate over all the top level tree elements. If any are visible, return early.
        var currentTreeElement = this._contentTreeOutline.children[0];
        while (currentTreeElement) {
            if (!currentTreeElement.hidden) {
                // Not hidden, so hide any empty content message.
                this.hideEmptyContentPlaceholder();
                this._emptyFilterResults = false;
                return;
            }

            currentTreeElement = currentTreeElement.nextSibling;
        }

        // All top level tree elements are hidden, so filtering hid everything. Show a message.
        this.showEmptyContentPlaceholder(WebInspector.UIString("No Filter Results"));
        this._emptyFilterResults = true;
    },

    _updateFilter: function()
    {
        var filters = this._filterBar.filters;
        this._textFilterRegex = simpleGlobStringToRegExp(filters.text, "i");
        this._filtersSetting.value = filters;

        // Update the whole tree.
        var currentTreeElement = this._contentTreeOutline.children[0];
        while (currentTreeElement && !currentTreeElement.root) {
            this.applyFiltersToTreeElement(currentTreeElement);
            currentTreeElement = currentTreeElement.traverseNextTreeElement(false, null, false);
        }

        this._checkForEmptyFilterResults();
        this._updateContentOverflowShadowVisibility();
    },

    _treeElementAddedOrChanged: function(treeElement)
    {
        // Apply the filters to the tree element and its descendants.
        var currentTreeElement = treeElement;
        while (currentTreeElement && !currentTreeElement.root) {
            this.applyFiltersToTreeElement(currentTreeElement);
            currentTreeElement = currentTreeElement.traverseNextTreeElement(false, treeElement, false);
        }

        this._checkForEmptyFilterResults();
        this._updateContentOverflowShadowVisibility();
    },

    _treeElementExpandedOrCollapsed: function(treeElement)
    {
        this._updateContentOverflowShadowVisibility();
    },

    _generateStyleRulesIfNeeded: function()
    {
        if (WebInspector.NavigationSidebarPanel._styleElement)
            return;

        WebInspector.NavigationSidebarPanel._styleElement = document.createElement("style");

        const maximumSidebarTreeDepth = 15;
        const baseLeftPadding = 5; // Matches the padding in NavigationSidebarPanel.css for the item class. Keep in sync.
        const depthPadding = 16;

        var styleText = "";
        var childrenSubstring = " > ";
        for (var i = 1; i <= maximumSidebarTreeDepth; ++i) {
            childrenSubstring += ".children > ";
            styleText += "." + WebInspector.NavigationSidebarPanel.ContentTreeOutlineElementStyleClassName + childrenSubstring + ".item { ";
            styleText += "padding-left: " + (baseLeftPadding + (depthPadding * i)) + "px; }\n";
        }

        WebInspector.NavigationSidebarPanel._styleElement.textContent = styleText;

        document.head.appendChild(WebInspector.NavigationSidebarPanel._styleElement);
    },

    _generateDisclosureTrianglesIfNeeded: function()
    {
        if (WebInspector.NavigationSidebarPanel._generatedDisclosureTriangles)
            return;

        // Set this early instead of in _generateDisclosureTriangle because we don't want multiple panels that are
        // created at the same time to duplicate the work (even though it would be harmless.)
        WebInspector.NavigationSidebarPanel._generatedDisclosureTriangles = true;

        var specifications = {};
        specifications[WebInspector.NavigationSidebarPanel.DisclosureTriangleNormalCanvasIdentifierSuffix] = {
            fillColor: [112, 126, 139],
            shadowColor: [255, 255, 255, 0.8],
            shadowOffsetX: 0,
            shadowOffsetY: 1,
            shadowBlur: 0
        };

        specifications[WebInspector.NavigationSidebarPanel.DisclosureTriangleSelectedCanvasIdentifierSuffix] = {
            fillColor: [255, 255, 255],
            shadowColor: [61, 91, 110, 0.8],
            shadowOffsetX: 0,
            shadowOffsetY: 1,
            shadowBlur: 2
        };

        generateColoredImagesForCSS("Images/DisclosureTriangleSmallOpen.pdf", specifications, 13, 13, WebInspector.NavigationSidebarPanel.DisclosureTriangleOpenCanvasIdentifier);
        generateColoredImagesForCSS("Images/DisclosureTriangleSmallClosed.pdf", specifications, 13, 13, WebInspector.NavigationSidebarPanel.DisclosureTriangleClosedCanvasIdentifier);
    },

    _checkForOldResources: function(event)
    {
        if (this._checkForOldResourcesTimeoutIdentifier)
            return;

        function delayedWork()
        {
            delete this._checkForOldResourcesTimeoutIdentifier;

            var contentTreeOutline = this.contentTreeOutlineToAutoPrune;

            // Check all the ResourceTreeElements at the top level to make sure their Resource still has a parentFrame in the frame hierarchy.
            // If the parentFrame is no longer in the frame hierarchy we know it was removed due to a navigation or some other page change and
            // we should remove the issues for that resource.
            for (var i = contentTreeOutline.children.length - 1; i >= 0; --i) {
                var treeElement = contentTreeOutline.children[i];
                if (!(treeElement instanceof WebInspector.ResourceTreeElement))
                    continue;

                var resource = treeElement.resource;
                if (!resource.parentFrame || resource.parentFrame.isDetached())
                    contentTreeOutline.removeChildAtIndex(i, true, true);
            }

            if (typeof this._updateEmptyContentPlaceholder === "function")
                this._updateEmptyContentPlaceholder();
        }

        // Check on a delay to coalesce multiple calls to _checkForOldResources.
        this._checkForOldResourcesTimeoutIdentifier = setTimeout(delayedWork.bind(this), 0);
    }
};

WebInspector.NavigationSidebarPanel.prototype.__proto__ = WebInspector.SidebarPanel.prototype;
