/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

function Builder(name, buildbot) {
    this.name = name;
    this.buildbot = buildbot;
    this._cache = {};
}

Builder.prototype = {
    buildURL: function(buildName) {
        return this.buildbot.buildURL(this.name, buildName);
    },

    failureDiagnosisTextAndURL: function(buildName, testName, testResult) {
        var urlStem = this.resultsDirectoryURL(buildName) + testName.replace(/\.[^.]+$/, '');
        var diagnosticInfo = {
            fail: {
                text: 'pretty diff',
                url: urlStem + '-pretty-diff.html',
            },
            flaky: {
                text: 'pretty diff (flaky)',
                url: urlStem + '-pretty-diff.html',
            },
            timeout: {
                text: 'timed out',
            },
            crash: {
                text: 'crash log',
                url: urlStem + '-crash-log.txt',
            },
            'webprocess crash': {
                text: 'web process crash log',
                url: urlStem + '-crash-log.txt',
            },
        };

        return diagnosticInfo[testResult.failureType];
    },

    getBuildNames: function(callback) {
        this._getBuildNamesFromResultsDirectory(this.buildbot.baseURL + 'results/' + this.name, callback);
    },

    getMostRecentCompletedBuildNumber: function(callback) {
        var cacheKey = 'getMostRecentCompletedBuildNumber';
        if (cacheKey in this._cache) {
            callback(this._cache[cacheKey]);
            return;
        }

        var self = this;
        getResource(self.buildbot.baseURL + 'json/builders/' + self.name, function(xhr) {
            var data = JSON.parse(xhr.responseText);

            var currentBuilds = {};
            if ('currentBuilds' in data)
                data.currentBuilds.forEach(function(buildNumber) { currentBuilds[buildNumber] = true });

            for (var i = data.cachedBuilds.length - 1; i >= 0; --i) {
                if (data.cachedBuilds[i] in currentBuilds)
                    continue;

                self._cache[cacheKey] = data.cachedBuilds[i];
                callback(data.cachedBuilds[i]);
                return;
            }

            self._cache[cacheKey] = -1;
            callback(self._cache[cacheKey]);
        },
        function(xhr) {
            self._cache[cacheKey] = -1;
            callback(self._cache[cacheKey]);
        });
    },

    getNumberOfFailingTests: function(buildNumber, callback) {
        var cacheKey = this.name + '_getNumberOfFailingTests_' + buildNumber;
        const currentCachedDataVersion = 3;
        if (PersistentCache.contains(cacheKey)) {
            var cachedData = PersistentCache.get(cacheKey);
            if (cachedData.version === currentCachedDataVersion) {
                callback(cachedData.failureCount, cachedData.tooManyFailures);
                return;
            }
        }

        var result = { failureCount: -1, tooManyFailures: false, version: currentCachedDataVersion };

        var self = this;
        self._getBuildJSON(buildNumber, function(data) {
            var layoutTestStep = data.steps.findFirst(function(step) { return step.name === 'layout-test'; });
            if (!layoutTestStep) {
                PersistentCache.set(cacheKey, result);
                callback(result.failureCount, result.tooManyFailures);
                return;
            }

            if (!('isFinished' in layoutTestStep)) {
                // run-webkit-tests never even ran, or didn't finish running.
                PersistentCache.set(cacheKey, result);
                callback(result.failureCount, result.tooManyFailures);
                return;
            }

            if (!('results' in layoutTestStep) || layoutTestStep.results[0] === 0) {
                if (!('times' in layoutTestStep) || layoutTestStep.times.length < 2 || layoutTestStep.times[1] - layoutTestStep.times[0] < self._minimumSuccessfulLayoutTestStepRunTime) {
                    // Either something caused the start/stop times not to be recorded, or
                    // run-webkit-tests ran so quickly that we can't believe there wasn't an error
                    // (e.g., a bug in the script that made it not find any tests to run).
                    PersistentCache.set(cacheKey, result);
                    callback(result.failureCount, result.tooManyFailures);
                    return;
                }

                // All tests passed.
                result.failureCount = 0;
                PersistentCache.set(cacheKey, result);
                callback(result.failureCount, result.tooManyFailures);
                return;
            }

            if (layoutTestStep.results[0] === 4) {
                // This build step was interrupted (perhaps due to the build slave restarting).
                PersistentCache.set(cacheKey, result);
                callback(result.failureCount, result.tooManyFailures);
                return;
            }

            if (/Exiting early/.test(layoutTestStep.results[1][0]))
                result.tooManyFailures = true;

            result.failureCount = layoutTestStep.results[1].reduce(function(sum, outputLine) {
                var match = /^(\d+)\s/.exec(outputLine);
                if (!match)
                    return sum;
                // Don't count leaks, new tests, or passes as failures.
                if (outputLine.contains('leak') || outputLine.contains('were new') || outputLine.contains('new passes'))
                    return sum;
                return sum + parseInt(match[1], 10);
            }, 0);

            if (!result.failureCount) {
                // run-webkit-tests exited with a non-zero exit status, but we
                // didn't find any output about the number of failed tests.
                // Something must have gone wrong (e.g., run-webkit-tests timed
                // out and was killed by buildbot).
                result.failureCount = -1;
            }

            PersistentCache.set(cacheKey, result);
            callback(result.failureCount, result.tooManyFailures);
        });
    },

    getOldBuildNames: function(callback) {
        this._getBuildNamesFromResultsDirectory(this.buildbot.baseURL + 'old-results/' + this.name, callback);
    },

    resultsDirectoryURL: function(buildName) {
        return this.buildbot.resultsDirectoryURL(this.name, buildName);
    },

    resultsPageURL: function(buildName) {
        return this.resultsDirectoryURL(buildName) + 'results.html';
    },

    _getBuildJSON: function(buildNumber, callback) {
        var cacheKey = 'getBuildJSON_' + buildNumber;
        if (cacheKey in this._cache) {
            callback(this._cache[cacheKey]);
            return;
        }

        var self = this;
        getResource(self.buildbot.baseURL + 'json/builders/' + self.name + '/builds/' + buildNumber, function(xhr) {
            var data = JSON.parse(xhr.responseText);
            self._cache[cacheKey] = data;
            callback(data);
        });
    },

    _getBuildNamesFromResultsDirectory: function(directoryURL, callback) {
        var cacheKey = '_getBuildNamesFromResultsDirectory.' + directoryURL;
        if (cacheKey in this._cache) {
            callback(this._cache[cacheKey]);
            return;
        }

        var self = this;

        function buildNamesFromDirectoryXHR(xhr) {
            var root = document.createElement('html');
            root.innerHTML = xhr.responseText;

            var buildNames = Array.prototype.map.call(root.querySelectorAll('td:first-child > a > b'), function(elem) {
                return elem.innerText.replace(/\/$/, '');
            }).filter(function(filename) {
                return self.buildbot.parseBuildName(filename);
            });
            buildNames.reverse();

            return buildNames;
        }

        getResource(directoryURL, function(xhr) {
            var buildNames = buildNamesFromDirectoryXHR(xhr);
            self._cache[cacheKey] = buildNames;
            callback(buildNames);
        });
    },

    // Any successful runs of run-webkit-tests that took less than this number of seconds are
    // assumed to be errors.
    _minimumSuccessfulLayoutTestStepRunTime: 20,
};
