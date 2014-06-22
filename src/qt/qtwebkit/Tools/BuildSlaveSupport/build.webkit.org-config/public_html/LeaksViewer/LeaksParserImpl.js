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

function LeaksParserImpl(didParseLeaksCallback) {
    this._didParseLeaksCallback = didParseLeaksCallback;
    this._profile = this._createNode("top level");
}

LeaksParserImpl.prototype = {
    addLeaksFile: function(leaksText) {
        this._incorporateLeaks(this._parseLeaks(leaksText));
        this._didParseLeaksCallback(this._profile);
    },

    _parseLeaks: function(text) {
        var leaks = [];
        var currentSize = 0;
        text.split("\n").forEach(function(line) {
            var match = /^Leak:.*\ssize=(\d+)\s/.exec(line);
            if (match) {
                currentSize = parseInt(match[1], 10);
                return;
            }
            if (!/^\s+Call stack:/.test(line))
                return;

            // The first frame is not really a frame at all ("Call stack: thread 0xNNNNN:"), so we omit it.
            leaks.push({ size: currentSize, stack: line.split(" | ").slice(1).map(function(str) { return str.trim(); }) });
            currentSize = 0;
        });
        return leaks;
    },

    _createNode: function(functionName) {
        var url;
        var lineNumber;
        var match = /(.*) (\S+):(\d+)$/.exec(functionName);
        if (match) {
            functionName = match[1];
            // FIXME: It would be nice to be able to link straight to the right line in Trac, but to
            // do that we'd have to have some way of translating from filenames to Trac URLs.
            // <http://webkit.org/b/72509>
            url = match[2];
            lineNumber = match[3];
        }

        return {
            functionName: functionName,
            selfTime: 0,
            totalTime: 0,
            averageTime: 0,
            numberOfCalls: 0,
            children: [],
            childrenByName: {},
            callUID: functionName,
            url: url,
            lineNumber: lineNumber,
        };
    },

    // This function creates a fake "profile" from a set of leak stacks. "selfTime" is the number of
    // stacks in which this function was at the top (in theory, only functions like malloc should have a
    // non-zero selfTime). "totalTime" is the number of stacks which contain this function (and thus is
    // the number of leaks that occurred in or beneath this function).
    // FIXME: This is expensive! Can we parallelize it?
    _incorporateLeaks: function(leaks) {
        var self = this;
        leaks.forEach(function(leak) {
            leak.stack.reduce(function(node, frame, index, array) {
                var childNode;
                if (frame in node.childrenByName)
                    childNode = node.childrenByName[frame];
                else {
                    childNode = self._createNode(frame);
                    childNode.head = self._profile;
                    node.childrenByName[frame] = childNode;
                    node.children.push(childNode);
                }
                if (index === array.length - 1)
                    childNode.selfTime += leak.size;
                childNode.totalTime += leak.size;
                ++childNode.numberOfCalls;
                return childNode;
            }, self._profile);
        });
        self._profile.totalTime = self._profile.children.reduce(function(sum, child) { return sum + child.totalTime; }, 0);
    },
};
