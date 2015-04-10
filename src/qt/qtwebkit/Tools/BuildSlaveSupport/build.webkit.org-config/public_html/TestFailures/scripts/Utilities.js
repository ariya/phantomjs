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

function createDescriptionList(items) {
    var list = document.createElement('dl');
    items.forEach(function(pair) {
        var dt = document.createElement('dt');
        dt.appendChild(pair[0]);
        var dd = document.createElement('dd');
        dd.appendChild(pair[1]);
        list.appendChild(dt);
        list.appendChild(dd);
    });
    return list;
}

function fetchResource(url, method, queryParameters, callback, errorCallback) {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState !== 4)
            return;
        // Allow a status of 0 for easier testing with local files.
        if (!this.status || this.status === 200)
            callback(this);
        else if (errorCallback)
            errorCallback(this);
    };

    if (method === 'GET' && queryParameters)
        url = addQueryParametersToURL(url, queryParameters);

    xhr.open(method, url);

    if (method === 'GET') {
        xhr.send();
        return;
    }

    var data = urlEncodeQueryParameters(queryParameters);
    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    xhr.send(data);
}

function getResource(url, callback, errorCallback) {
    fetchResource(url, 'GET', null, callback, errorCallback);
}

function urlEncodeQueryParameters(queryParameters) {
    var encodedParameters = Object.keys(queryParameters).map(function(key) {
        return key + '=' + encodeURIComponent(queryParameters[key]);
    });
    return encodedParameters.join('&');
}

function addQueryParametersToURL(url, queryParameters) {
    if (url.indexOf('?') < 0)
        url += '?';
    else
        url += '&';

    return url + urlEncodeQueryParameters(queryParameters);
}

function longestCommonPathPrefix(paths) {
    const separator = '/';

    var splitPaths = paths.map(function(path) { return path.split(separator) });
    var firstSplitPath = splitPaths.shift();

    var result = [];
    for (var i = 0; i < firstSplitPath.length; ++i) {
        if (!splitPaths.every(function(splitPath) { return splitPath[i] === firstSplitPath[i] }))
            break;
        result.push(firstSplitPath[i]);
    }

    if (!result.length)
        return null;
    return result.join(separator);
}

function removePathExtension(string) {
    var dotIndex = string.lastIndexOf('.');
    if (dotIndex < 0)
        return string;
    return string.substring(0, dotIndex);
}

function sorted(array, sortFunction) {
    var newArray = array.slice();
    newArray.sort(sortFunction);
    return newArray;
}

Array.prototype.contains = function(value) {
    return this.indexOf(value) >= 0;
}

Array.prototype.findFirst = function(predicate) {
    for (var i = 0; i < this.length; ++i) {
        if (predicate(this[i]))
            return this[i];
    }
    return null;
}

Array.prototype.findLast = function(predicate) {
    for (var i = this.length - 1; i >= 0; --i) {
        if (predicate(this[i]))
            return this[i];
    }
    return null;
}

Array.prototype.last = function() {
    if (!this.length)
        return undefined;
    return this[this.length - 1];
}

Element.prototype.hasStyleClass = function(klass) {
    var regex = new RegExp('\\b' + klass + '\\b');
    return regex.test(this.className);
}

Element.prototype.addStyleClass = function(klass) {
    if (this.hasStyleClass(klass))
        return;
    this.className += ' ' + klass;
}

Element.prototype.removeStyleClass = function(klass) {
    var regex = new RegExp('\\b' + klass + '\\b', 'g');
    this.className = this.className.replace(regex, '');
}

Element.prototype.toggleStyleClass = function(klass) {
    if (this.hasStyleClass(klass))
        this.removeStyleClass(klass);
    else
        this.addStyleClass(klass);
}

Node.prototype.appendChildren = function(children) {
    for (var i = 0; i < children.length; ++i)
        this.appendChild(children[i]);
}

Node.prototype.removeAllChildren = function() {
    while (this.firstChild)
        this.removeChild(this.firstChild);
}

String.prototype.contains = function(substring) {
    return this.indexOf(substring) >= 0;
}
