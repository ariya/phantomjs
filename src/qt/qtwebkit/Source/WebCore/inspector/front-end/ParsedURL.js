/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL GOOGLE INC.
 * OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @param {string} url
 */
WebInspector.ParsedURL = function(url)
{
    this.isValid = false;
    this.url = url;
    this.scheme = "";
    this.host = "";
    this.port = "";
    this.path = "";
    this.queryParams = "";
    this.fragment = "";
    this.folderPathComponents = "";
    this.lastPathComponent = "";

    // RegExp groups:
    // 1 - scheme (using the RFC3986 grammar)
    // 2 - hostname
    // 3 - ?port
    // 4 - ?path
    // 5 - ?fragment
    var match = url.match(/^([A-Za-z][A-Za-z0-9+.-]*):\/\/([^\/:]*)(?::([\d]+))?(?:(\/[^#]*)(?:#(.*))?)?$/i);
    if (match) {
        this.isValid = true;
        this.scheme = match[1].toLowerCase();
        this.host = match[2];
        this.port = match[3];
        this.path = match[4] || "/";
        this.fragment = match[5];
    } else {
        if (this.url.startsWith("data:")) {
            this.scheme = "data";
            return;
        }
        if (this.url === "about:blank") {
            this.scheme = "about";
            return;
        }
        this.path = this.url;
    }

    // First cut the query params.
    var path = this.path;
    var indexOfQuery = path.indexOf("?");
    if (indexOfQuery !== -1) {
        this.queryParams = path.substring(indexOfQuery + 1)
        path = path.substring(0, indexOfQuery);
    }

    // Then take last path component.
    var lastSlashIndex = path.lastIndexOf("/");
    if (lastSlashIndex !== -1) {
        this.folderPathComponents = path.substring(0, lastSlashIndex);
        this.lastPathComponent = path.substring(lastSlashIndex + 1);
    } else
        this.lastPathComponent = path;
}

/**
 * @param {string} url
 * @return {Array.<string>}
 */
WebInspector.ParsedURL.splitURL = function(url)
{
    var parsedURL = new WebInspector.ParsedURL(url);
    var origin;
    var folderPath;
    var name;
    if (parsedURL.isValid) {
        origin = parsedURL.scheme + "://" + parsedURL.host;
        if (parsedURL.port)
            origin += ":" + parsedURL.port;
        folderPath = parsedURL.folderPathComponents;
        name = parsedURL.lastPathComponent;
        if (parsedURL.queryParams)
            name += "?" + parsedURL.queryParams;
    } else {
        origin = "";
        folderPath = "";
        name = url;
    }
    var result = [origin];
    var splittedPath = folderPath.split("/");
    for (var i = 1; i < splittedPath.length; ++i)
        result.push(splittedPath[i]);
    result.push(name);
    return result;
}

/**
 * @param {string} baseURL
 * @param {string} href
 * @return {?string}
 */
WebInspector.ParsedURL.completeURL = function(baseURL, href)
{
    if (href) {
        // Return special URLs as-is.
        var trimmedHref = href.trim();
        if (trimmedHref.startsWith("data:") || trimmedHref.startsWith("blob:") || trimmedHref.startsWith("javascript:"))
            return href;

        // Return absolute URLs as-is.
        var parsedHref = trimmedHref.asParsedURL();
        if (parsedHref && parsedHref.scheme)
            return trimmedHref;
    } else
        return baseURL;

    var parsedURL = baseURL.asParsedURL();
    if (parsedURL) {
        if (parsedURL.isDataURL())
            return href;
        var path = href;
        if (path.charAt(0) !== "/") {
            var basePath = parsedURL.path;

            // Trim off the query part of the basePath.
            var questionMarkIndex = basePath.indexOf("?");
            if (questionMarkIndex > 0)
                basePath = basePath.substring(0, questionMarkIndex);
            // A href of "?foo=bar" implies "basePath?foo=bar".
            // With "basePath?a=b" and "?foo=bar" we should get "basePath?foo=bar".
            var prefix;
            if (path.charAt(0) === "?") {
                var basePathCutIndex = basePath.indexOf("?");
                if (basePathCutIndex !== -1)
                    prefix = basePath.substring(0, basePathCutIndex);
                else
                    prefix = basePath;
            } else
                prefix = basePath.substring(0, basePath.lastIndexOf("/")) + "/";

            path = prefix + path;
        } else if (path.length > 1 && path.charAt(1) === "/") {
            // href starts with "//" which is a full URL with the protocol dropped (use the baseURL protocol).
            return parsedURL.scheme + ":" + path;
        }
        return parsedURL.scheme + "://" + parsedURL.host + (parsedURL.port ? (":" + parsedURL.port) : "") + path;
    }
    return null;
}

WebInspector.ParsedURL.prototype = {
    get displayName()
    {
        if (this._displayName)
            return this._displayName;

        if (this.isDataURL())
            return this.dataURLDisplayName();
        if (this.isAboutBlank())
            return this.url;

        this._displayName = this.lastPathComponent;
        if (!this._displayName && this.host)
            this._displayName = this.host + "/";
        if (!this._displayName && this.url)
            this._displayName = this.url.trimURL(WebInspector.inspectedPageDomain ? WebInspector.inspectedPageDomain : "");
        if (this._displayName === "/")
            this._displayName = this.url;
        return this._displayName;
    },

    dataURLDisplayName: function()
    {
        if (this._dataURLDisplayName)
            return this._dataURLDisplayName;
        if (!this.isDataURL())
            return "";
        this._dataURLDisplayName = this.url.trimEnd(20);
        return this._dataURLDisplayName;
    },

    isAboutBlank: function()
    {
        return this.url === "about:blank";
    },

    isDataURL: function()
    {
        return this.scheme === "data";
    }
}

/**
 * @return {?WebInspector.ParsedURL}
 */
String.prototype.asParsedURL = function()
{
    var parsedURL = new WebInspector.ParsedURL(this.toString());
    if (parsedURL.isValid)
        return parsedURL;
    return null;
}
