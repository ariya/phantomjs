/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

WebInspector.SourceFrame = function(delegate, url)
{
    WebInspector.TextViewerDelegate.call(this);

    this._delegate = delegate;
    this._url = url;

    this._textModel = new WebInspector.TextEditorModel();
    this._textModel.replaceTabsWithSpaces = true;

    this._textViewer = new WebInspector.TextViewer(this._textModel, WebInspector.platform, this._url, this);
    this._textViewer.element.addStyleClass("script-view");
    this._visible = false;

    this._currentSearchResultIndex = -1;
    this._searchResults = [];

    this._messages = [];
    this._rowMessages = {};
    this._messageBubbles = {};

    this._breakpoints = {};
}

WebInspector.SourceFrame.Events = {
    Loaded: "loaded"
}

WebInspector.SourceFrame.prototype = {
    get visible()
    {
        return this._textViewer.visible;
    },

    set visible(x)
    {
        this._textViewer.visible = x;
    },

    show: function(parentElement)
    {
        this._ensureContentLoaded();

        this._textViewer.show(parentElement);
        this._textViewer.resize();

        if (this.loaded) {
            if (this._scrollTop)
                this._textViewer.scrollTop = this._scrollTop;
            if (this._scrollLeft)
                this._textViewer.scrollLeft = this._scrollLeft;
        }
    },

    hide: function()
    {
        if (this.loaded) {
            this._scrollTop = this._textViewer.scrollTop;
            this._scrollLeft = this._textViewer.scrollLeft;
            this._textViewer.freeCachedElements();
        }

        this._textViewer.hide();
        this._hidePopup();
        this._clearLineHighlight();
    },

    detach: function()
    {
        this._textViewer.detach();
    },

    get element()
    {
        return this._textViewer.element;
    },

    get loaded()
    {
        return !!this._content;
    },

    hasContent: function()
    {
        return true;
    },

    _ensureContentLoaded: function()
    {
        if (!this._contentRequested) {
            this._contentRequested = true;
            this.requestContent(this._initializeTextViewer.bind(this));
        }
    },

    requestContent: function(callback)
    {
        this._delegate.requestContent(callback);
    },

    markDiff: function(diffData)
    {
        if (this._diffLines && this.loaded)
            this._removeDiffDecorations();

        this._diffLines = diffData;
        if (this.loaded)
            this._updateDiffDecorations();
    },

    addMessage: function(msg)
    {
        this._messages.push(msg);
        if (this.loaded)
            this.addMessageToSource(msg.line - 1, msg);
    },

    clearMessages: function()
    {
        for (var line in this._messageBubbles) {
            var bubble = this._messageBubbles[line];
            bubble.parentNode.removeChild(bubble);
        }

        this._messages = [];
        this._rowMessages = {};
        this._messageBubbles = {};

        this._textViewer.resize();
    },

    get textModel()
    {
        return this._textModel;
    },

    get scrollTop()
    {
        return this.loaded ? this._textViewer.scrollTop : this._scrollTop;
    },

    set scrollTop(scrollTop)
    {
        this._scrollTop = scrollTop;
        if (this.loaded)
            this._textViewer.scrollTop = scrollTop;
    },

    highlightLine: function(line)
    {
        if (this.loaded)
            this._textViewer.highlightLine(line);
        else
            this._lineToHighlight = line;
    },

    _clearLineHighlight: function()
    {
        if (this.loaded)
            this._textViewer.clearLineHighlight();
        else
            delete this._lineToHighlight;
    },

    _saveViewerState: function()
    {
        this._viewerState = {
            textModelContent: this._textModel.text,
            executionLineNumber: this._executionLineNumber,
            messages: this._messages,
            diffLines: this._diffLines,
            breakpoints: this._breakpoints
        };
    },

    _restoreViewerState: function()
    {
        if (!this._viewerState)
            return;
        this._textModel.setText(null, this._viewerState.textModelContent);

        this._messages = this._viewerState.messages;
        this._diffLines = this._viewerState.diffLines;
        this._setTextViewerDecorations();

        if (typeof this._viewerState.executionLineNumber === "number") {
            this.clearExecutionLine();
            this.setExecutionLine(this._viewerState.executionLineNumber);
        }

        var oldBreakpoints = this._breakpoints;
        this._breakpoints = {};
        for (var lineNumber in oldBreakpoints)
            this.removeBreakpoint(Number(lineNumber));

        var newBreakpoints = this._viewerState.breakpoints;
        for (var lineNumber in newBreakpoints) {
            lineNumber = Number(lineNumber);
            var breakpoint = newBreakpoints[lineNumber];
            this.addBreakpoint(lineNumber, breakpoint.resolved, breakpoint.conditional, breakpoint.enabled);
        }

        delete this._viewerState;
    },

    isContentEditable: function()
    {
        return this._delegate.canEditScriptSource();
    },

    readOnlyStateChanged: function(readOnly)
    {
        WebInspector.markBeingEdited(this._textViewer.element, !readOnly);
    },

    startEditing: function()
    {
        if (!this._viewerState) {
            this._saveViewerState();
            this._delegate.setScriptSourceIsBeingEdited(true);
        }

        WebInspector.searchController.cancelSearch();
        this.clearMessages();
    },

    endEditing: function(oldRange, newRange)
    {
        if (!oldRange || !newRange)
            return;

        // Adjust execution line number.
        if (typeof this._executionLineNumber === "number") {
            var newExecutionLineNumber = this._lineNumberAfterEditing(this._executionLineNumber, oldRange, newRange);
            this.clearExecutionLine();
            this.setExecutionLine(newExecutionLineNumber, true);
        }

        // Adjust breakpoints.
        var oldBreakpoints = this._breakpoints;
        this._breakpoints = {};
        for (var lineNumber in oldBreakpoints) {
            lineNumber = Number(lineNumber);
            var breakpoint = oldBreakpoints[lineNumber];
            var newLineNumber = this._lineNumberAfterEditing(lineNumber, oldRange, newRange);
            if (lineNumber === newLineNumber)
                this._breakpoints[lineNumber] = breakpoint;
            else {
                this.removeBreakpoint(lineNumber);
                this.addBreakpoint(newLineNumber, breakpoint.resolved, breakpoint.conditional, breakpoint.enabled);
            }
        }
    },

    _lineNumberAfterEditing: function(lineNumber, oldRange, newRange)
    {
        var shiftOffset = lineNumber <= oldRange.startLine ? 0 : newRange.linesCount - oldRange.linesCount;

        // Special case of editing the line itself. We should decide whether the line number should move below or not.
        if (lineNumber === oldRange.startLine) {
            var whiteSpacesRegex = /^[\s\xA0]*$/;
            for (var i = 0; lineNumber + i <= newRange.endLine; ++i) {
                if (!whiteSpacesRegex.test(this._textModel.line(lineNumber + i))) {
                    shiftOffset = i;
                    break;
                }
            }
        }

        var newLineNumber = Math.max(0, lineNumber + shiftOffset);
        if (oldRange.startLine < lineNumber && lineNumber < oldRange.endLine)
            newLineNumber = oldRange.startLine;
        return newLineNumber;
    },

    _initializeTextViewer: function(mimeType, content)
    {
        this._textViewer.mimeType = mimeType;

        this._content = content;
        this._textModel.setText(null, content);

        var element = this._textViewer.element;
        if (this._delegate.debuggingSupported()) {
            element.addEventListener("mousedown", this._mouseDown.bind(this), true);
            element.addEventListener("mousemove", this._mouseMove.bind(this), true);
            element.addEventListener("scroll", this._scroll.bind(this), true);
        }

        this._textViewer.beginUpdates();

        this._setTextViewerDecorations();

        if (typeof this._executionLineNumber === "number")
            this.setExecutionLine(this._executionLineNumber);

        if (this._lineToHighlight) {
            this.highlightLine(this._lineToHighlight);
            delete this._lineToHighlight;
        }

        if (this._delayedFindSearchMatches) {
            this._delayedFindSearchMatches();
            delete this._delayedFindSearchMatches;
        }

        this.dispatchEventToListeners(WebInspector.SourceFrame.Events.Loaded);

        this._textViewer.endUpdates();

        if (this._parentElement)
            this.show(this._parentElement)
    },

    _setTextViewerDecorations: function()
    {
        this._rowMessages = {};
        this._messageBubbles = {};

        this._textViewer.beginUpdates();

        this._addExistingMessagesToSource();
        this._updateDiffDecorations();

        this._textViewer.resize();

        this._textViewer.endUpdates();
    },

    performSearch: function(query, callback)
    {
        // Call searchCanceled since it will reset everything we need before doing a new search.
        this.searchCanceled();

        function doFindSearchMatches(query)
        {
            this._currentSearchResultIndex = -1;
            this._searchResults = [];

            // First do case-insensitive search.
            var regexObject = createSearchRegex(query);
            this._collectRegexMatches(regexObject, this._searchResults);

            // Then try regex search if user knows the / / hint.
            try {
                if (/^\/.*\/$/.test(query))
                    this._collectRegexMatches(new RegExp(query.substring(1, query.length - 1)), this._searchResults);
            } catch (e) {
                // Silent catch.
            }

            callback(this, this._searchResults.length);
        }

        if (this.loaded)
            doFindSearchMatches.call(this, query);
        else
            this._delayedFindSearchMatches = doFindSearchMatches.bind(this, query);

        this._ensureContentLoaded();
    },

    searchCanceled: function()
    {
        delete this._delayedFindSearchMatches;
        if (!this.loaded)
            return;

        this._currentSearchResultIndex = -1;
        this._searchResults = [];
        this._textViewer.markAndRevealRange(null);
    },

    jumpToFirstSearchResult: function()
    {
        this._jumpToSearchResult(0);
    },

    jumpToLastSearchResult: function()
    {
        this._jumpToSearchResult(this._searchResults.length - 1);
    },

    jumpToNextSearchResult: function()
    {
        this._jumpToSearchResult(this._currentSearchResultIndex + 1);
    },

    jumpToPreviousSearchResult: function()
    {
        this._jumpToSearchResult(this._currentSearchResultIndex - 1);
    },

    showingFirstSearchResult: function()
    {
        return this._searchResults.length &&  this._currentSearchResultIndex === 0;
    },

    showingLastSearchResult: function()
    {
        return this._searchResults.length && this._currentSearchResultIndex === (this._searchResults.length - 1);
    },

    _jumpToSearchResult: function(index)
    {
        if (!this.loaded || !this._searchResults.length)
            return;
        this._currentSearchResultIndex = (index + this._searchResults.length) % this._searchResults.length;
        this._textViewer.markAndRevealRange(this._searchResults[this._currentSearchResultIndex]);
    },

    _collectRegexMatches: function(regexObject, ranges)
    {
        for (var i = 0; i < this._textModel.linesCount; ++i) {
            var line = this._textModel.line(i);
            var offset = 0;
            do {
                var match = regexObject.exec(line);
                if (match) {
                    ranges.push(new WebInspector.TextRange(i, offset + match.index, i, offset + match.index + match[0].length));
                    offset += match.index + 1;
                    line = line.substring(match.index + 1);
                }
            } while (match)
        }
        return ranges;
    },

    _incrementMessageRepeatCount: function(msg, repeatDelta)
    {
        if (!msg._resourceMessageLineElement)
            return;

        if (!msg._resourceMessageRepeatCountElement) {
            var repeatedElement = document.createElement("span");
            msg._resourceMessageLineElement.appendChild(repeatedElement);
            msg._resourceMessageRepeatCountElement = repeatedElement;
        }

        msg.repeatCount += repeatDelta;
        msg._resourceMessageRepeatCountElement.textContent = WebInspector.UIString(" (repeated %d times)", msg.repeatCount);
    },

    setExecutionLine: function(lineNumber, skipRevealLine)
    {
        this._executionLineNumber = lineNumber;
        if (this.loaded) {
            this._textViewer.addDecoration(lineNumber, "webkit-execution-line");
            if (!skipRevealLine)
                this._textViewer.revealLine(lineNumber);
        }
    },

    clearExecutionLine: function()
    {
        if (this.loaded)
            this._textViewer.removeDecoration(this._executionLineNumber, "webkit-execution-line");
        delete this._executionLineNumber;
    },

    _updateDiffDecorations: function()
    {
        if (!this._diffLines)
            return;

        function addDecorations(textViewer, lines, className)
        {
            for (var i = 0; i < lines.length; ++i)
                textViewer.addDecoration(lines[i], className);
        }
        addDecorations(this._textViewer, this._diffLines.added, "webkit-added-line");
        addDecorations(this._textViewer, this._diffLines.removed, "webkit-removed-line");
        addDecorations(this._textViewer, this._diffLines.changed, "webkit-changed-line");
    },

    _removeDiffDecorations: function()
    {
        function removeDecorations(textViewer, lines, className)
        {
            for (var i = 0; i < lines.length; ++i)
                textViewer.removeDecoration(lines[i], className);
        }
        removeDecorations(this._textViewer, this._diffLines.added, "webkit-added-line");
        removeDecorations(this._textViewer, this._diffLines.removed, "webkit-removed-line");
        removeDecorations(this._textViewer, this._diffLines.changed, "webkit-changed-line");
    },

    _addExistingMessagesToSource: function()
    {
        var length = this._messages.length;
        for (var i = 0; i < length; ++i)
            this.addMessageToSource(this._messages[i].line - 1, this._messages[i]);
    },

    addMessageToSource: function(lineNumber, msg)
    {
        if (lineNumber >= this._textModel.linesCount)
            lineNumber = this._textModel.linesCount - 1;
        if (lineNumber < 0)
            lineNumber = 0;

        var messageBubbleElement = this._messageBubbles[lineNumber];
        if (!messageBubbleElement || messageBubbleElement.nodeType !== Node.ELEMENT_NODE || !messageBubbleElement.hasStyleClass("webkit-html-message-bubble")) {
            messageBubbleElement = document.createElement("div");
            messageBubbleElement.className = "webkit-html-message-bubble";
            this._messageBubbles[lineNumber] = messageBubbleElement;
            this._textViewer.addDecoration(lineNumber, messageBubbleElement);
        }

        var rowMessages = this._rowMessages[lineNumber];
        if (!rowMessages) {
            rowMessages = [];
            this._rowMessages[lineNumber] = rowMessages;
        }

        for (var i = 0; i < rowMessages.length; ++i) {
            if (rowMessages[i].isEqual(msg)) {
                this._incrementMessageRepeatCount(rowMessages[i], msg.repeatDelta);
                return;
            }
        }

        rowMessages.push(msg);

        var imageURL;
        switch (msg.level) {
            case WebInspector.ConsoleMessage.MessageLevel.Error:
                messageBubbleElement.addStyleClass("webkit-html-error-message");
                imageURL = "Images/errorIcon.png";
                break;
            case WebInspector.ConsoleMessage.MessageLevel.Warning:
                messageBubbleElement.addStyleClass("webkit-html-warning-message");
                imageURL = "Images/warningIcon.png";
                break;
        }

        var messageLineElement = document.createElement("div");
        messageLineElement.className = "webkit-html-message-line";
        messageBubbleElement.appendChild(messageLineElement);

        // Create the image element in the Inspector's document so we can use relative image URLs.
        var image = document.createElement("img");
        image.src = imageURL;
        image.className = "webkit-html-message-icon";
        messageLineElement.appendChild(image);
        messageLineElement.appendChild(document.createTextNode(msg.message));

        msg._resourceMessageLineElement = messageLineElement;
    },

    addBreakpoint: function(lineNumber, resolved, conditional, enabled)
    {
        this._breakpoints[lineNumber] = {
            resolved: resolved,
            conditional: conditional,
            enabled: enabled
        };
        this._textViewer.beginUpdates();
        this._textViewer.addDecoration(lineNumber, "webkit-breakpoint");
        if (!enabled)
            this._textViewer.addDecoration(lineNumber, "webkit-breakpoint-disabled");
        if (conditional)
            this._textViewer.addDecoration(lineNumber, "webkit-breakpoint-conditional");
        this._textViewer.endUpdates();
    },

    removeBreakpoint: function(lineNumber)
    {
        delete this._breakpoints[lineNumber];
        this._textViewer.beginUpdates();
        this._textViewer.removeDecoration(lineNumber, "webkit-breakpoint");
        this._textViewer.removeDecoration(lineNumber, "webkit-breakpoint-disabled");
        this._textViewer.removeDecoration(lineNumber, "webkit-breakpoint-conditional");
        this._textViewer.endUpdates();
    },

    populateLineGutterContextMenu: function(lineNumber, contextMenu)
    {
        contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Continue to here" : "Continue to Here"), this._delegate.continueToLine.bind(this._delegate, lineNumber));

        var breakpoint = this._delegate.findBreakpoint(lineNumber);
        if (!breakpoint) {
            // This row doesn't have a breakpoint: We want to show Add Breakpoint and Add and Edit Breakpoint.
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Add breakpoint" : "Add Breakpoint"), this._delegate.setBreakpoint.bind(this._delegate, lineNumber, "", true));

            function addConditionalBreakpoint()
            {
                this.addBreakpoint(lineNumber, true, true, true);
                function didEditBreakpointCondition(committed, condition)
                {
                    this.removeBreakpoint(lineNumber);
                    if (committed)
                        this._delegate.setBreakpoint(lineNumber, condition, true);
                }
                this._editBreakpointCondition(lineNumber, "", didEditBreakpointCondition.bind(this));
            }
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Add conditional breakpoint…" : "Add Conditional Breakpoint…"), addConditionalBreakpoint.bind(this));
        } else {
            // This row has a breakpoint, we want to show edit and remove breakpoint, and either disable or enable.
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Remove breakpoint" : "Remove Breakpoint"), this._delegate.removeBreakpoint.bind(this._delegate, lineNumber));
            function editBreakpointCondition()
            {
                function didEditBreakpointCondition(committed, condition)
                {
                    if (committed)
                        this._delegate.updateBreakpoint(lineNumber, condition, breakpoint.enabled);
                }
                this._editBreakpointCondition(lineNumber, breakpoint.condition, didEditBreakpointCondition.bind(this));
            }
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Edit breakpoint…" : "Edit Breakpoint…"), editBreakpointCondition.bind(this));
            function setBreakpointEnabled(enabled)
            {
                this._delegate.updateBreakpoint(lineNumber, breakpoint.condition, enabled);
            }
            if (breakpoint.enabled)
                contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Disable breakpoint" : "Disable Breakpoint"), setBreakpointEnabled.bind(this, false));
            else
                contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Enable breakpoint" : "Enable Breakpoint"), setBreakpointEnabled.bind(this, true));
        }
    },

    populateTextAreaContextMenu: function(contextMenu)
    {
    },

    suggestedFileName: function()
    {
        return this._delegate.suggestedFileName();
    },

    _scroll: function(event)
    {
        this._hidePopup();
    },

    _mouseDown: function(event)
    {
        this._resetHoverTimer();
        this._hidePopup();
        if (event.button != 0 || event.altKey || event.ctrlKey || event.metaKey)
            return;
        var target = event.target.enclosingNodeOrSelfWithClass("webkit-line-number");
        if (!target)
            return;
        var lineNumber = target.lineNumber;

        var breakpoint = this._delegate.findBreakpoint(lineNumber);
        if (breakpoint) {
            if (event.shiftKey)
                this._delegate.updateBreakpoint(lineNumber, breakpoint.condition, !breakpoint.enabled);
            else
                this._delegate.removeBreakpoint(lineNumber);
        } else
            this._delegate.setBreakpoint(lineNumber, "", true);
        event.preventDefault();
    },

    _mouseMove: function(event)
    {
        // Pretend that nothing has happened.
        if (this._hoverElement === event.target || event.target.hasStyleClass("source-frame-eval-expression"))
            return;

        this._resetHoverTimer();
        // User has 500ms to reach the popup.
        if (this._popup) {
            var self = this;
            function doHide()
            {
                self._hidePopup();
                delete self._hidePopupTimer;
            }
            if (!("_hidePopupTimer" in this))
                this._hidePopupTimer = setTimeout(doHide, 500);
        }

        this._hoverElement = event.target;

        // Now that cleanup routines are set up above, leave this in case we are not on a break.
        if (!this._delegate.debuggerPaused())
            return;

        // We are interested in identifiers and "this" keyword.
        if (this._hoverElement.hasStyleClass("webkit-javascript-keyword")) {
            if (this._hoverElement.textContent !== "this")
                return;
        } else if (!this._hoverElement.hasStyleClass("webkit-javascript-ident"))
            return;

        const toolTipDelay = this._popup ? 600 : 1000;
        this._hoverTimer = setTimeout(this._mouseHover.bind(this, this._hoverElement), toolTipDelay);
    },

    _resetHoverTimer: function()
    {
        if (this._hoverTimer) {
            clearTimeout(this._hoverTimer);
            delete this._hoverTimer;
        }
    },

    _hidePopup: function()
    {
        if (!this._popup)
            return;

        // Replace higlight element with its contents inplace.
        var parentElement = this._popup.highlightElement.parentElement;
        var child = this._popup.highlightElement.firstChild;
        while (child) {
            var nextSibling = child.nextSibling;
            parentElement.insertBefore(child, this._popup.highlightElement);
            child = nextSibling;
        }
        parentElement.removeChild(this._popup.highlightElement);

        this._popup.hide();
        delete this._popup;
        this._delegate.releaseEvaluationResult();
    },

    _mouseHover: function(element)
    {
        delete this._hoverTimer;

        var lineRow = element.enclosingNodeOrSelfWithClass("webkit-line-content");
        if (!lineRow)
            return;

        // Collect tokens belonging to evaluated exression.
        var tokens = [ element ];
        var token = element.previousSibling;
        while (token && (token.className === "webkit-javascript-ident" || token.className === "webkit-javascript-keyword" || token.textContent.trim() === ".")) {
            tokens.push(token);
            token = token.previousSibling;
        }
        tokens.reverse();

        // Wrap them with highlight element.
        var parentElement = element.parentElement;
        var nextElement = element.nextSibling;
        var container = document.createElement("span");
        for (var i = 0; i < tokens.length; ++i)
            container.appendChild(tokens[i]);
        parentElement.insertBefore(container, nextElement);
        this._showPopup(container);
    },

    _showPopup: function(element)
    {
        if (!this._delegate.debuggerPaused())
            return;

        function killHidePopupTimer()
        {
            if (this._hidePopupTimer) {
                clearTimeout(this._hidePopupTimer);
                delete this._hidePopupTimer;

                // We know that we reached the popup, but we might have moved over other elements.
                // Discard pending command.
                this._resetHoverTimer();
            }
        }

        function showObjectPopup(result, wasThrown)
        {
            if (wasThrown || !this._delegate.debuggerPaused())
                return;

            var popupContentElement = null;
            if (result.type !== "object" && result.type !== "node" && result.type !== "array") {
                popupContentElement = document.createElement("span");
                popupContentElement.className = "monospace console-formatted-" + result.type;
                popupContentElement.style.whiteSpace = "pre";
                popupContentElement.textContent = result.description;
                if (result.type === "string")
                    popupContentElement.textContent = "\"" + popupContentElement.textContent + "\"";
                this._popup = new WebInspector.Popover(popupContentElement);
                this._popup.show(element);
            } else {
                var popupContentElement = document.createElement("div");

                var titleElement = document.createElement("div");
                titleElement.className = "source-frame-popover-title monospace";
                titleElement.textContent = result.description;
                popupContentElement.appendChild(titleElement);

                var section = new WebInspector.ObjectPropertiesSection(result);
                section.expanded = true;
                section.element.addStyleClass("source-frame-popover-tree");
                section.headerElement.addStyleClass("hidden");
                popupContentElement.appendChild(section.element);

                this._popup = new WebInspector.Popover(popupContentElement);
                const popupWidth = 300;
                const popupHeight = 250;
                this._popup.show(element, popupWidth, popupHeight);
            }
            this._popup.highlightElement = element;
            this._popup.highlightElement.addStyleClass("source-frame-eval-expression");
            popupContentElement.addEventListener("mousemove", killHidePopupTimer.bind(this), true);
        }

        this._delegate.evaluateInSelectedCallFrame(element.textContent, showObjectPopup.bind(this));
    },

    _editBreakpointCondition: function(lineNumber, condition, callback)
    {
        this._conditionElement = this._createConditionElement(lineNumber);
        this._textViewer.addDecoration(lineNumber, this._conditionElement);

        function finishEditing(committed, element, newText)
        {
            this._textViewer.removeDecoration(lineNumber, this._conditionElement);
            delete this._conditionEditorElement;
            delete this._conditionElement;
            callback(committed, newText);
        }

        WebInspector.startEditing(this._conditionEditorElement, {
            context: null,
            commitHandler: finishEditing.bind(this, true),
            cancelHandler: finishEditing.bind(this, false)
        });
        this._conditionEditorElement.value = condition;
        this._conditionEditorElement.select();
    },

    _createConditionElement: function(lineNumber)
    {
        var conditionElement = document.createElement("div");
        conditionElement.className = "source-frame-breakpoint-condition";

        var labelElement = document.createElement("label");
        labelElement.className = "source-frame-breakpoint-message";
        labelElement.htmlFor = "source-frame-breakpoint-condition";
        labelElement.appendChild(document.createTextNode(WebInspector.UIString("The breakpoint on line %d will stop only if this expression is true:", lineNumber)));
        conditionElement.appendChild(labelElement);

        var editorElement = document.createElement("input");
        editorElement.id = "source-frame-breakpoint-condition";
        editorElement.className = "monospace";
        editorElement.type = "text";
        conditionElement.appendChild(editorElement);
        this._conditionEditorElement = editorElement;

        return conditionElement;
    },

    resize: function()
    {
        this._textViewer.resize();
    },

    commitEditing: function(callback)
    {
        if (!this._viewerState) {
            // No editing was actually done.
            this._delegate.setScriptSourceIsBeingEdited(false);
            callback();
            return;
        }

        function didEditContent(error)
        {
            if (error) {
                if (error.data && error.data[0]) {
                    WebInspector.log(error.data[0], WebInspector.ConsoleMessage.MessageLevel.Error);
                    WebInspector.showConsole();
                }
                callback(error);
                return;
            }

            var newBreakpoints = {};
            for (var lineNumber in this._breakpoints) {
                newBreakpoints[lineNumber] = this._breakpoints[lineNumber];
                this.removeBreakpoint(Number(lineNumber));
            }

            for (var lineNumber in this._viewerState.breakpoints)
                this._delegate.removeBreakpoint(Number(lineNumber));

            for (var lineNumber in newBreakpoints) {
                var breakpoint = newBreakpoints[lineNumber];
                this._delegate.setBreakpoint(Number(lineNumber), breakpoint.condition, breakpoint.enabled);
            }

            delete this._viewerState;
            this._delegate.setScriptSourceIsBeingEdited(false);

            callback();
        }
        this.editContent(this._textModel.text, didEditContent.bind(this));
    },

    editContent: function(newContent, callback)
    {
        this._delegate.editScriptSource(newContent, callback);
    },

    cancelEditing: function()
    {
        this._restoreViewerState();
        this._delegate.setScriptSourceIsBeingEdited(false);
    }
}

WebInspector.SourceFrame.prototype.__proto__ = WebInspector.TextViewerDelegate.prototype;


WebInspector.SourceFrameDelegate = function()
{
}

WebInspector.SourceFrameDelegate.prototype = {
    requestContent: function(callback)
    {
        // Should be implemented by subclasses.
    },

    debuggingSupported: function()
    {
        return false;
    },

    setBreakpoint: function(lineNumber, condition, enabled)
    {
        // Should be implemented by subclasses.
    },

    removeBreakpoint: function(lineNumber)
    {
        // Should be implemented by subclasses.
    },

    updateBreakpoint: function(lineNumber, condition, enabled)
    {
        // Should be implemented by subclasses.
    },

    findBreakpoint: function(lineNumber)
    {
        // Should be implemented by subclasses.
    },

    continueToLine: function(lineNumber)
    {
        // Should be implemented by subclasses.
    },

    canEditScriptSource: function()
    {
        return false;
    },

    editScriptSource: function(text, callback)
    {
        // Should be implemented by subclasses.
    },

    setScriptSourceIsBeingEdited: function(inEditMode)
    {
        // Should be implemented by subclasses.
    },

    debuggerPaused: function()
    {
        // Should be implemented by subclasses.
    },

    evaluateInSelectedCallFrame: function(string)
    {
        // Should be implemented by subclasses.
    },

    releaseEvaluationResult: function()
    {
        // Should be implemented by subclasses.
    },

    suggestedFileName: function()
    {
        // Should be implemented by subclasses.
    }
}
