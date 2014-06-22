/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
 */
WebInspector.TextEditorHighlighter = function(textModel, damageCallback)
{
    this._textModel = textModel;
    this._mimeType = "text/html";
    this._tokenizer = WebInspector.SourceTokenizer.Registry.getInstance().getTokenizer(this._mimeType);
    this._damageCallback = damageCallback;
    this._highlightChunkLimit = 1000;
    this._highlightLineLimit = 500;
}

WebInspector.TextEditorHighlighter._MaxLineCount = 10000;

WebInspector.TextEditorHighlighter.prototype = {

    get mimeType()
    {
        return this._mimeType;
    },

    /**
     * @param {string} mimeType
     */
    set mimeType(mimeType)
    {
        var tokenizer = WebInspector.SourceTokenizer.Registry.getInstance().getTokenizer(mimeType);
        if (tokenizer) {
            this._tokenizer = tokenizer;
            this._mimeType = mimeType;
        }
    },

    set highlightChunkLimit(highlightChunkLimit)
    {
        this._highlightChunkLimit = highlightChunkLimit;
    },

    /**
     * @param {number} highlightLineLimit
     */
    setHighlightLineLimit: function(highlightLineLimit)
    {
        this._highlightLineLimit = highlightLineLimit;
    },

    /**
     * @param {boolean=} forceRun
     */
    highlight: function(endLine, forceRun)
    {
        if (this._textModel.linesCount > WebInspector.TextEditorHighlighter._MaxLineCount)
            return;

        // First check if we have work to do.
        var state = this._textModel.getAttribute(endLine - 1, "highlight");
        if (state && state.postConditionStringified) {
            // Last line is highlighted, just exit.
            return;
        }

        this._requestedEndLine = endLine;

        if (this._highlightTimer && !forceRun) {
            // There is a timer scheduled, it will catch the new job based on the new endLine set.
            return;
        }

        // We will be highlighting. First rewind to the last highlighted line to gain proper highlighter context.
        var startLine = endLine;
        while (startLine > 0) {
            state = this._textModel.getAttribute(startLine - 1, "highlight");
            if (state && state.postConditionStringified)
                break;
            startLine--;
        }

        // Do small highlight synchronously. This will provide instant highlight on PageUp / PageDown, gentle scrolling.
        this._highlightInChunks(startLine, endLine);
    },

    updateHighlight: function(startLine, endLine)
    {
        if (this._textModel.linesCount > WebInspector.TextEditorHighlighter._MaxLineCount)
            return;

        // Start line was edited, we should highlight everything until endLine.
        this._clearHighlightState(startLine);

        if (startLine) {
            var state = this._textModel.getAttribute(startLine - 1, "highlight");
            if (!state || !state.postConditionStringified) {
                // Highlighter did not reach this point yet, nothing to update. It will reach it on subsequent timer tick and do the job.
                return false;
            }
        }

        var restored = this._highlightLines(startLine, endLine);
        if (!restored) {
            for (var i = this._lastHighlightedLine; i < this._textModel.linesCount; ++i) {
                var state = this._textModel.getAttribute(i, "highlight");
                if (!state && i > endLine)
                    break;
                this._textModel.setAttribute(i, "highlight-outdated", state);
                this._textModel.removeAttribute(i, "highlight");
            }

            if (this._highlightTimer) {
                clearTimeout(this._highlightTimer);
                this._requestedEndLine = endLine;
                this._highlightTimer = setTimeout(this._highlightInChunks.bind(this, this._lastHighlightedLine, this._requestedEndLine), 10);
            }
        }
        return restored;
    },

    _highlightInChunks: function(startLine, endLine)
    {
        delete this._highlightTimer;

        // First we always check if we have work to do. Could be that user scrolled back and we can quit.
        var state = this._textModel.getAttribute(this._requestedEndLine - 1, "highlight");
        if (state && state.postConditionStringified)
            return;

        if (this._requestedEndLine !== endLine) {
            // User keeps updating the job in between of our timer ticks. Just reschedule self, don't eat CPU (they must be scrolling).
            this._highlightTimer = setTimeout(this._highlightInChunks.bind(this, startLine, this._requestedEndLine), 100);
            return;
        }

        // The textModel may have been already updated.
        if (this._requestedEndLine > this._textModel.linesCount)
            this._requestedEndLine = this._textModel.linesCount;

        this._highlightLines(startLine, this._requestedEndLine);

        // Schedule tail highlight if necessary.
        if (this._lastHighlightedLine < this._requestedEndLine)
            this._highlightTimer = setTimeout(this._highlightInChunks.bind(this, this._lastHighlightedLine, this._requestedEndLine), 10);
    },

    _highlightLines: function(startLine, endLine)
    {
        // Restore highlighter context taken from previous line.
        var state = this._textModel.getAttribute(startLine - 1, "highlight");
        var postConditionStringified = state ? state.postConditionStringified : JSON.stringify(this._tokenizer.createInitialCondition());

        var tokensCount = 0;
        for (var lineNumber = startLine; lineNumber < endLine; ++lineNumber) {
            state = this._selectHighlightState(lineNumber, postConditionStringified);
            if (state.postConditionStringified) {
                // This line is already highlighted.
                postConditionStringified = state.postConditionStringified;
            } else {
                var lastHighlightedColumn = 0;
                if (state.midConditionStringified) {
                    lastHighlightedColumn = state.lastHighlightedColumn;
                    postConditionStringified = state.midConditionStringified;
                }

                var line = this._textModel.line(lineNumber);
                this._tokenizer.line = line;
                this._tokenizer.condition = JSON.parse(postConditionStringified);

                // Highlight line.
                state.ranges = state.ranges || [];
                state.braces = state.braces || [];
                do {
                    var newColumn = this._tokenizer.nextToken(lastHighlightedColumn);
                    var tokenType = this._tokenizer.tokenType;
                    if (tokenType && lastHighlightedColumn < this._highlightLineLimit) {
                        if (tokenType === "brace-start" || tokenType === "brace-end" || tokenType === "block-start" || tokenType === "block-end") {
                            state.braces.push({
                                startColumn: lastHighlightedColumn,
                                endColumn: newColumn - 1,
                                token: tokenType
                            });
                        } else {
                            state.ranges.push({
                                startColumn: lastHighlightedColumn,
                                endColumn: newColumn - 1,
                                token: tokenType
                            });
                        }
                    }
                    lastHighlightedColumn = newColumn;
                    if (++tokensCount > this._highlightChunkLimit)
                        break;
                } while (lastHighlightedColumn < line.length);

                postConditionStringified = JSON.stringify(this._tokenizer.condition);

                if (lastHighlightedColumn < line.length) {
                    // Too much work for single chunk - exit.
                    state.lastHighlightedColumn = lastHighlightedColumn;
                    state.midConditionStringified = postConditionStringified;
                    break;
                } else {
                    delete state.lastHighlightedColumn;
                    delete state.midConditionStringified;
                    state.postConditionStringified = postConditionStringified;
                }
            }

            var nextLineState = this._textModel.getAttribute(lineNumber + 1, "highlight");
            if (nextLineState && nextLineState.preConditionStringified === state.postConditionStringified) {
                // Following lines are up to date, no need re-highlight.
                ++lineNumber;
                this._damageCallback(startLine, lineNumber);

                // Advance the "pointer" to the last highlighted line within the given chunk.
                for (; lineNumber < endLine; ++lineNumber) {
                    state = this._textModel.getAttribute(lineNumber, "highlight");
                    if (!state || !state.postConditionStringified)
                        break;
                }
                this._lastHighlightedLine = lineNumber;
                return true;
            }
        }

        this._damageCallback(startLine, lineNumber);
        this._lastHighlightedLine = lineNumber;
        return false;
    },

    _selectHighlightState: function(lineNumber, preConditionStringified)
    {
        var state = this._textModel.getAttribute(lineNumber, "highlight");
        if (state && state.preConditionStringified === preConditionStringified)
            return state;

        var outdatedState = this._textModel.getAttribute(lineNumber, "highlight-outdated");
        if (outdatedState && outdatedState.preConditionStringified === preConditionStringified) {
            // Swap states.
            this._textModel.setAttribute(lineNumber, "highlight", outdatedState);
            this._textModel.setAttribute(lineNumber, "highlight-outdated", state);
            return outdatedState;
        }

        if (state)
            this._textModel.setAttribute(lineNumber, "highlight-outdated", state);

        state = {};
        state.preConditionStringified = preConditionStringified;
        this._textModel.setAttribute(lineNumber, "highlight", state);
        return state;
    },

    _clearHighlightState: function(lineNumber)
    {
        this._textModel.removeAttribute(lineNumber, "highlight");
        this._textModel.removeAttribute(lineNumber, "highlight-outdated");
    }
}
