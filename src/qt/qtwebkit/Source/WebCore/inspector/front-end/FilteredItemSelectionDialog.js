/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.DialogDelegate}
 * @implements {WebInspector.ViewportControl.Provider}
 * @param {WebInspector.SelectionDialogContentProvider} delegate
 */
WebInspector.FilteredItemSelectionDialog = function(delegate)
{
    WebInspector.DialogDelegate.call(this);

    var xhr = new XMLHttpRequest();
    xhr.open("GET", "filteredItemSelectionDialog.css", false);
    xhr.send(null);

    this.element = document.createElement("div");
    this.element.className = "filtered-item-list-dialog";
    this.element.addEventListener("keydown", this._onKeyDown.bind(this), false);
    var styleElement = this.element.createChild("style");
    styleElement.type = "text/css";
    styleElement.textContent = xhr.responseText;

    this._promptElement = this.element.createChild("input", "monospace");
    this._promptElement.type = "text";
    this._promptElement.setAttribute("spellcheck", "false");

    this._progressElement = this.element.createChild("div", "progress");

    this._filteredItems = [];
    this._viewportControl = new WebInspector.ViewportControl(this);
    this._itemElementsContainer = this._viewportControl.element;
    this._itemElementsContainer.addStyleClass("container");
    this._itemElementsContainer.addStyleClass("monospace");
    this._itemElementsContainer.addEventListener("click", this._onClick.bind(this), false);
    this.element.appendChild(this._itemElementsContainer);

    this._delegate = delegate;
    this._delegate.requestItems(this._itemsLoaded.bind(this));
}

WebInspector.FilteredItemSelectionDialog.prototype = {
    /**
     * @param {Element} element
     * @param {Element} relativeToElement
     */
    position: function(element, relativeToElement)
    {
        const minWidth = 500;
        const minHeight = 204;
        var width = Math.max(relativeToElement.offsetWidth * 2 / 3, minWidth);
        var height = Math.max(relativeToElement.offsetHeight * 2 / 3, minHeight);

        this.element.style.width = width + "px";
        this.element.style.height = height + "px";

        const shadowPadding = 20; // shadow + padding
        element.positionAt(
            relativeToElement.totalOffsetLeft() + Math.max((relativeToElement.offsetWidth - width - 2 * shadowPadding) / 2, shadowPadding),
            relativeToElement.totalOffsetTop() + Math.max((relativeToElement.offsetHeight - height - 2 * shadowPadding) / 2, shadowPadding));
    },

    focus: function()
    {
        WebInspector.setCurrentFocusElement(this._promptElement);
        if (this._filteredItems.length && this._viewportControl.lastVisibleIndex() === -1)
            this._viewportControl.refresh();
    },

    willHide: function()
    {
        if (this._isHiding)
            return;
        this._isHiding = true;
        this._delegate.dispose();
        if (this._filterTimer)
            clearTimeout(this._filterTimer);
    },

    renderAsTwoRows: function()
    {
        this._renderAsTwoRows = true;
    },

    onEnter: function()
    {
        if (!this._delegate.itemsCount())
            return;
        this._delegate.selectItem(this._filteredItems[this._selectedIndexInFiltered], this._promptElement.value.trim());
    },

    /**
     * @param {number} loadedCount
     * @param {number} totalCount
     */
    _itemsLoaded: function(loadedCount, totalCount)
    {
        this._loadedCount = loadedCount;
        this._totalCount = totalCount;

        if (this._loadTimeout)
            return;
        this._loadTimeout = setTimeout(this._updateAfterItemsLoaded.bind(this), 100);
    },

    _updateAfterItemsLoaded: function()
    {
        delete this._loadTimeout;
        this._filterItems();
        if (this._loadedCount === this._totalCount)
            this._progressElement.style.backgroundImage = "";
        else {
            const color = "rgb(66, 129, 235)";
            const percent = ((this._loadedCount / this._totalCount) * 100) + "%";
            this._progressElement.style.backgroundImage = "-webkit-linear-gradient(left, " + color + ", " + color + " " + percent + ",  transparent " + percent + ")";
        }
    },

    /**
     * @param {number} index
     * @return {Element}
     */
    _createItemElement: function(index)
    {
        var itemElement = document.createElement("div");
        itemElement.className = "filtered-item-list-dialog-item " + (this._renderAsTwoRows ? "two-rows" : "one-row");
        itemElement._titleElement = itemElement.createChild("span");
        itemElement._titleElement.textContent = this._delegate.itemTitleAt(index);
        itemElement._titleSuffixElement = itemElement.createChild("span");
        itemElement._titleSuffixElement.textContent = this._delegate.itemSuffixAt(index);
        itemElement._subtitleElement = itemElement.createChild("div", "filtered-item-list-dialog-subtitle");
        itemElement._subtitleElement.textContent = this._delegate.itemSubtitleAt(index) || "\u200B";
        itemElement._index = index;

        var key = this._delegate.itemKeyAt(index);
        var ranges = [];
        var match;
        if (this._query) {
            var regex = this._createSearchRegex(this._query, true);
            while ((match = regex.exec(key)) !== null && match[0])
                ranges.push({ offset: match.index, length: regex.lastIndex - match.index });
            if (ranges.length)
                WebInspector.highlightRangesWithStyleClass(itemElement, ranges, "highlight");
        }
        if (index === this._filteredItems[this._selectedIndexInFiltered])
            itemElement.addStyleClass("selected");

        return itemElement;
    },

    /**
     * @param {?string} query
     * @param {boolean=} isGlobal
     */
    _createSearchRegex: function(query, isGlobal)
    {
        const toEscape = String.regexSpecialCharacters();
        var regexString = "";
        for (var i = 0; i < query.length; ++i) {
            var c = query.charAt(i);
            if (toEscape.indexOf(c) !== -1)
                c = "\\" + c;
            if (i)
                regexString += "[^" + c + "]*";
            regexString += c;
        }
        return new RegExp(regexString, "i" + (isGlobal ? "g" : ""));
    },

    /**
     * @param {string} query
     * @param {boolean} ignoreCase
     * @param {boolean} camelCase
     * @return {RegExp}
     */
    _createScoringRegex: function(query, ignoreCase, camelCase)
    {
        if (!camelCase || (camelCase && ignoreCase))
            query = query.toUpperCase();
        var regexString = "";
        for (var i = 0; i < query.length; ++i) {
            var c = query.charAt(i);
            if (c < "A" || c > "Z")
               continue;
            if (regexString)
               regexString += camelCase ? "[^A-Z]*" : "[^-_ .]*[-_ .]";
            regexString += c;
        }
        if (!camelCase)
            regexString = "(?:^|[-_ .])" + regexString;
        return new RegExp(regexString, camelCase ? "" : "i");
    },

    /**
     * @param {string} query
     */
    setQuery: function(query)
    {
        this._promptElement.value = query;
        this._scheduleFilter();
    },

    _filterItems: function()
    {
        delete this._filterTimer;

        var query = this._delegate.rewriteQuery(this._promptElement.value.trim());
        this._query = query;

        var ignoreCase = (query === query.toLowerCase());

        var filterRegex = query ? this._createSearchRegex(query) : null;
        var camelCaseScoringRegex = query ? this._createScoringRegex(query, ignoreCase, true) : null;
        var underscoreScoringRegex = query ? this._createScoringRegex(query, ignoreCase, false) : null;

        var oldSelectedAbsoluteIndex = this._selectedIndexInFiltered ? this._filteredItems[this._selectedIndexInFiltered] : null;
        this._filteredItems = [];
        this._selectedIndexInFiltered = 0;

        var cachedKeys = new Array(this._delegate.itemsCount());
        var scores = query ? new Array(this._delegate.itemsCount()) : null;

        for (var i = 0; i < this._delegate.itemsCount(); ++i) {
            var key = this._delegate.itemKeyAt(i);
            if (filterRegex && !filterRegex.test(key))
                continue;
            cachedKeys[i] = key;
            this._filteredItems.push(i);

            if (!filterRegex)
                continue;

            var score = 0;
            if (underscoreScoringRegex.test(key))
                score += 10;
            if (camelCaseScoringRegex.test(key))
                score += ignoreCase ? 10 : 20;
            for (var j = 0; j < key.length && j < query.length; ++j) {
                if (key[j] === query[j])
                    score++;
                if (key[j].toUpperCase() === query[j].toUpperCase())
                    score++;
                else
                    break;
            }
            scores[i] = score;
        }

        function compareFunction(index1, index2)
        {
            if (scores) {
                var score1 = scores[index1];
                var score2 = scores[index2];
                if (score1 > score2)
                    return -1;
                if (score1 < score2)
                    return 1;
            }
            var key1 = cachedKeys[index1];
            var key2 = cachedKeys[index2];
            return key1.compareTo(key2) || (index2 - index1);
        }

        const numberOfItemsToSort = 100;
        if (this._filteredItems.length > numberOfItemsToSort)
            this._filteredItems.sortRange(compareFunction.bind(this), 0, this._filteredItems.length - 1, numberOfItemsToSort);
        else
            this._filteredItems.sort(compareFunction.bind(this));

        for (var i = 0; i < this._filteredItems.length; ++i) {
            if (this._filteredItems[i] === oldSelectedAbsoluteIndex) {
                this._selectedIndexInFiltered = i;
                break;
            }
        }
        this._viewportControl.refresh();
        this._updateSelection(this._selectedIndexInFiltered, false);
    },

    _onKeyDown: function(event)
    {
        var newSelectedIndex = this._selectedIndexInFiltered;

        switch (event.keyCode) {
        case WebInspector.KeyboardShortcut.Keys.Down.code:
            if (++newSelectedIndex >= this._filteredItems.length)
                newSelectedIndex = this._filteredItems.length - 1;
            this._updateSelection(newSelectedIndex, true);
            event.consume(true);
            break;
        case WebInspector.KeyboardShortcut.Keys.Up.code:
            if (--newSelectedIndex < 0)
                newSelectedIndex = 0;
            this._updateSelection(newSelectedIndex, false);
            event.consume(true);
            break;
        case WebInspector.KeyboardShortcut.Keys.PageDown.code:
            newSelectedIndex = Math.min(newSelectedIndex + this._viewportControl.rowsPerViewport(), this._filteredItems.length - 1);
            this._updateSelection(newSelectedIndex, true);
            event.consume(true);
            break;
        case WebInspector.KeyboardShortcut.Keys.PageUp.code:
            newSelectedIndex = Math.max(newSelectedIndex - this._viewportControl.rowsPerViewport(), 0);
            this._updateSelection(newSelectedIndex, false);
            event.consume(true);
            break;
        default:
            if (event.keyIdentifier !== "Shift" && event.keyIdentifier !== "Ctrl" && event.keyIdentifier !== "Meta" && event.keyIdentifier !== "Left" && event.keyIdentifier !== "Right")
                this._scheduleFilter();
        }
    },

    _scheduleFilter: function()
    {
        if (this._filterTimer)
            return;
        this._filterTimer = setTimeout(this._filterItems.bind(this), 0);
    },

    /**
     * @param {number} index  
     * @param {boolean} makeLast
     */
    _updateSelection: function(index, makeLast)
    { 
        var element = this._viewportControl.renderedElementAt(this._selectedIndexInFiltered);
        if (element)
            element.removeStyleClass("selected");
        this._viewportControl.scrollItemIntoView(index, makeLast);
        this._selectedIndexInFiltered = index;
        element = this._viewportControl.renderedElementAt(index);
        if (element)
            element.addStyleClass("selected");
    },

    _onClick: function(event)
    {
        var itemElement = event.target.enclosingNodeOrSelfWithClass("filtered-item-list-dialog-item");
        if (!itemElement)
            return;
        this._delegate.selectItem(itemElement._index, this._promptElement.value.trim());
        WebInspector.Dialog.hide();
    },

    /**
     * @return {number}
     */
    itemCount: function()
    {
        return this._filteredItems.length;
    },

    /**
     * @param {number} index
     * @return {Element}
     */
    itemElement: function(index)
    {
        var delegateIndex = this._filteredItems[index];
        var element = this._createItemElement(delegateIndex);
        element._filteredIndex = index;
        return element;
    },

    __proto__: WebInspector.DialogDelegate.prototype
}

/**
 * @interface
 */
WebInspector.SelectionDialogContentProvider = function()
{
}

WebInspector.SelectionDialogContentProvider.prototype = {
    /**
     * @param {number} itemIndex
     * @return {string}
     */
    itemTitleAt: function(itemIndex) { },

    /*
     * @param {number} itemIndex
     * @return {string}
     */
    itemSuffixAt: function(itemIndex) { },

    /*
     * @param {number} itemIndex
     * @return {string}
     */
    itemSubtitleAt: function(itemIndex) { },

    /**
     * @param {number} itemIndex
     * @return {string}
     */
    itemKeyAt: function(itemIndex) { },

    /**
     * @return {number}
     */
    itemsCount: function() { },

    /**
     * @param {function(number, number)} callback
     */
    requestItems: function(callback) { },

    /**
     * @param {number} itemIndex
     * @param {string} promptValue
     */
    selectItem: function(itemIndex, promptValue) { },

    /**
     * @param {string} query
     * @return {string}
     */
    rewriteQuery: function(query) { },

    dispose: function() { }
}

/**
 * @constructor
 * @implements {WebInspector.SelectionDialogContentProvider}
 * @param {WebInspector.View} view
 * @param {WebInspector.ContentProvider} contentProvider
 */
WebInspector.JavaScriptOutlineDialog = function(view, contentProvider)
{
    WebInspector.SelectionDialogContentProvider.call(this);

    this._functionItems = [];
    this._view = view;
    this._contentProvider = contentProvider;
}

/**
 * @param {WebInspector.View} view
 * @param {WebInspector.ContentProvider} contentProvider
 */
WebInspector.JavaScriptOutlineDialog.show = function(view, contentProvider)
{
    if (WebInspector.Dialog.currentInstance())
        return null;
    var delegate = new WebInspector.JavaScriptOutlineDialog(view, contentProvider);
    var filteredItemSelectionDialog = new WebInspector.FilteredItemSelectionDialog(delegate);
    WebInspector.Dialog.show(view.element, filteredItemSelectionDialog);
}

WebInspector.JavaScriptOutlineDialog.prototype = {
    /**
     * @param {number} itemIndex
     * @return {string}
     */
    itemTitleAt: function(itemIndex)
    {
        var functionItem = this._functionItems[itemIndex];
        return functionItem.name + (functionItem.arguments ? functionItem.arguments : "");
    },

    /*
     * @param {number} itemIndex
     * @return {string}
     */
    itemSuffixAt: function(itemIndex)
    {
        return "";
    },

    /*
     * @param {number} itemIndex
     * @return {string}
     */
    itemSubtitleAt: function(itemIndex)
    {
        return ":" + (this._functionItems[itemIndex].line + 1);
    },

    /**
     * @param {number} itemIndex
     * @return {string}
     */
    itemKeyAt: function(itemIndex)
    {
        return this._functionItems[itemIndex].name;
    },

    /**
     * @return {number}
     */
    itemsCount: function()
    {
        return this._functionItems.length;
    },

    /**
     * @param {function(number, number)} callback
     */
    requestItems: function(callback)
    {
        /**
         * @param {?string} content
         * @param {boolean} contentEncoded
         * @param {string} mimeType
         */
        function contentCallback(content, contentEncoded, mimeType)
        {
            if (this._outlineWorker)
                this._outlineWorker.terminate();
            this._outlineWorker = new Worker("ScriptFormatterWorker.js");
            this._outlineWorker.onmessage = this._didBuildOutlineChunk.bind(this, callback);
            const method = "outline";
            this._outlineWorker.postMessage({ method: method, params: { content: content } });
        }
        this._contentProvider.requestContent(contentCallback.bind(this));
    },

    _didBuildOutlineChunk: function(callback, event)
    {
        var data = event.data;
        var chunk = data["chunk"];
        for (var i = 0; i < chunk.length; ++i)
            this._functionItems.push(chunk[i]);
        callback(data.index, data.total);

        if (data.total === data.index && this._outlineWorker) {
            this._outlineWorker.terminate();
            delete this._outlineWorker;
        }
    },

    /**
     * @param {number} itemIndex
     * @param {string} promptValue
     */
    selectItem: function(itemIndex, promptValue)
    {
        var lineNumber = this._functionItems[itemIndex].line;
        if (!isNaN(lineNumber) && lineNumber >= 0)
            this._view.highlightLine(lineNumber);
        this._view.focus();
    },

    /**
     * @param {string} query
     * @return {string}
     */
    rewriteQuery: function(query)
    {
        return query;
    },

    dispose: function()
    {
    }
}

/**
 * @constructor
 * @implements {WebInspector.SelectionDialogContentProvider}
 */
WebInspector.SelectUISourceCodeDialog = function()
{
    var projects = WebInspector.workspace.projects().filter(this.filterProject.bind(this));
    this._uiSourceCodes = [];
    for (var i = 0; i < projects.length; ++i)
        this._uiSourceCodes = this._uiSourceCodes.concat(projects[i].uiSourceCodes().filter(this.filterUISourceCode.bind(this)));
    WebInspector.workspace.addEventListener(WebInspector.UISourceCodeProvider.Events.UISourceCodeAdded, this._uiSourceCodeAdded, this);
}

WebInspector.SelectUISourceCodeDialog.prototype = {
    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     */
    uiSourceCodeSelected: function(uiSourceCode, lineNumber)
    {
        // Overridden by subclasses
    },

    /**
     * @param {WebInspector.Project} project
     */
    filterProject: function(project)
    {
        return true;
        // Overridden by subclasses
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    filterUISourceCode: function(uiSourceCode)
    {
        return uiSourceCode.name();
    },

    /**
     * @param {number} itemIndex
     * @return {string}
     */
    itemTitleAt: function(itemIndex)
    {
        return this._uiSourceCodes[itemIndex].name().trimEnd(100);
    },

    /*
     * @param {number} itemIndex
     * @return {string}
     */
    itemSuffixAt: function(itemIndex)
    {
        return this._queryLineNumber || "";
    },

    /*
     * @param {number} itemIndex
     * @return {string}
     */
    itemSubtitleAt: function(itemIndex)
    {
        var uiSourceCode = this._uiSourceCodes[itemIndex]
        var projectName = uiSourceCode.project().displayName();
        var path = uiSourceCode.path().slice();
        path.pop();
        path.unshift(projectName);
        return path.join("/");
    },

    /**
     * @param {number} itemIndex
     * @return {string}
     */
    itemKeyAt: function(itemIndex)
    {
        return this._uiSourceCodes[itemIndex].name();
    },

    /**
     * @return {number}
     */
    itemsCount: function()
    {
        return this._uiSourceCodes.length;
    },

    /**
     * @param {function(number, number)} callback
     */
    requestItems: function(callback)
    {
        this._itemsLoaded = callback;
        this._itemsLoaded(1, 1);
    },

    /**
     * @param {number} itemIndex
     * @param {string} promptValue
     */
    selectItem: function(itemIndex, promptValue)
    {
        var lineNumberMatch = promptValue.match(/[^:]+\:([\d]*)$/);
        var lineNumber = lineNumberMatch ? Math.max(parseInt(lineNumberMatch[1], 10) - 1, 0) : 0;
        this.uiSourceCodeSelected(this._uiSourceCodes[itemIndex], lineNumber);
    },

    /**
     * @param {string} query
     * @return {string}
     */
    rewriteQuery: function(query)
    {
        if (!query)
            return query;
        query = query.trim();
        var lineNumberMatch = query.match(/([^:]+)(\:[\d]*)$/);
        this._queryLineNumber = lineNumberMatch ? lineNumberMatch[2] : "";
        return lineNumberMatch ? lineNumberMatch[1] : query;
    },

    /**
     * @param {WebInspector.Event} event
     */
    _uiSourceCodeAdded: function(event)
    {
        var uiSourceCode = /** @type {WebInspector.UISourceCode} */ (event.data);
        if (!this.filterUISourceCode(uiSourceCode))
            return;
        this._uiSourceCodes.push(uiSourceCode)
        this._itemsLoaded(1, 1);
    },

    dispose: function()
    {
        WebInspector.workspace.removeEventListener(WebInspector.UISourceCodeProvider.Events.UISourceCodeAdded, this._uiSourceCodeAdded, this);
    }
}

/**
 * @constructor
 * @extends {WebInspector.SelectUISourceCodeDialog}
 * @param {WebInspector.ScriptsPanel} panel
 */
WebInspector.OpenResourceDialog = function(panel)
{
    WebInspector.SelectUISourceCodeDialog.call(this);
    this._panel = panel;
}

WebInspector.OpenResourceDialog.prototype = {

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     */
    uiSourceCodeSelected: function(uiSourceCode, lineNumber)
    {
        this._panel.showUISourceCode(uiSourceCode, lineNumber);
    },

    /**
     * @param {WebInspector.Project} project
     */
    filterProject: function(project)
    {
        return !project.isServiceProject();
    },

    __proto__: WebInspector.SelectUISourceCodeDialog.prototype
}

/**
 * @param {WebInspector.ScriptsPanel} panel
 * @param {Element} relativeToElement
 */
WebInspector.OpenResourceDialog.show = function(panel, relativeToElement)
{
    if (WebInspector.Dialog.currentInstance())
        return;

    var filteredItemSelectionDialog = new WebInspector.FilteredItemSelectionDialog(new WebInspector.OpenResourceDialog(panel));
    filteredItemSelectionDialog.renderAsTwoRows();
    WebInspector.Dialog.show(relativeToElement, filteredItemSelectionDialog);
}

/**
 * @constructor
 * @extends {WebInspector.SelectUISourceCodeDialog}
 * @param {string} type
 * @param {function(WebInspector.UISourceCode)} callback
 */
WebInspector.SelectUISourceCodeForProjectTypeDialog = function(type, callback)
{
    this._type = type;
    WebInspector.SelectUISourceCodeDialog.call(this);
    this._callback = callback;
}

WebInspector.SelectUISourceCodeForProjectTypeDialog.prototype = {
    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     */
    uiSourceCodeSelected: function(uiSourceCode, lineNumber)
    {
        this._callback(uiSourceCode);
    },

    /**
     * @param {WebInspector.Project} project
     */
    filterProject: function(project)
    {
        return project.type() === this._type;
    },

    __proto__: WebInspector.SelectUISourceCodeDialog.prototype
}

/**
 * @param {string} type
 * @param {function(WebInspector.UISourceCode)} callback
 * @param {Element} relativeToElement
 */
WebInspector.SelectUISourceCodeForProjectTypeDialog.show = function(name, type, callback, relativeToElement)
{
    if (WebInspector.Dialog.currentInstance())
        return;

    var filteredItemSelectionDialog = new WebInspector.FilteredItemSelectionDialog(new WebInspector.SelectUISourceCodeForProjectTypeDialog(type, callback));
    filteredItemSelectionDialog.setQuery(name);
    filteredItemSelectionDialog.renderAsTwoRows();
    WebInspector.Dialog.show(relativeToElement, filteredItemSelectionDialog);
}
