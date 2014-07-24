/*
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
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
 * @param {CSSAgent.SelectorProfile} data
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

        if (this._profileView.showTimeAsPercent.get())
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
            wrapperDiv.appendChild(WebInspector.linkifyResourceAsNode(this.rawData.url, this.rawData.lineNumber));
        }

        return cell;
    },

    __proto__: WebInspector.DataGridNode.prototype
}

/**
 * @constructor
 * @extends WebInspector.View
 * @param {CSSAgent.SelectorProfile} profile
 */
WebInspector.CSSSelectorProfileView = function(profile)
{
    WebInspector.View.call(this);

    this.element.addStyleClass("profile-view");

    this.showTimeAsPercent = WebInspector.settings.createSetting("selectorProfilerShowTimeAsPercent", true);

    var columns = [
        {id: "selector", title: WebInspector.UIString("Selector"), width: "550px", sortable: true},
        {id: "source", title: WebInspector.UIString("Source"), width: "100px", sortable: true},
        {id: "time", title: WebInspector.UIString("Total"), width: "72px", sort: WebInspector.DataGrid.Order.Descending, sortable: true},
        {id: "matches", title: WebInspector.UIString("Matches"), width: "72px", sortable: true}
    ];

    this.dataGrid = new WebInspector.DataGrid(columns);
    this.dataGrid.element.addStyleClass("selector-profile-view");
    this.dataGrid.addEventListener(WebInspector.DataGrid.Events.SortingChanged, this._sortProfile, this);
    this.dataGrid.element.addEventListener("mousedown", this._mouseDownInDataGrid.bind(this), true);
    this.dataGrid.show(this.element);

    this.percentButton = new WebInspector.StatusBarButton("", "percent-time-status-bar-item");
    this.percentButton.addEventListener("click", this._percentClicked, this);

    this.profile = profile;

    this._createProfileNodes();
    this._sortProfile();
    this._updatePercentButton();
}

WebInspector.CSSSelectorProfileView.prototype = {
    statusBarItems: function()
    {
        return [this.percentButton.element];
    },

    get profile()
    {
        return this._profile;
    },

    set profile(profile)
    {
        this._profile = profile;
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
        this.dataGrid.rootNode().removeChildren();

        var children = this.profile.children;
        var count = children.length;

        for (var index = 0; index < count; ++index)
            this.dataGrid.rootNode().appendChild(children[index]);
    },

    refreshData: function()
    {
        var child = this.dataGrid.rootNode().children[0];
        while (child) {
            child.refresh();
            child = child.traverseNextNode(false, null, true);
        }
    },

    refreshShowAsPercents: function()
    {
        this._updatePercentButton();
        this.refreshData();
    },

    _percentClicked: function(event)
    {
        this.showTimeAsPercent.set(!this.showTimeAsPercent.get());
        this.refreshShowAsPercents();
    },

    _updatePercentButton: function()
    {
        if (this.showTimeAsPercent.get()) {
            this.percentButton.title = WebInspector.UIString("Show absolute times.");
            this.percentButton.toggled = true;
        } else {
            this.percentButton.title = WebInspector.UIString("Show times as percentages.");
            this.percentButton.toggled = false;
        }
    },

    _sortProfile: function()
    {
        var sortAscending = this.dataGrid.isSortOrderAscending();
        var sortColumnIdentifier = this.dataGrid.sortColumnIdentifier();

        function selectorComparator(a, b)
        {
            var result = b.rawData.selector.compareTo(a.rawData.selector);
            return sortAscending ? -result : result;
        }

        function sourceComparator(a, b)
        {
            var aRawData = a.rawData;
            var bRawData = b.rawData;
            var result = bRawData.url.compareTo(aRawData.url);
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
    },

    _mouseDownInDataGrid: function(event)
    {
        if (event.detail < 2)
            return;

        var cell = event.target.enclosingNodeOrSelfWithNodeName("td");
        if (!cell)
            return;

        if (cell.hasStyleClass("time-column"))
            this.showTimeAsPercent.set(!this.showTimeAsPercent.get());
        else
            return;

        this.refreshShowAsPercents();

        event.consume(true);
    },

    __proto__: WebInspector.View.prototype
}

/**
 * @constructor
 * @extends {WebInspector.ProfileType}
 */
WebInspector.CSSSelectorProfileType = function()
{
    WebInspector.ProfileType.call(this, WebInspector.CSSSelectorProfileType.TypeId, WebInspector.UIString("Collect CSS Selector Profile"));
    this._recording = false;
    this._profileUid = 1;
    WebInspector.CSSSelectorProfileType.instance = this;
}

WebInspector.CSSSelectorProfileType.TypeId = "SELECTOR";

WebInspector.CSSSelectorProfileType.prototype = {
    get buttonTooltip()
    {
        return this._recording ? WebInspector.UIString("Stop CSS selector profiling.") : WebInspector.UIString("Start CSS selector profiling.");
    },

    /**
     * @override
     * @return {boolean}
     */
    buttonClicked: function()
    {
        if (this._recording) {
            this._stopRecordingProfile();
            return false;
        } else {
            this._startRecordingProfile();
            return true;
        }
    },

    get treeItemTitle()
    {
        return WebInspector.UIString("CSS SELECTOR PROFILES");
    },

    get description()
    {
        return WebInspector.UIString("CSS selector profiles show how long the selector matching has taken in total and how many times a certain selector has matched DOM elements (the results are approximate due to matching algorithm optimizations.)");
    },

    reset: function()
    {
        this._profileUid = 1;
    },

    setRecordingProfile: function(isProfiling)
    {
        this._recording = isProfiling;
    },

    _startRecordingProfile: function()
    {
        this._recording = true;
        CSSAgent.startSelectorProfiler();
    },

    _stopRecordingProfile: function()
    {
        /**
         * @param {?Protocol.Error} error
         * @param {CSSAgent.SelectorProfile} profile
         */
        function callback(error, profile)
        {
            if (error)
                return;

            var uid = this._profileUid++;
            var title = WebInspector.UIString("Profile %d", uid) + String.sprintf(" (%s)", Number.secondsToString(profile.totalTime / 1000));
            this.addProfile(new WebInspector.CSSProfileHeader(this, title, uid, profile));
        }

        this._recording = false;
        CSSAgent.stopSelectorProfiler(callback.bind(this));
    },

    /**
     * @override
     * @param {string=} title
     * @return {WebInspector.ProfileHeader}
     */
    createTemporaryProfile: function(title)
    {
        title = title || WebInspector.UIString("Recording\u2026");
        return new WebInspector.CSSProfileHeader(this, title);
    },

    __proto__: WebInspector.ProfileType.prototype
}


/**
 * @constructor
 * @extends {WebInspector.ProfileHeader}
 * @param {!WebInspector.CSSSelectorProfileType} type
 * @param {string} title
 * @param {number=} uid
 * @param {CSSAgent.SelectorProfile=} protocolData
 */
WebInspector.CSSProfileHeader = function(type, title, uid, protocolData)
{
    WebInspector.ProfileHeader.call(this, type, title, uid);
    this._protocolData = protocolData;
}

WebInspector.CSSProfileHeader.prototype = {
    /**
     * @override
     */
    createSidebarTreeElement: function()
    {
        return new WebInspector.ProfileSidebarTreeElement(this, this.title, "profile-sidebar-tree-item");
    },

    /**
     * @override
     * @param {WebInspector.ProfilesPanel} profilesPanel
     */
    createView: function(profilesPanel)
    {
        var profile = /** @type {CSSAgent.SelectorProfile} */ (this._protocolData);
        return new WebInspector.CSSSelectorProfileView(profile);
    },

    __proto__: WebInspector.ProfileHeader.prototype
}
