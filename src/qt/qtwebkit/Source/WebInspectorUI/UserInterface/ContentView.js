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

WebInspector.ContentView = function(representedObject)
{
    if (this.constructor === WebInspector.ContentView) {
        // When instantiated directly return an instance of a type-based concrete subclass.

        console.assert(representedObject);

        if (representedObject instanceof WebInspector.Frame)
            return new WebInspector.FrameContentView(representedObject);

        if (representedObject instanceof WebInspector.Resource)
            return new WebInspector.ResourceClusterContentView(representedObject);

        if (representedObject instanceof WebInspector.Script)
            return new WebInspector.ScriptContentView(representedObject);

        if (representedObject instanceof WebInspector.DOMStorageObject)
            return new WebInspector.DOMStorageContentView(representedObject);

        if (representedObject instanceof WebInspector.CookieStorageObject)
            return new WebInspector.CookieStorageContentView(representedObject);

        if (representedObject instanceof WebInspector.DatabaseTableObject)
            return new WebInspector.DatabaseTableContentView(representedObject);

        if (representedObject instanceof WebInspector.DatabaseObject)
            return new WebInspector.DatabaseContentView(representedObject);

        if (representedObject instanceof WebInspector.ApplicationCacheFrame)
            return new WebInspector.ApplicationCacheFrameContentView(representedObject);

        if (representedObject instanceof WebInspector.DOMTree)
            return new WebInspector.DOMTreeContentView(representedObject);

        if (representedObject instanceof WebInspector.LogObject)
            return new WebInspector.LogContentView(representedObject);

        if (representedObject instanceof WebInspector.TimelinesObject)
            return new WebInspector.TimelinesContentView(representedObject);

        if (representedObject instanceof WebInspector.JavaScriptProfileObject)
            return new WebInspector.JavaScriptProfileView(representedObject);
        
        if (representedObject instanceof WebInspector.CSSSelectorProfileObject)
            return new WebInspector.CSSSelectorProfileView(representedObject);

        if (typeof representedObject === "string" || representedObject instanceof String)
            return new WebInspector.TextContentView(representedObject);

        console.assert(!WebInspector.ContentView.isViewable(representedObject));

        throw "Can't make a ContentView for an unknown representedObject.";
    }

    // Concrete object instantiation.
    console.assert(this.constructor !== WebInspector.ContentView && this instanceof WebInspector.ContentView);
    console.assert(WebInspector.ContentView.isViewable(representedObject));

    WebInspector.Object.call(this);

    this._representedObject = representedObject;

    this._element = document.createElement("div");
    this._element.classList.add(WebInspector.ContentView.StyleClassName);

    this._parentContainer = null;
};

WebInspector.Object.addConstructorFunctions(WebInspector.ContentView);

WebInspector.ContentView.isViewable = function(representedObject)
{
    if (representedObject instanceof WebInspector.Frame)
        return true;
    if (representedObject instanceof WebInspector.Resource)
        return true;
    if (representedObject instanceof WebInspector.Script)
        return true;
    if (representedObject instanceof WebInspector.DOMStorageObject)
        return true;
    if (representedObject instanceof WebInspector.CookieStorageObject)
        return true;
    if (representedObject instanceof WebInspector.DatabaseTableObject)
        return true;
    if (representedObject instanceof WebInspector.DatabaseObject)
        return true;
    if (representedObject instanceof WebInspector.ApplicationCacheFrame)
        return true;
    if (representedObject instanceof WebInspector.DOMTree)
        return true;
    if (representedObject instanceof WebInspector.LogObject)
        return true;
    if (representedObject instanceof WebInspector.TimelinesObject)
        return true;
    if (representedObject instanceof WebInspector.JavaScriptProfileObject)
        return true;
    if (representedObject instanceof WebInspector.CSSSelectorProfileObject)
        return true;
    if (typeof representedObject === "string" || representedObject instanceof String)
        return true;
    return false;
};

WebInspector.ContentView.StyleClassName = "content-view";

WebInspector.ContentView.Event = {
    SelectionPathComponentsDidChange: "content-view-selection-path-components-did-change",
    SupplementalRepresentedObjectsDidChange: "content-view-supplemental-represented-objects-did-change",
    NumberOfSearchResultsDidChange: "content-view-number-of-search-results-did-change",
    NavigationItemsDidChange: "content-view-navigation-items-did-change"
};

WebInspector.ContentView.prototype = {
    constructor: WebInspector.ContentView,

    // Public

    get representedObject()
    {
        return this._representedObject;
    },

    get navigationItems()
    {
        // Navigation items that will be displayed by the ContentBrowser instance,
        // meant to be subclassed. Implemented by subclasses.
        return [];
    },

    get allowedNavigationSidebarPanels()
    {
        // Allow any navigation sidebar panel.
        return [];
    },

    get element()
    {
        return this._element;
    },

    get parentContainer()
    {
        return this._parentContainer;
    },

    get visible()
    {
        return this._visible;
    },

    set visible(flag)
    {
        this._visible = flag;
    },

    get scrollableElements()
    {
        // Implemented by subclasses.
        return [];
    },

    get shouldKeepElementsScrolledToBottom()
    {
        // Implemented by subclasses.
        return false;
    },

    get selectionPathComponents()
    {
        // Implemented by subclasses.
        return [];
    },

    get supplementalRepresentedObjects()
    {
        // Implemented by subclasses.
        return [];
    },

    get supportsSplitContentBrowser()
    {
        // Implemented by subclasses.
        return true;
    },

    updateLayout: function()
    {
        // Implemented by subclasses.
    },

    shown: function()
    {
        // Implemented by subclasses.
    },

    hidden: function()
    {
        // Implemented by subclasses.
    },

    closed: function()
    {
        // Implemented by subclasses.
    },

    canGoBack: function()
    {
        // Implemented by subclasses.
        return false;
    },

    canGoForward: function()
    {
        // Implemented by subclasses.
        return false;
    },

    goBack: function()
    {
        // Implemented by subclasses.
    },

    goForward: function()
    {
        // Implemented by subclasses.
    },

    get supportsSearch()
    {
        // Implemented by subclasses.
        return false;
    },

    get numberOfSearchResults()
    {
        // Implemented by subclasses.
        return null;
    },

    get hasPerformedSearch()
    {
        // Implemented by subclasses.
        return false;
    },

    set automaticallyRevealFirstSearchResult(reveal)
    {
        // Implemented by subclasses.
    },

    performSearch: function(query)
    {
        // Implemented by subclasses.
    },

    searchCleared: function()
    {
        // Implemented by subclasses.
    },

    searchQueryWithSelection: function()
    {
        // Implemented by subclasses.
        return null;
    },

    revealPreviousSearchResult: function(changeFocus)
    {
        // Implemented by subclasses.
    },

    revealNextSearchResult: function(changeFocus)
    {
        // Implemented by subclasses.
    }
};

WebInspector.ContentView.prototype.__proto__ = WebInspector.Object.prototype;
