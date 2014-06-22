/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Matt Lilek (pewtermoose@gmail.com).
 * Copyright (C) 2009 Joseph Pecoraro
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 */
WebInspector.SearchController = function()
{
    this._element = document.createElement("table");
    this._element.className = "toolbar-search";
    this._element.cellSpacing = 0;

    this._firstRowElement = this._element.createChild("tr");
    this._secondRowElement = this._element.createChild("tr", "hidden");

    // Column 1
    var searchControlElementColumn = this._firstRowElement.createChild("td"); 
    this._searchControlElement = searchControlElementColumn.createChild("span", "toolbar-search-control");
    this._searchInputElement = this._searchControlElement.createChild("input", "search-replace");
    this._searchInputElement.id = "search-input-field";
    this._searchInputElement.placeholder = WebInspector.UIString("Find");

    this._filterControlElement = searchControlElementColumn.createChild("span", "toolbar-search-control");
    this._filterControlElement.addStyleClass("hidden");
    this._filterInputElement = this._filterControlElement.createChild("input", "filter");
    this._filterInputElement.id = "filter-input-field";
    this._filterInputElement.placeholder = WebInspector.UIString("Filter");

    this._matchesElement = this._searchControlElement.createChild("label", "search-results-matches");
    this._matchesElement.setAttribute("for", "search-input-field");

    var searchNavigationElement = this._searchControlElement.createChild("div", "toolbar-search-navigation-controls");

    this._searchNavigationPrevElement = searchNavigationElement.createChild("div", "toolbar-search-navigation toolbar-search-navigation-prev");
    this._searchNavigationPrevElement.addEventListener("click", this._onPrevButtonSearch.bind(this), false);
    this._searchNavigationPrevElement.title = WebInspector.UIString("Search Previous");

    this._searchNavigationNextElement = searchNavigationElement.createChild("div", "toolbar-search-navigation toolbar-search-navigation-next"); 
    this._searchNavigationNextElement.addEventListener("click", this._onNextButtonSearch.bind(this), false);
    this._searchNavigationNextElement.title = WebInspector.UIString("Search Next");

    this._searchInputElement.addEventListener("mousedown", this._onSearchFieldManualFocus.bind(this), false); // when the search field is manually selected
    this._searchInputElement.addEventListener("keydown", this._onKeyDown.bind(this), true);
    this._filterInputElement.addEventListener("keydown", this._onKeyDown.bind(this), true);
    this._filterInputElement.addEventListener("input", this._onFilterInput.bind(this), false);
    this._searchInputElement.addEventListener("input", this._onSearchInput.bind(this), false);

    this._replaceInputElement = this._secondRowElement.createChild("td").createChild("input", "search-replace toolbar-replace-control");
    this._replaceInputElement.addEventListener("keydown", this._onKeyDown.bind(this), true);
    this._replaceInputElement.placeholder = WebInspector.UIString("Replace");

    // Column 2
    this._findButtonElement = this._firstRowElement.createChild("td").createChild("button", "hidden");
    this._findButtonElement.textContent = WebInspector.UIString("Find");
    this._findButtonElement.tabIndex = -1;
    this._findButtonElement.addEventListener("click", this._onNextButtonSearch.bind(this), false);

    this._replaceButtonElement = this._secondRowElement.createChild("td").createChild("button");
    this._replaceButtonElement.textContent = WebInspector.UIString("Replace");
    this._replaceButtonElement.disabled = true;
    this._replaceButtonElement.tabIndex = -1;
    this._replaceButtonElement.addEventListener("click", this._replace.bind(this), false);

    // Column 3
    this._prevButtonElement = this._firstRowElement.createChild("td").createChild("button", "hidden");
    this._prevButtonElement.textContent = WebInspector.UIString("Previous");
    this._prevButtonElement.disabled = true;
    this._prevButtonElement.tabIndex = -1;
    this._prevButtonElement.addEventListener("click", this._onPrevButtonSearch.bind(this), false);

    this._replaceAllButtonElement = this._secondRowElement.createChild("td").createChild("button");
    this._replaceAllButtonElement.textContent = WebInspector.UIString("Replace All");
    this._replaceAllButtonElement.addEventListener("click", this._replaceAll.bind(this), false);

    // Column 4
    this._replaceElement = this._firstRowElement.createChild("td").createChild("span");

    this._replaceCheckboxElement = this._replaceElement.createChild("input");
    this._replaceCheckboxElement.type = "checkbox";
    this._replaceCheckboxElement.id = "search-replace-trigger";
    this._replaceCheckboxElement.addEventListener("click", this._updateSecondRowVisibility.bind(this), false);

    this._replaceLabelElement = this._replaceElement.createChild("label");
    this._replaceLabelElement.textContent = WebInspector.UIString("Replace");
    this._replaceLabelElement.setAttribute("for", "search-replace-trigger");

    // Column 5
    this._filterCheckboxContainer = this._firstRowElement.createChild("td").createChild("span");

    this._filterCheckboxElement = this._filterCheckboxContainer.createChild("input");
    this._filterCheckboxElement.type = "checkbox";
    this._filterCheckboxElement.id = "filter-trigger";
    this._filterCheckboxElement.addEventListener("click", this._filterCheckboxClick.bind(this), false);
  
    this._filterLabelElement = this._filterCheckboxContainer.createChild("label");
    this._filterLabelElement.textContent = WebInspector.UIString("Filter");
    this._filterLabelElement.setAttribute("for", "filter-trigger");

    // Column 6
    var cancelButtonElement = this._firstRowElement.createChild("td").createChild("button");
    cancelButtonElement.textContent = WebInspector.UIString("Cancel");
    cancelButtonElement.tabIndex = -1;
    cancelButtonElement.addEventListener("click", this.cancelSearch.bind(this), false);
}

WebInspector.SearchController.prototype = {
    updateSearchMatchesCount: function(matches, panel)
    {
        if (!panel)
            panel = WebInspector.inspectorView.currentPanel();

        panel.currentSearchMatches = matches;

        if (panel === WebInspector.inspectorView.currentPanel())
            this._updateSearchMatchesCountAndCurrentMatchIndex(WebInspector.inspectorView.currentPanel().currentQuery ? matches : 0, -1);
    },

    updateCurrentMatchIndex: function(currentMatchIndex, panel)
    {
        if (panel === WebInspector.inspectorView.currentPanel())
            this._updateSearchMatchesCountAndCurrentMatchIndex(panel.currentSearchMatches, currentMatchIndex);
    },

    cancelSearch: function()
    {
        if (!this._searchIsVisible)
            return;
        if (this._filterCheckboxElement.checked) {
            this._filterCheckboxElement.checked = false;
            this._switchFilterToSearch();
        } 
        delete this._searchIsVisible;
        WebInspector.inspectorView.setFooterElement(null);
        this.resetSearch();
    },

    resetSearch: function()
    {
        this._performSearch("", false, false);
        this._updateReplaceVisibility();
        this._matchesElement.textContent = "";
    },

    disableSearchUntilExplicitAction: function()
    {
        this._performSearch("", false, false);
    },

    /**
     * @param {Event} event
     * @return {boolean}
     */
    handleShortcut: function(event)
    {
        var isMac = WebInspector.isMac();

        switch (event.keyIdentifier) {
            case "U+0046": // F key
                if (isMac)
                    var isFindKey = event.metaKey && !event.ctrlKey && !event.altKey && !event.shiftKey;
                else
                    var isFindKey = event.ctrlKey && !event.metaKey && !event.altKey && !event.shiftKey;

                if (isFindKey) {
                    this.showSearchField();
                    event.consume(true);
                    return true;
                }
                break;

            case "F3":
                if (!isMac) {
                    this.showSearchField();
                    event.consume();
                }
                break;

            case "U+0047": // G key
                var currentPanel = WebInspector.inspectorView.currentPanel();

                if (isMac && event.metaKey && !event.ctrlKey && !event.altKey) {
                    if (event.shiftKey)
                        currentPanel.jumpToPreviousSearchResult();
                    else
                        currentPanel.jumpToNextSearchResult();
                    event.consume(true);
                    return true;
                }
                break;
        }
        return false;
    },

    _updateSearchNavigationButtonState: function(enabled)
    {
        this._replaceButtonElement.disabled = !enabled;
        this._prevButtonElement.disabled = !enabled;
        var panel = WebInspector.inspectorView.currentPanel();
        if (enabled) {
            this._searchNavigationPrevElement.addStyleClass("enabled");
            this._searchNavigationNextElement.addStyleClass("enabled");
        } else {
            this._searchNavigationPrevElement.removeStyleClass("enabled");
            this._searchNavigationNextElement.removeStyleClass("enabled");
        }
    },

    /**
     * @param {number} matches
     * @param {number} currentMatchIndex
     */
    _updateSearchMatchesCountAndCurrentMatchIndex: function(matches, currentMatchIndex)
    {
        if (matches === 0 || currentMatchIndex >= 0)
            this._matchesElement.textContent = WebInspector.UIString("%d of %d", currentMatchIndex + 1, matches);
        this._updateSearchNavigationButtonState(matches > 0);
    },

    showSearchField: function()
    {
        WebInspector.inspectorView.setFooterElement(this._element);
        this._updateReplaceVisibility();
        this._updateFilterVisibility();
        if (WebInspector.currentFocusElement() !== this._searchInputElement) {
            var selection = window.getSelection();
            if (selection.rangeCount)
                this._searchInputElement.value = selection.toString().replace(/\r?\n.*/, "");
        }
        this._performSearch(this._searchInputElement.value, true, false);
        this._searchInputElement.focus();
        this._searchInputElement.select();
        this._searchIsVisible = true;
    },

    _switchFilterToSearch: function()
    {
        this._filterControlElement.addStyleClass("hidden");
        this._searchControlElement.removeStyleClass("hidden");
        this._searchInputElement.focus();
        this._searchInputElement.select();
        this._searchInputElement.value = this._filterInputElement.value;
        this.resetFilter();
    },

    _switchSearchToFilter: function()
    {
        this._filterControlElement.removeStyleClass("hidden");
        this._searchControlElement.addStyleClass("hidden");
        this._filterInputElement.focus();
        this._filterInputElement.select();
        this._filterInputElement.value = this._searchInputElement.value;
        this.resetSearch();
    },
    
    _updateFilterVisibility: function()
    {
        if (WebInspector.inspectorView.currentPanel().canFilter())
            this._filterCheckboxContainer.removeStyleClass("hidden");
        else
            this._filterCheckboxContainer.addStyleClass("hidden");
    },
  
    _updateReplaceVisibility: function()
    {
        var panel = WebInspector.inspectorView.currentPanel();
        if (panel && panel.canSearchAndReplace())
            this._replaceElement.removeStyleClass("hidden");
        else {
            this._replaceElement.addStyleClass("hidden");
            this._replaceCheckboxElement.checked = false;
            this._updateSecondRowVisibility();
        }
    },

    _onSearchFieldManualFocus: function(event)
    {
        WebInspector.setCurrentFocusElement(event.target);
    },

    _onKeyDown: function(event)
    {
        // Escape Key will clear the field and clear the search results
        if (event.keyCode === WebInspector.KeyboardShortcut.Keys.Esc.code) {
            event.consume(true);
            this.cancelSearch();
            WebInspector.setCurrentFocusElement(WebInspector.previousFocusElement());
            if (WebInspector.currentFocusElement() === event.target)
                WebInspector.currentFocusElement().select();
            return false;
        }

        if (isEnterKey(event)) {
            if (event.target === this._searchInputElement)
                this._performSearch(event.target.value, true, event.shiftKey);
            else if (event.target === this._replaceInputElement)
                this._replace();
        }
    },

    _onNextButtonSearch: function(event)
    {
        // Simulate next search on search-navigation-button click.
        this._performSearch(this._searchInputElement.value, true, false);
        this._searchInputElement.focus();
    },

    _onPrevButtonSearch: function(event)
    {
        if (!this._searchNavigationPrevElement.hasStyleClass("enabled"))
            return;
        // Simulate previous search on search-navigation-button click.
        this._performSearch(this._searchInputElement.value, true, true);
        this._searchInputElement.focus();
    },

    /**
     * @param {string} query
     * @param {boolean} forceSearch
     * @param {boolean} isBackwardSearch
     */
    _performSearch: function(query, forceSearch, isBackwardSearch)
    {
        if (!query || !query.length) {
            delete this._currentQuery;

            for (var panelName in WebInspector.panels) {
                var panel = WebInspector.panels[panelName];
                var hadCurrentQuery = !!panel.currentQuery;
                delete panel.currentQuery;
                if (hadCurrentQuery)
                    panel.searchCanceled();
            }
            this._updateSearchMatchesCountAndCurrentMatchIndex(0, -1);
            return;
        }

        var currentPanel = WebInspector.inspectorView.currentPanel();
        if (query === currentPanel.currentQuery && currentPanel.currentQuery === this._currentQuery) {
            // When this is the same query and a forced search, jump to the next
            // search result for a good user experience.
            if (forceSearch) {
                if (!isBackwardSearch)
                    currentPanel.jumpToNextSearchResult();
                else if (isBackwardSearch)
                    currentPanel.jumpToPreviousSearchResult();
            }
            return;
        }

        if (!forceSearch && query.length < 3 && !this._currentQuery)
            return;

        this._currentQuery = query;

        currentPanel.currentQuery = query;
        currentPanel.performSearch(query);
    },

    _updateSecondRowVisibility: function()
    {
        if (!this._searchIsVisible)
            return;
        if (this._replaceCheckboxElement.checked) {
            this._element.addStyleClass("toolbar-search-replace");
            this._secondRowElement.removeStyleClass("hidden");
            this._prevButtonElement.removeStyleClass("hidden");
            this._findButtonElement.removeStyleClass("hidden");
            this._replaceCheckboxElement.tabIndex = -1;
            this._replaceInputElement.focus();
        } else {
            this._element.removeStyleClass("toolbar-search-replace");
            this._secondRowElement.addStyleClass("hidden");
            this._prevButtonElement.addStyleClass("hidden");
            this._findButtonElement.addStyleClass("hidden");
            this._replaceCheckboxElement.tabIndex = 0;
            this._searchInputElement.focus();
        }
        WebInspector.inspectorView.setFooterElement(this._element);
    },

    _replace: function()
    {
        var currentPanel = WebInspector.inspectorView.currentPanel();
        currentPanel.replaceSelectionWith(this._replaceInputElement.value);
        var query = this._currentQuery;
        delete this._currentQuery;
        this._performSearch(query, true, false);
    },

    _replaceAll: function()
    {
        var currentPanel = WebInspector.inspectorView.currentPanel();
        currentPanel.replaceAllWith(this._searchInputElement.value, this._replaceInputElement.value);
    },
  
    _filterCheckboxClick: function()
    {
        if (this._filterCheckboxElement.checked) { 
            this._switchSearchToFilter();
            this._performFilter(this._filterInputElement.value);
        } else {
            this._switchFilterToSearch();
            this._performSearch(this._searchInputElement.value, false, false);
        }
    },
    
    /**
     * @param {string} query
     */
    _performFilter: function(query)
    {
        WebInspector.inspectorView.currentPanel().performFilter(query);
    },
  
    _onFilterInput: function(event)
    {
        this._performFilter(event.target.value);
    },
  
    _onSearchInput: function(event)
    {
        this._performSearch(event.target.value, false, false);
    },
    
    resetFilter: function()
    {
        this._performFilter("");
    }
}

/**
 * @type {?WebInspector.SearchController}
 */
WebInspector.searchController = null;
