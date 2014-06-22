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

WebInspector.TextResourceContentView = function(resource)
{
    WebInspector.ResourceContentView.call(this, resource, WebInspector.TextResourceContentView.StyleClassName);

    resource.addEventListener(WebInspector.SourceCode.Event.ContentDidChange, this._sourceCodeContentDidChange, this);

    this._textEditor = new WebInspector.SourceCodeTextEditor(resource);
    this._textEditor.addEventListener(WebInspector.TextEditor.Event.ExecutionLineNumberDidChange, this._executionLineNumberDidChange, this);
    this._textEditor.addEventListener(WebInspector.TextEditor.Event.NumberOfSearchResultsDidChange, this._numberOfSearchResultsDidChange, this);
    this._textEditor.addEventListener(WebInspector.TextEditor.Event.ContentDidChange, this._textEditorContentDidChange, this);
    this._textEditor.addEventListener(WebInspector.TextEditor.Event.FormattingDidChange, this._textEditorFormattingDidChange, this);
    this._textEditor.addEventListener(WebInspector.SourceCodeTextEditor.Event.ContentWillPopulate, this._contentWillPopulate, this);
    this._textEditor.addEventListener(WebInspector.SourceCodeTextEditor.Event.ContentDidPopulate, this._contentDidPopulate, this);

    var toolTip = WebInspector.UIString("Pretty print");
    var activatedToolTip = WebInspector.UIString("Original formatting");
    this._prettyPrintButtonNavigationItem = new WebInspector.ActivateButtonNavigationItem("pretty-print", toolTip, activatedToolTip, "Images/NavigationItemCurleyBraces.pdf", 16, 16);
    this._prettyPrintButtonNavigationItem.addEventListener(WebInspector.ButtonNavigationItem.Event.Clicked, this._togglePrettyPrint, this);
    this._prettyPrintButtonNavigationItem.enabled = false; // Enabled when the text editor is populated with content.
};

WebInspector.TextResourceContentView.StyleClassName = "text";

WebInspector.TextResourceContentView.prototype = {
    constructor: WebInspector.TextResourceContentView,

    // Public

    get navigationItems()
    {
        return [this._prettyPrintButtonNavigationItem];
    },

    get managesOwnIssues()
    {
        // SourceCodeTextEditor manages the issues, we don't need ResourceContentView doing it.
        return true;
    },

    get textEditor()
    {
        return this._textEditor;
    },

    get supplementalRepresentedObjects()
    {
        if (isNaN(this._textEditor.executionLineNumber))
            return [];

        // If the SourceCodeTextEditor has an executionLineNumber, we can assume
        // it is always the active call frame.
        return [WebInspector.debuggerManager.activeCallFrame];
    },

    revealPosition: function(position, textRangeToSelect, forceUnformatted)
    {
        this._textEditor.revealPosition(position, textRangeToSelect, forceUnformatted);
    },

    shown: function()
    {
        WebInspector.ResourceContentView.prototype.shown.call(this);

        this._textEditor.shown();
    },

    hidden: function()
    {
        WebInspector.ResourceContentView.prototype.hidden.call(this);

        this._textEditor.hidden();
    },

    closed: function()
    {
        WebInspector.ResourceContentView.prototype.closed.call(this);

        this.resource.removeEventListener(WebInspector.SourceCode.Event.ContentDidChange, this._sourceCodeContentDidChange, this);

        this._textEditor.close();
    },

    get supportsSave()
    {
        return true;
    },

    get saveData()
    {
        return {url: this.resource.url, content: this._textEditor.string};
    },

    get supportsSearch()
    {
        return true;
    },

    get numberOfSearchResults()
    {
        return this._textEditor.numberOfSearchResults;
    },

    get hasPerformedSearch()
    {
        return this._textEditor.currentSearchQuery !== null;
    },

    set automaticallyRevealFirstSearchResult(reveal)
    {
        this._textEditor.automaticallyRevealFirstSearchResult = reveal;
    },

    performSearch: function(query)
    {
        this._textEditor.performSearch(query);
    },

    searchCleared: function()
    {
        this._textEditor.searchCleared();
    },

    searchQueryWithSelection: function()
    {
        return this._textEditor.searchQueryWithSelection();
    },

    revealPreviousSearchResult: function(changeFocus)
    {
        this._textEditor.revealPreviousSearchResult(changeFocus);
    },

    revealNextSearchResult: function(changeFocus)
    {
        this._textEditor.revealNextSearchResult(changeFocus);
    },

    updateLayout: function()
    {
        this._textEditor.updateLayout();
    },

    // Private

    _contentWillPopulate: function(event)
    {
        if (this._textEditor.element.parentNode === this.element)
            return;

        // Check the MIME-type for CSS since Resource.Type.Stylesheet also includes XSL, which we can't edit yet.
        if (this.resource.type === WebInspector.Resource.Type.Stylesheet && this.resource.syntheticMIMEType === "text/css")
            this._textEditor.readOnly = false;

        // Allow editing any local file since edits can be saved and reloaded right from the Inspector.
        if (this.resource.urlComponents.scheme === "file")
            this._textEditor.readOnly = false;

        this.element.removeChildren();
        this.element.appendChild(this._textEditor.element);
    },

    _contentDidPopulate: function(event)
    {
        this._prettyPrintButtonNavigationItem.enabled = this._textEditor.canBeFormatted();
    },

    _togglePrettyPrint: function(event)
    {
        var activated = !this._prettyPrintButtonNavigationItem.activated;
        this._textEditor.formatted = activated;
    },

    _textEditorFormattingDidChange: function(event)
    {
        this._prettyPrintButtonNavigationItem.activated = this._textEditor.formatted;
    },

    _sourceCodeContentDidChange: function(event)
    {
        if (this._ignoreSourceCodeContentDidChangeEvent)
            return;

        this._textEditor.string = this.resource.currentRevision.content;
    },

    _textEditorContentDidChange: function(event)
    {
        this._ignoreSourceCodeContentDidChangeEvent = true;
        WebInspector.branchManager.currentBranch.revisionForRepresentedObject(this.resource).content = this._textEditor.string;
        delete this._ignoreSourceCodeContentDidChangeEvent;
    },

    _executionLineNumberDidChange: function(event)
    {
        this.dispatchEventToListeners(WebInspector.ContentView.Event.SupplementalRepresentedObjectsDidChange);
    },

    _numberOfSearchResultsDidChange: function(event)
    {
        this.dispatchEventToListeners(WebInspector.ContentView.Event.NumberOfSearchResultsDidChange);
    }
};

WebInspector.TextResourceContentView.prototype.__proto__ = WebInspector.ResourceContentView.prototype;
