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

(function() {

module('FlakyLayoutTestDetector');

const runs = [
    {
        buildName: 'build6',
        failingTests: {
            b: 'fail',
        },
        tooManyFailures: false,
    },
    {
        buildName: 'build5',
        failingTests: {
            a: 'fail',
            b: 'fail',
        },
        tooManyFailures: false,
    },
    {
        buildName: 'build4',
        failingTests: {
            b: 'fail',
        },
        tooManyFailures: false,
    },
    {
        buildName: 'build3',
        failingTests: {
            a: 'crash',
            b: 'fail',
        },
        tooManyFailures: false,
    },
    {
        buildName: 'build2',
        failingTests: {
            a: 'fail',
        },
        tooManyFailures: false,
    },
    {
        buildName: 'build1',
        failingTests: {},
        tooManyFailures: false,
    },
];

test('possiblyFlakyTests', 1, function() {
    var detector = new FlakyLayoutTestDetector();
    runs.forEach(function(run) { detector.incorporateTestResults(run.buildName, run.failingTests, run.tooManyFailures) });

    deepEqual(detector.possiblyFlakyTests, ['a']);
});

test('allFailures', 3, function() {
    var detector = new FlakyLayoutTestDetector();
    runs.forEach(function(run) { detector.incorporateTestResults(run.buildName, run.failingTests, run.tooManyFailures) });

    deepEqual(detector.allFailures('a'), [
        { build: 'build5', result: 'fail' },
        { build: 'build3', result: 'crash' },
        { build: 'build2', result: 'fail' },
    ]);

    deepEqual(detector.allFailures('b'), [
        { build: 'build6', result: 'fail' },
        { build: 'build5', result: 'fail' },
        { build: 'build4', result: 'fail' },
        { build: 'build3', result: 'fail' },
    ]);

    equal(detector.allFailures('c'), null);
});

test('failing many times in a row should not count as flaky', 3, function() {
    var detector = new FlakyLayoutTestDetector();
    for (var i = 0; i < 10; ++i)
        detector.incorporateTestResults('build', { a: 'fail' }, false);

    deepEqual(detector.possiblyFlakyTests, []);

    for (var i = 0; i < 3; ++i)
        detector.incorporateTestResults('build', {}, false);

    deepEqual(detector.possiblyFlakyTests, []);

    detector.incorporateTestResults('build', { a: 'fail' }, false);

    deepEqual(detector.possiblyFlakyTests, []);
});

test('failing after passing many times in a row should not count as flaky', 3, function() {
    var detector = new FlakyLayoutTestDetector();
    detector.incorporateTestResults('build', { a: 'fail' }, false);

    deepEqual(detector.possiblyFlakyTests, []);

    for (var i = 0; i < 10; ++i)
        detector.incorporateTestResults('build', {}, false);

    deepEqual(detector.possiblyFlakyTests, []);

    detector.incorporateTestResults('build', { a: 'fail' }, false);

    deepEqual(detector.possiblyFlakyTests, []);
});

test('flaking now should override many past failures', 1, function() {
    var detector = new FlakyLayoutTestDetector();
    detector.incorporateTestResults('build', { a: 'fail' }, false);
    detector.incorporateTestResults('build', {}, false);
    detector.incorporateTestResults('build', { a: 'fail' }, false);
    detector.incorporateTestResults('build', {}, false);
    for (var i = 0; i < 10; ++i)
        detector.incorporateTestResults('build', { a: 'fail' }, false);
    deepEqual(detector.possiblyFlakyTests, ['a']);
});

test('passing now should override past flakiness', 1, function() {
    var detector = new FlakyLayoutTestDetector();
    for (var i = 0; i < 10; ++i)
        detector.incorporateTestResults('build', {}, false);
    detector.incorporateTestResults('build', { a: 'fail' }, false);
    detector.incorporateTestResults('build', {}, false);
    detector.incorporateTestResults('build', { a: 'fail' }, false);
    detector.incorporateTestResults('build', {}, false);
    deepEqual(detector.possiblyFlakyTests, []);
});

test('too many failures now should not override past flakiness', 1, function() {
    var detector = new FlakyLayoutTestDetector();
    for (var i = 0; i < 10; ++i)
        detector.incorporateTestResults('build', {}, true);
    detector.incorporateTestResults('build', { a: 'fail' }, false);
    detector.incorporateTestResults('build', {}, false);
    detector.incorporateTestResults('build', { a: 'fail' }, false);
    detector.incorporateTestResults('build', {}, false);
    deepEqual(detector.possiblyFlakyTests, ['a']);
});

})();
