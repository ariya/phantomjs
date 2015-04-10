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
WebInspector.FileSystemModel = function()
{
    WebInspector.Object.call(this);

    this._fileSystemsForOrigin = {};

    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.SecurityOriginAdded, this._securityOriginAdded, this);
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.SecurityOriginRemoved, this._securityOriginRemoved, this);

    FileSystemAgent.enable();

    this._reset();
}

WebInspector.FileSystemModel.prototype = {
    _reset: function()
    {
        for (var securityOrigin in this._fileSystemsForOrigin)
            this._removeOrigin(securityOrigin);
        var securityOrigins = WebInspector.resourceTreeModel.securityOrigins();
        for (var i = 0; i < securityOrigins.length; ++i)
            this._addOrigin(securityOrigins[i]);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _securityOriginAdded: function(event)
    {
        var securityOrigin = /** @type {string} */ (event.data);
        this._addOrigin(securityOrigin);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _securityOriginRemoved: function(event)
    {
        var securityOrigin = /** @type {string} */ (event.data);
        this._removeOrigin(securityOrigin);
    },

    /**
     * @param {string} securityOrigin
     */
    _addOrigin: function(securityOrigin)
    {
        this._fileSystemsForOrigin[securityOrigin] = {};

        var types = ["persistent", "temporary"];
        for (var i = 0; i < types.length; ++i)
            this._requestFileSystemRoot(securityOrigin, types[i], this._fileSystemRootReceived.bind(this, securityOrigin, types[i], this._fileSystemsForOrigin[securityOrigin]));
    },

    /**
     * @param {string} securityOrigin
     */
    _removeOrigin: function(securityOrigin)
    {
        for (var type in this._fileSystemsForOrigin[securityOrigin]) {
            var fileSystem = this._fileSystemsForOrigin[securityOrigin][type];
            delete this._fileSystemsForOrigin[securityOrigin][type];
            this._fileSystemRemoved(fileSystem);
        }
        delete this._fileSystemsForOrigin[securityOrigin];
    },

    /**
     * @param {string} origin
     * @param {string} type
     * @param {function(number, FileSystemAgent.Entry=)} callback
     */
    _requestFileSystemRoot: function(origin, type, callback)
    {
        /**
         * @param {?Protocol.Error} error
         * @param {number} errorCode
         * @param {FileSystemAgent.Entry=} backendRootEntry
         */
        function innerCallback(error, errorCode, backendRootEntry)
        {
            if (error) {
                callback(FileError.SECURITY_ERR);
                return;
            }
            
            callback(errorCode, backendRootEntry);
        }

        FileSystemAgent.requestFileSystemRoot(origin, type, innerCallback.bind(this));
    },

    /**
     * @param {WebInspector.FileSystemModel.FileSystem} fileSystem
     */
    _fileSystemAdded: function(fileSystem)
    {
        this.dispatchEventToListeners(WebInspector.FileSystemModel.EventTypes.FileSystemAdded, fileSystem);
    },

    /**
     * @param {WebInspector.FileSystemModel.FileSystem} fileSystem
     */
    _fileSystemRemoved: function(fileSystem)
    {
        this.dispatchEventToListeners(WebInspector.FileSystemModel.EventTypes.FileSystemRemoved, fileSystem);
    },

    refreshFileSystemList: function()
    {
        this._reset();
    },

    /**
     * @param {string} origin
     * @param {string} type
     * @param {Object.<WebInspector.FileSystemModel.FileSystem>} store
     * @param {number} errorCode
     * @param {FileSystemAgent.Entry=} backendRootEntry
     */
    _fileSystemRootReceived: function(origin, type, store, errorCode, backendRootEntry)
    {
        if (!errorCode && backendRootEntry && this._fileSystemsForOrigin[origin] === store) {
            var fileSystem = new WebInspector.FileSystemModel.FileSystem(this, origin, type, backendRootEntry);
            store[type] = fileSystem;
            this._fileSystemAdded(fileSystem);
        }
    },

    /**
     * @param {WebInspector.FileSystemModel.Directory} directory
     * @param {function(number, Array.<WebInspector.FileSystemModel.Entry>=)} callback
     */
    requestDirectoryContent: function(directory, callback)
    {
        this._requestDirectoryContent(directory.url, this._directoryContentReceived.bind(this, directory, callback));
    },

    /**
     * @param {string} url
     * @param {function(number, Array.<FileSystemAgent.Entry>=)} callback
     */
    _requestDirectoryContent: function(url, callback)
    {
        /**
         * @param {?Protocol.Error} error
         * @param {number} errorCode
         * @param {Array.<FileSystemAgent.Entry>=} backendEntries
         */
        function innerCallback(error, errorCode, backendEntries)
        {
            if (error) {
                callback(FileError.SECURITY_ERR);
                return;
            }
            
            if (errorCode !== 0) {
                callback(errorCode, null);
                return;
            }

            callback(errorCode, backendEntries);
        }

        FileSystemAgent.requestDirectoryContent(url, innerCallback.bind(this));
    },

    /**
     * @param {WebInspector.FileSystemModel.Directory} parentDirectory
     * @param {function(number, Array.<WebInspector.FileSystemModel.Entry>=)} callback
     * @param {number} errorCode
     * @param {Array.<FileSystemAgent.Entry>=} backendEntries
     */
    _directoryContentReceived: function(parentDirectory, callback, errorCode, backendEntries)
    {
        var entries = [];
        for (var i = 0; i < backendEntries.length; ++i) {
            if (backendEntries[i].isDirectory)
                entries.push(new WebInspector.FileSystemModel.Directory(this, parentDirectory.fileSystem, backendEntries[i]));
            else
                entries.push(new WebInspector.FileSystemModel.File(this, parentDirectory.fileSystem, backendEntries[i]));
        }

        callback(errorCode, entries);
    },

    /**
     * @param {WebInspector.FileSystemModel.Entry} entry
     * @param {function(number, FileSystemAgent.Metadata=)} callback
     */
    requestMetadata: function(entry, callback)
    {
        /**
         * @param {?Protocol.Error} error
         * @param {number} errorCode
         * @param {FileSystemAgent.Metadata=} metadata
         */
        function innerCallback(error, errorCode, metadata)
        {
            if (error) {
                callback(FileError.SECURITY_ERR);
                return;
            }
            
            callback(errorCode, metadata);
        }

        FileSystemAgent.requestMetadata(entry.url, innerCallback.bind(this));
    },

    /**
     * @param {WebInspector.FileSystemModel.File} file
     * @param {boolean} readAsText
     * @param {number=} start
     * @param {number=} end
     * @param {string=} charset
     * @param {function(number, string=, string=)=} callback
     */
    requestFileContent: function(file, readAsText, start, end, charset, callback)
    {
        this._requestFileContent(file.url, readAsText, start, end, charset, callback);
    },

    /**
     * @param {string} url
     * @param {boolean} readAsText
     * @param {number=} start
     * @param {number=} end
     * @param {string=} charset
     * @param {function(number, string=, string=)=} callback
     */
    _requestFileContent: function(url, readAsText, start, end, charset, callback)
    {
        /**
         * @param {?Protocol.Error} error
         * @param {number} errorCode
         * @param {string=} content
         * @param {string=} charset
         */
        function innerCallback(error, errorCode, content, charset)
        {
            if (error) {
                if (callback)
                    callback(FileError.SECURITY_ERR);
                return;
            }
            
            if (callback)
                callback(errorCode, content, charset);
        }

        FileSystemAgent.requestFileContent(url, readAsText, start, end, charset, innerCallback.bind(this));
    },
    /**
     * @param {WebInspector.FileSystemModel.Entry} entry
     * @param {function(number)=} callback
     */
    deleteEntry: function(entry, callback)
    {
        var fileSystemModel = this;
        if (entry === entry.fileSystem.root)
            this._deleteEntry(entry.url, hookFileSystemDeletion);
        else
            this._deleteEntry(entry.url, callback);

        function hookFileSystemDeletion(errorCode)
        {
            callback(errorCode);
            if (!errorCode)
                fileSystemModel._removeFileSystem(entry.fileSystem);
        }
    },

    /**
     * @param {string} url
     * @param {function(number)=} callback
     */
    _deleteEntry: function(url, callback)
    {
        /**
         * @param {?Protocol.Error} error
         * @param {number} errorCode
         */
        function innerCallback(error, errorCode)
        {
            if (error) {
                if (callback)
                    callback(FileError.SECURITY_ERR);
                return;
            }
            
            if (callback)
                callback(errorCode);
        }

        FileSystemAgent.deleteEntry(url, innerCallback.bind(this));
    },

    /**
     * @param {WebInspector.FileSystemModel.FileSystem} fileSystem
     */
    _removeFileSystem: function(fileSystem)
    {
        var origin = fileSystem.origin;
        var type = fileSystem.type;
        if (this._fileSystemsForOrigin[origin] && this._fileSystemsForOrigin[origin][type]) {
            delete this._fileSystemsForOrigin[origin][type];
            this._fileSystemRemoved(fileSystem);

            if (Object.isEmpty(this._fileSystemsForOrigin[origin]))
                delete this._fileSystemsForOrigin[origin];
        }
    },

    __proto__: WebInspector.Object.prototype
}


WebInspector.FileSystemModel.EventTypes = {
    FileSystemAdded: "FileSystemAdded",
    FileSystemRemoved: "FileSystemRemoved"
}

/**
 * @constructor
 * @param {WebInspector.FileSystemModel} fileSystemModel
 * @param {string} origin
 * @param {string} type
 * @param {FileSystemAgent.Entry} backendRootEntry
 */
WebInspector.FileSystemModel.FileSystem = function(fileSystemModel, origin, type, backendRootEntry)
{
    this.origin = origin;
    this.type = type;

    this.root = new WebInspector.FileSystemModel.Directory(fileSystemModel, this, backendRootEntry);
}

WebInspector.FileSystemModel.FileSystem.prototype = {
    /**
     * @type {string}
     */
    get name()
    {
        return "filesystem:" + this.origin + "/" + this.type;
    }
}

/**
 * @constructor
 * @param {WebInspector.FileSystemModel} fileSystemModel
 * @param {WebInspector.FileSystemModel.FileSystem} fileSystem
 * @param {FileSystemAgent.Entry} backendEntry
 */
WebInspector.FileSystemModel.Entry = function(fileSystemModel, fileSystem, backendEntry)
{
    this._fileSystemModel = fileSystemModel;
    this._fileSystem = fileSystem;

    this._url = backendEntry.url;
    this._name = backendEntry.name;
    this._isDirectory = backendEntry.isDirectory;
}

/**
 * @param {WebInspector.FileSystemModel.Entry} x
 * @param {WebInspector.FileSystemModel.Entry} y
 * @return {number}
 */
WebInspector.FileSystemModel.Entry.compare = function(x, y)
{
    if (x.isDirectory != y.isDirectory)
        return y.isDirectory ? 1 : -1;
    return x.name.compareTo(y.name);
}

WebInspector.FileSystemModel.Entry.prototype = {
    /**
     * @type {WebInspector.FileSystemModel}
     */
    get fileSystemModel()
    {
        return this._fileSystemModel;
    },

    /**
     * @type {WebInspector.FileSystemModel.FileSystem}
     */
    get fileSystem()
    {
        return this._fileSystem;
    },

    /**
     * @type {string}
     */
    get url()
    {
        return this._url;
    },

    /**
     * @type {string}
     */
    get name()
    {
        return this._name;
    },

    /**
     * @type {boolean}
     */
    get isDirectory()
    {
        return this._isDirectory;
    },

    /**
     * @param {function(number, FileSystemAgent.Metadata)} callback
     */
    requestMetadata: function(callback)
    {
        this.fileSystemModel.requestMetadata(this, callback);
    },

    /**
     * @param {function(number)} callback
     */
    deleteEntry: function(callback)
    {
        this.fileSystemModel.deleteEntry(this, callback);
    }
}

/**
 * @constructor
 * @extends {WebInspector.FileSystemModel.Entry}
 * @param {WebInspector.FileSystemModel} fileSystemModel
 * @param {WebInspector.FileSystemModel.FileSystem} fileSystem
 * @param {FileSystemAgent.Entry} backendEntry
 */
WebInspector.FileSystemModel.Directory = function(fileSystemModel, fileSystem, backendEntry)
{
    WebInspector.FileSystemModel.Entry.call(this, fileSystemModel, fileSystem, backendEntry);
}

WebInspector.FileSystemModel.Directory.prototype = {
    /**
     * @param {function(number, Array.<WebInspector.FileSystemModel.Directory>)} callback
     */
    requestDirectoryContent: function(callback)
    {
        this.fileSystemModel.requestDirectoryContent(this, callback);
    },

    __proto__: WebInspector.FileSystemModel.Entry.prototype
}

/**
 * @constructor
 * @extends {WebInspector.FileSystemModel.Entry}
 * @param {WebInspector.FileSystemModel} fileSystemModel
 * @param {WebInspector.FileSystemModel.FileSystem} fileSystem
 * @param {FileSystemAgent.Entry} backendEntry
 */
WebInspector.FileSystemModel.File = function(fileSystemModel, fileSystem, backendEntry)
{
    WebInspector.FileSystemModel.Entry.call(this, fileSystemModel, fileSystem, backendEntry);

    this._mimeType = backendEntry.mimeType;
    this._resourceType = WebInspector.resourceTypes[backendEntry.resourceType];
    this._isTextFile = backendEntry.isTextFile;
}

WebInspector.FileSystemModel.File.prototype = {
    /**
     * @type {string}
     */
    get mimeType()
    {
        return this._mimeType;
    },

    /**
     * @type {WebInspector.ResourceType}
     */
    get resourceType()
    {
        return this._resourceType;
    },

    /**
     * @type {boolean}
     */
    get isTextFile()
    {
        return this._isTextFile;
    },

    /**
     * @param {boolean} readAsText
     * @param {number=} start
     * @param {number=} end
     * @param {string=} charset
     * @param {function(number, string=)=} callback
     */
    requestFileContent: function(readAsText, start, end, charset, callback)
    {
        this.fileSystemModel.requestFileContent(this, readAsText, start, end, charset, callback);
    },

    __proto__: WebInspector.FileSystemModel.Entry.prototype
}
