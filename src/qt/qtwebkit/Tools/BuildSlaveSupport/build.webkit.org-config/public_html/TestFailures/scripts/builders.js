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

var builders = builders || {};

(function() {

var kUpdateStepName = 'update';
var kUpdateScriptsStepName = 'update_scripts';
var kCompileStepName = 'compile';
var kWebKitTestsStepNames = ['webkit_tests', 'layout-test'];

var kCrashedOrHungOutputMarker = 'crashed or hung';

function buildBotURL(platform)
{
    return config.kPlatforms[platform].buildConsoleURL;
}

function urlForBuilderInfo(platform, builderName)
{
    return buildBotURL(platform) + '/json/builders/' + encodeURIComponent(builderName) + '/';
}

function urlForBuildInfo(platform, builderName, buildNumber)
{
    return buildBotURL(platform) + '/json/builders/' + encodeURIComponent(builderName) + '/builds/' + encodeURIComponent(buildNumber);
}

function didFail(step)
{
    if (kWebKitTestsStepNames.indexOf(step.name) != -1) {
        // run-webkit-tests fails to generate test coverage when it crashes or hangs.
        // FIXME: Do build.webkit.org bots output this marker when the tests fail to run?
        return step.text.indexOf(kCrashedOrHungOutputMarker) != -1;
    }
    // The first item in step.results is the success of the step:
    // 0 == pass, 1 == warning, 2 == fail
    return step.results[0] == 2;
}

function failingSteps(buildInfo)
{
    return buildInfo.steps.filter(didFail);
}

function mostRecentCompletedBuildNumber(individualBuilderStatus)
{
    if (!individualBuilderStatus)
        return null;

    for (var i = individualBuilderStatus.cachedBuilds.length - 1; i >= 0; --i) {
        var buildNumber = individualBuilderStatus.cachedBuilds[i];
        if (individualBuilderStatus.currentBuilds.indexOf(buildNumber) == -1)
            return buildNumber;
    }

    return null;
}

var g_buildInfoCache = new base.AsynchronousCache(function(key, callback) {
    var explodedKey = key.split('\n');
    net.get(urlForBuildInfo(explodedKey[0], explodedKey[1], explodedKey[2]), callback);
});

builders.clearBuildInfoCache = function()
{
    g_buildInfoCache.clear();
}

function fetchMostRecentBuildInfoByBuilder(platform, callback)
{
    net.get(buildBotURL(platform) + '/json/builders', function(builderStatus) {
        var buildInfoByBuilder = {};
        var builderNames = Object.keys(builderStatus);
        var requestTracker = new base.RequestTracker(builderNames.length, callback, [buildInfoByBuilder]);
        builderNames.forEach(function(builderName) {
            if (!config.builderApplies(builderName)) {
                requestTracker.requestComplete();
                return;
            }

            var buildNumber = mostRecentCompletedBuildNumber(builderStatus[builderName]);
            if (!buildNumber) {
                buildInfoByBuilder[builderName] = null;
                requestTracker.requestComplete();
                return;
            }

            g_buildInfoCache.get(platform + '\n' + builderName + '\n' + buildNumber, function(buildInfo) {
                buildInfoByBuilder[builderName] = buildInfo;
                requestTracker.requestComplete();
            });
        });
    });
}

builders.builderInfo = function(platform, builderName, callback)
{
    var builderInfoURL = urlForBuilderInfo(platform, builderName);
    net.get(builderInfoURL, callback);
};

builders.cachedBuildInfos = function(platform, builderName, callback)
{
    var builderInfoURL = urlForBuilderInfo(platform, builderName);
    net.get(builderInfoURL, function(builderInfo) {
        var selectURL = urlForBuilderInfo(platform, builderName) + 'builds';
        var start = Math.max(0, builderInfo.cachedBuilds.length - config.kBuildNumberLimit);
        var selectParams = { select : builderInfo.cachedBuilds.slice(start) };
        var traditionalEncoding = true;
        selectURL += '?' + $.param(selectParams, traditionalEncoding);
        net.get(selectURL, callback);
    });
}

builders.recentBuildInfos = function(callback)
{
    fetchMostRecentBuildInfoByBuilder(config.currentPlatform, function(buildInfoByBuilder) {
        var buildInfo = {};
        $.each(buildInfoByBuilder, function(builderName, thisBuildInfo) {
            if (!buildInfo)
                return;
            
            buildInfo[builderName] = thisBuildInfo;
        });
        callback(buildInfo);
    });
};

builders.buildersFailingNonLayoutTests = function(callback)
{
    fetchMostRecentBuildInfoByBuilder(config.currentPlatform, function(buildInfoByBuilder) {
        var failureList = {};
        $.each(buildInfoByBuilder, function(builderName, buildInfo) {
            if (!buildInfo)
                return;
            var failures = failingSteps(buildInfo);
            if (failures.length)
                failureList[builderName] = failures.map(function(failure) { return failure.name; });
        });
        callback(failureList);
    });
};

builders.perfBuilders = function(callback)
{
    fetchMostRecentBuildInfoByBuilder(config.currentPlatform, function(buildInfoByBuilder) {
        var perfTestMap = {};
        $.each(buildInfoByBuilder, function(builderName, buildInfo) {
            if (!buildInfo || builderName.indexOf('Perf') == -1)
                return;

            buildInfo.steps.forEach(function(step) {
                // FIXME: If the compile is broken, grab an older build.
                // If the compile/update is broken, no steps will have a results url.
                if (!step.urls.results)
                    return;
                if (!perfTestMap[step.name])
                    perfTestMap[step.name] = [];
                perfTestMap[step.name].push({ builder: builderName, url: step.urls.results });
            });
        });
        callback(perfTestMap);
    });
}

})();
