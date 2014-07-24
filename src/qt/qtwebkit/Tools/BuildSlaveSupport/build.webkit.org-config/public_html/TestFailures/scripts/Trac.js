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

var trac = trac || {};

(function() {

function findUsingRegExp(string, regexp)
{
    var match = regexp.exec(string);
    if (match)
        return match[1];
    return null;
}

function findReviewer(message)
{
    var regexp = /Reviewed by ([^.]+)/;
    return findUsingRegExp(message, regexp);
}

function findAuthor(message)
{
    var regexp = /Patch by ([^<]+) </;
    return findUsingRegExp(message, regexp);
}

function findBugID(message)
{
    var regexp = /\/show_bug.cgi\?id=(\d+)/;
    return parseInt(findUsingRegExp(message, regexp), 10);
}

function findRevision(title)
{
    var regexp = /^Revision (\d+):/;
    return parseInt(findUsingRegExp(title, regexp), 10);
}

function findSummary(message)
{
    var lines = message.split('\n');
    for (var i = 0; i < lines.length; ++i) {
        var line = lines[i]
        if (findBugID(line))
            continue;
        if (findReviewer(line))
            continue;
        // Old-style commit message.
        if (/^\d\d\d\d-\d\d-\d\d/.exec(line))
            continue;
        if (line.length > 0)
            return line;
    }
}

function findRevert(message)
{
    var regexp = /rolling out r(\d+)\./;
    var revision = findUsingRegExp(message, regexp);
    if (!revision)
        return undefined;
    return parseInt(revision, 10);
}

// FIXME: Consider exposing this method for unit testing.
function parseCommitData(responseXML)
{
    var commits = Array.prototype.map.call(responseXML.getElementsByTagName('item'), function(item) {
        var title = item.getElementsByTagName('title')[0].textContent;
        var author = item.getElementsByTagName('author')[0].textContent;
        var time = item.getElementsByTagName('pubDate')[0].textContent;

        // FIXME: This isn't a very high-fidelity reproduction of the commit message,
        // but it's good enough for our purposes.
        var container = document.createElement('div');
        container.innerHTML = item.getElementsByTagName('description')[0].textContent;
        var message = container.innerText;

        return {
            'revision': findRevision(title),
            'title': title,
            'time': time,
            'summary': findSummary(message),
            'author': findAuthor(message) || author,
            'reviewer': findReviewer(message),
            'bugID': findBugID(message),
            'message': message,
            'revertedRevision': findRevert(message),
        };
    });

    return commits;
}

var g_cache = new base.AsynchronousCache(function(key, callback) {
    var explodedKey = key.split('\n');

    var path = explodedKey[0];
    var startRevision = explodedKey[1];
    var endRevision = explodedKey[2];

    var url = trac.logURL(path, startRevision, endRevision, true, true);

    net.get(url, function(commitData) {
        callback(parseCommitData(commitData));
    });
});

trac.changesetURL = function(revision)
{
    return config.kTracURL + '/changeset/' + revision;
};

trac.logURL = function(path, startRevision, endRevision, showFullCommitLogs, formatAsRSS)
{
    var queryParameters = {
        rev: endRevision,
        stop_rev: startRevision,
        // Trac requires limit to be 1 more than the number of revisions we actually want to show.
        // See <http://trac.edgewall.org/ticket/10317>.
        limit: endRevision - startRevision + 2,
    };

    if (showFullCommitLogs)
        queryParameters.verbose = 'on';
    if (formatAsRSS)
        queryParameters.format = 'rss';

    return config.kTracURL + '/log/' + path + '?' + $.param(queryParameters);
};

trac.recentCommitData = function(path, limit, callback)
{
    var url = config.kTracURL + '/log/' + path + '?' + $.param({
        'verbose': 'on',
        'format': 'rss',
        'limit': limit,
    });

    net.get(url, function(commitData) {
        callback(parseCommitData(commitData));
    });
};

trac.commitDataForRevisionRange = function(path, startRevision, endRevision, callback)
{
    var key = [path, startRevision, endRevision].join('\n');
    g_cache.get(key, callback);
};

})();
