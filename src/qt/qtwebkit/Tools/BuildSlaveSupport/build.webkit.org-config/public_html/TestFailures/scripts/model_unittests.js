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

module("model");

var kExampleCommitDataXML =
    '<?xml version="1.0"?>\n\n' +
    '<rss xmlns:dc="http://purl.org/dc/elements/1.1/" version="2.0">\n\n' +
    '  \n\n' +
    '\n\n' +
    '  <channel>\n\n' +
    '    <title>Revisions of /trunk</title>\n\n' +
    '    <link>http://trac.webkit.org/log/trunk?rev=92362</link>\n\n' +
    '    <description>Trac Log - Revisions of /trunk</description>\n\n' +
    '    <language>en-US</language>\n\n' +
    '    <generator>Trac 0.11.7</generator>\n\n' +
    '    <image>\n\n' +
    '      <title>WebKit</title>\n\n' +
    '      <url>http://trac.webkit.org/chrome/site/icon.png</url>\n\n' +
    '      <link>http://trac.webkit.org/log/trunk?rev=92362</link>\n\n' +
    '    </image>\n\n' +
    '    <item>\n' +
    '          <author>commit-queue@webkit.org</author>\n' +
    '      <pubDate>Wed, 03 Aug 2011 04:26:52 GMT</pubDate>\n' +
    '      <title>Revision 92259: Unreviewed, rolling out r92256.\n' +
    'http://trac.webkit.org/changeset/92256 ...</title>\n' +
    '      <link>http://trac.webkit.org/changeset/92259/trunk</link>\n' +
    '      <guid isPermaLink="false">http://trac.webkit.org/changeset/92259/trunk</guid>\n' +
    '      <description>&lt;p&gt;\n' +
    'Unreviewed, rolling out &lt;a class="changeset" href="http://trac.webkit.org/changeset/92256" title="Make EventDispatchMediator RefCounted. ..."&gt;r92256&lt;/a&gt;.\n' +
    '&lt;a class="ext-link" href="http://trac.webkit.org/changeset/92256"&gt;&lt;span class="icon"&gt; &lt;/span&gt;http://trac.webkit.org/changeset/92256&lt;/a&gt;\n' +
    '&lt;a class="ext-link" href="https://bugs.webkit.org/show_bug.cgi?id=65593"&gt;&lt;span class="icon"&gt; &lt;/span&gt;https://bugs.webkit.org/show_bug.cgi?id=65593&lt;/a&gt;\n' +
    '&lt;/p&gt;\n' +
    '&lt;p&gt;\n' +
    'Causing tons of crashes on the chromium win bots (Requested by\n' +
    'jamesr on #webkit).\n' +
    '&lt;/p&gt;\n' +
    '&lt;p&gt;\n' +
    'Patch by Sheriff Bot &amp;lt;&lt;a class="mail-link" href="mailto:webkit.review.bot@gmail.com"&gt;&lt;span class="icon"&gt; &lt;/span&gt;webkit.review.bot@gmail.com&lt;/a&gt;&amp;gt; on 2011-08-02\n' +
    '&lt;/p&gt;\n' +
    '&lt;p&gt;\n' +
    '* dom/Event.cpp:\n' +
    '* dom/Event.h:\n' +
    '* dom/EventDispatcher.cpp:\n' +
    '(WebCore::EventDispatcher::dispatchEvent):\n' +
    '* dom/EventDispatcher.h:\n' +
    '* dom/KeyboardEvent.cpp:\n' +
    '* dom/KeyboardEvent.h:\n' +
    '* dom/MouseEvent.cpp:\n' +
    '* dom/MouseEvent.h:\n' +
    '* dom/Node.cpp:\n' +
    '(WebCore::Node::dispatchEvent):\n' +
    '(WebCore::Node::dispatchKeyEvent):\n' +
    '(WebCore::Node::dispatchMouseEvent):\n' +
    '(WebCore::Node::dispatchWheelEvent):\n' +
    '* dom/WheelEvent.cpp:\n' +
    '* dom/WheelEvent.h:\n' +
    '&lt;/p&gt;\n' +
    '</description>\n' +
    '      <category>Log</category>\n' +
    '    </item><item>\n' +
    '          <author>macpherson@chromium.org</author>\n\n' +
    '      <pubDate>Thu, 04 Aug 2011 02:09:19 GMT</pubDate>\n\n' +
    '      <title>Revision 92256: Support cast between CSSPrimitiveValue and EBoxSizing, use in ...</title>\n\n' +
    '      <link>http://trac.webkit.org/changeset/92256/trunk</link>\n\n' +
    '      <guid isPermaLink="false">http://trac.webkit.org/changeset/92256/trunk</guid>\n\n' +
    '      <description>&lt;p&gt;\n\n' +
    'Support cast between CSSPrimitiveValue and EBoxSizing, use in CSSStyleSelector.\n\n' +
    '&lt;a class="ext-link" href="https://bugs.webkit.org/show_bug.cgi?id=65657"&gt;&lt;span class="icon"&gt; &lt;/span&gt;https://bugs.webkit.org/show_bug.cgi?id=65657&lt;/a&gt;\n\n' +
    '&lt;/p&gt;\n\n' +
    '&lt;p&gt;\n\n' +
    'Reviewed by Simon Fraser.\n\n' +
    '&lt;/p&gt;\n\n' +
    '&lt;p&gt;\n\n' +
    'No new tests / refactoring only.\n\n' +
    '&lt;/p&gt;\n\n' +
    '&lt;p&gt;\n\n' +
    '* css/CSSPrimitiveValueMappings.h:\n\n' +
    '(WebCore::CSSPrimitiveValue::CSSPrimitiveValue):\n\n' +
    'Implement cast from EBoxSizing.\n' +
    '(WebCore::CSSPrimitiveValue::operator EBoxSizing):\n' +
    'Implement cast to EBoxSizing.\n' +
    '* css/CSSStyleSelector.cpp:\n' +
    '(WebCore::CSSStyleSelector::applyProperty):\n' +
    'Use appropriate macro to simplify code using cast.\n' +
    '&lt;/p&gt;\n' +
    '</description>\n' +
    '      <category>Log</category>\n' +
    '    </item>\n' +
    ' </channel>\n' +
    '</rss>\n'

test("rebaselineQueue", 3, function() {
    var queue = model.takeRebaselineQueue();
    deepEqual(queue, []);
    model.queueForRebaseline('failureInfo1');
    model.queueForRebaseline('failureInfo2');
    var queue = model.takeRebaselineQueue();
    deepEqual(queue, ['failureInfo1', 'failureInfo2']);
    var queue = model.takeRebaselineQueue();
    deepEqual(queue, []);
});

test("rebaselineQueue", 3, function() {
    var queue = model.takeExpectationUpdateQueue();
    deepEqual(queue, []);
    model.queueForExpectationUpdate('failureInfo1');
    model.queueForExpectationUpdate('failureInfo2');
    var queue = model.takeExpectationUpdateQueue();
    deepEqual(queue, ['failureInfo1', 'failureInfo2']);
    var queue = model.takeExpectationUpdateQueue();
    deepEqual(queue, []);
});

test("updateRecentCommits", 2, function() {
    var simulator = new NetworkSimulator();

    simulator.get = function(url, callback)
    {
        simulator.scheduleCallback(function() {
            var parser = new DOMParser();
            var responseDOM = parser.parseFromString(kExampleCommitDataXML, "application/xml");
            callback(responseDOM);
        });
    };

    simulator.runTest(function() {
        model.updateRecentCommits(function() {
            var recentCommits = model.state.recentCommits;
            delete model.state.recentCommits;
            $.each(recentCommits, function(index, commitData) {
                delete commitData.message;
            });
            deepEqual(recentCommits, [{
                "revision": 92259,
                "title": "Revision 92259: Unreviewed, rolling out r92256.\nhttp://trac.webkit.org/changeset/92256 ...",
                "time": "Wed, 03 Aug 2011 04:26:52 GMT",
                "summary": "Unreviewed, rolling out r92256.",
                "author": "Sheriff Bot",
                "reviewer": null,
                "bugID": 65593,
                "revertedRevision": 92256
             }, {
                "revision": 92256,
                "title": "Revision 92256: Support cast between CSSPrimitiveValue and EBoxSizing, use in ...",
                "time": "Thu, 04 Aug 2011 02:09:19 GMT",
                "summary": "Support cast between CSSPrimitiveValue and EBoxSizing, use in CSSStyleSelector.",
                "author": "macpherson@chromium.org",
                "reviewer": "Simon Fraser",
                "bugID": 65657,
                "revertedRevision": undefined,
                "wasReverted": true
            }]);
        });
    });
});

test("commitDataListForRevisionRange", 6, function() {
    var simulator = new NetworkSimulator();

    simulator.get = function(url, callback)
    {
        simulator.scheduleCallback(function() {
            var parser = new DOMParser();
            var responseDOM = parser.parseFromString(kExampleCommitDataXML, "application/xml");
            callback(responseDOM);
        });
    };

    simulator.runTest(function() {
        model.updateRecentCommits(function() {
            function extractBugIDs(commitData)
            {
                return commitData.bugID;
            }

            deepEqual(model.commitDataListForRevisionRange(92259, 92259).map(extractBugIDs), [65593]);
            deepEqual(model.commitDataListForRevisionRange(92256, 92259).map(extractBugIDs), [65657, 65593]);
            deepEqual(model.commitDataListForRevisionRange(92259, 92256).map(extractBugIDs), []);
            deepEqual(model.commitDataListForRevisionRange(0, 92256).map(extractBugIDs), [65657]);
            deepEqual(model.commitDataListForRevisionRange(92256, 0).map(extractBugIDs), []);
            delete model.state.recentCommits;
        });
    });
});

test("buildersInFlightForRevision", 3, function() {
    var unmock = model.state.resultsByBuilder;
    model.state.resultsByBuilder = {
        'Mr. Beasley': {revision: '5'},
        'Mr Dixon': {revision: '1'},
        'Mr. Sabatini': {revision: '4'},
        'Bob': {revision: '6'}
    };
    deepEqual(model.buildersInFlightForRevision(1), {});
    deepEqual(model.buildersInFlightForRevision(3), {
      "Mr Dixon": {
        "actual": "BUILDING"
      }
    });
    deepEqual(model.buildersInFlightForRevision(10), {
      "Mr. Beasley": {
        "actual": "BUILDING"
      },
      "Mr Dixon": {
        "actual": "BUILDING"
      },
      "Mr. Sabatini": {
        "actual": "BUILDING"
      },
      "Bob": {
        "actual": "BUILDING"
      }
    });
    model.state.resultsByBuilder = unmock;
});

test("latestRevisionWithNoBuildersInFlight", 1, function() {
    var unmock = model.state.resultsByBuilder;
    model.state.resultsByBuilder = {
        'Mr. Beasley': { },
        'Mr Dixon': {revision: '2'},
        'Mr. Sabatini': {revision: '4'},
        'Bob': {revision: '6'}
    };
    equals(model.latestRevisionWithNoBuildersInFlight(), 2);
    model.state.resultsByBuilder = unmock;
});

})();
