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

function Buildbot(baseURL) {
    this.baseURL = baseURL;
    this._cache = {};
}

Buildbot.prototype = {
    _builders: {},
    _resultsDirectory: 'results/',

    buildURL: function(builderName, buildName) {
        return this.baseURL + 'builders/' + builderName + '/builds/' + buildName;
    },

    builderNamed: function(name) {
        if (!(name in this._builders))
            this._builders[name] = new Builder(name, this);
        return this._builders[name];
    },

    getTesters: function(callback) {
        var cacheKey = 'getTesters';
        if (cacheKey in this._cache) {
            callback(this._buildersForNames(this._cache[cacheKey]));
            return;
        }

        var self = this;
        getResource(this.baseURL + this._resultsDirectory, function(xhr) {
            var root = document.createElement('html');
            root.innerHTML = xhr.responseText;
            var names = Array.prototype.map.call(root.querySelectorAll('td:first-child > a > b'), function(elem) {
                return elem.innerText.replace(/\/$/, '');
            });

            self._cache[cacheKey] = names;
            callback(self._buildersForNames(names));
        });
    },

    // Returns an object with at least the following properties:
    //   revision: source revision number for this build (integer)
    //   buildNumber: number of this build (integer)
    parseBuildName: function(buildName) {
        throw "Derived classes must implement";
    },

    resultsDirectoryURL: function(builderName, buildName) {
        return this.baseURL + this._resultsDirectory + encodeURIComponent(builderName) + '/' + encodeURIComponent(buildName) + '/';
    },

    _buildersForNames: function(names) {
        var self = this;
        return names.map(function(name) { return self.builderNamed(name) });
    },
};
