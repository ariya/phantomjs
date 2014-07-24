/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

/**
 * @constructor
 * @extends {WebInspector.View}
 */
WebInspector.ApplicationCacheItemsView = function(model, frameId)
{
    WebInspector.View.call(this);
    
    this._model = model;

    this.element.addStyleClass("storage-view");
    this.element.addStyleClass("table");

    // FIXME: Needs better tooltip. (Localized)
    this.deleteButton = new WebInspector.StatusBarButton(WebInspector.UIString("Delete"), "delete-storage-status-bar-item");
    this.deleteButton.visible = false;
    this.deleteButton.addEventListener("click", this._deleteButtonClicked, this);

    this.connectivityIcon = document.createElement("img");
    this.connectivityIcon.className = "storage-application-cache-connectivity-icon";
    this.connectivityIcon.src = "";
    this.connectivityMessage = document.createElement("span");
    this.connectivityMessage.className = "storage-application-cache-connectivity";
    this.connectivityMessage.textContent = "";

    this.divider = document.createElement("span");
    this.divider.className = "status-bar-item status-bar-divider";

    this.statusIcon = document.createElement("img");
    this.statusIcon.className = "storage-application-cache-status-icon";
    this.statusIcon.src = "";
    this.statusMessage = document.createElement("span");
    this.statusMessage.className = "storage-application-cache-status";
    this.statusMessage.textContent = "";

    this._frameId = frameId;

    this._emptyView = new WebInspector.EmptyView(WebInspector.UIString("No Application Cache information available."));
    this._emptyView.show(this.element);

    this._markDirty();
    
    var status = this._model.frameManifestStatus(frameId);
    this.updateStatus(status);
    
    this.updateNetworkState(this._model.onLine);

    // FIXME: Status bar items don't work well enough yet, so they are being hidden.
    // http://webkit.org/b/41637 Web Inspector: Give Semantics to "Refresh" and "Delete" Buttons in ApplicationCache DataGrid
    this.deleteButton.element.style.display = "none";
}

WebInspector.ApplicationCacheItemsView.prototype = {
    statusBarItems: function()
    {
        return [
            this.deleteButton.element,
            this.connectivityIcon, this.connectivityMessage, this.divider,
            this.statusIcon, this.statusMessage
        ];
    },

    wasShown: function()
    {
        this._maybeUpdate();
    },

    willHide: function()
    {
        this.deleteButton.visible = false;
    },

    _maybeUpdate: function()
    {
        if (!this.isShowing() || !this._viewDirty)
            return;
        
        this._update();
        this._viewDirty = false;
    },

    _markDirty: function()
    {
        this._viewDirty = true;
    },

    /**
     * @param {number} status
     */
    updateStatus: function(status)
    {
        var oldStatus = this._status;
        this._status = status;
        
        var statusInformation = {};
        // We should never have UNCACHED status, since we remove frames with UNCACHED application cache status from the tree. 
        statusInformation[applicationCache.UNCACHED]    = { src: "Images/errorRedDot.png", text: "UNCACHED" };
        statusInformation[applicationCache.IDLE]        = { src: "Images/successGreenDot.png", text: "IDLE" };
        statusInformation[applicationCache.CHECKING]    = { src: "Images/warningOrangeDot.png",  text: "CHECKING" };
        statusInformation[applicationCache.DOWNLOADING] = { src: "Images/warningOrangeDot.png",  text: "DOWNLOADING" };
        statusInformation[applicationCache.UPDATEREADY] = { src: "Images/successGreenDot.png",  text: "UPDATEREADY" };
        statusInformation[applicationCache.OBSOLETE]    = { src: "Images/errorRedDot.png",      text: "OBSOLETE" };

        var info = statusInformation[status] || statusInformation[applicationCache.UNCACHED];

        this.statusIcon.src = info.src;
        this.statusMessage.textContent = info.text;
        
        if (this.isShowing() && this._status === applicationCache.IDLE && (oldStatus === applicationCache.UPDATEREADY || !this._resources))
            this._markDirty();
        this._maybeUpdate();
    },

    /**
     * @param {boolean} isNowOnline
     */
    updateNetworkState: function(isNowOnline)
    {
        if (isNowOnline) {
            this.connectivityIcon.src = "Images/successGreenDot.png";
            this.connectivityMessage.textContent = WebInspector.UIString("Online");
        } else {
            this.connectivityIcon.src = "Images/errorRedDot.png";
            this.connectivityMessage.textContent = WebInspector.UIString("Offline");
        }
    },

    _update: function()
    {
        this._model.requestApplicationCache(this._frameId, this._updateCallback.bind(this));
    },

    /**
     * @param {Object} applicationCache
     */
    _updateCallback: function(applicationCache)
    {
        if (!applicationCache || !applicationCache.manifestURL) {
            delete this._manifest;
            delete this._creationTime;
            delete this._updateTime;
            delete this._size;
            delete this._resources;
            
            this._emptyView.show(this.element);
            this.deleteButton.visible = false;
            if (this._dataGrid)
                this._dataGrid.element.addStyleClass("hidden");
            return;
        }
        // FIXME: are these variables needed anywhere else?
        this._manifest = applicationCache.manifestURL;
        this._creationTime = applicationCache.creationTime;
        this._updateTime = applicationCache.updateTime;
        this._size = applicationCache.size;
        this._resources = applicationCache.resources;

        if (!this._dataGrid)
            this._createDataGrid();

        this._populateDataGrid();
        this._dataGrid.autoSizeColumns(20, 80);
        this._dataGrid.element.removeStyleClass("hidden");
        this._emptyView.detach();
        this.deleteButton.visible = true;

        // FIXME: For Chrome, put creationTime and updateTime somewhere.
        // NOTE: localizedString has not yet been added.
        // WebInspector.UIString("(%s) Created: %s Updated: %s", this._size, this._creationTime, this._updateTime);
    },

    _createDataGrid: function()
    {
        var columns = [
            {title: WebInspector.UIString("Resource"), sort: WebInspector.DataGrid.Order.Ascending, sortable: true},
            {title: WebInspector.UIString("Type"), sortable: true},
            {title: WebInspector.UIString("Size"), align: WebInspector.DataGrid.Align.Right, sortable: true}
        ];
        this._dataGrid = new WebInspector.DataGrid(columns);
        this._dataGrid.show(this.element);
        this._dataGrid.addEventListener(WebInspector.DataGrid.Events.SortingChanged, this._populateDataGrid, this);
    },

    _populateDataGrid: function()
    {
        var selectedResource = this._dataGrid.selectedNode ? this._dataGrid.selectedNode.resource : null;
        var sortDirection = this._dataGrid.isSortOrderAscending() ? 1 : -1;

        function numberCompare(field, resource1, resource2)
        {
            return sortDirection * (resource1[field] - resource2[field]);
        }
        function localeCompare(field, resource1, resource2)
        {
             return sortDirection * (resource1[field] + "").localeCompare(resource2[field] + "")
        }

        var comparator;
        switch (parseInt(this._dataGrid.sortColumnIdentifier(), 10)) {
            case 0: comparator = localeCompare.bind(this, "name"); break;
            case 1: comparator = localeCompare.bind(this, "type"); break;
            case 2: comparator = numberCompare.bind(this, "size"); break;
            default: localeCompare.bind(this, "resource"); // FIXME: comparator = ?
        }

        this._resources.sort(comparator);
        this._dataGrid.rootNode().removeChildren();

        var nodeToSelect;
        for (var i = 0; i < this._resources.length; ++i) {
            var data = {};
            var resource = this._resources[i];
            data[0] = resource.url;
            data[1] = resource.type;
            data[2] = Number.bytesToString(resource.size);
            var node = new WebInspector.DataGridNode(data);
            node.resource = resource;
            node.selectable = true;
            this._dataGrid.rootNode().appendChild(node);
            if (resource === selectedResource) {
                nodeToSelect = node;
                nodeToSelect.selected = true;
            }
        }

        if (!nodeToSelect && this._dataGrid.rootNode().children.length)
            this._dataGrid.rootNode().children[0].selected = true;
    },

    _deleteButtonClicked: function(event)
    {
        if (!this._dataGrid || !this._dataGrid.selectedNode)
            return;

        // FIXME: Delete Button semantics are not yet defined. (Delete a single, or all?)
        this._deleteCallback(this._dataGrid.selectedNode);
    },

    _deleteCallback: function(node)
    {
        // FIXME: Should we delete a single (selected) resource or all resources?
        // InspectorBackend.deleteCachedResource(...)
        // this._update();
    },

    __proto__: WebInspector.View.prototype
}

