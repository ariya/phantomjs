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

(function () {
    // By default CodeMirror defines syntax highlighting styles based on token
    // only and shared styles between modes. This limiting and does not match
    // what we have done in the Web Inspector. So this modifies the XML, CSS
    // and JavaScript modes to supply two styles for each token. One for the
    // token and one with the mode name.

    function tokenizeLinkString(stream, state)
    {
        console.assert(state._linkQuoteCharacter !== undefined);

        // Eat the string until the same quote is found that started the string.
        // If this is unquoted, then eat until whitespace or common parse errors.
        if (state._linkQuoteCharacter)
            stream.eatWhile(new RegExp("[^" + state._linkQuoteCharacter + "]"));
        else
            stream.eatWhile(/[^\s\u00a0=<>\"\']/);

        // If the stream isn't at the end of line then we found the end quote.
        // In the case, change _linkTokenize to parse the end of the link next.
        // Otherwise _linkTokenize will stay as-is to parse more of the link.
        if (!stream.eol())
            state._linkTokenize = tokenizeEndOfLinkString;

        return "link";
    }

    function tokenizeEndOfLinkString(stream, state)
    {
        console.assert(state._linkQuoteCharacter !== undefined);
        console.assert(state._linkBaseStyle);

        // Eat the quote character to style it with the base style.
        if (state._linkQuoteCharacter)
            stream.eat(state._linkQuoteCharacter);

        var style = state._linkBaseStyle;

        // Clean up the state.
        delete state._linkTokenize;
        delete state._linkQuoteCharacter;
        delete state._linkBaseStyle;

        return style;
    }

    function extendedXMLToken(stream, state)
    {
        if (state._linkTokenize) {
            // Call the link tokenizer instead.
            var style = state._linkTokenize(stream, state);
            return style && (style + " m-" + this.name);
        }

        // Remember the start position so we can rewind if needed.
        var startPosition = stream.pos;
        var style = this._token(stream, state);

        if (style === "attribute") {
            // Look for "href" or "src" attributes. If found then we should
            // expect a string later that should get the "link" style instead.
            var text = stream.current().toLowerCase();
            if (text === "href" || text === "src")
                state._expectLink = true;
            else
                delete state._expectLink;
        } else if (state._expectLink && style === "string") {
            delete state._expectLink;

            // This is a link, so setup the state to process it next.
            state._linkTokenize = tokenizeLinkString;
            state._linkBaseStyle = style;

            // The attribute may or may not be quoted.
            var quote = stream.current()[0];
            state._linkQuoteCharacter = quote === "'" || quote === "\"" ? quote : null;

            // Rewind the steam to the start of this token.
            stream.pos = startPosition;

            // Eat the open quote of the string so the string style
            // will be used for the quote character.
            if (state._linkQuoteCharacter)
                stream.eat(state._linkQuoteCharacter);
        } else if (style) {
            // We don't expect other tokens between attribute and string since
            // spaces and the equal character are not tokenized. So if we get
            // another token before a string then we stop expecting a link.
            delete state._expectLink;
        }

        return style && (style + " m-" + this.name);
    }

    function tokenizeCSSURLString(stream, state)
    {
        console.assert(state._urlQuoteCharacter);

        // If we are an unquoted url string, return whitespace blocks as a whitespace token (null).
        if (state._unquotedURLString && stream.eatSpace())
            return null;

        var ch = null;
        var escaped = false;
        var reachedEndOfURL = false;
        var lastNonWhitespace = stream.pos;
        var quote = state._urlQuoteCharacter;

        // Parse characters until the end of the stream/line or a proper end quote character.
        while ((ch = stream.next()) != null) {
            if (ch == quote && !escaped) {
                reachedEndOfURL = true;
                break;
            }
            escaped = !escaped && ch === "\\";
            if (!/[\s\u00a0]/.test(ch))
                lastNonWhitespace = stream.pos;
        }

        // If we are an unquoted url string, do not include trailing whitespace, rewind to the last real character.
        if (state._unquotedURLString)
            stream.pos = lastNonWhitespace;

        // If we have reached the proper the end of the url string, switch to the end tokenizer to reset the state.
        if (reachedEndOfURL) {
            if (!state._unquotedURLString)
                stream.backUp(1);
            this._urlTokenize = tokenizeEndOfCSSURLString;
        }

        return "link";
    }

    function tokenizeEndOfCSSURLString(stream, state)
    {
        console.assert(state._urlQuoteCharacter);
        console.assert(state._urlBaseStyle);

        // Eat the quote character to style it with the base style.
        if (!state._unquotedURLString)
            stream.eat(state._urlQuoteCharacter);

        var style = state._urlBaseStyle;

        delete state._urlTokenize;
        delete state._urlQuoteCharacter;
        delete state._urlBaseStyle;

        return style;
    }

    function extendedCSSToken(stream, state)
    {
        if (state._urlTokenize) {
            // Call the link tokenizer instead.
            var style = state._urlTokenize(stream, state);
            return style && (style + " m-" + (this.alternateName || this.name));
        }

        // Remember the start position so we can rewind if needed.
        var startPosition = stream.pos;
        var style = this._token(stream, state);

        if (style) {
            if (style === "variable-2" && stream.current() === "url") {
                // If the current text is "url" then we should expect the next string token to be a link.
                state._expectLink = true;
            } else if (state._expectLink && style === "string") {
                // We expected a string and got it. This is a link. Parse it the way we want it.
                delete state._expectLink;

                // This is a link, so setup the state to process it next.
                state._urlTokenize = tokenizeCSSURLString;
                state._urlBaseStyle = style;

                // The url may or may not be quoted.
                var quote = stream.current()[0];
                state._urlQuoteCharacter = quote === "'" || quote === "\"" ? quote : ")";
                state._unquotedURLString = state._urlQuoteCharacter === ")";

                // Rewind the steam to the start of this token.
                stream.pos = startPosition;

                // Eat the open quote of the string so the string style
                // will be used for the quote character.
                if (!state._unquotedURLString)
                    stream.eat(state._urlQuoteCharacter);
            } else if (state._expectLink) {
                // We expected a string and didn't get one. Cleanup.
                delete state._expectLink;
            }
        }

        return style && (style + " m-" + (this.alternateName || this.name));
    }

    function extendedToken(stream, state)
    {
        // CodeMirror moves the original token function to _token when we extended it.
        // So call it to get the style that we will add an additional class name to.
        var style = this._token(stream, state);
        return style && (style + " m-" + (this.alternateName || this.name));
    }

    function extendedCSSRuleStartState(base)
    {
        // CodeMirror moves the original token function to _startState when we extended it.
        // So call it to get the original start state that we will modify.
        var state = this._startState(base);

        // Start the stack off like it has already parsed a rule. This causes everything
        // after to be parsed as properties in a rule.
        state.stack = ["rule"];

        return state;
    }

    CodeMirror.extendMode("css-base", {token: extendedCSSToken, alternateName: "css"});
    CodeMirror.extendMode("xml", {token: extendedXMLToken});
    CodeMirror.extendMode("javascript", {token: extendedToken});

    CodeMirror.defineMode("css-rule", CodeMirror.modes.css);
    CodeMirror.extendMode("css-rule", {startState: extendedCSSRuleStartState});

    CodeMirror.defineExtension("hasLineClass", function(line, where, className) {
        // This matches the arguments to addLineClass and removeLineClass.
        var classProperty = (where === "text" ? "textClass" : (where == "background" ? "bgClass" : "wrapClass"));
        var lineInfo = this.lineInfo(line);
        if (!lineInfo)
            return false;

        if (!lineInfo[classProperty])
            return false;

        // Test for the simple case.
        if (lineInfo[classProperty] === className)
            return true;

        // Do a quick check for the substring. This is faster than a regex, which requires escaping the input first.
        var index = lineInfo[classProperty].indexOf(className);
        if (index === -1)
            return false;

        // Check that it is surrounded by spaces. Add padding spaces first to work with beginning and end of string cases.
        var paddedClass = " " + lineInfo[classProperty] + " ";
        return paddedClass.indexOf(" " + className + " ", index) !== -1;
    });

    CodeMirror.defineExtension("toggleLineClass", function(line, where, className) {
        if (this.hasLineClass(line, where, className)) {
            this.removeLineClass(line, where, className);
            return false;
        }

        this.addLineClass(line, where, className);
        return true;
    });

    CodeMirror.defineExtension("alterNumberInRange", function(amount, startPosition, endPosition, affectsSelection) {
        // We don't try if the range is multiline, pass to another key handler.
        if (startPosition.line !== endPosition.line)
            return false;

        var line = this.getLine(startPosition.line);

        var foundPeriod = false;

        var start = NaN;
        var end = NaN;

        for (var i = startPosition.ch; i >= 0; --i) {
            var character = line.charAt(i);

            if (character === ".") {
                if (foundPeriod)
                    break;
                foundPeriod = true;
            } else if (character !== "-" && character !== "+" && isNaN(parseInt(character))) {
                // Found the end already, just scan backwards.
                if (i === startPosition.ch) {
                    end = i;
                    continue;
                }

                break;
            }

            start = i;
        }

        if (isNaN(end)) {
            for (var i = startPosition.ch + 1; i < line.length; ++i) {
                var character = line.charAt(i);

                if (character === ".") {
                    if (foundPeriod) {
                        end = i;
                        break;
                    }

                    foundPeriod = true;
                } else if (isNaN(parseInt(character))) {
                    end = i;
                    break;
                }

                end = i + 1;
            }
        }

        // No number range found, pass to another key handler.
        if (isNaN(start) || isNaN(end))
            return false;

        var number = parseFloat(line.substring(start, end));
        if (number < 1 && number >= -1 && amount === 1)
            amount = 0.1;
        else if (number <= 1 && number > -1 && amount === -1)
            amount = -0.1;

        // Make the new number and constrain it to a precision of 6, this matches numbers the engine returns.
        // Use the Number constructor to forget the fixed precision, so 1.100000 will print as 1.1.
        var alteredNumber = Number((number + amount).toFixed(6));
        var alteredNumberString = alteredNumber.toString();

        var from = {line: startPosition.line, ch: start};
        var to = {line: startPosition.line, ch: end};

        this.replaceRange(alteredNumberString, from, to);

        if (affectsSelection) {
            var newTo = {line: startPosition.line, ch: from.ch + alteredNumberString.length};

            // Fix up the selection so it follows the increase or decrease in the replacement length.
            if (endPosition.ch >= to.ch)
                endPosition = newTo;

            if (startPosition.ch >= to.ch)
                startPosition = newTo;

            this.setSelection(startPosition, endPosition);
        }

        return true;
    });

    function alterNumber(amount, codeMirror)
    {
        var startPosition = codeMirror.getCursor("anchor");
        var endPosition = codeMirror.getCursor("head");

        var foundNumber = codeMirror.alterNumberInRange(amount, startPosition, endPosition, true);
        if (!foundNumber)
            return CodeMirror.Pass;
    }

    function ignoreKey(codeMirror)
    {
        // Do nothing to ignore the key.
    }

    CodeMirror.keyMap["default"] = {
        "Alt-Up": alterNumber.bind(null, 1),
        "Shift-Alt-Up": alterNumber.bind(null, 10),
        "Alt-PageUp": alterNumber.bind(null, 10),
        "Shift-Alt-PageUp": alterNumber.bind(null, 100),
        "Alt-Down": alterNumber.bind(null, -1),
        "Shift-Alt-Down": alterNumber.bind(null, -10),
        "Alt-PageDown": alterNumber.bind(null, -10),
        "Shift-Alt-PageDown": alterNumber.bind(null, -100),
        "Cmd-/": "toggleComment",
        "Shift-Tab": ignoreKey,
        fallthrough: "macDefault"
    };

    // Register some extra MIME-types for CodeMirror. These are in addition to the
    // ones CodeMirror already registers, like text/html, text/javascript, etc.
    const extraXMLTypes = ["text/xml", "text/xsl"];
    extraXMLTypes.forEach(function(type) {
        CodeMirror.defineMIME(type, "xml");
    });

    const extraHTMLTypes = ["application/xhtml+xml", "image/svg+xml"];
    extraHTMLTypes.forEach(function(type) {
        CodeMirror.defineMIME(type, "htmlmixed");
    });

    const extraJavaScriptTypes = ["text/ecmascript", "application/javascript", "application/ecmascript", "application/x-javascript",
        "text/x-javascript", "text/javascript1.1", "text/javascript1.2", "text/javascript1.3", "text/jscript", "text/livescript"];
    extraJavaScriptTypes.forEach(function(type) {
        CodeMirror.defineMIME(type, "javascript");
    });

    const extraJSONTypes = ["application/x-json", "text/x-json"];
    extraJSONTypes.forEach(function(type) {
        CodeMirror.defineMIME(type, {name: "javascript", json: true});
    });

})();

WebInspector.compareCodeMirrorPositions = function(a, b)
{
    var lineCompare = a.line - b.line;
    if (lineCompare !== 0)
        return lineCompare;

    var aColumn = "ch" in a ? a.ch : Number.MAX_VALUE;
    var bColumn = "ch" in b ? b.ch : Number.MAX_VALUE;
    return aColumn - bColumn;
};
