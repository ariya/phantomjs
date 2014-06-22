/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * @extends {WebInspector.View}
 * @constructor
 * @param {WebInspector.ContentProvider} contentProvider
 */
WebInspector.SourceFrame = function(contentProvider)
{
    WebInspector.View.call(this);
    this.element.addStyleClass("script-view");
    this.element.addStyleClass("fill");

    this._url = contentProvider.contentURL();
    this._contentProvider = contentProvider;

    var textEditorDelegate = new WebInspector.TextEditorDelegateForSourceFrame(this);

    if (WebInspector.experimentsSettings.codemirror.isEnabled()) {
        loadScript("CodeMirrorTextEditor.js");
        this._textEditor = new WebInspector.CodeMirrorTextEditor(this._url, textEditorDelegate);
    } else if (WebInspector.experimentsSettings.aceTextEditor.isEnabled()) {
        loadScript("AceTextEditor.js");
        this._textEditor = new WebInspector.AceTextEditor(this._url, textEditorDelegate);
    } else
        this._textEditor = new WebInspector.DefaultTextEditor(this._url, textEditorDelegate);

    this._currentSearchResultIndex = -1;
    this._searchResults = [];

    this._messages = [];
    this._rowMessages = {};
    this._messageBubbles = {};

    this._textEditor.setReadOnly(!this.canEditSource());

    this._shortcuts = {};
    this._shortcuts[WebInspector.KeyboardShortcut.makeKey("s", WebInspector.KeyboardShortcut.Modifiers.CtrlOrMeta)] = this._commitEditing.bind(this);
    this.element.addEventListener("keydown", this._handleKeyDown.bind(this), false);

    this._sourcePositionElement = document.createElement("div");
    this._sourcePositionElement.className = "source-frame-cursor-position";
}

/**
 * @param {string} query
 * @param {string=} modifiers
 */
WebInspector.SourceFrame.createSearchRegex = function(query, modifiers)
{
    var regex;
    modifiers = modifiers || "";

    // First try creating regex if user knows the / / hint.
    try {
        if (/^\/.*\/$/.test(query))
            regex = new RegExp(query.substring(1, query.length - 1), modifiers);
    } catch (e) {
        // Silent catch.
    }

    // Otherwise just do case-insensitive search.
    if (!regex)
        regex = createPlainTextSearchRegex(query, "i" + modifiers);

    return regex;
}

WebInspector.SourceFrame.Events = {
    ScrollChanged: "ScrollChanged",
    SelectionChanged: "SelectionChanged"
}

WebInspector.SourceFrame.prototype = {
    wasShown: function()
    {
        this._ensureContentLoaded();
        this._textEditor.show(this.element);
        this._editorAttached = true;
        this._wasShownOrLoaded();
    },

    /**
     * @return {boolean}
     */
    _isEditorShowing: function()
    {
        return this.isShowing() && this._editorAttached;
    },

    willHide: function()
    {
        WebInspector.View.prototype.willHide.call(this);

        this._clearLineHighlight();
        this._clearLineToReveal();
    },

    /**
     * @return {?Element}
     */
    statusBarText: function()
    {
        return this._sourcePositionElement;
    },

    /**
     * @return {Array.<Element>}
     */
    statusBarItems: function()
    {
        return [];
    },

    defaultFocusedElement: function()
    {
        return this._textEditor.defaultFocusedElement();
    },

    get loaded()
    {
        return this._loaded;
    },

    hasContent: function()
    {
        return true;
    },

    get textEditor()
    {
        return this._textEditor;
    },

    _ensureContentLoaded: function()
    {
        if (!this._contentRequested) {
            this._contentRequested = true;
            this._contentProvider.requestContent(this.setContent.bind(this));
        }
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
            var lineNumber = parseInt(line, 10);
            this._textEditor.removeDecoration(lineNumber, bubble);
        }

        this._messages = [];
        this._rowMessages = {};
        this._messageBubbles = {};
    },

    /**
     * @param {number} line
     */
    canHighlightLine: function(line)
    {
        return true;
    },

    /**
     * @param {number} line
     */
    highlightLine: function(line)
    {
        this._clearLineToReveal();
        this._clearLineToScrollTo();
        this._lineToHighlight = line;
        this._innerHighlightLineIfNeeded();
        this._textEditor.setSelection(WebInspector.TextRange.createFromLocation(line, 0));
    },

    _innerHighlightLineIfNeeded: function()
    {
        if (typeof this._lineToHighlight === "number") {
            if (this.loaded && this._isEditorShowing()) {
                this._textEditor.highlightLine(this._lineToHighlight);
                delete this._lineToHighlight
            }
        }
    },

    _clearLineHighlight: function()
    {
        this._textEditor.clearLineHighlight();
        delete this._lineToHighlight;
    },

    /**
     * @param {number} line
     */
    revealLine: function(line)
    {
        this._clearLineHighlight();
        this._clearLineToScrollTo();
        this._lineToReveal = line;
        this._innerRevealLineIfNeeded();
    },

    _innerRevealLineIfNeeded: function()
    {
        if (typeof this._lineToReveal === "number") {
            if (this.loaded && this._isEditorShowing()) {
                this._textEditor.revealLine(this._lineToReveal);
                delete this._lineToReveal
            }
        }
    },

    _clearLineToReveal: function()
    {
        delete this._lineToReveal;
    },

    /**
     * @param {number} line
     */
    scrollToLine: function(line)
    {
        this._clearLineHighlight();
        this._clearLineToReveal();
        this._lineToScrollTo = line;
        this._innerScrollToLineIfNeeded();
    },

    _innerScrollToLineIfNeeded: function()
    {
        if (typeof this._lineToScrollTo === "number") {
            if (this.loaded && this._isEditorShowing()) {
                this._textEditor.scrollToLine(this._lineToScrollTo);
                delete this._lineToScrollTo;
            }
        }
    },

    _clearLineToScrollTo: function()
    {
        delete this._lineToScrollTo;
    },

    /**
     * @param {WebInspector.TextRange} textRange
     */
    setSelection: function(textRange)
    {
        this._selectionToSet = textRange;
        this._innerSetSelectionIfNeeded();
    },

    _innerSetSelectionIfNeeded: function()
    {
        if (this._selectionToSet && this.loaded && this._isEditorShowing()) {
            this._textEditor.setSelection(this._selectionToSet);
            delete this._selectionToSet;
        }
    },

    _wasShownOrLoaded: function()
    {
        this._innerHighlightLineIfNeeded();
        this._innerRevealLineIfNeeded();
        this._innerScrollToLineIfNeeded();
        this._innerSetSelectionIfNeeded();
    },

    onTextChanged: function(oldRange, newRange)
    {
        if (!this._isReplacing)
            WebInspector.searchController.cancelSearch();
        this.clearMessages();
    },

    /**
     * @param {?string} content
     * @param {boolean} contentEncoded
     * @param {string} mimeType
     */
    setContent: function(content, contentEncoded, mimeType)
    {
        this._textEditor.mimeType = mimeType;

        if (!this._loaded) {
            this._loaded = true;
            this._textEditor.setText(content || "");
        } else
            this._textEditor.editRange(this._textEditor.range(), content || "");

        this._textEditor.beginUpdates();

        this._setTextEditorDecorations();

        this._wasShownOrLoaded();

        if (this._delayedFindSearchMatches) {
            this._delayedFindSearchMatches();
            delete this._delayedFindSearchMatches;
        }

        this.onTextEditorContentLoaded();

        this._textEditor.endUpdates();
    },

    onTextEditorContentLoaded: function() {},

    _setTextEditorDecorations: function()
    {
        this._rowMessages = {};
        this._messageBubbles = {};

        this._textEditor.beginUpdates();

        this._addExistingMessagesToSource();

        this._textEditor.endUpdates();
    },

    /**
     * @param {string} query
     * @param {function(WebInspector.View, number)} callback
     */
    performSearch: function(query, callback)
    {
        // Call searchCanceled since it will reset everything we need before doing a new search.
        this.searchCanceled();

        function doFindSearchMatches(query)
        {
            this._currentSearchResultIndex = -1;
            this._searchResults = [];

            var regex = WebInspector.SourceFrame.createSearchRegex(query);
            this._searchResults = this._collectRegexMatches(regex);
            var shiftToIndex = 0;
            var selection = this._textEditor.lastSelection();
            for (var i = 0; selection && i < this._searchResults.length; ++i) {
                if (this._searchResults[i].compareTo(selection) >= 0) {
                    shiftToIndex = i;
                    break;
                }
            }

            if (shiftToIndex)
                this._searchResults = this._searchResults.rotate(shiftToIndex);

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
        this._textEditor.markAndRevealRange(null);
    },

    hasSearchResults: function()
    {
        return this._searchResults.length > 0;
    },

    jumpToFirstSearchResult: function()
    {
        this.jumpToSearchResult(0);
    },

    jumpToLastSearchResult: function()
    {
        this.jumpToSearchResult(this._searchResults.length - 1);
    },

    jumpToNextSearchResult: function()
    {
        this.jumpToSearchResult(this._currentSearchResultIndex + 1);
    },

    jumpToPreviousSearchResult: function()
    {
        this.jumpToSearchResult(this._currentSearchResultIndex - 1);
    },

    showingFirstSearchResult: function()
    {
        return this._searchResults.length &&  this._currentSearchResultIndex === 0;
    },

    showingLastSearchResult: function()
    {
        return this._searchResults.length && this._currentSearchResultIndex === (this._searchResults.length - 1);
    },

    get currentSearchResultIndex()
    {
        return this._currentSearchResultIndex;
    },

    jumpToSearchResult: function(index)
    {
        if (!this.loaded || !this._searchResults.length)
            return;
        this._currentSearchResultIndex = (index + this._searchResults.length) % this._searchResults.length;
        this._textEditor.markAndRevealRange(this._searchResults[this._currentSearchResultIndex]);
    },

    /**
     * @param {string} text
     */
    replaceSearchMatchWith: function(text)
    {
        var range = this._searchResults[this._currentSearchResultIndex];
        if (!range)
            return;
        this._textEditor.markAndRevealRange(null);

        this._isReplacing = true;
        var newRange = this._textEditor.editRange(range, text);
        delete this._isReplacing;

        this._textEditor.setSelection(newRange.collapseToEnd());
    },

    /**
     * @param {string} query
     * @param {string} replacement
     */
    replaceAllWith: function(query, replacement)
    {
        this._textEditor.markAndRevealRange(null);

        var text = this._textEditor.text();
        var range = this._textEditor.range();
        text = text.replace(WebInspector.SourceFrame.createSearchRegex(query, "g"), replacement);

        this._isReplacing = true;
        this._textEditor.editRange(range, text);
        delete this._isReplacing;
    },

    _collectRegexMatches: function(regexObject)
    {
        var ranges = [];
        for (var i = 0; i < this._textEditor.linesCount; ++i) {
            var line = this._textEditor.line(i);
            var offset = 0;
            do {
                var match = regexObject.exec(line);
                if (match) {
                    if (match[0].length)
                        ranges.push(new WebInspector.TextRange(i, offset + match.index, i, offset + match.index + match[0].length));
                    offset += match.index + 1;
                    line = line.substring(match.index + 1);
                }
            } while (match && line);
        }
        return ranges;
    },

    _addExistingMessagesToSource: function()
    {
        var length = this._messages.length;
        for (var i = 0; i < length; ++i)
            this.addMessageToSource(this._messages[i].line - 1, this._messages[i]);
    },

    /**
     * @param {number} lineNumber
     * @param {WebInspector.ConsoleMessage} msg
     */
    addMessageToSource: function(lineNumber, msg)
    {
        if (lineNumber >= this._textEditor.linesCount)
            lineNumber = this._textEditor.linesCount - 1;
        if (lineNumber < 0)
            lineNumber = 0;

        var rowMessages = this._rowMessages[lineNumber];
        if (!rowMessages) {
            rowMessages = [];
            this._rowMessages[lineNumber] = rowMessages;
        }

        for (var i = 0; i < rowMessages.length; ++i) {
            if (rowMessages[i].consoleMessage.isEqual(msg)) {
                rowMessages[i].repeatCount = msg.totalRepeatCount;
                this._updateMessageRepeatCount(rowMessages[i]);
                return;
            }
        }

        var rowMessage = { consoleMessage: msg };
        rowMessages.push(rowMessage);

        this._textEditor.beginUpdates();
        var messageBubbleElement = this._messageBubbles[lineNumber];
        if (!messageBubbleElement) {
            messageBubbleElement = document.createElement("div");
            messageBubbleElement.className = "webkit-html-message-bubble";
            this._messageBubbles[lineNumber] = messageBubbleElement;
            this._textEditor.addDecoration(lineNumber, messageBubbleElement);
        }

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

        rowMessage.element = messageLineElement;
        rowMessage.repeatCount = msg.totalRepeatCount;
        this._updateMessageRepeatCount(rowMessage);
        this._textEditor.endUpdates();
    },

    _updateMessageRepeatCount: function(rowMessage)
    {
        if (rowMessage.repeatCount < 2)
            return;

        if (!rowMessage.repeatCountElement) {
            var repeatCountElement = document.createElement("span");
            rowMessage.element.appendChild(repeatCountElement);
            rowMessage.repeatCountElement = repeatCountElement;
        }

        rowMessage.repeatCountElement.textContent = WebInspector.UIString(" (repeated %d times)", rowMessage.repeatCount);
    },

    /**
     * @param {number} lineNumber
     * @param {WebInspector.ConsoleMessage} msg
     */
    removeMessageFromSource: function(lineNumber, msg)
    {
        if (lineNumber >= this._textEditor.linesCount)
            lineNumber = this._textEditor.linesCount - 1;
        if (lineNumber < 0)
            lineNumber = 0;

        var rowMessages = this._rowMessages[lineNumber];
        for (var i = 0; rowMessages && i < rowMessages.length; ++i) {
            var rowMessage = rowMessages[i];
            if (rowMessage.consoleMessage !== msg)
                continue;

            var messageLineElement = rowMessage.element;
            var messageBubbleElement = messageLineElement.parentElement;
            messageBubbleElement.removeChild(messageLineElement);
            rowMessages.remove(rowMessage);
            if (!rowMessages.length)
                delete this._rowMessages[lineNumber];
            if (!messageBubbleElement.childElementCount) {
                this._textEditor.removeDecoration(lineNumber, messageBubbleElement);
                delete this._messageBubbles[lineNumber];
            }
            break;
        }
    },

    populateLineGutterContextMenu: function(contextMenu, lineNumber)
    {
    },

    populateTextAreaContextMenu: function(contextMenu, lineNumber)
    {
    },

    inheritScrollPositions: function(sourceFrame)
    {
        this._textEditor.inheritScrollPositions(sourceFrame._textEditor);
    },

    /**
     * @return {boolean}
     */
    canEditSource: function()
    {
        return false;
    },

    /**
     * @param {string} text 
     */
    commitEditing: function(text)
    {
    },

    /**
     * @param {WebInspector.TextRange} textRange
     */
    selectionChanged: function(textRange)
    {
        this._updateSourcePosition(textRange);
        this.dispatchEventToListeners(WebInspector.SourceFrame.Events.SelectionChanged, textRange);
    },

    /**
     * @param {WebInspector.TextRange} textRange
     */
    _updateSourcePosition: function(textRange)
    {
        if (!textRange)
            return;

        if (textRange.isEmpty()) {
            this._sourcePositionElement.textContent = WebInspector.UIString("Line %d, Column %d", textRange.endLine + 1, textRange.endColumn + 1);
            return;
        }
        textRange = textRange.normalize();

        var selectedText = this._textEditor.copyRange(textRange);
        if (textRange.startLine === textRange.endLine)
            this._sourcePositionElement.textContent = WebInspector.UIString("%d characters selected", selectedText.length);
        else
            this._sourcePositionElement.textContent = WebInspector.UIString("%d lines, %d characters selected", textRange.endLine - textRange.startLine + 1, selectedText.length);
    },

    /**
     * @param {number} lineNumber
     */
    scrollChanged: function(lineNumber)
    {
        this.dispatchEventToListeners(WebInspector.SourceFrame.Events.ScrollChanged, lineNumber);
    },

    _handleKeyDown: function(e)
    {
        var shortcutKey = WebInspector.KeyboardShortcut.makeKeyFromEvent(e);
        var handler = this._shortcuts[shortcutKey];
        if (handler && handler())
            e.consume(true);
    },

    _commitEditing: function()
    {
        if (this._textEditor.readOnly())
            return false;

        var content = this._textEditor.text();
        this.commitEditing(content);
        return true;
    },

    __proto__: WebInspector.View.prototype
}


/**
 * @implements {WebInspector.TextEditorDelegate}
 * @constructor
 */
WebInspector.TextEditorDelegateForSourceFrame = function(sourceFrame)
{
    this._sourceFrame = sourceFrame;
}

WebInspector.TextEditorDelegateForSourceFrame.prototype = {
    onTextChanged: function(oldRange, newRange)
    {
        this._sourceFrame.onTextChanged(oldRange, newRange);
    },

    /**
     * @param {WebInspector.TextRange} textRange
     */
    selectionChanged: function(textRange)
    {
        this._sourceFrame.selectionChanged(textRange);
    },

    /**
     * @param {number} lineNumber
     */
    scrollChanged: function(lineNumber)
    {
        this._sourceFrame.scrollChanged(lineNumber);
    },

    populateLineGutterContextMenu: function(contextMenu, lineNumber)
    {
        this._sourceFrame.populateLineGutterContextMenu(contextMenu, lineNumber);
    },

    populateTextAreaContextMenu: function(contextMenu, lineNumber)
    {
        this._sourceFrame.populateTextAreaContextMenu(contextMenu, lineNumber);
    },

    /**
     * @param {string} hrefValue
     * @param {boolean} isExternal
     * @return {Element}
     */
    createLink: function(hrefValue, isExternal)
    {
        var targetLocation = WebInspector.ParsedURL.completeURL(this._sourceFrame._url, hrefValue);
        return WebInspector.linkifyURLAsNode(targetLocation || hrefValue, hrefValue, undefined, isExternal);
    },

    __proto__: WebInspector.TextEditorDelegate.prototype
}
