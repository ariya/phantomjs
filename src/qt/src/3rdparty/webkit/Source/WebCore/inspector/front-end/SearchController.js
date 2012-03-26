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

WebInspector.SearchController = function()
{
    this.element = document.getElementById("search");
    this._matchesElement = document.getElementById("search-results-matches");
    this._toolbarLabelElement = document.getElementById("search-toolbar-label");

    this.element.addEventListener("search", this._onSearch.bind(this), false); // when the search is emptied
    this.element.addEventListener("mousedown", this._onSearchFieldManualFocus.bind(this), false); // when the search field is manually selected
    this.element.addEventListener("keydown", this._onKeyDown.bind(this), true);
}

WebInspector.SearchController.prototype = {
    updateSearchMatchesCount: function(matches, panel)
    {
        if (!panel)
            panel = WebInspector.currentPanel;

        panel.currentSearchMatches = matches;

        if (panel === WebInspector.currentPanel)
            this._updateSearchMatchesCount(WebInspector.currentPanel.currentQuery && matches);
    },

    updateSearchLabel: function()
    {
        var panelName = WebInspector.currentPanel && WebInspector.currentPanel.toolbarItemLabel;
        if (!panelName)
            return;
        var newLabel = WebInspector.UIString("Search %s", panelName);
        if (WebInspector.attached)
            this.element.setAttribute("placeholder", newLabel);
        else {
            this.element.removeAttribute("placeholder");
            this._toolbarLabelElement.textContent = newLabel;
        }
    },

    cancelSearch: function()
    {
        this.element.value = "";
        this._performSearch("");
    },

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
                    this.focusSearchField();
                    event.handled = true;
                }
                break;


            case "F3":
                if (!isMac) {
                    this.focusSearchField();
                    event.handled = true;
                }
                break;

            case "U+0047": // G key
                var currentPanel = WebInspector.currentPanel;

                if (isMac && event.metaKey && !event.ctrlKey && !event.altKey) {
                    if (event.shiftKey) {
                        if (currentPanel.jumpToPreviousSearchResult)
                            currentPanel.jumpToPreviousSearchResult();
                    } else if (currentPanel.jumpToNextSearchResult)
                        currentPanel.jumpToNextSearchResult();
                    event.handled = true;
                }
                break;
        }
    },

    activePanelChanged: function()
    {
        this.updateSearchLabel();

        if (!this._currentQuery)
            return;
         
        panel = WebInspector.currentPanel;
        if (panel.performSearch) {
            function performPanelSearch()
            {
                this._updateSearchMatchesCount();

                panel.currentQuery = this._currentQuery;
                panel.performSearch(this._currentQuery);
            }

            // Perform the search on a timeout so the panel switches fast.
            setTimeout(performPanelSearch.bind(this), 0);
        } else {
            // Update to show Not found for panels that can't be searched.
            this._updateSearchMatchesCount();
        }
    },

    _updateSearchMatchesCount: function(matches)
    {
        if (matches == null) {
            this._matchesElement.addStyleClass("hidden");
            return;
        }

        if (matches) {
            if (matches === 1)
                var matchesString = WebInspector.UIString("1 match");
            else
                var matchesString = WebInspector.UIString("%d matches", matches);
        } else
            var matchesString = WebInspector.UIString("Not Found");

        this._matchesElement.removeStyleClass("hidden");
        this._matchesElement.textContent = matchesString;
        WebInspector.toolbar.resize();
    },

    focusSearchField: function()
    {
        this.element.focus();
        this.element.select();
    },

    _onSearchFieldManualFocus: function(event)
    {
        WebInspector.currentFocusElement = event.target;
    },

    _onKeyDown: function(event)
    {
        // Escape Key will clear the field and clear the search results
        if (event.keyCode === WebInspector.KeyboardShortcut.Keys.Esc.code) {
            // If focus belongs here and text is empty - nothing to do, return unhandled.
            // When search was selected manually and is currently blank, we'd like Esc stay unhandled
            // and hit console drawer handler.
            if (event.target.value === "" && this.currentFocusElement === this.previousFocusElement)
                return;
            event.preventDefault();
            event.stopPropagation();

            this.cancelSearch(event);
            WebInspector.currentFocusElement = WebInspector.previousFocusElement;
            if (WebInspector.currentFocusElement === event.target)
                WebInspector.currentFocusElement.currentFocusElement.select();
            return false;
        }

        if (!isEnterKey(event))
            return false;

        // Select all of the text so the user can easily type an entirely new query.
        event.target.select();

        // Only call performSearch if the Enter key was pressed. Otherwise the search
        // performance is poor because of searching on every key. The search field has
        // the incremental attribute set, so we still get incremental searches.
        this._onSearch(event);

        // Call preventDefault since this was the Enter key. This prevents a "search" event
        // from firing for key down. This stops performSearch from being called twice in a row.
        event.preventDefault();
    },

    _onSearch: function(event)
    {
        var forceSearch = event.keyIdentifier === "Enter";
        this._performSearch(event.target.value, forceSearch, event.shiftKey, false);
    },

    _performSearch: function(query, forceSearch, isBackwardSearch, repeatSearch)
    {
        var isShortSearch = (query.length < 3);

        // Clear a leftover short search flag due to a non-conflicting forced search.
        if (isShortSearch && this._shortSearchWasForcedByKeyEvent && this._currentQuery !== query)
            delete this._shortSearchWasForcedByKeyEvent;

        // Indicate this was a forced search on a short query.
        if (isShortSearch && forceSearch)
            this._shortSearchWasForcedByKeyEvent = true;

        if (!query || !query.length || (!forceSearch && isShortSearch)) {
            // Prevent clobbering a short search forced by the user.
            if (this._shortSearchWasForcedByKeyEvent) {
                delete this._shortSearchWasForcedByKeyEvent;
                return;
            }

            delete this._currentQuery;

            for (var panelName in WebInspector.panels) {
                var panel = WebInspector.panels[panelName];
                var hadCurrentQuery = !!panel.currentQuery;
                delete panel.currentQuery;
                if (hadCurrentQuery && panel.searchCanceled)
                    panel.searchCanceled();
            }

            this._updateSearchMatchesCount();

            return;
        }

        var currentPanel = WebInspector.currentPanel;
        if (!repeatSearch && query === currentPanel.currentQuery && currentPanel.currentQuery === this._currentQuery) {
            // When this is the same query and a forced search, jump to the next
            // search result for a good user experience.
            if (forceSearch) {
                if (!isBackwardSearch && currentPanel.jumpToNextSearchResult)
                    currentPanel.jumpToNextSearchResult();
                else if (isBackwardSearch && currentPanel.jumpToPreviousSearchResult)
                    currentPanel.jumpToPreviousSearchResult();
            }
            return;
        }

        this._currentQuery = query;

        this._updateSearchMatchesCount();

        if (!currentPanel.performSearch)
            return;

        currentPanel.currentQuery = query;
        currentPanel.performSearch(query);
    }
}
