/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.View}
 */
WebInspector.DatabaseQueryView = function(database)
{
    WebInspector.View.call(this);

    this.database = database;

    this.element.addStyleClass("storage-view");
    this.element.addStyleClass("query");
    this.element.addStyleClass("monospace");
    this.element.addEventListener("selectstart", this._selectStart.bind(this), false);

    this._promptElement = document.createElement("div");
    this._promptElement.className = "database-query-prompt";
    this._promptElement.appendChild(document.createElement("br"));
    this._promptElement.addEventListener("keydown", this._promptKeyDown.bind(this), true);
    this.element.appendChild(this._promptElement);

    this.prompt = new WebInspector.TextPromptWithHistory(this.completions.bind(this), " ");
    this.prompt.attach(this._promptElement);

    this.element.addEventListener("click", this._messagesClicked.bind(this), true);
}

WebInspector.DatabaseQueryView.Events = {
    SchemaUpdated: "SchemaUpdated"
}

WebInspector.DatabaseQueryView.prototype = {
    _messagesClicked: function()
    {
        if (!this.prompt.isCaretInsidePrompt() && window.getSelection().isCollapsed)
            this.prompt.moveCaretToEndOfPrompt();
    },
    
    /**
     * @param {Element} proxyElement
     * @param {Range} wordRange
     * @param {boolean} force
     * @param {function(!Array.<string>, number=)} completionsReadyCallback
     */
    completions: function(proxyElement, wordRange, force, completionsReadyCallback)
    {
        var prefix = wordRange.toString().toLowerCase();
        if (!prefix.length && !force)
            return;

        var results = [];

        function accumulateMatches(textArray)
        {
            for (var i = 0; i < textArray.length; ++i) {
                var text = textArray[i].toLowerCase();
                if (text.length < prefix.length)
                    continue;
                if (!text.startsWith(prefix))
                    continue;
                results.push(textArray[i]);
            }
        }

        function tableNamesCallback(tableNames)
        {
            accumulateMatches(tableNames.map(function(name) { return name + " " }));
            accumulateMatches(["SELECT ", "FROM ", "WHERE ", "LIMIT ", "DELETE FROM ", "CREATE ", "DROP ", "TABLE ", "INDEX ", "UPDATE ", "INSERT INTO ", "VALUES ("]);

            completionsReadyCallback(results);
        }
        this.database.getTableNames(tableNamesCallback);
    },

    _selectStart: function(event)
    {
        if (this._selectionTimeout)
            clearTimeout(this._selectionTimeout);

        this.prompt.clearAutoComplete();

        function moveBackIfOutside()
        {
            delete this._selectionTimeout;
            if (!this.prompt.isCaretInsidePrompt() && window.getSelection().isCollapsed)
                this.prompt.moveCaretToEndOfPrompt();
            this.prompt.autoCompleteSoon();
        }

        this._selectionTimeout = setTimeout(moveBackIfOutside.bind(this), 100);
    },

    _promptKeyDown: function(event)
    {
        if (isEnterKey(event)) {
            this._enterKeyPressed(event);
            return;
        }
    },

    _enterKeyPressed: function(event)
    {
        event.consume(true);

        this.prompt.clearAutoComplete(true);

        var query = this.prompt.text;
        if (!query.length)
            return;

        this.prompt.pushHistoryItem(query);
        this.prompt.text = "";

        this.database.executeSql(query, this._queryFinished.bind(this, query), this._queryError.bind(this, query));
    },

    _queryFinished: function(query, columnNames, values)
    {
        var dataGrid = WebInspector.DataGrid.createSortableDataGrid(columnNames, values);
        var trimmedQuery = query.trim();

        if (dataGrid) {
            dataGrid.renderInline();
            this._appendViewQueryResult(trimmedQuery, dataGrid);
            dataGrid.autoSizeColumns(5);
        }

        if (trimmedQuery.match(/^create /i) || trimmedQuery.match(/^drop table /i))
            this.dispatchEventToListeners(WebInspector.DatabaseQueryView.Events.SchemaUpdated, this.database);
    },

    _queryError: function(query, errorMessage)
    {
        this._appendErrorQueryResult(query, errorMessage);
    },

    /**
     * @param {string} query
     * @param {WebInspector.View} view
     */
    _appendViewQueryResult: function(query, view)
    {
        var resultElement = this._appendQueryResult(query);
        view.show(resultElement);

        this._promptElement.scrollIntoView(false);
    },

    /**
     * @param {string} query
     * @param {string} errorText
     */
    _appendErrorQueryResult: function(query, errorText)
    {
        var resultElement = this._appendQueryResult(query);
        resultElement.addStyleClass("error")
        resultElement.textContent = errorText;

        this._promptElement.scrollIntoView(false);
    },

    _appendQueryResult: function(query)
    {
        var element = document.createElement("div");
        element.className = "database-user-query";
        this.element.insertBefore(element, this.prompt.proxyElement);

        var commandTextElement = document.createElement("span");
        commandTextElement.className = "database-query-text";
        commandTextElement.textContent = query;
        element.appendChild(commandTextElement);

        var resultElement = document.createElement("div");
        resultElement.className = "database-query-result";
        element.appendChild(resultElement);
        return resultElement;
    },

    __proto__: WebInspector.View.prototype
}
