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
 * @interface
 */
WebInspector.TextEditor = function() { };

WebInspector.TextEditor.Events = {
    GutterClick: "gutterClick"
};

WebInspector.TextEditor.prototype = {
    /**
     * @return {boolean}
     */
    isClean: function() { },

    markClean: function() { },

    /*
     * @param {number} lineNumber
     * @param {number} column
     * @return {?{x: number, y: number, height: number}}
     */
    cursorPositionToCoordinates: function(lineNumber, column) { },

    /**
     * @param {number} x
     * @param {number} y
     * @return {?WebInspector.TextRange}
     */
    coordinatesToCursorPosition: function(x, y) { },

    /**
     * @param {number} lineNumber
     * @param {number} column
     * @return {?{startColumn: number, endColumn: number, type: string}}
     */
    tokenAtTextPosition: function(lineNumber, column) { },

    /**
     * @param {string} mimeType
     */
    set mimeType(mimeType) { },

    /**
     * @param {boolean} readOnly
     */
    setReadOnly: function(readOnly) { },

    /**
     * @return {boolean}
     */
    readOnly: function() { },

    /**
     * @return {Element}
     */
    defaultFocusedElement: function() { },

    /**
     * @param {string} regex
     * @param {string} cssClass
     * @return {Object}
     */
    highlightRegex: function(regex, cssClass) { },

    /**
     * @param {WebInspector.TextRange} range
     * @param {string} cssClass
     * @return {Object}
     */
    highlightRange: function(range, cssClass) { },

    /**
     * @param {Object} highlightDescriptor
     */
    removeHighlight: function(highlightDescriptor) { },

    /**
     * @param {number} lineNumber
     */
    revealLine: function(lineNumber) { },

    /**
     * @param {number} lineNumber
     * @param {boolean} disabled
     * @param {boolean} conditional
     */
    addBreakpoint: function(lineNumber, disabled, conditional) { },

    /**
     * @param {number} lineNumber
     */
    removeBreakpoint: function(lineNumber) { },

    /**
     * @param {number} lineNumber
     */
    setExecutionLine: function(lineNumber) { },

    clearExecutionLine: function() { },

    /**
     * @param {number} lineNumber
     * @param {Element} element
     */
    addDecoration: function(lineNumber, element) { },

    /**
     * @param {number} lineNumber
     * @param {Element} element
     */
    removeDecoration: function(lineNumber, element) { },

    /**
     * @param {WebInspector.TextRange} range
     */
    markAndRevealRange: function(range) { },

    /**
     * @param {number} lineNumber
     */
    highlightLine: function(lineNumber) { },

    clearLineHighlight: function() { },

    /**
     * @return {Array.<Element>}
     */
    elementsToRestoreScrollPositionsFor: function() { },

    /**
     * @param {WebInspector.TextEditor} textEditor
     */
    inheritScrollPositions: function(textEditor) { },

    beginUpdates: function() { },

    endUpdates: function() { },

    onResize: function() { },

    /**
     * @param {WebInspector.TextRange} range
     * @param {string} text
     * @return {WebInspector.TextRange}
     */
    editRange: function(range, text) { },

    /**
     * @param {number} lineNumber
     */
    scrollToLine: function(lineNumber) { },

    /**
     * @return {WebInspector.TextRange}
     */
    selection: function() { },

    /**
     * @return {WebInspector.TextRange?}
     */
    lastSelection: function() { },

    /**
     * @param {WebInspector.TextRange} textRange
     */
    setSelection: function(textRange) { },

    /**
     * @param {WebInspector.TextRange} range
     * @return {string}
     */
    copyRange: function(range) { },

    /**
     * @param {string} text 
     */
    setText: function(text) { },

    /**
     * @return {string}
     */
    text: function() { },

    /**
     * @return {WebInspector.TextRange}
     */
    range: function() { },

    /**
     * @param {number} lineNumber
     * @return {string}
     */
    line: function(lineNumber) { },

    /**
     * @return {number}
     */
    get linesCount() { },

    /**
     * @param {number} line
     * @param {string} name  
     * @param {Object?} value  
     */
    setAttribute: function(line, name, value) { },

    /**
     * @param {number} line
     * @param {string} name  
     * @return {Object|null} value  
     */
    getAttribute: function(line, name) { },

    /**
     * @param {number} line
     * @param {string} name
     */
    removeAttribute: function(line, name) { },

    wasShown: function() { },

    willHide: function() { }
}

/**
 * @interface
 */
WebInspector.TextEditorDelegate = function()
{
}

WebInspector.TextEditorDelegate.prototype = {
    /**
     * @param {WebInspector.TextRange} oldRange
     * @param {WebInspector.TextRange} newRange
     */
    onTextChanged: function(oldRange, newRange) { },

    /**
     * @param {WebInspector.TextRange} textRange
     */
    selectionChanged: function(textRange) { },

    /**
     * @param {number} lineNumber
     */
    scrollChanged: function(lineNumber) { },

    /**
     * @param {WebInspector.ContextMenu} contextMenu
     * @param {number} lineNumber
     */
    populateLineGutterContextMenu: function(contextMenu, lineNumber) { },

    /**
     * @param {WebInspector.ContextMenu} contextMenu
     * @param {number} lineNumber
     */
    populateTextAreaContextMenu: function(contextMenu, lineNumber) { },

    /**
     * @param {string} hrefValue
     * @param {boolean} isExternal
     * @return {Element}
     */
    createLink: function(hrefValue, isExternal) { }
}
