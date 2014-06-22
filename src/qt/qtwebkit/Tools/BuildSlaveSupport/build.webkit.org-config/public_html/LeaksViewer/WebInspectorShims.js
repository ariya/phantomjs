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

// This file contains definitions that are needed to satisfy the Web Inspector JavaScript files we
// import.

var WebInspector = {
    displayNameForURL: function(url) {
        return url;
    },

    linkifyURLAsNode: function(url, linkText, classes, tooltipText) {
        if (!linkText)
            linkText = url;
        classes = (classes ? classes + " " : "");

        // FIXME: Create an <a> element here once we actually have somewhere useful to link to.
        // <http://webkit.org/b/72509>
        var a = document.createElement("span");
        a.href = url;
        a.className = classes;
        if (typeof tooltipText === "undefined")
            a.title = url;
        else if (typeof tooltipText !== "string" || tooltipText.length)
            a.title = tooltipText;
        a.textContent = linkText;

        return a;
    },

    linkifyResourceAsNode: function(url, preferredPanel, lineNumber, classes, tooltipText) {
        var linkText = WebInspector.displayNameForURL(url);
        if (lineNumber)
            linkText += ":" + lineNumber;
        return WebInspector.linkifyURLAsNode(url, linkText, classes, tooltipText);
    },

    UIString: function(string) {
        return String.vsprintf(string, Array.prototype.slice.call(arguments, 1));
    },
};

var Preferences = {
    // Setting this to false causes the "Average" and "Calls" columns to be shown.
    samplingCPUProfiler: false,
};

var ProfilerAgent = {
    getProfile: function(typeId, uid, callback) {
        this._callback = callback;
    },

    profileReady: function(head) {
        this._callback({ head: head });
    },
};

// This function makes any required changes to objects imported from the Web Inspector JavaScript
// files.
function monkeyPatchInspectorObjects() {
    var originalGetter = WebInspector.ProfileDataGridNode.prototype.__lookupGetter__("data");
    WebInspector.ProfileDataGridNode.prototype.__defineGetter__("data", function() {
        var data = originalGetter.call(this);

        // ProfileDataGridNode formats values as milliseconds, but we are instead measuring bytes.
        if (!this.profileView.showSelfTimeAsPercent)
            data.self = Number.bytesToString(this.selfTime);
        if (!this.profileView.showTotalTimeAsPercent)
            data.total = Number.bytesToString(this.totalTime);
        if (!this.profileView.showAverageTimeAsPercent)
            data.average = Number.bytesToString(this.averageTime);

        return data;
    });
}
