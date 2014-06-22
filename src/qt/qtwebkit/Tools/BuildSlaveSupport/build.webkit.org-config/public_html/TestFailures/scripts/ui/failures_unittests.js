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

module('ui.failures');

test('Builder', 6, function() {
    var configuration;
    configuration = new ui.failures.Builder("Webkit Linux", ["update", "webkit_tests"]);
    deepEqual(Object.getOwnPropertyNames(configuration.__proto__).sort(), [
        '_addSpan',
        'equals',
        'init',
    ]);
    equal(configuration.outerHTML, '<a class="failing-builder" target="_blank" href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Webkit+Linux"><span class="version">lucid</span><span class="architecture">64-bit</span><span class="failures"> update, webkit_tests</span></a>');
    configuration = new ui.failures.Builder("Webkit Win");
    equal(configuration.outerHTML, '<a class="failing-builder" target="_blank" href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Webkit+Win"><span class="version">xp</span></a>');
    configuration._addSpan('foo', 'bar');
    equal(configuration.outerHTML, '<a class="failing-builder" target="_blank" href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Webkit+Win"><span class="version">xp</span><span class="foo">bar</span></a>');
    ok(configuration.equals({version: 'xp'}));
    ok(!configuration.equals({version: 'lucid', is64bit: true}));
});

test('FailureGrid', 10, function() {
    var grid = new ui.failures.FailureGrid();
    deepEqual(Object.getOwnPropertyNames(grid.__proto__).sort(), [
        "_reset",
        "_rowByResult",
        "init",
        "purge",
        "update"
    ]);
    equal(grid.outerHTML, '<table class="failures">' +
        '<thead><tr><td>type</td><td>release</td><td>debug</td></tr></thead>' +
        '<tbody><tr class="BUILDING" style="display: none; "><td><span>BUILDING</span></td><td></td><td></td></tr></tbody>' +
    '</table>');
    var row = grid._rowByResult('TEXT');
    equal(grid.outerHTML, '<table class="failures">' +
        '<thead><tr><td>type</td><td>release</td><td>debug</td></tr></thead>' +
        '<tbody>' +
            '<tr class="TEXT">' +
                '<td><span>TEXT</span></td><td></td><td></td>' +
            '</tr>' +
            '<tr class="BUILDING" style="display: none; "><td><span>BUILDING</span></td><td></td><td></td></tr>' +
        '</tbody>' +
    '</table>');
    equal(row.outerHTML, '<tr class="TEXT"><td><span>TEXT</span></td><td></td><td></td></tr>');
    grid.update({});
    equal(grid.outerHTML, '<table class="failures">' +
        '<thead><tr><td>type</td><td>release</td><td>debug</td></tr></thead>' +
        '<tbody>' +
            '<tr class="TEXT">' +
                '<td><span>TEXT</span></td><td></td><td></td>' +
            '</tr>' +
            '<tr class="BUILDING" style="display: none; "><td><span>BUILDING</span></td><td></td><td></td></tr>' +
        '</tbody>' +
    '</table>');
    raises(function() {
        grid.update({'Atari': {}})
    });
    grid.update({'Webkit Linux (dbg)': { actual: 'TEXT'}});
    equal(grid.outerHTML, '<table class="failures">' +
        '<thead><tr><td>type</td><td>release</td><td>debug</td></tr></thead>' +
        '<tbody>' +
            '<tr class="TEXT" style="">' +
                '<td><span>TEXT</span></td>' +
                '<td></td>' +
                '<td><a class="failing-builder" target="_blank" href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Webkit+Linux+(dbg)"><span class="version">lucid</span><span class="architecture">64-bit</span></a></td>' +
            '</tr>' +
            '<tr class="BUILDING" style="display: none; "><td><span>BUILDING</span></td><td></td><td></td></tr>' +
        '</tbody>' +
    '</table>');
    grid.update({'Webkit Mac10.6': { actual: 'IMAGE+TEXT'}});
    equal(grid.outerHTML, '<table class="failures">' +
        '<thead><tr><td>type</td><td>release</td><td>debug</td></tr></thead>' +
        '<tbody>' +
            '<tr class="IMAGE+TEXT">' +
                '<td><span>IMAGE+TEXT</span></td>' +
                '<td><a class="failing-builder" target="_blank" href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Webkit+Mac10.6"><span class="version">snowleopard</span></a></td>' +
                '<td></td>' +
            '</tr>' +
            '<tr class="TEXT" style="">' +
                '<td><span>TEXT</span></td>' +
                '<td></td>' +
                '<td><a class="failing-builder" target="_blank" href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Webkit+Linux+(dbg)"><span class="version">lucid</span><span class="architecture">64-bit</span></a></td>' +
            '</tr>' +
            '<tr class="BUILDING" style="display: none; "><td><span>BUILDING</span></td><td></td><td></td></tr>' +
        '</tbody>' +
    '</table>');
    grid.update({'Webkit Mac10.6': { actual: 'IMAGE+TEXT'}});
    equal(grid.outerHTML, '<table class="failures">' +
        '<thead><tr><td>type</td><td>release</td><td>debug</td></tr></thead>' +
        '<tbody>' +
            '<tr class="IMAGE+TEXT" style="">' +
                '<td><span>IMAGE+TEXT</span></td>' +
                '<td><a class="failing-builder" target="_blank" href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Webkit+Mac10.6"><span class="version">snowleopard</span></a></td>' +
                '<td></td>' +
            '</tr>' +
            '<tr class="TEXT" style="">' +
                '<td><span>TEXT</span></td>' +
                '<td></td>' +
                '<td><a class="failing-builder" target="_blank" href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Webkit+Linux+(dbg)"><span class="version">lucid</span><span class="architecture">64-bit</span></a></td>' +
            '</tr>' +
            '<tr class="BUILDING" style="display: none; "><td><span>BUILDING</span></td><td></td><td></td></tr>' +
        '</tbody>' +
    '</table>');
    grid.purge();
    grid.update({'Webkit Linux (dbg)': { actual: 'TEXT'}});
    equal(grid.outerHTML, '<table class="failures">' +
        '<thead><tr><td>type</td><td>release</td><td>debug</td></tr></thead>' +
        '<tbody>' +
            '<tr class="TEXT">' +
                '<td><span>TEXT</span></td>' +
                '<td></td>' +
                '<td><a class="failing-builder" target="_blank" href="http://build.chromium.org/p/chromium.webkit/waterfall?builder=Webkit+Linux+(dbg)"><span class="version">lucid</span><span class="architecture">64-bit</span></a></td>' +
            '</tr>' +
            '<tr class="BUILDING" style="display: none; "><td><span>BUILDING</span></td><td></td><td></td></tr>' +
        '</tbody>' +
    '</table>');
});

}());
