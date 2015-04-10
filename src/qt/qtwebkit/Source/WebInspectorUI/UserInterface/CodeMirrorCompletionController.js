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

WebInspector.CodeMirrorCompletionController = function(codeMirror, delegate, stopCharactersRegex)
{
    WebInspector.Object.call(this);

    console.assert(codeMirror);

    this._codeMirror = codeMirror;
    this._stopCharactersRegex = stopCharactersRegex || null;
    this._delegate = delegate || null;

    this._startOffset = NaN;
    this._endOffset = NaN;
    this._lineNumber = NaN;
    this._prefix = "";
    this._completions = [];

    this._suggestionsView = new WebInspector.CompletionSuggestionsView(this);

    this._keyMap = {
        "Up": this._handleUpKey.bind(this),
        "Down": this._handleDownKey.bind(this),
        "Right": this._handleRightOrEnterKey.bind(this),
        "Esc": this._handleEscapeKey.bind(this),
        "Enter": this._handleRightOrEnterKey.bind(this),
        "Tab": this._handleTabKey.bind(this),
        "Cmd-A": this._handleHideKey.bind(this),
        "Cmd-Z": this._handleHideKey.bind(this),
        "Shift-Cmd-Z": this._handleHideKey.bind(this),
        "Cmd-Y": this._handleHideKey.bind(this)
    };

    this._handleChangeListener = this._handleChange.bind(this);
    this._handleCursorActivityListener = this._handleCursorActivity.bind(this);
    this._handleHideActionListener = this._handleHideAction.bind(this);

    this._codeMirror.addKeyMap(this._keyMap);

    this._codeMirror.on("change", this._handleChangeListener);
    this._codeMirror.on("cursorActivity", this._handleCursorActivityListener);
    this._codeMirror.on("blur", this._handleHideActionListener);
    this._codeMirror.on("scroll", this._handleHideActionListener);
};

WebInspector.CodeMirrorCompletionController.GenericStopCharactersRegex = /[\s=:;,]/;
WebInspector.CodeMirrorCompletionController.DefaultStopCharactersRegexModeMap = {"css": /[\s:;,{}()]/, "javascript": /[\s=:;,!+\-*/%&|^~?<>.{}()[\]]/};
WebInspector.CodeMirrorCompletionController.BaseExpressionStopCharactersRegexModeMap = {"javascript": /[\s=:;,!+\-*/%&|^~?<>]/};
WebInspector.CodeMirrorCompletionController.OpenBracketCharactersRegex = /[({[]/;
WebInspector.CodeMirrorCompletionController.CloseBracketCharactersRegex = /[)}\]]/;
WebInspector.CodeMirrorCompletionController.MatchingBrackets = {"{": "}", "(": ")", "[": "]", "}": "{", ")": "(", "]": "["};
WebInspector.CodeMirrorCompletionController.CompletionHintStyleClassName = "completion-hint";
WebInspector.CodeMirrorCompletionController.CompletionsHiddenDelay = 250;
WebInspector.CodeMirrorCompletionController.CompletionTypingDelay = 250;
WebInspector.CodeMirrorCompletionController.CompletionOrigin = "+completion";
WebInspector.CodeMirrorCompletionController.DeleteCompletionOrigin = "+delete-completion";

WebInspector.CodeMirrorCompletionController.prototype = {
    constructor: WebInspector.CodeMirrorCompletionController,

    // Public

    get delegate()
    {
        return this._delegate;
    },

    updateCompletions: function(completions, implicitSuffix)
    {
        if (isNaN(this._startOffset) || isNaN(this._endOffset) || isNaN(this._lineNumber))
            return;

        if (!completions || !completions.length) {
            this.hideCompletions();
            return;
        }

        this._completions = completions;

        if (typeof implicitSuffix === "string")
            this._implicitSuffix = implicitSuffix;

        var from = {line: this._lineNumber, ch: this._startOffset};
        var to = {line: this._lineNumber, ch: this._endOffset};

        var firstCharCoords = this._codeMirror.cursorCoords(from);
        var lastCharCoords = this._codeMirror.cursorCoords(to);
        var bounds = new WebInspector.Rect(firstCharCoords.left, firstCharCoords.top, lastCharCoords.right - firstCharCoords.left, firstCharCoords.bottom - firstCharCoords.top);

        // Try to restore the previous selected index, otherwise just select the first.
        var index = this._currentCompletion ? completions.indexOf(this._currentCompletion) : 0;
        if (index === -1)
            index = 0;

        if (this._forced || completions.length > 1 || completions[index] !== this._prefix) {
            // Update and show the suggestion list.
            this._suggestionsView.update(completions, index);
            this._suggestionsView.show(bounds);
        } else if (this._implicitSuffix) {
            // The prefix and the completion exactly match, but there is an implicit suffix.
            // Just hide the suggestion list and keep the completion hint for the implicit suffix.
            this._suggestionsView.hide();
        } else {
            // The prefix and the completion exactly match, hide the completions. Return early so
            // the completion hint isn't updated.
            this.hideCompletions();
            return;
        }

        this._applyCompletionHint(completions[index]);
    },

    isCompletionChange: function(change)
    {
        return this._ignoreChange || change.origin === WebInspector.CodeMirrorCompletionController.CompletionOrigin || change.origin === WebInspector.CodeMirrorCompletionController.DeleteCompletionOrigin;
    },

    isShowingCompletions: function()
    {
        return this._suggestionsView.visible || (this._completionHintMarker && this._completionHintMarker.find());
    },

    isHandlingClickEvent: function()
    {
        return this._suggestionsView.isHandlingClickEvent();
    },

    hideCompletions: function()
    {
        this._suggestionsView.hide();

        this._removeCompletionHint();

        this._startOffset = NaN;
        this._endOffset = NaN;
        this._lineNumber = NaN;
        this._prefix = "";
        this._completions = [];
        this._implicitSuffix = "";
        this._forced = false;

        if (this._completionDelayTimeout) {
            clearTimeout(this._completionDelayTimeout)
            delete this._completionDelayTimeout;
        }

        delete this._currentCompletion;
        delete this._ignoreNextCursorActivity;
    },

    close: function()
    {
        this._codeMirror.removeKeyMap(this._keyMap);

        this._codeMirror.off("change", this._handleChangeListener);
        this._codeMirror.off("cursorActivity", this._handleCursorActivityListener);
        this._codeMirror.off("blur", this._handleHideActionListener);
        this._codeMirror.off("scroll", this._handleHideActionListener);
    },

    // Protected

    completionSuggestionsSelectedCompletion: function(suggestionsView, completionText)
    {
        this._applyCompletionHint(completionText);
    },

    completionSuggestionsClickedCompletion: function(suggestionsView, completionText)
    {
        // The clicked suggestion causes the editor to loose focus. Restore it so the user can keep typing.
        this._codeMirror.focus();

        this._applyCompletionHint(completionText);
        this._commitCompletionHint();
    },

    // Private

    get _currentReplacementText()
    {
        return this._currentCompletion + this._implicitSuffix;
    },

    _hasPendingCompletion: function()
    {
        return !isNaN(this._startOffset) && !isNaN(this._endOffset) && !isNaN(this._lineNumber);
    },

    _notifyCompletionsHiddenSoon: function()
    {
        function notify()
        {
            if (this._completionHintMarker)
                return;

            if (this._delegate && typeof this._delegate.completionControllerCompletionsHidden === "function")
                this._delegate.completionControllerCompletionsHidden(this);
        }

        if (this._notifyCompletionsHiddenIfNeededTimeout)
            clearTimeout(this._notifyCompletionsHiddenIfNeededTimeout);
        this._notifyCompletionsHiddenIfNeededTimeout = setTimeout(notify.bind(this), WebInspector.CodeMirrorCompletionController.CompletionsHiddenDelay);
    },

    _applyCompletionHint: function(completionText)
    {
        console.assert(completionText);
        if (!completionText)
            return;

        function update()
        {
            this._currentCompletion = completionText;

            this._removeCompletionHint(true, true);

            var replacementText = this._currentReplacementText;

            var from = {line: this._lineNumber, ch: this._startOffset};
            var cursor = {line: this._lineNumber, ch: this._endOffset};
            var to = {line: this._lineNumber, ch: this._startOffset + replacementText.length};

            this._codeMirror.replaceRange(replacementText, from, cursor, WebInspector.CodeMirrorCompletionController.CompletionOrigin);
            this._removeLastChangeFromHistory();

            this._codeMirror.setCursor(cursor);

            if (cursor.ch !== to.ch)
                this._completionHintMarker = this._codeMirror.markText(cursor, to, {className: WebInspector.CodeMirrorCompletionController.CompletionHintStyleClassName});
        }

        this._ignoreChange = true;
        this._ignoreNextCursorActivity = true;

        this._codeMirror.operation(update.bind(this));

        delete this._ignoreChange;
    },

    _commitCompletionHint: function()
    {
        function update()
        {
            this._removeCompletionHint(true, true);

            var replacementText = this._currentReplacementText;

            var from = {line: this._lineNumber, ch: this._startOffset};
            var cursor = {line: this._lineNumber, ch: this._endOffset};
            var to = {line: this._lineNumber, ch: this._startOffset + replacementText.length};

            var lastChar = this._currentCompletion.charAt(this._currentCompletion.length - 1);
            var isClosing = ")]}".indexOf(lastChar);
            if (isClosing !== -1)
                to.ch -= 1 + this._implicitSuffix.length;

            this._codeMirror.replaceRange(replacementText, from, cursor, WebInspector.CodeMirrorCompletionController.CompletionOrigin);

            // Don't call _removeLastChangeFromHistory here to allow the committed completion to be undone.

            this._codeMirror.setCursor(to);

            this.hideCompletions();
        }

        this._ignoreChange = true;
        this._ignoreNextCursorActivity = true;

        this._codeMirror.operation(update.bind(this));

        delete this._ignoreChange;
    },

    _removeLastChangeFromHistory: function()
    {
        var history = this._codeMirror.getHistory();

        // We don't expect a undone history. But if there is one clear it. If could lead to undefined behavior.
        console.assert(!history.undone.length);
        history.undone = [];

        // Pop the last item from the done history.
        console.assert(history.done.length);
        history.done.pop();

        this._codeMirror.setHistory(history);
    },

    _removeCompletionHint: function(nonatomic, dontRestorePrefix)
    {
        if (!this._completionHintMarker)
            return;

        this._notifyCompletionsHiddenSoon();

        function update()
        {
            var range = this._completionHintMarker.find();
            if (range) {
                this._completionHintMarker.clear();

                this._codeMirror.replaceRange("", range.from, range.to, WebInspector.CodeMirrorCompletionController.DeleteCompletionOrigin);
                this._removeLastChangeFromHistory();
            }

            this._completionHintMarker = null;

            if (dontRestorePrefix)
                return;

            console.assert(!isNaN(this._startOffset));
            console.assert(!isNaN(this._endOffset));
            console.assert(!isNaN(this._lineNumber));

            var from = {line: this._lineNumber, ch: this._startOffset};
            var to = {line: this._lineNumber, ch: this._endOffset};

            this._codeMirror.replaceRange(this._prefix, from, to, WebInspector.CodeMirrorCompletionController.DeleteCompletionOrigin);
            this._removeLastChangeFromHistory();
        }

        if (nonatomic) {
            update.call(this);
            return;
        }

        this._ignoreChange = true;

        this._codeMirror.operation(update.bind(this));

        delete this._ignoreChange;
    },

    _scanStringForExpression: function(modeName, string, startOffset, direction, allowMiddleAndEmpty, includeStopCharacter, ignoreInitialUnmatchedOpenBracket, stopCharactersRegex)
    {
        console.assert(direction === -1 || direction === 1);

        var stopCharactersRegex = stopCharactersRegex || this._stopCharactersRegex || WebInspector.CodeMirrorCompletionController.DefaultStopCharactersRegexModeMap[modeName] || WebInspector.CodeMirrorCompletionController.GenericStopCharactersRegex;

        function isStopCharacter(character)
        {
            return stopCharactersRegex.test(character);
        }

        function isOpenBracketCharacter(character)
        {
            return WebInspector.CodeMirrorCompletionController.OpenBracketCharactersRegex.test(character);
        }

        function isCloseBracketCharacter(character)
        {
            return WebInspector.CodeMirrorCompletionController.CloseBracketCharactersRegex.test(character);
        }

        function matchingBracketCharacter(character)
        {
            return WebInspector.CodeMirrorCompletionController.MatchingBrackets[character];
        }

        var endOffset = Math.min(startOffset, string.length);

        var endOfLineOrWord = endOffset === string.length || isStopCharacter(string.charAt(endOffset));

        if (!endOfLineOrWord && !allowMiddleAndEmpty)
            return null;

        var bracketStack = [];
        var bracketOffsetStack = [];
        var lastCloseBracketOffset = NaN;

        var startOffset = endOffset;
        var firstOffset = endOffset + direction;
        for (var i = firstOffset; direction > 0 ? i < string.length : i >= 0; i += direction) {
            var character = string.charAt(i);

            // Ignore stop characters when we are inside brackets.
            if (isStopCharacter(character) && !bracketStack.length)
                break;

            if (isCloseBracketCharacter(character)) {
                bracketStack.push(character);
                bracketOffsetStack.push(i);
            } else if (isOpenBracketCharacter(character)) {
                if ((!ignoreInitialUnmatchedOpenBracket || i !== firstOffset) && (!bracketStack.length || matchingBracketCharacter(character) !== bracketStack.lastValue))
                    break;

                bracketOffsetStack.pop();
                bracketStack.pop();
            }

            startOffset = i + (direction > 0 ? 1 : 0);
        }

        if (bracketOffsetStack.length)
            startOffset = bracketOffsetStack.pop() + 1;

        if (includeStopCharacter && startOffset > 0 && startOffset < string.length)
            startOffset += direction;

        if (direction > 0) {
            var tempEndOffset = endOffset;
            endOffset = startOffset;
            startOffset = tempEndOffset;
        }

        return {string: string.substring(startOffset, endOffset), startOffset: startOffset, endOffset: endOffset};
    },

    _completeAtCurrentPosition: function(force)
    {
        if (this._codeMirror.somethingSelected()) {
            this.hideCompletions();
            return;
        }

        if (this._completionDelayTimeout) {
            clearTimeout(this._completionDelayTimeout);
            delete this._completionDelayTimeout;
        }

        this._removeCompletionHint(true, true);

        var cursor = this._codeMirror.getCursor();
        var token = this._codeMirror.getTokenAt(cursor);

        // Don't try to complete inside comments.
        if (token.type && /\bcomment\b/.test(token.type)) {
            this.hideCompletions();
            return;
        }

        var mode = this._codeMirror.getMode();
        var innerMode = CodeMirror.innerMode(mode, token.state).mode;
        var modeName = innerMode.alternateName || innerMode.name;

        var lineNumber = cursor.line;
        var lineString = this._codeMirror.getLine(lineNumber);

        var backwardScanResult = this._scanStringForExpression(modeName, lineString, cursor.ch, -1, force);
        if (!backwardScanResult) {
            this.hideCompletions();
            return;
        }

        var forwardScanResult = this._scanStringForExpression(modeName, lineString, cursor.ch, 1, true, true);
        var suffix = forwardScanResult.string;

        this._ignoreNextCursorActivity = true;

        this._startOffset = backwardScanResult.startOffset;
        this._endOffset = backwardScanResult.endOffset;
        this._lineNumber = lineNumber;
        this._prefix = backwardScanResult.string;
        this._completions = [];
        this._implicitSuffix = "";
        this._forced = force;

        var baseExpressionStopCharactersRegex = WebInspector.CodeMirrorCompletionController.BaseExpressionStopCharactersRegexModeMap[modeName];
        if (baseExpressionStopCharactersRegex)
            var baseScanResult = this._scanStringForExpression(modeName, lineString, this._startOffset, -1, true, false, true, baseExpressionStopCharactersRegex);

        if (!force && !backwardScanResult.string && (!baseScanResult || !baseScanResult.string)) {
            this.hideCompletions();
            return;
        }

        var defaultCompletions = [];

        switch (modeName) {
        case "css":
            defaultCompletions = this._generateCSSCompletions(token, baseScanResult ? baseScanResult.string : null, suffix);
            break;
        case "javascript":
            defaultCompletions = this._generateJavaScriptCompletions(token, baseScanResult ? baseScanResult.string : null, suffix);
            break;
        }

        if (this._delegate && typeof this._delegate.completionControllerCompletionsNeeded === "function")
            this._delegate.completionControllerCompletionsNeeded(this, this._prefix, defaultCompletions, baseScanResult ? baseScanResult.string : null, suffix, force);
        else
            this.updateCompletions(defaultCompletions);
    },

    _generateCSSCompletions: function(mainToken, base, suffix)
    {
        // We only support completion inside CSS rules.
        if (!mainToken.state || !mainToken.state.stack || !mainToken.state.stack.contains("rule"))
            return [];

        var token = mainToken;
        var lineNumber = this._lineNumber;

        // Scan backwards looking for the current property.
        while (token.state.stack.lastValue === "propertyValue") {
            // Found the beginning of the line. Go to the previous line.
            if (!token.start) {
                --lineNumber;

                // No more lines, stop.
                if (lineNumber < 0)
                    break;
            }

            // Get the previous token.
            token = this._codeMirror.getTokenAt({line: lineNumber, ch: token.start ? token.start : Number.MAX_VALUE});
        }

        // If we have a property token and it's not the main token, then we are working on
        // the value for that property and should complete allowed values.
        if (mainToken !== token && token.type && /\bproperty\b/.test(token.type)) {
            var propertyName = token.string;

            // If there is a suffix and it isn't a semicolon, then we should use a space since
            // the user is editing in the middle.
            this._implicitSuffix = suffix && suffix !== ";" ? " " : ";";

            // Don't use an implicit suffix if it would be the same as the existing suffix.
            if (this._implicitSuffix === suffix)
                this._implicitSuffix = "";

            return WebInspector.CSSKeywordCompletions.forProperty(propertyName).startsWith(this._prefix);
        }

        this._implicitSuffix = suffix !== ":" ? ": " : "";

        // Complete property names.
        return WebInspector.CSSCompletions.cssNameCompletions.startsWith(this._prefix);
    },

    _generateJavaScriptCompletions: function(mainToken, base, suffix)
    {
        // If there is a base expression then we should not attempt to match any keywords or variables.
        // Allow only open bracket characters at the end of the base, otherwise leave completions with
        // a base up to the delegate to figure out.
        if (base && !/[({[]$/.test(base))
            return [];

        var matchingWords = [];

        const prefix = this._prefix;

        const declaringVariable = mainToken.state.lexical.type === "vardef";
        const insideSwitch = mainToken.state.lexical.prev ? mainToken.state.lexical.prev.info === "switch" : false;
        const insideBlock = mainToken.state.lexical.prev ? mainToken.state.lexical.prev.type === "}" : false;
        const insideParenthesis = mainToken.state.lexical.type === ")";
        const insideBrackets = mainToken.state.lexical.type === "]";

        const allKeywords = ["break", "case", "catch", "const", "continue", "debugger", "default", "delete", "do", "else", "false", "finally", "for", "function", "if", "in",
            "Infinity", "instanceof", "NaN", "new", "null", "return", "switch", "this", "throw", "true", "try", "typeof", "undefined", "var", "void", "while", "with"];
        const valueKeywords = ["false", "Infinity", "NaN", "null", "this", "true", "undefined"];

        const allowedKeywordsInsideBlocks = allKeywords.keySet();
        const allowedKeywordsWhenDeclaringVariable = valueKeywords.keySet();
        const allowedKeywordsInsideParenthesis = valueKeywords.concat(["function"]).keySet();
        const allowedKeywordsInsideBrackets = allowedKeywordsInsideParenthesis;
        const allowedKeywordsOnlyInsideSwitch = ["case", "default"].keySet();

        function matchKeywords(keywords)
        {
            matchingWords = matchingWords.concat(keywords.filter(function(word) {
                if (!insideSwitch && word in allowedKeywordsOnlyInsideSwitch)
                    return false;
                if (insideBlock && !(word in allowedKeywordsInsideBlocks))
                    return false;
                if (insideBrackets && !(word in allowedKeywordsInsideBrackets))
                    return false;
                if (insideParenthesis && !(word in allowedKeywordsInsideParenthesis))
                    return false;
                if (declaringVariable && !(word in allowedKeywordsWhenDeclaringVariable))
                    return false;
                return word.startsWith(prefix);
            }));
        }

        function matchVariables()
        {
            function filterVariables(variables)
            {
                for (var variable = variables; variable; variable = variable.next) {
                    // Don't match the variable if this token is in a variable declaration.
                    // Otherwise the currently typed text will always match and that isn't useful.
                    if (declaringVariable && variable.name === prefix)
                        continue;

                    if (variable.name.startsWith(prefix) && !matchingWords.contains(variable.name))
                        matchingWords.push(variable.name);
                }
            }

            var context = mainToken.state.context;
            while (context) {
                filterVariables(context.vars);
                context = context.prev;
            }

            filterVariables(mainToken.state.globalVars);
        }

        switch (suffix.substring(0, 1)) {
        case "":
        case " ":
            matchVariables();
            matchKeywords(allKeywords);
            break;

        case ".":
        case "[":
            matchVariables();
            matchKeywords(["false", "Infinity", "NaN", "this", "true"]);
            break;

        case "(":
            matchVariables();
            matchKeywords(["catch", "else", "for", "function", "if", "return", "switch", "throw", "while", "with"]);
            break;

        case "{":
            matchKeywords(["do", "else", "finally", "return", "try"]);
            break;

        case ":":
            if (insideSwitch)
                matchKeywords(["case", "default"]);
            break;

        case ";":
            matchVariables();
            matchKeywords(valueKeywords);
            matchKeywords(["break", "continue", "debugger", "return", "void"]);
            break;
        }

        return matchingWords;
    },

    _handleUpKey: function(codeMirror)
    {
        if (!this._hasPendingCompletion())
            return CodeMirror.Pass;

        if (!this.isShowingCompletions())
            return;

        this._suggestionsView.selectPrevious();
    },

    _handleDownKey: function(codeMirror)
    {
        if (!this._hasPendingCompletion())
            return CodeMirror.Pass;

        if (!this.isShowingCompletions())
            return;

        this._suggestionsView.selectNext();
    },

    _handleRightOrEnterKey: function(codeMirror)
    {
        if (!this._hasPendingCompletion())
            return CodeMirror.Pass;

        if (!this.isShowingCompletions())
            return;

        this._commitCompletionHint();
    },

    _handleEscapeKey: function(codeMirror)
    {
        var delegateImplementsShouldAllowEscapeCompletion = this._delegate && typeof this._delegate.completionControllerShouldAllowEscapeCompletion === "function";
        if (this._hasPendingCompletion())
            this.hideCompletions();
        else if (this._codeMirror.getOption("readOnly"))
            return CodeMirror.Pass;
        else if (!delegateImplementsShouldAllowEscapeCompletion || this._delegate.completionControllerShouldAllowEscapeCompletion(this))
            this._completeAtCurrentPosition(true);
        else
            return CodeMirror.Pass;
    },

    _handleTabKey: function(codeMirror)
    {
        if (!this._hasPendingCompletion())
            return CodeMirror.Pass;

        if (!this.isShowingCompletions())
            return;

        console.assert(this._completions.length);
        if (!this._completions.length)
            return;

        console.assert(this._currentCompletion);
        if (!this._currentCompletion)
            return;

        // Commit the current completion if there is only one suggestion.
        if (this._completions.length === 1) {
            this._commitCompletionHint();
            return;
        }

        var prefixLength = this._prefix.length;

        var commonPrefix = this._completions[0];
        for (var i = 1; i < this._completions.length; ++i) {
            var completion = this._completions[i];
            var lastIndex = Math.min(commonPrefix.length, completion.length);
            for (var j = prefixLength; j < lastIndex; ++j) {
                if (commonPrefix[j] !== completion[j]) {
                    commonPrefix = commonPrefix.substr(0, j);
                    break;
                }
            }
        }

        // Commit the current completion if there is no common prefix that is longer.
        if (commonPrefix === this._prefix) {
            this._commitCompletionHint();
            return;
        }

        // Set the prefix to the common prefix so _applyCompletionHint will insert the
        // common prefix as commited text. Adjust _endOffset to match the new prefix.
        this._prefix = commonPrefix;
        this._endOffset = this._startOffset + commonPrefix.length;

        this._applyCompletionHint(this._currentCompletion);
    },

    _handleChange: function(codeMirror, change)
    {
        if (this.isCompletionChange(change))
            return;

        this._ignoreNextCursorActivity = true;

        if (!change.origin || change.origin.charAt(0) !== "+") {
            this.hideCompletions();
            return;
        }

        // Only complete on delete if we are showing completions already.
        if (change.origin === "+delete" && !this._hasPendingCompletion())
            return;

        if (this._completionDelayTimeout) {
            clearTimeout(this._completionDelayTimeout);
            delete this._completionDelayTimeout;
        }

        if (this._hasPendingCompletion())
            this._completeAtCurrentPosition(false);
        else
            this._completionDelayTimeout = setTimeout(this._completeAtCurrentPosition.bind(this, false), WebInspector.CodeMirrorCompletionController.CompletionTypingDelay);
    },

    _handleCursorActivity: function(codeMirror)
    {
        if (this._ignoreChange)
            return;

        if (this._ignoreNextCursorActivity) {
            delete this._ignoreNextCursorActivity;
            return;
        }

        this.hideCompletions();
    },

    _handleHideKey: function(codeMirror)
    {
        this.hideCompletions();

        return CodeMirror.Pass;
    },

    _handleHideAction: function(codeMirror)
    {
        // Clicking a suggestion causes the editor to blur. We don't want to hide completions in this case.
        if (this.isHandlingClickEvent())
            return;

        this.hideCompletions();
    }
};

WebInspector.CodeMirrorCompletionController.prototype.__proto__ = WebInspector.Object.prototype;
