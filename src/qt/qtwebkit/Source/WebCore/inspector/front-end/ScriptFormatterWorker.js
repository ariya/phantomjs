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

onmessage = function(event) {
    if (!event.data.method)
        return;

    self[event.data.method](event.data.params);
};

function format(params)
{
    // Default to a 4-space indent.
    var indentString = params.indentString || "    ";
    var result = {};

    if (params.mimeType === "text/html") {
        var formatter = new HTMLScriptFormatter(indentString);
        result = formatter.format(params.content);
    } else {
        result.mapping = { original: [0], formatted: [0] };
        result.content = formatScript(params.content, result.mapping, 0, 0, indentString);
    }
    postMessage(result);
}

function getChunkCount(totalLength, chunkSize)
{
    if (totalLength <= chunkSize)
        return 1;

    var remainder = totalLength % chunkSize;
    var partialLength = totalLength - remainder;
    return (partialLength / chunkSize) + (remainder ? 1 : 0);
}

function outline(params)
{
    const chunkSize = 100000; // characters per data chunk
    const totalLength = params.content.length;
    const lines = params.content.split("\n");
    const chunkCount = getChunkCount(totalLength, chunkSize);
    var outlineChunk = [];
    var previousIdentifier = null;
    var previousToken = null;
    var previousTokenType = null;
    var currentChunk = 1;
    var processedChunkCharacters = 0;
    var addedFunction = false;
    var isReadingArguments = false;
    var argumentsText = "";
    var currentFunction = null;
    var scriptTokenizer = new WebInspector.SourceJavaScriptTokenizer();
    scriptTokenizer.condition = scriptTokenizer.createInitialCondition();

    for (var i = 0; i < lines.length; ++i) {
        var line = lines[i];
        var column = 0;
        scriptTokenizer.line = line;
        do {
            var newColumn = scriptTokenizer.nextToken(column);
            var tokenType = scriptTokenizer.tokenType;
            var tokenValue = line.substring(column, newColumn);
            if (tokenType === "javascript-ident") {
                previousIdentifier = tokenValue;
                if (tokenValue && previousToken === "function") {
                    // A named function: "function f...".
                    currentFunction = { line: i, name: tokenValue };
                    addedFunction = true;
                    previousIdentifier = null;
                }
            } else if (tokenType === "javascript-keyword") {
                if (tokenValue === "function") {
                    if (previousIdentifier && (previousToken === "=" || previousToken === ":")) {
                        // Anonymous function assigned to an identifier: "...f = function..."
                        // or "funcName: function...".
                        currentFunction = { line: i, name: previousIdentifier };
                        addedFunction = true;
                        previousIdentifier = null;
                    }
                }
            } else if (tokenValue === "." && previousTokenType === "javascript-ident")
                previousIdentifier += ".";
            else if (tokenValue === "(" && addedFunction)
                isReadingArguments = true;
            if (isReadingArguments && tokenValue)
                argumentsText += tokenValue;

            if (tokenValue === ")" && isReadingArguments) {
                addedFunction = false;
                isReadingArguments = false;
                currentFunction.arguments = argumentsText.replace(/,[\r\n\s]*/g, ", ").replace(/([^,])[\r\n\s]+/g, "$1");
                argumentsText = "";
                outlineChunk.push(currentFunction);
            }

            if (tokenValue.trim().length) {
                // Skip whitespace tokens.
                previousToken = tokenValue;
                previousTokenType = tokenType;
            }
            processedChunkCharacters += newColumn - column;
            column = newColumn;

            if (processedChunkCharacters >= chunkSize) {
                postMessage({ chunk: outlineChunk, total: chunkCount, index: currentChunk++ });
                outlineChunk = [];
                processedChunkCharacters = 0;
            }
        } while (column < line.length);
    }
    postMessage({ chunk: outlineChunk, total: chunkCount, index: chunkCount });
}

function formatScript(content, mapping, offset, formattedOffset, indentString)
{
    var formattedContent;
    try {
        var tokenizer = new Tokenizer(content);
        var builder = new FormattedContentBuilder(tokenizer.content(), mapping, offset, formattedOffset, indentString);
        var formatter = new JavaScriptFormatter(tokenizer, builder);
        formatter.format();
        formattedContent = builder.content();
    } catch (e) {
        formattedContent = content;
    }
    return formattedContent;
}

WebInspector = {};

Array.prototype.keySet = function()
{
    var keys = {};
    for (var i = 0; i < this.length; ++i)
        keys[this[i]] = true;
    return keys;
};

importScripts("SourceTokenizer.js");
importScripts("SourceHTMLTokenizer.js");
importScripts("SourceJavaScriptTokenizer.js");

HTMLScriptFormatter = function(indentString)
{
    WebInspector.SourceHTMLTokenizer.call(this);
    this._indentString = indentString;
}

HTMLScriptFormatter.prototype = {
    format: function(content)
    {
        this.line = content;
        this._content = content;
        this._formattedContent = "";
        this._mapping = { original: [0], formatted: [0] };
        this._position = 0;

        var cursor = 0;
        while (cursor < this._content.length)
            cursor = this.nextToken(cursor);

        this._formattedContent += this._content.substring(this._position);
        return { content: this._formattedContent, mapping: this._mapping };
    },

    scriptStarted: function(cursor)
    {
        this._formattedContent += this._content.substring(this._position, cursor);
        this._formattedContent += "\n";
        this._position = cursor;
    },

    scriptEnded: function(cursor)
    {
        if (cursor === this._position)
            return;

        var scriptContent = this._content.substring(this._position, cursor);
        this._mapping.original.push(this._position);
        this._mapping.formatted.push(this._formattedContent.length);
        var formattedScriptContent = formatScript(scriptContent, this._mapping, this._position, this._formattedContent.length, this._indentString);

        this._formattedContent += formattedScriptContent;
        this._position = cursor;
    },

    styleSheetStarted: function(cursor)
    {
    },

    styleSheetEnded: function(cursor)
    {
    },

    __proto__: WebInspector.SourceHTMLTokenizer.prototype
}

function require()
{
    return parse;
}

var exports = {};
importScripts("UglifyJS/parse-js.js");
var parse = exports;

importScripts("JavaScriptFormatter.js");
