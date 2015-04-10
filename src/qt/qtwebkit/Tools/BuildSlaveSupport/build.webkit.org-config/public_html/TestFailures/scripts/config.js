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

var config = config || {};

(function() {

config.kBuildNumberLimit = 20;

config.kPlatforms = {
    'apple' : {
        label : 'Apple',
        buildConsoleURL: 'http://build.webkit.org',
        layoutTestResultsURL: 'http://build.webkit.org/results',
        waterfallURL: 'http://build.webkit.org/waterfall',
        builders: {
            'Apple Lion Release WK1 (Tests)' : {version: 'lion' },
            'Apple Lion Debug WK1 (Tests)' : {version: 'lion', debug: true},
            'Apple Lion Release WK2 (Tests)' : {version: 'lion' },
            'Apple Lion Debug WK2 (Tests)' : {version: 'lion', debug: true},
            'Apple MountainLion Release WK1 (Tests)' : {version: 'mountainlion' },
            'Apple MountainLion Debug WK1 (Tests)' : {version: 'mountainlion', debug: true},
            'Apple MountainLion Release WK2 (Tests)' : {version: 'mountainlion' },
            'Apple MountainLion Debug WK2 (Tests)' : {version: 'mountainlion', debug: true},
            // 'Apple Win XP Debug (Tests)' : {version: 'xp',debug: true},
            // 'Apple Win 7 Release (Tests)' : {version: 'win7'},
        },
        haveBuilderAccumulatedResults : false,
        useDirectoryListingForOldBuilds: false,
        useFlakinessDashboard: false,
        resultsDirectoryNameFromBuilderName: function(builderName) {
            return encodeURIComponent(builderName);
        },
        resultsDirectoryForBuildNumber: function(buildNumber, revision) {
            return encodeURIComponent('r' + revision + ' (' + buildNumber + ')');
        },
        _builderApplies: function(builderName) {
            return builderName.indexOf('Apple') != -1;
        },
    },
    'gtk' : {
        label : 'GTK',
        buildConsoleURL: 'http://build.webkit.org',
        layoutTestResultsURL: 'http://build.webkit.org/results',
        waterfallURL: 'http://build.webkit.org/waterfall',
        builders: {
            'GTK Linux 32-bit Release' : {version: '32-bit release'},
            'GTK Linux 64-bit Release' : {version: '64-bit release'},
            'GTK Linux 64-bit Debug' : {version: '64-bit debug', debug: true},
        },
        haveBuilderAccumulatedResults : false,
        useDirectoryListingForOldBuilds: false,
        useFlakinessDashboard: false,
        resultsDirectoryNameFromBuilderName: function(builderName) {
            return encodeURIComponent(builderName);
        },
        resultsDirectoryForBuildNumber: function(buildNumber, revision) {
            return encodeURIComponent('r' + revision + ' (' + buildNumber + ')');
        },
        _builderApplies: function(builderName) {
            return builderName.indexOf('GTK') != -1;
        },
    },
    'qt' : {
        label : 'Qt',
        buildConsoleURL: 'http://build.webkit.org',
        layoutTestResultsURL: 'http://build.webkit.org/results',
        waterfallURL: 'http://build.webkit.org/waterfall',
        builders: {
            'Qt Linux Release' : {version : '32-bit release'},
        },
        haveBuilderAccumulatedResults : false,
        useDirectoryListingForOldBuilds: false,
        useFlakinessDashboard: false,
        resultsDirectoryNameFromBuilderName: function(builderName) {
            return encodeURIComponent(builderName);
        },
        resultsDirectoryForBuildNumber: function(buildNumber, revision) {
            return encodeURIComponent('r' + revision + ' (' + buildNumber + ')');
        },
        _builderApplies: function(builderName) {
            return builderName.indexOf('Qt') != -1;
        },
    },
    'efl' : {
        label : 'EFL',
        buildConsoleURL: 'http://build.webkit.org',
        layoutTestResultsURL: 'http://build.webkit.org/results',
        waterfallURL: 'http://build.webkit.org/waterfall',
        builders: {
            'EFL Linux 64-bit Debug WK2' : {version : '64-bit WK2', debug: true},
            'EFL Linux 64-bit Release WK2' : {version: '64-bit WK2'},
            'EFL Linux 64-bit Release' : {version: '64-bit'},
        },
        haveBuilderAccumulatedResults : false,
        useDirectoryListingForOldBuilds: false,
        useFlakinessDashboard: false,
        resultsDirectoryNameFromBuilderName: function(builderName) {
            return encodeURIComponent(builderName);
        },
        resultsDirectoryForBuildNumber: function(buildNumber, revision) {
            return encodeURIComponent('r' + revision + ' (' + buildNumber + ')');
        },
        _builderApplies: function(builderName) {
            return builderName.indexOf('EFL') != -1;
        },
    },
};

config.kTracURL = 'http://trac.webkit.org';
config.kBugzillaURL = 'https://bugs.webkit.org';

config.kRevisionAttr = 'data-revision';
config.kTestNameAttr = 'data-test-name';
config.kBuilderNameAttr = 'data-builder-name';
config.kFailureCountAttr = 'data-failure-count';
config.kFailureTypesAttr = 'data-failure-types';
config.kInfobarTypeAttr = 'data-infobar-type';

var kTenMinutesInMilliseconds = 10 * 60 * 1000;
config.kUpdateFrequency = kTenMinutesInMilliseconds;
config.kRelativeTimeUpdateFrequency = 1000 * 60;

config.kExperimentalFeatures = window.location.search.search('enableExperiments=1') != -1;

config.currentPlatform = base.getURLParameter('platform') || 'apple';

// FIXME: We should add a way to restrict the results to a subset of the builders
// (or maybe just a single builder) in the UI as well as via an URL parameter.
config.currentBuilder = base.getURLParameter('builder');

config.currentBuilders = function() {
    var current_builders = {};
    if (config.currentBuilder) {
        current_builders[config.currentBuilder] = config.kPlatforms[config.currentPlatform].builders[config.currentBuilder];
        return current_builders;
    } else {
        return config.kPlatforms[config.currentPlatform].builders;
    }
};

config.builderApplies = function(builderName) {
    if (config.currentBuilder)
        return builderName == config.currentBuilder;
    return config.kPlatforms[config.currentPlatform]._builderApplies(builderName);
};

config.setPlatform = function(platform) {
    if (!this.kPlatforms[platform]) {
        window.console.log(platform + ' is not a recognized platform');
        return;
    }
    config.currentPlatform = platform;
};

config.useLocalResults = Boolean(base.getURLParameter('useLocalResults')) || false;

})();
