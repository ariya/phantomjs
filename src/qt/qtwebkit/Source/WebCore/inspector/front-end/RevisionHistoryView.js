/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.View}
 */
WebInspector.RevisionHistoryView = function()
{
    WebInspector.View.call(this);
    this.registerRequiredCSS("revisionHistory.css");
    this.element.addStyleClass("revision-history-drawer");
    this.element.addStyleClass("fill");
    this.element.addStyleClass("outline-disclosure");
    this._uiSourceCodeItems = new Map();

    var olElement = this.element.createChild("ol");
    this._treeOutline = new TreeOutline(olElement);

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    function populateRevisions(uiSourceCode)
    {
        if (uiSourceCode.history.length)
            this._createUISourceCodeItem(uiSourceCode);
    }

    WebInspector.workspace.uiSourceCodes().forEach(populateRevisions.bind(this));
    WebInspector.workspace.addEventListener(WebInspector.Workspace.Events.UISourceCodeContentCommitted, this._revisionAdded, this);
    WebInspector.workspace.addEventListener(WebInspector.UISourceCodeProvider.Events.UISourceCodeRemoved, this._uiSourceCodeRemoved, this);
    WebInspector.workspace.addEventListener(WebInspector.Workspace.Events.ProjectWillReset, this._projectWillReset, this);

    this._statusElement = document.createElement("span");
    this._statusElement.textContent = WebInspector.UIString("Local modifications");

}

/**
 * @param {WebInspector.UISourceCode} uiSourceCode
 */
WebInspector.RevisionHistoryView.showHistory = function(uiSourceCode)
{
    if (!WebInspector.RevisionHistoryView._view) 
        WebInspector.RevisionHistoryView._view = new WebInspector.RevisionHistoryView();
    var view = WebInspector.RevisionHistoryView._view;
    WebInspector.showViewInDrawer(view._statusElement, view);
    view._revealUISourceCode(uiSourceCode);
}

WebInspector.RevisionHistoryView.prototype = {
    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    _createUISourceCodeItem: function(uiSourceCode)
    {
        var uiSourceCodeItem = new TreeElement(uiSourceCode.displayName(), null, true);
        uiSourceCodeItem.selectable = false;

        // Insert in sorted order
        for (var i = 0; i < this._treeOutline.children.length; ++i) {
            if (this._treeOutline.children[i].title.localeCompare(uiSourceCode.displayName()) > 0) {
                this._treeOutline.insertChild(uiSourceCodeItem, i);
                break;
            }
        }
        if (i === this._treeOutline.children.length)
            this._treeOutline.appendChild(uiSourceCodeItem);

        this._uiSourceCodeItems.put(uiSourceCode, uiSourceCodeItem);

        var revisionCount = uiSourceCode.history.length;
        for (var i = revisionCount - 1; i >= 0; --i) {
            var revision = uiSourceCode.history[i];
            var historyItem = new WebInspector.RevisionHistoryTreeElement(revision, uiSourceCode.history[i - 1], i !== revisionCount - 1);
            uiSourceCodeItem.appendChild(historyItem);
        }

        var linkItem = new TreeElement("", null, false);
        linkItem.selectable = false;
        uiSourceCodeItem.appendChild(linkItem);

        var revertToOriginal = linkItem.listItemElement.createChild("span", "revision-history-link revision-history-link-row");
        revertToOriginal.textContent = WebInspector.UIString("apply original content");
        revertToOriginal.addEventListener("click", uiSourceCode.revertToOriginal.bind(uiSourceCode));

        var clearHistoryElement = uiSourceCodeItem.listItemElement.createChild("span", "revision-history-link");
        clearHistoryElement.textContent = WebInspector.UIString("revert");
        clearHistoryElement.addEventListener("click", this._clearHistory.bind(this, uiSourceCode));
        return uiSourceCodeItem;
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    _clearHistory: function(uiSourceCode)
    {
        uiSourceCode.revertAndClearHistory(this._removeUISourceCode.bind(this));
    },

    _revisionAdded: function(event)
    {
        var uiSourceCode = /** @type {WebInspector.UISourceCode} */ (event.data.uiSourceCode);
        var uiSourceCodeItem = this._uiSourceCodeItems.get(uiSourceCode);
        if (!uiSourceCodeItem) {
            uiSourceCodeItem = this._createUISourceCodeItem(uiSourceCode);
            return;
        }

        var historyLength = uiSourceCode.history.length;
        var historyItem = new WebInspector.RevisionHistoryTreeElement(uiSourceCode.history[historyLength - 1], uiSourceCode.history[historyLength - 2], false);
        if (uiSourceCodeItem.children.length)
            uiSourceCodeItem.children[0].allowRevert();
        uiSourceCodeItem.insertChild(historyItem, 0);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    _revealUISourceCode: function(uiSourceCode)
    {
        var uiSourceCodeItem = this._uiSourceCodeItems.get(uiSourceCode);
        if (uiSourceCodeItem) {
            uiSourceCodeItem.reveal();
            uiSourceCodeItem.expand();
        }
    },

    _uiSourceCodeRemoved: function(event)
    {
        var uiSourceCode = /** @type {WebInspector.UISourceCode} */ (event.data);
        this._removeUISourceCode(uiSourceCode);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    _removeUISourceCode: function(uiSourceCode)
    {
        var uiSourceCodeItem = this._uiSourceCodeItems.get(uiSourceCode);
        if (!uiSourceCodeItem)
            return;
        this._treeOutline.removeChild(uiSourceCodeItem);
        this._uiSourceCodeItems.remove(uiSourceCode);
    },

    _projectWillReset: function(event)
    {
        var project = event.data;
        project.uiSourceCodes().forEach(this._removeUISourceCode.bind(this));
    },

    __proto__: WebInspector.View.prototype
}

/**
 * @constructor
 * @extends {TreeElement}
 * @param {WebInspector.Revision} revision
 * @param {WebInspector.Revision} baseRevision
 * @param {boolean} allowRevert
 */
WebInspector.RevisionHistoryTreeElement = function(revision, baseRevision, allowRevert)
{
    TreeElement.call(this, revision.timestamp.toLocaleTimeString(), null, true);
    this.selectable = false;

    this._revision = revision;
    this._baseRevision = baseRevision;

    this._revertElement = document.createElement("span");
    this._revertElement.className = "revision-history-link";
    this._revertElement.textContent = WebInspector.UIString("apply revision content");
    this._revertElement.addEventListener("click", this._revision.revertToThis.bind(this._revision), false);
    if (!allowRevert)
        this._revertElement.addStyleClass("hidden");
}

WebInspector.RevisionHistoryTreeElement.prototype = {
    onattach: function()
    {
        this.listItemElement.addStyleClass("revision-history-revision");
    },

    onexpand: function()
    {
        this.listItemElement.appendChild(this._revertElement);

        if (this._wasExpandedOnce)
            return;
        this._wasExpandedOnce = true;

        this.childrenListElement.addStyleClass("source-code");
        if (this._baseRevision)
            this._baseRevision.requestContent(step1.bind(this));
        else
            this._revision.uiSourceCode.requestOriginalContent(step1.bind(this));

        function step1(baseContent)
        {
            this._revision.requestContent(step2.bind(this, baseContent));
        }

        function step2(baseContent, newContent)
        {
            var baseLines = difflib.stringAsLines(baseContent);
            var newLines = difflib.stringAsLines(newContent);
            var sm = new difflib.SequenceMatcher(baseLines, newLines);
            var opcodes = sm.get_opcodes();
            var lastWasSeparator = false;

            for (var idx = 0; idx < opcodes.length; idx++) {
                var code = opcodes[idx];
                var change = code[0];
                var b = code[1];
                var be = code[2];
                var n = code[3];
                var ne = code[4];
                var rowCount = Math.max(be - b, ne - n);
                var topRows = [];
                var bottomRows = [];
                for (var i = 0; i < rowCount; i++) {
                    if (change === "delete" || (change === "replace" && b < be)) {
                        var lineNumber = b++;
                        this._createLine(lineNumber, null, baseLines[lineNumber], "removed");
                        lastWasSeparator = false;
                    }

                    if (change === "insert" || (change === "replace" && n < ne)) {
                        var lineNumber = n++;
                        this._createLine(null, lineNumber, newLines[lineNumber], "added");
                        lastWasSeparator = false;
                    }

                    if (change === "equal") {
                        b++;
                        n++;
                        if (!lastWasSeparator)
                            this._createLine(null, null, "    \u2026", "separator");
                        lastWasSeparator = true;
                    }
                }
            }
        }
    },

    oncollapse: function()
    {
        if (this._revertElement.parentElement)
            this._revertElement.parentElement.removeChild(this._revertElement);
    },

    /**
     * @param {?number} baseLineNumber
     * @param {?number} newLineNumber
     * @param {string} lineContent
     * @param {string} changeType
     */
    _createLine: function(baseLineNumber, newLineNumber, lineContent, changeType)
    {
        var child = new TreeElement("", null, false);
        child.selectable = false;
        this.appendChild(child);
        var lineElement = document.createElement("span");

        function appendLineNumber(lineNumber)
        {
            var numberString = lineNumber !== null ? numberToStringWithSpacesPadding(lineNumber + 1, 4) : "    ";
            var lineNumberSpan = document.createElement("span");
            lineNumberSpan.addStyleClass("webkit-line-number");
            lineNumberSpan.textContent = numberString;
            child.listItemElement.appendChild(lineNumberSpan);
        }

        appendLineNumber(baseLineNumber);
        appendLineNumber(newLineNumber);

        var contentSpan = document.createElement("span");
        contentSpan.textContent = lineContent;
        child.listItemElement.appendChild(contentSpan);
        child.listItemElement.addStyleClass("revision-history-line");
        child.listItemElement.addStyleClass("revision-history-line-" + changeType);
    },

    allowRevert: function()
    {
        this._revertElement.removeStyleClass("hidden");
    },

    __proto__: TreeElement.prototype
}
