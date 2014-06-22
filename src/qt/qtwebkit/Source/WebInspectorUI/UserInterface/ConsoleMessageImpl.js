/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
 * Copyright (C) 2007, 2008, 2013 Apple Inc.  All rights reserved.
 * Copyright (C) 2009 Joseph Pecoraro
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.ConsoleMessageImpl = function(source, level, message, linkifier, type, url, line, column, repeatCount, parameters, stackTrace, request)
{
    WebInspector.ConsoleMessage.call(this, source, level, url, line, column, repeatCount);

    this._linkifier = linkifier;
    this.type = type || WebInspector.ConsoleMessage.MessageType.Log;
    this._messageText = message;
    this._parameters = parameters;
    this._stackTrace = stackTrace;
    this._request = request;

    this._customFormatters = {
        "object": this._formatParameterAsObject,
        "array":  this._formatParameterAsArray,
        "node":   this._formatParameterAsNode,
        "string": this._formatParameterAsString
    };
}

WebInspector.ConsoleMessageImpl.prototype = {

    enforcesClipboardPrefixString: true,

    _formatMessage: function()
    {
        this._formattedMessage = document.createElement("span");
        this._formattedMessage.className = "console-message-text source-code";

        var messageText;
        if (this.source === WebInspector.ConsoleMessage.MessageSource.ConsoleAPI) {
            switch (this.type) {
                case WebInspector.ConsoleMessage.MessageType.Trace:
                    messageText = document.createTextNode("console.trace()");
                    break;
                case WebInspector.ConsoleMessage.MessageType.Assert:
                    var args = [WebInspector.UIString("Assertion failed:")];
                    if (this._parameters)
                        args = args.concat(this._parameters);
                    messageText = this._format(args);
                    break;
                case WebInspector.ConsoleMessage.MessageType.Dir:
                    var obj = this._parameters ? this._parameters[0] : undefined;
                    var args = ["%O", obj];
                    messageText = this._format(args);
                    break;
                default:
                    var args = this._parameters || [this._messageText];
                    messageText = this._format(args);
            }
        } else if (this.source === WebInspector.ConsoleMessage.MessageSource.Network) {
            if (this._request) {
                this._stackTrace = this._request.stackTrace;
                if (this._request.initiator && this._request.initiator.url) {
                    this.url = this._request.initiator.url;
                    this.line = this._request.initiator.lineNumber;
                }
                messageText = document.createElement("span");
                if (this.level === WebInspector.ConsoleMessage.MessageLevel.Error) {
                    messageText.appendChild(document.createTextNode(this._request.requestMethod + " "));
                    messageText.appendChild(WebInspector.linkifyRequestAsNode(this._request));
                    if (this._request.failed)
                        messageText.appendChild(document.createTextNode(" " + this._request.localizedFailDescription));
                    else
                        messageText.appendChild(document.createTextNode(" " + this._request.statusCode + " (" + this._request.statusText + ")"));
                } else {
                    var fragment = WebInspector.linkifyStringAsFragmentWithCustomLinkifier(this._messageText, WebInspector.linkifyRequestAsNode.bind(null, this._request, ""));
                    messageText.appendChild(fragment);
                }
            } else {
                if (this.url) {
                    var anchor = WebInspector.linkifyURLAsNode(this.url, this.url, "console-message-url");
                    this._formattedMessage.appendChild(anchor);
                }
                messageText = this._format([this._messageText]);
            }
        } else {
            var args = this._parameters || [this._messageText];
            messageText = this._format(args);
        }

        if (this.source !== WebInspector.ConsoleMessage.MessageSource.Network || this._request) {
            if (this._stackTrace && this._stackTrace.length && this._stackTrace[0].url) {
                var urlElement = this._linkifyCallFrame(this._stackTrace[0]);
                this._formattedMessage.appendChild(urlElement);
            } else if (this.url && this.url !== "undefined") {
                var urlElement = this._linkifyLocation(this.url, this.line, this.column);
                this._formattedMessage.appendChild(urlElement);
            }
        }

        this._formattedMessage.appendChild(messageText);

        if (this._shouldDumpStackTrace()) {
            var ol = document.createElement("ol");
            ol.className = "outline-disclosure";
            var treeOutline = new TreeOutline(ol);

            var content = this._formattedMessage;
            var root = new TreeElement(content, null, true);
            content.treeElementForTest = root;
            treeOutline.appendChild(root);
            if (this.type === WebInspector.ConsoleMessage.MessageType.Trace)
                root.expand();

            this._populateStackTraceTreeElement(root);
            this._formattedMessage = ol;
        }

        // This is used for inline message bubbles in SourceFrames, or other plain-text representations.
        this._message = messageText.textContent;
    },

    _shouldDumpStackTrace: function()
    {
        return !!this._stackTrace && this._stackTrace.length && (this.source === WebInspector.ConsoleMessage.MessageSource.Network || this.level === WebInspector.ConsoleMessage.MessageLevel.Error || this.type === WebInspector.ConsoleMessage.MessageType.Trace);
    },

    get message()
    {
        // force message formatting
        var formattedMessage = this.formattedMessage;
        return this._message;
    },

    get formattedMessage()
    {
        if (!this._formattedMessage)
            this._formatMessage();
        return this._formattedMessage;
    },

    _linkifyLocation: function(url, lineNumber, columnNumber)
    {
        // ConsoleMessage stack trace line numbers are one-based.
        lineNumber = lineNumber ? lineNumber - 1 : 0;

        return WebInspector.linkifyLocation(url, lineNumber, columnNumber, "console-message-url");
    },

    _linkifyCallFrame: function(callFrame)
    {
        return this._linkifyLocation(callFrame.url, callFrame.lineNumber, callFrame.columnNumber);
    },

    isErrorOrWarning: function()
    {
        return (this.level === WebInspector.ConsoleMessage.MessageLevel.Warning || this.level === WebInspector.ConsoleMessage.MessageLevel.Error);
    },

    _format: function(parameters)
    {
        // This node is used like a Builder. Values are continually appended onto it.
        var formattedResult = document.createElement("span");
        if (!parameters.length)
            return formattedResult;

        // Formatting code below assumes that parameters are all wrappers whereas frontend console
        // API allows passing arbitrary values as messages (strings, numbers, etc.). Wrap them here.
        for (var i = 0; i < parameters.length; ++i) {
            // FIXME: Only pass runtime wrappers here.
            if (parameters[i] instanceof WebInspector.RemoteObject)
                continue;

            if (typeof parameters[i] === "object")
                parameters[i] = WebInspector.RemoteObject.fromPayload(parameters[i]);
            else
                parameters[i] = WebInspector.RemoteObject.fromPrimitiveValue(parameters[i]);
        }

        // There can be string log and string eval result. We distinguish between them based on message type.
        var shouldFormatMessage = WebInspector.RemoteObject.type(parameters[0]) === "string" && this.type !== WebInspector.ConsoleMessage.MessageType.Result;

        // Multiple parameters with the first being a format string. Save unused substitutions.
        if (shouldFormatMessage) {
            // Multiple parameters with the first being a format string. Save unused substitutions.
            var result = this._formatWithSubstitutionString(parameters, formattedResult);
            parameters = result.unusedSubstitutions;
            if (parameters.length)
                formattedResult.appendChild(document.createTextNode(" "));
        }

        // Single parameter, or unused substitutions from above.
        for (var i = 0; i < parameters.length; ++i) {
            // Inline strings when formatting.
            if (shouldFormatMessage && parameters[i].type === "string")
                formattedResult.appendChild(document.createTextNode(parameters[i].description));
            else
                formattedResult.appendChild(this._formatParameter(parameters[i]));
            if (i < parameters.length - 1)
                formattedResult.appendChild(document.createTextNode(" "));
        }
        return formattedResult;
    },

    /**
     * @param {boolean=} forceObjectFormat
     */
    _formatParameter: function(output, forceObjectFormat)
    {
        var type;
        if (forceObjectFormat)
            type = "object";
        else if (output instanceof WebInspector.RemoteObject)
            type = output.subtype || output.type;
        else
            type = typeof output;

        var formatter = this._customFormatters[type];
        if (!formatter) {
            formatter = this._formatParameterAsValue;
            output = output.description;
        }

        var span = document.createElement("span");
        span.className = "console-formatted-" + type + " source-code";
        formatter.call(this, output, span);
        return span;
    },

    _formatParameterAsValue: function(val, elem)
    {
        elem.appendChild(document.createTextNode(val));
    },

    _formatParameterAsObject: function(obj, elem)
    {
        elem.appendChild(new WebInspector.ObjectPropertiesSection(obj, obj.description).element);
    },

    _formatParameterAsNode: function(object, elem)
    {
        function printNode(nodeId)
        {
            if (!nodeId) {
                // Sometimes DOM is loaded after the sync message is being formatted, so we get no
                // nodeId here. So we fall back to object formatting here.
                this._formatParameterAsObject(object, elem);
                return;
            }
            var treeOutline = new WebInspector.DOMTreeOutline(false, false, true);
            treeOutline.setVisible(true);
            treeOutline.rootDOMNode = WebInspector.domTreeManager.nodeForId(nodeId);
            treeOutline.element.classList.add("outline-disclosure");
            if (!treeOutline.children[0].hasChildren)
                treeOutline.element.classList.add("single-node");
            elem.appendChild(treeOutline.element);
        }
        object.pushNodeToFrontend(printNode.bind(this));
    },

    _formatParameterAsArray: function(arr, elem)
    {
        arr.getOwnProperties(this._printArray.bind(this, arr, elem));
    },

    _formatParameterAsString: function(output, elem)
    {
        var span = document.createElement("span");
        span.className = "console-formatted-string source-code";
        span.appendChild(WebInspector.linkifyStringAsFragment(output.description));

        // Make black quotes.
        elem.classList.remove("console-formatted-string");
        elem.appendChild(document.createTextNode("\""));
        elem.appendChild(span);
        elem.appendChild(document.createTextNode("\""));
    },

    _printArray: function(array, elem, properties)
    {
        if (!properties)
            return;

        var elements = [];
        for (var i = 0; i < properties.length; ++i) {
            var property = properties[i];
            var name = property.name;
            if (!isNaN(name))
                elements[name] = this._formatAsArrayEntry(property.value);
        }

        elem.appendChild(document.createTextNode("["));
        var lastNonEmptyIndex = -1;

        function appendUndefined(elem, index)
        {
            if (index - lastNonEmptyIndex <= 1)
                return;
            var span = elem.createChild("span", "console-formatted-undefined");
            span.textContent = WebInspector.UIString("undefined × %d").format(index - lastNonEmptyIndex - 1);
        }

        var length = array.arrayLength();
        for (var i = 0; i < length; ++i) {
            var element = elements[i];
            if (!element)
                continue;

            if (i - lastNonEmptyIndex > 1) {
                appendUndefined(elem, i);
                elem.appendChild(document.createTextNode(", "));
            }

            elem.appendChild(element);
            lastNonEmptyIndex = i;
            if (i < length - 1)
                elem.appendChild(document.createTextNode(", "));
        }
        appendUndefined(elem, length);

        elem.appendChild(document.createTextNode("]"));
    },

    _formatAsArrayEntry: function(output)
    {
        // Prevent infinite expansion of cross-referencing arrays.
        return this._formatParameter(output, output.subtype && output.subtype === "array");
    },

    _formatWithSubstitutionString: function(parameters, formattedResult)
    {
        var formatters = {}

        function parameterFormatter(force, obj)
        {
            return this._formatParameter(obj, force);
        }

        function stringFormatter(obj)
        {
            return obj.description;
        }

        function floatFormatter(obj)
        {
            if (typeof obj.value !== "number")
                return parseFloat(obj.description);
            return obj.value;
        }

        function integerFormatter(obj)
        {
            if (typeof obj.value !== "number")
                return parseInt(obj.description);
            return Math.floor(obj.value);
        }

        var currentStyle = null;
        function styleFormatter(obj)
        {
            currentStyle = {};
            var buffer = document.createElement("span");
            buffer.setAttribute("style", obj.description);
            for (var i = 0; i < buffer.style.length; i++) {
                var property = buffer.style[i];
                if (isWhitelistedProperty(property))
                    currentStyle[property] = buffer.style[property];
            }
        }

        function isWhitelistedProperty(property)
        {
            var prefixes = ["background", "border", "color", "font", "line", "margin", "padding", "text", "-webkit-background", "-webkit-border", "-webkit-font", "-webkit-margin", "-webkit-padding", "-webkit-text"];
            for (var i = 0; i < prefixes.length; i++) {
                if (property.startsWith(prefixes[i]))
                    return true;
            }
            return false;
        }

        // Firebug uses %o for formatting objects.
        formatters.o = parameterFormatter.bind(this, false);
        formatters.s = stringFormatter;
        formatters.f = floatFormatter;

        // Firebug allows both %i and %d for formatting integers.
        formatters.i = integerFormatter;
        formatters.d = integerFormatter;

        // Firebug uses %c for styling the message.
        formatters.c = styleFormatter;

        // Support %O to force object formatting, instead of the type-based %o formatting.
        formatters.O = parameterFormatter.bind(this, true);

        function append(a, b)
        {
            if (b instanceof Node)
                a.appendChild(b);
            else if (b) {
                var toAppend = WebInspector.linkifyStringAsFragment(b.toString());
                if (currentStyle) {
                    var wrapper = document.createElement("span");
                    for (var key in currentStyle)
                        wrapper.style[key] = currentStyle[key];
                    wrapper.appendChild(toAppend);
                    toAppend = wrapper;
                }
                a.appendChild(toAppend);
            }
            return a;
        }

        // String.format does treat formattedResult like a Builder, result is an object.
        return String.format(parameters[0].description, parameters.slice(1), formatters, formattedResult, append);
    },

    clearHighlight: function()
    {
        if (!this._formattedMessage)
            return;

        var highlightedMessage = this._formattedMessage;
        delete this._formattedMessage;
        this._formatMessage();
        this._element.replaceChild(this._formattedMessage, highlightedMessage);
    },

    highlightSearchResults: function(regexObject)
    {
        if (!this._formattedMessage)
            return;

        regexObject.lastIndex = 0;
        var text = this.message;
        var match = regexObject.exec(text);
        var offset = 0;
        var matchRanges = [];
        while (match) {
            matchRanges.push({ offset: match.index, length: match[0].length });
            match = regexObject.exec(text);
        }
        highlightSearchResults(this._formattedMessage, matchRanges);
        this._element.scrollIntoViewIfNeeded();
    },

    matchesRegex: function(regexObject)
    {
        return regexObject.test(this.message);
    },

    toMessageElement: function()
    {
        if (this._element)
            return this._element;

        var element = document.createElement("div");
        element.message = this;
        element.className = "console-message";

        this._element = element;

        switch (this.level) {
            case WebInspector.ConsoleMessage.MessageLevel.Tip:
                element.classList.add("console-tip-level");
                break;
            case WebInspector.ConsoleMessage.MessageLevel.Log:
                element.classList.add("console-log-level");
                break;
            case WebInspector.ConsoleMessage.MessageLevel.Debug:
                element.classList.add("console-debug-level");
                break;
            case WebInspector.ConsoleMessage.MessageLevel.Warning:
                element.classList.add("console-warning-level");
                break;
            case WebInspector.ConsoleMessage.MessageLevel.Error:
                element.classList.add("console-error-level");
                break;
        }

        if (this.type === WebInspector.ConsoleMessage.MessageType.StartGroup || this.type === WebInspector.ConsoleMessage.MessageType.StartGroupCollapsed)
            element.classList.add("console-group-title");

        element.appendChild(this.formattedMessage);

        if (this.repeatCount > 1)
            this.updateRepeatCount();

        return element;
    },

    _populateStackTraceTreeElement: function(parentTreeElement)
    {
        for (var i = 0; i < this._stackTrace.length; i++) {
            var frame = this._stackTrace[i];

            var content = document.createElement("div");
            var messageTextElement = document.createElement("span");
            messageTextElement.className = "console-message-text source-code";
            var functionName = frame.functionName || WebInspector.UIString("(anonymous function)");
            messageTextElement.appendChild(document.createTextNode(functionName));
            content.appendChild(messageTextElement);

            if (frame.url) {
                var urlElement = this._linkifyCallFrame(frame);
                content.appendChild(urlElement);
            }

            var treeElement = new TreeElement(content);
            parentTreeElement.appendChild(treeElement);
        }
    },

    updateRepeatCount: function() {
        if (!this.repeatCountElement) {
            this.repeatCountElement = document.createElement("span");
            this.repeatCountElement.className = "bubble";

            this._element.insertBefore(this.repeatCountElement, this._element.firstChild);
        }
        this.repeatCountElement.textContent = this.repeatCount;
    },

    toString: function()
    {
        var sourceString;
        switch (this.source) {
            case WebInspector.ConsoleMessage.MessageSource.HTML:
                sourceString = "HTML";
                break;
            case WebInspector.ConsoleMessage.MessageSource.XML:
                sourceString = "XML";
                break;
            case WebInspector.ConsoleMessage.MessageSource.JS:
                sourceString = "JS";
                break;
            case WebInspector.ConsoleMessage.MessageSource.Network:
                sourceString = "Network";
                break;
            case WebInspector.ConsoleMessage.MessageSource.ConsoleAPI:
                sourceString = "ConsoleAPI";
                break;
            case WebInspector.ConsoleMessage.MessageSource.Other:
                sourceString = "Other";
                break;
        }

        var typeString;
        switch (this.type) {
            case WebInspector.ConsoleMessage.MessageType.Log:
                typeString = "Log";
                break;
            case WebInspector.ConsoleMessage.MessageType.Dir:
                typeString = "Dir";
                break;
            case WebInspector.ConsoleMessage.MessageType.DirXML:
                typeString = "Dir XML";
                break;
            case WebInspector.ConsoleMessage.MessageType.Trace:
                typeString = "Trace";
                break;
            case WebInspector.ConsoleMessage.MessageType.StartGroupCollapsed:
            case WebInspector.ConsoleMessage.MessageType.StartGroup:
                typeString = "Start Group";
                break;
            case WebInspector.ConsoleMessage.MessageType.EndGroup:
                typeString = "End Group";
                break;
            case WebInspector.ConsoleMessage.MessageType.Assert:
                typeString = "Assert";
                break;
            case WebInspector.ConsoleMessage.MessageType.Result:
                typeString = "Result";
                break;
        }

        return sourceString + " " + typeString + " " + this.levelString + ": " + this.formattedMessage.textContent + "\n" + this.url + " line " + this.line;
    },

    get text()
    {
        return this._messageText;
    },

    isEqual: function(msg)
    {
        if (!msg)
            return false;

        if (this._stackTrace) {
            if (!msg._stackTrace)
                return false;
            var l = this._stackTrace;
            var r = msg._stackTrace;
            for (var i = 0; i < l.length; i++) {
                if (l[i].url !== r[i].url ||
                    l[i].functionName !== r[i].functionName ||
                    l[i].lineNumber !== r[i].lineNumber ||
                    l[i].columnNumber !== r[i].columnNumber)
                    return false;
            }
        }

        return (this.source === msg.source)
            && (this.type === msg.type)
            && (this.level === msg.level)
            && (this.line === msg.line)
            && (this.url === msg.url)
            && (this.message === msg.message)
            && (this._request === msg._request);
    },

    get stackTrace()
    {
        return this._stackTrace;
    },

    /**
     * @return {WebInspector.ConsoleMessage}
     */
    clone: function()
    {
        return WebInspector.ConsoleMessage.create(this.source, this.level, this._messageText, this.type, this.url, this.line, this.column, this.repeatCount, this._parameters, this._stackTrace, this._request);
    },

    get levelString()
    {
        switch (this.level) {
            case WebInspector.ConsoleMessage.MessageLevel.Tip:
                return "Tip";
            case WebInspector.ConsoleMessage.MessageLevel.Log:
                return "Log";
            case WebInspector.ConsoleMessage.MessageLevel.Warning:
                return "Warning";
            case WebInspector.ConsoleMessage.MessageLevel.Debug:
                return "Debug";
            case WebInspector.ConsoleMessage.MessageLevel.Error:
                return "Error";
        }
    },

    get clipboarPrefixString ()
    {
        return "[" + this.levelString + "] ";
    },

    toClipboardString: function(isPrefixOptional)
    {
        var isTrace = this._shouldDumpStackTrace();
        
        var clipboardString = "";
        if (this._formattedMessage && !isTrace)
            clipboardString = this._formattedMessage.querySelector("span").innerText;
        else
            clipboardString = this.type === WebInspector.ConsoleMessage.MessageType.Trace ? "console.trace()" : this._message || this._messageText;

        if (!isPrefixOptional || this.enforcesClipboardPrefixString)
            clipboardString = this.clipboarPrefixString + clipboardString;

        if (isTrace) {
            this._stackTrace.forEach(function(frame) {
                clipboardString += "\n\t" + (frame.functionName || WebInspector.UIString("(anonymous function)"));
                if (frame.url)
                    clipboardString += " (" + WebInspector.displayNameForURL(frame.url) + ", line " + frame.lineNumber + ")";
            });
        }
        else {
            var repeatString = this.repeatCount > 1 ? "x" + this.repeatCount : "";

            var urlLine = "";
            if (this.url) {
                var components = [WebInspector.displayNameForURL(this.url), "line " + this.line];
                if (repeatString)
                    components.push(repeatString);
                urlLine = " (" + components.join(", ") + ")";
            } else if (repeatString)
                urlLine = " (" + repeatString + ")";
        
            if (urlLine) {
                var lines = clipboardString.split("\n");
                lines[0] += urlLine;
                clipboardString = lines.join("\n");
            }
        }

        return clipboardString;
    }

}

WebInspector.ConsoleMessageImpl.prototype.__proto__ = WebInspector.ConsoleMessage.prototype;
