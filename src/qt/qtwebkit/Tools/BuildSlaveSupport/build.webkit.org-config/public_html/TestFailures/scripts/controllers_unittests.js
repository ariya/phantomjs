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

var kExampleResultsByBuilder = {
    "Mock Builder": unittest.kExampleResultsJSON,
};

module("controllers");

test("UnexpectedFailures", 3, function() {
    var simulator = new NetworkSimulator();

    simulator.probe = function() {
    };

    simulator.runTest(function() {
        var mockView = {};
        var mockState = {
            resultsByBuilder: kExampleResultsByBuilder
        };
        var expectedResultsByTest = null;
        var mockDelegate = {
            showResults: function(resultsView)
            {
                deepEqual(resultsView._resultsByTest, expectedResultsByTest);
            }
        }
        var controller = controllers.UnexpectedFailures(mockState, mockView, mockDelegate);

        var testNameList = null;
        var mockFailures = {
            testNameList: function() { return testNameList; }
        };

        testNameList = ["scrollbars/custom-scrollbar-with-incomplete-style.html"];
        expectedResultsByTest = {};
        controller.onExamine(mockFailures);

        testNameList = ["userscripts/another-test.html"];
        expectedResultsByTest = {
          "userscripts/another-test.html": {
            "Mock Builder": {
              "expected": "PASS",
              "actual": "TEXT"
            }
          }
        };
        controller.onExamine(mockFailures);
    });
});

test("controllers.FailingBuilders", 3, function() {
    var MockView = base.extends('div', {
        add: function(node) {
            this.appendChild(node);
        }
    })
    var view = new MockView();
    var failingBuilders = new controllers.FailingBuilders(view, 'dummy message');
    ok(!failingBuilders.hasFailures());

    failingBuilders.update({'DummyBuilder': ['webkit_tests']});
    ok(failingBuilders.hasFailures());

    equal(view.outerHTML, '<div>' +
        '<li style="opacity: 0; ">' +
            '<div class="how"><time class="relative"></time></div>' +
            '<div class="what">' +
                '<div class="problem">dummy message:' +
                    '<ul class="effects">' +
                        '<li class="builder"><a class="failing-builder" target="_blank" href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=DummyBuilder">' +
                            '<span class="version">DummyBuilder</span><span class="failures"> webkit_tests</span></a>' +
                        '</li>' +
                    '</ul>' +
                '</div>' +
                '<ul class="causes"></ul>' +
            '</div>' +
        '</li>' +
    '</div>')
});

})();
