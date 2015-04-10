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

WebInspector.JavaScriptLogViewController = function(element, scrollElement, textPrompt, delegate, historySettingIdentifier)
{
    WebInspector.Object.call(this);

    console.assert(textPrompt instanceof WebInspector.ConsolePrompt);
    console.assert(historySettingIdentifier);

    this._element = element;
    this._scrollElement = scrollElement;

    this._promptHistorySetting = new WebInspector.Setting(historySettingIdentifier, null);

    this._prompt = textPrompt;
    this._prompt.delegate = this;
    this._prompt.history = this._promptHistorySetting.value;

    this.delegate = delegate;

    this._cleared = true;
    this._previousMessage = null;
    this._repeatCountWasInterrupted = false;

    this._topConsoleGroups = [];

    this.messagesClearKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Command, "K", this._handleClearShortcut.bind(this));
    this.messagesAlternateClearKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Control, "L", this._handleClearShortcut.bind(this), this._element);

    this._messagesFindKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Command, "F", this._handleFindShortcut.bind(this), this._element);
    this._messagesFindNextKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Command, "G", this._handleFindNextShortcut.bind(this), this._element);
    this._messagesFindPreviousKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Command | WebInspector.KeyboardShortcut.Modifier.Shift, "G", this._handleFindPreviousShortcut.bind(this), this._element);

    this._promptAlternateClearKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Control, "L", this._handleClearShortcut.bind(this), this._prompt.element);
    this._promptFindKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Command, "F", this._handleFindShortcut.bind(this), this._prompt.element);
    this._promptFindNextKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Command, "G", this._handleFindNextShortcut.bind(this), this._prompt.element);
    this._promptFindPreviousKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Command | WebInspector.KeyboardShortcut.Modifier.Shift, "G", this._handleFindPreviousShortcut.bind(this), this._prompt.element);

    WebInspector.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange, this._clearLastProperties, this);

    this.startNewSession();
};

WebInspector.JavaScriptLogViewController.CachedPropertiesDuration = 30000;

WebInspector.JavaScriptLogViewController.prototype = {
    constructor: WebInspector.JavaScriptLogViewController,

    // Public

    get prompt()
    {
        return this._prompt;
    },

    get topConsoleGroup()
    {
        return this._topConsoleGroup;
    },

    get currentConsoleGroup()
    {
        return this._currentConsoleGroup;
    },

    clear: function()
    {
        this._cleared = true;

        this.startNewSession(true);

        this.prompt.focus();

        if (this.delegate && typeof this.delegate.didClearMessages === "function")
            this.delegate.didClearMessages();
    },

    startNewSession: function(clearPreviousSessions)
    {
        if (this._topConsoleGroups.length && clearPreviousSessions) {
            for (var i = 0; i < this._topConsoleGroups.length; ++i)
                this._element.removeChild(this._topConsoleGroups[i].element);

            this._topConsoleGroups = [];
            this._topConsoleGroup = null;
            this._currentConsoleGroup = null;
        }

        // Reuse the top group if it has no messages.
        if (this._topConsoleGroup && !this._topConsoleGroup.hasMessages()) {
            // Make sure the session is visible.
            this._topConsoleGroup.element.scrollIntoView();
            return;
        }

        var hasPreviousSession = !!this._topConsoleGroup;
        var consoleGroup = new WebInspector.ConsoleGroup(null, hasPreviousSession);

        this._previousMessage = null;
        this._repeatCountWasInterrupted = false;

        this._topConsoleGroups.push(consoleGroup);
        this._topConsoleGroup = consoleGroup;
        this._currentConsoleGroup = consoleGroup;

        this._element.appendChild(consoleGroup.element);

        // Make sure the new session is visible.
        consoleGroup.element.scrollIntoView();
    },

    appendConsoleMessage: function(consoleMessage)
    {
        // Clone the message since there might be multiple clients using the message,
        // and since the message has a DOM element it can't be two places at once.
        var messageClone = consoleMessage.clone();

        this._appendConsoleMessage(messageClone);

        return messageClone;
    },

    updatePreviousMessageRepeatCount: function(count)
    {
        console.assert(this._previousMessage);
        if (!this._previousMessage)
            return;

        if (!this._repeatCountWasInterrupted) {
            this._previousMessage.repeatCount = count - (this._previousMessage.ignoredCount || 0);
            this._previousMessage.updateRepeatCount();
        } else {
            var newMessage = this._previousMessage.clone();

            // If this message is repeated after being cloned, messageRepeatCountUpdated will be called with a
            // count that includes the count of this message before cloning. We should ignore instances of this
            // log that occurred before we cloned, so set a property on the message with the number of previous
            // instances of this log that we should ignore.
            newMessage.ignoredCount = newMessage.repeatCount + (this._previousMessage.ignoredCount || 0);
            newMessage.repeatCount = 1;

            this._appendConsoleMessage(newMessage);
        }
    },

    isScrolledToBottom: function()
    {
        // Lie about being scrolled to the bottom if we have a pending request to scroll to the bottom soon.
        return this._scrollToBottomTimeout || this._scrollElement.isScrolledToBottom();
    },

    scrollToBottom: function()
    {
        if (this._scrollToBottomTimeout)
            return;

        function delayedWork()
        {
            this._scrollToBottomTimeout = null;
            this._scrollElement.scrollTop = this._scrollElement.scrollHeight;
        }

        // Don't scroll immediately so we are not causing excessive layouts when there
        // are many messages being added at once.
        this._scrollToBottomTimeout = setTimeout(delayedWork.bind(this), 0);
    },

    // Protected

    consolePromptHistoryDidChange: function(prompt)
    {
        this._promptHistorySetting.value = this.prompt.history;
    },

    consolePromptShouldCommitText: function(prompt, text, cursorIsAtLastPosition, handler)
    {
        // Always commit the text if we are not at the last position.
        if (!cursorIsAtLastPosition) {
            handler(true);
            return;
        }

        // COMPATIBILITY (iOS 6): RuntimeAgent.parse did not exist in iOS 6. Always commit.
        if (!RuntimeAgent.parse) {
            handler(true);
            return;
        }

        function parseFinished(error, result, message, range)
        {
            handler(result !== RuntimeAgent.SyntaxErrorType.Recoverable);
        }

        RuntimeAgent.parse(text, parseFinished.bind(this));
    },

    consolePromptTextCommitted: function(prompt, text)
    {
        console.assert(text);

        var commandMessage = new WebInspector.ConsoleCommand(text);
        this._appendConsoleMessage(commandMessage, true);

        function printResult(result, wasThrown)
        {
            if (!result || this._cleared)
                return;

            this._appendConsoleMessage(new WebInspector.ConsoleCommandResult(result, wasThrown, commandMessage), true);
        }

        this._evaluateInInspectedWindow(text, "console", true, true, false, printResult.bind(this));
    },

    consolePromptCompletionsNeeded: function(prompt, defaultCompletions, base, prefix, suffix, forced)
    {
        // Don't allow non-forced empty prefix completions unless the base is that start of property access.
        if (!forced && !prefix && !/[.[]$/.test(base)) {
            prompt.updateCompletions(null);
            return;
        }

        // If the base ends with an open parentheses or open curly bracket then treat it like there is
        // no base so we get global object completions.
        if (/[({]$/.test(base))
            base = "";

        var lastBaseIndex = base.length - 1;
        var dotNotation = base[lastBaseIndex] === ".";
        var bracketNotation = base[lastBaseIndex] === "[";

        if (dotNotation || bracketNotation) {
            base = base.substring(0, lastBaseIndex);

            // Don't suggest anything for an empty base that is using dot notation.
            // Bracket notation with an empty base will be treated as an array.
            if (!base && dotNotation) {
                prompt.updateCompletions(defaultCompletions);
                return;
            }

            // Don't allow non-forced empty prefix completions if the user is entering a number, since it might be a float.
            // But allow number completions if the base already has a decimal, so "10.0." will suggest Number properties.
            if (!forced && !prefix && dotNotation && base.indexOf(".") === -1 && parseInt(base, 10) == base) {
                prompt.updateCompletions(null);
                return;
            }

            // An empty base with bracket notation is not property access, it is an array.
            // Clear the bracketNotation flag so completions are not quoted.
            if (!base && bracketNotation)
                bracketNotation = false;
        }

        // If the base is the same as the last time, we can reuse the property names we have already gathered.
        // Doing this eliminates delay caused by the async nature of the code below and it only calls getters
        // and functions once instead of repetitively. Sure, there can be difference each time the base is evaluated,
        // but this optimization gives us more of a win. We clear the cache after 30 seconds or when stepping in the
        // debugger to make sure we don't use stale properties in most cases.
        if (this._lastBase === base && this._lastPropertyNames) {
            receivedPropertyNames.call(this, this._lastPropertyNames);
            return;
        }

        this._lastBase = base;
        this._lastPropertyNames = null;

        var activeCallFrame = WebInspector.debuggerManager.activeCallFrame;
        if (!base && activeCallFrame)
            activeCallFrame.collectScopeChainVariableNames(receivedPropertyNames.bind(this));
        else
            this._evaluateInInspectedWindow(base, "completion", true, true, false, evaluated.bind(this));

        function updateLastPropertyNames(propertyNames)
        {
            if (this._clearLastPropertiesTimeout)
                clearTimeout(this._clearLastPropertiesTimeout);
            this._clearLastPropertiesTimeout = setTimeout(this._clearLastProperties.bind(this), WebInspector.JavaScriptLogViewController.CachedPropertiesDuration);

            this._lastPropertyNames = propertyNames || {};
        }

        function evaluated(result, wasThrown)
        {
            if (wasThrown || !result || result.type === "undefined" || (result.type === "object" && result.subtype === "null")) {
                RuntimeAgent.releaseObjectGroup("completion");

                updateLastPropertyNames.call(this, {});
                prompt.updateCompletions(defaultCompletions);

                return;
            }

            function getCompletions(primitiveType)
            {
                var object;
                if (primitiveType === "string")
                    object = new String("");
                else if (primitiveType === "number")
                    object = new Number(0);
                else if (primitiveType === "boolean")
                    object = new Boolean(false);
                else
                    object = this;

                var resultSet = {};
                for (var o = object; o; o = o.__proto__) {
                    try {
                        var names = Object.getOwnPropertyNames(o);
                        for (var i = 0; i < names.length; ++i)
                            resultSet[names[i]] = true;
                    } catch (e) {
                        // Ignore
                    }
                }

                return resultSet;
            }

            if (result.type === "object" || result.type === "function")
                result.callFunctionJSON(getCompletions, undefined, receivedPropertyNames.bind(this));
            else if (result.type === "string" || result.type === "number" || result.type === "boolean")
                this._evaluateInInspectedWindow("(" + getCompletions + ")(\"" + result.type + "\")", "completion", false, true, true, receivedPropertyNamesFromEvaluate.bind(this));
            else
                console.error("Unknown result type: " + result.type);
        }

        function receivedPropertyNamesFromEvaluate(object, wasThrown, result)
        {
            receivedPropertyNames.call(this, result && !wasThrown ? result.value : null);
        }

        function receivedPropertyNames(propertyNames)
        {
            propertyNames = propertyNames || {};

            updateLastPropertyNames.call(this, propertyNames);

            RuntimeAgent.releaseObjectGroup("completion");

            if (!base) {
                const commandLineAPI = ["$", "$$", "$x", "dir", "dirxml", "keys", "values", "profile", "profileEnd", "monitorEvents", "unmonitorEvents", "inspect", "copy", "clear", "getEventListeners", "$0", "$1", "$2", "$3", "$4", "$_"];
                for (var i = 0; i < commandLineAPI.length; ++i)
                    propertyNames[commandLineAPI[i]] = true;
            }

            propertyNames = Object.keys(propertyNames);

            var implicitSuffix = "";
            if (bracketNotation) {
                var quoteUsed = prefix[0] === "'" ? "'" : "\"";
                if (suffix !== "]" && suffix !== quoteUsed)
                    implicitSuffix = "]";
            }

            var completions = defaultCompletions;
            var knownCompletions = completions.keySet();

            for (var i = 0; i < propertyNames.length; ++i) {
                var property = propertyNames[i];

                if (dotNotation && !/^[a-zA-Z_$][a-zA-Z0-9_$]*$/.test(property))
                    continue;

                if (bracketNotation) {
                    if (parseInt(property) != property)
                        property = quoteUsed + property.escapeCharacters(quoteUsed + "\\") + (suffix !== quoteUsed ? quoteUsed : "");
                }

                if (!property.startsWith(prefix) || property in knownCompletions)
                    continue;

                completions.push(property);
                knownCompletions[property] = true;
            }

            function compare(a, b)
            {
                // Try to sort in numerical order first.
                var numericCompareResult = a - b;
                if (!isNaN(numericCompareResult))
                    return numericCompareResult;

                // Not numbers, sort as strings.
                return a.localeCompare(b);
            }

            completions.sort(compare);

            prompt.updateCompletions(completions, implicitSuffix);
        }
    },

    // Private

    _clearLastProperties: function()
    {
        if (this._clearLastPropertiesTimeout) {
            clearTimeout(this._clearLastPropertiesTimeout);
            delete this._clearLastPropertiesTimeout;
        }

        // Clear the cache of property names so any changes while stepping or sitting idle get picked up if the same
        // expression is evaluated again.
        this._lastPropertyNames = null;
    },

    _evaluateInInspectedWindow: function(expression, objectGroup, includeCommandLineAPI, doNotPauseOnExceptionsAndMuteConsole, returnByValue, callback)
    {
        if (!expression) {
            // There is no expression, so the completion should happen against global properties.
            expression = "this";
        }

        function evalCallback(error, result, wasThrown)
        {
            if (error) {
                console.error(error);
                callback(null, false);
                return;
            }

            if (returnByValue)
                callback(null, wasThrown, wasThrown ? null : result);
            else
                callback(WebInspector.RemoteObject.fromPayload(result), wasThrown);
        }

        if (WebInspector.debuggerManager.activeCallFrame) {
            DebuggerAgent.evaluateOnCallFrame(WebInspector.debuggerManager.activeCallFrame.id, expression, objectGroup, includeCommandLineAPI, doNotPauseOnExceptionsAndMuteConsole, returnByValue, evalCallback);
            return;
        }

        // COMPATIBILITY (iOS 6): Execution context identifiers (contextId) did not exist
        // in iOS 6. Fallback to including the frame identifier (frameId).
        var contextId = WebInspector.quickConsole.executionContextIdentifier;
        RuntimeAgent.evaluate.invoke({expression: expression, objectGroup: objectGroup, includeCommandLineAPI: includeCommandLineAPI, doNotPauseOnExceptionsAndMuteConsole: doNotPauseOnExceptionsAndMuteConsole, contextId: contextId, frameId: contextId, returnByValue: returnByValue}, evalCallback);
    },

    _handleClearShortcut: function()
    {
        this.clear();
    },

    _handleFindShortcut: function()
    {
        this.delegate.focusSearchBar();
    },

    _handleFindNextShortcut: function()
    {
        this.delegate.highlightNextSearchMatch();
    },

    _handleFindPreviousShortcut: function()
    {
        this.delegate.highlightPreviousSearchMatch();
    },

    _appendConsoleMessage: function(msg, repeatCountWasInterrupted)
    {
        var wasScrolledToBottom = this.isScrolledToBottom();

        this._cleared = false;
        this._repeatCountWasInterrupted = repeatCountWasInterrupted || false;

        if (!repeatCountWasInterrupted)
            this._previousMessage = msg;

        if (msg.type === WebInspector.ConsoleMessage.MessageType.EndGroup) {
            var parentGroup = this._currentConsoleGroup.parentGroup;
            if (parentGroup)
                this._currentConsoleGroup = parentGroup;
        } else {
            if (msg.type === WebInspector.ConsoleMessage.MessageType.StartGroup || msg.type === WebInspector.ConsoleMessage.MessageType.StartGroupCollapsed) {
                var group = new WebInspector.ConsoleGroup(this._currentConsoleGroup);
                this._currentConsoleGroup.messagesElement.appendChild(group.element);
                this._currentConsoleGroup = group;
            }

            this._currentConsoleGroup.addMessage(msg);
        }

        if (msg.type === WebInspector.ConsoleMessage.MessageType.Result || wasScrolledToBottom)
            this.scrollToBottom();

        if (this.delegate && typeof this.delegate.didAppendConsoleMessage === "function")
            this.delegate.didAppendConsoleMessage(msg);
    }
};

WebInspector.JavaScriptLogViewController.prototype.__proto__ = WebInspector.Object.prototype;
