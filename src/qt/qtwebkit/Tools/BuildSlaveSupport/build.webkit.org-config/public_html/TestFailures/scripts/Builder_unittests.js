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

(function() {

module("Builder");

function runGetNumberOfFailingTestsTest(jsonData, callback) {
    var mockBuildbot = {};
    mockBuildbot.baseURL = 'http://example.com/';

    var builder = new Builder('test builder', mockBuildbot);

    var realGetResource = window.getResource;
    window.getResource = function(url, successCallback, errorCallback) {
        var mockXHR = {};
        mockXHR.responseText = JSON.stringify(jsonData);

        // FIXME: It would be better for this callback to happen
        // asynchronously, to match what happens for real requests.
        successCallback(mockXHR);
    };

    // FIXME: It's lame to be modifying singletons like this. We should get rid
    // of our singleton usage entirely!
    ok(!("PersistentCache" in window));
    window.PersistentCache = {
        contains: function() { return false; },
        set: function() { },
        get: function() { },
    };

    builder.getNumberOfFailingTests(1, callback);

    window.getResource = realGetResource;
    equal(window.getResource, realGetResource);
    delete window.PersistentCache;
}

test("getNumberOfFailingTests shouldn't include leaks", 4, function() {
    const jsonData = {
        steps: [
            {
                isFinished: true,
                isStarted: true,
                name: 'layout-test',
                results: [
                    2,
                    [
                        "2178 total leaks found!", 
                        "2 test cases (<1%) had incorrect layout", 
                        "2 test cases (<1%) crashed",
                    ],
                ],
                times: [
                    1310599204.1231229,
                    1310600152.973659,
                ]
            },
        ],
    };

    runGetNumberOfFailingTestsTest(jsonData, function(failureCount, tooManyFailures) {
        equal(failureCount, 4);
        equal(tooManyFailures, false);
    });
});

test("getNumberOfFailingTests detects spurious run-webkit-tests failures", 4, function() {
    const jsonData = {
        steps: [
            {
                isFinished: true, 
                isStarted: true, 
                name: "layout-test", 
                results: [
                    2, 
                    [
                        "layout-test"
                    ]
                ], 
                step_number: 7, 
                text: [
                    "layout-test"
                ], 
                times: [
                    1310437736.00494, 
                    1310440118.038023
                ],
            }, 
        ],
    };

    runGetNumberOfFailingTestsTest(jsonData, function(failureCount, tooManyFailures) {
        equal(failureCount, -1);
        equal(tooManyFailures, false);
    });
});

test("getNumberOfFailingTests understands NRWT exiting early due to too many failures", 4, function() {
    const jsonData = {
        steps: [
            {
                isFinished: true, 
                isStarted: true, 
                name: "layout-test", 
                results: [
                  2, 
                  [
                      "2011-07-13 04:38:46,315 11247 manager.py:780 WARNING Exiting early after 20 crashes and 0 timeouts. 2251 tests run.", 
                      "20 failed"
                  ]
                ], 
                step_number: 4, 
                text: [
                    "2011-07-13 04:38:46,315 11247 manager.py:780 WARNING Exiting early after 20 crashes and 0 timeouts. 2251 tests run.", 
                    "20 failed"
                ], 
                times: [
                    1310557115.793082, 
                    1310557129.832104
                ]
            }, 
        ],
    };

    runGetNumberOfFailingTestsTest(jsonData, function(failureCount, tooManyFailures) {
        equal(failureCount, 20);
        equal(tooManyFailures, true);
    });
});

test("getNumberOfFailingTests treats build step interruptions as errors", 4, function() {
    const jsonData = {
        steps: [
            {
                isFinished: true,
                isStarted: true,
                name: "layout-test",
                results: [
                  4,
                  [
                      "interrupted",
                  ]
                ],
                step_number: 5,
                text: [
                    "layout-test",
                    "interrupted",
                ],
                times: [
                    1310599204.1231229,
                    1310600152.973659,
                ]
            },
        ],
    };

    runGetNumberOfFailingTestsTest(jsonData, function(failureCount, tooManyFailures) {
        equal(failureCount, -1);
        equal(tooManyFailures, false);
    });
});

test("getNumberOfFailingTests treats unfinished test runs as errors", 4, function() {
    const jsonData = {
        steps: [
            {
                isStarted: true, 
                name: "layout-test", 
                step_number: 5, 
                text: [
                    "layout-tests running"
                ], 
                times: [
                    1312989295.518481, 
                    null
                ]
            }, 
        ],
    };

    runGetNumberOfFailingTestsTest(jsonData, function(failureCount, tooManyFailures) {
        equal(failureCount, -1);
        equal(tooManyFailures, false);
    });
});

test("getNumberOfFailingTests treats successful but unbelievably short test runs as errors", 4, function() {
    const jsonData = {
        steps: [
            {
                isFinished: true, 
                isStarted: true, 
                name: "layout-test", 
                step_number: 7, 
                text: [
                    "layout-test"
                ], 
                times: [
                    1311288797.7207019, 
                    1311288802.7791941
                ]
            }, 
        ],
    };

    runGetNumberOfFailingTestsTest(jsonData, function(failureCount, tooManyFailures) {
        equal(failureCount, -1);
        equal(tooManyFailures, false);
    });
});

test("getNumberOfFailingTests doesn't care if a failing run is unbelievably short", 4, function() {
    const jsonData = {
        steps: [
            {
                isFinished: true, 
                isStarted: true, 
                name: "layout-test", 
                results: [
                  2, 
                  [
                      "2011-07-13 04:38:46,315 11247 manager.py:780 WARNING Exiting early after 20 crashes and 0 timeouts. 2251 tests run.", 
                      "20 failed"
                  ]
                ], 
                step_number: 4, 
                text: [
                    "2011-07-13 04:38:46,315 11247 manager.py:780 WARNING Exiting early after 20 crashes and 0 timeouts. 2251 tests run.", 
                    "20 failed"
                ], 
                times: [
                    1310557115.793082, 
                    1310557119.832104
                ]
            }, 
        ],
    };

    runGetNumberOfFailingTestsTest(jsonData, function(failureCount, tooManyFailures) {
        equal(failureCount, 20);
        equal(tooManyFailures, true);
    });
});

})();
