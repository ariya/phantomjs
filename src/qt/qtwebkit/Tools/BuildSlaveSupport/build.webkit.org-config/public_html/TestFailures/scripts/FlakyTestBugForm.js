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

function FlakyTestBugForm(tester, failingBuildNames, failingTest, oldestAnalyzedBuild, newestAnalyzedBuild, analyzedBuildCount) {
    TestRelatedBugForm.call(this, tester);

    this._failingBuildNames = failingBuildNames;
    this._failingTest = failingTest;
    this._oldestAnalyzedBuild = oldestAnalyzedBuild;
    this._newestAnalyzedBuild = newestAnalyzedBuild;
    this._analyzedBuildCount = analyzedBuildCount;

    this.description = this._createBugDescription();
    this.title = this._createBugTitle();
    this.url = this._tester.resultsPageURL(this._failingBuildNames[0]);
}

FlakyTestBugForm.prototype = {
    _createBugDescription: function() {
        var self = this;

        var result = self._failingTest
            + ' failed ' + self._failingBuildNames.length + ' out of ' + self._analyzedBuildCount + ' times'
            + ' on ' + self._tester.name
            + ' between r' + self._tester.buildbot.parseBuildName(self._oldestAnalyzedBuild).revision
            + ' and r' + self._tester.buildbot.parseBuildName(self._newestAnalyzedBuild).revision
            + ' (inclusive).\n\n';

        result += 'Failures:\n\n';
        result += self._failingBuildNames.map(function(buildName) {
            return self._tester.resultsPageURL(buildName) + '\n';
        }).join('');

        return result;
    },

    _createBugTitle: function() {
        return this._failingTest + ' sometimes fails on ' + this._tester.name;
    },
};

FlakyTestBugForm.prototype.__proto__ = TestRelatedBugForm.prototype;
