// Copyright (C) 2012 Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
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

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS
//////////////////////////////////////////////////////////////////////////////
var ALL = 'ALL';
var FORWARD = 'forward';
var BACKWARD = 'backward';
var GTEST_MODIFIERS = ['FLAKY', 'FAILS', 'MAYBE', 'DISABLED'];
var TEST_URL_BASE_PATH_TRAC = 'http://trac.webkit.org/browser/trunk/LayoutTests/';
var TEST_URL_BASE_PATH = "http://svn.webkit.org/repository/webkit/trunk/LayoutTests/";
var EXPECTATIONS_URL_BASE_PATH = TEST_URL_BASE_PATH + "platform/";
var TEST_RESULTS_BASE_PATH = 'http://build.chromium.org/f/chromium/layout_test_results/';
var GPU_RESULTS_BASE_PATH = 'http://chromium-browser-gpu-tests.commondatastorage.googleapis.com/runs/'

var PLATFORMS = {
    'APPLE': {
        subPlatforms: {
            'MAC': {
                expectationsDirectory: 'mac',
                subPlatforms: {
                    'LION': {
                        expectationsDirectory: 'mac-lion',
                        subPlatforms: {
                            'WK1': { fallbackPlatforms: ['APPLE_MAC_LION', 'APPLE_MAC'] },
                            'WK2': { fallbackPlatforms: ['APPLE_MAC_LION', 'APPLE_MAC', 'WK2'], expectationsDirectory: 'mac-wk2'}
                        }
                    },
                    'MOUNTAINLION': {
                        subPlatforms: {
                            'WK1': { fallbackPlatforms: ['APPLE_MAC_MOUNTAINLION', 'APPLE_MAC'] },
                            'WK2': { fallbackPlatforms: ['APPLE_MAC_MOUNTAINLION', 'APPLE_MAC', 'WK2'], expectationsDirectory: 'mac-wk2'}
                        }
                    },
                }
            },
            'WIN': {
                expectationsDirectory: 'win',
                subPlatforms: {
                    'XP': { fallbackPlatforms: ['APPLE_WIN'] },
                    'WIN7': { fallbackPlatforms: ['APPLE_WIN'] }
                }
            }
        }
    },
    'GTK': {
        expectationsDirectory: 'gtk',
        subPlatforms: {
            'LINUX': {
                subPlatforms: {
                    'WK1': { fallbackPlatforms: ['GTK'] },
                    'WK2': { fallbackPlatforms: ['GTK', 'WK2'], expectationsDirectory: 'gtk-wk2' }
                }
            }
        }
    },
    'QT': {
        expectationsDirectory: 'qt',
        subPlatforms: {
            'LINUX': { fallbackPlatforms: ['QT'] }
        }
    },
    'EFL': {
        expectationsDirectory: 'efl',
        subPlatforms: {
            'LINUX': {
                subPlatforms: {
                    'WK1': { fallbackPlatforms: ['EFL'], expectationsDirectory: 'efl-wk1' },
                    'WK2': { fallbackPlatforms: ['EFL', 'WK2'], expectationsDirectory: 'efl-wk2' }
                }
            }
        }
    },
    'WK2': {
        basePlatform: true,
        expectationsDirectory: 'wk2'
    }
};

var BUILD_TYPES = {'DEBUG': 'DBG', 'RELEASE': 'RELEASE'};
var MIN_SECONDS_FOR_SLOW_TEST = 4;
var MIN_SECONDS_FOR_SLOW_TEST_DEBUG = 2 * MIN_SECONDS_FOR_SLOW_TEST;
var FAIL_RESULTS = ['IMAGE', 'IMAGE+TEXT', 'TEXT', 'MISSING'];
var CHUNK_SIZE = 25;
var MAX_RESULTS = 1500;

// FIXME: Figure out how to make this not be hard-coded.
var VIRTUAL_SUITES = {
    'platform/chromium/virtual/gpu/fast/canvas': 'fast/canvas',
    'platform/chromium/virtual/gpu/canvas/philip': 'canvas/philip'
};

var resourceLoader;

function generatePage(historyInstance)
{
    if (historyInstance.crossDashboardState.useTestData)
        return;

    document.body.innerHTML = '<div id="loading-ui">LOADING...</div>';
    resourceLoader.showErrors();

    // tests expands to all tests that match the CSV list.
    // result expands to all tests that ever have the given result
    if (historyInstance.dashboardSpecificState.tests || historyInstance.dashboardSpecificState.result)
        generatePageForIndividualTests(individualTests());
    else if (historyInstance.dashboardSpecificState.expectationsUpdate)
        generatePageForExpectationsUpdate();
    else
        generatePageForBuilder(historyInstance.dashboardSpecificState.builder || currentBuilderGroup().defaultBuilder());

    for (var builder in currentBuilders())
        processTestResultsForBuilderAsync(builder);

    postHeightChangedMessage();
}

function handleValidHashParameter(historyInstance, key, value)
{
    switch(key) {
    case 'tests':
        history.validateParameter(historyInstance.dashboardSpecificState, key, value,
            function() {
                return string.isValidName(value);
            });
        return true;

    case 'result':
        value = value.toUpperCase();
        history.validateParameter(historyInstance.dashboardSpecificState, key, value,
            function() {
                for (var result in LAYOUT_TEST_EXPECTATIONS_MAP_) {
                    if (value == LAYOUT_TEST_EXPECTATIONS_MAP_[result])
                        return true;
                }
                return false;
            });
        return true;

    case 'builder':
        history.validateParameter(historyInstance.dashboardSpecificState, key, value,
            function() {
                return value in currentBuilders();
            });

        return true;

    case 'sortColumn':
        history.validateParameter(historyInstance.dashboardSpecificState, key, value,
            function() {
                // Get all possible headers since the actual used set of headers
                // depends on the values in historyInstance.dashboardSpecificState, which are currently being set.
                var headers = tableHeaders(true);
                for (var i = 0; i < headers.length; i++) {
                    if (value == sortColumnFromTableHeader(headers[i]))
                        return true;
                }
                return value == 'test' || value == 'builder';
            });
        return true;

    case 'sortOrder':
        history.validateParameter(historyInstance.dashboardSpecificState, key, value,
            function() {
                return value == FORWARD || value == BACKWARD;
            });
        return true;

    case 'resultsHeight':
    case 'updateIndex':
    case 'revision':
        history.validateParameter(historyInstance.dashboardSpecificState, key, Number(value),
            function() {
                return value.match(/^\d+$/);
            });
        return true;

    case 'showChrome':
    case 'showCorrectExpectations':
    case 'showWrongExpectations':
    case 'showExpectations':
    case 'showFlaky':
    case 'showLargeExpectations':
    case 'legacyExpectationsSemantics':
    case 'showSkipped':
    case 'showSlow':
    case 'showUnexpectedPasses':
    case 'showWontFixSkip':
    case 'expectationsUpdate':
        historyInstance.dashboardSpecificState[key] = value == 'true';
        return true;

    default:
        return false;
    }
}

// @param {Object} params New or modified query parameters as key: value.
function handleQueryParameterChange(historyInstance, params)
{
    for (key in params) {
        if (key == 'tests') {
            // Entering cross-builder view, only keep valid keys for that view.
            for (var currentKey in historyInstance.dashboardSpecificState) {
              if (isInvalidKeyForCrossBuilderView(currentKey)) {
                delete historyInstance.dashboardSpecificState[currentKey];
              }
            }
        } else if (isInvalidKeyForCrossBuilderView(key)) {
            delete historyInstance.dashboardSpecificState.tests;
            delete historyInstance.dashboardSpecificState.result;
        }
    }

    return true;
}

var defaultDashboardSpecificStateValues = {
    sortOrder: BACKWARD,
    sortColumn: 'flakiness',
    showExpectations: false,
    showFlaky: true,
    showLargeExpectations: false,
    legacyExpectationsSemantics: true,
    showChrome: true,
    showCorrectExpectations: false,
    showWrongExpectations: false,
    showWontFixSkip: false,
    showSlow: false,
    showSkipped: false,
    showUnexpectedPasses: false,
    expectationsUpdate: false,
    updateIndex: 0,
    resultsHeight: 300,
    revision: null,
    tests: '',
    result: '',
    builder: null
};

var DB_SPECIFIC_INVALIDATING_PARAMETERS = {
    'tests' : 'builder',
    'testType': 'builder',
    'group': 'builder'
};


var flakinessConfig = {
    defaultStateValues: defaultDashboardSpecificStateValues,
    generatePage: generatePage,
    handleValidHashParameter: handleValidHashParameter,
    handleQueryParameterChange: handleQueryParameterChange,
    invalidatingHashParameters: DB_SPECIFIC_INVALIDATING_PARAMETERS
};

// FIXME(jparent): Eventually remove all usage of global history object.
var g_history = new history.History(flakinessConfig);
g_history.parseCrossDashboardParameters();

//////////////////////////////////////////////////////////////////////////////
// GLOBALS
//////////////////////////////////////////////////////////////////////////////

var g_perBuilderPlatformAndBuildType = {};
var g_perBuilderFailures = {};
// Map of builder to arrays of tests that are listed in the expectations file
// but have for that builder.
var g_perBuilderWithExpectationsButNoFailures = {};
// Map of builder to arrays of paths that are skipped. This shows the raw
// path used in TestExpectations rather than the test path since we
// don't actually have any data here for skipped tests.
var g_perBuilderSkippedPaths = {};
// Maps test path to an array of {builder, testResults} objects.
var g_testToResultsMap = {};
// Tests that the user wants to update expectations for.
var g_confirmedTests = {};

function traversePlatformsTree(callback)
{
    function traverse(platformObject, parentPlatform) {
        Object.keys(platformObject).forEach(function(platformName) {
            var platform = platformObject[platformName];
            platformName = parentPlatform ? parentPlatform + platformName : platformName;

            if (platform.subPlatforms)
                traverse(platform.subPlatforms, platformName + '_');
            else if (!platform.basePlatform)
                callback(platform, platformName);
        });
    }
    traverse(PLATFORMS, null);
}

function createResultsObjectForTest(test, builder)
{
    return {
        test: test,
        builder: builder,
        // HTML for display of the results in the flakiness column
        html: '',
        flips: 0,
        slowestTime: 0,
        slowestNonTimeoutCrashTime: 0,
        meetsExpectations: true,
        isWontFixSkip: false,
        isFlaky: false,
        // Sorted string of missing expectations
        missing: '',
        // String of extra expectations (i.e. expectations that never occur).
        extra: '',
        modifiers: '',
        bugs: '',
        expectations : '',
        rawResults: '',
        // List of all the results the test actually has.
        actualResults: []
    };
}

function matchingElement(stringToMatch, elementsMap)
{
    for (var element in elementsMap) {
        if (string.contains(stringToMatch, elementsMap[element]))
            return element;
    }
}

function determineWKPlatform(builderName, basePlatform)
{
    var isWK2Builder = string.contains(builderName, 'WK2') || string.contains(builderName, 'WEBKIT2');
    return basePlatform + (isWK2Builder ? '_WK2' : '_WK1');
}

function determineBuilderPlatform(builderNameUpperCase)
{
    if (string.contains(builderNameUpperCase, 'WINDOWS 7'))
        return 'APPLE_WIN_WIN7';
    if (string.contains(builderNameUpperCase, 'WINDOWS XP'))
        return 'APPLE_WIN_XP';
    if (string.contains(builderNameUpperCase, 'QT LINUX'))
        return 'QT_LINUX';

    if (string.contains(builderNameUpperCase, 'MOUNTAINLION'))
        return determineWKPlatform(builderNameUpperCase, 'APPLE_MAC_MOUNTAINLION');
    if (string.contains(builderNameUpperCase, 'LION'))
        return determineWKPlatform(builderNameUpperCase, 'APPLE_MAC_LION');
    if (string.contains(builderNameUpperCase, 'GTK LINUX'))
        return determineWKPlatform(builderNameUpperCase, 'GTK_LINUX');
    if (string.contains(builderNameUpperCase, 'EFL'))
        return determineWKPlatform(builderNameUpperCase, 'EFL_LINUX');
}

function platformAndBuildType(builderName)
{
    if (!g_perBuilderPlatformAndBuildType[builderName]) {
        var builderNameUpperCase = builderName.toUpperCase();
        var platform = determineBuilderPlatform(builderNameUpperCase);
        if (!platform)
            console.error('Could not resolve platform for builder: ' + builderName);

        var buildType = string.contains(builderNameUpperCase, 'DEBUG') ? 'DEBUG' : 'RELEASE';
        g_perBuilderPlatformAndBuildType[builderName] = {platform: platform, buildType: buildType};
    }
    return g_perBuilderPlatformAndBuildType[builderName];
}

function isDebug(builderName)
{
    return platformAndBuildType(builderName).buildType == 'DEBUG';
}

// Returns the expectation string for the given single character result.
// This string should match the expectations that are put into
// test_expectations.py.
//
// For example, if we start explicitly listing IMAGE result failures,
// this function should start returning 'IMAGE'.
function expectationsFileStringForResult(result)
{
    // For the purposes of comparing against the expecations of a test,
    // consider simplified diff failures as just text failures since
    // the test_expectations file doesn't treat them specially.
    if (result == 'S')
        return 'TEXT';

    if (result == 'N')
        return '';

    return expectationsMap()[result];
}

var TestTrie = function(builders, resultsByBuilder)
{
    this._trie = {};

    for (var builder in builders) {
        var testsForBuilder = resultsByBuilder[builder].tests;
        for (var test in testsForBuilder)
            this._addTest(test.split('/'), this._trie);
    }
}

TestTrie.prototype.forEach = function(callback, startingTriePath)
{
    var testsTrie = this._trie;
    if (startingTriePath) {
        var splitPath = startingTriePath.split('/');
        while (splitPath.length && testsTrie)
            testsTrie = testsTrie[splitPath.shift()];
    }

    if (!testsTrie)
        return;

    function traverse(trie, triePath) {
        if (trie == true)
            callback(triePath);
        else {
            for (var member in trie)
                traverse(trie[member], triePath ? triePath + '/' + member : member);
        }
    }
    traverse(testsTrie, startingTriePath);
}

TestTrie.prototype._addTest = function(test, trie)
{
    var rootComponent = test.shift();
    if (!test.length) {
        if (!trie[rootComponent])
            trie[rootComponent] = true;
        return;
    }

    if (!trie[rootComponent] || trie[rootComponent] == true)
        trie[rootComponent] = {};
    this._addTest(test, trie[rootComponent]);
}

// Map of all tests to true values. This is just so we can have the list of
// all tests across all the builders.
var g_allTestsTrie;

function getAllTestsTrie()
{
    if (!g_allTestsTrie)
        g_allTestsTrie = new TestTrie(currentBuilders(), g_resultsByBuilder);

    return g_allTestsTrie;
}

// Returns an array of tests to be displayed in the individual tests view.
// Note that a directory can be listed as a test, so we expand that into all
// tests in the directory.
function individualTests()
{
    if (g_history.dashboardSpecificState.result)
        return allTestsWithResult(g_history.dashboardSpecificState.result);

    if (!g_history.dashboardSpecificState.tests)
        return [];

    return individualTestsForSubstringList();
}

function substringList()
{
    // Convert windows slashes to unix slashes.
    var tests = g_history.dashboardSpecificState.tests.replace(/\\/g, '/');
    var separator = string.contains(tests, ' ') ? ' ' : ',';
    var testList = tests.split(separator);

    if (g_history.isLayoutTestResults())
        return testList;

    var testListWithoutModifiers = [];
    testList.forEach(function(path) {
        GTEST_MODIFIERS.forEach(function(modifier) {
            path = path.replace('.' + modifier + '_', '.');
        });
        testListWithoutModifiers.push(path);
    });
    return testListWithoutModifiers;
}

function individualTestsForSubstringList()
{
    var testList = substringList();

    // Put the tests into an object first and then move them into an array
    // as a way of deduping.
    var testsMap = {};
    for (var i = 0; i < testList.length; i++) {
        var path = testList[i];

        // Ignore whitespace entries as they'd match every test.
        if (path.match(/^\s*$/))
            continue;

        var hasAnyMatches = false;
        getAllTestsTrie().forEach(function(triePath) {
            if (string.caseInsensitiveContains(triePath, path)) {
                testsMap[triePath] = 1;
                hasAnyMatches = true;
            }
        });

        // If a path doesn't match any tests, then assume it's a full path
        // to a test that passes on all builders.
        if (!hasAnyMatches)
            testsMap[path] = 1;
    }

    var testsArray = [];
    for (var test in testsMap)
        testsArray.push(test);
    return testsArray;
}

// Returns whether this test's slowest time is above the cutoff for
// being a slow test.
function isSlowTest(resultsForTest)
{
    var maxTime = isDebug(resultsForTest.builder) ? MIN_SECONDS_FOR_SLOW_TEST_DEBUG : MIN_SECONDS_FOR_SLOW_TEST;
    return resultsForTest.slowestNonTimeoutCrashTime > maxTime;
}

// Returns whether this test's slowest time is *well* below the cutoff for
// being a slow test.
function isFastTest(resultsForTest)
{
    var maxTime = isDebug(resultsForTest.builder) ? MIN_SECONDS_FOR_SLOW_TEST_DEBUG : MIN_SECONDS_FOR_SLOW_TEST;
    return resultsForTest.slowestNonTimeoutCrashTime < maxTime / 2;
}

function allTestsWithResult(result)
{
    processTestRunsForAllBuilders();
    var retVal = [];

    getAllTestsTrie().forEach(function(triePath) {
        for (var i = 0; i < g_testToResultsMap[triePath].length; i++) {
            if (g_testToResultsMap[triePath][i].actualResults.indexOf(result) != -1) {
                retVal.push(triePath);
                break;
            }
        }
    });

    return retVal;
}


// Adds all the tests for the given builder to the testMapToPopulate.
function addTestsForBuilder(builder, testMapToPopulate)
{
    var tests = g_resultsByBuilder[builder].tests;
    for (var test in tests) {
        testMapToPopulate[test] = true;
    }
}

// Map of all tests to true values by platform and build type.
// e.g. g_allTestsByPlatformAndBuildType['XP']['DEBUG'] will have the union
// of all tests run on the xp-debug builders.
var g_allTestsByPlatformAndBuildType = {};
traversePlatformsTree(function(platform, platformName) {
    g_allTestsByPlatformAndBuildType[platformName] = {};
});

// Map of all tests to true values by platform and build type.
// e.g. g_allTestsByPlatformAndBuildType['WIN']['DEBUG'] will have the union
// of all tests run on the win-debug builders.
function allTestsWithSamePlatformAndBuildType(platform, buildType)
{
    if (!g_allTestsByPlatformAndBuildType[platform][buildType]) {
        var tests = {};
        for (var thisBuilder in currentBuilders()) {
            var thisBuilderBuildInfo = platformAndBuildType(thisBuilder);
            if (thisBuilderBuildInfo.buildType == buildType && thisBuilderBuildInfo.platform == platform) {
                addTestsForBuilder(thisBuilder, tests);
            }
        }
        g_allTestsByPlatformAndBuildType[platform][buildType] = tests;
    }

    return g_allTestsByPlatformAndBuildType[platform][buildType];
}

function getExpectations(test, platform, buildType)
{
    var testObject = g_allExpectations[test];
    if (!testObject)
        return null;

    var platformObject = testObject[platform];
    if (!platformObject)
        return null;
        
    return platformObject[buildType];
}

function filterBugs(modifiers)
{
    var bugs = modifiers.match(/\b(Bug|webkit.org|crbug.com|code.google.com)\S*/g);
    if (!bugs)
        return {bugs: '', modifiers: modifiers};
    for (var j = 0; j < bugs.length; j++)
        modifiers = modifiers.replace(bugs[j], '');
    return {bugs: bugs.join(' '), modifiers: string.collapseWhitespace(string.trimString(modifiers))};
}

function populateExpectationsData(resultsObject)
{
    var buildInfo = platformAndBuildType(resultsObject.builder);
    var expectations = getExpectations(resultsObject.test, buildInfo.platform, buildInfo.buildType);
    if (!expectations)
        return;

    resultsObject.expectations = expectations.expectations;
    var filteredModifiers = filterBugs(expectations.modifiers);
    resultsObject.modifiers = filteredModifiers.modifiers;
    resultsObject.bugs = filteredModifiers.bugs;
    resultsObject.isWontFixSkip = string.contains(expectations.modifiers, 'WONTFIX') || string.contains(expectations.modifiers, 'SKIP'); 
}

function platformObjectForName(platformName)
{
    var platformsList = platformName.split("_");
    var platformObject = PLATFORMS[platformsList.shift()];
    platformsList.forEach(function(platformName) {
        platformObject = platformObject.subPlatforms[platformName];
    });
    return platformObject;
}

// Data structure to hold the processed expectations.
// g_allExpectations[testPath][platform][buildType] gets the object that has
// expectations and modifiers properties for this platform/buildType.
//
// platform and buildType both go through fallback sets of keys from most
// specific key to least specific. For example, on Windows XP, we first
// check the platform WIN-XP, if there's no such object, we check WIN,
// then finally we check ALL. For build types, we check the current
// buildType, then ALL.
var g_allExpectations;

function getParsedExpectations(data)
{
    var expectations = [];
    var lines = data.split('\n');
    lines.forEach(function(line) {
        line = string.trimString(line);
        if (!line || string.startsWith(line, '#'))
            return;

        // This code mimics _tokenize_line_using_new_format() in
        // Tools/Scripts/webkitpy/layout_tests/models/test_expectations.py
        //
        // FIXME: consider doing more error checking here.
        //
        // FIXME: Clean this all up once we've fully cut over to the new syntax.
        var tokens = line.split(/\s+/)
        var parsed_bugs = [];
        var parsed_modifiers = [];
        var parsed_path;
        var parsed_expectations = [];
        var state = 'start';

        // This clones _configuration_tokens_list in test_expectations.py.
        // FIXME: unify with the platforms constants at the top of the file.
        var configuration_tokens = {
            'Release': 'RELEASE',
            'Debug': 'DEBUG',
            'Mac': 'MAC',
            'Win': 'WIN',
            'Linux': 'LINUX',
            'SnowLeopard': 'SNOWLEOPARD',
            'Lion': 'LION',
            'MountainLion': 'MOUNTAINLION',
            'Win7': 'WIN7',
            'XP': 'XP',
            'Vista': 'VISTA',
            'Android': 'ANDROID',
        };

        var expectation_tokens = {
            'Crash': 'CRASH',
            'Failure': 'FAIL',
            'ImageOnlyFailure': 'IMAGE',
            'Missing': 'MISSING',
            'Pass': 'PASS',
            'Rebaseline': 'REBASELINE',
            'Skip': 'SKIP',
            'Slow': 'SLOW',
            'Timeout': 'TIMEOUT',
            'WontFix': 'WONTFIX',
        };

            
        tokens.forEach(function(token) {
          if (token.indexOf('Bug') != -1 ||
              token.indexOf('webkit.org') != -1 ||
              token.indexOf('crbug.com') != -1 ||
              token.indexOf('code.google.com') != -1) {
              parsed_bugs.push(token);
          } else if (token == '[') {
              if (state == 'start') {
                  state = 'configuration';
              } else if (state == 'name_found') {
                  state = 'expectations';
              }
          } else if (token == ']') {
              if (state == 'configuration') {
                  state = 'name';
              } else if (state == 'expectations') {
                  state = 'done';
              }
          } else if (state == 'configuration') {
              parsed_modifiers.push(configuration_tokens[token]);
          } else if (state == 'expectations') {
              if (token == 'Rebaseline' || token == 'Skip' || token == 'Slow' || token == 'WontFix') {
                  parsed_modifiers.push(token.toUpperCase());
              } else {
                  parsed_expectations.push(expectation_tokens[token]);
              }
          } else if (token == '#') {
              state = 'done';
          } else if (state == 'name' || state == 'start') {
              parsed_path = token;
              state = 'name_found';
          }
        });

        if (!parsed_expectations.length) {
            if (parsed_modifiers.indexOf('Slow') == -1) {
                parsed_modifiers.push('Skip');
                parsed_expectations = ['Pass'];
            }
        }

        // FIXME: Should we include line number and comment lines here?
        expectations.push({
            modifiers: parsed_bugs.concat(parsed_modifiers).join(' '),
            path: parsed_path,
            expectations: parsed_expectations.join(' '),
        });
    });
    return expectations;
}


function addTestToAllExpectationsForPlatform(test, platformName, expectations, modifiers)
{
    if (!g_allExpectations[test])
        g_allExpectations[test] = {};

    if (!g_allExpectations[test][platformName])
        g_allExpectations[test][platformName] = {};

    var allBuildTypes = [];
    modifiers.split(' ').forEach(function(modifier) {
        if (modifier in BUILD_TYPES) {
            allBuildTypes.push(modifier);
            return;
        }
    });
    if (!allBuildTypes.length)
        allBuildTypes = Object.keys(BUILD_TYPES);

    allBuildTypes.forEach(function(buildType) {
        g_allExpectations[test][platformName][buildType] = {modifiers: modifiers, expectations: expectations};
    });
}

function processExpectationsForPlatform(platformObject, platformName, expectationsArray)
{
    if (!g_allExpectations)
        g_allExpectations = {};

    if (!expectationsArray)
        return;

    // Sort the array to hit more specific paths last. More specific
    // paths (e.g. foo/bar/baz.html) override entries for less-specific ones (e.g. foo/bar).
    expectationsArray.sort(alphanumericCompare('path'));

    for (var i = 0; i < expectationsArray.length; i++) {
        var path = expectationsArray[i].path;
        var modifiers = expectationsArray[i].modifiers;
        var expectations = expectationsArray[i].expectations;

        getAllTestsTrie().forEach(function(triePath) {
            addTestToAllExpectationsForPlatform(triePath, platformName, expectations, modifiers);
        }, path);
    }
}

function processExpectations()
{
    // FIXME: An expectations-by-platform object should be passed into this function rather than checking
    // for a global object. That way this function can be better tested and meaningful errors can
    // be reported when expectations for a given platform are not found in that object.
    if (!g_expectationsByPlatform)
        return;

    traversePlatformsTree(function(platform, platformName) {
        if (platform.fallbackPlatforms) {
            platform.fallbackPlatforms.forEach(function(fallbackPlatform) {
                if (fallbackPlatform in g_expectationsByPlatform)
                    processExpectationsForPlatform(platform, platformName, g_expectationsByPlatform[fallbackPlatform]);
            });
        }

        if (platformName in g_expectationsByPlatform)
            processExpectationsForPlatform(platform, platformName, g_expectationsByPlatform[platformName]);
    });

    g_expectationsByPlatform = undefined;
}

function processMissingTestsWithExpectations(builder, platform, buildType)
{
    var noFailures = [];
    var skipped = [];

    var allTestsForPlatformAndBuildType = allTestsWithSamePlatformAndBuildType(platform, buildType);
    for (var test in g_allExpectations) {
        var expectations = getExpectations(test, platform, buildType);

        if (!expectations)
            continue;

        // Test has expectations, but no result in the builders results.
        // This means it's either SKIP or passes on all builds.
        if (!allTestsForPlatformAndBuildType[test] && !string.contains(expectations.modifiers, 'WONTFIX')) {
            if (string.contains(expectations.modifiers, 'SKIP'))
                skipped.push(test);
            else if (!expectations.expectations.match(/^\s*PASS\s*$/)) {
                // Don't show tests expected to always pass. This is used in ways like
                // the following:
                // foo/bar = FAIL
                // foo/bar/baz.html = PASS
                noFailures.push({test: test, expectations: expectations.expectations, modifiers: expectations.modifiers});
            }
        }
    }

    g_perBuilderSkippedPaths[builder] = skipped.sort();
    g_perBuilderWithExpectationsButNoFailures[builder] = noFailures.sort();
}

function processTestResultsForBuilderAsync(builder)
{
    setTimeout(function() { processTestRunsForBuilder(builder); }, 0);
}

function processTestRunsForAllBuilders()
{
    for (var builder in currentBuilders())
        processTestRunsForBuilder(builder);
}

function processTestRunsForBuilder(builderName)
{
    if (g_perBuilderFailures[builderName])
      return;

    if (!g_resultsByBuilder[builderName]) {
        console.error('No tests found for ' + builderName);
        g_perBuilderFailures[builderName] = [];
        return;
    }

    processExpectations();
   
    var buildInfo = platformAndBuildType(builderName);
    var platform = buildInfo.platform;
    var buildType = buildInfo.buildType;
    processMissingTestsWithExpectations(builderName, platform, buildType);

    var failures = [];
    var allTestsForThisBuilder = g_resultsByBuilder[builderName].tests;

    for (var test in allTestsForThisBuilder) {
        var resultsForTest = createResultsObjectForTest(test, builderName);
        populateExpectationsData(resultsForTest);

        var rawTest = g_resultsByBuilder[builderName].tests[test];
        resultsForTest.rawTimes = rawTest.times;
        var rawResults = rawTest.results;
        resultsForTest.rawResults = rawResults;

        // FIXME: Switch to resultsByBuild
        var times = resultsForTest.rawTimes;
        var numTimesSeen = 0;
        var numResultsSeen = 0;
        var resultsIndex = 0;
        var currentResult;
        for (var i = 0; i < times.length; i++) {
            numTimesSeen += times[i][RLE.LENGTH];

            while (rawResults[resultsIndex] && numTimesSeen > (numResultsSeen + rawResults[resultsIndex][RLE.LENGTH])) {
                numResultsSeen += rawResults[resultsIndex][RLE.LENGTH];
                resultsIndex++;
            }

            if (rawResults && rawResults[resultsIndex])
                currentResult = rawResults[resultsIndex][RLE.VALUE];

            var time = times[i][RLE.VALUE]

            // Ignore times for crashing/timeout runs for the sake of seeing if
            // a test should be marked slow.
            if (currentResult != 'C' && currentResult != 'T')
                resultsForTest.slowestNonTimeoutCrashTime = Math.max(resultsForTest.slowestNonTimeoutCrashTime, time);
            resultsForTest.slowestTime = Math.max(resultsForTest.slowestTime, time);
        }

        processMissingAndExtraExpectations(resultsForTest);
        failures.push(resultsForTest);

        if (!g_testToResultsMap[test])
            g_testToResultsMap[test] = [];
        g_testToResultsMap[test].push(resultsForTest);
    }

    g_perBuilderFailures[builderName] = failures;
}

function processMissingAndExtraExpectations(resultsForTest)
{
    // Heuristic for determining whether expectations apply to a given test:
    // -If a test result happens < MIN_RUNS_FOR_FLAKE, then consider it a flaky
    // result and include it in the list of expected results.
    // -Otherwise, grab the first contiguous set of runs with the same result
    // for >= MIN_RUNS_FOR_FLAKE and ignore all following runs >=
    // MIN_RUNS_FOR_FLAKE.
    // This lets us rule out common cases of a test changing expectations for
    // a few runs, then being fixed or otherwise modified in a non-flaky way.
    var rawResults = resultsForTest.rawResults;

    // If the first result is no-data that means the test is skipped or is
    // being run on a different builder (e.g. moved from one shard to another).
    // Ignore these results since we have no real data about what's going on.
    if (rawResults[0][RLE.VALUE] == 'N')
        return;

    // Only consider flake if it doesn't happen twice in a row.
    var MIN_RUNS_FOR_FLAKE = 2;
    var resultsMap = {}
    var numResultsSeen = 0;
    var haveSeenNonFlakeResult = false;
    var numRealResults = 0;

    var seenResults = {};
    for (var i = 0; i < rawResults.length; i++) {
        var numResults = rawResults[i][RLE.LENGTH];
        numResultsSeen += numResults;

        var result = rawResults[i][RLE.VALUE];

        var hasMinRuns = numResults >= MIN_RUNS_FOR_FLAKE;
        if (haveSeenNonFlakeResult && hasMinRuns)
            continue;
        else if (hasMinRuns)
            haveSeenNonFlakeResult = true;
        else if (!seenResults[result]) {
            // Only consider a short-lived result if we've seen it more than once.
            // Otherwise, we include lots of false-positives due to tests that fail
            // for a couple runs and then start passing.
            seenResults[result] = true;
            continue;
        }

        var expectation = expectationsFileStringForResult(result);
        resultsMap[expectation] = true;
        numRealResults++;
    }

    resultsForTest.flips = i - 1;
    resultsForTest.isFlaky = numRealResults > 1;

    var missingExpectations = [];
    var extraExpectations = [];

    if (g_history.isLayoutTestResults()) {
        var expectationsArray = resultsForTest.expectations ? resultsForTest.expectations.split(' ') : [];
        extraExpectations = expectationsArray.filter(
            function(element) {
                // FIXME: Once all the FAIL lines are removed from
                // TestExpectations, delete all the legacyExpectationsSemantics
                // code.
                if (g_history.dashboardSpecificState.legacyExpectationsSemantics) {
                    if (element == 'FAIL') {
                        for (var i = 0; i < FAIL_RESULTS.length; i++) {
                            if (resultsMap[FAIL_RESULTS[i]])
                                return false;
                        }
                        return true;
                    }
                }

                return element && !resultsMap[element] && !string.contains(element, 'BUG');
            });

        for (var result in resultsMap) {
            resultsForTest.actualResults.push(result);
            var hasExpectation = false;
            for (var i = 0; i < expectationsArray.length; i++) {
                var expectation = expectationsArray[i];
                // FIXME: Once all the FAIL lines are removed from
                // TestExpectations, delete all the legacyExpectationsSemantics
                // code.
                if (g_history.dashboardSpecificState.legacyExpectationsSemantics) {
                    if (expectation == 'FAIL') {
                        for (var j = 0; j < FAIL_RESULTS.length; j++) {
                            if (result == FAIL_RESULTS[j]) {
                                hasExpectation = true;
                                break;
                            }
                        }
                    }
                }

                if (result == expectation)
                    hasExpectation = true;

                if (hasExpectation)
                    break;
            }
            // If we have no expectations for a test and it only passes, then don't
            // list PASS as a missing expectation. We only want to list PASS if it
            // flaky passes, so there would be other expectations.
            if (!hasExpectation && !(!expectationsArray.length && result == 'PASS' && numRealResults == 1))
                missingExpectations.push(result);
        }

        // Only highlight tests that take > 2 seconds as needing to be marked as
        // slow. There are too many tests that take ~2 seconds every couple
        // hundred runs. It's not worth the manual maintenance effort.
        // Also, if a test times out, then it should not be marked as slow.
        var minTimeForNeedsSlow = isDebug(resultsForTest.builder) ? 2 : 1;
        if (isSlowTest(resultsForTest) && !resultsMap['TIMEOUT'] && (!resultsForTest.modifiers || !string.contains(resultsForTest.modifiers, 'SLOW')))
            missingExpectations.push('SLOW');
        else if (isFastTest(resultsForTest) && resultsForTest.modifiers && string.contains(resultsForTest.modifiers, 'SLOW'))
            extraExpectations.push('SLOW');

        // If there are no missing results or modifiers besides build
        // type, platform, or bug and the expectations are all extra
        // that is, extraExpectations - expectations = PASS,
        // include PASS as extra, since that means this line in
        // test_expectations can be deleted..
        if (!missingExpectations.length && !(resultsForTest.modifiers && realModifiers(resultsForTest.modifiers))) {
            var extraPlusPass = extraExpectations.concat(['PASS']);
            if (extraPlusPass.sort().toString() == expectationsArray.slice(0).sort().toString())
                extraExpectations.push('PASS');
        }

    }

    resultsForTest.meetsExpectations = !missingExpectations.length && !extraExpectations.length;
    resultsForTest.missing = missingExpectations.sort().join(' ');
    resultsForTest.extra = extraExpectations.sort().join(' ');
}


var BUG_URL_PREFIX = '<a href="http://';
var BUG_URL_POSTFIX = '/$1">crbug.com/$1</a> ';
var WEBKIT_BUG_URL_POSTFIX = '/$1">webkit.org/b/$1</a> ';
var INTERNAL_BUG_REPLACE_VALUE = BUG_URL_PREFIX + 'b' + BUG_URL_POSTFIX;
var EXTERNAL_BUG_REPLACE_VALUE = BUG_URL_PREFIX + 'crbug.com' + BUG_URL_POSTFIX;
var WEBKIT_BUG_REPLACE_VALUE = BUG_URL_PREFIX + 'webkit.org/b' + WEBKIT_BUG_URL_POSTFIX;

function htmlForBugs(bugs)
{
    bugs = bugs.replace(/crbug.com\/(\d+)(\ |$)/g, EXTERNAL_BUG_REPLACE_VALUE);
    bugs = bugs.replace(/webkit.org\/b\/(\d+)(\ |$)/g, WEBKIT_BUG_REPLACE_VALUE);
    return bugs;
}

function linkHTMLToOpenWindow(url, text)
{
    return '<a href="' + url + '" target="_blank">' + text + '</a>';
}

// FIXME: replaced with ui.html.chromiumRevisionLink/ui.html.webKitRevisionLink
function createBlameListHTML(revisions, index, urlBase, separator, repo)
{
    var thisRevision = revisions[index];
    if (!thisRevision)
        return '';

    var previousRevision = revisions[index + 1];
    if (previousRevision && previousRevision != thisRevision) {
        previousRevision++;
        return linkHTMLToOpenWindow(urlBase + thisRevision + separator + previousRevision,
            repo + ' blamelist r' + previousRevision + ':r' + thisRevision);
    } else
        return 'At ' + repo + ' revision: ' + thisRevision;
}

// Returns whether the result for index'th result for testName on builder was
// a failure.
function isFailure(builder, testName, index)
{
    var currentIndex = 0;
    var rawResults = g_resultsByBuilder[builder].tests[testName].results;
    for (var i = 0; i < rawResults.length; i++) {
        currentIndex += rawResults[i][RLE.LENGTH];
        if (currentIndex > index)
            return isFailingResult(rawResults[i][RLE.VALUE]);
    }
    console.error('Index exceeds number of results: ' + index);
}

// Returns an array of indexes for all builds where this test failed.
function indexesForFailures(builder, testName)
{
    var rawResults = g_resultsByBuilder[builder].tests[testName].results;
    var buildNumbers = g_resultsByBuilder[builder].buildNumbers;
    var index = 0;
    var failures = [];
    for (var i = 0; i < rawResults.length; i++) {
        var numResults = rawResults[i][RLE.LENGTH];
        if (isFailingResult(rawResults[i][RLE.VALUE])) {
            for (var j = 0; j < numResults; j++)
                failures.push(index + j);
        }
        index += numResults;
    }
    return failures;
}

// Returns the path to the failure log for this non-webkit test.
function pathToFailureLog(testName)
{
    return '/steps/' + g_history.crossDashboardState.testType + '/logs/' + testName.split('.')[1]
}

function showPopupForBuild(e, builder, index, opt_testName)
{
    var html = '';

    var time = g_resultsByBuilder[builder].secondsSinceEpoch[index];
    if (time) {
        var date = new Date(time * 1000);
        html += date.toLocaleDateString() + ' ' + date.toLocaleTimeString();
    }

    var buildNumber = g_resultsByBuilder[builder].buildNumbers[index];
    var master = builderMaster(builder);
    var buildBasePath = master.logPath(builder, buildNumber);

    html += '<ul><li>' + linkHTMLToOpenWindow(buildBasePath, 'Build log') +
        '</li><li>' +
        createBlameListHTML(g_resultsByBuilder[builder].webkitRevision, index,
            'http://trac.webkit.org/log/?verbose=on&rev=', '&stop_rev=',
            'WebKit') +
        '</li>';

    if (master.name == WEBKIT_BUILDER_MASTER) {
        var revision = g_resultsByBuilder[builder].webkitRevision[index];
        html += '<li><span class=link onclick="g_history.setQueryParameter(\'revision\',' +
            revision + ')">Show results for WebKit r' + revision +
            '</span></li>';
    } else {
        html += '<li>' +
            createBlameListHTML(g_resultsByBuilder[builder].chromeRevision, index,
                'http://build.chromium.org/f/chromium/perf/dashboard/ui/changelog.html?url=/trunk/src&mode=html&range=', ':', 'Chrome') +
            '</li>';

        var chromeRevision = g_resultsByBuilder[builder].chromeRevision[index];
        if (chromeRevision && g_history.isLayoutTestResults()) {
            html += '<li><a href="' + TEST_RESULTS_BASE_PATH + currentBuilders()[builder] +
                '/' + chromeRevision + '/layout-test-results.zip">layout-test-results.zip</a></li>';
        }
    }

    if (!g_history.isLayoutTestResults() && opt_testName && isFailure(builder, opt_testName, index))
        html += '<li>' + linkHTMLToOpenWindow(buildBasePath + pathToFailureLog(opt_testName), 'Failure log') + '</li>';

    html += '</ul>';
    ui.popup.show(e.target, html);
}

function htmlForTestResults(test)
{
    var html = '';
    var results = test.rawResults.concat();
    var times = test.rawTimes.concat();
    var builder = test.builder;
    var master = builderMaster(builder);
    var buildNumbers = g_resultsByBuilder[builder].buildNumbers;

    var indexToReplaceCurrentResult = -1;
    var indexToReplaceCurrentTime = -1;
    var currentResultArray, currentTimeArray, currentResult, innerHTML, resultString;
    for (var i = 0; i < buildNumbers.length; i++) {
        if (i > indexToReplaceCurrentResult) {
            currentResultArray = results.shift();
            if (currentResultArray) {
                currentResult = currentResultArray[RLE.VALUE];
                // Treat simplified diff failures as just text failures.
                if (currentResult == 'S')
                    currentResult = 'F';
                indexToReplaceCurrentResult += currentResultArray[RLE.LENGTH];
            } else {
                currentResult = 'N';
                indexToReplaceCurrentResult += buildNumbers.length;
            }
            resultString = expectationsFileStringForResult(currentResult);
        }

        if (i > indexToReplaceCurrentTime) {
            currentTimeArray = times.shift();
            var currentTime = 0;
            if (currentResultArray) {
              currentTime = currentTimeArray[RLE.VALUE];
              indexToReplaceCurrentTime += currentTimeArray[RLE.LENGTH];
            } else
              indexToReplaceCurrentTime += buildNumbers.length;

            innerHTML = currentTime || '&nbsp;';
        }

        var extraClassNames = '';
        var webkitRevision = g_resultsByBuilder[builder].webkitRevision;
        var isWebkitMerge = webkitRevision[i + 1] && webkitRevision[i] != webkitRevision[i + 1];
        if (isWebkitMerge && master.name != WEBKIT_BUILDER_MASTER)
            extraClassNames += ' merge';

        html += '<td title="' + (resultString || 'NO DATA') + '. Click for more info." class="results ' + currentResult +
          extraClassNames + '" onclick=\'showPopupForBuild(event, "' + builder + '",' + i + ',"' + test.test + '")\'>' + innerHTML;
    }
    return html;
}

function htmlForTestsWithExpectationsButNoFailures(builder)
{
    var tests = g_perBuilderWithExpectationsButNoFailures[builder];
    var skippedPaths = g_perBuilderSkippedPaths[builder];
    var showUnexpectedPassesLink =  linkHTMLToToggleState('showUnexpectedPasses', 'tests that have not failed in last ' + g_resultsByBuilder[builder].buildNumbers.length + ' runs');
    var showSkippedLink = linkHTMLToToggleState('showSkipped', 'skipped tests in TestExpectations');
    
    var html = '';
    if (g_history.isLayoutTestResults() && (tests.length || skippedPaths.length)) {
        var buildInfo = platformAndBuildType(builder);
        html += '<h2 style="display:inline-block">Expectations for ' + buildInfo.platform + '-' + buildInfo.buildType + '</h2> ';
        if (!g_history.dashboardSpecificState.showUnexpectedPasses && tests.length)
            html += showUnexpectedPassesLink;
        html += ' ';
        if (!g_history.dashboardSpecificState.showSkipped && skippedPaths.length)
            html += showSkippedLink;
    }

    var open = '<div onclick="selectContents(this)">';

    if (g_history.dashboardSpecificState.showUnexpectedPasses && tests.length) {
        html += '<div id="passing-tests">' + showUnexpectedPassesLink;
        for (var i = 0; i < tests.length; i++)
            html += open + tests[i].test + '</div>';
        html += '</div>';
    }

    if (g_history.dashboardSpecificState.showSkipped && skippedPaths.length)
        html += '<div id="skipped-tests">' + showSkippedLink + open + skippedPaths.join('</div>' + open) + '</div></div>';
    return html + '<br>';
}

// Returns whether we should exclude test results from the test table.
function shouldHideTest(testResult)
{
    // For non-layout tests, we always show everything.
    if (!g_history.isLayoutTestResults())
        return false;

    if (testResult.isWontFixSkip)
        return !g_history.dashboardSpecificState.showWontFixSkip;

    if (testResult.isFlaky)
        return !g_history.dashboardSpecificState.showFlaky;

    if (isSlowTest(testResult))
        return !g_history.dashboardSpecificState.showSlow;

    if (testResult.meetsExpectations)
        return !g_history.dashboardSpecificState.showCorrectExpectations;

    return !g_history.dashboardSpecificState.showWrongExpectations;
}

// Sets the browser's selection to the element's contents.
function selectContents(element)
{
    window.getSelection().selectAllChildren(element);
}

function createBugHTML(test)
{
    var symptom = test.isFlaky ? 'flaky' : 'failing';
    var title = encodeURIComponent('Layout Test ' + test.test + ' is ' + symptom);
    var description = encodeURIComponent('The following layout test is ' + symptom + ' on ' +
        '[insert platform]\n\n' + test.test + '\n\nProbable cause:\n\n' +
        '[insert probable cause]');
    
    var component = encodeURIComponent('Tools / Tests');
    url = 'https://bugs.webkit.org/enter_bug.cgi?assigned_to=webkit-unassigned%40lists.webkit.org&product=WebKit&form_name=enter_bug&component=' + component + '&short_desc=' + title + '&comment=' + description;
    return '<a href="' + url + '" class="file-bug">FILE BUG</a>';
}

function isCrossBuilderView()
{
    return g_history.dashboardSpecificState.tests || g_history.dashboardSpecificState.result || g_history.dashboardSpecificState.expectationsUpdate;
}

function tableHeaders(opt_getAll)
{
    var headers = [];
    if (isCrossBuilderView() || opt_getAll)
        headers.push('builder');

    if (!isCrossBuilderView() || opt_getAll)
        headers.push('test');

    if (g_history.isLayoutTestResults() || opt_getAll)
        headers.push('bugs', 'modifiers', 'expectations');

    headers.push('slowest run', 'flakiness (numbers are runtimes in seconds)');
    return headers;
}

function htmlForSingleTestRow(test)
{
    if (!isCrossBuilderView() && shouldHideTest(test)) {
        // The innerHTML call is considerably faster if we exclude the rows for
        // items we're not showing than if we hide them using display:none.
        // For the crossBuilderView, we want to show all rows the user is
        // explicitly listing tests to view.
        return '';
    }

    var headers = tableHeaders();
    var html = '';
    for (var i = 0; i < headers.length; i++) {
        var header = headers[i];
        if (string.startsWith(header, 'test') || string.startsWith(header, 'builder')) {
            // If isCrossBuilderView() is true, we're just viewing a single test
            // with results for many builders, so the first column is builder names
            // instead of test paths.
            var testCellClassName = 'test-link' + (isCrossBuilderView() ? ' builder-name' : '');
            var testCellHTML = isCrossBuilderView() ? test.builder : '<span class="link" onclick="g_history.setQueryParameter(\'tests\',\'' + test.test +'\');">' + test.test + '</span>';

            html += '<tr><td class="' + testCellClassName + '">' + testCellHTML;
        } else if (string.startsWith(header, 'bugs'))
            html += '<td class=options-container>' + (test.bugs ? htmlForBugs(test.bugs) : createBugHTML(test));
        else if (string.startsWith(header, 'modifiers'))
            html += '<td class=options-container>' + test.modifiers;
        else if (string.startsWith(header, 'expectations'))
            html += '<td class=options-container>' + test.expectations;
        else if (string.startsWith(header, 'slowest'))
            html += '<td>' + (test.slowestTime ? test.slowestTime + 's' : '');
        else if (string.startsWith(header, 'flakiness'))
            html += htmlForTestResults(test);
    }
    return html;
}

function sortColumnFromTableHeader(headerText)
{
    return headerText.split(' ', 1)[0];
}

function htmlForTableColumnHeader(headerName, opt_fillColSpan)
{
    // Use the first word of the header title as the sortkey
    var thisSortValue = sortColumnFromTableHeader(headerName);
    var arrowHTML = thisSortValue == g_history.dashboardSpecificState.sortColumn ?
        '<span class=' + g_history.dashboardSpecificState.sortOrder + '>' + (g_history.dashboardSpecificState.sortOrder == FORWARD ? '&uarr;' : '&darr;' ) + '</span>' : '';
    return '<th sortValue=' + thisSortValue +
        // Extend last th through all the rest of the columns.
        (opt_fillColSpan ? ' colspan=10000' : '') +
        // Extra span here is so flex boxing actually centers.
        // There's probably a better way to do this with CSS only though.
        '><div class=table-header-content><span></span>' + arrowHTML +
        '<span class=header-text>' + headerName + '</span>' + arrowHTML + '</div></th>';
}

function htmlForTestTable(rowsHTML, opt_excludeHeaders)
{
    var html = '<table class=test-table>';
    if (!opt_excludeHeaders) {
        html += '<thead><tr>';
        var headers = tableHeaders();
        for (var i = 0; i < headers.length; i++)
            html += htmlForTableColumnHeader(headers[i], i == headers.length - 1);
        html += '</tr></thead>';
    }
    return html + '<tbody>' + rowsHTML + '</tbody></table>';
}

function appendHTML(html)
{
    // InnerHTML to a div that's not in the document. This is
    // ~300ms faster in Safari 4 and Chrome 4 on mac.
    var div = document.createElement('div');
    div.innerHTML = html;
    document.body.appendChild(div);
    postHeightChangedMessage();
}

function alphanumericCompare(column, reverse)
{
    return reversibleCompareFunction(function(a, b) {
        // Put null entries at the bottom
        var a = a[column] ? String(a[column]) : 'z';
        var b = b[column] ? String(b[column]) : 'z';

        if (a < b)
            return -1;
        else if (a == b)
            return 0;
        else
            return 1;
    }, reverse);
}

function numericSort(column, reverse)
{
    return reversibleCompareFunction(function(a, b) {
        a = parseFloat(a[column]);
        b = parseFloat(b[column]);
        return a - b;
    }, reverse);
}

function reversibleCompareFunction(compare, reverse)
{
    return function(a, b) {
        return compare(reverse ? b : a, reverse ? a : b);
    };
}

function changeSort(e)
{
    var target = e.currentTarget;
    e.preventDefault();

    var sortValue = target.getAttribute('sortValue');
    while (target && target.tagName != 'TABLE')
        target = target.parentNode;

    var sort = 'sortColumn';
    var orderKey = 'sortOrder';
    if (sortValue == g_history.dashboardSpecificState[sort] && g_history.dashboardSpecificState[orderKey] == FORWARD)
        order = BACKWARD;
    else
        order = FORWARD;

    g_history.setQueryParameter(sort, sortValue, orderKey, order);
}

function sortTests(tests, column, order)
{
    var resultsProperty, sortFunctionGetter;
    if (column == 'flakiness') {
        sortFunctionGetter = numericSort;
        resultsProperty = 'flips';
    } else if (column == 'slowest') {
        sortFunctionGetter = numericSort;
        resultsProperty = 'slowestTime';
    } else {
        sortFunctionGetter = alphanumericCompare;
        resultsProperty = column;
    }

    tests.sort(sortFunctionGetter(resultsProperty, order == BACKWARD));
}

// Sorts a space separated expectations string in alphanumeric order.
// @param {string} str The expectations string.
// @return {string} The sorted string.
function sortExpectationsString(str)
{
    return str.split(' ').sort().join(' ');
}

function addUpdate(testsNeedingUpdate, test, builderName, missing, extra)
{
    if (!testsNeedingUpdate[test])
        testsNeedingUpdate[test] = {};

    var buildInfo = platformAndBuildType(builderName);
    var builder = buildInfo.platform + ' ' + buildInfo.buildType;
    if (!testsNeedingUpdate[test][builder])
        testsNeedingUpdate[test][builder] = {};

    if (missing)
        testsNeedingUpdate[test][builder].missing = sortExpectationsString(missing);

    if (extra)
        testsNeedingUpdate[test][builder].extra = sortExpectationsString(extra);
}


// From a string of modifiers, returns a string of modifiers that
// are for real result changes, like SLOW, and excludes modifiers
// that specificy things like platform, build_type, bug.
// @param {string} modifierString String containing all modifiers.
// @return {string} String containing only modifiers that effect the results.
function realModifiers(modifierString)
{
    var modifiers = modifierString.split(' ');;
    return modifiers.filter(function(modifier) {
        return !(modifier in BUILD_TYPES || string.startsWith(modifier, 'BUG'));
    }).join(' ');
}

function generatePageForExpectationsUpdate()
{
    // Always show all runs when auto-updating expectations.
    if (!g_history.crossDashboardState.showAllRuns)
        g_history.setQueryParameter('showAllRuns', true);

    processTestRunsForAllBuilders();
    var testsNeedingUpdate = {};
    for (var test in g_testToResultsMap) {
        var results = g_testToResultsMap[test];
        for (var i = 0; i < results.length; i++) {
            var thisResult = results[i];
            
            if (!thisResult.missing && !thisResult.extra)
                continue;

            var allPassesOrNoDatas = thisResult.rawResults.filter(function (x) { return x[1] != "P" && x[1] != "N"; }).length == 0;

            if (allPassesOrNoDatas)
                continue;

            addUpdate(testsNeedingUpdate, test, thisResult.builder, thisResult.missing, thisResult.extra);
        }
    }

    for (var builder in currentBuilders()) {
        var tests = g_perBuilderWithExpectationsButNoFailures[builder]
        for (var i = 0; i < tests.length; i++) {
            // Anything extra in this case is what is listed in expectations
            // plus modifiers other than bug, platform, build type.
            var modifiers = realModifiers(tests[i].modifiers);
            var extras = tests[i].expectations;
            extras += modifiers ? ' ' + modifiers : '';
            addUpdate(testsNeedingUpdate, tests[i].test, builder, null, extras);
        }
    }

    // Get the keys in alphabetical order, so it is easy to process groups
    // of tests.
    var keys = Object.keys(testsNeedingUpdate).sort();
    showUpdateInfoForTest(testsNeedingUpdate, keys);
}

// Show the test results and the json for differing expectations, and
// allow the user to include or exclude this update.
//
// @param {Object} testsNeedingUpdate Tests that need updating.
// @param {Array.<string>} keys Keys into the testNeedingUpdate object.
function showUpdateInfoForTest(testsNeedingUpdate, keys)
{
    var test = keys[g_history.dashboardSpecificState.updateIndex];
    document.body.innerHTML = '';

    // FIXME: Make this DOM creation less verbose.
    var index = document.createElement('div');
    index.style.cssFloat = 'right';
    index.textContent = (g_history.dashboardSpecificState.updateIndex + 1) + ' of ' + keys.length + ' tests';
    document.body.appendChild(index);

    var buttonRegion = document.createElement('div');
    var includeBtn = document.createElement('input');
    includeBtn.type = 'button';
    includeBtn.value = 'include selected';
    includeBtn.addEventListener('click', partial(handleUpdate, testsNeedingUpdate, keys), false);
    buttonRegion.appendChild(includeBtn);

    var previousBtn = document.createElement('input');
    previousBtn.type = 'button';
    previousBtn.value = 'previous';
    previousBtn.addEventListener('click',
        function() {
          setUpdateIndex(g_history.dashboardSpecificState.updateIndex - 1, testsNeedingUpdate, keys);
        },
        false);
    buttonRegion.appendChild(previousBtn);

    var nextBtn = document.createElement('input');
    nextBtn.type = 'button';
    nextBtn.value = 'next';
    nextBtn.addEventListener('click', partial(nextUpdate, testsNeedingUpdate, keys), false);
    buttonRegion.appendChild(nextBtn);

    var doneBtn = document.createElement('input');
    doneBtn.type = 'button';
    doneBtn.value = 'done';
    doneBtn.addEventListener('click', finishUpdate, false);
    buttonRegion.appendChild(doneBtn);

    document.body.appendChild(buttonRegion);

    var updates = testsNeedingUpdate[test];
    var checkboxes = document.createElement('div');
    for (var builder in updates) {
        // Create a checkbox for each builder.
        var checkboxRegion = document.createElement('div');
        var checkbox = document.createElement('input');
        checkbox.type = 'checkbox';
        checkbox.id = builder;
        checkbox.checked = true;
        checkboxRegion.appendChild(checkbox);
        checkboxRegion.appendChild(document.createTextNode(builder + ' : ' + JSON.stringify(updates[builder])));
        checkboxes.appendChild(checkboxRegion);
    }
    document.body.appendChild(checkboxes);

    var div = document.createElement('div');
    div.innerHTML = htmlForIndividualTestOnAllBuildersWithResultsLinks(test);
    document.body.appendChild(div);
    appendExpectations();
}


// When the user has finished selecting expectations to update, provide them
// with json to copy over.
function finishUpdate()
{
    document.body.innerHTML = 'The next step is to copy the output below ' +
        'into a local file and save it.  Then, run<br><code>python ' +
        'src/webkit/tools/layout_tests/webkitpy/layout_tests/update_expectat' +
        'ions_from_dashboard.py path/to/local/file</code><br>in order to ' +
        'update the expectations file.<br><textarea id="results" '+
        'style="width:600px;height:600px;"> ' +
        JSON.stringify(g_confirmedTests) + '</textarea>';
    results.focus();
    document.execCommand('SelectAll');
}

// Handle user click on "include selected" button.
// Includes the tests that are selected and exclude the rest.
// @param {Object} testsNeedingUpdate Tests that need updating.
// @param {Array.<string>} keys Keys into the testNeedingUpdate object.
function handleUpdate(testsNeedingUpdate, keys)
{
    var test = keys[g_history.dashboardSpecificState.updateIndex];
    var updates = testsNeedingUpdate[test];
    for (var builder in updates) {
        // Add included tests, and delete excluded tests if
        // they were previously included.
        if ($(builder).checked) {
            if (!g_confirmedTests[test])
                g_confirmedTests[test] = {};
            g_confirmedTests[test][builder] = testsNeedingUpdate[test][builder];
        } else if (g_confirmedTests[test] && g_confirmedTests[test][builder]) {
            delete g_confirmedTests[test][builder];
            if (!Object.keys(g_confirmedTests[test]).length)
                delete g_confirmedTests[test];
        }
    }
    nextUpdate(testsNeedingUpdate, keys);
}


// Move to the next item to update.
// @param {Object} testsNeedingUpdate Tests that need updating.
// @param {Array.<string>} keys Keys into the testNeedingUpdate object.
function nextUpdate(testsNeedingUpdate, keys)
{
    setUpdateIndex(g_history.dashboardSpecificState.updateIndex + 1, testsNeedingUpdate, keys);
}


// Advance the index we are updating at.  If we walk over the end
// or beginning, just loop.
// @param {string} newIndex The index into the keys to move to.
// @param {Object} testsNeedingUpdate Tests that need updating.
// @param {Array.<string>} keys Keys into the testNeedingUpdate object.
function setUpdateIndex(newIndex, testsNeedingUpdate, keys)
{
    if (newIndex == -1)
        newIndex = keys.length - 1;
    else if (newIndex == keys.length)
        newIndex = 0;
    g_history.setQueryParameter("updateIndex", newIndex);
    showUpdateInfoForTest(testsNeedingUpdate, keys);
}

function htmlForIndividualTestOnAllBuilders(test)
{
    processTestRunsForAllBuilders();

    var testResults = g_testToResultsMap[test];
    if (!testResults)
        return '<div class="not-found">Test not found. Either it does not exist, is skipped or passes on all platforms.</div>';
        
    var html = '';
    var shownBuilders = [];
    for (var j = 0; j < testResults.length; j++) {
        shownBuilders.push(testResults[j].builder);
        html += htmlForSingleTestRow(testResults[j]);
    }

    var skippedBuilders = []
    for (builder in currentBuilders()) {
        if (shownBuilders.indexOf(builder) == -1)
            skippedBuilders.push(builder);
    }

    var skippedBuildersHtml = '';
    if (skippedBuilders.length) {
        skippedBuildersHtml = '<div>The following builders either don\'t run this test (e.g. it\'s skipped) or all runs passed:</div>' +
            '<div class=skipped-builder-list><div class=skipped-builder>' + skippedBuilders.join('</div><div class=skipped-builder>') + '</div></div>';
    }

    return htmlForTestTable(html) + skippedBuildersHtml;
}

function htmlForIndividualTestOnAllBuildersWithResultsLinks(test)
{
    processTestRunsForAllBuilders();

    var testResults = g_testToResultsMap[test];
    var html = '';
    html += htmlForIndividualTestOnAllBuilders(test);

    html += '<div class=expectations test=' + test + '><div>' +
        linkHTMLToToggleState('showExpectations', 'results')

    if (g_history.isLayoutTestResults() || g_history.isGPUTestResults()) {
        if (g_history.isLayoutTestResults())
            html += ' | ' + linkHTMLToToggleState('showLargeExpectations', 'large thumbnails');
        if (testResults && currentBuilderGroup().master().name == WEBKIT_BUILDER_MASTER) {
            var revision = g_history.dashboardSpecificState.revision || '';
            html += '<form onsubmit="g_history.setQueryParameter(\'revision\', revision.value);' +
                'return false;">Show results for WebKit revision: ' +
                '<input name=revision placeholder="e.g. 65540" value="' + revision +
                '" id=revision-input></form>';
        } else
            html += ' | <b>Only shows actual results/diffs from the most recent *failure* on each bot.</b>';
    } else {
      html += ' | <span>Results height:<input ' +
          'onchange="g_history.setQueryParameter(\'resultsHeight\',this.value)" value="' +
          g_history.dashboardSpecificState.resultsHeight + '" style="width:2.5em">px</span>';
    }
    html += '</div></div>';
    return html;
}

function getExpectationsContainer(expectationsContainers, parentContainer, expectationsType)
{
    if (!expectationsContainers[expectationsType]) {
        var container = document.createElement('div');
        container.className = 'expectations-container';
        parentContainer.appendChild(container);
        expectationsContainers[expectationsType] = container;
    }
    return expectationsContainers[expectationsType];
}

function ensureTrailingSlash(path)
{
    if (path.match(/\/$/))
        return path;
    return path + '/';
}

function maybeAddPngChecksum(expectationDiv, pngUrl)
{
    // pngUrl gets served from the browser cache since we just loaded it in an
    // <img> tag.
    loader.request(pngUrl,
        function(xhr) {
            // Convert the first 2k of the response to a byte string.
            var bytes = xhr.responseText.substring(0, 2048);
            for (var position = 0; position < bytes.length; ++position)
                bytes[position] = bytes[position] & 0xff;

            // Look for the comment.
            var commentKey = 'tEXtchecksum\x00';
            var checksumPosition = bytes.indexOf(commentKey);
            if (checksumPosition == -1)
                return;

            var checksum = bytes.substring(checksumPosition + commentKey.length, checksumPosition + commentKey.length + 32);
            var checksumContainer = document.createElement('span');
            checksumContainer.innerText = 'Embedded checksum: ' + checksum;
            checksumContainer.setAttribute('class', 'pngchecksum');
            expectationDiv.parentNode.appendChild(checksumContainer);
        },
        function(xhr) {},
        true);
}

// Adds a specific expectation. If it's an image, it's only added on the
// image's onload handler. If it's a text file, then a script tag is appended
// as a hack to see if the file 404s (necessary since it's cross-domain).
// Once all the expectations for a specific type have loaded or errored
// (e.g. all the text results), then we go through and identify which platform
// uses which expectation.
//
// @param {Object} expectationsContainers Map from expectations type to
//     container DIV.
// @param {Element} parentContainer Container element for
//     expectationsContainer divs.
// @param {string} platform Platform string. Empty string for non-platform
//     specific expectations.
// @param {string} path Relative path to the expectation.
// @param {string} base Base path for the expectation URL.
// @param {string} opt_builder Builder whose actual results this expectation
//     points to.
// @param {string} opt_suite "virtual suite" that the test belongs to, if any.
function addExpectationItem(expectationsContainers, parentContainer, platform, path, base, opt_builder, opt_suite)
{
    var parts = path.split('.')
    var fileExtension = parts[parts.length - 1];
    if (fileExtension == 'html')
        fileExtension = 'txt';
    
    var container = getExpectationsContainer(expectationsContainers, parentContainer, fileExtension);
    var isImage = path.match(/\.png$/);

    // FIXME: Stop using script tags once all the places we pull from support CORS.
    var platformPart = platform ? ensureTrailingSlash(platform) : '';
    var suitePart = opt_suite ? ensureTrailingSlash(opt_suite) : '';

    var childContainer = document.createElement('span');
    childContainer.className = 'unloaded';

    var appendExpectationsItem = function(item) {
        childContainer.appendChild(expectationsTitle(platformPart + suitePart, path, opt_builder));
        childContainer.className = 'expectations-item';
        item.className = 'expectation ' + fileExtension;
        if (g_history.dashboardSpecificState.showLargeExpectations)
            item.className += ' large';
        childContainer.appendChild(item);
        handleFinishedLoadingExpectations(container);
    };

    var url = base + platformPart + path;
    if (isImage || !string.startsWith(base, 'http://svn.webkit.org')) {
        var dummyNode = document.createElement(isImage ? 'img' : 'script');
        dummyNode.src = url;
        dummyNode.onload = function() {
            var item;
            if (isImage) {
                item = dummyNode;
                if (string.startsWith(base, 'http://svn.webkit.org'))
                    maybeAddPngChecksum(item, url);
            } else {
                item = document.createElement('iframe');
                item.src = url;
            }
            appendExpectationsItem(item);
        }
        dummyNode.onerror = function() {
            childContainer.parentNode.removeChild(childContainer);
            handleFinishedLoadingExpectations(container);
        }

        // Append script elements now so that they load. Images load without being
        // appended to the DOM.
        if (!isImage)
            childContainer.appendChild(dummyNode);
    } else {
        loader.request(url,
            function(xhr) {
                var item = document.createElement('pre');
                item.innerText = xhr.responseText;
                appendExpectationsItem(item);
            },
            function(xhr) {/* Do nothing on errors since they're expected */});
    }

    container.appendChild(childContainer);
}


// Identifies which expectations are used on which platform once all the
// expectations of a given type have loaded (e.g. the container for png
// expectations for this test had no child elements with the class
// "unloaded").
//
// @param {string} container Element containing the expectations for a given
//     test and a given type (e.g. png).
function handleFinishedLoadingExpectations(container)
{
    if (container.getElementsByClassName('unloaded').length)
        return;

    var titles = container.getElementsByClassName('expectations-title');
    for (var platform in g_fallbacksMap) {
        var fallbacks = g_fallbacksMap[platform];
        var winner = null;
        var winningIndex = -1;
        for (var i = 0; i < titles.length; i++) {
            var title = titles[i];

            if (!winner && title.platform == "") {
                winner = title;
                continue;
            }

            var rawPlatform = title.platform && title.platform.replace('platform/', '');
            for (var j = 0; j < fallbacks.length; j++) {
                if ((winningIndex == -1 || winningIndex > j) && rawPlatform == fallbacks[j]) {
                    winningIndex = j;
                    winner = title;
                    break;
                }
            }
        }
        if (winner)
            winner.getElementsByClassName('platforms')[0].innerHTML += '<div class=used-platform>' + platform + '</div>';
        else {
            console.log('No expectations identified for this test. This means ' +
                'there is a logic bug in the dashboard for which expectations a ' +
                'platform uses or trac.webkit.org/src.chromium.org is giving 5XXs.');
        }
    }

    consolidateUsedPlatforms(container);
}

// Consolidate platforms when all sub-platforms for a given platform are represented.
// e.g., if all of the WIN- platforms are there, replace them with just WIN.
function consolidateUsedPlatforms(container)
{
    var allPlatforms = Object.keys(g_fallbacksMap);

    var platformElements = container.getElementsByClassName('platforms');
    for (var i = 0, platformsLength = platformElements.length; i < platformsLength; i++) {
        var usedPlatforms = platformElements[i].getElementsByClassName('used-platform');
        if (!usedPlatforms.length)
            continue;

        var platforms = {};
        platforms['MAC'] = {};
        platforms['WIN'] = {};
        platforms['LINUX'] = {};
        allPlatforms.forEach(function(platform) {
            if (string.startsWith(platform, 'MAC'))
                platforms['MAC'][platform] = 1;
            else if (string.startsWith(platform, 'WIN'))
                platforms['WIN'][platform] = 1;
            else if (string.startsWith(platform, 'LINUX'))
                platforms['LINUX'][platform] = 1;
        });

        for (var j = 0, usedPlatformsLength = usedPlatforms.length; j < usedPlatformsLength; j++) {
            for (var platform in platforms)
                delete platforms[platform][usedPlatforms[j].textContent];
        }

        for (var platform in platforms) {
            if (!Object.keys(platforms[platform]).length) {
                var nodesToRemove = [];
                for (var j = 0, usedPlatformsLength = usedPlatforms.length; j < usedPlatformsLength; j++) {
                    var usedPlatform = usedPlatforms[j];
                    if (string.startsWith(usedPlatform.textContent, platform))
                        nodesToRemove.push(usedPlatform);
                }

                nodesToRemove.forEach(function(element) { element.parentNode.removeChild(element); });
                platformElements[i].insertAdjacentHTML('afterBegin', '<div class=used-platform>' + platform + '</div>');
            }
        }
    }
}

function addExpectations(expectationsContainers, container, base,
    platform, text, png, reftest_html_file, reftest_mismatch_html_file, suite)
{
    var builder = '';
    addExpectationItem(expectationsContainers, container, platform, text, base, builder, suite);
    addExpectationItem(expectationsContainers, container, platform, png, base, builder, suite);
    addExpectationItem(expectationsContainers, container, platform, reftest_html_file, base, builder, suite);
    addExpectationItem(expectationsContainers, container, platform, reftest_mismatch_html_file, base, builder, suite);
}

function expectationsTitle(platform, path, builder)
{
    var header = document.createElement('h3');
    header.className = 'expectations-title';

    var innerHTML;
    if (builder) {
        var resultsType;
        if (string.endsWith(path, '-crash-log.txt'))
            resultsType = 'STACKTRACE';
        else if (string.endsWith(path, '-actual.txt') || string.endsWith(path, '-actual.png'))
            resultsType = 'ACTUAL RESULTS';
        else if (string.endsWith(path, '-wdiff.html'))
            resultsType = 'WDIFF';
        else
            resultsType = 'DIFF';

        innerHTML = resultsType + ': ' + builder;
    } else if (platform === "") {
        var parts = path.split('/');
        innerHTML = parts[parts.length - 1];
    } else
        innerHTML = platform || path;

    header.innerHTML = '<div class=title>' + innerHTML +
        '</div><div style="float:left">&nbsp;</div>' +
        '<div class=platforms style="float:right"></div>';
    header.platform = platform;
    return header;
}

function loadExpectations(expectationsContainer)
{
    var test = expectationsContainer.getAttribute('test');
    if (g_history.isLayoutTestResults())
        loadExpectationsLayoutTests(test, expectationsContainer);
    else {
        var results = g_testToResultsMap[test];
        for (var i = 0; i < results.length; i++)
            if (g_history.isGPUTestResults())
                loadGPUResultsForBuilder(results[i].builder, test, expectationsContainer);
            else
                loadNonWebKitResultsForBuilder(results[i].builder, test, expectationsContainer);
    }
}

function gpuResultsPath(chromeRevision, builder)
{
  return chromeRevision + '_' + builder.replace(/[^A-Za-z0-9]+/g, '_');
}

function loadGPUResultsForBuilder(builder, test, expectationsContainer)
{
    var container = document.createElement('div');
    container.className = 'expectations-container';
    container.innerHTML = '<div><b>' + builder + '</b></div>';
    expectationsContainer.appendChild(container);

    var failureIndex = indexesForFailures(builder, test)[0];

    var buildNumber = g_resultsByBuilder[builder].buildNumbers[failureIndex];
    var pathToLog = builderMaster(builder).logPath(builder, buildNumber) + pathToFailureLog(test);

    var chromeRevision = g_resultsByBuilder[builder].chromeRevision[failureIndex];
    var resultsUrl = GPU_RESULTS_BASE_PATH + gpuResultsPath(chromeRevision, builder);
    var filename = test.split(/\./)[1] + '.png';

    appendNonWebKitResults(container, pathToLog, 'non-webkit-results');
    appendNonWebKitResults(container, resultsUrl + '/gen/' + filename, 'gpu-test-results', 'Generated');
    appendNonWebKitResults(container, resultsUrl + '/ref/' + filename, 'gpu-test-results', 'Reference');
    appendNonWebKitResults(container, resultsUrl + '/diff/' + filename, 'gpu-test-results', 'Diff');
}

function loadNonWebKitResultsForBuilder(builder, test, expectationsContainer)
{
    var failureIndexes = indexesForFailures(builder, test);
    var container = document.createElement('div');
    container.innerHTML = '<div><b>' + builder + '</b></div>';
    expectationsContainer.appendChild(container);
    for (var i = 0; i < failureIndexes.length; i++) {
        // FIXME: This doesn't seem to work anymore. Did the paths change?
        // Once that's resolved, see if we need to try each GTEST_MODIFIERS prefix as well.
        var buildNumber = g_resultsByBuilder[builder].buildNumbers[failureIndexes[i]];
        var pathToLog = builderMaster(builder).logPath(builder, buildNumber) + pathToFailureLog(test);
        appendNonWebKitResults(container, pathToLog, 'non-webkit-results');
    }
}

function appendNonWebKitResults(container, url, itemClassName, opt_title)
{
    // Use a script tag to detect whether the URL 404s.
    // Need to use a script tag since the URL is cross-domain.
    var dummyNode = document.createElement('script');
    dummyNode.src = url;

    dummyNode.onload = function() {
        var item = document.createElement('iframe');
        item.src = dummyNode.src;
        item.className = itemClassName;
        item.style.height = g_history.dashboardSpecificState.resultsHeight + 'px';

        if (opt_title) {
            var childContainer = document.createElement('div');
            childContainer.style.display = 'inline-block';
            var title = document.createElement('div');
            title.textContent = opt_title;
            childContainer.appendChild(title);
            childContainer.appendChild(item);
            container.replaceChild(childContainer, dummyNode);
        } else
            container.replaceChild(item, dummyNode);
    }
    dummyNode.onerror = function() {
        container.removeChild(dummyNode);
    }

    container.appendChild(dummyNode);
}

function buildInfoForRevision(builder, revision)
{
    var revisions = g_resultsByBuilder[builder].webkitRevision;
    var revisionStart = 0, revisionEnd = 0, buildNumber = 0;
    for (var i = 0; i < revisions.length; i++) {
        if (revision > revisions[i]) {
            revisionStart = revisions[i - 1];
            revisionEnd = revisions[i];
            buildNumber = g_resultsByBuilder[builder].buildNumbers[i - 1];
            break;
        }
    }

    if (revisionEnd)
      revisionEnd++;
    else
      revisionEnd = '';

    return {revisionStart: revisionStart, revisionEnd: revisionEnd, buildNumber: buildNumber};
}

function lookupVirtualTestSuite(test) {
    for (var suite in VIRTUAL_SUITES) {
        if (test.indexOf(suite) != -1)
            return suite;
    }
    return '';
}

function baseTest(test, suite) {
    base = VIRTUAL_SUITES[suite];
    return base ? test.replace(suite, base) : test;
}

function loadBaselinesForTest(expectationsContainers, expectationsContainer, test) {
    var testWithoutSuffix = test.substring(0, test.lastIndexOf('.'));
    var text = testWithoutSuffix + "-expected.txt";
    var png = testWithoutSuffix + "-expected.png";
    var reftest_html_file = testWithoutSuffix + "-expected.html";
    var reftest_mismatch_html_file = testWithoutSuffix + "-expected-mismatch.html";
    var suite = lookupVirtualTestSuite(test);

    if (!suite)
        addExpectationItem(expectationsContainers, expectationsContainer, null, test, TEST_URL_BASE_PATH);

    addExpectations(expectationsContainers, expectationsContainer,
        TEST_URL_BASE_PATH, '', text, png, reftest_html_file, reftest_mismatch_html_file, suite);

    var fallbacks = allFallbacks();
    for (var i = 0; i < fallbacks.length; i++) {
      var fallback = 'platform/' + fallbacks[i];
      addExpectations(expectationsContainers, expectationsContainer, TEST_URL_BASE_PATH, fallback, text, png,
          reftest_html_file, reftest_mismatch_html_file, suite);
    }

    if (suite)
        loadBaselinesForTest(expectationsContainers, expectationsContainer, baseTest(test, suite));
}

function loadExpectationsLayoutTests(test, expectationsContainer)
{
    // Map from file extension to container div for expectations of that type.
    var expectationsContainers = {};

    var revisionContainer = document.createElement('div');
    revisionContainer.textContent = "Showing results for: "
    expectationsContainer.appendChild(revisionContainer);
    for (var builder in currentBuilders()) {
        if (builderMaster(builder).name == WEBKIT_BUILDER_MASTER) {
            var latestRevision = g_history.dashboardSpecificState.revision || g_resultsByBuilder[builder].webkitRevision[0];
            var buildInfo = buildInfoForRevision(builder, latestRevision);
            var revisionInfo = document.createElement('div');
            revisionInfo.style.cssText = 'background:lightgray;margin:0 3px;padding:0 2px;display:inline-block;';
            revisionInfo.innerHTML = builder + ' r' + buildInfo.revisionEnd +
                ':r' + buildInfo.revisionStart + ', build ' + buildInfo.buildNumber;
            revisionContainer.appendChild(revisionInfo);
        }
    }

    loadBaselinesForTest(expectationsContainers, expectationsContainer, test);
        
    var testWithoutSuffix = test.substring(0, test.lastIndexOf('.'));
    var actualResultSuffixes = ['-actual.txt', '-actual.png', '-crash-log.txt', '-diff.txt', '-wdiff.html', '-diff.png'];

    for (var builder in currentBuilders()) {
        var actualResultsBase;
        if (builderMaster(builder).name == WEBKIT_BUILDER_MASTER) {
            var latestRevision = g_history.dashboardSpecificState.revision || g_resultsByBuilder[builder].webkitRevision[0];
            var buildInfo = buildInfoForRevision(builder, latestRevision);
            actualResultsBase = 'http://build.webkit.org/results/' + builder +
                '/r' + buildInfo.revisionStart + ' (' + buildInfo.buildNumber + ')/';
        } else
            actualResultsBase = TEST_RESULTS_BASE_PATH + currentBuilders()[builder] + '/results/layout-test-results/';

        for (var i = 0; i < actualResultSuffixes.length; i++) {
            addExpectationItem(expectationsContainers, expectationsContainer, null,
                testWithoutSuffix + actualResultSuffixes[i], actualResultsBase, builder);
        }
    }

    // Add a clearing element so floated elements don't bleed out of their
    // containing block.
    var br = document.createElement('br');
    br.style.clear = 'both';
    expectationsContainer.appendChild(br);
}

var g_allFallbacks;

// Returns the reverse sorted, deduped list of all platform fallback
// directories.
function allFallbacks()
{
    if (!g_allFallbacks) {
        var holder = {};
        for (var platform in g_fallbacksMap) {
            var fallbacks = g_fallbacksMap[platform];
            for (var i = 0; i < fallbacks.length; i++)
                holder[fallbacks[i]] = 1;
        }

        g_allFallbacks = [];
        for (var fallback in holder)
            g_allFallbacks.push(fallback);

        g_allFallbacks.sort(function(a, b) {
            if (a == b)
                return 0;
            return a < b;
        });
    }
    return g_allFallbacks;
}

function appendExpectations()
{
    var expectations = g_history.dashboardSpecificState.showExpectations ? document.getElementsByClassName('expectations') : [];
    // Loading expectations is *very* slow. Use a large timeout to avoid
    // totally hanging the renderer.
    performChunkedAction(expectations, function(chunk) {
        for (var i = 0, len = chunk.length; i < len; i++)
            loadExpectations(chunk[i]);
        postHeightChangedMessage();

    }, hideLoadingUI, 10000);
}

function hideLoadingUI()
{
    var loadingDiv = $('loading-ui');
    if (loadingDiv)
        loadingDiv.style.display = 'none';
    postHeightChangedMessage();
}

function generatePageForIndividualTests(tests)
{
    console.log('Number of tests: ' + tests.length);
    if (g_history.dashboardSpecificState.showChrome)
        appendHTML(htmlForNavBar());
    performChunkedAction(tests, function(chunk) {
        appendHTML(htmlForIndividualTests(chunk));
    }, appendExpectations, 500);
    if (g_history.dashboardSpecificState.showChrome)
        $('tests-input').value = g_history.dashboardSpecificState.tests;
}

function performChunkedAction(tests, handleChunk, onComplete, timeout, opt_index) {
    var index = opt_index || 0;
    setTimeout(function() {
        var chunk = Array.prototype.slice.call(tests, index * CHUNK_SIZE, (index + 1) * CHUNK_SIZE);
        if (chunk.length) {
            handleChunk(chunk);
            performChunkedAction(tests, handleChunk, onComplete, timeout, ++index);
        } else
            onComplete();
    // No need for a timeout on the first chunked action.
    }, index ? timeout : 0);
}

function htmlForIndividualTests(tests)
{
    var testsHTML = [];
    for (var i = 0; i < tests.length; i++) {
        var test = tests[i];
        var testNameHtml = '';
        if (g_history.dashboardSpecificState.showChrome || tests.length > 1) {
            if (g_history.isLayoutTestResults()) {
                var suite = lookupVirtualTestSuite(test);
                var base = suite ? baseTest(test, suite) : test;
                var tracURL = TEST_URL_BASE_PATH_TRAC + base;
                testNameHtml += '<h2>' + linkHTMLToOpenWindow(tracURL, test) + '</h2>';
            } else
                testNameHtml += '<h2>' + test + '</h2>';
        }

        testsHTML.push(testNameHtml + htmlForIndividualTestOnAllBuildersWithResultsLinks(test));
    }
    return testsHTML.join('<hr>');
}

function htmlForNavBar()
{
    var extraHTML = '';
    var html = ui.html.testTypeSwitcher(false, extraHTML, isCrossBuilderView());
    html += '<div class=forms><form id=result-form ' +
        'onsubmit="g_history.setQueryParameter(\'result\', result.value);' +
        'return false;">Show all tests with result: ' +
        '<input name=result placeholder="e.g. CRASH" id=result-input>' +
        '</form><form id=tests-form ' +
        'onsubmit="g_history.setQueryParameter(\'tests\', tests.value);' +
        'return false;"><span>Show tests on all platforms: </span>' +
        '<input name=tests ' +
        'placeholder="Comma or space-separated list of tests or partial ' +
        'paths to show test results across all builders, e.g., ' +
        'foo/bar.html,foo/baz,domstorage" id=tests-input></form>' +
        '<span class=link onclick="showLegend()">Show legend [type ?]</span></div>';
    return html;
}

function checkBoxToToggleState(key, text)
{
    var stateEnabled = g_history.dashboardSpecificState[key];
    return '<label><input type=checkbox ' + (stateEnabled ? 'checked ' : '') + 'onclick="g_history.setQueryParameter(\'' + key + '\', ' + !stateEnabled + ')">' + text + '</label> ';
}

function linkHTMLToToggleState(key, linkText)
{
    var stateEnabled = g_history.dashboardSpecificState[key];
    return '<span class=link onclick="g_history.setQueryParameter(\'' + key + '\', ' + !stateEnabled + ')">' + (stateEnabled ? 'Hide' : 'Show') + ' ' + linkText + '</span>';
}

function headerForTestTableHtml()
{
    return '<h2 style="display:inline-block">Failing tests</h2>' +
        checkBoxToToggleState('showWontFixSkip', 'WONTFIX/SKIP') +
        checkBoxToToggleState('showCorrectExpectations', 'tests with correct expectations') +
        checkBoxToToggleState('showWrongExpectations', 'tests with wrong expectations') +
        checkBoxToToggleState('showFlaky', 'flaky') +
        checkBoxToToggleState('showSlow', 'slow');
}

function generatePageForBuilder(builderName)
{
    processTestRunsForBuilder(builderName);

    var results = g_perBuilderFailures[builderName];
    sortTests(results, g_history.dashboardSpecificState.sortColumn, g_history.dashboardSpecificState.sortOrder);

    var testsHTML = '';
    if (results.length) {
        var tableRowsHTML = '';
        for (var i = 0; i < results.length; i++)
            tableRowsHTML += htmlForSingleTestRow(results[i])
        testsHTML = htmlForTestTable(tableRowsHTML);
    } else {
        testsHTML = '<div>No tests found. ';
        if (g_history.isLayoutTestResults())
            testsHTML += 'Try showing tests with correct expectations.</div>';
        else
            testsHTML += 'This means no tests have failed!</div>';
    }

    var html = htmlForNavBar();

    if (g_history.isLayoutTestResults())
        html += htmlForTestsWithExpectationsButNoFailures(builderName) + headerForTestTableHtml();

    html += '<br>' + testsHTML;
    appendHTML(html);

    var ths = document.getElementsByTagName('th');
    for (var i = 0; i < ths.length; i++) {
        ths[i].addEventListener('click', changeSort, false);
        ths[i].className = "sortable";
    }

    hideLoadingUI();
}

var VALID_KEYS_FOR_CROSS_BUILDER_VIEW = {
    tests: 1,
    result: 1,
    showChrome: 1,
    showExpectations: 1,
    showLargeExpectations: 1,
    legacyExpectationsSemantics: 1,
    resultsHeight: 1,
    revision: 1
};

function isInvalidKeyForCrossBuilderView(key)
{
    return !(key in VALID_KEYS_FOR_CROSS_BUILDER_VIEW) && !(key in history.DEFAULT_CROSS_DASHBOARD_STATE_VALUES);
}

function hideLegend()
{
    var legend = $('legend');
    if (legend)
        legend.parentNode.removeChild(legend);
}

var g_fallbacksMap = {};
g_fallbacksMap['WIN-XP'] = ['chromium-win-xp', 'chromium-win', 'chromium'];
g_fallbacksMap['WIN-7'] = ['chromium-win', 'chromium'];
g_fallbacksMap['MAC-SNOWLEOPARD'] = ['chromium-mac-snowleopard', 'chromium-mac', 'chromium'];
g_fallbacksMap['MAC-LION'] = ['chromium-mac', 'chromium'];
g_fallbacksMap['LINUX-32'] = ['chromium-linux-x86', 'chromium-linux', 'chromium-win', 'chromium'];
g_fallbacksMap['LINUX-64'] = ['chromium-linux', 'chromium-win', 'chromium'];

function showLegend()
{
    var legend = $('legend');
    if (!legend) {
        legend = document.createElement('div');
        legend.id = 'legend';
        document.body.appendChild(legend);
    }

    var html = '<div id=legend-toggle onclick="hideLegend()">Hide ' +
        'legend [type esc]</div><div id=legend-contents>';
    for (var expectation in expectationsMap())
        html += '<div class=' + expectation + '>' + expectationsMap()[expectation] + '</div>';

    html += '<div class=merge>WEBKIT MERGE</div>';
    if (g_history.isLayoutTestResults())
      html += '</div><br style="clear:both"><div>TIMES:</div>' +
          htmlForSlowTimes(MIN_SECONDS_FOR_SLOW_TEST) +
          '<div>DEBUG TIMES:</div>' +
          htmlForSlowTimes(MIN_SECONDS_FOR_SLOW_TEST_DEBUG);

    legend.innerHTML = html;
}

function htmlForSlowTimes(minTime)
{
    return '<ul><li>&lt;1 second == !SLOW</li><li>&gt;1 second && &lt;' +
        minTime + ' seconds == SLOW || !SLOW is fine</li><li>&gt;' +
        minTime + ' seconds == SLOW</li></ul>';
}

function postHeightChangedMessage()
{
    if (window == parent)
        return;

    var root = document.documentElement;
    var height = root.offsetHeight;
    if (root.offsetWidth < root.scrollWidth) {
        // We have a horizontal scrollbar. Include it in the height.
        var dummyNode = document.createElement('div');
        dummyNode.style.overflow = 'scroll';
        document.body.appendChild(dummyNode);
        var scrollbarWidth = dummyNode.offsetHeight - dummyNode.clientHeight;
        document.body.removeChild(dummyNode);
        height += scrollbarWidth;
    }
    parent.postMessage({command: 'heightChanged', height: height}, '*')
}

if (window != parent)
    window.addEventListener('blur', ui.popup.hide);

document.addEventListener('keydown', function(e) {
    if (e.keyIdentifier == 'U+003F' || e.keyIdentifier == 'U+00BF') {
        // WebKit MAC retursn 3F. WebKit WIN returns BF. This is a bug!
        // ? key
        showLegend();
    } else if (e.keyIdentifier == 'U+001B') {
        // escape key
        hideLegend();
        ui.popup.hide();
    }
}, false);

window.addEventListener('load', function() {
    resourceLoader = new loader.Loader();
    resourceLoader.load();
}, false);
