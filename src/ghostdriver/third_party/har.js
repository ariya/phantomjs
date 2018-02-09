/**
 * Page object
 * @typedef {Object} PageObject
 * @property {String} title - contents of <title> tag
 * @property {String} url - page URL
 * @property {Date} startTime - time when page starts loading
 * @property {Date} endTime - time when onLoad event fires
 */

/**
 * Resource object
 * @typedef {Object} ResourceObject
 * @property {Object} request - PhantomJS request object
 * @property {Object} startReply - PhantomJS response object
 * @property {Object} endReply - PhantomJS response object
 */

/**
 * This function is based on PhantomJS network logging example:
 * https://github.com/ariya/phantomjs/blob/master/examples/netsniff.js
 *
 * @param {PageObject} page
 * @param {ResourceObject} resources
 * @returns {{log: {version: string, creator: {name: string, version: string}, pages: Array, entries: Array}}}
 */
exports.createHar = function (page, resources) {
    var entries = [];

    resources.forEach(function (resource) {
        var request = resource.request,
            startReply = resource.startReply,
            endReply = resource.endReply,
            error = resource.error;

        if (!request) {
            return;
        }

        // Exclude Data URI from HAR file because
        // they aren't included in specification
        if (request.url.match(/(^data:image\/.*)/i)) {
            return;
        }

        if (error) {
            // according to http://qt-project.org/doc/qt-4.8/qnetworkreply.html
            switch (error.errorCode) {
                case 1:
                    error.errorString = '(refused)';
                    break;
                case 2:
                    error.errorString = '(closed)';
                    break;
                case 3:
                    error.errorString = '(host not found)';
                    break;
                case 4:
                    error.errorString = '(timeout)';
                    break;
                case 5:
                    error.errorString = '(canceled)';
                    break;
                case 6:
                    error.errorString = '(ssl failure)';
                    break;
                case 7:
                    error.errorString = '(net failure)';
                    break;
            }
        }

        if (startReply && endReply) {
            entries.push({
                startedDateTime: request.time.toISOString(),
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
                    status: error ? null : endReply.status,
                    statusText: error ? error.errorString : endReply.statusText,
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
                pageref: page.url
            });
        } else if (error) {
            entries.push({
                startedDateTime: request.time.toISOString(),
                time: 0,
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
                    status: null,
                    statusText: error.errorString,
                    httpVersion: "HTTP/1.1",
                    cookies: [],
                    headers: [],
                    redirectURL: "",
                    headersSize: -1,
                    bodySize: 0,
                    content: {}
                },
                cache: {},
                timings: {
                    blocked: 0,
                    dns: -1,
                    connect: -1,
                    send: 0,
                    wait: 0,
                    receive: 0,
                    ssl: -1
                },
                pageref: page.url
            });
        }
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
                startedDateTime: (page.startTime instanceof Date)
                    ? page.startTime.toISOString() : null,
                id: page.url,
                title: page.title,
                pageTimings: {
                    onLoad: (page.startTime instanceof Date && page.endTime instanceof Date)
                        ? page.endTime.getTime() - page.startTime.getTime() : null
                }
            }],
            entries: entries
        }
    };
};
