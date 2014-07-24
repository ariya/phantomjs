/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
 * Copyright (C) 2006, 2007, 2008 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Matt Lilek (pewtermoose@gmail.com).
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

/**
 * @param {Element} element
 * @param {?function(Event): boolean} elementDragStart
 * @param {function(Event)} elementDrag
 * @param {?function(Event)} elementDragEnd
 * @param {string} cursor
 */
WebInspector.installDragHandle = function(element, elementDragStart, elementDrag, elementDragEnd, cursor)
{
    element.addEventListener("mousedown", WebInspector._elementDragStart.bind(WebInspector, elementDragStart, elementDrag, elementDragEnd, cursor), false);
}

/**
 * @param {?function(Event)} elementDragStart
 * @param {function(Event)} elementDrag
 * @param {?function(Event)} elementDragEnd
 * @param {string} cursor
 * @param {Event} event
 */
WebInspector._elementDragStart = function(elementDragStart, elementDrag, elementDragEnd, cursor, event)
{
    // Only drag upon left button. Right will likely cause a context menu. So will ctrl-click on mac.
    if (event.button || (WebInspector.isMac() && event.ctrlKey))
        return;

    if (WebInspector._elementDraggingEventListener)
        return;

    if (elementDragStart && !elementDragStart(event))
        return;

    if (WebInspector._elementDraggingGlassPane) {
        WebInspector._elementDraggingGlassPane.dispose();
        delete WebInspector._elementDraggingGlassPane;
    }

    var targetDocument = event.target.ownerDocument;

    WebInspector._elementDraggingEventListener = elementDrag;
    WebInspector._elementEndDraggingEventListener = elementDragEnd;
    WebInspector._mouseOutWhileDraggingTargetDocument = targetDocument;

    targetDocument.addEventListener("mousemove", WebInspector._elementDragMove, true);
    targetDocument.addEventListener("mouseup", WebInspector._elementDragEnd, true);
    targetDocument.addEventListener("mouseout", WebInspector._mouseOutWhileDragging, true);

    targetDocument.body.style.cursor = cursor;

    event.preventDefault();
}

WebInspector._mouseOutWhileDragging = function()
{
    WebInspector._unregisterMouseOutWhileDragging();
    WebInspector._elementDraggingGlassPane = new WebInspector.GlassPane();
}

WebInspector._unregisterMouseOutWhileDragging = function()
{
    if (!WebInspector._mouseOutWhileDraggingTargetDocument)
        return;
    WebInspector._mouseOutWhileDraggingTargetDocument.removeEventListener("mouseout", WebInspector._mouseOutWhileDragging, true);
    delete WebInspector._mouseOutWhileDraggingTargetDocument;
}

WebInspector._elementDragMove = function(event)
{
    if (WebInspector._elementDraggingEventListener(event))
        WebInspector._cancelDragEvents(event);
}

WebInspector._cancelDragEvents = function(event)
{
    var targetDocument = event.target.ownerDocument;
    targetDocument.removeEventListener("mousemove", WebInspector._elementDragMove, true);
    targetDocument.removeEventListener("mouseup", WebInspector._elementDragEnd, true);
    WebInspector._unregisterMouseOutWhileDragging();

    targetDocument.body.style.removeProperty("cursor");

    if (WebInspector._elementDraggingGlassPane)
        WebInspector._elementDraggingGlassPane.dispose();

    delete WebInspector._elementDraggingGlassPane;
    delete WebInspector._elementDraggingEventListener;
    delete WebInspector._elementEndDraggingEventListener;
}

WebInspector._elementDragEnd = function(event)
{
    var elementDragEnd = WebInspector._elementEndDraggingEventListener;

    WebInspector._cancelDragEvents(event);

    event.preventDefault();
    if (elementDragEnd)
        elementDragEnd(event);
}

/**
 * @constructor
 */
WebInspector.GlassPane = function()
{
    this.element = document.createElement("div");
    this.element.style.cssText = "position:absolute;top:0;bottom:0;left:0;right:0;background-color:transparent;z-index:1000;";
    this.element.id = "glass-pane-for-drag";
    document.body.appendChild(this.element);
    WebInspector._glassPane = this;
}

WebInspector.GlassPane.prototype = {
    dispose: function()
    {
        delete WebInspector._glassPane;
        WebInspector.inspectorView.focus();
        if (this.element.parentElement)
            this.element.parentElement.removeChild(this.element);
    }
}

WebInspector.isBeingEdited = function(element)
{
    if (element.hasStyleClass("text-prompt") || element.nodeName === "INPUT")
        return true;

    if (!WebInspector.__editingCount)
        return false;

    while (element) {
        if (element.__editing)
            return true;
        element = element.parentElement;
    }
    return false;
}

WebInspector.markBeingEdited = function(element, value)
{
    if (value) {
        if (element.__editing)
            return false;
        element.__editing = true;
        WebInspector.__editingCount = (WebInspector.__editingCount || 0) + 1;
    } else {
        if (!element.__editing)
            return false;
        delete element.__editing;
        --WebInspector.__editingCount;
    }
    return true;
}

/**
 * @constructor
 * @param {function(Element,string,string,*,string)} commitHandler
 * @param {function(Element,*)} cancelHandler
 * @param {*=} context
 */
WebInspector.EditingConfig = function(commitHandler, cancelHandler, context)
{
    this.commitHandler = commitHandler;
    this.cancelHandler = cancelHandler
    this.context = context;

    /**
     * Handles the "paste" event, return values are the same as those for customFinishHandler
     * @type {function(Element)|undefined}
     */
    this.pasteHandler;

    /** 
     * Whether the edited element is multiline
     * @type {boolean|undefined}
     */
    this.multiline;

    /**
     * Custom finish handler for the editing session (invoked on keydown)
     * @type {function(Element,*)|undefined}
     */
    this.customFinishHandler;
}

WebInspector.EditingConfig.prototype = {
    setPasteHandler: function(pasteHandler)
    {
        this.pasteHandler = pasteHandler;
    },

    /**
     * @param {string} initialValue
     * @param {Object} mode
     * @param {string} theme
     * @param {boolean=} lineWrapping
     * @param {boolean=} smartIndent
     */
    setMultilineOptions: function(initialValue, mode, theme, lineWrapping, smartIndent)
    {
        this.multiline = true;
        this.initialValue = initialValue;
        this.mode = mode;
        this.theme = theme;
        this.lineWrapping = lineWrapping;
        this.smartIndent = smartIndent;
    },

    setCustomFinishHandler: function(customFinishHandler)
    {
        this.customFinishHandler = customFinishHandler;
    }
}

WebInspector.CSSNumberRegex = /^(-?(?:\d+(?:\.\d+)?|\.\d+))$/;

WebInspector.StyleValueDelimiters = " \xA0\t\n\"':;,/()";


/**
  * @param {Event} event
  * @return {?string}
  */
WebInspector._valueModificationDirection = function(event)
{
    var direction = null;
    if (event.type === "mousewheel") {
        if (event.wheelDeltaY > 0)
            direction = "Up";
        else if (event.wheelDeltaY < 0)
            direction = "Down";
    } else {
        if (event.keyIdentifier === "Up" || event.keyIdentifier === "PageUp")
            direction = "Up";
        else if (event.keyIdentifier === "Down" || event.keyIdentifier === "PageDown")
            direction = "Down";        
    }
    return direction;
}

/**
 * @param {string} hexString
 * @param {Event} event
 */
WebInspector._modifiedHexValue = function(hexString, event)
{
    var direction = WebInspector._valueModificationDirection(event);
    if (!direction)
        return hexString;

    var number = parseInt(hexString, 16);
    if (isNaN(number) || !isFinite(number))
        return hexString;

    var maxValue = Math.pow(16, hexString.length) - 1;
    var arrowKeyOrMouseWheelEvent = (event.keyIdentifier === "Up" || event.keyIdentifier === "Down" || event.type === "mousewheel");
    var delta;

    if (arrowKeyOrMouseWheelEvent)
        delta = (direction === "Up") ? 1 : -1;
    else
        delta = (event.keyIdentifier === "PageUp") ? 16 : -16;

    if (event.shiftKey)
        delta *= 16;

    var result = number + delta;
    if (result < 0)
        result = 0; // Color hex values are never negative, so clamp to 0.
    else if (result > maxValue)
        return hexString;

    // Ensure the result length is the same as the original hex value.
    var resultString = result.toString(16).toUpperCase();
    for (var i = 0, lengthDelta = hexString.length - resultString.length; i < lengthDelta; ++i)
        resultString = "0" + resultString;
    return resultString;
}

/**
 * @param {number} number
 * @param {Event} event
 */
WebInspector._modifiedFloatNumber = function(number, event)
{
    var direction = WebInspector._valueModificationDirection(event);
    if (!direction)
        return number;
    
    var arrowKeyOrMouseWheelEvent = (event.keyIdentifier === "Up" || event.keyIdentifier === "Down" || event.type === "mousewheel");

    // Jump by 10 when shift is down or jump by 0.1 when Alt/Option is down.
    // Also jump by 10 for page up and down, or by 100 if shift is held with a page key.
    var changeAmount = 1;
    if (event.shiftKey && !arrowKeyOrMouseWheelEvent)
        changeAmount = 100;
    else if (event.shiftKey || !arrowKeyOrMouseWheelEvent)
        changeAmount = 10;
    else if (event.altKey)
        changeAmount = 0.1;

    if (direction === "Down")
        changeAmount *= -1;

    // Make the new number and constrain it to a precision of 6, this matches numbers the engine returns.
    // Use the Number constructor to forget the fixed precision, so 1.100000 will print as 1.1.
    var result = Number((number + changeAmount).toFixed(6));
    if (!String(result).match(WebInspector.CSSNumberRegex))
        return null;

    return result;
}

/**
  * @param {Event} event
  * @param {Element} element
  * @param {function(string,string)=} finishHandler
  * @param {function(string)=} suggestionHandler
  * @param {function(number):number=} customNumberHandler
 */
WebInspector.handleElementValueModifications = function(event, element, finishHandler, suggestionHandler, customNumberHandler)
{
    var arrowKeyOrMouseWheelEvent = (event.keyIdentifier === "Up" || event.keyIdentifier === "Down" || event.type === "mousewheel");
    var pageKeyPressed = (event.keyIdentifier === "PageUp" || event.keyIdentifier === "PageDown");
    if (!arrowKeyOrMouseWheelEvent && !pageKeyPressed)
        return false;

    var selection = window.getSelection();
    if (!selection.rangeCount)
        return false;

    var selectionRange = selection.getRangeAt(0);
    if (!selectionRange.commonAncestorContainer.isSelfOrDescendant(element))
        return false;

    var originalValue = element.textContent;
    var wordRange = selectionRange.startContainer.rangeOfWord(selectionRange.startOffset, WebInspector.StyleValueDelimiters, element);
    var wordString = wordRange.toString();
    
    if (suggestionHandler && suggestionHandler(wordString))
        return false;

    var replacementString;
    var prefix, suffix, number;

    var matches;
    matches = /(.*#)([\da-fA-F]+)(.*)/.exec(wordString);
    if (matches && matches.length) {
        prefix = matches[1];
        suffix = matches[3];
        number = WebInspector._modifiedHexValue(matches[2], event);
        
        if (customNumberHandler)
            number = customNumberHandler(number);

        replacementString = prefix + number + suffix;
    } else {
        matches = /(.*?)(-?(?:\d+(?:\.\d+)?|\.\d+))(.*)/.exec(wordString);
        if (matches && matches.length) {
            prefix = matches[1];
            suffix = matches[3];
            number = WebInspector._modifiedFloatNumber(parseFloat(matches[2]), event);
            
            // Need to check for null explicitly.
            if (number === null)                
                return false;
            
            if (customNumberHandler)
                number = customNumberHandler(number);

            replacementString = prefix + number + suffix;
        }
    }

    if (replacementString) {
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
                
        if (finishHandler)
            finishHandler(originalValue, replacementString);

        return true;
    }
    return false;
}

/** 
 * @param {Element} element
 * @param {WebInspector.EditingConfig=} config
 */
WebInspector.startEditing = function(element, config)
{
    if (!WebInspector.markBeingEdited(element, true))
        return null;

    config = config || new WebInspector.EditingConfig(function() {}, function() {});
    var committedCallback = config.commitHandler;
    var cancelledCallback = config.cancelHandler;
    var pasteCallback = config.pasteHandler;
    var context = config.context;
    var isMultiline = config.multiline || false;
    var oldText = isMultiline ? config.initialValue : getContent(element);
    var moveDirection = "";
    var oldTabIndex;
    var codeMirror;
    var cssLoadView;

    function consumeCopy(e)
    {
        e.consume();
    }

    if (isMultiline) {
        loadScript("CodeMirrorTextEditor.js");
        cssLoadView = new WebInspector.CodeMirrorCSSLoadView();
        cssLoadView.show(element);
        WebInspector.setCurrentFocusElement(element);
        element.addEventListener("copy", consumeCopy, true);
        codeMirror = window.CodeMirror(element, {
            mode: config.mode,
            lineWrapping: config.lineWrapping,
            smartIndent: config.smartIndent,
            autofocus: true,
            theme: config.theme,
            value: oldText
        });
    } else {
        element.addStyleClass("editing");

        oldTabIndex = element.getAttribute("tabIndex");
        if (typeof oldTabIndex !== "number" || oldTabIndex < 0)
            element.tabIndex = 0;
        WebInspector.setCurrentFocusElement(element);
    }

    /**
     * @param {Event=} e
     */
    function blurEventListener(e) {
        if (!isMultiline || !e || !e.relatedTarget || !e.relatedTarget.isSelfOrDescendant(element))
            editingCommitted.call(element);
    }

    function getContent(element) {
        if (isMultiline)
            return codeMirror.getValue();

        if (element.tagName === "INPUT" && element.type === "text")
            return element.value;

        return element.textContent;
    }

    /** @this {Element} */
    function cleanUpAfterEditing()
    {
        WebInspector.markBeingEdited(element, false);

        element.removeEventListener("blur", blurEventListener, isMultiline);
        element.removeEventListener("keydown", keyDownEventListener, true);
        if (pasteCallback)
            element.removeEventListener("paste", pasteEventListener, true);

        WebInspector.restoreFocusFromElement(element);

        if (isMultiline) {
            element.removeEventListener("copy", consumeCopy, true);
            cssLoadView.detach();
            return;
        }

        this.removeStyleClass("editing");
        
        if (typeof oldTabIndex !== "number")
            element.removeAttribute("tabIndex");
        else
            this.tabIndex = oldTabIndex;
        this.scrollTop = 0;
        this.scrollLeft = 0;
    }

    /** @this {Element} */
    function editingCancelled()
    {
        if (isMultiline)
            codeMirror.setValue(oldText);
        else {
            if (this.tagName === "INPUT" && this.type === "text")
                this.value = oldText;
            else
                this.textContent = oldText;
        }

        cleanUpAfterEditing.call(this);

        cancelledCallback(this, context);
    }

    /** @this {Element} */
    function editingCommitted()
    {
        cleanUpAfterEditing.call(this);

        committedCallback(this, getContent(this), oldText, context, moveDirection);
    }

    function defaultFinishHandler(event)
    {
        var isMetaOrCtrl = WebInspector.isMac() ?
            event.metaKey && !event.shiftKey && !event.ctrlKey && !event.altKey :
            event.ctrlKey && !event.shiftKey && !event.metaKey && !event.altKey;
        if (isEnterKey(event) && (event.isMetaOrCtrlForTest || !isMultiline || isMetaOrCtrl))
            return "commit";
        else if (event.keyCode === WebInspector.KeyboardShortcut.Keys.Esc.code || event.keyIdentifier === "U+001B")
            return "cancel";
        else if (!isMultiline && event.keyIdentifier === "U+0009") // Tab key
            return "move-" + (event.shiftKey ? "backward" : "forward");
    }

    function handleEditingResult(result, event)
    {
        if (result === "commit") {
            editingCommitted.call(element);
            event.consume(true);
        } else if (result === "cancel") {
            editingCancelled.call(element);
            event.consume(true);
        } else if (result && result.startsWith("move-")) {
            moveDirection = result.substring(5);
            if (event.keyIdentifier !== "U+0009")
                blurEventListener();
        }
    }

    function pasteEventListener(event)
    {
        var result = pasteCallback(event);
        handleEditingResult(result, event);
    }

    function keyDownEventListener(event)
    {
        var handler = config.customFinishHandler || defaultFinishHandler;
        var result = handler(event);
        handleEditingResult(result, event);
    }

    element.addEventListener("blur", blurEventListener, isMultiline);
    element.addEventListener("keydown", keyDownEventListener, true);
    if (pasteCallback)
        element.addEventListener("paste", pasteEventListener, true);

    return {
        cancel: editingCancelled.bind(element),
        commit: editingCommitted.bind(element),
        codeMirror: codeMirror // For testing.
    };
}

/**
 * @param {number} seconds
 * @param {boolean=} higherResolution
 * @return {string}
 */
Number.secondsToString = function(seconds, higherResolution)
{
    if (!isFinite(seconds))
        return "-";

    if (seconds === 0)
        return "0";

    var ms = seconds * 1000;
    if (higherResolution && ms < 1000)
        return WebInspector.UIString("%.3f\u2009ms", ms);
    else if (ms < 1000)
        return WebInspector.UIString("%.0f\u2009ms", ms);

    if (seconds < 60)
        return WebInspector.UIString("%.2f\u2009s", seconds);

    var minutes = seconds / 60;
    if (minutes < 60)
        return WebInspector.UIString("%.1f\u2009min", minutes);

    var hours = minutes / 60;
    if (hours < 24)
        return WebInspector.UIString("%.1f\u2009hrs", hours);

    var days = hours / 24;
    return WebInspector.UIString("%.1f\u2009days", days);
}

/**
 * @param {number} bytes
 * @return {string}
 */
Number.bytesToString = function(bytes)
{
    if (bytes < 1024)
        return WebInspector.UIString("%.0f\u2009B", bytes);

    var kilobytes = bytes / 1024;
    if (kilobytes < 100)
        return WebInspector.UIString("%.1f\u2009KB", kilobytes);
    if (kilobytes < 1024)
        return WebInspector.UIString("%.0f\u2009KB", kilobytes);

    var megabytes = kilobytes / 1024;
    if (megabytes < 100)
        return WebInspector.UIString("%.1f\u2009MB", megabytes);
    else
        return WebInspector.UIString("%.0f\u2009MB", megabytes);
}

Number.withThousandsSeparator = function(num)
{
    var str = num + "";
    var re = /(\d+)(\d{3})/;
    while (str.match(re))
        str = str.replace(re, "$1\u2009$2"); // \u2009 is a thin space.
    return str;
}

WebInspector.useLowerCaseMenuTitles = function()
{
    return WebInspector.platform() === "windows" && Preferences.useLowerCaseMenuTitlesOnWindows;
}

WebInspector.formatLocalized = function(format, substitutions, formatters, initialValue, append)
{
    return String.format(WebInspector.UIString(format), substitutions, formatters, initialValue, append);
}

WebInspector.openLinkExternallyLabel = function()
{
    return WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Open link in new tab" : "Open Link in New Tab");
}

WebInspector.copyLinkAddressLabel = function()
{
    return WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Copy link address" : "Copy Link Address");
}

WebInspector.platform = function()
{
    if (!WebInspector._platform)
        WebInspector._platform = InspectorFrontendHost.platform();
    return WebInspector._platform;
}

WebInspector.isMac = function()
{
    if (typeof WebInspector._isMac === "undefined")
        WebInspector._isMac = WebInspector.platform() === "mac";

    return WebInspector._isMac;
}

WebInspector.isWin = function()
{
    if (typeof WebInspector._isWin === "undefined")
        WebInspector._isWin = WebInspector.platform() === "windows";

    return WebInspector._isWin;
}

WebInspector.PlatformFlavor = {
    WindowsVista: "windows-vista",
    MacTiger: "mac-tiger",
    MacLeopard: "mac-leopard",
    MacSnowLeopard: "mac-snowleopard",
    MacLion: "mac-lion",
    MacMountainLion: "mac-mountain-lion"
}

WebInspector.platformFlavor = function()
{
    function detectFlavor()
    {
        const userAgent = navigator.userAgent;

        if (WebInspector.platform() === "windows") {
            var match = userAgent.match(/Windows NT (\d+)\.(?:\d+)/);
            if (match && match[1] >= 6)
                return WebInspector.PlatformFlavor.WindowsVista;
            return null;
        } else if (WebInspector.platform() === "mac") {
            var match = userAgent.match(/Mac OS X\s*(?:(\d+)_(\d+))?/);
            if (!match || match[1] != 10)
                return WebInspector.PlatformFlavor.MacSnowLeopard;
            switch (Number(match[2])) {
                case 4:
                    return WebInspector.PlatformFlavor.MacTiger;
                case 5:
                    return WebInspector.PlatformFlavor.MacLeopard;
                case 6:
                    return WebInspector.PlatformFlavor.MacSnowLeopard;
                case 7:
                    return WebInspector.PlatformFlavor.MacLion;
                case 8:
                    return WebInspector.PlatformFlavor.MacMountainLion;
                default:
                    return "";
            }
        }
    }

    if (!WebInspector._platformFlavor)
        WebInspector._platformFlavor = detectFlavor();

    return WebInspector._platformFlavor;
}

WebInspector.port = function()
{
    if (!WebInspector._port)
        WebInspector._port = InspectorFrontendHost.port();

    return WebInspector._port;
}

WebInspector.installPortStyles = function()
{
    var platform = WebInspector.platform();
    document.body.addStyleClass("platform-" + platform);
    var flavor = WebInspector.platformFlavor();
    if (flavor)
        document.body.addStyleClass("platform-" + flavor);
    var port = WebInspector.port();
    document.body.addStyleClass("port-" + port);
}

WebInspector._windowFocused = function(event)
{
    if (event.target.document.nodeType === Node.DOCUMENT_NODE)
        document.body.removeStyleClass("inactive");
}

WebInspector._windowBlurred = function(event)
{
    if (event.target.document.nodeType === Node.DOCUMENT_NODE)
        document.body.addStyleClass("inactive");
}

WebInspector.previousFocusElement = function()
{
    return WebInspector._previousFocusElement;
}

WebInspector.currentFocusElement = function()
{
    return WebInspector._currentFocusElement;
}

WebInspector._focusChanged = function(event)
{
    WebInspector.setCurrentFocusElement(event.target);
}

WebInspector._textInputTypes = ["text", "search", "tel", "url", "email", "password"].keySet(); 
WebInspector._isTextEditingElement = function(element)
{
    if (element instanceof HTMLInputElement)
        return element.type in WebInspector._textInputTypes;

    if (element instanceof HTMLTextAreaElement)
        return true;

    return false;
}

WebInspector.setCurrentFocusElement = function(x)
{
    if (WebInspector._glassPane && x && !WebInspector._glassPane.element.isAncestor(x))
        return;
    if (WebInspector._currentFocusElement !== x)
        WebInspector._previousFocusElement = WebInspector._currentFocusElement;
    WebInspector._currentFocusElement = x;

    if (WebInspector._currentFocusElement) {
        WebInspector._currentFocusElement.focus();

        // Make a caret selection inside the new element if there isn't a range selection and there isn't already a caret selection inside.
        // This is needed (at least) to remove caret from console when focus is moved to some element in the panel.
        // The code below should not be applied to text fields and text areas, hence _isTextEditingElement check.
        var selection = window.getSelection();
        if (!WebInspector._isTextEditingElement(WebInspector._currentFocusElement) && selection.isCollapsed && !WebInspector._currentFocusElement.isInsertionCaretInside()) {
            var selectionRange = WebInspector._currentFocusElement.ownerDocument.createRange();
            selectionRange.setStart(WebInspector._currentFocusElement, 0);
            selectionRange.setEnd(WebInspector._currentFocusElement, 0);

            selection.removeAllRanges();
            selection.addRange(selectionRange);
        }
    } else if (WebInspector._previousFocusElement)
        WebInspector._previousFocusElement.blur();
}

WebInspector.restoreFocusFromElement = function(element)
{
    if (element && element.isSelfOrAncestor(WebInspector.currentFocusElement()))
        WebInspector.setCurrentFocusElement(WebInspector.previousFocusElement());
}

WebInspector.setToolbarColors = function(backgroundColor, color)
{
    if (!WebInspector._themeStyleElement) {
        WebInspector._themeStyleElement = document.createElement("style");
        document.head.appendChild(WebInspector._themeStyleElement);
    }
    WebInspector._themeStyleElement.textContent =
        "#toolbar {\
             background-image: none !important;\
             background-color: " + backgroundColor + " !important;\
         }\
         \
         .toolbar-label {\
             color: " + color + " !important;\
             text-shadow: none;\
         }";
}

WebInspector.resetToolbarColors = function()
{
    if (WebInspector._themeStyleElement)
        WebInspector._themeStyleElement.textContent = "";
}

/**
 * @param {Element} element
 * @param {number} offset
 * @param {number} length
 * @param {Array.<Object>=} domChanges
 */
WebInspector.highlightSearchResult = function(element, offset, length, domChanges)
{
    var result = WebInspector.highlightSearchResults(element, [{offset: offset, length: length }], domChanges);
    return result.length ? result[0] : null;
}

/**
 * @param {Element} element
 * @param {Array.<Object>} resultRanges
 * @param {Array.<Object>=} changes
 */
WebInspector.highlightSearchResults = function(element, resultRanges, changes)
{
    return WebInspector.highlightRangesWithStyleClass(element, resultRanges, "webkit-search-result", changes);
}

/**
 * @param {Element} element
 * @param {Array.<Object>} resultRanges
 * @param {string} styleClass
 * @param {Array.<Object>=} changes
 */
WebInspector.highlightRangesWithStyleClass = function(element, resultRanges, styleClass, changes)
{
    changes = changes || [];
    var highlightNodes = [];
    var lineText = element.textContent;
    var ownerDocument = element.ownerDocument;
    var textNodeSnapshot = ownerDocument.evaluate(".//text()", element, null, XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null);

    var snapshotLength = textNodeSnapshot.snapshotLength;
    if (snapshotLength === 0)
        return highlightNodes;

    var nodeRanges = [];
    var rangeEndOffset = 0;
    for (var i = 0; i < snapshotLength; ++i) {
        var range = {};
        range.offset = rangeEndOffset;
        range.length = textNodeSnapshot.snapshotItem(i).textContent.length;
        rangeEndOffset = range.offset + range.length;
        nodeRanges.push(range);
    }

    var startIndex = 0;
    for (var i = 0; i < resultRanges.length; ++i) {
        var startOffset = resultRanges[i].offset;
        var endOffset = startOffset + resultRanges[i].length;

        while (startIndex < snapshotLength && nodeRanges[startIndex].offset + nodeRanges[startIndex].length <= startOffset)
            startIndex++;
        var endIndex = startIndex;
        while (endIndex < snapshotLength && nodeRanges[endIndex].offset + nodeRanges[endIndex].length < endOffset)
            endIndex++;
        if (endIndex === snapshotLength)
            break;

        var highlightNode = ownerDocument.createElement("span");
        highlightNode.className = styleClass;
        highlightNode.textContent = lineText.substring(startOffset, endOffset);

        var lastTextNode = textNodeSnapshot.snapshotItem(endIndex);
        var lastText = lastTextNode.textContent;
        lastTextNode.textContent = lastText.substring(endOffset - nodeRanges[endIndex].offset);
        changes.push({ node: lastTextNode, type: "changed", oldText: lastText, newText: lastTextNode.textContent });

        if (startIndex === endIndex) {
            lastTextNode.parentElement.insertBefore(highlightNode, lastTextNode);
            changes.push({ node: highlightNode, type: "added", nextSibling: lastTextNode, parent: lastTextNode.parentElement });
            highlightNodes.push(highlightNode);

            var prefixNode = ownerDocument.createTextNode(lastText.substring(0, startOffset - nodeRanges[startIndex].offset));
            lastTextNode.parentElement.insertBefore(prefixNode, highlightNode);
            changes.push({ node: prefixNode, type: "added", nextSibling: highlightNode, parent: lastTextNode.parentElement });
        } else {
            var firstTextNode = textNodeSnapshot.snapshotItem(startIndex);
            var firstText = firstTextNode.textContent;
            var anchorElement = firstTextNode.nextSibling;

            firstTextNode.parentElement.insertBefore(highlightNode, anchorElement);
            changes.push({ node: highlightNode, type: "added", nextSibling: anchorElement, parent: firstTextNode.parentElement });
            highlightNodes.push(highlightNode);

            firstTextNode.textContent = firstText.substring(0, startOffset - nodeRanges[startIndex].offset);
            changes.push({ node: firstTextNode, type: "changed", oldText: firstText, newText: firstTextNode.textContent });

            for (var j = startIndex + 1; j < endIndex; j++) {
                var textNode = textNodeSnapshot.snapshotItem(j);
                var text = textNode.textContent;
                textNode.textContent = "";
                changes.push({ node: textNode, type: "changed", oldText: text, newText: textNode.textContent });
            }
        }
        startIndex = endIndex;
        nodeRanges[startIndex].offset = endOffset;
        nodeRanges[startIndex].length = lastTextNode.textContent.length;

    }
    return highlightNodes;
}

WebInspector.applyDomChanges = function(domChanges)
{
    for (var i = 0, size = domChanges.length; i < size; ++i) {
        var entry = domChanges[i];
        switch (entry.type) {
        case "added":
            entry.parent.insertBefore(entry.node, entry.nextSibling);
            break;
        case "changed":
            entry.node.textContent = entry.newText;
            break;
        }
    }
}

WebInspector.revertDomChanges = function(domChanges)
{
    for (var i = domChanges.length - 1; i >= 0; --i) {
        var entry = domChanges[i];
        switch (entry.type) {
        case "added":
            if (entry.node.parentElement)
                entry.node.parentElement.removeChild(entry.node);
            break;
        case "changed":
            entry.node.textContent = entry.oldText;
            break;
        }
    }
}

WebInspector._coalescingLevel = 0;

WebInspector.startBatchUpdate = function()
{
    if (!WebInspector._coalescingLevel)
        WebInspector._postUpdateHandlers = new Map();
    WebInspector._coalescingLevel++;
}

WebInspector.endBatchUpdate = function()
{
    if (--WebInspector._coalescingLevel)
        return;

    var handlers = WebInspector._postUpdateHandlers;
    delete WebInspector._postUpdateHandlers;

    var keys = handlers.keys();
    for (var i = 0; i < keys.length; ++i) {
        var object = keys[i];
        var methods = handlers.get(object).keys();
        for (var j = 0; j < methods.length; ++j)
            methods[j].call(object);
    }
}

/**
 * @param {Object} object
 * @param {function()} method
 */
WebInspector.invokeOnceAfterBatchUpdate = function(object, method)
{
    if (!WebInspector._coalescingLevel) {
        method.call(object);
        return;
    }
    
    var methods = WebInspector._postUpdateHandlers.get(object);
    if (!methods) {
        methods = new Map();
        WebInspector._postUpdateHandlers.put(object, methods);
    }
    methods.put(method);
}

/**
 * This bogus view is needed to load/unload CodeMirror-related CSS on demand.
 *
 * @constructor
 * @extends {WebInspector.View}
 */
WebInspector.CodeMirrorCSSLoadView = function()
{
    WebInspector.View.call(this);
    this.element.addStyleClass("hidden");
    this.registerRequiredCSS("cm/codemirror.css");
    this.registerRequiredCSS("cm/cmdevtools.css");
}

WebInspector.CodeMirrorCSSLoadView.prototype = {
    __proto__: WebInspector.View.prototype
}

;(function() {

function windowLoaded()
{
    window.addEventListener("focus", WebInspector._windowFocused, false);
    window.addEventListener("blur", WebInspector._windowBlurred, false);
    document.addEventListener("focus", WebInspector._focusChanged.bind(this), true);
    window.removeEventListener("DOMContentLoaded", windowLoaded, false);
}

window.addEventListener("DOMContentLoaded", windowLoaded, false);

})();
