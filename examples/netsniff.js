if (!Date.prototype.toISOString) {
    Date.prototype.toISOString = function () {
        function pad(n) { return n < 10 ? '0' + n : n; }
        function ms(n) { return n < 10 ? '00'+ n : n < 100 ? '0' + n : n }
        return this.getFullYear() + '-' +
            pad(this.getMonth() + 1) + '-' +
            pad(this.getDate()) + 'T' +
            pad(this.getHours()) + ':' +
            pad(this.getMinutes()) + ':' +
            pad(this.getSeconds()) + '.' +
            ms(this.getMilliseconds()) + 'Z';
    }
}

function createHAR(address, title, startTime, resources, endTime, dom_element_count)
{
    var entries = [];
    var bodySize = 0;

    resources.forEach(function (resource) {
        var request = resource.request,
            startReply = resource.startReply,
            endReply = resource.endReply;

        if (!request || !startReply || !endReply) {
            return;
        }
        bodySize = bodySize + startReply.bodySize;

        // Exclude Data URI from HAR file because
        // they aren't included in specification
        if (request.url.match(/(^data:image\/.*)/i)) {
            return;
        }

        entries.push({
            startedDateTime: startReply.time.toISOString(),
            time: endReply.time - request.time,
            request: {
                method: request.method,
                url: request.url,
                httpVersion: "HTTP/1.1",
                cookies: [],
                headers: request.headers,
                queryString: [],
                headersSize: -1,
                bodySize: -1
            },
            response: {
                status: endReply.status,
                statusText: endReply.statusText,
                httpVersion: "HTTP/1.1",
                cookies: [],
                headers: endReply.headers,
                redirectURL: "",
                headersSize: -1,
                bodySize: startReply.bodySize,
                content: {
                    size: startReply.bodySize,
                    mimeType: endReply.contentType
                }
            },
            cache: {},
            timings: {
                blocked: 0,
                dns: -1,
                connect: -1,
                send: 0,
                wait: startReply.time - request.time,
                receive: endReply.time - startReply.time,
                ssl: -1
            },
            pageref: address
        });
    });

    return {
        log: {
            version: '1.2',
            creator: {
                name: "PhantomJS",
                version: phantom.version.major + '.' + phantom.version.minor +
                    '.' + phantom.version.patch
            },
            pages: [{
                startedDateTime: startTime.toISOString(),
                endedDateTime: page.endTime.toISOString(),
                id: address,
                size: bodySize,
                resourcesCount: resources.length,
                domElementsCount: dom_element_count,
                title: title,
                jscheck: page.jscheck,
                jscheckout: page.jscheckout,
                pageTimings: {
                    onLoad: page.endTime - page.startTime
                }
            }],
            entries: entries
        }
    };
}

var page = new WebPage(), output;
page.viewportSize = { width: 1600, height: 1200 };

if (phantom.args.length === 0) {
    console.log('Usage: netsniff.js <some URL> <HEADERS>');
    phantom.exit(1);
} else {

    page.address = phantom.args[0];
    if (phantom.args[1]) {
    	page.customHeaders = JSON.parse(phantom.args[1]);
    }
    page.settings.userAgent = 'hggh PhantomJS Webspeed Test';
    page.resources = [];

    page.onLoadStarted = function () {
        page.startTime = new Date();
    };

    page.onResourceRequested = function (req) {
        page.resources[req.id] = {
            request: req,
            startReply: null,
            endReply: null
        };
    };

    page.onResourceReceived = function (res) {
        if (res.stage === 'start') {
            page.resources[res.id].startReply = res;
        }
        if (res.stage === 'end') {
            page.resources[res.id].endReply = res;
        }
    };

    page.onError = function(msg, trace) {
        var msgStack = ['ERROR: ' + msg];
        if (trace && trace.length) {
            msgStack.push('TRACE:');
            trace.forEach(function(t) {
                msgStack.push(' -> ' + t.file + ': ' + t.line + (t.function ? ' (in function "' + t.function + '")' : ''));
            });
        }
    };

    page.onLoadFinished = function (status) {
        var har;
        if (status !== 'success') {
            console.log(
                "Error opening url \"" + page.reason_url
                + "\": " + page.reason
            );
            phantom.exit(1);
        } else {
            page.endTime = new Date();
            page.title = page.evaluate(function () {
                return document.title;
            });
            var dom_element_count = page.evaluate(function (s) {
                return document.getElementsByTagName('*').length;
            });
            page.jscheckout = page.evaluate(function (){return eval(arguments[0]);},page.jscheck);
            har = createHAR(page.address, page.title, page.startTime, page.resources, page.EndTime, dom_element_count);
            console.log(JSON.stringify(har, undefined, 4));
            phantom.exit();
        }
    };
    page.open(page.address);
}
