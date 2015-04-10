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

function NRWTResultsParser() {
}

NRWTResultsParser.prototype = {
    parse: function(unexpectedResultsJS) {
        var data;
        function ADD_RESULTS(x) {
            data = x;
        }

        eval(unexpectedResultsJS);
        console.assert(data);

        var result = { tests: {}, tooManyFailures: data.interrupted };

        function forEachTest(tree, handler, opt_prefix) {
            var prefix = opt_prefix || '';

            for (var key in tree) {
                var newPrefix = prefix ? (prefix + '/' + key) : key;
                if ('actual' in tree[key]) {
                    var testObject = tree[key];
                    testObject.name = newPrefix;
                    handler(testObject);
                } else
                    forEachTest(tree[key], handler, newPrefix);
            }
        }

        function isFailureExpected(expected, actual) {
            if (actual === 'SKIP')
                return true;

            var expectedArray = expected.split(' ');
            var actualArray = actual.split(' ');
            for (var i = 0; i < actualArray.length; i++) {
                var actualValue = actualArray[i];
                if (expectedArray.contains(actualValue))
                    continue;
                if (expectedArray.contains('FAIL') && ['IMAGE', 'TEXT', 'IMAGE+TEXT'].contains(actualValue))
                    continue;
                return false;
            }

            return true;
        }

        function convertNRWTResultString(nrwtResult) {
            const translations = {
                CRASH: 'crash',
                'IMAGE+TEXT': 'fail',
                IMAGE: 'fail',
                TEXT: 'fail',
                TIMEOUT: 'timeout',
            };

            if (nrwtResult in translations)
                return translations[nrwtResult];

            if (nrwtResult.contains(' '))
                return 'flaky';

            return 'unknown failure type ' + nrwtResult;
        }

        forEachTest(data.tests, function(test) {
            if (isFailureExpected(test.expected, test.actual))
                return;
            result.tests[test.name] = { failureType: convertNRWTResultString(test.actual) };
        });

        return result;
    },
};
