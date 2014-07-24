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

module("trac");

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
    '    <item>\n\n' +
    '          <author>macpherson@chromium.org</author>\n\n' +
    '      <pubDate>Thu, 04 Aug 2011 02:09:19 GMT</pubDate>\n\n' +
    '      <title>Revision 92342: Support cast between CSSPrimitiveValue and EBoxSizing, use in ...</title>\n\n' +
    '      <link>http://trac.webkit.org/changeset/92342/trunk</link>\n\n' +
    '      <guid isPermaLink="false">http://trac.webkit.org/changeset/92342/trunk</guid>\n\n' +
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
    '    </item><item>\n' +
    '          <author>commit-queue@webkit.org</author>\n' +
    '      <pubDate>Thu, 04 Aug 2011 02:01:31 GMT</pubDate>\n' +
    '      <title>Revision 92341: Implement EventSender.scalePageBy() ...</title>\n' +
    '      <link>http://trac.webkit.org/changeset/92341/trunk</link>\n' +
    '      <guid isPermaLink="false">http://trac.webkit.org/changeset/92341/trunk</guid>\n' +
    '      <description>&lt;p&gt;\n' +
    'Implement EventSender.scalePageBy()\n' +
    '&lt;a class="ext-link" href="https://bugs.webkit.org/show_bug.cgi?id=58013"&gt;&lt;span class="icon"&gt; &lt;/span&gt;https://bugs.webkit.org/show_bug.cgi?id=58013&lt;/a&gt;\n' +
    '&lt;/p&gt;\n' +
    '&lt;p&gt;\n' +
    'Patch by Kentaro Hara &amp;lt;&lt;a class="mail-link" href="mailto:haraken@google.com"&gt;&lt;span class="icon"&gt; &lt;/span&gt;haraken@google.com&lt;/a&gt;&amp;gt; on 2011-08-03\n' +
    'Reviewed by Darin Fisher.\n' +
    '&lt;/p&gt;\n' +
    '&lt;p&gt;\n' +
    'Implemented EventSender.scalePageBy(f, x, y), which scales a page by a factor of f\n' +
    'and then sets a scroll position to (x, y). Enabled the tests that had been waiting\n' +
    'for the implementation of EventSender.scalePageBy(f, x, y).\n' +
    '&lt;/p&gt;\n' +
    '&lt;p&gt;\n' +
    'Source/WebKit/chromium:\n' +
    '&lt;/p&gt;\n' +
    '&lt;p&gt;\n' +
    'Tests: compositing/scaling/tiled-layer-recursion.html\n' +
    '&lt;/p&gt;\n' +
    '&lt;blockquote&gt;\n' +
    '&lt;p&gt;\n' +
    'fast/repaint/scale-page-shrink.html\n' +
    'fast/dom/Element/scale-page-client-rects.html\n' +
    'fast/dom/Range/scale-page-client-rects.html\n' +
    'fast/events/scroll-in-scaled-page-with-overflow-hidden.html\n' +
    'fast/dom/Element/scale-page-bounding-client-rect.html\n' +
    'fast/dom/Range/scale-page-bounding-client-rect.html\n' +
    '&lt;/p&gt;\n' +
    '&lt;/blockquote&gt;\n' +
    '&lt;p&gt;\n' +
    '* public/WebView.h:\n' +
    '* src/WebViewImpl.cpp:\n' +
    '(WebKit::WebViewImpl::scalePage): A wrapper method for scalePage() in WebCore.\n' +
    '* src/WebViewImpl.h:\n' +
    '&lt;/p&gt;\n' +
    '&lt;p&gt;\n' +
    'Tools:\n' +
    '&lt;/p&gt;\n' +
    '&lt;p&gt;\n' +
    'Tests: compositing/scaling/tiled-layer-recursion.html\n' +
    '&lt;/p&gt;\n' +
    '&lt;blockquote&gt;\n' +
    '&lt;p&gt;\n' +
    'fast/repaint/scale-page-shrink.html\n' +
    'fast/dom/Element/scale-page-client-rects.html\n' +
    'fast/dom/Range/scale-page-client-rects.html\n' +
    'fast/events/scroll-in-scaled-page-with-overflow-hidden.html\n' +
    'fast/dom/Element/scale-page-bounding-client-rect.html\n' +
    'fast/dom/Range/scale-page-bounding-client-rect.html\n' +
    '&lt;/p&gt;\n' +
    '&lt;/blockquote&gt;\n' +
    '&lt;p&gt;\n' +
    '* DumpRenderTree/chromium/EventSender.cpp:\n' +
    '(EventSender::EventSender): Added bindings for scalePageBy().\n' +
    '(EventSender::scalePageBy): A wrapper method for scalePage() in WebView.\n' +
    '* DumpRenderTree/chromium/EventSender.h:\n' +
    '* DumpRenderTree/chromium/TestShell.cpp:\n' +
    '(TestShell::resetTestController): Resets the scale factor to 1.\n' +
    '&lt;/p&gt;\n' +
    '&lt;p&gt;\n' +
    'LayoutTests:\n' +
    '&lt;/p&gt;\n' +
    '&lt;p&gt;\n' +
    '* platform/chromium-linux/compositing/scaling/tiled-layer-recursion-expected.png: Added.\n' +
    '* platform/chromium-linux/fast/repaint/scale-page-shrink-expected.png: Added.\n' +
    '* platform/chromium-linux/fast/repaint/scale-page-shrink-expected.txt: Added.\n' +
    '* platform/chromium-mac/fast/dom/Element/scale-page-bounding-client-rect-expected.txt: Removed.\n' +
    '* platform/chromium-mac/fast/dom/Range/scale-page-bounding-client-rect-expected.txt: Removed.\n' +
    '* platform/chromium-win/fast/dom/Element/scale-page-bounding-client-rect-expected.txt: Removed.\n' +
    '* platform/chromium-win/fast/dom/Element/scale-page-client-rects-expected.txt: Removed.\n' +
    '* platform/chromium-win/fast/dom/Range/scale-page-bounding-client-rect-expected.txt: Removed.\n' +
    '* platform/chromium-win/fast/dom/Range/scale-page-client-rects-expected.txt: Removed.\n' +
    '* platform/chromium/test_expectations.txt: Enabled one test. Enabled two tests for chromium-linux.\n' +
    '&lt;/p&gt;\n' +
    '</description>\n' +
    '      <category>Log</category>\n' +
    '    </item><item>\n' +
    '          <author>rniwa@webkit.org</author>\n' +
    '      <pubDate>Thu, 04 Aug 2011 01:41:29 GMT</pubDate>\n' +
    '      <title>Revision 92338: Revert an erroneous rebaseline from r92315.\n' +
    '* ...</title>\n' +
    '      <link>http://trac.webkit.org/changeset/92338/trunk</link>\n' +
    '      <guid isPermaLink="false">http://trac.webkit.org/changeset/92338/trunk</guid>\n' +
    '      <description>&lt;p&gt;\n' +
    'Revert an erroneous rebaseline from &lt;a class="changeset" href="http://trac.webkit.org/changeset/92315" title="Remove LegacyDefaultOptionalArguments flag from navigator IDL files ..."&gt;r92315&lt;/a&gt;.\n' +
    '&lt;/p&gt;\n' +
    '&lt;p&gt;\n' +
    '* fast/dom/navigator-detached-no-crash-expected.txt:\n' +
    '&lt;/p&gt;\n' +
    '</description>\n' +
    '      <category>Log</category>\n' +
    '    </item><item>\n' +
    '          <author>noam.rosenthal@nokia.com</author>\n' +
    '      <pubDate>Thu, 04 Aug 2011 00:22:21 GMT</pubDate>\n' +
    '      <title>Revision 92337: [Qt][Texmap][REGRESSION] http://webkit.org/blog-files/transform-style.html ...</title>\n' +
    '      <link>http://trac.webkit.org/changeset/92337/trunk</link>\n' +
    '      <guid isPermaLink="false">http://trac.webkit.org/changeset/92337/trunk</guid>\n' +
    '      <description>&lt;p&gt;\n' +
    '[Qt][Texmap][REGRESSION] &lt;a class="ext-link" href="http://webkit.org/blog-files/transform-style.html"&gt;&lt;span class="icon"&gt; &lt;/span&gt;http://webkit.org/blog-files/transform-style.html&lt;/a&gt; doesn\'t show composited content\n' +
    '&lt;a class="ext-link" href="https://bugs.webkit.org/show_bug.cgi?id=65629"&gt;&lt;span class="icon"&gt; &lt;/span&gt;https://bugs.webkit.org/show_bug.cgi?id=65629&lt;/a&gt;\n' +
    '&lt;/p&gt;\n' +
    '&lt;p&gt;\n' +
    'Reviewed by Benjamin Poulain.\n' +
    '&lt;/p&gt;\n' +
    '&lt;p&gt;\n' +
    'Some non-ES2 initialization was wrongfully #ifdefed in CPU(X86) and thus compiled-out.\n' +
    'When put it in the correct #ifdef, composited layers which require an intermediate buffer\n' +
    'work again.\n' +
    '&lt;/p&gt;\n' +
    '&lt;p&gt;\n' +
    'No new tests. Existing opacity tests in LayoutTests/compositing test this.\n' +
    '&lt;/p&gt;\n' +
    '&lt;p&gt;\n' +
    '* platform/graphics/opengl/TextureMapperGL.cpp:\n' +
    '(WebCore::BitmapTextureGL::bind):\n' +
    '&lt;/p&gt;\n' +
    '</description>\n' +
    '      <category>Log</category>\n' +
    '    </item><item>\n' +
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
    '    </item>\n' +
    ' </channel>\n' +
    '</rss>\n'

var kExampleCommitDataList = [{
    "revision": 92342,
    "title": "Revision 92342: Support cast between CSSPrimitiveValue and EBoxSizing, use in ...",
    "time": "Thu, 04 Aug 2011 02:09:19 GMT",
    "summary": "Support cast between CSSPrimitiveValue and EBoxSizing, use in CSSStyleSelector.",
    "author": "macpherson@chromium.org",
    "reviewer": "Simon Fraser",
    "bugID": 65657,
    "revertedRevision": undefined
}, {
    "revision": 92341,
    "title": "Revision 92341: Implement EventSender.scalePageBy() ...",
    "time": "Thu, 04 Aug 2011 02:01:31 GMT",
    "summary": "Implement EventSender.scalePageBy()",
    "author": "Kentaro Hara",
    "reviewer": "Darin Fisher",
    "bugID": 58013,
    "revertedRevision": undefined
}, {
    "revision": 92338,
    "title": "Revision 92338: Revert an erroneous rebaseline from r92315.\n* ...",
    "time": "Thu, 04 Aug 2011 01:41:29 GMT",
    "summary": "Revert an erroneous rebaseline from r92315.",
    "author": "rniwa@webkit.org",
    "reviewer": null,
    "bugID": NaN,
    "revertedRevision": undefined
}, {
    "revision": 92337,
    "title": "Revision 92337: [Qt][Texmap][REGRESSION] http://webkit.org/blog-files/transform-style.html ...",
    "time": "Thu, 04 Aug 2011 00:22:21 GMT",
    "summary": "[Qt][Texmap][REGRESSION]  http://webkit.org/blog-files/transform-style.html doesn't show composited content",
    "author": "noam.rosenthal@nokia.com",
    "reviewer": "Benjamin Poulain",
    "bugID": 65629,
    "revertedRevision": undefined
}, {
    "revision": 92259,
    "title": "Revision 92259: Unreviewed, rolling out r92256.\nhttp://trac.webkit.org/changeset/92256 ...",
    "time": "Wed, 03 Aug 2011 04:26:52 GMT",
    "summary": "Unreviewed, rolling out r92256.",
    "author": "Sheriff Bot",
    "reviewer": null,
    "bugID": 65593,
    "revertedRevision": 92256
}];

test("changesetURL", 1, function() {
    equals(trac.changesetURL(1234), "http://trac.webkit.org/changeset/1234");
});

test("logURL", 4, function() {
    equals(trac.logURL('trunk', 1234, 1236, false, false), "http://trac.webkit.org/log/trunk?rev=1236&stop_rev=1234&limit=4");
    equals(trac.logURL('trunk', 1234, 1234, true, false), "http://trac.webkit.org/log/trunk?rev=1234&stop_rev=1234&limit=2&verbose=on");
    equals(trac.logURL('trunk', 1236, 1236, false, true), "http://trac.webkit.org/log/trunk?rev=1236&stop_rev=1236&limit=2&format=rss");
    equals(trac.logURL('trunk', 1234, 1236, true, true), "http://trac.webkit.org/log/trunk?rev=1236&stop_rev=1234&limit=4&verbose=on&format=rss");
});

test("recentCommitData", 3, function() {
    var simulator = new NetworkSimulator();
    simulator.get = function(url, callback)
    {
        equals(url, 'http://trac.webkit.org/log/trunk?verbose=on&format=rss&limit=10');
        simulator.scheduleCallback(function() {
            var parser = new DOMParser();
            var responseDOM = parser.parseFromString(kExampleCommitDataXML, "application/xml");
            callback(responseDOM);
        });
    };

    simulator.runTest(function() {
        trac.recentCommitData('trunk', 10, function(commitDataList) {
            $.each(commitDataList, function(index, commitData) {
                // Including the entire message makes the deepEqual below to unwieldy.
                delete commitData.message;
            });
            deepEqual(commitDataList, kExampleCommitDataList);
        });
    });
});

test("commitDataForRevisionRange", 3, function() {
    var simulator = new NetworkSimulator();
    simulator.get = function(url, callback)
    {
        equals(url, 'http://trac.webkit.org/log/trunk?rev=12365&stop_rev=12345&limit=22&verbose=on&format=rss');
        simulator.scheduleCallback(function() {
            var parser = new DOMParser();
            var responseDOM = parser.parseFromString(kExampleCommitDataXML, "application/xml");
            callback(responseDOM);
        });
    };

    simulator.runTest(function() {
        trac.commitDataForRevisionRange('trunk', 12345, 12365, function(commitDataList) {
            $.each(commitDataList, function(index, commitData) {
                // Including the entire message makes the deepEqual below to unwieldy.
                delete commitData.message;
            });
            deepEqual(commitDataList, kExampleCommitDataList);
        });
    });
});

})();
