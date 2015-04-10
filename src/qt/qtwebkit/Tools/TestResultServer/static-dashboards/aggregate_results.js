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
//
// @fileoverview Creates a dashboard for tracking number of passes/failures per run.
//
// Currently, only webkit tests are supported, but adding other test types
// should just require the following steps:
//     -generate results.json for these tests
//     -copy them to the appropriate location
//     -add the builder name to the list of builders in dashboard_base.js.

function generatePage(historyInstance)
{
    var html = ui.html.testTypeSwitcher(true) + '<br>';
    for (var builder in currentBuilders())
        html += htmlForBuilder(builder);
    document.body.innerHTML = html;
}

function handleValidHashParameter(historyInstance, key, value)
{
    switch(key) {
    case 'rawValues':
        historyInstance.dashboardSpecificState[key] = value == 'true';
        return true;

    default:
        return false;
    }
}

var defaultDashboardSpecificStateValues = {
    rawValues: false
};


var aggregateResultsConfig = {
    defaultStateValues: defaultDashboardSpecificStateValues,
    generatePage: generatePage,
    handleValidHashParameter: handleValidHashParameter,
};

// FIXME(jparent): Eventually remove all usage of global history object.
var g_history = new history.History(aggregateResultsConfig);
g_history.parseCrossDashboardParameters();

function htmlForBuilder(builder)
{
    var results = g_resultsByBuilder[builder];
    // Some keys were added later than others, so they don't have as many
    // builds. Use the shortest.
    // FIXME: Once 500 runs have finished, we can get rid of passing this
    // around and just assume all keys have the same number of builders for a
    // given builder.
    var numColumns = results[ALL_FIXABLE_COUNT_KEY].length;
    var html = '<div class=container><h2>' + builder + '</h2>';

    if (g_history.dashboardSpecificState.rawValues)
        html += rawValuesHTML(results, numColumns);
    else {
        html += '<a href="timeline_explorer.html' + (location.hash ? location.hash + '&' : '#') + 'builder=' + builder + '">' +
            chartHTML(results, numColumns) + '</a>';
    }

    html += '</div>';
    return html;
}

function rawValuesHTML(results, numColumns)
{
    var html = htmlForSummaryTable(results, numColumns) +
        htmlForTestType(results, FIXABLE_COUNTS_KEY, FIXABLE_DESCRIPTION, numColumns);
    if (g_history.isLayoutTestResults()) {
        html += htmlForTestType(results, DEFERRED_COUNTS_KEY, DEFERRED_DESCRIPTION, numColumns) +
            htmlForTestType(results, WONTFIX_COUNTS_KEY, WONTFIX_DESCRIPTION, numColumns);
    }
    return html;
}

function chartHTML(results, numColumns)
{
    var revisionKey = WEBKIT_REVISIONS_KEY;
    var revisionLabel = "WebKit Revision";
    var startRevision = results[revisionKey][numColumns - 1];
    var endRevision = results[revisionKey][0];

    var fixable = results[FIXABLE_COUNT_KEY].slice(0, numColumns);
    var html = chart("Total failing", {"": fixable}, revisionLabel, startRevision, endRevision);

    var values = valuesPerExpectation(results[FIXABLE_COUNTS_KEY], numColumns);
    // Don't care about number of passes for the charts.
    delete(values['P']);

    return html + chart("Detailed breakdown", values, revisionLabel, startRevision, endRevision);
}

var LABEL_COLORS = ['FF0000', '00FF00', '0000FF', '000000', 'FF6EB4', 'FFA812', '9B30FF', '00FFCC'];

// FIXME: Find a better way to exclude outliers. This is just so we exclude
// runs where every test failed.
var MAX_VALUE = 10000;

function filteredValues(values, desiredNumberOfPoints)
{
    // Filter out values to make the graph a bit more readable and to keep URLs
    // from exceeding the browsers max length restriction.
    var filterAmount = Math.floor(values.length / desiredNumberOfPoints);
    if (filterAmount < 1)
        return values;

    return values.filter(function(element, index, array) {
        // Include the most recent and oldest values and exclude outliers.
        return (index % filterAmount == 0 || index == array.length - 1) && (array[index] < MAX_VALUE && array[index] != 0);
    });
}

function chartUrl(title, values, revisionLabel, startRevision, endRevision, desiredNumberOfPoints) {
    var maxValue = 0;
    for (var expectation in values)
        maxValue = Math.max(maxValue, Math.max.apply(null, filteredValues(values[expectation], desiredNumberOfPoints)));

    var chartData = '';
    var labels = '';
    var numLabels = 0;

    var first = true;
    for (var expectation in values) {
        chartData += (first ? 'e:' : ',') + extendedEncode(filteredValues(values[expectation], desiredNumberOfPoints).reverse(), maxValue);

        if (expectation) {
            numLabels++;
            labels += (first ? '' : '|') + expectationsMap()[expectation];
        }
        first = false;
    }

    var url = "http://chart.apis.google.com/chart?cht=lc&chs=600x400&chd=" +
            chartData + "&chg=15,15,1,3&chxt=x,x,y&chxl=1:||" + revisionLabel +
            "|&chxr=0," + startRevision + "," + endRevision + "|2,0," + maxValue + "&chtt=" + title;


    if (labels)
        url += "&chdl=" + labels + "&chco=" + LABEL_COLORS.slice(0, numLabels).join(',');
    return url;
}

function chart(title, values, revisionLabel, startRevision, endRevision)
{
    var desiredNumberOfPoints = 400;
    var url = chartUrl(title, values, revisionLabel, startRevision, endRevision, desiredNumberOfPoints);

    while (url.length >= 2048) {
        // Decrease the desired number of points gradually until we get a URL that
        // doesn't exceed chartserver's max URL length.
        desiredNumberOfPoints = 3 / 4 * desiredNumberOfPoints;
        url = chartUrl(title, values, revisionLabel, startRevision, endRevision, desiredNumberOfPoints);
    }

    return '<img src="' + url + '">';
}

function wrapHTMLInTable(description, html)
{
    return '<h3>' + description + '</h3><table><tbody>' + html + '</tbody></table>';
}

function htmlForSummaryTable(results, numColumns)
{
    var percent = [];
    var fixable = results[FIXABLE_COUNT_KEY].slice(0, numColumns);
    var allFixable = results[ALL_FIXABLE_COUNT_KEY].slice(0, numColumns);
    for (var i = 0; i < numColumns; i++) {
        var percentage = 100 * (allFixable[i] - fixable[i]) / allFixable[i];
        // Round to the nearest tenth of a percent.
        percent.push(Math.round(percentage * 10) / 10 + '%');
    }
    var html = htmlForTableRow('WebKit Revision', results[WEBKIT_REVISIONS_KEY].slice(0, numColumns)) +
        htmlForTableRow('Percent passed', percent) +
        htmlForTableRow('Failures (deduped)', fixable) +
        htmlForTableRow('Fixable Tests', allFixable);
    return wrapHTMLInTable('Summary', html);
}

function valuesPerExpectation(counts, numColumns)
{
    var values = {};
    for (var i = 0; i < numColumns; i++) {
        for (var expectation in expectationsMap()) {
            if (expectation in counts[i]) {
                var count = counts[i][expectation];
                if (!values[expectation])
                    values[expectation] = [];
                values[expectation].push(count);
            }
        }
    }
    return values;
}

function htmlForTestType(results, key, description, numColumns)
{
    var counts = results[key];
    var html = htmlForTableRow('WebKit Revision', results[WEBKIT_REVISIONS_KEY].slice(0, numColumns));
    var values = valuesPerExpectation(counts, numColumns);
    for (var expectation in values)
        html += htmlForTableRow(expectationsMap()[expectation], values[expectation]);
    return wrapHTMLInTable(description, html);
}

function htmlForTableRow(columnName, values)
{
    return '<tr><td>' + columnName + '</td><td>' + values.join('</td><td>') + '</td></tr>';
}

// Taken from http://code.google.com/apis/chart/docs/data_formats.html.
function extendedEncode(arrVals, maxVal)
{
    var map = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.';
    var mapLength = map.length;
    var mapLengthSquared = mapLength * mapLength;

    var chartData = '';

    for (var i = 0, len = arrVals.length; i < len; i++) {
        // In case the array vals were translated to strings.
        var numericVal = new Number(arrVals[i]);
        // Scale the value to maxVal.
        var scaledVal = Math.floor(mapLengthSquared * numericVal / maxVal);

        if(scaledVal > mapLengthSquared - 1)
            chartData += "..";
        else if (scaledVal < 0)
            chartData += '__';
        else {
            // Calculate first and second digits and add them to the output.
            var quotient = Math.floor(scaledVal / mapLength);
            var remainder = scaledVal - mapLength * quotient;
            chartData += map.charAt(quotient) + map.charAt(remainder);
        }
    }

    return chartData;
}

window.addEventListener('load', function() {
    var resourceLoader = new loader.Loader();
    resourceLoader.load();
}, false);
