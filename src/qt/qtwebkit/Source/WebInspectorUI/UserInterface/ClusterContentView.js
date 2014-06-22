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

WebInspector.ClusterContentView = function(representedObject)
{
    WebInspector.ContentView.call(this, representedObject);

    this.element.classList.add(WebInspector.ClusterContentView.StyleClassName);

    this._contentViewContainer = new WebInspector.ContentViewContainer();
    this._contentViewContainer.addEventListener(WebInspector.ContentViewContainer.Event.CurrentContentViewDidChange, this._currentContentViewDidChange, this);
    this.element.appendChild(this._contentViewContainer.element);

    WebInspector.ContentView.addEventListener(WebInspector.ContentView.Event.SelectionPathComponentsDidChange, this._contentViewSelectionPathComponentDidChange, this);
    WebInspector.ContentView.addEventListener(WebInspector.ContentView.Event.SupplementalRepresentedObjectsDidChange, this._contentViewSupplementalRepresentedObjectsDidChange, this);
    WebInspector.ContentView.addEventListener(WebInspector.ContentView.Event.NumberOfSearchResultsDidChange, this._contentViewNumberOfSearchResultsDidChange, this);
};

WebInspector.ClusterContentView.StyleClassName = "cluster";

WebInspector.ClusterContentView.prototype = {
    constructor: WebInspector.ClusterContentView,

    // Public

    get navigationItems()
    {
        var currentContentView = this._contentViewContainer.currentContentView;
        return currentContentView ? currentContentView.navigationItems : [];
    },

    get contentViewContainer()
    {
        return this._contentViewContainer;
    },

    get supportsSplitContentBrowser()
    {
        if (this._contentViewContainer.currentContentView)
            return this._contentViewContainer.currentContentView.supportsSplitContentBrowser;
        return true;
    },

    updateLayout: function()
    {
        var currentContentView = this._contentViewContainer.currentContentView;
        if (currentContentView)
            currentContentView.updateLayout();
    },

    shown: function()
    {
        this._contentViewContainer.shown();
    },

    hidden: function()
    {
        this._contentViewContainer.hidden();
    },

    closed: function()
    {
        this._contentViewContainer.closeAllContentViews();

        WebInspector.ContentView.removeEventListener(WebInspector.ContentView.Event.SelectionPathComponentsDidChange, this._contentViewSelectionPathComponentDidChange, this);
        WebInspector.ContentView.removeEventListener(WebInspector.ContentView.Event.SupplementalRepresentedObjectsDidChange, this._contentViewSupplementalRepresentedObjectsDidChange, this);
        WebInspector.ContentView.removeEventListener(WebInspector.ContentView.Event.NumberOfSearchResultsDidChange, this._contentViewNumberOfSearchResultsDidChange, this);
    },

    canGoBack: function()
    {
        return this._contentViewContainer.canGoBack();
    },

    canGoForward: function()
    {
        return this._contentViewContainer.canGoForward();
    },

    goBack: function()
    {
        this._contentViewContainer.goBack()
    },

    goForward: function()
    {
        this._contentViewContainer.goForward()
    },

    get selectionPathComponents()
    {
        if (!this._contentViewContainer.currentContentView)
            return [];
        return this._contentViewContainer.currentContentView.selectionPathComponents;
    },

    get supplementalRepresentedObjects()
    {
        if (!this._contentViewContainer.currentContentView)
            return [];
        return this._contentViewContainer.currentContentView.supplementalRepresentedObjects;
    },

    get handleCopyEvent()
    {
        var currentContentView = this._contentViewContainer.currentContentView;
        return currentContentView && typeof currentContentView.handleCopyEvent === "function" ? currentContentView.handleCopyEvent.bind(currentContentView) : null;
    },

    get supportsSave()
    {
        var currentContentView = this._contentViewContainer.currentContentView;
        return currentContentView && currentContentView.supportsSave;
    },

    get saveData()
    {
        var currentContentView = this._contentViewContainer.currentContentView;
        return currentContentView && currentContentView.saveData || null;
    },

    get supportsSearch()
    {
        // Always return true so we can intercept the search query to resend it when switching content views.
        return true;
    },

    get numberOfSearchResults()
    {
        var currentContentView = this._contentViewContainer.currentContentView;
        if (!currentContentView || !currentContentView.supportsSearch)
            return null;
        return currentContentView.numberOfSearchResults;
    },

    get hasPerformedSearch()
    {
        var currentContentView = this._contentViewContainer.currentContentView;
        if (!currentContentView || !currentContentView.supportsSearch)
            return false;
        return currentContentView.hasPerformedSearch;
    },

    set automaticallyRevealFirstSearchResult(reveal)
    {
        var currentContentView = this._contentViewContainer.currentContentView;
        if (!currentContentView || !currentContentView.supportsSearch)
            return;
        currentContentView.automaticallyRevealFirstSearchResult = reveal;
    },

    performSearch: function(query)
    {
        this._searchQuery = query;

        var currentContentView = this._contentViewContainer.currentContentView;
        if (!currentContentView || !currentContentView.supportsSearch)
            return;
        currentContentView.performSearch(query);
    },

    searchCleared: function()
    {
        this._searchQuery = null;

        var currentContentView = this._contentViewContainer.currentContentView;
        if (!currentContentView || !currentContentView.supportsSearch)
            return;
        currentContentView.searchCleared();
    },

    searchQueryWithSelection: function()
    {
        var currentContentView = this._contentViewContainer.currentContentView;
        if (!currentContentView || !currentContentView.supportsSearch)
            return null;
        return currentContentView.searchQueryWithSelection();
    },

    revealPreviousSearchResult: function(changeFocus)
    {
        var currentContentView = this._contentViewContainer.currentContentView;
        if (!currentContentView || !currentContentView.supportsSearch)
            return;
        currentContentView.revealPreviousSearchResult(changeFocus);
    },

    revealNextSearchResult: function(changeFocus)
    {
        var currentContentView = this._contentViewContainer.currentContentView;
        if (!currentContentView || !currentContentView.supportsSearch)
            return;
        currentContentView.revealNextSearchResult(changeFocus);
    },

    // Private

    _currentContentViewDidChange: function(event)
    {
        var currentContentView = this._contentViewContainer.currentContentView;
        if (currentContentView && currentContentView.supportsSearch) {
            if (this._searchQuery)
                currentContentView.performSearch(this._searchQuery);
            else
                currentContentView.searchCleared();
        }

        this.dispatchEventToListeners(WebInspector.ContentView.Event.SelectionPathComponentsDidChange);
        this.dispatchEventToListeners(WebInspector.ContentView.Event.NumberOfSearchResultsDidChange);
        this.dispatchEventToListeners(WebInspector.ContentView.Event.NavigationItemsDidChange);
    },

    _contentViewSelectionPathComponentDidChange: function(event)
    {
        if (event.target !== this._contentViewContainer.currentContentView)
            return;
        this.dispatchEventToListeners(WebInspector.ContentView.Event.SelectionPathComponentsDidChange);
    },

    _contentViewSupplementalRepresentedObjectsDidChange: function(event)
    {
        if (event.target !== this._contentViewContainer.currentContentView)
            return;
        this.dispatchEventToListeners(WebInspector.ContentView.Event.SupplementalRepresentedObjectsDidChange);
    },

    _contentViewNumberOfSearchResultsDidChange: function(event)
    {
        if (event.target !== this._contentViewContainer.currentContentView)
            return;
        this.dispatchEventToListeners(WebInspector.ContentView.Event.NumberOfSearchResultsDidChange);
    }
};

WebInspector.ClusterContentView.prototype.__proto__ = WebInspector.ContentView.prototype;
