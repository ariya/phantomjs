// Copyright (C) 2013 Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//         * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//         * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//         * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


var history = history || {};

(function() {

history.DEFAULT_CROSS_DASHBOARD_STATE_VALUES = {
    group: null,
    showAllRuns: false,
    testType: 'layout-tests',
    useTestData: false,
}    

history.validateParameter = function(state, key, value, validateFn)
{
    if (validateFn())
        state[key] = value;
    else
        console.log(key + ' value is not valid: ' + value);
}

history.isTreeMap = function()
{
    return string.endsWith(window.location.pathname, 'treemap.html');
}

// TODO(jparent): Make private once callers move here.
history.queryHashAsMap = function()
{
    var hash = window.location.hash;
    var paramsList = hash ? hash.substring(1).split('&') : [];
    var paramsMap = {};
    var invalidKeys = [];
    for (var i = 0; i < paramsList.length; i++) {
        var thisParam = paramsList[i].split('=');
        if (thisParam.length != 2) {
            console.log('Invalid query parameter: ' + paramsList[i]);
            continue;
        }

        paramsMap[thisParam[0]] = decodeURIComponent(thisParam[1]);
    }

    // FIXME: remove support for mapping from the master parameter to the group
    // one once the waterfall starts to pass in the builder name instead.
    if (paramsMap.master) {
        paramsMap.group = LEGACY_BUILDER_MASTERS_TO_GROUPS[paramsMap.master];
        if (!paramsMap.group)
            console.log('ERROR: Unknown master name: ' + paramsMap.master);
        window.location.hash = window.location.hash.replace('master=' + paramsMap.master, 'group=' + paramsMap.group);
        delete paramsMap.master;
    }

    return paramsMap;
}

history._diffStates = function(oldState, newState)
{
    // If there is no old state, everything in the current state is new.
    if (!oldState)
        return newState;

    var changedParams = {};
    for (curKey in newState) {
        var oldVal = oldState[curKey];
        var newVal = newState[curKey];
        // Add new keys or changed values.
        if (!oldVal || oldVal != newVal)
            changedParams[curKey] = newVal;
    }
    return changedParams;
}

history._fillMissingValues = function(to, from)
{
    for (var state in from) {
        if (!(state in to))
            to[state] = from[state];
    }
}

history.History = function(configuration)
{
    this.crossDashboardState = {};
    this.dashboardSpecificState = {};

    if (configuration) {
        this._defaultDashboardSpecificStateValues = configuration.defaultStateValues;
        this._handleValidHashParameter = configuration.handleValidHashParameter;
        this._handleQueryParameterChange = configuration.handleQueryParameterChange || function(historyInstance, params) { return true; };
        this._dashboardSpecificInvalidatingParameters = configuration.invalidatingHashParameters;
        this._generatePage = configuration.generatePage;
    }
}

var RELOAD_REQUIRING_PARAMETERS = ['showAllRuns', 'group', 'testType'];

var CROSS_DB_INVALIDATING_PARAMETERS = {
    'testType': 'group'
};

history.History.prototype = {
    initialize: function()
    {
        window.onhashchange = this._handleLocationChange.bind(this);
        this._handleLocationChange();
    },
    isLayoutTestResults: function()
    {
        return this.crossDashboardState.testType == 'layout-tests';
    },
    isGPUTestResults: function()
    {
        return this.crossDashboardState.testType == 'gpu_tests';
    },
    parseCrossDashboardParameters: function()
    {
        this.crossDashboardState = {};
        var parameters = history.queryHashAsMap();
        for (parameterName in history.DEFAULT_CROSS_DASHBOARD_STATE_VALUES)
            this.parseParameter(parameters, parameterName);

        history._fillMissingValues(this.crossDashboardState, history.DEFAULT_CROSS_DASHBOARD_STATE_VALUES);
    },
    _parseDashboardSpecificParameters: function()
    {
        this.dashboardSpecificState = {};
        var parameters = history.queryHashAsMap();
        for (parameterName in this._defaultDashboardSpecificStateValues)
            this.parseParameter(parameters, parameterName);
    },
    // TODO(jparent): Make private once callers move here.
    parseParameters: function()
    {
        var oldCrossDashboardState = this.crossDashboardState;
        var oldDashboardSpecificState = this.dashboardSpecificState;

        this.parseCrossDashboardParameters();
        
        // Some parameters require loading different JSON files when the value changes. Do a reload.
        if (Object.keys(oldCrossDashboardState).length) {
            for (var key in this.crossDashboardState) {
                if (oldCrossDashboardState[key] != this.crossDashboardState[key] && RELOAD_REQUIRING_PARAMETERS.indexOf(key) != -1) {
                    window.location.reload();
                    return false;
                }
            }
        }

        this._parseDashboardSpecificParameters();
        var dashboardSpecificDiffState = history._diffStates(oldDashboardSpecificState, this.dashboardSpecificState);

        history._fillMissingValues(this.dashboardSpecificState, this._defaultDashboardSpecificStateValues);

        // FIXME: dashboard_base shouldn't know anything about specific dashboard specific keys.
        if (dashboardSpecificDiffState.builder)
            delete this.dashboardSpecificState.tests;
        if (this.dashboardSpecificState.tests)
            delete this.dashboardSpecificState.builder;

        var shouldGeneratePage = true;
        if (Object.keys(dashboardSpecificDiffState).length)
            shouldGeneratePage = this._handleQueryParameterChange(this, dashboardSpecificDiffState);
        return shouldGeneratePage;
    },
    // TODO(jparent): Make private once callers move here.
    parseParameter: function(parameters, key)
    {
        if (!(key in parameters))
            return;
        var value = parameters[key];
        if (!this._handleValidHashParameterWrapper(key, value))
            console.log("Invalid query parameter: " + key + '=' + value);
    },
    // Takes a key and a value and sets the this.dashboardSpecificState[key] = value iff key is
    // a valid hash parameter and the value is a valid value for that key. Handles
    // cross-dashboard parameters then falls back to calling
    // handleValidHashParameter for dashboard-specific parameters.
    //
    // @return {boolean} Whether the key what inserted into the this.dashboardSpecificState.
    _handleValidHashParameterWrapper: function(key, value)
    {
        switch(key) {
        case 'testType':
            history.validateParameter(this.crossDashboardState, key, value,
                function() { return TEST_TYPES.indexOf(value) != -1; });
            return true;

        case 'group':
            history.validateParameter(this.crossDashboardState, key, value,
                function() {
                  return value in LAYOUT_TESTS_BUILDER_GROUPS;
                });
            return true;

        case 'useTestData':
        case 'showAllRuns':
            this.crossDashboardState[key] = value == 'true';
            return true;

        default:
            return this._handleValidHashParameter(this, key, value);
        }
    },
    queryParameterValue: function(parameter)
    {
        return this.dashboardSpecificState[parameter] || this.crossDashboardState[parameter];
    }, 
    // Sets the page state. Takes varargs of key, value pairs.
    setQueryParameter: function(var_args)
    {
        var queryParamsAsState = {};
        for (var i = 0; i < arguments.length; i += 2) {
            var key = arguments[i];
            queryParamsAsState[key] = arguments[i + 1];
        }

        this.invalidateQueryParameters(queryParamsAsState);

        var newState = this._combinedDashboardState();
        for (var key in queryParamsAsState) {
            newState[key] = queryParamsAsState[key];
        }

        // Note: We use window.location.hash rather that window.location.replace
        // because of bugs in Chrome where extra entries were getting created
        // when back button was pressed and full page navigation was occuring.
        // FIXME: file those bugs.
        window.location.hash = this._permaLinkURLHash(newState);
    },
    toggleQueryParameter: function(param)
    {
        this.setQueryParameter(param, !this.queryParameterValue(param));
    },
    invalidateQueryParameters: function(queryParamsAsState)
    {
        for (var key in queryParamsAsState) {
            if (key in CROSS_DB_INVALIDATING_PARAMETERS)
                delete this.crossDashboardState[CROSS_DB_INVALIDATING_PARAMETERS[key]];
            if (this._dashboardSpecificInvalidatingParameters && key in this._dashboardSpecificInvalidatingParameters)
                delete this.dashboardSpecificState[this._dashboardSpecificInvalidatingParameters[key]];
        }
    },
    _joinParameters: function(stateObject)
    {
        var state = [];
        for (var key in stateObject) {
            var value = stateObject[key];
            if (value != this._defaultValue(key))
                state.push(key + '=' + encodeURIComponent(value));
        }
        return state.join('&');
    }, 
    _permaLinkURLHash: function(opt_state)
    {
        var state = opt_state || this._combinedDashboardState();
        return '#' + this._joinParameters(state);
    },
    _combinedDashboardState: function()
    {
        var combinedState = Object.create(this.dashboardSpecificState);
        for (var key in this.crossDashboardState)
            combinedState[key] = this.crossDashboardState[key];
        return combinedState;    
    },
    _defaultValue: function(key)
    {
        if (key in this._defaultDashboardSpecificStateValues)
            return this._defaultDashboardSpecificStateValues[key];
        return history.DEFAULT_CROSS_DASHBOARD_STATE_VALUES[key];
    },
    _handleLocationChange: function()
    {
        if (this.parseParameters())
            this._generatePage(this);
    }

}

})();
