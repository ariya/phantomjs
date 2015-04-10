/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

/**
 * @constructor
 * @extends {WebInspector.View}
 * @param {WebInspector.NetworkRequest} request
 */
WebInspector.RequestTimingView = function(request)
{
    WebInspector.View.call(this);
    this.element.addStyleClass("resource-timing-view");

    this._request = request;
}

WebInspector.RequestTimingView.prototype = {
    wasShown: function()
    {
        this._request.addEventListener(WebInspector.NetworkRequest.Events.TimingChanged, this._refresh, this);

        if (!this._request.timing) {
            if (!this._emptyView) {
                this._emptyView = new WebInspector.EmptyView(WebInspector.UIString("This request has no detailed timing info."));
                this._emptyView.show(this.element);
                this.innerView = this._emptyView;
            }
            return;
        }

        if (this._emptyView) {
            this._emptyView.detach();
            delete this._emptyView;
        }

        this._refresh();
    },

    willHide: function()
    {
        this._request.removeEventListener(WebInspector.NetworkRequest.Events.TimingChanged, this._refresh, this);
    },

    _refresh: function()
    {
        if (this._tableElement)
            this._tableElement.parentElement.removeChild(this._tableElement);

        this._tableElement = WebInspector.RequestTimingView.createTimingTable(this._request);
        this.element.appendChild(this._tableElement);
    },

    __proto__: WebInspector.View.prototype
}


WebInspector.RequestTimingView.createTimingTable = function(request)
{
    var tableElement = document.createElement("table");
    var rows = [];

    function addRow(title, className, start, end)
    {
        var row = {};
        row.title = title;
        row.className = className;
        row.start = start;
        row.end = end;
        rows.push(row);
    }

    if (request.timing.proxyStart !== -1)
        addRow(WebInspector.UIString("Proxy"), "proxy", request.timing.proxyStart, request.timing.proxyEnd);

    if (request.timing.dnsStart !== -1)
        addRow(WebInspector.UIString("DNS Lookup"), "dns", request.timing.dnsStart, request.timing.dnsEnd);

    if (request.timing.connectStart !== -1) {
        if (request.connectionReused)
            addRow(WebInspector.UIString("Blocking"), "connecting", request.timing.connectStart, request.timing.connectEnd);
        else {
            var connectStart = request.timing.connectStart;
            // Connection includes DNS, subtract it here.
            if (request.timing.dnsStart !== -1)
                connectStart += request.timing.dnsEnd - request.timing.dnsStart;
            addRow(WebInspector.UIString("Connecting"), "connecting", connectStart, request.timing.connectEnd);
        }
    }

    if (request.timing.sslStart !== -1)
        addRow(WebInspector.UIString("SSL"), "ssl", request.timing.sslStart, request.timing.sslEnd);

    var sendStart = request.timing.sendStart;
    if (request.timing.sslStart !== -1)
        sendStart += request.timing.sslEnd - request.timing.sslStart;

    addRow(WebInspector.UIString("Sending"), "sending", request.timing.sendStart, request.timing.sendEnd);
    addRow(WebInspector.UIString("Waiting"), "waiting", request.timing.sendEnd, request.timing.receiveHeadersEnd);
    addRow(WebInspector.UIString("Receiving"), "receiving", (request.responseReceivedTime - request.timing.requestTime) * 1000, (request.endTime - request.timing.requestTime) * 1000);

    const chartWidth = 200;
    var total = (request.endTime - request.timing.requestTime) * 1000;
    var scale = chartWidth / total;

    for (var i = 0; i < rows.length; ++i) {
        var tr = document.createElement("tr");
        tableElement.appendChild(tr);

        var td = document.createElement("td");
        td.textContent = rows[i].title;
        tr.appendChild(td);

        td = document.createElement("td");
        td.width = chartWidth + "px";

        var row = document.createElement("div");
        row.className = "network-timing-row";
        td.appendChild(row);

        var bar = document.createElement("span");
        bar.className = "network-timing-bar " + rows[i].className;
        bar.style.left = scale * rows[i].start + "px";
        bar.style.right = scale * (total - rows[i].end) + "px";
        bar.style.backgroundColor = rows[i].color;
        bar.textContent = "\u200B"; // Important for 0-time items to have 0 width.
        row.appendChild(bar);

        var title = document.createElement("span");
        title.className = "network-timing-bar-title";
        if (total - rows[i].end < rows[i].start)
            title.style.right = (scale * (total - rows[i].end) + 3) + "px";
        else
            title.style.left = (scale * rows[i].start + 3) + "px";
        title.textContent = Number.secondsToString((rows[i].end - rows[i].start) / 1000);
        row.appendChild(title);

        tr.appendChild(td);
    }
    return tableElement;
}
