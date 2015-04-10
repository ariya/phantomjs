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

/**
 * @interface
 */
WebInspector.ContentProvider = function() { }

WebInspector.ContentProvider.prototype = {
    /**
     * @return {string}
     */
    contentURL: function() { },

    /**
     * @return {WebInspector.ResourceType}
     */
    contentType: function() { },

    /**
     * @param {function(?string,boolean,string)} callback
     */
    requestContent: function(callback) { },

    /**
     * @param {string} query
     * @param {boolean} caseSensitive
     * @param {boolean} isRegex
     * @param {function(Array.<WebInspector.ContentProvider.SearchMatch>)} callback
     */
    searchInContent: function(query, caseSensitive, isRegex, callback) { }
}

/**
 * @constructor
 * @param {number} lineNumber
 * @param {string} lineContent
 */
WebInspector.ContentProvider.SearchMatch = function(lineNumber, lineContent) {
    this.lineNumber = lineNumber;
    this.lineContent = lineContent;
}

/**
 * @param {string} content
 * @param {string} query
 * @param {boolean} caseSensitive
 * @param {boolean} isRegex
 * @return {Array.<WebInspector.ContentProvider.SearchMatch>}
 */
WebInspector.ContentProvider.performSearchInContent = function(content, query, caseSensitive, isRegex)
{
    var regex = createSearchRegex(query, caseSensitive, isRegex);

    var result = [];
    var lineEndings = content.lineEndings();
    for (var i = 0; i < lineEndings.length; ++i) {
        var lineStart = i > 0 ? lineEndings[i - 1] + 1 : 0;
        var lineEnd = lineEndings[i];
        var lineContent = content.substring(lineStart, lineEnd);
        if (lineContent.length > 0 && lineContent.charAt(lineContent.length - 1) === "\r")
            lineContent = lineContent.substring(0, lineContent.length - 1)

        if (regex.exec(lineContent))
            result.push(new WebInspector.ContentProvider.SearchMatch(i, lineContent));
    }
    return result;
}
