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

module("builders");

var kExampleBuilderStatusJSON =  {
    "WebKit Linux": {
        "basedir": "WebKit_Linux",
        "cachedBuilds": [11459, 11460, 11461, 11462],
        "category": "6webkit linux latest",
        "currentBuilds": [11462],
        "pendingBuilds": 0,
        "slaves": ["vm124-m1"],
        "state": "building"
    },
    "WebKit Mac10.6": {
        "basedir": "WebKit_Linux",
        "cachedBuilds": [11459, 11460, 11461, 11462],
        "category": "6webkit linux latest",
        "currentBuilds": [11461, 11462],
        "pendingBuilds": 0,
        "slaves": ["vm124-m1"],
        "state": "building"
    },
    "WebKit ASAN": {
        "basedir": "WebKit_Linux",
        "cachedBuilds": [11459, 11460, 11461, 11462],
        "category": "6webkit linux latest",
        "currentBuilds": [11461, 11462],
        "pendingBuilds": 0,
        "slaves": ["vm124-m1"],
        "state": "building"
    },
};

var kExampleWebKitDotOrgBuilderStatusJSON =  {
    "Apple Lion Release WK2 (Tests)": {
        "basedir": "WebKit_Linux",
        "cachedBuilds": [11459, 11460, 11461, 11462],
        "category": "6webkit linux latest",
        "currentBuilds": [11462],
        "pendingBuilds": 0,
        "slaves": ["vm124-m1"],
        "state": "building"
    },
    "GTK Linux 64-bit Debug": {
        "basedir": "WebKit_Linux",
        "cachedBuilds": [11459, 11460, 11461, 11462],
        "category": "6webkit linux latest",
        "currentBuilds": [11461, 11462],
        "pendingBuilds": 0,
        "slaves": ["vm124-m1"],
        "state": "building"
    },
    "Qt Linux Release": {
        "basedir": "WebKit_Linux",
        "cachedBuilds": [11459, 11460, 11461, 11462],
        "category": "6webkit linux latest",
        "currentBuilds": [11461, 11462],
        "pendingBuilds": 0,
        "slaves": ["vm124-m1"],
        "state": "building"
    },
};

var kExampleBuildInfoJSON = {
    "blame": ["abarth@webkit.org"],
    "builderName": "WebKit Linux",
    "changes": ["Files:\n Tools/BuildSlaveSupport/build.webkit.org-config/public_html/TestFailures/main.js\n Tools/ChangeLog\nAt: Thu 04 Aug 2011 00:50:38\nChanged By: abarth@webkit.org\nComments: Fix types.  Sadly, main.js has no test coverage.  (I need to think\nabout how to test this part of the code.)\n\n* BuildSlaveSupport/build.webkit.org-config/public_html/TestFailures/main.js:Properties: \n\n\n", "Files:\n LayoutTests/ChangeLog\n LayoutTests/platform/chromium-mac/fast/box-shadow/inset-box-shadows-expected.png\n LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-horizontal-expected.png\n LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-strict-horizontal-expected.png\n LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-strict-vertical-expected.png\n LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-vertical-expected.png\nAt: Thu 04 Aug 2011 00:50:38\nChanged By: abarth@webkit.org\nComments: Update baselines after <http://trac.webkit.org/changeset/92340>.\n\n* platform/chromium-mac/fast/box-shadow/inset-box-shadows-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-horizontal-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-strict-horizontal-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-strict-vertical-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-vertical-expected.png:Properties: \n\n\n"],
    "currentStep": null,
    "eta": null,
    "logs": [
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/update_scripts/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/update/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/compile/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/test_shell_tests/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/webkit_unit_tests/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/webkit_tests/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/archive_webkit_tests_results/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/webkit_gpu_tests/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/archive_webkit_tests_gpu_results/logs/stdio"]
    ],
    "number": 11461,
    "properties": [
        ["blamelist", ["abarth@webkit.org"], "Build"],
        ["branch", "trunk", "Build"],
        ["buildername", "WebKit Linux", "Build"],
        ["buildnumber", 11461, "Build"],
        ["got_revision", "95395", "Source"],
        ["got_webkit_revision", "92358", "Source"],
        ["gtest_filter", null, "Factory"],
        ["mastername", "chromium.webkit", "master.cfg"],
        ["revision", "92358", "Build"],
        ["scheduler", "s6_webkit_rel", "Scheduler"],
        ["slavename", "vm124-m1", "BuildSlave"]
    ],
    "reason": "",
    "requests": [{
        "builderName": "WebKit Linux",
        "builds": [11461],
        "source": {
            "branch": "trunk",
            "changes": [{
                "branch": "trunk",
                "category": null,
                "comments": "Fix types.  Sadly, main.js has no test coverage.  (I need to think\nabout how to test this part of the code.)\n\n* BuildSlaveSupport/build.webkit.org-config/public_html/TestFailures/main.js:",
                "files": ["Tools/BuildSlaveSupport/build.webkit.org-config/public_html/TestFailures/main.js", "Tools/ChangeLog"],
                "number": 43707,
                "properties": [],
                "repository": "",
                "revision": "92357",
                "revlink": "http://trac.webkit.org/changeset/92357",
                "when": 1312444238.855081,
                "who": "abarth@webkit.org"
            }, {
                "branch": "trunk",
                "category": null,
                "comments": "Update baselines after <http://trac.webkit.org/changeset/92340>.\n\n* platform/chromium-mac/fast/box-shadow/inset-box-shadows-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-horizontal-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-strict-horizontal-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-strict-vertical-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-vertical-expected.png:",
                "files": ["LayoutTests/ChangeLog", "LayoutTests/platform/chromium-mac/fast/box-shadow/inset-box-shadows-expected.png", "LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-horizontal-expected.png", "LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-strict-horizontal-expected.png", "LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-strict-vertical-expected.png", "LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-vertical-expected.png"],
                "number": 43708,
                "properties": [],
                "repository": "",
                "revision": "92358",
                "revlink": "http://trac.webkit.org/changeset/92358",
                "when": 1312444238.855538,
                "who": "abarth@webkit.org"
            }],
            "hasPatch": false,
            "revision": "92358"
        },
        "submittedAt": 1312444298.989818
    }],
    "results": 2,
    "slave": "vm124-m1",
    "sourceStamp": {
        "branch": "trunk",
        "changes": [{
            "branch": "trunk",
            "category": null,
            "comments": "Fix types.  Sadly, main.js has no test coverage.  (I need to think\nabout how to test this part of the code.)\n\n* BuildSlaveSupport/build.webkit.org-config/public_html/TestFailures/main.js:",
            "files": ["Tools/BuildSlaveSupport/build.webkit.org-config/public_html/TestFailures/main.js", "Tools/ChangeLog"],
            "number": 43707,
            "properties": [],
            "repository": "",
            "revision": "92357",
            "revlink": "http://trac.webkit.org/changeset/92357",
            "when": 1312444238.855081,
            "who": "abarth@webkit.org"
        }, {
            "branch": "trunk",
            "category": null,
            "comments": "Update baselines after <http://trac.webkit.org/changeset/92340>.\n\n* platform/chromium-mac/fast/box-shadow/inset-box-shadows-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-horizontal-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-strict-horizontal-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-strict-vertical-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-vertical-expected.png:",
            "files": ["LayoutTests/ChangeLog", "LayoutTests/platform/chromium-mac/fast/box-shadow/inset-box-shadows-expected.png", "LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-horizontal-expected.png", "LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-strict-horizontal-expected.png", "LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-strict-vertical-expected.png", "LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-vertical-expected.png"],
            "number": 43708,
            "properties": [],
            "repository": "",
            "revision": "92358",
            "revlink": "http://trac.webkit.org/changeset/92358",
            "when": 1312444238.855538,
            "who": "abarth@webkit.org"
        }],
        "hasPatch": false,
        "revision": "92358"
    },
    "steps": [{
        "eta": null,
        "expectations": [
            ["output", 2297, 2300.6571014083784]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/update_scripts/logs/stdio"]
        ],
        "name": "update_scripts",
        "results": [0, []],
        "statistics": {},
        "step_number": 0,
        "text": ["update_scripts"],
        "times": [1312444299.102834, 1312444309.270324],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 20453, 17580.5002389645]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/update/logs/stdio"] ],
        "name": "update",
        "results": [0, []],
        "statistics": {},
        "step_number": 1,
        "text": ["update", "r95395", "webkit r92358"],
        "times": [1312444309.270763, 1312444398.468139],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 17316, 2652.4850499589456]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/compile/logs/stdio"]
        ],
        "name": "compile",
        "results": [0, []],
        "statistics": {},
        "step_number": 2,
        "text": ["compile"],
        "times": [1312444398.46855, 1312444441.68838],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 91980, 92002.12628325251]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/test_shell_tests/logs/stdio"]
        ],
        "name": "test_shell_tests",
        "results": [0, []],
        "statistics": {},
        "step_number": 3,
        "text": ["test_shell_tests", "1 disabled", "2 flaky"],
        "times": [1312444441.688756, 1312444451.74908],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 20772, 20772.638503443086]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/webkit_unit_tests/logs/stdio"]
        ],
        "name": "webkit_unit_tests",
        "results": [0, []],
        "statistics": {},
        "step_number": 4,
        "text": ["webkit_unit_tests", "1 disabled"],
        "times": [1312444451.749574, 1312444452.306143],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 2477406, 2498915.6146275494]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/webkit_tests/logs/stdio"]
        ],
        "name": "webkit_tests",
        "results": [2, ["webkit_tests"]],
        "statistics": {},
        "step_number": 5,
        "text": ["webkit_tests", "2014 fixable", "(370 skipped)", "failed 1", "<div class=\"BuildResultInfo\">", "<a href=\"http://webkit-test-results.appspot.com/dashboards/flakiness_dashboard.html#master=ChromiumWebkit&tests=fast/box-shadow/box-shadow-clipped-slices.html\">", "<abbr title=\"fast/box-shadow/box-shadow-clipped-slices.html\">box-shadow-clipped-slices.html</abbr>", "</a>", "</div>"],
        "times": [1312444452.306695, 1312444768.888266],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 2668845, 2672746.372363254]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/archive_webkit_tests_results/logs/stdio"]
        ],
        "name": "archive_webkit_tests_results",
        "results": [0, []],
        "statistics": {},
        "step_number": 6,
        "text": ["archived webkit_tests results"],
        "times": [1312444768.888746, 1312444781.444399],
        "urls": {
            "layout test results": "http://build.chromium.org/buildbot/layout_test_results/WebKit_Linux\r/95395\rNone"
        }
    }, {
        "eta": null,
        "expectations": [
            ["output", 210958, 208138.4965182993]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/webkit_gpu_tests/logs/stdio"]
        ],
        "name": "webkit_gpu_tests",
        "results": [2, ["webkit_gpu_tests"]],
        "statistics": {},
        "step_number": 7,
        "text": ["webkit_gpu_tests", "148 fixable", "(24 skipped)", "failed 1", "<div class=\"BuildResultInfo\">", "<a href=\"http://webkit-test-results.appspot.com/dashboards/flakiness_dashboard.html#master=ChromiumWebkit&tests=compositing/scaling/tiled-layer-recursion.html\">", "<abbr title=\"compositing/scaling/tiled-layer-recursion.html\">tiled-layer-recursion.html</abbr>", "</a>", "</div>"],
        "times": [1312444781.444903, 1312444966.856074],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 148825, 147822.1074277072]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/archive_webkit_tests_gpu_results/logs/stdio"]
        ],
        "name": "archive_webkit_tests_gpu_results",
        "results": [0, []],
        "statistics": {},
        "step_number": 8,
        "text": ["archived webkit_tests gpu results"],
        "times": [1312444966.856575, 1312444970.458655],
        "urls": {
            "layout test gpu results": "http://build.chromium.org/buildbot/layout_test_results/WebKit_Linux_-_GPU\r/95395\rNone"
        }
    }],
    "text": ["failed", "webkit_tests", "webkit_gpu_tests"],
    "times": [1312444299.10216, 1312444970.459138]
};

var kExampleBuildInfoWithWebKitTestCrashJSON = {
    "blame": ["asvitkine@chromium.org", "derat@chromium.org", "nirnimesh@chromium.org"],
    "builderName": "WebKit Win (deps)(dbg)(2)",
    "currentStep": null,
    "eta": null,
    "logs": [
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/svnkill/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/update_scripts/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/taskkill/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/update/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/extract_build/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/webkit_tests/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/archive_webkit_tests_results/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/webkit_gpu_tests/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/archive_webkit_tests_gpu_results/logs/stdio"]
    ],
    "number": 7653,
    "properties": [
        ["blamelist", ["asvitkine@chromium.org", "derat@chromium.org", "nirnimesh@chromium.org"], "Build"],
        ["branch", "src", "Build"],
        ["buildername", "WebKit Win (deps)(dbg)(2)", "Builder"],
        ["buildnumber", 7653, "Build"],
        ["got_revision", "104939", "Source"],
        ["gtest_filter", null, "Factory"],
        ["mastername", "chromium.webkit", "master.cfg"],
        ["project", "", "Build"],
        ["repository", "svn://svn-mirror.golo.chromium.org/chrome/trunk", "Build"],
        ["revision", "104939", "Build"],
        ["scheduler", "s1_chromium_dbg_dep", "Scheduler"],
        ["slavename", "vm114-m1", "BuildSlave"]
    ],
    "reason": "downstream",
    "results": 2,
    "slave": "vm114-m1",
    "sourceStamp": {
        "branch": "src",
        "changes": [{
            "at": "Tue 11 Oct 2011 12:18:40",
            "branch": "src",
            "category": null,
            "comments": "Disable prefs.PrefsTest.testGeolocationPref on win\n\nTBR=dennisjeffrey@chromium.org\nBUG=99865\nTEST=\n\nReview URL: http://codereview.chromium.org/8234007",
            "files": [{
                "name": "chrome/test/functional/PYAUTO_TESTS",
                "url": null
            }],
            "number": 1397,
            "project": "",
            "properties": [],
            "repository": "svn://svn-mirror.golo.chromium.org/chrome/trunk",
            "rev": "104936",
            "revision": "104936",
            "revlink": "http://src.chromium.org/viewvc/chrome?view=rev&revision=104936",
            "when": 1318360720,
            "who": "nirnimesh@chromium.org"
        }, {
            "at": "Tue 11 Oct 2011 12:23:11",
            "branch": "src",
            "category": null,
            "comments": "aura: Get rid of some unneeded stubs.\n\nMost of these are for dialogs that already had WebUI\nimplementations.\n\nBUG=99718\nTEST=built with use_aura=1\n\nReview URL: http://codereview.chromium.org/8218027",
            "files": [{
                "name": "chrome/browser/ui/login/login_prompt_ui.cc",
                "url": null
            }, {
                "name": "chrome/browser/ui/views/stubs_aura.cc",
                "url": null
            }, {
                "name": "chrome/chrome_browser.gypi",
                "url": null
            }, {
                "name": "chrome/common/url_constants.cc",
                "url": null
            }, {
                "name": "chrome/common/url_constants.h",
                "url": null
            }],
            "number": 1398,
            "project": "",
            "properties": [],
            "repository": "svn://svn-mirror.golo.chromium.org/chrome/trunk",
            "rev": "104937",
            "revision": "104937",
            "revlink": "http://src.chromium.org/viewvc/chrome?view=rev&revision=104937",
            "when": 1318360991,
            "who": "derat@chromium.org"
        }, {
            "at": "Tue 11 Oct 2011 12:34:10",
            "branch": "src",
            "category": null,
            "comments": "Add Windows manifest for views_examples.\n\nThis allows views_examples to use themed controls and fixes\na problem with combo box drop down menus not being shown.\n\nManifest file copied from chrome/app/chrome.exe.manifest.\n\nBUG=none\nTEST=manual\n\nReview URL: http://codereview.chromium.org/8227017",
            "files": [{
                "name": "views/examples/views_examples.exe.manifest",
                "url": null
            }, {
                "name": "views/views.gyp",
                "url": null
            }],
            "number": 1400,
            "project": "",
            "properties": [],
            "repository": "svn://svn-mirror.golo.chromium.org/chrome/trunk",
            "rev": "104939",
            "revision": "104939",
            "revlink": "http://src.chromium.org/viewvc/chrome?view=rev&revision=104939",
            "when": 1318361650,
            "who": "asvitkine@chromium.org"
        }],
        "hasPatch": false,
        "project": "",
        "repository": "svn://svn-mirror.golo.chromium.org/chrome/trunk",
        "revision": "104939"
    },
    "steps": [{
        "eta": null,
        "expectations": [
            ["output", 1444, 1444.0]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/svnkill/logs/stdio"]
        ],
        "name": "svnkill",
        "results": [0, []],
        "statistics": {},
        "step_number": 0,
        "text": ["svnkill"],
        "times": [1318364210.0697701, 1318364210.463649],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 2916, 2757.1580967830819]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/update_scripts/logs/stdio"]
        ],
        "name": "update_scripts",
        "results": [0, []],
        "statistics": {},
        "step_number": 1,
        "text": ["update_scripts"],
        "times": [1318364210.464299, 1318364234.741575],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 4544, 4543.9687499691299]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/taskkill/logs/stdio"]
        ],
        "name": "taskkill",
        "results": [0, []],
        "statistics": {},
        "step_number": 2,
        "text": ["taskkill"],
        "times": [1318364234.742265, 1318364245.842006],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 1019773, 22826.104572568838]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/update/logs/stdio"]
        ],
        "name": "update",
        "results": [0, []],
        "statistics": {},
        "step_number": 3,
        "text": ["update", "r104939"],
        "times": [1318364245.843008, 1318366370.946759],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 19829, 19620.437500003842]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/extract_build/logs/stdio"]
        ],
        "name": "extract_build",
        "results": [1, []],
        "statistics": {},
        "step_number": 4,
        "text": ["extract_build", "warnings"],
        "times": [1318366370.94771, 1318366404.552783],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 2685, 1320624.4817683753]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/webkit_tests/logs/stdio"]
        ],
        "name": "webkit_tests",
        "results": [2, ["webkit_tests"]],
        "statistics": {},
        "step_number": 5,
        "text": ["webkit_tests", "?? fixable", "(0 skipped)", "crashed or hung"],
        "times": [1318366404.5537021, 1318366405.2564809],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 3736, 606747.58321854263]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/archive_webkit_tests_results/logs/stdio"]
        ],
        "name": "archive_webkit_tests_results",
        "results": [2, ["archive_webkit_tests_results"]],
        "statistics": {},
        "step_number": 6,
        "text": ["archived webkit_tests results", "failed"],
        "times": [1318366405.2573731, 1318366406.505815],
        "urls": {
            "layout test results": "http://build.chromium.org/buildbot/layout_test_results/WebKit_Win__deps__dbg__2_\r/0\rNone"
        }
    }, {
        "eta": null,
        "expectations": [
            ["output", 2751, 124434.9266140602]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/webkit_gpu_tests/logs/stdio"]
        ],
        "name": "webkit_gpu_tests",
        "results": [2, ["webkit_gpu_tests"]],
        "statistics": {},
        "step_number": 7,
        "text": ["webkit_gpu_tests", "?? fixable", "(0 skipped)", "crashed or hung"],
        "times": [1318366406.506711, 1318366407.179585],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 3760, 35709.407800958375]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/archive_webkit_tests_gpu_results/logs/stdio"]
        ],
        "name": "archive_webkit_tests_gpu_results",
        "results": [2, ["archive_webkit_tests_gpu_results"]],
        "statistics": {},
        "step_number": 8,
        "text": ["archived webkit_tests gpu results", "failed"],
        "times": [1318366407.1804891, 1318366408.071501],
        "urls": {
            "layout test gpu results": "http://build.chromium.org/buildbot/layout_test_results/WebKit_Win__deps__dbg__2__-_GPU\r/0\rNone"
        }
    }],
    "text": ["failed", "webkit_tests", "archive_webkit_tests_results", "webkit_gpu_tests", "archive_webkit_tests_gpu_results"],
    "times": [1318364210.066524, 1318366408.0732119]
};

var kExampleBuildInfoWithTaskKillWarning = {
    "blame": ["asvitkine@chromium.org", "derat@chromium.org", "nirnimesh@chromium.org"],
    "builderName": "WebKit Win (deps)(dbg)(2)",
    "currentStep": null,
    "eta": null,
    "logs": [
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Win%20%28deps%29%28dbg%29%282%29/builds/7653/steps/svnkill/logs/stdio"],
    ],
    "number": 7653,
    "properties": [
        ["blamelist", ["asvitkine@chromium.org", "derat@chromium.org", "nirnimesh@chromium.org"], "Build"],
        ["branch", "src", "Build"],
        ["buildername", "WebKit Win (deps)(dbg)(2)", "Builder"],
        ["buildnumber", 7653, "Build"],
        ["got_revision", "104939", "Source"],
        ["gtest_filter", null, "Factory"],
        ["mastername", "chromium.webkit", "master.cfg"],
        ["project", "", "Build"],
        ["repository", "svn://svn-mirror.golo.chromium.org/chrome/trunk", "Build"],
        ["revision", "104939", "Build"],
        ["scheduler", "s1_chromium_dbg_dep", "Scheduler"],
        ["slavename", "vm114-m1", "BuildSlave"]
    ],
    "reason": "downstream",
    "results": 2,
    "slave": "vm114-m1",
    "sourceStamp": {
        "branch": "src",
        "changes": [{
            "at": "Tue 11 Oct 2011 12:18:40",
            "branch": "src",
            "category": null,
            "comments": "Disable prefs.PrefsTest.testGeolocationPref on win\n\nTBR=dennisjeffrey@chromium.org\nBUG=99865\nTEST=\n\nReview URL: http://codereview.chromium.org/8234007",
            "files": [{
                "name": "chrome/test/functional/PYAUTO_TESTS",
                "url": null
            }],
            "number": 1397,
            "project": "",
            "properties": [],
            "repository": "svn://svn-mirror.golo.chromium.org/chrome/trunk",
            "rev": "104936",
            "revision": "104936",
            "revlink": "http://src.chromium.org/viewvc/chrome?view=rev&revision=104936",
            "when": 1318360720,
            "who": "nirnimesh@chromium.org"
        }],
        "hasPatch": false,
        "project": "",
        "repository": "svn://svn-mirror.golo.chromium.org/chrome/trunk",
        "revision": "104939"
    },
    "steps": [{
        "eta": null,
        "expectations": [["output",1776,1534.0625014267862]],
        "isFinished": true,
        "isStarted": true,
        "logs": [["stdio","http://build.chromium.org/p/chromium.webkitbuilders/XP%20Perf/builds/10268/steps/taskkill/logs/stdio"]],
        "name": "taskkill",
        "results": [1,[]],
        "statistics": {},
        "step_number": 2,
        "text": ["taskkill","warning"],
        "times": [1339438214.177362,1339438222.555572],
        "urls": {}
    }],
    "text": ["failed", "webkit_tests", "archive_webkit_tests_results", "webkit_gpu_tests", "archive_webkit_tests_gpu_results"],
    "times": [1318364210.066524, 1318366408.0732119]
};


var kExamplePerfBuilderStatusJSON =  {
    "WebKit Linux": {
        "basedir": "WebKit_Linux",
        "cachedBuilds": [11459, 11460, 11461, 11462],
        "category": "6webkit linux latest",
        "currentBuilds": [11462],
        "pendingBuilds": 0,
        "slaves": ["vm124-m1"],
        "state": "building"
    },
    "Mac10.6 Perf": {
        "basedir": "Mac10_6_Perf",
        "cachedBuilds": [11459, 11460, 11461, 11462],
        "category": "6webkit linux latest",
        "currentBuilds": [11461, 11462],
        "pendingBuilds": 0,
        "slaves": ["vm124-m1"],
        "state": "building"
    },
};

var kExamplePerfBuildInfoJSON = {
    "blame": ["abarth@webkit.org"],
    "builderName": "Mac10.6 Perf",
    "changes": ["Files:\n Tools/BuildSlaveSupport/build.webkit.org-config/public_html/TestFailures/main.js\n Tools/ChangeLog\nAt: Thu 04 Aug 2011 00:50:38\nChanged By: abarth@webkit.org\nComments: Fix types.  Sadly, main.js has no test coverage.  (I need to think\nabout how to test this part of the code.)\n\n* BuildSlaveSupport/build.webkit.org-config/public_html/TestFailures/main.js:Properties: \n\n\n", "Files:\n LayoutTests/ChangeLog\n LayoutTests/platform/chromium-mac/fast/box-shadow/inset-box-shadows-expected.png\n LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-horizontal-expected.png\n LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-strict-horizontal-expected.png\n LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-strict-vertical-expected.png\n LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-vertical-expected.png\nAt: Thu 04 Aug 2011 00:50:38\nChanged By: abarth@webkit.org\nComments: Update baselines after <http://trac.webkit.org/changeset/92340>.\n\n* platform/chromium-mac/fast/box-shadow/inset-box-shadows-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-horizontal-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-strict-horizontal-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-strict-vertical-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-vertical-expected.png:Properties: \n\n\n"],
    "currentStep": null,
    "eta": null,
    "logs": [
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/update_scripts/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/update/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/compile/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/test_shell_tests/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/webkit_unit_tests/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/webkit_tests/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/archive_webkit_tests_results/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/webkit_gpu_tests/logs/stdio"],
        ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/archive_webkit_tests_gpu_results/logs/stdio"]
    ],
    "number": 11461,
    "properties": [
        ["blamelist", ["abarth@webkit.org"], "Build"],
        ["branch", "trunk", "Build"],
        ["buildername", "WebKit Linux", "Build"],
        ["buildnumber", 11461, "Build"],
        ["got_revision", "95395", "Source"],
        ["got_webkit_revision", "92358", "Source"],
        ["gtest_filter", null, "Factory"],
        ["mastername", "chromium.webkit", "master.cfg"],
        ["revision", "92358", "Build"],
        ["scheduler", "s6_webkit_rel", "Scheduler"],
        ["slavename", "vm124-m1", "BuildSlave"]
    ],
    "reason": "",
    "requests": [{
        "builderName": "WebKit Linux",
        "builds": [11461],
        "source": {
            "branch": "trunk",
            "changes": [{
                "branch": "trunk",
                "category": null,
                "comments": "Fix types.  Sadly, main.js has no test coverage.  (I need to think\nabout how to test this part of the code.)\n\n* BuildSlaveSupport/build.webkit.org-config/public_html/TestFailures/main.js:",
                "files": ["Tools/BuildSlaveSupport/build.webkit.org-config/public_html/TestFailures/main.js", "Tools/ChangeLog"],
                "number": 43707,
                "properties": [],
                "repository": "",
                "revision": "92357",
                "revlink": "http://trac.webkit.org/changeset/92357",
                "when": 1312444238.855081,
                "who": "abarth@webkit.org"
            }, {
                "branch": "trunk",
                "category": null,
                "comments": "Update baselines after <http://trac.webkit.org/changeset/92340>.\n\n* platform/chromium-mac/fast/box-shadow/inset-box-shadows-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-horizontal-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-strict-horizontal-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-strict-vertical-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-vertical-expected.png:",
                "files": ["LayoutTests/ChangeLog", "LayoutTests/platform/chromium-mac/fast/box-shadow/inset-box-shadows-expected.png", "LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-horizontal-expected.png", "LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-strict-horizontal-expected.png", "LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-strict-vertical-expected.png", "LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-vertical-expected.png"],
                "number": 43708,
                "properties": [],
                "repository": "",
                "revision": "92358",
                "revlink": "http://trac.webkit.org/changeset/92358",
                "when": 1312444238.855538,
                "who": "abarth@webkit.org"
            }],
            "hasPatch": false,
            "revision": "92358"
        },
        "submittedAt": 1312444298.989818
    }],
    "results": 2,
    "slave": "vm124-m1",
    "sourceStamp": {
        "branch": "trunk",
        "changes": [{
            "branch": "trunk",
            "category": null,
            "comments": "Fix types.  Sadly, main.js has no test coverage.  (I need to think\nabout how to test this part of the code.)\n\n* BuildSlaveSupport/build.webkit.org-config/public_html/TestFailures/main.js:",
            "files": ["Tools/BuildSlaveSupport/build.webkit.org-config/public_html/TestFailures/main.js", "Tools/ChangeLog"],
            "number": 43707,
            "properties": [],
            "repository": "",
            "revision": "92357",
            "revlink": "http://trac.webkit.org/changeset/92357",
            "when": 1312444238.855081,
            "who": "abarth@webkit.org"
        }, {
            "branch": "trunk",
            "category": null,
            "comments": "Update baselines after <http://trac.webkit.org/changeset/92340>.\n\n* platform/chromium-mac/fast/box-shadow/inset-box-shadows-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-horizontal-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-strict-horizontal-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-strict-vertical-expected.png:\n* platform/chromium-mac/fast/repaint/shadow-multiple-vertical-expected.png:",
            "files": ["LayoutTests/ChangeLog", "LayoutTests/platform/chromium-mac/fast/box-shadow/inset-box-shadows-expected.png", "LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-horizontal-expected.png", "LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-strict-horizontal-expected.png", "LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-strict-vertical-expected.png", "LayoutTests/platform/chromium-mac/fast/repaint/shadow-multiple-vertical-expected.png"],
            "number": 43708,
            "properties": [],
            "repository": "",
            "revision": "92358",
            "revlink": "http://trac.webkit.org/changeset/92358",
            "when": 1312444238.855538,
            "who": "abarth@webkit.org"
        }],
        "hasPatch": false,
        "revision": "92358"
    },
    "steps": [{
        "eta": null,
        "expectations": [
            ["output", 2297, 2300.6571014083784]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/update_scripts/logs/stdio"]
        ],
        "name": "update_scripts",
        "results": [0, []],
        "statistics": {},
        "step_number": 0,
        "text": ["update_scripts"],
        "times": [1312444299.102834, 1312444309.270324],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 20453, 17580.5002389645]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/update/logs/stdio"]
        ],
        "name": "update",
        "results": [0, []],
        "statistics": {},
        "step_number": 1,
        "text": ["update", "r95395", "webkit r92358"],
        "times": [1312444309.270763, 1312444398.468139],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 17316, 2652.4850499589456]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/compile/logs/stdio"]
        ],
        "name": "compile",
        "results": [0, []],
        "statistics": {},
        "step_number": 2,
        "text": ["compile"],
        "times": [1312444398.46855, 1312444441.68838],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 91980, 92002.12628325251]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/test_shell_tests/logs/stdio"]
        ],
        "name": "test_shell_tests",
        "results": [0, []],
        "statistics": {},
        "step_number": 3,
        "text": ["test_shell_tests", "1 disabled", "2 flaky"],
        "times": [1312444441.688756, 1312444451.74908],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 20772, 20772.638503443086]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/webkit_unit_tests/logs/stdio"]
        ],
        "name": "webkit_unit_tests",
        "results": [0, []],
        "statistics": {},
        "step_number": 4,
        "text": ["webkit_unit_tests", "1 disabled"],
        "times": [1312444451.749574, 1312444452.306143],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 2477406, 2498915.6146275494]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/webkit_tests/logs/stdio"]
        ],
        "name": "webkit_tests",
        "results": [2, ["webkit_tests"]],
        "statistics": {},
        "step_number": 5,
        "text": ["webkit_tests", "2014 fixable", "(370 skipped)", "failed 1", "<div class=\"BuildResultInfo\">", "<a href=\"http://webkit-test-results.appspot.com/dashboards/flakiness_dashboard.html#master=ChromiumWebkit&tests=fast/box-shadow/box-shadow-clipped-slices.html\">", "<abbr title=\"fast/box-shadow/box-shadow-clipped-slices.html\">box-shadow-clipped-slices.html</abbr>", "</a>", "</div>"],
        "times": [1312444452.306695, 1312444768.888266],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 2668845, 2672746.372363254]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/archive_webkit_tests_results/logs/stdio"]
        ],
        "name": "dummy_perf_test_1",
        "results": [0, []],
        "statistics": {},
        "step_number": 6,
        "text": ["archived webkit_tests results"],
        "times": [1312444768.888746, 1312444781.444399],
        "urls": {
            "results": "http://dummyurl1"
        }
    }, {
        "eta": null,
        "expectations": [
            ["output", 210958, 208138.4965182993]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/webkit_gpu_tests/logs/stdio"]
        ],
        "name": "webkit_gpu_tests",
        "results": [2, ["webkit_gpu_tests"]],
        "statistics": {},
        "step_number": 7,
        "text": ["webkit_gpu_tests", "148 fixable", "(24 skipped)", "failed 1", "<div class=\"BuildResultInfo\">", "<a href=\"http://webkit-test-results.appspot.com/dashboards/flakiness_dashboard.html#master=ChromiumWebkit&tests=compositing/scaling/tiled-layer-recursion.html\">", "<abbr title=\"compositing/scaling/tiled-layer-recursion.html\">tiled-layer-recursion.html</abbr>", "</a>", "</div>"],
        "times": [1312444781.444903, 1312444966.856074],
        "urls": {}
    }, {
        "eta": null,
        "expectations": [
            ["output", 148825, 147822.1074277072]
        ],
        "isFinished": true,
        "isStarted": true,
        "logs": [
            ["stdio", "http://build.chromium.org/p/chromium.webkitbuilders/WebKit%20Linux/builds/11461/steps/archive_webkit_tests_gpu_results/logs/stdio"]
        ],
        "name": "dummy_perf_test_2",
        "results": [0, []],
        "statistics": {},
        "step_number": 8,
        "text": ["archived webkit_tests gpu results"],
        "times": [1312444966.856575, 1312444970.458655],
        "urls": {
            "results": "http://dummyurl2"
        }
    }],
    "text": ["failed", "webkit_tests", "webkit_gpu_tests"],
    "times": [1312444299.10216, 1312444970.459138]
};

test("buildersFailing", 3, function() {
    var simulator = new NetworkSimulator();
    builders.clearBuildInfoCache();

    var failingBuildInfoJSON = JSON.parse(JSON.stringify(kExampleBuildInfoJSON));
    failingBuildInfoJSON.number = 11460;
    failingBuildInfoJSON.steps[2].results[0] = 1;

    var requestedURLs = [];
    simulator.get = function(url, callback)
    {
        requestedURLs.push(url);
        simulator.scheduleCallback(function() {
            if (/\/json\/builders$/.exec(url))
                callback(kExampleBuilderStatusJSON);
            else if (/WebKit%20Linux/.exec(url))
                callback(kExampleBuildInfoJSON);
            else if (/WebKit%20Mac10\.6/.exec(url))
                callback(failingBuildInfoJSON);
            else if (/WebKit%20ASAN/.exec(url))
                callback(failingBuildInfoJSON);
            else {
                ok(false, "Unexpected URL: " + url);
                callback();
            }
        });
    };

    simulator.runTest(function() {
        builders.buildersFailingNonLayoutTests(function(builderNameList) {
            deepEqual(builderNameList, {
                "WebKit Linux": [
                    "webkit_gpu_tests"
                ],
                "WebKit Mac10.6": [
                    "webkit_gpu_tests"
                ]
            });
        });
    });

    deepEqual(requestedURLs, [
      "http://build.chromium.org/p/chromium.webkit/json/builders",
      "http://build.chromium.org/p/chromium.webkit/json/builders/WebKit%20Linux/builds/11461",
      "http://build.chromium.org/p/chromium.webkit/json/builders/WebKit%20Mac10.6/builds/11460",
    ]);
});

test("buildersFailing (Apple)", 3, function() {
    var simulator = new NetworkSimulator();
    builders.clearBuildInfoCache();

    config.currentPlatform = 'apple';

    var failingBuildInfoJSON = JSON.parse(JSON.stringify(kExampleBuildInfoJSON));
    failingBuildInfoJSON.number = 11460;
    failingBuildInfoJSON.steps[2].results[0] = 1;

    var requestedURLs = [];
    simulator.get = function(url, callback)
    {
        requestedURLs.push(url);
        simulator.scheduleCallback(function() {
            if (/\/json\/builders$/.exec(url))
                callback(kExampleWebKitDotOrgBuilderStatusJSON);
            else if (/Apple%20Lion%20Release%20WK2%20\(Tests\)/.exec(url))
                callback(kExampleBuildInfoJSON);
            else {
                ok(false, "Unexpected URL: " + url);
                callback();
            }
        });
    };

    simulator.runTest(function() {
        builders.buildersFailingNonLayoutTests(function(builderNameList) {
            deepEqual(builderNameList, {
                "Apple Lion Release WK2 (Tests)": [
                    "webkit_gpu_tests"
                ]
            });
        });
    });

    deepEqual(requestedURLs, [
        "http://build.webkit.org/json/builders",
        "http://build.webkit.org/json/builders/Apple%20Lion%20Release%20WK2%20(Tests)/builds/11461"
    ]);

    config.currentPlatform = 'chromium';
});


test("buildersFailing (run-webkit-tests crash)", 3, function() {
    var simulator = new NetworkSimulator();
    builders.clearBuildInfoCache();

    var builderStatusJSON = JSON.parse(JSON.stringify(kExampleBuilderStatusJSON));
    delete builderStatusJSON['WebKit Mac10.6'];
    builderStatusJSON['WebKit Linux'].cachedBuilds = [21460];
    builderStatusJSON['WebKit Linux'].currentBuilds = [];

    var failingBuildInfoJSON = JSON.parse(JSON.stringify(kExampleBuildInfoWithWebKitTestCrashJSON));
    failingBuildInfoJSON.number = 21460;

    var requestedURLs = [];
    simulator.get = function(url, callback)
    {
        requestedURLs.push(url);
        simulator.scheduleCallback(function() {
            if (/\/json\/builders$/.exec(url))
                callback(builderStatusJSON);
            else if (/WebKit%20Linux/.exec(url))
                callback(failingBuildInfoJSON);
            else {
                ok(false, "Unexpected URL: " + url);
                callback();
            }
        });
    };

    simulator.runTest(function() {
        builders.buildersFailingNonLayoutTests(function(builderNameList) {
            deepEqual(builderNameList, {
                "WebKit Linux": [
                    "webkit_tests",
                    "archive_webkit_tests_results",
                    "webkit_gpu_tests",
                    "archive_webkit_tests_gpu_results"
                ]
            });
        });
    });

    deepEqual(requestedURLs, [
      "http://build.chromium.org/p/chromium.webkit/json/builders",
      "http://build.chromium.org/p/chromium.webkit/json/builders/WebKit%20Linux/builds/21460",
    ]);
});

test("buildersFailing (taskkill warning)", 3, function() {
    var simulator = new NetworkSimulator();
    builders.clearBuildInfoCache();

    var builderStatusJSON = JSON.parse(JSON.stringify(kExampleBuilderStatusJSON));
    delete builderStatusJSON['WebKit Mac10.6'];
    builderStatusJSON['WebKit Linux'].cachedBuilds = [21460];
    builderStatusJSON['WebKit Linux'].currentBuilds = [];

    var failingBuildInfoJSON = JSON.parse(JSON.stringify(kExampleBuildInfoWithTaskKillWarning));
    failingBuildInfoJSON.number = 21460;

    var requestedURLs = [];
    simulator.get = function(url, callback)
    {
        requestedURLs.push(url);
        simulator.scheduleCallback(function() {
            if (/\/json\/builders$/.exec(url))
                callback(builderStatusJSON);
            else if (/WebKit%20Linux/.exec(url))
                callback(failingBuildInfoJSON);
            else {
                ok(false, "Unexpected URL: " + url);
                callback();
            }
        });
    };

    simulator.runTest(function() {
        builders.buildersFailingNonLayoutTests(function(builderNameList) {
            deepEqual(builderNameList, {});
        });
    });

    deepEqual(requestedURLs, [
      "http://build.chromium.org/p/chromium.webkit/json/builders",
      "http://build.chromium.org/p/chromium.webkit/json/builders/WebKit%20Linux/builds/21460",
    ]);
});

test("builders.perfBuilders", 2, function() {
    var simulator = new NetworkSimulator();
    builders.clearBuildInfoCache();

    var builderStatusJSON = JSON.parse(JSON.stringify(kExamplePerfBuilderStatusJSON));
    var failingBuildInfoJSON = JSON.parse(JSON.stringify(kExamplePerfBuildInfoJSON));

    var requestedURLs = [];
    simulator.get = function(url, callback)
    {
        requestedURLs.push(url);
        simulator.scheduleCallback(function() {
            if (/\/json\/builders$/.exec(url))
                callback(builderStatusJSON);
            else if (/Mac10.6%20Perf/.exec(url))
                callback(failingBuildInfoJSON);
            else
                callback();
        });
    };

    simulator.runTest(function() {
        builders.perfBuilders(function(builderNameList) {
            deepEqual(builderNameList, {
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
                    }
                ]
            });
        });
    });
});

})();
