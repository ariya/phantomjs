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

if (!("bind" in Function.prototype)) {
    Object.defineProperty(Function.prototype, "bind",
    {
        value: function(thisObject)
        {
            var func = this;
            var args = Array.prototype.slice.call(arguments, 1);
            return function bound()
            {
                return func.apply(thisObject, args.concat(Array.prototype.slice.call(arguments, 0)));
            };
        }
    });
}

Object.defineProperty(Object, "shallowCopy",
{
    value: function(object)
    {
        // Make a new object and copy all the key/values. The values are not copied.
        var copy = {};
        var keys = Object.keys(object);
        for (var i = 0; i < keys.length; ++i)
            copy[keys[i]] = object[keys[i]];
        return copy;
    }
});

Object.defineProperty(Object, "shallowEqual",
{
    value: function(a, b)
    {
        // Checks if two objects have the same top-level properties.

        // Check for strict equality in case they are the same object.
        if (a === b)
            return true;

        // Only objects can proceed. null is an object, but Object.keys throws for null.
        if (typeof a !== "object" || typeof b !== "object" || a === null || b === null)
            return false;

        var aKeys = Object.keys(a);
        var bKeys = Object.keys(b);

        // Check that each object has the same number of keys.
        if (aKeys.length !== bKeys.length)
            return false;

        // Check if all the keys and their values are equal.
        for (var i = 0; i < aKeys.length; ++i) {
            // Check that b has the same key as a.
            if (!(aKeys[i] in b))
                return false;

            // Check that the values are strict equal since this is only
            // a shallow check, not a recursive one.
            if (a[aKeys[i]] !== b[aKeys[i]])
                return false;
        }

        return true;
    }
});

Object.defineProperty(Object.prototype, "valueForCaseInsensitiveKey",
{
    value: function(key)
    {
        if (this.hasOwnProperty(key))
            return this[key];

        var lowerCaseKey = key.toLowerCase();
        for (var currentKey in this) {
            if (currentKey.toLowerCase() === lowerCaseKey)
                return this[currentKey];
        }

        return undefined;
    }
});

Object.defineProperty(Node.prototype, "enclosingNodeOrSelfWithClass",
{
    value: function(className)
    {
        for (var node = this; node && node !== this.ownerDocument; node = node.parentNode)
            if (node.nodeType === Node.ELEMENT_NODE && node.classList.contains(className))
                return node;
        return null;
    }
});

Object.defineProperty(Node.prototype, "enclosingNodeWithClass",
{
    value: function(className)
    {
        if (!this.parentNode)
            return null;
        return this.parentNode.enclosingNodeOrSelfWithClass(className);
    }
});

Object.defineProperty(Node.prototype, "enclosingNodeOrSelfWithNodeNameInArray",
{
    value: function(nameArray)
    {
        var lowerCaseNameArray = nameArray.map(function(name) { return name.toLowerCase() });
        for (var node = this; node && node !== this.ownerDocument; node = node.parentNode) {
            for (var i = 0; i < nameArray.length; ++i) {
                if (node.nodeName.toLowerCase() === lowerCaseNameArray[i])
                    return node;
            }
        }

        return null;
    }
});

Object.defineProperty(Node.prototype, "enclosingNodeOrSelfWithNodeName",
{
    value: function(nodeName)
    {
        return this.enclosingNodeOrSelfWithNodeNameInArray([nodeName]);
    }
});

Object.defineProperty(Node.prototype, "isAncestor",
{
    value: function(node)
    {
        if (!node)
            return false;

        var currentNode = node.parentNode;
        while (currentNode) {
            if (this === currentNode)
                return true;
            currentNode = currentNode.parentNode;
        }

        return false;
    }
});

Object.defineProperty(Node.prototype, "isDescendant",
{
    value: function(descendant)
    {
        return !!descendant && descendant.isAncestor(this);
    }
});


Object.defineProperty(Node.prototype, "isSelfOrAncestor",
{
    value: function(node)
    {
        return !!node && (node === this || this.isAncestor(node));
    }
});


Object.defineProperty(Node.prototype, "isSelfOrDescendant",
{
    value: function(node)
    {
        return !!node && (node === this || this.isDescendant(node));
    }
});

Object.defineProperty(Node.prototype, "traverseNextNode",
{
    value: function(stayWithin)
    {
        var node = this.firstChild;
        if (node)
            return node;

        if (stayWithin && this === stayWithin)
            return null;

        node = this.nextSibling;
        if (node)
            return node;

        node = this;
        while (node && !node.nextSibling && (!stayWithin || !node.parentNode || node.parentNode !== stayWithin))
            node = node.parentNode;
        if (!node)
            return null;

        return node.nextSibling;
    }
});

Object.defineProperty(Node.prototype, "traversePreviousNode",
{
    value: function(stayWithin)
    {
       if (stayWithin && this === stayWithin)
            return null;
        var node = this.previousSibling;
        while (node && node.lastChild)
            node = node.lastChild;
        if (node)
            return node;
        return this.parentNode;
    }
});


Object.defineProperty(Node.prototype, "traverseNextTextNode",
{
    value: function(stayWithin)
    {
        var node = this.traverseNextNode(stayWithin);
        if (!node)
            return;

        while (node && node.nodeType !== Node.TEXT_NODE)
            node = node.traverseNextNode(stayWithin);

        return node;
    }
});

Object.defineProperty(Node.prototype, "rangeBoundaryForOffset",
{
    value: function(offset)
    {
        var textNode = this.traverseNextTextNode(this);
        while (textNode && offset > textNode.data.length) {
            offset -= textNode.data.length;
            textNode = textNode.traverseNextTextNode(this);
        }

        if (!textNode)
            return {container: this, offset: 0};
        return {container: textNode, offset: offset};
    }
});

Object.defineProperty(Node.prototype, "rangeOfWord",
{
    value: function(offset, stopCharacters, stayWithinNode, direction)
    {
        var startNode;
        var startOffset = 0;
        var endNode;
        var endOffset = 0;

        if (!stayWithinNode)
            stayWithinNode = this;

        if (!direction || direction === "backward" || direction === "both") {
            var node = this;
            while (node) {
                if (node === stayWithinNode) {
                    if (!startNode)
                        startNode = stayWithinNode;
                    break;
                }

                if (node.nodeType === Node.TEXT_NODE) {
                    var start = (node === this ? (offset - 1) : (node.nodeValue.length - 1));
                    for (var i = start; i >= 0; --i) {
                        if (stopCharacters.indexOf(node.nodeValue[i]) !== -1) {
                            startNode = node;
                            startOffset = i + 1;
                            break;
                        }
                    }
                }

                if (startNode)
                    break;

                node = node.traversePreviousNode(stayWithinNode);
            }

            if (!startNode) {
                startNode = stayWithinNode;
                startOffset = 0;
            }
        } else {
            startNode = this;
            startOffset = offset;
        }

        if (!direction || direction === "forward" || direction === "both") {
            node = this;
            while (node) {
                if (node === stayWithinNode) {
                    if (!endNode)
                        endNode = stayWithinNode;
                    break;
                }

                if (node.nodeType === Node.TEXT_NODE) {
                    var start = (node === this ? offset : 0);
                    for (var i = start; i < node.nodeValue.length; ++i) {
                        if (stopCharacters.indexOf(node.nodeValue[i]) !== -1) {
                            endNode = node;
                            endOffset = i;
                            break;
                        }
                    }
                }

                if (endNode)
                    break;

                node = node.traverseNextNode(stayWithinNode);
            }

            if (!endNode) {
                endNode = stayWithinNode;
                endOffset = stayWithinNode.nodeType === Node.TEXT_NODE ? stayWithinNode.nodeValue.length : stayWithinNode.childNodes.length;
            }
        } else {
            endNode = this;
            endOffset = offset;
        }

        var result = this.ownerDocument.createRange();
        result.setStart(startNode, startOffset);
        result.setEnd(endNode, endOffset);

        return result;

    }
});

if (!("remove" in Element.prototype)) {
    Object.defineProperty(Element.prototype, "remove",
    {
        value: function()
        {
            if (this.parentNode)
                this.parentNode.removeChild(this);
        }
    });
}

Object.defineProperty(Element.prototype, "totalOffsetLeft",
{
    get: function()
    {
        return this.getBoundingClientRect().left;
    }
});

Object.defineProperty(Element.prototype, "totalOffsetTop",
{
    get: function()
    {
        return this.getBoundingClientRect().top;
    }
});

Object.defineProperty(Element.prototype, "removeChildren",
{
    value: function()
    {
        // This has been tested to be the fastest removal method.
        if (this.firstChild)
            this.textContent = "";
    }
});

Object.defineProperty(Element.prototype, "isInsertionCaretInside",
{
    value: function()
    {
        var selection = window.getSelection();
        if (!selection.rangeCount || !selection.isCollapsed)
            return false;
        var selectionRange = selection.getRangeAt(0);
        return selectionRange.startContainer === this || selectionRange.startContainer.isDescendant(this);
    }
});

Object.defineProperty(Element.prototype, "removeMatchingStyleClasses",
{
    value: function(classNameRegex)
    {
        var regex = new RegExp("(^|\\s+)" + classNameRegex + "($|\\s+)");
        if (regex.test(this.className))
            this.className = this.className.replace(regex, " ");
    }
});

Object.defineProperty(Element.prototype, "createChild",
{
    value: function(elementName, className)
    {
        var element = this.ownerDocument.createElement(elementName);
        if (className)
            element.className = className;
        this.appendChild(element);
        return element;
    }
});

function AnchorBox(x, y, width, height)
{
    this.x = x || 0;
    this.y = y || 0;
    this.width = width || 0;
    this.height = height || 0;
}

Object.defineProperty(Element.prototype, "boxInWindow",
{
    value: function()
    {
        var anchorBox = new AnchorBox;

        var clientRect = this.getBoundingClientRect();
        console.assert(clientRect);

        anchorBox.x = clientRect.left;
        anchorBox.y = clientRect.top;
        anchorBox.width = clientRect.width;
        anchorBox.height = clientRect.height;

        return anchorBox;
    }
});

Object.defineProperty(Element.prototype, "positionAt",
{
    value: function(x, y)
    {
        this.style.left = x + "px";
        this.style.top = y + "px";
    }
});

Object.defineProperty(Element.prototype, "pruneEmptyTextNodes",
{
    value: function()
    {
        var sibling = this.firstChild;
        while (sibling) {
            var nextSibling = sibling.nextSibling;
            if (sibling.nodeType === this.TEXT_NODE && sibling.nodeValue === "")
                this.removeChild(sibling);
            sibling = nextSibling;
        }
    }
});

Object.defineProperty(Element.prototype, "isScrolledToBottom",
{
    value: function()
    {
        // This code works only for 0-width border
        return this.scrollTop + this.clientHeight === this.scrollHeight;
    }
});

Object.defineProperty(DocumentFragment.prototype, "createChild",
{
    value: Element.prototype.createChild
});

Object.defineProperty(Array.prototype, "contains",
{
    value: function(value)
    {
        return this.indexOf(value) !== -1;
    }
});

Object.defineProperty(Array.prototype, "lastValue",
{
    get: function()
    {
        if (!this.length)
            return undefined;
        return this[this.length - 1];
    }
});

Object.defineProperty(Array.prototype, "remove",
{
    value: function(value, onlyFirst)
    {
        for (var i = this.length - 1; i >= 0; --i) {
            if (this[i] === value) {
                this.splice(i, 1);
                if (onlyFirst)
                    return;
            }
        }
    }
});

Object.defineProperty(Array.prototype, "keySet",
{
    value: function()
    {
        var keys = {};
        for (var i = 0; i < this.length; ++i)
            keys[this[i]] = true;
        return keys;
    }
});

Object.defineProperty(Array.prototype, "upperBound",
{
    value: function(value)
    {
        var first = 0;
        var count = this.length;
        while (count > 0) {
          var step = count >> 1;
          var middle = first + step;
          if (value >= this[middle]) {
              first = middle + 1;
              count -= step + 1;
          } else
              count = step;
        }
        return first;
    }
});

Object.defineProperty(Array, "convert",
{
    value: function(list, startIndex, endIndex)
    {
        return Array.prototype.slice.call(list, startIndex, endIndex);
    }
});

Object.defineProperty(String.prototype, "trimMiddle",
{
    value: function(maxLength)
    {
        if (this.length <= maxLength)
            return this;
        var leftHalf = maxLength >> 1;
        var rightHalf = maxLength - leftHalf - 1;
        return this.substr(0, leftHalf) + "\u2026" + this.substr(this.length - rightHalf, rightHalf);
    }
});

Object.defineProperty(String.prototype, "trimEnd",
{
    value: function(maxLength)
    {
        if (this.length <= maxLength)
            return this;
        return this.substr(0, maxLength - 1) + "\u2026";
    }
});

Object.defineProperty(String.prototype, "collapseWhitespace",
{
    value: function()
    {
        return this.replace(/[\s\xA0]+/g, " ");
    }
});

Object.defineProperty(String.prototype, "escapeCharacters",
{
    value: function(chars)
    {
        var foundChar = false;
        for (var i = 0; i < chars.length; ++i) {
            if (this.indexOf(chars.charAt(i)) !== -1) {
                foundChar = true;
                break;
            }
        }

        if (!foundChar)
            return this;

        var result = "";
        for (var i = 0; i < this.length; ++i) {
            if (chars.indexOf(this.charAt(i)) !== -1)
                result += "\\";
            result += this.charAt(i);
        }

        return result;
    }
});

Object.defineProperty(String.prototype, "escapeForRegExp",
{
    value: function()
    {
        return this.escapeCharacters("^[]{}()\\.$*+?|");
    }
});

Object.defineProperty(String.prototype, "capitalize",
{
    value: function()
    {
        return this.charAt(0).toUpperCase() + this.slice(1);
    }
});

Object.defineProperty(String, "tokenizeFormatString",
{
    value: function(format)
    {
        var tokens = [];
        var substitutionIndex = 0;

        function addStringToken(str)
        {
            tokens.push({ type: "string", value: str });
        }

        function addSpecifierToken(specifier, precision, substitutionIndex)
        {
            tokens.push({ type: "specifier", specifier: specifier, precision: precision, substitutionIndex: substitutionIndex });
        }

        var index = 0;
        for (var precentIndex = format.indexOf("%", index); precentIndex !== -1; precentIndex = format.indexOf("%", index)) {
            addStringToken(format.substring(index, precentIndex));
            index = precentIndex + 1;

            if (format[index] === "%") {
                addStringToken("%");
                ++index;
                continue;
            }

            if (!isNaN(format[index])) {
                // The first character is a number, it might be a substitution index.
                var number = parseInt(format.substring(index), 10);
                while (!isNaN(format[index]))
                    ++index;

                // If the number is greater than zero and ends with a "$",
                // then this is a substitution index.
                if (number > 0 && format[index] === "$") {
                    substitutionIndex = (number - 1);
                    ++index;
                }
            }

            var precision = -1;
            if (format[index] === ".") {
                // This is a precision specifier. If no digit follows the ".",
                // then the precision should be zero.
                ++index;

                precision = parseInt(format.substring(index), 10);
                if (isNaN(precision))
                    precision = 0;

                while (!isNaN(format[index]))
                    ++index;
            }

            addSpecifierToken(format[index], precision, substitutionIndex);

            ++substitutionIndex;
            ++index;
        }

        addStringToken(format.substring(index));

        return tokens;
    }
});

Object.defineProperty(String.prototype, "startsWith", 
{
    value: function(string)
    {
        return this.lastIndexOf(string, 0) === 0;
    }
});

Object.defineProperty(String.prototype, "hash",
{
    get: function()
    {
        // Matches the wtf/StringHasher.h (SuperFastHash) algorithm.

        // Arbitrary start value to avoid mapping all 0's to all 0's.
        const stringHashingStartValue = 0x9e3779b9;

        var result = stringHashingStartValue;
        var pendingCharacter = null;
        for (var i = 0; i < this.length; ++i) {
            var currentCharacter = this[i].charCodeAt(0);
            if (pendingCharacter === null) {
                pendingCharacter = currentCharacter
                continue;
            }

            result += pendingCharacter;
            result = (result << 16) ^ ((currentCharacter << 11) ^ result);
            result += result >> 11;

            pendingCharacter = null;
        }

        // Handle the last character in odd length strings.
        if (pendingCharacter !== null) {
            result += pendingCharacter;
            result ^= result << 11;
            result += result >> 17;
        }

        // Force "avalanching" of final 31 bits.
        result ^= result << 3;
        result += result >> 5;
        result ^= result << 2;
        result += result >> 15;
        result ^= result << 10;

        // Prevent 0 and negative results.
        return (0xffffffff + result + 1).toString(36);
    }
});

Object.defineProperty(String, "standardFormatters",
{
    value: {
        d: function(substitution)
        {
            return !isNaN(substitution) ? substitution : 0;
        },

        f: function(substitution, token)
        {
            if (substitution && token.precision > -1)
                substitution = substitution.toFixed(token.precision);
            return !isNaN(substitution) ? substitution : (token.precision > -1 ? Number(0).toFixed(token.precision) : 0);
        },

        s: function(substitution)
        {
            return substitution;
        }
    }
});

Object.defineProperty(String, "format",
{
    value: function(format, substitutions, formatters, initialValue, append)
    {
        if (!format || !substitutions || !substitutions.length)
            return { formattedResult: append(initialValue, format), unusedSubstitutions: substitutions };

        function prettyFunctionName()
        {
            return "String.format(\"" + format + "\", \"" + substitutions.join("\", \"") + "\")";
        }

        function warn(msg)
        {
            console.warn(prettyFunctionName() + ": " + msg);
        }

        function error(msg)
        {
            console.error(prettyFunctionName() + ": " + msg);
        }

        var result = initialValue;
        var tokens = String.tokenizeFormatString(format);
        var usedSubstitutionIndexes = {};

        for (var i = 0; i < tokens.length; ++i) {
            var token = tokens[i];

            if (token.type === "string") {
                result = append(result, token.value);
                continue;
            }

            if (token.type !== "specifier") {
                error("Unknown token type \"" + token.type + "\" found.");
                continue;
            }

            if (token.substitutionIndex >= substitutions.length) {
                // If there are not enough substitutions for the current substitutionIndex
                // just output the format specifier literally and move on.
                error("not enough substitution arguments. Had " + substitutions.length + " but needed " + (token.substitutionIndex + 1) + ", so substitution was skipped.");
                result = append(result, "%" + (token.precision > -1 ? token.precision : "") + token.specifier);
                continue;
            }

            usedSubstitutionIndexes[token.substitutionIndex] = true;

            if (!(token.specifier in formatters)) {
                // Encountered an unsupported format character, treat as a string.
                warn("unsupported format character \u201C" + token.specifier + "\u201D. Treating as a string.");
                result = append(result, substitutions[token.substitutionIndex]);
                continue;
            }

            result = append(result, formatters[token.specifier](substitutions[token.substitutionIndex], token));
        }

        var unusedSubstitutions = [];
        for (var i = 0; i < substitutions.length; ++i) {
            if (i in usedSubstitutionIndexes)
                continue;
            unusedSubstitutions.push(substitutions[i]);
        }

        return {formattedResult: result, unusedSubstitutions: unusedSubstitutions};
    }
});

Object.defineProperty(String.prototype, "format",
{
    value: function()
    {
        return String.format(this, arguments, String.standardFormatters, "", function(a, b) { return a + b; }).formattedResult;
    }
});

Object.defineProperty(String.prototype, "insertWordBreakCharacters",
{
    value: function()
    {
        // Add zero width spaces after characters that are good to break after.
        // Otherwise a string with no spaces will not break and overflow its container.
        // This is mainly used on URL strings, so the characters are tailored for URLs.
        return this.replace(/([\/;:\)\]\}&?])/g, "$1\u200b");
    }
});

Object.defineProperty(String.prototype, "removeWordBreakCharacters",
{
    value: function()
    {
        // Undoes what insertWordBreakCharacters did.
        return this.replace(/\u200b/g, "");
    }
});

Object.defineProperty(Number, "constrain",
{
    value: function(num, min, max)
    {
        if (num < min)
            num = min;
        else if (num > max)
            num = max;
        return num;
    }
});

Object.defineProperty(Number, "secondsToString",
{
    value: function(seconds, higherResolution)
    {
        var ms = seconds * 1000;

        if (higherResolution && ms < 100)
            return WebInspector.UIString("%.2fms").format(ms);
        else if (ms < 100)
            return WebInspector.UIString("%.1fms").format(ms);

        if (higherResolution && ms < 1000)
            return WebInspector.UIString("%.1fms").format(ms);
        else if (ms < 1000)
            return WebInspector.UIString("%.0fms").format(ms);

        if (seconds < 60)
            return WebInspector.UIString("%.2fs").format(seconds);

        var minutes = seconds / 60;
        if (minutes < 60)
            return WebInspector.UIString("%.1fmin").format(minutes);

        var hours = minutes / 60;
        if (hours < 24)
            return WebInspector.UIString("%.1fhrs").format(hours);

        var days = hours / 24;
        return WebInspector.UIString("%.1f days").format(days);
    }
});

Object.defineProperty(Number, "bytesToString",
{
    value: function(bytes, higherResolution)
    {
        if (higherResolution === undefined)
            higherResolution = true;

        if (bytes < 1024)
            return WebInspector.UIString("%.0f B").format(bytes);

        var kilobytes = bytes / 1024;
        if (higherResolution && kilobytes < 1024)
            return WebInspector.UIString("%.2f KB").format(kilobytes);
        else if (kilobytes < 1024)
            return WebInspector.UIString("%.0f KB").format(kilobytes);

        var megabytes = kilobytes / 1024;
        if (higherResolution)
            return WebInspector.UIString("%.2f MB").format(megabytes);
        else
            return WebInspector.UIString("%.0f MB").format(megabytes);
    }
});

Object.defineProperty(Uint32Array, "isLittleEndian",
{
    value: function()
    {
        if ("_isLittleEndian" in this)
            return this._isLittleEndian;

        var buffer = new ArrayBuffer(4);
        var longData = new Uint32Array(buffer);
        var data = new Uint8Array(buffer);

        longData[0] = 0x0a0b0c0d;

        this._isLittleEndian = data[0] === 0x0d && data[1] === 0x0c && data[2] === 0x0b && data[3] === 0x0a;

        return this._isLittleEndian;
    }
});

function isEmptyObject(object)
{
    for (var property in object)
        return false;
    return true;
}

function isEnterKey(event)
{
    // Check if this is an IME event.
    return event.keyCode !== 229 && event.keyIdentifier === "Enter";
}

function removeURLFragment(url)
{
    var hashIndex = url.indexOf("#");
    if (hashIndex >= 0)
        return url.substring(0, hashIndex);
    return url;
}

function relativePath(path, basePath)
{
    console.assert(path.charAt(0) === "/");
    console.assert(basePath.charAt(0) === "/");
    
    var pathComponents = path.split("/");
    var baseComponents = basePath.replace(/\/$/, "").split("/");
    var finalComponents = [];

    var index = 1;
    for (; index < pathComponents.length && index < baseComponents.length; ++index) {
        if (pathComponents[index] !== baseComponents[index])
            break;
    }

    for (var i = index; i < baseComponents.length; ++i)
        finalComponents.push("..");

    for (var i = index; i < pathComponents.length; ++i)
        finalComponents.push(pathComponents[i]);

    return finalComponents.join("/");
}

function resolveDotsInPath(path)
{
    if (!path)
        return path;

    if (path.indexOf("./") === -1)
        return path;

    console.assert(path.charAt(0) === "/");

    var result = [];

    var components = path.split("/");
    for (var i = 0; i < components.length; ++i) {
        var component = components[i];

        // Skip over "./".
        if (component === ".")
            continue;

        // Rewind one component for "../".
        if (component === "..") {
            if (result.length === 1)
                continue;
            result.pop();
            continue;
        }

        result.push(component);
    }

    return result.join("/");
}

function parseURL(url)
{
    url = url ? url.trim() : "";

    var match = url.match(/^([^:]+):\/\/([^\/:]*)(?::([\d]+))?(?:(\/[^#]*)(?:#(.*))?)?$/i);
    if (!match)
        return {scheme: null, host: null, port: null, path: null, queryString: null, fragment: null, lastPathComponent: null};

    var scheme = match[1].toLowerCase();
    var host = match[2].toLowerCase();
    var port = Number(match[3]) || null;
    var wholePath = match[4] || null;
    var fragment = match[5] || null;
    var path = wholePath;
    var queryString = null;

    // Split the path and the query string.
    if (wholePath) {
        var indexOfQuery = wholePath.indexOf("?");
        if (indexOfQuery !== -1) {
            path = wholePath.substring(0, indexOfQuery);
            queryString = wholePath.substring(indexOfQuery + 1);
        }
        path = resolveDotsInPath(path);
    }

    // Find last path component.
    var lastPathComponent = null;
    if (path && path !== "/") {
        // Skip the trailing slash if there is one.
        var endOffset = path[path.length - 1] === "/" ? 1 : 0;
        var lastSlashIndex = path.lastIndexOf("/", path.length - 1 - endOffset);
        if (lastSlashIndex !== -1)
            lastPathComponent = path.substring(lastSlashIndex + 1, path.length - endOffset);
    }

    return {scheme: scheme, host: host, port: port, path: path, queryString: queryString, fragment: fragment, lastPathComponent: lastPathComponent};
}

function absoluteURL(partialURL, baseURL)
{
    partialURL = partialURL ? partialURL.trim() : "";

    // Return data and javascript URLs as-is.
    if (partialURL.startsWith("data:") || partialURL.startsWith("javascript:") || partialURL.startsWith("mailto:"))
        return partialURL;

    // If the URL has a scheme it is already a full URL, so return it.
    if (parseURL(partialURL).scheme)
        return partialURL;

    // If there is no partial URL, just return the base URL.
    if (!partialURL)
        return baseURL || null;

    var baseURLComponents = parseURL(baseURL);

    // The base URL needs to be an absolute URL. Return null if it isn't.
    if (!baseURLComponents.scheme)
        return null;

    // A URL that starts with "//" is a full URL without the scheme. Use the base URL scheme.
    if (partialURL[0] === "/" && partialURL[1] === "/")
        return baseURLComponents.scheme + ":" + partialURL;

    // The path can be null for URLs that have just a scheme and host (like "http://apple.com"). So make the path be "/".
    if (!baseURLComponents.path)
        baseURLComponents.path = "/";

    // Generate the base URL prefix that is used in the rest of the cases.
    var baseURLPrefix = baseURLComponents.scheme + "://" + baseURLComponents.host + (baseURLComponents.port ? (":" + baseURLComponents.port) : "");

    // A URL that starts with "?" is just a query string that gets applied to the base URL (replacing the base URL query string and fragment).
    if (partialURL[0] === "?")
        return baseURLPrefix + baseURLComponents.path + partialURL;

    // A URL that starts with "/" is an absolute path that gets applied to the base URL (replacing the base URL path, query string and fragment).
    if (partialURL[0] === "/")
        return baseURLPrefix + resolveDotsInPath(partialURL);

    // Generate the base path that is used in the final case by removing everything after the last "/" from the base URL's path.
    var basePath = baseURLComponents.path.substring(0, baseURLComponents.path.lastIndexOf("/")) + "/";
    return baseURLPrefix + resolveDotsInPath(basePath + partialURL);
}

function simpleGlobStringToRegExp(globString, regExpFlags)
{
    // Only supports "*" globs.

    if (!globString)
        return null;

    // Escape everything from String.prototype.escapeForRegExp except "*".
    var regexString = globString.escapeCharacters("^[]{}()\\.$+?|");

    // Unescape all doubly escaped backslashes in front of escaped asterisks.
    // So "\\*" will become "\*" again, undoing escapeCharacters escaping of "\".
    // This makes "\*" match a literal "*" instead of using the "*" for globbing.
    regexString = regexString.replace(/\\\\\*/g, "\\*");

    // The following regex doesn't match an asterisk that has a backslash in front.
    // It also catches consecutive asterisks so they collapse down when replaced.
    var unescapedAsteriskRegex = /(^|[^\\])\*+/g;
    if (unescapedAsteriskRegex.test(globString)) {
        // Replace all unescaped asterisks with ".*".
        regexString = regexString.replace(unescapedAsteriskRegex, "$1.*");

        // Match edge boundaries when there is an asterisk to better meet the expectations
        // of the user. When someone types "*.js" they don't expect "foo.json" to match. They
        // would only expect that if they type "*.js*". We use \b (instead of ^ and $) to allow
        // matches inside paths or URLs, so "ba*.js" will match "foo/bar.js" but not "boo/bbar.js".
        // When there isn't an asterisk the regexString is just a substring search.
        regexString = "\\b" + regexString + "\\b";
    }

    return new RegExp(regexString, regExpFlags);
}

function parseLocationQueryParameters(arrayResult)
{
    // The first character is always the "?".
    return parseQueryString(window.location.search.substring(1), arrayResult);
}

function parseQueryString(queryString, arrayResult)
{
    if (!queryString)
        return arrayResult ? [] : {};

    function decode(string)
    {
        try {
            // Replace "+" with " " then decode precent encoded values.
            return decodeURIComponent(string.replace(/\+/g, " "));
        } catch (e) {
            return string;
        }
    }

    var parameters = arrayResult ? [] : {};
    var parameterStrings = queryString.split("&");
    for (var i = 0; i < parameterStrings.length; ++i) {
        var pair = parameterStrings[i].split("=").map(decode);
        if (arrayResult)
            parameters.push({name: pair[0], value: pair[1]});
        else
            parameters[pair[0]] = pair[1];
    }

    return parameters;
}
