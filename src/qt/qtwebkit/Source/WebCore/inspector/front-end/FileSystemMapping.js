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
 * @interface
 */
WebInspector.FileSystemMapping = function() { }

WebInspector.FileSystemMapping.prototype = {
    /**
     * @return {Array.<string>}
     */
    fileSystemPaths: function() { },

    /**
     * @param {string} prefix
     * @return {?string}
     */
    fileSystemPathForPrefix: function(prefix) { }
}

/**
 * @constructor
 * @implements {WebInspector.FileSystemMapping}
 * @extends {WebInspector.Object}
 */
WebInspector.FileSystemMappingImpl = function()
{
    WebInspector.Object.call(this);
    this._fileSystemMappingSetting = WebInspector.settings.createSetting("fileSystemMapping", {});
    /** @type {!Object.<string, boolean>} */
    this._fileSystemPaths = {};
    this._loadFromSettings();
}

WebInspector.FileSystemMappingImpl.prototype = {
    _loadFromSettings: function()
    {
        var savedMapping = this._fileSystemMappingSetting.get();
        this._fileSystemPaths = savedMapping ? /** @type {!Object.<string, string>} */ (savedMapping.registeredFileSystemPaths) || {} : {};
    },

    _saveToSettings: function()
    {
        var savedMapping = {};
        savedMapping.registeredFileSystemPaths = this._fileSystemPaths;
        this._fileSystemMappingSetting.set(savedMapping);
    },


    /**
     * @param {string} fileSystemPath
     */
    addFileSystemMapping: function(fileSystemPath)
    {
        if (this._fileSystemPaths[fileSystemPath])
            return;

        this._fileSystemPaths[fileSystemPath] = true;
        this._saveToSettings();
        delete this._cachedFileSystemPaths;
    },

    /**
     * @param {string} fileSystemPath
     */
    removeFileSystemMapping: function(fileSystemPath)
    {
        if (!this._fileSystemPaths[fileSystemPath])
            return;
        delete this._fileSystemPaths[fileSystemPath];
        this._saveToSettings();
        delete this._cachedFileSystemPaths;
    },

    /**
     * @return {Array.<string>}
     */
    fileSystemPaths: function()
    {
        return Object.keys(this._fileSystemPaths);
    },

    /**
     * @param {string} prefix
     * @return {?string}
     */
    fileSystemPathForPrefix: function(prefix)
    {
        this._cachedFileSystemPaths = this._cachedFileSystemPaths || {};
        if (this._cachedFileSystemPaths.hasOwnProperty(prefix))
            return this._cachedFileSystemPaths[prefix];
        var result = null;
        for (var fileSystemPath in this._fileSystemPaths) {
            if (prefix.startsWith(fileSystemPath + "/")) {
                result = fileSystemPath;
                break;
            }
        }
        this._cachedFileSystemPaths[prefix] = result;
        return result;
    },

    __proto__: WebInspector.Object.prototype
}
