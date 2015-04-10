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

module('ui.notifications');

test('Notification', 5, function() {
    var notification = new ui.notifications.Notification();
    equal(notification.tagName, 'LI');
    equal(notification.innerHTML, '<div class="how"></div><div class="what"></div>');
    equal(notification.index(), 0);
    notification.setIndex(1);
    equal(notification.index(), 1);
    // FIXME: Really need to figure out how to mock/test animated removal.
    ok(notification.dismiss);
});

test('Stream', 11, function() {
    var stream = new ui.notifications.Stream();
    equal(stream.tagName, 'OL');
    equal(stream.className, 'notifications');
    equal(stream.childElementCount, 0);

    var notification;

    notification = new ui.notifications.Info('-o-matic');
    notification.setIndex(2);
    stream.add(notification);
    equal(stream.childElementCount, 1);
    equal(stream.textContent, '-o-matic');

    notification = new ui.notifications.Info('garden');
    notification.setIndex(3);
    stream.add(notification);
    equal(stream.childElementCount, 2);
    equal(stream.textContent, 'garden-o-matic');

    notification = new ui.notifications.Info(' is ');
    notification.setIndex(1);
    stream.add(notification);
    equal(stream.childElementCount, 3);
    equal(stream.textContent, 'garden-o-matic is ');

    notification = new ui.notifications.Info('awesome!');
    stream.add(notification);
    equal(stream.childElementCount, 4);
    equal(stream.textContent, 'garden-o-matic is awesome!');
});

test('Info', 2, function() {
    var info = new ui.notifications.Info('info');
    equal(info.tagName, 'LI');
    equal(info.innerHTML, '<div class="how"></div><div class="what">info</div>');
});

test('FailingTestGroup', 2, function() {
    var failingTest = new ui.notifications.FailingTestGroup('test', ['test.html']);
    equal(failingTest.tagName, 'LI');
    equal(failingTest.innerHTML, '<a href="http://webkit-test-results.appspot.com/dashboards/flakiness_dashboard.html#tests=test.html" target="_blank">test</a>');
});

test('SuspiciousCommit', 2, function() {
    var suspiciousCommit = new ui.notifications.SuspiciousCommit({revision: 1, summary: "summary", author: "author", reviewer: "reviewer"});
    equal(suspiciousCommit.tagName, 'LI');
    equal(suspiciousCommit.innerHTML,
        '<div class="description">' +
            '<a href="http://trac.webkit.org/changeset/1" target="_blank">1</a>' +
            '<span>' +
                '<span class="summary">summary</span>' +
                '<span class="author">author</span>' +
                '<span class="reviewer">reviewer</span>' +
            '</span>' +
        '</div>');
});

test('FailingTestsSummary', 12, function() {
    var testFailures = new ui.notifications.FailingTestsSummary();
    equal(testFailures.tagName, 'LI');
    equal(testFailures.innerHTML,
        '<div class="how">' +
            '<time class="relative"></time>' +
            '<table class="failures">' +
                '<thead><tr><td>type</td><td>release</td><td>debug</td></tr></thead>' +
                '<tbody><tr class="BUILDING" style="display: none; "><td><span>BUILDING</span></td><td></td><td></td></tr></tbody>' +
            '</table>' +
        '</div>' +
        '<div class="what">' +
            '<div class="problem">' +
                '<ul class="effects"></ul>' +
                '<ul class="actions">' +
                    '<li><button class="action default" title="Examine these failures in detail.">Examine</button></li>' +
                    '<li><button class="action">Rebaseline</button></li>' +
                '</ul>' +
            '</div>' +
            '<ul class="causes"></ul>' +
        '</div>');
    testFailures.addFailureAnalysis({testName: 'test', resultNodesByBuilder: {}});
    equal(testFailures.index(), 0);
    equal(testFailures.innerHTML,
        '<div class="how">' +
            '<time class="relative"></time>' +
            '<table class="failures">' +
                '<thead><tr><td>type</td><td>release</td><td>debug</td></tr></thead>' +
                '<tbody><tr class="BUILDING" style="display: none; "><td><span>BUILDING</span></td><td></td><td></td></tr></tbody>' +
            '</table>' +
        '</div>' +
        '<div class="what">' +
            '<div class="problem">' +
                '<ul class="effects">' +
                    '<li><a href="http://webkit-test-results.appspot.com/dashboards/flakiness_dashboard.html#tests=test" target="_blank">test</a></li>' +
                '</ul>' +
                '<ul class="actions">' +
                    '<li><button class="action default" title="Examine these failures in detail.">Examine</button></li>' +
                    '<li><button class="action">Rebaseline</button></li>' +
                '</ul>' +
            '</div>' +
            '<ul class="causes"></ul>' +
        '</div>');
    ok(testFailures.containsFailureAnalysis({testName: 'test'}));
    ok(!testFailures.containsFailureAnalysis({testName: 'foo'}));
    testFailures.addFailureAnalysis({testName: 'test'});
    equal(testFailures.innerHTML,
        '<div class="how">' +
            '<time class="relative"></time>' +
            '<table class="failures">' +
                '<thead><tr><td>type</td><td>release</td><td>debug</td></tr></thead>' +
                '<tbody><tr class="BUILDING" style="display: none; "><td><span>BUILDING</span></td><td></td><td></td></tr></tbody>' +
            '</table>' +
        '</div>' +
        '<div class="what">' +
            '<div class="problem">' +
                '<ul class="effects">' +
                    '<li><a href="http://webkit-test-results.appspot.com/dashboards/flakiness_dashboard.html#tests=test" target="_blank">test</a></li>' +
                '</ul>' +
                '<ul class="actions">' +
                    '<li><button class="action default" title="Examine these failures in detail.">Examine</button></li>' +
                    '<li><button class="action">Rebaseline</button></li>' +
                '</ul>' +
            '</div>' +
            '<ul class="causes"></ul>' +
        '</div>');
    deepEqual(testFailures.testNameList(), ['test']);
    var time = new Date();
    time.setMinutes(time.getMinutes() - 10);
    testFailures.addCommitData({revision: 1, time: time, summary: "summary", author: "author", reviewer: "reviewer"});
    equal(testFailures.index(), time.getTime());
    equal(testFailures.innerHTML,
        '<div class="how">' +
            '<time class="relative">10 minutes ago</time>' +
            '<table class="failures">' +
                '<thead><tr><td>type</td><td>release</td><td>debug</td></tr></thead>' +
                '<tbody><tr class="BUILDING" style="display: none; "><td><span>BUILDING</span></td><td></td><td></td></tr></tbody>' +
            '</table>' +
        '</div>' +
        '<div class="what">' +
            '<div class="problem">' +
                '<ul class="effects">' +
                    '<li><a href="http://webkit-test-results.appspot.com/dashboards/flakiness_dashboard.html#tests=test" target="_blank">test</a></li>' +
                '</ul>' +
                '<ul class="actions">' +
                    '<li><button class="action default" title="Examine these failures in detail.">Examine</button></li>' +
                    '<li><button class="action">Rebaseline</button></li>' +
                '</ul>' +
            '</div>' +
            '<ul class="causes">' +
                '<li>' +
                    '<div class="description">' +
                        '<a href="http://trac.webkit.org/changeset/1" target="_blank">1</a>' +
                        '<span>' +
                            '<span class="summary">summary</span>' +
                            '<span class="author">author</span>' +
                            '<span class="reviewer">reviewer</span>' +
                        '</span>' +
                    '</div>' +
                '</li>' +
            '</ul>' +
        '</div>');

    testFailures.addFailureAnalysis({testName: 'foo', resultNodesByBuilder: {'Webkit Linux (dbg)': { actual: 'TEXT'}}});
    equal(testFailures.innerHTML,
        '<div class="how">' +
            '<time class="relative">10 minutes ago</time>' +
            '<table class="failures">' +
                '<thead><tr><td>type</td><td>release</td><td>debug</td></tr></thead>' +
                '<tbody>' +
                    '<tr class="TEXT">' +
                        '<td><span>TEXT</span></td>' +
                        '<td></td>' +
                        '<td><a class="failing-builder" target="_blank" href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Webkit+Linux+(dbg)"><span class="version">lucid</span><span class="architecture">64-bit</span></a></td>' +
                    '</tr>' +
                    '<tr class="BUILDING" style="display: none; "><td><span>BUILDING</span></td><td></td><td></td></tr>' +
                '</tbody>' +
            '</table>' +
        '</div>' +
        '<div class="what">' +
            '<div class="problem">' +
                '<ul class="effects">' +
                    '<li><a href="http://webkit-test-results.appspot.com/dashboards/flakiness_dashboard.html#tests=foo" target="_blank">foo</a></li>' +
                    '<li><a href="http://webkit-test-results.appspot.com/dashboards/flakiness_dashboard.html#tests=test" target="_blank">test</a></li>' +
                '</ul>' +
                '<ul class="actions">' +
                    '<li><button class="action default" title="Examine these failures in detail.">Examine</button></li>' +
                    '<li><button class="action">Rebaseline</button></li>' +
                '</ul>' +
            '</div>' +
            '<ul class="causes">' +
                '<li>' +
                    '<div class="description">' +
                        '<a href="http://trac.webkit.org/changeset/1" target="_blank">1</a>' +
                        '<span>' +
                            '<span class="summary">summary</span>' +
                            '<span class="author">author</span>' +
                            '<span class="reviewer">reviewer</span>' +
                        '</span>' +
                    '</div>' +
                '</li>' +
            '</ul>' +
        '</div>');

    testFailures.updateBuilderResults({'Webkit Mac10.6': { actual: 'BUILDING'}});
    equal(testFailures.innerHTML,
        '<div class="how">' +
            '<time class="relative">10 minutes ago</time>' +
            '<table class="failures">' +
                '<thead><tr><td>type</td><td>release</td><td>debug</td></tr></thead>' +
                '<tbody>' +
                    '<tr class="TEXT">' +
                        '<td><span>TEXT</span></td>' +
                        '<td></td>' +
                        '<td><a class="failing-builder" target="_blank" href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Webkit+Linux+(dbg)"><span class="version">lucid</span><span class="architecture">64-bit</span></a></td>' +
                    '</tr>' +
                    '<tr class="BUILDING" style="">' +
                        '<td><span>BUILDING</span></td>' +
                        '<td><a class="failing-builder" target="_blank" href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Webkit+Mac10.6"><span class="version">snowleopard</span></a></td>' +
                        '<td></td>' +
                    '</tr>' +
                '</tbody>' +
            '</table>' +
        '</div>' +
        '<div class="what">' +
            '<div class="problem">' +
                '<ul class="effects">' +
                    '<li><a href="http://webkit-test-results.appspot.com/dashboards/flakiness_dashboard.html#tests=foo" target="_blank">foo</a></li>' +
                    '<li><a href="http://webkit-test-results.appspot.com/dashboards/flakiness_dashboard.html#tests=test" target="_blank">test</a></li>' +
                '</ul>' +
                '<ul class="actions">' +
                    '<li><button class="action default" title="Examine these failures in detail.">Examine</button></li>' +
                    '<li><button class="action">Rebaseline</button></li>' +
                '</ul>' +
            '</div>' +
            '<ul class="causes">' +
                '<li>' +
                    '<div class="description">' +
                        '<a href="http://trac.webkit.org/changeset/1" target="_blank">1</a>' +
                        '<span>' +
                            '<span class="summary">summary</span>' +
                            '<span class="author">author</span>' +
                            '<span class="reviewer">reviewer</span>' +
                        '</span>' +
                    '</div>' +
                '</li>' +
            '</ul>' +
        '</div>');
});

test('FailingTestsSummary (grouping)', 1, function() {
    var testFailures = new ui.notifications.FailingTestsSummary();
    testFailures.addFailureAnalysis({testName: 'path/to/test1.html', resultNodesByBuilder: {}});
    testFailures.addFailureAnalysis({testName: 'path/to/test2.html', resultNodesByBuilder: {}});
    testFailures.addFailureAnalysis({testName: 'path/to/test3.html', resultNodesByBuilder: {}});
    testFailures.addFailureAnalysis({testName: 'path/to/test4.html', resultNodesByBuilder: {}});
    testFailures.addFailureAnalysis({testName: 'path/another/test.html', resultNodesByBuilder: {}});
    equal(testFailures.innerHTML,
        '<div class="how">' +
            '<time class="relative"></time>' +
            '<table class="failures">' +
                '<thead><tr><td>type</td><td>release</td><td>debug</td></tr></thead>' +
                '<tbody><tr class="BUILDING" style="display: none; "><td><span>BUILDING</span></td><td></td><td></td></tr></tbody>' +
            '</table>' +
        '</div>' +
        '<div class="what">' +
            '<div class="problem">' +
                '<ul class="effects">' +
                    '<li><a href="http://webkit-test-results.appspot.com/dashboards/flakiness_dashboard.html#tests=path%2Fto%2Ftest1.html%2Cpath%2Fto%2Ftest2.html%2Cpath%2Fto%2Ftest3.html%2Cpath%2Fto%2Ftest4.html" target="_blank">path/to (4 tests)</a></li>' +
                    '<li><a href="http://webkit-test-results.appspot.com/dashboards/flakiness_dashboard.html#tests=path%2Fanother%2Ftest.html" target="_blank">path/another/test.html</a></li>' +
                '</ul>' +
                '<ul class="actions">' +
                    '<li><button class="action default" title="Examine these failures in detail.">Examine</button></li>' +
                    '<li><button class="action">Rebaseline</button></li>' +
                '</ul>' +
            '</div>' +
            '<ul class="causes"></ul>' +
        '</div>');
});

test('BuildersFailing', 1, function() {
    var builderFailing = new ui.notifications.BuildersFailing('Disasterifying');
    builderFailing.setFailingBuilders({'Webkit Linux': ['compile'], 'Webkit Win7': ['webkit_tests', 'update']});
    equal(builderFailing.innerHTML,
        '<div class="how">' +
            '<time class="relative"></time>' +
        '</div>' +
        '<div class="what">' +
            '<div class="problem">Disasterifying:' +
                '<ul class="effects">' +
                    '<li class="builder"><a class="failing-builder" target="_blank" href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Webkit+Linux"><span class="version">lucid</span><span class="architecture">64-bit</span><span class="failures"> compile</span></a></li>' +
                    '<li class="builder"><a class="failing-builder" target="_blank" href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Webkit+Win7"><span class="version">win7</span><span class="failures"> webkit_tests, update</span></a></li>' +
                '</ul>' +
            '</div>' +
            '<ul class="causes"></ul>' +
        '</div>');
});

}());
