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

var defaultDashboardSpecificStateValues = {
    builder: null,
    treemapfocus: '',
};

var DB_SPECIFIC_INVALIDATING_PARAMETERS = {
    'testType': 'builder',
    'group': 'builder'
};

function generatePage(historyInstance)
{
    $('header-container').innerHTML = ui.html.testTypeSwitcher();

    g_isGeneratingPage = true;

    var rawTree = g_resultsByBuilder[historyInstance.dashboardSpecificState.builder || currentBuilderGroup().defaultBuilder()];
    g_webTree = convertToWebTreemapFormat('LayoutTests', rawTree);
    appendTreemap($('map'), g_webTree);

    if (historyInstance.dashboardSpecificState.treemapfocus)
        focusPath(g_webTree, historyInstance.dashboardSpecificState.treemapfocus)

    g_isGeneratingPage = false;
}

function handleValidHashParameter(historyInstance, key, value)
{
    switch(key) {
    case 'builder':
        history.validateParameter(historyInstance.dashboardSpecificState, key, value,
            function() { return value in currentBuilders(); });
        return true;

    case 'treemapfocus':
        history.validateParameter(historyInstance.dashboardSpecificState, key, value,
            function() {
                // FIXME: There's probably a simpler regexp here. Just trying to match ascii + forward-slash.
                // e.g. LayoutTests/foo/bar.html
                return (value.match(/^(\w+\/\w*)*$/));
            });
        return true;

    default:
        return false;
    }
}

function handleQueryParameterChange(historyInstance, params)
{
    for (var param in params) {
        if (param != 'treemapfocus') {
            $('map').innerHTML = 'Loading...';
            return true;
        }
    }
    return false;
}

var treemapConfig = {
    defaultStateValues: defaultDashboardSpecificStateValues,
    generatePage: generatePage,
    handleValidHashParameter: handleValidHashParameter,
    handleQueryParameterChange: handleQueryParameterChange,
    invalidatingHashParameters: DB_SPECIFIC_INVALIDATING_PARAMETERS
};

// FIXME(jparent): Eventually remove all usage of global history object.
var g_history = new history.History(treemapConfig);
g_history.parseCrossDashboardParameters();

var TEST_URL_BASE_PATH = "http://svn.webkit.org/repository/webkit/trunk/";

function humanReadableTime(milliseconds)
{
    if (milliseconds < 1000)
        return Math.floor(milliseconds) + 'ms';
    else if (milliseconds < 60000)
        return (milliseconds / 1000).toPrecision(2) + 's';

    var minutes = Math.floor(milliseconds / 60000);
    var seconds = Math.floor((milliseconds - minutes * 60000) / 1000);
    return minutes + 'm' + seconds + 's';
}

// This looks like:
// { "data": {"$area": (sum of all timings)},
//   "name": (name of this node),
//   "children": [ (child nodes, in the same format as this) ] }
// childCount is added just to be includes in the node's name
function convertToWebTreemapFormat(treename, tree, path)
{
    var total = 0;
    var childCount = 0;
    var children = [];
    for (var name in tree) {
        var treeNode = tree[name];
        if (typeof treeNode == "number") {
            var time = treeNode;
            var node = {
                "data": {"$area": time},
                "name": name + " (" + humanReadableTime(time) + ")"
            };
            children.push(node);
            total += time;
            childCount++;
        } else {
            var newPath = path ? path + '/' + name : name;
            var subtree = convertToWebTreemapFormat(name, treeNode, newPath);
            children.push(subtree);
            total += subtree["data"]["$area"];
            childCount += subtree["childCount"];
        }
    }

    children.sort(function(a, b) {
        aTime = a.data["$area"]
        bTime = b.data["$area"]
        return bTime - aTime;
    });

    return {
        "data": {"$area": total},
        "name": treename + " (" + humanReadableTime(total) + " - " + childCount + " tests)",
        "children": children,
        "childCount": childCount,
        "path": path
    };
}

function listOfAllNonLeafNodes(tree, list)
{
    if (!tree.children)
        return;

    if (!list)
        list = [];
    list.push(tree);

    tree.children.forEach(function(child) {
        listOfAllNonLeafNodes(child, list);
    });
    return list;
}

function reverseSortByAverage(list)
{
    list.sort(function(a, b) {
        var avgA = a.data['$area'] / a.childCount;
        var avgB = b.data['$area'] / b.childCount;
        return avgB - avgA;
    });
}

function showAverages()
{
    if (!document.getElementById('map'))
        return;

    var table = document.createElement('table');
    table.innerHTML = '<th>directory</th><th># tests</th><th>avg time / test</th>';

    var allNodes = listOfAllNonLeafNodes(g_webTree);
    reverseSortByAverage(allNodes);
    allNodes.forEach(function(node) {
        var average = node.data['$area'] / node.childCount;
        if (average > 100 && node.childCount != 1) {
            var tr = document.createElement('tr');
            tr.innerHTML = '<td></td><td>' + node.childCount + '</td><td>' + humanReadableTime(average) + '</td>';
            tr.querySelector('td').innerText = node.path;
            table.appendChild(tr);
        }
    });

    var map = document.getElementById('map');
    map.parentNode.replaceChild(table, map);
}

var g_isGeneratingPage = false;
var g_webTree;

function focusPath(tree, path)
{
    var parts = decodeURIComponent(path).split('/');
    if (extractName(tree) != parts[0]) {
        console.error('Could not focus tree rooted at ' + parts[0]);
        return;
    }

    for (var i = 1; i < parts.length; i++) {
        var children = tree.children;
        for (var j = 0; j < children.length; j++) {
            var child = children[j];
            if (extractName(child) == parts[i]) {
                tree = child;
                focus(tree);
                break;
            }
        }
        if (j == children.length) {
            console.error('Could not find tree at ' + parts[i]);
            break;
        }
    }

}

function extractName(node)
{
    return node.name.split(' ')[0];
}

function fullName(node)
{
    var buffer = [extractName(node)];
    while (node.parent) {
        node = node.parent;
        buffer.unshift(extractName(node));
    }
    return buffer.join('/');
}

function handleFocus(tree)
{
    var currentlyFocusedNode = $('focused-leaf');
    if (currentlyFocusedNode)
        currentlyFocusedNode.id = '';

    if (!tree.children)
        tree.dom.id = 'focused-leaf';

    var name = fullName(tree);

    if (!tree.children && !tree.extraDom && g_history.isLayoutTestResults()) {
        tree.extraDom = document.createElement('pre');
        tree.extraDom.className = 'extra-dom';
        tree.dom.appendChild(tree.extraDom);

        loader.request(TEST_URL_BASE_PATH + name,
            function(xhr) {
                tree.extraDom.onmousedown = function(e) {
                    e.stopPropagation();
                };
                tree.extraDom.textContent = xhr.responseText;
            },
            function (xhr) {
                tree.extraDom.textContent = "Could not load test."
        });
    }

    // We don't want the focus calls during generatePage to try to modify the query state.
    if (!g_isGeneratingPage)
        g_history.setQueryParameter('treemapfocus', name);
}

window.addEventListener('load', function() {
    var resourceLoader = new loader.Loader();
    resourceLoader.load();
}, false);
