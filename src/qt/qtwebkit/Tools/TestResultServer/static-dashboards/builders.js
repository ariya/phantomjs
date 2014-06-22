// Copyright (C) 2012 Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
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

// @fileoverview File that lists builders, their masters, and logical groupings
// of them.

function LOAD_BUILDBOT_DATA(builderData)
{
    builders.masters = {};
    builderData.forEach(function(master) {
        builders.masters[master.name] = new builders.BuilderMaster(master.name, master.url, master.tests);
    })
}

var builders = builders || {};

(function() {

// FIXME: Move some of this loading logic into loader.js.

builders._loadScript = function(url, success, error)
{
    var script = document.createElement('script');
    script.src = url;
    script.onload = success;
    script.onerror = error;
    document.head.appendChild(script);
}

builders._requestBuilders = function()
{
    var buildersUrl = 'builders.jsonp';
    builders._loadScript(buildersUrl, function() {}, function() {
        console.error('Could not load ' + buildersUrl);
    });
}


builders.BuilderMaster = function(name, basePath, tests)
{
    this.name = name;
    this.basePath = basePath;
    this.tests = tests;
}

builders.BuilderMaster.prototype = {
    logPath: function(builder, buildNumber)
    {
        return this.basePath + '/builders/' + builder + '/builds/' + buildNumber;
    },
    builderJsonPath: function()
    {
        return this.basePath + '/json/builders';
    },
}

builders._requestBuilders();

})();

// FIXME: Move everything below into the anonymous namespace above.

WEBKIT_BUILDER_MASTER = 'webkit.org';
var LEGACY_BUILDER_MASTERS_TO_GROUPS = {
    'webkit.org': '@ToT - webkit.org'
};

function BuilderGroup()
{
    // Map of builderName (the name shown in the waterfall) to builderPath (the
    // path used in the builder's URL)
    this.builders = {};
}

BuilderGroup.prototype.append = function(builders) {
    builders.forEach(function(builderName) {
        this.builders[builderName] = builderName.replace(/[ .()]/g, '_');
    }, this);
};

BuilderGroup.prototype.defaultBuilder = function()
{
    for (var builder in this.builders)
        return builder;
    console.error('There are no builders in this builder group.');
}

BuilderGroup.prototype.master = function()
{
    return builderMaster(this.defaultBuilder());
}

var BUILDER_TO_MASTER = {};

function builderMaster(builderName)
{
    return BUILDER_TO_MASTER[builderName];
}

function requestBuilderList(builderGroups, masterName, groupName, builderGroup, testType, opt_builderFilter)
{
    if (!builderGroups[groupName])
        builderGroups[groupName] = builderGroup;
    var master = builders.masters[masterName];
    var builderList = master.tests[testType].builders;
    if (opt_builderFilter)
        builderList = builderList.filter(opt_builderFilter);
    builderList.forEach(function(builderName) {
        BUILDER_TO_MASTER[builderName] = master;
    });
    builderGroups[groupName].append(builderList);
}

// FIXME: Look into whether we can move the grouping logic into builders.jsonp and get rid of this code.
function loadBuildersList(groupName, testType) {
    switch (testType) {
    case 'layout-tests':
        switch(groupName) {
        case '@ToT - webkit.org':
            var builderGroup = new BuilderGroup();
            requestBuilderList(LAYOUT_TESTS_BUILDER_GROUPS, WEBKIT_BUILDER_MASTER, groupName, builderGroup, testType);
            break;
        default:
            console.error('Tried to load builders for an unsupported group "' + groupName + '"');
        }
        break;

    default:
        console.error('Tried to load builders for an unsupported test type "' + testType + '"');
    }
}

var LAYOUT_TESTS_BUILDER_GROUPS = {
    '@ToT - webkit.org': null,
};
