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

(function() {

var g_info = null;
var g_revisionHint = null;

var g_updateTimerId = 0;
var g_buildersFailing = null;

var g_unexpectedFailuresController = null;
var g_failuresController = null;

var g_nonLayoutTestFailureBuilders = null;

function updatePartyTime()
{
    if (!g_unexpectedFailuresController.length() && !g_nonLayoutTestFailureBuilders.hasFailures())
        $('#onebar').addClass('partytime');
    else
        $('#onebar').removeClass('partytime');
}

function update()
{
    if (g_revisionHint)
        g_revisionHint.dismiss();

    var gtestIframe = document.querySelector('#chromium-gtests iframe');
    if (gtestIframe)
        gtestIframe.src = gtestIframe.src;

    // FIXME: This should be a button with a progress element.
    var numberOfTestsAnalyzed = 0;
    var updating = new ui.notifications.Info('Loading commit data ...');

    g_info.add(updating);

    builders.buildersFailingNonLayoutTests(function(failuresList) {
        g_nonLayoutTestFailureBuilders.update(failuresList);
        updatePartyTime();
    });

    base.callInParallel([model.updateRecentCommits, model.updateResultsByBuilder], function() {
        if (g_failuresController)
            g_failuresController.update();

        updating.update('Analyzing test failures ...');

        model.analyzeUnexpectedFailures(function(failureAnalysis) {
            updating.update('Analyzing test failures ... ' + ++numberOfTestsAnalyzed + ' tests analyzed.');
            g_unexpectedFailuresController.update(failureAnalysis);
        }, function() {
            updatePartyTime();
            g_unexpectedFailuresController.purge();

            Object.keys(config.currentBuilders()).forEach(function(builderName) {
                if (!model.state.resultsByBuilder[builderName])
                    g_info.add(new ui.notifications.Info('Could not find test results for ' + builderName + ' in the last ' + config.kBuildNumberLimit + ' runs.'));            
            });

            updating.dismiss();

            g_revisionHint = new ui.notifications.Info('');
            
            var latestRevisionSpan = document.createElement('span');
            latestRevisionSpan.appendChild(document.createTextNode('Latest revision processed by every bot: '));
            
            var latestRevision = model.latestRevisionWithNoBuildersInFlight();
            latestRevisionSpan.appendChild(base.createLinkNode(trac.changesetURL(latestRevision), latestRevision));
            
            var totRevision = model.latestRevision();
            latestRevisionSpan.appendChild(document.createTextNode(' (trunk is at '));
            latestRevisionSpan.appendChild(base.createLinkNode(trac.changesetURL(totRevision), totRevision));
            latestRevisionSpan.appendChild(document.createTextNode(')'));
            
            g_revisionHint.updateWithNode(latestRevisionSpan);
            
            g_info.add(g_revisionHint);
        });
    });
}

$(document).ready(function() {
    g_updateTimerId = window.setInterval(update, config.kUpdateFrequency);

    pixelzoomer.installEventListeners();

    onebar = new ui.onebar();
    onebar.attach();

    // FIXME: This doesn't belong here.
    var onebarController = {
        showResults: function(resultsView)
        {
            var resultsContainer = onebar.results();
            $(resultsContainer).empty().append(resultsView);
            onebar.select('results');
        }
    };

    var unexpectedFailuresView = new ui.notifications.Stream();
    g_unexpectedFailuresController = new controllers.UnexpectedFailures(model.state, unexpectedFailuresView, onebarController);

    g_info = new ui.notifications.Stream();
    g_nonLayoutTestFailureBuilders = new controllers.FailingBuilders(g_info, 'Non-layout test failures');

    // FIXME: This should be an Action object.
    var updateButton = document.body.insertBefore(document.createElement('button'), document.body.firstChild);
    updateButton.addEventListener("click", update);
    updateButton.textContent = 'update';

    var unexpected = onebar.unexpected();
    unexpected.appendChild(updateButton);
    unexpected.appendChild(g_info);
    unexpected.appendChild(unexpectedFailuresView);

    var expected = onebar.expected();
    if (expected) {
        var failuresView = new ui.failures.List();
        g_failuresController = new controllers.ExpectedFailures(model.state, failuresView, onebarController);
        expected.appendChild(failuresView);
    }

    onebar.perf().appendChild(new ui.perf.View());

    update();
});

})();
