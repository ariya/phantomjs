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

WebInspector.SourceCodeTextEditor = function(sourceCode)
{
    console.assert(sourceCode instanceof WebInspector.SourceCode);

    this._sourceCode = sourceCode;
    this._breakpointMap = {};
    this._issuesLineNumberMap = {};
    this._contentPopulated = false;
    this._invalidLineNumbers = {0: true};

    WebInspector.TextEditor.call(this, null, null, this);

    // FIXME: Currently this just jumps between resources and related source map resources. It doesn't "jump to symbol" yet.
    this._jumpToSymbolTrackingModeEnabled = false;
    this._disableJumpToSymbolTrackingModeSettings();

    this.element.classList.add(WebInspector.SourceCodeTextEditor.StyleClassName);

    if (this._supportsDebugging) {
        WebInspector.Breakpoint.addEventListener(WebInspector.Breakpoint.Event.DisabledStateDidChange, this._updateBreakpointStatus, this);
        WebInspector.Breakpoint.addEventListener(WebInspector.Breakpoint.Event.ResolvedStateDidChange, this._updateBreakpointStatus, this);
        WebInspector.Breakpoint.addEventListener(WebInspector.Breakpoint.Event.LocationDidChange, this._updateBreakpointLocation, this);

        WebInspector.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.BreakpointAdded, this._breakpointAdded, this);
        WebInspector.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.BreakpointRemoved, this._breakpointRemoved, this);
        WebInspector.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange, this._activeCallFrameDidChange, this);

        WebInspector.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.Paused, this._debuggerDidPause, this);
        WebInspector.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.Resumed, this._debuggerDidResume, this);
        if (WebInspector.debuggerManager.activeCallFrame)
            this._debuggerDidPause();

        this._activeCallFrameDidChange();
    }

    WebInspector.issueManager.addEventListener(WebInspector.IssueManager.Event.IssueWasAdded, this._issueWasAdded, this);

    if (this._sourceCode instanceof WebInspector.SourceMapResource || this._sourceCode.sourceMaps.length > 0)
        WebInspector.notifications.addEventListener(WebInspector.Notification.GlobalModifierKeysDidChange, this._updateJumpToSymbolTrackingMode, this);
    else
        this._sourceCode.addEventListener(WebInspector.SourceCode.Event.SourceMapAdded, this._sourceCodeSourceMapAdded, this);

    sourceCode.requestContent(this._contentAvailable.bind(this));
};

WebInspector.Object.addConstructorFunctions(WebInspector.SourceCodeTextEditor);

WebInspector.SourceCodeTextEditor.StyleClassName = "source-code";
WebInspector.SourceCodeTextEditor.LineErrorStyleClassName = "error";
WebInspector.SourceCodeTextEditor.LineWarningStyleClassName = "warning";
WebInspector.SourceCodeTextEditor.PopoverDebuggerContentStyleClassName = "debugger-popover-content";
WebInspector.SourceCodeTextEditor.HoveredExpressionHighlightStyleClassName = "hovered-expression-highlight";
WebInspector.SourceCodeTextEditor.DurationToMouseOverTokenToMakeHoveredToken = 500;
WebInspector.SourceCodeTextEditor.DurationToMouseOutOfHoveredTokenToRelease = 1000;

WebInspector.SourceCodeTextEditor.AutoFormatMinimumLineLength = 500;

WebInspector.SourceCodeTextEditor.Event = {
    ContentWillPopulate: "source-code-text-editor-content-will-populate",
    ContentDidPopulate: "source-code-text-editor-content-did-populate"
};

WebInspector.SourceCodeTextEditor.prototype = {
    constructor: WebInspector.SourceCodeTextEditor,

    // Public

    get sourceCode()
    {
        return this._sourceCode;
    },

    hidden: function()
    {
        WebInspector.TextEditor.prototype.hidden.call(this);

        this.tokenTrackingController.removeHighlightedRange();

        this._dismissPopover();
    },

    close: function()
    {
        if (this._supportsDebugging) {
            WebInspector.Breakpoint.removeEventListener(WebInspector.Breakpoint.Event.DisabledStateDidChange, this._updateBreakpointStatus, this);
            WebInspector.Breakpoint.removeEventListener(WebInspector.Breakpoint.Event.ResolvedStateDidChange, this._updateBreakpointStatus, this);
            WebInspector.Breakpoint.removeEventListener(WebInspector.Breakpoint.Event.LocationDidChange, this._updateBreakpointLocation, this);

            WebInspector.debuggerManager.removeEventListener(WebInspector.DebuggerManager.Event.BreakpointAdded, this._breakpointAdded, this);
            WebInspector.debuggerManager.removeEventListener(WebInspector.DebuggerManager.Event.BreakpointRemoved, this._breakpointRemoved, this);
            WebInspector.debuggerManager.removeEventListener(WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange, this._activeCallFrameDidChange, this);

            if (this._activeCallFrameSourceCodeLocation) {
                this._activeCallFrameSourceCodeLocation.removeEventListener(WebInspector.SourceCodeLocation.Event.LocationChanged, this._activeCallFrameSourceCodeLocationChanged, this);
                delete this._activeCallFrameSourceCodeLocation;
            }
        }

        WebInspector.issueManager.removeEventListener(WebInspector.IssueManager.Event.IssueWasAdded, this._issueWasAdded, this);

        WebInspector.notifications.removeEventListener(WebInspector.Notification.GlobalModifierKeysDidChange, this._updateJumpToSymbolTrackingMode, this);
        this._sourceCode.removeEventListener(WebInspector.SourceCode.Event.SourceMapAdded, this._sourceCodeSourceMapAdded, this);
    },

    canBeFormatted: function()
    {
        // Currently we assume that source map resources are formatted how the author wants it.
        // We could allow source map resources to be formatted, we would then need to make
        // SourceCodeLocation watch listen for mappedResource's formatting changes, and keep
        // a formatted location alongside the regular mapped location.
        if (this._sourceCode instanceof WebInspector.SourceMapResource)
            return false;

        return WebInspector.TextEditor.prototype.canBeFormatted.call(this);
    },

    customPerformSearch: function(query)
    {
        function searchResultCallback(error, matches)
        {
            // Bail if the query changed since we started.
            if (this.currentSearchQuery !== query)
                return;

            if (error || !matches || !matches.length) {
                // Report zero matches.
                this.dispatchEventToListeners(WebInspector.TextEditor.Event.NumberOfSearchResultsDidChange);
                return;
            }

            var queryRegex = new RegExp(query.escapeForRegExp(), "gi");
            var searchResults = [];

            for (var i = 0; i < matches.length; ++i) {
                var matchLineNumber = matches[i].lineNumber;
                var line = this.line(matchLineNumber);

                // Reset the last index to reuse the regex on a new line.
                queryRegex.lastIndex = 0;

                // Search the line and mark the ranges.
                var lineMatch = null;
                while (queryRegex.lastIndex + query.length <= line.length && (lineMatch = queryRegex.exec(line))) {
                    var resultTextRange = new WebInspector.TextRange(matchLineNumber, lineMatch.index, matchLineNumber, queryRegex.lastIndex);
                    searchResults.push(resultTextRange);
                }
            }

            this.addSearchResults(searchResults);

            this.dispatchEventToListeners(WebInspector.TextEditor.Event.NumberOfSearchResultsDidChange);
        }

        if (this._sourceCode instanceof WebInspector.SourceMapResource)
            return false;

        if (this._sourceCode instanceof WebInspector.Resource)
            PageAgent.searchInResource(this._sourceCode.parentFrame.id, this._sourceCode.url, query, false, false, searchResultCallback.bind(this));
        else if (this._sourceCode instanceof WebInspector.Script)
            DebuggerAgent.searchInContent(this._sourceCode.id, query, false, false, searchResultCallback.bind(this));
        return true;
    },

    // Private

    _unformattedLineInfoForEditorLineInfo: function(lineInfo)
    {
        if (this.formatterSourceMap)
            return this.formatterSourceMap.formattedToOriginal(lineInfo.lineNumber, lineInfo.columnNumber);
        return lineInfo;
    },

    _sourceCodeLocationForEditorPosition: function(position)
    {
        var lineInfo = {lineNumber: position.line, columnNumber: position.ch};
        var unformattedLineInfo = this._unformattedLineInfoForEditorLineInfo(lineInfo);
        return this.sourceCode.createSourceCodeLocation(unformattedLineInfo.lineNumber, unformattedLineInfo.columnNumber);
    },

    _editorLineInfoForSourceCodeLocation: function(sourceCodeLocation)
    {
        if (this._sourceCode instanceof WebInspector.SourceMapResource)
            return {lineNumber: sourceCodeLocation.displayLineNumber, columnNumber: sourceCodeLocation.displayColumnNumber};
        return {lineNumber: sourceCodeLocation.formattedLineNumber, columnNumber: sourceCodeLocation.formattedColumnNumber};
    },

    _breakpointForEditorLineInfo: function(lineInfo)
    {
        if (!this._breakpointMap[lineInfo.lineNumber])
            return null;
        return this._breakpointMap[lineInfo.lineNumber][lineInfo.columnNumber];
    },

    _addBreakpointWithEditorLineInfo: function(breakpoint, lineInfo)
    {
        if (!this._breakpointMap[lineInfo.lineNumber])
            this._breakpointMap[lineInfo.lineNumber] = {}

        this._breakpointMap[lineInfo.lineNumber][lineInfo.columnNumber] = breakpoint;
    },

    _removeBreakpointWithEditorLineInfo: function(breakpoint, lineInfo)
    {
        console.assert(breakpoint === this._breakpointMap[lineInfo.lineNumber][lineInfo.columnNumber]);

        delete this._breakpointMap[lineInfo.lineNumber][lineInfo.columnNumber];

        if (isEmptyObject(this._breakpointMap[lineInfo.lineNumber]))
            delete this._breakpointMap[lineInfo.lineNumber];
    },

    _contentWillPopulate: function(content)
    {
        this.dispatchEventToListeners(WebInspector.SourceCodeTextEditor.Event.ContentWillPopulate);

        // We only do the rest of this work before the first populate.
        if (this._contentPopulated)
            return;

        if (this._supportsDebugging) {
            this._breakpointMap = {};

            var breakpoints = WebInspector.debuggerManager.breakpointsForSourceCode(this._sourceCode);
            for (var i = 0; i < breakpoints.length; ++i) {
                var breakpoint = breakpoints[i];
                console.assert(this._matchesBreakpoint(breakpoint));
                var lineInfo = this._editorLineInfoForSourceCodeLocation(breakpoint.sourceCodeLocation);
                this._addBreakpointWithEditorLineInfo(breakpoint, lineInfo);
                this.setBreakpointInfoForLineAndColumn(lineInfo.lineNumber, lineInfo.columnNumber, this._breakpointInfoForBreakpoint(breakpoint));
            }
        }

        if (this._sourceCode instanceof WebInspector.Resource)
            this.mimeType = this._sourceCode.syntheticMIMEType;
        else if (this._sourceCode instanceof WebInspector.Script)
            this.mimeType = "text/javascript";

        // Automatically format the content if it looks minified and it can be formatted.
        console.assert(!this.formatted);
        if (this.canBeFormatted()) {
            var lastNewlineIndex = 0;
            while (true) {
                var nextNewlineIndex = content.indexOf("\n", lastNewlineIndex);
                if (nextNewlineIndex === -1) {
                    if (content.length - lastNewlineIndex > WebInspector.SourceCodeTextEditor.AutoFormatMinimumLineLength)
                        this.autoFormat = true;
                    break;
                }

                if (nextNewlineIndex - lastNewlineIndex > WebInspector.SourceCodeTextEditor.AutoFormatMinimumLineLength) {
                    this.autoFormat = true;
                    break;
                }

                lastNewlineIndex = nextNewlineIndex + 1;
            }
        }
    },

    _contentDidPopulate: function()
    {
        this._contentPopulated = true;

        this.dispatchEventToListeners(WebInspector.SourceCodeTextEditor.Event.ContentDidPopulate);

        // We add the issues each time content is populated. This is needed because lines might not exist
        // if we tried added them before when the full content wasn't avaiable. (When populating with
        // partial script content this can be called multiple times.)

        this._issuesLineNumberMap = {};

        var issues = WebInspector.issueManager.issuesForSourceCode(this._sourceCode);
        for (var i = 0; i < issues.length; ++i) {
            var issue = issues[i];
            console.assert(this._matchesIssue(issue));
            this._addIssue(issue);
        }

        this._updateJumpToSymbolTrackingMode();
    },

    _populateWithContent: function(content)
    {
        content = content || "";

        this._contentWillPopulate(content);
        this.string = content;
        this._contentDidPopulate();
    },

    _contentAvailable: function(sourceCode, content, base64Encoded)
    {
        console.assert(sourceCode === this._sourceCode);
        console.assert(!base64Encoded);

        // Abort if the full content populated while waiting for this async callback.
        if (this._fullContentPopulated)
            return;

        this._fullContentPopulated = true;
        this._invalidLineNumbers = {};

        this._populateWithContent(content);
    },

    _updateBreakpointStatus: function(event)
    {
        console.assert(this._supportsDebugging);

        if (!this._contentPopulated)
            return;

        var breakpoint = event.target;
        if (!this._matchesBreakpoint(breakpoint))
            return;

        var lineInfo = this._editorLineInfoForSourceCodeLocation(breakpoint.sourceCodeLocation);
        this.setBreakpointInfoForLineAndColumn(lineInfo.lineNumber, lineInfo.columnNumber, this._breakpointInfoForBreakpoint(breakpoint));
    },

    _updateBreakpointLocation: function(event)
    {
        console.assert(this._supportsDebugging);

        if (!this._contentPopulated)
            return;

        var breakpoint = event.target;
        if (!this._matchesBreakpoint(breakpoint))
            return;

        if (this._ignoreAllBreakpointLocationUpdates)
            return;

        if (breakpoint === this._ignoreLocationUpdateBreakpoint)
            return;

        var sourceCodeLocation = breakpoint.sourceCodeLocation;

        if (this._sourceCode instanceof WebInspector.SourceMapResource) {
            // Update our breakpoint location if the display location changed.
            if (sourceCodeLocation.displaySourceCode !== this._sourceCode)
                return;
            var oldLineInfo = {lineNumber: event.data.oldDisplayLineNumber, columnNumber: event.data.oldDisplayColumnNumber};
            var newLineInfo = {lineNumber: sourceCodeLocation.displayLineNumber, columnNumber: sourceCodeLocation.displayColumnNumber};
        } else {
            // Update our breakpoint location if the original location changed.
            if (sourceCodeLocation.sourceCode !== this._sourceCode)
                return;
            var oldLineInfo = {lineNumber: event.data.oldFormattedLineNumber, columnNumber: event.data.oldFormattedColumnNumber};
            var newLineInfo = {lineNumber: sourceCodeLocation.formattedLineNumber, columnNumber: sourceCodeLocation.formattedColumnNumber};
        }

        var existingBreakpoint = this._breakpointForEditorLineInfo(oldLineInfo);
        if (!existingBreakpoint)
            return;

        console.assert(breakpoint === existingBreakpoint);

        this.setBreakpointInfoForLineAndColumn(oldLineInfo.lineNumber, oldLineInfo.columnNumber, null);
        this.setBreakpointInfoForLineAndColumn(newLineInfo.lineNumber, newLineInfo.columnNumber, this._breakpointInfoForBreakpoint(breakpoint));

        this._removeBreakpointWithEditorLineInfo(breakpoint, oldLineInfo);
        this._addBreakpointWithEditorLineInfo(breakpoint, newLineInfo);
    },

    _breakpointAdded: function(event)
    {
        console.assert(this._supportsDebugging);

        if (!this._contentPopulated)
            return;

        var breakpoint = event.data.breakpoint;
        if (!this._matchesBreakpoint(breakpoint))
            return;

        if (breakpoint === this._ignoreBreakpointAddedBreakpoint)
            return;

        var lineInfo = this._editorLineInfoForSourceCodeLocation(breakpoint.sourceCodeLocation);
        this._addBreakpointWithEditorLineInfo(breakpoint, lineInfo);
        this.setBreakpointInfoForLineAndColumn(lineInfo.lineNumber, lineInfo.columnNumber, this._breakpointInfoForBreakpoint(breakpoint));
    },

    _breakpointRemoved: function(event)
    {
        console.assert(this._supportsDebugging);

        if (!this._contentPopulated)
            return;

        var breakpoint = event.data.breakpoint;
        if (!this._matchesBreakpoint(breakpoint))
            return;

        if (breakpoint === this._ignoreBreakpointRemovedBreakpoint)
            return;

        var lineInfo = this._editorLineInfoForSourceCodeLocation(breakpoint.sourceCodeLocation);
        this._removeBreakpointWithEditorLineInfo(breakpoint, lineInfo);
        this.setBreakpointInfoForLineAndColumn(lineInfo.lineNumber, lineInfo.columnNumber, null);
    },

    _activeCallFrameDidChange: function()
    {
        console.assert(this._supportsDebugging);

        if (this._activeCallFrameSourceCodeLocation) {
            this._activeCallFrameSourceCodeLocation.removeEventListener(WebInspector.SourceCodeLocation.Event.LocationChanged, this._activeCallFrameSourceCodeLocationChanged, this);
            delete this._activeCallFrameSourceCodeLocation;
        }

        var activeCallFrame = WebInspector.debuggerManager.activeCallFrame;
        if (!activeCallFrame || !this._matchesSourceCodeLocation(activeCallFrame.sourceCodeLocation)) {
            this.executionLineNumber = NaN;
            this.executionColumnNumber = NaN;
            return;
        }

        this._dismissPopover();

        this._activeCallFrameSourceCodeLocation = activeCallFrame.sourceCodeLocation;
        this._activeCallFrameSourceCodeLocation.addEventListener(WebInspector.SourceCodeLocation.Event.LocationChanged, this._activeCallFrameSourceCodeLocationChanged, this);

        // Don't return early if the line number didn't change. The execution state still
        // could have changed (e.g. continuing in a loop with a breakpoint inside).

        var lineInfo = this._editorLineInfoForSourceCodeLocation(activeCallFrame.sourceCodeLocation);
        this.executionLineNumber = lineInfo.lineNumber;
        this.executionColumnNumber = lineInfo.columnNumber;

        // If we have full content or this source code isn't a Resource we can return early.
        // Script source code populates from the request started in the constructor.
        if (this._fullContentPopulated || !(this._sourceCode instanceof WebInspector.Resource) || this._requestingScriptContent)
            return;

        // Since we are paused in the debugger we need to show some content, and since the Resource
        // content hasn't populated yet we need to populate with content from the Scripts by URL.
        // Document resources will attempt to populate the scripts as inline (in <script> tags.)
        // Other resources are assumed to be full scripts (JavaScript resources).
        if (this._sourceCode.type === WebInspector.Resource.Type.Document)
            this._populateWithInlineScriptContent();
        else
            this._populateWithScriptContent();
    },

    _activeCallFrameSourceCodeLocationChanged: function(event)
    {
        console.assert(!isNaN(this.executionLineNumber));
        if (isNaN(this.executionLineNumber))
            return;

        console.assert(WebInspector.debuggerManager.activeCallFrame);
        console.assert(this._activeCallFrameSourceCodeLocation === WebInspector.debuggerManager.activeCallFrame.sourceCodeLocation);

        var lineInfo = this._editorLineInfoForSourceCodeLocation(this._activeCallFrameSourceCodeLocation);
        this.executionLineNumber = lineInfo.lineNumber;
        this.executionColumnNumber = lineInfo.columnNumber;
    },

    _populateWithInlineScriptContent: function()
    {
        console.assert(this._sourceCode instanceof WebInspector.Resource);
        console.assert(!this._fullContentPopulated);
        console.assert(!this._requestingScriptContent);

        var scripts = this._sourceCode.scripts;
        console.assert(scripts.length);
        if (!scripts.length)
            return;

        var pendingRequestCount = scripts.length;

        // If the number of scripts hasn't change since the last populate, then there is nothing to do.
        if (this._inlineScriptContentPopulated === pendingRequestCount)
            return;

        this._inlineScriptContentPopulated = pendingRequestCount;

        function scriptContentAvailable(error, content)
        {
            // Return early if we are still waiting for content from other scripts.
            if (--pendingRequestCount)
                return;

            delete this._requestingScriptContent;

            // Abort if the full content populated while waiting for these async callbacks.
            if (this._fullContentPopulated)
                return;

            const scriptOpenTag = "<script>";
            const scriptCloseTag = "</script>";

            var content = "";
            var lineNumber = 0;
            var columnNumber = 0;

            this._invalidLineNumbers = {};

            for (var i = 0; i < scripts.length; ++i) {
                // Fill the line gap with newline characters.
                for (var newLinesCount = scripts[i].range.startLine - lineNumber; newLinesCount > 0; --newLinesCount) {
                    if (!columnNumber)
                        this._invalidLineNumbers[scripts[i].range.startLine - newLinesCount] = true;
                    columnNumber = 0;
                    content += "\n";
                }

                // Fill the column gap with space characters.
                for (var spacesCount = scripts[i].range.startColumn - columnNumber - scriptOpenTag.length; spacesCount > 0; --spacesCount)
                    content += " ";

                // Add script tags and content.
                content += scriptOpenTag;
                content += scripts[i].content;
                content += scriptCloseTag;

                lineNumber = scripts[i].range.endLine;
                columnNumber = scripts[i].range.endColumn + scriptCloseTag.length;
            }

            this._populateWithContent(content);
        }

        this._requestingScriptContent = true;

        var boundScriptContentAvailable = scriptContentAvailable.bind(this);
        for (var i = 0; i < scripts.length; ++i)
            scripts[i].requestContent(boundScriptContentAvailable);
    },

    _populateWithScriptContent: function()
    {
        console.assert(this._sourceCode instanceof WebInspector.Resource);
        console.assert(!this._fullContentPopulated);
        console.assert(!this._requestingScriptContent);

        // We can assume this resource only has one script that starts at line/column 0.
        var scripts = this._sourceCode.scripts;
        console.assert(scripts.length === 1);
        if (!scripts.length)
            return;

        console.assert(scripts[0].range.startLine === 0);
        console.assert(scripts[0].range.startColumn === 0);

        function scriptContentAvailable(error, content)
        {
            delete this._requestingScriptContent;

            // Abort if the full content populated while waiting for this async callback.
            if (this._fullContentPopulated)
                return;

            // This is the full content.
            this._fullContentPopulated = true;

            this._populateWithContent(content);
        }

        this._requestingScriptContent = true;

        scripts[0].requestContent(scriptContentAvailable.bind(this));
    },

    _matchesSourceCodeLocation: function(sourceCodeLocation)
    {
        if (this._sourceCode instanceof WebInspector.SourceMapResource)
            return sourceCodeLocation.displaySourceCode === this._sourceCode;
        if (this._sourceCode instanceof WebInspector.Resource)
            return sourceCodeLocation.sourceCode.url === this._sourceCode.url;
        if (this._sourceCode instanceof WebInspector.Script)
            return sourceCodeLocation.sourceCode === this._sourceCode;
        return false;
    },

    _matchesBreakpoint: function(breakpoint)
    {
        console.assert(this._supportsDebugging);
        if (this._sourceCode instanceof WebInspector.SourceMapResource)
            return breakpoint.sourceCodeLocation.displaySourceCode === this._sourceCode;
        if (this._sourceCode instanceof WebInspector.Resource)
            return breakpoint.url === this._sourceCode.url;
        if (this._sourceCode instanceof WebInspector.Script)
            return breakpoint.url === this._sourceCode.url || breakpoint.scriptIdentifier === this._sourceCode.id;
        return false;
    },

    _matchesIssue: function(issue)
    {
        if (this._sourceCode instanceof WebInspector.Resource)
            return issue.url === this._sourceCode.url;
        // FIXME: Support issues for Scripts based on id, not only by URL.
        if (this._sourceCode instanceof WebInspector.Script)
            return issue.url === this._sourceCode.url;
        return false;
    },

    _issueWasAdded: function(event)
    {
        var issue = event.data.issue;
        if (!this._matchesIssue(issue))
            return;

        this._addIssue(issue);
    },

    _addIssue: function(issue)
    {
        var lineNumberIssues = this._issuesLineNumberMap[issue.lineNumber];
        if (!lineNumberIssues)
            lineNumberIssues = this._issuesLineNumberMap[issue.lineNumber] = [];

        lineNumberIssues.push(issue);

        if (issue.level === WebInspector.IssueMessage.Level.Error)
            this.addStyleClassToLine(issue.lineNumber, WebInspector.SourceCodeTextEditor.LineErrorStyleClassName);
        else if (issue.level === WebInspector.IssueMessage.Level.Warning)
            this.addStyleClassToLine(issue.lineNumber, WebInspector.SourceCodeTextEditor.LineWarningStyleClassName);
        else
            console.error("Unknown issue level");

        // FIXME <rdar://problem/10854857>: Show the issue message on the line as a bubble.
    },

    _breakpointInfoForBreakpoint: function(breakpoint)
    {
        return {resolved: breakpoint.resolved, disabled: breakpoint.disabled};
    },

    get _supportsDebugging()
    {
        if (this._sourceCode instanceof WebInspector.Resource)
            return this._sourceCode.type === WebInspector.Resource.Type.Document || this._sourceCode.type === WebInspector.Resource.Type.Script;
        if (this._sourceCode instanceof WebInspector.Script)
            return true;
        return false;
    },

    // TextEditor Delegate

    textEditorBaseURL: function(textEditor)
    {
        return this._sourceCode.url;
    },

    textEditorShouldHideLineNumber: function(textEditor, lineNumber)
    {
        return lineNumber in this._invalidLineNumbers;
    },

    textEditorBreakpointAdded: function(textEditor, lineNumber, columnNumber)
    {
        if (!this._supportsDebugging)
            return null;

        var editorLineInfo = {lineNumber:lineNumber, columnNumber:columnNumber};
        var unformattedLineInfo = this._unformattedLineInfoForEditorLineInfo(editorLineInfo);
        var sourceCodeLocation = this._sourceCode.createSourceCodeLocation(unformattedLineInfo.lineNumber, unformattedLineInfo.columnNumber);
        var breakpoint = new WebInspector.Breakpoint(sourceCodeLocation);

        var lineInfo = this._editorLineInfoForSourceCodeLocation(breakpoint.sourceCodeLocation);
        this._addBreakpointWithEditorLineInfo(breakpoint, lineInfo);

        this._ignoreBreakpointAddedBreakpoint = breakpoint;
        WebInspector.debuggerManager.addBreakpoint(breakpoint);
        delete this._ignoreBreakpointAddedBreakpoint;

        // Return the more accurate location and breakpoint info.
        return {
            breakpointInfo: this._breakpointInfoForBreakpoint(breakpoint),
            lineNumber: lineInfo.lineNumber,
            columnNumber: lineInfo.columnNumber
        };
    },

    textEditorBreakpointRemoved: function(textEditor, lineNumber, columnNumber)
    {
        console.assert(this._supportsDebugging);
        if (!this._supportsDebugging)
            return;

        var lineInfo = {lineNumber: lineNumber, columnNumber: columnNumber};
        var breakpoint = this._breakpointForEditorLineInfo(lineInfo);
        console.assert(breakpoint);
        if (!breakpoint)
            return;

        this._removeBreakpointWithEditorLineInfo(breakpoint, lineInfo);

        this._ignoreBreakpointRemovedBreakpoint = breakpoint;
        WebInspector.debuggerManager.removeBreakpoint(breakpoint);
        delete this._ignoreBreakpointAddedBreakpoint;
    },

    textEditorBreakpointMoved: function(textEditor, oldLineNumber, oldColumnNumber, newLineNumber, newColumnNumber)
    {
        console.assert(this._supportsDebugging);
        if (!this._supportsDebugging)
            return;

        var oldLineInfo = {lineNumber: oldLineNumber, columnNumber: oldColumnNumber};
        var breakpoint = this._breakpointForEditorLineInfo(oldLineInfo);
        console.assert(breakpoint);
        if (!breakpoint)
            return;

        this._removeBreakpointWithEditorLineInfo(breakpoint, oldLineInfo);

        var newLineInfo = {lineNumber: newLineNumber, columnNumber: newColumnNumber};
        var unformattedNewLineInfo = this._unformattedLineInfoForEditorLineInfo(newLineInfo);
        this._ignoreLocationUpdateBreakpoint = breakpoint;
        breakpoint.sourceCodeLocation.update(this._sourceCode, unformattedNewLineInfo.lineNumber, unformattedNewLineInfo.columnNumber);
        delete this._ignoreLocationUpdateBreakpoint;

        var accurateNewLineInfo = this._editorLineInfoForSourceCodeLocation(breakpoint.sourceCodeLocation);
        this._addBreakpointWithEditorLineInfo(breakpoint, accurateNewLineInfo);

        if (accurateNewLineInfo.lineNumber !== newLineInfo.lineNumber || accurateNewLineInfo.columnNumber !== newLineInfo.columnNumber)
            this.updateBreakpointLineAndColumn(newLineInfo.lineNumber, newLineInfo.columnNumber, accurateNewLineInfo.lineNumber, accurateNewLineInfo.columnNumber);
    },

    textEditorBreakpointToggled: function(textEditor, lineNumber, columnNumber, disabled)
    {
        console.assert(this._supportsDebugging);
        if (!this._supportsDebugging)
            return;

        var breakpoint = this._breakpointForEditorLineInfo({lineNumber: lineNumber, columnNumber: columnNumber});
        console.assert(breakpoint);
        if (!breakpoint)
            return;

        breakpoint.disabled = disabled;
    },

    textEditorUpdatedFormatting: function(textEditor)
    {
        this._ignoreAllBreakpointLocationUpdates = true;
        this._sourceCode.formatterSourceMap = this.formatterSourceMap;
        delete this._ignoreAllBreakpointLocationUpdates;

        // Always put the source map on both the Script and Resource if both exist. For example,
        // if this SourceCode is a Resource, then there might also be a Script. In the debugger,
        // the backend identifies call frames with Script line and column information, and the
        // Script needs the formatter source map to produce the proper display line and column.
        if (this._sourceCode instanceof WebInspector.Resource && !(this._sourceCode instanceof WebInspector.SourceMapResource)) {
            var scripts = this._sourceCode.scripts;
            for (var i = 0; i < scripts.length; ++i)
                scripts[i].formatterSourceMap = this.formatterSourceMap;
        } else if (this._sourceCode instanceof WebInspector.Script) {
            if (this._sourceCode.resource)
                this._sourceCode.resource.formatterSourceMap = this.formatterSourceMap;
        }

        // Some breakpoints may have moved, some might not have. Just go through
        // and remove and reinsert all the breakpoints.

        var oldBreakpointMap = this._breakpointMap;
        this._breakpointMap = {};

        for (var lineNumber in oldBreakpointMap) {
            for (var columnNumber in oldBreakpointMap[lineNumber]) {
                var breakpoint = oldBreakpointMap[lineNumber][columnNumber];
                var newLineInfo = this._editorLineInfoForSourceCodeLocation(breakpoint.sourceCodeLocation);
                this._addBreakpointWithEditorLineInfo(breakpoint, newLineInfo);
                this.setBreakpointInfoForLineAndColumn(lineNumber, columnNumber, null);
                this.setBreakpointInfoForLineAndColumn(newLineInfo.lineNumber, newLineInfo.columnNumber, this._breakpointInfoForBreakpoint(breakpoint));
            }
        }
    },

    _shouldTrackTokenHovering: function()
    {
        return this._jumpToSymbolTrackingModeEnabled || WebInspector.debuggerManager.activeCallFrame;
    },

    _startTrackingTokenHoveringIfNeeded: function()
    {
        if (this._shouldTrackTokenHovering() && !this.tokenTrackingController.tracking)
            this.tokenTrackingController.startTracking();
    },

    _stopTrackingTokenHoveringIfNeeded: function()
    {
        if (!this._shouldTrackTokenHovering() && this.tokenTrackingController.tracking)
            this.tokenTrackingController.stopTracking();
    },

    _debuggerDidPause: function(event)
    {
        this._startTrackingTokenHoveringIfNeeded();
    },

    _debuggerDidResume: function(event)
    {
        this._stopTrackingTokenHoveringIfNeeded();

        this._dismissPopover();
    },

    _sourceCodeSourceMapAdded: function(event)
    {
        WebInspector.notifications.addEventListener(WebInspector.Notification.GlobalModifierKeysDidChange, this._updateJumpToSymbolTrackingMode, this);
        this._sourceCode.removeEventListener(WebInspector.SourceCode.Event.SourceMapAdded, this._sourceCodeSourceMapAdded, this);

        this._updateJumpToSymbolTrackingMode();
    },

    _updateJumpToSymbolTrackingMode: function()
    {
        var oldJumpToSymbolTrackingModeEnabled = this._jumpToSymbolTrackingModeEnabled;

        if (!(this._sourceCode instanceof WebInspector.SourceMapResource) && this._sourceCode.sourceMaps.length === 0)
            this._jumpToSymbolTrackingModeEnabled = false;
        else
            this._jumpToSymbolTrackingModeEnabled = WebInspector.modifierKeys.metaKey && !WebInspector.modifierKeys.altKey && !WebInspector.modifierKeys.shiftKey;

        if (oldJumpToSymbolTrackingModeEnabled !== this._jumpToSymbolTrackingModeEnabled) {
            if (this._jumpToSymbolTrackingModeEnabled) {
                this._enableJumpToSymbolTrackingModeSettings();
                this._startTrackingTokenHoveringIfNeeded();
            } else {
                this._stopTrackingTokenHoveringIfNeeded();
                this._disableJumpToSymbolTrackingModeSettings();
                if (!this.tokenTrackingController.tracking)
                    this.tokenTrackingController.removeHighlightedRange();
            }
        }
    },

    _enableJumpToSymbolTrackingModeSettings: function()
    {
        this.tokenTrackingController.classNameForHighlightedRange = WebInspector.CodeMirrorTokenTrackingController.JumpToSymbolHighlightStyleClassName;
        this.tokenTrackingController.mouseOverDelayDuration = 0;
        this.tokenTrackingController.mouseOutReleaseDelayDuration = 0;

        this.tokenTrackingController.mode = WebInspector.CodeMirrorTokenTrackingController.Mode.NonSymbolTokens;
    },

    _disableJumpToSymbolTrackingModeSettings: function()
    {
        this.tokenTrackingController.classNameForHighlightedRange = WebInspector.SourceCodeTextEditor.HoveredExpressionHighlightStyleClassName;
        this.tokenTrackingController.mouseOverDelayDuration = WebInspector.SourceCodeTextEditor.DurationToMouseOverTokenToMakeHoveredToken;
        this.tokenTrackingController.mouseOutReleaseDelayDuration = WebInspector.SourceCodeTextEditor.DurationToMouseOutOfHoveredTokenToRelease;

        this.tokenTrackingController.mode = WebInspector.CodeMirrorTokenTrackingController.Mode.JavaScriptExpression;
    },

    // CodeMirrorTokenTrackingController Delegate

    tokenTrackingControllerCanReleaseHighlightedRange: function(tokenTrackingController, element)
    {
        if (!this._popover)
            return true;

        if (!window.getSelection().isCollapsed && this._popover.element.contains(window.getSelection().anchorNode))
            return false;

        return true;
    },

    tokenTrackingControllerHighlightedRangeReleased: function(tokenTrackingController)
    {
        if (!this._mouseIsOverPopover)
            this._dismissPopover();
    },

    tokenTrackingControllerHighlightedRangeWasClicked: function(tokenTrackingController)
    {
        if (this._jumpToSymbolTrackingModeEnabled) {
            // Links are handled by TextEditor.
            if (/\blink\b/.test(this.tokenTrackingController.candidate.hoveredToken.type))
                return;

            var sourceCodeLocation = this._sourceCodeLocationForEditorPosition(this.tokenTrackingController.candidate.hoveredTokenRange.start);
            if (this.sourceCode instanceof WebInspector.SourceMapResource)
                WebInspector.resourceSidebarPanel.showOriginalOrFormattedSourceCodeLocation(sourceCodeLocation);
            else
                WebInspector.resourceSidebarPanel.showSourceCodeLocation(sourceCodeLocation);
        }
    },

    tokenTrackingControllerNewHighlightCandidate: function(tokenTrackingController, candidate)
    {
        if (this._jumpToSymbolTrackingModeEnabled) {
            this.tokenTrackingController.highlightRange(candidate.hoveredTokenRange);
            return;
        }

        if (!WebInspector.debuggerManager.activeCallFrame)
            return;

        console.assert(candidate.expression);

        function populate(error, result, wasThrown)
        {
            if (error || wasThrown)
                return;

            if (candidate !== this.tokenTrackingController.candidate)
                return;

            var data = WebInspector.RemoteObject.fromPayload(result);
            switch (data.type) {
            case "function":
                this._showPopoverForFunction(data);
                break;
            case "object":
                this._showPopoverForObject(data);
                break;
            case "string":
                this._showPopoverForString(data);
                break;
            case "number":
                this._showPopoverForNumber(data);
                break;
            case "boolean":
                this._showPopoverForBoolean(data);
                break;
            case "undefined":
                this._showPopoverForUndefined(data);
                break;
            }
        }

        DebuggerAgent.evaluateOnCallFrame.invoke({callFrameId: WebInspector.debuggerManager.activeCallFrame.id, expression: candidate.expression, objectGroup: "popover", doNotPauseOnExceptionsAndMuteConsole: true}, populate.bind(this));
    },

    _showPopover: function(content)
    {
        console.assert(this.tokenTrackingController.candidate);

        var candidate = this.tokenTrackingController.candidate;
        if (!candidate)
            return;

        var bounds = this.tokenTrackingController.boundsForRange(candidate.hoveredTokenRange);
        if (!bounds)
            return;

        content.classList.add(WebInspector.SourceCodeTextEditor.PopoverDebuggerContentStyleClassName);

        const padding = 5;
        bounds.origin.x -= padding;
        bounds.origin.y -= padding;
        bounds.size.width += padding * 2;
        bounds.size.height += padding * 2;

        this._popover = this._popover || new WebInspector.Popover(this);
        this._popover.content = content;
        this._popover.present(bounds, [WebInspector.RectEdge.MIN_Y, WebInspector.RectEdge.MAX_Y, WebInspector.RectEdge.MAX_X]);

        this._trackPopoverEvents();

        this.tokenTrackingController.highlightRange(candidate.expressionRange);
    },

    _showPopoverForFunction: function(data)
    {
        var candidate = this.tokenTrackingController.candidate;

        function didGetDetails(error, response)
        {
            if (error) {
                console.error(error);
                this._dismissPopover();
                return;
            }

            // Nothing to do if the token has changed since the time we
            // asked for the function details from the backend.
            if (candidate !== this.tokenTrackingController.candidate)
                return;

            var wrapper = document.createElement("div");
            wrapper.className = "body console-formatted-function";
            wrapper.textContent = data.description;

            var content = document.createElement("div");
            content.className = "function";

            var title = content.appendChild(document.createElement("div"));
            title.className = "title";
            title.textContent = response.name || response.inferredName || response.displayName || WebInspector.UIString("(anonymous function)");

            content.appendChild(wrapper);

            this._showPopover(content);
        }
        DebuggerAgent.getFunctionDetails(data.objectId, didGetDetails.bind(this));
    },

    _showPopoverForObject: function(data)
    {
        if (data.subtype === "null") {
            this._showPopoverForNull(data);
            return;
        }

        var content = document.createElement("div");
        content.className = "object expandable";

        var titleElement = document.createElement("div");
        titleElement.className = "title";
        titleElement.textContent = data.description;
        content.appendChild(titleElement);

        var section = new WebInspector.ObjectPropertiesSection(data);
        section.expanded = true;
        section.element.classList.add("body");
        content.appendChild(section.element);

        this._showPopover(content);
    },

    _showPopoverForString: function(data)
    {
        var content = document.createElement("div");
        content.className = "string console-formatted-string";
        content.textContent = "\"" + data.description + "\"";

        this._showPopover(content);
    },

    _showPopoverForNumber: function(data)
    {
        var content = document.createElement("span");
        content.className = "number console-formatted-number";
        content.textContent = data.description;

        this._showPopover(content);
    },

    _showPopoverForBoolean: function(data)
    {
        var content = document.createElement("span");
        content.className = "boolean console-formatted-boolean";
        content.textContent = data.description;

        this._showPopover(content);
    },

    _showPopoverForNull: function(data)
    {
        var content = document.createElement("span");
        content.className = "boolean console-formatted-null";
        content.textContent = data.description;

        this._showPopover(content);
    },

    _showPopoverForUndefined: function(data)
    {
        var content = document.createElement("span");
        content.className = "boolean console-formatted-undefined";
        content.textContent = data.description;

        this._showPopover(content);
    },

    willDismissPopover: function(popover)
    {
        this.tokenTrackingController.removeHighlightedRange();

        RuntimeAgent.releaseObjectGroup("popover");
    },

    _dismissPopover: function()
    {
        if (!this._popover)
            return;

        this._popover.dismiss();

        if (this._popoverEventHandler)
            this._popoverEventHandler.stopTrackingEvents();
    },

    _trackPopoverEvents: function()
    {
        if (!this._popoverEventHandler) {
            this._popoverEventHandler = new WebInspector.EventHandler(this, {
                "mouseover": this._popoverMouseover,
                "mouseout": this._popoverMouseout,
            });
        }

        this._popoverEventHandler.trackEvents(this._popover.element);
    },

    _popoverMouseover: function(event)
    {
        this._mouseIsOverPopover = true;
    },

    _popoverMouseout: function(event)
    {
        this._mouseIsOverPopover = this._popover.element.contains(event.relatedTarget);
    }
};

WebInspector.SourceCodeTextEditor.prototype.__proto__ = WebInspector.TextEditor.prototype;
