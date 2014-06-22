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

module('NewBugForm');

const testFormData = {
    component: { formName: 'component', value: 'Tools / Tests' },
    description: { formName: 'comment', value: 'This is a description\n\nwith\nnewlines' },
    keywords: { formName: 'keywords', value: 'Qt, PlatformOnly' },
    operatingSystem: { formName: 'op_sys', value: 'Windows 7' },
    platform: { formName: 'rep_platform', value: 'PC' },
    product: { formName: 'product', value: 'WebKit' },
    title: { formName: 'short_desc', value: 'This is a bug title' },
    url: { formName: 'bug_file_loc', value: 'http://example.com/path?query=foo#anchor' },
    version: { formName: 'version', value: '528+ (Nightly Build)' },
};

function createTestForm() {
    var form = new NewBugForm();
    for (var key in testFormData) {
        form[key] = testFormData[key].value;
    }
    return form;
}

test('properties are set', 9, function() {
    var form = createTestForm();

    for (var key in testFormData) {
        equal(form[key], testFormData[key].value);
    }
});

test('domElement() posts to enter_bug.cgi', 3, function() {
    var formElement = createTestForm().domElement();
    equal(formElement.tagName, 'FORM');
    equal(formElement.method, 'post');
    equal(formElement.action, 'https://bugs.webkit.org/enter_bug.cgi');
});

test('domElement() contains only hidden input elements', 9, function() {
    var elements = createTestForm().domElement().elements;
    for (var i = 0; i < elements.length; ++i) {
        equal(elements[i].type, 'hidden');
    }
});

test('domElement() contains all form values', 9, function() {
    var formElement = createTestForm().domElement();

    for (var key in testFormData) {
        var data = testFormData[key];
        equal(formElement[data.formName].value, data.value, key);
    }
});

})();
