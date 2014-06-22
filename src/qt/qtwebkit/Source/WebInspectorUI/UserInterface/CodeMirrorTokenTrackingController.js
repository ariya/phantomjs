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

WebInspector.CodeMirrorTokenTrackingController = function(codeMirror, delegate)
{
    WebInspector.Object.call(this);

    console.assert(codeMirror);

    this._codeMirror = codeMirror;
    this._delegate = delegate || null;
    this._mode = WebInspector.CodeMirrorTokenTrackingController.Mode.NonSymbolTokens;

    this._mouseOverDelayDuration = 0;
    this._mouseOutReleaseDelayDuration = 0;
    this._classNameForHighlightedRange = null;

    this._tracking = false;
    this._hoveredTokenInfo = null;
};

WebInspector.CodeMirrorTokenTrackingController.JumpToSymbolHighlightStyleClassName = "jump-to-symbol-highlight";

WebInspector.CodeMirrorTokenTrackingController.Mode = {
    NonSymbolTokens: "non-symbol-tokens",
    JavaScriptExpression: "javascript-expression",
}

WebInspector.CodeMirrorTokenTrackingController.prototype = {
    constructor: WebInspector.CodeMirrorTokenTrackingController,

    // Public

    get delegate()
    {
        return this._delegate;
    },

    set delegate(x)
    {
        this._delegate = x;
    },

    get mode()
    {
        return this._mode;
    },

    set mode(x)
    {
        var oldMode = this._mode;

        this._mode = x || WebInspector.CodeMirrorTokenTrackingController.Mode.NonSymbolTokens;

        if (oldMode !== this._mode && this._tracking && this._hoveredTokenInfo)
            this._processNewHoveredToken();
    },

    get mouseOverDelayDuration()
    {
        return this._mouseOverDelayDuration;
    },

    set mouseOverDelayDuration(x)
    {
        console.assert(x >= 0);
        this._mouseOverDelayDuration = Math.max(x, 0);
    },

    get mouseOutReleaseDelayDuration()
    {
        return this._mouseOutReleaseDelayDuration;
    },

    set mouseOutReleaseDelayDuration(x)
    {
        console.assert(x >= 0);
        this._mouseOutReleaseDelayDuration = Math.max(x, 0);
    },

    get classNameForHighlightedRange()
    {
        return this._classNameForHighlightedRange;
    },

    set classNameForHighlightedRange(x)
    {
        this._classNameForHighlightedRange = x || null;
    },

    get tracking()
    {
        return this._tracking;
    },

    get candidate()
    {
        return this._candidate;
    },

    startTracking: function()
    {
        console.assert(!this._tracking);
        if (this._tracking)
            return;

        this._tracking = true;

        var wrapper = this._codeMirror.getWrapperElement();
        wrapper.addEventListener("mousemove", this, true);
        wrapper.addEventListener("mouseout", this, false);
        wrapper.addEventListener("mousedown", this, false);
        wrapper.addEventListener("mouseup", this, false);
        window.addEventListener("blur", this, true);
    },

    stopTracking: function()
    {
        console.assert(this._tracking);
        if (!this._tracking)
            return;

        this._tracking = false;

        var wrapper = this._codeMirror.getWrapperElement();
        wrapper.removeEventListener("mousemove", this, true);
        wrapper.removeEventListener("mouseout", this, false);
        wrapper.removeEventListener("mousedown", this, false);
        wrapper.removeEventListener("mouseup", this, false);
        window.removeEventListener("blur", this, true);
        window.removeEventListener("mousemove", this, true);

        clearTimeout(this._tokenHoverTimer);
        delete this._selectionMayBeInProgress;
        delete this._hoveredTokenInfo;
    },

    highlightRange: function(range)
    {
        this.removeHighlightedRange();

        var className = this._classNameForHighlightedRange || "";
        this._codeMirrorMarkedText = this._codeMirror.markText(range.start, range.end, {className: className});

        window.addEventListener("mousemove", this, true);
    },

    removeHighlightedRange: function()
    {
        if (!this._codeMirrorMarkedText)
            return;

        this._codeMirrorMarkedText.clear();
        delete this._codeMirrorMarkedText;

        window.removeEventListener("mousemove", this, true);
    },

    boundsForRange: function(range)
    {
        var firstCharCoords = this._codeMirror.cursorCoords(range.start);
        var lastCharCoords = this._codeMirror.cursorCoords(range.end);
        return new WebInspector.Rect(firstCharCoords.left, firstCharCoords.top, lastCharCoords.right - firstCharCoords.left, firstCharCoords.bottom - firstCharCoords.top);
    },

    // Private

    handleEvent: function(event)
    {
        switch (event.type) {
        case "mousemove":
            if (event.currentTarget === window)
                this._mouseMovedWithMarkedText(event);
            else
                this._mouseMovedOverEditor(event);
            break;
        case "mouseout":
            // Only deal with a mouseout event that has the editor wrapper as the target.
            if (!event.currentTarget.contains(event.relatedTarget))
                this._mouseMovedOutOfEditor(event);
            break;
        case "mousedown":
            this._mouseButtonWasPressedOverEditor(event);
            break;
        case "mouseup":
            this._mouseButtonWasReleasedOverEditor(event);
            break;
        case "blur":
            this._windowLostFocus(event);
            break;
        }
    },

    _mouseMovedWithMarkedText: function(event)
    {
        var shouldRelease = !event.target.classList.contains(this._classNameForHighlightedRange);
        if (shouldRelease && this._delegate && typeof this._delegate.tokenTrackingControllerCanReleaseHighlightedRange === "function")
            shouldRelease = this._delegate.tokenTrackingControllerCanReleaseHighlightedRange(this, event.target);

        if (shouldRelease) {
            if (!this._markedTextMouseoutTimer)
                this._markedTextMouseoutTimer = setTimeout(this._markedTextIsNoLongerHovered.bind(this), this._mouseOutReleaseDelayDuration);
            return;
        }

        clearTimeout(this._markedTextMouseoutTimer);
        delete this._markedTextMouseoutTimer;
    },

    _markedTextIsNoLongerHovered: function()
    {
        if (this._delegate && typeof this._delegate.tokenTrackingControllerHighlightedRangeReleased === "function")
            this._delegate.tokenTrackingControllerHighlightedRangeReleased(this);
        delete this._markedTextMouseoutTimer;
    },

    _mouseMovedOverEditor: function(event)
    {
        // Get the position in the text and the token at that position.
        var position = this._codeMirror.coordsChar({left: event.pageX, top: event.pageY});
        var token = this._codeMirror.getTokenAt(position);

        if (!token || !token.type || !token.string) {
            clearTimeout(this._tokenHoverTimer);
            delete this._hoveredTokenInfo;
            return;
        }

        // Stop right here if we're hovering the same token as we were last time.
        if (this._hoveredTokenInfo &&
            this._hoveredTokenInfo.position.line === position.line &&
            this._hoveredTokenInfo.token.start === token.start &&
            this._hoveredTokenInfo.token.end === token.end)
            return;

        // We have a new hovered token.
        var innerMode = CodeMirror.innerMode(this._codeMirror.getMode(), token.state);
        var codeMirrorModeName = innerMode.mode.alternateName || innerMode.mode.name;
        this._hoveredTokenInfo = {
            token: token,
            position: position,
            innerMode: innerMode,
            modeName: codeMirrorModeName
        };

        clearTimeout(this._tokenHoverTimer);

        if (this._codeMirrorMarkedText || !this._mouseOverDelayDuration)
            this._processNewHoveredToken();
        else
            this._tokenHoverTimer = setTimeout(this._processNewHoveredToken.bind(this), this._mouseOverDelayDuration);
    },

    _mouseMovedOutOfEditor: function(event)
    {
        clearTimeout(this._tokenHoverTimer);
        delete this._hoveredTokenInfo;
        delete this._selectionMayBeInProgress;
    },

    _mouseButtonWasPressedOverEditor: function(event)
    {
        this._selectionMayBeInProgress = true;
    },

    _mouseButtonWasReleasedOverEditor: function(event)
    {
        delete this._selectionMayBeInProgress;
        this._mouseMovedOverEditor(event);

        if (this._codeMirrorMarkedText && this._hoveredTokenInfo) {
            var position = this._codeMirror.coordsChar({left: event.pageX, top: event.pageY});
            var marks = this._codeMirror.findMarksAt(position);
            for (var i = 0; i < marks.length; ++i) {
                if (marks[i] === this._codeMirrorMarkedText) {
                    if (this._delegate && typeof this._delegate.tokenTrackingControllerHighlightedRangeWasClicked === "function") {
                        // Trigger the clicked delegate asynchronously, letting the editor complete handling of the click.
                        setTimeout(function() { this._delegate.tokenTrackingControllerHighlightedRangeWasClicked(this); }.bind(this), 0);
                    }
                    break;
                }
            }
        }
    },

    _windowLostFocus: function(event)
    {
        delete this._selectionMayBeInProgress;
    },

    _processNewHoveredToken: function()
    {
        console.assert(this._hoveredTokenInfo);

        if (this._selectionMayBeInProgress)
            return;

        this._candidate = null;

        switch (this._mode) {
        case WebInspector.CodeMirrorTokenTrackingController.Mode.NonSymbolTokens:
            this._candidate = this._processNonSymbolToken();
            break;
        case WebInspector.CodeMirrorTokenTrackingController.Mode.JavaScriptExpression:
            this._candidate = this._processJavaScriptExpression();
            break;
        }

        if (!this._candidate)
            return;

        clearTimeout(this._markedTextMouseoutTimer);
        delete this._markedTextMouseoutTimer;

        if (this._delegate && typeof this._delegate.tokenTrackingControllerNewHighlightCandidate === "function")
            this._delegate.tokenTrackingControllerNewHighlightCandidate(this, this._candidate);
    },

    _processNonSymbolToken: function()
    {
        // Ignore any symbol tokens.
        var type = this._hoveredTokenInfo.token.type;
        if (!type)
            return null;

        var startPosition = {line: this._hoveredTokenInfo.position.line, ch: this._hoveredTokenInfo.token.start};
        var endPosition = {line: this._hoveredTokenInfo.position.line, ch: this._hoveredTokenInfo.token.end};

        return {
            hoveredToken: this._hoveredTokenInfo.token,
            hoveredTokenRange: {start: startPosition, end: endPosition},
        };
    },

    _processJavaScriptExpression: function()
    {
        // Only valid within JavaScript.
        if (this._hoveredTokenInfo.modeName !== "javascript")
            return null;

        // We only handle vars, definitions, properties, and the keyword 'this'.
        var type = this._hoveredTokenInfo.token.type;
        var isProperty = type.indexOf("property") !== -1;
        var isKeyword = type.indexOf("keyword") !== -1;
        if (!isProperty && !isKeyword && type.indexOf("variable") === -1 && type.indexOf("def") === -1)
            return null;

        // Not object literal properties.
        var state = this._hoveredTokenInfo.innerMode.state;
        if (isProperty && state.lexical && state.lexical.type === "}")
            return null;

        // Only the "this" keyword.
        if (isKeyword && this._hoveredTokenInfo.token.string !== "this")
            return null;

        // Work out the full hovered expression.
        var expression = this._hoveredTokenInfo.token.string;
        var expressionStartPosition = {line: this._hoveredTokenInfo.position.line, ch: this._hoveredTokenInfo.token.start};
        var startPosition = {line: this._hoveredTokenInfo.position.line, ch: this._hoveredTokenInfo.token.start};
        var endPosition = {line: this._hoveredTokenInfo.position.line, ch: this._hoveredTokenInfo.token.end};
        while (true) {
            var token = this._codeMirror.getTokenAt(expressionStartPosition);
            var isDot = token && !token.type && token.string === ".";
            var isExpression = token && token.type && token.type.indexOf("m-javascript") !== -1;
            if (!isDot && !isExpression)
                break;
            expression = token.string + expression;
            expressionStartPosition.ch = token.start;
        }

        // Return the candidate for this token and expression.
        return {
            hoveredToken: this._hoveredTokenInfo.token,
            hoveredTokenRange: {start: startPosition, end: endPosition},
            expression: expression,
            expressionRange: {start: expressionStartPosition, end: endPosition},
        };
    }
};

WebInspector.CodeMirrorCompletionController.prototype.__proto__ = WebInspector.Object.prototype;
