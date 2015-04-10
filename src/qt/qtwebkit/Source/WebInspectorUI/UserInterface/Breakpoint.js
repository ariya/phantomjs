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

WebInspector.Breakpoint = function(sourceCodeLocationOrInfo, disabled, condition)
{
    WebInspector.Object.call(this);

    if (sourceCodeLocationOrInfo instanceof WebInspector.SourceCodeLocation) {
        var sourceCode = sourceCodeLocationOrInfo.sourceCode;
        var url = sourceCode ? sourceCode.url : null;
        var scriptIdentifier = sourceCode instanceof WebInspector.Script ? sourceCode.id : null;
        var location = sourceCodeLocationOrInfo;
    } else if (sourceCodeLocationOrInfo && typeof sourceCodeLocationOrInfo === "object") {
        var url = sourceCodeLocationOrInfo.url;
        var lineNumber = sourceCodeLocationOrInfo.lineNumber || 0;
        var columnNumber = sourceCodeLocationOrInfo.columnNumber || 0;
        var location = new WebInspector.SourceCodeLocation(null, lineNumber, columnNumber);
        disabled = sourceCodeLocationOrInfo.disabled;
        condition = sourceCodeLocationOrInfo.condition;
    } else
        console.error("Unexpected type passed to WebInspector.Breakpoint", sourceCodeLocationOrInfo);

    this._id = null;
    this._url = url || null;
    this._scriptIdentifier = scriptIdentifier || null;
    this._disabled = disabled || false;
    this._condition = condition || "";
    this._resolved = false;

    this._sourceCodeLocation = location;
    this._sourceCodeLocation.addEventListener(WebInspector.SourceCodeLocation.Event.LocationChanged, this._sourceCodeLocationLocationChanged, this);
    this._sourceCodeLocation.addEventListener(WebInspector.SourceCodeLocation.Event.DisplayLocationChanged, this._sourceCodeLocationDisplayLocationChanged, this);
};

WebInspector.Object.addConstructorFunctions(WebInspector.Breakpoint);

WebInspector.Breakpoint.PopoverClassName = "edit-breakpoint-popover-content";
WebInspector.Breakpoint.PopoverConditionInputId = "edit-breakpoint-popover-condition";

WebInspector.Breakpoint.Event = {
    DisabledStateDidChange: "breakpoint-disabled-state-did-change",
    ResolvedStateDidChange: "breakpoint-resolved-state-did-change",
    ConditionDidChange: "breakpoint-condition-did-change",
    LocationDidChange: "breakpoint-location-did-change",
    DisplayLocationDidChange: "breakpoint-display-location-did-change",
};

WebInspector.Breakpoint.prototype = {
    constructor: WebInspector.Breakpoint,

    // Public

    get id()
    {
        return this._id;
    },

    set id(id)
    {
        this._id = id || null;
    },

    get url()
    {
        return this._url;
    },

    get scriptIdentifier()
    {
        return this._scriptIdentifier;
    },

    get sourceCodeLocation()
    {
        return this._sourceCodeLocation;
    },

    get resolved()
    {
        return this._resolved && WebInspector.debuggerManager.breakpointsEnabled;
    },

    set resolved(resolved)
    {
        if (this._resolved === resolved)
            return;

        this._resolved = resolved || false;

        this.dispatchEventToListeners(WebInspector.Breakpoint.Event.ResolvedStateDidChange);
    },

    get disabled()
    {
        return this._disabled;
    },

    set disabled(disabled)
    {
        if (this._disabled === disabled)
            return;

        this._disabled = disabled || false;

        this.dispatchEventToListeners(WebInspector.Breakpoint.Event.DisabledStateDidChange);
    },

    get condition()
    {
        return this._condition;
    },

    set condition(condition)
    {
        if (this._condition === condition)
            return;

        this._condition = condition;

        this.dispatchEventToListeners(WebInspector.Breakpoint.Event.ConditionDidChange);
    },

    get info()
    {
        // The id, scriptIdentifier and resolved state are tied to the current session, so don't include them for serialization.
        return {
            url: this._url,
            lineNumber: this._sourceCodeLocation.lineNumber,
            columnNumber: this._sourceCodeLocation.columnNumber,
            disabled: this._disabled,
            condition: this._condition
        };
    },

    appendContextMenuItems: function(contextMenu, breakpointDisplayElement)
    {
        function editBreakpoint()
        {
            this._showEditBreakpointPopover(breakpointDisplayElement);
        }

        function removeBreakpoint()
        {
            WebInspector.debuggerManager.removeBreakpoint(this);
        }

        function toggleBreakpoint()
        {
            this.disabled = !this.disabled;
        }

        function revealOriginalSourceCodeLocation()
        {
            WebInspector.resourceSidebarPanel.showOriginalOrFormattedSourceCodeLocation(this._sourceCodeLocation);
        }

        if (WebInspector.debuggerManager.isBreakpointEditable(this))
            contextMenu.appendItem(WebInspector.UIString("Edit Breakpoint…"), editBreakpoint.bind(this));

        if (this._disabled)
            contextMenu.appendItem(WebInspector.UIString("Enable Breakpoint"), toggleBreakpoint.bind(this));
        else
            contextMenu.appendItem(WebInspector.UIString("Disable Breakpoint"), toggleBreakpoint.bind(this));

        if (WebInspector.debuggerManager.isBreakpointRemovable(this)) {
            contextMenu.appendSeparator();
            contextMenu.appendItem(WebInspector.UIString("Delete Breakpoint"), removeBreakpoint.bind(this));
        }

        if (this._sourceCodeLocation.hasMappedLocation()) {
            contextMenu.appendSeparator();
            contextMenu.appendItem(WebInspector.UIString("Reveal in Original Resource"), revealOriginalSourceCodeLocation.bind(this));
        }
    },

    // Private

    _popoverToggleCheckboxChanged: function(event)
    {
        this.disabled = !event.target.checked;
    },

    _popoverConditionInputChanged: function(event)
    {
        this.condition = event.target.value;
    },

    _popoverConditionInputKeyDown: function(event)
    {
        if (this._keyboardShortcutEsc.matchesEvent(event) || this._keyboardShortcutEnter.matchesEvent(event)) {
            this._popover.dismiss();
            event.stopPropagation();
            event.preventDefault();
        }
    },

    _editBreakpointPopoverContentElement: function()
    {
        var content = document.createElement("div");
        content.className = WebInspector.Breakpoint.PopoverClassName;

        var checkboxElement = document.createElement("input");
        checkboxElement.type = "checkbox";
        checkboxElement.checked = !this._disabled;
        checkboxElement.addEventListener("change", this._popoverToggleCheckboxChanged.bind(this));

        var checkboxLabel = document.createElement("label");
        checkboxLabel.className = "toggle";
        checkboxLabel.appendChild(checkboxElement);
        checkboxLabel.appendChild(document.createTextNode(this._sourceCodeLocation.displayLocationString()));

        var table = document.createElement("table");

        var conditionRow = table.appendChild(document.createElement("tr"));
        var conditionHeader = conditionRow.appendChild(document.createElement("th"));
        var conditionData = conditionRow.appendChild(document.createElement("td"));
        var conditionLabel = conditionHeader.appendChild(document.createElement("label"));
        var conditionInput = conditionData.appendChild(document.createElement("input"));
        conditionInput.id = WebInspector.Breakpoint.PopoverConditionInputId;
        conditionInput.value = this._condition || "";
        conditionInput.spellcheck = false;
        conditionInput.addEventListener("change", this._popoverConditionInputChanged.bind(this));
        conditionInput.addEventListener("keydown", this._popoverConditionInputKeyDown.bind(this));
        conditionInput.placeholder = WebInspector.UIString("Conditional expression");
        conditionLabel.setAttribute("for", conditionInput.id);
        conditionLabel.textContent = WebInspector.UIString("Condition");

        content.appendChild(checkboxLabel);
        content.appendChild(table);

        return content;
    },

    _showEditBreakpointPopover: function(element)
    {
        const padding = 2;
        var bounds = WebInspector.Rect.rectFromClientRect(element.getBoundingClientRect());
        bounds.origin.x -= 1; // Move the anchor left one pixel so it looks more centered.
        bounds.origin.x -= padding;
        bounds.origin.y -= padding;
        bounds.size.width += padding * 2; 
        bounds.size.height += padding * 2; 

        this._popover = this._popover || new WebInspector.Popover(this);
        this._popover.content = this._editBreakpointPopoverContentElement();
        this._popover.present(bounds, [WebInspector.RectEdge.MAX_Y]);

        if (!this._keyboardShortcutEsc) {
            this._keyboardShortcutEsc = new WebInspector.KeyboardShortcut(null, WebInspector.KeyboardShortcut.Key.Escape);
            this._keyboardShortcutEnter = new WebInspector.KeyboardShortcut(null, WebInspector.KeyboardShortcut.Key.Enter);
        }

        document.getElementById(WebInspector.Breakpoint.PopoverConditionInputId).select();
    },

    _sourceCodeLocationLocationChanged: function(event)
    {
        this.dispatchEventToListeners(WebInspector.Breakpoint.Event.LocationDidChange, event.data);
    },

    _sourceCodeLocationDisplayLocationChanged: function(event)
    {
        this.dispatchEventToListeners(WebInspector.Breakpoint.Event.DisplayLocationDidChange, event.data);
    }
};

WebInspector.Breakpoint.prototype.__proto__ = WebInspector.Object.prototype;
