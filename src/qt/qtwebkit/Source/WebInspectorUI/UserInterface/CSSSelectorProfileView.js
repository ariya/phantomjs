/*
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
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
 * @extends WebInspector.DataGridNode
 * @param {WebInspector.CSSSelectorProfileView} profileView
 */
WebInspector.CSSSelectorDataGridNode = function(profileView, data)
{
    WebInspector.DataGridNode.call(this, data, false);
    this._profileView = profileView;
}

WebInspector.CSSSelectorDataGridNode.prototype = {
    get data()
    {
        var data = {};
        data.selector = this._data.selector;
        data.matches = this._data.matchCount;

        if (this._profileView.showTimeAsPercent.value)
            data.time = Number(this._data.timePercent).toFixed(1) + "%";
        else
            data.time = Number.secondsToString(this._data.time / 1000, true);

        return data;
    },

    get rawData()
    {
        return this._data;
    },

    createCell: function(columnIdentifier)
    {
        var cell = WebInspector.DataGridNode.prototype.createCell.call(this, columnIdentifier);
        if (columnIdentifier === "selector" && cell.firstChild) {
            cell.firstChild.title = this.rawData.selector;
            return cell;
        }

        if (columnIdentifier !== "source")
            return cell;

        cell.removeChildren();

        if (this.rawData.url) {
            var wrapperDiv = cell.createChild("div");
            wrapperDiv.appendChild(this._linkifyLocation(this.rawData.url, this.rawData.lineNumber));
        }

        return cell;
    },

    _linkifyLocation: function(url, lineNumber)
    {
        // FIXME: SelectorProfileEntry need columnNumbers.
        return WebInspector.linkifyLocation(url, lineNumber, 0);
    }
}

WebInspector.CSSSelectorDataGridNode.prototype.__proto__ = WebInspector.DataGridNode.prototype;

/**
 * @constructor
 * @extends WebInspector.View
 * @param {CSSAgent.SelectorProfile} profile
 */
WebInspector.CSSSelectorProfileView = function(profile)
{
    console.assert(profile instanceof WebInspector.CSSSelectorProfileObject);

    WebInspector.ProfileView.call(this, profile, "css-selector-profiler-show-time-as-percent");
}

WebInspector.CSSSelectorProfileView.prototype = {
    constructor: WebInspector.CSSSelectorProfileView,

    updateLayout: function()
    {
        if (this.dataGrid)
            this.dataGrid.updateLayout();
    },

    get recordingTitle()
    {
        return WebInspector.UIString("Recording CSS Selector Profile\u2026");
    },

    displayProfile: function()
    {
        var columns = { "selector": { title: WebInspector.UIString("Selector"), width: "550px", sortable: true },
                        "source": { title: WebInspector.UIString("Source"), width: "100px", sortable: true },
                        "time": { title: WebInspector.UIString("Total"), width: "72px", sort: "descending", sortable: true },
                        "matches": { title: WebInspector.UIString("Matches"), width: "72px", sortable: true } };

        this.dataGrid = new WebInspector.DataGrid(columns);
        this.dataGrid.element.classList.add("selector-profile-view");
        this.dataGrid.addEventListener(WebInspector.DataGrid.Event.SortChanged, this._sortProfile, this);

        this.element.appendChild(this.dataGrid.element);
        this.dataGrid.updateLayout();

        this._createProfileNodes();
        this._sortProfile();
    },

    _createProfileNodes: function()
    {
        var data = this.profile.data;
        if (!data) {
            // The profiler may have been terminated with the "Clear all profiles." button.
            return;
        }

        this.profile.children = [];
        for (var i = 0; i < data.length; ++i) {
            data[i].timePercent = data[i].time * 100 / this.profile.totalTime;
            var node = new WebInspector.CSSSelectorDataGridNode(this, data[i]);
            this.profile.children.push(node);
        }
    },

    rebuildGridItems: function()
    {
        this.dataGrid.removeChildren();

        var children = this.profile.children;
        var count = children.length;

        for (var index = 0; index < count; ++index)
            this.dataGrid.appendChild(children[index]);
    },

    refreshData: function()
    {
        var child = this.dataGrid.children[0];
        while (child) {
            child.refresh();
            child = child.traverseNextNode(false, null, true);
        }
    },

    toggleTimeDisplay: function(event)
    {
        WebInspector.ProfileView.prototype.toggleTimeDisplay.call(this, event);
        this.showTimeAsPercent.value = !this.showTimeAsPercent.value;
        this.refreshData();
    },

    _sortProfile: function()
    {
        var sortAscending = this.dataGrid.sortOrder === "ascending";
        var sortColumnIdentifier = this.dataGrid.sortColumnIdentifier;

        function selectorComparator(a, b)
        {
            var result = b.rawData.selector.localeCompare(a.rawData.selector);
            return sortAscending ? -result : result;
        }

        function sourceComparator(a, b)
        {
            var aRawData = a.rawData;
            var bRawData = b.rawData;
            var result = bRawData.url.localeCompare(aRawData.url);
            if (!result)
                result = bRawData.lineNumber - aRawData.lineNumber;
            return sortAscending ? -result : result;
        }

        function timeComparator(a, b)
        {
            const result = b.rawData.time - a.rawData.time;
            return sortAscending ? -result : result;
        }

        function matchesComparator(a, b)
        {
            const result = b.rawData.matchCount - a.rawData.matchCount;
            return sortAscending ? -result : result;
        }

        var comparator;
        switch (sortColumnIdentifier) {
        case "time":
            comparator = timeComparator;
            break;
        case "matches":
            comparator = matchesComparator;
            break;
        case "selector":
            comparator = selectorComparator;
            break;
        case "source":
            comparator = sourceComparator;
            break;
        }

        this.profile.children.sort(comparator);

        this.rebuildGridItems();
    }
}

WebInspector.CSSSelectorProfileView.prototype.__proto__ = WebInspector.ProfileView.prototype;
