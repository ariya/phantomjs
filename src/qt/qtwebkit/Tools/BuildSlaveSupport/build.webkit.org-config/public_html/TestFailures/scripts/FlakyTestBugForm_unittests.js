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

(function() {

module('FlakyTestBugForm');

function createTestForm(failingBuildNames, failingTest, oldestAnalyzedBuild, newestAnalyzedBuild, analyzedBuildCount) {
    var mockBuildbot = {};
    mockBuildbot.parseBuildName = function(buildName) {
        var match = /(\d+)/.exec(buildName);
        return {
            revision: parseInt(match[1], 10),
            buildNumber: parseInt(match[2], 10),
        };
    };

    var mockBuilder = {};
    mockBuilder.name = '[BUILDER NAME]';
    mockBuilder.buildbot = mockBuildbot;
    mockBuilder.resultsPageURL = function(buildName) {
        return '[RESULTS PAGE URL ' + this.name + ', ' + buildName + ']';
    }

    return new FlakyTestBugForm(mockBuilder, failingBuildNames, failingTest, oldestAnalyzedBuild, newestAnalyzedBuild, analyzedBuildCount);
}

const testCases = [
    {
        oldestAnalyzedBuild: 'r1 (1)',
        newestAnalyzedBuild: 'r15 (8)',
        analyzedBuildCount: 8,
        failingBuildNames: [
            'r10 (5)',
            'r8 (2)',
        ],
        failingTest: 'css1/basic/class_as_selector.html',
        expectedDescription: 'css1/basic/class_as_selector.html failed 2 out of 8 times on [BUILDER NAME] between r1 and r15 (inclusive).\n\nFailures:\n\n[RESULTS PAGE URL [BUILDER NAME], r10 (5)]\n[RESULTS PAGE URL [BUILDER NAME], r8 (2)]\n',
        expectedTitle: 'css1/basic/class_as_selector.html sometimes fails on [BUILDER NAME]',
        expectedURL: '[RESULTS PAGE URL [BUILDER NAME], r10 (5)]',
    },
];

test('titles', 1, function() {
    testCases.forEach(function(testCase) {
        var form = createTestForm(testCase.failingBuildNames, testCase.failingTest, testCase.oldestAnalyzedBuild, testCase.newestAnalyzedBuild, testCase.analyzedBuildCount);
        equal(form.title, testCase.expectedTitle);
    });
});

test('descriptions', 1, function() {
    testCases.forEach(function(testCase) {
        var form = createTestForm(testCase.failingBuildNames, testCase.failingTest, testCase.oldestAnalyzedBuild, testCase.newestAnalyzedBuild, testCase.analyzedBuildCount);
        equal(form.description, testCase.expectedDescription);
    });
});

test('URLs', 1, function() {
    testCases.forEach(function(testCase) {
        var form = createTestForm(testCase.failingBuildNames, testCase.failingTest, testCase.oldestAnalyzedBuild, testCase.newestAnalyzedBuild, testCase.analyzedBuildCount);
        equal(form.url, testCase.expectedURL);
    });
});

})();
