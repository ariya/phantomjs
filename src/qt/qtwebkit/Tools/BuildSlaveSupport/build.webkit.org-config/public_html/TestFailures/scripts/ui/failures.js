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
ui.failures = ui.failures || {};

(function(){

var kBuildingResult = 'BUILDING';

ui.failures.Builder = base.extends('a', {
    init: function(builderName, failures)
    {
        var platformBuilders = config.currentBuilders();
        var configuration = platformBuilders[builderName];
        if (configuration) {
            if (configuration.version)
                this._addSpan('version', configuration.version);
            if (configuration.is64bit)
                this._addSpan('architecture', '64-bit');
            this._configuration = configuration;
        } else
            this._addSpan('version', builderName);

        this.className = 'failing-builder';
        this.target = '_blank';
        this.href = ui.displayURLForBuilder(builderName);
        if (failures)
            this._addSpan('failures', ' ' + failures.join(', '));
    },
    _addSpan: function(className, text)
    {
        var span = this.appendChild(document.createElement('span'));
        span.className = className;
        span.textContent = text;
    },
    equals: function(configuration)
    {
        return this._configuration && this._configuration.is64bit == configuration.is64bit && this._configuration.version == configuration.version; 
    }
});

function cellContainsConfiguration(cell, configuration)
{
    return Array.prototype.some.call(cell.children, function(configurationElement) {
        return configurationElement.equals && configurationElement.equals(configuration);
    });
}

function cellByBuildType(row, configuration)
{
    return row.cells[configuration.debug ? 2 : 1];
}

ui.failures.FailureGrid = base.extends('table', {
    init: function()
    {
        this.className = 'failures';
        var titles = this.createTHead().insertRow();
        titles.insertCell().textContent = 'debug';
        titles.insertCell().textContent = 'release';
        titles.insertCell().textContent = 'type';
        this._body = this.appendChild(document.createElement('tbody'));
        this._reset();
    },
    _rowByResult: function(result)
    {
        var row = this._resultRows[result];
        $(row).show();
        if (row)
            return row;

        row = this._resultRows[result] = this._body.insertRow(0);
        row.className = result;
        row.insertCell();
        row.insertCell();
        var titleCell = row.insertCell();
        titleCell.appendChild(document.createElement('span')).textContent = result;
        return row;
    },
    update: function(resultsByBuilder)
    {
        if (this._pendingReset)
            this._reset();

        if (!resultsByBuilder)
            return;

        Object.keys(resultsByBuilder).forEach(function(builderName) {
            var configuration = config.kPlatforms[config.currentPlatform].builders[builderName];
            if (!configuration)
                throw "Unknown builder name: " + builderName;
            var row = this._rowByResult(resultsByBuilder[builderName].actual);
            var cell = cellByBuildType(row, configuration);
            if (cellContainsConfiguration(cell, configuration))
                return;
            cell.appendChild(new ui.failures.Builder(builderName));
        }, this);
    },
    purge: function()
    {
        this._pendingReset = true;
    },
    _reset: function()
    {
        this._pendingReset = false;
        this._resultRows = {};
        $(this._body).empty();
        // Add the BUILDING row eagerly so that it appears last.
        this._rowByResult(kBuildingResult);
        $(this._resultRows[kBuildingResult]).hide();
    }
});

ui.failures.ListItem = base.extends('li', {
    init: function(groupName, failingTestsList)
    {
        this._failingTestsList = failingTestsList;
        this.appendChild(new ui.actions.List([
            new ui.actions.Examine().makeDefault(),
        ]));
        var label = this.appendChild(document.createElement('label'))
        label.textContent = failingTestsList.length == 1 ? failingTestsList[0] : groupName;
    },
});

ui.failures.List = base.extends('ul', {
    init: function()
    {
        this.className = 'failures';
        this.textContent = 'Loading...';
    }
});

})();
