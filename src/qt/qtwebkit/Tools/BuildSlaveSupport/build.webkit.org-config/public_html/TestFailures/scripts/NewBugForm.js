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

function NewBugForm() {
}

NewBugForm.prototype = {
    domElement: function() {
        var formData = {};

        if (this.component)
            formData.component = this.component;
        if (this.description)
            formData.comment = this.description;
        if (this.keywords)
            formData.keywords = this.keywords;
        if (this.operatingSystem)
            formData.op_sys = this.operatingSystem;
        if (this.platform)
            formData.rep_platform = this.platform;
        if (this.product)
            formData.product = this.product;
        if (this.title)
            formData.short_desc = this.title;
        if (this.url)
            formData.bug_file_loc = this.url;
        if (this.version)
            formData.version = this.version;

        var form = document.createElement('form');
        form.method = 'POST';
        form.action = config.kBugzillaURL + '/enter_bug.cgi';

        for (var key in formData) {
            var input = document.createElement('input');
            input.type = 'hidden';
            input.name = key;
            input.value = formData[key];
            form.appendChild(input);
        }

        return form;
    },
};
