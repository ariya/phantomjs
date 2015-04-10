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

function ViewController(buildbot) {
    this._buildbot = buildbot;
    this._navigationID = 0;

    var self = this;
    addEventListener('load', function() { self.loaded() }, false);
    addEventListener('hashchange', function() { self.parseHash(location.hash) }, false);
}

ViewController.prototype = {
    loaded: function() {
        this._header = document.createElement('h1');
        document.body.appendChild(this._header);
        this._mainContentElement = document.createElement('div');
        document.body.appendChild(this._mainContentElement);
        document.body.appendChild(this._domForAuxiliaryUIElements());

        this.parseHash(location.hash);
    },

    parseHash: function(hash) {
        ++this._navigationID;

        var match = /#\/(.*)/.exec(hash);
        if (match)
            this._displayBuilder(this._buildbot.builderNamed(decodeURIComponent(match[1])));
        else
            this._displayTesters();
    },

    _displayBuilder: function(builder) {
        this._setTitle(builder.name);
        this._mainContentElement.removeAllChildren();

        var navigationID = this._navigationID;

        var self = this;
        (new LayoutTestHistoryAnalyzer(builder)).start(function(data, stillFetchingData) {
            if (self._navigationID !== navigationID) {
                // The user has navigated somewhere else. Stop loading data about this tester.
                return false;
            }

            var list = document.createElement('ol');
            list.id = 'failure-history';

            var buildNames = Object.keys(data.history)
            buildNames.forEach(function(buildName, buildIndex, buildNameArray) {
                var failingTestNames = Object.keys(data.history[buildName].tests);
                if (!failingTestNames.length)
                    return;

                var item = document.createElement('li');
                list.appendChild(item);

                var testList = document.createElement('ol');
                item.appendChild(testList);

                testList.className = 'test-list';
                for (var testName in data.history[buildName].tests) {
                    var testItem = document.createElement('li');
                    testItem.appendChild(self._domForFailedTest(builder, buildName, testName, data.history[buildName].tests[testName]));
                    testList.appendChild(testItem);
                }

                if (data.history[buildName].tooManyFailures) {
                    var p = document.createElement('p');
                    p.className = 'info';
                    p.appendChild(document.createTextNode('run-webkit-tests exited early due to too many failures/crashes/timeouts'));
                    item.appendChild(p);
                }

                var passingBuildName;
                if (buildIndex + 1 < buildNameArray.length)
                    passingBuildName = buildNameArray[buildIndex + 1];

                item.appendChild(self._domForRegressionRange(builder, buildName, passingBuildName, failingTestNames));

                if (passingBuildName || !stillFetchingData) {
                    var bugForm = new FailingTestsBugForm(builder, buildName, passingBuildName, failingTestNames);
                    item.appendChild(self._domForNewAndExistingBugs(builder, failingTestNames, bugForm))
                }
            });

            self._mainContentElement.removeAllChildren();
            self._mainContentElement.appendChild(list);
            self._mainContentElement.appendChild(self._domForPossiblyFlakyTests(builder, data.possiblyFlaky, buildNames));

            if (!stillFetchingData)
                PersistentCache.prune();

            return true;
        });
    },

    _displayTesters: function() {
        this._setTitle('Testers');
        this._mainContentElement.removeAllChildren();

        var list = document.createElement('ul');
        this._mainContentElement.appendChild(list);

        var latestBuildInfos = [];
        var navigationID = this._navigationID;

        function updateList() {
            latestBuildInfos.sort(function(a, b) { return a.tester.name.localeCompare(b.tester.name) });
            list.removeAllChildren();
            latestBuildInfos.forEach(function(buildInfo) {
                var link = base.createLinkNode('#/' + buildInfo.tester.name, buildInfo.tester.name)
                var item = document.createElement('li');
                item.appendChild(link);
                if (buildInfo.tooManyFailures)
                    item.appendChild(document.createTextNode(' (too many failures/crashes/timeouts)'));
                else
                    item.appendChild(document.createTextNode(' (' + buildInfo.failureCount + ' failing test' + (buildInfo.failureCount > 1 ? 's' : '') + ')'));
                list.appendChild(item);
            });
        }

        var self = this;
        this._buildbot.getTesters(function(testers) {
            if (self._navigationID !== navigationID) {
                // The user has navigated somewhere else.
                return;
            }
            testers.forEach(function(tester) {
                tester.getMostRecentCompletedBuildNumber(function(buildNumber) {
                    if (self._navigationID !== navigationID)
                        return;
                    if (buildNumber < 0)
                        return;
                    tester.getNumberOfFailingTests(buildNumber, function(failureCount, tooManyFailures) {
                        if (self._navigationID !== navigationID)
                            return;
                        if (failureCount <= 0)
                            return;
                        latestBuildInfos.push({ tester: tester, failureCount: failureCount, tooManyFailures: tooManyFailures });
                        updateList();
                    });
                });
            });
        });
    },

    _domForRegressionRange: function(builder, failingBuildName, passingBuildName, failingTestNames) {
        var result = document.createDocumentFragment();

        var dlItems = [
            [document.createTextNode('Failed'), this._domForBuildName(builder, failingBuildName)],
        ];
        if (passingBuildName)
            dlItems.push([document.createTextNode('Passed'), this._domForBuildName(builder, passingBuildName)]);
        result.appendChild(createDescriptionList(dlItems));

        if (!passingBuildName)
            return result;

        var firstSuspectRevision = this._buildbot.parseBuildName(passingBuildName).revision + 1;
        var lastSuspectRevision = this._buildbot.parseBuildName(failingBuildName).revision;

        if (firstSuspectRevision === lastSuspectRevision)
            return result;

        var suspectsContainer = document.createElement('div');
        result.appendChild(suspectsContainer);

        var link = base.createLinkNode(trac.logURL('trunk', firstSuspectRevision, lastSuspectRevision, true), 'View regression range in Trac');
        result.appendChild(link);

        suspectsContainer.appendChild(document.createTextNode('Searching for suspect revisions\u2026'));

        // FIXME: Maybe some of this code should go in LayoutTestHistoryAnalyzer, or some other class?
        var self = this;
        trac.commitDataForRevisionRange('trunk', firstSuspectRevision, lastSuspectRevision, function(commits) {
            var failingTestNamesWithoutExtensions = failingTestNames.map(removePathExtension);
            var suspectCommits = commits.filter(function(commit) {
                return failingTestNamesWithoutExtensions.some(function(testName) {
                    return commit.message.contains(testName);
                });
            });

            suspectsContainer.removeAllChildren();

            if (!suspectCommits.length)
                return;

            var title = 'Suspect revision' + (suspectCommits.length > 1 ? 's' : '') + ':';
            suspectsContainer.appendChild(document.createTextNode(title));

            var list = document.createElement('ul');
            suspectsContainer.appendChild(list);
            list.className = 'suspect-revisions-list';

            function compareCommits(a, b) {
                return b.revision - a.revision;
            }

            list.appendChildren(sorted(suspectCommits, compareCommits).map(function(commit) {
                var item = document.createElement('li');
                item.appendChild(base.createLinkNode(trac.changesetURL(commit.revision), commit.title));
                return item;
            }));
        });

        return result;
    },

    _domForAuxiliaryUIElements: function() {
        var aside = document.createElement('aside');
        aside.appendChild(document.createTextNode('Something not working? Have an idea to improve this page? '));
        var queryParameters = {
            product: 'WebKit',
            component: 'Tools / Tests',
            version: '528+ (Nightly build)',
            bug_file_loc: location.href,
            cc: 'aroben@apple.com',
            short_desc: 'TestFailures page needs more unicorns!',
        };
        aside.appendChild(base.createLinkNode(addQueryParametersToURL(config.kBugzillaURL + '/enter_bug.cgi', queryParameters), 'File a bug!', '_blank'));
        return aside;
    },

    _domForBuildName: function(builder, buildName) {
        var parsed = this._buildbot.parseBuildName(buildName);

        var sourceLink = base.createLinkNode('http://trac.webkit.org/changeset/' + parsed.revision, 'r' + parsed.revision);
        var buildLink = base.createLinkNode(builder.buildURL(parsed.buildNumber), parsed.buildNumber);
        var resultsLink = base.createLinkNode(builder.resultsPageURL(buildName), 'results.html');

        var result = document.createDocumentFragment();
        result.appendChild(sourceLink);
        result.appendChild(document.createTextNode(' ('));
        result.appendChild(buildLink);
        result.appendChild(document.createTextNode(') ('));
        result.appendChild(resultsLink);
        result.appendChild(document.createTextNode(')'));

        return result;
    },

    _domForFailedTest: function(builder, buildName, testName, testResult) {
        var result = document.createDocumentFragment();
        result.appendChild(document.createTextNode(testName + ': '));
        result.appendChild(this._domForFailureDiagnosis(builder, buildName, testName, testResult));
        return result;
    },

    _domForFailureDiagnosis: function(builder, buildName, testName, testResult) {
        var diagnosticInfo = builder.failureDiagnosisTextAndURL(buildName, testName, testResult);
        if (!diagnosticInfo)
            return document.createTextNode(testResult.failureType);

        var textAndCrashingSymbol = document.createDocumentFragment();
        textAndCrashingSymbol.appendChild(document.createTextNode(diagnosticInfo.text));
        if (testResult.crashingSymbol) {
            var code = document.createElement('code');
            code.appendChild(document.createTextNode(testResult.crashingSymbol));
            textAndCrashingSymbol.appendChild(document.createTextNode(' ('));
            textAndCrashingSymbol.appendChild(code);
            textAndCrashingSymbol.appendChild(document.createTextNode(')'));
        }

        if (!('url' in diagnosticInfo))
            return textAndCrashingSymbol;

        return base.createLinkNode(diagnosticInfo.url, textAndCrashingSymbol);
    },

    _domForNewAndExistingBugs: function(tester, failingTests, bugForm) {
        var result = document.createDocumentFragment();

        var container = document.createElement('p');
        result.appendChild(container);

        container.className = 'existing-and-new-bugs';

        var bugsContainer = document.createElement('div');
        container.appendChild(bugsContainer);

        bugsContainer.appendChild(document.createTextNode('Searching for bugs related to ' + (failingTests.length > 1 ? 'these tests' : 'this test') + '\u2026'));

        bugzilla.quickSearch('ALL ' + failingTests.join('|'), function(bugs) {
            if (!bugs.length) {
                bugsContainer.parentNode.removeChild(bugsContainer);
                return;
            }

            while (bugsContainer.firstChild)
                bugsContainer.removeChild(bugsContainer.firstChild);

            bugsContainer.appendChild(document.createTextNode('Existing bugs related to ' + (failingTests.length > 1 ? 'these tests' : 'this test') + ':'));

            var list = document.createElement('ul');
            bugsContainer.appendChild(list);

            list.className = 'existing-bugs-list';

            function bugToListItem(bug) {
                var link = base.createLinkNode(bug.url, bug.title);
                var item = document.createElement('li');
                item.appendChild(link);

                return item;
            }

            var openBugs = bugs.filter(function(bug) { return bugzilla.isOpenStatus(bug.status) });
            var closedBugs = bugs.filter(function(bug) { return !bugzilla.isOpenStatus(bug.status) });

            list.appendChildren(openBugs.map(bugToListItem));

            if (!closedBugs.length)
                return;

            var item = document.createElement('li');
            list.appendChild(item);

            item.appendChild(document.createTextNode('Closed bugs:'));

            var closedList = document.createElement('ul');
            item.appendChild(closedList);

            closedList.appendChildren(closedBugs.map(bugToListItem));
        });

        var form = bugForm.domElement();
        result.appendChild(form);

        var linkText = 'File bug for ' + (failingTests.length > 1 ? 'these failures' : 'this failure');
        var link = base.createLinkNode('#', linkText);
        link.addEventListener('click', function(event) { form.submit(); event.preventDefault(); });

        container.appendChild(link);
        return result;
    },

    _domForPossiblyFlakyTests: function(builder, possiblyFlakyTestData, allBuilds) {
        var result = document.createDocumentFragment();
        var flakyTests = Object.keys(possiblyFlakyTestData);
        if (!flakyTests.length)
            return result;

        var flakyHeader = document.createElement('h2');
        result.appendChild(flakyHeader);
        flakyHeader.appendChild(document.createTextNode('Possibly Flaky Tests'));

        var flakyList = document.createElement('ol');
        result.appendChild(flakyList);

        flakyList.id = 'possibly-flaky-tests';

        var self = this;
        flakyList.appendChildren(sorted(flakyTests).map(function(testName) {
            var item = document.createElement('li');

            var disclosureTriangle = document.createElement('span');
            item.appendChild(disclosureTriangle);

            disclosureTriangle.className = 'disclosure-triangle';
            const blackRightPointingSmallTriangle = '\u25b8';
            disclosureTriangle.appendChild(document.createTextNode(blackRightPointingSmallTriangle));

            var failures = possiblyFlakyTestData[testName];

            item.appendChild(document.createTextNode(testName + ' (failed ' + failures.length + ' out of ' + allBuilds.length + ' times)'));

            var container = document.createElement('div');
            item.appendChild(container);

            container.className = 'expandable';

            disclosureTriangle.addEventListener('click', function() {
                item.toggleStyleClass('expanded');
                if (!item.hasStyleClass('expanded'))
                    return;

                if (!container.firstChild) {
                    var failureList = document.createElement('ol');
                    container.appendChild(failureList);

                    failureList.className = 'flakiness-examples-list';

                    failureList.appendChildren(failures.map(function(historyItem) {
                        var item = document.createElement('li');
                        item.appendChild(self._domForBuildName(builder, historyItem.build));
                        item.appendChild(document.createTextNode(': '));
                        item.appendChild(self._domForFailureDiagnosis(builder, historyItem.build, testName, historyItem.result));
                        return item;
                    }));

                    var failingBuildNames = failures.map(function(historyItem) { return historyItem.build });
                    var bugForm = new FlakyTestBugForm(builder, failingBuildNames, testName, allBuilds.last(), allBuilds[0], allBuilds.length);
                    container.appendChild(self._domForNewAndExistingBugs(builder, [testName], bugForm));
                }
            });

            return item;
        }));

        return result;
    },

    _setTitle: function(title) {
        document.title = title;
        this._header.textContent = title;
    },
};
