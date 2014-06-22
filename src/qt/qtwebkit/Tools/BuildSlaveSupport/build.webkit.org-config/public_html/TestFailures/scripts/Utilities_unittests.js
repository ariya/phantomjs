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

module('Utilities');

test('hasStyleClass', 11, function() {
    var element = document.createElement('div');

    ok(!element.hasStyleClass('foo'));
    ok(!element.hasStyleClass('bar'));

    element.className = 'foo';
    ok(element.hasStyleClass('foo'))
    ok(!element.hasStyleClass('bar'));

    element.className = 'foo foo';
    ok(element.hasStyleClass('foo'))
    ok(!element.hasStyleClass('bar'));

    element.className = 'foo bar';
    ok(element.hasStyleClass('foo'))
    ok(element.hasStyleClass('bar'));
    ok(!element.hasStyleClass('baz'));

    element.className = 'food';
    ok(!element.hasStyleClass('foo'));
    ok(element.hasStyleClass('food'));
});

test('addStyleClass', 4, function() {
    var element = document.createElement('div');

    element.addStyleClass('foo');
    equal(element.className, ' foo');

    element.addStyleClass('foo');
    equal(element.className, ' foo');

    element.addStyleClass('bar');
    equal(element.className, ' foo bar');

    element.addStyleClass('foo');
    equal(element.className, ' foo bar');
});

test('removeStyleClass', 8, function() {
    var element = document.createElement('div');

    element.removeStyleClass('foo');
    equal(element.className, '');

    element.className = 'foo';
    element.removeStyleClass('foo');
    equal(element.className, '');

    element.className = ' foo';
    element.removeStyleClass('foo');
    equal(element.className, ' ');

    element.className = 'foo foo';
    element.removeStyleClass('foo');
    equal(element.className, ' ');

    element.className = 'foo bar';
    element.removeStyleClass('foo');
    equal(element.className, ' bar');

    element.className = 'foo bar foo bar';
    element.removeStyleClass('bar');
    equal(element.className, 'foo  foo ');

    element.className = 'food';
    element.removeStyleClass('foo');
    equal(element.className, 'food');

    element.className = 'foo';
    element.removeStyleClass('food');
    equal(element.className, 'foo');
});

test('toggleStyleClass', 5, function() {
    var element = document.createElement('div');

    element.toggleStyleClass('foo');
    equal(element.className, ' foo');

    element.toggleStyleClass('foo');
    equal(element.className, ' ');

    element.className = 'bar';
    element.toggleStyleClass('foo');
    equal(element.className, 'bar foo');

    element.className = 'food';
    element.toggleStyleClass('foo');
    equal(element.className, 'food foo');

    element.className = 'foo';
    element.toggleStyleClass('food');
    equal(element.className, 'foo food');
});

})();
