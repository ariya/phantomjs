/*
 * Copyright (C) 2010, 2013 Apple Inc. All rights reserved.
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
WebInspector.ApplicationCacheFrameContentView = function(representedObject)
{
    console.assert(representedObject instanceof WebInspector.ApplicationCacheFrame);

    WebInspector.ContentView.call(this, representedObject);

    this.element.classList.add(WebInspector.ApplicationCacheFrameContentView.StyleClassName);

    this.element.classList.add("storage-view");
    this.element.classList.add("table");

    this._frame = representedObject.frame;

    this._emptyView = WebInspector.createMessageTextView(WebInspector.UIString("No Application Cache information available"), false);
    this._emptyView.classList.add("hidden");
    this.element.appendChild(this._emptyView);

    this._markDirty();
    
    var status = representedObject.status;
    this.updateStatus(status);

    WebInspector.applicationCacheManager.addEventListener(WebInspector.ApplicationCacheManager.Event.FrameManifestStatusChanged, this._updateStatus, this);
}

WebInspector.ApplicationCacheFrameContentView.StyleClassName = "application-cache-frame";

WebInspector.ApplicationCacheFrameContentView.prototype = {
    constructor: WebInspector.ApplicationCacheFrameContentView,

    shown: function()
    {
        this._maybeUpdate();
    },

    closed: function()
    {
        WebInspector.applicationCacheManager.removeEventListener(WebInspector.ApplicationCacheManager.Event.FrameManifestStatusChanged, this._updateStatus, this);
    },

    updateLayout: function()
    {
        if (this.dataGrid)
            this.dataGrid.updateLayout();
    },

    get scrollableElements()
    {
        if (!this._dataGrid)
            return [];
        return [this._dataGrid.scrollContainer];
    },

    _maybeUpdate: function()
    {
        if (!this.visible || !this._viewDirty)
            return;
        
        this._update();
        this._viewDirty = false;
    },

    _markDirty: function()
    {
        this._viewDirty = true;
    },

    _updateStatus: function(event)
    {
        var frameManifest = event.data.frameManifest;
        if (frameManifest !== this.representedObject)
            return;
        
        console.assert(frameManifest instanceof WebInspector.ApplicationCacheFrame);
        
        this.updateStatus(frameManifest.status);
    },

    /**
     * @param {number} status
     */
    updateStatus: function(status)
    {
        var oldStatus = this._status;
        this._status = status;
        
        if (this.visible && this._status === WebInspector.ApplicationCacheManager.Status.Idle && (oldStatus === WebInspector.ApplicationCacheManager.Status.UpdateReady || !this._resources))
            this._markDirty();

        this._maybeUpdate();
    },

    _update: function()
    {
        WebInspector.applicationCacheManager.requestApplicationCache(this._frame, this._updateCallback.bind(this));
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
            
            this._emptyView.classList.remove("hidden");
            
            if (this._dataGrid)
                this._dataGrid.element.classList.add("hidden");
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
        this._dataGrid.element.classList.remove("hidden");
        
        this._emptyView.classList.add("hidden");
    },

    _createDataGrid: function()
    {
        var columns = { 0: {}, 1: {}, 2: {} };
        columns[0].title = WebInspector.UIString("Resource");
        columns[0].sort = "ascending";
        columns[0].sortable = true;
        columns[1].title = WebInspector.UIString("Type");
        columns[1].sortable = true;
        columns[2].title = WebInspector.UIString("Size");
        columns[2].aligned = "right";
        columns[2].sortable = true;
        this._dataGrid = new WebInspector.DataGrid(columns);
        this.element.appendChild(this._dataGrid.element);
        this._dataGrid.updateLayout();
        this._dataGrid.addEventListener(WebInspector.DataGrid.Event.SortChanged, this._populateDataGrid, this);
    },

    _populateDataGrid: function()
    {
        var selectedResource = this._dataGrid.selectedNode ? this._dataGrid.selectedNode.resource : null;
        var sortDirection = this._dataGrid.sortOrder === "ascending" ? 1 : -1;

        function numberCompare(field, resource1, resource2)
        {
            return sortDirection * (resource1[field] - resource2[field]);
        }
        function localeCompare(field, resource1, resource2)
        {
             return sortDirection * (resource1[field] + "").localeCompare(resource2[field] + "")
        }

        var comparator;
        switch (parseInt(this._dataGrid.sortColumnIdentifier, 10)) {
            case 0: comparator = localeCompare.bind(this, "url"); break;
            case 1: comparator = localeCompare.bind(this, "type"); break;
            case 2: comparator = numberCompare.bind(this, "size"); break;
            default: localeCompare.bind(this, "resource"); // FIXME: comparator = ?
        }

        this._resources.sort(comparator);
        this._dataGrid.removeChildren();

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
            this._dataGrid.appendChild(node);
            if (resource === selectedResource) {
                nodeToSelect = node;
                nodeToSelect.selected = true;
            }
        }

        if (!nodeToSelect && this._dataGrid.children.length)
            this._dataGrid.children[0].selected = true;
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
    }
}

WebInspector.ApplicationCacheFrameContentView.prototype.__proto__ = WebInspector.ContentView.prototype;
