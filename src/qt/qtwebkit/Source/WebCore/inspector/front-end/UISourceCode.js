/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * @implements {WebInspector.ContentProvider}
 * @param {WebInspector.Project} project
 * @param {Array.<string>} path
 * @param {string} url
 * @param {WebInspector.ResourceType} contentType
 * @param {boolean} isEditable
 */
WebInspector.UISourceCode = function(project, path, originURL, url, contentType, isEditable)
{
    this._project = project;
    this._path = path;
    this._originURL = originURL;
    this._url = url;
    this._contentType = contentType;
    this._isEditable = isEditable;
    /**
     * @type Array.<function(?string,boolean,string)>
     */
    this._requestContentCallbacks = [];
    this._liveLocations = new Set();
    /**
     * @type {Array.<WebInspector.PresentationConsoleMessage>}
     */
    this._consoleMessages = [];
    
    /**
     * @type {Array.<WebInspector.Revision>}
     */
    this.history = [];
    if (this.isEditable() && this._url)
        this._restoreRevisionHistory();
    this._formatterMapping = new WebInspector.IdentityFormatterSourceMapping();
}

WebInspector.UISourceCode.Events = {
    FormattedChanged: "FormattedChanged",
    WorkingCopyChanged: "WorkingCopyChanged",
    WorkingCopyCommitted: "WorkingCopyCommitted",
    TitleChanged: "TitleChanged",
    ConsoleMessageAdded: "ConsoleMessageAdded",
    ConsoleMessageRemoved: "ConsoleMessageRemoved",
    ConsoleMessagesCleared: "ConsoleMessagesCleared",
    SourceMappingChanged: "SourceMappingChanged",
}

WebInspector.UISourceCode.prototype = {
    /**
     * @return {string}
     */
    get url()
    {
        return this._url;
    },

    /**
     * @return {Array.<string>}
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
        return this._path[this._path.length - 1];
    },

    /**
     * @return {string}
     */
    displayName: function()
    {
        var displayName = this.name() || (this._project.displayName() + "/" + this._path.join("/"));
        return displayName.trimEnd(100);
    },

    /**
     * @return {string}
     */
    uri: function()
    {
        if (!this._project.id())
            return this._path.join("/");
        if (!this._path.length)
            return this._project.id();
        return this._project.id() + "/" + this._path.join("/");
    },

    /**
     * @return {string}
     */
    originURL: function()
    {
        return this._originURL;
    },

    /**
     * @param {string} newName
     */
    rename: function(newName)
    {
        if (!this._path.length)
            return;
        this._path[this._path.length - 1] = newName;
        this._url = newName;
        this._originURL = newName;
        this.dispatchEventToListeners(WebInspector.UISourceCode.Events.TitleChanged, null);
    },

    /**
     * @return {string}
     */
    contentURL: function()
    {
        return this.originURL();
    },

    /**
     * @return {WebInspector.ResourceType}
     */
    contentType: function()
    {
        return this._contentType;
    },

    /**
     * @return {WebInspector.ScriptFile}
     */
    scriptFile: function()
    {
        return this._scriptFile;
    },

    /**
     * @param {WebInspector.ScriptFile} scriptFile
     */
    setScriptFile: function(scriptFile)
    {
        this._scriptFile = scriptFile;
    },

    /**
     * @return {WebInspector.StyleFile}
     */
    styleFile: function()
    {
        return this._styleFile;
    },

    /**
     * @param {WebInspector.StyleFile} styleFile
     */
    setStyleFile: function(styleFile)
    {
        this._styleFile = styleFile;
    },

    /**
     * @return {WebInspector.Project}
     */
    project: function()
    {
        return this._project;
    },

    /**
     * @param {function(?string,boolean,string)} callback
     */
    requestContent: function(callback)
    {
        if (this._content || this._contentLoaded) {
            callback(this._content, false, this._mimeType);
            return;
        }
        this._requestContentCallbacks.push(callback);
        if (this._requestContentCallbacks.length === 1)
            this._project.requestFileContent(this, this._fireContentAvailable.bind(this));
    },

    checkContentUpdated: function()
    {
        if (!this._project.canSetFileContent())
            return;
        if (this._checkingContent)
            return;
        this._checkingContent = true;
        this._project.requestFileContent(this, contentLoaded.bind(this));

        function contentLoaded(updatedContent)
        {
            if (updatedContent === null) {
                var workingCopy = this.workingCopy();
                this._commitContent("", false);
                this.setWorkingCopy(workingCopy);
                delete this._checkingContent;
                return;
            }
            if (typeof this._lastAcceptedContent === "string" && this._lastAcceptedContent === updatedContent) {
                delete this._checkingContent;
                return;
            }
            if (this._content === updatedContent) {
                delete this._lastAcceptedContent;
                delete this._checkingContent;
                return;
            }

            if (!this.isDirty()) {
                this._commitContent(updatedContent, false);
                delete this._checkingContent;
                return;
            }

            var shouldUpdate = window.confirm(WebInspector.UIString("This file was changed externally. Would you like to reload it?"));
            if (shouldUpdate)
                this._commitContent(updatedContent, false);
            else
                this._lastAcceptedContent = updatedContent;
            delete this._checkingContent;
        }
    },

    /**
     * @param {function(?string,boolean,string)} callback
     */
    requestOriginalContent: function(callback)
    {
        this._project.requestFileContent(this, callback);
    },

    /**
     * @param {string} content
     * @param {boolean} shouldSetContentInProject
     */
    _commitContent: function(content, shouldSetContentInProject)
    {
        delete this._lastAcceptedContent;
        this._content = content;
        this._contentLoaded = true;
        
        var lastRevision = this.history.length ? this.history[this.history.length - 1] : null;
        if (!lastRevision || lastRevision._content !== this._content) {
            var revision = new WebInspector.Revision(this, this._content, new Date());
            this.history.push(revision);
            revision._persist();
        }

        var oldWorkingCopy = this._workingCopy;
        delete this._workingCopy;
        this.dispatchEventToListeners(WebInspector.UISourceCode.Events.WorkingCopyCommitted, {oldWorkingCopy: oldWorkingCopy, workingCopy: this.workingCopy()});
        if (this._url && WebInspector.fileManager.isURLSaved(this._url)) {
            WebInspector.fileManager.save(this._url, this._content, false);
            WebInspector.fileManager.close(this._url);
        }
        if (shouldSetContentInProject)
            this._project.setFileContent(this, this._content, function() { });
    },

    /**
     * @param {string} content
     */
    addRevision: function(content)
    {
        this._commitContent(content, true);
    },

    _restoreRevisionHistory: function()
    {
        if (!window.localStorage)
            return;

        var registry = WebInspector.Revision._revisionHistoryRegistry();
        var historyItems = registry[this.url];
        if (!historyItems || !historyItems.length)
            return;
        for (var i = 0; i < historyItems.length; ++i) {
            var content = window.localStorage[historyItems[i].key];
            var timestamp = new Date(historyItems[i].timestamp);
            var revision = new WebInspector.Revision(this, content, timestamp);
            this.history.push(revision);
        }
        this._content = this.history[this.history.length - 1].content;
        this._contentLoaded = true;
        this._mimeType = this.canonicalMimeType();
    },

    _clearRevisionHistory: function()
    {
        if (!window.localStorage)
            return;

        var registry = WebInspector.Revision._revisionHistoryRegistry();
        var historyItems = registry[this.url];
        for (var i = 0; historyItems && i < historyItems.length; ++i)
            delete window.localStorage[historyItems[i].key];
        delete registry[this.url];
        window.localStorage["revision-history"] = JSON.stringify(registry);
    },
   
    revertToOriginal: function()
    {
        /**
         * @this {WebInspector.UISourceCode}
         * @param {?string} content
         * @param {boolean} contentEncoded
         * @param {string} mimeType
         */
        function callback(content, contentEncoded, mimeType)
        {
            if (typeof content !== "string")
                return;

            this.addRevision(content);
        }

        this.requestOriginalContent(callback.bind(this));

        WebInspector.notifications.dispatchEventToListeners(WebInspector.UserMetrics.UserAction, {
            action: WebInspector.UserMetrics.UserActionNames.ApplyOriginalContent,
            url: this.url
        });
    },

    /**
     * @param {function(WebInspector.UISourceCode)} callback
     */
    revertAndClearHistory: function(callback)
    {
        /**
         * @this {WebInspector.UISourceCode}
         * @param {?string} content
         * @param {boolean} contentEncoded
         * @param {string} mimeType
         */
        function revert(content, contentEncoded, mimeType)
        {
            if (typeof content !== "string")
                return;

            this.addRevision(content);
            this._clearRevisionHistory();
            this.history = [];
            callback(this);
        }

        this.requestOriginalContent(revert.bind(this));

        WebInspector.notifications.dispatchEventToListeners(WebInspector.UserMetrics.UserAction, {
            action: WebInspector.UserMetrics.UserActionNames.RevertRevision,
            url: this.url
        });
    },

    /**
     * @return {boolean}
     */
    isEditable: function()
    {
        return this._isEditable;
    },

    /**
     * @return {string}
     */
    workingCopy: function()
    {
        if (this.isDirty())
            return this._workingCopy;
        return this._content;
    },

    resetWorkingCopy: function()
    {
        this.setWorkingCopy(this._content);
    },

    /**
     * @param {string} newWorkingCopy
     */
    setWorkingCopy: function(newWorkingCopy)
    {
        var wasDirty = this.isDirty();        
        this._mimeType = this.canonicalMimeType();
        var oldWorkingCopy = this._workingCopy;
        if (this._content === newWorkingCopy)
            delete this._workingCopy;
        else
            this._workingCopy = newWorkingCopy;
        this.dispatchEventToListeners(WebInspector.UISourceCode.Events.WorkingCopyChanged, {oldWorkingCopy: oldWorkingCopy, workingCopy: this.workingCopy(), wasDirty: wasDirty});
    },

    /**
     * @param {function(?string)} callback
     */
    commitWorkingCopy: function(callback)
    {
        if (!this.isDirty()) {
            callback(null);
            return;
        }

        this._commitContent(this._workingCopy, true);
        callback(null);

        WebInspector.notifications.dispatchEventToListeners(WebInspector.UserMetrics.UserAction, {
            action: WebInspector.UserMetrics.UserActionNames.FileSaved,
            url: this.url
        });
    },

    /**
     * @return {boolean}
     */
    isDirty: function()
    {
        return typeof this._workingCopy !== "undefined" && this._workingCopy !== this._content;
    },

    /**
     * @return {string}
     */
    mimeType: function()
    {
        return this._mimeType;
    },

    /**
     * @return {string}
     */
    canonicalMimeType: function()
    {
        return this.contentType().canonicalMimeType() || this._mimeType;
    },

    /**
     * @return {?string}
     */
    content: function()
    {
        return this._content;
    },

    /**
     * @param {string} query
     * @param {boolean} caseSensitive
     * @param {boolean} isRegex
     * @param {function(Array.<WebInspector.ContentProvider.SearchMatch>)} callback
     */
    searchInContent: function(query, caseSensitive, isRegex, callback)
    {
        var content = this.content();
        if (content) {
            var provider = new WebInspector.StaticContentProvider(this.contentType(), content);
            provider.searchInContent(query, caseSensitive, isRegex, callback);
            return;
        }

        this._project.searchInFileContent(this, query, caseSensitive, isRegex, callback);
    },

    /**
     * @param {?string} content
     * @param {boolean} contentEncoded
     * @param {string} mimeType
     */
    _fireContentAvailable: function(content, contentEncoded, mimeType)
    {
        this._contentLoaded = true;
        this._mimeType = mimeType;
        this._content = content;

        var callbacks = this._requestContentCallbacks.slice();
        this._requestContentCallbacks = [];
        for (var i = 0; i < callbacks.length; ++i)
            callbacks[i](content, contentEncoded, mimeType);

        if (this._formatOnLoad) {
            delete this._formatOnLoad;
            this.setFormatted(true);
        }
    },

    /**
     * @return {boolean}
     */
    contentLoaded: function()
    {
        return this._contentLoaded;
    },

    /**
     * @param {number} lineNumber
     * @param {number} columnNumber
     * @return {WebInspector.RawLocation}
     */
    uiLocationToRawLocation: function(lineNumber, columnNumber)
    {
        if (!this._sourceMapping)
            return null;
        var location = this._formatterMapping.formattedToOriginal(lineNumber, columnNumber);
        return this._sourceMapping.uiLocationToRawLocation(this, location[0], location[1]);
    },

    /**
     * @param {!WebInspector.LiveLocation} liveLocation
     */
    addLiveLocation: function(liveLocation)
    {
        this._liveLocations.add(liveLocation);
    },

    /**
     * @param {!WebInspector.LiveLocation} liveLocation
     */
    removeLiveLocation: function(liveLocation)
    {
        this._liveLocations.remove(liveLocation);
    },

    updateLiveLocations: function()
    {
        var items = this._liveLocations.items();
        for (var i = 0; i < items.length; ++i)
            items[i].update();
    },

    /**
     * @param {WebInspector.UILocation} uiLocation
     */
    overrideLocation: function(uiLocation)
    {
        var location = this._formatterMapping.originalToFormatted(uiLocation.lineNumber, uiLocation.columnNumber);
        uiLocation.lineNumber = location[0];
        uiLocation.columnNumber = location[1];
        return uiLocation;
    },

    /**
     * @return {Array.<WebInspector.PresentationConsoleMessage>}
     */
    consoleMessages: function()
    {
        return this._consoleMessages;
    },

    /**
     * @param {WebInspector.PresentationConsoleMessage} message
     */
    consoleMessageAdded: function(message)
    {
        this._consoleMessages.push(message);
        this.dispatchEventToListeners(WebInspector.UISourceCode.Events.ConsoleMessageAdded, message);
    },

    /**
     * @param {WebInspector.PresentationConsoleMessage} message
     */
    consoleMessageRemoved: function(message)
    {
        this._consoleMessages.remove(message);
        this.dispatchEventToListeners(WebInspector.UISourceCode.Events.ConsoleMessageRemoved, message);
    },

    consoleMessagesCleared: function()
    {
        this._consoleMessages = [];
        this.dispatchEventToListeners(WebInspector.UISourceCode.Events.ConsoleMessagesCleared);
    },

    /**
     * @return {boolean}
     */
    formatted: function()
    {
        return !!this._formatted;
    },

    /**
     * @param {boolean} formatted
     */
    setFormatted: function(formatted)
    {
        if (!this.contentLoaded()) {
            this._formatOnLoad = formatted;
            return;
        }

        if (this._formatted === formatted)
            return;

        this._formatted = formatted;

        // Re-request content
        this._contentLoaded = false;
        this._content = false;
        WebInspector.UISourceCode.prototype.requestContent.call(this, didGetContent.bind(this));
  
        /**
         * @this {WebInspector.UISourceCode}
         * @param {?string} content
         * @param {boolean} contentEncoded
         * @param {string} mimeType
         */
        function didGetContent(content, contentEncoded, mimeType)
        {
            var formatter;
            if (!formatted)
                formatter = new WebInspector.IdentityFormatter();
            else
                formatter = WebInspector.Formatter.createFormatter(this.contentType());
            formatter.formatContent(mimeType, content || "", formattedChanged.bind(this));
  
            /**
             * @this {WebInspector.UISourceCode}
             * @param {string} content
             * @param {WebInspector.FormatterSourceMapping} formatterMapping
             */
            function formattedChanged(content, formatterMapping)
            {
                this._content = content;
                delete this._workingCopy;
                this._formatterMapping = formatterMapping;
                this.dispatchEventToListeners(WebInspector.UISourceCode.Events.FormattedChanged, {content: content});
                this.updateLiveLocations();
            }
        }
    },

    /**
     * @return {WebInspector.Formatter} formatter
     */
    createFormatter: function()
    {
        // overridden by subclasses.
        return null;
    },

    /**
     * @param {WebInspector.SourceMapping} sourceMapping
     */
    setSourceMapping: function(sourceMapping)
    {
        var wasIdentity = this._sourceMapping ? this._sourceMapping.isIdentity() : true;
        this._sourceMapping = sourceMapping;
        var data = {}
        data.isIdentity = sourceMapping ? sourceMapping.isIdentity() : true;
        data.identityHasChanged = data.isIdentity !== wasIdentity;
        this.dispatchEventToListeners(WebInspector.UISourceCode.Events.SourceMappingChanged, data);
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @interface
 * @extends {WebInspector.EventTarget}
 */
WebInspector.UISourceCodeProvider = function()
{
}

WebInspector.UISourceCodeProvider.Events = {
    UISourceCodeAdded: "UISourceCodeAdded",
    UISourceCodeRemoved: "UISourceCodeRemoved"
}

WebInspector.UISourceCodeProvider.prototype = {
    /**
     * @return {Array.<WebInspector.UISourceCode>}
     */
    uiSourceCodes: function() {},
}

/**
 * @constructor
 * @param {WebInspector.UISourceCode} uiSourceCode
 * @param {number} lineNumber
 * @param {number} columnNumber
 */
WebInspector.UILocation = function(uiSourceCode, lineNumber, columnNumber)
{
    this.uiSourceCode = uiSourceCode;
    this.lineNumber = lineNumber;
    this.columnNumber = columnNumber;
}

WebInspector.UILocation.prototype = {
    /**
     * @return {WebInspector.RawLocation}
     */
    uiLocationToRawLocation: function()
    {
        return this.uiSourceCode.uiLocationToRawLocation(this.lineNumber, this.columnNumber);
    },

    /**
     * @return {?string}
     */
    url: function()
    {
        return this.uiSourceCode.contentURL();
    }
}

/**
 * @interface
 */
WebInspector.RawLocation = function()
{
}

/**
 * @constructor
 * @param {WebInspector.RawLocation} rawLocation
 * @param {function(WebInspector.UILocation):(boolean|undefined)} updateDelegate
 */
WebInspector.LiveLocation = function(rawLocation, updateDelegate)
{
    this._rawLocation = rawLocation;
    this._updateDelegate = updateDelegate;
    this._uiSourceCodes = [];
}

WebInspector.LiveLocation.prototype = {
    update: function()
    {
        var uiLocation = this.uiLocation();
        if (uiLocation) {
            var uiSourceCode = uiLocation.uiSourceCode;
            if (this._uiSourceCodes.indexOf(uiSourceCode) === -1) {
                uiSourceCode.addLiveLocation(this);
                this._uiSourceCodes.push(uiSourceCode);
            }
            var oneTime = this._updateDelegate(uiLocation);
            if (oneTime)
                this.dispose();
        }
    },

    /**
     * @return {WebInspector.RawLocation}
     */
    rawLocation: function()
    {
        return this._rawLocation;
    },

    /**
     * @return {WebInspector.UILocation}
     */
    uiLocation: function()
    {
        // Should be overridden by subclasses.
    },

    dispose: function()
    {
        for (var i = 0; i < this._uiSourceCodes.length; ++i)
            this._uiSourceCodes[i].removeLiveLocation(this);
        this._uiSourceCodes = [];
    }
}

/**
 * @constructor
 * @implements {WebInspector.ContentProvider}
 * @param {WebInspector.UISourceCode} uiSourceCode
 * @param {?string|undefined} content
 * @param {Date} timestamp
 */
WebInspector.Revision = function(uiSourceCode, content, timestamp)
{
    this._uiSourceCode = uiSourceCode;
    this._content = content;
    this._timestamp = timestamp;
}

WebInspector.Revision._revisionHistoryRegistry = function()
{
    if (!WebInspector.Revision._revisionHistoryRegistryObject) {
        if (window.localStorage) {
            var revisionHistory = window.localStorage["revision-history"];
            try {
                WebInspector.Revision._revisionHistoryRegistryObject = revisionHistory ? JSON.parse(revisionHistory) : {};
            } catch (e) {
                WebInspector.Revision._revisionHistoryRegistryObject = {};
            }
        } else
            WebInspector.Revision._revisionHistoryRegistryObject = {};
    }
    return WebInspector.Revision._revisionHistoryRegistryObject;
}

WebInspector.Revision.filterOutStaleRevisions = function()
{
    if (!window.localStorage)
        return;

    var registry = WebInspector.Revision._revisionHistoryRegistry();
    var filteredRegistry = {};
    for (var url in registry) {
        var historyItems = registry[url];
        var filteredHistoryItems = [];
        for (var i = 0; historyItems && i < historyItems.length; ++i) {
            var historyItem = historyItems[i];
            if (historyItem.loaderId === WebInspector.resourceTreeModel.mainFrame.loaderId) {
                filteredHistoryItems.push(historyItem);
                filteredRegistry[url] = filteredHistoryItems;
            } else
                delete window.localStorage[historyItem.key];
        }
    }
    WebInspector.Revision._revisionHistoryRegistryObject = filteredRegistry;

    function persist()
    {
        window.localStorage["revision-history"] = JSON.stringify(filteredRegistry);
    }

    // Schedule async storage.
    setTimeout(persist, 0);
}

WebInspector.Revision.prototype = {
    /**
     * @return {WebInspector.UISourceCode}
     */
    get uiSourceCode()
    {
        return this._uiSourceCode;
    },

    /**
     * @return {Date}
     */
    get timestamp()
    {
        return this._timestamp;
    },

    /**
     * @return {?string}
     */
    get content()
    {
        return this._content || null;
    },

    revertToThis: function()
    {
        function revert(content)
        {
            if (this._uiSourceCode._content !== content)
                this._uiSourceCode.addRevision(content);
        }
        this.requestContent(revert.bind(this));
    },

    /**
     * @return {string}
     */
    contentURL: function()
    {
        return this._uiSourceCode.originURL();
    },

    /**
     * @return {WebInspector.ResourceType}
     */
    contentType: function()
    {
        return this._uiSourceCode.contentType();
    },

    /**
     * @param {function(?string, boolean, string)} callback
     */
    requestContent: function(callback)
    {
        callback(this._content || "", false, this.uiSourceCode.canonicalMimeType());
    },

    /**
     * @param {string} query
     * @param {boolean} caseSensitive
     * @param {boolean} isRegex
     * @param {function(Array.<WebInspector.ContentProvider.SearchMatch>)} callback
     */
    searchInContent: function(query, caseSensitive, isRegex, callback)
    {
        callback([]);
    },

    _persist: function()
    {
        if (!window.localStorage)
            return;

        var url = this.contentURL();
        if (!url || url.startsWith("inspector://"))
            return;

        var loaderId = WebInspector.resourceTreeModel.mainFrame.loaderId;
        var timestamp = this.timestamp.getTime();
        var key = "revision-history|" + url + "|" + loaderId + "|" + timestamp;

        var registry = WebInspector.Revision._revisionHistoryRegistry();

        var historyItems = registry[url];
        if (!historyItems) {
            historyItems = [];
            registry[url] = historyItems;
        }
        historyItems.push({url: url, loaderId: loaderId, timestamp: timestamp, key: key});

        function persist()
        {
            window.localStorage[key] = this._content;
            window.localStorage["revision-history"] = JSON.stringify(registry);
        }

        // Schedule async storage.
        setTimeout(persist.bind(this), 0);
    }
}
