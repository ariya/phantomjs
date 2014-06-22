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

var ui = ui || {};
ui.results = ui.results || {};

(function(){

var kResultsPrefetchDelayMS = 500;

// FIXME: Rather than using table, should we be using something fancier?
ui.results.Comparison = base.extends('table', {
    init: function()
    {
        this.className = 'comparison';
        this.innerHTML = '<thead><tr><th>Expected</th><th>Actual</th><th>Diff</th></tr></thead>' +
                         '<tbody><tr><td class="expected result-container"></td><td class="actual result-container"></td><td class="diff result-container"></td></tr></tbody>';
    },
    _selectorForKind: function(kind)
    {
        switch (kind) {
        case results.kExpectedKind:
            return '.expected';
        case results.kActualKind:
            return '.actual';
        case results.kDiffKind:
            return '.diff';
        }
        return '.unknown';
    },
    update: function(kind, result)
    {
        var selector = this._selectorForKind(kind);
        $(selector, this).empty().append(result);
        return result;
    },
});

// We'd really like TextResult and ImageResult to extend a common Result base
// class, but we can't seem to do that because they inherit from different
// HTMLElements. We could have them inherit from <div>, but that seems lame.

ui.results.TextResult = base.extends('iframe', {
    init: function(url)
    {
        this.className = 'text-result';
        this.src = url;
    }
});

ui.results.ImageResult = base.extends('img', {
    init: function(url)
    {
        this.className = 'image-result';
        this.src = url;
    }
});

ui.results.AudioResult = base.extends('audio', {
    init: function(url)
    {
        this.className = 'audio-result';
        this.src = url;
        this.controls = 'controls';
    }
});

function constructorForResultType(type)
{
    if (type == results.kImageType)
        return ui.results.ImageResult;
    if (type == results.kAudioType)
        return ui.results.AudioResult;
    return ui.results.TextResult;
}

ui.results.ResultsGrid = base.extends('div', {
    init: function()
    {
        this.className = 'results-grid';
    },
    _addResult: function(comparison, constructor, resultsURLsByKind, kind)
    {
        var url = resultsURLsByKind[kind];
        if (!url)
            return;
        comparison.update(kind, new constructor(url));
    },
    addComparison: function(resultType, resultsURLsByKind)
    {
        var comparison = new ui.results.Comparison();
        var constructor = constructorForResultType(resultType);

        this._addResult(comparison, constructor, resultsURLsByKind, results.kExpectedKind);
        this._addResult(comparison, constructor, resultsURLsByKind, results.kActualKind);
        this._addResult(comparison, constructor, resultsURLsByKind, results.kDiffKind);

        this.appendChild(comparison);
        return comparison;
    },
    addRow: function(resultType, url)
    {
        var constructor = constructorForResultType(resultType);
        var view = new constructor(url);
        this.appendChild(view);
        return view;
    },
    addResults: function(resultsURLs)
    {
        var resultsURLsByTypeAndKind = {};

        resultsURLsByTypeAndKind[results.kImageType] = {};
        resultsURLsByTypeAndKind[results.kAudioType] = {};
        resultsURLsByTypeAndKind[results.kTextType] = {};

        resultsURLs.forEach(function(url) {
            resultsURLsByTypeAndKind[results.resultType(url)][results.resultKind(url)] = url;
        });

        $.each(resultsURLsByTypeAndKind, function(resultType, resultsURLsByKind) {
            if ($.isEmptyObject(resultsURLsByKind))
                return;
            if (results.kUnknownKind in resultsURLsByKind) {
                // This is something like "crash" that isn't a comparison.
                this.addRow(resultType, resultsURLsByKind[results.kUnknownKind]);
                return;
            }
            this.addComparison(resultType, resultsURLsByKind);
        }.bind(this));

        if (!this.children.length)
            this.textContent = 'No results to display.'
    }
});

ui.results.ResultsDetails = base.extends('div', {
    init: function(delegate, failureInfo)
    {
        this.className = 'results-detail';
        this._delegate = delegate;
        this._failureInfo = failureInfo;
        this._haveShownOnce = false;
    },
    show: function() {
        if (this._haveShownOnce)
            return;
        this._haveShownOnce = true;
        this._delegate.fetchResultsURLs(this._failureInfo, function(resultsURLs) {
            var resultsGrid = new ui.results.ResultsGrid();
            resultsGrid.addResults(resultsURLs);

            $(this).empty().append(
                new ui.actions.List([
                    new ui.actions.Previous(),
                    new ui.actions.Next()
                ])).append(resultsGrid);


        }.bind(this));
    },
});

function isAnyReftest(testName, resultsByTest)
{
    return Object.keys(resultsByTest[testName]).map(function(builder) {
        return resultsByTest[testName][builder];
    }).some(function(resultNode) {
        return resultNode.reftest_type && resultNode.reftest_type.length;
    });
}

ui.results.FlakinessData = base.extends('iframe', {
    init: function()
    {
        this.className = 'flakiness-iframe';
        this.src = ui.urlForEmbeddedFlakinessDashboard();
        this.addEventListener('load', function() {
            window.addEventListener('message', this._handleMessage.bind(this));
        });
    },
    _handleMessage: function(event) {
        if (!this.contentWindow)
            return;

        // Check for null event.origin so that the unittests can get past this point.
        // FIXME: Is this safe? In practice, there's no meaningful harm that can come from
        // a malicious page sending us heightChanged commands, so it doesn't really matter.
        if (event.origin !== 'null' && event.origin != 'http://webkit-test-results.appspot.com') {
            console.log('Invalid origin: ' + event.origin);
            return;
        }

        if (event.data.command != 'heightChanged') {
            console.log('Unknown postMessage command: ' + event.data);
            return;
        }

        this.style.height = event.data.height + 'px';
    }
});

ui.results.TestSelector = base.extends('div', {
    init: function(delegate, resultsByTest)
    {
        this.className = 'test-selector';
        this._delegate = delegate;

        var topPanel = document.createElement('div');
        topPanel.className = 'top-panel';
        this.appendChild(topPanel);

        this._appendResizeHandle();

        var bottomPanel = document.createElement('div');
        bottomPanel.className = 'bottom-panel';
        this.appendChild(bottomPanel);

        this._flakinessData = new ui.results.FlakinessData();
        this.appendChild(this._flakinessData);

        var testNames = Object.keys(resultsByTest);
        testNames.sort().forEach(function(testName) {
            var nonLinkTitle = document.createElement('a');
            nonLinkTitle.classList.add('non-link-title');
            nonLinkTitle.textContent = testName;

            var linkTitle = document.createElement('a');
            linkTitle.classList.add('link-title');
            linkTitle.setAttribute('href', ui.urlForFlakinessDashboard([testName]))
            linkTitle.textContent = testName;

            var header = document.createElement('h3');
            header.appendChild(nonLinkTitle);
            header.appendChild(linkTitle);
            header.addEventListener('click', this._showResults.bind(this, header, false));
            topPanel.appendChild(header);
        }, this);

        // If we have a small amount of content, don't show the resize handler.
        // Otherwise, set the minHeight so that the percentage height of the 
        // topPanel is not too small.
        if (testNames.length <= 4)
            this.removeChild(this.querySelector('.resize-handle'));
        else
            topPanel.style.minHeight = '100px';
    },
    _appendResizeHandle: function()
    {
        var resizeHandle = document.createElement('div');
        resizeHandle.className = 'resize-handle';
        this.appendChild(resizeHandle);

        resizeHandle.addEventListener('mousedown', function(event) {
            this._is_resizing = true;
            event.preventDefault();
        }.bind(this));

        var cancelResize = function(event) { this._is_resizing = false; }.bind(this);
        this.addEventListener('mouseup', cancelResize);
        // FIXME: Use addEventListener once WebKit adds support for mouseleave/mouseenter.
        $(window).bind('mouseleave', cancelResize);

        this.addEventListener('mousemove', function(event) {
            if (!this._is_resizing)
                return;
            var mouseY = event.clientY + document.body.scrollTop - this.offsetTop;
            var percentage = 100 * mouseY / this.offsetHeight;
            document.querySelector('.top-panel').style.maxHeight = percentage + '%';
        }.bind(this))
    },
    _showResults: function(header, scrollInfoView)
    {
        if (!header)
            return false;

        var activeHeader = this.querySelector('.active')
        if (activeHeader)
            activeHeader.classList.remove('active');
        header.classList.add('active');

        var testName = this.currentTestName();
        this._flakinessData.src = ui.urlForEmbeddedFlakinessDashboard([testName]);

        var bottomPanel = this.querySelector('.bottom-panel')
        bottomPanel.innerHTML = '';
        bottomPanel.appendChild(this._delegate.contentForTest(testName));

        var topPanel = this.querySelector('.top-panel');
        if (scrollInfoView) {
            topPanel.scrollTop = header.offsetTop;
            if (header.offsetTop - topPanel.scrollTop < header.offsetHeight)
                topPanel.scrollTop = topPanel.scrollTop - header.offsetHeight;
        }

        var resultsDetails = this.querySelectorAll('.results-detail');
        if (resultsDetails.length)
            resultsDetails[0].show();
        setTimeout(function() {
            Array.prototype.forEach.call(resultsDetails, function(resultsDetail) {
                resultsDetail.show();
            });
        }, kResultsPrefetchDelayMS);

        return true;
    },
    nextResult: function()
    {
        if (this.querySelector('.builder-selector').nextResult())
            return true;
        return this.nextTest();
    },
    previousResult: function()
    {
        if (this.querySelector('.builder-selector').previousResult())
            return true;
        return this.previousTest();
    },
    nextTest: function()
    {
        return this._showResults(this.querySelector('.active').nextSibling, true);
    },
    previousTest: function()
    {
        var succeeded = this._showResults(this.querySelector('.active').previousSibling, true);
        if (succeeded)
            this.querySelector('.builder-selector').lastResult();
        return succeeded;
    },
    firstResult: function()
    {
        this._showResults(this.querySelector('h3'), true);
    },
    currentTestName: function()
    {
        return this.querySelector('.active .non-link-title').textContent;
    }
});

ui.results.BuilderSelector = base.extends('div', {
    init: function(delegate, testName, resultsByBuilder)
    {
        this.className = 'builder-selector';
        this._delegate = delegate;

        var tabStrip = this.appendChild(document.createElement('ul'));

        Object.keys(resultsByBuilder).sort().forEach(function(builderName) {
            var builderHash = base.underscoredBuilderName(builderName);

            var link = document.createElement('a');
            $(link).attr('href', "#" + builderHash).text(ui.displayNameForBuilder(builderName));
            tabStrip.appendChild(document.createElement('li')).appendChild(link);

            var content = this._delegate.contentForTestAndBuilder(testName, builderName);
            content.id = builderHash;
            this.appendChild(content);
        }, this);

        $(this).tabs();
    },
    nextResult: function()
    {
        var nextIndex = $(this).tabs('option', 'selected') + 1;
        if (nextIndex >= $(this).tabs('length'))
            return false
        $(this).tabs('option', 'selected', nextIndex);
        return true;
    },
    previousResult: function()
    {
        var previousIndex = $(this).tabs('option', 'selected') - 1;
        if (previousIndex < 0)
            return false;
        $(this).tabs('option', 'selected', previousIndex);
        return true;
    },
    firstResult: function()
    {
        $(this).tabs('option', 'selected', 0);
    },
    lastResult: function()
    {
        $(this).tabs('option', 'selected', $(this).tabs('length') - 1);
    }
});

ui.results.View = base.extends('div', {
    init: function(delegate)
    {
        this.className = 'results-view';
        this._delegate = delegate;
    },
    contentForTest: function(testName)
    {
        var rebaselineAction;
        if (isAnyReftest(testName, this._resultsByTest))
            rebaselineAction = $('<div class="non-action-button">Reftests cannot be rebaselined. Email webkit-gardening@chromium.org if unsure how to fix this.</div>');
        else
            rebaselineAction = new ui.actions.List([new ui.actions.Rebaseline().makeDefault()]);
        $(rebaselineAction).addClass('rebaseline-action');

        var builderSelector = new ui.results.BuilderSelector(this, testName, this._resultsByTest[testName]);
        $(builderSelector).append(rebaselineAction).append($('<br style="clear:both">'));
        $(builderSelector).bind('tabsselect', function(event, ui) {
            // We will probably have pre-fetched the tab already, but we need to make sure.
            ui.panel.show();
        });
        return builderSelector;
    },
    contentForTestAndBuilder: function(testName, builderName)
    {
        var failureInfo = results.failureInfoForTestAndBuilder(this._resultsByTest, testName, builderName);
        return new ui.results.ResultsDetails(this, failureInfo);
    },
    setResultsByTest: function(resultsByTest)
    {
        $(this).empty();
        this._resultsByTest = resultsByTest;
        this._testSelector = new ui.results.TestSelector(this, resultsByTest);
        this.appendChild(this._testSelector);
    },
    fetchResultsURLs: function(failureInfo, callback)
    {
        this._delegate.fetchResultsURLs(failureInfo, callback)
    },
    nextResult: function()
    {
        return this._testSelector.nextResult();
    },
    previousResult: function()
    {
        return this._testSelector.previousResult();
    },
    nextTest: function()
    {
        return this._testSelector.nextTest();
    },
    previousTest: function()
    {
        return this._testSelector.previousTest();
    },
    firstResult: function()
    {
        this._testSelector.firstResult()
    },
    currentTestName: function()
    {
        return this._testSelector.currentTestName()
    }
});

})();
