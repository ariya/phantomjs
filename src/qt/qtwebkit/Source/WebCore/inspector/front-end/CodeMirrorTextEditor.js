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

importScript("cm/codemirror.js");
importScript("cm/css.js");
importScript("cm/javascript.js");
importScript("cm/xml.js");
importScript("cm/htmlmixed.js");
importScript("cm/matchbrackets.js");
importScript("cm/closebrackets.js");

/**
 * @constructor
 * @extends {WebInspector.View}
 * @implements {WebInspector.TextEditor}
 * @param {?string} url
 * @param {WebInspector.TextEditorDelegate} delegate
 */
WebInspector.CodeMirrorTextEditor = function(url, delegate)
{
    WebInspector.View.call(this);
    this._delegate = delegate;
    this._url = url;

    this.registerRequiredCSS("cm/codemirror.css");
    this.registerRequiredCSS("cm/cmdevtools.css");

    this._codeMirror = window.CodeMirror(this.element, {
        lineNumbers: true,
        gutters: ["CodeMirror-linenumbers", "breakpoints"],
        matchBrackets: true,
        autoCloseBrackets: WebInspector.experimentsSettings.textEditorSmartBraces.isEnabled()
    });

    var indent = WebInspector.settings.textEditorIndent.get();
    if (indent === WebInspector.TextUtils.Indent.TabCharacter) {
        this._codeMirror.setOption("indentWithTabs", true);
        this._codeMirror.setOption("indentUnit", 4);
    } else {
        this._codeMirror.setOption("indentWithTabs", false);
        this._codeMirror.setOption("indentUnit", indent.length);
    }

    this._tokenHighlighter = new WebInspector.CodeMirrorTextEditor.TokenHighlighter(this._codeMirror);
    this._blockIndentController = new WebInspector.CodeMirrorTextEditor.BlockIndentController(this._codeMirror);
    this._fixWordMovement = new WebInspector.CodeMirrorTextEditor.FixWordMovement(this._codeMirror);

    this._codeMirror.on("change", this._change.bind(this));
    this._codeMirror.on("gutterClick", this._gutterClick.bind(this));
    this._codeMirror.on("cursorActivity", this._cursorActivity.bind(this));
    this._codeMirror.on("scroll", this._scroll.bind(this));
    this.element.addEventListener("contextmenu", this._contextMenu.bind(this));

    this._lastRange = this.range();

    this.element.firstChild.addStyleClass("source-code");
    this.element.addStyleClass("fill");
    this.markAsLayoutBoundary();

    this._elementToWidget = new Map();
    this._nestedUpdatesCounter = 0;

    this.element.addEventListener("focus", this._handleElementFocus.bind(this), false);
    this.element.tabIndex = 0;
}

WebInspector.CodeMirrorTextEditor.prototype = {

    /**
     * @param {number} lineNumber
     * @param {number} column
     * @return {?{x: number, y: number, height: number}}
     */
    cursorPositionToCoordinates: function(lineNumber, column)
    {
        if (lineNumber >= this._codeMirror.lineCount || column > this._codeMirror.getLine(lineNumber).length || lineNumber < 0 || column < 0)
            return null;

        var metrics = this._codeMirror.cursorCoords(CodeMirror.Pos(lineNumber, column));

        return {
            x: metrics.left,
            y: metrics.top,
            height: metrics.bottom - metrics.top
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
        if (!element || !element.isSelfOrDescendant(this._codeMirror.getWrapperElement()))
            return null;
        var gutterBox = this._codeMirror.getGutterElement().boxInWindow();
        if (x >= gutterBox.x && x <= gutterBox.x + gutterBox.width &&
            y >= gutterBox.y && y <= gutterBox.y + gutterBox.height)
            return null;
        var coords = this._codeMirror.coordsChar({left: x, top: y});
        ++coords.ch;
        return this._toRange(coords, coords);
    },

    /**
     * @param {number} lineNumber
     * @param {number} column
     * @return {?{startColumn: number, endColumn: number, token: string}}
     */
    tokenAtTextPosition: function(lineNumber, column)
    {
        if (lineNumber < 0 || lineNumber >= this._codeMirror.lineCount())
            return null;
        var token = this._codeMirror.getTokenAt(CodeMirror.Pos(lineNumber, column || 1));
        if (!token || !token.type)
            return null;
        var convertedType = null;
        if (token.type.startsWith("variable") || token.type.startsWith("property")) {
            return {
                startColumn: token.start,
                endColumn: token.end - 1,
                type: "javascript-ident"
            };
        }
        return null;
    },

    /**
     * @param {WebInspector.TextRange} textRange
     * @return {string}
     */
    copyRange: function(textRange)
    {
        var pos = this._toPos(textRange);
        return this._codeMirror.getRange(pos.start, pos.end);
    },

    /**
     * @return {boolean}
     */
    isClean: function()
    {
        return this._codeMirror.isClean();
    },

    markClean: function()
    {
        this._codeMirror.markClean();
    },

    /**
     * @param {string} mimeType
     */
    set mimeType(mimeType)
    {
        this._codeMirror.setOption("mode", mimeType);
        switch(mimeType) {
            case "text/html": this._codeMirror.setOption("theme", "web-inspector-html"); break;
            case "text/css": this._codeMirror.setOption("theme", "web-inspector-css"); break;
            case "text/javascript": this._codeMirror.setOption("theme", "web-inspector-js"); break;
        }
    },

    /**
     * @param {boolean} readOnly
     */
    setReadOnly: function(readOnly)
    {
        this._codeMirror.setOption("readOnly", readOnly ? "nocursor" : false);
    },

    /**
     * @return {boolean}
     */
    readOnly: function()
    {
        return !!this._codeMirror.getOption("readOnly");
    },

    /**
     * @param {Object} highlightDescriptor
     */
    removeHighlight: function(highlightDescriptor)
    {
        highlightDescriptor.clear();
    },

    /**
     * @param {WebInspector.TextRange} range
     * @param {string} cssClass
     * @return {Object}
     */
    highlightRange: function(range, cssClass)
    {
        var pos = this._toPos(range);
        ++pos.end.ch;
        return this._codeMirror.markText(pos.start, pos.end, {
            className: cssClass,
            startStyle: cssClass + "-start",
            endStyle: cssClass + "-end"
        });
    },

    /**
     * @return {Element}
     */
    defaultFocusedElement: function()
    {
        return this.element;
    },

    focus: function()
    {
        this._codeMirror.focus();
    },

    _handleElementFocus: function()
    {
        this._codeMirror.focus();
    },

    beginUpdates: function()
    {
        ++this._nestedUpdatesCounter;
    },

    endUpdates: function()
    {
        if (!--this._nestedUpdatesCounter);
            this._codeMirror.refresh();
    },

    /**
     * @param {number} lineNumber
     */
    revealLine: function(lineNumber)
    {
        var pos = CodeMirror.Pos(lineNumber, 0);
        var topLine = this._topScrolledLine();
        var bottomLine = this._bottomScrolledLine();

        var margin = null;
        var lineMargin = 3;
        var scrollInfo = this._codeMirror.getScrollInfo();
        if ((lineNumber < topLine + lineMargin) || (lineNumber >= bottomLine - lineMargin)) {
            // scrollIntoView could get into infinite loop if margin exceeds half of the clientHeight.
            margin = (scrollInfo.clientHeight*0.9/2) >>> 0;
        }
        this._codeMirror.scrollIntoView(pos, margin);
    },

    _gutterClick: function(instance, lineNumber, gutter, event)
    {
        this.dispatchEventToListeners(WebInspector.TextEditor.Events.GutterClick, { lineNumber: lineNumber, event: event });
    },

    _contextMenu: function(event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);
        var target = event.target.enclosingNodeOrSelfWithClass("CodeMirror-gutter-elt");
        if (target)
            this._delegate.populateLineGutterContextMenu(contextMenu, parseInt(target.textContent, 10) - 1);
        else
            this._delegate.populateTextAreaContextMenu(contextMenu, null);
        contextMenu.show();
    },

    /**
     * @param {number} lineNumber
     * @param {boolean} disabled
     * @param {boolean} conditional
     */
    addBreakpoint: function(lineNumber, disabled, conditional)
    {
        var element = document.createElement("span");
        element.textContent = lineNumber + 1;
        element.className = "cm-breakpoint" + (disabled ? " cm-breakpoint-disabled" : "") + (conditional ? " cm-breakpoint-conditional" : "");
        this._codeMirror.setGutterMarker(lineNumber, "breakpoints", element);
    },

    /**
     * @param {number} lineNumber
     */
    removeBreakpoint: function(lineNumber)
    {
        this._codeMirror.setGutterMarker(lineNumber, "breakpoints", null);
    },

    /**
     * @param {number} lineNumber
     */
    setExecutionLine: function(lineNumber)
    {
        this._executionLine = this._codeMirror.getLineHandle(lineNumber);
        this._codeMirror.addLineClass(this._executionLine, null, "cm-execution-line");
    },

    clearExecutionLine: function()
    {
        if (this._executionLine)
            this._codeMirror.removeLineClass(this._executionLine, null, "cm-execution-line");
        delete this._executionLine;
    },

    /**
     * @param {number} lineNumber
     * @param {Element} element
     */
    addDecoration: function(lineNumber, element)
    {
        var widget = this._codeMirror.addLineWidget(lineNumber, element);
        this._elementToWidget.put(element, widget);
    },

    /**
     * @param {number} lineNumber
     * @param {Element} element
     */
    removeDecoration: function(lineNumber, element)
    {
        var widget = this._elementToWidget.remove(element);
        if (widget)
            this._codeMirror.removeLineWidget(widget);
    },

    /**
     * @param {WebInspector.TextRange} range
     */
    markAndRevealRange: function(range)
    {
        if (range)
            this.setSelection(range);
    },

    /**
     * @param {number} lineNumber
     */
    highlightLine: function(lineNumber)
    {
        this.clearLineHighlight();
        this._highlightedLine = this._codeMirror.getLineHandle(lineNumber);
        if (!this._highlightedLine)
          return;
        this.revealLine(lineNumber);
        this._codeMirror.addLineClass(this._highlightedLine, null, "cm-highlight");
        this._clearHighlightTimeout = setTimeout(this.clearLineHighlight.bind(this), 2000);
    },

    clearLineHighlight: function()
    {
        if (this._clearHighlightTimeout)
            clearTimeout(this._clearHighlightTimeout);
        delete this._clearHighlightTimeout;

         if (this._highlightedLine)
            this._codeMirror.removeLineClass(this._highlightedLine, null, "cm-highlight");
        delete this._highlightedLine;
    },

    /**
     * @return {Array.<Element>}
     */
    elementsToRestoreScrollPositionsFor: function()
    {
        return [];
    },

    /**
     * @param {WebInspector.TextEditor} textEditor
     */
    inheritScrollPositions: function(textEditor)
    {
    },

    onResize: function()
    {
        this._codeMirror.refresh();
    },

    /**
     * @param {WebInspector.TextRange} range
     * @param {string} text
     * @return {WebInspector.TextRange}
     */
    editRange: function(range, text)
    {
        var pos = this._toPos(range);
        this._codeMirror.replaceRange(text, pos.start, pos.end);
        var newRange = this._toRange(pos.start, this._codeMirror.posFromIndex(this._codeMirror.indexFromPos(pos.start) + text.length));
        this._delegate.onTextChanged(range, newRange);
        return newRange;
    },

    _change: function()
    {
        var widgets = this._elementToWidget.values();
        for (var i = 0; i < widgets.length; ++i)
            this._codeMirror.removeLineWidget(widgets[i]);
        this._elementToWidget.clear();

        var newRange = this.range();
        this._delegate.onTextChanged(this._lastRange, newRange);
        this._lastRange = newRange;
    },

    _cursorActivity: function()
    {
        var start = this._codeMirror.getCursor("anchor");
        var end = this._codeMirror.getCursor("head");
        this._delegate.selectionChanged(this._toRange(start, end));
    },

    _coordsCharLocal: function(coords)
    {
        var top = coords.top;
        var totalLines = this._codeMirror.lineCount();
        var begin = 0;
        var end = totalLines - 1;
        while (end - begin > 1) {
            var middle = (begin + end) >> 1;
            var coords = this._codeMirror.charCoords(CodeMirror.Pos(middle, 0), "local");
            if (coords.top >= top)
                end = middle;
            else
                begin = middle;
        }

        return end;
    },

    _topScrolledLine: function()
    {
        var scrollInfo = this._codeMirror.getScrollInfo();
        // Workaround for CodeMirror's coordsChar incorrect result for "local" mode.
        return this._coordsCharLocal(scrollInfo);
    },

    _bottomScrolledLine: function()
    {
        var scrollInfo = this._codeMirror.getScrollInfo();
        scrollInfo.top += scrollInfo.clientHeight;
        // Workaround for CodeMirror's coordsChar incorrect result for "local" mode.
        return this._coordsCharLocal(scrollInfo);
    },

    _scroll: function()
    {
        this._delegate.scrollChanged(this._topScrolledLine());
    },

    /**
     * @param {number} lineNumber
     */
    scrollToLine: function(lineNumber)
    {
        function performScroll()
        {
            var pos = CodeMirror.Pos(lineNumber, 0);
            var coords = this._codeMirror.charCoords(pos, "local");
            this._codeMirror.scrollTo(0, coords.top);
        }

        setTimeout(performScroll.bind(this), 0);
    },

    /**
     * @return {WebInspector.TextRange}
     */
    selection: function(textRange)
    {
        var start = this._codeMirror.getCursor(true);
        var end = this._codeMirror.getCursor(false);

        if (start.line > end.line || (start.line == end.line && start.ch > end.ch))
            return this._toRange(end, start);

        return this._toRange(start, end);
    },

    /**
     * @return {WebInspector.TextRange?}
     */
    lastSelection: function()
    {
        return this._lastSelection;
    },

    /**
     * @param {WebInspector.TextRange} textRange
     */
    setSelection: function(textRange)
    {
        function performSelectionSet()
        {
            this._lastSelection = textRange;
            var pos = this._toPos(textRange);
            this._codeMirror.setSelection(pos.start, pos.end);
        }

        setTimeout(performSelectionSet.bind(this), 0);
    },

    /**
     * @param {string} text
     */
    setText: function(text)
    {
        this._codeMirror.setValue(text);
    },

    /**
     * @return {string}
     */
    text: function()
    {
        return this._codeMirror.getValue();
    },

    /**
     * @return {WebInspector.TextRange}
     */
    range: function()
    {
        var lineCount = this.linesCount;
        var lastLine = this._codeMirror.getLine(lineCount - 1);
        return this._toRange({ line: 0, ch: 0 }, { line: lineCount - 1, ch: lastLine.length });
    },

    /**
     * @param {number} lineNumber
     * @return {string}
     */
    line: function(lineNumber)
    {
        return this._codeMirror.getLine(lineNumber);
    },

    /**
     * @return {number}
     */
    get linesCount()
    {
        return this._codeMirror.lineCount();
    },

    /**
     * @param {number} line
     * @param {string} name
     * @param {Object?} value
     */
    setAttribute: function(line, name, value)
    {
        var handle = this._codeMirror.getLineHandle(line);
        if (handle.attributes === undefined) handle.attributes = {};
        handle.attributes[name] = value;
    },

    /**
     * @param {number} line
     * @param {string} name
     * @return {Object|null} value
     */
    getAttribute: function(line, name)
    {
        var handle = this._codeMirror.getLineHandle(line);
        return handle.attributes && handle.attributes[name] !== undefined ? handle.attributes[name] : null;
    },

    /**
     * @param {number} line
     * @param {string} name
     */
    removeAttribute: function(line, name)
    {
        var handle = this._codeMirror.getLineHandle(line);
        if (handle && handle.attributes)
            delete handle.attributes[name];
    },

    _toPos: function(range)
    {
        return {
            start: {line: range.startLine, ch: range.startColumn},
            end: {line: range.endLine, ch: range.endColumn}
        }
    },

    _toRange: function(start, end)
    {
        return new WebInspector.TextRange(start.line, start.ch, end.line, end.ch);
    },

    __proto__: WebInspector.View.prototype
}

WebInspector.CodeMirrorTextEditor.TokenHighlighter = function(codeMirror)
{
    this._codeMirror = codeMirror;
    this._codeMirror.on("cursorActivity", this._cursorChange.bind(this));
}

WebInspector.CodeMirrorTextEditor.TokenHighlighter.prototype = {
    _cursorChange: function()
    {
        this._codeMirror.operation(this._removeHighlight.bind(this));
        var selectionStart = this._codeMirror.getCursor("start");
        var selectionEnd = this._codeMirror.getCursor("end");
        if (selectionStart.line !== selectionEnd.line)
            return;
        if (selectionStart.ch === selectionEnd.ch)
            return;

        var selectedText = this._codeMirror.getSelection();
        if (this._isWord(selectedText, selectionStart.line, selectionStart.ch, selectionEnd.ch))
            this._codeMirror.operation(this._addHighlight.bind(this, selectedText, selectionStart));
    },

    _isWord: function(selectedText, lineNumber, startColumn, endColumn)
    {
        var line = this._codeMirror.getLine(lineNumber);
        var leftBound = startColumn === 0 || !WebInspector.TextUtils.isWordChar(line.charAt(startColumn - 1));
        var rightBound = endColumn === line.length || !WebInspector.TextUtils.isWordChar(line.charAt(endColumn));
        return leftBound && rightBound && WebInspector.TextUtils.isWord(selectedText);
    },

    _removeHighlight: function()
    {
        if (this._highlightDescriptor) {
            this._codeMirror.removeOverlay(this._highlightDescriptor.overlay);
            this._codeMirror.removeLineClass(this._highlightDescriptor.selectionStart.line, "wrap", "cm-line-with-selection");
            delete this._highlightDescriptor;
        }
    },

    _addHighlight: function(token, selectionStart)
    {
        const tokenFirstChar = token.charAt(0);
        function nextToken(stream)
        {
            if (stream.match(token) && (stream.eol() || !WebInspector.TextUtils.isWordChar(stream.peek())))
                return stream.column() === selectionStart.ch ? "token-highlight column-with-selection" : "token-highlight";

            var eatenChar;
            do {
                eatenChar = stream.next();
            } while (eatenChar && (WebInspector.TextUtils.isWordChar(eatenChar) || stream.peek() !== tokenFirstChar));
        }

        var overlayMode = {
            token: nextToken
        };
        this._codeMirror.addOverlay(overlayMode);
        this._codeMirror.addLineClass(selectionStart.line, "wrap", "cm-line-with-selection")
        this._highlightDescriptor = {
            overlay: overlayMode,
            selectionStart: selectionStart
        };
    }
}

WebInspector.CodeMirrorTextEditor.BlockIndentController = function(codeMirror)
{
    codeMirror.addKeyMap(this);
}

WebInspector.CodeMirrorTextEditor.BlockIndentController.prototype = {
    name: "blockIndentKeymap",

    Enter: function(codeMirror)
    {
        if (codeMirror.somethingSelected())
            return CodeMirror.Pass;
        var cursor = codeMirror.getCursor();
        var line = codeMirror.getLine(cursor.line);
        if (line.substr(cursor.ch - 1, 2) === "{}") {
            codeMirror.execCommand("newlineAndIndent");
            codeMirror.setCursor(cursor);
            codeMirror.execCommand("newlineAndIndent");
        } else
            return CodeMirror.Pass;
    }
}

WebInspector.CodeMirrorTextEditor.FixWordMovement = function(codeMirror)
{
    function moveLeft(shift, codeMirror)
    {
        var cursor = codeMirror.getCursor("head");
        if (cursor.ch !== 0 || cursor.line === 0)
            return CodeMirror.Pass;
        codeMirror.setExtending(shift);
        codeMirror.execCommand("goLineUp");
        codeMirror.execCommand("goLineEnd")
        codeMirror.setExtending(false);
    }
    function moveRight(shift, codeMirror)
    {
        var cursor = codeMirror.getCursor("head");
        var line = codeMirror.getLine(cursor.line);
        if (cursor.ch !== line.length || cursor.line + 1 === codeMirror.lineCount())
            return CodeMirror.Pass;
        codeMirror.setExtending(shift);
        codeMirror.execCommand("goLineDown");
        codeMirror.execCommand("goLineStart");
        codeMirror.setExtending(false);
    }

    var modifierKey = WebInspector.isMac() ? "Alt" : "Ctrl";
    var leftKey = modifierKey + "-Left";
    var rightKey = modifierKey + "-Right";
    var keyMap = {};
    keyMap[leftKey] = moveLeft.bind(this, false);
    keyMap[rightKey] = moveRight.bind(this, false);
    keyMap["Shift-" + leftKey] = moveLeft.bind(this, true);
    keyMap["Shift-" + rightKey] = moveRight.bind(this, true);
    codeMirror.addKeyMap(keyMap);
}
