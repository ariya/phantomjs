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

function LeaksLoader(didCountLeaksFilesCallback, didLoadLeaksFileCallback) {
    this._didCountLeaksFilesCallback = didCountLeaksFilesCallback;
    this._didLoadLeaksFileCallback = didLoadLeaksFileCallback;
}

LeaksLoader.prototype = {
    start: function(url) {
        if (/\.txt$/.test(url))
            this._loadLeaksFiles([url]);
        else
            this._loadLeaksFromResultsPage(url);
    },

    _loadLeaksFiles: function(urls) {
        this._didCountLeaksFilesCallback(urls.length);

        var self = this;
        var pendingURLs = urls.length;
        urls.forEach(function(url) {
            getResource(url, function(xhr) {
                self._didLoadLeaksFileCallback(xhr.responseText);
            });
        });
    },

    _loadLeaksFromResultsPage: function(url) {
        var self = this;
        getResource(url, function(xhr) {
            var root = document.createElement("html");
            root.innerHTML = xhr.responseText;

            // Strip off everything after the last /.
            var baseURL = url.substring(0, url.lastIndexOf("/") + 1);

            var urls = Array.prototype.map.call(root.querySelectorAll("tr.file > td > a[href$='-leaks.txt']"), function(link) { return baseURL + link.getAttribute("href"); });

            self._loadLeaksFiles(urls);
        });
    },
};
