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

(function () {

module('ui.actions');

test('Buttons', 6, function() {
    var buttonInfos = [{
        view: new ui.actions.Rollout(),
        text: 'Roll out',
        event: 'rollout'
    }, {
        view: new ui.actions.Examine(),
        text: 'Examine',
        event: 'examine'
    }, {
        view: new ui.actions.Rebaseline(),
        text: 'Rebaseline',
        event: 'rebaseline'
    }];
    buttonInfos.forEach(function(buttonInfo) {
        equal(buttonInfo.view.textContent, buttonInfo.text);
        equal(buttonInfo.view._eventName, buttonInfo.event);
    });
});

test('click', 1, function() {
    var examine = new ui.actions.Examine();
    $(examine).bind('examine', function() {
        ok('Examine triggered.');
    });
    $(examine).trigger('click');
});

test('default', 2, function() {
    var next = new ui.actions.Next();
    equal(next.makeDefault(), next);
    equal(next.className, 'action next default');
});

test('List', 1, function() {
    var list = new ui.actions.List([
        new ui.actions.Rebaseline(),
        new ui.actions.Previous(),
        new ui.actions.Next()
    ]);
    equal(list.innerHTML,
        '<li><button class="action">Rebaseline</button></li>' +
        '<li><button class="action previous">\u25C0</button></li>' +
        '<li><button class="action next">\u25B6</button></li>');
});

}());
