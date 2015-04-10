// Copyright (C) 2013 Google Inc. All rights reserved.
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

var ui = ui || {};

(function() {

ui.popup = {};

ui.popup.hide = function()
{
    var popup = $('popup');
    if (popup) {
        popup.parentNode.removeChild(popup);
        document.removeEventListener('mousedown', ui.popup._handleMouseDown, false);
    }
}

ui.popup.show = function(target, html)
{
    var popup = $('popup');
    if (!popup) {
        popup = document.createElement('div');
        popup.id = 'popup';
        document.body.appendChild(popup);
        document.addEventListener('mousedown', ui.popup._handleMouseDown, false);
    }

    // Set html first so that we can get accurate size metrics on the popup.
    popup.innerHTML = html;

    var targetRect = target.getBoundingClientRect();

    var x = Math.min(targetRect.left - 10, document.documentElement.clientWidth - popup.offsetWidth);
    x = Math.max(0, x);
    popup.style.left = x + document.body.scrollLeft + 'px';

    var y = targetRect.top + targetRect.height;
    if (y + popup.offsetHeight > document.documentElement.clientHeight)
        y = targetRect.top - popup.offsetHeight;
    y = Math.max(0, y);
    popup.style.top = y + document.body.scrollTop + 'px';
}

ui.popup._handleMouseDown = function(e) {
    // Clear the open popup, unless the click was inside the popup.
    var popup = $('popup');
    if (popup && e.target != popup && !(popup.compareDocumentPosition(e.target) & 16)) 
        ui.popup.hide();    
}

ui.html = {};

ui.html.checkbox = function(queryParameter, label, isChecked, opt_extraJavaScript)
{
    var js = opt_extraJavaScript || '';
    return '<label style="padding-left: 2em">' +
        '<input type="checkbox" onchange="g_history.toggleQueryParameter(\'' + queryParameter + '\');' + js + '" ' +
            (isChecked ? 'checked' : '') + '>' + label +
        '</label> ';
}

ui.html.select = function(label, queryParameter, options)
{
    var html = '<label style="padding-left: 2em">' + label + ': ' +
        '<select onchange="g_history.setQueryParameter(\'' + queryParameter + '\', this[this.selectedIndex].value)">';

    for (var i = 0; i < options.length; i++) {
        var value = options[i];
        html += '<option value="' + value + '" ' +
            (g_history.queryParameterValue(queryParameter) == value ? 'selected' : '') +
            '>' + value + '</option>'
    }
    html += '</select></label> ';
    return html;
}

// Returns the HTML for the select element to switch to different testTypes.
ui.html.testTypeSwitcher = function(opt_noBuilderMenu, opt_extraHtml, opt_includeNoneBuilder)
{
    var html = '<div style="border-bottom:1px dashed">';
    html += '' +
        ui.html._dashboardLink('Stats', 'aggregate_results.html') +
        ui.html._dashboardLink('Timeline', 'timeline_explorer.html') +
        ui.html._dashboardLink('Results', 'flakiness_dashboard.html') +
        ui.html._dashboardLink('Treemap', 'treemap.html');

    html += ui.html.select('Test type', 'testType', TEST_TYPES);

    if (!opt_noBuilderMenu) {
        var buildersForMenu = Object.keys(currentBuilders());
        if (opt_includeNoneBuilder)
            buildersForMenu.unshift('--------------');
        html += ui.html.select('Builder', 'builder', buildersForMenu);
    }

    html += ui.html.select('Group', 'group',
        Object.keys(currentBuilderGroupCategory()));

    if (!history.isTreeMap())
        html += ui.html.checkbox('showAllRuns', 'Show all runs', g_history.crossDashboardState.showAllRuns);

    if (opt_extraHtml)
        html += opt_extraHtml;
    return html + '</div>';
}

ui.html._loadDashboard = function(fileName)
{
    var pathName = window.location.pathname;
    pathName = pathName.substring(0, pathName.lastIndexOf('/') + 1);
    window.location = pathName + fileName + window.location.hash;
}

ui.html._topLink = function(html, onClick, isSelected)
{
    var cssText = isSelected ? 'font-weight: bold;' : 'color:blue;text-decoration:underline;cursor:pointer;';
    cssText += 'margin: 0 5px;';
    return '<span style="' + cssText + '" onclick="' + onClick + '">' + html + '</span>';
}

ui.html._dashboardLink = function(html, fileName)
{
    var pathName = window.location.pathname;
    var currentFileName = pathName.substring(pathName.lastIndexOf('/') + 1);
    var isSelected = currentFileName == fileName;
    var onClick = 'ui.html._loadDashboard(\'' + fileName + '\')';
    return ui.html._topLink(html, onClick, isSelected);
}

ui.html._revisionLink = function(results, index, key, singleUrlTemplate, rangeUrlTemplate)
{
    var currentRevision = parseInt(results[key][index], 10);
    var previousRevision = parseInt(results[key][index + 1], 10);

    function singleUrl()
    {
        return singleUrlTemplate.replace('<rev>', currentRevision);
    }

    function rangeUrl()
    {
        return rangeUrlTemplate.replace('<rev1>', currentRevision).replace('<rev2>', previousRevision + 1);
    }

    if (currentRevision == previousRevision)
        return 'At <a href="' + singleUrl() + '">r' + currentRevision    + '</a>';
    else if (currentRevision - previousRevision == 1)
        return '<a href="' + singleUrl() + '">r' + currentRevision    + '</a>';
    else
        return '<a href="' + rangeUrl() + '">r' + (previousRevision + 1) + ' to r' + currentRevision + '</a>';
}

ui.html.chromiumRevisionLink = function(results, index)
{
    return ui.html._revisionLink(
        results,
        index,
        CHROME_REVISIONS_KEY,
        'http://src.chromium.org/viewvc/chrome?view=rev&revision=<rev>',
        'http://build.chromium.org/f/chromium/perf/dashboard/ui/changelog.html?url=/trunk/src&range=<rev2>:<rev1>&mode=html');
}

ui.html.webKitRevisionLink = function(results, index)
{
    return ui.html._revisionLink(
        results,
        index,
        WEBKIT_REVISIONS_KEY,
        'http://trac.webkit.org/changeset/<rev>',
        'http://trac.webkit.org/log/trunk/?rev=<rev1>&stop_rev=<rev2>&limit=100&verbose=on');
}


ui.Errors = function() {
    this._messages = '';
    // Element to display the errors within.
    this._containerElement = null;
}

ui.Errors.prototype = {
    show: function()
    {
        if (!this._containerElement) {
            this._containerElement = document.createElement('H2');
            this._containerElement.style.color = 'red';
            this._containerElement.id = 'errors';
            document.body.appendChild(this._containerElement);
        }

        this._containerElement.innerHTML = this._messages;
    },
    // Record a new error message.
    addError: function(message)
    {
        this._messages += message + '<br>';
    }
}

})();