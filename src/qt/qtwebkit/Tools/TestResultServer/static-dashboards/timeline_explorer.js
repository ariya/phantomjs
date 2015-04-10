// Copyright (C) 2013 Google Inc. All rights reserved.
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
var FAILING_TESTS_DATASET_NAME = 'Failing tests';

var g_dygraph;
var g_buildIndicesByTimestamp = {};
var g_currentBuildIndex = -1;
var g_currentBuilderTestResults;

var defaultDashboardSpecificStateValues = {
    builder: null,
    buildTimestamp: -1,
    ignoreFlakyTests: true
};

var DB_SPECIFIC_INVALIDATING_PARAMETERS = {
    'testType': 'builder',
    'group': 'builder'
};

function generatePage(historyInstance)
{
    g_buildIndicesByTimestamp = {};
    var results = g_resultsByBuilder[historyInstance.dashboardSpecificState.builder || currentBuilderGroup().defaultBuilder()];

    for (var i = 0; i < results[FIXABLE_COUNTS_KEY].length; i++) {
        var buildDate = new Date(results[TIMESTAMPS_KEY][i] * 1000);
        g_buildIndicesByTimestamp[buildDate.getTime()] = i;
    }

    if (historyInstance.dashboardSpecificState.buildTimestamp != -1 && historyInstance.dashboardSpecificState.buildTimestamp in g_buildIndicesByTimestamp) {
        var newBuildIndex = g_buildIndicesByTimestamp[historyInstance.dashboardSpecificState.buildTimestamp];

        if (newBuildIndex == g_currentBuildIndex) {
            // This happens when selectBuild is called, which updates the UI
            // immediately, in addition to updating the location hash (we don't
            // just rely on the hash change since we don't want to regenerate the
            // whole page just because the user clicked on something)
            return;
        } else if (newBuildIndex)
            g_currentBuildIndex = newBuildIndex;
    }

    initCurrentBuilderTestResults();

    $('test-type-switcher').innerHTML = ui.html.testTypeSwitcher( false,
        ui.html.checkbox('ignoreFlakyTests', 'Ignore flaky tests', historyInstance.dashboardSpecificState.ignoreFlakyTests, 'g_currentBuildIndex = -1')
    );

    updateTimelineForBuilder();
}

function handleValidHashParameter(historyInstance, key, value)
{
    switch(key) {
    case 'builder':
        history.validateParameter(historyInstance.dashboardSpecificState, key, value,
            function() { return value in currentBuilders(); });
        return true;
    case 'buildTimestamp':
        historyInstance.dashboardSpecificState.buildTimestamp = parseInt(value, 10);
        return true;
    case 'ignoreFlakyTests':
        historyInstance.dashboardSpecificState.ignoreFlakyTests = value == 'true';
        return true;
    default:
        return false;
    }
}

var timelineConfig = {
    defaultStateValues: defaultDashboardSpecificStateValues,
    generatePage: generatePage,
    handleValidHashParameter: handleValidHashParameter,
    invalidatingHashParameters: DB_SPECIFIC_INVALIDATING_PARAMETERS
};

// FIXME(jparent): Eventually remove all usage of global history object.
var g_history = new history.History(timelineConfig);
g_history.parseCrossDashboardParameters();

function initCurrentBuilderTestResults()
{
    var startTime = Date.now();
    g_currentBuilderTestResults = _decompressResults(g_resultsByBuilder[g_history.dashboardSpecificState.builder || currentBuilderGroup().defaultBuilder()]);
    console.log( 'Time to get test results by build: ' + (Date.now() - startTime));
}

function updateTimelineForBuilder()
{
    var builder = g_history.dashboardSpecificState.builder || currentBuilderGroup().defaultBuilder();
    var results = g_resultsByBuilder[builder];
    var graphData = [];

    var annotations = [];

    // Dygraph prefers to be handed data in chronological order.
    for (var i = results[FIXABLE_COUNTS_KEY].length - 1; i >= 0; i--) {
        var buildDate = new Date(results[TIMESTAMPS_KEY][i] * 1000);
        // FIXME: Find a better way to exclude outliers. This is just so we
        // exclude runs where every test failed.
        var failureCount = Math.min(results[FIXABLE_COUNT_KEY][i], 10000);

        if (g_history.dashboardSpecificState.ignoreFlakyTests)
            failureCount -= g_currentBuilderTestResults.flakyDeltasByBuild[i].total || 0;

        graphData.push([buildDate, failureCount]);
    }

    var windowWidth = document.documentElement.clientWidth;
    var windowHeight = document.documentElement.clientHeight;
    var switcherNode = $('test-type-switcher');
    var inspectorNode = $('inspector-container');
    var graphWidth = windowWidth - 20 - inspectorNode.offsetWidth;
    var graphHeight = windowHeight - switcherNode.offsetTop - switcherNode.offsetHeight - 20;

    var containerNode = $('timeline-container');
    containerNode.style.height = graphHeight + 'px';
    containerNode.style.width = graphWidth + 'px';
    inspectorNode.style.height = graphHeight + 'px';

    g_dygraph = new Dygraph(
        containerNode,
        graphData, {
            labels: ['Date', FAILING_TESTS_DATASET_NAME],
            width: graphWidth,
            height: graphHeight,
            clickCallback: function(event, date) {
                selectBuild(results, builder, g_dygraph, g_buildIndicesByTimestamp[date]);
            },
            drawCallback: function(dygraph, isInitial) {
                if (isInitial)
                    return;
                updateBuildIndicator(results, dygraph);
            },
            // xValueParser is necessary for annotations to work, even though we
            // already have Date instances
            xValueParser: function(input) { return input.getTime(); }
        });
    if (annotations.length)
        g_dygraph.setAnnotations(annotations);

    inspectorNode.style.visibility = 'visible';

    if (g_currentBuildIndex != -1)
        selectBuild(results, builder, g_dygraph, g_currentBuildIndex);
}

function selectBuild(results, builder, dygraph, index)
{
    g_currentBuildIndex = index;
    updateBuildIndicator(results, dygraph);
    updateBuildInspector(results, builder, dygraph, index);
    g_history.setQueryParameter('buildTimestamp', results[TIMESTAMPS_KEY][index] * 1000);
}

function updateBuildIndicator(results, dygraph)
{
    var indicatorNode = $('indicator');

    if (!indicatorNode) {
        var containerNode = $('timeline-container');
        indicatorNode = document.createElement('div');
        indicatorNode.id = 'indicator';
        indicatorNode.style.height = containerNode.offsetHeight + 'px';
        containerNode.appendChild(indicatorNode);
    }

    if (g_currentBuildIndex == -1)
        indicatorNode.style.display = 'none';
    else {
        indicatorNode.style.display = 'block';
        var buildDate = new Date(results[TIMESTAMPS_KEY][g_currentBuildIndex] * 1000);
        var domCoords = dygraph.toDomCoords(buildDate, 0);
        indicatorNode.style.left = domCoords[0] + 'px';
    }
}

function updateBuildInspector(results, builder, dygraph, index)
{
    var html = '<table id="inspector-table"><caption>Details</caption>';

    function addRow(label, value)
    {
        html += '<tr><td class="label">' + label + '</td><td>' + value + '</td></tr>';
    }

    // Builder and results links
    var buildNumber = results[BUILD_NUMBERS_KEY][index];
    addRow('', '');
    var master = builderMaster(builder);
    var buildUrl = master.logPath(builder, results[BUILD_NUMBERS_KEY][index]);
    var resultsUrl = 'http://build.webkit.org/results/' + builder + '/r' + results[WEBKIT_REVISIONS_KEY][index] +
        ' (' + results[BUILD_NUMBERS_KEY][index] + ')';

    addRow('Build:', '<a href="' + buildUrl + '" target="_blank">' + buildNumber + '</a> (<a href="' + resultsUrl + '" target="_blank">results</a>)');

    // Revision link
    addRow('WebKit change:', ui.html.webKitRevisionLink(results, index));

    // Test status/counts
    addRow('', '');

    function addNumberRow(label, currentValue, previousValue)
    {
        var delta = currentValue - previousValue;
        var deltaText = ''
        if (delta < 0)
            deltaText = ' <span class="delta negative">' + delta + '</span>';
        else if (delta > 0)
            deltaText = ' <span class="delta positive">+' + delta + '</span>';

        addRow(label, currentValue + deltaText);
    }

    var expectations = expectationsMap();
    var flakyDeltasByBuild = g_currentBuilderTestResults.flakyDeltasByBuild;
    for (var expectationKey in expectations) {
        if (expectationKey in results[FIXABLE_COUNTS_KEY][index]) {
            var currentCount = results[FIXABLE_COUNTS_KEY][index][expectationKey];
            var previousCount = results[FIXABLE_COUNTS_KEY][index + 1][expectationKey];
            if (g_history.dashboardSpecificState.ignoreFlakyTests) {
                currentCount -= flakyDeltasByBuild[index][expectationKey] || 0;
                previousCount -= flakyDeltasByBuild[index + 1][expectationKey] || 0;
            }
            addNumberRow(expectations[expectationKey], currentCount, previousCount);
        }
    }

    var currentTotal = results[FIXABLE_COUNT_KEY][index];
    var previousTotal = results[FIXABLE_COUNT_KEY][index + 1];
    if (g_history.dashboardSpecificState.ignoreFlakyTests) {
        currentTotal -= flakyDeltasByBuild[index].total || 0;
        previousTotal -= flakyDeltasByBuild[index + 1].total || 0;
    }
    addNumberRow('Total failing tests:', currentTotal, previousTotal);

    html += '</table>';

    html += '<div id="changes-button" class="buttons">';
    html += '<button>Show changed test results</button>';
    html += '</div>';

    html += '<div id="build-buttons" class="buttons">';
    html += '<button>Previous build</button> <button>Next build</button>';
    html += '</div>';

    var inspectorNode = $('inspector-container');
    inspectorNode.innerHTML = html;

    inspectorNode.getElementsByTagName('button')[0].onclick = function() {
        showResultsDelta(index, buildNumber, buildUrl, resultsUrl);
    };
    inspectorNode.getElementsByTagName('button')[1].onclick = function() {
        selectBuild(results, builder, dygraph, index + 1);
    };
    inspectorNode.getElementsByTagName('button')[2].onclick = function() {
        selectBuild(results, builder, dygraph, index - 1);
    };
}

function showResultsDelta(index, buildNumber, buildUrl, resultsUrl)
{
    var flakyTests = g_currentBuilderTestResults.flakyTests;
    var currentResults = g_currentBuilderTestResults.resultsByBuild[index];
    var testNames = g_currentBuilderTestResults.testNames;
    var previousResults = g_currentBuilderTestResults.resultsByBuild[index + 1];
    var expectations = expectationsMap();

    var deltas = {};
    function addDelta(category, testIndex)
    {
        if (g_history.dashboardSpecificState.ignoreFlakyTests && flakyTests[testIndex])
            return;
        if (!(category in deltas))
            deltas[category] = [];
        var testName = testNames[testIndex];
        var flakinessDashboardUrl = 'flakiness_dashboard.html' + (location.hash ? location.hash + '&' : '#') + 'tests=' + testName;
        var html = '<a href="' + flakinessDashboardUrl + '">' + testName + '</a>';
        if (flakyTests[testIndex])
            html += ' <span style="color: #f66">possibly flaky</span>';
        deltas[category].push(html);
    }

    for (var testIndex = 0; testIndex < currentResults.length; testIndex++) {
        if (currentResults[testIndex] === undefined)
            continue;

        if (previousResults[testIndex] !== undefined) {
            if (currentResults[testIndex] == previousResults[testIndex])
                continue;
            addDelta('Was <b>' + expectations[previousResults[testIndex]] + '</b> now <b>' + expectations[currentResults[testIndex]] + '</b>', testIndex);
        } else
            addDelta('Newly <b>' + expectations[currentResults[testIndex]] + '</b>', testIndex);
    }

    for (var testIndex = 0; testIndex < previousResults.length; testIndex++) {
        if (previousResults[testIndex] === undefined)
            continue;
        if (currentResults[testIndex] === undefined)
            addDelta('Was <b>' + expectations[previousResults[testIndex]] + '</b>', testIndex);
    }

    var html = '';

    html += '<head><base target="_blank"></head>';
    html += '<h1>Changes in test results</h1>';

    html += '<p>For build <a href="' + buildUrl + '" target="_blank">' +
        buildNumber + '</a> ' + '(<a href="' + resultsUrl +
        '" target="_blank">results</a>)</p>';

    for (var deltaCategory in deltas) {
        html += '<p><div>' + deltaCategory + ' (' + deltas[deltaCategory].length + ')</div><ul>';
        deltas[deltaCategory].forEach(function(deltaHtml) {
            html += '<li>' + deltaHtml + '</li>';
        });
        html += '</ul></p>';
    }

    var deltaWindow = window.open();
    deltaWindow.document.write(html);
}

var _FAILURE_EXPECTATIONS = {
    'T': 1,
    'F': 1,
    'C': 1,
    'I': 1,
    'Z': 1
};

// "Decompresses" the RLE-encoding of test results so that we can query it
// by build index and test name.
//
// @param {Object} results results for the current builder
// @return Object with these properties:
//     - testNames: array mapping test index to test names.
//     - resultsByBuild: array of builds, for each build a (sparse) array of test results by test index.
//     - flakyTests: array with the boolean value true at test indices that are considered flaky (more than one single-build failure).
//     - flakyDeltasByBuild: array of builds, for each build a count of flaky test results by expectation, as well as a total.
function _decompressResults(builderResults)
{
    var builderTestResults = builderResults[TESTS_KEY];
    var buildCount = builderResults[FIXABLE_COUNTS_KEY].length;
    var resultsByBuild = new Array(buildCount);
    var flakyDeltasByBuild = new Array(buildCount);

    // Pre-sizing the test result arrays for each build saves us ~250ms
    var testCount = 0;
    for (var testName in builderTestResults)
        testCount++;
    for (var i = 0; i < buildCount; i++) {
        resultsByBuild[i] = new Array(testCount);
        resultsByBuild[i][testCount - 1] = undefined;
        flakyDeltasByBuild[i] = {};
    }

    // Using indices instead of the full test names for each build saves us
    // ~1500ms
    var testIndex = 0;
    var testNames = new Array(testCount);
    var flakyTests = new Array(testCount);

    // Decompress and "invert" test results (by build instead of by test) and
    // determine which are flaky.
    for (var testName in builderTestResults) {
        var oneBuildFailureCount = 0;

        testNames[testIndex] = testName;
        var testResults = builderTestResults[testName].results;
        for (var i = 0, rleResult, currentBuildIndex = 0; (rleResult = testResults[i]) && currentBuildIndex < buildCount; i++) {
            var count = rleResult[RLE.LENGTH];
            var value = rleResult[RLE.VALUE];

            if (count == 1 && value in _FAILURE_EXPECTATIONS)
                oneBuildFailureCount++;

            for (var j = 0; j < count; j++) {
                resultsByBuild[currentBuildIndex++][testIndex] = value;
                if (currentBuildIndex == buildCount)
                    break;
            }
        }

        if (oneBuildFailureCount > 2)
            flakyTests[testIndex] = true;

        testIndex++;
    }

    // Now that we know which tests are flaky, count the test results that are
    // from flaky tests for each build.
    testIndex = 0;
    for (var testName in builderTestResults) {
        if (!flakyTests[testIndex++])
            continue;

        var testResults = builderTestResults[testName].results;
        for (var i = 0, rleResult, currentBuildIndex = 0; (rleResult = testResults[i]) && currentBuildIndex < buildCount; i++) {
            var count = rleResult[RLE.LENGTH];
            var value = rleResult[RLE.VALUE];

            for (var j = 0; j < count; j++) {
                var buildTestResults = flakyDeltasByBuild[currentBuildIndex++];
                function addFlakyDelta(key)
                {
                    if (!(key in buildTestResults))
                        buildTestResults[key] = 0;
                    buildTestResults[key]++;
                }
                addFlakyDelta(value);
                if (value != 'P' && value != 'N')
                    addFlakyDelta('total');
                if (currentBuildIndex == buildCount)
                    break;
            }
        }
    }

    return {
        testNames: testNames,
        resultsByBuild: resultsByBuild,
        flakyTests: flakyTests,
        flakyDeltasByBuild: flakyDeltasByBuild
    };
}

document.addEventListener('keydown', function(e) {
    if (g_currentBuildIndex == -1)
        return;

    var builder = g_history.dashboardSpecificState.builder || currentBuilderGroup().defaultBuilder();
    switch (e.keyIdentifier) {
    case 'Left':
        selectBuild(
            g_resultsByBuilder[builder],
            builder,
            g_dygraph,
            g_currentBuildIndex + 1);
        break;
    case 'Right':
        selectBuild(
            g_resultsByBuilder[builder],
            builder,
            g_dygraph,
            g_currentBuildIndex - 1);
        break;
    }
});

window.addEventListener('load', function() {
    var resourceLoader = new loader.Loader();
    resourceLoader.load();
}, false);
