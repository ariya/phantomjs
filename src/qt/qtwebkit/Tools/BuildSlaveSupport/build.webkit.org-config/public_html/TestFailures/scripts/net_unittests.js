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

// Export NetworkSimulator for use by other unittests.
function NetworkSimulator()
{
    this._pendingCallbacks = [];
};

NetworkSimulator.prototype.scheduleCallback = function(callback)
{
    this._pendingCallbacks.push(callback);
};

NetworkSimulator.prototype.runTest = function(testCase)
{
    var self = this;
    var realNet = window.net;

    window.net = {};
    if (self.probe)
        net.probe = self.probe;
    if (self.jsonp)
        net.jsonp = self.jsonp;
    if (self.get)
        net.get = self.get;
    if (self.post)
        net.post = self.post;
    if (self.ajax)
        net.ajax = self.ajax;

    testCase();

    while (this._pendingCallbacks.length) {
        var callback = this._pendingCallbacks.shift();
        callback();
    }

    window.net = realNet;
    equal(window.net, realNet, "Failed to restore real base!");
};

(function () {

module("net");

// No unit tests yet!

})();
