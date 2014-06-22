/*
 * Copyright (C) 2007, 2008, 2010 Apple Inc.  All rights reserved.
 * Copyright (C) 2009 Joseph Pecoraro
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

importScript("ApplicationCacheItemsView.js");
importScript("DOMStorageItemsView.js");
importScript("DatabaseQueryView.js");
importScript("DatabaseTableView.js");
importScript("DirectoryContentView.js");
importScript("IndexedDBViews.js");
importScript("FileContentView.js");
importScript("FileSystemView.js");

/**
 * @constructor
 * @extends {WebInspector.Panel}
 */
WebInspector.ResourcesPanel = function(database)
{
    WebInspector.Panel.call(this, "resources");
    this.registerRequiredCSS("resourcesPanel.css");

    WebInspector.settings.resourcesLastSelectedItem = WebInspector.settings.createSetting("resourcesLastSelectedItem", {});

    this.createSidebarViewWithTree();
    this.sidebarElement.addStyleClass("outline-disclosure");
    this.sidebarElement.addStyleClass("filter-all");
    this.sidebarElement.addStyleClass("children");
    this.sidebarElement.addStyleClass("small");

    this.sidebarTreeElement.removeStyleClass("sidebar-tree");

    this.resourcesListTreeElement = new WebInspector.StorageCategoryTreeElement(this, WebInspector.UIString("Frames"), "Frames", ["frame-storage-tree-item"]);
    this.sidebarTree.appendChild(this.resourcesListTreeElement);

    this.databasesListTreeElement = new WebInspector.StorageCategoryTreeElement(this, WebInspector.UIString("Web SQL"), "Databases", ["database-storage-tree-item"]);
    this.sidebarTree.appendChild(this.databasesListTreeElement);

    this.indexedDBListTreeElement = new WebInspector.IndexedDBTreeElement(this);
    this.sidebarTree.appendChild(this.indexedDBListTreeElement);

    this.localStorageListTreeElement = new WebInspector.StorageCategoryTreeElement(this, WebInspector.UIString("Local Storage"), "LocalStorage", ["domstorage-storage-tree-item", "local-storage"]);
    this.sidebarTree.appendChild(this.localStorageListTreeElement);

    this.sessionStorageListTreeElement = new WebInspector.StorageCategoryTreeElement(this, WebInspector.UIString("Session Storage"), "SessionStorage", ["domstorage-storage-tree-item", "session-storage"]);
    this.sidebarTree.appendChild(this.sessionStorageListTreeElement);

    this.cookieListTreeElement = new WebInspector.StorageCategoryTreeElement(this, WebInspector.UIString("Cookies"), "Cookies", ["cookie-storage-tree-item"]);
    this.sidebarTree.appendChild(this.cookieListTreeElement);

    this.applicationCacheListTreeElement = new WebInspector.StorageCategoryTreeElement(this, WebInspector.UIString("Application Cache"), "ApplicationCache", ["application-cache-storage-tree-item"]);
    this.sidebarTree.appendChild(this.applicationCacheListTreeElement);

    if (Preferences.exposeFileSystemInspection && WebInspector.experimentsSettings.fileSystemInspection.isEnabled()) {
        this.fileSystemListTreeElement = new WebInspector.FileSystemListTreeElement(this);
        this.sidebarTree.appendChild(this.fileSystemListTreeElement);
    }

    this.storageViews = this.splitView.mainElement;
    this.storageViews.addStyleClass("diff-container");

    this.storageViewStatusBarItemsContainer = document.createElement("div");
    this.storageViewStatusBarItemsContainer.className = "status-bar-items";

    this._databaseTableViews = new Map();
    this._databaseQueryViews = new Map();
    this._databaseTreeElements = new Map();
    this._domStorageViews = new Map();
    this._domStorageTreeElements = new Map();
    this._cookieViews = {};
    this._origins = {};
    this._domains = {};

    this.sidebarElement.addEventListener("mousemove", this._onmousemove.bind(this), false);
    this.sidebarElement.addEventListener("mouseout", this._onmouseout.bind(this), false);

    function viewGetter()
    {
        return this.visibleView;
    }
    WebInspector.GoToLineDialog.install(this, viewGetter.bind(this));

    if (WebInspector.resourceTreeModel.cachedResourcesLoaded())
        this._cachedResourcesLoaded();

    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.OnLoad, this._onLoadEventFired, this);
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.CachedResourcesLoaded, this._cachedResourcesLoaded, this);
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.WillLoadCachedResources, this._resetWithFrames, this);

    WebInspector.databaseModel.databases().forEach(this._addDatabase.bind(this));
    WebInspector.databaseModel.addEventListener(WebInspector.DatabaseModel.Events.DatabaseAdded, this._databaseAdded, this);
}

WebInspector.ResourcesPanel.prototype = {
    statusBarItems: function()
    {
        return [this.storageViewStatusBarItemsContainer];
    },

    wasShown: function()
    {
        WebInspector.Panel.prototype.wasShown.call(this);
        this._initialize();
    },

    _initialize: function()
    {
        if (!this._initialized && this.isShowing() && this._cachedResourcesWereLoaded) {
            this._populateResourceTree();
            this._populateDOMStorageTree();
            this._populateApplicationCacheTree();
            this._initDefaultSelection();
            this._initialized = true;
        }
    },

    _onLoadEventFired: function()
    {
        this._initDefaultSelection();
    },

    _initDefaultSelection: function()
    {
        if (!this._initialized)
            return;

        var itemURL = WebInspector.settings.resourcesLastSelectedItem.get();
        if (itemURL) {
            for (var treeElement = this.sidebarTree.children[0]; treeElement; treeElement = treeElement.traverseNextTreeElement(false, this.sidebarTree, true)) {
                if (treeElement.itemURL === itemURL) {
                    treeElement.revealAndSelect(true);
                    return;
                }
            }
        }

        var mainResource = WebInspector.inspectedPageURL && this.resourcesListTreeElement && this.resourcesListTreeElement.expanded && WebInspector.resourceTreeModel.resourceForURL(WebInspector.inspectedPageURL);
        if (mainResource)
            this.showResource(mainResource);
    },

    _resetWithFrames: function()
    {
        this.resourcesListTreeElement.removeChildren();
        this._treeElementForFrameId = {};
        this._reset();
    },

    _reset: function()
    {
        this._origins = {};
        this._domains = {};
        var queryViews = this._databaseQueryViews.values();
        for (var i = 0; i < queryViews.length; ++i)
            queryViews[i].removeEventListener(WebInspector.DatabaseQueryView.Events.SchemaUpdated, this._updateDatabaseTables, this);
        this._databaseTableViews.clear();
        this._databaseQueryViews.clear();
        this._databaseTreeElements.clear();
        this._domStorageViews.clear();
        this._domStorageTreeElements.clear();
        this._cookieViews = {};

        this.databasesListTreeElement.removeChildren();
        this.localStorageListTreeElement.removeChildren();
        this.sessionStorageListTreeElement.removeChildren();
        this.cookieListTreeElement.removeChildren();

        if (this.visibleView && !(this.visibleView instanceof WebInspector.StorageCategoryView))
            this.visibleView.detach();

        this.storageViewStatusBarItemsContainer.removeChildren();

        if (this.sidebarTree.selectedTreeElement)
            this.sidebarTree.selectedTreeElement.deselect();
    },

    _populateResourceTree: function()
    {
        this._treeElementForFrameId = {};
        WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.FrameAdded, this._frameAdded, this);
        WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.FrameNavigated, this._frameNavigated, this);
        WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.FrameDetached, this._frameDetached, this);
        WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.ResourceAdded, this._resourceAdded, this);

        function populateFrame(frame)
        {
            this._frameAdded({data:frame});
            for (var i = 0; i < frame.childFrames.length; ++i)
                populateFrame.call(this, frame.childFrames[i]);

            var resources = frame.resources();
            for (var i = 0; i < resources.length; ++i)
                this._resourceAdded({data:resources[i]});
        }
        populateFrame.call(this, WebInspector.resourceTreeModel.mainFrame);
    },

    _frameAdded: function(event)
    {
        var frame = event.data;
        var parentFrame = frame.parentFrame;

        var parentTreeElement = parentFrame ? this._treeElementForFrameId[parentFrame.id] : this.resourcesListTreeElement;
        if (!parentTreeElement) {
            console.warn("No frame to route " + frame.url + " to.")
            return;
        }

        var frameTreeElement = new WebInspector.FrameTreeElement(this, frame);
        this._treeElementForFrameId[frame.id] = frameTreeElement;
        parentTreeElement.appendChild(frameTreeElement);
    },

    _frameDetached: function(event)
    {
        var frame = event.data;
        var frameTreeElement = this._treeElementForFrameId[frame.id];
        if (!frameTreeElement)
            return;

        delete this._treeElementForFrameId[frame.id];
        if (frameTreeElement.parent)
            frameTreeElement.parent.removeChild(frameTreeElement);
    },

    _resourceAdded: function(event)
    {
        var resource = event.data;
        var frameId = resource.frameId;

        if (resource.statusCode >= 301 && resource.statusCode <= 303)
            return;

        var frameTreeElement = this._treeElementForFrameId[frameId];
        if (!frameTreeElement) {
            // This is a frame's main resource, it will be retained
            // and re-added by the resource manager;
            return;
        }

        frameTreeElement.appendResource(resource);
    },

    _frameNavigated: function(event)
    {
        var frame = event.data;

        if (!frame.parentFrame)
            this._reset();

        var frameId = frame.id;
        var frameTreeElement = this._treeElementForFrameId[frameId];
        if (frameTreeElement)
            frameTreeElement.frameNavigated(frame);

        var applicationCacheFrameTreeElement = this._applicationCacheFrameElements[frameId];
        if (applicationCacheFrameTreeElement)
            applicationCacheFrameTreeElement.frameNavigated(frame);
    },

    _cachedResourcesLoaded: function()
    {
        this._cachedResourcesWereLoaded = true;
        this._initialize();
    },

    /**
     * @param {WebInspector.Event} event
     */
    _databaseAdded: function(event)
    {
        var database = /** @type {WebInspector.Database} */ (event.data);
        this._addDatabase(database);
    },

    /**
     * @param {WebInspector.Database} database
     */
    _addDatabase: function(database)
    {
        var databaseTreeElement = new WebInspector.DatabaseTreeElement(this, database);
        this._databaseTreeElements.put(database, databaseTreeElement);
        this.databasesListTreeElement.appendChild(databaseTreeElement);
    },

    addDocumentURL: function(url)
    {
        var parsedURL = url.asParsedURL();
        if (!parsedURL)
            return;

        var domain = parsedURL.host;
        if (!this._domains[domain]) {
            this._domains[domain] = true;

            var cookieDomainTreeElement = new WebInspector.CookieTreeElement(this, domain);
            this.cookieListTreeElement.appendChild(cookieDomainTreeElement);
        }
    },

    /**
     * @param {WebInspector.Event} event
     */
    _domStorageAdded: function(event)
    {
        var domStorage = /** @type {WebInspector.DOMStorage} */ (event.data);
        this._addDOMStorage(domStorage);
    },

    /**
     * @param {WebInspector.DOMStorage} domStorage
     */
    _addDOMStorage: function(domStorage)
    {
        console.assert(!this._domStorageTreeElements.get(domStorage));

        var domStorageTreeElement = new WebInspector.DOMStorageTreeElement(this, domStorage, (domStorage.isLocalStorage ? "local-storage" : "session-storage"));
        this._domStorageTreeElements.put(domStorage, domStorageTreeElement);
        if (domStorage.isLocalStorage)
            this.localStorageListTreeElement.appendChild(domStorageTreeElement);
        else
            this.sessionStorageListTreeElement.appendChild(domStorageTreeElement);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _domStorageRemoved: function(event)
    {
        var domStorage = /** @type {WebInspector.DOMStorage} */ (event.data);
        this._removeDOMStorage(domStorage);
    },

    /**
     * @param {WebInspector.DOMStorage} domStorage
     */
    _removeDOMStorage: function(domStorage)
    {
        var treeElement = this._domStorageTreeElements.get(domStorage);
        if (!treeElement)
            return;
        var wasSelected = treeElement.selected;
        var parentListTreeElement = treeElement.parent;
        parentListTreeElement.removeChild(treeElement);
        if (wasSelected)
            parentListTreeElement.select();
        this._domStorageTreeElements.remove(treeElement);
        this._domStorageViews.remove(domStorage);
    },

    /**
     * @param {WebInspector.Database} database
     */
    selectDatabase: function(database)
    {
        if (database) {
            this._showDatabase(database);
            this._databaseTreeElements.get(database).select();
        }
    },

    /**
     * @param {WebInspector.DOMStorage} domStorage
     */
    selectDOMStorage: function(domStorage)
    {
        if (domStorage) {
            this._showDOMStorage(domStorage);
            this._domStorageTreeElements.get(domStorage).select();
        }
    },

    canShowAnchorLocation: function(anchor)
    {
        return !!WebInspector.resourceForURL(anchor.href);
    },

    showAnchorLocation: function(anchor)
    {
        var resource = WebInspector.resourceForURL(anchor.href);
        this.showResource(resource, anchor.lineNumber);
    },

    /**
     * @param {number=} line
     */
    showResource: function(resource, line)
    {
        var resourceTreeElement = this._findTreeElementForResource(resource);
        if (resourceTreeElement)
            resourceTreeElement.revealAndSelect();

        if (typeof line === "number") {
            var view = this._resourceViewForResource(resource);
            if (view.canHighlightLine())
                view.highlightLine(line);
        }
        return true;
    },

    _showResourceView: function(resource)
    {
        var view = this._resourceViewForResource(resource);
        if (!view) {
            this.visibleView.detach();
            return;
        }
        if (view.searchCanceled)
            view.searchCanceled();
        this._innerShowView(view);
    },

    _resourceViewForResource: function(resource)
    {
        if (WebInspector.ResourceView.hasTextContent(resource)) {
            var treeElement = this._findTreeElementForResource(resource);
            if (!treeElement)
                return null;
            return treeElement.sourceView();
        }
        return WebInspector.ResourceView.nonSourceViewForResource(resource);
    },

    /**
     * @param {string=} tableName
     */
    _showDatabase: function(database, tableName)
    {
        if (!database)
            return;

        var view;
        if (tableName) {
            var tableViews = this._databaseTableViews.get(database);
            if (!tableViews) {
                tableViews = {};
                this._databaseTableViews.put(database, tableViews);
            }
            view = tableViews[tableName];
            if (!view) {
                view = new WebInspector.DatabaseTableView(database, tableName);
                tableViews[tableName] = view;
            }
        } else {
            view = this._databaseQueryViews.get(database);
            if (!view) {
                view = new WebInspector.DatabaseQueryView(database);
                this._databaseQueryViews.put(database, view);
                view.addEventListener(WebInspector.DatabaseQueryView.Events.SchemaUpdated, this._updateDatabaseTables, this);
            }
        }

        this._innerShowView(view);
    },

    /**
     * @param {WebInspector.View} view
     */
    showIndexedDB: function(view)
    {
        this._innerShowView(view);
    },

    _showDOMStorage: function(domStorage)
    {
        if (!domStorage)
            return;

        var view;
        view = this._domStorageViews.get(domStorage);
        if (!view) {
            view = new WebInspector.DOMStorageItemsView(domStorage, WebInspector.domStorageModel);
            this._domStorageViews.put(domStorage, view);
        }

        this._innerShowView(view);
    },

    showCookies: function(treeElement, cookieDomain)
    {
        var view = this._cookieViews[cookieDomain];
        if (!view) {
            view = new WebInspector.CookieItemsView(treeElement, cookieDomain);
            this._cookieViews[cookieDomain] = view;
        }

        this._innerShowView(view);
    },

    /**
     * @param {string} cookieDomain
     */
    clearCookies: function(cookieDomain)
    {
        this._cookieViews[cookieDomain].clear();
    },

    showApplicationCache: function(frameId)
    {
        if (!this._applicationCacheViews[frameId])
            this._applicationCacheViews[frameId] = new WebInspector.ApplicationCacheItemsView(this._applicationCacheModel, frameId);

        this._innerShowView(this._applicationCacheViews[frameId]);
    },

    /**
     *  @param {WebInspector.View} view
     */
    showFileSystem: function(view)
    {
        this._innerShowView(view);
    },

    showCategoryView: function(categoryName)
    {
        if (!this._categoryView)
            this._categoryView = new WebInspector.StorageCategoryView();
        this._categoryView.setText(categoryName);
        this._innerShowView(this._categoryView);
    },

    _innerShowView: function(view)
    {
        if (this.visibleView === view)
            return;

        if (this.visibleView)
            this.visibleView.detach();

        view.show(this.storageViews);
        this.visibleView = view;

        this.storageViewStatusBarItemsContainer.removeChildren();
        var statusBarItems = view.statusBarItems() || [];
        for (var i = 0; i < statusBarItems.length; ++i)
            this.storageViewStatusBarItemsContainer.appendChild(statusBarItems[i]);
    },

    closeVisibleView: function()
    {
        if (!this.visibleView)
            return;
        this.visibleView.detach();
        delete this.visibleView;
    },

    _updateDatabaseTables: function(event)
    {
        var database = event.data;

        if (!database)
            return;

        var databasesTreeElement = this._databaseTreeElements.get(database);
        if (!databasesTreeElement)
            return;

        databasesTreeElement.shouldRefreshChildren = true;
        var tableViews = this._databaseTableViews.get(database);

        if (!tableViews)
            return;

        var tableNamesHash = {};
        var self = this;
        function tableNamesCallback(tableNames)
        {
            var tableNamesLength = tableNames.length;
            for (var i = 0; i < tableNamesLength; ++i)
                tableNamesHash[tableNames[i]] = true;

            for (var tableName in tableViews) {
                if (!(tableName in tableNamesHash)) {
                    if (self.visibleView === tableViews[tableName])
                        self.closeVisibleView();
                    delete tableViews[tableName];
                }
            }
        }
        database.getTableNames(tableNamesCallback);
    },

    _populateDOMStorageTree: function()
    {
        WebInspector.domStorageModel.storages().forEach(this._addDOMStorage.bind(this));
        WebInspector.domStorageModel.addEventListener(WebInspector.DOMStorageModel.Events.DOMStorageAdded, this._domStorageAdded, this);
        WebInspector.domStorageModel.addEventListener(WebInspector.DOMStorageModel.Events.DOMStorageRemoved, this._domStorageRemoved, this);
    },

    _populateApplicationCacheTree: function()
    {
        this._applicationCacheModel = new WebInspector.ApplicationCacheModel();

        this._applicationCacheViews = {};
        this._applicationCacheFrameElements = {};
        this._applicationCacheManifestElements = {};

        this._applicationCacheModel.addEventListener(WebInspector.ApplicationCacheModel.EventTypes.FrameManifestAdded, this._applicationCacheFrameManifestAdded, this);
        this._applicationCacheModel.addEventListener(WebInspector.ApplicationCacheModel.EventTypes.FrameManifestRemoved, this._applicationCacheFrameManifestRemoved, this);

        this._applicationCacheModel.addEventListener(WebInspector.ApplicationCacheModel.EventTypes.FrameManifestStatusUpdated, this._applicationCacheFrameManifestStatusChanged, this);
        this._applicationCacheModel.addEventListener(WebInspector.ApplicationCacheModel.EventTypes.NetworkStateChanged, this._applicationCacheNetworkStateChanged, this);
    },

    _applicationCacheFrameManifestAdded: function(event)
    {
        var frameId = event.data;
        var manifestURL = this._applicationCacheModel.frameManifestURL(frameId);
        var status = this._applicationCacheModel.frameManifestStatus(frameId)

        var manifestTreeElement = this._applicationCacheManifestElements[manifestURL]
        if (!manifestTreeElement) {
            manifestTreeElement = new WebInspector.ApplicationCacheManifestTreeElement(this, manifestURL);
            this.applicationCacheListTreeElement.appendChild(manifestTreeElement);
            this._applicationCacheManifestElements[manifestURL] = manifestTreeElement;
        }

        var frameTreeElement = new WebInspector.ApplicationCacheFrameTreeElement(this, frameId, manifestURL);
        manifestTreeElement.appendChild(frameTreeElement);
        manifestTreeElement.expand();
        this._applicationCacheFrameElements[frameId] = frameTreeElement;
    },

    _applicationCacheFrameManifestRemoved: function(event)
    {
        var frameId = event.data;
        var frameTreeElement = this._applicationCacheFrameElements[frameId];
        if (!frameTreeElement)
            return;

        var manifestURL = frameTreeElement.manifestURL;
        delete this._applicationCacheFrameElements[frameId];
        delete this._applicationCacheViews[frameId];
        frameTreeElement.parent.removeChild(frameTreeElement);

        var manifestTreeElement = this._applicationCacheManifestElements[manifestURL];
        if (manifestTreeElement.children.length !== 0)
            return;

        delete this._applicationCacheManifestElements[manifestURL];
        manifestTreeElement.parent.removeChild(manifestTreeElement);
    },

    _applicationCacheFrameManifestStatusChanged: function(event)
    {
        var frameId = event.data;
        var status = this._applicationCacheModel.frameManifestStatus(frameId)

        if (this._applicationCacheViews[frameId])
            this._applicationCacheViews[frameId].updateStatus(status);
    },

    _applicationCacheNetworkStateChanged: function(event)
    {
        var isNowOnline = event.data;

        for (var manifestURL in this._applicationCacheViews)
            this._applicationCacheViews[manifestURL].updateNetworkState(isNowOnline);
    },

    sidebarResized: function(event)
    {
        var width = event.data;
        this.storageViewStatusBarItemsContainer.style.left = width + "px";
    },

    /**
     * @param {string} query
     */
    performSearch: function(query)
    {
        this._resetSearchResults();
        var regex = WebInspector.SourceFrame.createSearchRegex(query);
        var totalMatchesCount = 0;

        function callback(error, result)
        {
            if (!error) {
                for (var i = 0; i < result.length; i++) {
                    var searchResult = result[i];
                    var frameTreeElement = this._treeElementForFrameId[searchResult.frameId];
                    if (!frameTreeElement)
                        continue;
                    var resource = frameTreeElement.resourceByURL(searchResult.url);

                    // FIXME: When the same script is used in several frames and this script contains at least
                    // one search result then some search results can not be matched with a resource on panel.
                    // https://bugs.webkit.org/show_bug.cgi?id=66005
                    if (!resource)
                        continue;

                    this._findTreeElementForResource(resource).searchMatchesFound(searchResult.matchesCount);
                    totalMatchesCount += searchResult.matchesCount;
                }
            }

            WebInspector.searchController.updateSearchMatchesCount(totalMatchesCount, this);
            this._searchController = new WebInspector.ResourcesSearchController(this.resourcesListTreeElement, totalMatchesCount);

            if (this.sidebarTree.selectedTreeElement && this.sidebarTree.selectedTreeElement.searchMatchesCount)
                this.jumpToNextSearchResult();
        }

        PageAgent.searchInResources(regex.source, !regex.ignoreCase, true, callback.bind(this));
    },

    _ensureViewSearchPerformed: function(callback)
    {
        function viewSearchPerformedCallback(searchId)
        {
            if (searchId !== this._lastViewSearchId)
                return; // Search is obsolete.
            this._viewSearchInProgress = false;
            callback();
        }

        if (!this._viewSearchInProgress) {
            if (!this.visibleView.hasSearchResults()) {
                // We give id to each search, so that we can skip callbacks for obsolete searches.
                this._lastViewSearchId = this._lastViewSearchId ? this._lastViewSearchId + 1 : 0;
                this._viewSearchInProgress = true;
                this.visibleView.performSearch(this.currentQuery, viewSearchPerformedCallback.bind(this, this._lastViewSearchId));
            } else
                callback();
        }
    },

    _showSearchResult: function(searchResult)
    {
        this._lastSearchResultIndex = searchResult.index;
        this._lastSearchResultTreeElement = searchResult.treeElement;

        // At first show view for treeElement.
        if (searchResult.treeElement !== this.sidebarTree.selectedTreeElement) {
            this.showResource(searchResult.treeElement.representedObject);
            WebInspector.searchController.showSearchField();
        }

        function callback(searchId)
        {
            if (this.sidebarTree.selectedTreeElement !== this._lastSearchResultTreeElement)
                return; // User has selected another view while we were searching.
            if (this._lastSearchResultIndex != -1)
                this.visibleView.jumpToSearchResult(this._lastSearchResultIndex);
            WebInspector.searchController.updateCurrentMatchIndex(searchResult.currentMatchIndex - 1, this);
        }

        // Then run SourceFrame search if needed and jump to search result index when done.
        this._ensureViewSearchPerformed(callback.bind(this));
    },

    _resetSearchResults: function()
    {
        function callback(resourceTreeElement)
        {
            resourceTreeElement._resetSearchResults();
        }

        this._forAllResourceTreeElements(callback);
        if (this.visibleView && this.visibleView.searchCanceled)
            this.visibleView.searchCanceled();

        this._lastSearchResultTreeElement = null;
        this._lastSearchResultIndex = -1;
        this._viewSearchInProgress = false;
    },

    searchCanceled: function()
    {
        function callback(resourceTreeElement)
        {
            resourceTreeElement._updateErrorsAndWarningsBubbles();
        }

        WebInspector.searchController.updateSearchMatchesCount(0, this);
        this._resetSearchResults();
        this._forAllResourceTreeElements(callback);
    },

    jumpToNextSearchResult: function()
    {
        if (!this.currentSearchMatches)
            return;
        var currentTreeElement = this.sidebarTree.selectedTreeElement;
        var nextSearchResult = this._searchController.nextSearchResult(currentTreeElement);
        this._showSearchResult(nextSearchResult);
    },

    jumpToPreviousSearchResult: function()
    {
        if (!this.currentSearchMatches)
            return;
        var currentTreeElement = this.sidebarTree.selectedTreeElement;
        var previousSearchResult = this._searchController.previousSearchResult(currentTreeElement);
        this._showSearchResult(previousSearchResult);
    },

    _forAllResourceTreeElements: function(callback)
    {
        var stop = false;
        for (var treeElement = this.resourcesListTreeElement; !stop && treeElement; treeElement = treeElement.traverseNextTreeElement(false, this.resourcesListTreeElement, true)) {
            if (treeElement instanceof WebInspector.FrameResourceTreeElement)
                stop = callback(treeElement);
        }
    },

    _findTreeElementForResource: function(resource)
    {
        function isAncestor(ancestor, object)
        {
            // Redirects, XHRs do not belong to the tree, it is fine to silently return false here.
            return false;
        }

        function getParent(object)
        {
            // Redirects, XHRs do not belong to the tree, it is fine to silently return false here.
            return null;
        }

        return this.sidebarTree.findTreeElement(resource, isAncestor, getParent);
    },

    showView: function(view)
    {
        if (view)
            this.showResource(view.resource);
    },

    _onmousemove: function(event)
    {
        var nodeUnderMouse = document.elementFromPoint(event.pageX, event.pageY);
        if (!nodeUnderMouse)
            return;

        var listNode = nodeUnderMouse.enclosingNodeOrSelfWithNodeName("li");
        if (!listNode)
            return;

        var element = listNode.treeElement;
        if (this._previousHoveredElement === element)
            return;

        if (this._previousHoveredElement) {
            this._previousHoveredElement.hovered = false;
            delete this._previousHoveredElement;
        }

        if (element instanceof WebInspector.FrameTreeElement) {
            this._previousHoveredElement = element;
            element.hovered = true;
        }
    },

    _onmouseout: function(event)
    {
        if (this._previousHoveredElement) {
            this._previousHoveredElement.hovered = false;
            delete this._previousHoveredElement;
        }
    },

    __proto__: WebInspector.Panel.prototype
}

/**
 * @constructor
 * @extends {TreeElement}
 * @param {boolean=} hasChildren
 * @param {boolean=} noIcon
 */
WebInspector.BaseStorageTreeElement = function(storagePanel, representedObject, title, iconClasses, hasChildren, noIcon)
{
    TreeElement.call(this, "", representedObject, hasChildren);
    this._storagePanel = storagePanel;
    this._titleText = title;
    this._iconClasses = iconClasses;
    this._noIcon = noIcon;
}

WebInspector.BaseStorageTreeElement.prototype = {
    onattach: function()
    {
        this.listItemElement.removeChildren();
        if (this._iconClasses) {
            for (var i = 0; i < this._iconClasses.length; ++i)
                this.listItemElement.addStyleClass(this._iconClasses[i]);
        }

        var selectionElement = document.createElement("div");
        selectionElement.className = "selection";
        this.listItemElement.appendChild(selectionElement);

        if (!this._noIcon) {
            this.imageElement = document.createElement("img");
            this.imageElement.className = "icon";
            this.listItemElement.appendChild(this.imageElement);
        }

        this.titleElement = document.createElement("div");
        this.titleElement.className = "base-storage-tree-element-title";
        this._titleTextNode = document.createTextNode("");
        this.titleElement.appendChild(this._titleTextNode);
        this._updateTitle();
        this._updateSubtitle();
        this.listItemElement.appendChild(this.titleElement);
    },

    get displayName()
    {
        return this._displayName;
    },

    _updateDisplayName: function()
    {
        this._displayName = this._titleText || "";
        if (this._subtitleText)
            this._displayName += " (" + this._subtitleText + ")";
    },

    _updateTitle: function()
    {
        this._updateDisplayName();

        if (!this.titleElement)
            return;

        this._titleTextNode.textContent = this._titleText || "";
    },

    _updateSubtitle: function()
    {
        this._updateDisplayName();

        if (!this.titleElement)
            return;

        if (this._subtitleText) {
            if (!this._subtitleElement) {
                this._subtitleElement = document.createElement("span");
                this._subtitleElement.className = "base-storage-tree-element-subtitle";
                this.titleElement.appendChild(this._subtitleElement);
            }
            this._subtitleElement.textContent = "(" + this._subtitleText + ")";
        } else if (this._subtitleElement) {
            this.titleElement.removeChild(this._subtitleElement);
            delete this._subtitleElement;
        }
    },

    onselect: function(selectedByUser)
    {
        if (!selectedByUser)
            return;
        var itemURL = this.itemURL;
        if (itemURL)
            WebInspector.settings.resourcesLastSelectedItem.set(itemURL);
    },

    onreveal: function()
    {
        if (this.listItemElement)
            this.listItemElement.scrollIntoViewIfNeeded(false);
    },

    get titleText()
    {
        return this._titleText;
    },

    set titleText(titleText)
    {
        this._titleText = titleText;
        this._updateTitle();
    },

    get subtitleText()
    {
        return this._subtitleText;
    },

    set subtitleText(subtitleText)
    {
        this._subtitleText = subtitleText;
        this._updateSubtitle();
    },

    get searchMatchesCount()
    {
        return 0;
    },

    __proto__: TreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.BaseStorageTreeElement}
 * @param {boolean=} noIcon
 */
WebInspector.StorageCategoryTreeElement = function(storagePanel, categoryName, settingsKey, iconClasses, noIcon)
{
    WebInspector.BaseStorageTreeElement.call(this, storagePanel, null, categoryName, iconClasses, true, noIcon);
    this._expandedSettingKey = "resources" + settingsKey + "Expanded";
    WebInspector.settings[this._expandedSettingKey] = WebInspector.settings.createSetting(this._expandedSettingKey, settingsKey === "Frames");
    this._categoryName = categoryName;
}

WebInspector.StorageCategoryTreeElement.prototype = {
    get itemURL()
    {
        return "category://" + this._categoryName;
    },

    onselect: function(selectedByUser)
    {
        WebInspector.BaseStorageTreeElement.prototype.onselect.call(this, selectedByUser);
        this._storagePanel.showCategoryView(this._categoryName);
    },

    onattach: function()
    {
        WebInspector.BaseStorageTreeElement.prototype.onattach.call(this);
        if (WebInspector.settings[this._expandedSettingKey].get())
            this.expand();
    },

    onexpand: function()
    {
        WebInspector.settings[this._expandedSettingKey].set(true);
    },

    oncollapse: function()
    {
        WebInspector.settings[this._expandedSettingKey].set(false);
    },

    __proto__: WebInspector.BaseStorageTreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.BaseStorageTreeElement}
 */
WebInspector.FrameTreeElement = function(storagePanel, frame)
{
    WebInspector.BaseStorageTreeElement.call(this, storagePanel, null, "", ["frame-storage-tree-item"]);
    this._frame = frame;
    this.frameNavigated(frame);
}

WebInspector.FrameTreeElement.prototype = {
    frameNavigated: function(frame)
    {
        this.removeChildren();
        this._frameId = frame.id;

        this.titleText = frame.name;
        this.subtitleText = new WebInspector.ParsedURL(frame.url).displayName;

        this._categoryElements = {};
        this._treeElementForResource = {};

        this._storagePanel.addDocumentURL(frame.url);
    },

    get itemURL()
    {
        return "frame://" + encodeURI(this.displayName);
    },

    onselect: function(selectedByUser)
    {
        WebInspector.BaseStorageTreeElement.prototype.onselect.call(this, selectedByUser);
        this._storagePanel.showCategoryView(this.displayName);

        this.listItemElement.removeStyleClass("hovered");
        DOMAgent.hideHighlight();
    },

    set hovered(hovered)
    {
        if (hovered) {
            this.listItemElement.addStyleClass("hovered");
            DOMAgent.highlightFrame(this._frameId, WebInspector.Color.PageHighlight.Content.toProtocolRGBA(), WebInspector.Color.PageHighlight.ContentOutline.toProtocolRGBA());
        } else {
            this.listItemElement.removeStyleClass("hovered");
            DOMAgent.hideHighlight();
        }
    },

    appendResource: function(resource)
    {
        if (resource.isHidden())
            return;
        var categoryName = resource.type.name();
        var categoryElement = resource.type === WebInspector.resourceTypes.Document ? this : this._categoryElements[categoryName];
        if (!categoryElement) {
            categoryElement = new WebInspector.StorageCategoryTreeElement(this._storagePanel, resource.type.categoryTitle(), categoryName, null, true);
            this._categoryElements[resource.type.name()] = categoryElement;
            this._insertInPresentationOrder(this, categoryElement);
        }
        var resourceTreeElement = new WebInspector.FrameResourceTreeElement(this._storagePanel, resource);
        this._insertInPresentationOrder(categoryElement, resourceTreeElement);
        this._treeElementForResource[resource.url] = resourceTreeElement;
    },

    resourceByURL: function(url)
    {
        var treeElement = this._treeElementForResource[url];
        return treeElement ? treeElement.representedObject : null;
    },

    appendChild: function(treeElement)
    {
        this._insertInPresentationOrder(this, treeElement);
    },

    _insertInPresentationOrder: function(parentTreeElement, childTreeElement)
    {
        // Insert in the alphabetical order, first frames, then resources. Document resource goes last.
        function typeWeight(treeElement)
        {
            if (treeElement instanceof WebInspector.StorageCategoryTreeElement)
                return 2;
            if (treeElement instanceof WebInspector.FrameTreeElement)
                return 1;
            return 3;
        }

        function compare(treeElement1, treeElement2)
        {
            var typeWeight1 = typeWeight(treeElement1);
            var typeWeight2 = typeWeight(treeElement2);

            var result;
            if (typeWeight1 > typeWeight2)
                result = 1;
            else if (typeWeight1 < typeWeight2)
                result = -1;
            else {
                var title1 = treeElement1.displayName || treeElement1.titleText;
                var title2 = treeElement2.displayName || treeElement2.titleText;
                result = title1.localeCompare(title2);
            }
            return result;
        }

        var children = parentTreeElement.children;
        var i;
        for (i = 0; i < children.length; ++i) {
            if (compare(childTreeElement, children[i]) < 0)
                break;
        }
        parentTreeElement.insertChild(childTreeElement, i);
    },

    __proto__: WebInspector.BaseStorageTreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.BaseStorageTreeElement}
 */
WebInspector.FrameResourceTreeElement = function(storagePanel, resource)
{
    WebInspector.BaseStorageTreeElement.call(this, storagePanel, resource, resource.displayName, ["resource-sidebar-tree-item", "resources-type-" + resource.type.name()]);
    this._resource = resource;
    this._resource.addEventListener(WebInspector.Resource.Events.MessageAdded, this._consoleMessageAdded, this);
    this._resource.addEventListener(WebInspector.Resource.Events.MessagesCleared, this._consoleMessagesCleared, this);
    this.tooltip = resource.url;
}

WebInspector.FrameResourceTreeElement.prototype = {
    get itemURL()
    {
        return this._resource.url;
    },

    onselect: function(selectedByUser)
    {
        WebInspector.BaseStorageTreeElement.prototype.onselect.call(this, selectedByUser);
        this._storagePanel._showResourceView(this._resource);
    },

    ondblclick: function(event)
    {
        InspectorFrontendHost.openInNewTab(this._resource.url);
    },

    onattach: function()
    {
        WebInspector.BaseStorageTreeElement.prototype.onattach.call(this);

        if (this._resource.type === WebInspector.resourceTypes.Image) {
            var previewImage = document.createElement("img");
            previewImage.className = "image-resource-icon-preview";
            this._resource.populateImageSource(previewImage);

            var iconElement = document.createElement("div");
            iconElement.className = "icon";
            iconElement.appendChild(previewImage);
            this.listItemElement.replaceChild(iconElement, this.imageElement);
        }

        this._statusElement = document.createElement("div");
        this._statusElement.className = "status";
        this.listItemElement.insertBefore(this._statusElement, this.titleElement);

        this.listItemElement.draggable = true;
        this.listItemElement.addEventListener("dragstart", this._ondragstart.bind(this), false);
        this.listItemElement.addEventListener("contextmenu", this._handleContextMenuEvent.bind(this), true);

        this._updateErrorsAndWarningsBubbles();
    },

    _ondragstart: function(event)
    {
        event.dataTransfer.setData("text/plain", this._resource.content);
        event.dataTransfer.effectAllowed = "copy";
        return true;
    },

    _handleContextMenuEvent: function(event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);
        contextMenu.appendApplicableItems(this._resource);
        if (this._resource.request)
            contextMenu.appendApplicableItems(this._resource.request);
        contextMenu.show();
    },

    _setBubbleText: function(x)
    {
        if (!this._bubbleElement) {
            this._bubbleElement = document.createElement("div");
            this._bubbleElement.className = "bubble";
            this._statusElement.appendChild(this._bubbleElement);
        }

        this._bubbleElement.textContent = x;
    },

    _resetBubble: function()
    {
        if (this._bubbleElement) {
            this._bubbleElement.textContent = "";
            this._bubbleElement.removeStyleClass("search-matches");
            this._bubbleElement.removeStyleClass("warning");
            this._bubbleElement.removeStyleClass("error");
        }
    },

    _resetSearchResults: function()
    {
        this._resetBubble();
        this._searchMatchesCount = 0;
    },

    get searchMatchesCount()
    {
        return this._searchMatchesCount;
    },

    searchMatchesFound: function(matchesCount)
    {
        this._resetSearchResults();

        this._searchMatchesCount = matchesCount;
        this._setBubbleText(matchesCount);
        this._bubbleElement.addStyleClass("search-matches");

        // Expand, do not scroll into view.
        var currentAncestor = this.parent;
        while (currentAncestor && !currentAncestor.root) {
            if (!currentAncestor.expanded)
                currentAncestor.expand();
            currentAncestor = currentAncestor.parent;
        }
    },

    _updateErrorsAndWarningsBubbles: function()
    {
        if (this._storagePanel.currentQuery)
            return;

        this._resetBubble();

        if (this._resource.warnings || this._resource.errors)
            this._setBubbleText(this._resource.warnings + this._resource.errors);

        if (this._resource.warnings)
            this._bubbleElement.addStyleClass("warning");

        if (this._resource.errors)
            this._bubbleElement.addStyleClass("error");
    },

    _consoleMessagesCleared: function()
    {
        // FIXME: move to the SourceFrame.
        if (this._sourceView)
            this._sourceView.clearMessages();

        this._updateErrorsAndWarningsBubbles();
    },

    _consoleMessageAdded: function(event)
    {
        var msg = event.data;
        if (this._sourceView)
            this._sourceView.addMessage(msg);
        this._updateErrorsAndWarningsBubbles();
    },

    sourceView: function()
    {
        if (!this._sourceView) {
            this._sourceView = new WebInspector.ResourceSourceFrame(this._resource);
            if (this._resource.messages) {
                for (var i = 0; i < this._resource.messages.length; i++)
                    this._sourceView.addMessage(this._resource.messages[i]);
            }
        }
        return this._sourceView;
    },

    __proto__: WebInspector.BaseStorageTreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.BaseStorageTreeElement}
 */
WebInspector.DatabaseTreeElement = function(storagePanel, database)
{
    WebInspector.BaseStorageTreeElement.call(this, storagePanel, null, database.name, ["database-storage-tree-item"], true);
    this._database = database;
}

WebInspector.DatabaseTreeElement.prototype = {
    get itemURL()
    {
        return "database://" + encodeURI(this._database.name);
    },

    onselect: function(selectedByUser)
    {
        WebInspector.BaseStorageTreeElement.prototype.onselect.call(this, selectedByUser);
        this._storagePanel._showDatabase(this._database);
    },

    onexpand: function()
    {
        this._updateChildren();
    },

    _updateChildren: function()
    {
        this.removeChildren();

        function tableNamesCallback(tableNames)
        {
            var tableNamesLength = tableNames.length;
            for (var i = 0; i < tableNamesLength; ++i)
                this.appendChild(new WebInspector.DatabaseTableTreeElement(this._storagePanel, this._database, tableNames[i]));
        }
        this._database.getTableNames(tableNamesCallback.bind(this));
    },

    __proto__: WebInspector.BaseStorageTreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.BaseStorageTreeElement}
 */
WebInspector.DatabaseTableTreeElement = function(storagePanel, database, tableName)
{
    WebInspector.BaseStorageTreeElement.call(this, storagePanel, null, tableName, ["database-storage-tree-item"]);
    this._database = database;
    this._tableName = tableName;
}

WebInspector.DatabaseTableTreeElement.prototype = {
    get itemURL()
    {
        return "database://" + encodeURI(this._database.name) + "/" + encodeURI(this._tableName);
    },

    onselect: function(selectedByUser)
    {
        WebInspector.BaseStorageTreeElement.prototype.onselect.call(this, selectedByUser);
        this._storagePanel._showDatabase(this._database, this._tableName);
    },

    __proto__: WebInspector.BaseStorageTreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.StorageCategoryTreeElement}
 * @param {WebInspector.ResourcesPanel} storagePanel
 */
WebInspector.IndexedDBTreeElement = function(storagePanel)
{
    WebInspector.StorageCategoryTreeElement.call(this, storagePanel, WebInspector.UIString("IndexedDB"), "IndexedDB", ["indexed-db-storage-tree-item"]);
}

WebInspector.IndexedDBTreeElement.prototype = {
    onexpand: function()
    {
        WebInspector.StorageCategoryTreeElement.prototype.onexpand.call(this);
        if (!this._indexedDBModel)
            this._createIndexedDBModel();
    },

    onattach: function()
    {
        WebInspector.StorageCategoryTreeElement.prototype.onattach.call(this);
        this.listItemElement.addEventListener("contextmenu", this._handleContextMenuEvent.bind(this), true);
    },

    _handleContextMenuEvent: function(event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);
        contextMenu.appendItem(WebInspector.UIString("Refresh IndexedDB"), this.refreshIndexedDB.bind(this));
        contextMenu.show();
    },

    _createIndexedDBModel: function()
    {
        this._indexedDBModel = new WebInspector.IndexedDBModel();
        this._idbDatabaseTreeElements = [];
        this._indexedDBModel.addEventListener(WebInspector.IndexedDBModel.EventTypes.DatabaseAdded, this._indexedDBAdded, this);
        this._indexedDBModel.addEventListener(WebInspector.IndexedDBModel.EventTypes.DatabaseRemoved, this._indexedDBRemoved, this);
        this._indexedDBModel.addEventListener(WebInspector.IndexedDBModel.EventTypes.DatabaseLoaded, this._indexedDBLoaded, this);
    },

    refreshIndexedDB: function()
    {
        if (!this._indexedDBModel) {
            this._createIndexedDBModel();
            return;
        }

        this._indexedDBModel.refreshDatabaseNames();
    },

    /**
     * @param {WebInspector.Event} event
     */
    _indexedDBAdded: function(event)
    {
        var databaseId = /** @type {WebInspector.IndexedDBModel.DatabaseId} */ (event.data);

        var idbDatabaseTreeElement = new WebInspector.IDBDatabaseTreeElement(this._storagePanel, this._indexedDBModel, databaseId);
        this._idbDatabaseTreeElements.push(idbDatabaseTreeElement);
        this.appendChild(idbDatabaseTreeElement);

        this._indexedDBModel.refreshDatabase(databaseId);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _indexedDBRemoved: function(event)
    {
        var databaseId = /** @type {WebInspector.IndexedDBModel.DatabaseId} */ (event.data);

        var idbDatabaseTreeElement = this._idbDatabaseTreeElement(databaseId)
        if (!idbDatabaseTreeElement)
            return;

        idbDatabaseTreeElement.clear();
        this.removeChild(idbDatabaseTreeElement);
        this._idbDatabaseTreeElements.remove(idbDatabaseTreeElement);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _indexedDBLoaded: function(event)
    {
        var database = /** @type {WebInspector.IndexedDBModel.Database} */ (event.data);

        var idbDatabaseTreeElement = this._idbDatabaseTreeElement(database.databaseId)
        if (!idbDatabaseTreeElement)
            return;

        idbDatabaseTreeElement.update(database);
    },

    /**
     * @param {WebInspector.IndexedDBModel.DatabaseId} databaseId
     * @return {WebInspector.IDBDatabaseTreeElement}
     */
    _idbDatabaseTreeElement: function(databaseId)
    {
        var index = -1;
        for (var i = 0; i < this._idbDatabaseTreeElements.length; ++i) {
            if (this._idbDatabaseTreeElements[i]._databaseId.equals(databaseId)) {
                index = i;
                break;
            }
        }
        if (index !== -1)
            return this._idbDatabaseTreeElements[i];
        return null;
    },

    __proto__: WebInspector.StorageCategoryTreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.StorageCategoryTreeElement}
 * @param {WebInspector.ResourcesPanel} storagePanel
 */
WebInspector.FileSystemListTreeElement = function(storagePanel)
{
    WebInspector.StorageCategoryTreeElement.call(this, storagePanel, WebInspector.UIString("FileSystem"), "FileSystem", ["file-system-storage-tree-item"]);
}

WebInspector.FileSystemListTreeElement.prototype = {
    onexpand: function()
    {
        WebInspector.StorageCategoryTreeElement.prototype.onexpand.call(this);
        this._refreshFileSystem();
    },

    onattach: function()
    {
        WebInspector.StorageCategoryTreeElement.prototype.onattach.call(this);
        this.listItemElement.addEventListener("contextmenu", this._handleContextMenuEvent.bind(this), true);
    },

    _handleContextMenuEvent: function(event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);
        contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Refresh FileSystem list" : "Refresh FileSystem List"), this._refreshFileSystem.bind(this));
        contextMenu.show();
    },

    _fileSystemAdded: function(event)
    {
        var fileSystem = /** @type {WebInspector.FileSystemModel.FileSystem} */ (event.data);
        var fileSystemTreeElement = new WebInspector.FileSystemTreeElement(this._storagePanel, fileSystem);
        this.appendChild(fileSystemTreeElement);
    },

    _fileSystemRemoved: function(event)
    {
        var fileSystem = /** @type {WebInspector.FileSystemModel.FileSystem} */ (event.data);
        var fileSystemTreeElement = this._fileSystemTreeElementByName(fileSystem.name);
        if (!fileSystemTreeElement)
            return;
        fileSystemTreeElement.clear();
        this.removeChild(fileSystemTreeElement);
    },

    _fileSystemTreeElementByName: function(fileSystemName)
    {
        for (var i = 0; i < this.children.length; ++i) {
            var child = /** @type {WebInspector.FileSystemTreeElement} */ (this.children[i]);
            if (child.fileSystemName === fileSystemName)
                return this.children[i];
        }
        return null;
    },

    _refreshFileSystem: function()
    {
        if (!this._fileSystemModel) {
            this._fileSystemModel = new WebInspector.FileSystemModel();
            this._fileSystemModel.addEventListener(WebInspector.FileSystemModel.EventTypes.FileSystemAdded, this._fileSystemAdded, this);
            this._fileSystemModel.addEventListener(WebInspector.FileSystemModel.EventTypes.FileSystemRemoved, this._fileSystemRemoved, this);
        }

        this._fileSystemModel.refreshFileSystemList();
    },

    __proto__: WebInspector.StorageCategoryTreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.BaseStorageTreeElement}
 * @param {WebInspector.ResourcesPanel} storagePanel
 * @param {WebInspector.IndexedDBModel} model
 * @param {WebInspector.IndexedDBModel.DatabaseId} databaseId
 */
WebInspector.IDBDatabaseTreeElement = function(storagePanel, model, databaseId)
{
    WebInspector.BaseStorageTreeElement.call(this, storagePanel, null, databaseId.name + " - " + databaseId.securityOrigin, ["indexed-db-storage-tree-item"]);
    this._model = model;
    this._databaseId = databaseId;
    this._idbObjectStoreTreeElements = {};
}

WebInspector.IDBDatabaseTreeElement.prototype = {
    get itemURL()
    {
        return "indexedDB://" + this._databaseId.securityOrigin + "/" + this._databaseId.name;
    },

    onattach: function()
    {
        WebInspector.BaseStorageTreeElement.prototype.onattach.call(this);
        this.listItemElement.addEventListener("contextmenu", this._handleContextMenuEvent.bind(this), true);
    },

    _handleContextMenuEvent: function(event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);
        contextMenu.appendItem(WebInspector.UIString("Refresh IndexedDB"), this._refreshIndexedDB.bind(this));
        contextMenu.show();
    },

    _refreshIndexedDB: function()
    {
        this._model.refreshDatabaseNames();
    },

    /**
     * @param {WebInspector.IndexedDBModel.Database} database
     */
    update: function(database)
    {
        this._database = database;
        var objectStoreNames = {};
        for (var objectStoreName in this._database.objectStores) {
            var objectStore = this._database.objectStores[objectStoreName];
            objectStoreNames[objectStore.name] = true;
            if (!this._idbObjectStoreTreeElements[objectStore.name]) {
                var idbObjectStoreTreeElement = new WebInspector.IDBObjectStoreTreeElement(this._storagePanel, this._model, this._databaseId, objectStore);
                this._idbObjectStoreTreeElements[objectStore.name] = idbObjectStoreTreeElement;
                this.appendChild(idbObjectStoreTreeElement);
            }
            this._idbObjectStoreTreeElements[objectStore.name].update(objectStore);
        }
        for (var objectStoreName in this._idbObjectStoreTreeElements) {
            if (!objectStoreNames[objectStoreName])
                this._objectStoreRemoved(objectStoreName);
        }

        if (this.children.length) {
            this.hasChildren = true;
            this.expand();
        }

        if (this._view)
            this._view.update(database);

        this._updateTooltip();
    },

    _updateTooltip: function()
    {
        this.tooltip = WebInspector.UIString("Version") + ": " + this._database.version;
    },

    onselect: function(selectedByUser)
    {
        WebInspector.BaseStorageTreeElement.prototype.onselect.call(this, selectedByUser);
        if (!this._view)
            this._view = new WebInspector.IDBDatabaseView(this._database);

        this._storagePanel.showIndexedDB(this._view);
    },

    /**
     * @param {string} objectStoreName
     */
    _objectStoreRemoved: function(objectStoreName)
    {
        var objectStoreTreeElement = this._idbObjectStoreTreeElements[objectStoreName];
        objectStoreTreeElement.clear();
        this.removeChild(objectStoreTreeElement);
        delete this._idbObjectStoreTreeElements[objectStoreName];
    },

    clear: function()
    {
        for (var objectStoreName in this._idbObjectStoreTreeElements)
            this._objectStoreRemoved(objectStoreName);
    },

    __proto__: WebInspector.BaseStorageTreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.BaseStorageTreeElement}
 * @param {WebInspector.ResourcesPanel} storagePanel
 * @param {WebInspector.IndexedDBModel} model
 * @param {WebInspector.IndexedDBModel.DatabaseId} databaseId
 * @param {WebInspector.IndexedDBModel.ObjectStore} objectStore
 */
WebInspector.IDBObjectStoreTreeElement = function(storagePanel, model, databaseId, objectStore)
{
    WebInspector.BaseStorageTreeElement.call(this, storagePanel, null, objectStore.name, ["indexed-db-object-store-storage-tree-item"]);
    this._model = model;
    this._databaseId = databaseId;
    this._idbIndexTreeElements = {};
}

WebInspector.IDBObjectStoreTreeElement.prototype = {
    get itemURL()
    {
        return "indexedDB://" + this._databaseId.securityOrigin + "/" + this._databaseId.name + "/" + this._objectStore.name;
    },

    onattach: function()
    {
        WebInspector.BaseStorageTreeElement.prototype.onattach.call(this);
        this.listItemElement.addEventListener("contextmenu", this._handleContextMenuEvent.bind(this), true);
    },

    _handleContextMenuEvent: function(event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);
        contextMenu.appendItem(WebInspector.UIString("Clear"), this._clearObjectStore.bind(this));
        contextMenu.show();
    },

    _clearObjectStore: function()
    {
        function callback() {
            this.update(this._objectStore);
        }
        this._model.clearObjectStore(this._databaseId, this._objectStore.name, callback.bind(this));
    },

   /**
     * @param {WebInspector.IndexedDBModel.ObjectStore} objectStore
     */
    update: function(objectStore)
    {
        this._objectStore = objectStore;

        var indexNames = {};
        for (var indexName in this._objectStore.indexes) {
            var index = this._objectStore.indexes[indexName];
            indexNames[index.name] = true;
            if (!this._idbIndexTreeElements[index.name]) {
                var idbIndexTreeElement = new WebInspector.IDBIndexTreeElement(this._storagePanel, this._model, this._databaseId, this._objectStore, index);
                this._idbIndexTreeElements[index.name] = idbIndexTreeElement;
                this.appendChild(idbIndexTreeElement);
            }
            this._idbIndexTreeElements[index.name].update(index);
        }
        for (var indexName in this._idbIndexTreeElements) {
            if (!indexNames[indexName])
                this._indexRemoved(indexName);
        }
        for (var indexName in this._idbIndexTreeElements) {
            if (!indexNames[indexName]) {
                this.removeChild(this._idbIndexTreeElements[indexName]);
                delete this._idbIndexTreeElements[indexName];
            }
        }

        if (this.children.length) {
            this.hasChildren = true;
            this.expand();
        }

        if (this._view)
            this._view.update(this._objectStore);

        this._updateTooltip();
    },

    _updateTooltip: function()
    {

        var keyPathString = this._objectStore.keyPathString;
        var tooltipString = keyPathString !== null ? (WebInspector.UIString("Key path: ") + keyPathString) : "";
        if (this._objectStore.autoIncrement)
            tooltipString += "\n" + WebInspector.UIString("autoIncrement");
        this.tooltip = tooltipString
    },

    onselect: function(selectedByUser)
    {
        WebInspector.BaseStorageTreeElement.prototype.onselect.call(this, selectedByUser);
        if (!this._view)
            this._view = new WebInspector.IDBDataView(this._model, this._databaseId, this._objectStore, null);

        this._storagePanel.showIndexedDB(this._view);
    },

    /**
     * @param {string} indexName
     */
    _indexRemoved: function(indexName)
    {
        var indexTreeElement = this._idbIndexTreeElements[indexName];
        indexTreeElement.clear();
        this.removeChild(indexTreeElement);
        delete this._idbIndexTreeElements[indexName];
    },

    clear: function()
    {
        for (var indexName in this._idbIndexTreeElements)
            this._indexRemoved(indexName);
        if (this._view)
            this._view.clear();
    },

    __proto__: WebInspector.BaseStorageTreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.BaseStorageTreeElement}
 * @param {WebInspector.ResourcesPanel} storagePanel
 * @param {WebInspector.IndexedDBModel} model
 * @param {WebInspector.IndexedDBModel.DatabaseId} databaseId
 * @param {WebInspector.IndexedDBModel.ObjectStore} objectStore
 * @param {WebInspector.IndexedDBModel.Index} index
 */
WebInspector.IDBIndexTreeElement = function(storagePanel, model, databaseId, objectStore, index)
{
    WebInspector.BaseStorageTreeElement.call(this, storagePanel, null, index.name, ["indexed-db-index-storage-tree-item"]);
    this._model = model;
    this._databaseId = databaseId;
    this._objectStore = objectStore;
    this._index = index;
}

WebInspector.IDBIndexTreeElement.prototype = {
    get itemURL()
    {
        return "indexedDB://" + this._databaseId.securityOrigin + "/" + this._databaseId.name + "/" + this._objectStore.name + "/" + this._index.name;
    },

    /**
     * @param {WebInspector.IndexedDBModel.Index} index
     */
    update: function(index)
    {
        this._index = index;

        if (this._view)
            this._view.update(this._index);

        this._updateTooltip();
    },

    _updateTooltip: function()
    {
        var tooltipLines = [];
        var keyPathString = this._index.keyPathString;
        tooltipLines.push(WebInspector.UIString("Key path: ") + keyPathString);
        if (this._index.unique)
            tooltipLines.push(WebInspector.UIString("unique"));
        if (this._index.multiEntry)
            tooltipLines.push(WebInspector.UIString("multiEntry"));
        this.tooltip = tooltipLines.join("\n");
    },

    onselect: function(selectedByUser)
    {
        WebInspector.BaseStorageTreeElement.prototype.onselect.call(this, selectedByUser);
        if (!this._view)
            this._view = new WebInspector.IDBDataView(this._model, this._databaseId, this._objectStore, this._index);

        this._storagePanel.showIndexedDB(this._view);
    },

    clear: function()
    {
        if (this._view)
            this._view.clear();
    },

    __proto__: WebInspector.BaseStorageTreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.BaseStorageTreeElement}
 */
WebInspector.DOMStorageTreeElement = function(storagePanel, domStorage, className)
{
    WebInspector.BaseStorageTreeElement.call(this, storagePanel, null, domStorage.securityOrigin ? domStorage.securityOrigin : WebInspector.UIString("Local Files"), ["domstorage-storage-tree-item", className]);
    this._domStorage = domStorage;
}

WebInspector.DOMStorageTreeElement.prototype = {
    get itemURL()
    {
        return "storage://" + this._domStorage.securityOrigin + "/" + (this._domStorage.isLocalStorage ? "local" : "session");
    },

    onselect: function(selectedByUser)
    {
        WebInspector.BaseStorageTreeElement.prototype.onselect.call(this, selectedByUser);
        this._storagePanel._showDOMStorage(this._domStorage);
    },

    __proto__: WebInspector.BaseStorageTreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.BaseStorageTreeElement}
 */
WebInspector.CookieTreeElement = function(storagePanel, cookieDomain)
{
    WebInspector.BaseStorageTreeElement.call(this, storagePanel, null, cookieDomain ? cookieDomain : WebInspector.UIString("Local Files"), ["cookie-storage-tree-item"]);
    this._cookieDomain = cookieDomain;
}

WebInspector.CookieTreeElement.prototype = {
    get itemURL()
    {
        return "cookies://" + this._cookieDomain;
    },

    onattach: function()
    {
        WebInspector.BaseStorageTreeElement.prototype.onattach.call(this);
        this.listItemElement.addEventListener("contextmenu", this._handleContextMenuEvent.bind(this), true);
    },

    /**
     * @param {Event} event
     */
    _handleContextMenuEvent: function(event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);
        contextMenu.appendItem(WebInspector.UIString("Clear"), this._clearCookies.bind(this));
        contextMenu.show();
    },

    /**
     * @param {string} domain
     */
    _clearCookies: function(domain)
    {
        this._storagePanel.clearCookies(this._cookieDomain);
    },

    onselect: function(selectedByUser)
    {
        WebInspector.BaseStorageTreeElement.prototype.onselect.call(this, selectedByUser);
        this._storagePanel.showCookies(this, this._cookieDomain);
    },

    __proto__: WebInspector.BaseStorageTreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.BaseStorageTreeElement}
 */
WebInspector.ApplicationCacheManifestTreeElement = function(storagePanel, manifestURL)
{
    var title = new WebInspector.ParsedURL(manifestURL).displayName;
    WebInspector.BaseStorageTreeElement.call(this, storagePanel, null, title, ["application-cache-storage-tree-item"]);
    this.tooltip = manifestURL;
    this._manifestURL = manifestURL;
}

WebInspector.ApplicationCacheManifestTreeElement.prototype = {
    get itemURL()
    {
        return "appcache://" + this._manifestURL;
    },

    get manifestURL()
    {
        return this._manifestURL;
    },

    onselect: function(selectedByUser)
    {
        WebInspector.BaseStorageTreeElement.prototype.onselect.call(this, selectedByUser);
        this._storagePanel.showCategoryView(this._manifestURL);
    },

    __proto__: WebInspector.BaseStorageTreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.BaseStorageTreeElement}
 */
WebInspector.ApplicationCacheFrameTreeElement = function(storagePanel, frameId, manifestURL)
{
    WebInspector.BaseStorageTreeElement.call(this, storagePanel, null, "", ["frame-storage-tree-item"]);
    this._frameId = frameId;
    this._manifestURL = manifestURL;
    this._refreshTitles();
}

WebInspector.ApplicationCacheFrameTreeElement.prototype = {
    get itemURL()
    {
        return "appcache://" + this._manifestURL + "/" + encodeURI(this.displayName);
    },

    get frameId()
    {
        return this._frameId;
    },

    get manifestURL()
    {
        return this._manifestURL;
    },

    _refreshTitles: function()
    {
        var frame = WebInspector.resourceTreeModel.frameForId(this._frameId);
        if (!frame) {
            this.subtitleText = WebInspector.UIString("new frame");
            return;
        }
        this.titleText = frame.name;
        this.subtitleText = new WebInspector.ParsedURL(frame.url).displayName;
    },

    frameNavigated: function()
    {
        this._refreshTitles();
    },

    onselect: function(selectedByUser)
    {
        WebInspector.BaseStorageTreeElement.prototype.onselect.call(this, selectedByUser);
        this._storagePanel.showApplicationCache(this._frameId);
    },

    __proto__: WebInspector.BaseStorageTreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.BaseStorageTreeElement}
 * @param {WebInspector.ResourcesPanel} storagePanel
 * @param {WebInspector.FileSystemModel.FileSystem} fileSystem
 */
WebInspector.FileSystemTreeElement = function(storagePanel, fileSystem)
{
    var displayName = fileSystem.type + " - " + fileSystem.origin;
    WebInspector.BaseStorageTreeElement.call(this, storagePanel, null, displayName, ["file-system-storage-tree-item"]);
    this._fileSystem = fileSystem;
}

WebInspector.FileSystemTreeElement.prototype = {
    get fileSystemName()
    {
        return this._fileSystem.name;
    },

    get itemURL()
    {
        return "filesystem://" + this._fileSystem.name;
    },

    onselect: function(selectedByUser)
    {
        WebInspector.BaseStorageTreeElement.prototype.onselect.call(this, selectedByUser);
        this._fileSystemView = new WebInspector.FileSystemView(this._fileSystem);
        this._storagePanel.showFileSystem(this._fileSystemView);
    },

    clear: function()
    {
        if (this.fileSystemView && this._storagePanel.visibleView === this.fileSystemView)
            this._storagePanel.closeVisibleView();
    },

    __proto__: WebInspector.BaseStorageTreeElement.prototype
}

/**
 * @constructor
 * @extends {WebInspector.View}
 */
WebInspector.StorageCategoryView = function()
{
    WebInspector.View.call(this);

    this.element.addStyleClass("storage-view");
    this._emptyView = new WebInspector.EmptyView("");
    this._emptyView.show(this.element);
}

WebInspector.StorageCategoryView.prototype = {
    setText: function(text)
    {
        this._emptyView.text = text;
    },

    __proto__: WebInspector.View.prototype
}

/**
 * @constructor
 * @param {WebInspector.BaseStorageTreeElement} rootElement
 * @param {number} matchesCount
 */
WebInspector.ResourcesSearchController = function(rootElement, matchesCount)
{
    this._root = rootElement;
    this._matchesCount = matchesCount;
    this._traverser = new WebInspector.SearchResultsTreeElementsTraverser(rootElement);
    this._lastTreeElement = null;
    this._lastIndex = -1;
}

WebInspector.ResourcesSearchController.prototype = {
    /**
     * @param {WebInspector.BaseStorageTreeElement} currentTreeElement
     */
    nextSearchResult: function(currentTreeElement)
    {
        if (!currentTreeElement)
            return this._searchResult(this._traverser.first(), 0, 1);

        if (!currentTreeElement.searchMatchesCount)
            return this._searchResult(this._traverser.next(currentTreeElement), 0);

        if (this._lastTreeElement !== currentTreeElement || this._lastIndex === -1)
            return this._searchResult(currentTreeElement, 0);

        if (this._lastIndex === currentTreeElement.searchMatchesCount - 1)
            return this._searchResult(this._traverser.next(currentTreeElement), 0, this._currentMatchIndex % this._matchesCount + 1);

        return this._searchResult(currentTreeElement, this._lastIndex + 1, this._currentMatchIndex + 1);
    },

    /**
     * @param {WebInspector.BaseStorageTreeElement} currentTreeElement
     */
    previousSearchResult: function(currentTreeElement)
    {
        if (!currentTreeElement) {
            var treeElement = this._traverser.last();
            return this._searchResult(treeElement, treeElement.searchMatchesCount - 1, this._matchesCount);
        }

        if (currentTreeElement.searchMatchesCount && this._lastTreeElement === currentTreeElement) {
            if (this._lastIndex > 0)
                return this._searchResult(currentTreeElement, this._lastIndex - 1, this._currentMatchIndex - 1);
            else {
                var treeElement = this._traverser.previous(currentTreeElement);
                var currentMatchIndex = this._currentMatchIndex - 1 ? this._currentMatchIndex - 1 : this._matchesCount;
                return this._searchResult(treeElement, treeElement.searchMatchesCount - 1, currentMatchIndex);
            }
        }

        var treeElement = this._traverser.previous(currentTreeElement)
        return this._searchResult(treeElement, treeElement.searchMatchesCount - 1);
    },

    /**
     * @param {WebInspector.BaseStorageTreeElement} treeElement
     * @param {number} index
     * @param {number=} currentMatchIndex
     * @return {Object}
     */
    _searchResult: function(treeElement, index, currentMatchIndex)
    {
        this._lastTreeElement = treeElement;
        this._lastIndex = index;
        if (!currentMatchIndex)
            currentMatchIndex = this._traverser.matchIndex(treeElement, index);
        this._currentMatchIndex = currentMatchIndex;
        return {treeElement: treeElement, index: index, currentMatchIndex: currentMatchIndex};
    }
}

/**
 * @constructor
 * @param {WebInspector.BaseStorageTreeElement} rootElement
 */
WebInspector.SearchResultsTreeElementsTraverser = function(rootElement)
{
    this._root = rootElement;
}

WebInspector.SearchResultsTreeElementsTraverser.prototype = {
    /**
     * @return {WebInspector.BaseStorageTreeElement}
     */
    first: function()
    {
        return this.next(this._root);
    },

    /**
     * @return {WebInspector.BaseStorageTreeElement}
     */
    last: function()
    {
        return this.previous(this._root);
    },

    /**
     * @param {WebInspector.BaseStorageTreeElement} startTreeElement
     * @return {WebInspector.BaseStorageTreeElement}
     */
    next: function(startTreeElement)
    {
        var treeElement = startTreeElement;
        do {
            treeElement = this._traverseNext(treeElement) || this._root;
        } while (treeElement != startTreeElement && !this._elementSearchMatchesCount(treeElement));
        return treeElement;
    },

    /**
     * @param {WebInspector.BaseStorageTreeElement} startTreeElement
     * @return {WebInspector.BaseStorageTreeElement}
     */
    previous: function(startTreeElement)
    {
        var treeElement = startTreeElement;
        do {
            treeElement = this._traversePrevious(treeElement) || this._lastTreeElement();
        } while (treeElement != startTreeElement && !this._elementSearchMatchesCount(treeElement));
        return treeElement;
    },

    /**
     * @param {WebInspector.BaseStorageTreeElement} startTreeElement
     * @param {number} index
     * @return {number}
     */
    matchIndex: function(startTreeElement, index)
    {
        var matchIndex = 1;
        var treeElement = this._root;
        while (treeElement != startTreeElement) {
            matchIndex += this._elementSearchMatchesCount(treeElement);
            treeElement = this._traverseNext(treeElement) || this._root;
            if (treeElement === this._root)
                return 0;
        }
        return matchIndex + index;
    },

    /**
     * @param {WebInspector.BaseStorageTreeElement} treeElement
     * @return {number}
     */
    _elementSearchMatchesCount: function(treeElement)
    {
        return treeElement.searchMatchesCount;
    },

    /**
     * @param {WebInspector.BaseStorageTreeElement} treeElement
     * @return {WebInspector.BaseStorageTreeElement}
     */
    _traverseNext: function(treeElement)
    {
        return /** @type {WebInspector.BaseStorageTreeElement} */ (treeElement.traverseNextTreeElement(false, this._root, true));
    },

    /**
     * @param {WebInspector.BaseStorageTreeElement} treeElement
     * @return {WebInspector.BaseStorageTreeElement}
     */
    _traversePrevious: function(treeElement)
    {
        return /** @type {WebInspector.BaseStorageTreeElement} */ (treeElement.traversePreviousTreeElement(false, true));
    },

    /**
     * @return {WebInspector.BaseStorageTreeElement}
     */
    _lastTreeElement: function()
    {
        var treeElement = this._root;
        var nextTreeElement;
        while (nextTreeElement = this._traverseNext(treeElement))
            treeElement = nextTreeElement;
        return treeElement;
    }
}
