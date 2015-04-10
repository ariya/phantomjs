/*
 * Copyright (C) 2008, 2013 Apple Inc. All Rights Reserved.
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

WebInspector.JavaScriptProfileView = function(profile)
{
    console.assert(profile instanceof WebInspector.JavaScriptProfileObject);

    WebInspector.ProfileView.call(this, profile, "javascript-profiler-show-time-as-percent");

    this._showTreeBottomUpSetting = new WebInspector.Setting("javascript-profiler-show-tree-bottom-up", true);

    this._showTreeBottomUpNavigationItem = new WebInspector.ActivateButtonNavigationItem("show-tree-bottom-up-navigation-item", WebInspector.UIString("Invert call tree"), WebInspector.UIString("Normal call tree"), "Images/BottomUpTree.pdf", 16, 16);
    this._showTreeBottomUpNavigationItem.addEventListener(WebInspector.ButtonNavigationItem.Event.Clicked, this._toggleBottomUpView, this);
    this._showTreeBottomUpNavigationItem.activated = this._showTreeBottomUpSetting.value;

    this._viewType = new WebInspector.Setting("javascript-profiler-view", WebInspector.JavaScriptProfileView._TypeHeavy);
}

WebInspector.JavaScriptProfileView._TypeTree = "Tree";
WebInspector.JavaScriptProfileView._TypeHeavy = "Heavy";

WebInspector.JavaScriptProfileView.prototype = {
    constructor: WebInspector.JavaScriptProfileView,

    updateLayout: function()
    {
        if (this.dataGrid)
            this.dataGrid.updateLayout();
    },

    get recordingTitle()
    {
        return WebInspector.UIString("Recording JavaScript Profile\u2026");
    },

    displayProfile: function()
    {
        var columns = { "self": { title: WebInspector.UIString("Self"), width: "72px", sort: "descending", sortable: true },
                        "total": { title: WebInspector.UIString("Total"), width: "72px", sortable: true },
                        "average": { title: WebInspector.UIString("Average"), width: "72px", sortable: true },
                        "calls": { title: WebInspector.UIString("Calls"), width: "54px", sortable: true },
                        "function": { title: WebInspector.UIString("Function"), disclosure: true, sortable: true } };

        this.dataGrid = new WebInspector.DataGrid(columns);
        this.dataGrid.addEventListener(WebInspector.DataGrid.Event.SortChanged, this._sortData, this);

        this.element.appendChild(this.dataGrid.element);
        this.dataGrid.updateLayout();

        function profileCallback(error, profile)
        {
            if (error)
                return;

            if (!profile.head) {
                // Profiling was tentatively terminated with the "Clear all profiles." button.
                return;
            }
            this.profile.head = profile.head;
            this._assignParentsInProfile();
            this._changeView();
        }

        // COMPATIBILITY (iOS 6): getCPUProfile use to be called getProfile.
        if (ProfilerAgent.getProfile)
            ProfilerAgent.getProfile(this.profile.type, this.profile.id, profileCallback.bind(this));
        else
            ProfilerAgent.getCPUProfile(this.profile.id, profileCallback.bind(this));
    },

    get navigationItems()
    {
        return [this._showTreeBottomUpNavigationItem, this.showTimeAsPercentNavigationItem];
    },

    get bottomUpProfileDataGridTree()
    {
        if (!this._bottomUpProfileDataGridTree)
            this._bottomUpProfileDataGridTree = new WebInspector.BottomUpProfileDataGridTree(this, this.profile.head);
        return this._bottomUpProfileDataGridTree;
    },

    get topDownProfileDataGridTree()
    {
        if (!this._topDownProfileDataGridTree)
            this._topDownProfileDataGridTree = new WebInspector.TopDownProfileDataGridTree(this, this.profile.head);
        return this._topDownProfileDataGridTree;
    },

    willHide: function()
    {
        this._currentSearchResultIndex = -1;
    },

    refresh: function()
    {
        var selectedProfileNode = this.dataGrid.selectedNode ? this.dataGrid.selectedNode.profileNode : null;

        this.dataGrid.removeChildren();

        var children = this.profileDataGridTree.children;
        var count = children.length;

        for (var index = 0; index < count; ++index)
            this.dataGrid.appendChild(children[index]);

        if (selectedProfileNode)
            selectedProfileNode.selected = true;
    },

    refreshVisibleData: function()
    {
        var child = this.dataGrid.children[0];
        while (child) {
            child.refresh();
            child = child.traverseNextNode(false, null, true);
        }
    },

    searchCanceled: function()
    {
        if (this._searchResults) {
            for (var i = 0; i < this._searchResults.length; ++i) {
                var profileNode = this._searchResults[i].profileNode;

                delete profileNode._searchMatchedSelfColumn;
                delete profileNode._searchMatchedTotalColumn;
                delete profileNode._searchMatchedCallsColumn;
                delete profileNode._searchMatchedFunctionColumn;

                profileNode.refresh();
            }
        }

        delete this._searchFinishedCallback;
        this._currentSearchResultIndex = -1;
        this._searchResults = [];
    },

    performSearch: function(query, finishedCallback)
    {
        // Call searchCanceled since it will reset everything we need before doing a new search.
        this.searchCanceled();

        query = query.trim();

        if (!query.length)
            return;

        this._searchFinishedCallback = finishedCallback;

        var greaterThan = query.charAt(0) === ">";
        var lessThan = query.charAt(0) === "<";
        var equalTo = (query.charAt(0) === "=" || ((greaterThan || lessThan) && query.charAt(1) === "="));
        var percentUnits = (query.lastIndexOf("%") === (query.length - 1));
        var millisecondsUnits = (query.length > 2 && query.lastIndexOf("ms") === (query.length - 2));
        var secondsUnits = (!millisecondsUnits && query.lastIndexOf("s") === (query.length - 1));

        var queryNumber = parseFloat(query);
        if (greaterThan || lessThan || equalTo) {
            if (equalTo && (greaterThan || lessThan))
                queryNumber = parseFloat(query.substring(2));
            else
                queryNumber = parseFloat(query.substring(1));
        }

        var queryNumberMilliseconds = (secondsUnits ? (queryNumber * 1000) : queryNumber);

        // Make equalTo implicitly true if it wasn't specified there is no other operator.
        if (!isNaN(queryNumber) && !(greaterThan || lessThan))
            equalTo = true;

        function matchesQuery(/*ProfileDataGridNode*/ profileDataGridNode)
        {
            delete profileDataGridNode._searchMatchedSelfColumn;
            delete profileDataGridNode._searchMatchedTotalColumn;
            delete profileDataGridNode._searchMatchedAverageColumn;
            delete profileDataGridNode._searchMatchedCallsColumn;
            delete profileDataGridNode._searchMatchedFunctionColumn;

            if (percentUnits) {
                if (lessThan) {
                    if (profileDataGridNode.selfPercent < queryNumber)
                        profileDataGridNode._searchMatchedSelfColumn = true;
                    if (profileDataGridNode.totalPercent < queryNumber)
                        profileDataGridNode._searchMatchedTotalColumn = true;
                    if (profileDataGridNode.averagePercent < queryNumberMilliseconds)
                        profileDataGridNode._searchMatchedAverageColumn = true;
                } else if (greaterThan) {
                    if (profileDataGridNode.selfPercent > queryNumber)
                        profileDataGridNode._searchMatchedSelfColumn = true;
                    if (profileDataGridNode.totalPercent > queryNumber)
                        profileDataGridNode._searchMatchedTotalColumn = true;
                    if (profileDataGridNode.averagePercent < queryNumberMilliseconds)
                        profileDataGridNode._searchMatchedAverageColumn = true;
                }

                if (equalTo) {
                    if (profileDataGridNode.selfPercent == queryNumber)
                        profileDataGridNode._searchMatchedSelfColumn = true;
                    if (profileDataGridNode.totalPercent == queryNumber)
                        profileDataGridNode._searchMatchedTotalColumn = true;
                    if (profileDataGridNode.averagePercent < queryNumberMilliseconds)
                        profileDataGridNode._searchMatchedAverageColumn = true;
                }
            } else if (millisecondsUnits || secondsUnits) {
                if (lessThan) {
                    if (profileDataGridNode.selfTime < queryNumberMilliseconds)
                        profileDataGridNode._searchMatchedSelfColumn = true;
                    if (profileDataGridNode.totalTime < queryNumberMilliseconds)
                        profileDataGridNode._searchMatchedTotalColumn = true;
                    if (profileDataGridNode.averageTime < queryNumberMilliseconds)
                        profileDataGridNode._searchMatchedAverageColumn = true;
                } else if (greaterThan) {
                    if (profileDataGridNode.selfTime > queryNumberMilliseconds)
                        profileDataGridNode._searchMatchedSelfColumn = true;
                    if (profileDataGridNode.totalTime > queryNumberMilliseconds)
                        profileDataGridNode._searchMatchedTotalColumn = true;
                    if (profileDataGridNode.averageTime > queryNumberMilliseconds)
                        profileDataGridNode._searchMatchedAverageColumn = true;
                }

                if (equalTo) {
                    if (profileDataGridNode.selfTime == queryNumberMilliseconds)
                        profileDataGridNode._searchMatchedSelfColumn = true;
                    if (profileDataGridNode.totalTime == queryNumberMilliseconds)
                        profileDataGridNode._searchMatchedTotalColumn = true;
                    if (profileDataGridNode.averageTime == queryNumberMilliseconds)
                        profileDataGridNode._searchMatchedAverageColumn = true;
                }
            } else {
                if (equalTo && profileDataGridNode.numberOfCalls == queryNumber)
                    profileDataGridNode._searchMatchedCallsColumn = true;
                if (greaterThan && profileDataGridNode.numberOfCalls > queryNumber)
                    profileDataGridNode._searchMatchedCallsColumn = true;
                if (lessThan && profileDataGridNode.numberOfCalls < queryNumber)
                    profileDataGridNode._searchMatchedCallsColumn = true;
            }

            if (profileDataGridNode.functionName.hasSubstring(query, true) || profileDataGridNode.url.hasSubstring(query, true))
                profileDataGridNode._searchMatchedFunctionColumn = true;

            if (profileDataGridNode._searchMatchedSelfColumn ||
                profileDataGridNode._searchMatchedTotalColumn ||
                profileDataGridNode._searchMatchedAverageColumn ||
                profileDataGridNode._searchMatchedCallsColumn ||
                profileDataGridNode._searchMatchedFunctionColumn)
            {
                profileDataGridNode.refresh();
                return true;
            }

            return false;
        }

        var current = this.profileDataGridTree.children[0];

        while (current) {
            if (matchesQuery(current)) {
                this._searchResults.push({ profileNode: current });
            }

            current = current.traverseNextNode(false, null, false);
        }

        finishedCallback(this, this._searchResults.length);
    },

    jumpToFirstSearchResult: function()
    {
        if (!this._searchResults || !this._searchResults.length)
            return;
        this._currentSearchResultIndex = 0;
        this._jumpToSearchResult(this._currentSearchResultIndex);
    },

    jumpToLastSearchResult: function()
    {
        if (!this._searchResults || !this._searchResults.length)
            return;
        this._currentSearchResultIndex = (this._searchResults.length - 1);
        this._jumpToSearchResult(this._currentSearchResultIndex);
    },

    jumpToNextSearchResult: function()
    {
        if (!this._searchResults || !this._searchResults.length)
            return;
        if (++this._currentSearchResultIndex >= this._searchResults.length)
            this._currentSearchResultIndex = 0;
        this._jumpToSearchResult(this._currentSearchResultIndex);
    },

    jumpToPreviousSearchResult: function()
    {
        if (!this._searchResults || !this._searchResults.length)
            return;
        if (--this._currentSearchResultIndex < 0)
            this._currentSearchResultIndex = (this._searchResults.length - 1);
        this._jumpToSearchResult(this._currentSearchResultIndex);
    },

    showingFirstSearchResult: function()
    {
        return (this._currentSearchResultIndex === 0);
    },

    showingLastSearchResult: function()
    {
        return (this._searchResults && this._currentSearchResultIndex === (this._searchResults.length - 1));
    },

    _jumpToSearchResult: function(index)
    {
        var searchResult = this._searchResults[index];
        if (!searchResult)
            return;

        var profileNode = searchResult.profileNode;
        profileNode.revealAndSelect();
    },

    _toggleBottomUpView: function(event)
    {
        this._showTreeBottomUpSetting.value = !this._showTreeBottomUpSetting.value;
        this._showTreeBottomUpNavigationItem.activated = this._showTreeBottomUpSetting.value;
        this._changeView();
    },

    _changeView: function()
    {
        if (!this.profile)
            return;
    
        if (!this._showTreeBottomUpSetting.value) {
            this.profileDataGridTree = this.topDownProfileDataGridTree;
            this._sortProfile();
            this._viewType.value = WebInspector.JavaScriptProfileView._TypeTree;
        } else {
            this.profileDataGridTree = this.bottomUpProfileDataGridTree;
            this._sortProfile();
            this._viewType.value = WebInspector.JavaScriptProfileView._TypeHeavy;
        }

        if (!this.currentQuery || !this._searchFinishedCallback || !this._searchResults)
            return;

        // The current search needs to be performed again. First negate out previous match
        // count by calling the search finished callback with a negative number of matches.
        // Then perform the search again the with same query and callback.
        this._searchFinishedCallback(this, -this._searchResults.length);
        this.performSearch(this.currentQuery, this._searchFinishedCallback);
    },

    toggleTimeDisplay: function(event)
    {
        WebInspector.ProfileView.prototype.toggleTimeDisplay.call(this, event);
        this.showTimeAsPercent.value = !this.showTimeAsPercent.value;
        this.refreshVisibleData();
    },

    _focusClicked: function(event)
    {
        if (!this.dataGrid.selectedNode)
            return;

        this.profileDataGridTree.focus(this.dataGrid.selectedNode);
        this.refresh();
        this.refreshVisibleData();
    },

    _excludeClicked: function(event)
    {
        var selectedNode = this.dataGrid.selectedNode

        if (!selectedNode)
            return;

        selectedNode.deselect();

        this.profileDataGridTree.exclude(selectedNode);
        this.refresh();
        this.refreshVisibleData();
    },

    _resetClicked: function(event)
    {
        this.profileDataGridTree.restore();
        this.refresh();
        this.refreshVisibleData();
    },

    _dataGridNodeSelected: function(node)
    {
        // FIXME: Update UI we have to focus on a data grid node or exclude it from the profile.
    },

    _dataGridNodeDeselected: function(node)
    {
        // FIXME: Update UI we have to focus on a data grid node or exclude it from the profile.
    },

    _sortData: function(event)
    {
        this._sortProfile(this.profile);
    },

    _sortProfile: function()
    {
        var sortAscending = this.dataGrid.sortOrder === "ascending";
        var sortColumnIdentifier = this.dataGrid.sortColumnIdentifier;
        var sortProperty = {
                "average": "averageTime",
                "self": "selfTime",
                "total": "totalTime",
                "calls": "numberOfCalls",
                "function": "functionName"
            }[sortColumnIdentifier];

        this.profileDataGridTree.sort(WebInspector.ProfileDataGridTree.propertyComparator(sortProperty, sortAscending));

        this.refresh();
    },

    _assignParentsInProfile: function()
    {
        var head = this.profile.head;
        head.parent = null;
        head.head = null;
        var nodesToTraverse = [ { parent: head, children: head.children } ];
        while (nodesToTraverse.length > 0) {
            var pair = nodesToTraverse.shift();
            var parent = pair.parent;
            var children = pair.children;
            var length = children.length;
            for (var i = 0; i < length; ++i) {
                children[i].head = head;
                children[i].parent = parent;
                if (children[i].children.length > 0)
                    nodesToTraverse.push({ parent: children[i], children: children[i].children });
            }
        }
    }
}

WebInspector.JavaScriptProfileView.prototype.__proto__ = WebInspector.ProfileView.prototype;
