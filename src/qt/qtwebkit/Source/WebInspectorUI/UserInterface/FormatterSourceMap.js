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

WebInspector.FormatterSourceMap = function(originalLineEndings, formattedLineEndings, mapping)
{
    WebInspector.Object.call(this);

    this._originalLineEndings = originalLineEndings;
    this._formattedLineEndings = formattedLineEndings;
    this._mapping = mapping;
};

WebInspector.FormatterSourceMap.fromBuilder = function(builder)
{
    return new WebInspector.FormatterSourceMap(builder.originalLineEndings, builder.formattedLineEndings, builder.mapping);
}

WebInspector.FormatterSourceMap.prototype = {
    constructor: WebInspector.FormatterSourceMap,

    // Public

    originalToFormatted: function(lineNumber, columnNumber)
    {
        var originalPosition = this._locationToPosition(this._originalLineEndings, lineNumber || 0, columnNumber || 0);
        var formattedPosition = this._convertPosition(this._mapping.original, this._mapping.formatted, originalPosition);
        return this._positionToLocation(this._formattedLineEndings, formattedPosition);
    },

    formattedToOriginal: function(lineNumber, columnNumber)
    {
        var formattedPosition = this._locationToPosition(this._formattedLineEndings, lineNumber || 0, columnNumber || 0);
        var originalPosition = this._convertPosition(this._mapping.formatted, this._mapping.original, formattedPosition);
        return this._positionToLocation(this._originalLineEndings, originalPosition);
    },

    // Private

    _locationToPosition: function(lineEndings, lineNumber, columnNumber)
    {
        var lineOffset = lineNumber ? lineEndings[lineNumber - 1] + 1 : 0;
        return lineOffset + columnNumber;
    },

    _positionToLocation: function(lineEndings, position)
    {
        var lineNumber = lineEndings.upperBound(position - 1);
        if (!lineNumber)
            var columnNumber = position;
        else
            var columnNumber = position - lineEndings[lineNumber - 1] - 1;
        return {lineNumber:lineNumber, columnNumber:columnNumber};
    },

    _convertPosition: function(positions1, positions2, positionInPosition1)
    {
        var index = positions1.upperBound(positionInPosition1) - 1;
        var convertedPosition = positions2[index] + positionInPosition1 - positions1[index];
        if (index < positions2.length - 1 && convertedPosition > positions2[index + 1])
            convertedPosition = positions2[index + 1];
        return convertedPosition;
    }
};

WebInspector.FormatterSourceMap.__proto__ = WebInspector.Object;
