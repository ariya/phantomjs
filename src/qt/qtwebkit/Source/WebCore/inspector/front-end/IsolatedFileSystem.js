/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
 * @param {WebInspector.IsolatedFileSystemManager} manager
 * @param {string} path
 */
WebInspector.IsolatedFileSystem = function(manager, path, name, rootURL)
{
    this._manager = manager;
    this._path = path;
    this._name = name;
    this._rootURL = rootURL;
}

WebInspector.IsolatedFileSystem.errorMessage = function(error)
{
    var msg;
    switch (error.code) {
    case FileError.QUOTA_EXCEEDED_ERR:
        msg = "QUOTA_EXCEEDED_ERR";
        break;
    case FileError.NOT_FOUND_ERR:
        msg = "NOT_FOUND_ERR";
        break;
    case FileError.SECURITY_ERR:
        msg = "SECURITY_ERR";
        break;
    case FileError.INVALID_MODIFICATION_ERR:
        msg = "INVALID_MODIFICATION_ERR";
        break;
    case FileError.INVALID_STATE_ERR:
        msg = "INVALID_STATE_ERR";
        break;
    default:
        msg = "Unknown Error";
        break;
    };

    return "File system error: " + msg;
}

WebInspector.IsolatedFileSystem.prototype = {
    /**
     * @return {string}
     */
    path: function()
    {
        return this._path;
    },

    /**
     * @return {string}
     */
    name: function()
    {
        return this._name;
    },

    /**
     * @return {string}
     */
    rootURL: function()
    {
        return this._rootURL;
    },

    /**
     * @param {function(DOMFileSystem)} callback
     */
    _requestFileSystem: function(callback)
    {
        this._manager.requestDOMFileSystem(this._path, callback);
    },

    /**
     * @param {string} path
     * @param {function(string)} callback
     */
    requestFilesRecursive: function(path, callback)
    {
        this._requestFileSystem(fileSystemLoaded.bind(this));

        var domFileSystem;
        /**
         * @param {DOMFileSystem} fs
         */
        function fileSystemLoaded(fs)
        {
            domFileSystem = fs;
            this._requestEntries(domFileSystem, path, innerCallback.bind(this));
        }

        /**
         * @param {Array.<FileEntry>} entries
         */
        function innerCallback(entries)
        {
            for (var i = 0; i < entries.length; ++i) {
                var entry = entries[i];
                if (!entry.isDirectory)
                    callback(entry.fullPath);
                else
                    this._requestEntries(domFileSystem, entry.fullPath, innerCallback.bind(this));
            }
        }
    },

    /**
     * @param {string} path
     * @param {function(?string)} callback
     */
    requestFileContent: function(path, callback)
    {
        this._requestFileSystem(fileSystemLoaded.bind(this));

        /**
         * @param {DOMFileSystem} domFileSystem
         */
        function fileSystemLoaded(domFileSystem)
        {
            domFileSystem.root.getFile(path, null, fileEntryLoaded, errorHandler);
        }

        /**
         * @param {FileEntry} entry
         */
        function fileEntryLoaded(entry)
        {
            entry.file(fileLoaded, errorHandler);
        }

        /**
         * @param {!Blob} file
         */
        function fileLoaded(file)
        {
            var reader = new FileReader();
            reader.onloadend = readerLoadEnd;
            reader.readAsText(file);
        }

        /**
         * @this {FileReader}
         */
        function readerLoadEnd()
        {
            callback(/** @type {string} */ (this.result));
        }

        function errorHandler(error)
        {
            if (error.code === FileError.NOT_FOUND_ERR) {
                callback(null);
                return;
            }

            var errorMessage = WebInspector.IsolatedFileSystem.errorMessage(error);
            console.error(errorMessage + " when getting content for file '" + (this._path + "/" + path) + "'");
            callback(null);
        }
    },

    /**
     * @param {string} path
     * @param {string} content
     * @param {function()} callback
     */
    setFileContent: function(path, content, callback)
    {
        this._requestFileSystem(fileSystemLoaded);

        /**
         * @param {DOMFileSystem} domFileSystem
         */
        function fileSystemLoaded(domFileSystem)
        {
            domFileSystem.root.getFile(path, { create: true }, fileEntryLoaded, errorHandler);
        }

        /**
         * @param {FileEntry} entry
         */
        function fileEntryLoaded(entry)
        {
            entry.createWriter(fileWriterCreated, errorHandler);
        }

        /**
         * @param {FileWriter} fileWriter
         */
        function fileWriterCreated(fileWriter)
        {
            fileWriter.onerror = errorHandler;
            fileWriter.onwriteend = fileTruncated;
            fileWriter.truncate(0);

            function fileTruncated()
            {
                fileWriter.onwriteend = writerEnd;
                var blob = new Blob([content], { type: "text/plain" });
                fileWriter.write(blob);
            }
        }

        function writerEnd()
        {
            callback();
        }

        function errorHandler(error)
        {
            var errorMessage = WebInspector.IsolatedFileSystem.errorMessage(error);
            console.error(errorMessage + " when setting content for file '" + (this._path + "/" + path) + "'");
            callback();
        }
    },

    /**
     * @param {DirectoryEntry} dirEntry
     * @param {function(Array.<FileEntry>)} callback
     */
    _readDirectory: function(dirEntry, callback)
    {
        var dirReader = dirEntry.createReader();
        var entries = [];

        function innerCallback(results)
        {
            if (!results.length)
                callback(entries.sort());
            else {
                entries = entries.concat(toArray(results));
                dirReader.readEntries(innerCallback, errorHandler);
            }
        }

        function toArray(list)
        {
            return Array.prototype.slice.call(list || [], 0);
        }    

        dirReader.readEntries(innerCallback, errorHandler);

        function errorHandler(error)
        {
            var errorMessage = WebInspector.IsolatedFileSystem.errorMessage(error);
            console.error(errorMessage + " when reading directory '" + dirEntry.fullPath + "'");
            callback([]);
        }
    },

    /**
     * @param {DOMFileSystem} domFileSystem
     * @param {string} path
     * @param {function(Array.<FileEntry>)} callback
     */
    _requestEntries: function(domFileSystem, path, callback)
    {
        domFileSystem.root.getDirectory(path, null, innerCallback.bind(this), errorHandler);

        function innerCallback(dirEntry)
        {
            this._readDirectory(dirEntry, callback)
        }

        function errorHandler(error)
        {
            var errorMessage = WebInspector.IsolatedFileSystem.errorMessage(error);
            console.error(errorMessage + " when requesting entry '" + path + "'");
            callback([]);
        }
    }
}
