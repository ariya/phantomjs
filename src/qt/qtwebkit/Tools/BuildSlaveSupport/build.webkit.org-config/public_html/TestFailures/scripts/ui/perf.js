/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
ui.perf = ui.perf || {};

(function(){

ui.perf.Picker = base.extends('select', {
    init: function(items, onChange, opt_values)
    {
        this.onchange = onChange;
        this._values = opt_values;
        items.forEach(this._appendItem.bind(this));
    },
    _appendItem: function(item, index)
    {
        var option = document.createElement('option');
        option.innerHTML = item;
        if (this._values)
            option.value = this._values[index];
        this.appendChild(option);
    }
});

ui.perf.View = base.extends('div', {
    init: function()
    {
        this.id = 'perf-view';
        builders.perfBuilders(this.loadGraphs.bind(this));

        var stream = new ui.notifications.Stream();
        var notifications = document.createElement()
        this._notification = ui.notifications.Info("Loading list of perf dashboards...");
        stream.appendChild(this._notification);
        this.appendChild(stream);
    },
    loadGraphs: function(graphData)
    {
        this._notification.dismiss();

        // FIXME: Add next/previous buttons for easy navigation through all the graphs.
        // FIXME: Also, show the list of failing perf builders along with which steps are failing.
        this._data = graphData;

        this._titleBar = document.createElement('div');
        this._titleBar.className = 'title-bar';
        this.appendChild(this._titleBar);

        var testSuites = Object.keys(graphData);
        var suitePicker = new ui.perf.Picker(testSuites, this._onChangeSuite.bind(this));
        this._titleBar.appendChild(suitePicker);

        this._titleBar.appendChild(new ui.actions.List([new ui.actions.Previous(), new ui.actions.Next()]));

        $(this).bind('next', this._nextGraph.bind(this));
        $(this).bind('previous', this._previousGraph.bind(this));

        this._suitePicker = suitePicker;
        this._onChangeSuite();
    },
    _onChangeSuite: function()
    {
        this._updateBuilderPicker();
        this._displayGraph();
    },
    _updateBuilderPicker: function()
    {
        if (this._builderPicker)
            this._titleBar.removeChild(this._builderPicker);

        if (!this._suitePicker.length)
            return;
        var selectedSuite = this._suitePicker[this._suitePicker.selectedIndex].text;
        var builders = [];
        var urls = [];
        this._data[selectedSuite].forEach(function(config) {
            builders.push(config.builder);
            urls.push(config.url);
        });
        this._builderPicker = new ui.perf.Picker(builders, this._displayGraph.bind(this), urls);
        this._titleBar.appendChild(this._builderPicker);
    },
    _displayGraph: function()
    {
        var popOutLink = this.querySelector('.pop-out');
        if (!popOutLink) {
            popOutLink = document.createElement('a');
            popOutLink.className = 'pop-out';
            popOutLink.textContent = 'Pop out';
            popOutLink.target = '_blank';
            this._titleBar.appendChild(popOutLink);
        }

        var graph = this.querySelector('iframe');
        if (!graph) {
            graph = document.createElement('iframe');
            this.appendChild(graph);
        }

        if (!this._builderPicker || !this._builderPicker.length)
            return;

        var url = this._builderPicker[this._builderPicker.selectedIndex].value;
        popOutLink.href = url;
        graph.src = url;
    },
    _nextGraph: function()
    {
        if (!this._builderPicker)
            return;

        if (this._builderPicker.selectedIndex < this._builderPicker.length - 1) {
            this._builderPicker.selectedIndex++;
            this._displayGraph();
            return;
        }

        if (this._suitePicker.selectedIndex < this._suitePicker.length - 1) {
            this._suitePicker.selectedIndex++;
            this._onChangeSuite();
        }
    },
    _previousGraph: function()
    {
        if (!this._builderPicker)
            return;

        if (this._builderPicker.selectedIndex > 0) {
            this._builderPicker.selectedIndex--;
            this._displayGraph();
            return;
        }

        if (this._suitePicker.selectedIndex > 0) {
            this._suitePicker.selectedIndex--;
            this._updateBuilderPicker();
            this._builderPicker.selectedIndex = this._builderPicker.length - 1;
            this._displayGraph();
        }
    }
});

})();
