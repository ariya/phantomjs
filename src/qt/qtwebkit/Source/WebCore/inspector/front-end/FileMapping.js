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
 */
WebInspector.FileMapping = function()
{
    this._mappingEntriesSetting = WebInspector.settings.createSetting("fileMappingEntries", []);
    /** @type {Array.<WebInspector.FileMapping.Entry>} */
    this._entries = [];
    this._loadFromSettings();
}

WebInspector.FileMapping.prototype = {
    /**
     * @param {string} url
     * @return {?WebInspector.FileMapping.Entry}
     */
    mappingEntryForURL: function(url)
    {
        for (var i = 0; i < this._entries.length; ++i) {
            var entry = this._entries[i];
            if (url.startsWith(entry.urlPrefix))
                return entry;
        }
        return null;
    },

    /**
     * @param {string} path
     * @return {?WebInspector.FileMapping.Entry}
     */
    mappingEntryForPath: function(path)
    {
        for (var i = 0; i < this._entries.length; ++i) {
            var entry = this._entries[i];
            if (path.startsWith(entry.pathPrefix))
                return entry;
        }
        return null;
    },

    /**
     * @return {Array.<WebInspector.FileMapping.Entry>}
     */
    mappingEntries: function()
    {
        return this._entries.slice();
    },

    /**
     * @param {Array.<WebInspector.FileMapping.Entry>} mappingEntries
     */
    setMappingEntries: function(mappingEntries)
    {
        this._entries = mappingEntries;
        this._mappingEntriesSetting.set(mappingEntries);
    },

    _loadFromSettings: function()
    {
        var savedEntries = this._mappingEntriesSetting.get();
        this._entries = [];
        for (var i = 0; i < savedEntries.length; ++i) {
            var entry = new WebInspector.FileMapping.Entry(savedEntries[i].urlPrefix, savedEntries[i].pathPrefix);
            this._entries.push(entry);
        }
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 * @param {string} urlPrefix
 * @param {string} pathPrefix
 */
WebInspector.FileMapping.Entry = function(urlPrefix, pathPrefix)
{
    this.urlPrefix = urlPrefix;
    this.pathPrefix = pathPrefix;
}

/**
 * @type {?WebInspector.FileMapping}
 */
WebInspector.fileMapping = null;
