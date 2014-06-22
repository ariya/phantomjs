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

module("ui");

var kExampleResultsByTest = {
    "scrollbars/custom-scrollbar-with-incomplete-style.html": {
        "Mock Builder": {
            "expected": "IMAGE",
            "actual": "CRASH"
        },
        "Mock Linux": {
            "expected": "TEXT",
            "actual": "CRASH"
        }
    },
    "userscripts/another-test.html": {
        "Mock Builder": {
            "expected": "PASS",
            "actual": "TEXT"
        }
    }
}

test("ui.onebar", 3, function() {
    if (window.location.hash) {
        window.location.hash = '';
    }

    onebar = new ui.onebar();
    onebar.attach();
    equal(onebar.innerHTML,
        '<div><select id="platform-picker"><option>Apple</option><option>Chromium</option><option>GTK</option><option>Qt</option></select></div>' +
        '<ul class="ui-tabs-nav ui-helper-reset ui-helper-clearfix ui-widget-header ui-corner-all">' +
            '<li class="ui-state-default ui-corner-top ui-tabs-selected ui-state-active"><a href="#unexpected">Unexpected Failures</a></li>' +
            '<li class="ui-state-default ui-corner-top"><a href="#expected">Expected Failures</a></li>' +
            '<li class="ui-state-default ui-corner-top ui-state-disabled"><a href="#results">Results</a></li>' +
            '<li class="ui-state-default ui-corner-top"><a href="#perf">perf</a></li>' +
        '</ul>' +
        '<div id="unexpected" class="ui-tabs-panel ui-widget-content ui-corner-bottom"></div>' +
        '<div id="expected" class="ui-tabs-panel ui-widget-content ui-corner-bottom ui-tabs-hide"></div>' +
        '<div id="results" class="ui-tabs-panel ui-widget-content ui-corner-bottom ui-tabs-hide"></div>' +
        '<div id="perf" class="ui-tabs-panel ui-widget-content ui-corner-bottom ui-tabs-hide"></div>');

    onebar.select('expected');
    equal(window.location.hash, '#expected');
    onebar.select('unexpected');
    equal(window.location.hash, '#unexpected');

    $(onebar).detach();
});

test("results.ResultsGrid", 1, function() {
    var grid = new ui.results.ResultsGrid()
    grid.addResults([
        'http://example.com/layout-test-results/foo-bar-diff.txt',
        'http://example.com/layout-test-results/foo-bar-expected.png',
        'http://example.com/layout-test-results/foo-bar-actual.png',
        'http://example.com/layout-test-results/foo-bar-diff.png',
    ]);
    equal(grid.innerHTML,
        '<table class="comparison">' +
            '<thead>' +
                '<tr>' +
                    '<th>Expected</th>' +
                    '<th>Actual</th>' +
                    '<th>Diff</th>' +
                '</tr>' +
            '</thead>' +
            '<tbody>' +
                '<tr>' +
                    '<td class="expected result-container"><img class="image-result" src="http://example.com/layout-test-results/foo-bar-expected.png"></td>' +
                    '<td class="actual result-container"><img class="image-result" src="http://example.com/layout-test-results/foo-bar-actual.png"></td>' +
                    '<td class="diff result-container"><img class="image-result" src="http://example.com/layout-test-results/foo-bar-diff.png"></td>' +
                '</tr>' +
            '</tbody>' +
        '</table>' +
        '<table class="comparison">' +
            '<thead>' +
                '<tr>' +
                    '<th>Expected</th>' +
                    '<th>Actual</th>' +
                    '<th>Diff</th>' +
                '</tr>' +
            '</thead>' +
            '<tbody>' +
                '<tr>' +
                    '<td class="expected result-container"></td>' +
                    '<td class="actual result-container"></td>' +
                    '<td class="diff result-container"><iframe class="text-result" src="http://example.com/layout-test-results/foo-bar-diff.txt"></iframe></td>' +
                '</tr>' +
            '</tbody>' +
        '</table>');
});

test("results.ResultsGrid (crashlog)", 1, function() {
    var grid = new ui.results.ResultsGrid()
    grid.addResults(['http://example.com/layout-test-results/foo-bar-crash-log.txt']);
    equal(grid.innerHTML, '<iframe class="text-result" src="http://example.com/layout-test-results/foo-bar-crash-log.txt"></iframe>');
});

test("results.ResultsGrid (empty)", 1, function() {
    var grid = new ui.results.ResultsGrid()
    grid.addResults([]);
    equal(grid.innerHTML, 'No results to display.');
});

test("time", 6, function() {
    var time = new ui.RelativeTime();
    equal(time.tagName, 'TIME');
    equal(time.className, 'relative');
    deepEqual(Object.getOwnPropertyNames(time.__proto__).sort(), [
        'date',
        'init',
        'setDate',
        'update',
    ]);
    equal(time.outerHTML, '<time class="relative"></time>');
    var tenMinutesAgo = new Date();
    tenMinutesAgo.setMinutes(tenMinutesAgo.getMinutes() - 10);
    time.setDate(tenMinutesAgo);
    equal(time.outerHTML, '<time class="relative">10 minutes ago</time>');
    equal(time.date().getTime(), tenMinutesAgo.getTime());
});

test("StatusArea", 3, function() {
    var statusArea = new ui.StatusArea();
    var id = statusArea.newId();
    statusArea.addMessage(id, 'First Message');
    statusArea.addMessage(id, 'Second Message');
    equal(statusArea.outerHTML,
        '<div class="status processing" style="visibility: visible; ">' +
            '<ul class="actions"><li><button class="action">Close</button></li></ul>' +
            '<progress class="process-text">Processing...</progress>' +
            '<div id="status-content-1" class="status-content">' +
                '<div class="message">First Message</div>' +
                '<div class="message">Second Message</div>' +
            '</div>' +
        '</div>');

    var secondStatusArea = new ui.StatusArea();
    var secondId = secondStatusArea.newId();
    secondStatusArea.addMessage(secondId, 'First Message second id');

    equal(statusArea.outerHTML,
        '<div class="status processing" style="visibility: visible; ">' +
            '<ul class="actions"><li><button class="action">Close</button></li></ul>' +
            '<progress class="process-text">Processing...</progress>' +
            '<div id="status-content-1" class="status-content">' +
                '<div class="message">First Message</div>' +
                '<div class="message">Second Message</div>' +
            '</div>' +
            '<div id="status-content-2" class="status-content">' +
                '<div class="message">First Message second id</div>' +
            '</div>' +
        '</div>');

    statusArea.addFinalMessage(id, 'Final Message 1');
    statusArea.addFinalMessage(secondId, 'Final Message 2');

    equal(statusArea.outerHTML,
        '<div class="status" style="visibility: visible; ">' +
            '<ul class="actions"><li><button class="action">Close</button></li></ul>' +
            '<progress class="process-text">Processing...</progress>' +
            '<div id="status-content-1" class="status-content">' +
                '<div class="message">First Message</div>' +
                '<div class="message">Second Message</div>' +
                '<div class="message">Final Message 1</div>' +
            '</div>' +
            '<div id="status-content-2" class="status-content">' +
                '<div class="message">First Message second id</div>' +
                '<div class="message">Final Message 2</div>' +
            '</div>' +
        '</div>');

    statusArea.close();
});

})();
