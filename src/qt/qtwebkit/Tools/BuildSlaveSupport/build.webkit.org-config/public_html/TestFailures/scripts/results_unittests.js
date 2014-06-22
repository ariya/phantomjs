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

var unittest = unittest || {};

(function () {

module("results");

unittest.kExampleResultsJSON = {
    "tests": {
        "scrollbars": {
            "custom-scrollbar-with-incomplete-style.html": {
                "expected": "IMAGE",
                "actual": "IMAGE"
            },
            "expected-wontfix": {
                "expected": "IMAGE",
                "actual": "IMAGE",
                "wontfix": true
            },
            "unexpected-wontfix": {
                "expected": "IMAGE",
                "actual": "TEXT",
                "wontfix": true
            },
            "flaky-scrollbar.html": {
                "expected": "PASS",
                "actual": "PASS TEXT"
            },
            "unexpected-failing-flaky-scrollbar.html": {
                "expected": "TEXT",
                "actual": "TIMEOUT TEXT"
            },
            "unexpected-pass.html": {
                "expected": "FAIL",
                "actual": "PASS"
            }
        },
        "userscripts": {
            "user-script-video-document.html": {
                "expected": "FAIL",
                "actual": "TEXT"
            },
            "another-test.html": {
                "expected": "PASS",
                "actual": "TEXT"
            }
        },
    },
    "skipped": 339,
    "num_regressions": 14,
    "interrupted": false,
    "layout_tests_dir": "\/mnt\/data\/b\/build\/slave\/Webkit_Linux\/build\/src\/third_party\/WebKit\/LayoutTests",
    "version": 3,
    "num_passes": 15566,
    "has_pretty_patch": false,
    "fixable": 1233,
    "num_flaky":1,
    "uses_expectations_file": true,
    "has_wdiff": true,
    "revision": "90430"
};

test("ResultAnalyzer", 55, function() {
    var analyzer;

    analyzer = new results.ResultAnalyzer({expected: 'PASS', actual: 'TEXT'});
    ok(analyzer.expectedToSucceed());
    ok(analyzer.hasUnexpectedFailures());
    deepEqual(analyzer.unexpectedResults(), ['TEXT']);
    ok(!analyzer.succeeded());
    ok(!analyzer.flaky());

    analyzer = new results.ResultAnalyzer({expected: 'PASS TIMEOUT', actual: 'TEXT'});
    ok(analyzer.expectedToSucceed());
    ok(analyzer.hasUnexpectedFailures());
    deepEqual(analyzer.unexpectedResults(), ['TEXT']);
    ok(!analyzer.succeeded());
    ok(!analyzer.flaky());

    analyzer = new results.ResultAnalyzer({expected: 'TEXT', actual: 'TEXT TIMEOUT'});
    ok(!analyzer.expectedToSucceed());
    ok(analyzer.hasUnexpectedFailures());
    deepEqual(analyzer.unexpectedResults(), ['TIMEOUT']);
    ok(!analyzer.succeeded());
    ok(analyzer.flaky());

    analyzer = new results.ResultAnalyzer({expected: 'PASS', actual: 'TEXT TIMEOUT'});
    ok(analyzer.expectedToSucceed());
    ok(analyzer.hasUnexpectedFailures());
    deepEqual(analyzer.unexpectedResults(), ['TEXT', 'TIMEOUT']);
    ok(!analyzer.succeeded());
    ok(analyzer.flaky());

    analyzer = new results.ResultAnalyzer({expected: 'PASS TIMEOUT', actual: 'PASS TIMEOUT'});
    ok(analyzer.expectedToSucceed());
    ok(!analyzer.hasUnexpectedFailures());
    deepEqual(analyzer.unexpectedResults(), []);
    ok(analyzer.succeeded());
    ok(analyzer.flaky());

    analyzer = new results.ResultAnalyzer({expected: 'PASS TIMEOUT', actual: 'TIMEOUT PASS'});
    ok(analyzer.expectedToSucceed());
    ok(!analyzer.hasUnexpectedFailures());
    deepEqual(analyzer.unexpectedResults(), []);
    ok(analyzer.succeeded());
    ok(analyzer.flaky());

    analyzer = new results.ResultAnalyzer({expected: 'FAIL', actual: 'TIMEOUT'});
    ok(!analyzer.expectedToSucceed());
    ok(analyzer.hasUnexpectedFailures());
    deepEqual(analyzer.unexpectedResults(), ['TIMEOUT']);
    ok(!analyzer.succeeded());
    ok(!analyzer.flaky());

    analyzer = new results.ResultAnalyzer({expected: 'FAIL', actual: 'IMAGE'});
    ok(!analyzer.expectedToSucceed());
    ok(analyzer.hasUnexpectedFailures());
    deepEqual(analyzer.unexpectedResults(), ['IMAGE']);
    ok(!analyzer.succeeded());
    ok(!analyzer.flaky());

    analyzer = new results.ResultAnalyzer({expected: 'FAIL', actual: 'AUDIO'});
    ok(!analyzer.expectedToSucceed());
    ok(!analyzer.hasUnexpectedFailures());
    deepEqual(analyzer.unexpectedResults(), []);
    ok(!analyzer.succeeded());
    ok(!analyzer.flaky());

    analyzer = new results.ResultAnalyzer({expected: 'FAIL', actual: 'TEXT'});
    ok(!analyzer.expectedToSucceed());
    ok(!analyzer.hasUnexpectedFailures());
    deepEqual(analyzer.unexpectedResults(), []);
    ok(!analyzer.succeeded());
    ok(!analyzer.flaky());

    analyzer = new results.ResultAnalyzer({expected: 'FAIL', actual: 'IMAGE+TEXT'});
    ok(!analyzer.expectedToSucceed());
    ok(!analyzer.hasUnexpectedFailures());
    deepEqual(analyzer.unexpectedResults(), []);
    ok(!analyzer.succeeded());
    ok(!analyzer.flaky());
});

test("expectedFailures", 1, function() {
    var expectedFailures = results.expectedFailures(unittest.kExampleResultsJSON);
    deepEqual(expectedFailures, {
        "scrollbars/custom-scrollbar-with-incomplete-style.html": {
            "expected": "IMAGE",
            "actual": "IMAGE"
        },
        "userscripts/user-script-video-document.html": {
            "expected": "FAIL",
            "actual": "TEXT"
        }
    });
});

test("unexpectedFailures", 1, function() {
    var unexpectedFailures = results.unexpectedFailures(unittest.kExampleResultsJSON);
    deepEqual(unexpectedFailures, {
        "userscripts/another-test.html": {
            "expected": "PASS",
            "actual": "TEXT"
        }
    });
});

test("unexpectedFailuresByTest", 1, function() {
    var unexpectedFailuresByTest = results.unexpectedFailuresByTest({
        "Mock Builder": unittest.kExampleResultsJSON
    });
    deepEqual(unexpectedFailuresByTest, {
        "userscripts/another-test.html": {
            "Mock Builder": {
                "expected": "PASS",
                "actual": "TEXT"
            }
        }
    });
});

test("unexpectedSuccessesByTest", 1, function() {
    var unexpectedFailuresByTest = results.unexpectedSuccessesByTest({
        "Mock Builder": unittest.kExampleResultsJSON
    });
    deepEqual(unexpectedFailuresByTest, {
        "scrollbars/unexpected-pass.html": {
            "Mock Builder": {
                "expected": "FAIL",
                "actual": "PASS"
            }
        }
    });
});

test("failureInfoForTestAndBuilder", 1, function() {
    var unexpectedFailuresByTest = results.unexpectedFailuresByTest({
        "Mock Builder": unittest.kExampleResultsJSON
    });
    var failureInfo = results.failureInfoForTestAndBuilder(unexpectedFailuresByTest, "userscripts/another-test.html", "Mock Builder");
    deepEqual(failureInfo, {
        "testName": "userscripts/another-test.html",
        "builderName": "Mock Builder",
        "failureTypeList": ["TEXT"],
    });
});

test("resultKind", 12, function() {
    equals(results.resultKind("http://example.com/foo-actual.txt"), "actual");
    equals(results.resultKind("http://example.com/foo-expected.txt"), "expected");
    equals(results.resultKind("http://example.com/foo-diff.txt"), "diff");
    equals(results.resultKind("http://example.com/foo.bar-actual.txt"), "actual");
    equals(results.resultKind("http://example.com/foo.bar-expected.txt"), "expected");
    equals(results.resultKind("http://example.com/foo.bar-diff.txt"), "diff");
    equals(results.resultKind("http://example.com/foo-actual.png"), "actual");
    equals(results.resultKind("http://example.com/foo-expected.png"), "expected");
    equals(results.resultKind("http://example.com/foo-diff.png"), "diff");
    equals(results.resultKind("http://example.com/foo-pretty-diff.html"), "diff");
    equals(results.resultKind("http://example.com/foo-wdiff.html"), "diff");
    equals(results.resultKind("http://example.com/foo-xyz.html"), "unknown");
});

test("resultType", 12, function() {
    equals(results.resultType("http://example.com/foo-actual.txt"), "text");
    equals(results.resultType("http://example.com/foo-expected.txt"), "text");
    equals(results.resultType("http://example.com/foo-diff.txt"), "text");
    equals(results.resultType("http://example.com/foo.bar-actual.txt"), "text");
    equals(results.resultType("http://example.com/foo.bar-expected.txt"), "text");
    equals(results.resultType("http://example.com/foo.bar-diff.txt"), "text");
    equals(results.resultType("http://example.com/foo-actual.png"), "image");
    equals(results.resultType("http://example.com/foo-expected.png"), "image");
    equals(results.resultType("http://example.com/foo-diff.png"), "image");
    equals(results.resultType("http://example.com/foo-pretty-diff.html"), "text");
    equals(results.resultType("http://example.com/foo-wdiff.html"), "text");
    equals(results.resultType("http://example.com/foo.xyz"), "text");
});

test("resultNodeForTest", 4, function() {
    deepEqual(results.resultNodeForTest(unittest.kExampleResultsJSON, "userscripts/another-test.html"), {
        "expected": "PASS",
        "actual": "TEXT"
    });
    equals(results.resultNodeForTest(unittest.kExampleResultsJSON, "foo.html"), null);
    equals(results.resultNodeForTest(unittest.kExampleResultsJSON, "userscripts/foo.html"), null);
    equals(results.resultNodeForTest(unittest.kExampleResultsJSON, "userscripts/foo/bar.html"), null);
});

test("walkHistory", 6, function() {
    var simulator = new NetworkSimulator();

    var keyMap = {
        "Mock_Builder": {
            "11108": {
                "tests": {
                    "userscripts": {
                        "another-test.html": {
                            "expected": "PASS",
                            "actual": "TEXT"
                        }
                    },
                },
                "revision": "90430"
            },
            "11107":{
                "tests": {
                    "userscripts": {
                        "user-script-video-document.html": {
                            "expected": "FAIL",
                            "actual": "TEXT"
                        },
                        "another-test.html": {
                            "expected": "PASS",
                            "actual": "TEXT"
                        }
                    },
                },
                "revision": "90429"
            },
            "11106":{
                "tests": {
                    "userscripts": {
                        "another-test.html": {
                            "expected": "PASS",
                            "actual": "TEXT"
                        }
                    },
                },
                "revision": "90426"
            },
            "11105":{
                "tests": {
                    "userscripts": {
                        "user-script-video-document.html": {
                            "expected": "FAIL",
                            "actual": "TEXT"
                        },
                    },
                },
                "revision": "90424"
            },
        },
        "Another_Builder": {
            "22202":{
                "tests": {
                    "userscripts": {
                        "another-test.html": {
                            "expected": "PASS",
                            "actual": "TEXT"
                        }
                    },
                },
                "revision": "90426"
            },
            "22201":{
                "tests": {
                },
                "revision": "90425"
            },
        },
    };

    simulator.jsonp = function(url, callback) {
        simulator.scheduleCallback(function() {
            var result = keyMap[/[^/]+_Builder/.exec(url)][/\d+/.exec(url)];
            callback(result ? result : {});
        });
    };

    simulator.get = function(url, callback) {
        simulator.scheduleCallback(function() {
            if (/Mock_Builder/.test(url))
                callback('<a href="11101/"></a><a href="11102/"></a><a href="11103/"></a><a href="11104/"></a><a href="11105/"></a><a href="11106/"></a><a href="11107/"></a><a href="11108/"></a>');
            else if (/Another_Builder/.test(url))
                callback('<a href="22201/"></a><a href="22202/"></a>');
            else
                ok(false, 'Unexpected URL: ' + url);
        });
    };

    simulator.runTest(function() {
        results.regressionRangeForFailure("Mock Builder", "userscripts/another-test.html", function(oldestFailingRevision, newestPassingRevision) {
            equals(oldestFailingRevision, 90426);
            equals(newestPassingRevision, 90424);
        });

        results.unifyRegressionRanges(["Mock Builder", "Another Builder"], "userscripts/another-test.html", function(oldestFailingRevision, newestPassingRevision) {
            equals(oldestFailingRevision, 90426);
            equals(newestPassingRevision, 90425);
        });

        results.countFailureOccurences(["Mock Builder", "Another Builder"], "userscripts/another-test.html", function(failureCount) {
            equals(failureCount, 4);
        });
    });
});

test("walkHistory (no revision)", 3, function() {
    var simulator = new NetworkSimulator();

    var keyMap = {
        "Mock_Builder": {
            "11103": {
                "tests": {
                    "userscripts": {
                        "another-test.html": {
                            "expected": "PASS",
                            "actual": "TEXT"
                        }
                    },
                },
                "revision": ""
            },
            "11102":{
                "tests": {
                },
                "revision": ""
            },
        },
    };

    simulator.jsonp = function(url, callback) {
        simulator.scheduleCallback(function() {
            var result = keyMap[/[^/]+_Builder/.exec(url)][/\d+/.exec(url)];
            callback(result ? result : {});
        });
    };

    simulator.get = function(url, callback) {
        simulator.scheduleCallback(function() {
            callback('<a href="11101/"></a><a href="11102/"></a><a href="11103/"></a>');
        });
    };


    simulator.runTest(function() {
        results.regressionRangeForFailure("Mock Builder", "userscripts/another-test.html", function(oldestFailingRevision, newestPassingRevision) {
            equals(oldestFailingRevision, 0);
            equals(newestPassingRevision, 0);
        });
    });
});

test("collectUnexpectedResults", 1, function() {
    var dictionaryOfResultNodes = {
        "foo": {
            "expected": "IMAGE",
            "actual": "IMAGE"
        },
        "bar": {
            "expected": "PASS",
            "actual": "PASS TEXT"
        },
        "baz": {
            "expected": "TEXT",
            "actual": "IMAGE"
        },
        "qux": {
            "expected": "PASS",
            "actual": "TEXT"
        },
        "taco": {
            "expected": "PASS",
            "actual": "TEXT"
        },
    };

    var collectedResults = results.collectUnexpectedResults(dictionaryOfResultNodes);
    deepEqual(collectedResults, ["TEXT", "IMAGE"]);
});

test("failureTypeToExtensionList", 5, function() {
    deepEqual(results.failureTypeToExtensionList('TEXT'), ['txt']);
    deepEqual(results.failureTypeToExtensionList('IMAGE+TEXT'), ['txt', 'png']);
    deepEqual(results.failureTypeToExtensionList('IMAGE'), ['png']);
    deepEqual(results.failureTypeToExtensionList('CRASH'), []);
    deepEqual(results.failureTypeToExtensionList('TIMEOUT'), []);
});

test("canRebaseline", 6, function() {
    ok(results.canRebaseline(['TEXT']));
    ok(results.canRebaseline(['IMAGE+TEXT', 'CRASH']));
    ok(results.canRebaseline(['IMAGE']));
    ok(!results.canRebaseline(['CRASH']));
    ok(!results.canRebaseline(['TIMEOUT']));
    ok(!results.canRebaseline([]));
});

test("fetchResultsURLs", 5, function() {
    var simulator = new NetworkSimulator();

    var probedURLs = [];
    simulator.probe = function(url, options)
    {
        simulator.scheduleCallback(function() {
            probedURLs.push(url);
            if (base.endsWith(url, '.txt'))
                options.success.call();
            else if (/taco.+png$/.test(url))
                options.success.call();
            else
                options.error.call();
        });
    };

    simulator.runTest(function() {
        results.fetchResultsURLs({
            'builderName': "Mock Builder",
            'testName': "userscripts/another-test.html",
            'failureTypeList': ['IMAGE', 'CRASH'],
        }, function(resultURLs) {
            deepEqual(resultURLs, [
                "http://build.chromium.org/f/chromium/layout_test_results/Mock_Builder/results/layout-test-results/userscripts/another-test-crash-log.txt"
            ]);
        });
        results.fetchResultsURLs({
            'builderName': "Mock Builder",
            'testName': "userscripts/another-test.html",
            'failureTypeList': ['TIMEOUT'],
        }, function(resultURLs) {
            deepEqual(resultURLs, []);
        });
        results.fetchResultsURLs({
            'builderName': "Mock Builder",
            'testName': "userscripts/taco.html",
            'failureTypeList': ['IMAGE', 'IMAGE+TEXT'],
        }, function(resultURLs) {
            deepEqual(resultURLs, [
                "http://build.chromium.org/f/chromium/layout_test_results/Mock_Builder/results/layout-test-results/userscripts/taco-expected.png",
                "http://build.chromium.org/f/chromium/layout_test_results/Mock_Builder/results/layout-test-results/userscripts/taco-actual.png",
                "http://build.chromium.org/f/chromium/layout_test_results/Mock_Builder/results/layout-test-results/userscripts/taco-diff.png",
                "http://build.chromium.org/f/chromium/layout_test_results/Mock_Builder/results/layout-test-results/userscripts/taco-expected.txt",
                "http://build.chromium.org/f/chromium/layout_test_results/Mock_Builder/results/layout-test-results/userscripts/taco-actual.txt",
                "http://build.chromium.org/f/chromium/layout_test_results/Mock_Builder/results/layout-test-results/userscripts/taco-diff.txt",
            ]);
        });
    });

    deepEqual(probedURLs, [
        "http://build.chromium.org/f/chromium/layout_test_results/Mock_Builder/results/layout-test-results/userscripts/another-test-expected.png",
        "http://build.chromium.org/f/chromium/layout_test_results/Mock_Builder/results/layout-test-results/userscripts/another-test-actual.png",
        "http://build.chromium.org/f/chromium/layout_test_results/Mock_Builder/results/layout-test-results/userscripts/another-test-diff.png",
        "http://build.chromium.org/f/chromium/layout_test_results/Mock_Builder/results/layout-test-results/userscripts/another-test-crash-log.txt",
        "http://build.chromium.org/f/chromium/layout_test_results/Mock_Builder/results/layout-test-results/userscripts/taco-expected.png",
        "http://build.chromium.org/f/chromium/layout_test_results/Mock_Builder/results/layout-test-results/userscripts/taco-actual.png",
        "http://build.chromium.org/f/chromium/layout_test_results/Mock_Builder/results/layout-test-results/userscripts/taco-diff.png",
        "http://build.chromium.org/f/chromium/layout_test_results/Mock_Builder/results/layout-test-results/userscripts/taco-actual.txt",
        "http://build.chromium.org/f/chromium/layout_test_results/Mock_Builder/results/layout-test-results/userscripts/taco-expected.txt",
        "http://build.chromium.org/f/chromium/layout_test_results/Mock_Builder/results/layout-test-results/userscripts/taco-diff.txt",
    ]);
});

test("fetchResultsByBuilder", 5, function() {
    var simulator = new NetworkSimulator();

    var probedURLs = [];
    simulator.jsonp = function(url, callback)
    {
        simulator.scheduleCallback(function() {
            probedURLs.push(url);
            if (base.endsWith(url, 'MockBuilder2/r1%20(1)/full_results.json'))
                callback({'MockBuildResults': true});
            else
                callback({});
        });
    };

    config.currentPlatform = 'gtk';

    var oldCachedBuildInfos = builders.cachedBuildInfos;
    builders.cachedBuildInfos = function(platform, builderName, callback) {
        callback(
        {
            1: {
                sourceStamp: {
                    revision: 1
                }
            },
            2: {
                sourceStamp: {
                    revision: 2
                }
            }

        });
    };

    var oldRecentBuildInfos = builders.recentBuildInfos;
    builders.recentBuildInfos = function(callback) {
        callback({
            'MockBuilder1': true,
            'MockBuilder2': true
        });
    };

    simulator.runTest(function() {
        results.fetchResultsByBuilder(['MockBuilder1', 'MockBuilder2'], function(resultsByBuilder) {
            deepEqual(resultsByBuilder, {
                "MockBuilder2": {
                    "MockBuildResults": true,
                    "_buildLocation": {
                        "buildNumber": "1",
                        "revision": 1,
                        "url": "http://build.webkit.org/results/MockBuilder2/r1%20(1)/full_results.json"
                    }
                }
            });
        });
    });

    deepEqual(probedURLs, [
        "http://build.webkit.org/results/MockBuilder1/r2%20(2)/full_results.json",
        "http://build.webkit.org/results/MockBuilder2/r2%20(2)/full_results.json",
        "http://build.webkit.org/results/MockBuilder1/r1%20(1)/full_results.json",
        "http://build.webkit.org/results/MockBuilder2/r1%20(1)/full_results.json"
    ]);

    builders.cachedBuildInfos = oldCachedBuildInfos;
    equal(builders.cachedBuildInfos, oldCachedBuildInfos, "Failed to restore real base!");

    builders.recentBuildInfos = oldRecentBuildInfos;
    equal(builders.recentBuildInfos, oldRecentBuildInfos, "Failed to restore real base!");
});

})();
