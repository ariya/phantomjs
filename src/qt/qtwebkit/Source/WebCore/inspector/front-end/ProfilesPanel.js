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

const UserInitiatedProfileName = "org.webkit.profiles.user-initiated";

/**
 * @constructor
 * @extends {WebInspector.Object}
 * @param {string} id
 * @param {string} name
 */
WebInspector.ProfileType = function(id, name)
{
    this._id = id;
    this._name = name;
    /** @type {!Array.<!WebInspector.ProfileHeader>} */
    this._profiles = [];
    this._profilesIdMap = {};
    /** @type {WebInspector.SidebarSectionTreeElement} */
    this.treeElement = null;
}

WebInspector.ProfileType.Events = {
    AddProfileHeader: "add-profile-header",
    RemoveProfileHeader: "remove-profile-header",
    ProgressUpdated: "progress-updated",
    ViewUpdated: "view-updated"
}

WebInspector.ProfileType.prototype = {
    statusBarItems: function()
    {
        return [];
    },

    get buttonTooltip()
    {
        return "";
    },

    get id()
    {
        return this._id;
    },

    get treeItemTitle()
    {
        return this._name;
    },

    get name()
    {
        return this._name;
    },

    /**
     * @return {boolean}
     */
    buttonClicked: function()
    {
        return false;
    },

    get description()
    {
        return "";
    },

    /**
     * @return {boolean}
     */
    isInstantProfile: function()
    {
        return false;
    },

    /**
     * @return {!Array.<!WebInspector.ProfileHeader>}
     */
    getProfiles: function()
    {
        return this._profiles.filter(function(profile) { return !profile.isTemporary; });
    },

    /**
     * @return {Element}
     */
    decorationElement: function()
    {
        return null;
    },

    /**
     * @nosideeffects
     * @param {number} uid
     * @return {WebInspector.ProfileHeader}
     */
    getProfile: function(uid)
    {
        return this._profilesIdMap[this._makeKey(uid)];
    },

    // Must be implemented by subclasses.
    /**
     * @param {string=} title
     * @return {!WebInspector.ProfileHeader}
     */
    createTemporaryProfile: function(title)
    {
        throw new Error("Needs implemented.");
    },

    /**
     * @param {ProfilerAgent.ProfileHeader} profile
     * @return {!WebInspector.ProfileHeader}
     */
    createProfile: function(profile)
    {
        throw new Error("Not supported for " + this._name + " profiles.");
    },

    /**
     * @nosideeffects
     * @param {number} id
     * @return {string}
     */
    _makeKey: function(id)
    {
        return id + '/' + escape(this.id);
    },

    /**
     * @param {!WebInspector.ProfileHeader} profile
     */
    addProfile: function(profile)
    {
        this._profiles.push(profile);
        // FIXME: uid only based key should be enough.
        this._profilesIdMap[this._makeKey(profile.uid)] = profile;
        this.dispatchEventToListeners(WebInspector.ProfileType.Events.AddProfileHeader, profile);
    },

    /**
     * @param {!WebInspector.ProfileHeader} profile
     */
    removeProfile: function(profile)
    {
        for (var i = 0; i < this._profiles.length; ++i) {
            if (this._profiles[i].uid === profile.uid) {
                this._profiles.splice(i, 1);
                break;
            }
        }
        delete this._profilesIdMap[this._makeKey(profile.uid)];
    },

    /**
     * @nosideeffects
     * @return {WebInspector.ProfileHeader}
     */
    findTemporaryProfile: function()
    {
        for (var i = 0; i < this._profiles.length; ++i) {
            if (this._profiles[i].isTemporary)
                return this._profiles[i];
        }
        return null;
    },

    _reset: function()
    {
        var profiles = this._profiles.slice(0);
        for (var i = 0; i < profiles.length; ++i) {
            var profile = profiles[i];
            var view = profile.existingView();
            if (view) {
                view.detach();
                if ("dispose" in view)
                    view.dispose();
            }
            this.dispatchEventToListeners(WebInspector.ProfileType.Events.RemoveProfileHeader, profile);
        }
        this.treeElement.removeChildren();
        this._profiles = [];
        this._profilesIdMap = {};
    },

    /**
     * @param {function(this:WebInspector.ProfileType, ?string, !Array.<!ProfilerAgent.ProfileHeader>)} populateCallback
     */
    _requestProfilesFromBackend: function(populateCallback)
    {
    },

    _populateProfiles: function()
    {
        /**
         * @param {?string} error
         * @param {!Array.<!ProfilerAgent.ProfileHeader>} profileHeaders
         */
        function populateCallback(error, profileHeaders) {
            if (error)
                return;
            profileHeaders.sort(function(a, b) { return a.uid - b.uid; });
            var count = profileHeaders.length;
            for (var i = 0; i < count; ++i)
                this.addProfile(this.createProfile(profileHeaders[i]));
        }
        this._requestProfilesFromBackend(populateCallback.bind(this));
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 * @param {!WebInspector.ProfileType} profileType
 * @param {string} title
 * @param {number=} uid
 */
WebInspector.ProfileHeader = function(profileType, title, uid)
{
    this._profileType = profileType;
    this.title = title;
    this.isTemporary = uid === undefined;
    this.uid = this.isTemporary ? -1 : uid;
    this._fromFile = false;
}

WebInspector.ProfileHeader.prototype = {
    /**
     * @return {!WebInspector.ProfileType}
     */
    profileType: function()
    {
        return this._profileType;
    },

    /**
     * Must be implemented by subclasses.
     * @return {WebInspector.ProfileSidebarTreeElement}
     */
    createSidebarTreeElement: function()
    {
        throw new Error("Needs implemented.");
    },

    /**
     * @return {?WebInspector.View}
     */
    existingView: function()
    {
        return this._view;
    },

    /**
     * @param {!WebInspector.ProfilesPanel} panel
     * @return {!WebInspector.View}
     */
    view: function(panel)
    {
        if (!this._view)
            this._view = this.createView(panel);
        return this._view;
    },

    /**
     * @param {!WebInspector.ProfilesPanel} panel
     * @return {!WebInspector.View}
     */
    createView: function(panel)
    {
        throw new Error("Not implemented.");
    },

    dispose: function()
    {
    },

    /**
     * @param {Function} callback
     */
    load: function(callback)
    {
    },

    /**
     * @return {boolean}
     */
    canSaveToFile: function()
    {
        return false;
    },

    saveToFile: function()
    {
        throw new Error("Needs implemented");
    },

    /**
     * @param {File} file
     */
    loadFromFile: function(file)
    {
        throw new Error("Needs implemented");
    },

    /**
     * @return {boolean}
     */
    fromFile: function()
    {
        return this._fromFile;
    }
}

/**
 * @constructor
 * @extends {WebInspector.Panel}
 * @implements {WebInspector.ContextMenu.Provider}
 * @param {string=} name
 * @param {WebInspector.ProfileType=} type
 */
WebInspector.ProfilesPanel = function(name, type)
{
    // If the name is not specified the ProfilesPanel works in multi-profile mode.
    var singleProfileMode = typeof name !== "undefined";
    name = name || "profiles";
    WebInspector.Panel.call(this, name);
    this.registerRequiredCSS("panelEnablerView.css");
    this.registerRequiredCSS("heapProfiler.css");
    this.registerRequiredCSS("profilesPanel.css");

    this.createSidebarViewWithTree();

    this.profilesItemTreeElement = new WebInspector.ProfilesSidebarTreeElement(this);
    this.sidebarTree.appendChild(this.profilesItemTreeElement);

    this._singleProfileMode = singleProfileMode;
    this._profileTypesByIdMap = {};

    var panelEnablerHeading = WebInspector.UIString("You need to enable profiling before you can use the Profiles panel.");
    var panelEnablerDisclaimer = WebInspector.UIString("Enabling profiling will make scripts run slower.");
    var panelEnablerButton = WebInspector.UIString("Enable Profiling");
    this.panelEnablerView = new WebInspector.PanelEnablerView(name, panelEnablerHeading, panelEnablerDisclaimer, panelEnablerButton);
    this.panelEnablerView.addEventListener("enable clicked", this.enableProfiler, this);

    this.profileViews = document.createElement("div");
    this.profileViews.id = "profile-views";
    this.splitView.mainElement.appendChild(this.profileViews);

    this._statusBarButtons = [];

    this.enableToggleButton = new WebInspector.StatusBarButton("", "enable-toggle-status-bar-item");
    if (Capabilities.profilerCausesRecompilation) {
        this._statusBarButtons.push(this.enableToggleButton);
        this.enableToggleButton.addEventListener("click", this._onToggleProfiling, this);
    }
    this.recordButton = new WebInspector.StatusBarButton("", "record-profile-status-bar-item");
    this.recordButton.addEventListener("click", this.toggleRecordButton, this);
    this._statusBarButtons.push(this.recordButton);

    this.clearResultsButton = new WebInspector.StatusBarButton(WebInspector.UIString("Clear all profiles."), "clear-status-bar-item");
    this.clearResultsButton.addEventListener("click", this._clearProfiles, this);
    this._statusBarButtons.push(this.clearResultsButton);

    this._profileTypeStatusBarItemsContainer = document.createElement("div");
    this._profileTypeStatusBarItemsContainer.className = "status-bar-items";

    this._profileViewStatusBarItemsContainer = document.createElement("div");
    this._profileViewStatusBarItemsContainer.className = "status-bar-items";

    this._profilerEnabled = !Capabilities.profilerCausesRecompilation || WebInspector.settings.profilerEnabled.get();

    if (singleProfileMode) {
        this._launcherView = this._createLauncherView();
        this._registerProfileType(/** @type {!WebInspector.ProfileType} */ (type));
        this._selectedProfileType = type;
        this._updateProfileTypeSpecificUI();
    } else {
        this._launcherView = new WebInspector.MultiProfileLauncherView(this);
        this._launcherView.addEventListener(WebInspector.MultiProfileLauncherView.EventTypes.ProfileTypeSelected, this._onProfileTypeSelected, this);

        this._registerProfileType(new WebInspector.CPUProfileType());
        if (!WebInspector.WorkerManager.isWorkerFrontend())
            this._registerProfileType(new WebInspector.CSSSelectorProfileType());
        if (Capabilities.heapProfilerPresent)
            this._registerProfileType(new WebInspector.HeapSnapshotProfileType());
        if (!WebInspector.WorkerManager.isWorkerFrontend() && WebInspector.experimentsSettings.canvasInspection.isEnabled())
            this._registerProfileType(new WebInspector.CanvasProfileType());
    }

    this._reset();

    this._createFileSelectorElement();
    this.element.addEventListener("contextmenu", this._handleContextMenuEvent.bind(this), true);

    WebInspector.ContextMenu.registerProvider(this);
}

WebInspector.ProfilesPanel.prototype = {
    _createFileSelectorElement: function()
    {
        if (this._fileSelectorElement)
            this.element.removeChild(this._fileSelectorElement);
        this._fileSelectorElement = WebInspector.createFileSelectorElement(this._loadFromFile.bind(this));
        this.element.appendChild(this._fileSelectorElement);
    },

    /**
     * @return {!WebInspector.ProfileLauncherView}
     */
    _createLauncherView: function()
    {
        return new WebInspector.ProfileLauncherView(this);
    },

    /**
     * @param {!File} file
     */
    _loadFromFile: function(file)
    {
        if (!file.name.endsWith(".heapsnapshot")) {
            WebInspector.log(WebInspector.UIString("Only heap snapshots from files with extension '.heapsnapshot' can be loaded."));
            return;
        }

        var profileType = this.getProfileType(WebInspector.HeapSnapshotProfileType.TypeId);
        if (!!profileType.findTemporaryProfile()) {
            WebInspector.log(WebInspector.UIString("Can't load profile when other profile is recording."));
            return;
        }

        var temporaryProfile = profileType.createTemporaryProfile(WebInspector.ProfilesPanelDescriptor.UserInitiatedProfileName + "." + file.name);
        profileType.addProfile(temporaryProfile);

        temporaryProfile._fromFile = true;
        temporaryProfile.loadFromFile(file);
        this._createFileSelectorElement();
    },

    statusBarItems: function()
    {
        return this._statusBarButtons.select("element").concat(this._profileTypeStatusBarItemsContainer, this._profileViewStatusBarItemsContainer);
    },

    toggleRecordButton: function()
    {
        var isProfiling = this._selectedProfileType.buttonClicked();
        this.setRecordingProfile(this._selectedProfileType.id, isProfiling);
    },

    _populateAllProfiles: function()
    {
        if (!this._profilerEnabled || this._profilesWereRequested)
            return;
        this._profilesWereRequested = true;
        for (var typeId in this._profileTypesByIdMap)
            this._profileTypesByIdMap[typeId]._populateProfiles();
    },

    wasShown: function()
    {
        WebInspector.Panel.prototype.wasShown.call(this);
        this._populateAllProfiles();
    },

    _profilerWasEnabled: function()
    {
        if (this._profilerEnabled)
            return;

        this._profilerEnabled = true;

        this._reset();
        if (this.isShowing())
            this._populateAllProfiles();
    },

    _profilerWasDisabled: function()
    {
        if (!this._profilerEnabled)
            return;

        this._profilerEnabled = false;
        this._reset();
    },

    /**
     * @param {WebInspector.Event} event
     */
    _onProfileTypeSelected: function(event)
    {
        this._selectedProfileType = /** @type {!WebInspector.ProfileType} */ (event.data);
        this._updateProfileTypeSpecificUI();
    },

    _updateProfileTypeSpecificUI: function()
    {
        this.recordButton.title = this._selectedProfileType.buttonTooltip;

        this._profileTypeStatusBarItemsContainer.removeChildren();
        var statusBarItems = this._selectedProfileType.statusBarItems();
        if (statusBarItems) {
            for (var i = 0; i < statusBarItems.length; ++i)
                this._profileTypeStatusBarItemsContainer.appendChild(statusBarItems[i]);
        }
        this._resize(this.splitView.sidebarWidth());
    },

    _reset: function()
    {
        WebInspector.Panel.prototype.reset.call(this);

        for (var typeId in this._profileTypesByIdMap)
            this._profileTypesByIdMap[typeId]._reset();

        delete this.visibleView;
        delete this.currentQuery;
        this.searchCanceled();

        this._profileGroups = {};
        this._profilesWereRequested = false;
        this.recordButton.toggled = false;
        if (this._selectedProfileType)
            this.recordButton.title = this._selectedProfileType.buttonTooltip;
        this._launcherView.profileFinished();

        this.sidebarTreeElement.removeStyleClass("some-expandable");

        this.profileViews.removeChildren();
        this._profileViewStatusBarItemsContainer.removeChildren();

        this.removeAllListeners();

        this._updateInterface();
        this.profilesItemTreeElement.select();
        this._showLauncherView();
    },

    _showLauncherView: function()
    {
        this.closeVisibleView();
        this._profileViewStatusBarItemsContainer.removeChildren();
        this._launcherView.show(this.splitView.mainElement);
        this.visibleView = this._launcherView;
    },

    _clearProfiles: function()
    {
        ProfilerAgent.clearProfiles();
        HeapProfilerAgent.clearProfiles();
        this._reset();
    },

    _garbageCollectButtonClicked: function()
    {
        HeapProfilerAgent.collectGarbage();
    },

    /**
     * @param {!WebInspector.ProfileType} profileType
     */
    _registerProfileType: function(profileType)
    {
        this._profileTypesByIdMap[profileType.id] = profileType;
        this._launcherView.addProfileType(profileType);
        profileType.treeElement = new WebInspector.SidebarSectionTreeElement(profileType.treeItemTitle, null, true);
        profileType.treeElement.hidden = !this._singleProfileMode;
        this.sidebarTree.appendChild(profileType.treeElement);
        profileType.treeElement.childrenListElement.addEventListener("contextmenu", this._handleContextMenuEvent.bind(this), true);
        function onAddProfileHeader(event)
        {
            this._addProfileHeader(event.data);
        }
        function onRemoveProfileHeader(event)
        {
            this._removeProfileHeader(event.data);
        }
        function onProgressUpdated(event)
        {
            this._reportProfileProgress(event.data.profile, event.data.done, event.data.total);
        }
        profileType.addEventListener(WebInspector.ProfileType.Events.ViewUpdated, this._updateProfileTypeSpecificUI, this);
        profileType.addEventListener(WebInspector.ProfileType.Events.AddProfileHeader, onAddProfileHeader, this);
        profileType.addEventListener(WebInspector.ProfileType.Events.RemoveProfileHeader, onRemoveProfileHeader, this);
        profileType.addEventListener(WebInspector.ProfileType.Events.ProgressUpdated, onProgressUpdated, this);
    },

    /**
     * @param {Event} event
     */
    _handleContextMenuEvent: function(event)
    {
        var element = event.srcElement;
        while (element && !element.treeElement && element !== this.element)
            element = element.parentElement;
        if (!element)
            return;
        if (element.treeElement && element.treeElement.handleContextMenuEvent) {
            element.treeElement.handleContextMenuEvent(event, this);
            return;
        }
        if (element !== this.element || event.srcElement === this.sidebarElement) {
            var contextMenu = new WebInspector.ContextMenu(event);
            if (this.visibleView instanceof WebInspector.HeapSnapshotView)
                this.visibleView.populateContextMenu(contextMenu, event);
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Load heap snapshot\u2026" : "Load Heap Snapshot\u2026"), this._fileSelectorElement.click.bind(this._fileSelectorElement));
            contextMenu.show();
        }

    },

    /**
     * @nosideeffects
     * @param {string} text
     * @param {string} profileTypeId
     * @return {string}
     */
    _makeTitleKey: function(text, profileTypeId)
    {
        return escape(text) + '/' + escape(profileTypeId);
    },

    /**
     * @param {!WebInspector.ProfileHeader} profile
     */
    _addProfileHeader: function(profile)
    {
        if (!profile.isTemporary)
            this._removeTemporaryProfile(profile.profileType().id);

        var profileType = profile.profileType();
        var typeId = profileType.id;
        var sidebarParent = profileType.treeElement;
        sidebarParent.hidden = false;
        var small = false;
        var alternateTitle;

        if (!WebInspector.ProfilesPanelDescriptor.isUserInitiatedProfile(profile.title) && !profile.isTemporary) {
            var profileTitleKey = this._makeTitleKey(profile.title, typeId);
            if (!(profileTitleKey in this._profileGroups))
                this._profileGroups[profileTitleKey] = [];

            var group = this._profileGroups[profileTitleKey];
            group.push(profile);
            if (group.length === 2) {
                // Make a group TreeElement now that there are 2 profiles.
                group._profilesTreeElement = new WebInspector.ProfileGroupSidebarTreeElement(this, profile.title);

                // Insert at the same index for the first profile of the group.
                var index = sidebarParent.children.indexOf(group[0]._profilesTreeElement);
                sidebarParent.insertChild(group._profilesTreeElement, index);

                // Move the first profile to the group.
                var selected = group[0]._profilesTreeElement.selected;
                sidebarParent.removeChild(group[0]._profilesTreeElement);
                group._profilesTreeElement.appendChild(group[0]._profilesTreeElement);
                if (selected)
                    group[0]._profilesTreeElement.revealAndSelect();

                group[0]._profilesTreeElement.small = true;
                group[0]._profilesTreeElement.mainTitle = WebInspector.UIString("Run %d", 1);

                this.sidebarTreeElement.addStyleClass("some-expandable");
            }

            if (group.length >= 2) {
                sidebarParent = group._profilesTreeElement;
                alternateTitle = WebInspector.UIString("Run %d", group.length);
                small = true;
            }
        }

        var profileTreeElement = profile.createSidebarTreeElement();
        profile.sidebarElement = profileTreeElement;
        profileTreeElement.small = small;
        if (alternateTitle)
            profileTreeElement.mainTitle = alternateTitle;
        profile._profilesTreeElement = profileTreeElement;

        sidebarParent.appendChild(profileTreeElement);
        if (!profile.isTemporary) {
            if (!this.visibleView)
                this._showProfile(profile);
            this.dispatchEventToListeners("profile added", {
                type: typeId
            });
        }
    },

    /**
     * @param {!WebInspector.ProfileHeader} profile
     */
    _removeProfileHeader: function(profile)
    {
        profile.dispose();
        profile.profileType().removeProfile(profile);

        var sidebarParent = profile.profileType().treeElement;
        var profileTitleKey = this._makeTitleKey(profile.title, profile.profileType().id);
        var group = this._profileGroups[profileTitleKey];
        if (group) {
            group.splice(group.indexOf(profile), 1);
            if (group.length === 1) {
                // Move the last profile out of its group and remove the group.
                var index = sidebarParent.children.indexOf(group._profilesTreeElement);
                sidebarParent.insertChild(group[0]._profilesTreeElement, index);
                group[0]._profilesTreeElement.small = false;
                group[0]._profilesTreeElement.mainTitle = group[0].title;
                sidebarParent.removeChild(group._profilesTreeElement);
            }
            if (group.length !== 0)
                sidebarParent = group._profilesTreeElement;
            else
                delete this._profileGroups[profileTitleKey];
        }
        sidebarParent.removeChild(profile._profilesTreeElement);

        // No other item will be selected if there aren't any other profiles, so
        // make sure that view gets cleared when the last profile is removed.
        if (!sidebarParent.children.length) {
            this.profilesItemTreeElement.select();
            this._showLauncherView();
            sidebarParent.hidden = !this._singleProfileMode;
        }
    },

    /**
     * @param {!WebInspector.ProfileHeader} profile
     */
    _showProfile: function(profile)
    {
        if (!profile || profile.isTemporary)
            return;

        var view = profile.view(this);
        if (view === this.visibleView)
            return;

        this.closeVisibleView();

        view.show(this.profileViews);

        profile._profilesTreeElement._suppressOnSelect = true;
        profile._profilesTreeElement.revealAndSelect();
        delete profile._profilesTreeElement._suppressOnSelect;

        this.visibleView = view;

        this._profileViewStatusBarItemsContainer.removeChildren();

        var statusBarItems = view.statusBarItems();
        if (statusBarItems)
            for (var i = 0; i < statusBarItems.length; ++i)
                this._profileViewStatusBarItemsContainer.appendChild(statusBarItems[i]);
    },

    /**
     * @param {HeapProfilerAgent.HeapSnapshotObjectId} snapshotObjectId
     * @param {string} viewName
     */
    showObject: function(snapshotObjectId, viewName)
    {
        var heapProfiles = this.getProfileType(WebInspector.HeapSnapshotProfileType.TypeId).getProfiles();
        for (var i = 0; i < heapProfiles.length; i++) {
            var profile = heapProfiles[i];
            // FIXME: allow to choose snapshot if there are several options.
            if (profile.maxJSObjectId >= snapshotObjectId) {
                this._showProfile(profile);
                var view = profile.view(this);
                view.changeView(viewName, function() {
                    view.dataGrid.highlightObjectByHeapSnapshotId(snapshotObjectId);
                });
                break;
            }
        }
    },

    /**
     * @param {string} typeId
     */
    _createTemporaryProfile: function(typeId)
    {
        var type = this.getProfileType(typeId);
        if (!type.findTemporaryProfile())
            type.addProfile(type.createTemporaryProfile());
    },

    /**
     * @param {string} typeId
     */
    _removeTemporaryProfile: function(typeId)
    {
        var temporaryProfile = this.getProfileType(typeId).findTemporaryProfile();
        if (!!temporaryProfile)
            this._removeProfileHeader(temporaryProfile);
    },

    /**
     * @param {string} typeId
     * @param {number} uid
     */
    getProfile: function(typeId, uid)
    {
        return this.getProfileType(typeId).getProfile(uid);
    },

    /**
     * @param {WebInspector.View} view
     */
    showView: function(view)
    {
        this._showProfile(view.profile);
    },

    /**
     * @param {string} typeId
     */
    getProfileType: function(typeId)
    {
        return this._profileTypesByIdMap[typeId];
    },

    /**
     * @param {string} typeId
     * @param {string} uid
     */
    showProfile: function(typeId, uid)
    {
        this._showProfile(this.getProfile(typeId, Number(uid)));
    },

    closeVisibleView: function()
    {
        if (this.visibleView)
            this.visibleView.detach();
        delete this.visibleView;
    },

    /**
     * @param {string} query
     */
    performSearch: function(query)
    {
        this.searchCanceled();

        var searchableViews = this._searchableViews();
        if (!searchableViews || !searchableViews.length)
            return;

        var visibleView = this.visibleView;

        var matchesCountUpdateTimeout = null;

        function updateMatchesCount()
        {
            WebInspector.searchController.updateSearchMatchesCount(this._totalSearchMatches, this);
            WebInspector.searchController.updateCurrentMatchIndex(this._currentSearchResultIndex, this);
            matchesCountUpdateTimeout = null;
        }

        function updateMatchesCountSoon()
        {
            if (matchesCountUpdateTimeout)
                return;
            // Update the matches count every half-second so it doesn't feel twitchy.
            matchesCountUpdateTimeout = setTimeout(updateMatchesCount.bind(this), 500);
        }

        function finishedCallback(view, searchMatches)
        {
            if (!searchMatches)
                return;

            this._totalSearchMatches += searchMatches;
            this._searchResults.push(view);

            if (this.searchMatchFound)
                this.searchMatchFound(view, searchMatches);

            updateMatchesCountSoon.call(this);

            if (view === visibleView)
                view.jumpToFirstSearchResult();
        }

        var i = 0;
        var panel = this;
        var boundFinishedCallback = finishedCallback.bind(this);
        var chunkIntervalIdentifier = null;

        // Split up the work into chunks so we don't block the
        // UI thread while processing.

        function processChunk()
        {
            var view = searchableViews[i];

            if (++i >= searchableViews.length) {
                if (panel._currentSearchChunkIntervalIdentifier === chunkIntervalIdentifier)
                    delete panel._currentSearchChunkIntervalIdentifier;
                clearInterval(chunkIntervalIdentifier);
            }

            if (!view)
                return;

            view.currentQuery = query;
            view.performSearch(query, boundFinishedCallback);
        }

        processChunk();

        chunkIntervalIdentifier = setInterval(processChunk, 25);
        this._currentSearchChunkIntervalIdentifier = chunkIntervalIdentifier;
    },

    jumpToNextSearchResult: function()
    {
        if (!this.showView || !this._searchResults || !this._searchResults.length)
            return;

        var showFirstResult = false;

        this._currentSearchResultIndex = this._searchResults.indexOf(this.visibleView);
        if (this._currentSearchResultIndex === -1) {
            this._currentSearchResultIndex = 0;
            showFirstResult = true;
        }

        var currentView = this._searchResults[this._currentSearchResultIndex];

        if (currentView.showingLastSearchResult()) {
            if (++this._currentSearchResultIndex >= this._searchResults.length)
                this._currentSearchResultIndex = 0;
            currentView = this._searchResults[this._currentSearchResultIndex];
            showFirstResult = true;
        }

        WebInspector.searchController.updateCurrentMatchIndex(this._currentSearchResultIndex, this);

        if (currentView !== this.visibleView) {
            this.showView(currentView);
            WebInspector.searchController.showSearchField();
        }

        if (showFirstResult)
            currentView.jumpToFirstSearchResult();
        else
            currentView.jumpToNextSearchResult();
    },

    jumpToPreviousSearchResult: function()
    {
        if (!this.showView || !this._searchResults || !this._searchResults.length)
            return;

        var showLastResult = false;

        this._currentSearchResultIndex = this._searchResults.indexOf(this.visibleView);
        if (this._currentSearchResultIndex === -1) {
            this._currentSearchResultIndex = 0;
            showLastResult = true;
        }

        var currentView = this._searchResults[this._currentSearchResultIndex];

        if (currentView.showingFirstSearchResult()) {
            if (--this._currentSearchResultIndex < 0)
                this._currentSearchResultIndex = (this._searchResults.length - 1);
            currentView = this._searchResults[this._currentSearchResultIndex];
            showLastResult = true;
        }

        WebInspector.searchController.updateCurrentMatchIndex(this._currentSearchResultIndex, this);

        if (currentView !== this.visibleView) {
            this.showView(currentView);
            WebInspector.searchController.showSearchField();
        }

        if (showLastResult)
            currentView.jumpToLastSearchResult();
        else
            currentView.jumpToPreviousSearchResult();
    },

    /**
     * @return {!Array.<!WebInspector.ProfileHeader>}
     */
    _getAllProfiles: function()
    {
        var profiles = [];
        for (var typeId in this._profileTypesByIdMap)
            profiles = profiles.concat(this._profileTypesByIdMap[typeId].getProfiles());
        return profiles;
    },

    /**
     * @return {!Array.<!WebInspector.View>}
     */
    _searchableViews: function()
    {
        var profiles = this._getAllProfiles();
        var searchableViews = [];
        for (var i = 0; i < profiles.length; ++i) {
            var view = profiles[i].view(this);
            if (view.performSearch)
                searchableViews.push(view)
        }
        var index = searchableViews.indexOf(this.visibleView);
        if (index > 0) {
            // Move visibleView to the first position.
            searchableViews[index] = searchableViews[0];
            searchableViews[0] = this.visibleView;
        }
        return searchableViews;
    },

    searchMatchFound: function(view, matches)
    {
        view.profile._profilesTreeElement.searchMatches = matches;
    },

    searchCanceled: function()
    {
        if (this._searchResults) {
            for (var i = 0; i < this._searchResults.length; ++i) {
                var view = this._searchResults[i];
                if (view.searchCanceled)
                    view.searchCanceled();
                delete view.currentQuery;
            }
        }

        WebInspector.Panel.prototype.searchCanceled.call(this);

        if (this._currentSearchChunkIntervalIdentifier) {
            clearInterval(this._currentSearchChunkIntervalIdentifier);
            delete this._currentSearchChunkIntervalIdentifier;
        }

        this._totalSearchMatches = 0;
        this._currentSearchResultIndex = 0;
        this._searchResults = [];

        var profiles = this._getAllProfiles();
        for (var i = 0; i < profiles.length; ++i)
            profiles[i]._profilesTreeElement.searchMatches = 0;
    },

    _updateInterface: function()
    {
        // FIXME: Replace ProfileType-specific button visibility changes by a single ProfileType-agnostic "combo-button" visibility change.
        if (this._profilerEnabled) {
            this.enableToggleButton.title = WebInspector.UIString("Profiling enabled. Click to disable.");
            this.enableToggleButton.toggled = true;
            this.recordButton.visible = true;
            this._profileViewStatusBarItemsContainer.removeStyleClass("hidden");
            this.clearResultsButton.element.removeStyleClass("hidden");
            this.panelEnablerView.detach();
        } else {
            this.enableToggleButton.title = WebInspector.UIString("Profiling disabled. Click to enable.");
            this.enableToggleButton.toggled = false;
            this.recordButton.visible = false;
            this._profileViewStatusBarItemsContainer.addStyleClass("hidden");
            this.clearResultsButton.element.addStyleClass("hidden");
            this.panelEnablerView.show(this.element);
        }
    },

    get profilerEnabled()
    {
        return this._profilerEnabled;
    },

    enableProfiler: function()
    {
        if (this._profilerEnabled)
            return;
        this._toggleProfiling(this.panelEnablerView.alwaysEnabled);
    },

    disableProfiler: function()
    {
        if (!this._profilerEnabled)
            return;
        this._toggleProfiling(this.panelEnablerView.alwaysEnabled);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _onToggleProfiling: function(event) {
        this._toggleProfiling(true);
    },

    /**
     * @param {boolean} always
     */
    _toggleProfiling: function(always)
    {
        if (this._profilerEnabled) {
            WebInspector.settings.profilerEnabled.set(false);
            ProfilerAgent.disable(this._profilerWasDisabled.bind(this));
        } else {
            WebInspector.settings.profilerEnabled.set(always);
            ProfilerAgent.enable(this._profilerWasEnabled.bind(this));
        }
    },

    /**
     * @param {!WebInspector.Event} event
     */
    sidebarResized: function(event)
    {
        var sidebarWidth = /** @type {number} */ (event.data);
        this._resize(sidebarWidth);
    },

    onResize: function()
    {
        this._resize(this.splitView.sidebarWidth());
    },

    /**
     * @param {number} sidebarWidth
     */
    _resize: function(sidebarWidth)
    {
        var lastItemElement = this._statusBarButtons[this._statusBarButtons.length - 1].element;
        var left = lastItemElement.totalOffsetLeft() + lastItemElement.offsetWidth;
        this._profileTypeStatusBarItemsContainer.style.left = left + "px";
        left += this._profileTypeStatusBarItemsContainer.offsetWidth - 1;
        this._profileViewStatusBarItemsContainer.style.left = Math.max(left, sidebarWidth) + "px";
    },

    /**
     * @param {string} profileType
     * @param {boolean} isProfiling
     */
    setRecordingProfile: function(profileType, isProfiling)
    {
        var profileTypeObject = this.getProfileType(profileType);
        this.recordButton.toggled = isProfiling;
        this.recordButton.title = profileTypeObject.buttonTooltip;
        if (isProfiling) {
            this._launcherView.profileStarted();
            this._createTemporaryProfile(profileType);
        } else
            this._launcherView.profileFinished();
    },

    /**
     * @param {!WebInspector.ProfileHeader} profile
     * @param {number} done
     * @param {number} total
     */
    _reportProfileProgress: function(profile, done, total)
    {
        profile.sidebarElement.subtitle = WebInspector.UIString("%.0f%", (done / total) * 100);
        profile.sidebarElement.wait = true;
    },

    /** 
     * @param {WebInspector.ContextMenu} contextMenu
     * @param {Object} target
     */
    appendApplicableItems: function(event, contextMenu, target)
    {
        if (WebInspector.inspectorView.currentPanel() !== this)
            return;

        var object = /** @type {WebInspector.RemoteObject} */ (target);
        var objectId = object.objectId;
        if (!objectId)
            return;

        var heapProfiles = this.getProfileType(WebInspector.HeapSnapshotProfileType.TypeId).getProfiles();
        if (!heapProfiles.length)
            return;

        function revealInView(viewName)
        {
            HeapProfilerAgent.getHeapObjectId(objectId, didReceiveHeapObjectId.bind(this, viewName));
        }

        function didReceiveHeapObjectId(viewName, error, result)
        {
            if (WebInspector.inspectorView.currentPanel() !== this)
                return;
            if (!error)
                this.showObject(result, viewName);
        }

        contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Reveal in Dominators view" : "Reveal in Dominators View"), revealInView.bind(this, "Dominators"));
        contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Reveal in Summary view" : "Reveal in Summary View"), revealInView.bind(this, "Summary"));
    },

    __proto__: WebInspector.Panel.prototype
}

/**
 * @constructor
 * @extends {WebInspector.SidebarTreeElement}
 * @param {!WebInspector.ProfileHeader} profile
 * @param {string} titleFormat
 * @param {string} className
 */
WebInspector.ProfileSidebarTreeElement = function(profile, titleFormat, className)
{
    this.profile = profile;
    this._titleFormat = titleFormat;

    if (WebInspector.ProfilesPanelDescriptor.isUserInitiatedProfile(this.profile.title))
        this._profileNumber = WebInspector.ProfilesPanelDescriptor.userInitiatedProfileIndex(this.profile.title);

    WebInspector.SidebarTreeElement.call(this, className, "", "", profile, false);

    this.refreshTitles();
}

WebInspector.ProfileSidebarTreeElement.prototype = {
    onselect: function()
    {
        if (!this._suppressOnSelect)
            this.treeOutline.panel._showProfile(this.profile);
    },

    ondelete: function()
    {
        this.treeOutline.panel._removeProfileHeader(this.profile);
        return true;
    },

    get mainTitle()
    {
        if (this._mainTitle)
            return this._mainTitle;
        if (WebInspector.ProfilesPanelDescriptor.isUserInitiatedProfile(this.profile.title))
            return WebInspector.UIString(this._titleFormat, this._profileNumber);
        return this.profile.title;
    },

    set mainTitle(x)
    {
        this._mainTitle = x;
        this.refreshTitles();
    },

    set searchMatches(matches)
    {
        if (!matches) {
            if (!this.bubbleElement)
                return;
            this.bubbleElement.removeStyleClass("search-matches");
            this.bubbleText = "";
            return;
        }

        this.bubbleText = matches;
        this.bubbleElement.addStyleClass("search-matches");
    },

    /**
     * @param {!Event} event
     * @param {!WebInspector.ProfilesPanel} panel
     */
    handleContextMenuEvent: function(event, panel)
    {
        var profile = this.profile;
        var contextMenu = new WebInspector.ContextMenu(event);
        // FIXME: use context menu provider
        if (profile.canSaveToFile()) {
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Save heap snapshot\u2026" : "Save Heap Snapshot\u2026"), profile.saveToFile.bind(profile));
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Load heap snapshot\u2026" : "Load Heap Snapshot\u2026"), panel._fileSelectorElement.click.bind(panel._fileSelectorElement));
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Delete heap snapshot" : "Delete Heap Snapshot"), this.ondelete.bind(this));
        } else {
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Load heap snapshot\u2026" : "Load Heap Snapshot\u2026"), panel._fileSelectorElement.click.bind(panel._fileSelectorElement));
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Delete profile" : "Delete Profile"), this.ondelete.bind(this));
        }
        contextMenu.show();
    },

    __proto__: WebInspector.SidebarTreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.SidebarTreeElement}
 * @param {WebInspector.ProfilesPanel} panel
 * @param {string} title
 * @param {string=} subtitle
 */
WebInspector.ProfileGroupSidebarTreeElement = function(panel, title, subtitle)
{
    WebInspector.SidebarTreeElement.call(this, "profile-group-sidebar-tree-item", title, subtitle, null, true);
    this._panel = panel;
}

WebInspector.ProfileGroupSidebarTreeElement.prototype = {
    onselect: function()
    {
        if (this.children.length > 0)
            this._panel._showProfile(this.children[this.children.length - 1].profile);
    },

    __proto__: WebInspector.SidebarTreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.SidebarTreeElement}
 * @param {!WebInspector.ProfilesPanel} panel
 */
WebInspector.ProfilesSidebarTreeElement = function(panel)
{
    this._panel = panel;
    this.small = false;

    WebInspector.SidebarTreeElement.call(this, "profile-launcher-view-tree-item", WebInspector.UIString("Profiles"), "", null, false);
}

WebInspector.ProfilesSidebarTreeElement.prototype = {
    onselect: function()
    {
        this._panel._showLauncherView();
    },

    get selectable()
    {
        return true;
    },

    __proto__: WebInspector.SidebarTreeElement.prototype
}


/**
 * @constructor
 * @extends {WebInspector.ProfilesPanel}
 */
WebInspector.CPUProfilerPanel = function()
{
    WebInspector.ProfilesPanel.call(this, "cpu-profiler", new WebInspector.CPUProfileType());
}

WebInspector.CPUProfilerPanel.prototype = {
    __proto__: WebInspector.ProfilesPanel.prototype
}


/**
 * @constructor
 * @extends {WebInspector.ProfilesPanel}
 */
WebInspector.CSSSelectorProfilerPanel = function()
{
    WebInspector.ProfilesPanel.call(this, "css-profiler", new WebInspector.CSSSelectorProfileType());
}

WebInspector.CSSSelectorProfilerPanel.prototype = {
    __proto__: WebInspector.ProfilesPanel.prototype
}


/**
 * @constructor
 * @extends {WebInspector.ProfilesPanel}
 */
WebInspector.HeapProfilerPanel = function()
{
    WebInspector.ProfilesPanel.call(this, "heap-profiler", new WebInspector.HeapSnapshotProfileType());
}

WebInspector.HeapProfilerPanel.prototype = {
    __proto__: WebInspector.ProfilesPanel.prototype
}


/**
 * @constructor
 * @extends {WebInspector.ProfilesPanel}
 */
WebInspector.CanvasProfilerPanel = function()
{
    WebInspector.ProfilesPanel.call(this, "canvas-profiler", new WebInspector.CanvasProfileType());
}

WebInspector.CanvasProfilerPanel.prototype = {
    __proto__: WebInspector.ProfilesPanel.prototype
}


importScript("ProfileDataGridTree.js");
importScript("BottomUpProfileDataGridTree.js");
importScript("CPUProfileView.js");
importScript("CSSSelectorProfileView.js");
importScript("FlameChart.js");
importScript("HeapSnapshot.js");
importScript("HeapSnapshotDataGrids.js");
importScript("HeapSnapshotGridNodes.js");
importScript("HeapSnapshotLoader.js");
importScript("HeapSnapshotProxy.js");
importScript("HeapSnapshotView.js");
importScript("HeapSnapshotWorkerDispatcher.js");
importScript("JSHeapSnapshot.js");
importScript("NativeHeapSnapshot.js");
importScript("ProfileLauncherView.js");
importScript("TopDownProfileDataGridTree.js");
importScript("CanvasProfileView.js");
