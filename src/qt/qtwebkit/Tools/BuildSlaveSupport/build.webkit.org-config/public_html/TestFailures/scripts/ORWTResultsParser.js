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

function ORWTResultsParser() {
}

ORWTResultsParser.prototype = {
    parse: function(resultsHTMLString) {
        var result = { tests: {}, tooManyFailures: false };

        var root = document.createElement('html');
        root.innerHTML = resultsHTMLString;

        // Note that some old results.html files won't contain this element even if we had to stop
        // running early due to too many failures. Our caller will have to figure that out
        // themselves.
        result.tooManyFailures = root.getElementsByClassName('stopped-running-early-message').length > 0;

        function parseResultTable(regex) {
            var paragraph = Array.prototype.findFirst.call(root.querySelectorAll('p'), function(paragraph) {
                return regex.test(paragraph.innerText);
            });
            if (!paragraph)
                return [];
            var table = paragraph.nextElementSibling;
            console.assert(table.nodeName === 'TABLE');
            return Array.prototype.map.call(table.rows, function(row) {
                var links = row.getElementsByTagName('a');
                var result = {
                    name: links[0].innerText,
                };
                for (var i = 1; i < links.length; ++i) {
                    var match = /^crash log \((.*)\)$/.exec(links[i].innerText);
                    if (!match)
                        continue;
                    result.crashingSymbol = match[1];
                    break;
                }
                return result;
            });
        }

        parseResultTable(/did not match expected results/).forEach(function(testData) {
            result.tests[testData.name] = { failureType: 'fail' };
        });
        parseResultTable(/timed out/).forEach(function(testData) {
            result.tests[testData.name] = { failureType: 'timeout' };
        });
        parseResultTable(/tool to crash/).forEach(function(testData) {
            result.tests[testData.name] = {
                failureType: 'crash',
                crashingSymbol: testData.crashingSymbol,
            };
        });
        parseResultTable(/Web process to crash/).forEach(function(testData) {
            result.tests[testData.name] = {
                failureType: 'webprocess crash',
                crashingSymbol: testData.crashingSymbol,
            };
        });

        return result;
    },
};
