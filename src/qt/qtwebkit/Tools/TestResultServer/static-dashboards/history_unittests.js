// Copyright (C) 2013 Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
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

test('queryHashAsMap', 2, function() {
    equal(window.location.hash, '#useTestData=true');
    deepEqual(history.queryHashAsMap(), {useTestData: 'true'});
});

test('diffStates', 5, function() {
    var newState = {a: 1, b: 2};
    deepEqual(history._diffStates(null, newState), newState);

    var oldState = {a: 1};
    deepEqual(history._diffStates(oldState, newState), {b: 2});

    // FIXME: This is kind of weird. I think the existing users of this code work correctly, but it's a confusing result.
    var oldState = {c: 1};
    deepEqual(history._diffStates(oldState, newState), {a:1, b: 2});

    var oldState = {a: 1, b: 2};
    deepEqual(history._diffStates(oldState, newState), {});

    var oldState = {a: 2, b: 3};
    deepEqual(history._diffStates(oldState, newState), {a: 1, b: 2});
});

test('parseCrossDashboardParameters', 2, function() {
    var historyInstance = new history.History();
    // FIXME(jparent): Remove this once parseParameters moves onto history obj.
    g_history = historyInstance;
    equal(window.location.hash, '#useTestData=true');
    historyInstance.parseCrossDashboardParameters();

    var expectedParameters = {};
    for (var key in history.DEFAULT_CROSS_DASHBOARD_STATE_VALUES)
        expectedParameters[key] = history.DEFAULT_CROSS_DASHBOARD_STATE_VALUES[key];
    expectedParameters.useTestData = true;

    deepEqual(historyInstance.crossDashboardState, expectedParameters);
});
