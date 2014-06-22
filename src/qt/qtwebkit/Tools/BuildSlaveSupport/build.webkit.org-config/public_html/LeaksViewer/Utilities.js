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

function getResource(url, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        // Allow a status of 0 for easier testing with local files.
        if (this.readyState == 4 && (!this.status || this.status == 200))
            callback(this);
    };
    xhr.open("GET", url);
    xhr.send();
}

function range(n) {
    var result = new Array(n);
    for (var i = 0; i < n; ++i)
        result[i] = i;
    return result;
}

function workersSupportCyclicStructures() {
    var worker = new Worker("Utilities.js");
    var supportsCyclicStructures = true;

    try {
        var cyclicStructure = { cycle: null };
        cyclicStructure.cycle = cyclicStructure;

        worker.postMessage(cyclicStructure);
    } catch(e) {
        supportsCyclicStructures = false;
    }

    return supportsCyclicStructures;
}

Array.prototype.first = function(predicate) {
    for (var i = 0; i < this.length; ++i) {
        if (predicate(this[i]))
            return this[i];
    }
    return null;
}
