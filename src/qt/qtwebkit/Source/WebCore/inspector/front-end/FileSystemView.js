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
 * @extends {WebInspector.SidebarView}
 * @param {WebInspector.FileSystemModel.FileSystem} fileSystem
 */
WebInspector.FileSystemView = function(fileSystem)
{
    WebInspector.SidebarView.call(this, WebInspector.SidebarView.SidebarPosition.Start, "FileSystemViewSidebarWidth");
    this.element.addStyleClass("file-system-view");
    this.element.addStyleClass("storage-view");

    var directoryTreeElement = this.element.createChild("ol", "filesystem-directory-tree");
    this._directoryTree = new TreeOutline(directoryTreeElement);
    this.sidebarElement.appendChild(directoryTreeElement);
    this.sidebarElement.addStyleClass("outline-disclosure");
    this.sidebarElement.addStyleClass("sidebar");

    var rootItem = new WebInspector.FileSystemView.EntryTreeElement(this, fileSystem.root);
    rootItem.expanded = true;
    this._directoryTree.appendChild(rootItem);
    this._visibleView = null;

    this._refreshButton = new WebInspector.StatusBarButton(WebInspector.UIString("Refresh"), "refresh-storage-status-bar-item");
    this._refreshButton.visible = true;
    this._refreshButton.addEventListener("click", this._refresh, this);

    this._deleteButton = new WebInspector.StatusBarButton(WebInspector.UIString("Delete"), "delete-storage-status-bar-item");
    this._deleteButton.visible = true;
    this._deleteButton.addEventListener("click", this._confirmDelete, this);
}

WebInspector.FileSystemView.prototype = {
    /**
     * @return {Array.<Element>}
     */
    statusBarItems: function()
    {
        return [this._refreshButton.element, this._deleteButton.element];
    },

    /**
     * @type {WebInspector.View}
     */
    get visibleView()
    {
        return this._visibleView;
    },

    /**
     * @param {WebInspector.View} view
     */
    showView: function(view)
    {
        if (this._visibleView === view)
            return;
        if (this._visibleView)
            this._visibleView.detach();
        this._visibleView = view;
        view.show(this.mainElement);
    },

    _refresh: function()
    {
        this._directoryTree.children[0].refresh();
    },

    _confirmDelete: function()
    {
        if (confirm(WebInspector.UIString("Are you sure you want to delete the selected entry?")))
            this._delete();
    },

    _delete: function()
    {
        this._directoryTree.selectedTreeElement.deleteEntry();
    },

    __proto__: WebInspector.SidebarView.prototype
}

/**
 * @constructor
 * @extends {TreeElement}
 * @param {WebInspector.FileSystemView} fileSystemView
 * @param {WebInspector.FileSystemModel.Entry} entry
 */
WebInspector.FileSystemView.EntryTreeElement = function(fileSystemView, entry)
{
    TreeElement.call(this, entry.name, null, entry.isDirectory);

    this._entry = entry;
    this._fileSystemView = fileSystemView;
}

WebInspector.FileSystemView.EntryTreeElement.prototype = {
    onattach: function()
    {
        var selection = this.listItemElement.createChild("div", "selection");
        this.listItemElement.insertBefore(selection, this.listItemElement.firstChild);
    },

    onselect: function()
    {
        if (!this._view) {
            if (this._entry.isDirectory)
                this._view = new WebInspector.DirectoryContentView();
            else {
                var file = /** @type {WebInspector.FileSystemModel.File} */ (this._entry);
                this._view = new WebInspector.FileContentView(file);
            }
        }
        this._fileSystemView.showView(this._view);
        this.refresh();
    },

    onpopulate: function()
    {
        this.refresh();
    },

    /**
     * @param {number} errorCode
     * @param {Array.<WebInspector.FileSystemModel.Entry>=} entries
     */
    _directoryContentReceived: function(errorCode, entries)
    {
        if (errorCode === FileError.NOT_FOUND_ERR) {
            if (this.parent !== this.treeOutline)
                this.parent.refresh();
            return;
        }

        if (errorCode !== 0 || !entries) {
            console.error("Failed to read directory: " + errorCode);
            return;
        }

        entries.sort(WebInspector.FileSystemModel.Entry.compare);
        if (this._view)
            this._view.showEntries(entries);

        var oldChildren = this.children.slice(0);

        var newEntryIndex = 0;
        var oldChildIndex = 0;
        var currentTreeItem = 0;
        while (newEntryIndex < entries.length && oldChildIndex < oldChildren.length) {
            var newEntry = entries[newEntryIndex];
            var oldChild = oldChildren[oldChildIndex];
            var order = newEntry.name.compareTo(oldChild._entry.name);

            if (order === 0) {
                if (oldChild._entry.isDirectory)
                    oldChild.shouldRefreshChildren = true;
                else
                    oldChild.refresh();

                ++newEntryIndex;
                ++oldChildIndex;
                ++currentTreeItem;
                continue;
            }
            if (order < 0) {
                this.insertChild(new WebInspector.FileSystemView.EntryTreeElement(this._fileSystemView, newEntry), currentTreeItem);
                ++newEntryIndex;
                ++currentTreeItem;
                continue;
            }

            this.removeChildAtIndex(currentTreeItem);
            ++oldChildIndex;
        }
        for (; newEntryIndex < entries.length; ++newEntryIndex)
            this.appendChild(new WebInspector.FileSystemView.EntryTreeElement(this._fileSystemView, entries[newEntryIndex]));

        for (; oldChildIndex < oldChildren.length; ++oldChildIndex)
            this.removeChild(oldChildren[oldChildIndex]);
    },

    refresh: function()
    {
        if (!this._entry.isDirectory) {
            if (this._view && this._view === this._fileSystemView.visibleView) {
                var fileContentView = /** @type {WebInspector.FileContentView} */ (this._view);
                fileContentView.refresh();
            }
        } else
            this._entry.requestDirectoryContent(this._directoryContentReceived.bind(this));
    },

    deleteEntry: function()
    {
        this._entry.deleteEntry(this._deletionCompleted.bind(this));
    },

    _deletionCompleted: function()
    {
        if (this._entry != this._entry.fileSystem.root)
            this.parent.refresh();
    },

    __proto__: TreeElement.prototype
}
