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

var bugzilla = bugzilla || {};

(function() {

var kOpenStatuses = {
    UNCONFIRMED: true,
    NEW: true,
    ASSIGNED: true,
    REOPENED: true,
};

function createDetachedFragment(htmlFragment)
{
    // Step 1: Create a detached Document to perform the parsing.
    var detachedDocument = document.implementation.createHTMLDocument();

    // Step 2: Create a detached Element associated with the detached Document.
    var container = detachedDocument.createElement('div');

    // Step 3: Pull the trigger.
    container.innerHTML = htmlFragment;
    return container;
}

var g_searchCache = new base.AsynchronousCache(function(query, callback) {
    var url = config.kBugzillaURL + '/buglist.cgi?' + $.param({
        ctype: 'rss',
        order: 'bugs.bug_id desc',
        quicksearch: query,
    });

    net.get(url, function(responseXML) {
        var entries = responseXML.getElementsByTagName('entry');
        var results = Array.prototype.map.call(entries, function(entry) {
            var htmlFragment = entry.getElementsByTagName('summary')[0].textContent;
            var fragment = createDetachedFragment(htmlFragment);
            var statusRow = fragment.querySelector('tr.bz_feed_bug_status');
            return {
                title: entry.getElementsByTagName('title')[0].textContent,
                url: entry.getElementsByTagName('id')[0].textContent,
                status: statusRow.cells[1].textContent,
            };
        });
        callback(results);
    });
});

bugzilla.bugURL = function(bugNumber)
{
    return config.kBugzillaURL + '/show_bug.cgi?id=' + bugNumber;
};

bugzilla.quickSearch = function(query, callback)
{
    g_searchCache.get(query, callback);
};

bugzilla.isOpenStatus = function(status)
{
    return status in kOpenStatuses;
};

// This value is built-in to all Bugzilla installations. See <http://webkit.org/b/61660>.
bugzilla.kMaximumBugTitleLength = 255;

})();
