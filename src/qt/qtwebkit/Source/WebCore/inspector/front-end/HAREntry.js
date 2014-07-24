/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// See http://groups.google.com/group/http-archive-specification/web/har-1-2-spec
// for HAR specification.

// FIXME: Some fields are not yet supported due to back-end limitations.
// See https://bugs.webkit.org/show_bug.cgi?id=58127 for details.

/**
 * @constructor
 * @param {WebInspector.NetworkRequest} request
 */
WebInspector.HAREntry = function(request)
{
    this._request = request;
}

WebInspector.HAREntry.prototype = {
    /**
     * @return {Object}
     */
    build: function()
    {
        var entry =  {
            startedDateTime: new Date(this._request.startTime * 1000),
            time: WebInspector.HAREntry._toMilliseconds(this._request.duration),
            request: this._buildRequest(),
            response: this._buildResponse(),
            cache: { }, // Not supported yet.
            timings: this._buildTimings()
        };
        var page = WebInspector.networkLog.pageLoadForRequest(this._request);
        if (page)
            entry.pageref = "page_" + page.id;
        return entry;
    },

    /**
     * @return {Object}
     */
    _buildRequest: function()
    {
        var res = {
            method: this._request.requestMethod,
            url: this._buildRequestURL(this._request.url),
            httpVersion: this._request.requestHttpVersion,
            headers: this._request.requestHeaders,
            queryString: this._buildParameters(this._request.queryParameters || []),
            cookies: this._buildCookies(this._request.requestCookies || []),
            headersSize: this._request.requestHeadersSize,
            bodySize: this.requestBodySize
        };
        if (this._request.requestFormData)
            res.postData = this._buildPostData();

        return res;
    },

    /**
     * @return {Object}
     */
    _buildResponse: function()
    {
        return {
            status: this._request.statusCode,
            statusText: this._request.statusText,
            httpVersion: this._request.responseHttpVersion,
            headers: this._request.responseHeaders,
            cookies: this._buildCookies(this._request.responseCookies || []),
            content: this._buildContent(),
            redirectURL: this._request.responseHeaderValue("Location") || "",
            headersSize: this._request.responseHeadersSize,
            bodySize: this.responseBodySize
        };
    },

    /**
     * @return {Object}
     */
    _buildContent: function()
    {
        var content = {
            size: this._request.resourceSize,
            mimeType: this._request.mimeType,
            // text: this._request.content // TODO: pull out into a boolean flag, as content can be huge (and needs to be requested with an async call)
        };
        var compression = this.responseCompression;
        if (typeof compression === "number")
            content.compression = compression;
        return content;
    },

    /**
     * @return {Object}
     */
    _buildTimings: function()
    {
        var waitForConnection = this._interval("connectStart", "connectEnd");
        var blocked;
        var connect;
        var dns = this._interval("dnsStart", "dnsEnd");
        var send = this._interval("sendStart", "sendEnd");
        var ssl = this._interval("sslStart", "sslEnd");

        if (ssl !== -1 && send !== -1)
            send -= ssl;

        if (this._request.connectionReused) {
            connect = -1;
            blocked = waitForConnection;
        } else {
            blocked = 0;
            connect = waitForConnection;
            if (dns !== -1)
                connect -= dns;
        }

        return {
            blocked: blocked,
            dns: dns,
            connect: connect,
            send: send,
            wait: this._interval("sendEnd", "receiveHeadersEnd"),
            receive: WebInspector.HAREntry._toMilliseconds(this._request.receiveDuration),
            ssl: ssl
        };
    },

    /**
     * @return {Object}
     */
    _buildPostData: function()
    {
        var res = {
            mimeType: this._request.requestHeaderValue("Content-Type"),
            text: this._request.requestFormData
        };
        if (this._request.formParameters)
            res.params = this._buildParameters(this._request.formParameters);
        return res;
    },

    /**
     * @param {Array.<Object>} parameters
     * @return {Array.<Object>}
     */
    _buildParameters: function(parameters)
    {
        return parameters.slice();
    },

    /**
     * @param {string} url
     * @return {string}
     */
    _buildRequestURL: function(url)
    {
        return url.split("#", 2)[0];
    },

    /**
     * @param {Array.<WebInspector.Cookie>} cookies
     * @return {Array.<Object>}
     */
    _buildCookies: function(cookies)
    {
        return cookies.map(this._buildCookie.bind(this));
    },

    /**
     * @param {WebInspector.Cookie} cookie
     * @return {Object}
     */
    _buildCookie: function(cookie)
    {
        return {
            name: cookie.name(),
            value: cookie.value(),
            path: cookie.path(),
            domain: cookie.domain(),
            expires: cookie.expiresDate(new Date(this._request.startTime * 1000)),
            httpOnly: cookie.httpOnly(),
            secure: cookie.secure()
        };
    },

    /**
     * @param {string} start
     * @param {string} end
     * @return {number}
     */
    _interval: function(start, end)
    {
        var timing = this._request.timing;
        if (!timing)
            return -1;
        var startTime = timing[start];
        return typeof startTime !== "number" || startTime === -1 ? -1 : Math.round(timing[end] - startTime);
    },

    /**
     * @return {number}
     */
    get requestBodySize()
    {
        return !this._request.requestFormData ? 0 : this._request.requestFormData.length;
    },

    /**
     * @return {number}
     */
    get responseBodySize()
    {
        if (this._request.cached || this._request.statusCode === 304)
            return 0;
        return this._request.transferSize - this._request.responseHeadersSize
    },

    /**
     * @return {number|undefined}
     */
    get responseCompression()
    {
        if (this._request.cached || this._request.statusCode === 304)
            return;
        return this._request.resourceSize - (this._request.transferSize - this._request.responseHeadersSize);
    }
}

/**
 * @param {number} time
 * @return {number}
 */
WebInspector.HAREntry._toMilliseconds = function(time)
{
    return time === -1 ? -1 : Math.round(time * 1000);
}

/**
 * @constructor
 * @param {Array.<WebInspector.NetworkRequest>} requests
 */
WebInspector.HARLog = function(requests)
{
    this._requests = requests;
}

WebInspector.HARLog.prototype = {
    /**
     * @return {Object}
     */
    build: function()
    {
        return {
            version: "1.2",
            creator: this._creator(),
            pages: this._buildPages(),
            entries: this._requests.map(this._convertResource.bind(this))
        }
    },

    _creator: function()
    {
        var webKitVersion = /AppleWebKit\/([^ ]+)/.exec(window.navigator.userAgent);

        return {
            name: "WebInspector",
            version: webKitVersion ? webKitVersion[1] : "n/a"
        };
    },

    /**
     * @return {Array}
     */
    _buildPages: function()
    {
        var seenIdentifiers = {};
        var pages = [];
        for (var i = 0; i < this._requests.length; ++i) {
            var page = WebInspector.networkLog.pageLoadForRequest(this._requests[i]);
            if (!page || seenIdentifiers[page.id])
                continue;
            seenIdentifiers[page.id] = true;
            pages.push(this._convertPage(page));
        }
        return pages;
    },

    /**
     * @param {WebInspector.PageLoad} page
     * @return {Object}
     */
    _convertPage: function(page)
    {
        return {
            startedDateTime: new Date(page.startTime * 1000),
            id: "page_" + page.id,
            title: page.url, // We don't have actual page title here. URL is probably better than nothing.
            pageTimings: {
                onContentLoad: this._pageEventTime(page, page.contentLoadTime),
                onLoad: this._pageEventTime(page, page.loadTime)
            }
        }
    },

    /**
     * @param {WebInspector.NetworkRequest} request
     * @return {Object}
     */
    _convertResource: function(request)
    {
        return (new WebInspector.HAREntry(request)).build();
    },

    /**
     * @param {WebInspector.PageLoad} page
     * @param {number} time
     * @return {number}
     */
    _pageEventTime: function(page, time)
    {
        var startTime = page.startTime;
        if (time === -1 || startTime === -1)
            return -1;
        return WebInspector.HAREntry._toMilliseconds(time - startTime);
    }
}

/**
 * @constructor
 */
WebInspector.HARWriter = function()
{
}

WebInspector.HARWriter.prototype = {
    /**
     * @param {WebInspector.OutputStream} stream
     * @param {Array.<WebInspector.NetworkRequest>} requests
     * @param {WebInspector.Progress} progress
     */
    write: function(stream, requests, progress)
    {
        this._stream = stream;
        this._harLog = (new WebInspector.HARLog(requests)).build();
        this._pendingRequests = 1; // Guard against completing resource transfer before all requests are made.
        var entries = this._harLog.entries;
        for (var i = 0; i < entries.length; ++i) {
            var content = requests[i].content;
            if (typeof content === "undefined" && requests[i].finished) {
                ++this._pendingRequests;
                requests[i].requestContent(this._onContentAvailable.bind(this, entries[i]));
            } else if (content !== null)
                entries[i].response.content.text = content;
        }
        var compositeProgress = new WebInspector.CompositeProgress(progress);
        this._writeProgress = compositeProgress.createSubProgress();
        if (--this._pendingRequests) {
            this._requestsProgress = compositeProgress.createSubProgress();
            this._requestsProgress.setTitle(WebInspector.UIString("Collecting content…"));
            this._requestsProgress.setTotalWork(this._pendingRequests);
        } else
            this._beginWrite();
    },

    /**
     * @param {Object} entry
     * @param {string|null} content
     * @param {boolean} contentEncoded
     * @param {string=} mimeType
     */
    _onContentAvailable: function(entry, content, contentEncoded, mimeType)
    {
        if (content !== null)
            entry.response.content.text = content;
        if (this._requestsProgress)
            this._requestsProgress.worked();
        if (!--this._pendingRequests) {
            this._requestsProgress.done();
            this._beginWrite();
        }
    },

    _beginWrite: function()
    {
        const jsonIndent = 2;
        this._text = JSON.stringify({log: this._harLog}, null, jsonIndent);
        this._writeProgress.setTitle(WebInspector.UIString("Writing file…"));
        this._writeProgress.setTotalWork(this._text.length);
        this._bytesWritten = 0;
        this._writeNextChunk(this._stream);
    },

    /**
     * @param {WebInspector.OutputStream} stream
     * @param {string=} error
     */
    _writeNextChunk: function(stream, error)
    {
        if (this._bytesWritten >= this._text.length || error) {
            stream.close();
            this._writeProgress.done();
            return;
        }
        const chunkSize = 100000;
        var text = this._text.substring(this._bytesWritten, this._bytesWritten + chunkSize);
        this._bytesWritten += text.length;
        stream.write(text, this._writeNextChunk.bind(this));
        this._writeProgress.setWorked(this._bytesWritten);
    }
}
