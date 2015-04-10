// Copyright (C) Zan Dobersek <zandobersek@gmail.com>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//         * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//         * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//         * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

module('loader');

test('loading steps', 1, function() {
    resetGlobals();
    var loadedSteps = [];
    g_history._handleLocationChange = function() {
        deepEqual(loadedSteps, ['step 1', 'step 2']);
    }
    var resourceLoader = new loader.Loader();
    function loadingStep1() {
        loadedSteps.push('step 1');
        resourceLoader.load();
    }
    function loadingStep2() {
        loadedSteps.push('step 2');
        resourceLoader.load();
    }

    resourceLoader._loadingSteps = [loadingStep1, loadingStep2];
    resourceLoader.load();
});

// Total number of assertions is 1 for the deepEqual of the builder lists
// and then 2 per builder (one for ok, one for deepEqual of tests).
test('results files loading', 9, function() {
    resetGlobals();
    var expectedLoadedBuilders =  ['Apple Lion Debug WK2 (Tests)', 'Apple Lion Release WK2 (Tests)', 'GTK Linux 64-bit Release', 'Qt Linux Tests'];
    var loadedBuilders = [];
    var resourceLoader = new loader.Loader();
    resourceLoader._loadNext = function() {
        deepEqual(loadedBuilders.sort(), expectedLoadedBuilders);
        loadedBuilders.forEach(function(builderName) {
            ok('secondsSinceEpoch' in g_resultsByBuilder[builderName]);
            deepEqual(g_resultsByBuilder[builderName].tests, {});
        });
    }

    var requestFunction = loader.request;
    loader.request = function(url, successCallback, errorCallback) {
        var builderName = /builder=([\w ().-]+)&/.exec(url)[1];
        loadedBuilders.push(builderName);
        successCallback({responseText: '{"version": 4, "' + builderName + '": {"secondsSinceEpoch": [' + Date.now() + '], "tests": {}}}'});
    }

    loadBuildersList('@ToT - webkit.org', 'layout-tests');
 
    try {
        resourceLoader._loadResultsFiles();
    } finally {
        loader.request = requestFunction;
    }
});

test('expectations files loading', 1, function() {
    resetGlobals();
    g_history.parseCrossDashboardParameters();
    var expectedLoadedPlatforms = ["efl", "efl-wk1", "efl-wk2", "gtk", "gtk-wk2",
        "mac", "mac-lion", "mac-wk2", "mac-wk2", "qt", "win", "wk2"];
    var loadedPlatforms = [];
    var resourceLoader = new loader.Loader();
    resourceLoader._loadNext = function() {
        deepEqual(loadedPlatforms.sort(), expectedLoadedPlatforms);
    }

    var requestFunction = loader.request;
    loader.request = function(url, successCallback, errorCallback) {
        loadedPlatforms.push(/LayoutTests\/platform\/(.+)\/TestExpectations/.exec(url)[1]);
        successCallback({responseText: ''});
    }

    try {
        resourceLoader._loadExpectationsFiles();
    } finally {
        loader.request = requestFunction;
    }
});

test('results file failing to load', 2, function() {
    resetGlobals();
    loadBuildersList('@ToT - webkit.org', 'layout-tests');
    
    var resourceLoader = new loader.Loader();
    var resourceLoadCount = 0;
    resourceLoader._handleResourceLoad = function() {
        resourceLoadCount++;
    }

    var builder1 = 'builder1';
    currentBuilders()[builder1] = true;
    resourceLoader._handleResultsFileLoadError(builder1);

    var builder2 = 'builder2';
    currentBuilders()[builder2] = true;
    resourceLoader._handleResultsFileLoadError(builder2);

    deepEqual(resourceLoader._buildersThatFailedToLoad, [builder1, builder2]);
    equal(resourceLoadCount, 2);

});

test('Default builder gets set.', 3, function() {
    resetGlobals();
    loadBuildersList('@ToT - webkit.org', 'layout-tests');
    
    var defaultBuilder = currentBuilderGroup().defaultBuilder();
    ok(defaultBuilder, "Default builder should exist.");
   
    // Simulate error loading the default builder data, then make sure
    // a new defaultBuilder is set, and isn't the now invalid one.
    var resourceLoader = new loader.Loader();
    resourceLoader._handleResultsFileLoadError(defaultBuilder);
    var newDefaultBuilder = currentBuilderGroup().defaultBuilder();
    ok(newDefaultBuilder, "There should still be a default builder.");
    notEqual(newDefaultBuilder, defaultBuilder, "Default builder should not be the old default builder");
});

test('addBuilderLoadErrors', 1, function() {
    var resourceLoader = new loader.Loader();
    resourceLoader._buildersThatFailedToLoad = ['builder1', 'builder2'];
    resourceLoader._staleBuilders = ['staleBuilder1'];
    resourceLoader._addErrors();
    equal(resourceLoader._errors._messages, 'ERROR: Failed to get data from builder1,builder2.<br>ERROR: Data from staleBuilder1 is more than 1 day stale.<br>');
});


test('flattenTrie', 1, function() {
    resetGlobals();
    var tests = {
        'bar.html': {'results': [[100, 'F']], 'times': [[100, 0]]},
        'foo': {
            'bar': {
                'baz.html': {'results': [[100, 'F']], 'times': [[100, 0]]},
            }
        }
    };
    var expectedFlattenedTests = {
        'bar.html': {'results': [[100, 'F']], 'times': [[100, 0]]},
        'foo/bar/baz.html': {'results': [[100, 'F']], 'times': [[100, 0]]},
    };
    equal(JSON.stringify(loader.Loader._flattenTrie(tests)), JSON.stringify(expectedFlattenedTests))
});
