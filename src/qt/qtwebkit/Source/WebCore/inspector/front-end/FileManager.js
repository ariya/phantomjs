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
WebInspector.FileManager = function()
{
}

WebInspector.FileManager.EventTypes = {
    SavedURL: "SavedURL",
    AppendedToURL: "AppendedToURL"
}

WebInspector.FileManager.prototype = {
    /**
     * @return {boolean}
     */
    canSave: function()
    {
        return InspectorFrontendHost.canSave();
    },

    /**
     * @param {string} url
     * @param {string} content
     * @param {boolean} forceSaveAs
     */
    save: function(url, content, forceSaveAs)
    {
        // Remove this url from the saved URLs while it is being saved.
        var savedURLs = WebInspector.settings.savedURLs.get();
        delete savedURLs[url];
        WebInspector.settings.savedURLs.set(savedURLs);
        InspectorFrontendHost.save(url, content, forceSaveAs);
    },

    /**
     * @param {string} url
     */
    savedURL: function(url)
    {
        var savedURLs = WebInspector.settings.savedURLs.get();
        savedURLs[url] = true;
        WebInspector.settings.savedURLs.set(savedURLs);
        this.dispatchEventToListeners(WebInspector.FileManager.EventTypes.SavedURL, url);
    },

    /**
     * @param {string} url
     * @return {boolean}
     */
    isURLSaved: function(url)
    {
        var savedURLs = WebInspector.settings.savedURLs.get();
        return savedURLs[url];
    },

    /**
     * @param {string} url
     * @param {string} content
     */
    append: function(url, content)
    {
        InspectorFrontendHost.append(url, content);
    },

    /**
     * @param {string} url
     */
    close: function(url)
    {
        InspectorFrontendHost.close(url);
    },

    /**
     * @param {string} url
     */
    appendedToURL: function(url)
    {
        this.dispatchEventToListeners(WebInspector.FileManager.EventTypes.AppendedToURL, url);
    },

    __proto__: WebInspector.Object.prototype
}

WebInspector.fileManager = new WebInspector.FileManager();
