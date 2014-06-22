/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

module('ui.perf');

var kDummyPerfGraphData = {
    "dummy_perf_test_1": [
        {
            "builder": "Mac10.6 Perf",
            "url": "http://dummyurl1"
        }
    ],
    "dummy_perf_test_2": [
        {
            "builder": "Mac10.6 Perf",
            "url": "http://dummyurl2"
        },
        {
            "builder": "Linux Perf",
            "url": "http://dummyurl3"
        }
    ]
};

asyncTest("View", 7, function() {
    var oldPerfBuilders = builders.perfBuilders;
    builders.perfBuilders = function(callback) {
        setTimeout(function() { callback(kDummyPerfGraphData) }, 0);
    }

    var view = new ui.perf.View();

    setTimeout(function() {
        equal(view.outerHTML, '<div id="perf-view">' +
            '<ol class="notifications">' +
                '<li style="opacity: 0; ">' +
                    '<div class="how"></div><div class="what">Loading list of perf dashboards...</div>' +
                '</li>' +
            '</ol>' +
            '<div class="title-bar">' +
                '<select><option>dummy_perf_test_1</option><option>dummy_perf_test_2</option></select>' +
                '<ul class="actions"><li><button class="action previous">\u25C0</button></li><li><button class="action next">\u25B6</button></li></ul>' +
                '<select><option value="http://dummyurl1">Mac10.6 Perf</option></select>' +
                '<a class="pop-out" target="_blank" href="http://dummyurl1">Pop out</a>' +
            '</div>' +
            '<iframe src="http://dummyurl1"></iframe>' +
        '</div>');

        view._nextGraph();

        equal(view.outerHTML, '<div id="perf-view">' +
            '<ol class="notifications">' +
                '<li style="opacity: 0; ">' +
                    '<div class="how"></div><div class="what">Loading list of perf dashboards...</div>' +
                '</li>' +
            '</ol>' +
            '<div class="title-bar">' +
                '<select><option>dummy_perf_test_1</option><option>dummy_perf_test_2</option></select>' +
                '<ul class="actions"><li><button class="action previous">\u25C0</button></li><li><button class="action next">\u25B6</button></li></ul>' +
                '<a class="pop-out" target="_blank" href="http://dummyurl2">Pop out</a>' +
                '<select><option value="http://dummyurl2">Mac10.6 Perf</option><option value="http://dummyurl3">Linux Perf</option></select>' +
            '</div>' +
            '<iframe src="http://dummyurl2"></iframe>' +
        '</div>');

        view._nextGraph();

        equal(view.outerHTML, '<div id="perf-view">' +
            '<ol class="notifications">' +
                '<li style="opacity: 0; ">' +
                    '<div class="how"></div><div class="what">Loading list of perf dashboards...</div>' +
                '</li>' +
            '</ol>' +
            '<div class="title-bar">' +
                '<select><option>dummy_perf_test_1</option><option>dummy_perf_test_2</option></select>' +
                '<ul class="actions"><li><button class="action previous">\u25C0</button></li><li><button class="action next">\u25B6</button></li></ul>' +
                '<a class="pop-out" target="_blank" href="http://dummyurl3">Pop out</a>' +
                '<select><option value="http://dummyurl2">Mac10.6 Perf</option><option value="http://dummyurl3">Linux Perf</option></select>' +
            '</div>' +
            '<iframe src="http://dummyurl3"></iframe>' +
        '</div>');

        view._nextGraph();

        equal(view.outerHTML, '<div id="perf-view">' +
            '<ol class="notifications">' +
                '<li style="opacity: 0; ">' +
                    '<div class="how"></div><div class="what">Loading list of perf dashboards...</div>' +
                '</li>' +
            '</ol>' +
            '<div class="title-bar">' +
                '<select><option>dummy_perf_test_1</option><option>dummy_perf_test_2</option></select>' +
                '<ul class="actions"><li><button class="action previous">\u25C0</button></li><li><button class="action next">\u25B6</button></li></ul>' +
                '<a class="pop-out" target="_blank" href="http://dummyurl3">Pop out</a>' +
                '<select><option value="http://dummyurl2">Mac10.6 Perf</option><option value="http://dummyurl3">Linux Perf</option></select>' +
            '</div>' +
            '<iframe src="http://dummyurl3"></iframe>' +
        '</div>');

        view._previousGraph();

        equal(view.outerHTML, '<div id="perf-view">' +
            '<ol class="notifications">' +
                '<li style="opacity: 0; ">' +
                    '<div class="how"></div><div class="what">Loading list of perf dashboards...</div>' +
                '</li>' +
            '</ol>' +
            '<div class="title-bar">' +
                '<select><option>dummy_perf_test_1</option><option>dummy_perf_test_2</option></select>' +
                '<ul class="actions"><li><button class="action previous">\u25C0</button></li><li><button class="action next">\u25B6</button></li></ul>' +
                '<a class="pop-out" target="_blank" href="http://dummyurl2">Pop out</a>' +
                '<select><option value="http://dummyurl2">Mac10.6 Perf</option><option value="http://dummyurl3">Linux Perf</option></select>' +
            '</div>' +
            '<iframe src="http://dummyurl2"></iframe>' +
        '</div>');

        view._previousGraph();

        equal(view.outerHTML, '<div id="perf-view">' +
            '<ol class="notifications">' +
                '<li style="opacity: 0; ">' +
                    '<div class="how"></div><div class="what">Loading list of perf dashboards...</div>' +
                '</li>' +
            '</ol>' +
            '<div class="title-bar">' +
                '<select><option>dummy_perf_test_1</option><option>dummy_perf_test_2</option></select>' +
                '<ul class="actions"><li><button class="action previous">\u25C0</button></li><li><button class="action next">\u25B6</button></li></ul>' +
                '<a class="pop-out" target="_blank" href="http://dummyurl1">Pop out</a>' +
                '<select><option value="http://dummyurl1">Mac10.6 Perf</option></select>' +
            '</div>' +
            '<iframe src="http://dummyurl1"></iframe>' +
        '</div>');

        view._previousGraph();

        equal(view.outerHTML, '<div id="perf-view">' +
            '<ol class="notifications">' +
                '<li style="opacity: 0; ">' +
                    '<div class="how"></div><div class="what">Loading list of perf dashboards...</div>' +
                '</li>' +
            '</ol>' +
            '<div class="title-bar">' +
                '<select><option>dummy_perf_test_1</option><option>dummy_perf_test_2</option></select>' +
                '<ul class="actions"><li><button class="action previous">\u25C0</button></li><li><button class="action next">\u25B6</button></li></ul>' +
                '<a class="pop-out" target="_blank" href="http://dummyurl1">Pop out</a>' +
                '<select><option value="http://dummyurl1">Mac10.6 Perf</option></select>' +
            '</div>' +
            '<iframe src="http://dummyurl1"></iframe>' +
        '</div>');

        builders.perfBuilders = oldPerfBuilders;
        start();
    }, 0);
});

})();
