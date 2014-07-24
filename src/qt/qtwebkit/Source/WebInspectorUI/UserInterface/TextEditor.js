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

WebInspector.TextEditor = function(element, mimeType, delegate)
{
    WebInspector.Object.call(this);

    var text = (element ? element.textContent : "");
    this._element = element || document.createElement("div");
    this._element.classList.add(WebInspector.TextEditor.StyleClassName);
    this._element.classList.add(WebInspector.SyntaxHighlightedStyleClassName);

    this._readOnly = true;

    this._codeMirror = CodeMirror(this.element, {
        readOnly: this._readOnly,
        indentWithTabs: true,
        indentUnit: 4,
        lineNumbers: true,
        lineWrapping: true,
        matchBrackets: true,
        autoCloseBrackets: true
    });

    this._codeMirror.on("change", this._contentChanged.bind(this));
    this._codeMirror.on("gutterClick", this._gutterMouseDown.bind(this));
    this._codeMirror.getScrollerElement().addEventListener("click", this._openClickedLinks.bind(this), true);

    this._completionController = new WebInspector.CodeMirrorCompletionController(this._codeMirror, this);
    this._tokenTrackingController = new WebInspector.CodeMirrorTokenTrackingController(this._codeMirror, this);

    this._initialStringNotSet = true;

    this.mimeType = mimeType;

    this._breakpoints = {};
    this._executionLineNumber = NaN;
    this._executionColumnNumber = NaN;

    this._searchQuery = null;
    this._searchResults = [];
    this._currentSearchResultIndex = -1;

    this._formatted = false
    this._formatterSourceMap = null;

    this._delegate = delegate || null;
};

WebInspector.Object.addConstructorFunctions(WebInspector.TextEditor);

WebInspector.TextEditor.StyleClassName = "text-editor";
WebInspector.TextEditor.HighlightedStyleClassName = "highlighted";
WebInspector.TextEditor.SearchResultStyleClassName = "search-result";
WebInspector.TextEditor.HasBreakpointStyleClassName = "has-breakpoint";
WebInspector.TextEditor.BreakpointResolvedStyleClassName = "breakpoint-resolved";
WebInspector.TextEditor.BreakpointDisabledStyleClassName = "breakpoint-disabled";
WebInspector.TextEditor.MultipleBreakpointsStyleClassName = "multiple-breakpoints";
WebInspector.TextEditor.ExecutionLineStyleClassName = "execution-line";
WebInspector.TextEditor.BouncyHighlightStyleClassName = "bouncy-highlight";
WebInspector.TextEditor.NumberOfFindsPerSearchBatch = 10;
WebInspector.TextEditor.HighlightAnimationDuration = 2000;

WebInspector.TextEditor.Event = {
    ExecutionLineNumberDidChange: "text-editor-execution-line-number-did-change",
    NumberOfSearchResultsDidChange: "text-editor-number-of-search-results-did-change",
    ContentDidChange: "text-editor-content-did-change",
    FormattingDidChange: "text-editor-formatting-did-change"
};

WebInspector.TextEditor.prototype = {
    constructor: WebInspector.TextEditor,

    // Public

    get element()
    {
        return this._element;
    },

    get string()
    {
        return this._codeMirror.getValue();
    },

    set string(newString)
    {
        function update()
        {
            this._codeMirror.setValue(newString);

            if (this._initialStringNotSet) {
                this._codeMirror.clearHistory();
                this._codeMirror.markClean();
                delete this._initialStringNotSet;
            }

            // Automatically format the content.
            if (this._autoFormat) {
                console.assert(!this.formatted);
                this.formatted = true;
                delete this._autoFormat;
            }

            // Update the execution line now that we might have content for that line.
            this._updateExecutionLine();

            // Set the breakpoint styles now that we might have content for those lines.
            for (var lineNumber in this._breakpoints)
                this._setBreakpointStylesOnLine(lineNumber);

            // Try revealing the pending line now that we might have content with enough lines.
            this._revealPendingPositionIfPossible();
        }

        this._ignoreCodeMirrorContentDidChangeEvent = true;
        this._codeMirror.operation(update.bind(this));
        delete this._ignoreCodeMirrorContentDidChangeEvent;
    },

    get readOnly()
    {
        return this._codeMirror.getOption("readOnly") || false;
    },

    set readOnly(readOnly)
    {
        this._readOnly = readOnly;
        this._updateCodeMirrorReadOnly();
    },

    get formatted()
    {
        return this._formatted;
    },

    set formatted(formatted)
    {
        if (this._formatted === formatted)
            return;

        console.assert(!formatted || this.canBeFormatted());
        if (formatted && !this.canBeFormatted())
            return;

        this._ignoreCodeMirrorContentDidChangeEvent = true;
        this._prettyPrint(formatted);
        delete this._ignoreCodeMirrorContentDidChangeEvent;

        this._formatted = formatted;
        this._updateCodeMirrorReadOnly();

        this.dispatchEventToListeners(WebInspector.TextEditor.Event.FormattingDidChange);
    },

    set autoFormat(auto)
    {
        this._autoFormat = auto;
    },

    hasFormatter: function()
    {
        const supportedModes = {
            "javascript": true,
            "css-base": true,
        };

        var mode = this._codeMirror.getMode();
        return mode.name in supportedModes;
    },

    canBeFormatted: function()
    {
        // Can be overriden by subclasses.
        return this.hasFormatter();
    },

    get selectedTextRange()
    {
        var start = this._codeMirror.getCursor(true);
        var end = this._codeMirror.getCursor(false);
        return this._textRangeFromCodeMirrorPosition(start, end);
    },

    set selectedTextRange(textRange)
    {
        var position = this._codeMirrorPositionFromTextRange(textRange);
        this._codeMirror.setSelection(position.start, position.end);
    },

    get mimeType()
    {
        return this._mimeType;
    },

    set mimeType(newMIMEType)
    {
        this._mimeType = newMIMEType;
        this._codeMirror.setOption("mode", newMIMEType);
    },

    get executionLineNumber()
    {
        return this._executionLineNumber;
    },

    set executionLineNumber(lineNumber)
    {
        // Only return early if there isn't a line handle and that isn't changing.
        if (!this._executionLineHandle && isNaN(lineNumber))
            return;

        this._executionLineNumber = lineNumber;
        this._updateExecutionLine();

        // Still dispatch the event even if the number didn't change. The execution state still
        // could have changed (e.g. continuing in a loop with a breakpoint inside).
        this.dispatchEventToListeners(WebInspector.TextEditor.Event.ExecutionLineNumberDidChange);
    },

    get executionColumnNumber()
    {
        return this._executionColumnNumber;
    },

    set executionColumnNumber(columnNumber)
    {
        this._executionColumnNumber = columnNumber;
    },

    get formatterSourceMap()
    {
        return this._formatterSourceMap;
    },

    get tokenTrackingController()
    {
        return this._tokenTrackingController;
    },

    get delegate()
    {
        return this._delegate;
    },

    set delegate(newDelegate)
    {
        this._delegate = newDelegate || null;
    },

    get numberOfSearchResults()
    {
        return this._searchResults.length;
    },

    get currentSearchQuery()
    {
        return this._searchQuery;
    },

    set automaticallyRevealFirstSearchResult(reveal)
    {
        this._automaticallyRevealFirstSearchResult = reveal;

        // If we haven't shown a search result yet, reveal one now.
        if (this._automaticallyRevealFirstSearchResult && this._searchResults.length > 0) {
            if (this._currentSearchResultIndex === -1)
                this._revealFirstSearchResultAfterCursor();
        }
    },

    performSearch: function(query)
    {
        if (this._searchQuery === query)
            return;

        this.searchCleared();

        this._searchQuery = query;

        // Allow subclasses to handle the searching if they have a better way.
        // If we are formatted, just use CodeMirror's search.
        if (typeof this.customPerformSearch === "function" && !this.formatted) {
            if (this.customPerformSearch(query))
                return;
        }

        // Go down the slow patch for all other text content.
        var searchCursor = this._codeMirror.getSearchCursor(query, {line: 0, ch: 0}, true);
        var boundBatchSearch = batchSearch.bind(this);
        var numberOfSearchResultsDidChangeTimeout = null;

        function reportNumberOfSearchResultsDidChange()
        {
            if (numberOfSearchResultsDidChangeTimeout) {
                clearTimeout(numberOfSearchResultsDidChangeTimeout);
                numberOfSearchResultsDidChangeTimeout = null;
            }

            this.dispatchEventToListeners(WebInspector.TextEditor.Event.NumberOfSearchResultsDidChange);
        }

        function batchSearch()
        {
            // Bail if the query changed since we started.
            if (this._searchQuery !== query)
                return;

            var newSearchResults = [];
            var foundResult = false;
            for (var i = 0; i < WebInspector.TextEditor.NumberOfFindsPerSearchBatch && (foundResult = searchCursor.findNext()); ++i) {
                var textRange = this._textRangeFromCodeMirrorPosition(searchCursor.from(), searchCursor.to());
                newSearchResults.push(textRange);
            }

            this.addSearchResults(newSearchResults);

            // Don't report immediately, coalesce updates so they come in no faster than half a second.
            if (!numberOfSearchResultsDidChangeTimeout)
                numberOfSearchResultsDidChangeTimeout = setTimeout(reportNumberOfSearchResultsDidChange.bind(this), 500);

            if (foundResult) {
                // More lines to search, set a timeout so we don't block the UI long.
                setTimeout(boundBatchSearch, 50);
            } else {
                // Report immediately now that we are finished, canceling any pending update.
                reportNumberOfSearchResultsDidChange.call(this);
            }
        }

        // Start the search.
        boundBatchSearch();
    },

    addSearchResults: function(textRanges)
    {
        console.assert(textRanges);
        if (!textRanges || !textRanges.length)
            return;

        function markRanges()
        {
            for (var i = 0; i < textRanges.length; ++i) {
                var position = this._codeMirrorPositionFromTextRange(textRanges[i]);
                var mark = this._codeMirror.markText(position.start, position.end, {className: WebInspector.TextEditor.SearchResultStyleClassName});
                this._searchResults.push(mark);
            }

            // If we haven't shown a search result yet, reveal one now.
            if (this._automaticallyRevealFirstSearchResult) {
                if (this._currentSearchResultIndex === -1)
                    this._revealFirstSearchResultAfterCursor();
            }
        }

        this._codeMirror.operation(markRanges.bind(this));
    },

    searchCleared: function()
    {
        function clearResults() {
            for (var i = 0; i < this._searchResults.length; ++i)
                this._searchResults[i].clear();
        }

        this._codeMirror.operation(clearResults.bind(this));

        this._searchQuery = null;
        this._searchResults = [];
        this._currentSearchResultIndex = -1;
    },

    searchQueryWithSelection: function()
    {
        if (!this._codeMirror.somethingSelected())
            return null;

        return this._codeMirror.getSelection();
    },

    revealPreviousSearchResult: function(changeFocus)
    {
        if (!this._searchResults.length)
            return;

        if (this._currentSearchResultIndex === -1 || this._cursorDoesNotMatchLastRevealedSearchResult()) {
            this._revealFirstSearchResultBeforeCursor(changeFocus);
            return;
        }

        if (this._currentSearchResultIndex > 0)
            --this._currentSearchResultIndex;
        else
            this._currentSearchResultIndex = this._searchResults.length - 1;

        this._revealSearchResult(this._searchResults[this._currentSearchResultIndex], changeFocus, -1);
    },

    revealNextSearchResult: function(changeFocus)
    {
        if (!this._searchResults.length)
            return;

        if (this._currentSearchResultIndex === -1 || this._cursorDoesNotMatchLastRevealedSearchResult()) {
            this._revealFirstSearchResultAfterCursor(changeFocus);
            return;
        }

        if (this._currentSearchResultIndex + 1 < this._searchResults.length)
            ++this._currentSearchResultIndex;
        else
            this._currentSearchResultIndex = 0;

        this._revealSearchResult(this._searchResults[this._currentSearchResultIndex], changeFocus, 1);
    },

    line: function(lineNumber)
    {
        return this._codeMirror.getLine(lineNumber);
    },

    revealPosition: function(position, textRangeToSelect, forceUnformatted)
    {
        console.assert(position === undefined || position instanceof WebInspector.SourceCodePosition, "revealPosition called without a SourceCodePosition");
        if (!(position instanceof WebInspector.SourceCodePosition))
            return;

        var lineHandle = this._codeMirror.getLineHandle(position.lineNumber);
        if (!lineHandle || !this._visible || this._initialStringNotSet) {
            // If we can't get a line handle or are not visible then we wait to do the reveal.
            this._positionToReveal = position;
            this._textRangeToSelect = textRangeToSelect;
            this._forceUnformatted = forceUnformatted;
            return;
        }

        // Delete now that the reveal is happening.
        delete this._positionToReveal;
        delete this._textRangeToSelect;
        delete this._forceUnformatted;

        // If we need to unformat, reveal the line after a wait.
        // Otherwise the line highlight doesn't work properly.
        if (this._formatted && forceUnformatted) {
            this.formatted = false;
            setTimeout(this.revealPosition.bind(this), 0, position, textRangeToSelect);
            return;
        }

        if (!textRangeToSelect)
            textRangeToSelect = new WebInspector.TextRange(position.lineNumber, position.columnNumber, position.lineNumber, position.columnNumber);

        function removeStyleClass()
        {
            this._codeMirror.removeLineClass(lineHandle, "wrap", WebInspector.TextEditor.HighlightedStyleClassName);
        }

        function revealAndHighlightLine()
        {
            // If the line is not visible, reveal it as the center line in the editor.
            var position = this._codeMirrorPositionFromTextRange(textRangeToSelect);
            if (!this._isPositionVisible(position.start))
                this._scrollIntoViewCentered(position.start);

            this.selectedTextRange = textRangeToSelect;

            this._codeMirror.addLineClass(lineHandle, "wrap", WebInspector.TextEditor.HighlightedStyleClassName);

            // Use a timeout instead of a webkitAnimationEnd event listener because the line element might
            // be removed if the user scrolls during the animation. In that case webkitAnimationEnd isn't
            // fired, and the line would highlight again the next time it scrolls into view.
            setTimeout(removeStyleClass.bind(this), WebInspector.TextEditor.HighlightAnimationDuration);
        }

        this._codeMirror.operation(revealAndHighlightLine.bind(this));
    },

    updateLayout: function(force)
    {
        this._codeMirror.refresh();
    },

    shown: function()
    {
        this._visible = true;

        // Refresh since our size might have changed.
        this._codeMirror.refresh();

        // Try revealing the pending line now that we are visible.
        // This needs to be done as a separate operation from the refresh
        // so that the scrollInfo coordinates are correct.
        this._revealPendingPositionIfPossible();
    },

    hidden: function()
    {
        this._visible = false;
    },

    setBreakpointInfoForLineAndColumn: function(lineNumber, columnNumber, breakpointInfo)
    {
        if (this._ignoreSetBreakpointInfoCalls)
            return;

        if (breakpointInfo)
            this._addBreakpointToLineAndColumnWithInfo(lineNumber, columnNumber, breakpointInfo);
        else
            this._removeBreakpointFromLineAndColumn(lineNumber, columnNumber);
    },

    updateBreakpointLineAndColumn: function(oldLineNumber, oldColumnNumber, newLineNumber, newColumnNumber)
    {
        console.assert(this._breakpoints[oldLineNumber]);
        if (!this._breakpoints[oldLineNumber])
            return;

        console.assert(this._breakpoints[oldLineNumber][oldColumnNumber]);
        if (!this._breakpoints[oldLineNumber][oldColumnNumber])
            return;

        var breakpointInfo = this._breakpoints[oldLineNumber][oldColumnNumber];
        this._removeBreakpointFromLineAndColumn(oldLineNumber, oldColumnNumber);
        this._addBreakpointToLineAndColumnWithInfo(newLineNumber, newColumnNumber, breakpointInfo);
    },

    addStyleClassToLine: function(lineNumber, styleClassName)
    {
        var lineHandle = this._codeMirror.getLineHandle(lineNumber);
        console.assert(lineHandle);
        if (!lineHandle)
            return;

        return this._codeMirror.addLineClass(lineHandle, "wrap", styleClassName);
    },

    removeStyleClassFromLine: function(lineNumber, styleClassName)
    {
        var lineHandle = this._codeMirror.getLineHandle(lineNumber);
        console.assert(lineHandle);
        if (!lineHandle)
            return;

        return this._codeMirror.removeLineClass(lineHandle, "wrap", styleClassName);
    },

    toggleStyleClassForLine: function(lineNumber, styleClassName)
    {
        var lineHandle = this._codeMirror.getLineHandle(lineNumber);
        console.assert(lineHandle);
        if (!lineHandle)
            return;

        return this._codeMirror.toggleLineClass(lineHandle, "wrap", styleClassName);
    },

    // Private

    _updateCodeMirrorReadOnly: function()
    {
        this._codeMirror.setOption("readOnly", this._readOnly || this._formatted);
    },

    _contentChanged: function(codeMirror, change)
    {
        if (this._ignoreCodeMirrorContentDidChangeEvent)
            return;
        this.dispatchEventToListeners(WebInspector.TextEditor.Event.ContentDidChange);
    },

    _textRangeFromCodeMirrorPosition: function(start, end)
    {
        console.assert(start);
        console.assert(end);

        return new WebInspector.TextRange(start.line, start.ch, end.line, end.ch);
    },

    _codeMirrorPositionFromTextRange: function(textRange)
    {
        console.assert(textRange);

        var start = {line: textRange.startLine, ch: textRange.startColumn};
        var end = {line: textRange.endLine, ch: textRange.endColumn};
        return {start: start, end: end};
    },

    _revealPendingPositionIfPossible: function()
    {
        // Nothing to do if we don't have a pending position.
        if (!this._positionToReveal)
            return;

        // Don't try to reveal unless we are visible.
        if (!this._visible)
            return;

        this.revealPosition(this._positionToReveal, this._textRangeToSelect, this._forceUnformatted);
    },

    _revealSearchResult: function(result, changeFocus, directionInCaseOfRevalidation)
    {
        var position = result.find();

        // Check for a valid position, it might have been removed from editing by the user.
        // If the position is invalide, revalidate all positions reveal as needed.
        if (!position) {
            this._revalidateSearchResults(directionInCaseOfRevalidation);
            return;
        }

        // If the line is not visible, reveal it as the center line in the editor.
        if (!this._isPositionVisible(position.from))
            this._scrollIntoViewCentered(position.from);

        // Update the text selection to select the search result.
        this.selectedTextRange = this._textRangeFromCodeMirrorPosition(position.from, position.to);

        // Remove the automatically reveal state now that we have revealed a search result.
        this._automaticallyRevealFirstSearchResult = false;

        // Focus the editor if requested.
        if (changeFocus)
            this._codeMirror.focus();

        // Remove the bouncy highlight if it is still around. The animation will not
        // start unless we remove it and add it back to the document.
        if (this._bouncyHighlightElement)
            this._bouncyHighlightElement.remove();

        // Create the bouncy highlight.
        this._bouncyHighlightElement = document.createElement("div");
        this._bouncyHighlightElement.className = WebInspector.TextEditor.BouncyHighlightStyleClassName;

        // Collect info for the bouncy highlight.
        var textContent = this._codeMirror.getSelection();
        var coordinates = this._codeMirror.cursorCoords(true, "page");

        // Adjust the coordinates to be based in the text editor's space.
        var textEditorRect = this._element.getBoundingClientRect();
        coordinates.top -= textEditorRect.top;
        coordinates.left -= textEditorRect.left;

        // Position and show the bouncy highlight.
        this._bouncyHighlightElement.textContent = textContent;
        this._bouncyHighlightElement.style.top = coordinates.top + "px";
        this._bouncyHighlightElement.style.left = coordinates.left + "px";
        this._element.appendChild(this._bouncyHighlightElement);

        function animationEnded()
        {
            if (!this._bouncyHighlightElement)
                return;

            this._bouncyHighlightElement.remove();
            delete this._bouncyHighlightElement;
        }

        // Listen for the end of the animation so we can remove the element.
        this._bouncyHighlightElement.addEventListener("webkitAnimationEnd", animationEnded.bind(this));
    },

    _binarySearchInsertionIndexInSearchResults: function(object, comparator)
    {
        // It is possible that markers in the search results array may have been deleted.
        // In those cases the comparator will return "null" and we immediately stop
        // the binary search and return null. The search results list needs to be updated.
        var array = this._searchResults;

        var first = 0;
        var last = array.length - 1;

        while (first <= last) {
            var mid = (first + last) >> 1;
            var c = comparator(object, array[mid]);
            if (c === null)
                return null;
            if (c > 0)
                first = mid + 1;
            else if (c < 0)
                last = mid - 1;
            else
                return mid;
        }

        return first - 1;
    },

    _revealFirstSearchResultBeforeCursor: function(changeFocus)
    {
        console.assert(this._searchResults.length);

        var currentCursorPosition = this._codeMirror.getCursor("start");
        if (currentCursorPosition.line === 0 && currentCursorPosition.ch === 0) {
            this._currentSearchResultIndex = this._searchResults.length - 1;
            this._revealSearchResult(this._searchResults[this._currentSearchResultIndex], changeFocus, -1);
            return;
        }

        var index = this._binarySearchInsertionIndexInSearchResults(currentCursorPosition, function(current, searchResult) {
            var searchResultMarker = searchResult.find();
            if (!searchResultMarker)
                return null;
            return WebInspector.compareCodeMirrorPositions(current, searchResultMarker.from);
        });

        if (index === null) {
            this._revalidateSearchResults(-1);
            return;
        }

        this._currentSearchResultIndex = index;
        this._revealSearchResult(this._searchResults[this._currentSearchResultIndex], changeFocus);
    },

    _revealFirstSearchResultAfterCursor: function(changeFocus)
    {
        console.assert(this._searchResults.length);

        var currentCursorPosition = this._codeMirror.getCursor("start");
        if (currentCursorPosition.line === 0 && currentCursorPosition.ch === 0) {
            this._currentSearchResultIndex = 0;
            this._revealSearchResult(this._searchResults[this._currentSearchResultIndex], changeFocus, 1);
            return;
        }

        var index = this._binarySearchInsertionIndexInSearchResults(currentCursorPosition, function(current, searchResult) {
            var searchResultMarker = searchResult.find();
            if (!searchResultMarker)
                return null;
            return WebInspector.compareCodeMirrorPositions(current, searchResultMarker.from);
        });

        if (index === null) {
            this._revalidateSearchResults(1);
            return;
        }

        if (index + 1 < this._searchResults.length)
            ++index;
        else
            index = 0;

        this._currentSearchResultIndex = index;
        this._revealSearchResult(this._searchResults[this._currentSearchResultIndex], changeFocus);
    },

    _cursorDoesNotMatchLastRevealedSearchResult: function()
    {
        console.assert(this._currentSearchResultIndex !== -1);
        console.assert(this._searchResults.length);

        var lastRevealedSearchResultMarker = this._searchResults[this._currentSearchResultIndex].find();
        if (!lastRevealedSearchResultMarker)
            return true;

        var currentCursorPosition = this._codeMirror.getCursor("start");
        var lastRevealedSearchResultPosition = lastRevealedSearchResultMarker.from;

        return WebInspector.compareCodeMirrorPositions(currentCursorPosition, lastRevealedSearchResultPosition) !== 0;
    },

    _revalidateSearchResults: function(direction)
    {
        console.assert(direction !== undefined);

        this._currentSearchResultIndex = -1;

        var updatedSearchResults = [];
        for (var i = 0; i < this._searchResults.length; ++i) {
            if (this._searchResults[i].find())
                updatedSearchResults.push(this._searchResults[i]);
        }

        console.assert(updatedSearchResults.length !== this._searchResults.length);

        this._searchResults = updatedSearchResults;
        this.dispatchEventToListeners(WebInspector.TextEditor.Event.NumberOfSearchResultsDidChange);

        if (this._searchResults.length) {
            if (direction > 0)
                this._revealFirstSearchResultAfterCursor();
            else
                this._revealFirstSearchResultBeforeCursor();
        }
    },

    _updateExecutionLine: function()
    {
        function update()
        {
            if (this._executionLineHandle)
                this._codeMirror.removeLineClass(this._executionLineHandle, "wrap", WebInspector.TextEditor.ExecutionLineStyleClassName);

            this._executionLineHandle = !isNaN(this._executionLineNumber) ? this._codeMirror.getLineHandle(this._executionLineNumber) : null;

            if (this._executionLineHandle)
                this._codeMirror.addLineClass(this._executionLineHandle, "wrap", WebInspector.TextEditor.ExecutionLineStyleClassName);
        }

        this._codeMirror.operation(update.bind(this));
    },

    _setBreakpointStylesOnLine: function(lineNumber)
    {
        var columnBreakpoints = this._breakpoints[lineNumber];
        console.assert(columnBreakpoints);
        if (!columnBreakpoints)
            return;

        var allDisabled = true;
        var allResolved = true;
        var multiple = Object.keys(columnBreakpoints).length > 1;
        for (var columnNumber in columnBreakpoints) {
            var breakpointInfo = columnBreakpoints[columnNumber];
            if (!breakpointInfo.disabled)
                allDisabled = false;
            if (!breakpointInfo.resolved)
                allResolved = false;
        }

        function updateStyles()
        {
            // We might not have a line if the content isn't fully populated yet.
            // This will be called again when the content is available.
            var lineHandle = this._codeMirror.getLineHandle(lineNumber);
            if (!lineHandle)
                return;

            this._codeMirror.addLineClass(lineHandle, "wrap", WebInspector.TextEditor.HasBreakpointStyleClassName);

            if (allResolved)
                this._codeMirror.addLineClass(lineHandle, "wrap", WebInspector.TextEditor.BreakpointResolvedStyleClassName);
            else
                this._codeMirror.removeLineClass(lineHandle, "wrap", WebInspector.TextEditor.BreakpointResolvedStyleClassName);

            if (allDisabled)
                this._codeMirror.addLineClass(lineHandle, "wrap", WebInspector.TextEditor.BreakpointDisabledStyleClassName);
            else
                this._codeMirror.removeLineClass(lineHandle, "wrap", WebInspector.TextEditor.BreakpointDisabledStyleClassName);

            if (multiple)
                this._codeMirror.addLineClass(lineHandle, "wrap", WebInspector.TextEditor.MultipleBreakpointsStyleClassName);
            else
                this._codeMirror.removeLineClass(lineHandle, "wrap", WebInspector.TextEditor.MultipleBreakpointsStyleClassName);
        }

        this._codeMirror.operation(updateStyles.bind(this));
    },

    _addBreakpointToLineAndColumnWithInfo: function(lineNumber, columnNumber, breakpointInfo)
    {
        if (!this._breakpoints[lineNumber])
            this._breakpoints[lineNumber] = {};
        this._breakpoints[lineNumber][columnNumber] = breakpointInfo;

        this._setBreakpointStylesOnLine(lineNumber);
    },

    _removeBreakpointFromLineAndColumn: function(lineNumber, columnNumber)
    {
        console.assert(columnNumber in this._breakpoints[lineNumber]);
        delete this._breakpoints[lineNumber][columnNumber];

        // There are still breakpoints on the line. Update the breakpoint style.
        if (!isEmptyObject(this._breakpoints[lineNumber])) {
            this._setBreakpointStylesOnLine(lineNumber);
            return;
        }

        delete this._breakpoints[lineNumber];

        function updateStyles()
        {
            var lineHandle = this._codeMirror.getLineHandle(lineNumber);
            if (!lineHandle)
                return;

            this._codeMirror.removeLineClass(lineHandle, "wrap", WebInspector.TextEditor.HasBreakpointStyleClassName);
            this._codeMirror.removeLineClass(lineHandle, "wrap", WebInspector.TextEditor.BreakpointResolvedStyleClassName);
            this._codeMirror.removeLineClass(lineHandle, "wrap", WebInspector.TextEditor.BreakpointDisabledStyleClassName);
            this._codeMirror.removeLineClass(lineHandle, "wrap", WebInspector.TextEditor.MultipleBreakpointsStyleClassName);
        }

        this._codeMirror.operation(updateStyles.bind(this));
    },

    _allColumnBreakpointInfoForLine: function(lineNumber)
    {
        return this._breakpoints[lineNumber];
    },

    _setColumnBreakpointInfoForLine: function(lineNumber, columnBreakpointInfo)
    {
        console.assert(columnBreakpointInfo);
        this._breakpoints[lineNumber] = columnBreakpointInfo;
        this._setBreakpointStylesOnLine(lineNumber);
    },

    _gutterMouseDown: function(codeMirror, lineNumber, gutterElement, event)
    {
        if (event.button !== 0 || event.ctrlKey)
            return;

        if (!this._codeMirror.hasLineClass(lineNumber, "wrap", WebInspector.TextEditor.HasBreakpointStyleClassName)) {
            console.assert(!(lineNumber in this._breakpoints));

            // No breakpoint, add a new one.
            if (this._delegate && typeof this._delegate.textEditorBreakpointAdded === "function") {
                var data = this._delegate.textEditorBreakpointAdded(this, lineNumber, 0);
                if (data) {
                    var breakpointInfo = data.breakpointInfo;
                    if (breakpointInfo)
                        this._addBreakpointToLineAndColumnWithInfo(data.lineNumber, data.columnNumber, breakpointInfo);
                }
            }

            return;
        }

        console.assert(lineNumber in this._breakpoints);

        if (this._codeMirror.hasLineClass(lineNumber, "wrap", WebInspector.TextEditor.MultipleBreakpointsStyleClassName)) {
            console.assert(!isEmptyObject(this._breakpoints[lineNumber]));
            return;
        }

        // Single existing breakpoint, start tracking it for dragging.
        console.assert(Object.keys(this._breakpoints[lineNumber]).length === 1);
        var columnNumber = Object.keys(this._breakpoints[lineNumber])[0];
        this._draggingBreakpointInfo = this._breakpoints[lineNumber][columnNumber];
        this._lineNumberWithMousedDownBreakpoint = lineNumber;
        this._lineNumberWithDraggedBreakpoint = lineNumber;
        this._columnNumberWithMousedDownBreakpoint = columnNumber;
        this._columnNumberWithDraggedBreakpoint = columnNumber;

        this._documentMouseMovedEventListener = this._documentMouseMoved.bind(this);
        this._documentMouseUpEventListener = this._documentMouseUp.bind(this);

        // Register these listeners on the document so we can track the mouse if it leaves the gutter.
        document.addEventListener("mousemove", this._documentMouseMovedEventListener, true);
        document.addEventListener("mouseup", this._documentMouseUpEventListener, true);
    },

    _documentMouseMoved: function(event)
    {
        console.assert("_lineNumberWithMousedDownBreakpoint" in this);
        if (!("_lineNumberWithMousedDownBreakpoint" in this))
            return;

        event.preventDefault();

        var lineNumber;
        var position = this._codeMirror.coordsChar({left: event.pageX, top: event.pageY});

        // CodeMirror's coordsChar returns a position even if it is outside the bounds. Nullify the position
        // if the event is outside the bounds of the gutter so we will remove the breakpoint.
        var gutterBounds = this._codeMirror.getGutterElement().getBoundingClientRect();
        if (event.pageX < gutterBounds.left || event.pageX > gutterBounds.right || event.pageY < gutterBounds.top || event.pageY > gutterBounds.bottom)
            position = null;

        // If we have a position and it has a line then use it.
        if (position && "line" in position)
            lineNumber = position.line;

        // The _lineNumberWithDraggedBreakpoint property can be undefined if the user drags
        // outside of the gutter. The lineNumber variable can be undefined for the same reason.

        if (lineNumber === this._lineNumberWithDraggedBreakpoint)
            return;

        // Record that the mouse dragged some so when mouse up fires we know to do the
        // work of removing and moving the breakpoint.
        this._mouseDragged = true;

        if ("_lineNumberWithDraggedBreakpoint" in this) {
            // We have a line that is currently showing the dragged breakpoint. Remove that breakpoint
            // and restore the previous one (if any.)
            if (this._previousColumnBreakpointInfo)
                this._setColumnBreakpointInfoForLine(this._lineNumberWithDraggedBreakpoint, this._previousColumnBreakpointInfo);
            else
                this._removeBreakpointFromLineAndColumn(this._lineNumberWithDraggedBreakpoint, this._columnNumberWithDraggedBreakpoint);

            delete this._previousColumnBreakpointInfo;
            delete this._lineNumberWithDraggedBreakpoint;
            delete this._columnNumberWithDraggedBreakpoint;
        }

        if (lineNumber !== undefined) {
            // We have a new line that will now show the dragged breakpoint.
            var newColumnBreakpoints = {};
            var columnNumber = (lineNumber === this._lineNumberWithMousedDownBreakpoint ? this._columnNumberWithDraggedBreakpoint : 0)
            newColumnBreakpoints[columnNumber] = this._draggingBreakpointInfo;
            this._previousColumnBreakpointInfo = this._allColumnBreakpointInfoForLine(lineNumber);
            this._setColumnBreakpointInfoForLine(lineNumber, newColumnBreakpoints);
            this._lineNumberWithDraggedBreakpoint = lineNumber;
            this._columnNumberWithDraggedBreakpoint = columnNumber;
        }
    },

    _documentMouseUp: function(event)
    {
        console.assert("_lineNumberWithMousedDownBreakpoint" in this);
        if (!("_lineNumberWithMousedDownBreakpoint" in this))
            return;

        event.preventDefault();

        document.removeEventListener("mousemove", this._documentMouseMovedEventListener, true);
        document.removeEventListener("mouseup", this._documentMouseUpEventListener, true);

        const delegateImplementsBreakpointToggled = this._delegate && typeof this._delegate.textEditorBreakpointToggled === "function";
        const delegateImplementsBreakpointRemoved = this._delegate && typeof this._delegate.textEditorBreakpointRemoved === "function";
        const delegateImplementsBreakpointMoved = this._delegate && typeof this._delegate.textEditorBreakpointMoved === "function";

        if (this._mouseDragged) {
            if (!("_lineNumberWithDraggedBreakpoint" in this)) {
                // The breakpoint was dragged off the gutter, remove it.
                if (delegateImplementsBreakpointRemoved) {
                    this._ignoreSetBreakpointInfoCalls = true;
                    this._delegate.textEditorBreakpointRemoved(this, this._lineNumberWithMousedDownBreakpoint, this._columnNumberWithMousedDownBreakpoint);
                    delete this._ignoreSetBreakpointInfoCalls;
                }
            } else if (this._lineNumberWithMousedDownBreakpoint !== this._lineNumberWithDraggedBreakpoint) {
                // The dragged breakpoint was moved to a new line.

                // If there is are breakpoints already at the drop line, tell the delegate to remove them.
                // We have already updated the breakpoint info internally, so when the delegate removes the breakpoints
                // and tells us to clear the breakpoint info, we can ignore those calls.
                if (this._previousColumnBreakpointInfo && delegateImplementsBreakpointRemoved) {
                    this._ignoreSetBreakpointInfoCalls = true;
                    for (var columnNumber in this._previousColumnBreakpointInfo)
                        this._delegate.textEditorBreakpointRemoved(this, this._lineNumberWithDraggedBreakpoint, columnNumber);
                    delete this._ignoreSetBreakpointInfoCalls;
                }

                // Tell the delegate to move the breakpoint from one line to another.
                if (delegateImplementsBreakpointMoved) {
                    this._ignoreSetBreakpointInfoCalls = true;
                    this._delegate.textEditorBreakpointMoved(this, this._lineNumberWithMousedDownBreakpoint, this._columnNumberWithMousedDownBreakpoint, this._lineNumberWithDraggedBreakpoint, this._columnNumberWithDraggedBreakpoint);
                    delete this._ignoreSetBreakpointInfoCalls;
                }
            }
        } else {
            // Toggle the disabled state of the breakpoint.
            console.assert(this._lineNumberWithMousedDownBreakpoint in this._breakpoints);
            console.assert(this._columnNumberWithMousedDownBreakpoint in this._breakpoints[this._lineNumberWithMousedDownBreakpoint]);
            if (this._lineNumberWithMousedDownBreakpoint in this._breakpoints && this._columnNumberWithMousedDownBreakpoint in this._breakpoints[this._lineNumberWithMousedDownBreakpoint] && delegateImplementsBreakpointToggled) {
                var disabled = this._codeMirror.toggleLineClass(this._lineNumberWithMousedDownBreakpoint, "wrap", WebInspector.TextEditor.BreakpointDisabledStyleClassName);
                this._breakpoints[this._lineNumberWithMousedDownBreakpoint][this._columnNumberWithMousedDownBreakpoint].disabled = disabled;
                this._delegate.textEditorBreakpointToggled(this, this._lineNumberWithMousedDownBreakpoint, this._columnNumberWithMousedDownBreakpoint, disabled);
            }
        }

        delete this._documentMouseMovedEventListener;
        delete this._documentMouseUpEventListener;
        delete this._lineNumberWithMousedDownBreakpoint;
        delete this._lineNumberWithDraggedBreakpoint;
        delete this._columnNumberWithMousedDownBreakpoint;
        delete this._columnNumberWithDraggedBreakpoint;
        delete this._previousColumnBreakpointInfo;
        delete this._mouseDragged;
    },

    _openClickedLinks: function(event)
    {
        // Get the position in the text and the token at that position.
        var position = this._codeMirror.coordsChar({left: event.pageX, top: event.pageY});
        var tokenInfo = this._codeMirror.getTokenAt(position);
        if (!tokenInfo || !tokenInfo.type || !tokenInfo.string)
            return;

        // If the token is not a link, then ignore it.
        if (!/\blink\b/.test(tokenInfo.type))
            return;

        // The token string is the URL we should open. It might be a relative URL.
        var url = tokenInfo.string;

        // Get the base URL.
        var baseURL = "";
        if (this._delegate && typeof this._delegate.textEditorBaseURL === "function")
            baseURL = this._delegate.textEditorBaseURL(this);

        // Open the link after resolving the absolute URL from the base URL.
        WebInspector.openURL(absoluteURL(url, baseURL));

        // Stop processing the event.
        event.preventDefault();
        event.stopPropagation();
    },

    _isPositionVisible: function(position)
    {
        var scrollInfo = this._codeMirror.getScrollInfo();
        var visibleRangeStart = scrollInfo.top;
        var visibleRangeEnd = visibleRangeStart + scrollInfo.clientHeight;
        var coords = this._codeMirror.charCoords(position, "local");

        return coords.top >= visibleRangeStart && coords.bottom <= visibleRangeEnd;
    },

    _scrollIntoViewCentered: function(position)
    {
        var scrollInfo = this._codeMirror.getScrollInfo();
        var lineHeight = Math.ceil(this._codeMirror.defaultTextHeight());
        var margin = Math.floor((scrollInfo.clientHeight - lineHeight) / 2);
        this._codeMirror.scrollIntoView(position, margin);
    },

    _prettyPrint: function(pretty)
    {
        function prettyPrintAndUpdateEditor()
        {
            const start = {line: 0, ch: 0};
            const end = {line: this._codeMirror.lineCount() - 1};

            var oldSelectionAnchor = this._codeMirror.getCursor("anchor");
            var oldSelectionHead = this._codeMirror.getCursor("head");
            var newSelectionAnchor, newSelectionHead;
            var newExecutionLocation = null;

            if (pretty) {
                // <rdar://problem/10593948> Provide a way to change the tab width in the Web Inspector
                const indentString = "    ";
                var originalLineEndings = [];
                var formattedLineEndings = [];
                var mapping = {original: [0], formatted: [0]};
                var builder = new FormatterContentBuilder(mapping, originalLineEndings, formattedLineEndings, 0, 0, indentString);
                var formatter = new Formatter(this._codeMirror, builder);
                formatter.format(start, end);

                this._formatterSourceMap = WebInspector.FormatterSourceMap.fromBuilder(builder);

                this._codeMirror.setValue(builder.formattedContent);

                if (this._positionToReveal) {
                    var newRevealPosition = this._formatterSourceMap.originalToFormatted(this._positionToReveal.lineNumber, this._positionToReveal.columnNumber);
                    this._positionToReveal = new WebInspector.SourceCodePosition(newRevealPosition.lineNumber, newRevealPosition.columnNumber);
                }

                if (this._textRangeToSelect) {
                    var mappedRevealSelectionStart = this._formatterSourceMap.originalToFormatted(this._textRangeToSelect.startLine, this._textRangeToSelect.startColumn);
                    var mappedRevealSelectionEnd = this._formatterSourceMap.originalToFormatted(this._textRangeToSelect.endLine, this._textRangeToSelect.endColumn);
                    this._textRangeToSelect = new WebInspector.TextRange(mappedRevealSelectionStart.lineNumber, mappedRevealSelectionStart.columnNumber, mappedRevealSelectionEnd.lineNumber, mappedRevealSelectionEnd.columnNumber);
                }

                if (!isNaN(this._executionLineNumber)) {
                    console.assert(this._executionLineHandle);
                    console.assert(!isNaN(this._executionColumnNumber));
                    newExecutionLocation = this._formatterSourceMap.originalToFormatted(this._executionLineNumber, this._executionColumnNumber);
                }

                var mappedAnchorLocation = this._formatterSourceMap.originalToFormatted(oldSelectionAnchor.line, oldSelectionAnchor.ch);
                var mappedHeadLocation = this._formatterSourceMap.originalToFormatted(oldSelectionHead.line, oldSelectionHead.ch);
                newSelectionAnchor = {line:mappedAnchorLocation.lineNumber, ch:mappedAnchorLocation.columnNumber};
                newSelectionHead = {line:mappedHeadLocation.lineNumber, ch:mappedHeadLocation.columnNumber};
            } else {
                this._codeMirror.undo();

                if (this._positionToReveal) {
                    var newRevealPosition = this._formatterSourceMap.formattedToOriginal(this._positionToReveal.lineNumber, this._positionToReveal.columnNumber);
                    this._positionToReveal = new WebInspector.SourceCodePosition(newRevealPosition.lineNumber, newRevealPosition.columnNumber);
                }

                if (this._textRangeToSelect) {
                    var mappedRevealSelectionStart = this._formatterSourceMap.formattedToOriginal(this._textRangeToSelect.startLine, this._textRangeToSelect.startColumn);
                    var mappedRevealSelectionEnd = this._formatterSourceMap.formattedToOriginal(this._textRangeToSelect.endLine, this._textRangeToSelect.endColumn);
                    this._textRangeToSelect = new WebInspector.TextRange(mappedRevealSelectionStart.lineNumber, mappedRevealSelectionStart.columnNumber, mappedRevealSelectionEnd.lineNumber, mappedRevealSelectionEnd.columnNumber);
                }

                if (!isNaN(this._executionLineNumber)) {
                    console.assert(this._executionLineHandle);
                    console.assert(!isNaN(this._executionColumnNumber));
                    newExecutionLocation = this._formatterSourceMap.formattedToOriginal(this._executionLineNumber, this._executionColumnNumber);
                }

                var mappedAnchorLocation = this._formatterSourceMap.formattedToOriginal(oldSelectionAnchor.line, oldSelectionAnchor.ch);
                var mappedHeadLocation = this._formatterSourceMap.formattedToOriginal(oldSelectionHead.line, oldSelectionHead.ch);
                newSelectionAnchor = {line:mappedAnchorLocation.lineNumber, ch:mappedAnchorLocation.columnNumber};
                newSelectionHead = {line:mappedHeadLocation.lineNumber, ch:mappedHeadLocation.columnNumber};

                this._formatterSourceMap = null;
            }

            this._scrollIntoViewCentered(newSelectionAnchor);
            this._codeMirror.setSelection(newSelectionAnchor, newSelectionHead);

            if (newExecutionLocation) {
                delete this._executionLineHandle;
                this.executionColumnNumber = newExecutionLocation.columnNumber;
                this.executionLineNumber = newExecutionLocation.lineNumber;
            }

            // FIXME: <rdar://problem/13129955> FindBanner: New searches should not lose search position (start from current selection/caret)
            if (this.currentSearchQuery) {
                var searchQuery = this.currentSearchQuery;
                this.searchCleared();
                // Set timeout so that this happens after the current CodeMirror operation.
                // The editor has to update for the value and selection changes.
                setTimeout(function(query) {
                    this.performSearch(searchQuery);
                }.bind(this), 0);
            }

            if (this._delegate && typeof this._delegate.textEditorUpdatedFormatting === "function")
                this._delegate.textEditorUpdatedFormatting(this);
        }

        this._codeMirror.operation(prettyPrintAndUpdateEditor.bind(this));
    }
};

WebInspector.TextEditor.prototype.__proto__ = WebInspector.Object.prototype;
