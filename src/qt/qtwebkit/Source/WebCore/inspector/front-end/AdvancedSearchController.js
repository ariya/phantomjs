/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL GOOGLE INC.
 * OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 */
WebInspector.AdvancedSearchController = function()
{
    this._shortcut = WebInspector.AdvancedSearchController.createShortcut();
    this._searchId = 0;
    
    WebInspector.settings.advancedSearchConfig = WebInspector.settings.createSetting("advancedSearchConfig", new WebInspector.SearchConfig("", true, false));
    
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.FrameNavigated, this._frameNavigated, this);
}

/**
 * @return {!WebInspector.KeyboardShortcut.Descriptor}
 */
WebInspector.AdvancedSearchController.createShortcut = function()
{
    if (WebInspector.isMac())
        return WebInspector.KeyboardShortcut.makeDescriptor("f", WebInspector.KeyboardShortcut.Modifiers.Meta | WebInspector.KeyboardShortcut.Modifiers.Alt);
    else
        return WebInspector.KeyboardShortcut.makeDescriptor("f", WebInspector.KeyboardShortcut.Modifiers.Ctrl | WebInspector.KeyboardShortcut.Modifiers.Shift);
}

WebInspector.AdvancedSearchController.prototype = {
    /**
     * @param {KeyboardEvent} event
     * @return {boolean}
     */
    handleShortcut: function(event)
    {
        if (WebInspector.KeyboardShortcut.makeKeyFromEvent(event) === this._shortcut.key) {
            if (!this._searchView || !this._searchView.isShowing() || this._searchView._search !== document.activeElement) {
                WebInspector.showPanel("scripts");
                this.show();
            } else
                this.close();
            event.consume(true);
            return true;
        }
        return false;
    },

    _frameNavigated: function()
    {
        this.resetSearch();
    },

    /**
     * @param {WebInspector.SearchScope} searchScope
     */
    registerSearchScope: function(searchScope)
    {
        // FIXME: implement multiple search scopes.
        this._searchScope = searchScope;
    },

    show: function()
    {
        if (!this._searchView)
            this._searchView = new WebInspector.SearchView(this);
        
        this._searchView.syncToSelection();

        if (this._searchView.isShowing())
            this._searchView.focus();
        else
            WebInspector.showViewInDrawer(this._searchView._searchPanelElement, this._searchView, this.stopSearch.bind(this));
    },

    close: function()
    {
        this.stopSearch();
        WebInspector.closeViewInDrawer();
    },

    /**
     * @param {number} searchId
     * @param {WebInspector.FileBasedSearchResultsPane.SearchResult} searchResult
     */
    _onSearchResult: function(searchId, searchResult)
    {
        if (searchId !== this._searchId)
            return;

        this._searchView.addSearchResult(searchResult);
        if (!searchResult.searchMatches.length)
            return;
        
        if (!this._searchResultsPane) 
            this._searchResultsPane = this._currentSearchScope.createSearchResultsPane(this._searchConfig);        
        this._searchView.resultsPane = this._searchResultsPane; 
        this._searchResultsPane.addSearchResult(searchResult);
    },
    
    /**
     * @param {number} searchId
     * @param {boolean} finished
     */
    _onSearchFinished: function(searchId, finished)
    {
        if (searchId !== this._searchId)
            return;

        if (!this._searchResultsPane)
            this._searchView.nothingFound();
        
        this._searchView.searchFinished(finished);
    },
    
    /**
     * @param {WebInspector.SearchConfig} searchConfig
     */
    startSearch: function(searchConfig)
    {
        this.resetSearch();
        ++this._searchId;

        this._searchConfig = searchConfig;
        // FIXME: this._currentSearchScope should be initialized based on searchConfig
        this._currentSearchScope = this._searchScope;

        var totalSearchResultsCount = this._currentSearchScope.performSearch(searchConfig, this._onSearchResult.bind(this, this._searchId), this._onSearchFinished.bind(this, this._searchId));
        this._searchView.searchStarted(totalSearchResultsCount);
    },
    
    resetSearch: function()
    {
        this.stopSearch();

        if (this._searchResultsPane) {
            this._searchView.resetResults();
            delete this._searchResultsPane;
        }
    },
    
    stopSearch: function()
    {
        if (this._currentSearchScope)
            this._currentSearchScope.stopSearch();
    }
}

/**
 * @constructor
 * @extends {WebInspector.View}
 * @param {WebInspector.AdvancedSearchController} controller
 */
WebInspector.SearchView = function(controller)
{
    WebInspector.View.call(this);
    this.registerRequiredCSS("textEditor.css");
    
    this._controller = controller;

    this.element.className = "search-view";

    this._searchPanelElement = document.createElement("span");
    this._searchPanelElement.className = "search-drawer-header";
    this._searchPanelElement.addEventListener("keydown", this._onKeyDown.bind(this), false);
    
    this._searchResultsElement = this.element.createChild("div");
    this._searchResultsElement.className = "search-results";
    
    this._searchLabel = this._searchPanelElement.createChild("span");
    this._searchLabel.textContent = WebInspector.UIString("Search sources");
    this._search = this._searchPanelElement.createChild("input");
    this._search.setAttribute("type", "search");
    this._search.addStyleClass("search-config-search");
    this._search.setAttribute("results", "0");
    this._search.setAttribute("size", 30);

    this._ignoreCaseLabel = this._searchPanelElement.createChild("label");
    this._ignoreCaseLabel.addStyleClass("search-config-label");
    this._ignoreCaseCheckbox = this._ignoreCaseLabel.createChild("input");
    this._ignoreCaseCheckbox.setAttribute("type", "checkbox");
    this._ignoreCaseCheckbox.addStyleClass("search-config-checkbox");
    this._ignoreCaseLabel.appendChild(document.createTextNode(WebInspector.UIString("Ignore case")));
    
    this._regexLabel = this._searchPanelElement.createChild("label");
    this._regexLabel.addStyleClass("search-config-label");
    this._regexCheckbox = this._regexLabel.createChild("input");
    this._regexCheckbox.setAttribute("type", "checkbox");
    this._regexCheckbox.addStyleClass("search-config-checkbox");
    this._regexLabel.appendChild(document.createTextNode(WebInspector.UIString("Regular expression")));
    
    this._searchStatusBarElement = document.createElement("div");
    this._searchStatusBarElement.className = "search-status-bar-item";
    this._searchMessageElement = this._searchStatusBarElement.createChild("div");
    this._searchMessageElement.className = "search-status-bar-message";

    this._searchResultsMessageElement = document.createElement("span");
    this._searchResultsMessageElement.className = "search-results-status-bar-message";

    this._load();
}

// Number of recent search queries to store.
WebInspector.SearchView.maxQueriesCount = 20;

WebInspector.SearchView.prototype = {
    /**
     * @return {Array.<Element>}
     */
    statusBarItems: function()
    {
        return [this._searchStatusBarElement, this._searchResultsMessageElement];
    },

    /**
     * @return {WebInspector.SearchConfig}
     */
    get searchConfig()
    {
        return new WebInspector.SearchConfig(this._search.value, this._ignoreCaseCheckbox.checked, this._regexCheckbox.checked);
    },

    syncToSelection: function()
    {
        var selection = window.getSelection();
        if (selection.rangeCount)
            this._search.value = selection.toString().replace(/\r?\n.*/, "");
    },
    
    /**
     * @type {WebInspector.SearchResultsPane}
     */
    set resultsPane(resultsPane)
    {
        this.resetResults();
        this._searchResultsElement.appendChild(resultsPane.element);
    },
    
    /**
     * @param {number} totalSearchResultsCount
     */
    searchStarted: function(totalSearchResultsCount)
    {
        this.resetResults();
        this._resetCounters();

        this._searchMessageElement.textContent = WebInspector.UIString("Searching...");
        this._progressIndicator = new WebInspector.ProgressIndicator();
        this._progressIndicator.setTotalWork(totalSearchResultsCount);
        this._progressIndicator.show(this._searchStatusBarElement);
        
        this._updateSearchResultsMessage();
        
        if (!this._searchingView)
            this._searchingView = new WebInspector.EmptyView(WebInspector.UIString("Searching..."));
        this._searchingView.show(this._searchResultsElement);
    },

    _updateSearchResultsMessage: function()
    {
        if (this._searchMatchesCount && this._searchResultsCount)
            this._searchResultsMessageElement.textContent = WebInspector.UIString("Found %d matches in %d files.", this._searchMatchesCount, this._nonEmptySearchResultsCount);
        else
            this._searchResultsMessageElement.textContent = "";
    },

    resetResults: function()
    {
        if (this._searchingView)
            this._searchingView.detach();
        if (this._notFoundView)
            this._notFoundView.detach();
        this._searchResultsElement.removeChildren();
    },

    _resetCounters: function()
    {
        this._searchMatchesCount = 0;
        this._searchResultsCount = 0;
        this._nonEmptySearchResultsCount = 0;
    },

    nothingFound: function()
    {
        this.resetResults();

        if (!this._notFoundView)
            this._notFoundView = new WebInspector.EmptyView(WebInspector.UIString("No matches found."));
        this._notFoundView.show(this._searchResultsElement);
        this._searchResultsMessageElement.textContent = WebInspector.UIString("No matches found.");
    },

    /**
     * @param {WebInspector.FileBasedSearchResultsPane.SearchResult} searchResult
     */
    addSearchResult: function(searchResult)
    {
        this._searchMatchesCount += searchResult.searchMatches.length;
        this._searchResultsCount++;
        if (searchResult.searchMatches.length)
            this._nonEmptySearchResultsCount++;
        this._updateSearchResultsMessage();
        if (this._progressIndicator.isCanceled())
            this._onCancel();
        else
            this._progressIndicator.setWorked(this._searchResultsCount);
    },

    /**
     * @param {boolean} finished
     */
    searchFinished: function(finished)
    {
        this._progressIndicator.done();
        this._searchMessageElement.textContent = finished ? WebInspector.UIString("Search finished.") : WebInspector.UIString("Search interrupted.");
    },

    focus: function()
    {
        WebInspector.setCurrentFocusElement(this._search);
        this._search.select();
    },

    wasShown: function()
    {
        this.focus();
    },

    willHide: function()
    {
        this._controller.stopSearch();
    },

    /**
     * @param {Event} event
     */
    _onKeyDown: function(event)
    {
        switch (event.keyCode) {
        case WebInspector.KeyboardShortcut.Keys.Enter.code:
            this._onAction();
            break;
        case WebInspector.KeyboardShortcut.Keys.Esc.code:
            this._controller.close();
            event.consume(true);
            break;
        }        
    },
    
    _save: function()
    {
        var searchConfig = new WebInspector.SearchConfig(this.searchConfig.query, this.searchConfig.ignoreCase, this.searchConfig.isRegex); 
        WebInspector.settings.advancedSearchConfig.set(searchConfig);
    },
    
    _load: function()
    {
        var searchConfig = WebInspector.settings.advancedSearchConfig.get();
        this._search.value = searchConfig.query;
        this._ignoreCaseCheckbox.checked = searchConfig.ignoreCase;
        this._regexCheckbox.checked = searchConfig.isRegex;
    },

    _onCancel: function()
    {
        this._controller.stopSearch();
        this.focus();
    },
    
    _onAction: function()
    {
        if (!this.searchConfig.query || !this.searchConfig.query.length)
            return;
        
        this._save();
        this._controller.startSearch(this.searchConfig);
    },

    __proto__: WebInspector.View.prototype
}


/**
 * @constructor
 * @param {string} query
 * @param {boolean} ignoreCase
 * @param {boolean} isRegex
 */
WebInspector.SearchConfig = function(query, ignoreCase, isRegex)
{
    this.query = query;
    this.ignoreCase = ignoreCase;
    this.isRegex = isRegex;
}

/**
 * @interface
 */
WebInspector.SearchScope = function()
{
}

WebInspector.SearchScope.prototype = {
    /**
     * @param {WebInspector.SearchConfig} searchConfig
     * @param {function(WebInspector.FileBasedSearchResultsPane.SearchResult)} searchResultCallback
     * @param {function(boolean)} searchFinishedCallback
     */
    performSearch: function(searchConfig, searchResultCallback, searchFinishedCallback) { },

    stopSearch: function() { },
    
    /**
     * @param {WebInspector.SearchConfig} searchConfig
     * @return {WebInspector.SearchResultsPane}
     */
    createSearchResultsPane: function(searchConfig) { }
}

/**
 * @constructor
 * @param {number} offset
 * @param {number} length
 */
WebInspector.SearchResult = function(offset, length)
{
    this.offset = offset;
    this.length = length;    
}

/**
 * @constructor
 * @param {WebInspector.SearchConfig} searchConfig
 */
WebInspector.SearchResultsPane = function(searchConfig)
{
    this._searchConfig = searchConfig;
    this.element = document.createElement("div");
}

WebInspector.SearchResultsPane.prototype = {
    /**
     * @return {WebInspector.SearchConfig}
     */
    get searchConfig()
    {
        return this._searchConfig;
    },

    /**
     * @param {WebInspector.FileBasedSearchResultsPane.SearchResult} searchResult
     */
    addSearchResult: function(searchResult) { }
}

/**
 * @constructor
 * @extends {WebInspector.SearchResultsPane} 
 * @param {WebInspector.SearchConfig} searchConfig
 */
WebInspector.FileBasedSearchResultsPane = function(searchConfig)
{
    WebInspector.SearchResultsPane.call(this, searchConfig);
    
    this._searchResults = [];

    this.element.id ="search-results-pane-file-based";
    
    this._treeOutlineElement = document.createElement("ol");
    this._treeOutlineElement.className = "search-results-outline-disclosure";
    this.element.appendChild(this._treeOutlineElement);
    this._treeOutline = new TreeOutline(this._treeOutlineElement);
    
    this._matchesExpandedCount = 0;
}

WebInspector.FileBasedSearchResultsPane.matchesExpandedByDefaultCount = 20;
WebInspector.FileBasedSearchResultsPane.fileMatchesShownAtOnce = 20;

WebInspector.FileBasedSearchResultsPane.prototype = {
    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     * @param {number} columnNumber
     * @return {Element}
     */
    _createAnchor: function(uiSourceCode, lineNumber, columnNumber)
    {
        var anchor = document.createElement("a");
        anchor.preferredPanel = "scripts";
        anchor.href = sanitizeHref(uiSourceCode.originURL());
        anchor.uiSourceCode = uiSourceCode;
        anchor.lineNumber = lineNumber;
        return anchor;
    },

    /**
     * @param {WebInspector.FileBasedSearchResultsPane.SearchResult} searchResult
     */
    addSearchResult: function(searchResult)
    {
        this._searchResults.push(searchResult);
        var uiSourceCode = searchResult.uiSourceCode;
        var searchMatches = searchResult.searchMatches;

        var fileTreeElement = this._addFileTreeElement(uiSourceCode.originURL(), searchMatches.length, this._searchResults.length - 1);
    },

    /**
     * @param {WebInspector.FileBasedSearchResultsPane.SearchResult} searchResult
     * @param {TreeElement} fileTreeElement
     */
    _fileTreeElementExpanded: function(searchResult, fileTreeElement)
    {
        if (fileTreeElement._initialized)
            return;
        
        var toIndex = Math.min(searchResult.searchMatches.length, WebInspector.FileBasedSearchResultsPane.fileMatchesShownAtOnce);
        if (toIndex < searchResult.searchMatches.length) {
            this._appendSearchMatches(fileTreeElement, searchResult, 0, toIndex - 1);
            this._appendShowMoreMatchesElement(fileTreeElement, searchResult, toIndex - 1);
        } else
            this._appendSearchMatches(fileTreeElement, searchResult, 0, toIndex);
        
        fileTreeElement._initialized = true;
    },

    /**
     * @param {TreeElement} fileTreeElement
     * @param {WebInspector.FileBasedSearchResultsPane.SearchResult} searchResult
     * @param {number} fromIndex
     * @param {number} toIndex
     */
    _appendSearchMatches: function(fileTreeElement, searchResult, fromIndex, toIndex)
    {
        var uiSourceCode = searchResult.uiSourceCode;
        var searchMatches = searchResult.searchMatches;

        var regex = createSearchRegex(this._searchConfig.query, !this._searchConfig.ignoreCase, this._searchConfig.isRegex);
        for (var i = fromIndex; i < toIndex; ++i) {
            var lineNumber = searchMatches[i].lineNumber;
            var lineContent = searchMatches[i].lineContent;
            var matchRanges = this._regexMatchRanges(lineContent, regex);
            
            var anchor = this._createAnchor(uiSourceCode, lineNumber, matchRanges[0].offset);
            
            var numberString = numberToStringWithSpacesPadding(lineNumber + 1, 4);
            var lineNumberSpan = document.createElement("span");
            lineNumberSpan.addStyleClass("webkit-line-number");
            lineNumberSpan.addStyleClass("search-match-line-number");
            lineNumberSpan.textContent = numberString;
            anchor.appendChild(lineNumberSpan);
            
            var contentSpan = this._createContentSpan(lineContent, matchRanges);
            anchor.appendChild(contentSpan);
            
            var searchMatchElement = new TreeElement("", null, false);
            fileTreeElement.appendChild(searchMatchElement);
            searchMatchElement.listItemElement.className = "search-match source-code";
            searchMatchElement.listItemElement.appendChild(anchor);
        }
    },

    /**
     * @param {TreeElement} fileTreeElement
     * @param {WebInspector.FileBasedSearchResultsPane.SearchResult} searchResult
     * @param {number} startMatchIndex
     */
    _appendShowMoreMatchesElement: function(fileTreeElement, searchResult, startMatchIndex)
    {
        var matchesLeftCount = searchResult.searchMatches.length - startMatchIndex;
        var showMoreMatchesText = WebInspector.UIString("Show all matches (%d more).", matchesLeftCount);
        var showMoreMatchesElement = new TreeElement(showMoreMatchesText, null, false);
        fileTreeElement.appendChild(showMoreMatchesElement);
        showMoreMatchesElement.listItemElement.addStyleClass("show-more-matches");
        showMoreMatchesElement.onselect = this._showMoreMatchesElementSelected.bind(this, searchResult, startMatchIndex, showMoreMatchesElement);
    },

    /**
     * @param {WebInspector.FileBasedSearchResultsPane.SearchResult} searchResult
     * @param {number} startMatchIndex
     * @param {TreeElement} showMoreMatchesElement
     */
    _showMoreMatchesElementSelected: function(searchResult, startMatchIndex, showMoreMatchesElement)
    {
        var fileTreeElement = showMoreMatchesElement.parent;
        fileTreeElement.removeChild(showMoreMatchesElement);
        this._appendSearchMatches(fileTreeElement, searchResult, startMatchIndex, searchResult.searchMatches.length);
    },

    /**
     * @param {string} fileName
     * @param {number} searchMatchesCount
     * @param {number} searchResultIndex
     */
    _addFileTreeElement: function(fileName, searchMatchesCount, searchResultIndex)
    {
        var fileTreeElement = new TreeElement("", null, true);
        fileTreeElement.toggleOnClick = true;
        fileTreeElement.selectable = false;

        this._treeOutline.appendChild(fileTreeElement);
        fileTreeElement.listItemElement.addStyleClass("search-result");

        var fileNameSpan = document.createElement("span");
        fileNameSpan.className = "search-result-file-name";
        fileNameSpan.textContent = fileName;
        fileTreeElement.listItemElement.appendChild(fileNameSpan);

        var matchesCountSpan = document.createElement("span");
        matchesCountSpan.className = "search-result-matches-count";
        if (searchMatchesCount === 1)
            matchesCountSpan.textContent = WebInspector.UIString("(%d match)", searchMatchesCount);
        else
            matchesCountSpan.textContent = WebInspector.UIString("(%d matches)", searchMatchesCount);
        
        fileTreeElement.listItemElement.appendChild(matchesCountSpan);
        
        var searchResult = this._searchResults[searchResultIndex];
        fileTreeElement.onexpand = this._fileTreeElementExpanded.bind(this, searchResult, fileTreeElement);

        // Expand until at least certain amount of matches is expanded.
        if (this._matchesExpandedCount < WebInspector.FileBasedSearchResultsPane.matchesExpandedByDefaultCount)
            fileTreeElement.expand();
        this._matchesExpandedCount += searchResult.searchMatches.length;

        return fileTreeElement; 
    },

    /**
     * @param {string} lineContent
     * @param {RegExp} regex
     * @return {Array.<WebInspector.SearchResult>}
     */
    _regexMatchRanges: function(lineContent, regex)
    {
        regex.lastIndex = 0;
        var match;
        var offset = 0;
        var matchRanges = [];
        while ((regex.lastIndex < lineContent.length) && (match = regex.exec(lineContent)))
            matchRanges.push(new WebInspector.SearchResult(match.index, match[0].length));

        return matchRanges;
    },
    
    /**
     * @param {string} lineContent
     * @param {Array.<WebInspector.SearchResult>} matchRanges
     */
    _createContentSpan: function(lineContent, matchRanges)
    {
        var contentSpan = document.createElement("span");
        contentSpan.className = "search-match-content";
        contentSpan.textContent = lineContent;
        WebInspector.highlightRangesWithStyleClass(contentSpan, matchRanges, "highlighted-match");
        return contentSpan;
    },

    __proto__: WebInspector.SearchResultsPane.prototype
}

/**
 * @constructor
 * @param {WebInspector.UISourceCode} uiSourceCode
 * @param {Array.<Object>} searchMatches
 */
WebInspector.FileBasedSearchResultsPane.SearchResult = function(uiSourceCode, searchMatches) {
    this.uiSourceCode = uiSourceCode;
    this.searchMatches = searchMatches;
}

/**
 * @type {WebInspector.AdvancedSearchController}
 */
WebInspector.advancedSearchController = null;
