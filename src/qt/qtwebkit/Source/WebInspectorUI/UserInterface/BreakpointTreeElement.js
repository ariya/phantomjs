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

WebInspector.BreakpointTreeElement = function(breakpoint, className, title)
{
    console.assert(breakpoint instanceof WebInspector.Breakpoint);

    if (!className)
        className = WebInspector.BreakpointTreeElement.GenericLineIconStyleClassName;

    WebInspector.GeneralTreeElement.call(this, [WebInspector.BreakpointTreeElement.StyleClassName, className], title, null, breakpoint, false);

    this._breakpoint = breakpoint;

    if (!title)
        this._breakpoint.addEventListener(WebInspector.Breakpoint.Event.LocationDidChange, this._breakpointLocationDidChange, this);
    this._breakpoint.addEventListener(WebInspector.Breakpoint.Event.DisabledStateDidChange, this._updateStatus, this);
    this._breakpoint.addEventListener(WebInspector.Breakpoint.Event.ResolvedStateDidChange, this._updateStatus, this);

    this._statusImageElement = document.createElement("img");
    this._statusImageElement.className = WebInspector.BreakpointTreeElement.StatusImageElementStyleClassName;
    this._statusImageElement.addEventListener("mousedown", this._statusImageElementMouseDown.bind(this));
    this._statusImageElement.addEventListener("click", this._statusImageElementClicked.bind(this));

    if (!title)
        this._updateTitles();
    this._updateStatus();

    this.status = this._statusImageElement;
    this.small = true;
};

WebInspector.BreakpointTreeElement.GenericLineIconStyleClassName = "breakpoint-generic-line-icon";
WebInspector.BreakpointTreeElement.StyleClassName = "breakpoint";
WebInspector.BreakpointTreeElement.StatusImageElementStyleClassName = "status-image";
WebInspector.BreakpointTreeElement.StatusImageResolvedStyleClassName = "resolved";
WebInspector.BreakpointTreeElement.StatusImageDisabledStyleClassName = "disabled";
WebInspector.BreakpointTreeElement.FormattedLocationStyleClassName = "formatted-location";

WebInspector.BreakpointTreeElement.prototype = {
    constructor: WebInspector.BreakpointTreeElement,

    // Public

    get breakpoint()
    {
        return this._breakpoint;
    },

    ondelete: function()
    {
        if (!WebInspector.debuggerManager.isBreakpointRemovable(this._breakpoint))
            return false;

        WebInspector.debuggerManager.removeBreakpoint(this._breakpoint);
        return true;
    },

    onenter: function()
    {
        this._breakpoint.disabled = !this._breakpoint.disabled;
        return true;
    },

    onspace: function()
    {
        this._breakpoint.disabled = !this._breakpoint.disabled;
        return true;
    },

    oncontextmenu: function(event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);
        this._breakpoint.appendContextMenuItems(contextMenu, this._statusImageElement);
        contextMenu.show();
    },

    // Private

    _updateTitles: function()
    {
        var sourceCodeLocation = this._breakpoint.sourceCodeLocation;

        var displayLineNumber = sourceCodeLocation.displayLineNumber;
        var displayColumnNumber = sourceCodeLocation.displayColumnNumber;
        if (displayColumnNumber > 0)
            this.mainTitle = WebInspector.UIString("Line %d:%d").format(displayLineNumber + 1, displayColumnNumber + 1); // The user visible line and column numbers are 1-based.
        else
            this.mainTitle = WebInspector.UIString("Line %d").format(displayLineNumber + 1); // The user visible line number is 1-based.

        if (sourceCodeLocation.hasMappedLocation()) {
            this.subtitle = sourceCodeLocation.formattedLocationString();

            if (sourceCodeLocation.hasFormattedLocation())
                this.subtitleElement.classList.add(WebInspector.BreakpointTreeElement.FormattedLocationStyleClassName);
            else
                this.subtitleElement.classList.remove(WebInspector.BreakpointTreeElement.FormattedLocationStyleClassName);

            this.tooltip = this.mainTitle + " \u2014 " + WebInspector.UIString("originally %s").format(sourceCodeLocation.originalLocationString());
        }
    },

    _updateStatus: function()
    {
        if (this._breakpoint.disabled)
            this._statusImageElement.classList.add(WebInspector.BreakpointTreeElement.StatusImageDisabledStyleClassName);
        else
            this._statusImageElement.classList.remove(WebInspector.BreakpointTreeElement.StatusImageDisabledStyleClassName);

        if (this._breakpoint.resolved)
            this._statusImageElement.classList.add(WebInspector.BreakpointTreeElement.StatusImageResolvedStyleClassName);
        else
            this._statusImageElement.classList.remove(WebInspector.BreakpointTreeElement.StatusImageResolvedStyleClassName);
    },

    _breakpointLocationDidChange: function(event)
    {
        console.assert(event.target === this._breakpoint);

        // The Breakpoint has a new display SourceCode. The sidebar will remove us. Stop listening to the breakpoint.
        if (event.data.oldDisplaySourceCode === this._breakpoint.displaySourceCode) {
            this._breakpoint.addEventListener(WebInspector.Breakpoint.Event.LocationDidChange, this._breakpointLocationDidChange, this);
            this._breakpoint.addEventListener(WebInspector.Breakpoint.Event.DisabledStateDidChange, this._updateStatus, this);
            this._breakpoint.addEventListener(WebInspector.Breakpoint.Event.ResolvedStateDidChange, this._updateStatus, this);
            return;
        }

        this._updateTitles();
    },

    _statusImageElementMouseDown: function(event)
    {
        // To prevent the tree element from selecting.
        event.stopPropagation();
    },

    _statusImageElementClicked: function(event)
    {
        this._breakpoint.disabled = !this._breakpoint.disabled;
    }
};

WebInspector.BreakpointTreeElement.prototype.__proto__ = WebInspector.GeneralTreeElement.prototype;
