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

WebInspector.ConsolePrompt = function(delegate, mimeType, element)
{
    WebInspector.Object.call(this);

    this._element = element || document.createElement("div");
    this._element.classList.add(WebInspector.ConsolePrompt.StyleClassName);
    this._element.classList.add(WebInspector.SyntaxHighlightedStyleClassName);

    this._delegate = delegate || null;

    this._codeMirror = CodeMirror(this.element, {
        lineWrapping: true,
        mode: mimeType,
        indentWithTabs: true,
        indentUnit: 4,
        matchBrackets: true
    });

    var keyMap = {
        "Up": this._handlePreviousKey.bind(this),
        "Down": this._handleNextKey.bind(this),
        "Ctrl-P": this._handlePreviousKey.bind(this),
        "Ctrl-N": this._handleNextKey.bind(this),
        "Enter": this._handleEnterKey.bind(this),
        "Cmd-Enter": this._handleCommandEnterKey.bind(this)
    };

    this._codeMirror.addKeyMap(keyMap);

    this._completionController = new WebInspector.CodeMirrorCompletionController(this._codeMirror, this);

    this._history = [{}];
    this._historyIndex = 0;
};

WebInspector.ConsolePrompt.StyleClassName = "console-prompt";
WebInspector.ConsolePrompt.MaximumHistorySize = 30;

WebInspector.ConsolePrompt.prototype = {
    constructor: WebInspector.ConsolePrompt,

    // Public

    get element()
    {
        return this._element;
    },

    get delegate()
    {
        return this._delegate;
    },

    set delegate(delegate)
    {
        this._delegate = delegate || null;
    },

    get text()
    {
        return this._codeMirror.getValue();
    },

    set text(text)
    {
        this._codeMirror.setValue(text || "");
        this._codeMirror.clearHistory();
        this._codeMirror.markClean();
    },

    get history()
    {
        this._history[this._historyIndex] = this._historyEntryForCurrentText();
        return this._history;
    },

    set history(history)
    {
        this._history = history instanceof Array ? history.slice(0, WebInspector.ConsolePrompt.MaximumHistorySize) : [{}];
        this._historyIndex = 0;
        this._restoreHistoryEntry(0);
    },

    get focused()
    {
        return this._codeMirror.getWrapperElement().classList.contains("CodeMirror-focused");
    },

    focus: function()
    {
        this._codeMirror.focus();
    },

    shown: function()
    {
        this._codeMirror.refresh();
    },

    updateLayout: function()
    {
        this._codeMirror.refresh();
    },

    updateCompletions: function(completions, implicitSuffix)
    {
        this._completionController.updateCompletions(completions, implicitSuffix);
    },

    // Protected

    completionControllerCompletionsNeeded: function(completionController, prefix, defaultCompletions, base, suffix, forced)
    {
        if (this.delegate && typeof this.delegate.consolePromptCompletionsNeeded === "function")
            this.delegate.consolePromptCompletionsNeeded(this, defaultCompletions, base, prefix, suffix, forced);
        else
            this._completionController.updateCompletions(defaultCompletions);
    },

    completionControllerShouldAllowEscapeCompletion: function(completionController)
    {
        // Only allow escape to complete if there is text in the prompt. Otherwise allow it to pass through
        // so escape to toggle the quick console still works.
        return !!this.text;
    },

    // Private

    _handlePreviousKey: function(codeMirror)
    {
        if (this._codeMirror.somethingSelected())
            return CodeMirror.Pass;

        // Pass unless we are on the first line.
        if (this._codeMirror.getCursor().line)
            return CodeMirror.Pass;

        var historyEntry = this._history[this._historyIndex + 1];
        if (!historyEntry)
            return CodeMirror.Pass;

        this._rememberCurrentTextInHistory();

        ++this._historyIndex;

        this._restoreHistoryEntry(this._historyIndex);
    },

    _handleNextKey: function(codeMirror)
    {
        if (this._codeMirror.somethingSelected())
            return CodeMirror.Pass;

        // Pass unless we are on the last line.
        if (this._codeMirror.getCursor().line !== this._codeMirror.lastLine())
            return CodeMirror.Pass;

        var historyEntry = this._history[this._historyIndex - 1];
        if (!historyEntry)
            return CodeMirror.Pass;

        this._rememberCurrentTextInHistory();

        --this._historyIndex;

        this._restoreHistoryEntry(this._historyIndex);
    },

    _handleEnterKey: function(codeMirror, forceCommit)
    {
        var currentText = this.text;

        // Always do nothing when there is just whitespace.
        if (!currentText.trim())
            return;

        var cursor = this._codeMirror.getCursor();
        var lastLine = this._codeMirror.lastLine();
        var lastLineLength = this._codeMirror.getLine(lastLine).length;
        var cursorIsAtLastPosition = positionsEqual(cursor, {line: lastLine, ch: lastLineLength});

        function positionsEqual(a, b)
        {
            console.assert(a);
            console.assert(b);
            return a.line === b.line && a.ch === b.ch;
        }

        function commitTextOrInsertNewLine(commit)
        {
            if (!commit) {
                // Only insert a new line if the previous cursor and the current cursor are in the same position.
                if (positionsEqual(cursor, this._codeMirror.getCursor()))
                    CodeMirror.commands.newlineAndIndent(this._codeMirror);
                return;
            }

            var historyEntry = this._historyEntryForCurrentText();

            // Replace the previous entry if it does not have text or if the text is the same.
            if (this._history[1] && (!this._history[1].text || this._history[1].text === historyEntry.text)) {
                this._history[1] = historyEntry;
                this._history[0] = {};
            } else {
                // Replace the first history entry and push a new empty one.
                this._history[0] = historyEntry;
                this._history.unshift({});

                // Trim the history length if needed.
                if (this._history.length > WebInspector.ConsolePrompt.MaximumHistorySize)
                    this._history = this._history.slice(0, WebInspector.ConsolePrompt.MaximumHistorySize);
            }

            this._historyIndex = 0;

            this._codeMirror.setValue("");
            this._codeMirror.clearHistory();

            if (this.delegate && typeof this.delegate.consolePromptHistoryDidChange === "function")
                this.delegate.consolePromptHistoryDidChange(this);

            if (this.delegate && typeof this.delegate.consolePromptTextCommitted === "function")
                this.delegate.consolePromptTextCommitted(this, currentText);
        }

        if (!forceCommit && this.delegate && typeof this.delegate.consolePromptShouldCommitText === "function") {
            this.delegate.consolePromptShouldCommitText(this, currentText, cursorIsAtLastPosition, commitTextOrInsertNewLine.bind(this));
            return;
        }

        commitTextOrInsertNewLine.call(this, true);
    },

    _handleCommandEnterKey: function(codeMirror)
    {
        this._handleEnterKey(codeMirror, true);
    },

    _restoreHistoryEntry: function(index)
    {
        var historyEntry = this._history[index];

        this._codeMirror.setValue(historyEntry.text || "");

        if (historyEntry.undoHistory)
            this._codeMirror.setHistory(historyEntry.undoHistory);
        else
            this._codeMirror.clearHistory();

        this._codeMirror.setCursor(historyEntry.cursor || {line: 0});
    },

    _historyEntryForCurrentText: function()
    {
        return {text: this.text, undoHistory: this._codeMirror.getHistory(), cursor: this._codeMirror.getCursor()};
    },

    _rememberCurrentTextInHistory: function()
    {
        this._history[this._historyIndex] = this._historyEntryForCurrentText();

        if (this.delegate && typeof this.delegate.consolePromptHistoryDidChange === "function")
            this.delegate.consolePromptHistoryDidChange(this);
    }
};

WebInspector.ConsolePrompt.prototype.__proto__ = WebInspector.Object.prototype;
