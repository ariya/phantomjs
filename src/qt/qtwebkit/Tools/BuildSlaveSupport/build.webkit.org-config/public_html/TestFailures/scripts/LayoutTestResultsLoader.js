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

function LayoutTestResultsLoader(builder) {
    this._builder = builder;
}

LayoutTestResultsLoader.prototype = {
    start: function(buildName, callback, errorCallback) {
        var cacheKey = 'LayoutTestResultsLoader.' + this._builder.name + '.' + buildName;
        const currentCachedDataVersion = 8;
        if (PersistentCache.contains(cacheKey)) {
            var cachedData = PersistentCache.get(cacheKey);
            if (cachedData.version === currentCachedDataVersion) {
                if (cachedData.error)
                    errorCallback(cachedData.tests, cachedData.tooManyFailures);
                else
                    callback(cachedData.tests, cachedData.tooManyFailures);
                return;
            }
        }

        var result = { tests: {}, tooManyFailures: false, error: false, version: currentCachedDataVersion };

        function cacheParseResultsAndCallCallback(parseResult) {
            result.tests = parseResult.tests;
            result.tooManyFailures = parseResult.tooManyFailures;

            PersistentCache.set(cacheKey, result);
            callback(result.tests, result.tooManyFailures);
        }

        var self = this;
        self._fetchAndParseNRWTResults(buildName, cacheParseResultsAndCallCallback, function() {
            self._fetchAndParseORWTResults(buildName, cacheParseResultsAndCallCallback, function() {
                // We couldn't fetch results for this build.
                result.error = true;
                PersistentCache.set(cacheKey, result);
                errorCallback(result.tests, result.tooManyFailures);
            });
        });
    },

    _fetchAndParseNRWTResults: function(buildName, successCallback, errorCallback) {
        getResource(this._builder.resultsDirectoryURL(buildName) + 'full_results.json', function(xhr) {
            successCallback((new NRWTResultsParser()).parse(xhr.responseText));
        },
        function(xhr) {
            errorCallback();
        });
    },

    _fetchAndParseORWTResults: function(buildName, successCallback, errorCallback) {
        var parsedBuildName = this._builder.buildbot.parseBuildName(buildName);

        // http://webkit.org/b/62380 was fixed in r89610.
        var resultsHTMLSupportsTooManyFailuresInfo = parsedBuildName.revision >= 89610;

        var result = { tests: {}, tooManyFailures: false };

        var self = this;

        function fetchAndParseResultsHTML(successCallback, errorCallback) {
            getResource(self._builder.resultsPageURL(buildName), function(xhr) {
                var parseResult = (new ORWTResultsParser()).parse(xhr.responseText);
                result.tests = parseResult.tests;
                if (resultsHTMLSupportsTooManyFailuresInfo)
                    result.tooManyFailures = parseResult.tooManyFailures;
                successCallback();
            },
            function(xhr) {
                // We failed to fetch results.html.
                errorCallback();
            });
        }

        function fetchNumberOfFailingTests(successCallback, errorCallback) {
            self._builder.getNumberOfFailingTests(parsedBuildName.buildNumber, function(failingTestCount, tooManyFailures) {
                result.tooManyFailures = tooManyFailures;

                if (failingTestCount < 0) {
                    // The number of failing tests couldn't be determined.
                    errorCallback();
                    return;
                }

                successCallback(failingTestCount);
            });
        }

        if (resultsHTMLSupportsTooManyFailuresInfo) {
            fetchAndParseResultsHTML(function() {
                successCallback(result);
            },
            function() {
                fetchNumberOfFailingTests(function(failingTestCount) {
                    if (!failingTestCount) {
                        // All tests passed, so no results.html was generated.
                        successCallback(result);
                        return;
                    }

                    // Something went wrong with fetching results.
                    errorCallback();
                }, errorCallback);
            });
            return;
        }

        fetchNumberOfFailingTests(function(failingTestCount) {
            if (!failingTestCount) {
                // All tests passed.
                successCallback(result);
                return;
            }

            // Find out which tests failed.
            fetchAndParseResultsHTML(function() {
                successCallback(result);
            }, errorCallback);
        }, errorCallback);
    },
};
