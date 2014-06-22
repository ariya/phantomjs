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
WebInspector.WorkspaceController = function(workspace)
{
    this._workspace = workspace;
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.InspectedURLChanged, this._inspectedURLChanged, this);
}

WebInspector.WorkspaceController.prototype = {
    /**
     * @param {WebInspector.Event} event
     */
    _inspectedURLChanged: function(event)
    {
        WebInspector.Revision.filterOutStaleRevisions();
    }
}

/**
 * @constructor
 * @param {Array.<string>} path
 * @param {string} originURL
 * @param {string} url
 * @param {WebInspector.ResourceType} contentType
 * @param {boolean} isEditable
 * @param {boolean=} isContentScript
 */
WebInspector.FileDescriptor = function(path, originURL, url, contentType, isEditable, isContentScript)
{
    this.path = path;
    this.originURL = originURL;
    this.url = url;
    this.contentType = contentType;
    this.isEditable = isEditable;
    this.isContentScript = isContentScript || false;
}

/**
 * @interface
 * @extends {WebInspector.EventTarget}
 */
WebInspector.ProjectDelegate = function() { }

WebInspector.ProjectDelegate.Events = {
    FileAdded: "FileAdded",
    FileRemoved: "FileRemoved",
    Reset: "Reset",
}

WebInspector.ProjectDelegate.prototype = {
    /**
     * @return {string}
     */
    id: function() { },

    /**
     * @return {string}
     */
    type: function() { },

    /**
     * @return {string}
     */
    displayName: function() { }, 

    /**
     * @param {Array.<string>} path
     * @param {function(?string,boolean,string)} callback
     */
    requestFileContent: function(path, callback) { },

    /**
     * @return {boolean}
     */
    canSetFileContent: function() { },

    /**
     * @param {Array.<string>} path
     * @param {string} newContent
     * @param {function(?string)} callback
     */
    setFileContent: function(path, newContent, callback) { },

    /**
     * @param {Array.<string>} path
     * @param {string} query
     * @param {boolean} caseSensitive
     * @param {boolean} isRegex
     * @param {function(Array.<WebInspector.ContentProvider.SearchMatch>)} callback
     */
    searchInFileContent: function(path, query, caseSensitive, isRegex, callback) { }
}

/**
 * @type {?WebInspector.WorkspaceController}
 */
WebInspector.workspaceController = null;

/**
 * @param {WebInspector.Workspace} workspace
 * @param {WebInspector.ProjectDelegate} projectDelegate
 * @constructor
 */
WebInspector.Project = function(workspace, projectDelegate)
{
    /** @type {Object.<string, WebInspector.UISourceCode>} */
    this._uiSourceCodes = {};
    this._workspace = workspace;
    this._projectDelegate = projectDelegate;
    this._projectDelegate.addEventListener(WebInspector.ProjectDelegate.Events.FileAdded, this._fileAdded, this);
    this._projectDelegate.addEventListener(WebInspector.ProjectDelegate.Events.FileRemoved, this._fileRemoved, this);
    this._projectDelegate.addEventListener(WebInspector.ProjectDelegate.Events.Reset, this._reset, this);
}

WebInspector.Project.prototype = {
    /**
     * @return {string}
     */
    id: function()
    {
        return this._projectDelegate.id();
    },

    /**
     * @return {string}
     */
    type: function()
    {
        return this._projectDelegate.type(); 
    },

    /**
     * @return {string}
     */
    displayName: function() 
    {
        return this._projectDelegate.displayName(); 
    },

    /**
     * @return {boolean}
     */
    isServiceProject: function()
    {
        return this._projectDelegate.type() === WebInspector.projectTypes.Debugger || this._projectDelegate.type() === WebInspector.projectTypes.LiveEdit;
    },

    _fileAdded: function(event)
    {
        var fileDescriptor = /** @type {WebInspector.FileDescriptor} */ (event.data);
        var uiSourceCode = this.uiSourceCode(fileDescriptor.path);
        if (uiSourceCode) {
            // FIXME: Implement
            return;
        }

        uiSourceCode = new WebInspector.UISourceCode(this, fileDescriptor.path, fileDescriptor.originURL, fileDescriptor.url, fileDescriptor.contentType, fileDescriptor.isEditable); 
        uiSourceCode.isContentScript = fileDescriptor.isContentScript;
        this._uiSourceCodes[uiSourceCode.path().join("/")] = uiSourceCode;
        this._workspace.dispatchEventToListeners(WebInspector.UISourceCodeProvider.Events.UISourceCodeAdded, uiSourceCode);
    },

    _fileRemoved: function(event)
    {
        var path = /** @type {Array.<string>} */ (event.data);
        var uiSourceCode = this.uiSourceCode(path);
        if (!uiSourceCode)
            return;
        delete this._uiSourceCodes[uiSourceCode.path().join("/")];
        this._workspace.dispatchEventToListeners(WebInspector.UISourceCodeProvider.Events.UISourceCodeRemoved, uiSourceCode);
    },

    _reset: function()
    {
        this._workspace.dispatchEventToListeners(WebInspector.Workspace.Events.ProjectWillReset, this);
        this._uiSourceCodes = {};
    },

    /**
     * @param {Array.<string>} path
     * @return {?WebInspector.UISourceCode}
     */
    uiSourceCode: function(path)
    {
        return this._uiSourceCodes[path.join("/")] || null;
    },

    /**
     * @param {string} originURL
     * @return {?WebInspector.UISourceCode}
     */
    uiSourceCodeForOriginURL: function(originURL)
    {
        for (var path in this._uiSourceCodes) {
            var uiSourceCode = this._uiSourceCodes[path];
            if (uiSourceCode.originURL() === originURL)
                return uiSourceCode;
        }
        return null;
    },

    /**
     * @return {Array.<WebInspector.UISourceCode>}
     */
    uiSourceCodes: function()
    {
        return Object.values(this._uiSourceCodes);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {function(?string,boolean,string)} callback
     */
    requestFileContent: function(uiSourceCode, callback)
    {
        this._projectDelegate.requestFileContent(uiSourceCode.path(), callback);
    },

    /**
     * @return {boolean}
     */
    canSetFileContent: function()
    {
        return this._projectDelegate.canSetFileContent();
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {string} newContent
     * @param {function(?string)} callback
     */
    setFileContent: function(uiSourceCode, newContent, callback)
    {
        this._projectDelegate.setFileContent(uiSourceCode.path(), newContent, callback);
        this._workspace.dispatchEventToListeners(WebInspector.Workspace.Events.UISourceCodeContentCommitted, { uiSourceCode: uiSourceCode, content: newContent });
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {string} query
     * @param {boolean} caseSensitive
     * @param {boolean} isRegex
     * @param {function(Array.<WebInspector.ContentProvider.SearchMatch>)} callback
     */
    searchInFileContent: function(uiSourceCode, query, caseSensitive, isRegex, callback)
    {
        this._projectDelegate.searchInFileContent(uiSourceCode.path(), query, caseSensitive, isRegex, callback);
    },

    dispose: function()
    {
        this._projectDelegate.reset();
    }
}

WebInspector.projectTypes = {
    Debugger: "debugger",
    LiveEdit: "liveedit",
    Network: "network",
    Snippets: "snippets",
    FileSystem: "filesystem"
}

/**
 * @constructor
 * @implements {WebInspector.UISourceCodeProvider}
 * @extends {WebInspector.Object}
 * @param {WebInspector.FileMapping} fileMapping
 * @param {WebInspector.FileSystemMapping} fileSystemMapping
 */
WebInspector.Workspace = function(fileMapping, fileSystemMapping)
{
    this._fileMapping = fileMapping;
    this._fileSystemMapping = fileSystemMapping;
    /** @type {!Object.<string, WebInspector.Project>} */
    this._projects = {};
}

WebInspector.Workspace.Events = {
    UISourceCodeContentCommitted: "UISourceCodeContentCommitted",
    ProjectWillReset: "ProjectWillReset"
}

WebInspector.Workspace.prototype = {
    /**
     * @param {string} projectId
     * @param {Array.<string>} path
     * @return {?WebInspector.UISourceCode}
     */
    uiSourceCode: function(projectId, path)
    {
        var project = this._projects[projectId];
        return project ? project.uiSourceCode(path) : null;
    },

    /**
     * @param {string} originURL
     * @return {?WebInspector.UISourceCode}
     */
    uiSourceCodeForOriginURL: function(originURL)
    {
        var networkProjects = this.projectsForType(WebInspector.projectTypes.Network)
        for (var i = 0; i < networkProjects.length; ++i) {
            var project = networkProjects[i];
            var uiSourceCode = project.uiSourceCodeForOriginURL(originURL);
            if (uiSourceCode)
                return uiSourceCode;
        }
        return null;
    },

    /**
     * @param {string} type
     * @return {Array.<WebInspector.UISourceCode>}
     */
    uiSourceCodesForProjectType: function(type)
    {
        var result = [];
        for (var projectName in this._projects) {
            var project = this._projects[projectName];
            if (project.type() === type)
                result = result.concat(project.uiSourceCodes());
        }
        return result;
    },

    /**
     * @param {WebInspector.ProjectDelegate} projectDelegate
     * @return {WebInspector.Project}
     */
    addProject: function(projectDelegate)
    {
        var projectId = projectDelegate.id();
        this._projects[projectId] = new WebInspector.Project(this, projectDelegate);
        return this._projects[projectId];
    },

    /**
     * @param {string} projectId
     */
    removeProject: function(projectId)
    {
        var project = this._projects[projectId];
        if (!project)
            return;
        project.dispose();
        delete this._projects[projectId];
    },

    /**
     * @param {string} projectId
     * @return {WebInspector.Project}
     */
    project: function(projectId)
    {
        return this._projects[projectId];
    },

    /**
     * @return {Array.<WebInspector.Project>}
     */
    projects: function()
    {
        return Object.values(this._projects);
    },

    /**
     * @param {string} type
     * @return {Array.<WebInspector.Project>}
     */
    projectsForType: function(type)
    {
        function filterByType(project)
        {
            return project.type() === type;
        }
        return this.projects().filter(filterByType);
    },

    /**
     * @return {Array.<WebInspector.UISourceCode>}
     */
    uiSourceCodes: function()
    {
        var result = [];
        for (var projectId in this._projects) {
            var project = this._projects[projectId];
            result = result.concat(project.uiSourceCodes());
        }
        return result;
    },

    /**
     * @param {string} url
     * @return {boolean}
     */
    hasMappingForURL: function(url)
    {
        var entry = this._fileMapping.mappingEntryForURL(url);
        if (!entry)
            return false;
        return !!this._fileSystemPathForEntry(entry);
    },
    
    /**
     * @param {WebInspector.FileMapping.Entry} entry
     * @return {?string}
     */
    _fileSystemPathForEntry: function(entry)
    {
        return this._fileSystemMapping.fileSystemPathForPrefix(entry.pathPrefix);
    },
    /**
     * @param {string} url
     * @return {WebInspector.UISourceCode}
     */
    uiSourceCodeForURL: function(url)
    {
        var entry = this._fileMapping.mappingEntryForURL(url);
        var fileSystemPath = entry ? this._fileSystemPathForEntry(entry) : null;
        if (!fileSystemPath) {
            var splittedURL = WebInspector.ParsedURL.splitURL(url);
            var projectId = WebInspector.SimpleProjectDelegate.projectId(splittedURL[0], WebInspector.projectTypes.Network);
            var path = WebInspector.SimpleWorkspaceProvider.pathForSplittedURL(splittedURL);
            var project = this.project(projectId);
            return project ? project.uiSourceCode(path) : null;
        }

        var projectId = WebInspector.FileSystemProjectDelegate.projectId(fileSystemPath);
        var pathPrefix = entry.pathPrefix.substr(fileSystemPath.length + 1);
        var path = pathPrefix + url.substr(entry.urlPrefix.length);
        var project = this.project(projectId);
        return project ? project.uiSourceCode(path.split("/")) : null;
    },

    /**
     * @param {string} path
     * @return {string}
     */
    urlForPath: function(path)
    {
        var entry = this._fileMapping.mappingEntryForPath(path);
        if (!entry)
            return "";
        return entry.urlPrefix + path.substring(entry.pathPrefix.length);
    },

    /**
     * @param {WebInspector.UISourceCode} networkUISourceCode
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {WebInspector.FileSystemWorkspaceProvider} fileSystemWorkspaceProvider
     */
    addMapping: function(networkUISourceCode, uiSourceCode, fileSystemWorkspaceProvider)
    {
        var url = networkUISourceCode.url;
        var path = uiSourceCode.path();
        var suffix = "";
        for (var i = path.length - 1; i >= 0; --i) {
            var nextSuffix = "/" + path[i] + suffix;
            if (!url.endsWith(nextSuffix))
                break;
            suffix = nextSuffix;
        }
        var fileSystemPath = fileSystemWorkspaceProvider.fileSystemPath(uiSourceCode);
        var filePath = "/" + path.join("/");
        var pathPrefix = fileSystemPath + filePath.substr(0, filePath.length - suffix.length) + "/";
        var urlPrefix = url.substr(0, url.length - suffix.length) + "/";

        var entries = this._fileMapping.mappingEntries();
        var entry = new WebInspector.FileMapping.Entry(urlPrefix, pathPrefix);
        entries.push(entry);
        this._fileMapping.setMappingEntries(entries);
        WebInspector.suggestReload();
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    removeMapping: function(uiSourceCode)
    {
        var entry = this._fileMapping.mappingEntryForURL(uiSourceCode.url);
        var entries = this._fileMapping.mappingEntries();
        entries.remove(entry);
        this._fileMapping.setMappingEntries(entries);
        WebInspector.suggestReload();
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @type {?WebInspector.Workspace}
 */
WebInspector.workspace = null;
