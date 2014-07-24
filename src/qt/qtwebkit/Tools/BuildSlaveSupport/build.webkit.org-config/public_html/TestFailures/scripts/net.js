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

var net = net || {};

(function () {

// FIXME: Excise this last bit of jquery ajax code.
// There are callers that depend on automatically parsing the content as JSON or XML
// based off the content-type. Instead we should add net.json and net.xml for those cases.
net.get = function(url, success)
{
    $.get(url, success);
};

net.ajax = function(options)
{
    var xhr = new XMLHttpRequest();
    var method = options.type || 'GET';
    var async = true;
    xhr.open(method, options.url, async);
    xhr.onload = function() {
        if (xhr.status == 200 && options.success)
            options.success(xhr.responseText);
        else if (xhr.status != 200 && options.error)
            options.error();
    };
    xhr.onerror = function() {
        if (options.error)
            options.error();
    };
    var data = options.data || null;
    if (data)
        xhr.setRequestHeader("content-type","application/x-www-form-urlencoded");
    xhr.send(data);
};

net.post = function(url, data, success)
{
    net.ajax({
        url: url,
        type: 'POST',
        data: data,
        success: success,
    });

};

net.probe = function(url, options)
{
    net.ajax({
        url: url,
        type: 'HEAD',
        success: options.success,
        error: options.error,
    });
};

// We use XMLHttpRequest and CORS to fetch JSONP rather than using script tags.
// That's better for security and performance, but we need the server to cooperate
// by setting CORS headers.
net.jsonp = function(url, callback)
{
    net.ajax({
        url: url,
        success: function(jsonp) {
            callback(base.parseJSONP(jsonp));
        },
        error: function() {
            callback({});
        },
    });
};

})();
