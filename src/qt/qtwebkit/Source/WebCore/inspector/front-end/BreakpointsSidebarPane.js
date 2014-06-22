/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @param {WebInspector.BreakpointManager} breakpointManager
 * @extends {WebInspector.SidebarPane}
 */
WebInspector.JavaScriptBreakpointsSidebarPane = function(breakpointManager, showSourceLineDelegate)
{
    WebInspector.SidebarPane.call(this, WebInspector.UIString("Breakpoints"));
    this.registerRequiredCSS("breakpointsList.css");

    this._breakpointManager = breakpointManager;
    this._showSourceLineDelegate = showSourceLineDelegate;

    this.listElement = document.createElement("ol");
    this.listElement.className = "breakpoint-list";

    this.emptyElement = document.createElement("div");
    this.emptyElement.className = "info";
    this.emptyElement.textContent = WebInspector.UIString("No Breakpoints");

    this.bodyElement.appendChild(this.emptyElement);

    this._items = new Map();
    
    var breakpointLocations = this._breakpointManager.allBreakpointLocations();
    for (var i = 0; i < breakpointLocations.length; ++i)
        this._addBreakpoint(breakpointLocations[i].breakpoint, breakpointLocations[i].uiLocation);

    this._breakpointManager.addEventListener(WebInspector.BreakpointManager.Events.BreakpointAdded, this._breakpointAdded, this);
    this._breakpointManager.addEventListener(WebInspector.BreakpointManager.Events.BreakpointRemoved, this._breakpointRemoved, this);

    this.emptyElement.addEventListener("contextmenu", this._emptyElementContextMenu.bind(this), true);
}

WebInspector.JavaScriptBreakpointsSidebarPane.prototype = {
    _emptyElementContextMenu: function(event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);
        var breakpointActive = WebInspector.debuggerModel.breakpointsActive();
        var breakpointActiveTitle = breakpointActive ?
            WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Deactivate breakpoints" : "Deactivate Breakpoints") :
            WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Activate breakpoints" : "Activate Breakpoints");
        contextMenu.appendItem(breakpointActiveTitle, WebInspector.debuggerModel.setBreakpointsActive.bind(WebInspector.debuggerModel, !breakpointActive));
        contextMenu.show();
    },

    /**
     * @param {WebInspector.Event} event
     */
    _breakpointAdded: function(event)
    {
        this._breakpointRemoved(event);

        var breakpoint = /** @type {WebInspector.BreakpointManager.Breakpoint} */ (event.data.breakpoint);
        var uiLocation = /** @type {WebInspector.UILocation} */ (event.data.uiLocation);
        this._addBreakpoint(breakpoint, uiLocation);
    },

    /**
     * @param {WebInspector.BreakpointManager.Breakpoint} breakpoint
     * @param {WebInspector.UILocation} uiLocation
     */
    _addBreakpoint: function(breakpoint, uiLocation)
    {
        var element = document.createElement("li");
        element.addStyleClass("cursor-pointer");
        element.addEventListener("contextmenu", this._breakpointContextMenu.bind(this, breakpoint), true);
        element.addEventListener("click", this._breakpointClicked.bind(this, uiLocation), false);

        var checkbox = document.createElement("input");
        checkbox.className = "checkbox-elem";
        checkbox.type = "checkbox";
        checkbox.checked = breakpoint.enabled();
        checkbox.addEventListener("click", this._breakpointCheckboxClicked.bind(this, breakpoint), false);
        element.appendChild(checkbox);

        var labelElement = document.createTextNode(WebInspector.formatLinkText(uiLocation.uiSourceCode.originURL(), uiLocation.lineNumber));
        element.appendChild(labelElement);

        var snippetElement = document.createElement("div");
        snippetElement.className = "source-text monospace";
        element.appendChild(snippetElement);

        /**
         * @param {?string} content
         * @param {boolean} contentEncoded
         * @param {string} mimeType
         */
        function didRequestContent(content, contentEncoded, mimeType)
        {
            var lineEndings = content.lineEndings();
            if (uiLocation.lineNumber < lineEndings.length)
                snippetElement.textContent = content.substring(lineEndings[uiLocation.lineNumber - 1], lineEndings[uiLocation.lineNumber]);
        }
        uiLocation.uiSourceCode.requestContent(didRequestContent.bind(this));

        element._data = uiLocation;
        var currentElement = this.listElement.firstChild;
        while (currentElement) {
            if (currentElement._data && this._compareBreakpoints(currentElement._data, element._data) > 0)
                break;
            currentElement = currentElement.nextSibling;
        }
        this._addListElement(element, currentElement);

        var breakpointItem = {};
        breakpointItem.element = element;
        breakpointItem.checkbox = checkbox;
        this._items.put(breakpoint, breakpointItem);

        this.expand();
    },

    /**
     * @param {WebInspector.Event} event
     */
    _breakpointRemoved: function(event)
    {
        var breakpoint = /** @type {WebInspector.BreakpointManager.Breakpoint} */ (event.data.breakpoint);
        var uiLocation = /** @type {WebInspector.UILocation} */ (event.data.uiLocation);
        var breakpointItem = this._items.get(breakpoint);
        if (!breakpointItem)
            return;
        this._items.remove(breakpoint);
        this._removeListElement(breakpointItem.element);
    },

    /**
     * @param {WebInspector.BreakpointManager.Breakpoint} breakpoint
     */
    highlightBreakpoint: function(breakpoint)
    {
        var breakpointItem = this._items.get(breakpoint);
        if (!breakpointItem)
            return;
        breakpointItem.element.addStyleClass("breakpoint-hit");
        this._highlightedBreakpointItem = breakpointItem;
    },

    clearBreakpointHighlight: function()
    {
        if (this._highlightedBreakpointItem) {
            this._highlightedBreakpointItem.element.removeStyleClass("breakpoint-hit");
            delete this._highlightedBreakpointItem;
        }
    },

    _breakpointClicked: function(uiLocation, event)
    {
        this._showSourceLineDelegate(uiLocation.uiSourceCode, uiLocation.lineNumber);
    },

    /**
     * @param {WebInspector.BreakpointManager.Breakpoint} breakpoint
     */
    _breakpointCheckboxClicked: function(breakpoint, event)
    {
        // Breakpoint element has it's own click handler.
        event.consume();
        breakpoint.setEnabled(event.target.checked);
    },

    /**
     * @param {WebInspector.BreakpointManager.Breakpoint} breakpoint
     */
    _breakpointContextMenu: function(breakpoint, event)
    {
        var breakpoints = this._items.values();
        var contextMenu = new WebInspector.ContextMenu(event);
        contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Remove breakpoint" : "Remove Breakpoint"), breakpoint.remove.bind(breakpoint));
        if (breakpoints.length > 1) {
            var removeAllTitle = WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Remove all breakpoints" : "Remove All Breakpoints");
            contextMenu.appendItem(removeAllTitle, this._breakpointManager.removeAllBreakpoints.bind(this._breakpointManager));
        }

        contextMenu.appendSeparator();
        var breakpointActive = WebInspector.debuggerModel.breakpointsActive();
        var breakpointActiveTitle = breakpointActive ?
            WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Deactivate breakpoints" : "Deactivate Breakpoints") :
            WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Activate breakpoints" : "Activate Breakpoints");
        contextMenu.appendItem(breakpointActiveTitle, WebInspector.debuggerModel.setBreakpointsActive.bind(WebInspector.debuggerModel, !breakpointActive));

        function enabledBreakpointCount(breakpoints)
        {
            var count = 0;
            for (var i = 0; i < breakpoints.length; ++i) {
                if (breakpoints[i].checkbox.checked)
                    count++;
            }
            return count;
        }
        if (breakpoints.length > 1) {
            var enableBreakpointCount = enabledBreakpointCount(breakpoints);
            var enableTitle = WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Enable all breakpoints" : "Enable All Breakpoints");
            var disableTitle = WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Disable all breakpoints" : "Disable All Breakpoints");

            contextMenu.appendSeparator();

            contextMenu.appendItem(enableTitle, this._breakpointManager.toggleAllBreakpoints.bind(this._breakpointManager, true), !(enableBreakpointCount != breakpoints.length));
            contextMenu.appendItem(disableTitle, this._breakpointManager.toggleAllBreakpoints.bind(this._breakpointManager, false), !(enableBreakpointCount > 1));
        }

        contextMenu.show();
    },

    _addListElement: function(element, beforeElement)
    {
        if (beforeElement)
            this.listElement.insertBefore(element, beforeElement);
        else {
            if (!this.listElement.firstChild) {
                this.bodyElement.removeChild(this.emptyElement);
                this.bodyElement.appendChild(this.listElement);
            }
            this.listElement.appendChild(element);
        }
    },

    _removeListElement: function(element)
    {
        this.listElement.removeChild(element);
        if (!this.listElement.firstChild) {
            this.bodyElement.removeChild(this.listElement);
            this.bodyElement.appendChild(this.emptyElement);
        }
    },

    _compare: function(x, y)
    {
        if (x !== y)
            return x < y ? -1 : 1;
        return 0;
    },

    _compareBreakpoints: function(b1, b2)
    {
        return this._compare(b1.uiSourceCode.originURL(), b2.uiSourceCode.originURL()) || this._compare(b1.lineNumber, b2.lineNumber);
    },

    reset: function()
    {
        this.listElement.removeChildren();
        if (this.listElement.parentElement) {
            this.bodyElement.removeChild(this.listElement);
            this.bodyElement.appendChild(this.emptyElement);
        }
        this._items.clear();
    },

    __proto__: WebInspector.SidebarPane.prototype
}

/**
 * @constructor
 * @extends {WebInspector.NativeBreakpointsSidebarPane}
 */
WebInspector.XHRBreakpointsSidebarPane = function()
{
    WebInspector.NativeBreakpointsSidebarPane.call(this, WebInspector.UIString("XHR Breakpoints"));

    this._breakpointElements = {};

    var addButton = document.createElement("button");
    addButton.className = "pane-title-button add";
    addButton.addEventListener("click", this._addButtonClicked.bind(this), false);
    addButton.title = WebInspector.UIString("Add XHR breakpoint");
    this.titleElement.appendChild(addButton);

    this.emptyElement.addEventListener("contextmenu", this._emptyElementContextMenu.bind(this), true);

    this._restoreBreakpoints();
}

WebInspector.XHRBreakpointsSidebarPane.prototype = {
    _emptyElementContextMenu: function(event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);
        contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Add breakpoint" : "Add Breakpoint"), this._addButtonClicked.bind(this));
        contextMenu.show();
    },

    _addButtonClicked: function(event)
    {
        if (event)
            event.consume();

        this.expand();

        var inputElementContainer = document.createElement("p");
        inputElementContainer.className = "breakpoint-condition";
        var inputElement = document.createElement("span");
        inputElementContainer.textContent = WebInspector.UIString("Break when URL contains:");
        inputElement.className = "editing";
        inputElement.id = "breakpoint-condition-input";
        inputElementContainer.appendChild(inputElement);
        this._addListElement(inputElementContainer, this.listElement.firstChild);

        function finishEditing(accept, e, text)
        {
            this._removeListElement(inputElementContainer);
            if (accept) {
                this._setBreakpoint(text, true);
                this._saveBreakpoints();
            }
        }

        var config = new WebInspector.EditingConfig(finishEditing.bind(this, true), finishEditing.bind(this, false));
        WebInspector.startEditing(inputElement, config);
    },

    _setBreakpoint: function(url, enabled)
    {
        if (url in this._breakpointElements)
            return;

        var element = document.createElement("li");
        element._url = url;
        element.addEventListener("contextmenu", this._contextMenu.bind(this, url), true);

        var checkboxElement = document.createElement("input");
        checkboxElement.className = "checkbox-elem";
        checkboxElement.type = "checkbox";
        checkboxElement.checked = enabled;
        checkboxElement.addEventListener("click", this._checkboxClicked.bind(this, url), false);
        element._checkboxElement = checkboxElement;
        element.appendChild(checkboxElement);

        var labelElement = document.createElement("span");
        if (!url)
            labelElement.textContent = WebInspector.UIString("Any XHR");
        else
            labelElement.textContent = WebInspector.UIString("URL contains \"%s\"", url);
        labelElement.addStyleClass("cursor-auto");
        labelElement.addEventListener("dblclick", this._labelClicked.bind(this, url), false);
        element.appendChild(labelElement);

        var currentElement = this.listElement.firstChild;
        while (currentElement) {
            if (currentElement._url && currentElement._url < element._url)
                break;
            currentElement = currentElement.nextSibling;
        }
        this._addListElement(element, currentElement);
        this._breakpointElements[url] = element;
        if (enabled)
            DOMDebuggerAgent.setXHRBreakpoint(url);
    },

    _removeBreakpoint: function(url)
    {
        var element = this._breakpointElements[url];
        if (!element)
            return;

        this._removeListElement(element);
        delete this._breakpointElements[url];
        if (element._checkboxElement.checked)
            DOMDebuggerAgent.removeXHRBreakpoint(url);
    },

    _contextMenu: function(url, event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);
        function removeBreakpoint()
        {
            this._removeBreakpoint(url);
            this._saveBreakpoints();
        }
        function removeAllBreakpoints()
        {
            for (var url in this._breakpointElements)
                this._removeBreakpoint(url);
            this._saveBreakpoints();
        }
        var removeAllTitle = WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Remove all breakpoints" : "Remove All Breakpoints");

        contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Add breakpoint" : "Add Breakpoint"), this._addButtonClicked.bind(this));
        contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Remove breakpoint" : "Remove Breakpoint"), removeBreakpoint.bind(this));
        contextMenu.appendItem(removeAllTitle, removeAllBreakpoints.bind(this));
        contextMenu.show();
    },

    _checkboxClicked: function(url, event)
    {
        if (event.target.checked)
            DOMDebuggerAgent.setXHRBreakpoint(url);
        else
            DOMDebuggerAgent.removeXHRBreakpoint(url);
        this._saveBreakpoints();
    },

    _labelClicked: function(url)
    {
        var element = this._breakpointElements[url];
        var inputElement = document.createElement("span");
        inputElement.className = "breakpoint-condition editing";
        inputElement.textContent = url;
        this.listElement.insertBefore(inputElement, element);
        element.addStyleClass("hidden");

        function finishEditing(accept, e, text)
        {
            this._removeListElement(inputElement);
            if (accept) {
                this._removeBreakpoint(url);
                this._setBreakpoint(text, element._checkboxElement.checked);
                this._saveBreakpoints();
            } else
                element.removeStyleClass("hidden");
        }

        WebInspector.startEditing(inputElement, new WebInspector.EditingConfig(finishEditing.bind(this, true), finishEditing.bind(this, false)));
    },

    highlightBreakpoint: function(url)
    {
        var element = this._breakpointElements[url];
        if (!element)
            return;
        this.expand();
        element.addStyleClass("breakpoint-hit");
        this._highlightedElement = element;
    },

    clearBreakpointHighlight: function()
    {
        if (this._highlightedElement) {
            this._highlightedElement.removeStyleClass("breakpoint-hit");
            delete this._highlightedElement;
        }
    },

    _saveBreakpoints: function()
    {
        var breakpoints = [];
        for (var url in this._breakpointElements)
            breakpoints.push({ url: url, enabled: this._breakpointElements[url]._checkboxElement.checked });
        WebInspector.settings.xhrBreakpoints.set(breakpoints);
    },

    _restoreBreakpoints: function()
    {
        var breakpoints = WebInspector.settings.xhrBreakpoints.get();
        for (var i = 0; i < breakpoints.length; ++i) {
            var breakpoint = breakpoints[i];
            if (breakpoint && typeof breakpoint.url === "string")
                this._setBreakpoint(breakpoint.url, breakpoint.enabled);
        }
    },

    __proto__: WebInspector.NativeBreakpointsSidebarPane.prototype
}

/**
 * @constructor
 * @extends {WebInspector.SidebarPane}
 */
WebInspector.EventListenerBreakpointsSidebarPane = function()
{
    WebInspector.SidebarPane.call(this, WebInspector.UIString("Event Listener Breakpoints"));
    this.registerRequiredCSS("breakpointsList.css");

    this.categoriesElement = document.createElement("ol");
    this.categoriesElement.tabIndex = 0;
    this.categoriesElement.addStyleClass("properties-tree");
    this.categoriesElement.addStyleClass("event-listener-breakpoints");
    this.categoriesTreeOutline = new TreeOutline(this.categoriesElement);
    this.bodyElement.appendChild(this.categoriesElement);

    this._breakpointItems = {};
    // FIXME: uncomment following once inspector stops being drop targer in major ports.
    // Otherwise, inspector page reacts on drop event and tries to load the event data.
    // this._createCategory(WebInspector.UIString("Drag"), true, ["drag", "drop", "dragstart", "dragend", "dragenter", "dragleave", "dragover"]);
    this._createCategory(WebInspector.UIString("Animation"), false, ["requestAnimationFrame", "cancelAnimationFrame", "animationFrameFired"]);
    this._createCategory(WebInspector.UIString("Control"), true, ["resize", "scroll", "zoom", "focus", "blur", "select", "change", "submit", "reset"]);
    this._createCategory(WebInspector.UIString("Clipboard"), true, ["copy", "cut", "paste", "beforecopy", "beforecut", "beforepaste"]);
    this._createCategory(WebInspector.UIString("DOM Mutation"), true, ["DOMActivate", "DOMFocusIn", "DOMFocusOut", "DOMAttrModified", "DOMCharacterDataModified", "DOMNodeInserted", "DOMNodeInsertedIntoDocument", "DOMNodeRemoved", "DOMNodeRemovedFromDocument", "DOMSubtreeModified", "DOMContentLoaded"]);
    this._createCategory(WebInspector.UIString("Device"), true, ["deviceorientation", "devicemotion"]);
    this._createCategory(WebInspector.UIString("Keyboard"), true, ["keydown", "keyup", "keypress", "input"]);
    this._createCategory(WebInspector.UIString("Load"), true, ["load", "unload", "abort", "error"]);
    this._createCategory(WebInspector.UIString("Mouse"), true, ["click", "dblclick", "mousedown", "mouseup", "mouseover", "mousemove", "mouseout", "mousewheel"]);
    this._createCategory(WebInspector.UIString("Timer"), false, ["setTimer", "clearTimer", "timerFired"]);
    this._createCategory(WebInspector.UIString("Touch"), true, ["touchstart", "touchmove", "touchend", "touchcancel"]);

    this._restoreBreakpoints();
}

WebInspector.EventListenerBreakpointsSidebarPane.categotyListener = "listener:";
WebInspector.EventListenerBreakpointsSidebarPane.categotyInstrumentation = "instrumentation:";

WebInspector.EventListenerBreakpointsSidebarPane.eventNameForUI = function(eventName)
{
    if (!WebInspector.EventListenerBreakpointsSidebarPane._eventNamesForUI) {
        WebInspector.EventListenerBreakpointsSidebarPane._eventNamesForUI = {
            "instrumentation:setTimer": WebInspector.UIString("Set Timer"),
            "instrumentation:clearTimer": WebInspector.UIString("Clear Timer"),
            "instrumentation:timerFired": WebInspector.UIString("Timer Fired"),
            "instrumentation:requestAnimationFrame": WebInspector.UIString("Request Animation Frame"),
            "instrumentation:cancelAnimationFrame": WebInspector.UIString("Cancel Animation Frame"),
            "instrumentation:animationFrameFired": WebInspector.UIString("Animation Frame Fired")
        };
    }
    return WebInspector.EventListenerBreakpointsSidebarPane._eventNamesForUI[eventName] || eventName.substring(eventName.indexOf(":") + 1);
}

WebInspector.EventListenerBreakpointsSidebarPane.prototype = {
    _createCategory: function(name, isDOMEvent, eventNames)
    {
        var categoryItem = {};
        categoryItem.element = new TreeElement(name);
        this.categoriesTreeOutline.appendChild(categoryItem.element);
        categoryItem.element.listItemElement.addStyleClass("event-category");
        categoryItem.element.selectable = true;

        categoryItem.checkbox = this._createCheckbox(categoryItem.element);
        categoryItem.checkbox.addEventListener("click", this._categoryCheckboxClicked.bind(this, categoryItem), true);

        categoryItem.children = {};
        for (var i = 0; i < eventNames.length; ++i) {
            var eventName = (isDOMEvent ? WebInspector.EventListenerBreakpointsSidebarPane.categotyListener :  WebInspector.EventListenerBreakpointsSidebarPane.categotyInstrumentation) + eventNames[i];

            var breakpointItem = {};
            var title = WebInspector.EventListenerBreakpointsSidebarPane.eventNameForUI(eventName);
            breakpointItem.element = new TreeElement(title);
            categoryItem.element.appendChild(breakpointItem.element);
            var hitMarker = document.createElement("div");
            hitMarker.className = "breakpoint-hit-marker";
            breakpointItem.element.listItemElement.appendChild(hitMarker);
            breakpointItem.element.listItemElement.addStyleClass("source-code");
            breakpointItem.element.selectable = true;

            breakpointItem.checkbox = this._createCheckbox(breakpointItem.element);
            breakpointItem.checkbox.addEventListener("click", this._breakpointCheckboxClicked.bind(this, eventName), true);
            breakpointItem.parent = categoryItem;

            this._breakpointItems[eventName] = breakpointItem;
            categoryItem.children[eventName] = breakpointItem;
        }
    },

    _createCheckbox: function(treeElement)
    {
        var checkbox = document.createElement("input");
        checkbox.className = "checkbox-elem";
        checkbox.type = "checkbox";
        treeElement.listItemElement.insertBefore(checkbox, treeElement.listItemElement.firstChild);
        return checkbox;
    },

    _categoryCheckboxClicked: function(categoryItem)
    {
        var checked = categoryItem.checkbox.checked;
        for (var eventName in categoryItem.children) {
            var breakpointItem = categoryItem.children[eventName];
            if (breakpointItem.checkbox.checked === checked)
                continue;
            if (checked)
                this._setBreakpoint(eventName);
            else
                this._removeBreakpoint(eventName);
        }
        this._saveBreakpoints();
    },

    _breakpointCheckboxClicked: function(eventName, event)
    {
        if (event.target.checked)
            this._setBreakpoint(eventName);
        else
            this._removeBreakpoint(eventName);
        this._saveBreakpoints();
    },

    _setBreakpoint: function(eventName)
    {
        var breakpointItem = this._breakpointItems[eventName];
        if (!breakpointItem)
            return;
        breakpointItem.checkbox.checked = true;
        if (eventName.startsWith(WebInspector.EventListenerBreakpointsSidebarPane.categotyListener))
            DOMDebuggerAgent.setEventListenerBreakpoint(eventName.substring(WebInspector.EventListenerBreakpointsSidebarPane.categotyListener.length));
        else if (eventName.startsWith(WebInspector.EventListenerBreakpointsSidebarPane.categotyInstrumentation))
            DOMDebuggerAgent.setInstrumentationBreakpoint(eventName.substring(WebInspector.EventListenerBreakpointsSidebarPane.categotyInstrumentation.length));
        this._updateCategoryCheckbox(breakpointItem.parent);
    },

    _removeBreakpoint: function(eventName)
    {
        var breakpointItem = this._breakpointItems[eventName];
        if (!breakpointItem)
            return;
        breakpointItem.checkbox.checked = false;
        if (eventName.startsWith(WebInspector.EventListenerBreakpointsSidebarPane.categotyListener))
            DOMDebuggerAgent.removeEventListenerBreakpoint(eventName.substring(WebInspector.EventListenerBreakpointsSidebarPane.categotyListener.length));
        else if (eventName.startsWith(WebInspector.EventListenerBreakpointsSidebarPane.categotyInstrumentation))
            DOMDebuggerAgent.removeInstrumentationBreakpoint(eventName.substring(WebInspector.EventListenerBreakpointsSidebarPane.categotyInstrumentation.length));
        this._updateCategoryCheckbox(breakpointItem.parent);
    },

    _updateCategoryCheckbox: function(categoryItem)
    {
        var hasEnabled = false, hasDisabled = false;
        for (var eventName in categoryItem.children) {
            var breakpointItem = categoryItem.children[eventName];
            if (breakpointItem.checkbox.checked)
                hasEnabled = true;
            else
                hasDisabled = true;
        }
        categoryItem.checkbox.checked = hasEnabled;
        categoryItem.checkbox.indeterminate = hasEnabled && hasDisabled;
    },

    highlightBreakpoint: function(eventName)
    {
        var breakpointItem = this._breakpointItems[eventName];
        if (!breakpointItem)
            return;
        this.expand();
        breakpointItem.parent.element.expand();
        breakpointItem.element.listItemElement.addStyleClass("breakpoint-hit");
        this._highlightedElement = breakpointItem.element.listItemElement;
    },

    clearBreakpointHighlight: function()
    {
        if (this._highlightedElement) {
            this._highlightedElement.removeStyleClass("breakpoint-hit");
            delete this._highlightedElement;
        }
    },

    _saveBreakpoints: function()
    {
        var breakpoints = [];
        for (var eventName in this._breakpointItems) {
            if (this._breakpointItems[eventName].checkbox.checked)
                breakpoints.push({ eventName: eventName });
        }
        WebInspector.settings.eventListenerBreakpoints.set(breakpoints);
    },

    _restoreBreakpoints: function()
    {
        var breakpoints = WebInspector.settings.eventListenerBreakpoints.get();
        for (var i = 0; i < breakpoints.length; ++i) {
            var breakpoint = breakpoints[i];
            if (breakpoint && typeof breakpoint.eventName === "string")
                this._setBreakpoint(breakpoint.eventName);
        }
    },

    __proto__: WebInspector.SidebarPane.prototype
}
