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

module('TestRelatedBugForm');

function createTestForm(testerName) {
    var mockBuildbot = {};
    mockBuildbot.parseBuildName = function(buildName) {
        var match = /(\d+)/.exec(buildName);
        return {
            revision: parseInt(match[1], 10),
            buildNumber: parseInt(match[2], 10),
        };
    };

    var mockBuilder = {};
    mockBuilder.name = testerName;
    mockBuilder.buildbot = mockBuildbot;
    mockBuilder.resultsPageURL = function(buildName) {
        return '[RESULTS PAGE URL ' + this.name + ', ' + buildName + ']';
    }

    return new TestRelatedBugForm(mockBuilder);
}

test('component and keywords are set', 2, function() {
    var form = createTestForm('Windows 7 Release (Tests)');

    equal(form.component, WebKitBugzilla.Component.ToolsTests);
    deepEqual(form.keywords.split(', '), [WebKitBugzilla.Keyword.LayoutTestFailure, WebKitBugzilla.Keyword.MakingBotsRed]);
});

const testers = {
    'GTK Linux 32-bit Release': {
        operatingSystem: '',
        platform: '',
    },
    'Leopard Intel Release (Tests)': {
        operatingSystem: WebKitBugzilla.OperatingSystem.Leopard,
        platform: WebKitBugzilla.Platform.Macintosh,
    },
    'SnowLeopard Intel Release (Tests)': {
        operatingSystem: WebKitBugzilla.OperatingSystem.SnowLeopard,
        platform: WebKitBugzilla.Platform.Macintosh,
    },
    'Windows 7 Release (Tests)': {
        operatingSystem: WebKitBugzilla.OperatingSystem.Windows7,
        platform: WebKitBugzilla.Platform.PC,
    },
    'Windows XP Debug (Tests)': {
        operatingSystem: WebKitBugzilla.OperatingSystem.WindowsXP,
        platform: WebKitBugzilla.Platform.PC,
    },
};

test('operating system is deduced', 5, function() {
    for (var name in testers) {
        var form = createTestForm(name);
        equal(form.operatingSystem, testers[name].operatingSystem);
    }
});

test('platform is deduced', 5, function() {
    for (var name in testers) {
        var form = createTestForm(name);
        equal(form.platform, testers[name].platform);
    }
});

})();
