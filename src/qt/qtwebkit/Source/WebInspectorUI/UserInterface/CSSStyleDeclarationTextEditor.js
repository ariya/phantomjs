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

WebInspector.CSSStyleDeclarationTextEditor = function(delegate, style, element)
{
    WebInspector.Object.call(this);

    this._element = element || document.createElement("div");
    this._element.classList.add(WebInspector.CSSStyleDeclarationTextEditor.StyleClassName);
    this._element.classList.add(WebInspector.SyntaxHighlightedStyleClassName);

    this._showsImplicitProperties = true;
    this._alwaysShowPropertyNames = {};
    this._sortProperties = false;

    this._prefixWhitespace = "";
    this._suffixWhitespace = "";
    this._linePrefixWhitespace = "";

    this._delegate = delegate || null;

    this._codeMirror = CodeMirror(this.element, {
        readOnly: true,
        lineWrapping: true,
        mode: "css-rule",
        electricChars: false,
        indentWithTabs: true,
        indentUnit: 4,
        smartIndent: false,
        matchBrackets: true,
        autoCloseBrackets: true
    });

    this._codeMirror.on("change", this._contentChanged.bind(this));
    this._codeMirror.on("blur", this._editorBlured.bind(this));

    this._completionController = new WebInspector.CodeMirrorCompletionController(this._codeMirror, this);
    this._tokenTrackingController = new WebInspector.CodeMirrorTokenTrackingController(this._codeMirror, this);

    this._jumpToSymbolTrackingModeEnabled = false;
    this._tokenTrackingController.classNameForHighlightedRange = WebInspector.CodeMirrorTokenTrackingController.JumpToSymbolHighlightStyleClassName;
    this._tokenTrackingController.mouseOverDelayDuration = 0;
    this._tokenTrackingController.mouseOutReleaseDelayDuration = 0;
    this._tokenTrackingController.mode = WebInspector.CodeMirrorTokenTrackingController.Mode.NonSymbolTokens;

    this.style = style;
};

WebInspector.Object.addConstructorFunctions(WebInspector.CSSStyleDeclarationTextEditor);

WebInspector.CSSStyleDeclarationTextEditor.StyleClassName = "css-style-text-editor";
WebInspector.CSSStyleDeclarationTextEditor.ReadOnlyStyleClassName = "read-only";
WebInspector.CSSStyleDeclarationTextEditor.ColorSwatchElementStyleClassName = "color-swatch";
WebInspector.CSSStyleDeclarationTextEditor.CheckboxPlaceholderElementStyleClassName = "checkbox-placeholder";
WebInspector.CSSStyleDeclarationTextEditor.EditingLineStyleClassName = "editing-line";
WebInspector.CSSStyleDeclarationTextEditor.CommitCoalesceDelay = 250;
WebInspector.CSSStyleDeclarationTextEditor.RemoveEditingLineClassesDelay = 2000;

WebInspector.CSSStyleDeclarationTextEditor.prototype = {
    constructor: WebInspector.CSSStyleDeclarationTextEditor,

    // Public

    get element()
    {
        return this._element;
    },

    get delegate()
    {
        return this._delegate;
    },

    set delegate(delegate)
    {
        this._delegate = delegate || null;
    },

    get style()
    {
        return this._style;
    },

    set style(style)
    {
        if (this._style === style)
            return;

        if (this._style) {
            this._style.removeEventListener(WebInspector.CSSStyleDeclaration.Event.PropertiesChanged, this._propertiesChanged, this);
            if (this._style.ownerRule && this._style.ownerRule.sourceCodeLocation)
                WebInspector.notifications.removeEventListener(WebInspector.Notification.GlobalModifierKeysDidChange, this._updateJumpToSymbolTrackingMode, this);
        }

        this._style = style || null;

        if (this._style) {
            this._style.addEventListener(WebInspector.CSSStyleDeclaration.Event.PropertiesChanged, this._propertiesChanged, this);
            if (this._style.ownerRule && this._style.ownerRule.sourceCodeLocation)
                WebInspector.notifications.addEventListener(WebInspector.Notification.GlobalModifierKeysDidChange, this._updateJumpToSymbolTrackingMode, this);
        }

        this._updateJumpToSymbolTrackingMode();

        this._resetContent();
    },

    get focused()
    {
        return this._codeMirror.getWrapperElement().classList.contains("CodeMirror-focused");
    },

    get alwaysShowPropertyNames()
    {
        return Object.keys(this._alwaysShowPropertyNames);
    },

    set alwaysShowPropertyNames(alwaysShowPropertyNames)
    {
        this._alwaysShowPropertyNames = (alwaysShowPropertyNames || []).keySet();

        this._resetContent();
    },

    get showsImplicitProperties()
    {
        return this._showsImplicitProperties;
    },

    set showsImplicitProperties(showsImplicitProperties)
    {
        if (this._showsImplicitProperties === showsImplicitProperties)
            return;

        this._showsImplicitProperties = showsImplicitProperties;

        this._resetContent();
    },

    get sortProperties()
    {
        return this._sortProperties;
    },

    set sortProperties(sortProperties)
    {
        if (this._sortProperties === sortProperties)
            return;

        this._sortProperties = sortProperties;

        this._resetContent();
    },

    focus: function()
    {
        this._codeMirror.focus();
    },

    refresh: function()
    {
        this._resetContent();
    },

    updateLayout: function(force)
    {
        this._codeMirror.refresh();
    },

    // Protected

    didDismissPopover: function(popover) 
    {
        if (popover === this._colorPickerPopover)
            delete this._colorPickerPopover;
    },

    completionControllerCompletionsHidden: function(completionController)
    {
        var styleText = this._style.text;
        var currentText = this._formattedContent();

        // If the style text and the current editor text differ then we need to commit.
        // Otherwise we can just update the properties that got skipped because a completion
        // was pending the last time _propertiesChanged was called.
        if (styleText !== currentText)
            this._commitChanges();
        else
            this._propertiesChanged();
    },

    // Private

    _clearRemoveEditingLineClassesTimeout: function()
    {
        if (!this._removeEditingLineClassesTimeout)
            return;

        clearTimeout(this._removeEditingLineClassesTimeout);
        delete this._removeEditingLineClassesTimeout;
    },

    _removeEditingLineClasses: function()
    {
        this._clearRemoveEditingLineClassesTimeout();

        function removeEditingLineClasses()
        {
            var lineCount = this._codeMirror.lineCount();
            for (var i = 0; i < lineCount; ++i)
                this._codeMirror.removeLineClass(i, "wrap", WebInspector.CSSStyleDeclarationTextEditor.EditingLineStyleClassName);
        }

        this._codeMirror.operation(removeEditingLineClasses.bind(this));
    },

    _removeEditingLineClassesSoon: function()
    {
        if (this._removeEditingLineClassesTimeout)
            return;
        this._removeEditingLineClassesTimeout = setTimeout(this._removeEditingLineClasses.bind(this), WebInspector.CSSStyleDeclarationTextEditor.RemoveEditingLineClassesDelay);
    },

    _formattedContent: function()
    {
        // Start with the prefix whitespace we stripped.
        var content = this._prefixWhitespace;

        // Get each line and add the line prefix whitespace and newlines.
        var lineCount = this._codeMirror.lineCount();
        for (var i = 0; i < lineCount; ++i) {
            var lineContent = this._codeMirror.getLine(i);
            content += this._linePrefixWhitespace + lineContent;
            if (i !== lineCount - 1)
                content += "\n";
        }

        // Add the suffix whitespace we stripped.
        content += this._suffixWhitespace;

        return content;
    },

    _commitChanges: function()
    {
        if (this._commitChangesTimeout) {
            clearTimeout(this._commitChangesTimeout);
            delete this._commitChangesTimeout;
        }

        this._style.text = this._formattedContent();
    },

    _editorBlured: function(codeMirror)
    {
        // Clicking a suggestion causes the editor to blur. We don't want to reset content in this case.
        if (this._completionController.isHandlingClickEvent())
            return;

        // Reset the content on blur since we stop accepting external changes while the the editor is focused.
        // This causes us to pick up any change that was suppressed while the editor was focused.
        this._resetContent();
    },

    _contentChanged: function(codeMirror, change)
    {
        // Return early if the style isn't editable. This still can be called when readOnly is set because
        // clicking on a color swatch modifies the text.
        if (!this._style || !this._style.editable || this._ignoreCodeMirrorContentDidChangeEvent)
            return;

        this._markLinesWithCheckboxPlaceholder();

        this._clearRemoveEditingLineClassesTimeout();
        this._codeMirror.addLineClass(change.from.line, "wrap", WebInspector.CSSStyleDeclarationTextEditor.EditingLineStyleClassName);

        // When the change is a completion change, create color swatches now since the changes
        // will not go through _propertiesChanged until completionControllerCompletionsHidden happens.
        // This way any auto completed colors get swatches right away.
        if (this._completionController.isCompletionChange(change))
            this._createColorSwatches(false, change.from.line);

        // Use a short delay for user input to coalesce more changes before committing. Other actions like
        // undo, redo and paste are atomic and work better with a zero delay. CodeMirror identifies changes that
        // get coalesced in the undo stack with a "+" prefix on the origin. Use that to set the delay for our coalescing.
        const delay = change.origin && change.origin.charAt(0) === "+" ? WebInspector.CSSStyleDeclarationTextEditor.CommitCoalesceDelay : 0;

        // Reset the timeout so rapid changes coalesce after a short delay.
        if (this._commitChangesTimeout)
            clearTimeout(this._commitChangesTimeout);
        this._commitChangesTimeout = setTimeout(this._commitChanges.bind(this), delay);
    },

    _updateTextMarkers: function(nonatomic)
    {
        function update()
        {
            this._clearTextMarkers(true);

            var styleText = this._style.text;

            this._iterateOverProperties(true, function(property) {
                var styleTextRange = property.styleDeclarationTextRange;
                console.assert(styleTextRange);
                if (!styleTextRange)
                    return;

                var from = {line: styleTextRange.startLine, ch: styleTextRange.startColumn};
                var to = {line: styleTextRange.endLine, ch: styleTextRange.endColumn};

                // Adjust the line position for the missing prefix line.
                if (this._prefixWhitespace) {
                    --from.line;
                    --to.line;
                }

                // Adjust the column for the stripped line prefix whitespace.
                from.ch -= this._linePrefixWhitespace.length;
                to.ch -= this._linePrefixWhitespace.length;

                this._createTextMarkerForPropertyIfNeeded(from, to, property);
            });

            if (!this._codeMirror.getOption("readOnly")) {
                // Matches a comment like: /* -webkit-foo: bar; */
                const commentedPropertyRegex = /\/\*\s*[-\w]+\s*:\s*[^;]+;?\s*\*\//g;

                // Look for comments that look like properties and add checkboxes in front of them.
                var lineCount = this._codeMirror.lineCount();
                for (var i = 0; i < lineCount; ++i) {
                    var lineContent = this._codeMirror.getLine(i);

                    var match = commentedPropertyRegex.exec(lineContent);
                    while (match) {
                        var checkboxElement = document.createElement("input");
                        checkboxElement.type = "checkbox";
                        checkboxElement.checked = false;
                        checkboxElement.addEventListener("change", this._propertyCommentCheckboxChanged.bind(this));

                        var from = {line: i, ch: match.index};
                        var to = {line: i, ch: match.index + match[0].length};

                        var checkboxMarker = this._codeMirror.setBookmark(from, checkboxElement);
                        checkboxMarker.__propertyCheckbox = true;

                        var commentTextMarker = this._codeMirror.markText(from, to);

                        checkboxElement.__commentTextMarker = commentTextMarker;

                        match = commentedPropertyRegex.exec(lineContent);
                    }
                }
            }

            // Look for colors and make swatches.
            this._createColorSwatches(true);

            this._markLinesWithCheckboxPlaceholder();
        }

        if (nonatomic)
            update.call(this);
        else
            this._codeMirror.operation(update.bind(this));
    },

    _createColorSwatches: function(nonatomic, lineNumber)
    {
        function update()
        {
            // Matches rgba(0, 0, 0, 0.5), rgb(0, 0, 0), hsl(), hsla(), #fff, #ffffff, white
            const colorRegex = /((?:rgb|hsl)a?\([^)]+\)|#[0-9a-fA-F]{6}|#[0-9a-fA-F]{3}|\b\w+\b(?![-.]))/g;

            var start = typeof lineNumber === "number" ? lineNumber : 0;
            var end = typeof lineNumber === "number" ? lineNumber + 1 : this._codeMirror.lineCount();

            // Look for color strings and add swatches in front of them.
            for (var i = start; i < end; ++i) {
                var lineContent = this._codeMirror.getLine(i);

                var match = colorRegex.exec(lineContent);
                while (match) {

                    // Act as a negative look-behind and disallow the color from being prefixing with certain characters.
                    if (match.index > 0 && /[-.]/.test(lineContent[match.index - 1])) {
                        match = colorRegex.exec(lineContent);
                        continue;
                    }

                    var from = {line: i, ch: match.index};
                    var to = {line: i, ch: match.index + match[0].length};

                    var foundColorMarker = false;
                    var marks = this._codeMirror.findMarksAt(to);
                    for (var j = 0; j < marks.length; ++j) {
                        if (!marks[j].__markedColor)
                            continue;
                        foundColorMarker = true;
                        break;
                    }

                    if (foundColorMarker) {
                        match = colorRegex.exec(lineContent);
                        continue;
                    }

                    try {
                        var color = new WebInspector.Color(match[0]);
                    } catch (e) {
                        match = colorRegex.exec(lineContent);
                        continue;
                    }

                    var swatchElement = document.createElement("span");
                    swatchElement.title = WebInspector.UIString("Click to open a colorpicker. Shift-click to change color format.");
                    swatchElement.className = WebInspector.CSSStyleDeclarationTextEditor.ColorSwatchElementStyleClassName;
                    swatchElement.addEventListener("click", this._colorSwatchClicked.bind(this));

                    var swatchInnerElement = document.createElement("span");
                    swatchInnerElement.style.setProperty("background-color", match[0]);
                    swatchElement.appendChild(swatchInnerElement);

                    var swatchMarker = this._codeMirror.setBookmark(from, swatchElement);

                    var colorTextMarker = this._codeMirror.markText(from, to);
                    colorTextMarker.__markedColor = true;

                    swatchInnerElement.__colorTextMarker = colorTextMarker;
                    swatchInnerElement.__color = color;

                    match = colorRegex.exec(lineContent);
                }
            }
        }

        if (nonatomic)
            update.call(this);
        else
            this._codeMirror.operation(update.bind(this));
    },

    _updateTextMarkerForPropertyIfNeeded: function(property)
    {
        var textMarker = property.__propertyTextMarker;
        console.assert(textMarker);
        if (!textMarker)
            return;

        var range = textMarker.find();
        console.assert(range);
        if (!range)
            return;

        this._createTextMarkerForPropertyIfNeeded(range.from, range.to, property);
    },

    _createTextMarkerForPropertyIfNeeded: function(from, to, property)
    {
        if (!this._codeMirror.getOption("readOnly")) {
            // Create a new checkbox element and marker.

            console.assert(property.enabled);

            var checkboxElement = document.createElement("input");
            checkboxElement.type = "checkbox";
            checkboxElement.checked = true;
            checkboxElement.addEventListener("change", this._propertyCheckboxChanged.bind(this));
            checkboxElement.__cssProperty = property;

            var checkboxMarker = this._codeMirror.setBookmark(from, checkboxElement);
            checkboxMarker.__propertyCheckbox = true;
        }

        var classNames = ["css-style-declaration-property"];

        if (property.overridden)
            classNames.push("overridden");

        if (property.implicit)
            classNames.push("implicit");

        if (this._style.inherited && !property.inherited)
            classNames.push("not-inherited");

        if (!property.valid && property.hasOtherVendorNameOrKeyword())
            classNames.push("other-vendor");
        else if (!property.valid)
            classNames.push("invalid");

        if (!property.enabled)
            classNames.push("disabled");

        var classNamesString = classNames.join(" ");

        // If there is already a text marker and it's in the same document, then try to avoid recreating it.
        // FIXME: If there are multiple CSSStyleDeclarationTextEditors for the same style then this will cause
        // both editors to fight and always recreate their text markers. This isn't really common.
        if (property.__propertyTextMarker && property.__propertyTextMarker.doc.cm === this._codeMirror && property.__propertyTextMarker.find()) {
            // If the class name is the same then we don't need to make a new marker.
            if (property.__propertyTextMarker.className === classNamesString)
                return;

            property.__propertyTextMarker.clear();
        }

        var propertyTextMarker = this._codeMirror.markText(from, to, {className: classNamesString});

        propertyTextMarker.__cssProperty = property;
        property.__propertyTextMarker = propertyTextMarker;

        property.addEventListener(WebInspector.CSSProperty.Event.OverriddenStatusChanged, this._propertyOverriddenStatusChanged, this);

        this._removeCheckboxPlaceholder(from.line);
    },

    _clearTextMarkers: function(nonatomic, all)
    {
        function clear()
        {
            var markers = this._codeMirror.getAllMarks();
            for (var i = 0; i < markers.length; ++i) {
                var textMarker = markers[i];

                if (!all && textMarker.__checkboxPlaceholder) {
                    var position = textMarker.find();

                    // Only keep checkbox placeholders if they are in the first column.
                    if (position && !position.ch)
                        continue;
                }

                if (textMarker.__cssProperty) {
                    textMarker.__cssProperty.removeEventListener(null, null, this);

                    delete textMarker.__cssProperty.__propertyTextMarker;
                    delete textMarker.__cssProperty;
                }

                textMarker.clear();
            }
        }

        if (nonatomic)
            clear.call(this);
        else
            this._codeMirror.operation(clear.bind(this));
    },

    _iterateOverProperties: function(onlyVisibleProperties, callback)
    {
        var properties = onlyVisibleProperties ? this._style.visibleProperties : this._style.properties;

        if (!onlyVisibleProperties) {
            // Filter based on options only when all properties are used.
            properties = properties.filter((function(property) {
                return !property.implicit || this._showsImplicitProperties || property.canonicalName in this._alwaysShowPropertyNames;
            }).bind(this));

            if (this._sortProperties)
                properties.sort(function(a, b) { return a.name.localeCompare(b.name) });
        }

        for (var i = 0; i < properties.length; ++i) {
            if (callback.call(this, properties[i], i === properties.length - 1))
                break;
        }
    },

    _propertyCheckboxChanged: function(event)
    {
        var property = event.target.__cssProperty;
        console.assert(property);
        if (!property)
            return;

        var textMarker = property.__propertyTextMarker;
        console.assert(textMarker);
        if (!textMarker)
            return;

        // Check if the property has been removed already, like from double-clicking
        // the checkbox and calling this event listener multiple times.
        var range = textMarker.find();
        if (!range)
            return;

        var text = this._codeMirror.getRange(range.from, range.to);

        function update()
        {
            // Replace the text with a commented version.
            this._codeMirror.replaceRange("/* " + text + " */", range.from, range.to);

            // Update the line for any color swatches that got removed.
            this._createColorSwatches(true, range.from.line);
        }

        this._codeMirror.operation(update.bind(this));
    },

    _propertyCommentCheckboxChanged: function(event)
    {
        var commentTextMarker = event.target.__commentTextMarker;
        console.assert(commentTextMarker);
        if (!commentTextMarker)
            return;

        // Check if the comment has been removed already, like from double-clicking
        // the checkbox and calling event listener multiple times.
        var range = commentTextMarker.find();
        if (!range)
            return;

        var text = this._codeMirror.getRange(range.from, range.to);

        // Remove the comment prefix and suffix.
        text = text.replace(/^\/\*\s*/, "").replace(/\s*\*\/$/, "");

        // Add a semicolon if there isn't one already.
        if (text.length && text.charAt(text.length - 1) !== ";")
            text += ";";

        function update()
        {
            this._codeMirror.addLineClass(range.from.line, "wrap", WebInspector.CSSStyleDeclarationTextEditor.EditingLineStyleClassName);
            this._codeMirror.replaceRange(text, range.from, range.to);

            // Update the line for any color swatches that got removed.
            this._createColorSwatches(true, range.from.line);
        }

        this._codeMirror.operation(update.bind(this));
    },

    _colorSwatchClicked: function(event)
    {
        if (this._colorPickerPopover)
            return;

        var swatch = event.target;

        var color = swatch.__color;
        console.assert(color);
        if (!color)
            return;

        var colorTextMarker = swatch.__colorTextMarker;
        console.assert(colorTextMarker);
        if (!colorTextMarker)
            return;

        var range = colorTextMarker.find();
        console.assert(range);
        if (!range)
            return;

        function updateCodeMirror(newColorText)
        {
            function update()
            {
                // The original text marker might have been cleared by a style update,
                // in this case we need to find the new color text marker so we know
                // the right range for the new style color text.
                if (!colorTextMarker || !colorTextMarker.find()) {
                    colorTextMarker = null;

                    var marks = this._codeMirror.findMarksAt(range.from);
                    if (!marks.length)
                        return;

                    for (var i = 0; i < marks.length; ++i) {
                        var mark = marks[i];
                        if (!mark.__markedColor)
                            continue;
                        colorTextMarker = mark;
                        break;
                    }
                }

                if (!colorTextMarker)
                    return;

                // Sometimes we still might find a stale text marker with findMarksAt.
                var newRange = colorTextMarker.find();
                if (!newRange)
                    return;

                range = newRange;

                colorTextMarker.clear();

                this._codeMirror.replaceRange(newColorText, range.from, range.to);

                // The color's text format could have changed, so we need to update the "range" 
                // variable to anticipate a different "range.to" property.
                range.to.ch = range.from.ch + newColorText.length;

                colorTextMarker = this._codeMirror.markText(range.from, range.to);
                colorTextMarker.__markedColor = true;

                swatch.__colorTextMarker = colorTextMarker;
            }

            this._codeMirror.operation(update.bind(this));
        }

        if (event.shiftKey) {
            var nextFormat = color.nextFormat();
            console.assert(nextFormat);
            if (!nextFormat)
                return;
            color.format = nextFormat;

            var newColorText = color.toString();

            // Ignore the change so we don't commit the format change. However, any future user
            // edits will commit the color format.
            this._ignoreCodeMirrorContentDidChangeEvent = true;
            updateCodeMirror.call(this, newColorText);
            delete this._ignoreCodeMirrorContentDidChangeEvent;
        } else {
            this._colorPickerPopover = new WebInspector.Popover(this);

            var colorPicker = new WebInspector.CSSColorPicker;
            colorPicker.color = color;

            colorPicker.addEventListener(WebInspector.CSSColorPicker.Event.ColorChanged, function(event) {
                updateCodeMirror.call(this, event.data.color.toString());
            }.bind(this));

            var bounds = WebInspector.Rect.rectFromClientRect(swatch.getBoundingClientRect());
            
            this._colorPickerPopover.content = colorPicker.element;
            this._colorPickerPopover.present(bounds, [WebInspector.RectEdge.MIN_X]);

            colorPicker.shown();
        }
    },

    _propertyOverriddenStatusChanged: function(event)
    {
        this._updateTextMarkerForPropertyIfNeeded(event.target);
    },

    _propertiesChanged: function(event)
    {
        // Don't try to update the document while completions are showing. Doing so will clear
        // the completion hint and prevent further interaction with the completion.
        if (this._completionController.isShowingCompletions())
            return;

        // Reset the content if the text is different and we are not focused.
        if (!this.focused && this._style.text !== this._formattedContent()) {
            this._resetContent();
            return;
        }

        this._removeEditingLineClassesSoon();

        this._updateTextMarkers();
    },

    _markLinesWithCheckboxPlaceholder: function()
    {
        if (this._codeMirror.getOption("readOnly"))
            return;

        var linesWithPropertyCheckboxes = {};
        var linesWithCheckboxPlaceholders = {};

        var markers = this._codeMirror.getAllMarks();
        for (var i = 0; i < markers.length; ++i) {
            var textMarker = markers[i];
            if (textMarker.__propertyCheckbox) {
                var position = textMarker.find();
                if (position)
                    linesWithPropertyCheckboxes[position.line] = true;
            } else if (textMarker.__checkboxPlaceholder) {
                var position = textMarker.find();
                if (position)
                    linesWithCheckboxPlaceholders[position.line] = true;
            }
        }

        var lineCount = this._codeMirror.lineCount();

        for (var i = 0; i < lineCount; ++i) {
            if (i in linesWithPropertyCheckboxes || i in linesWithCheckboxPlaceholders)
                continue;

            var position = {line: i, ch: 0};

            var placeholderElement = document.createElement("div");
            placeholderElement.className = WebInspector.CSSStyleDeclarationTextEditor.CheckboxPlaceholderElementStyleClassName;

            var placeholderMark = this._codeMirror.setBookmark(position, placeholderElement);
            placeholderMark.__checkboxPlaceholder = true;
        }
    },

    _removeCheckboxPlaceholder: function(lineNumber)
    {
        var marks = this._codeMirror.findMarksAt({line: lineNumber, ch: 0});
        for (var i = 0; i < marks.length; ++i) {
            var mark = marks[i];
            if (!mark.__checkboxPlaceholder)
                continue;

            mark.clear();
            return;
        }
    },

    _resetContent: function()
    {
        if (this._commitChangesTimeout) {
            clearTimeout(this._commitChangesTimeout);
            delete this._commitChangesTimeout;
        }

        this._removeEditingLineClasses();

        // Only allow editing if we have a style, it is editable and we have text range in the stylesheet.
        var readOnly = !this._style || !this._style.editable || !this._style.styleSheetTextRange;
        this._codeMirror.setOption("readOnly", readOnly);

        if (readOnly) {
            this.element.classList.add(WebInspector.CSSStyleDeclarationTextEditor.ReadOnlyStyleClassName);
            this._codeMirror.setOption("placeholder", WebInspector.UIString("No Properties"));
        } else {
            this.element.classList.remove(WebInspector.CSSStyleDeclarationTextEditor.ReadOnlyStyleClassName);
            this._codeMirror.setOption("placeholder", WebInspector.UIString("No Properties \u2014 Click to Edit"));
        }

        if (!this._style) {
            this._ignoreCodeMirrorContentDidChangeEvent = true;

            this._clearTextMarkers(false, true);

            this._codeMirror.setValue("");
            this._codeMirror.clearHistory();
            this._codeMirror.markClean();

            delete this._ignoreCodeMirrorContentDidChangeEvent;

            return;
        }

        function update()
        {
            // Remember the cursor position/selection.
            var selectionAnchor = this._codeMirror.getCursor("anchor");
            var selectionHead = this._codeMirror.getCursor("head");

            function countNewLineCharacters(text)
            {
                var matches = text.match(/\n/g);
                return matches ? matches.length : 0;
            }

            var styleText = this._style.text;

            // Pretty print the content if there are more properties than there are lines.
            // This could be an option exposed to the user; however, it is almost always
            // desired in this case.

            if (styleText && this._style.visibleProperties.length <= countNewLineCharacters(styleText.trim()) + 1) {
                // This style has formatted text content, so use it for a high-fidelity experience.

                var prefixWhitespaceMatch = styleText.match(/^[ \t]*\n/);
                this._prefixWhitespace = prefixWhitespaceMatch ? prefixWhitespaceMatch[0] : "";

                var suffixWhitespaceMatch = styleText.match(/\n[ \t]*$/);
                this._suffixWhitespace = suffixWhitespaceMatch ? suffixWhitespaceMatch[0] : "";

                this._codeMirror.setValue(styleText);

                if (this._prefixWhitespace)
                    this._codeMirror.removeLine(0);

                if (this._suffixWhitespace) {
                    var lineCount = this._codeMirror.lineCount();
                    this._codeMirror.replaceRange("", {line: lineCount - 2}, {line: lineCount - 1});
                }

                this._linePrefixWhitespace = "";

                var linesToStrip = [];

                // Remember the whitespace so it can be restored on commit.
                var lineCount = this._codeMirror.lineCount();
                for (var i = 0; i < lineCount; ++i) {
                    var lineContent = this._codeMirror.getLine(i);

                    var prefixWhitespaceMatch = lineContent.match(/^\s+/);
                    if (!prefixWhitespaceMatch)
                        continue;

                    linesToStrip.push(i);

                    // Only remember the shortest whitespace so we don't loose any of the
                    // original author's whitespace if their indentation lengths differed.
                    // Using the shortest also makes the adjustment work in _updateTextMarkers.

                    // FIXME: This messes up if there is a mix of spaces and tabs. One tab
                    // will be shorter than 4 or 8 spaces, but will look the same visually.
                    if (!this._linePrefixWhitespace || prefixWhitespaceMatch[0].length < this._linePrefixWhitespace.length)
                        this._linePrefixWhitespace = prefixWhitespaceMatch[0];
                }

                // Strip the whitespace from the beginning of each line.
                for (var i = 0; i < linesToStrip.length; ++i) {
                    var lineNumber = linesToStrip[i];
                    var from = {line: lineNumber, ch: 0};
                    var to = {line: lineNumber, ch: this._linePrefixWhitespace.length};
                    this._codeMirror.replaceRange("", from, to);
                }

                // Update all the text markers.
                this._updateTextMarkers(true);
            } else {
                // This style does not have text content or it is minified, so we want to synthesize the text content.

                this._prefixWhitespace = "";
                this._suffixWhitespace = "";
                this._linePrefixWhitespace = "";

                this._codeMirror.setValue("");

                var lineNumber = 0;

                // Iterate only visible properties if we have original style text. That way we known we only syntesize
                // what was originaly in the style text.
                this._iterateOverProperties(styleText ? true : false, function(property) {
                    // Some property text can have line breaks, so consider that in the ranges below.
                    var propertyText = property.synthesizedText;
                    var propertyLineCount = countNewLineCharacters(propertyText);

                    var from = {line: lineNumber, ch: 0};
                    var to = {line: lineNumber + propertyLineCount};

                    this._codeMirror.replaceRange((lineNumber ? "\n" : "") + propertyText, from);
                    this._createTextMarkerForPropertyIfNeeded(from, to, property);

                    lineNumber += propertyLineCount + 1;
                });

                // Look for colors and make swatches.
                this._createColorSwatches(true);
            }

            this._markLinesWithCheckboxPlaceholder();

            // Restore the cursor position/selection.
            this._codeMirror.setSelection(selectionAnchor, selectionHead);

            // Reset undo history since undo past the reset is wrong when the content was empty before
            // or the content was representing a previous style object.
            this._codeMirror.clearHistory();

            // Mark the editor as clean (unedited state).
            this._codeMirror.markClean();
        }

        // This needs to be done first and as a separate operation to avoid an exception in CodeMirror.
        this._clearTextMarkers(false, true);

        this._ignoreCodeMirrorContentDidChangeEvent = true;
        this._codeMirror.operation(update.bind(this));
        delete this._ignoreCodeMirrorContentDidChangeEvent;
    },

    _updateJumpToSymbolTrackingMode: function()
    {
        var oldJumpToSymbolTrackingModeEnabled = this._jumpToSymbolTrackingModeEnabled;

        if (!this._style || !this._style.ownerRule || !this._style.ownerRule.sourceCodeLocation)
            this._jumpToSymbolTrackingModeEnabled = false;
        else
            this._jumpToSymbolTrackingModeEnabled = WebInspector.modifierKeys.metaKey && !WebInspector.modifierKeys.altKey && !WebInspector.modifierKeys.shiftKey;

        if (oldJumpToSymbolTrackingModeEnabled !== this._jumpToSymbolTrackingModeEnabled) {
            if (this._jumpToSymbolTrackingModeEnabled)
                this._tokenTrackingController.startTracking();
            else {
                this._tokenTrackingController.stopTracking();
                this._tokenTrackingController.removeHighlightedRange();
            }
        }
    },

    tokenTrackingControllerHighlightedRangeWasClicked: function(tokenTrackingController)
    {
        console.assert(this._style.ownerRule.sourceCodeLocation);
        if (!this._style.ownerRule.sourceCodeLocation)
            return;

        // Special case command clicking url(...) links.
        var token = this._tokenTrackingController.candidate.hoveredToken;
        if (/\blink\b/.test(token.type)) {
            var url = token.string;
            var baseURL = this._style.ownerRule.sourceCodeLocation.sourceCode.url;
            WebInspector.openURL(absoluteURL(url, baseURL));
            return;
        }

        // Jump to the rule if we can't find a property.
        // Find a better source code location from the property that was clicked.
        var sourceCodeLocation = this._style.ownerRule.sourceCodeLocation;
        var marks = this._codeMirror.findMarksAt(this._tokenTrackingController.candidate.hoveredTokenRange.start);
        for (var i = 0; i < marks.length; ++i) {
            var mark = marks[i];
            var property = mark.__cssProperty;
            if (property) {
                var sourceCode = sourceCodeLocation.sourceCode;
                var styleSheetTextRange = property.styleSheetTextRange;
                sourceCodeLocation = sourceCode.createSourceCodeLocation(styleSheetTextRange.startLine, styleSheetTextRange.startColumn);
            }
        }

        WebInspector.resourceSidebarPanel.showSourceCodeLocation(sourceCodeLocation);
    },

    tokenTrackingControllerNewHighlightCandidate: function(tokenTrackingController, candidate)
    {
        this._tokenTrackingController.highlightRange(candidate.hoveredTokenRange);
    }
};

WebInspector.CSSStyleDeclarationTextEditor.prototype.__proto__ = WebInspector.Object.prototype;
