/*
 * Copyright (C) 2009 Joseph Pecoraro
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
 * @extends {WebInspector.Panel}
 */
WebInspector.ConsolePanel = function()
{
    WebInspector.Panel.call(this, "console");

    WebInspector.consoleView.addEventListener(WebInspector.ConsoleView.Events.EntryAdded, this._consoleMessageAdded, this);
    WebInspector.consoleView.addEventListener(WebInspector.ConsoleView.Events.ConsoleCleared, this._consoleCleared, this);
    this._view = WebInspector.consoleView;
}

WebInspector.ConsolePanel.prototype = {
    statusBarItems: function()
    {
        return this._view.statusBarItems();
    },

    wasShown: function()
    {
        WebInspector.Panel.prototype.wasShown.call(this);
        if (WebInspector.drawer.visible) {
            WebInspector.drawer.hide(WebInspector.Drawer.AnimationType.Immediately);
            this._drawerWasVisible = true;
        }
        this._view.show(this.element);
    },

    willHide: function()
    {
        if (this._drawerWasVisible) {
            WebInspector.drawer.show(this._view, WebInspector.Drawer.AnimationType.Immediately);
            delete this._drawerWasVisible;
        }
        WebInspector.Panel.prototype.willHide.call(this);
    },

    searchCanceled: function()
    {
        this._clearCurrentSearchResultHighlight();
        delete this._searchResults;
        delete this._searchRegex;
    },

    /**
     * @param {string} query
     */
    performSearch: function(query)
    {
        WebInspector.searchController.updateSearchMatchesCount(0, this);
        this.searchCanceled();
        this._searchRegex = createPlainTextSearchRegex(query, "gi");

        this._searchResults = [];
        var messages = WebInspector.console.messages;
        for (var i = 0; i < messages.length; i++) {
            if (messages[i].matchesRegex(this._searchRegex)) {
                this._searchResults.push(messages[i]);
                this._searchRegex.lastIndex = 0;
            }
        }
        WebInspector.searchController.updateSearchMatchesCount(this._searchResults.length, this);
        this._currentSearchResultIndex = -1;
        if (this._searchResults.length)
            this._jumpToSearchResult(0);
    },

    jumpToNextSearchResult: function()
    {
        if (!this._searchResults || !this._searchResults.length)
            return;
        this._jumpToSearchResult((this._currentSearchResultIndex + 1) % this._searchResults.length);
    },

    jumpToPreviousSearchResult: function()
    {
        if (!this._searchResults || !this._searchResults.length)
            return;
        var index = this._currentSearchResultIndex - 1;
        if (index === -1)
            index = this._searchResults.length - 1;
        this._jumpToSearchResult(index);
    },

    _clearCurrentSearchResultHighlight: function()
    {
        if (!this._searchResults)
            return;
        var highlightedMessage = this._searchResults[this._currentSearchResultIndex];
        if (highlightedMessage)
            highlightedMessage.clearHighlight();
        this._currentSearchResultIndex = -1;
    },

    _jumpToSearchResult: function(index)
    {
        this._clearCurrentSearchResultHighlight();
        this._currentSearchResultIndex = index;
        WebInspector.searchController.updateCurrentMatchIndex(this._currentSearchResultIndex, this);
        this._searchResults[index].highlightSearchResults(this._searchRegex);
    },

    _consoleMessageAdded: function(event)
    {
        if (!this._searchRegex || !this.isShowing())
            return;
        var message = event.data;
        this._searchRegex.lastIndex = 0;
        if (message.matchesRegex(this._searchRegex)) {
            this._searchResults.push(message);
            WebInspector.searchController.updateSearchMatchesCount(this._searchResults.length, this);
        }
    },

    _consoleCleared: function()
    {
        if (!this._searchResults)
            return;
        this._clearCurrentSearchResultHighlight();
        this._searchResults.length = 0;
        if (this.isShowing())
            WebInspector.searchController.updateSearchMatchesCount(0, this);
    },

    __proto__: WebInspector.Panel.prototype
}
