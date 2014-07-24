/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * @extends {WebInspector.View}
 * @implements {WebInspector.TextEditor}
 * @param {?string} url
 * @param {WebInspector.TextEditorDelegate} delegate
 */
WebInspector.DefaultTextEditor = function(url, delegate)
{
    WebInspector.View.call(this);
    this._delegate = delegate;
    this._url = url;

    this.registerRequiredCSS("textEditor.css");

    this.element.className = "text-editor monospace";
    this.markAsLayoutBoundary();

    // Prevent middle-click pasting in the editor unless it is explicitly enabled for certain component.
    this.element.addEventListener("mouseup", preventDefaultOnMouseUp.bind(this), false);
    function preventDefaultOnMouseUp(event)
    {
        if (event.button === 1)
            event.consume(true);
    }

    this._textModel = new WebInspector.TextEditorModel();
    this._textModel.addEventListener(WebInspector.TextEditorModel.Events.TextChanged, this._textChanged, this);

    var syncScrollListener = this._syncScroll.bind(this);
    var syncDecorationsForLineListener = this._syncDecorationsForLine.bind(this);
    var syncLineHeightListener = this._syncLineHeight.bind(this);
    this._mainPanel = new WebInspector.TextEditorMainPanel(this._delegate, this._textModel, url, syncScrollListener, syncDecorationsForLineListener);
    this._gutterPanel = new WebInspector.TextEditorGutterPanel(this._textModel, syncDecorationsForLineListener, syncLineHeightListener);

    this._mainPanel.element.addEventListener("scroll", this._handleScrollChanged.bind(this), false);

    this._gutterPanel.element.addEventListener("mousedown", this._onMouseDown.bind(this), true);

    // Explicitly enable middle-click pasting in the editor main panel.
    this._mainPanel.element.addEventListener("mouseup", consumeMouseUp.bind(this), false);
    function consumeMouseUp(event)
    {
        if (event.button === 1)
            event.consume(false);
    }

    this.element.appendChild(this._mainPanel.element);
    this.element.appendChild(this._gutterPanel.element);

    // Forward mouse wheel events from the unscrollable gutter to the main panel.
    function forwardWheelEvent(event)
    {
        var clone = document.createEvent("WheelEvent");
        clone.initWebKitWheelEvent(event.wheelDeltaX, event.wheelDeltaY,
                                   event.view,
                                   event.screenX, event.screenY,
                                   event.clientX, event.clientY,
                                   event.ctrlKey, event.altKey, event.shiftKey, event.metaKey);
        this._mainPanel.element.dispatchEvent(clone);
    }
    this._gutterPanel.element.addEventListener("mousewheel", forwardWheelEvent.bind(this), false);

    this.element.addEventListener("keydown", this._handleKeyDown.bind(this), false);
    this.element.addEventListener("contextmenu", this._contextMenu.bind(this), true);

    this._wordMovementController = new WebInspector.DefaultTextEditor.WordMovementController(this, this._textModel);
    this._registerShortcuts();
}

/**
 * @constructor
 * @param {WebInspector.TextRange} range
 * @param {string} text
 */
WebInspector.DefaultTextEditor.EditInfo = function(range, text)
{
    this.range = range;
    this.text = text;
}

WebInspector.DefaultTextEditor.prototype = {
    /**
     * @return {boolean}
     */
    isClean: function()
    {
        return this._textModel.isClean();
    },

    markClean: function()
    {
        this._textModel.markClean();
    },
    /**
     * @param {number} lineNumber
     * @param {number} column
     * @return {?{startColumn: number, endColumn: number, type: string}}
     */
    tokenAtTextPosition: function(lineNumber, column)
    {
        return this._mainPanel.tokenAtTextPosition(lineNumber, column);
    },

    /*
     * @param {number} lineNumber
     * @param {number} column
     * @return {?{x: number, y: number, height: number}}
     */
    cursorPositionToCoordinates: function(lineNumber, column)
    {
        return this._mainPanel.cursorPositionToCoordinates(lineNumber, column);
    },

    /**
     * @param {number} x
     * @param {number} y
     * @return {?WebInspector.TextRange}
     */
    coordinatesToCursorPosition: function(x, y)
    {
        return this._mainPanel.coordinatesToCursorPosition(x, y);
    },

    /**
     * @param {WebInspector.TextRange} range
     * @return {string}
     */
    copyRange: function(range)
    {
        return this._textModel.copyRange(range);
    },

    /**
     * @param {string} regex
     * @param {string} cssClass
     * @return {Object}
     */
    highlightRegex: function(regex, cssClass)
    {
        return this._mainPanel.highlightRegex(regex, cssClass);
    },

    /**
     * @param {Object} highlightDescriptor
     */
    removeHighlight: function(highlightDescriptor)
    {
        this._mainPanel.removeHighlight(highlightDescriptor);
    },

    /**
     * @param {WebInspector.TextRange} range
     * @param {string} cssClass
     * @return {Object}
     */
    highlightRange: function(range, cssClass)
    {
        return this._mainPanel.highlightRange(range, cssClass);
    },

    /**
     * @param {string} mimeType
     */
    set mimeType(mimeType)
    {
        this._mainPanel.mimeType = mimeType;
    },

    /**
     * @param {boolean} readOnly
     */
    setReadOnly: function(readOnly)
    {
        if (this._mainPanel.readOnly() === readOnly)
            return;
        this._mainPanel.setReadOnly(readOnly, this.isShowing());
        WebInspector.markBeingEdited(this.element, !readOnly);
    },

    /**
     * @return {boolean}
     */
    readOnly: function()
    {
        return this._mainPanel.readOnly();
    },

    /**
     * @return {Element}
     */
    defaultFocusedElement: function()
    {
        return this._mainPanel.defaultFocusedElement();
    },

    /**
     * @param {number} lineNumber
     */
    revealLine: function(lineNumber)
    {
        this._mainPanel.revealLine(lineNumber);
    },

    _onMouseDown: function(event)
    {
        var target = event.target.enclosingNodeOrSelfWithClass("webkit-line-number");
        if (!target)
            return;
        this.dispatchEventToListeners(WebInspector.TextEditor.Events.GutterClick, { lineNumber: target.lineNumber, event: event });
    },

    /**
     * @param {number} lineNumber
     * @param {boolean} disabled
     * @param {boolean} conditional
     */
    addBreakpoint: function(lineNumber, disabled, conditional)
    {
        this.beginUpdates();
        this._gutterPanel.addDecoration(lineNumber, "webkit-breakpoint");
        if (disabled)
            this._gutterPanel.addDecoration(lineNumber, "webkit-breakpoint-disabled");
        else
            this._gutterPanel.removeDecoration(lineNumber, "webkit-breakpoint-disabled");
        if (conditional)
            this._gutterPanel.addDecoration(lineNumber, "webkit-breakpoint-conditional");
        else
            this._gutterPanel.removeDecoration(lineNumber, "webkit-breakpoint-conditional");
        this.endUpdates();
    },

    /**
     * @param {number} lineNumber
     */
    removeBreakpoint: function(lineNumber)
    {
        this.beginUpdates();
        this._gutterPanel.removeDecoration(lineNumber, "webkit-breakpoint");
        this._gutterPanel.removeDecoration(lineNumber, "webkit-breakpoint-disabled");
        this._gutterPanel.removeDecoration(lineNumber, "webkit-breakpoint-conditional");
        this.endUpdates();
    },

    /**
     * @param {number} lineNumber
     */
    setExecutionLine: function(lineNumber)
    {
        this._executionLineNumber = lineNumber;
        this._mainPanel.addDecoration(lineNumber, "webkit-execution-line");
        this._gutterPanel.addDecoration(lineNumber, "webkit-execution-line");
    },

    clearExecutionLine: function()
    {
        if (typeof this._executionLineNumber === "number") {
            this._mainPanel.removeDecoration(this._executionLineNumber, "webkit-execution-line");
            this._gutterPanel.removeDecoration(this._executionLineNumber, "webkit-execution-line");
        }
        delete this._executionLineNumber;
    },

    /**
     * @param {number} lineNumber
     * @param {Element} element
     */
    addDecoration: function(lineNumber, element)
    {
        this._mainPanel.addDecoration(lineNumber, element);
        this._gutterPanel.addDecoration(lineNumber, element);
        this._syncDecorationsForLine(lineNumber);
    },

    /**
     * @param {number} lineNumber
     * @param {Element} element
     */
    removeDecoration: function(lineNumber, element)
    {
        this._mainPanel.removeDecoration(lineNumber, element);
        this._gutterPanel.removeDecoration(lineNumber, element);
        this._syncDecorationsForLine(lineNumber);
    },

    /**
     * @param {WebInspector.TextRange} range
     */
    markAndRevealRange: function(range)
    {
        if (range)
            this.setSelection(range);
        this._mainPanel.markAndRevealRange(range);
    },

    /**
     * @param {number} lineNumber
     */
    highlightLine: function(lineNumber)
    {
        if (typeof lineNumber !== "number" || lineNumber < 0)
            return;

        lineNumber = Math.min(lineNumber, this._textModel.linesCount - 1);
        this._mainPanel.highlightLine(lineNumber);
    },

    clearLineHighlight: function()
    {
        this._mainPanel.clearLineHighlight();
    },

    /**
     * @return {Array.<Element>}
     */
    elementsToRestoreScrollPositionsFor: function()
    {
        return [this._mainPanel.element];
    },

    /**
     * @param {WebInspector.TextEditor} textEditor
     */
    inheritScrollPositions: function(textEditor)
    {
        this._mainPanel.element._scrollTop = textEditor._mainPanel.element.scrollTop;
        this._mainPanel.element._scrollLeft = textEditor._mainPanel.element.scrollLeft;
    },

    beginUpdates: function()
    {
        this._mainPanel.beginUpdates();
        this._gutterPanel.beginUpdates();
    },

    endUpdates: function()
    {
        this._mainPanel.endUpdates();
        this._gutterPanel.endUpdates();
        this._updatePanelOffsets();
    },

    onResize: function()
    {
        this._mainPanel.resize();
        this._gutterPanel.resize();
        this._updatePanelOffsets();
    },

    _textChanged: function(event)
    {
        this._mainPanel.textChanged(event.data.oldRange, event.data.newRange);
        this._gutterPanel.textChanged(event.data.oldRange, event.data.newRange);
        this._updatePanelOffsets();
        if (event.data.editRange)
            this._delegate.onTextChanged(event.data.oldRange, event.data.newRange);
    },

    /**
     * @param {WebInspector.TextRange} range
     * @param {string} text
     * @return {WebInspector.TextRange}
     */
    editRange: function(range, text)
    {
        return this._textModel.editRange(range, text, this.lastSelection());
    },

    _updatePanelOffsets: function()
    {
        var lineNumbersWidth = this._gutterPanel.element.offsetWidth;
        if (lineNumbersWidth)
            this._mainPanel.element.style.setProperty("left", (lineNumbersWidth + 2) + "px");
        else
            this._mainPanel.element.style.removeProperty("left"); // Use default value set in CSS.
    },

    _syncScroll: function()
    {
        var mainElement = this._mainPanel.element;
        var gutterElement = this._gutterPanel.element;
        // Handle horizontal scroll bar at the bottom of the main panel.
        this._gutterPanel.syncClientHeight(mainElement.clientHeight);
        gutterElement.scrollTop = mainElement.scrollTop;
    },

    /**
     * @param {number} lineNumber
     */
    _syncDecorationsForLine: function(lineNumber)
    {
        if (lineNumber >= this._textModel.linesCount)
            return;

        var mainChunk = this._mainPanel.chunkForLine(lineNumber);
        if (mainChunk.linesCount === 1 && mainChunk.isDecorated()) {
            var gutterChunk = this._gutterPanel.makeLineAChunk(lineNumber);
            var height = mainChunk.height;
            if (height)
                gutterChunk.element.style.setProperty("height", height + "px");
            else
                gutterChunk.element.style.removeProperty("height");
        } else {
            var gutterChunk = this._gutterPanel.chunkForLine(lineNumber);
            if (gutterChunk.linesCount === 1)
                gutterChunk.element.style.removeProperty("height");
        }
    },

    /**
     * @param {Element} gutterRow
     */
    _syncLineHeight: function(gutterRow)
    {
        if (this._lineHeightSynced)
            return;
        if (gutterRow && gutterRow.offsetHeight) {
            // Force equal line heights for the child panels.
            this.element.style.setProperty("line-height", gutterRow.offsetHeight + "px");
            this._lineHeightSynced = true;
        }
    },

    _registerShortcuts: function()
    {
        var keys = WebInspector.KeyboardShortcut.Keys;
        var modifiers = WebInspector.KeyboardShortcut.Modifiers;

        this._shortcuts = {};

        this._shortcuts[WebInspector.KeyboardShortcut.SelectAll] = this._handleSelectAll.bind(this);
        this._wordMovementController._registerShortcuts(this._shortcuts);
    },

    _handleSelectAll: function()
    {
        this.setSelection(this._textModel.range());
        return true;
    },

    _handleKeyDown: function(e)
    {
        // If the event was not triggered from the entire editor, then
        // ignore it. https://bugs.webkit.org/show_bug.cgi?id=102906
        if (e.target.enclosingNodeOrSelfWithClass("webkit-line-decorations"))
            return;

        var shortcutKey = WebInspector.KeyboardShortcut.makeKeyFromEvent(e);

        var handler = this._shortcuts[shortcutKey];
        if (handler && handler()) {
            e.consume(true);
            return;
        }
        this._mainPanel.handleKeyDown(shortcutKey, e);
    },

    _contextMenu: function(event)
    {
        var anchor = event.target.enclosingNodeOrSelfWithNodeName("a");
        if (anchor)
            return;
        var contextMenu = new WebInspector.ContextMenu(event);
        var target = event.target.enclosingNodeOrSelfWithClass("webkit-line-number");
        if (target)
            this._delegate.populateLineGutterContextMenu(contextMenu, target.lineNumber);
        else {
            this._mainPanel.populateContextMenu(event.target, contextMenu);
        }
        contextMenu.show();
    },

    _handleScrollChanged: function(event)
    {
        var visibleFrom = this._mainPanel.scrollTop();
        var firstVisibleLineNumber = this._mainPanel.lineNumberAtOffset(visibleFrom);
        this._delegate.scrollChanged(firstVisibleLineNumber);
    },

    /**
     * @param {number} lineNumber
     */
    scrollToLine: function(lineNumber)
    {
        this._mainPanel.scrollToLine(lineNumber);
    },

    /**
     * @return {WebInspector.TextRange}
     */
    selection: function()
    {
        return this._mainPanel.selection();
    },

    /**
     * @return {WebInspector.TextRange?}
     */
    lastSelection: function()
    {
        return this._mainPanel.lastSelection();
    },

    /**
     * @param {WebInspector.TextRange} textRange
     */
    setSelection: function(textRange)
    {
        this._mainPanel.setSelection(textRange);
    },

    /**
     * @param {string} text
     */
    setText: function(text)
    {
        this._textModel.setText(text);
    },

    /**
     * @return {string}
     */
    text: function()
    {
        return this._textModel.text();
    },

    /**
     * @return {WebInspector.TextRange}
     */
    range: function()
    {
        return this._textModel.range();
    },

    /**
     * @param {number} lineNumber
     * @return {string}
     */
    line: function(lineNumber)
    {
        return this._textModel.line(lineNumber);
    },

    /**
     * @return {number}
     */
    get linesCount()
    {
        return this._textModel.linesCount;
    },

    /**
     * @param {number} line
     * @param {string} name
     * @param {?Object} value
     */
    setAttribute: function(line, name, value)
    {
        this._textModel.setAttribute(line, name, value);
    },

    /**
     * @param {number} line
     * @param {string} name
     * @return {?Object} value
     */
    getAttribute: function(line, name)
    {
        return this._textModel.getAttribute(line, name);
    },

    /**
     * @param {number} line
     * @param {string} name
     */
    removeAttribute: function(line, name)
    {
        this._textModel.removeAttribute(line, name);
    },

    wasShown: function()
    {
        if (!this.readOnly())
            WebInspector.markBeingEdited(this.element, true);

        this._mainPanel.wasShown();
    },

    willHide: function()
    {
        this._mainPanel.willHide();
        this._gutterPanel.willHide();

        if (!this.readOnly())
            WebInspector.markBeingEdited(this.element, false);
    },

    /**
     * @param {Element} element
     * @param {Array.<Object>} resultRanges
     * @param {string} styleClass
     * @param {Array.<Object>=} changes
     */
    highlightRangesWithStyleClass: function(element, resultRanges, styleClass, changes)
    {
        this._mainPanel.beginDomUpdates();
        WebInspector.highlightRangesWithStyleClass(element, resultRanges, styleClass, changes);
        this._mainPanel.endDomUpdates();
    },

    /**
     * @param {number} scrollTop
     * @param {number} clientHeight
     * @param {number} chunkSize
     */
    overrideViewportForTest: function(scrollTop, clientHeight, chunkSize)
    {
        this._mainPanel.overrideViewportForTest(scrollTop, clientHeight, chunkSize);
    },

    __proto__: WebInspector.View.prototype
}

/**
 * @constructor
 * @param {WebInspector.TextEditorModel} textModel
 */
WebInspector.TextEditorChunkedPanel = function(textModel)
{
    this._textModel = textModel;

    this.element = document.createElement("div");
    this.element.addEventListener("scroll", this._scroll.bind(this), false);

    this._defaultChunkSize = 50;
    this._paintCoalescingLevel = 0;
    this._domUpdateCoalescingLevel = 0;
}

WebInspector.TextEditorChunkedPanel.prototype = {
    /**
     * @param {number} lineNumber
     */
    scrollToLine: function(lineNumber)
    {
        if (lineNumber >= this._textModel.linesCount)
            return;

        var chunk = this.makeLineAChunk(lineNumber);
        this.element.scrollTop = chunk.offsetTop;
    },

    /**
     * @param {number} lineNumber
     */
    revealLine: function(lineNumber)
    {
        if (lineNumber >= this._textModel.linesCount)
            return;

        var chunk = this.makeLineAChunk(lineNumber);
        chunk.element.scrollIntoViewIfNeeded();
    },

    /**
     * @param {number} lineNumber
     * @param {string|Element} decoration
     */
    addDecoration: function(lineNumber, decoration)
    {
        if (lineNumber >= this._textModel.linesCount)
            return;

        var chunk = this.makeLineAChunk(lineNumber);
        chunk.addDecoration(decoration);
    },

    /**
     * @param {number} lineNumber
     * @param {string|Element} decoration
     */
    removeDecoration: function(lineNumber, decoration)
    {
        if (lineNumber >= this._textModel.linesCount)
            return;

        var chunk = this.chunkForLine(lineNumber);
        chunk.removeDecoration(decoration);
    },

    buildChunks: function()
    {
        this.beginDomUpdates();

        this._container.removeChildren();

        this._textChunks = [];
        for (var i = 0; i < this._textModel.linesCount; i += this._defaultChunkSize) {
            var chunk = this.createNewChunk(i, i + this._defaultChunkSize);
            this._textChunks.push(chunk);
            this._container.appendChild(chunk.element);
        }

        this.repaintAll();

        this.endDomUpdates();
    },

    /**
     * @param {number} lineNumber
     * @return {Object}
     */
    makeLineAChunk: function(lineNumber)
    {
        var chunkNumber = this.chunkNumberForLine(lineNumber);
        var oldChunk = this._textChunks[chunkNumber];

        if (!oldChunk) {
            console.error("No chunk for line number: " + lineNumber);
            return null;
        }

        if (oldChunk.linesCount === 1)
            return oldChunk;

        return this.splitChunkOnALine(lineNumber, chunkNumber, true);
    },

    /**
     * @param {number} lineNumber
     * @param {number} chunkNumber
     * @param {boolean=} createSuffixChunk
     * @return {Object}
     */
    splitChunkOnALine: function(lineNumber, chunkNumber, createSuffixChunk)
    {
        this.beginDomUpdates();

        var oldChunk = this._textChunks[chunkNumber];
        var wasExpanded = oldChunk.expanded();
        oldChunk.collapse();

        var insertIndex = chunkNumber + 1;

        // Prefix chunk.
        if (lineNumber > oldChunk.startLine) {
            var prefixChunk = this.createNewChunk(oldChunk.startLine, lineNumber);
            this._textChunks.splice(insertIndex++, 0, prefixChunk);
            this._container.insertBefore(prefixChunk.element, oldChunk.element);
        }

        // Line chunk.
        var endLine = createSuffixChunk ? lineNumber + 1 : oldChunk.startLine + oldChunk.linesCount;
        var lineChunk = this.createNewChunk(lineNumber, endLine);
        this._textChunks.splice(insertIndex++, 0, lineChunk);
        this._container.insertBefore(lineChunk.element, oldChunk.element);

        // Suffix chunk.
        if (oldChunk.startLine + oldChunk.linesCount > endLine) {
            var suffixChunk = this.createNewChunk(endLine, oldChunk.startLine + oldChunk.linesCount);
            this._textChunks.splice(insertIndex, 0, suffixChunk);
            this._container.insertBefore(suffixChunk.element, oldChunk.element);
        }

        // Remove enclosing chunk.
        this._textChunks.splice(chunkNumber, 1);
        this._container.removeChild(oldChunk.element);

        if (wasExpanded) {
            if (prefixChunk)
                prefixChunk.expand();
            lineChunk.expand();
            if (suffixChunk)
                suffixChunk.expand();
        }

        this.endDomUpdates();

        return lineChunk;
    },

    createNewChunk: function(startLine, endLine)
    {
        throw new Error("createNewChunk() should be implemented by descendants");
    },

    _scroll: function()
    {
        this._scheduleRepaintAll();
        if (this._syncScrollListener)
            this._syncScrollListener();
    },

    _scheduleRepaintAll: function()
    {
        if (this._repaintAllTimer)
            clearTimeout(this._repaintAllTimer);
        this._repaintAllTimer = setTimeout(this.repaintAll.bind(this), 50);
    },

    beginUpdates: function()
    {
        this._paintCoalescingLevel++;
    },

    endUpdates: function()
    {
        this._paintCoalescingLevel--;
        if (!this._paintCoalescingLevel)
            this.repaintAll();
    },

    beginDomUpdates: function()
    {
        this._domUpdateCoalescingLevel++;
    },

    endDomUpdates: function()
    {
        this._domUpdateCoalescingLevel--;
    },

    /**
     * @param {number} lineNumber
     * @return {number}
     */
    chunkNumberForLine: function(lineNumber)
    {
        function compareLineNumbers(value, chunk)
        {
            return value < chunk.startLine ? -1 : 1;
        }
        var insertBefore = insertionIndexForObjectInListSortedByFunction(lineNumber, this._textChunks, compareLineNumbers);
        return insertBefore - 1;
    },

    /**
     * @param {number} lineNumber
     * @return {Object}
     */
    chunkForLine: function(lineNumber)
    {
        return this._textChunks[this.chunkNumberForLine(lineNumber)];
    },

    /**
     * @param {number} visibleFrom
     * @return {number}
     */
    _findFirstVisibleChunkNumber: function(visibleFrom)
    {
        function compareOffsetTops(value, chunk)
        {
            return value < chunk.offsetTop ? -1 : 1;
        }
        var insertBefore = insertionIndexForObjectInListSortedByFunction(visibleFrom, this._textChunks, compareOffsetTops);
        return insertBefore - 1;
    },

    /**
     * @param {number} visibleFrom
     * @param {number} visibleTo
     * @return {{start: number, end: number}}
     */
    findVisibleChunks: function(visibleFrom, visibleTo)
    {
        var span = (visibleTo - visibleFrom) * 0.5;
        visibleFrom = Math.max(visibleFrom - span, 0);
        visibleTo = visibleTo + span;

        var from = this._findFirstVisibleChunkNumber(visibleFrom);
        for (var to = from + 1; to < this._textChunks.length; ++to) {
            if (this._textChunks[to].offsetTop >= visibleTo)
                break;
        }
        return { start: from, end: to };
    },

    /**
     * @param {number} visibleFrom
     * @return {number}
     */
    lineNumberAtOffset: function(visibleFrom)
    {
        var chunk = this._textChunks[this._findFirstVisibleChunkNumber(visibleFrom)];
        if (!chunk.expanded())
            return chunk.startLine;

        var lineNumbers = [];
        for (var i = 0; i < chunk.linesCount; ++i) {
            lineNumbers.push(chunk.startLine + i);
        }

        function compareLineRowOffsetTops(value, lineNumber)
        {
            var lineRow = chunk.expandedLineRow(lineNumber);
            return value < lineRow.offsetTop ? -1 : 1;
        }
        var insertBefore = insertionIndexForObjectInListSortedByFunction(visibleFrom, lineNumbers, compareLineRowOffsetTops);
        return lineNumbers[insertBefore - 1];
    },

    repaintAll: function()
    {
        delete this._repaintAllTimer;

        if (this._paintCoalescingLevel)
            return;

        var visibleFrom = this.scrollTop();
        var visibleTo = visibleFrom + this.clientHeight();

        if (visibleTo) {
            var result = this.findVisibleChunks(visibleFrom, visibleTo);
            this.expandChunks(result.start, result.end);
        }
    },

    scrollTop: function()
    {
        return typeof this._scrollTopOverrideForTest === "number" ? this._scrollTopOverrideForTest : this.element.scrollTop;
    },

    clientHeight: function()
    {
        return typeof this._clientHeightOverrideForTest === "number" ? this._clientHeightOverrideForTest : this.element.clientHeight;
    },

    /**
     * @param {number} fromIndex
     * @param {number} toIndex
     */
    expandChunks: function(fromIndex, toIndex)
    {
        // First collapse chunks to collect the DOM elements into a cache to reuse them later.
        for (var i = 0; i < fromIndex; ++i)
            this._textChunks[i].collapse();
        for (var i = toIndex; i < this._textChunks.length; ++i)
            this._textChunks[i].collapse();
        for (var i = fromIndex; i < toIndex; ++i)
            this._textChunks[i].expand();
    },

    /**
     * @param {Element} firstElement
     * @param {Element=} lastElement
     * @return {number}
     */
    totalHeight: function(firstElement, lastElement)
    {
        lastElement = (lastElement || firstElement).nextElementSibling;
        if (lastElement)
            return lastElement.offsetTop - firstElement.offsetTop;

        var offsetParent = firstElement.offsetParent;
        if (offsetParent && offsetParent.scrollHeight > offsetParent.clientHeight)
            return offsetParent.scrollHeight - firstElement.offsetTop;

        var total = 0;
        while (firstElement && firstElement !== lastElement) {
            total += firstElement.offsetHeight;
            firstElement = firstElement.nextElementSibling;
        }
        return total;
    },

    resize: function()
    {
        this.repaintAll();
    }
}

/**
 * @constructor
 * @extends {WebInspector.TextEditorChunkedPanel}
 * @param {WebInspector.TextEditorModel} textModel
 * @param {function(number)} syncDecorationsForLineListener
 * @param {function(Element)} syncLineHeightListener
 */
WebInspector.TextEditorGutterPanel = function(textModel, syncDecorationsForLineListener, syncLineHeightListener)
{
    WebInspector.TextEditorChunkedPanel.call(this, textModel);

    this._syncDecorationsForLineListener = syncDecorationsForLineListener;
    this._syncLineHeightListener = syncLineHeightListener;

    this.element.className = "text-editor-lines";

    this._container = document.createElement("div");
    this._container.className = "inner-container";
    this.element.appendChild(this._container);

    this._freeCachedElements();
    this.buildChunks();
    this._decorations = {};
}

WebInspector.TextEditorGutterPanel.prototype = {
    _freeCachedElements: function()
    {
        this._cachedRows = [];
    },

    willHide: function()
    {
        this._freeCachedElements();
    },

    /**
     * @param {number} startLine
     * @param {number} endLine
     * @return {WebInspector.TextEditorGutterChunk}
     */
    createNewChunk: function(startLine, endLine)
    {
        return new WebInspector.TextEditorGutterChunk(this, startLine, endLine);
    },

    /**
     * @param {WebInspector.TextRange} oldRange
     * @param {WebInspector.TextRange} newRange
     */
    textChanged: function(oldRange, newRange)
    {
        this.beginDomUpdates();

        var linesDiff = newRange.linesCount - oldRange.linesCount;
        if (linesDiff) {
            // Remove old chunks (if needed).
            for (var chunkNumber = this._textChunks.length - 1; chunkNumber >= 0; --chunkNumber) {
                var chunk = this._textChunks[chunkNumber];
                if (chunk.startLine + chunk.linesCount <= this._textModel.linesCount)
                    break;
                chunk.collapse();
                this._container.removeChild(chunk.element);
            }
            this._textChunks.length = chunkNumber + 1;

            // Add new chunks (if needed).
            var totalLines = 0;
            if (this._textChunks.length) {
                var lastChunk = this._textChunks[this._textChunks.length - 1];
                totalLines = lastChunk.startLine + lastChunk.linesCount;
            }

            for (var i = totalLines; i < this._textModel.linesCount; i += this._defaultChunkSize) {
                var chunk = this.createNewChunk(i, i + this._defaultChunkSize);
                this._textChunks.push(chunk);
                this._container.appendChild(chunk.element);
            }

            // Shift decorations if necessary
            var decorationsToRestore = {};
            for (var lineNumber in this._decorations) {
                lineNumber = parseInt(lineNumber, 10);

                // Do not move decorations before the start position.
                if (lineNumber < oldRange.startLine)
                    continue;
                // Decorations follow the first character of line.
                if (lineNumber === oldRange.startLine && oldRange.startColumn)
                    continue;

                var lineDecorationsCopy = this._decorations[lineNumber].slice();
                for (var i = 0; i < lineDecorationsCopy.length; ++i)
                    this.removeDecoration(lineNumber, lineDecorationsCopy[i]);
                // Do not restore the decorations before the end position.
                if (lineNumber >= oldRange.endLine)
                    decorationsToRestore[lineNumber] = lineDecorationsCopy;
            }
            for (var lineNumber in decorationsToRestore) {
                lineNumber = parseInt(lineNumber, 10);
                var lineDecorationsCopy = decorationsToRestore[lineNumber];
                for (var i = 0; i < lineDecorationsCopy.length; ++i)
                    this.addDecoration(lineNumber + linesDiff, lineDecorationsCopy[i]);
            }


            this.repaintAll();
        } else {
            // Decorations may have been removed, so we may have to sync those lines.
            var chunkNumber = this.chunkNumberForLine(newRange.startLine);
            var chunk = this._textChunks[chunkNumber];
            while (chunk && chunk.startLine <= newRange.endLine) {
                if (chunk.linesCount === 1)
                    this._syncDecorationsForLineListener(chunk.startLine);
                chunk = this._textChunks[++chunkNumber];
            }
        }

        this.endDomUpdates();
    },

    /**
     * @param {number} clientHeight
     */
    syncClientHeight: function(clientHeight)
    {
        if (this.element.offsetHeight > clientHeight)
            this._container.style.setProperty("padding-bottom", (this.element.offsetHeight - clientHeight) + "px");
        else
            this._container.style.removeProperty("padding-bottom");
    },

    /**
     * @param {number} lineNumber
     * @param {string|Element} decoration
     */
    addDecoration: function(lineNumber, decoration)
    {
        WebInspector.TextEditorChunkedPanel.prototype.addDecoration.call(this, lineNumber, decoration);
        var decorations = this._decorations[lineNumber];
        if (!decorations) {
            decorations = [];
            this._decorations[lineNumber] = decorations;
        }
        decorations.push(decoration);
    },

    /**
     * @param {number} lineNumber
     * @param {string|Element} decoration
     */
    removeDecoration: function(lineNumber, decoration)
    {
        WebInspector.TextEditorChunkedPanel.prototype.removeDecoration.call(this, lineNumber, decoration);
        var decorations = this._decorations[lineNumber];
        if (decorations) {
            decorations.remove(decoration);
            if (!decorations.length)
                delete this._decorations[lineNumber];
        }
    },

    __proto__: WebInspector.TextEditorChunkedPanel.prototype
}

/**
 * @constructor
 * @param {WebInspector.TextEditorGutterPanel} chunkedPanel
 * @param {number} startLine
 * @param {number} endLine
 */
WebInspector.TextEditorGutterChunk = function(chunkedPanel, startLine, endLine)
{
    this._chunkedPanel = chunkedPanel;
    this._textModel = chunkedPanel._textModel;

    this.startLine = startLine;
    endLine = Math.min(this._textModel.linesCount, endLine);
    this.linesCount = endLine - startLine;

    this._expanded = false;

    this.element = document.createElement("div");
    this.element.lineNumber = startLine;
    this.element.className = "webkit-line-number";

    if (this.linesCount === 1) {
        // Single line chunks are typically created for decorations. Host line number in
        // the sub-element in order to allow flexible border / margin management.
        var innerSpan = document.createElement("span");
        innerSpan.className = "webkit-line-number-inner";
        innerSpan.textContent = startLine + 1;
        var outerSpan = document.createElement("div");
        outerSpan.className = "webkit-line-number-outer";
        outerSpan.appendChild(innerSpan);
        this.element.appendChild(outerSpan);
    } else {
        var lineNumbers = [];
        for (var i = startLine; i < endLine; ++i)
            lineNumbers.push(i + 1);
        this.element.textContent = lineNumbers.join("\n");
    }
}

WebInspector.TextEditorGutterChunk.prototype = {
    /**
     * @param {string} decoration
     */
    addDecoration: function(decoration)
    {
        this._chunkedPanel.beginDomUpdates();
        if (typeof decoration === "string")
            this.element.addStyleClass(decoration);
        this._chunkedPanel.endDomUpdates();
    },

    /**
     * @param {string} decoration
     */
    removeDecoration: function(decoration)
    {
        this._chunkedPanel.beginDomUpdates();
        if (typeof decoration === "string")
            this.element.removeStyleClass(decoration);
        this._chunkedPanel.endDomUpdates();
    },

    /**
     * @return {boolean}
     */
    expanded: function()
    {
        return this._expanded;
    },

    expand: function()
    {
        if (this.linesCount === 1)
            this._chunkedPanel._syncDecorationsForLineListener(this.startLine);

        if (this._expanded)
            return;

        this._expanded = true;

        if (this.linesCount === 1)
            return;

        this._chunkedPanel.beginDomUpdates();

        this._expandedLineRows = [];
        var parentElement = this.element.parentElement;
        for (var i = this.startLine; i < this.startLine + this.linesCount; ++i) {
            var lineRow = this._createRow(i);
            parentElement.insertBefore(lineRow, this.element);
            this._expandedLineRows.push(lineRow);
        }
        parentElement.removeChild(this.element);
        this._chunkedPanel._syncLineHeightListener(this._expandedLineRows[0]);

        this._chunkedPanel.endDomUpdates();
    },

    collapse: function()
    {
        if (this.linesCount === 1)
            this._chunkedPanel._syncDecorationsForLineListener(this.startLine);

        if (!this._expanded)
            return;

        this._expanded = false;

        if (this.linesCount === 1)
            return;

        this._chunkedPanel.beginDomUpdates();

        var elementInserted = false;
        for (var i = 0; i < this._expandedLineRows.length; ++i) {
            var lineRow = this._expandedLineRows[i];
            var parentElement = lineRow.parentElement;
            if (parentElement) {
                if (!elementInserted) {
                    elementInserted = true;
                    parentElement.insertBefore(this.element, lineRow);
                }
                parentElement.removeChild(lineRow);
            }
            this._chunkedPanel._cachedRows.push(lineRow);
        }
        delete this._expandedLineRows;

        this._chunkedPanel.endDomUpdates();
    },

    /**
     * @return {number}
     */
    get height()
    {
        if (!this._expandedLineRows)
            return this._chunkedPanel.totalHeight(this.element);
        return this._chunkedPanel.totalHeight(this._expandedLineRows[0], this._expandedLineRows[this._expandedLineRows.length - 1]);
    },

    /**
     * @return {number}
     */
    get offsetTop()
    {
        return (this._expandedLineRows && this._expandedLineRows.length) ? this._expandedLineRows[0].offsetTop : this.element.offsetTop;
    },

    /**
     * @param {number} lineNumber
     * @return {Element}
     */
    _createRow: function(lineNumber)
    {
        var lineRow = this._chunkedPanel._cachedRows.pop() || document.createElement("div");
        lineRow.lineNumber = lineNumber;
        lineRow.className = "webkit-line-number";
        lineRow.textContent = lineNumber + 1;
        return lineRow;
    }
}

/**
 * @constructor
 * @extends {WebInspector.TextEditorChunkedPanel}
 * @param {WebInspector.TextEditorDelegate} delegate
 * @param {WebInspector.TextEditorModel} textModel
 * @param {?string} url
 * @param {function()} syncScrollListener
 * @param {function(number)} syncDecorationsForLineListener
 */
WebInspector.TextEditorMainPanel = function(delegate, textModel, url, syncScrollListener, syncDecorationsForLineListener)
{
    WebInspector.TextEditorChunkedPanel.call(this, textModel);

    this._delegate = delegate;
    this._syncScrollListener = syncScrollListener;
    this._syncDecorationsForLineListener = syncDecorationsForLineListener;

    this._url = url;
    this._highlighter = new WebInspector.TextEditorHighlighter(textModel, this._highlightDataReady.bind(this));
    this._readOnly = true;

    this.element.className = "text-editor-contents";
    this.element.tabIndex = 0;

    this._container = document.createElement("div");
    this._container.className = "inner-container";
    this._container.tabIndex = 0;
    this.element.appendChild(this._container);

    this.element.addEventListener("focus", this._handleElementFocus.bind(this), false);
    this.element.addEventListener("textInput", this._handleTextInput.bind(this), false);
    this.element.addEventListener("cut", this._handleCut.bind(this), false);
    this.element.addEventListener("keypress", this._handleKeyPress.bind(this), false);

    this._showWhitespace = WebInspector.experimentsSettings.showWhitespaceInEditor.isEnabled();

    this._container.addEventListener("focus", this._handleFocused.bind(this), false);

    this._highlightDescriptors = [];

    this._tokenHighlighter = new WebInspector.TextEditorMainPanel.TokenHighlighter(this, textModel);
    this._braceMatcher = new WebInspector.TextEditorModel.BraceMatcher(textModel);
    this._braceHighlighter = new WebInspector.TextEditorMainPanel.BraceHighlightController(this, textModel, this._braceMatcher);
    this._smartBraceController = new WebInspector.TextEditorMainPanel.SmartBraceController(this, textModel, this._braceMatcher);

    this._freeCachedElements();
    this.buildChunks();
    this._registerShortcuts();
}

WebInspector.TextEditorMainPanel._ConsecutiveWhitespaceChars = {
    1: " ",
    2: "  ",
    4: "    ",
    8: "        ",
    16: "                "
};

WebInspector.TextEditorMainPanel.prototype = {
    /**
     * @param {number} lineNumber
     * @param {number} column
     * @return {?{startColumn: number, endColumn: number, type: string}}
     */
    tokenAtTextPosition: function(lineNumber, column)
    {
        if (lineNumber >= this._textModel.linesCount || lineNumber < 0)
            return null;
        var line = this._textModel.line(lineNumber);
        if (column >= line.length || column < 0)
            return null;
        var highlight = this._textModel.getAttribute(lineNumber, "highlight");
        if (!highlight)
            return this._tokenAtUnhighlightedLine(line, column);
        function compare(value, object)
        {
            if (value >= object.startColumn && value <= object.endColumn)
                return 0;
            return value - object.startColumn;
        }
        var index = binarySearch(column, highlight.ranges, compare);
        if (index >= 0) {
            var range = highlight.ranges[index];
            return {
                startColumn: range.startColumn,
                endColumn: range.endColumn,
                type: range.token
            };
        }
        return null;
    },

    /**
     * @param {number} lineNumber
     * @param {number} column
     * @return {?{x: number, y: number, height: number}}
     */
    cursorPositionToCoordinates: function(lineNumber, column)
    {
        if (lineNumber >= this._textModel.linesCount || lineNumber < 0)
            return null;
        var line = this._textModel.line(lineNumber);
        if (column > line.length || column < 0)
            return null;

        var chunk = this.chunkForLine(lineNumber);
        if (!chunk.expanded())
            return null;
        var lineRow = chunk.expandedLineRow(lineNumber);
        var ranges = [{
            startColumn: column,
            endColumn: column,
            token: "measure-cursor-position"
        }];
        var selection = this.selection();

        this.beginDomUpdates();
        this._renderRanges(lineRow, line, ranges);
        var spans = lineRow.getElementsByClassName("webkit-measure-cursor-position");
        if (WebInspector.debugDefaultTextEditor)
            console.assert(spans.length === 0);
        var totalOffset = spans[0].totalOffset();
        var height = spans[0].offsetHeight;
        this._paintLineRows([lineRow]);
        this.endDomUpdates();

        this._restoreSelection(selection);
        return {
            x: totalOffset.left,
            y: totalOffset.top,
            height: height
        };
    },

    /**
     * @param {number} x
     * @param {number} y
     * @return {?WebInspector.TextRange}
     */
    coordinatesToCursorPosition: function(x, y)
    {
        var element = document.elementFromPoint(x, y);
        if (!element)
            return null;
        var lineRow = element.enclosingNodeOrSelfWithClass("webkit-line-content");
        if (!lineRow)
            return null;

        var line = this._textModel.line(lineRow.lineNumber) + " ";
        var ranges = [];
        const prefix = "character-position-";
        for(var i = 0; i < line.length; ++i) {
            ranges.push({
                startColumn: i,
                endColumn: i,
                token: prefix + i
            });
        }

        var selection = this.selection();

        this.beginDomUpdates();
        this._renderRanges(lineRow, line, ranges);
        var charElement = document.elementFromPoint(x, y);
        this._paintLineRows([lineRow]);
        this.endDomUpdates();

        this._restoreSelection(selection);
        var className = charElement.className;
        if (className.indexOf(prefix) < 0)
            return null;
        var column = parseInt(className.substring(className.indexOf(prefix) + prefix.length), 10);

        return WebInspector.TextRange.createFromLocation(lineRow.lineNumber, column);
    },

    /**
     * @param {string} line
     * @param {number} column
     * @return {?{startColumn: number, endColumn: number, type: string}}
     */
    _tokenAtUnhighlightedLine: function(line, column)
    {
        var tokenizer = WebInspector.SourceTokenizer.Registry.getInstance().getTokenizer(this.mimeType);
        tokenizer.condition = tokenizer.createInitialCondition();
        tokenizer.line = line;
        var lastTokenizedColumn = 0;
        while (lastTokenizedColumn < line.length) {
            var newColumn = tokenizer.nextToken(lastTokenizedColumn);
            if (column < newColumn) {
                if (!tokenizer.tokenType)
                    return null;
                return {
                    startColumn: lastTokenizedColumn,
                    endColumn: newColumn - 1,
                    type: tokenizer.tokenType
                };
            } else
                lastTokenizedColumn = newColumn;
        }
        return null;
    },

    _registerShortcuts: function()
    {
        var keys = WebInspector.KeyboardShortcut.Keys;
        var modifiers = WebInspector.KeyboardShortcut.Modifiers;

        this._shortcuts = {};

        this._shortcuts[WebInspector.KeyboardShortcut.makeKey(keys.Enter.code, WebInspector.KeyboardShortcut.Modifiers.None)] = this._handleEnterKey.bind(this);
        this._shortcuts[WebInspector.KeyboardShortcut.makeKey("z", modifiers.CtrlOrMeta)] = this._handleUndoRedo.bind(this, false);

        var handleRedo = this._handleUndoRedo.bind(this, true);
        this._shortcuts[WebInspector.KeyboardShortcut.makeKey("z", modifiers.Shift | modifiers.CtrlOrMeta)] = handleRedo;
        if (!WebInspector.isMac())
            this._shortcuts[WebInspector.KeyboardShortcut.makeKey("y", modifiers.CtrlOrMeta)] = handleRedo;

        var handleTabKey = this._handleTabKeyPress.bind(this, false);
        var handleShiftTabKey = this._handleTabKeyPress.bind(this, true);
        this._shortcuts[WebInspector.KeyboardShortcut.makeKey(keys.Tab.code)] = handleTabKey;
        this._shortcuts[WebInspector.KeyboardShortcut.makeKey(keys.Tab.code, modifiers.Shift)] = handleShiftTabKey;

        var homeKey = WebInspector.isMac() ? keys.Right : keys.Home;
        var homeModifier = WebInspector.isMac() ? modifiers.Meta : modifiers.None;
        this._shortcuts[WebInspector.KeyboardShortcut.makeKey(homeKey.code, homeModifier)] = this._handleHomeKey.bind(this, false);
        this._shortcuts[WebInspector.KeyboardShortcut.makeKey(homeKey.code, homeModifier | modifiers.Shift)] = this._handleHomeKey.bind(this, true);

        this._charOverrides = {};

        this._smartBraceController.registerShortcuts(this._shortcuts);
        this._smartBraceController.registerCharOverrides(this._charOverrides);
    },

    _handleKeyPress: function(event)
    {
        var char = String.fromCharCode(event.which);
        var handler = this._charOverrides[char];
        if (handler && handler()) {
            event.consume(true);
            return;
        }
        this._keyDownCode = event.keyCode;
    },

    /**
     * @param {boolean} shift
     */
    _handleHomeKey: function(shift)
    {
        var selection = this.selection();

        var line = this._textModel.line(selection.endLine);
        var firstNonBlankCharacter = 0;
        while (firstNonBlankCharacter < line.length) {
            var char = line.charAt(firstNonBlankCharacter);
            if (char === " " || char === "\t")
                ++firstNonBlankCharacter;
            else
                break;
        }
        if (firstNonBlankCharacter >= line.length || selection.endColumn === firstNonBlankCharacter)
            return false;

        selection.endColumn = firstNonBlankCharacter;
        if (!shift)
            selection = selection.collapseToEnd();
        this._restoreSelection(selection);
        return true;
    },

    /**
     * @param {string} regex
     * @param {string} cssClass
     * @return {Object}
     */
    highlightRegex: function(regex, cssClass)
    {
        var highlightDescriptor = new WebInspector.TextEditorMainPanel.RegexHighlightDescriptor(new RegExp(regex, "g"), cssClass);
        this._highlightDescriptors.push(highlightDescriptor);
        this._repaintLineRowsAffectedByHighlightDescriptors([highlightDescriptor]);
        return highlightDescriptor;
    },

    /**
     * @param {Object} highlightDescriptor
     */
    removeHighlight: function(highlightDescriptor)
    {
        this._highlightDescriptors.remove(highlightDescriptor);
        this._repaintLineRowsAffectedByHighlightDescriptors([highlightDescriptor]);
    },

    /**
     * @param {WebInspector.TextRange} range
     * @param {string} cssClass
     * @return {Object}
     */
    highlightRange: function(range, cssClass)
    {
        var highlightDescriptor = new WebInspector.TextEditorMainPanel.RangeHighlightDescriptor(range, cssClass);
        this._highlightDescriptors.push(highlightDescriptor);
        this._repaintLineRowsAffectedByHighlightDescriptors([highlightDescriptor]);
        return highlightDescriptor;
    },

    /**
     * @param {Array.<WebInspector.TextEditorMainPanel.HighlightDescriptor>} highlightDescriptors
     */
    _repaintLineRowsAffectedByHighlightDescriptors: function(highlightDescriptors)
    {
        var visibleFrom = this.scrollTop();
        var visibleTo = visibleFrom + this.clientHeight();

        var visibleChunks = this.findVisibleChunks(visibleFrom, visibleTo);

        var affectedLineRows = [];
        for (var i = visibleChunks.start; i < visibleChunks.end; ++i) {
            var chunk = this._textChunks[i];
            if (!chunk.expanded())
                continue;
            for (var lineNumber = chunk.startLine; lineNumber < chunk.startLine + chunk.linesCount; ++lineNumber) {
                var lineRow = chunk.expandedLineRow(lineNumber);
                var line = this._textModel.line(lineNumber);
                for(var j = 0; j < highlightDescriptors.length; ++j) {
                    if (highlightDescriptors[j].affectsLine(lineNumber, line)) {
                        affectedLineRows.push(lineRow);
                        break;
                    }
                }
            }
        }
        if (affectedLineRows.length === 0)
            return;
        var selection = this.selection();
        this._paintLineRows(affectedLineRows);
        this._restoreSelection(selection);
    },

    resize: function()
    {
        WebInspector.TextEditorChunkedPanel.prototype.resize.call(this);
        this._repaintLineRowsAffectedByHighlightDescriptors(this._highlightDescriptors);
    },

    wasShown: function()
    {
        this._boundSelectionChangeListener = this._handleSelectionChange.bind(this);
        document.addEventListener("selectionchange", this._boundSelectionChangeListener, false);

        this._isShowing = true;
        this._attachMutationObserver();
    },

    willHide: function()
    {
        document.removeEventListener("selectionchange", this._boundSelectionChangeListener, false);
        delete this._boundSelectionChangeListener;

        this._detachMutationObserver();
        this._isShowing = false;
        this._freeCachedElements();
    },

    /**
     * @param {Element} eventTarget
     * @param {WebInspector.ContextMenu} contextMenu
     */
    populateContextMenu: function(eventTarget, contextMenu)
    {
        var target = this._enclosingLineRowOrSelf(eventTarget);
        this._delegate.populateTextAreaContextMenu(contextMenu, target && target.lineNumber);
    },

    /**
     * @param {WebInspector.TextRange} textRange
     */
    setSelection: function(textRange)
    {
        this._lastSelection = textRange;
        if (this.element.isAncestor(document.activeElement))
            this._restoreSelection(textRange);
    },

    _handleFocused: function()
    {
        if (this._lastSelection)
            this.setSelection(this._lastSelection);
    },

    _attachMutationObserver: function()
    {
        if (!this._isShowing)
            return;

        if (this._mutationObserver)
            this._mutationObserver.disconnect();
        this._mutationObserver = new NonLeakingMutationObserver(this._handleMutations.bind(this));
        this._mutationObserver.observe(this._container, { subtree: true, childList: true, characterData: true });
    },

    _detachMutationObserver: function()
    {
        if (!this._isShowing)
            return;

        if (this._mutationObserver) {
            this._mutationObserver.disconnect();
            delete this._mutationObserver;
        }
    },

    /**
     * @param {string} mimeType
     */
    set mimeType(mimeType)
    {
        this._highlighter.mimeType = mimeType;
    },

    get mimeType()
    {
        return this._highlighter.mimeType;
    },

    /**
     * @param {boolean} readOnly
     * @param {boolean} requestFocus
     */
    setReadOnly: function(readOnly, requestFocus)
    {
        if (this._readOnly === readOnly)
            return;

        this.beginDomUpdates();
        this._readOnly = readOnly;
        if (this._readOnly)
            this._container.removeStyleClass("text-editor-editable");
        else {
            this._container.addStyleClass("text-editor-editable");
            if (requestFocus)
                this._updateSelectionOnStartEditing();
        }
        this.endDomUpdates();
    },

    /**
     * @return {boolean}
     */
    readOnly: function()
    {
        return this._readOnly;
    },

    _handleElementFocus: function()
    {
        if (!this._readOnly)
            this._container.focus();
    },

    /**
     * @return {Element}
     */
    defaultFocusedElement: function()
    {
        if (this._readOnly)
            return this.element;
        return this._container;
    },

    _updateSelectionOnStartEditing: function()
    {
        // focus() needs to go first for the case when the last selection was inside the editor and
        // the "Edit" button was clicked. In this case we bail at the check below, but the
        // editor does not receive the focus, thus "Esc" does not cancel editing until at least
        // one change has been made to the editor contents.
        this._container.focus();
        var selection = window.getSelection();
        if (selection.rangeCount) {
            var commonAncestorContainer = selection.getRangeAt(0).commonAncestorContainer;
            if (this._container.isSelfOrAncestor(commonAncestorContainer))
                return;
        }

        selection.removeAllRanges();
        var range = document.createRange();
        range.setStart(this._container, 0);
        range.setEnd(this._container, 0);
        selection.addRange(range);
    },

    /**
     * @param {WebInspector.TextRange} range
     */
    markAndRevealRange: function(range)
    {
        if (this._rangeToMark) {
            var markedLine = this._rangeToMark.startLine;
            delete this._rangeToMark;
            // Remove the marked region immediately.
            this.beginDomUpdates();
            var chunk = this.chunkForLine(markedLine);
            var wasExpanded = chunk.expanded();
            chunk.collapse();
            chunk.updateCollapsedLineRow();
            if (wasExpanded)
                chunk.expand();
            this.endDomUpdates();
        }

        if (range) {
            this._rangeToMark = range;
            this.revealLine(range.startLine);
            var chunk = this.makeLineAChunk(range.startLine);
            this._paintLines(chunk.startLine, chunk.startLine + 1);
            if (this._markedRangeElement)
                this._markedRangeElement.scrollIntoViewIfNeeded();
        }
        delete this._markedRangeElement;
    },

    /**
     * @param {number} lineNumber
     */
    highlightLine: function(lineNumber)
    {
        this.clearLineHighlight();
        this._highlightedLine = lineNumber;
        this.revealLine(lineNumber);

        if (!this._readOnly)
            this._restoreSelection(WebInspector.TextRange.createFromLocation(lineNumber, 0), false);

        this.addDecoration(lineNumber, "webkit-highlighted-line");
    },

    clearLineHighlight: function()
    {
        if (typeof this._highlightedLine === "number") {
            this.removeDecoration(this._highlightedLine, "webkit-highlighted-line");
            delete this._highlightedLine;
        }
    },

    _freeCachedElements: function()
    {
        this._cachedSpans = [];
        this._cachedTextNodes = [];
        this._cachedRows = [];
    },

    /**
     * @param {boolean} redo
     * @return {boolean}
     */
    _handleUndoRedo: function(redo)
    {
        if (this.readOnly())
            return false;

        this.beginUpdates();

        var range = redo ? this._textModel.redo() : this._textModel.undo();

        this.endUpdates();

        // Restore location post-repaint.
        if (range)
            this._restoreSelection(range, true);

        return true;
    },

    /**
     * @param {boolean} shiftKey
     * @return {boolean}
     */
    _handleTabKeyPress: function(shiftKey)
    {
        if (this.readOnly())
            return false;

        var selection = this.selection();
        if (!selection)
            return false;

        var range = selection.normalize();

        this.beginUpdates();

        var newRange;
        var rangeWasEmpty = range.isEmpty();
        if (shiftKey)
            newRange = this._textModel.unindentLines(range);
        else {
            if (rangeWasEmpty)
                newRange = this._textModel.editRange(range, WebInspector.settings.textEditorIndent.get());
            else
                newRange = this._textModel.indentLines(range);
        }

        this.endUpdates();
        if (rangeWasEmpty)
            newRange.startColumn = newRange.endColumn;
        this._restoreSelection(newRange, true);
        return true;
    },

    _handleEnterKey: function()
    {
        if (this.readOnly())
            return false;

        var range = this.selection();
        if (!range)
            return false;

        range = range.normalize();

        if (range.endColumn === 0)
            return false;

        var line = this._textModel.line(range.startLine);
        var linePrefix = line.substring(0, range.startColumn);
        var indentMatch = linePrefix.match(/^\s+/);
        var currentIndent = indentMatch ? indentMatch[0] : "";

        var textEditorIndent = WebInspector.settings.textEditorIndent.get();
        var indent = WebInspector.TextEditorModel.endsWithBracketRegex.test(linePrefix) ? currentIndent + textEditorIndent : currentIndent;

        if (!indent)
            return false;

        this.beginDomUpdates();

        var lineBreak = this._textModel.lineBreak;
        var newRange;
        if (range.isEmpty() && line.substr(range.endColumn - 1, 2) === '{}') {
            // {|}
            // becomes
            // {
            //     |
            // }
            newRange = this._textModel.editRange(range, lineBreak + indent + lineBreak + currentIndent);
            newRange.endLine--;
            newRange.endColumn += textEditorIndent.length;
        } else
            newRange = this._textModel.editRange(range, lineBreak + indent);

        this.endDomUpdates();
        this._restoreSelection(newRange.collapseToEnd(), true);

        return true;
    },

    /**
     * @param {number} lineNumber
     * @param {number} chunkNumber
     * @param {boolean=} createSuffixChunk
     * @return {Object}
     */
    splitChunkOnALine: function(lineNumber, chunkNumber, createSuffixChunk)
    {
        var selection = this.selection();
        var chunk = WebInspector.TextEditorChunkedPanel.prototype.splitChunkOnALine.call(this, lineNumber, chunkNumber, createSuffixChunk);
        this._restoreSelection(selection);
        return chunk;
    },

    beginDomUpdates: function()
    {
        if (!this._domUpdateCoalescingLevel)
            this._detachMutationObserver();
        WebInspector.TextEditorChunkedPanel.prototype.beginDomUpdates.call(this);
    },

    endDomUpdates: function()
    {
        WebInspector.TextEditorChunkedPanel.prototype.endDomUpdates.call(this);
        if (!this._domUpdateCoalescingLevel)
            this._attachMutationObserver();
    },

    buildChunks: function()
    {
        for (var i = 0; i < this._textModel.linesCount; ++i)
            this._textModel.removeAttribute(i, "highlight");

        WebInspector.TextEditorChunkedPanel.prototype.buildChunks.call(this);
    },

    /**
     * @param {number} startLine
     * @param {number} endLine
     * @return {WebInspector.TextEditorMainChunk}
     */
    createNewChunk: function(startLine, endLine)
    {
        return new WebInspector.TextEditorMainChunk(this, startLine, endLine);
    },

    /**
     * @param {number} fromIndex
     * @param {number} toIndex
     */
    expandChunks: function(fromIndex, toIndex)
    {
        var lastChunk = this._textChunks[toIndex - 1];
        var lastVisibleLine = lastChunk.startLine + lastChunk.linesCount;

        var selection = this.selection();

        this._muteHighlightListener = true;
        this._highlighter.highlight(lastVisibleLine);
        delete this._muteHighlightListener;

        WebInspector.TextEditorChunkedPanel.prototype.expandChunks.call(this, fromIndex, toIndex);

        this._restoreSelection(selection);
    },

    /**
     * @param {number} fromLine
     * @param {number} toLine
     */
    _highlightDataReady: function(fromLine, toLine)
    {
        if (this._muteHighlightListener)
            return;
        this._paintLines(fromLine, toLine, true /*restoreSelection*/);
    },

    /**
     * @param {number} fromLine
     * @param {number} toLine
     * @param {boolean=} restoreSelection
     */
    _paintLines: function(fromLine, toLine, restoreSelection)
    {
        var lineRows = [];
        var chunk;
        for (var lineNumber = fromLine; lineNumber < toLine; ++lineNumber) {
            if (!chunk || lineNumber < chunk.startLine || lineNumber >= chunk.startLine + chunk.linesCount)
                chunk = this.chunkForLine(lineNumber);
            var lineRow = chunk.expandedLineRow(lineNumber);
            if (!lineRow)
                continue;
            lineRows.push(lineRow);
        }
        if (lineRows.length === 0)
            return;

        var selection;
        if (restoreSelection)
            selection = this.selection();

        this._paintLineRows(lineRows);

        if (restoreSelection)
            this._restoreSelection(selection);
    },

    /**
     * @param {Array.<Element>} lineRows
     */
    _paintLineRows: function(lineRows)
    {
        var highlight = {};
        this.beginDomUpdates();
        for(var i = 0; i < this._highlightDescriptors.length; ++i) {
            var highlightDescriptor = this._highlightDescriptors[i];
            this._measureHighlightDescriptor(highlight, lineRows, highlightDescriptor);
        }

        for(var i = 0; i < lineRows.length; ++i)
            this._paintLine(lineRows[i], highlight[lineRows[i].lineNumber]);

        this.endDomUpdates();
    },

    /**
     * @param {Object.<number, Array.<WebInspector.TextEditorMainPanel.LineOverlayHighlight>>} highlight
     * @param {Array.<Element>} lineRows
     * @param {WebInspector.TextEditorMainPanel.HighlightDescriptor} highlightDescriptor
     */
    _measureHighlightDescriptor: function(highlight, lineRows, highlightDescriptor)
    {
        var rowsToMeasure = [];
        for(var i = 0; i < lineRows.length; ++i) {
            var lineRow = lineRows[i];
            var line = this._textModel.line(lineRow.lineNumber);
            var ranges = highlightDescriptor.rangesForLine(lineRow.lineNumber, line);
            if (ranges.length === 0)
                continue;
            for(var j = 0; j < ranges.length; ++j)
                ranges[j].token = "measure-span";

            this._renderRanges(lineRow, line, ranges);
            rowsToMeasure.push(lineRow);
        }

        for(var i = 0; i < rowsToMeasure.length; ++i) {
            var lineRow = rowsToMeasure[i];
            var lineNumber = lineRow.lineNumber;
            var metrics = this._measureSpans(lineRow);

            if (!highlight[lineNumber])
                highlight[lineNumber] = [];

            highlight[lineNumber].push(new WebInspector.TextEditorMainPanel.LineOverlayHighlight(metrics, highlightDescriptor.cssClass()));
        }
    },

    /**
     * @param {Element} lineRow
     * @return {Array.<WebInspector.TextEditorMainPanel.ElementMetrics>}
     */
    _measureSpans: function(lineRow)
    {
        var spans = lineRow.getElementsByClassName("webkit-measure-span");
        var metrics = [];
        for(var i = 0; i < spans.length; ++i)
            metrics.push(new WebInspector.TextEditorMainPanel.ElementMetrics(spans[i]));
        return metrics;
    },

    /**
     * @param {Element} lineRow
     * @param {WebInspector.TextEditorMainPanel.LineOverlayHighlight} highlight
     */
    _appendOverlayHighlight: function(lineRow, highlight)
    {
        var metrics = highlight.metrics;
        var cssClass = highlight.cssClass;
        for(var i = 0; i < metrics.length; ++i) {
            var highlightSpan = document.createElement("span");
            highlightSpan._isOverlayHighlightElement = true;
            highlightSpan.addStyleClass(cssClass);
            highlightSpan.style.left = metrics[i].left + "px";
            highlightSpan.style.width = metrics[i].width + "px";
            highlightSpan.style.height = metrics[i].height + "px";
            highlightSpan.addStyleClass("text-editor-overlay-highlight");
            lineRow.insertBefore(highlightSpan, lineRow.decorationsElement);
        }
    },

    /**
     * @param {Element} lineRow
     * @param {string} line
     * @param {Array.<{startColumn: number, endColumn: number, token: ?string}>} ranges
     * @param {boolean=} splitWhitespaceSequences
     */
    _renderRanges: function(lineRow, line, ranges, splitWhitespaceSequences)
    {
        var decorationsElement = lineRow.decorationsElement;

        if (!decorationsElement)
            lineRow.removeChildren();
        else {
            while (true) {
                var child = lineRow.firstChild;
                if (!child || child === decorationsElement)
                    break;
                lineRow.removeChild(child);
            }
        }

        if (!line)
            lineRow.insertBefore(document.createElement("br"), decorationsElement);

        var plainTextStart = 0;
        for(var i = 0; i < ranges.length; i++) {
            var rangeStart = ranges[i].startColumn;
            var rangeEnd = ranges[i].endColumn;

            if (plainTextStart < rangeStart) {
                this._insertSpanBefore(lineRow, decorationsElement, line.substring(plainTextStart, rangeStart));
            }

            if (splitWhitespaceSequences && ranges[i].token === "whitespace")
                this._renderWhitespaceCharsWithFixedSizeSpans(lineRow, decorationsElement, rangeEnd - rangeStart + 1);
            else
                this._insertSpanBefore(lineRow, decorationsElement, line.substring(rangeStart, rangeEnd + 1), ranges[i].token ? "webkit-" + ranges[i].token : "");
            plainTextStart = rangeEnd + 1;
        }
        if (plainTextStart < line.length) {
            this._insertSpanBefore(lineRow, decorationsElement, line.substring(plainTextStart, line.length));
        }
    },

    /**
     * @param {Element} lineRow
     * @param {Element} decorationsElement
     * @param {number} length
     */
    _renderWhitespaceCharsWithFixedSizeSpans: function(lineRow, decorationsElement, length)
    {
        for (var whitespaceLength = 16; whitespaceLength > 0; whitespaceLength >>= 1) {
            var cssClass = "webkit-whitespace webkit-whitespace-" + whitespaceLength;
            for (; length >= whitespaceLength; length -= whitespaceLength)
                this._insertSpanBefore(lineRow, decorationsElement, WebInspector.TextEditorMainPanel._ConsecutiveWhitespaceChars[whitespaceLength], cssClass);
        }
    },

    /**
     * @param {Element} lineRow
     * @param {Array.<WebInspector.TextEditorMainPanel.LineOverlayHighlight>} overlayHighlight
     */
    _paintLine: function(lineRow, overlayHighlight)
    {
        var lineNumber = lineRow.lineNumber;

        this.beginDomUpdates();
        try {
            var syntaxHighlight = this._textModel.getAttribute(lineNumber, "highlight");

            var line = this._textModel.line(lineNumber);
            var ranges = syntaxHighlight ? syntaxHighlight.ranges : [];
            this._renderRanges(lineRow, line, ranges, this._showWhitespace);

            if (overlayHighlight)
                for(var i = 0; i < overlayHighlight.length; ++i)
                    this._appendOverlayHighlight(lineRow, overlayHighlight[i]);
        } finally {
            if (this._rangeToMark && this._rangeToMark.startLine === lineNumber)
                this._markedRangeElement = WebInspector.highlightSearchResult(lineRow, this._rangeToMark.startColumn, this._rangeToMark.endColumn - this._rangeToMark.startColumn);
            this.endDomUpdates();
        }
    },

    /**
     * @param {Element} lineRow
     */
    _releaseLinesHighlight: function(lineRow)
    {
        if (!lineRow)
            return;
        if ("spans" in lineRow) {
            var spans = lineRow.spans;
            for (var j = 0; j < spans.length; ++j)
                this._cachedSpans.push(spans[j]);
            delete lineRow.spans;
        }
        if ("textNodes" in lineRow) {
            var textNodes = lineRow.textNodes;
            for (var j = 0; j < textNodes.length; ++j)
                this._cachedTextNodes.push(textNodes[j]);
            delete lineRow.textNodes;
        }
        this._cachedRows.push(lineRow);
    },

    /**
     * @param {?Node=} lastUndamagedLineRow
     * @return {WebInspector.TextRange}
     */
    selection: function(lastUndamagedLineRow)
    {
        var selection = window.getSelection();
        if (!selection.rangeCount)
            return null;
        // Selection may be outside of the editor.
        if (!this._container.isAncestor(selection.anchorNode) || !this._container.isAncestor(selection.focusNode))
            return null;
        // Selection may be inside one of decorations.
        if (selection.focusNode.enclosingNodeOrSelfWithClass("webkit-line-decorations", this._container))
            return null;
        var start = this._selectionToPosition(selection.anchorNode, selection.anchorOffset, lastUndamagedLineRow);
        var end = selection.isCollapsed ? start : this._selectionToPosition(selection.focusNode, selection.focusOffset, lastUndamagedLineRow);
        return new WebInspector.TextRange(start.line, start.column, end.line, end.column);
    },

    lastSelection: function()
    {
        return this._lastSelection;
    },

    /**
     * @param {boolean=} scrollIntoView
     */
    _restoreSelection: function(range, scrollIntoView)
    {
        if (!range)
            return;

        var start = this._positionToSelection(range.startLine, range.startColumn);
        var end = range.isEmpty() ? start : this._positionToSelection(range.endLine, range.endColumn);
        window.getSelection().setBaseAndExtent(start.container, start.offset, end.container, end.offset);

        if (scrollIntoView) {
            for (var node = end.container; node; node = node.parentElement) {
                if (node.scrollIntoViewIfNeeded) {
                    node.scrollIntoViewIfNeeded();
                    break;
                }
            }
        }
        this._lastSelection = range;
    },

    /**
     * @param {Node} container
     * @param {number} offset
     * @param {?Node=} lastUndamagedLineRow
     * @return {{line: number, column: number}}
     */
    _selectionToPosition: function(container, offset, lastUndamagedLineRow)
    {
        if (container === this._container && offset === 0)
            return { line: 0, column: 0 };
        if (container === this._container && offset === 1)
            return { line: this._textModel.linesCount - 1, column: this._textModel.lineLength(this._textModel.linesCount - 1) };

        // This method can be called on the damaged DOM (when DOM does not match model).
        // We need to start counting lines from the first undamaged line if it is given.
        var lineNumber;
        var column = 0;
        var node;
        var scopeNode;
        if (lastUndamagedLineRow === null) {
             // Last undamaged row is given, but is null - force traverse from the beginning
            node = this._container.firstChild;
            scopeNode = this._container;
            lineNumber = 0;
        } else {
            var lineRow = this._enclosingLineRowOrSelf(container);
            if (!lastUndamagedLineRow || (typeof lineRow.lineNumber === "number" && lineRow.lineNumber <= lastUndamagedLineRow.lineNumber)) {
                // DOM is consistent (or we belong to the first damaged row)- lookup the row we belong to and start with it.
                node = lineRow;
                scopeNode = node;
                lineNumber = node.lineNumber;
            } else {
                // Start with the node following undamaged row. It corresponds to lineNumber + 1.
                node = lastUndamagedLineRow.nextSibling;
                scopeNode = this._container;
                lineNumber = lastUndamagedLineRow.lineNumber + 1;
            }
        }

        // Fast return the line start.
        if (container === node && offset === 0)
            return { line: lineNumber, column: 0 };

        // Traverse text and increment lineNumber / column.
        for (; node && node !== container; node = node.traverseNextNode(scopeNode)) {
            if (node.nodeName.toLowerCase() === "br") {
                lineNumber++;
                column = 0;
            } else if (node.nodeType === Node.TEXT_NODE) {
                var text = node.textContent;
                for (var i = 0; i < text.length; ++i) {
                    if (text.charAt(i) === "\n") {
                        lineNumber++;
                        column = 0;
                    } else
                        column++;
                }
            }
        }

        // We reached our container node, traverse within itself until we reach given offset.
        if (node === container && offset) {
            var text = node.textContent;
            // In case offset == 1 and lineRow is a chunk div, we need to traverse it all.
            var textOffset = (node._chunk && offset === 1) ? text.length : offset;
            for (var i = 0; i < textOffset; ++i) {
                if (text.charAt(i) === "\n") {
                    lineNumber++;
                    column = 0;
                } else
                    column++;
            }
        }
        return { line: lineNumber, column: column };
    },

    /**
     * @param {number} line
     * @param {number} column
     * @return {{container: Element, offset: number}}
     */
    _positionToSelection: function(line, column)
    {
        var chunk = this.chunkForLine(line);
        // One-lined collapsed chunks may still stay highlighted.
        var lineRow = chunk.linesCount === 1 ? chunk.element : chunk.expandedLineRow(line);
        if (lineRow)
            var rangeBoundary = lineRow.rangeBoundaryForOffset(column);
        else {
            var offset = column;
            for (var i = chunk.startLine; i < line && i < this._textModel.linesCount; ++i)
                offset += this._textModel.lineLength(i) + 1; // \n
            lineRow = chunk.element;
            if (lineRow.firstChild)
                var rangeBoundary = { container: lineRow.firstChild, offset: offset };
            else
                var rangeBoundary = { container: lineRow, offset: 0 };
        }
        return rangeBoundary;
    },

    /**
     * @param {Node} element
     * @return {?Node}
     */
    _enclosingLineRowOrSelf: function(element)
    {
        var lineRow = element.enclosingNodeOrSelfWithClass("webkit-line-content");
        if (lineRow)
            return lineRow;

        for (lineRow = element; lineRow; lineRow = lineRow.parentElement) {
            if (lineRow.parentElement === this._container)
                return lineRow;
        }
        return null;
    },

    /**
     * @param {Element} element
     * @param {Element} oldChild
     * @param {string} content
     * @param {string=} className
     */
    _insertSpanBefore: function(element, oldChild, content, className)
    {
        if (className === "html-resource-link" || className === "html-external-link") {
            element.insertBefore(this._createLink(content, className === "html-external-link"), oldChild);
            return;
        }

        var span = this._cachedSpans.pop() || document.createElement("span");
        if (!className)
            span.removeAttribute("class");
        else
            span.className = className;
        if (WebInspector.FALSE) // For paint debugging.
            span.addStyleClass("debug-fadeout");
        span.textContent = content;
        element.insertBefore(span, oldChild);
        if (!("spans" in element))
            element.spans = [];
        element.spans.push(span);
    },

    /**
     * @param {Element} element
     * @param {Element} oldChild
     * @param {string} text
     */
    _insertTextNodeBefore: function(element, oldChild, text)
    {
        var textNode = this._cachedTextNodes.pop();
        if (textNode)
            textNode.nodeValue = text;
        else
            textNode = document.createTextNode(text);
        element.insertBefore(textNode, oldChild);
        if (!("textNodes" in element))
            element.textNodes = [];
        element.textNodes.push(textNode);
    },

    /**
     * @param {string} content
     * @param {boolean} isExternal
     * @return {Element}
     */
    _createLink: function(content, isExternal)
    {
        var quote = content.charAt(0);
        if (content.length > 1 && (quote === "\"" || quote === "'"))
            content = content.substring(1, content.length - 1);
        else
            quote = null;

        var span = document.createElement("span");
        span.className = "webkit-html-attribute-value";
        if (quote)
            span.appendChild(document.createTextNode(quote));
        span.appendChild(this._delegate.createLink(content, isExternal));
        if (quote)
            span.appendChild(document.createTextNode(quote));
        return span;
    },

    /**
     * @param {Array.<WebKitMutation>} mutations
     */
    _handleMutations: function(mutations)
    {
        if (this._readOnly) {
            delete this._keyDownCode;
            return;
        }

        // Annihilate noop BR addition + removal that takes place upon line removal.
        var filteredMutations = mutations.slice();
        var addedBRs = new Map();
        for (var i = 0; i < mutations.length; ++i) {
            var mutation = mutations[i];
            if (mutation.type !== "childList")
                continue;
            if (mutation.addedNodes.length === 1 && mutation.addedNodes[0].nodeName === "BR")
                addedBRs.put(mutation.addedNodes[0], mutation);
            else if (mutation.removedNodes.length === 1 && mutation.removedNodes[0].nodeName === "BR") {
                var noopMutation = addedBRs.get(mutation.removedNodes[0]);
                if (noopMutation) {
                    filteredMutations.remove(mutation);
                    filteredMutations.remove(noopMutation);
                }
            }
        }

        var dirtyLines;
        for (var i = 0; i < filteredMutations.length; ++i) {
            var mutation = filteredMutations[i];
            var changedNodes = [];
            if (mutation.type === "childList" && mutation.addedNodes.length)
                changedNodes = Array.prototype.slice.call(mutation.addedNodes);
            else if (mutation.type === "childList" && mutation.removedNodes.length)
                changedNodes = Array.prototype.slice.call(mutation.removedNodes);
            changedNodes.push(mutation.target);

            for (var j = 0; j < changedNodes.length; ++j) {
                var lines = this._collectDirtyLines(mutation, changedNodes[j]);
                if (!lines)
                    continue;
                if (!dirtyLines) {
                    dirtyLines = lines;
                    continue;
                }
                dirtyLines.start = Math.min(dirtyLines.start, lines.start);
                dirtyLines.end = Math.max(dirtyLines.end, lines.end);
            }
        }
        if (dirtyLines) {
            delete this._rangeToMark;
            this._applyDomUpdates(dirtyLines);
        }

        this._assertDOMMatchesTextModel();

        delete this._keyDownCode;
    },

    /**
     * @param {WebKitMutation} mutation
     * @param {Node} target
     * @return {?Object}
     */
    _collectDirtyLines: function(mutation, target)
    {
        var lineRow = this._enclosingLineRowOrSelf(target);
        if (!lineRow)
            return null;

        if (lineRow.decorationsElement && lineRow.decorationsElement.isSelfOrAncestor(target)) {
            if (this._syncDecorationsForLineListener)
                this._syncDecorationsForLineListener(lineRow.lineNumber);
            return null;
        }

        if (typeof lineRow.lineNumber !== "number")
            return null;

        var startLine = lineRow.lineNumber;
        var endLine = lineRow._chunk ? lineRow._chunk.endLine - 1 : lineRow.lineNumber;
        return { start: startLine, end: endLine };
    },

    /**
     * @param {Object} dirtyLines
     */
    _applyDomUpdates: function(dirtyLines)
    {
        var lastUndamagedLineNumber = dirtyLines.start - 1; // Can be -1
        var firstUndamagedLineNumber = dirtyLines.end + 1; // Can be this._textModel.linesCount

        var lastUndamagedLineChunk = lastUndamagedLineNumber >= 0 ? this._textChunks[this.chunkNumberForLine(lastUndamagedLineNumber)] : null;
        var firstUndamagedLineChunk = firstUndamagedLineNumber < this._textModel.linesCount ? this._textChunks[this.chunkNumberForLine(firstUndamagedLineNumber)] : null;

        var collectLinesFromNode = lastUndamagedLineChunk ? lastUndamagedLineChunk.lineRowContainingLine(lastUndamagedLineNumber) : null;
        var collectLinesToNode = firstUndamagedLineChunk ? firstUndamagedLineChunk.lineRowContainingLine(firstUndamagedLineNumber) : null;
        var lines = this._collectLinesFromDOM(collectLinesFromNode, collectLinesToNode);

        var startLine = dirtyLines.start;
        var endLine = dirtyLines.end;

        var originalSelection = this._lastSelection;
        var editInfo = this._guessEditRangeBasedOnSelection(startLine, endLine, lines);
        if (!editInfo) {
            if (WebInspector.debugDefaultTextEditor)
                console.warn("Falling back to expensive edit");
            var range = new WebInspector.TextRange(startLine, 0, endLine, this._textModel.lineLength(endLine));
            if (!lines.length) {
                // Entire damaged area has collapsed. Replace everything between start and end lines with nothing.
                editInfo = new WebInspector.DefaultTextEditor.EditInfo(this._textModel.growRangeRight(range), "");
            } else
                editInfo = new WebInspector.DefaultTextEditor.EditInfo(range, lines.join("\n"));
        }

        var selection = this.selection(collectLinesFromNode);

        // Unindent after block
        if (editInfo.text === "}" && editInfo.range.isEmpty() && selection.isEmpty() && !this._textModel.line(editInfo.range.endLine).trim()) {
            var offset = this._closingBlockOffset(editInfo.range);
            if (offset >= 0) {
                editInfo.range.startColumn = offset;
                selection.startColumn = offset + 1;
                selection.endColumn = offset + 1;
            }
        }

        this._textModel.editRange(editInfo.range, editInfo.text, originalSelection);
        this._restoreSelection(selection);
    },

    /**
     * @param {number} startLine
     * @param {number} endLine
     * @param {Array.<string>} lines
     * @return {?WebInspector.DefaultTextEditor.EditInfo}
     */
    _guessEditRangeBasedOnSelection: function(startLine, endLine, lines)
    {
        // Analyze input data
        var textInputData = this._textInputData;
        delete this._textInputData;
        var isBackspace = this._keyDownCode === WebInspector.KeyboardShortcut.Keys.Backspace.code;
        var isDelete = this._keyDownCode === WebInspector.KeyboardShortcut.Keys.Delete.code;

        if (!textInputData && (isDelete || isBackspace))
            textInputData = "";

        // Return if there is no input data or selection
        if (typeof textInputData === "undefined" || !this._lastSelection)
            return null;

        // Adjust selection based on the keyboard actions (grow for backspace, etc.).
        textInputData = textInputData || "";
        var range = this._lastSelection.normalize();
        if (isBackspace && range.isEmpty())
            range = this._textModel.growRangeLeft(range);
        else if (isDelete && range.isEmpty())
            range = this._textModel.growRangeRight(range);

        // Test that selection intersects damaged lines
        if (startLine > range.endLine || endLine < range.startLine)
            return null;

        var replacementLineCount = textInputData.split("\n").length - 1;
        var lineCountDelta = replacementLineCount - range.linesCount;
        if (startLine + lines.length - endLine - 1 !== lineCountDelta)
            return null;

        // Clone text model of the size that fits both: selection before edit and the damaged lines after edit.
        var cloneFromLine = Math.min(range.startLine, startLine);
        var postLastLine = startLine + lines.length + lineCountDelta;
        var cloneToLine = Math.min(Math.max(postLastLine, range.endLine) + 1, this._textModel.linesCount);
        var domModel = this._textModel.slice(cloneFromLine, cloneToLine);
        domModel.editRange(range.shift(-cloneFromLine), textInputData);

        // Then we'll test if this new model matches the DOM lines.
        for (var i = 0; i < lines.length; ++i) {
            if (domModel.line(i + startLine - cloneFromLine) !== lines[i])
                return null;
        }
        return new WebInspector.DefaultTextEditor.EditInfo(range, textInputData);
    },

    _assertDOMMatchesTextModel: function()
    {
        if (!WebInspector.debugDefaultTextEditor)
            return;

        console.assert(this.element.innerText === this._textModel.text() + "\n", "DOM does not match model.");
        for (var lineRow = this._container.firstChild; lineRow; lineRow = lineRow.nextSibling) {
            var lineNumber = lineRow.lineNumber;
            if (typeof lineNumber !== "number") {
                console.warn("No line number on line row");
                continue;
            }
            if (lineRow._chunk) {
                var chunk = lineRow._chunk;
                console.assert(lineNumber === chunk.startLine);
                var chunkText = this._textModel.copyRange(new WebInspector.TextRange(chunk.startLine, 0, chunk.endLine - 1, this._textModel.lineLength(chunk.endLine - 1)));
                if (chunkText !== lineRow.textContent)
                    console.warn("Chunk is not matching: %d %O", lineNumber, lineRow);
            } else if (this._textModel.line(lineNumber) !== lineRow.textContent)
                console.warn("Line is not matching: %d %O", lineNumber, lineRow);
        }
    },

    /**
     * @param {WebInspector.TextRange} oldRange
     * @return {number}
     */
    _closingBlockOffset: function(oldRange)
    {
        var leftBrace = this._braceMatcher.findLeftCandidate(oldRange.startLine, oldRange.startColumn);
        if (!leftBrace || leftBrace.token !== "block-start")
            return -1;
        var lineContent = this._textModel.line(leftBrace.lineNumber);
        return lineContent.length - lineContent.trimLeft().length;
    },

    /**
     * @param {WebInspector.TextRange} oldRange
     * @param {WebInspector.TextRange} newRange
     */
    textChanged: function(oldRange, newRange)
    {
        this.beginDomUpdates();
        this._removeDecorationsInRange(oldRange);
        this._updateChunksForRanges(oldRange, newRange);
        this._updateHighlightsForRange(newRange);
        this.endDomUpdates();
    },

    /**
     * @param {WebInspector.TextRange} range
     */
    _removeDecorationsInRange: function(range)
    {
        for (var i = this.chunkNumberForLine(range.startLine); i < this._textChunks.length; ++i) {
            var chunk = this._textChunks[i];
            if (chunk.startLine > range.endLine)
                break;
            chunk.removeAllDecorations();
        }
    },

    /**
     * @param {WebInspector.TextRange} oldRange
     * @param {WebInspector.TextRange} newRange
     */
    _updateChunksForRanges: function(oldRange, newRange)
    {
        var firstDamagedChunkNumber = this.chunkNumberForLine(oldRange.startLine);
        var lastDamagedChunkNumber = firstDamagedChunkNumber;
        while (lastDamagedChunkNumber + 1 < this._textChunks.length) {
            if (this._textChunks[lastDamagedChunkNumber + 1].startLine > oldRange.endLine)
                break;
            ++lastDamagedChunkNumber;
        }

        var firstDamagedChunk = this._textChunks[firstDamagedChunkNumber];
        var lastDamagedChunk = this._textChunks[lastDamagedChunkNumber];

        var linesDiff = newRange.linesCount - oldRange.linesCount;

        // First, detect chunks that have not been modified and simply shift them.
        if (linesDiff) {
            for (var chunkNumber = lastDamagedChunkNumber + 1; chunkNumber < this._textChunks.length; ++chunkNumber)
                this._textChunks[chunkNumber].startLine += linesDiff;
        }

        // Remove damaged chunks from DOM and from textChunks model.
        var lastUndamagedChunk = firstDamagedChunkNumber > 0 ? this._textChunks[firstDamagedChunkNumber - 1] : null;
        var firstUndamagedChunk = lastDamagedChunkNumber + 1 < this._textChunks.length ? this._textChunks[lastDamagedChunkNumber + 1] : null;

        var removeDOMFromNode = lastUndamagedChunk ? lastUndamagedChunk.lastElement().nextSibling : this._container.firstChild;
        var removeDOMToNode = firstUndamagedChunk ? firstUndamagedChunk.firstElement() : null;

        // Fast case - patch single expanded chunk that did not grow / shrink during edit.
        if (!linesDiff && firstDamagedChunk === lastDamagedChunk && firstDamagedChunk._expandedLineRows) {
            var lastUndamagedLineRow = lastDamagedChunk.expandedLineRow(oldRange.startLine - 1);
            var firstUndamagedLineRow = firstDamagedChunk.expandedLineRow(oldRange.endLine + 1);
            var localRemoveDOMFromNode = lastUndamagedLineRow ? lastUndamagedLineRow.nextSibling : removeDOMFromNode;
            var localRemoveDOMToNode = firstUndamagedLineRow || removeDOMToNode;
            removeSubsequentNodes(localRemoveDOMFromNode, localRemoveDOMToNode);
            for (var i = newRange.startLine; i < newRange.endLine + 1; ++i) {
                var row = firstDamagedChunk._createRow(i);
                firstDamagedChunk._expandedLineRows[i - firstDamagedChunk.startLine] = row;
                this._container.insertBefore(row, localRemoveDOMToNode);
            }
            firstDamagedChunk.updateCollapsedLineRow();
            this._assertDOMMatchesTextModel();
            return;
        }

        removeSubsequentNodes(removeDOMFromNode, removeDOMToNode);
        this._textChunks.splice(firstDamagedChunkNumber, lastDamagedChunkNumber - firstDamagedChunkNumber + 1);

        // Compute damaged chunks span
        var startLine = firstDamagedChunk.startLine;
        var endLine = lastDamagedChunk.endLine + linesDiff;
        var lineSpan = endLine - startLine;

        // Re-create chunks for damaged area.
        var insertionIndex = firstDamagedChunkNumber;
        var chunkSize = Math.ceil(lineSpan / Math.ceil(lineSpan / this._defaultChunkSize));

        for (var i = startLine; i < endLine; i += chunkSize) {
            var chunk = this.createNewChunk(i, Math.min(endLine, i + chunkSize));
            this._textChunks.splice(insertionIndex++, 0, chunk);
            this._container.insertBefore(chunk.element, removeDOMToNode);
        }

        this._assertDOMMatchesTextModel();
    },

    /**
     * @param {WebInspector.TextRange} range
     */
    _updateHighlightsForRange: function(range)
    {
        var visibleFrom = this.scrollTop();
        var visibleTo = visibleFrom + this.clientHeight();

        var result = this.findVisibleChunks(visibleFrom, visibleTo);
        var chunk = this._textChunks[result.end - 1];
        var lastVisibleLine = chunk.startLine + chunk.linesCount;

        lastVisibleLine = Math.max(lastVisibleLine, range.endLine + 1);
        lastVisibleLine = Math.min(lastVisibleLine, this._textModel.linesCount);

        var updated = this._highlighter.updateHighlight(range.startLine, lastVisibleLine);
        if (!updated) {
            // Highlights for the chunks below are invalid, so just collapse them.
            for (var i = this.chunkNumberForLine(range.startLine); i < this._textChunks.length; ++i)
                this._textChunks[i].collapse();
        }

        this.repaintAll();
    },

    /**
     * @param {Node} from
     * @param {Node} to
     * @return {Array.<string>}
     */
    _collectLinesFromDOM: function(from, to)
    {
        var textContents = [];
        var hasContent = false;
        for (var node = from ? from.nextSibling : this._container; node && node !== to; node = node.traverseNextNode(this._container)) {
            // Skip all children of the decoration container and overlay highlight spans.
            while (node && node !== to && (node._isDecorationsElement || node._isOverlayHighlightElement))
                node = node.nextSibling;
            if (!node || node === to)
                break;

            hasContent = true;
            if (node.nodeName.toLowerCase() === "br")
                textContents.push("\n");
            else if (node.nodeType === Node.TEXT_NODE)
                textContents.push(node.textContent);
        }
        if (!hasContent)
            return [];

        var textContent = textContents.join("");
        // The last \n (if any) does not "count" in a DIV.
        textContent = textContent.replace(/\n$/, "");

        return textContent.split("\n");
    },

    /**
     * @param {Event} event
     */
    _handleSelectionChange: function(event)
    {
        var textRange = this.selection();
        if (textRange)
            this._lastSelection = textRange;

        this._tokenHighlighter.handleSelectionChange(textRange);
        this._braceHighlighter.handleSelectionChange(textRange);
        this._delegate.selectionChanged(textRange);
    },

    /**
     * @param {Event} event
     */
    _handleTextInput: function(event)
    {
        this._textInputData = event.data;
    },

    /**
     * @param {number} shortcutKey
     * @param {Event} event
     */
    handleKeyDown: function(shortcutKey, event)
    {
        var handler = this._shortcuts[shortcutKey];
        if (handler && handler()) {
            event.consume(true);
            return;
        }

        this._keyDownCode = event.keyCode;
    },

    /**
     * @param {Event} event
     */
    _handleCut: function(event)
    {
        this._keyDownCode = WebInspector.KeyboardShortcut.Keys.Delete.code;
    },

    /**
     * @param {number} scrollTop
     * @param {number} clientHeight
     * @param {number} chunkSize
     */
    overrideViewportForTest: function(scrollTop, clientHeight, chunkSize)
    {
        this._scrollTopOverrideForTest = scrollTop;
        this._clientHeightOverrideForTest = clientHeight;
        this._defaultChunkSize = chunkSize;
    },

    __proto__: WebInspector.TextEditorChunkedPanel.prototype
}

/**
 * @interface
 */
WebInspector.TextEditorMainPanel.HighlightDescriptor = function() { }

WebInspector.TextEditorMainPanel.HighlightDescriptor.prototype = {
    /**
     * @param {number} lineNumber
     * @param {string} line
     * @return {boolean}
     */
    affectsLine: function(lineNumber, line) { return false; },

    /**
     * @param {number} lineNumber
     * @param {string} line
     * @return {Array.<{startColumn: number, endColumn: number}>}
     */
    rangesForLine: function(lineNumber, line) { return []; },

    /**
     * @return {string}
     */
    cssClass: function() { return ""; },
}

/**
 * @constructor
 * @implements {WebInspector.TextEditorMainPanel.HighlightDescriptor}
 */
WebInspector.TextEditorMainPanel.RegexHighlightDescriptor = function(regex, cssClass)
{
    this._cssClass = cssClass;
    this._regex = regex;
}

WebInspector.TextEditorMainPanel.RegexHighlightDescriptor.prototype = {
    /**
     * @param {number} lineNumber
     * @param {string} line
     * @return {boolean}
     */
    affectsLine: function(lineNumber, line)
    {
        this._regex.lastIndex = 0;
        return this._regex.test(line);
    },

    /**
     * @param {number} lineNumber
     * @param {string} line
     * @return {Array.<{startColumn: number, endColumn: number}>}
     */
    rangesForLine: function(lineNumber, line)
    {
        var ranges = [];
        var regexResult;
        this._regex.lastIndex = 0;
        while (regexResult = this._regex.exec(line)) {
            ranges.push({
                startColumn: regexResult.index,
                endColumn: regexResult.index + regexResult[0].length - 1
            });
        }
        return ranges;
    },

    /**
     * @return {string}
     */
    cssClass: function()
    {
        return this._cssClass;
    }
}

/**
 * @constructor
 * @implements {WebInspector.TextEditorMainPanel.HighlightDescriptor}
 * @param {WebInspector.TextRange} range
 * @param {string} cssClass
 */
WebInspector.TextEditorMainPanel.RangeHighlightDescriptor = function(range, cssClass)
{
    this._cssClass = cssClass;
    this._range = range;
}

WebInspector.TextEditorMainPanel.RangeHighlightDescriptor.prototype = {
    /**
     * @param {number} lineNumber
     * @param {string} line
     * @return {boolean}
     */
    affectsLine: function(lineNumber, line)
    {
        return this._range.startLine <= lineNumber && lineNumber <= this._range.endLine && line.length > 0;
    },

    /**
     * @param {number} lineNumber
     * @param {string} line
     * @return {Array.<{startColumn: number, endColumn: number}>}
     */
    rangesForLine: function(lineNumber, line)
    {
        if (!this.affectsLine(lineNumber, line))
            return [];

        var startColumn = lineNumber === this._range.startLine ? this._range.startColumn : 0;
        var endColumn = lineNumber === this._range.endLine ? Math.min(this._range.endColumn, line.length) : line.length;
        return [{
            startColumn: startColumn,
            endColumn: endColumn
        }];
    },

    /**
     * @return {string}
     */
    cssClass: function()
    {
        return this._cssClass;
    }
}

/**
 * @constructor
 * @param {Element} element
 */
WebInspector.TextEditorMainPanel.ElementMetrics = function(element)
{
    this.width = element.offsetWidth;
    this.height = element.offsetHeight;
    this.left = element.offsetLeft;
}

/**
 * @constructor
 * @param {Array.<WebInspector.TextEditorMainPanel.ElementMetrics>} metrics
 * @param {string} cssClass
 */
WebInspector.TextEditorMainPanel.LineOverlayHighlight = function(metrics, cssClass)
{
    this.metrics = metrics;
    this.cssClass = cssClass;
}

/**
 * @constructor
 * @param {WebInspector.TextEditorChunkedPanel} chunkedPanel
 * @param {number} startLine
 * @param {number} endLine
 */
WebInspector.TextEditorMainChunk = function(chunkedPanel, startLine, endLine)
{
    this._chunkedPanel = chunkedPanel;
    this._textModel = chunkedPanel._textModel;

    this.element = document.createElement("div");
    this.element.lineNumber = startLine;
    this.element.className = "webkit-line-content";
    this.element._chunk = this;

    this._startLine = startLine;
    endLine = Math.min(this._textModel.linesCount, endLine);
    this.linesCount = endLine - startLine;

    this._expanded = false;

    this.updateCollapsedLineRow();
}

WebInspector.TextEditorMainChunk.prototype = {
    /**
     * @param {Element|string} decoration
     */
    addDecoration: function(decoration)
    {
        this._chunkedPanel.beginDomUpdates();
        if (typeof decoration === "string")
            this.element.addStyleClass(decoration);
        else {
            if (!this.element.decorationsElement) {
                this.element.decorationsElement = document.createElement("div");
                this.element.decorationsElement.className = "webkit-line-decorations";
                this.element.decorationsElement._isDecorationsElement = true;
                this.element.appendChild(this.element.decorationsElement);
            }
            this.element.decorationsElement.appendChild(decoration);
        }
        this._chunkedPanel.endDomUpdates();
    },

    /**
     * @param {string|Element} decoration
     */
    removeDecoration: function(decoration)
    {
        this._chunkedPanel.beginDomUpdates();
        if (typeof decoration === "string")
            this.element.removeStyleClass(decoration);
        else if (this.element.decorationsElement)
            this.element.decorationsElement.removeChild(decoration);
        this._chunkedPanel.endDomUpdates();
    },

    removeAllDecorations: function()
    {
        this._chunkedPanel.beginDomUpdates();
        this.element.className = "webkit-line-content";
        if (this.element.decorationsElement) {
            if (this.element.decorationsElement.parentElement)
                this.element.removeChild(this.element.decorationsElement);
            delete this.element.decorationsElement;
        }
        this._chunkedPanel.endDomUpdates();
    },

    /**
     * @return {boolean}
     */
    isDecorated: function()
    {
        return this.element.className !== "webkit-line-content" || !!(this.element.decorationsElement && this.element.decorationsElement.firstChild);
    },

    /**
     * @return {number}
     */
    get startLine()
    {
        return this._startLine;
    },

    /**
     * @return {number}
     */
    get endLine()
    {
        return this._startLine + this.linesCount;
    },

    set startLine(startLine)
    {
        this._startLine = startLine;
        this.element.lineNumber = startLine;
        if (this._expandedLineRows) {
            for (var i = 0; i < this._expandedLineRows.length; ++i)
                this._expandedLineRows[i].lineNumber = startLine + i;
        }
    },

    /**
     * @return {boolean}
     */
    expanded: function()
    {
        return this._expanded;
    },

    expand: function()
    {
        if (this._expanded)
            return;

        this._expanded = true;

        if (this.linesCount === 1) {
            this._chunkedPanel._paintLines(this.startLine, this.startLine + 1);
            return;
        }

        this._chunkedPanel.beginDomUpdates();

        this._expandedLineRows = [];
        var parentElement = this.element.parentElement;
        for (var i = this.startLine; i < this.startLine + this.linesCount; ++i) {
            var lineRow = this._createRow(i);
            parentElement.insertBefore(lineRow, this.element);
            this._expandedLineRows.push(lineRow);
        }
        parentElement.removeChild(this.element);
        this._chunkedPanel._paintLines(this.startLine, this.startLine + this.linesCount);

        this._chunkedPanel.endDomUpdates();
    },

    collapse: function()
    {
        if (!this._expanded)
            return;

        this._expanded = false;
        if (this.linesCount === 1)
            return;

        this._chunkedPanel.beginDomUpdates();

        var elementInserted = false;
        for (var i = 0; i < this._expandedLineRows.length; ++i) {
            var lineRow = this._expandedLineRows[i];
            var parentElement = lineRow.parentElement;
            if (parentElement) {
                if (!elementInserted) {
                    elementInserted = true;
                    parentElement.insertBefore(this.element, lineRow);
                }
                parentElement.removeChild(lineRow);
            }
            this._chunkedPanel._releaseLinesHighlight(lineRow);
        }
        delete this._expandedLineRows;

        this._chunkedPanel.endDomUpdates();
    },

    /**
     * @return {number}
     */
    get height()
    {
        if (!this._expandedLineRows)
            return this._chunkedPanel.totalHeight(this.element);
        return this._chunkedPanel.totalHeight(this._expandedLineRows[0], this._expandedLineRows[this._expandedLineRows.length - 1]);
    },

    /**
     * @return {number}
     */
    get offsetTop()
    {
        return (this._expandedLineRows && this._expandedLineRows.length) ? this._expandedLineRows[0].offsetTop : this.element.offsetTop;
    },

    /**
     * @param {number} lineNumber
     * @return {Element}
     */
    _createRow: function(lineNumber)
    {
        var lineRow = this._chunkedPanel._cachedRows.pop() || document.createElement("div");
        lineRow.lineNumber = lineNumber;
        lineRow.className = "webkit-line-content";
        lineRow.textContent = this._textModel.line(lineNumber);
        if (!lineRow.textContent)
            lineRow.appendChild(document.createElement("br"));
        return lineRow;
    },

    /**
     * Called on potentially damaged / inconsistent chunk
     * @param {number} lineNumber
     * @return {?Node}
     */
    lineRowContainingLine: function(lineNumber)
    {
        if (!this._expanded)
            return this.element;
        return this.expandedLineRow(lineNumber);
    },

    /**
     * @param {number} lineNumber
     * @return {Element}
     */
    expandedLineRow: function(lineNumber)
    {
        if (!this._expanded || lineNumber < this.startLine || lineNumber >= this.startLine + this.linesCount)
            return null;
        if (!this._expandedLineRows)
            return this.element;
        return this._expandedLineRows[lineNumber - this.startLine];
    },

    updateCollapsedLineRow: function()
    {
        if (this.linesCount === 1 && this._expanded)
            return;

        var lines = [];
        for (var i = this.startLine; i < this.startLine + this.linesCount; ++i)
            lines.push(this._textModel.line(i));

        if (WebInspector.FALSE)
            console.log("Rebuilding chunk with " + lines.length + " lines");

        this.element.removeChildren();
        this.element.textContent = lines.join("\n");
        // The last empty line will get swallowed otherwise.
        if (!lines[lines.length - 1])
            this.element.appendChild(document.createElement("br"));
    },

    firstElement: function()
    {
        return this._expandedLineRows ? this._expandedLineRows[0] : this.element;
    },

    /**
     * @return {Element}
     */
    lastElement: function()
    {
        return this._expandedLineRows ? this._expandedLineRows[this._expandedLineRows.length - 1] : this.element;
    }
}

/**
 * @constructor
 * @param {WebInspector.TextEditorMainPanel} mainPanel
 * @param {WebInspector.TextEditorModel} textModel
 */
WebInspector.TextEditorMainPanel.TokenHighlighter = function(mainPanel, textModel)
{
    this._mainPanel = mainPanel;
    this._textModel = textModel;
}

WebInspector.TextEditorMainPanel.TokenHighlighter.prototype = {
    /**
     * @param {WebInspector.TextRange} range
     */
    handleSelectionChange: function(range)
    {
        if (!range) {
            this._removeHighlight();
            return;
        }

        if (range.startLine !== range.endLine) {
            this._removeHighlight();
            return;
        }

        range = range.normalize();
        var selectedText = this._textModel.copyRange(range);
        if (selectedText === this._selectedWord)
            return;

        if (selectedText === "") {
            this._removeHighlight();
            return;
        }

        if (this._isWord(range, selectedText))
            this._highlight(selectedText);
        else
            this._removeHighlight();
    },

    /**
     * @param {string} word
     */
    _regexString: function(word)
    {
        return "\\b" + word + "\\b";
    },

    /**
     * @param {string} selectedWord
     */
    _highlight: function(selectedWord)
    {
        this._removeHighlight();
        this._selectedWord = selectedWord;
        this._highlightDescriptor = this._mainPanel.highlightRegex(this._regexString(selectedWord), "text-editor-token-highlight")
    },

    _removeHighlight: function()
    {
        if (this._selectedWord) {
            this._mainPanel.removeHighlight(this._highlightDescriptor);
            delete this._selectedWord;
            delete this._highlightDescriptor;
        }
    },

    /**
     * @param {WebInspector.TextRange} range
     * @param {string} selectedText
     * @return {boolean}
     */
    _isWord: function(range, selectedText)
    {
        var line = this._textModel.line(range.startLine);
        var leftBound = range.startColumn === 0 || !WebInspector.TextUtils.isWordChar(line.charAt(range.startColumn - 1));
        var rightBound = range.endColumn === line.length || !WebInspector.TextUtils.isWordChar(line.charAt(range.endColumn));
        return leftBound && rightBound && WebInspector.TextUtils.isWord(selectedText);
    }
}

/**
 * @constructor
 * @param {WebInspector.TextEditorModel} textModel
 * @param {WebInspector.TextEditor} textEditor
 */
WebInspector.DefaultTextEditor.WordMovementController = function(textEditor, textModel)
{
    this._textModel = textModel;
    this._textEditor = textEditor;
}

WebInspector.DefaultTextEditor.WordMovementController.prototype = {

    /**
     * @param {Object.<number, function()>} shortcuts
     */
    _registerShortcuts: function(shortcuts)
    {
        var keys = WebInspector.KeyboardShortcut.Keys;
        var modifiers = WebInspector.KeyboardShortcut.Modifiers;

        const wordJumpModifier = WebInspector.isMac() ? modifiers.Alt : modifiers.Ctrl;
        shortcuts[WebInspector.KeyboardShortcut.makeKey(keys.Backspace.code, wordJumpModifier)] = this._handleCtrlBackspace.bind(this);
        shortcuts[WebInspector.KeyboardShortcut.makeKey(keys.Left.code, wordJumpModifier)] = this._handleCtrlArrow.bind(this, "left");
        shortcuts[WebInspector.KeyboardShortcut.makeKey(keys.Right.code, wordJumpModifier)] = this._handleCtrlArrow.bind(this, "right");
        shortcuts[WebInspector.KeyboardShortcut.makeKey(keys.Left.code, modifiers.Shift | wordJumpModifier)] = this._handleCtrlShiftArrow.bind(this, "left");
        shortcuts[WebInspector.KeyboardShortcut.makeKey(keys.Right.code, modifiers.Shift | wordJumpModifier)] = this._handleCtrlShiftArrow.bind(this, "right");
    },

    /**
     * @param {WebInspector.TextRange} selection
     * @param {string} direction
     * @return {WebInspector.TextRange}
     */
    _rangeForCtrlArrowMove: function(selection, direction)
    {
        const isStopChar = WebInspector.TextUtils.isStopChar;
        const isSpaceChar = WebInspector.TextUtils.isSpaceChar;

        var lineNumber = selection.endLine;
        var column = selection.endColumn;
        if (direction === "left")
            --column;

        if (column === -1 && direction === "left") {
            if (lineNumber > 0)
                return new WebInspector.TextRange(selection.startLine, selection.startColumn, lineNumber - 1, this._textModel.line(lineNumber - 1).length);
            else
                return selection.clone();
        }

        var line = this._textModel.line(lineNumber);
        if (column === line.length && direction === "right") {
            if (lineNumber + 1 < this._textModel.linesCount)
                return new WebInspector.TextRange(selection.startLine, selection.startColumn, selection.endLine + 1, 0);
            else
                return selection.clone();
        }

        var delta = direction === "left" ? -1 : +1;
        var directionDependentEndColumnOffset = (delta + 1) / 2;

        if (isSpaceChar(line.charAt(column))) {
            while(column + delta >= 0 && column + delta < line.length && isSpaceChar(line.charAt(column + delta)))
                column += delta;
            if (column + delta < 0 || column + delta === line.length)
                return new WebInspector.TextRange(selection.startLine, selection.startColumn, lineNumber, column + directionDependentEndColumnOffset);
            else
                column += delta;
        }

        var group = isStopChar(line.charAt(column));

        while(column + delta >= 0 && column + delta < line.length && isStopChar(line.charAt(column + delta)) === group && !isSpaceChar(line.charAt(column + delta)))
            column += delta;

        return new WebInspector.TextRange(selection.startLine, selection.startColumn, lineNumber, column + directionDependentEndColumnOffset);
    },

    /**
     * @param {string} direction
     * @return {boolean}
     */
    _handleCtrlArrow: function(direction)
    {
        var newSelection = this._rangeForCtrlArrowMove(this._textEditor.selection(), direction);
        this._textEditor.setSelection(newSelection.collapseToEnd());
        return true;
    },

    /**
     * @param {string} direction
     * @return {boolean}
     */
    _handleCtrlShiftArrow: function(direction)
    {
        this._textEditor.setSelection(this._rangeForCtrlArrowMove(this._textEditor.selection(), direction));
        return true;
    },

    /**
     * @return {boolean}
     */
    _handleCtrlBackspace: function()
    {
        var selection = this._textEditor.selection();
        if (!selection.isEmpty())
            return false;

        var newSelection = this._rangeForCtrlArrowMove(selection, "left");
        this._textModel.editRange(newSelection.normalize(), "", selection);

        this._textEditor.setSelection(newSelection.collapseToEnd());
        return true;
    }
}

/**
 * @constructor
 * @param {WebInspector.TextEditorMainPanel} textEditor
 * @param {WebInspector.TextEditorModel} textModel
 * @param {WebInspector.TextEditorModel.BraceMatcher} braceMatcher
 */
WebInspector.TextEditorMainPanel.BraceHighlightController = function(textEditor, textModel, braceMatcher)
{
    this._textEditor = textEditor;
    this._textModel = textModel;
    this._braceMatcher = braceMatcher;
    this._highlightDescriptors = [];
}

WebInspector.TextEditorMainPanel.BraceHighlightController.prototype = {
    /**
     * @param {string} line
     * @param {number} column
     * @return {number}
     */
    activeBraceColumnForCursorPosition: function(line, column)
    {
        var char = line.charAt(column);
        if (WebInspector.TextUtils.isOpeningBraceChar(char))
            return column;

        var previousChar = line.charAt(column - 1);
        if (WebInspector.TextUtils.isBraceChar(previousChar))
            return column - 1;

        if (WebInspector.TextUtils.isBraceChar(char))
            return column;
        else
            return -1;
    },

    /**
     * @param {WebInspector.TextRange} selectionRange
     */
    handleSelectionChange: function(selectionRange)
    {
        if (!selectionRange || !selectionRange.isEmpty()) {
            this._removeHighlight();
            return;
        }

        if (this._highlightedRange && this._highlightedRange.compareTo(selectionRange) === 0)
            return;

        this._removeHighlight();
        var lineNumber = selectionRange.startLine;
        var column = selectionRange.startColumn;
        var line = this._textModel.line(lineNumber);
        column = this.activeBraceColumnForCursorPosition(line, column);
        if (column < 0)
            return;

        var enclosingBraces = this._braceMatcher.enclosingBraces(lineNumber, column);
        if (!enclosingBraces)
            return;

        this._highlightedRange = selectionRange;
        this._highlightDescriptors.push(this._textEditor.highlightRange(WebInspector.TextRange.createFromLocation(enclosingBraces.leftBrace.lineNumber, enclosingBraces.leftBrace.column), "text-editor-brace-match"));
        this._highlightDescriptors.push(this._textEditor.highlightRange(WebInspector.TextRange.createFromLocation(enclosingBraces.rightBrace.lineNumber, enclosingBraces.rightBrace.column), "text-editor-brace-match"));
    },

    _removeHighlight: function()
    {
        if (!this._highlightDescriptors.length)
            return;

        for(var i = 0; i < this._highlightDescriptors.length; ++i)
            this._textEditor.removeHighlight(this._highlightDescriptors[i]);

        this._highlightDescriptors = [];
        delete this._highlightedRange;
    }
}

/**
 * @constructor
 * @param {WebInspector.TextEditorMainPanel} mainPanel
 * @param {WebInspector.TextEditorModel} textModel
 * @param {WebInspector.TextEditorModel.BraceMatcher} braceMatcher
 */
WebInspector.TextEditorMainPanel.SmartBraceController = function(mainPanel, textModel, braceMatcher)
{
    this._mainPanel = mainPanel;
    this._textModel = textModel;
    this._braceMatcher = braceMatcher
}

WebInspector.TextEditorMainPanel.SmartBraceController.prototype = {
    /**
     * @param {Object.<number, function()>} shortcuts
     */
    registerShortcuts: function(shortcuts)
    {
        if (!WebInspector.experimentsSettings.textEditorSmartBraces.isEnabled())
            return;

        var keys = WebInspector.KeyboardShortcut.Keys;
        var modifiers = WebInspector.KeyboardShortcut.Modifiers;

        shortcuts[WebInspector.KeyboardShortcut.makeKey(keys.Backspace.code, modifiers.None)] = this._handleBackspace.bind(this);
    },

    /**
     * @param {Object.<string, function()>} charOverrides
     */
    registerCharOverrides: function(charOverrides)
    {
        if (!WebInspector.experimentsSettings.textEditorSmartBraces.isEnabled())
            return;
        charOverrides["("] = this._handleBracePairInsertion.bind(this, "()");
        charOverrides[")"] = this._handleClosingBraceOverride.bind(this, ")");
        charOverrides["{"] = this._handleBracePairInsertion.bind(this, "{}");
        charOverrides["}"] = this._handleClosingBraceOverride.bind(this, "}");
    },

    _handleBackspace: function()
    {
        var selection = this._mainPanel.lastSelection();
        if (!selection || !selection.isEmpty())
            return false;

        var column = selection.startColumn;
        if (column == 0)
            return false;

        var lineNumber = selection.startLine;
        var line = this._textModel.line(lineNumber);
        if (column === line.length)
            return false;

        var pair = line.substr(column - 1, 2);
        if (pair === "()" || pair === "{}") {
            this._textModel.editRange(new WebInspector.TextRange(lineNumber, column - 1, lineNumber, column + 1), "");
            this._mainPanel.setSelection(WebInspector.TextRange.createFromLocation(lineNumber, column - 1));
            return true;
        } else
            return false;
    },

    /**
     * @param {string} bracePair
     * @return {boolean}
     */
    _handleBracePairInsertion: function(bracePair)
    {
        var selection = this._mainPanel.lastSelection().normalize();
        if (selection.isEmpty()) {
            var lineNumber = selection.startLine;
            var column = selection.startColumn;
            var line = this._textModel.line(lineNumber);
            if (column < line.length) {
                var char = line.charAt(column);
                if (WebInspector.TextUtils.isWordChar(char) || (!WebInspector.TextUtils.isBraceChar(char) && WebInspector.TextUtils.isStopChar(char)))
                    return false;
            }
        }
        this._textModel.editRange(selection, bracePair);
        this._mainPanel.setSelection(WebInspector.TextRange.createFromLocation(selection.startLine, selection.startColumn + 1));
        return true;
    },

    /**
     * @param {string} brace
     * @return {boolean}
     */
    _handleClosingBraceOverride: function(brace)
    {
        var selection = this._mainPanel.lastSelection().normalize();
        if (!selection || !selection.isEmpty())
            return false;

        var lineNumber = selection.startLine;
        var column = selection.startColumn;
        var line = this._textModel.line(lineNumber);
        if (line.charAt(column) !== brace)
            return false;

        var braces = this._braceMatcher.enclosingBraces(lineNumber, column);
        if (braces && braces.rightBrace.lineNumber === lineNumber && braces.rightBrace.column === column) {
            this._mainPanel.setSelection(WebInspector.TextRange.createFromLocation(lineNumber, column + 1));
            return true;
        } else
            return false;
    },
}

WebInspector.debugDefaultTextEditor = false;
