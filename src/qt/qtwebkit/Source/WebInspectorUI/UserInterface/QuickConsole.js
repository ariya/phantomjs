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

WebInspector.QuickConsole = function(element)
{
    WebInspector.Object.call(this);

    this._toggleOrFocusKeyboardShortcut = new WebInspector.KeyboardShortcut(null, WebInspector.KeyboardShortcut.Key.Escape, this._toggleOrFocus.bind(this));

    var mainFrameExecutionContext = new WebInspector.ExecutionContext(WebInspector.QuickConsole.MainFrameContextExecutionIdentifier, WebInspector.UIString("Main Frame"), true, null);
    this._mainFrameExecutionContextPathComponent = this._createExecutionContextPathComponent(mainFrameExecutionContext.name, mainFrameExecutionContext.identifier);
    this._selectedExecutionContextPathComponent = this._mainFrameExecutionContextPathComponent;

    this._otherExecutionContextPathComponents = [];
    this._frameIdentifierToExecutionContextPathComponentMap = {};

    this._element = element || document.createElement("div");
    this._element.classList.add(WebInspector.QuickConsole.StyleClassName);

    this.prompt = new WebInspector.ConsolePrompt(null, "text/javascript");
    this.prompt.element.classList.add(WebInspector.QuickConsole.TextPromptStyleClassName);
    this._element.appendChild(this.prompt.element);

    this.prompt.shown();

    this._navigationBar = new WebInspector.QuickConsoleNavigationBar;
    this._element.appendChild(this._navigationBar.element);

    this._executionContextSelectorItem = new WebInspector.HierarchicalPathNavigationItem;
    this._executionContextSelectorItem.showSelectorArrows = true;
    this._navigationBar.addNavigationItem(this._executionContextSelectorItem);

    this._executionContextSelectorDivider = new WebInspector.DividerNavigationItem;
    this._navigationBar.addNavigationItem(this._executionContextSelectorDivider);

    this._rebuildExecutionContextPathComponents();

    // COMPATIBILITY (iOS 6): Execution contexts did not exist, evaluation worked with frame ids.
    if (WebInspector.ExecutionContext.supported()) {
        WebInspector.Frame.addEventListener(WebInspector.Frame.Event.PageExecutionContextChanged, this._framePageExecutionContextsChanged, this);
        WebInspector.Frame.addEventListener(WebInspector.Frame.Event.ExecutionContextsCleared, this._frameExecutionContextsCleared, this);
    } else {
        WebInspector.frameResourceManager.addEventListener(WebInspector.FrameResourceManager.Event.FrameWasAdded, this._frameAdded, this);
        WebInspector.frameResourceManager.addEventListener(WebInspector.FrameResourceManager.Event.FrameWasRemoved, this._frameRemoved, this);
        WebInspector.Frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._frameMainResourceChanged, this);
    }

    WebInspector.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange, this._debuggerActiveCallFrameDidChange, this);
};

WebInspector.QuickConsole.StyleClassName = "quick-console";
WebInspector.QuickConsole.ShowingLogClassName = "showing-log";
WebInspector.QuickConsole.NavigationBarContainerStyleClassName = "navigation-bar-container";
WebInspector.QuickConsole.NavigationBarSpacerStyleClassName = "navigation-bar-spacer";
WebInspector.QuickConsole.TextPromptStyleClassName = "text-prompt";

WebInspector.QuickConsole.ToolbarSingleLineHeight = 21;
WebInspector.QuickConsole.ToolbarPromptPadding = 4;
WebInspector.QuickConsole.ToolbarTopBorder = 1;

WebInspector.QuickConsole.MainFrameContextExecutionIdentifier = undefined;

WebInspector.QuickConsole.Event = {
    DidResize: "quick-console-did-resize"
};

WebInspector.QuickConsole.prototype = {
    constructor: WebInspector.QuickConsole,

    // Public

    get element()
    {
        return this._element;
    },

    get navigationBar()
    {
        return this._navigationBar;
    },

    get executionContextIdentifier()
    {
        return this._selectedExecutionContextPathComponent._executionContextIdentifier;
    },

    updateLayout: function()
    {
        // A hard maximum size of 33% of the window.
        const maximumAllowedHeight = Math.round(window.innerHeight * 0.33);
        this.prompt.element.style.maxHeight = maximumAllowedHeight + "px";
    },

    consoleLogVisibilityChanged: function(visible)
    {
        if (visible)
            this.element.classList.add(WebInspector.QuickConsole.ShowingLogClassName);
        else
            this.element.classList.remove(WebInspector.QuickConsole.ShowingLogClassName);

        this.dispatchEventToListeners(WebInspector.QuickConsole.Event.DidResize);
    },

    // Private

    _executionContextPathComponentsToDisplay: function()
    {
        // If we are in the debugger the console will use the active call frame, don't show the selector.
        if (WebInspector.debuggerManager.activeCallFrame)
            return [];

        // If there is only the Main Frame, don't show the selector.
        if (!this._otherExecutionContextPathComponents.length)
            return [];

        return [this._selectedExecutionContextPathComponent];
    },

    _rebuildExecutionContextPathComponents: function()
    {
        var components = this._executionContextPathComponentsToDisplay();
        var isEmpty = !components.length;

        this._executionContextSelectorItem.components = components;

        this._executionContextSelectorItem.hidden = isEmpty;
        this._executionContextSelectorDivider.hidden = isEmpty;
    },

    _framePageExecutionContextsChanged: function(event)
    {
        var frame = event.target;

        var shouldAutomaticallySelect = this._restoreSelectedExecutionContextForFrame === frame;

        var newExecutionContextPathComponent = this._insertExecutionContextPathComponentForFrame(frame, shouldAutomaticallySelect);

        if (shouldAutomaticallySelect) {
            delete this._restoreSelectedExecutionContextForFrame;
            this._selectedExecutionContextPathComponent = newExecutionContextPathComponent;
            this._rebuildExecutionContextPathComponents();
        }
    },

    _frameExecutionContextsCleared: function(event)
    {
        var frame = event.target;

        // If this frame is navigating and it is selected in the UI we want to reselect its new item after navigation.
        if (event.data.committingProvisionalLoad && !this._restoreSelectedExecutionContextForFrame) {
            var executionContextPathComponent = this._frameIdentifierToExecutionContextPathComponentMap[frame.id];
            if (this._selectedExecutionContextPathComponent === executionContextPathComponent) {
                this._restoreSelectedExecutionContextForFrame = frame;
                // As a fail safe, if the frame never gets an execution context, clear the restore value.
                setTimeout(function() { delete this._restoreSelectedExecutionContextForFrame; }.bind(this), 10);
            }
        }

        this._removeExecutionContextPathComponentForFrame(frame);
    },

    _frameAdded: function(event)
    {
        var frame = event.data.frame;
        this._insertExecutionContextPathComponentForFrame(frame);
    },

    _frameRemoved: function(event)
    {
        var frame = event.data.frame;
        this._removeExecutionContextPathComponentForFrame(frame);
    },

    _frameMainResourceChanged: function(event)
    {
        var frame = event.target;
        this._updateExecutionContextPathComponentForFrame(frame);
    },

    _createExecutionContextPathComponent: function(name, identifier)
    {
        var pathComponent = new WebInspector.HierarchicalPathComponent(name, "execution-context", identifier, true, true);
        pathComponent.addEventListener(WebInspector.HierarchicalPathComponent.Event.SiblingWasSelected, this._pathComponentSelected, this);
        pathComponent.addEventListener(WebInspector.HierarchicalPathComponent.Event.Clicked, this._pathComponentClicked, this);
        pathComponent.truncatedDisplayNameLength = 50;
        pathComponent._executionContextIdentifier = identifier;
        return pathComponent;
    },

    _createExecutionContextPathComponentFromFrame: function(frame)
    {
        var name = frame.name ? frame.name + " \u2014 " + frame.mainResource.displayName : frame.mainResource.displayName;
        var identifier = WebInspector.ExecutionContext.supported() ? frame.pageExecutionContext.id : frame.id;

        var pathComponent = this._createExecutionContextPathComponent(name, identifier);
        pathComponent._frame = frame;

        return pathComponent;
    },

    _compareExecutionContextPathComponents: function(a, b)
    {
        // "Main Frame" always on top.
        if (!a._frame)
            return -1;
        if (!b._frame)
            return 1;

        // Frames with a name above frames without a name.
        if (a._frame.name && !b._frame.name)
            return -1;
        if (!a._frame.name && b._frame.name)
            return 1;

        return a.displayName.localeCompare(b.displayName);
    },

    _insertExecutionContextPathComponentForFrame: function(frame, skipRebuild)
    {
        if (frame.isMainFrame())
            return;

        console.assert(!this._frameIdentifierToExecutionContextPathComponentMap[frame.id]);
        if (this._frameIdentifierToExecutionContextPathComponentMap[frame.id])
            return;

        var executionContextPathComponent = this._createExecutionContextPathComponentFromFrame(frame);

        var index = insertionIndexForObjectInListSortedByFunction(executionContextPathComponent, this._otherExecutionContextPathComponents, this._compareExecutionContextPathComponents);

        var prev = index > 0 ? this._otherExecutionContextPathComponents[index - 1] : this._mainFrameExecutionContextPathComponent;
        var next = this._otherExecutionContextPathComponents[index] || null;
        if (prev) {
            prev.nextSibling = executionContextPathComponent;
            executionContextPathComponent.previousSibling = prev;
        }
        if (next) {
            next.previousSibling = executionContextPathComponent;
            executionContextPathComponent.nextSibling = next;
        }

        this._otherExecutionContextPathComponents.splice(index, 0, executionContextPathComponent);
        this._frameIdentifierToExecutionContextPathComponentMap[frame.id] = executionContextPathComponent;

        if (!skipRebuild)
            this._rebuildExecutionContextPathComponents();

        return executionContextPathComponent;
    },

    _removeExecutionContextPathComponentForFrame: function(frame, skipRebuild)
    {
        if (frame.isMainFrame())
            return;

        var executionContextPathComponent = this._frameIdentifierToExecutionContextPathComponentMap[frame.id];
        console.assert(executionContextPathComponent);
        if (!executionContextPathComponent)
            return;

        executionContextPathComponent.removeEventListener(WebInspector.HierarchicalPathComponent.Event.SiblingWasSelected, this._pathComponentSelected, this);
        executionContextPathComponent.removeEventListener(WebInspector.HierarchicalPathComponent.Event.Clicked, this._pathComponentClicked, this);

        var prev = executionContextPathComponent.previousSibling;
        var next = executionContextPathComponent.nextSibling;
        if (prev)
            prev.nextSibling = next;
        if (next)
            next.previousSibling = prev;

        if (this._selectedExecutionContextPathComponent === executionContextPathComponent)
            this._selectedExecutionContextPathComponent = this._mainFrameExecutionContextPathComponent;

        this._otherExecutionContextPathComponents.remove(executionContextPathComponent, true);
        delete this._frameIdentifierToExecutionContextPathComponentMap[frame.id];

        if (!skipRebuild)
            this._rebuildExecutionContextPathComponents();
    },

    _updateExecutionContextPathComponentForFrame: function(frame)
    {
        if (frame.isMainFrame())
            return;

        var executionContextPathComponent = this._frameIdentifierToExecutionContextPathComponentMap[frame.id];
        if (!executionContextPathComponent)
            return;

        var wasSelected = this._selectedExecutionContextPathComponent === executionContextPathComponent;

        this._removeExecutionContextPathComponentForFrame(frame, true);
        var newExecutionContextPathComponent = this._insertExecutionContextPathComponentForFrame(frame, true);

        if (wasSelected)
            this._selectedExecutionContextPathComponent = newExecutionContextPathComponent;

        this._rebuildExecutionContextPathComponents();
    },

    _pathComponentSelected: function(event)
    {
        if (event.data.pathComponent === this._selectedExecutionContextPathComponent)
            return;

        this._selectedExecutionContextPathComponent = event.data.pathComponent;

        this._rebuildExecutionContextPathComponents();
    },

    _pathComponentClicked: function(event)
    {
        this.prompt.focus();
    },

    _debuggerActiveCallFrameDidChange: function(event)
    {
        this._rebuildExecutionContextPathComponents();
    },

    _toggleOrFocus: function(event)
    {
        if (this.prompt.focused)
            WebInspector.toggleSplitConsole();
        else if (!WebInspector.isEditingAnyField() && !WebInspector.isEventTargetAnEditableField(event))
            this.prompt.focus();
    }
};

WebInspector.QuickConsole.prototype.__proto__ = WebInspector.Object.prototype;
