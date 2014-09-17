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
    var result = {};
    if (event.data.mimeType === "text/html") {
        var formatter = new HTMLScriptFormatter();
        result = formatter.format(event.data.content);
    } else {
        result.mapping = { original: [], formatted: [] };
        result.content = formatScript(event.data.content, result.mapping, 0, 0);
    }
    postMessage(result);
};

function formatScript(content, mapping, offset, formattedOffset)
{
    var formattedContent;
    try {
        var tokenizer = new Tokenizer(content);
        var builder = new FormattedContentBuilder(tokenizer.content(), mapping, offset, formattedOffset);
        var formatter = new JavaScriptFormatter(tokenizer, builder);
        formatter.format();
        formattedContent = builder.content();
    } catch (e) {
        formattedContent = content;
    }
    return formattedContent;
}

WebInspector = {};
importScripts("SourceTokenizer.js");
importScripts("SourceHTMLTokenizer.js");

HTMLScriptFormatter = function()
{
    WebInspector.SourceHTMLTokenizer.call(this);
}

HTMLScriptFormatter.prototype = {
    format: function(content)
    {
        this.line = content;
        this._content = content;
        this._formattedContent = "";
        this._mapping = { original: [], formatted: [] };
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
        var formattedScriptContent = formatScript(scriptContent, this._mapping, this._position, this._formattedContent.length);

        this._formattedContent += formattedScriptContent;
        this._position = cursor;
    },

    styleSheetStarted: function(cursor)
    {
    },

    styleSheetEnded: function(cursor)
    {
    }
}

HTMLScriptFormatter.prototype.__proto__ = WebInspector.SourceHTMLTokenizer.prototype;

function require()
{
    return parse;
}

var exports = {};
importScripts("UglifyJS/parse-js.js");
var parse = exports;

importScripts("JavaScriptFormatter.js");
