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

var controllers = controllers || {};

(function(){

var kCheckoutUnavailableMessage = 'Failed! Garden-o-matic needs a local server to modify your working copy. Please run "webkit-patch garden-o-matic" start the local server.';

// FIXME: Where should this function go?
function rebaselineWithStatusUpdates(failureInfoList, resultsByTest)
{
    var statusView = new ui.StatusArea('Rebaseline');
    var id = statusView.newId();

    var failuresToRebaseline = [];
    var testNamesLogged = [];
    failureInfoList.forEach(function(failureInfo) {
        if (isAnyReftest(failureInfo.testName, resultsByTest)) {
            if (testNamesLogged.indexOf(failureInfo.testName) == -1) {
                statusView.addMessage(id, failureInfo.testName + ' is a ref test, skipping');
                testNamesLogged.push(failureInfo.testName);
            }
        } else {
            failuresToRebaseline.push(failureInfo);
            if (testNamesLogged.indexOf(failureInfo.testName) == -1) {
                statusView.addMessage(id, 'Rebaselining ' + failureInfo.testName + '...');
                testNamesLogged.push(failureInfo.testName);
            }
        }
    });
    
    if (failuresToRebaseline.length) {
        checkout.rebaseline(failuresToRebaseline, function() {
            statusView.addFinalMessage(id, 'Rebaseline done! Please land with "webkit-patch land-cowhand".');
        }, function(failureInfo) {
            statusView.addMessage(id, failureInfo.testName + ' on ' + ui.displayNameForBuilder(failureInfo.builderName));
        }, function() {
            statusView.addFinalMessage(id, kCheckoutUnavailableMessage);
        });
    } else {
        statusView.addFinalMessage(id, 'No non-reftests left to rebaseline!')
    }
}

// FIXME: This is duplicated from ui/results.js :(.
function isAnyReftest(testName, resultsByTest)
{
    return Object.keys(resultsByTest[testName]).map(function(builder) {
        return resultsByTest[testName][builder];
    }).some(function(resultNode) {
        return resultNode.reftest_type && resultNode.reftest_type.length;
    });
}

// FIXME: Where should this function go?
function updateExpectationsWithStatusUpdates(failureInfoList)
{
    var statusView = new ui.StatusArea('Expectations Update');
    var id = statusView.newId();

    var testNames = base.uniquifyArray(failureInfoList.map(function(failureInfo) { return failureInfo.testName; }));
    var testName = testNames.length == 1 ? testNames[0] : testNames.length + ' tests';
    statusView.addMessage(id, 'Updating expectations of ' + testName + '...');

    checkout.updateExpectations(failureInfoList, function() {
        statusView.addFinalMessage(id, 'Expectations update done! Please land with "webkit-patch land-cowhand".');
    }, function() {
        statusView.addFinalMessage(id, kCheckoutUnavailableMessage);
    });
}

controllers.ResultsDetails = base.extends(Object, {
    init: function(view, resultsByTest)
    {
        this._view = view;
        this._resultsByTest = resultsByTest;
        this._view.setResultsByTest(resultsByTest);

        this._view.firstResult();

        $(this._view).bind('next', this.onNext.bind(this));
        $(this._view).bind('previous', this.onPrevious.bind(this));
        $(this._view).bind('rebaseline', this.onRebaseline.bind(this));
        $(this._view).bind('expectfailure', this.onUpdateExpectations.bind(this));
    },
    onNext: function()
    {
        this._view.nextResult();
    },
    onPrevious: function()
    {
        this._view.previousResult();
    },
    _failureInfoList: function()
    {
        var testName = this._view.currentTestName();
        return Object.keys(this._resultsByTest[testName]).map(function(builderName) {
            return results.failureInfoForTestAndBuilder(this._resultsByTest, testName, builderName);
        }.bind(this));
    },
    onRebaseline: function()
    {
        rebaselineWithStatusUpdates(this._failureInfoList(), this._resultsByTest);
        this._view.nextTest();
    },
    onUpdateExpectations: function()
    {
        updateExpectationsWithStatusUpdates(this._failureInfoList());
    }
});

controllers.ExpectedFailures = base.extends(Object, {
    init: function(model, view, delegate)
    {
        this._model = model;
        this._view = view;
        this._delegate = delegate;
    },
    update: function()
    {
        var expectedFailures = results.expectedFailuresByTest(this._model.resultsByBuilder);
        var failingTestsList = Object.keys(expectedFailures);

        $(this._view).empty();
        base.forEachDirectory(failingTestsList, function(label, testsFailingInDirectory) {
            var listItem = new ui.failures.ListItem(label, testsFailingInDirectory);
            this._view.appendChild(listItem);
            $(listItem).bind('examine', function() {
                this.onExamine(testsFailingInDirectory);
            }.bind(this));
        }.bind(this));
    },
    onExamine: function(failingTestsList)
    {
        var resultsView = new ui.results.View({
            fetchResultsURLs: results.fetchResultsURLs
        });
        var failuresByTest = base.filterDictionary(
            results.expectedFailuresByTest(this._model.resultsByBuilder),
            function(key) {
                return failingTestsList.indexOf(key) != -1;
            });
        var controller = new controllers.ResultsDetails(resultsView, failuresByTest);
        this._delegate.showResults(resultsView);
    }
});

var FailureStreamController = base.extends(Object, {
    _resultsFilter: null,
    _keyFor: function(failureAnalysis) { throw "Not implemented!"; },
    _createFailureView: function(failureAnalysis) { throw "Not implemented!"; },

    init: function(model, view, delegate)
    {
        this._model = model;
        this._view = view;
        this._delegate = delegate;
        this._testFailures = new base.UpdateTracker();
    },
    update: function(failureAnalysis)
    {
        var key = this._keyFor(failureAnalysis);
        var failure = this._testFailures.get(key);
        if (!failure) {
            failure = this._createFailureView(failureAnalysis);
            this._view.add(failure);
            $(failure).bind('examine', function() {
                this.onExamine(failure);
            }.bind(this));
            $(failure).bind('rebaseline', function() {
                this.onRebaseline(failure);
            }.bind(this));
            $(failure).bind('expectfailure', function() {
                this.onUpdateExpectations(failure);
            }.bind(this));
        }
        failure.addFailureAnalysis(failureAnalysis);
        this._testFailures.update(key, failure);
        return failure;
    },
    purge: function() {
        this._testFailures.purge(function(failure) {
            failure.dismiss();
        });
        this._testFailures.forEach(function(failure) {
            failure.purge();
        });
    },
    onExamine: function(failures)
    {
        var resultsView = new ui.results.View({
            fetchResultsURLs: results.fetchResultsURLs
        });

        var testNameList = failures.testNameList();
        var failuresByTest = base.filterDictionary(
            this._resultsFilter(this._model.resultsByBuilder),
            function(key) {
                return testNameList.indexOf(key) != -1;
            });

        var controller = new controllers.ResultsDetails(resultsView, failuresByTest);
        this._delegate.showResults(resultsView);
    },
    _toFailureInfoList: function(failures)
    {
        return base.flattenArray(failures.testNameList().map(model.unexpectedFailureInfoForTestName));
    },
    onRebaseline: function(failures)
    {
        var testNameList = failures.testNameList();
        var failuresByTest = base.filterDictionary(
            this._resultsFilter(this._model.resultsByBuilder),
            function(key) {
                return testNameList.indexOf(key) != -1;
            });

        rebaselineWithStatusUpdates(this._toFailureInfoList(failures), failuresByTest);
    },
    onUpdateExpectations: function(failures)
    {
        updateExpectationsWithStatusUpdates(this._toFailureInfoList(failures));
    }
});

controllers.UnexpectedFailures = base.extends(FailureStreamController, {
    _resultsFilter: results.unexpectedFailuresByTest,

    _impliedFirstFailingRevision: function(failureAnalysis)
    {
        return failureAnalysis.newestPassingRevision + 1;
    },
    _keyFor: function(failureAnalysis)
    {
        return failureAnalysis.newestPassingRevision + "+" + failureAnalysis.oldestFailingRevision;
    },
    _createFailureView: function(failureAnalysis)
    {
        var failure = new ui.notifications.FailingTestsSummary();
        model.commitDataListForRevisionRange(this._impliedFirstFailingRevision(failureAnalysis), failureAnalysis.oldestFailingRevision).forEach(function(commitData) {
            var suspiciousCommit = failure.addCommitData(commitData);
            $(suspiciousCommit).bind('rollout', function() {
                this.onRollout(commitData.revision, failure.testNameList());
            }.bind(this));
            $(failure).bind('blame', function() {
                this.onBlame(failure, commitData);
            }.bind(this));
        }, this);

        return failure;
    },
    update: function(failureAnalysis)
    {
        var failure = FailureStreamController.prototype.update.call(this, failureAnalysis);
        failure.updateBuilderResults(model.buildersInFlightForRevision(this._impliedFirstFailingRevision(failureAnalysis)));
    },
    length: function()
    {
        return this._testFailures.length();
    },
    onBlame: function(failure, commitData)
    {
        failure.pinToCommitData(commitData);
        $('.action', failure).each(function() {
            // FIXME: This isn't the right way of finding and disabling this action.
            if (this.textContent == 'Blame')
                this.disabled = true;
        });
    },
    onRollout: function(revision, testNameList)
    {
        checkout.rollout(revision, ui.rolloutReasonForTestNameList(testNameList), $.noop, function() {
            // FIXME: We should have a better error UI.
            alert(kCheckoutUnavailableMessage);
        });
    }
});

controllers.Failures = base.extends(FailureStreamController, {
    _resultsFilter: results.expectedFailuresByTest,

    _keyFor: function(failureAnalysis)
    {
        return base.dirName(failureAnalysis.testName);
    },
    _createFailureView: function(failureAnalysis)
    {
        return new ui.notifications.FailingTests();
    },
});

controllers.FailingBuilders = base.extends(Object, {
    init: function(view, message)
    {
        this._view = view;
        this._message = message;
        this._notification = null;
    },
    hasFailures: function()
    {
        return !!this._notification;
    },
    update: function(failuresList)
    {
        if (Object.keys(failuresList).length == 0) {
            if (this._notification) {
                this._notification.dismiss();
                this._notification = null;
            }
            return;
        }
        if (!this._notification) {
            this._notification = new ui.notifications.BuildersFailing(this._message);
            this._view.add(this._notification);
        }
        // FIXME: We should provide regression ranges for the failing builders.
        // This doesn't seem to happen often enough to worry too much about that, however.
        this._notification.setFailingBuilders(failuresList);
    }
});

})();
