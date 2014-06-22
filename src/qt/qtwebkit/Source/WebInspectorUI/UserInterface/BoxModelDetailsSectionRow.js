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

WebInspector.BoxModelDetailsSectionRow = function() {
    WebInspector.DetailsSectionRow.call(this, WebInspector.UIString("No Box Model Information"));

    this.element.classList.add(WebInspector.BoxModelDetailsSectionRow.StyleClassName);

    this._nodeStyles = null;
};

WebInspector.BoxModelDetailsSectionRow.StyleClassName = "box-model";
WebInspector.BoxModelDetailsSectionRow.StyleValueDelimiters = " \xA0\t\n\"':;,/()";
WebInspector.BoxModelDetailsSectionRow.CSSNumberRegex = /^(-?(?:\d+(?:\.\d+)?|\.\d+))$/;

WebInspector.BoxModelDetailsSectionRow.prototype = {
    constructor: WebInspector.BoxModelDetailsSectionRow,

    // Public

    get nodeStyles()
    {
        return this._nodeStyles;
    },

    set nodeStyles(nodeStyles)
    {
        this._nodeStyles = nodeStyles;

        this._refresh();
    },

    // Private

    _refresh: function()
    {
        if (this._ignoreNextRefresh) {
            delete this._ignoreNextRefresh;
            return;
        }

        this._updateMetrics();
    },

    _getPropertyValueAsPx: function(style, propertyName)
    {
        return Number(style.propertyForName(propertyName).value.replace(/px$/, "") || 0);
    },

    _getBox: function(computedStyle, componentName)
    {
        var suffix = componentName === "border" ? "-width" : "";
        var left = this._getPropertyValueAsPx(computedStyle, componentName + "-left" + suffix);
        var top = this._getPropertyValueAsPx(computedStyle, componentName + "-top" + suffix);
        var right = this._getPropertyValueAsPx(computedStyle, componentName + "-right" + suffix);
        var bottom = this._getPropertyValueAsPx(computedStyle, componentName + "-bottom" + suffix);
        return { left: left, top: top, right: right, bottom: bottom };
    },

    _highlightDOMNode: function(showHighlight, mode, event)
    {
        event.stopPropagation();

        var nodeId = showHighlight ? this.nodeStyles.node.id : 0;
        if (nodeId) {
            if (this._highlightMode === mode)
                return;
            this._highlightMode = mode;
            WebInspector.domTreeManager.highlightDOMNode(nodeId, mode);
        } else {
            delete this._highlightMode;
            WebInspector.domTreeManager.hideDOMNodeHighlight();
        }

        for (var i = 0; this._boxElements && i < this._boxElements.length; ++i) {
            var element = this._boxElements[i];
            if (nodeId && (mode === "all" || element._name === mode))
                element.classList.add("active");
            else
                element.classList.remove("active");
        }
    },

    _updateMetrics: function()
    {
        // Updating with computed style.
        var metricsElement = document.createElement("div");

        var self = this;
        var style = this._nodeStyles.computedStyle;

        function createElement(type, value, name, propertyName, style)
        {
            // Check if the value is a float and whether it should be rounded.
            var floatValue = parseFloat(value);
            var shouldRoundValue = (!isNaN(floatValue) && floatValue % 1 !== 0);

            var element = document.createElement(type);
            element.textContent = shouldRoundValue ? ("~" + Math.round(floatValue * 100) / 100) : value;
            if (shouldRoundValue)
                element.title = value;
            element.addEventListener("dblclick", this._startEditing.bind(this, element, name, propertyName, style), false);
            return element;
        }

        function createBoxPartElement(style, name, side, suffix)
        {
            var propertyName = (name !== "position" ? name + "-" : "") + side + suffix;
            var value = style.propertyForName(propertyName).value;
            if (value === "" || (name !== "position" && value === "0px"))
                value = "\u2012";
            else if (name === "position" && value === "auto")
                value = "\u2012";
            value = value.replace(/px$/, "");

            var element = createElement.call(this, "div", value, name, propertyName, style);
            element.className = side;
            return element;
        }

        function createContentAreaWidthElement(style)
        {
            var width = style.propertyForName("width").value.replace(/px$/, "");
            if (style.propertyForName("box-sizing").value === "border-box") {
                var borderBox = self._getBox(style, "border");
                var paddingBox = self._getBox(style, "padding");

                width = width - borderBox.left - borderBox.right - paddingBox.left - paddingBox.right;
            }

            return createElement.call(this, "span", width, "width", "width", style);
        }

        function createContentAreaHeightElement(style)
        {
            var height = style.propertyForName("height").value.replace(/px$/, "");
            if (style.propertyForName("box-sizing").value === "border-box") {
                var borderBox = self._getBox(style, "border");
                var paddingBox = self._getBox(style, "padding");

                height = height - borderBox.top - borderBox.bottom - paddingBox.top - paddingBox.bottom;
            }

            return createElement.call(this, "span", height, "height", "height", style);
        }

        // Display types for which margin is ignored.
        var noMarginDisplayType = {
            "table-cell": true,
            "table-column": true,
            "table-column-group": true,
            "table-footer-group": true,
            "table-header-group": true,
            "table-row": true,
            "table-row-group": true
        };

        // Display types for which padding is ignored.
        var noPaddingDisplayType = {
            "table-column": true,
            "table-column-group": true,
            "table-footer-group": true,
            "table-header-group": true,
            "table-row": true,
            "table-row-group": true
        };

        // Position types for which top, left, bottom and right are ignored.
        var noPositionType = {
            "static": true
        };

        this._boxElements = [];
        var boxes = ["content", "padding", "border", "margin", "position"];

        if (!style.properties.length) {
            this.showEmptyMessage();
            return;
        }

        var previousBox = null;
        for (var i = 0; i < boxes.length; ++i) {
            var name = boxes[i];

            if (name === "margin" && noMarginDisplayType[style.propertyForName("display").value])
                continue;
            if (name === "padding" && noPaddingDisplayType[style.propertyForName("display").value])
                continue;
            if (name === "position" && noPositionType[style.propertyForName("position").value])
                continue;

            var boxElement = document.createElement("div");
            boxElement.className = name;
            boxElement._name = name;
            boxElement.addEventListener("mouseover", this._highlightDOMNode.bind(this, true, name === "position" ? "all" : name), false);
            this._boxElements.push(boxElement);

            if (name === "content") {
                var widthElement = createContentAreaWidthElement.call(this, style);
                var heightElement = createContentAreaHeightElement.call(this, style);

                boxElement.appendChild(widthElement);
                boxElement.appendChild(document.createTextNode(" \u00D7 "));
                boxElement.appendChild(heightElement);
            } else {
                var suffix = (name === "border" ? "-width" : "");

                var labelElement = document.createElement("div");
                labelElement.className = "label";
                labelElement.textContent = boxes[i];
                boxElement.appendChild(labelElement);

                boxElement.appendChild(createBoxPartElement.call(this, style, name, "top", suffix));
                boxElement.appendChild(document.createElement("br"));
                boxElement.appendChild(createBoxPartElement.call(this, style, name, "left", suffix));

                if (previousBox)
                    boxElement.appendChild(previousBox);

                boxElement.appendChild(createBoxPartElement.call(this, style, name, "right", suffix));
                boxElement.appendChild(document.createElement("br"));
                boxElement.appendChild(createBoxPartElement.call(this, style, name, "bottom", suffix));
            }

            previousBox = boxElement;
        }

        metricsElement.appendChild(previousBox);
        metricsElement.addEventListener("mouseover", this._highlightDOMNode.bind(this, false, ""), false);

        this.hideEmptyMessage();
        this.element.appendChild(metricsElement);
    },

    _startEditing: function(targetElement, box, styleProperty, computedStyle)
    {
        if (WebInspector.isBeingEdited(targetElement))
            return;

        // If the target element has a title use it as the editing value
        // since the current text is likely truncated/rounded.
        if (targetElement.title)
            targetElement.textContent = targetElement.title;

        var context = {box: box, styleProperty: styleProperty};
        var boundKeyDown = this._handleKeyDown.bind(this, context, styleProperty);
        context.keyDownHandler = boundKeyDown;
        targetElement.addEventListener("keydown", boundKeyDown, false);

        this._isEditingMetrics = true;

        var config = new WebInspector.EditingConfig(this._editingCommitted.bind(this), this._editingCancelled.bind(this), context);
        WebInspector.startEditing(targetElement, config);

        window.getSelection().setBaseAndExtent(targetElement, 0, targetElement, 1);
    },

    _alteredFloatNumber: function(number, event)
    {
        var arrowKeyPressed = (event.keyIdentifier === "Up" || event.keyIdentifier === "Down");

        // Jump by 10 when shift is down or jump by 0.1 when Alt/Option is down.
        // Also jump by 10 for page up and down, or by 100 if shift is held with a page key.
        var changeAmount = 1;
        if (event.shiftKey && !arrowKeyPressed)
            changeAmount = 100;
        else if (event.shiftKey || !arrowKeyPressed)
            changeAmount = 10;
        else if (event.altKey)
            changeAmount = 0.1;

        if (event.keyIdentifier === "Down" || event.keyIdentifier === "PageDown")
            changeAmount *= -1;

        // Make the new number and constrain it to a precision of 6, this matches numbers the engine returns.
        // Use the Number constructor to forget the fixed precision, so 1.100000 will print as 1.1.
        var result = Number((number + changeAmount).toFixed(6));
        if (!String(result).match(WebInspector.BoxModelDetailsSectionRow.CSSNumberRegex))
            return null;

        return result;
    },

    _handleKeyDown: function(context, styleProperty, event)
    {
        if (!/^(?:Page)?(?:Up|Down)$/.test(event.keyIdentifier))
            return;

        var element = event.currentTarget;

        var selection = window.getSelection();
        if (!selection.rangeCount)
            return;

        var selectionRange = selection.getRangeAt(0);
        if (!selectionRange.commonAncestorContainer.isSelfOrDescendant(element))
            return;

        var originalValue = element.textContent;
        var wordRange = selectionRange.startContainer.rangeOfWord(selectionRange.startOffset, WebInspector.BoxModelDetailsSectionRow.StyleValueDelimiters, element);
        var wordString = wordRange.toString();

        var matches = /(.*?)(-?(?:\d+(?:\.\d+)?|\.\d+))(.*)/.exec(wordString);
        var replacementString;
        if (matches && matches.length) {
            var prefix = matches[1];
            var suffix = matches[3];
            var number = this._alteredFloatNumber(parseFloat(matches[2]), event);
            if (number === null) {
                // Need to check for null explicitly.
                return;
            }

            if (styleProperty !== "margin" && number < 0)
                number = 0;

            replacementString = prefix + number + suffix;
        }

        if (!replacementString)
            return;

        var replacementTextNode = document.createTextNode(replacementString);

        wordRange.deleteContents();
        wordRange.insertNode(replacementTextNode);

        var finalSelectionRange = document.createRange();
        finalSelectionRange.setStart(replacementTextNode, 0);
        finalSelectionRange.setEnd(replacementTextNode, replacementString.length);

        selection.removeAllRanges();
        selection.addRange(finalSelectionRange);

        event.handled = true;
        event.preventDefault();

        this._ignoreNextRefresh = true;

        this._applyUserInput(element, replacementString, originalValue, context, false);
    },

    _editingEnded: function(element, context)
    {
        delete this.originalPropertyData;
        delete this.previousPropertyDataCandidate;
        element.removeEventListener("keydown", context.keyDownHandler, false);
        delete this._isEditingMetrics;
    },

    _editingCancelled: function(element, context)
    {
        this._editingEnded(element, context);
        this._refresh();
    },

    _applyUserInput: function(element, userInput, previousContent, context, commitEditor)
    {
        if (commitEditor && userInput === previousContent)
            return this._editingCancelled(element, context); // nothing changed, so cancel

        if (context.box !== "position" && (!userInput || userInput === "\u2012"))
            userInput = "0px";
        else if (context.box === "position" && (!userInput || userInput === "\u2012"))
            userInput = "auto";

        userInput = userInput.toLowerCase();
        // Append a "px" unit if the user input was just a number.
        if (/^-?(?:\d+(?:\.\d+)?|\.\d+)$/.test(userInput))
            userInput += "px";

        var styleProperty = context.styleProperty;
        var computedStyle = this._nodeStyles.computedStyle;

        if (computedStyle.propertyForName("box-sizing").value === "border-box" && (styleProperty === "width" || styleProperty === "height")) {
            if (!userInput.match(/px$/)) {
                console.error("For elements with box-sizing: border-box, only absolute content area dimensions can be applied");
                return;
            }

            var borderBox = this._getBox(computedStyle, "border");
            var paddingBox = this._getBox(computedStyle, "padding");
            var userValuePx = Number(userInput.replace(/px$/, ""));
            if (isNaN(userValuePx))
                return;
            if (styleProperty === "width")
                userValuePx += borderBox.left + borderBox.right + paddingBox.left + paddingBox.right;
            else
                userValuePx += borderBox.top + borderBox.bottom + paddingBox.top + paddingBox.bottom;

            userInput = userValuePx + "px";
        }

        var property = this._nodeStyles.inlineStyle.propertyForName(context.styleProperty);
        property.value = userInput;
        property.add();
    },

    _editingCommitted: function(element, userInput, previousContent, context)
    {
        this._editingEnded(element, context);
        this._applyUserInput(element, userInput, previousContent, context, true);
    }
};

WebInspector.BoxModelDetailsSectionRow.prototype.__proto__ = WebInspector.DetailsSectionRow.prototype;
