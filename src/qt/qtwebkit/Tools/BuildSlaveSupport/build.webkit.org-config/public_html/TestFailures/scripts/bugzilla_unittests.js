/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

(function () {

module("bugzilla");

var kExampleResponseXML =
    '<?xml version="1.0" encoding="UTF-8"?>' +
    '<feed xmlns="http://www.w3.org/2005/Atom">' +
    '  <title>Bugzilla Bugs</title>' +
    '  <link rel="alternate" type="text/html"' +
    '        href="https://bugs.webkit.org/buglist.cgi?bug_status=REOPENED&amp;bug_status=NEW&amp;bug_status=ASSIGNED&amp;bug_status=UNCONFIRMED&amp;field-1-0-0=bug_status&amp;field0-0-0=product&amp;field0-0-1=component&amp;field0-0-2=short_desc&amp;field0-0-3=status_whiteboard&amp;field0-0-4=longdesc&amp;query_format=advanced&amp;remaction=&amp;type-1-0-0=anyexact&amp;type0-0-0=substring&amp;type0-0-1=substring&amp;type0-0-2=substring&amp;type0-0-3=substring&amp;type0-0-4=substring&amp;value-1-0-0=REOPENED%2CNEW%2CASSIGNED%2CUNCONFIRMED&amp;value0-0-0=garden-o-matic&amp;value0-0-1=garden-o-matic&amp;value0-0-2=garden-o-matic&amp;value0-0-3=garden-o-matic&amp;value0-0-4=garden-o-matic"/>' +
    '  <link rel="self" type="application/atom+xml"' +
    '        href="https://bugs.webkit.org/buglist.cgi?bug_status=REOPENED&amp;bug_status=NEW&amp;bug_status=ASSIGNED&amp;bug_status=UNCONFIRMED&amp;ctype=atom&amp;field-1-0-0=bug_status&amp;field0-0-0=product&amp;field0-0-1=component&amp;field0-0-2=short_desc&amp;field0-0-3=status_whiteboard&amp;field0-0-4=longdesc&amp;query_format=advanced&amp;remaction=&amp;type-1-0-0=anyexact&amp;type0-0-0=substring&amp;type0-0-1=substring&amp;type0-0-2=substring&amp;type0-0-3=substring&amp;type0-0-4=substring&amp;value-1-0-0=REOPENED%2CNEW%2CASSIGNED%2CUNCONFIRMED&amp;value0-0-0=garden-o-matic&amp;value0-0-1=garden-o-matic&amp;value0-0-2=garden-o-matic&amp;value0-0-3=garden-o-matic&amp;value0-0-4=garden-o-matic"/>' +
    '  <updated>2011-08-04T00:22:49Z</updated>' +
    '  <id>https://bugs.webkit.org/buglist.cgi?bug_status=REOPENED&amp;bug_status=NEW&amp;bug_status=ASSIGNED&amp;bug_status=UNCONFIRMED&amp;ctype=atom&amp;field-1-0-0=bug_status&amp;field0-0-0=product&amp;field0-0-1=component&amp;field0-0-2=short_desc&amp;field0-0-3=status_whiteboard&amp;field0-0-4=longdesc&amp;query_format=advanced&amp;remaction=&amp;type-1-0-0=anyexact&amp;type0-0-0=substring&amp;type0-0-1=substring&amp;type0-0-2=substring&amp;type0-0-3=substring&amp;type0-0-4=substring&amp;value-1-0-0=REOPENED%2CNEW%2CASSIGNED%2CUNCONFIRMED&amp;value0-0-0=garden-o-matic&amp;value0-0-1=garden-o-matic&amp;value0-0-2=garden-o-matic&amp;value0-0-3=garden-o-matic&amp;value0-0-4=garden-o-matic</id>' +
    '' +
    '  <entry>' +
    '    <title>[Bug 65654] Add missing license blocks to garden-o-matic</title>' +
    '    <link rel="alternate" type="text/html"' +
    '          href="https://bugs.webkit.org/show_bug.cgi?id=65654"/>' +
    '    <id>https://bugs.webkit.org/show_bug.cgi?id=65654</id>' +
    '    <author>' +
    '      <name>Adam Barth</name>' +
    '    </author>' +
    '    <updated>2011-08-04T00:22:26Z</updated> ' +
    '    <summary type="html">' +
    '      ' +
    '      &lt;table&gt;' +
    '      &lt;tr&gt;' +
    '        &lt;th&gt;Field&lt;/th&gt;&lt;th&gt;Value&lt;/th&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_product&quot;&gt;' +
    '        &lt;td&gt;Product&lt;/td&gt;' +
    '        &lt;td&gt;WebKit&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_component&quot;&gt;' +
    '        &lt;td&gt;Component&lt;/td&gt;' +
    '        &lt;td&gt;New Bugs&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_assignee&quot;&gt;' +
    '        &lt;td&gt;Assignee&lt;/td&gt;' +
    '        &lt;td&gt;Adam Barth&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_reporter&quot;&gt;' +
    '        &lt;td&gt;Reporter&lt;/td&gt;' +
    '        &lt;td&gt;Adam Barth&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_bug_status&quot;&gt;' +
    '        &lt;td&gt;Status&lt;/td&gt;' +
    '        &lt;td&gt;NEW&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_resolution&quot;&gt;' +
    '        &lt;td&gt;Resolution &lt;/td&gt;' +
    '        &lt;td&gt;&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_priority&quot;&gt;' +
    '        &lt;td&gt;Priority&lt;/td&gt;' +
    '        &lt;td&gt;P2&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_severity&quot;&gt;' +
    '        &lt;td&gt;Severity &lt;/td&gt;' +
    '        &lt;td&gt;Normal&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_creation_date&quot;&gt;' +
    '        &lt;td&gt;Opened&lt;/td&gt;' +
    '        &lt;td&gt;16:21:52&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_changed_date&quot;&gt;' +
    '        &lt;td&gt;Changed&lt;/td&gt;' +
    '        &lt;td&gt;16:22:26&lt;/td&gt;' +
    '      &lt;/tr&gt;' +
    '      &lt;/table&gt;' +
    '    </summary>' +
    '  </entry>' +
    '  <entry>' +
    '    <title>[Bug 65653] garden-o-matic needs a way to mock out the network</title>' +
    '    <link rel="alternate" type="text/html"' +
    '          href="https://bugs.webkit.org/show_bug.cgi?id=65653"/>' +
    '    <id>https://bugs.webkit.org/show_bug.cgi?id=65653</id>' +
    '    <author>' +
    '      <name>Adam Barth</name>' +
    '    </author>' +
    '    <updated>2011-08-04T00:22:49Z</updated> ' +
    '    <summary type="html">' +
    '      ' +
    '      &lt;table&gt;' +
    '      &lt;tr&gt;' +
    '        &lt;th&gt;Field&lt;/th&gt;&lt;th&gt;Value&lt;/th&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_product&quot;&gt;' +
    '        &lt;td&gt;Product&lt;/td&gt;' +
    '        &lt;td&gt;WebKit&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_component&quot;&gt;' +
    '        &lt;td&gt;Component&lt;/td&gt;' +
    '        &lt;td&gt;New Bugs&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_assignee&quot;&gt;' +
    '        &lt;td&gt;Assignee&lt;/td&gt;' +
    '        &lt;td&gt;Adam Barth&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_reporter&quot;&gt;' +
    '        &lt;td&gt;Reporter&lt;/td&gt;' +
    '        &lt;td&gt;Adam Barth&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_bug_status&quot;&gt;' +
    '        &lt;td&gt;Status&lt;/td&gt;' +
    '        &lt;td&gt;NEW&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_resolution&quot;&gt;' +
    '        &lt;td&gt;Resolution &lt;/td&gt;' +
    '        &lt;td&gt;&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_priority&quot;&gt;' +
    '        &lt;td&gt;Priority&lt;/td&gt;' +
    '        &lt;td&gt;P2&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_severity&quot;&gt;' +
    '        &lt;td&gt;Severity &lt;/td&gt;' +
    '        &lt;td&gt;Normal&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_creation_date&quot;&gt;' +
    '        &lt;td&gt;Opened&lt;/td&gt;' +
    '        &lt;td&gt;16:16:44&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_changed_date&quot;&gt;' +
    '        &lt;td&gt;Changed&lt;/td&gt;' +
    '        &lt;td&gt;16:22:49&lt;/td&gt;' +
    '      &lt;/tr&gt;' +
    '      &lt;/table&gt;' +
    '    </summary>' +
    '  </entry>' +
    '  <entry>' +
    '    <title>[Bug 65650] Use failureInfo more pervasively in garden-o-matic</title>' +
    '    <link rel="alternate" type="text/html"' +
    '          href="https://bugs.webkit.org/show_bug.cgi?id=65650"/>' +
    '    <id>https://bugs.webkit.org/show_bug.cgi?id=65650</id>' +
    '    <author>' +
    '      <name>Adam Barth</name>' +
    '    </author>' +
    '    <updated>2011-08-03T23:40:34Z</updated> ' +
    '    <summary type="html">' +
    '      ' +
    '      &lt;table&gt;' +
    '      &lt;tr&gt;' +
    '        &lt;th&gt;Field&lt;/th&gt;&lt;th&gt;Value&lt;/th&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_product&quot;&gt;' +
    '        &lt;td&gt;Product&lt;/td&gt;' +
    '        &lt;td&gt;WebKit&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_component&quot;&gt;' +
    '        &lt;td&gt;Component&lt;/td&gt;' +
    '        &lt;td&gt;New Bugs&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_assignee&quot;&gt;' +
    '        &lt;td&gt;Assignee&lt;/td&gt;' +
    '        &lt;td&gt;Adam Barth&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_reporter&quot;&gt;' +
    '        &lt;td&gt;Reporter&lt;/td&gt;' +
    '        &lt;td&gt;Adam Barth&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_bug_status&quot;&gt;' +
    '        &lt;td&gt;Status&lt;/td&gt;' +
    '        &lt;td&gt;NEW&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_resolution&quot;&gt;' +
    '        &lt;td&gt;Resolution &lt;/td&gt;' +
    '        &lt;td&gt;&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_priority&quot;&gt;' +
    '        &lt;td&gt;Priority&lt;/td&gt;' +
    '        &lt;td&gt;P2&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_severity&quot;&gt;' +
    '        &lt;td&gt;Severity &lt;/td&gt;' +
    '        &lt;td&gt;Normal&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_creation_date&quot;&gt;' +
    '        &lt;td&gt;Opened&lt;/td&gt;' +
    '        &lt;td&gt;15:35:55&lt;/td&gt;' +
    '      &lt;/tr&gt;&lt;tr class=&quot;bz_feed_changed_date&quot;&gt;' +
    '        &lt;td&gt;Changed&lt;/td&gt;' +
    '        &lt;td&gt;15:40:34&lt;/td&gt;' +
    '      &lt;/tr&gt;' +
    '      &lt;/table&gt;' +
    '    </summary>' +
    '  </entry>' +
    '</feed>'

test("quickSearch", 3, function() {
    var simulator = new NetworkSimulator();

    var requestedURLs = [];
    simulator.get = function(url, callback)
    {
        requestedURLs.push(url);
        simulator.scheduleCallback(function() {
            var parser = new DOMParser();
            var responseDOM = parser.parseFromString(kExampleResponseXML, "application/xml");
            callback(responseDOM);
        });
    };

    simulator.runTest(function() {
        bugzilla.quickSearch('garden-o-matic', function(bugs) {
            deepEqual(bugs, [{
                "title": "[Bug 65654] Add missing license blocks to garden-o-matic",
                "url": "https://bugs.webkit.org/show_bug.cgi?id=65654",
                "status": "NEW"
            }, {
                "title": "[Bug 65653] garden-o-matic needs a way to mock out the network",
                "url": "https://bugs.webkit.org/show_bug.cgi?id=65653",
                "status": "NEW"
            }, {
                "title": "[Bug 65650] Use failureInfo more pervasively in garden-o-matic",
                "url": "https://bugs.webkit.org/show_bug.cgi?id=65650",
                "status": "NEW"
            }]);
        });
    });

    deepEqual(requestedURLs, [
        "https://bugs.webkit.org/buglist.cgi?ctype=rss&order=bugs.bug_id+desc&quicksearch=garden-o-matic",
    ]);
});

test("isOpenStatus", 6, function() {
    ok(bugzilla.isOpenStatus('UNCONFIRMED'));
    ok(bugzilla.isOpenStatus('NEW'));
    ok(bugzilla.isOpenStatus('ASSIGNED'));
    ok(bugzilla.isOpenStatus('REOPENED'));
    ok(!bugzilla.isOpenStatus('FIXED'));
    ok(!bugzilla.isOpenStatus('VERIFIED'));
});

})();
