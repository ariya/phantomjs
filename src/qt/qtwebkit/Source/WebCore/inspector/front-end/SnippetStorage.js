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
WebInspector.SnippetStorage = function(settingPrefix, namePrefix)
{
    this._snippets = {};

    this._lastSnippetIdentifierSetting = WebInspector.settings.createSetting(settingPrefix + "Snippets_lastIdentifier", 0);
    this._snippetsSetting = WebInspector.settings.createSetting(settingPrefix + "Snippets", []);
    this._namePrefix = namePrefix;

    this._loadSettings();
}

WebInspector.SnippetStorage.prototype = {
    get namePrefix()
    {
        return this._namePrefix;
    },

    _saveSettings: function()
    {
        var savedSnippets = [];
        for (var id in this._snippets)
            savedSnippets.push(this._snippets[id].serializeToObject());
        this._snippetsSetting.set(savedSnippets);
    },

    /**
     * @return {Array.<WebInspector.Snippet>}
     */
    snippets: function()
    {
        var result = [];
        for (var id in this._snippets)
            result.push(this._snippets[id]);
        return result;
    },

    /**
     * @param {string} id
     * @return {WebInspector.Snippet}
     */
    snippetForId: function(id)
    {
        return this._snippets[id];
    },

    _loadSettings: function()
    {
        var savedSnippets = this._snippetsSetting.get();
        for (var i = 0; i < savedSnippets.length; ++i)
            this._snippetAdded(WebInspector.Snippet.fromObject(this, savedSnippets[i]));
    },

    /**
     * @param {WebInspector.Snippet} snippet
     */
    deleteSnippet: function(snippet)
    {
        delete this._snippets[snippet.id];
        this._saveSettings();
    },

    /**
     * @return {WebInspector.Snippet}
     */
    createSnippet: function()
    {
        var nextId = this._lastSnippetIdentifierSetting.get() + 1;
        var snippetId = String(nextId);
        this._lastSnippetIdentifierSetting.set(nextId);
        var snippet = new WebInspector.Snippet(this, snippetId);
        this._snippetAdded(snippet);
        this._saveSettings();
        return snippet;
    },

    /**
     * @param {WebInspector.Snippet} snippet
     */
    _snippetAdded: function(snippet)
    {
        this._snippets[snippet.id] = snippet;
    },
    
    reset: function()
    {
        this._lastSnippetIdentifierSetting.set(0);
        this._snippetsSetting.set([]);
        this._snippets = {};
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @constructor
 * @extends {WebInspector.Object}
 * @param {WebInspector.SnippetStorage} storage
 * @param {string} id
 * @param {string=} name
 * @param {string=} content
 */
WebInspector.Snippet = function(storage, id, name, content)
{
    this._storage = storage;
    this._id = id;
    this._name = name || storage.namePrefix + id;
    this._content = content || "";
}

/**
 * @param {WebInspector.SnippetStorage} storage
 * @param {Object} serializedSnippet
 * @return {WebInspector.Snippet}
 */
WebInspector.Snippet.fromObject = function(storage, serializedSnippet)
{
    return new WebInspector.Snippet(storage, serializedSnippet.id, serializedSnippet.name, serializedSnippet.content);
}

WebInspector.Snippet.prototype = {
    /**
     * @return {string}
     */
    get id()
    {
        return this._id;
    },

    /**
     * @return {string}
     */
    get name()
    {
        return this._name;
    },

    set name(name)
    {
        if (this._name === name)
            return;

        this._name = name;
        this._storage._saveSettings();
    },

    /**
     * @return {string}
     */
    get content()
    {
        return this._content;
    },

    set content(content)
    {
        if (this._content === content)
            return;

        this._content = content;
        this._storage._saveSettings();
    },

    /**
     * @return {Object}
     */
    serializeToObject: function()
    {
        var serializedSnippet = {};
        serializedSnippet.id = this.id;
        serializedSnippet.name = this.name;
        serializedSnippet.content = this.content;
        return serializedSnippet;
    },

    __proto__: WebInspector.Object.prototype
}
