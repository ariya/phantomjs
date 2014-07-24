/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

var LeaksViewer = {
    loaded: function() {
        this._loader = new LeaksLoader(this._didCountLeaksFiles.bind(this), this._didLoadLeaksFile.bind(this));
        this._parser = new LeaksParser(this._didParseLeaksFile.bind(this));

        this._loadingIndicator = document.getElementById("loading-indicator");
        this._loadingIndicatorLabel = document.getElementById("loading-indicator-label");

        this._profileView = new WebInspector.CPUProfileView({});
        document.getElementById("main-panels").appendChild(this._profileView.element);
        this._profileView.show();

        // From WebInspector.Panel.prototype.show
        var statusBarItems = this._profileView.statusBarItems;
        if (statusBarItems) {
            this._statusBarItemContainer = document.createElement("div");
            for (var i = 0; i < statusBarItems.length; ++i)
                this._statusBarItemContainer.appendChild(statusBarItems[i]);
            document.getElementById("main-status-bar").appendChild(this._statusBarItemContainer);
        }

        var url;
        var match = /url=([^&]+)/.exec(location.search);
        if (match)
            url = decodeURIComponent(match[1]);

        if (url)
            this._loadLeaksFromURL(url)
        else
            this._displayURLPrompt();
    },

    get filesLeftToParse() {
        if (!('_filesLeftToParse' in this))
            this._filesLeftToParse = 0;
        return this._filesLeftToParse;
    },

    set filesLeftToParse(x) {
        if (this._filesLeftToParse === x)
            return;
        this._filesLeftToParse = x;
        this._loadingStatusChanged();
    },

    get loading() {
        return this._isLoading;
    },

    set loading(x) {
        if (this._isLoading === x)
            return;
        this._isLoading = x;
        this._loadingStatusChanged();
    },

    get url() {
        return this._url;
    },

    set url(x) {
        if (this._url === x)
            return;

        this._url = x;
        this._updateTitle();
    },

    urlPromptButtonClicked: function(e) {
        this._urlChosenFromPrompt(document.getElementById("url").value);
    },

    _didCountLeaksFiles: function(fileCount) {
        this._fileCount = fileCount;
        this.filesLeftToParse = fileCount;
    },

    _didLoadLeaksFile: function(leaksText) {
        this._parser.addLeaksFile(leaksText);
    },

    _didLoadRecentBuilds: function(builds) {
        this._recentBuilds = builds;
        this._updateURLPrompt();
    },

    _didParseLeaksFile: function(profile) {
        if (--this.filesLeftToParse)
            return;
        ProfilerAgent.profileReady(profile);
        this.loading = false;
    },

    _displayURLPrompt: function() {
        document.getElementById("url-prompt-container").removeStyleClass("hidden");
        document.getElementById("url").focus();
        var loader = new RecentBuildsLoader(this._didLoadRecentBuilds.bind(this));
        loader.start("Lion Leaks", this._numberOfRecentBuildsToLoad);
    },

    _loadLeaksFromURL: function(url) {
        this.url = url;
        this.loading = true;

        this._loader.start(this.url);
    },

    _loadingIndicatorText: function() {
        var text = "Loading";
        if (this.filesLeftToParse)
            text += " " + (this._fileCount - this.filesLeftToParse + 1) + "/" + this._fileCount + " files";
        text += "\u2026";
        return text;
    },

    _loadingStatusChanged: function() {
        this._setLoadingIndicatorHidden(!this.loading);
        this._updateLoadingIndicatorLabel();
        this._updateTitle();
    },

    _numberOfRecentBuildsToLoad: 10,

    _setLoadingIndicatorHidden: function(hidden) {
        if (hidden)
            this._loadingIndicator.addStyleClass("hidden");
        else
            this._loadingIndicator.removeStyleClass("hidden");
    },

    _updateLoadingIndicatorLabel: function() {
        this._loadingIndicatorLabel.innerText = this._loadingIndicatorText();
    },

    _updateTitle: function() {
        var title = "Leaks Viewer \u2014 ";
        if (this.loading)
            title += "(" + this._loadingIndicatorText() + ") ";
        title += this.url;
        document.title = title;
    },

    _updateURLPrompt: function() {
        var recentBuildsContainer = document.getElementById("recent-builds-container");
        recentBuildsContainer.removeChildren();
        if (this._recentBuilds && this._recentBuilds.length) {
            var list = document.createElement("ol");
            list.id = "recent-builds-list";

            var self = this;
            this._recentBuilds.forEach(function(build) {
                var link = document.createElement("a");
                link.href = document.location.href + "?url=" + encodeURIComponent(build.url);
                link.addEventListener("click", function(e) {
                    self._urlChosenFromPrompt(build.url);
                    e.preventDefault();
                });
                link.appendChild(document.createTextNode("r" + build.revision + ": " + build.leakCount + " leaks"));
                var item = document.createElement("li");
                item.appendChild(link);

                list.appendChild(item);
            });

            recentBuildsContainer.appendChild(list);
        } else
            recentBuildsContainer.appendChild(document.createTextNode("No recent leaky builds found."));
    },

    _urlChosenFromPrompt: function(url) {
        this._loadLeaksFromURL(url);
        document.getElementById("url-prompt-container").addStyleClass("hidden");
    },

};

addEventListener("load", LeaksViewer.loaded.bind(LeaksViewer));
