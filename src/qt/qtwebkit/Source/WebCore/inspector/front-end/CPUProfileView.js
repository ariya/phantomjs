/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * @extends {WebInspector.View}
 * @param {WebInspector.CPUProfileHeader} profile
 */
WebInspector.CPUProfileView = function(profile)
{
    WebInspector.View.call(this);

    this.element.addStyleClass("profile-view");
    
    this.showSelfTimeAsPercent = WebInspector.settings.createSetting("cpuProfilerShowSelfTimeAsPercent", true);
    this.showTotalTimeAsPercent = WebInspector.settings.createSetting("cpuProfilerShowTotalTimeAsPercent", true);
    this.showAverageTimeAsPercent = WebInspector.settings.createSetting("cpuProfilerShowAverageTimeAsPercent", true);
    this._viewType = WebInspector.settings.createSetting("cpuProfilerView", WebInspector.CPUProfileView._TypeHeavy);

    var columns = [];
    columns.push({id: "self", title: WebInspector.UIString("Self"), width: "72px", sort: WebInspector.DataGrid.Order.Descending, sortable: true});
    columns.push({id: "total", title: WebInspector.UIString("Total"), width: "72px", sortable: true});
    if (!Capabilities.samplingCPUProfiler) {
        columns.push({id: "average", title: WebInspector.UIString("Average"), width: "72px", sortable: true});
        columns.push({id: "calls", title: WebInspector.UIString("Calls"), width: "54px", sortable: true});
    }
    columns.push({id: "function", title: WebInspector.UIString("Function"), disclosure: true, sortable: true});

    this.dataGrid = new WebInspector.DataGrid(columns);
    this.dataGrid.addEventListener(WebInspector.DataGrid.Events.SortingChanged, this._sortProfile, this);
    this.dataGrid.element.addEventListener("mousedown", this._mouseDownInDataGrid.bind(this), true);

    if (WebInspector.experimentsSettings.cpuFlameChart.isEnabled()) {
        this._splitView = new WebInspector.SplitView(false, "flameChartSplitLocation");
        this._splitView.show(this.element);

        this.flameChart = new WebInspector.FlameChart(this);
        this.flameChart.addEventListener(WebInspector.FlameChart.Events.SelectedNode, this._revealProfilerNode.bind(this));
        this.flameChart.show(this._splitView.firstElement());

        this.dataGrid.show(this._splitView.secondElement());
    } else
        this.dataGrid.show(this.element);

    this.viewSelectComboBox = new WebInspector.StatusBarComboBox(this._changeView.bind(this));

    var heavyViewOption = this.viewSelectComboBox.createOption(WebInspector.UIString("Heavy (Bottom Up)"), "", WebInspector.CPUProfileView._TypeHeavy);
    var treeViewOption = this.viewSelectComboBox.createOption(WebInspector.UIString("Tree (Top Down)"), "", WebInspector.CPUProfileView._TypeTree);
    this.viewSelectComboBox.select(this._viewType.get() === WebInspector.CPUProfileView._TypeHeavy ? heavyViewOption : treeViewOption);

    this.percentButton = new WebInspector.StatusBarButton("", "percent-time-status-bar-item");
    this.percentButton.addEventListener("click", this._percentClicked, this);

    this.focusButton = new WebInspector.StatusBarButton(WebInspector.UIString("Focus selected function."), "focus-profile-node-status-bar-item");
    this.focusButton.setEnabled(false);
    this.focusButton.addEventListener("click", this._focusClicked, this);

    this.excludeButton = new WebInspector.StatusBarButton(WebInspector.UIString("Exclude selected function."), "exclude-profile-node-status-bar-item");
    this.excludeButton.setEnabled(false);
    this.excludeButton.addEventListener("click", this._excludeClicked, this);

    this.resetButton = new WebInspector.StatusBarButton(WebInspector.UIString("Restore all functions."), "reset-profile-status-bar-item");
    this.resetButton.visible = false;
    this.resetButton.addEventListener("click", this._resetClicked, this);

    this.profileHead = /** @type {?ProfilerAgent.CPUProfileNode} */ (null);
    this.profile = profile;

    this._linkifier = new WebInspector.Linkifier(new WebInspector.Linkifier.DefaultFormatter(30));

    ProfilerAgent.getCPUProfile(this.profile.uid, this._getCPUProfileCallback.bind(this));
}

WebInspector.CPUProfileView._TypeTree = "Tree";
WebInspector.CPUProfileView._TypeHeavy = "Heavy";

WebInspector.CPUProfileView.prototype = {
    _revealProfilerNode: function(event)
    {
        var current = this.profileDataGridTree.children[0];

        while (current && current.profileNode !== event.data)
            current = current.traverseNextNode(false, null, false);

        if (current)
            current.revealAndSelect();
    },

    /**
     * @param {?Protocol.Error} error
     * @param {ProfilerAgent.CPUProfile} profile
     */
    _getCPUProfileCallback: function(error, profile)
    {
        if (error)
            return;

        if (!profile.head) {
            // Profiling was tentatively terminated with the "Clear all profiles." button.
            return;
        }
        this.profileHead = profile.head;
        this.samples = profile.samples;

        if (profile.idleTime)
            this._injectIdleTimeNode(profile);

        this._assignParentsInProfile();
        if (this.samples)
            this._buildIdToNodeMap();
        this._changeView();
        this._updatePercentButton();
        if (this.flameChart)
            this.flameChart.update();
    },

    statusBarItems: function()
    {
        return [this.viewSelectComboBox.element, this.percentButton.element, this.focusButton.element, this.excludeButton.element, this.resetButton.element];
    },

    /**
     * @return {!WebInspector.ProfileDataGridTree}
     */
    _getBottomUpProfileDataGridTree: function()
    {
        if (!this._bottomUpProfileDataGridTree)
            this._bottomUpProfileDataGridTree = new WebInspector.BottomUpProfileDataGridTree(this, this.profileHead);
        return this._bottomUpProfileDataGridTree;
    },

    /**
     * @return {!WebInspector.ProfileDataGridTree}
     */
    _getTopDownProfileDataGridTree: function()
    {
        if (!this._topDownProfileDataGridTree)
            this._topDownProfileDataGridTree = new WebInspector.TopDownProfileDataGridTree(this, this.profileHead);
        return this._topDownProfileDataGridTree;
    },

    willHide: function()
    {
        this._currentSearchResultIndex = -1;
    },

    refresh: function()
    {
        var selectedProfileNode = this.dataGrid.selectedNode ? this.dataGrid.selectedNode.profileNode : null;

        this.dataGrid.rootNode().removeChildren();

        var children = this.profileDataGridTree.children;
        var count = children.length;

        for (var index = 0; index < count; ++index)
            this.dataGrid.rootNode().appendChild(children[index]);

        if (selectedProfileNode)
            selectedProfileNode.selected = true;
    },

    refreshVisibleData: function()
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
        this.refreshVisibleData();
    },

    searchCanceled: function()
    {
        if (this._searchResults) {
            for (var i = 0; i < this._searchResults.length; ++i) {
                var profileNode = this._searchResults[i].profileNode;

                delete profileNode._searchMatchedSelfColumn;
                delete profileNode._searchMatchedTotalColumn;
                delete profileNode._searchMatchedAverageColumn;
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

        var greaterThan = (query.startsWith(">"));
        var lessThan = (query.startsWith("<"));
        var equalTo = (query.startsWith("=") || ((greaterThan || lessThan) && query.indexOf("=") === 1));
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

        var matcher = new RegExp(query.escapeForRegExp(), "i");

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

            if (profileDataGridNode.functionName.match(matcher) || (profileDataGridNode.url && profileDataGridNode.url.match(matcher)))
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

    _changeView: function()
    {
        if (!this.profile)
            return;

        switch (this.viewSelectComboBox.selectedOption().value) {
        case WebInspector.CPUProfileView._TypeTree:
            this.profileDataGridTree = this._getTopDownProfileDataGridTree();
            this._sortProfile();
            this._viewType.set(WebInspector.CPUProfileView._TypeTree);
            break;
        case WebInspector.CPUProfileView._TypeHeavy:
            this.profileDataGridTree = this._getBottomUpProfileDataGridTree();
            this._sortProfile();
            this._viewType.set(WebInspector.CPUProfileView._TypeHeavy);
        }

        if (!this.currentQuery || !this._searchFinishedCallback || !this._searchResults)
            return;

        // The current search needs to be performed again. First negate out previous match
        // count by calling the search finished callback with a negative number of matches.
        // Then perform the search again the with same query and callback.
        this._searchFinishedCallback(this, -this._searchResults.length);
        this.performSearch(this.currentQuery, this._searchFinishedCallback);
    },

    _percentClicked: function(event)
    {
        var currentState = this.showSelfTimeAsPercent.get() && this.showTotalTimeAsPercent.get() && this.showAverageTimeAsPercent.get();
        this.showSelfTimeAsPercent.set(!currentState);
        this.showTotalTimeAsPercent.set(!currentState);
        this.showAverageTimeAsPercent.set(!currentState);
        this.refreshShowAsPercents();
    },

    _updatePercentButton: function()
    {
        if (this.showSelfTimeAsPercent.get() && this.showTotalTimeAsPercent.get() && this.showAverageTimeAsPercent.get()) {
            this.percentButton.title = WebInspector.UIString("Show absolute total and self times.");
            this.percentButton.toggled = true;
        } else {
            this.percentButton.title = WebInspector.UIString("Show total and self times as percentages.");
            this.percentButton.toggled = false;
        }
    },

    _focusClicked: function(event)
    {
        if (!this.dataGrid.selectedNode)
            return;

        this.resetButton.visible = true;
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

        this.resetButton.visible = true;
        this.profileDataGridTree.exclude(selectedNode);
        this.refresh();
        this.refreshVisibleData();
    },

    _resetClicked: function(event)
    {
        this.resetButton.visible = false;
        this.profileDataGridTree.restore();
        this._linkifier.reset();
        this.refresh();
        this.refreshVisibleData();
    },

    _dataGridNodeSelected: function(node)
    {
        this.focusButton.setEnabled(true);
        this.excludeButton.setEnabled(true);
    },

    _dataGridNodeDeselected: function(node)
    {
        this.focusButton.setEnabled(false);
        this.excludeButton.setEnabled(false);
    },

    _sortProfile: function()
    {
        var sortAscending = this.dataGrid.isSortOrderAscending();
        var sortColumnIdentifier = this.dataGrid.sortColumnIdentifier();
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

    _mouseDownInDataGrid: function(event)
    {
        if (event.detail < 2)
            return;

        var cell = event.target.enclosingNodeOrSelfWithNodeName("td");
        if (!cell || (!cell.hasStyleClass("total-column") && !cell.hasStyleClass("self-column") && !cell.hasStyleClass("average-column")))
            return;

        if (cell.hasStyleClass("total-column"))
            this.showTotalTimeAsPercent.set(!this.showTotalTimeAsPercent.get());
        else if (cell.hasStyleClass("self-column"))
            this.showSelfTimeAsPercent.set(!this.showSelfTimeAsPercent.get());
        else if (cell.hasStyleClass("average-column"))
            this.showAverageTimeAsPercent.set(!this.showAverageTimeAsPercent.get());

        this.refreshShowAsPercents();

        event.consume(true);
    },

    _assignParentsInProfile: function()
    {
        var head = this.profileHead;
        head.parent = null;
        head.head = null;
        var nodesToTraverse = [ { parent: head, children: head.children } ];
        while (nodesToTraverse.length > 0) {
            var pair = nodesToTraverse.pop();
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
    },

    _buildIdToNodeMap: function()
    {
        var idToNode = this._idToNode = {};
        var stack = [this.profileHead];
        while (stack.length) {
            var node = stack.pop();
            idToNode[node.id] = node;
            for (var i = 0; i < node.children.length; i++)
                stack.push(node.children[i]);
        }
    },

    /**
     * @param {ProfilerAgent.CPUProfile} profile
     */
    _injectIdleTimeNode: function(profile)
    {
        var idleTime = profile.idleTime;
        var nodes = profile.head.children;

        var programNode = {selfTime: 0};
        for (var i = nodes.length - 1; i >= 0; --i) {
            if (nodes[i].functionName === "(program)") {
                programNode = nodes[i];
                break;
            }
        }
        var programTime = programNode.selfTime;
        if (idleTime > programTime)
            idleTime = programTime;
        programTime = programTime - idleTime;
        programNode.selfTime = programTime;
        programNode.totalTime = programTime;
        var idleNode = {
            functionName: "(idle)",
            url: null,
            lineNumber: 0,
            totalTime: idleTime,
            selfTime: idleTime,
            numberOfCalls: 0,
            visible: true,
            callUID: 0,
            children: []
        };
        nodes.push(idleNode);
    },

    __proto__: WebInspector.View.prototype
}

/**
 * @constructor
 * @extends {WebInspector.ProfileType}
 * @implements {ProfilerAgent.Dispatcher}
 */
WebInspector.CPUProfileType = function()
{
    WebInspector.ProfileType.call(this, WebInspector.CPUProfileType.TypeId, WebInspector.UIString("Collect JavaScript CPU Profile"));
    InspectorBackend.registerProfilerDispatcher(this);
    this._recording = false;
    WebInspector.CPUProfileType.instance = this;
}

WebInspector.CPUProfileType.TypeId = "CPU";

WebInspector.CPUProfileType.prototype = {
    get buttonTooltip()
    {
        return this._recording ? WebInspector.UIString("Stop CPU profiling.") : WebInspector.UIString("Start CPU profiling.");
    },

    /**
     * @override
     * @return {boolean}
     */
    buttonClicked: function()
    {
        if (this._recording) {
            this.stopRecordingProfile();
            return false;
        } else {
            this.startRecordingProfile();
            return true;
        }
    },

    get treeItemTitle()
    {
        return WebInspector.UIString("CPU PROFILES");
    },

    get description()
    {
        return WebInspector.UIString("CPU profiles show where the execution time is spent in your page's JavaScript functions.");
    },

    /**
     * @param {ProfilerAgent.ProfileHeader} profileHeader
     */
    addProfileHeader: function(profileHeader)
    {
        this.addProfile(this.createProfile(profileHeader));
    },

    isRecordingProfile: function()
    {
        return this._recording;
    },

    startRecordingProfile: function()
    {
        this._recording = true;
        WebInspector.userMetrics.ProfilesCPUProfileTaken.record();
        ProfilerAgent.start();
    },

    stopRecordingProfile: function()
    {
        this._recording = false;
        ProfilerAgent.stop();
    },

    /**
     * @param {boolean} isProfiling
     */
    setRecordingProfile: function(isProfiling)
    {
        this._recording = isProfiling;
    },

    /**
     * @override
     * @param {string=} title
     * @return {!WebInspector.ProfileHeader}
     */
    createTemporaryProfile: function(title)
    {
        title = title || WebInspector.UIString("Recording\u2026");
        return new WebInspector.CPUProfileHeader(this, title);
    },

    /**
     * @override
     * @param {ProfilerAgent.ProfileHeader} profile
     * @return {!WebInspector.ProfileHeader}
     */
    createProfile: function(profile)
    {
        return new WebInspector.CPUProfileHeader(this, profile.title, profile.uid);
    },

    /**
     * @override
     * @param {!WebInspector.ProfileHeader} profile
     */
    removeProfile: function(profile)
    {
        WebInspector.ProfileType.prototype.removeProfile.call(this, profile);
        if (!profile.isTemporary)
            ProfilerAgent.removeProfile(this.id, profile.uid);
    },

    /**
     * @override
     * @param {function(this:WebInspector.ProfileType, ?string, Array.<ProfilerAgent.ProfileHeader>)} populateCallback
     */
    _requestProfilesFromBackend: function(populateCallback)
    {
        ProfilerAgent.getProfileHeaders(populateCallback);
    },

    /**
     * @override
     */
    resetProfiles: function()
    {
        this._reset();
    },

    /** @deprecated To be removed from the protocol */
    addHeapSnapshotChunk: function(uid, chunk)
    {
        throw new Error("Never called");
    },

    /** @deprecated To be removed from the protocol */
    finishHeapSnapshot: function(uid)
    {
        throw new Error("Never called");
    },

    /** @deprecated To be removed from the protocol */
    reportHeapSnapshotProgress: function(done, total)
    {
        throw new Error("Never called");
    },

    __proto__: WebInspector.ProfileType.prototype
}

/**
 * @constructor
 * @extends {WebInspector.ProfileHeader}
 * @param {!WebInspector.CPUProfileType} type
 * @param {string} title
 * @param {number=} uid
 */
WebInspector.CPUProfileHeader = function(type, title, uid)
{
    WebInspector.ProfileHeader.call(this, type, title, uid);
}

WebInspector.CPUProfileHeader.prototype = {
    /**
     * @override
     */
    createSidebarTreeElement: function()
    {
        return new WebInspector.ProfileSidebarTreeElement(this, WebInspector.UIString("Profile %d"), "profile-sidebar-tree-item");
    },

    /**
     * @override
     * @param {WebInspector.ProfilesPanel} profilesPanel
     */
    createView: function(profilesPanel)
    {
        return new WebInspector.CPUProfileView(this);
    },

    __proto__: WebInspector.ProfileHeader.prototype
}
