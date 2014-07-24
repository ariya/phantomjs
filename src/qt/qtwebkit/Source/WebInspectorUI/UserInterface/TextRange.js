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

WebInspector.TextRange = function(startLineOrStartOffset, startColumnOrEndOffset, endLine, endColumn)
{
    WebInspector.Object.call(this);

    if (arguments.length === 4) {
        console.assert(startLineOrStartOffset <= endLine);
        console.assert(startLineOrStartOffset !== endLine || startColumnOrEndOffset <= endColumn);

        this._startLine = typeof startLineOrStartOffset === "number" ? startLineOrStartOffset : NaN;
        this._startColumn = typeof startColumnOrEndOffset === "number" ? startColumnOrEndOffset : NaN;
        this._endLine = typeof endLine === "number" ? endLine : NaN;
        this._endColumn = typeof endColumn === "number" ? endColumn : NaN;

        this._startOffset = NaN;
        this._endOffset = NaN;
    } else if (arguments.length === 2) {
        console.assert(startLineOrStartOffset <= startColumnOrEndOffset);

        this._startOffset = typeof startLineOrStartOffset === "number" ? startLineOrStartOffset : NaN;
        this._endOffset = typeof startColumnOrEndOffset === "number" ? startColumnOrEndOffset : NaN;

        this._startLine = NaN;
        this._startColumn = NaN;
        this._endLine = NaN;
        this._endColumn = NaN;
    }
}

WebInspector.TextRange.prototype = {
    constructor: WebInspector.TextRange,

    // Public

    get startLine()
    {
        return this._startLine;
    },

    get startColumn()
    {
        return this._startColumn;
    },

    get endLine()
    {
        return this._endLine;
    },

    get endColumn()
    {
        return this._endColumn;
    },

    get startOffset()
    {
        return this._startOffset;
    },

    get endOffset()
    {
        return this._endOffset;
    },

    startPosition: function()
    {
        return new WebInspector.SourceCodePosition(this._startLine, this._startColumn);
    },

    endPosition: function()
    {
        return new WebInspector.SourceCodePosition(this._endLine, this._endColumn);
    },

    resolveLinesAndColumns: function(text)
    {
        console.assert(typeof text === "string");
        if (typeof text !== "string")
            return;

        console.assert(!isNaN(this._startOffset));
        console.assert(!isNaN(this._endOffset));
        if (isNaN(this._startOffset) || isNaN(this._endOffset))
            return;

        function countNewLineCharacters(text)
        {
            var matches = text.match(/\n/g);
            return matches ? matches.length : 0;
        }

        var startSubstring = text.substring(0, this._startOffset);
        var rangeSubstring = text.substring(this._startOffset, this._endOffset);

        var startNewLineCount = countNewLineCharacters(startSubstring);
        var rangeNewLineCount = countNewLineCharacters(rangeSubstring);

        this._startLine = startNewLineCount;
        this._endLine = startNewLineCount + rangeNewLineCount;

        var lastNewLineOffset = startNewLineCount ? startSubstring.lastIndexOf("\n") + 1 : 0;
        this._startColumn = startSubstring.length - lastNewLineOffset;

        lastNewLineOffset = rangeNewLineCount ? rangeSubstring.lastIndexOf("\n") + 1 : 0;
        this._endColumn = rangeSubstring.length - lastNewLineOffset;

        if (this._startLine === this._endLine)
            this._endColumn += this._startColumn;
    },

    resolveOffsets: function(text)
    {
        console.assert(typeof text === "string");
        if (typeof text !== "string")
            return;

        console.assert(!isNaN(this._startLine));
        console.assert(!isNaN(this._startColumn));
        console.assert(!isNaN(this._endLine));
        console.assert(!isNaN(this._endColumn));
        if (isNaN(this._startLine) || isNaN(this._startColumn) || isNaN(this._endLine) || isNaN(this._endColumn))
            return;

        var lastNewLineOffset = 0;
        for (var i = 0; i < this._startLine; ++i)
            lastNewLineOffset = text.indexOf("\n", lastNewLineOffset) + 1;

        this._startOffset = lastNewLineOffset + this._startColumn;

        for (var i = this._startLine; i < this._endLine; ++i)
            lastNewLineOffset = text.indexOf("\n", lastNewLineOffset) + 1;

        this._endOffset = lastNewLineOffset + this._endColumn;
    }
};

WebInspector.TextRange.prototype.__proto__ = WebInspector.Object.prototype;
