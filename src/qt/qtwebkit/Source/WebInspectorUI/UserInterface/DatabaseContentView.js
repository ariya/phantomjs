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

WebInspector.DatabaseContentView = function(representedObject)
{
    WebInspector.ContentView.call(this, representedObject);

    this.database = representedObject;

    this.element.classList.add("storage-view");
    this.element.classList.add("query");
    this.element.classList.add("monospace");

    this._promptElement = document.createElement("div");
    this._promptElement.className = "database-query-prompt";
    this.element.appendChild(this._promptElement);

    this.prompt = new WebInspector.ConsolePrompt(this, "text/x-sql");
    this._promptElement.appendChild(this.prompt.element);

    this.element.addEventListener("click", this._messagesClicked.bind(this), true);
}

WebInspector.DatabaseContentView.Event = {
    SchemaUpdated: "SchemaUpdated"
}

WebInspector.DatabaseContentView.prototype = {
    constructor: WebInspector.DatabaseContentView,

    shown: function()
    {
        this.prompt.shown();
    },

    updateLayout: function()
    {
        this.prompt.updateLayout();

        var results = this.element.querySelectorAll(".database-query-result");
        for (var i = 0; i < results.length; ++i) {
            var resultElement = results[i];
            if (resultElement.dataGrid)
                resultElement.dataGrid.updateLayout();
        }
    },

    _messagesClicked: function()
    {
        this.prompt.focus();
    },
    
    consolePromptCompletionsNeeded: function(prompt, defaultCompletions, base, prefix, suffix)
    {
        var results = [];

        prefix = prefix.toLowerCase();

        function accumulateMatches(textArray)
        {
            for (var i = 0; i < textArray.length; ++i) {
                var lowerCaseText = textArray[i].toLowerCase();
                if (lowerCaseText.startsWith(prefix))
                    results.push(textArray[i]);
            }
        }

        function tableNamesCallback(tableNames)
        {
            accumulateMatches(tableNames);
            accumulateMatches(["SELECT", "FROM", "WHERE", "LIMIT", "DELETE FROM", "CREATE", "DROP", "TABLE", "INDEX", "UPDATE", "INSERT INTO", "VALUES"]);

            this.prompt.updateCompletions(results, " ");
        }

        this.database.getTableNames(tableNamesCallback.bind(this));
    },

    consolePromptTextCommitted: function(prompt, query)
    {
        this.database.executeSQL(query, this._queryFinished.bind(this, query), this._queryError.bind(this, query));
    },

    _queryFinished: function(query, columnNames, values)
    {
        var dataGrid = WebInspector.DataGrid.createSortableDataGrid(columnNames, values);
        var trimmedQuery = query.trim();

        if (dataGrid) {
            dataGrid.element.classList.add("inline");
            this._appendViewQueryResult(trimmedQuery, dataGrid);
            dataGrid.autoSizeColumns(5);
        }

        if (trimmedQuery.match(/^create /i) || trimmedQuery.match(/^drop table /i))
            this.dispatchEventToListeners(WebInspector.DatabaseContentView.Event.SchemaUpdated, this.database);
    },

    _queryError: function(query, error)
    {
        if (error.message)
            var message = error.message;
        else if (error.code == 2)
            var message = WebInspector.UIString("Database no longer has expected version.");
        else
            var message = WebInspector.UIString("An unexpected error %s occurred.").format(error.code);

        this._appendErrorQueryResult(query, message);
    },

    /**
     * @param {string} query
     * @param {WebInspector.View} view
     */
    _appendViewQueryResult: function(query, view)
    {
        var resultElement = this._appendQueryResult(query);

        // Add our DataGrid with the results to the database query result div.
        resultElement.dataGrid = view;
        resultElement.appendChild(view.element);

        this._promptElement.scrollIntoView(false);
    },

    /**
     * @param {string} query
     * @param {string} errorText
     */
    _appendErrorQueryResult: function(query, errorText)
    {
        var resultElement = this._appendQueryResult(query);
        resultElement.classList.add("error")
        resultElement.textContent = errorText;

        this._promptElement.scrollIntoView(false);
    },

    _appendQueryResult: function(query)
    {
        var element = document.createElement("div");
        element.className = "database-user-query";
        this.element.insertBefore(element, this._promptElement);

        var commandTextElement = document.createElement("span");
        commandTextElement.className = "database-query-text";
        commandTextElement.textContent = query;
        element.appendChild(commandTextElement);

        var resultElement = document.createElement("div");
        resultElement.className = "database-query-result";
        element.appendChild(resultElement);
        return resultElement;
    }
}

WebInspector.DatabaseContentView.prototype.__proto__ = WebInspector.ContentView.prototype;
