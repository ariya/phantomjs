/*jslint sloppy: true, nomen: true */

/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

var REPL = REPL || {};

(function () {

/**
 * Cache to hold completions
 */
var _cache = {};

/**
 * Return the Completions of the Object, applying the prefix
 *
 * @param obj Object to get completions of
 * @param prefix Limit completions to the one starting with this prefix
 */
REPL._getCompletions = function (obj, prefix) {
    var completions = [];

    if (typeof prefix !== "string") {
        prefix = "";
    } else {
        prefix = prefix.trim();
    }

    try {
        // Try to get `QObject` inherited class's name (throws exception if not
        // inherited from `QObject`)
        var className = _repl._getClassName(obj);

        // Initialize completions for this class as needed
        if (null == _cache[className]) {
            _cache[className] = _repl._enumerateCompletions(obj);
        }

        var key = className;
        if ("" !== prefix) {
            key = "-" + prefix;
            if (null == _cache[key]) {
                // Filter out completions
                var regexp = new RegExp("^" + prefix);
                _cache[key] = _cache[className].filter(function (elm) {
                    return regexp.test(elm);
                });
            }
        }
        completions = _cache[key];
    } catch (e) {
        try {
            Object.keys(obj).forEach(function (k) {
                if (obj.hasOwnProperty(k) && k.indexOf(prefix) === 0) {
                    completions.push(k);
                }
            });
            completions.sort();
        } catch (e) {
            // Ignore...
        }
    }

    return completions;
};

/**
 * This utility function is used to pretty-print the result of an expression.
 * @see https://developer.mozilla.org/En/Using_native_JSON#The_replacer_parameter
 *
 * @param k Property key name - empty string if it's the object being stringified
 * @param v Property value
 */
REPL._expResStringifyReplacer = function (k, v) {
    var i, iarr,
        mock = {},
        funcToStr = "[Function]";

    // If the result of the last evaluated expression is an object
    if (k === ""                        //< only first level of recursive calls
        && REPL._lastEval) {

        // Get all the completions for the object we are going to pretty-print
        iarr = REPL._getCompletions(REPL._lastEval);

        // nothing to enumerate
        if (iarr.length == 0)
            return v;

        for (i in iarr) {
            if (typeof(v[iarr[i]]) !== "undefined") {
                // add a reference to this "real" property into the mock object
                mock[iarr[i]] = v[iarr[i]];
            } else {
                // add a "function" for this "shimmed" property into the mock object
                mock[iarr[i]] = funcToStr;
            }
        }

        return mock;
    }

    // Else, just act normally
    if (typeof(v) === "function") {
        // Normally functions are ignored by JSON.stringify
        return funcToStr;
    }

    return v;
};

})();
