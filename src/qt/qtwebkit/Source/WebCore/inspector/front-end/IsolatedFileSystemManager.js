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
 * @extends {WebInspector.Object}
 */
WebInspector.IsolatedFileSystemManager = function()
{
    /** @type {!Object.<string, WebInspector.IsolatedFileSystem>} */
    this._fileSystems = {};
    /** @type {Object.<string, Array.<function(DOMFileSystem)>>} */
    this._pendingFileSystemRequests = {};
    this._fileSystemMapping = new WebInspector.FileSystemMappingImpl();

    if (this.supportsFileSystems())
        this._requestFileSystems();
}

/** @typedef {{fileSystemName: string, rootURL: string, fileSystemPath: string}} */
WebInspector.IsolatedFileSystemManager.FileSystem;

WebInspector.IsolatedFileSystemManager.Events = {
    FileSystemAdded: "FileSystemAdded",
    FileSystemRemoved: "FileSystemRemoved"
}

WebInspector.IsolatedFileSystemManager.prototype = {
    /**
     * @return {WebInspector.FileSystemMapping}
     */
    mapping: function()
    {
        return this._fileSystemMapping;
    },

    /**
     * @return {boolean}
     */
    supportsFileSystems: function()
    {
        return InspectorFrontendHost.supportsFileSystems();
    },

    _requestFileSystems: function()
    {
        console.assert(!this._loaded);
        InspectorFrontendHost.requestFileSystems();
    },

    /**
     * @param {function(?string)} callback
     */
    addFileSystem: function(callback)
    {
        this._selectFileSystemPathCallback = callback;
        InspectorFrontendHost.addFileSystem();
    },

    /**
     * @param {function()} callback
     */
    removeFileSystem: function(fileSystemPath, callback)
    {
        this._removeFileSystemCallback = callback;
        InspectorFrontendHost.removeFileSystem(fileSystemPath);
    },

    /**
     * @param {Array.<WebInspector.IsolatedFileSystemManager.FileSystem>} fileSystems
     */
    _fileSystemsLoaded: function(fileSystems)
    {
        for (var i = 0; i < fileSystems.length; ++i)
            this._innerAddFileSystem(fileSystems[i]);
        this._loaded = true;
        this._processPendingFileSystemRequests();
    },

    /**
     * @param {WebInspector.IsolatedFileSystemManager.FileSystem} fileSystem
     */
    _innerAddFileSystem: function(fileSystem)
    {
        var fileSystemPath = fileSystem.fileSystemPath;
        this._fileSystemMapping.addFileSystemMapping(fileSystemPath);
        var isolatedFileSystem = new WebInspector.IsolatedFileSystem(this, fileSystemPath, fileSystem.fileSystemName, fileSystem.rootURL);
        this._fileSystems[fileSystemPath] = isolatedFileSystem;
        this.dispatchEventToListeners(WebInspector.IsolatedFileSystemManager.Events.FileSystemAdded, isolatedFileSystem);
    },

    /**
     * @return {Array.<string>}
     */
    _fileSystemPaths: function()
    {
        return Object.keys(this._fileSystems);
    },

    _processPendingFileSystemRequests: function()
    {
        for (var fileSystemPath in this._pendingFileSystemRequests) {
            var callbacks = this._pendingFileSystemRequests[fileSystemPath];
            for (var i = 0; i < callbacks.length; ++i)
                callbacks[i](this._isolatedFileSystem(fileSystemPath));
        }
        delete this._pendingFileSystemRequests;
    },

    /**
     * @param {string} errorMessage
     * @param {WebInspector.IsolatedFileSystemManager.FileSystem} fileSystem
     */
    _fileSystemAdded: function(errorMessage, fileSystem)
    {
        var fileSystemPath;
        if (errorMessage)
            WebInspector.showErrorMessage(errorMessage)
        else if (fileSystem) {
            this._innerAddFileSystem(fileSystem);
            fileSystemPath = fileSystem.fileSystemPath;
        }

        if (this._selectFileSystemPathCallback) {
            this._selectFileSystemPathCallback(fileSystemPath);
            delete this._selectFileSystemPathCallback;
        }
    },

    /**
     * @param {string} fileSystemPath
     */
    _fileSystemRemoved: function(fileSystemPath)
    {
        this._fileSystemMapping.removeFileSystemMapping(fileSystemPath);
        var isolatedFileSystem = this._fileSystems[fileSystemPath];
        delete this._fileSystems[fileSystemPath];
        if (this._removeFileSystemCallback) {
            this._removeFileSystemCallback(fileSystemPath);
            delete this._removeFileSystemCallback;
        }
        this.dispatchEventToListeners(WebInspector.IsolatedFileSystemManager.Events.FileSystemRemoved, isolatedFileSystem);
    },

    /**
     * @param {string} fileSystemPath
     * @return {DOMFileSystem}
     */
    _isolatedFileSystem: function(fileSystemPath)
    {
        var fileSystem = this._fileSystems[fileSystemPath];
        if (!fileSystem)
            return null;
        if (!InspectorFrontendHost.isolatedFileSystem)
            return null;
        return InspectorFrontendHost.isolatedFileSystem(fileSystem.name(), fileSystem.rootURL());
    },

    /**
     * @param {string} fileSystemPath
     * @param {function(DOMFileSystem)} callback
     */
    requestDOMFileSystem: function(fileSystemPath, callback)
    {
        if (!this._loaded) {
            if (!this._pendingFileSystemRequests[fileSystemPath])
                this._pendingFileSystemRequests[fileSystemPath] = this._pendingFileSystemRequests[fileSystemPath] || [];
            this._pendingFileSystemRequests[fileSystemPath].push(callback);
            return;
        }
        callback(this._isolatedFileSystem(fileSystemPath));
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @type {?WebInspector.IsolatedFileSystemManager}
 */
WebInspector.isolatedFileSystemManager = null;

/**
 * @constructor
 * @param {WebInspector.IsolatedFileSystemManager} IsolatedFileSystemManager
 */
WebInspector.IsolatedFileSystemDispatcher = function(IsolatedFileSystemManager)
{
    this._IsolatedFileSystemManager = IsolatedFileSystemManager;
}

WebInspector.IsolatedFileSystemDispatcher.prototype = {
    /**
     * @param {Array.<WebInspector.IsolatedFileSystemManager.FileSystem>} fileSystems
     */
    fileSystemsLoaded: function(fileSystems)
    {
        this._IsolatedFileSystemManager._fileSystemsLoaded(fileSystems);
    },

    /**
     * @param {string} fileSystemPath
     */
    fileSystemRemoved: function(fileSystemPath)
    {
        this._IsolatedFileSystemManager._fileSystemRemoved(fileSystemPath);
    },

    /**
     * @param {string} errorMessage
     * @param {WebInspector.IsolatedFileSystemManager.FileSystem} fileSystem
     */
    fileSystemAdded: function(errorMessage, fileSystem)
    {
        this._IsolatedFileSystemManager._fileSystemAdded(errorMessage, fileSystem);
    }
}

/**
 * @type {?WebInspector.IsolatedFileSystemDispatcher}
 */
WebInspector.isolatedFileSystemDispatcher = null;
