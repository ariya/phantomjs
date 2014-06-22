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

function minutesAgo(minutes)
{
    var time = new Date();
    time.setMinutes(time.getMinutes() - minutes);
    return time;
}

function Cycler(items, repeat)
{
    this._index = 0;
    this._repeat = repeat || 1;
    this._repeated = 0;
    this._items = items;
}

Cycler.prototype = {
    _updateRepeat: function()
    {
        if (++this._repeated >= this._repeat) {
            this._repeated = this._repeat;
            return false;
        }
        return true;
    },
    _updateIndex: function()
    {
        if (this._updateRepeat())
            return;
        if (++this._index >= this._items.length)
            this._index = 0;
    },
    cycle: function()
    {
        var result = this._items[this._index];
        this._updateIndex();
        return result;
    }
}

var people = new Cycler([
    'Eustace Bagge',
    'Dick Dastardly',
    'Major Glory',
    'Barney Rubble',
    'Bunny Bravo',
    'Race Bannon',
]);

var bugTitles = new Cycler([
    'Unreviewed. Chromium rebaselines for r93794. * ...',
    'Fix build when GCC 4.2 is not installed. * ... ',
    '[Qt] Unreviewed gardening. * platform/qt/Skipped: Skip new tests until ...',
    'garden-o-matic needs a way to report where and how tests are failing in ... ',
    'REGRESSION(r90971): Fix an assertion failure with textarea placeholder. ...',
    'Incorrect layout of :before and :after content, with display table, ...',
    ' JSHTMLImageElement (and associated Node) is abandoned when image load is ... '
]);

var testNames = new Cycler([
    'fast/ruby/text-emphasis.html',
    'plugins/destroy-during-npp-new.html',
    'tables/mozilla/bugs/bug60749.html',
    'tables/mozilla/bugs/bug51727.html',
    'tables/mozilla/bugs/bug33855.html',
    'tables/mozilla/bugs/bug52506.htm',
    'tables/mozilla/bugs/bug18359.html',
    'tables/mozilla/bugs/bug46368-1.html',
    'tables/mozilla/bugs/bug46368-2.html',
    'tables/mozilla/bugs/bug52505.html'
]);

var builders = new Cycler(Object.keys(config.currentBuilders()), 3);

var expectations = new Cycler([
    'TEXT',
    'IMAGE+TEXT',
    'TIMEOUT',
    'CRASH'
], 4);

function createResultNodesByBuilder(builderFailureCount)
{
    var result = {};
    for(var i = 0; i < builderFailureCount; ++i)
        result[builders.cycle()] = { actual: expectations.cycle() };
    return result;
}

var currentRevision = 66666;
var currentMinutesAgo = 0;

function createFailingTestsSummary(commitDataCount, failureAnalysisCount, builderFailureCount)
{
    var failingTestsSummary = new ui.notifications.FailingTestsSummary();
    for (var i = 0; i < commitDataCount; ++i)
        failingTestsSummary.addCommitData({
            time: minutesAgo(currentMinutesAgo++),
            revision: currentRevision++,
            summary: bugTitles.cycle(),
            author: people.cycle(),
            reviewer: people.cycle()
        });
    for (var i = 0; i < failureAnalysisCount; ++i)
        failingTestsSummary.addFailureAnalysis({
            testName: testNames.cycle(),
            resultNodesByBuilder: createResultNodesByBuilder(builderFailureCount)
        });
    return failingTestsSummary;
}

function createBuildersFailing(failingBuilderCount)
{
    var buildersFailing = new ui.notifications.BuildersFailing();
    builderNameList = [];
    for (var i = 0; i < failingBuilderCount; ++i)
        builderNameList.push(builders.cycle());
    buildersFailing.setFailingBuilders(builderNameList);
    return buildersFailing
}

$(document).ready(function() {

    var actions = new ui.notifications.Stream();
    document.body.insertBefore(actions, document.body.firstChild);

    // FIXME: This should be an Action object.
    var button = document.body.insertBefore(document.createElement('button'), document.body.firstChild);
    button.textContent = 'update';

    actions.add(createFailingTestsSummary(3, 4, 1));
    actions.add(createFailingTestsSummary(3, 1, 3));
    actions.add(createFailingTestsSummary(1, 20, 1));
    actions.add(createBuildersFailing(1));
    actions.add(createBuildersFailing(8));
});

})();
