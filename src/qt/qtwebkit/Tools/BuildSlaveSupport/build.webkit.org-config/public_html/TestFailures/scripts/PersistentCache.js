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

var PersistentCache = {
    contains: function(key) {
        return key in localStorage;
    },

    get: function(key) {
        var string = localStorage[key];
        if (!string)
            return string;

        // FIXME: We could update the date stored with the value here to make this more of an MRU
        // cache (instead of most-recently-stored), but that would result in extra disk access that
        // might not be so great.
        return JSON.parse(this._parseDateAndJSONFromString(string).json);
    },

    set: function(key, value) {
        try {
            localStorage[key] = this._addDateToJSONString(JSON.stringify(value));
        } catch (e) {
            if (e.code !== 22) // QUOTA_EXCEEDED_ERR
                throw e;

            // We've run out of space in localStorage. Let's just throw away everything and try
            // again.
            this._emptyCache();
            this.set(key, value);
        }
    },

    prune: function() {
        var now = Date.now();
        for (var key in localStorage) {
            var date = this._parseDateAndJSONFromString(localStorage[key]).date;
            if (now - date <= this._dataAgeLimitMS)
                continue;
            delete localStorage[key];
        }

        this.set(this._lastPruneDateKey, now);
    },

    _addDateToJSONString: function(jsonString) {
        return Date.now() + this._dateAndJSONSeparator + jsonString;
    },

    _dataAgeLimitMS: 1000 * 60 * 60 * 24 * 1.1, // Just over one day

    _dateAndJSONSeparator: ': ',

    _emptyCache: function() {
        for (var key in localStorage)
            delete localStorage[key];
    },

    _parseDateAndJSONFromString: function(string) {
        var separatorIndex = string.indexOf(this._dateAndJSONSeparator);
        return {
            date: new Date(parseInt(string.substring(0, separatorIndex), 10)),
            json: string.substring(separatorIndex + this._dateAndJSONSeparator.length),
        };
    },
};
