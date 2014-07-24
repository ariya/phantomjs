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

WebInspector.DebuggerSidebarPanel = function()
{
    WebInspector.NavigationSidebarPanel.call(this, "debugger", WebInspector.UIString("Debugger"), "Images/NavigationItemBug.pdf", "3", true);

    WebInspector.Frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainResourceChanged, this);
    WebInspector.Frame.addEventListener(WebInspector.Frame.Event.ResourceWasAdded, this._resourceAdded, this);

    WebInspector.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.CallFramesDidChange, this._debuggerCallFramesDidChange, this);
    WebInspector.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.BreakpointAdded, this._breakpointAdded, this);
    WebInspector.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.BreakpointRemoved, this._breakpointRemoved, this);
    WebInspector.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.ScriptAdded, this._scriptAdded, this);
    WebInspector.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.ScriptsCleared, this._scriptsCleared, this);
    WebInspector.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.Paused, this._debuggerDidPause, this);
    WebInspector.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.Resumed, this._debuggerDidResume, this);

    this._pauseOrResumeKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Control | WebInspector.KeyboardShortcut.Modifier.Command, "Y", this._debuggerPauseResumeButtonClicked.bind(this));
    this._stepOverKeyboardShortcut = new WebInspector.KeyboardShortcut(null, WebInspector.KeyboardShortcut.Key.F6, this._debuggerStepOverButtonClicked.bind(this));
    this._stepIntoKeyboardShortcut = new WebInspector.KeyboardShortcut(null, WebInspector.KeyboardShortcut.Key.F7, this._debuggerStepIntoButtonClicked.bind(this));
    this._stepOutKeyboardShortcut = new WebInspector.KeyboardShortcut(null, WebInspector.KeyboardShortcut.Key.F8, this._debuggerStepOutButtonClicked.bind(this));

    this._pauseOrResumeAlternateKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Command, WebInspector.KeyboardShortcut.Key.Slash, this._debuggerPauseResumeButtonClicked.bind(this));
    this._stepOverAlternateKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Command, WebInspector.KeyboardShortcut.Key.SingleQuote, this._debuggerStepOverButtonClicked.bind(this));
    this._stepIntoAlternateKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Command, WebInspector.KeyboardShortcut.Key.Semicolon, this._debuggerStepIntoButtonClicked.bind(this));
    this._stepOutAlternateKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Shift | WebInspector.KeyboardShortcut.Modifier.Command, WebInspector.KeyboardShortcut.Key.Semicolon, this._debuggerStepOutButtonClicked.bind(this));

    this._navigationBar = new WebInspector.NavigationBar;
    this.element.appendChild(this._navigationBar.element);

    var toolTip = WebInspector.UIString("Enable all breakpoints");
    var altToolTip = WebInspector.UIString("Disable all breakpoints");

    this._debuggerBreakpointsButtonItem = new WebInspector.ActivateButtonNavigationItem("debugger-breakpoints", toolTip, altToolTip, "Images/Breakpoints.pdf", 16, 16);
    this._debuggerBreakpointsButtonItem.activated = WebInspector.debuggerManager.breakpointsEnabled;
    this._debuggerBreakpointsButtonItem.addEventListener(WebInspector.ButtonNavigationItem.Event.Clicked, this._breakpointsToggleButtonClicked, this);
    this._navigationBar.addNavigationItem(this._debuggerBreakpointsButtonItem);

    toolTip = WebInspector.UIString("Pause script execution (%s or %s)").format(this._pauseOrResumeKeyboardShortcut.displayName, this._pauseOrResumeAlternateKeyboardShortcut.displayName);
    altToolTip = WebInspector.UIString("Continue script execution (%s or %s)").format(this._pauseOrResumeKeyboardShortcut.displayName, this._pauseOrResumeAlternateKeyboardShortcut.displayName);

    this._debuggerPauseResumeButtonItem = new WebInspector.ToggleButtonNavigationItem("debugger-pause-resume", toolTip, altToolTip, "Images/Pause.pdf", "Images/Resume.pdf", 16, 16);
    this._debuggerPauseResumeButtonItem.addEventListener(WebInspector.ButtonNavigationItem.Event.Clicked, this._debuggerPauseResumeButtonClicked, this);
    this._navigationBar.addNavigationItem(this._debuggerPauseResumeButtonItem);

    this._debuggerStepOverButtonItem = new WebInspector.ButtonNavigationItem("debugger-step-over", WebInspector.UIString("Step over (%s or %s)").format(this._stepOverKeyboardShortcut.displayName, this._stepOverAlternateKeyboardShortcut.displayName), "Images/StepOver.pdf", 16, 16);
    this._debuggerStepOverButtonItem.addEventListener(WebInspector.ButtonNavigationItem.Event.Clicked, this._debuggerStepOverButtonClicked, this);
    this._debuggerStepOverButtonItem.enabled = false;
    this._navigationBar.addNavigationItem(this._debuggerStepOverButtonItem);

    this._debuggerStepIntoButtonItem = new WebInspector.ButtonNavigationItem("debugger-step-into", WebInspector.UIString("Step into (%s or %s)").format(this._stepIntoKeyboardShortcut.displayName, this._stepIntoAlternateKeyboardShortcut.displayName), "Images/StepInto.pdf", 16, 16);
    this._debuggerStepIntoButtonItem.addEventListener(WebInspector.ButtonNavigationItem.Event.Clicked, this._debuggerStepIntoButtonClicked, this);
    this._debuggerStepIntoButtonItem.enabled = false;
    this._navigationBar.addNavigationItem(this._debuggerStepIntoButtonItem);

    this._debuggerStepOutButtonItem = new WebInspector.ButtonNavigationItem("debugger-step-out", WebInspector.UIString("Step out (%s or %s)").format(this._stepOutKeyboardShortcut.displayName, this._stepOutAlternateKeyboardShortcut.displayName), "Images/StepOut.pdf", 16, 16);
    this._debuggerStepOutButtonItem.addEventListener(WebInspector.ButtonNavigationItem.Event.Clicked, this._debuggerStepOutButtonClicked, this);
    this._debuggerStepOutButtonItem.enabled = false;
    this._navigationBar.addNavigationItem(this._debuggerStepOutButtonItem);

    // Add this offset-sections class name so the sticky headers don't overlap the navigation bar.
    this.element.classList.add(WebInspector.DebuggerSidebarPanel.OffsetSectionsStyleClassName);

    this._allExceptionsBreakpointTreeElement = new WebInspector.BreakpointTreeElement(WebInspector.debuggerManager.allExceptionsBreakpoint, WebInspector.DebuggerSidebarPanel.ExceptionIconStyleClassName, WebInspector.UIString("All Exceptions"));
    this._allUncaughtExceptionsBreakpointTreeElement = new WebInspector.BreakpointTreeElement(WebInspector.debuggerManager.allUncaughtExceptionsBreakpoint, WebInspector.DebuggerSidebarPanel.ExceptionIconStyleClassName, WebInspector.UIString("All Uncaught Exceptions"));

    this.filterBar.placeholder = WebInspector.UIString("Filter Breakpoint List");

    this._breakpointsContentTreeOutline = this.contentTreeOutline;
    this._breakpointsContentTreeOutline.onselect = this._treeElementSelected.bind(this);
    this._breakpointsContentTreeOutline.ondelete = this._breakpointTreeOutlineDeleteTreeElement.bind(this);
    this._breakpointsContentTreeOutline.oncontextmenu = this._breakpointTreeOutlineContextMenuTreeElement.bind(this);

    this._breakpointsContentTreeOutline.appendChild(this._allExceptionsBreakpointTreeElement);
    this._breakpointsContentTreeOutline.appendChild(this._allUncaughtExceptionsBreakpointTreeElement);

    var breakpointsRow = new WebInspector.DetailsSectionRow;
    breakpointsRow.element.appendChild(this._breakpointsContentTreeOutline.element);

    var breakpointsGroup = new WebInspector.DetailsSectionGroup([breakpointsRow]);
    var breakpointsSection = new WebInspector.DetailsSection("breakpoints", WebInspector.UIString("Breakpoints"), [breakpointsGroup]);
    this.contentElement.appendChild(breakpointsSection.element);

    this._callStackContentTreeOutline = this.createContentTreeOutline(true);
    this._callStackContentTreeOutline.onselect = this._treeElementSelected.bind(this);

    this._callStackRow = new WebInspector.DetailsSectionRow(WebInspector.UIString("No Call Frames"));
    this._callStackRow.showEmptyMessage();

    var callStackGroup = new WebInspector.DetailsSectionGroup([this._callStackRow]);
    this._callStackSection = new WebInspector.DetailsSection("call-stack", WebInspector.UIString("Call Stack"), [callStackGroup]);

    WebInspector.Breakpoint.addEventListener(WebInspector.Breakpoint.Event.DisplayLocationDidChange, this._breakpointDisplayLocationDidChange, this);
};

WebInspector.DebuggerSidebarPanel.OffsetSectionsStyleClassName = "offset-sections";
WebInspector.DebuggerSidebarPanel.ExceptionIconStyleClassName = "breakpoint-exception-icon";

WebInspector.DebuggerSidebarPanel.prototype = {
    constructor: WebInspector.DebuggerSidebarPanel,

    // Private

    _debuggerPauseResumeButtonClicked: function(event)
    {
        if (WebInspector.debuggerManager.paused)
            WebInspector.debuggerManager.resume();
        else {
            this._debuggerPauseResumeButtonItem.enabled = false;
            WebInspector.debuggerManager.pause();
        }
    },

    _debuggerStepOverButtonClicked: function(event)
    {
        WebInspector.debuggerManager.stepOver();
    },

    _debuggerStepIntoButtonClicked: function(event)
    {
        WebInspector.debuggerManager.stepInto();
    },

    _debuggerStepOutButtonClicked: function(event)
    {
        WebInspector.debuggerManager.stepOut();
    },

    _debuggerDidPause: function(event)
    {
        this.contentElement.insertBefore(this._callStackSection.element, this.contentElement.firstChild);

        this._debuggerPauseResumeButtonItem.enabled = true;
        this._debuggerPauseResumeButtonItem.toggled = true;
        this._debuggerStepOverButtonItem.enabled = true;
        this._debuggerStepIntoButtonItem.enabled = true;
        this._debuggerStepOutButtonItem.enabled = true;
    },

    _debuggerDidResume: function(event)
    {
        this._callStackSection.element.remove();

        this._debuggerPauseResumeButtonItem.enabled = true;
        this._debuggerPauseResumeButtonItem.toggled = false;
        this._debuggerStepOverButtonItem.enabled = false;
        this._debuggerStepIntoButtonItem.enabled = false;
        this._debuggerStepOutButtonItem.enabled = false;
    },

    _breakpointsToggleButtonClicked: function(event)
    {
        this._debuggerBreakpointsButtonItem.activated = !this._debuggerBreakpointsButtonItem.activated;
        WebInspector.debuggerManager.breakpointsEnabled = this._debuggerBreakpointsButtonItem.activated;
    },

    _addBreakpoint: function(breakpoint, sourceCode)
    {
        var sourceCode = breakpoint.sourceCodeLocation.displaySourceCode;
        if (!sourceCode)
            return null;

        var parentTreeElement = this._breakpointsContentTreeOutline.getCachedTreeElement(sourceCode);
        if (!parentTreeElement) {
            if (sourceCode instanceof WebInspector.SourceMapResource)
                parentTreeElement = new WebInspector.SourceMapResourceTreeElement(sourceCode);
            else if (sourceCode instanceof WebInspector.Resource)
                parentTreeElement = new WebInspector.ResourceTreeElement(sourceCode);
            else if (sourceCode instanceof WebInspector.Script)
                parentTreeElement = new WebInspector.ScriptTreeElement(sourceCode);
        }

        if (!parentTreeElement.parent) {
            parentTreeElement.hasChildren = true;
            parentTreeElement.expand();

            this._breakpointsContentTreeOutline.insertChild(parentTreeElement, insertionIndexForObjectInListSortedByFunction(parentTreeElement, this._breakpointsContentTreeOutline.children, this._compareTopLevelTreeElements.bind(this)));
        }

        // Mark disabled breakpoints as resolved if there is source code loaded with that URL.
        // This gives the illusion the breakpoint was resolved, but since we don't send disabled
        // breakpoints to the backend we don't know for sure. If the user enables the breakpoint
        // it will be resolved properly.
        if (breakpoint.disabled)
            breakpoint.resolved = true;

        var breakpointTreeElement = new WebInspector.BreakpointTreeElement(breakpoint);
        parentTreeElement.insertChild(breakpointTreeElement, insertionIndexForObjectInListSortedByFunction(breakpointTreeElement, parentTreeElement.children, this._compareBreakpointTreeElements));
        return breakpointTreeElement;
    },

    _addBreakpointsForSourceCode: function(sourceCode)
    {
        var breakpoints = WebInspector.debuggerManager.breakpointsForSourceCode(sourceCode);
        for (var i = 0; i < breakpoints.length; ++i)
            this._addBreakpoint(breakpoints[i], sourceCode);
    },

    _resourceAdded: function(event)
    {
        var resource = event.data.resource;
        this._addBreakpointsForSourceCode(resource);
    },

    _mainResourceChanged: function(event)
    {
        var resource = event.target.mainResource;
        this._addBreakpointsForSourceCode(resource);
    },

    _scriptAdded: function(event)
    {
        var script = event.data.script;

        // Don't add breakpoints if the script is represented by a Resource. They were
        // already added by _resourceAdded.
        if (script.resource)
            return;

        this._addBreakpointsForSourceCode(script);
    },

    _scriptsCleared: function(event)
    {
        for (var i = this._breakpointsContentTreeOutline.children.length - 1; i >= 0; --i) {
            var treeElement = this._breakpointsContentTreeOutline.children[i];
            if (!(treeElement instanceof WebInspector.ScriptTreeElement))
                continue;

            this._breakpointsContentTreeOutline.removeChildAtIndex(i, true, true);
        }
    },

    _breakpointAdded: function(event)
    {
        var breakpoint = event.data.breakpoint;
        this._addBreakpoint(breakpoint);
    },

    _breakpointRemoved: function(event)
    {
        var breakpoint = event.data.breakpoint;

        var breakpointTreeElement = this._breakpointsContentTreeOutline.getCachedTreeElement(breakpoint);
        console.assert(breakpointTreeElement);
        if (!breakpointTreeElement)
            return;

        this._removeBreakpointTreeElement(breakpointTreeElement);
    },

    _breakpointDisplayLocationDidChange: function(event)
    {
        var breakpoint = event.target;
        if (event.data.oldDisplaySourceCode === breakpoint.displaySourceCode)
            return;

        var breakpointTreeElement = this._breakpointsContentTreeOutline.getCachedTreeElement(breakpoint);
        if (!breakpointTreeElement)
            return;

        // A known breakpoint moved between resources, remove the old tree element
        // and create a new tree element with the updated file.

        var wasSelected = breakpointTreeElement.selected;

        this._removeBreakpointTreeElement(breakpointTreeElement);
        var newBreakpointTreeElement = this._addBreakpoint(breakpoint);

        if (newBreakpointTreeElement && wasSelected)
            newBreakpointTreeElement.revealAndSelect(true, false, true, true);
    },

    _removeBreakpointTreeElement: function(breakpointTreeElement)
    {
        var parentTreeElement = breakpointTreeElement.parent;
        parentTreeElement.removeChild(breakpointTreeElement);

        console.assert(parentTreeElement.parent === this._breakpointsContentTreeOutline);

        if (!parentTreeElement.children.length)
            this._breakpointsContentTreeOutline.removeChild(parentTreeElement);
    },

    _debuggerCallFramesDidChange: function()
    {
        this._callStackContentTreeOutline.removeChildren();

        var callFrames = WebInspector.debuggerManager.callFrames;
        if (!callFrames || !callFrames.length) {
            this._callStackRow.showEmptyMessage();
            return;
        }

        this._callStackRow.hideEmptyMessage();
        this._callStackRow.element.appendChild(this._callStackContentTreeOutline.element);

        var treeElementToSelect = null;

        var activeCallFrame = WebInspector.debuggerManager.activeCallFrame;
        for (var i = 0; i < callFrames.length; ++i) {
            var callFrameTreeElement = new WebInspector.CallFrameTreeElement(callFrames[i]);
            if (callFrames[i] === activeCallFrame)
                treeElementToSelect = callFrameTreeElement;
            this._callStackContentTreeOutline.appendChild(callFrameTreeElement);
        }

        if (treeElementToSelect)
            treeElementToSelect.select(true, true);
    },

    _breakpointsBeneathTreeElement: function(treeElement)
    {
        console.assert(treeElement instanceof WebInspector.ResourceTreeElement || treeElement instanceof WebInspector.ScriptTreeElement);
        if (!(treeElement instanceof WebInspector.ResourceTreeElement) && !(treeElement instanceof WebInspector.ScriptTreeElement))
            return [];

        var breakpoints = [];
        var breakpointTreeElements = treeElement.children;
        for (var i = 0; i < breakpointTreeElements.length; ++i) {
            console.assert(breakpointTreeElements[i] instanceof WebInspector.BreakpointTreeElement);
            console.assert(breakpointTreeElements[i].breakpoint);
            var breakpoint = breakpointTreeElements[i].breakpoint;
            if (breakpoint)
                breakpoints.push(breakpoint);
        }

        return breakpoints;
    },

    _removeAllBreakpoints: function(breakpoints)
    {
        for (var i = 0; i < breakpoints.length; ++i) {
            var breakpoint = breakpoints[i];
            if (WebInspector.debuggerManager.isBreakpointRemovable(breakpoint))
                WebInspector.debuggerManager.removeBreakpoint(breakpoint);
        }
    },

    _toggleAllBreakpoints: function(breakpoints, disabled)
    {
        for (var i = 0; i < breakpoints.length; ++i)
            breakpoints[i].disabled = disabled;
    },

    _breakpointTreeOutlineDeleteTreeElement: function(treeElement)
    {
        console.assert(treeElement.selected);
        console.assert(treeElement instanceof WebInspector.ResourceTreeElement || treeElement instanceof WebInspector.ScriptTreeElement);
        if (!(treeElement instanceof WebInspector.ResourceTreeElement) && !(treeElement instanceof WebInspector.ScriptTreeElement))
            return false;

        var wasTopResourceTreeElement = treeElement.previousSibling === this._allUncaughtExceptionsBreakpointTreeElement;
        var nextSibling = treeElement.nextSibling;

        var breakpoints = this._breakpointsBeneathTreeElement(treeElement);
        this._removeAllBreakpoints(breakpoints);

        if (wasTopResourceTreeElement && nextSibling)
            nextSibling.select(true, true);

        return true;
    },

    _breakpointTreeOutlineContextMenuTreeElement: function(event, treeElement)
    {
        console.assert(treeElement instanceof WebInspector.ResourceTreeElement || treeElement instanceof WebInspector.ScriptTreeElement);
        if (!(treeElement instanceof WebInspector.ResourceTreeElement) && !(treeElement instanceof WebInspector.ScriptTreeElement))
            return;

        var breakpoints = this._breakpointsBeneathTreeElement(treeElement);
        var shouldDisable = false;
        for (var i = 0; i < breakpoints.length; ++i) {
            if (!breakpoints[i].disabled) {
                shouldDisable = true;
                break;
            }
        }

        function removeAllResourceBreakpoints()
        {
            this._removeAllBreakpoints(breakpoints);
        }

        function toggleAllResourceBreakpoints()
        {
            this._toggleAllBreakpoints(breakpoints, shouldDisable);
        }

        var contextMenu = new WebInspector.ContextMenu(event);
        if (shouldDisable)
            contextMenu.appendItem(WebInspector.UIString("Disable Breakpoints"), toggleAllResourceBreakpoints.bind(this));
        else
            contextMenu.appendItem(WebInspector.UIString("Enable Breakpoints"), toggleAllResourceBreakpoints.bind(this));
        contextMenu.appendItem(WebInspector.UIString("Delete Breakpoints"), removeAllResourceBreakpoints.bind(this));
        contextMenu.show();
    },

    _treeElementSelected: function(treeElement, selectedByUser)
    {
        function deselectCallStackContentTreeElements()
        {
            // Deselect any tree element in the call stack content tree outline to prevent two selections in the sidebar.
            var selectedTreeElement = this._callStackContentTreeOutline.selectedTreeElement;
            if (selectedTreeElement)
                selectedTreeElement.deselect();
        }

        if (treeElement instanceof WebInspector.ResourceTreeElement || treeElement instanceof WebInspector.ScriptTreeElement) {
            // If the resource is being selected when it has no children it is in the process of being deleted, don't do anything.
            if (!treeElement.children.length)
                return;
            deselectCallStackContentTreeElements.call(this);
            WebInspector.resourceSidebarPanel.showSourceCode(treeElement.representedObject);
            return;
        }

        if (treeElement instanceof WebInspector.CallFrameTreeElement) {
            // Deselect any tree element in the breakpoints content tree outline to prevent two selections in the sidebar.
            var selectedTreeElement = this._breakpointsContentTreeOutline.selectedTreeElement;
            if (selectedTreeElement)
                selectedTreeElement.deselect();

            var callFrame = treeElement.callFrame;
            WebInspector.debuggerManager.activeCallFrame = callFrame;
            WebInspector.resourceSidebarPanel.showSourceCodeLocation(callFrame.sourceCodeLocation);
            return;
        }

        if (!(treeElement instanceof WebInspector.BreakpointTreeElement))
            return;

        deselectCallStackContentTreeElements.call(this);

        if (!treeElement.parent.representedObject)
            return;

        console.assert(treeElement.parent.representedObject instanceof WebInspector.SourceCode);
        if (!(treeElement.parent.representedObject instanceof WebInspector.SourceCode))
            return;

        var breakpoint = treeElement.breakpoint;
        WebInspector.resourceSidebarPanel.showSourceCodeLocation(breakpoint.sourceCodeLocation);
    },

    _compareTopLevelTreeElements: function(a, b)
    {
        if (a === this._allExceptionsBreakpointTreeElement)
            return -1;
        if (b === this._allExceptionsBreakpointTreeElement)
            return 1;

        if (a === this._allUncaughtExceptionsBreakpointTreeElement)
            return -1;
        if (b === this._allUncaughtExceptionsBreakpointTreeElement)
            return 1;

        return a.mainTitle.localeCompare(b.mainTitle);
    },

    _compareBreakpointTreeElements: function(a, b)
    {
        var aLocation = a.breakpoint.sourceCodeLocation;
        var bLocation = b.breakpoint.sourceCodeLocation;

        var comparisonResult = aLocation.displayLineNumber - bLocation.displayLineNumber
        if (comparisonResult !== 0)
            return comparisonResult;

        return aLocation.displayColumnNumber - bLocation.displayColumnNumber;
    }
};

WebInspector.DebuggerSidebarPanel.prototype.__proto__ = WebInspector.NavigationSidebarPanel.prototype;
