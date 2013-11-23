function checkClipRect(page, clipRect) {
    expectHasProperty(page, 'clipRect');
    it("should have clipRect with height "+clipRect.height, function () {
        expect(page.clipRect.height).toEqual(clipRect.height);
    });
    it("should have clipRect with left "+clipRect.left, function () {
        expect(page.clipRect.left).toEqual(clipRect.left);
    });
    it("should have clipRect with top "+clipRect.top, function () {
        expect(page.clipRect.top).toEqual(clipRect.top);
    });
    it("should have clipRect with width "+clipRect.width, function () {
        expect(page.clipRect.width).toEqual(clipRect.width);
    });
}

function checkScrollPosition(page, scrollPosition) {
    expectHasProperty(page, 'scrollPosition');
    it("should have scrollPosition with left "+scrollPosition.left, function () {
        expect(page.scrollPosition.left).toEqual(scrollPosition.left);
    });
    it("should have scrollPosition with top "+scrollPosition.top, function () {
        expect(page.scrollPosition.top).toEqual(scrollPosition.top);
    });
}

function checkViewportSize(page, viewportSize) {
    expectHasProperty(page, 'viewportSize');
    it("should have viewportSize with height "+viewportSize.height, function () {
        expect(page.viewportSize.height).toEqual(viewportSize.height);
    });
    it("should have viewportSize with width "+viewportSize.width, function () {
        expect(page.viewportSize.width).toEqual(viewportSize.width);
    });
}

function checkPageCallback(page) {
    it("should pass variables from/to window.callPhantom/page.onCallback", function() {
        var msgA = "a",
            msgB = "b",
            result,
            expected = msgA + msgB;
        page.onCallback = function(a, b) {
            return a + b;
        };
        result = page.evaluate(function(a, b) {
            return callPhantom(a, b);
        }, msgA, msgB);

        expect(result).toEqual(expected);
    });
}

function checkPageConfirm(page) {
    it("should pass result from/to window.confirm/page.onConfirm", function() {
        var msg = "message body",
            result,
            expected = true;
        page.onConfirm = function(msg) {
            return true;
        };
        result = page.evaluate(function(m) {
            return window.confirm(m);
        }, msg);

        expect(result).toEqual(expected);
    });
}

function checkPagePrompt(page) {
    it("should pass result from/to window.prompt/page.onPrompt", function() {
        var msg = "message",
            value = "value",
            result,
            expected = "extra-value";
        page.onPrompt = function(msg, value) {
            return "extra-"+value;
        };
        result = page.evaluate(function(m, v) {
            return window.prompt(m, v);
        }, msg, value);

        expect(result).toEqual(expected);
    });
}

describe("WebPage constructor", function() {
    it("should exist in window", function() {
        expect(window.hasOwnProperty('WebPage')).toBeTruthy();
    });

    it("should be a function", function() {
        expect(typeof window.WebPage).toEqual('function');
    });
});

describe("WebPage object", function() {
    var page = new WebPage();

    it("should be creatable", function() {
        expect(typeof page).toEqual('object');
        expect(page).toNotEqual(null);
    });

    it("should be able to get any signal handler that are currently set on it", function() {
        page.onInitialized = undefined;
        expect(page.onInitialized).toBeUndefined();
        var onInitialized1 = function() { var x = "x"; };
        page.onInitialized = onInitialized1;
        expect(page.onInitialized).toEqual(onInitialized1);
        var onInitialized2 = function() { var y = "y"; };
        page.onInitialized = onInitialized2;
        expect(page.onInitialized).toEqual(onInitialized2);
        expect(page.onInitialized).toNotEqual(onInitialized1);
        page.onInitialized = null;
        // Will only allow setting to a function value, so setting it to `null` returns `undefined`
        expect(page.onInitialized).toBeUndefined();
        page.onInitialized = undefined;
        expect(page.onInitialized).toBeUndefined();
    });

    it("should be able to get any callback handler that are currently set on it", function() {
        page.onConfirm = undefined;
        expect(page.onConfirm).toBeUndefined();
        var onConfirmFunc1 = function() { return !"x"; };
        page.onConfirm = onConfirmFunc1;
        expect(page.onConfirm).toEqual(onConfirmFunc1);
        var onConfirmFunc2 = function() { return !!"y"; };
        page.onConfirm = onConfirmFunc2;
        expect(page.onConfirm).toEqual(onConfirmFunc2);
        expect(page.onConfirm).toNotEqual(onConfirmFunc1);
        page.onConfirm = null;
        // Will only allow setting to a function value, so setting it to `null` returns `undefined`
        expect(page.onConfirm).toBeUndefined();
        page.onConfirm = undefined;
        expect(page.onConfirm).toBeUndefined();
    });

    it("should be able to get the error signal handler that is currently set on it (currently a special 1-off case)", function() {
        page.onError = undefined;
        expect(page.onError).toBeUndefined();
        var onErrorFunc1 = function() { return !"x"; };
        page.onError = onErrorFunc1;
        expect(page.onError).toEqual(onErrorFunc1);
        var onErrorFunc2 = function() { return !!"y"; };
        page.onError = onErrorFunc2;
        expect(page.onError).toEqual(onErrorFunc2);
        expect(page.onError).toNotEqual(onErrorFunc1);
        page.onError = null;
        // Will only allow setting to a function value, so setting it to `null` returns `undefined`
        expect(page.onError).toBeUndefined();
        page.onError = undefined;
        expect(page.onError).toBeUndefined();
    });

    checkPageCallback(page);
    checkPageConfirm(page);
    checkPagePrompt(page);

    checkClipRect(page, {height:0,left:0,top:0,width:0});

    expectHasPropertyString(page, 'content');
    expectHasPropertyString(page, 'plainText');

    expectHasPropertyString(page, 'libraryPath');
    expectHasPropertyString(page, 'offlineStoragePath');
    expectHasProperty(page, 'offlineStorageQuota');

    it("should have objectName as 'WebPage'", function() {
        expect(page.objectName).toEqual('WebPage');
    });

    expectHasProperty(page, 'paperSize');
    it("should have paperSize as an empty object", function() {
        expect(page.paperSize).toEqual({});
    });

    checkScrollPosition(page, {left:0,top:0});

    expectHasProperty(page, 'settings');
    it("should have non-empty settings", function() {
        expect(page.settings).toNotEqual(null);
        expect(page.settings).toNotEqual({});
    });

    expectHasProperty(page, 'customHeaders');
    it("should have customHeaders as an empty object", function() {
        expect(page.customHeaders).toEqual({});
    });

    expectHasProperty(page, 'zoomFactor');
    it("should have zoomFactor of 1", function() {
        expect(page.zoomFactor).toEqual(1.0);
    });

    expectHasProperty(page, 'event');
    expectHasProperty(page, 'cookies');

    checkViewportSize(page, {height:300,width:400});

    expectHasFunction(page, 'deleteLater');
    expectHasFunction(page, 'destroyed');
    expectHasFunction(page, 'evaluate');
    expectHasFunction(page, 'initialized');
    expectHasFunction(page, 'injectJs');
    expectHasFunction(page, 'javaScriptAlertSent');
    expectHasFunction(page, 'javaScriptConsoleMessageSent');
    expectHasFunction(page, 'loadFinished');
    expectHasFunction(page, 'loadStarted');
    expectHasFunction(page, 'openUrl');
    expectHasFunction(page, 'release');
    expectHasFunction(page, 'close');
    expectHasFunction(page, 'render');
    expectHasFunction(page, 'resourceReceived');
    expectHasFunction(page, 'resourceRequested');
    expectHasFunction(page, 'resourceError');
    expectHasFunction(page, 'uploadFile');
    expectHasFunction(page, 'sendEvent');
    expectHasFunction(page, 'childFramesCount');
    expectHasFunction(page, 'childFramesName');
    expectHasFunction(page, 'switchToChildFrame');
    expectHasFunction(page, 'switchToMainFrame');
    expectHasFunction(page, 'switchToParentFrame');
    expectHasFunction(page, 'currentFrameName');
    expectHasFunction(page, 'addCookie');
    expectHasFunction(page, 'deleteCookie');
    expectHasFunction(page, 'clearCookies');
    expectHasFunction(page, 'setContent');

    it("should set content and location", function() {
        runs(function() {
            var expectedContent = "<html><body><div>Test div</div></body></html>";
            var expectedLocation = "http://www.phantomjs.org/";
            page.setContent(expectedContent, expectedLocation);
            var actualLocation = page.evaluate(function(){
                return window.location.href;
            });
            var actualContent = page.evaluate(function(){
                return document.documentElement.textContent;
            });
            expect(expectedLocation).toEqual(actualLocation);
            expect(expectedContent).toContain("Test div");
        });
    });

    it("should handle keydown event", function() {
        runs(function() {
            page.evaluate(function() {
                window.addEventListener('keydown', function(event) {
                    window.loggedEvent = window.loggedEvent || {};
                    window.loggedEvent.keydown = event;
                }, false);
            });
            page.sendEvent('keydown', page.event.key.A);
        });

        waits(50);

        runs(function() {
            var event = page.evaluate(function() {
                return window.loggedEvent.keydown;
            });
            expect(event.which).toEqual(page.event.key.A);
        });
    });

    it("should handle keyup event", function() {
        runs(function() {
            page.evaluate(function() {
                window.addEventListener('keyup', function(event) {
                    window.loggedEvent = window.loggedEvent || {};
                    window.loggedEvent.keyup = event;
                }, false);
            });
            page.sendEvent('keyup', page.event.key.A);
        });

        waits(50);

        runs(function() {
            var event = page.evaluate(function() {
                return window.loggedEvent.keyup;
            });
            expect(event.which).toEqual(page.event.key.A);
        });
    });

    it("should handle keypress event", function() {
        runs(function() {
            page.evaluate(function() {
                window.addEventListener('keypress', function(event) {
                    window.loggedEvent = window.loggedEvent || {};
                    window.loggedEvent.keypress = event;
                }, false);
            });
            page.sendEvent('keypress', page.event.key.A);
        });

        waits(50);

        runs(function() {
            var event = page.evaluate(function() {
                return window.loggedEvent.keypress;
            });
            expect(event.which).toEqual(page.event.key.A);
        });
    });

    it("should handle keypress event with inputs", function() {
        runs(function() {
            page.content = '<input type="text">';
            page.evaluate(function() {
                document.querySelector('input').focus();
            });
            var getText = function() {
                return page.evaluate(function() {
                    return document.querySelector('input').value;
                });
            };
            page.sendEvent('keypress', page.event.key.A);
            expect(getText()).toEqual("A");
            page.sendEvent('keypress', page.event.key.B);
            expect(getText()).toEqual("AB");
            page.sendEvent('keypress', page.event.key.Backspace);
            expect(getText()).toEqual("A");
        });
    });

    it("should handle keypress event of string with inputs", function() {
        runs(function() {
            page.content = '<input type="text">';
            page.evaluate(function() {
                document.querySelector('input').focus();
            });
            page.sendEvent('keypress', "ABCD");
            // 0x02000000 is the Shift modifier.
            page.sendEvent('keypress', page.event.key.Home, null, null,  0x02000000);
            page.sendEvent('keypress', page.event.key.Delete);
            var text = page.evaluate(function() {
                return document.querySelector('input').value;
            });
            expect(text).toEqual("");
        });
    });

    it("should handle key events with modifier keys", function() {
        runs(function() {
            page.content = '<input type="text">';
            page.evaluate(function() {
                document.querySelector('input').focus();
            });
            page.sendEvent('keypress', "ABCD");
            var text = page.evaluate(function() {
                return document.querySelector('input').value;
            });
            expect(text).toEqual("ABCD");
        });
    });

    it("should send proper key codes for text", function () {
        runs(function() {
            page.content = '<input type="text">';
            page.evaluate(function() {
                document.querySelector('input').focus();
            });
            page.sendEvent('keypress', "ABCD");
            // 0x02000000 is the Shift modifier.
            page.sendEvent('keypress', page.event.key.Home, null, null,  0x02000000);
            // 0x04000000 is the Control modifier.
            page.sendEvent('keypress', 'x', null, null, 0x04000000);
            var text = page.evaluate(function() {
                return document.querySelector('input').value;
            });
            expect(text).toEqual("");
            page.sendEvent('keypress', 'v', null, null, 0x04000000);
            text = page.evaluate(function() {
                return document.querySelector('input').value;
            });
            expect(text).toEqual("ABCD");
        });
    });

    it("should handle keypress event of umlaut char with inputs", function() {
        runs(function() {
            page.content = '<input type="text">';
            page.evaluate(function() {
                document.querySelector('input').focus();
            });
            page.sendEvent('keypress', "ä");
            var text = page.evaluate(function() {
                return document.querySelector('input').value;
            });
            expect(text).toEqual("ä");
        });
    });

    it("should handle mousedown event", function() {
        runs(function() {
            page.evaluate(function() {
                window.addEventListener('mousedown', function(event) {
                    window.loggedEvent = window.loggedEvent || {};
                    window.loggedEvent.mousedown = event;
                }, false);
            });
            page.sendEvent('mousedown', 42, 217);
        });

        runs(function() {
            var event = page.evaluate(function() {
                return window.loggedEvent.mousedown;
            });
            expect(event.clientX).toEqual(42);
            expect(event.clientY).toEqual(217);
        });
    });

    it("should handle mouseup event", function() {
        runs(function() {
            page.evaluate(function() {
                window.addEventListener('mouseup', function(event) {
                    window.loggedEvent = window.loggedEvent || {};
                    window.loggedEvent.mouseup = event;
                }, false);
            });
            page.sendEvent('mouseup', 14, 3);
        });

        waits(50);

        runs(function() {
            var event = page.evaluate(function() {
                return window.loggedEvent.mouseup;
            });
            expect(event.clientX).toEqual(14);
            expect(event.clientY).toEqual(3);
        });
    });

    it("should handle mousemove event", function() {
        runs(function() {
            page.evaluate(function() {
                window.addEventListener('mousemove', function(event) {
                    window.loggedEvent = window.loggedEvent || {};
                    window.loggedEvent.mousemove = event;
                }, false);
            });
            page.sendEvent('mousemove', 14, 7);
        });

        waits(50);

        runs(function() {
            var event = page.evaluate(function() {
                return window.loggedEvent.mousemove;
            });
            expect(event.clientX).toEqual(14);
            expect(event.clientY).toEqual(7);
        });
    });


    it("should handle click event", function() {
        runs(function() {
            page.evaluate(function() {
                window.addEventListener('mousedown', function(event) {
                    window.loggedEvent = window.loggedEvent || {};
                    window.loggedEvent.mousedown = event;
                }, false);
                window.addEventListener('mouseup', function(event) {
                    window.loggedEvent = window.loggedEvent || {};
                    window.loggedEvent.mouseup = event;
                }, false);
            });
            page.sendEvent('click', 42, 217);
        });

        waits(50);

        runs(function() {
            var event = page.evaluate(function() {
                return window.loggedEvent;
            });
            expect(event.mouseup.clientX).toEqual(42);
            expect(event.mouseup.clientY).toEqual(217);
            expect(event.mousedown.clientX).toEqual(42);
            expect(event.mousedown.clientY).toEqual(217);
        });
    });

    it("should handle doubleclick event", function () {
        runs(function () {
            page.content = '<input id="doubleClickField" type="text" onclick="document.getElementById(\'doubleClickField\').value=\'clicked\';" ondblclick="document.getElementById(\'doubleClickField\').value=\'doubleclicked\';" oncontextmenu="document.getElementById(\'doubleClickField\').value=\'rightclicked\'; return false;" value="hello"/>';
            var point = page.evaluate(function () {
                var el = document.querySelector('input');
                var rect = el.getBoundingClientRect();
                return { x: rect.left + Math.floor(rect.width / 2), y: rect.top + (rect.height / 2) };
            });
            page.sendEvent('doubleclick', point.x, point.y);
        });

        waits(50);

        runs(function () {
            var text = page.evaluate(function () {
                return document.querySelector('input').value;
            });
            expect(text).toEqual("doubleclicked");
        });
    });

    it("should handle file uploads", function() {
        runs(function() {
            page.content = '<input type="file" id="file">\n' +
                           '<input type="file" id="file2" multiple>\n' +
                           '<input type="file" id="file3" multiple>';
            page.uploadFile("#file", "run-tests.js");
            page.uploadFile("#file2", "run-tests.js");
            page.uploadFile("#file3", ["run-tests.js", "webpage-spec.js"]);
        });

        waits(50);

        runs(function() {
            var fileName;

            fileName = page.evaluate(function() {
                return document.getElementById('file').files[0].fileName;
            });
            expect(fileName).toEqual("run-tests.js");

            fileName = page.evaluate(function() {
                return document.getElementById('file2').files[0].fileName;
            });
            expect(fileName).toEqual("run-tests.js");

            var files = page.evaluate(function() {
                var files = document.getElementById('file3').files;
                return {
                    length: files.length,
                    fileNames: [files[0].fileName, files[1].fileName]
                }
            });
            expect(files.length).toEqual(2)
            expect(files.fileNames[0]).toEqual("run-tests.js");
            expect(files.fileNames[1]).toEqual("webpage-spec.js");
        });
    });

    it("should support console.log with multiple arguments", function() {
        var message;
        runs(function() {
            page.onConsoleMessage = function (msg) {
                message = msg;
            };
        });

        waits(50);

        runs(function() {
            page.evaluate(function () { console.log('answer', 42); });
            expect(message).toEqual("answer 42");
        });
    });

    it("should not load any NPAPI plugins (e.g. Flash)", function() {
        runs(function() {
            expect(page.evaluate(function () { return window.navigator.plugins.length; })).toEqual(0);
        });
    });

    it("reports unhandled errors", function() {
        var lastError = null;

        var page = new require('webpage').create();
        page.onError = function(message) { lastError = message; };

        runs(function() {
            page.evaluate(function() {
                setTimeout(function() { referenceError(); }, 0);
            });
        });

        waits(0);

        runs(function() {
            expect(lastError).toEqual("ReferenceError: Can't find variable: referenceError");

            page.evaluate(function() { referenceError2(); });
            expect(lastError).toEqual("ReferenceError: Can't find variable: referenceError2");

            page.evaluate(function() { throw "foo"; });
            expect(lastError).toEqual("foo");

            page.evaluate(function() { throw Error("foo"); });
            expect(lastError).toEqual("Error: foo");
        });
    });

    it("doesn't report handled errors", function() {
        var hadError    = false;
        var caughtError = false;
        var page        = require('webpage').create();

        runs(function() {
            page.onError = function() { hadError = true; };
            page.evaluate(function() {
                caughtError = false;

                try {
                    referenceError();
                } catch(e) {
                    caughtError = true;
                }
            });

            expect(hadError).toEqual(false);
            expect(page.evaluate(function() { return caughtError; })).toEqual(true);
        });
    });

    it("reports the sourceURL and line of errors", function() {
        runs(function() {
            var e1, e2;

            try {
                referenceError();
            } catch (e) {
                e1 = e;
            }

            try {
                referenceError();
            } catch (e) {
                e2 = e;
            }

            expect(e1.sourceURL).toMatch(/webpage-spec.js$/);
            expect(e1.line).toBeGreaterThan(1);
            expect(e2.line).toEqual(e1.line + 6);
        });
    });

    it("reports the stack of errors", function() {
        var helperFile = "./fixtures/error-helper.js";
        phantom.injectJs(helperFile);

        var page = require('webpage').create(), stack;

        runs(function() {
            function test() {
                ErrorHelper.foo();
            }

            var err;
            try {
                test();
            } catch (e) {
                err = e;
            }

            var lines = err.stack.split("\n");

            expect(lines[0]).toEqual("ReferenceError: Can't find variable: referenceError");
            expect(lines[1]).toEqual("    at bar (./fixtures/error-helper.js:7)");
            expect(lines[2]).toEqual("    at ./fixtures/error-helper.js:3");
            expect(lines[3]).toMatch(/    at test \(\.\/webpage-spec\.js:\d+\)/);

            page.injectJs(helperFile);

            page.onError = function(message, s) { stack = s; };
            page.evaluate(function() { setTimeout(function() { ErrorHelper.foo(); }, 0); });
        });

        waits(0);

        runs(function() {
            expect(stack[0].file).toEqual("./fixtures/error-helper.js");
            expect(stack[0].line).toEqual(7);
            expect(stack[0]["function"]).toEqual("bar");
        });
    });

    it("reports errors that occur in the main context", function() {
        var error;
        phantom.onError = function(e) { error = e; };

        runs(function() {
            setTimeout(function() { zomg(); }, 0);
        });

        waits(0);

        runs(function() {
            expect(error.toString()).toEqual("ReferenceError: Can't find variable: zomg");
            phantom.onError = phantom.defaultErrorHandler;
        });
    });

    it("should set custom headers properly", function() {
        var server = require('webserver').create();
        server.listen(12345, function(request, response) {
            // echo received request headers in response body
            response.write(JSON.stringify(request.headers));
            response.close();
        });

        var url = "http://localhost:12345/foo/headers.txt?ab=cd";
        var customHeaders = {
            "Custom-Key" : "Custom-Value",
            "User-Agent" : "Overriden-UA",
            "Referer" : "Overriden-Referer"
        };
        page.customHeaders = customHeaders;

        var handled = false;
        runs(function() {
            expect(handled).toEqual(false);
            page.open(url, function (status) {
                expect(status == 'success').toEqual(true);
                handled = true;

                var echoedHeaders = JSON.parse(page.plainText);
                // console.log(JSON.stringify(echoedHeaders, null, 4));
                // console.log(JSON.stringify(customHeaders, null, 4));

                expect(echoedHeaders["Custom-Key"]).toEqual(customHeaders["Custom-Key"]);
                expect(echoedHeaders["User-Agent"]).toEqual(customHeaders["User-Agent"]);
                expect(echoedHeaders["Referer"]).toEqual(customHeaders["Referer"]);

            });
        });

        waits(50);

        runs(function() {
            expect(handled).toEqual(true);
            server.close();
        });

    });

    it("should process request body properly for POST", function() {
        var server = require('webserver').create();
        server.listen(12345, function(request, response) {
            // echo received request body in response body;
            response.setHeader('Content-Type', 'text/html; charset=utf-8');
            response.write(Object.keys(request.post)[0]);
            response.close();
        });

        var url = "http://localhost:12345/foo/body";

        var handled = false;
        runs(function() {
            expect(handled).toEqual(false);
            var utfString = '안녕';
            var openOptions = {
                operation: 'POST',
                data:       utfString,
                encoding:  'utf8'
            };
            var pageOptions = {
                onLoadFinished: function(status) {
                    expect(status == 'success').toEqual(true);
                    handled = true;

                    expect(page.plainText).toEqual(utfString);
                }
            };

            var page = new WebPage(pageOptions);

            page.openUrl(url, openOptions, {});
        });

        waits(50);

        runs(function() {
            expect(handled).toEqual(true);
            server.close();
        });

    });

    it("should return properly from a 401 status", function() {
        var server = require('webserver').create();
        server.listen(12345, function(request, response) {
            response.statusCode = 401;
            response.setHeader('WWW-Authenticate', 'Basic realm="PhantomJS test"');
            response.write('Authentication Required');
            response.close();
        });

        var url = "http://localhost:12345/foo";
        var handled = 0;
        runs(function() {
            expect(handled).toEqual(0);
            page.onResourceReceived = function(resource) {
                expect(resource.status).toEqual(401);
                handled++;
            };
            page.open(url, function(status) {
                expect(status).toEqual('fail');
                handled++;
            });
        });

        waits(50);

        runs(function() {
            expect(handled).toEqual(2);
            page.onResourceReceived = null;
            server.close();
        });

    });

    it("should set valid cookie properly, then remove it", function() {
        var server = require('webserver').create();
        server.listen(12345, function(request, response) {
            // echo received request headers in response body
            response.write(JSON.stringify(request.headers));
            response.close();
        });

        var url = "http://localhost:12345/foo/headers.txt?ab=cd";

        page.cookies = [{
            'name' : 'Valid-Cookie-Name',
            'value' : 'Valid-Cookie-Value',
            'domain' : 'localhost',
            'path' : '/foo',
            'httponly' : true,
            'secure' : false
        },{
            'name' : 'Valid-Cookie-Name-Sec',
            'value' : 'Valid-Cookie-Value-Sec',
            'domain' : 'localhost',
            'path' : '/foo',
            'httponly' : true,
            'secure' : false,
            'expires' : new Date().getTime() + 3600 //< expires in 1h
        }];

        var handled = false;
        runs(function() {
            expect(handled).toEqual(false);
            page.open(url, function (status) {
                expect(status == 'success').toEqual(true);
                handled = true;

                var echoedHeaders = JSON.parse(page.plainText);
                // console.log(JSON.stringify(echoedHeaders));
                expect(echoedHeaders["Cookie"]).toContain("Valid-Cookie-Name");
                expect(echoedHeaders["Cookie"]).toContain("Valid-Cookie-Value");
                expect(echoedHeaders["Cookie"]).toContain("Valid-Cookie-Name-Sec");
                expect(echoedHeaders["Cookie"]).toContain("Valid-Cookie-Value-Sec");
            });
        });

        waits(50);

        runs(function() {
            expect(handled).toEqual(true);
            expect(page.cookies.length).toNotBe(0);
            page.cookies = []; //< delete all the cookies visible to this URL
            expect(page.cookies.length).toBe(0);
            server.close();
        });
    });

    it("should not set invalid cookies", function() {
        var server = require('webserver').create();
        server.listen(12345, function(request, response) {
            // echo received request headers in response body
            response.write(JSON.stringify(request.headers));
            response.close();
        });

        var url = "http://localhost:12345/foo/headers.txt?ab=cd";

        page.cookies = [
        { // domain mismatch.
            'name' : 'Invalid-Cookie-Name',
            'value' : 'Invalid-Cookie-Value',
            'domain' : 'foo.com'
        },{ // path mismatch: the cookie will be set,
            // but won't be visible from the given URL (not same path).
            'name' : 'Invalid-Cookie-Name',
            'value' : 'Invalid-Cookie-Value',
            'domain' : 'localhost',
            'path' : '/bar'
        },{ // cookie expired.
            'name' : 'Invalid-Cookie-Name',
            'value' : 'Invalid-Cookie-Value',
            'domain' : 'localhost',
            'expires' : 'Sat, 09 Jun 2012 00:00:00 GMT'
        },{ // https only: the cookie will be set,
            // but won't be visible from the given URL (not https).
            'name' : 'Invalid-Cookie-Name',
            'value' : 'Invalid-Cookie-Value',
            'domain' : 'localhost',
            'secure' : true
        },{ // cookie expired (date in "sec since epoch").
            'name' : 'Invalid-Cookie-Name',
            'value' : 'Invalid-Cookie-Value',
            'domain' : 'localhost',
            'expires' : new Date().getTime() - 1 //< date in the past
        },{ // cookie expired (date in "sec since epoch" - using "expiry").
            'name' : 'Invalid-Cookie-Name',
            'value' : 'Invalid-Cookie-Value',
            'domain' : 'localhost',
            'expiry' : new Date().getTime() - 1 //< date in the past
        }];

        var handled = false;
        runs(function() {
            expect(handled).toEqual(false);
            page.open(url, function (status) {
                expect(status == 'success').toEqual(true);
                handled = true;

                var echoedHeaders = JSON.parse(page.plainText);
                // console.log(JSON.stringify(echoedHeaders));
                expect(echoedHeaders["Cookie"]).toBeUndefined();
            });
        });

        waits(50);

        runs(function() {
            expect(handled).toEqual(true);
            expect(page.cookies.length).toBe(0);
            page.clearCookies(); //< delete all the cookies visible to this URL
            expect(page.cookies.length).toBe(0);
            server.close();
        });
    });

    it("should add a cookie", function() {
        var server = require('webserver').create();
        server.listen(12345, function(request, response) {
            // echo received request headers in response body
            response.write(JSON.stringify(request.headers));
            response.close();
        });

        var url = "http://localhost:12345/foo/headers.txt?ab=cd";

        page.addCookie({
            'name' : 'Added-Cookie-Name',
            'value' : 'Added-Cookie-Value',
            'domain' : 'localhost'
        });

        var handled = false;
        runs(function() {
            expect(handled).toEqual(false);
            page.open(url, function (status) {
                expect(status == 'success').toEqual(true);
                handled = true;

                var echoedHeaders = JSON.parse(page.plainText);
                // console.log(JSON.stringify(echoedHeaders));
                expect(echoedHeaders["Cookie"]).toContain("Added-Cookie-Name");
                expect(echoedHeaders["Cookie"]).toContain("Added-Cookie-Value");
            });
        });

        waits(50);

        runs(function() {
            expect(handled).toEqual(true);
            server.close();
        });
    });

    it("should delete a cookie", function() {
        var server = require('webserver').create();
        server.listen(12345, function(request, response) {
            // echo received request headers in response body
            response.write(JSON.stringify(request.headers));
            response.close();
        });

        var url = "http://localhost:12345/foo/headers.txt?ab=cd";

        page.deleteCookie("Added-Cookie-Name");

        var handled = false;
        runs(function() {
            expect(handled).toEqual(false);
            page.open(url, function (status) {
                expect(status == 'success').toEqual(true);
                handled = true;

                var echoedHeaders = JSON.parse(page.plainText);
                // console.log(JSON.stringify(echoedHeaders));
                expect(echoedHeaders["Cookie"]).toBeUndefined();
            });
        });

        waits(50);

        runs(function() {
            expect(handled).toEqual(true);
            server.close();
        });
    });

    it("should pass variables to functions properly", function() {
        var testPrimitiveArgs = function() {
            var samples = [
                true,
                0,
                "`~!@#$%^&*()_+-=[]\\{}|;':\",./<>?",
                undefined,
                null
            ];
            for (var i = 0; i < samples.length; i++) {
                if (samples[i] !== arguments[i]) {
                    console.log("FAIL");
                }
            }
        };

        var testComplexArgs = function() {
            var samples = [
                {a:true, b:0, c:"string"},
                function() { return true; },
                [true, 0, "string"],
                /\d+\w*\//
            ];
            for (var i = 0; i < samples.length; i++) {
                if (typeof samples[i] !== typeof arguments[i] ||
                    samples[i].toString() !== arguments[i].toString()) {
                    console.log("FAIL");
                }
            }
        };

        var message;
        runs(function() {
            page.onConsoleMessage = function (msg) {
                message = msg;
            };
        });

        waits(0);

        runs(function() {
            page.evaluate(function() {
                console.log("PASS");
            });
            page.evaluate(testPrimitiveArgs,
                true,
                0,
                "`~!@#$%^&*()_+-=[]\\{}|;':\",./<>?",
                undefined,
                null);
            page.evaluate(testComplexArgs,
                {a:true, b:0, c:"string"},
                function() { return true; },
                [true, 0, "string"],
                /\d+\w*\//);
            expect(message).toEqual("PASS");
        });
    });

    it('should open url using secure connection', function() {
        var page = require('webpage').create();
        var url = 'https://www.google.com/m';

        var loaded = false, handled = false;

        runs(function() {
            page.open(url, function(status) {
                loaded = true;
                expect(status == 'success').toEqual(true);
                handled = true;
            });
        });

        waitsFor(function () {
            return loaded;
        }, 'Can not load ' + url, 3000);

        runs(function() {
            expect(handled).toEqual(true);
        });
    });

    it('should handle resource request errors', function() {
        var server = require('webserver').create();
        var page = require('webpage').create();

        server.listen(12345, function(request, response) {
            if (request.url == '/notExistResource.png') {
                response.statusCode = 404;
                response.write('Not found!');
                response.close();
            } else {
                response.statusCode = 200;
                response.write('<html><body><img src="notExistResource.png"/></body></html>');
                response.close();
            }
        });

        var handled = false;

        runs(function() {
            page.onResourceError = function(errorData) {
                expect(errorData['url']).toEqual('http://localhost:12345/notExistResource.png');
                expect(errorData['errorCode']).toEqual(203);
                expect(errorData['errorString']).toContain('notExistResource.png - server replied: Not Found');
                handled = true;
            };

            page.open('http://localhost:12345', function(status) {
                expect(status).toEqual('success');
            });
        });

        waits(5000);

        runs(function() {
            expect(handled).toEqual(true);
            page.close();
            server.close();
        });
    });


    it("should change a url request with an encoded query string", function() {
        var page = new require('webpage').create();

        var server = require('webserver').create();
        server.listen(12345, function(request, response) {
            // echo received request headers in response body
            response.write(JSON.stringify(request.headers));
            response.close();
        });

        var url = "http://localhost:12345/cdn-cgi/pe/bag?r%5B%5D=http%3A%2F%2Fwww.example.org%2Fcdn-cgi%2Fnexp%2Fabv%3D927102467%2Fapps%2Fabetterbrowser.js";

        var handled = false;
        runs(function() {
            expect(handled).toEqual(false);

            page.onResourceRequested = function(requestData, request) {
                request.changeUrl(requestData.url);
            };

           page.onResourceReceived = function(data) {
                if (data['stage'] === 'end') {
                    expect(data.url).toEqual(url);
                }
           };

           page.open(url, function (status) {
                expect(status == 'success').toEqual(true);
                handled = true;
                server.close();
            });
        });
    });


    it('should able to abort a network request', function() {
        var page = require('webpage').create();
        var url = 'http://phantomjs.org';
        var urlToBlock = 'http://phantomjs.org/images/phantomjs-logo.png';

        var handled = false;

        runs(function() {
            page.onResourceRequested = function(requestData, request) {
                if (requestData['url'] == urlToBlock) {
                    expect(typeof request).toEqual('object');
                    expect(typeof request.abort).toEqual('function');
                    request.abort();
                    handled = true;
                }
            };

            page.open(url, function(status) {
                expect(status).toEqual('success');
            });
        });
        
        waits(5000);
        
        runs(function() {
            page.close();
            expect(handled).toBeTruthy();
        });
    });

    it('should fail on secure connection to url with bad cert', function() {
        var page = require('webpage').create();
        var url = 'https://tv.eurosport.com/';
        /* example from:
         * https://onlinessl.netlock.hu/en/test-center/bad-ssl-certificate-usage.html
         */

        var handled = false;

        runs(function() {
            page.open(url, function(status) {
                expect(status == 'success').toEqual(false);
                handled = true;
            });
        });

        waits(3000);

        runs(function() {
            expect(handled).toEqual(true);
        });
    });

    it('should change the url of the request', function() {
        var page = require('webpage').create();
        var url = 'http://phantomjs.org';
        var urlToChange = 'http://phantomjs.org/images/phantomjs-logo.png';
        var fakeImageUrl = 'http://phantomjs.org/images/icon-release.png';

        var handled = false;

        runs(function() {
            page.onResourceRequested = function(requestData, request) {
                if (requestData['url'] == urlToChange) {
                    expect(typeof request).toEqual('object');
                    expect(typeof request.changeUrl).toEqual('function');
                    request.changeUrl(fakeImageUrl);
                }
            };

            page.onResourceReceived = function(data) {
                if (data['stage'] === 'end' && data['url'] == fakeImageUrl) {
                    handled = true;
                }
            };

            page.open(url, function(status) {
                expect(status).toEqual('success');
            });
        });

        waits(3000);

        runs(function() {
            expect(handled).toBe(true);
        });
    });
    
    it('should fire `onResourceReceived` callback when the resource error occured', function() {
        var page = require('webpage').create();
        var server = require('webserver').create();
        var service = server.listen(12345, function (request, response) {
            var code = parseInt(/^\/(\d+)$/.exec(request.url)[1], 10);
            response.statusCode = code;
            response.write("how");
            response.close();
        });
        var handled = 0;

        runs(function() {
            page.onResourceReceived = function(res) {
                handled++;
            };

            page.open('http://localhost:12345/400', function() {
                server.close();
            });
        });

        waits(5000);

        runs(function() {
            expect(handled).toEqual(2);
            page.close();
            server.close();
        });
    });
});

describe("WebPage construction with options", function () {
    it("should accept an opts object", function() {
        var opts = {},
            page = new WebPage(opts);
        expect(typeof page).toEqual('object');
        expect(page).toNotEqual(null);
    });

    describe("specifying clipRect", function() {
        var opts = {
            clipRect: {
                height: 100,
                left: 10,
                top: 20,
                width: 200
            }
        };
        checkClipRect(new WebPage(opts), opts.clipRect);
    });

    describe("specifying onConsoleMessage", function() {
        var message = false,
            opts = {
                onConsoleMessage: function (msg) {
                    message = msg;
                }
            };
        var page = new WebPage(opts);
        it("should have onConsoleMessage that was specified",function () {
            page.evaluate("function () {console.log('test log')}");
            expect(message).toEqual("test log");
        });
    });

    describe("specifying onLoadStarted", function() {
        var started = false,
            opts = {
                onLoadStarted: function (status) {
                    started = true;
                }
            };
        var page = new WebPage(opts);
        it("should have onLoadStarted that was specified",function () {
            runs(function() {
                expect(started).toEqual(false);
                page.open("about:blank");
            });

            waits(0);

            runs(function() {
                expect(started).toEqual(true);
            });
        });
    });

    describe("specifying onLoadFinished", function() {
        var finished = false,
            opts = {
                onLoadFinished: function (status) {
                    finished = true;
                }
            };
        var page = new WebPage(opts);
        it("should have onLoadFinished that was specified",function () {
            runs(function() {
                expect(finished).toEqual(false);
                page.open("about:blank");
            });

            waits(0);

            runs(function() {
                expect(finished).toEqual(true);
            });
        });
    });

    describe("specifying scrollPosition", function () {
        var opts = {
            scrollPosition: {
                left: 1,
                top: 2
            }
        };
        checkScrollPosition(new WebPage(opts), opts.scrollPosition);
    });

    describe("specifying timeout", function () {
        var opts = {
            settings: {
                timeout: 100 // time in ms
            }
        };
        var page = new WebPage(opts);
        it("should have timeout as "+opts.settings.timeout,function () {
            expect(page.settings.timeout).toEqual(opts.settings.timeout);
        });
    });

    describe("specifying userAgent", function () {
        var opts = {
            settings: {
                userAgent: "PHANTOMJS-TEST-USER-AGENT"
            }
        };
        var page = new WebPage(opts);
        it("should have userAgent as '"+opts.settings.userAgent+"'",function () {
            expect(page.settings.userAgent).toEqual(opts.settings.userAgent);
        });
    });

    describe("specifying viewportSize", function () {
        var opts = {
            viewportSize: {
                height: 100,
                width: 200
            }
        };
        checkViewportSize(new WebPage(opts), opts.viewportSize);
    });
});

describe("WebPage switch frame of execution (deprecated API)", function(){
    var p = require("webpage").create();

    function pageTitle(page) {
        return page.evaluate(function(){
            return window.document.title;
        });
    }

    function setPageTitle(page, newTitle) {
        page.evaluate(function(newTitle){
            window.document.title = newTitle;
        }, newTitle);
    }

    it("should load a page full of frames", function(){
        runs(function() {
            p.open("../test/webpage-spec-frames/index.html");
        });
        waits(50);
    });

    it("should be able to detect frames at level 0", function(){
        expect(pageTitle(p)).toEqual("index");
        expect(p.currentFrameName()).toEqual("");
        expect(p.childFramesCount()).toEqual(2);
        expect(p.childFramesName()).toEqual(["frame1", "frame2"]);
        setPageTitle(p, pageTitle(p) + "-visited");
    });

    it("should go down to a child frame at level 1", function(){
        expect(p.switchToChildFrame("frame1")).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame1");
        expect(p.currentFrameName()).toEqual("frame1");
        expect(p.childFramesCount()).toEqual(2);
        expect(p.childFramesName()).toEqual(["frame1-1", "frame1-2"]);
        setPageTitle(p, pageTitle(p) + "-visited");
    });

    it("should go down to a child frame at level 2", function(){
        expect(p.switchToChildFrame("frame1-2")).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame1-2");
        expect(p.currentFrameName()).toEqual("frame1-2");
        expect(p.childFramesCount()).toEqual(0);
        expect(p.childFramesName()).toEqual([]);
        setPageTitle(p, pageTitle(p) + "-visited");
    });

    it("should go up to the parent frame at level 1", function(){
        expect(p.switchToParentFrame()).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame1-visited");
        expect(p.currentFrameName()).toEqual("frame1");
        expect(p.childFramesCount()).toEqual(2);
        expect(p.childFramesName()).toEqual(["frame1-1", "frame1-2"]);
    });

    it("should go down to a child frame at level 2 (again)", function(){
        expect(p.switchToChildFrame(0)).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame1-1");
        expect(p.currentFrameName()).toEqual("frame1-1");
        expect(p.childFramesCount()).toEqual(0);
        expect(p.childFramesName()).toEqual([]);
    });

    it("should go up to the main (top) frame at level 0", function(){
        expect(p.switchToMainFrame()).toBeUndefined();
        expect(pageTitle(p)).toEqual("index-visited");
        expect(p.currentFrameName()).toEqual("");
        expect(p.childFramesCount()).toEqual(2);
        expect(p.childFramesName()).toEqual(["frame1", "frame2"]);
    });

    it("should go down to (the other) child frame at level 1", function(){
        expect(p.switchToChildFrame("frame2")).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame2");
        expect(p.currentFrameName()).toEqual("frame2");
        expect(p.childFramesCount()).toEqual(3);
        expect(p.childFramesName()).toEqual(["frame2-1", "frame2-2", "frame2-3"]);
    });
});

describe("WebPage switch frame of execution", function(){
    var p = require("webpage").create();

    function pageTitle(page) {
        return page.evaluate(function(){
            return window.document.title;
        });
    }

    function setPageTitle(page, newTitle) {
        page.evaluate(function(newTitle){
            window.document.title = newTitle;
        }, newTitle);
    }

    it("should load a page full of frames", function(){
        runs(function() {
            p.open("../test/webpage-spec-frames/index.html");
        });
        waits(50);
    });

    it("should be able to detect frames at level 0", function(){
        expect(pageTitle(p)).toEqual("index");
        expect(p.frameName).toEqual("");
        expect(p.framesCount).toEqual(2);
        expect(p.framesName).toEqual(["frame1", "frame2"]);
        setPageTitle(p, pageTitle(p) + "-visited");
    });

    it("should go down to a child frame at level 1", function(){
        expect(p.switchToFrame("frame1")).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame1");
        expect(p.frameName).toEqual("frame1");
        expect(p.framesCount).toEqual(2);
        expect(p.framesName).toEqual(["frame1-1", "frame1-2"]);
        setPageTitle(p, pageTitle(p) + "-visited");
    });

    it("should go down to a child frame at level 2", function(){
        expect(p.switchToFrame("frame1-2")).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame1-2");
        expect(p.frameName).toEqual("frame1-2");
        expect(p.framesCount).toEqual(0);
        expect(p.framesName).toEqual([]);
        setPageTitle(p, pageTitle(p) + "-visited");
    });

    it("should go up to the parent frame at level 1", function(){
        expect(p.switchToParentFrame()).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame1-visited");
        expect(p.frameName).toEqual("frame1");
        expect(p.framesCount).toEqual(2);
        expect(p.framesName).toEqual(["frame1-1", "frame1-2"]);
    });

    it("should go down to a child frame at level 2 (again)", function(){
        expect(p.switchToFrame(0)).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame1-1");
        expect(p.frameName).toEqual("frame1-1");
        expect(p.framesCount).toEqual(0);
        expect(p.framesName).toEqual([]);
    });

    it("should go up to the main (top) frame at level 0", function(){
        expect(p.switchToMainFrame()).toBeUndefined();
        expect(pageTitle(p)).toEqual("index-visited");
        expect(p.frameName).toEqual("");
        expect(p.framesCount).toEqual(2);
        expect(p.framesName).toEqual(["frame1", "frame2"]);
    });

    it("should go down to (the other) child frame at level 1", function(){
        expect(p.switchToFrame("frame2")).toBeTruthy();
        expect(pageTitle(p)).toEqual("frame2");
        expect(p.frameName).toEqual("frame2");
        expect(p.framesCount).toEqual(3);
        expect(p.framesName).toEqual(["frame2-1", "frame2-2", "frame2-3"]);
    });

    it("should have top level as focused frame", function(){
        expect(p.focusedFrameName).toEqual("");
    });

    it("should move focus to level 1 frame", function(){
        p.evaluate(function(){
            window.focus();
        });
        expect(p.focusedFrameName).toEqual("frame2");
    });

    it("should move focus to level 2 frame", function(){
        expect(p.switchToFrame("frame2-1")).toBeTruthy();
        p.evaluate(function(){
            window.focus();
        });
        expect(p.focusedFrameName).toEqual("frame2-1");
    });

    it("should move focus back to main frame", function(){
        expect(p.switchToMainFrame()).toBeUndefined();
        p.evaluate(function(){
            window.focus();
        });
        expect(p.focusedFrameName).toEqual("");
    });

    it("should maintain focus but move current frame", function(){
        p.evaluate(function(){
            window.frames[0].focus();
        });
        expect(p.focusedFrameName).toEqual("frame1");
        expect(p.frameName).toEqual("");
    });

    it("should change current frame to focused frame", function(){
        expect(p.switchToFocusedFrame()).toBeUndefined();
        expect(p.frameName).toEqual("frame1");
    });
});

describe("WebPage opening and closing of windows/child-pages", function(){
    var p = require("webpage").create();

    it("should call 'onPageCreated' every time a call to 'window.open' is done", function(){
        p.onPageCreated = jasmine.createSpy("onPageCreated spy");

        p.evaluate(function() {
            // yeah, I know globals. YIKES!
            window.w1 = window.open("http://www.google.com", "google");
            window.w2 = window.open("http://www.yahoo.com", "yahoo");
            window.w3 = window.open("http://www.bing.com", "bing");
        });
        expect(p.onPageCreated).toHaveBeenCalled();
        expect(p.onPageCreated.calls.length).toEqual(3);
    });

    it("should correctly resize the 'pages' array if a page gets closed", function(){
        expect(p.pages.length).toEqual(3);
        expect(p.pagesWindowName).toEqual(["google", "yahoo", "bing"]);

        p.evaluate(function() {
            window.w1.close();
        });

        waitsFor(function(){
            return p.pages.length === 2;
        }, "'pages' array didn't shrink after 1sec", 1000);

        runs(function(){
            expect(p.pages.length).toEqual(2);
            expect(p.pagesWindowName).toEqual(["yahoo", "bing"]);
        });
    });

    it("should resize the 'pages' array even more, when closing a page directly", function() {
        expect(p.pages.length).toEqual(2);
        expect(p.pagesWindowName).toEqual(["yahoo", "bing"]);

        var yahoo = p.getPage("yahoo");
        expect(yahoo).not.toBe(null);
        yahoo.close();

        waitsFor(function(){
            return p.pages.length === 1;
        }, "'pages' array didn't shrink after 1sec", 1000);

        runs(function(){
            expect(p.pages.length).toEqual(1);
            expect(p.pagesWindowName).toEqual(["bing"]);
            p.close();
        });
    });
});

describe("WebPage timeout handling", function(){
    it("should call 'onResourceTimeout' on timeout", function(){
        var p = require("webpage").create(),
            spy;

        // assume that requesting a web page will take longer than a millisecond
        p.settings.resourceTimeout = 1;

        spy = jasmine.createSpy("onResourceTimeout spy");
        p.onResourceTimeout = spy;

        expect(spy.calls.length).toEqual(0);

        p.open("http://www.google.com:81/");

        waitsFor(function() {
            return spy.calls.length==1;
        }, "after 1+ milliseconds 'onResourceTimeout' should have been invoked", 10);

        runs(function() {
            expect(spy).toHaveBeenCalled();         //< called
            expect(spy.calls.length).toEqual(1);    //< only once
            expect(1).toEqual(1);
        });

    });
});

describe("WebPage closing notification/alerting", function(){
    it("should call 'onClosing' when 'page.close()' is called", function(){
        var p = require("webpage").create(),
            spy;

        spy = jasmine.createSpy("onClosing spy");
        p.onClosing = spy;

        expect(spy.calls.length).toEqual(0);

        p.close();

        waitsFor(function() {
            return spy.calls.length === 1;
        }, "after 2sec 'onClosing' had still not been invoked", 2000);

        runs(function() {
            expect(spy).toHaveBeenCalled();         //< called
            expect(spy.calls.length).toEqual(1);    //< only once
            expect(spy).toHaveBeenCalledWith(p);    //< called passing reference to the closing page 'p'
        });
    });

    it("should call 'onClosing' when a page closes on it's own", function(){
        var p = require("webpage").create(),
            spy;

        spy = jasmine.createSpy("onClosing spy");
        p.onClosing = spy;

        expect(spy.calls.length).toEqual(0);

        p.evaluate(function() {
            window.close();
        });

        waitsFor(function() {
            return spy.calls.length === 1;
        }, "after 2sec 'onClosing' had still not been invoked", 2000);

        runs(function() {
            expect(spy).toHaveBeenCalled();         //< called
            expect(spy.calls.length).toEqual(1);    //< only once
            expect(spy).toHaveBeenCalledWith(p);    //< called passing reference to the closing page 'p'
        });
    });
});

describe("WebPage closing notification/alerting: closing propagation control", function(){
    it("should close all 4 pages if parent page is closed (default value for 'ownsPages')", function(){
        var p = require("webpage").create(),
            pages,
            openPagesCount = 0;

        p.onPageCreated = jasmine.createSpy("onPageCreated spy");

        expect(p.ownsPages).toBeTruthy();

        p.evaluate(function() {
            // yeah, I know globals. YIKES!
            window.w1 = window.open("http://www.google.com", "google");
            window.w2 = window.open("http://www.yahoo.com", "yahoo");
            window.w3 = window.open("http://www.bing.com", "bing");
        });
        pages = p.pages;
        openPagesCount = p.pages.length + 1;
        expect(p.onPageCreated).toHaveBeenCalled();
        expect(p.onPageCreated.calls.length).toEqual(3);
        expect(p.pages.length).toEqual(3);

        p.onClosing = function() { --openPagesCount; };
        pages[0].onClosing = function() { --openPagesCount; };
        pages[1].onClosing = function() { --openPagesCount; };
        pages[2].onClosing = function() { --openPagesCount; };

        p.close();

        waitsFor(function() {
            return openPagesCount === 0;
        }, "after 2sec pages were still open", 2000);

        runs(function() {
            expect(openPagesCount).toBe(0);
        });
    });

    it("should NOT close all 4 pages if parent page is closed, just parent itself ('ownsPages' set to false)", function(){
        var p = require("webpage").create(),
            pages,
            openPagesCount = 0;
        p.ownsPages = false;

        p.onPageCreated = jasmine.createSpy("onPageCreated spy");

        expect(p.ownsPages).toBeFalsy();

        p.evaluate(function() {
            // yeah, I know globals. YIKES!
            window.w1 = window.open("http://www.google.com", "google");
            window.w2 = window.open("http://www.yahoo.com", "yahoo");
            window.w3 = window.open("http://www.bing.com", "bing");
        });
        pages = p.pages;
        openPagesCount = 1;
        expect(p.onPageCreated).toHaveBeenCalled();
        expect(p.onPageCreated.calls.length).toEqual(3);
        expect(p.pages.length).toEqual(0);

        p.onClosing = function() { --openPagesCount; };

        p.close();

        waitsFor(function() {
            return openPagesCount === 0;
        }, "after 2sec pages were still open", 2000);

        runs(function() {
            expect(openPagesCount).toBe(0);
        });
    });
});

describe("WebPage 'onFilePicker'", function() {
    it("should be able to set the file to upload when the File Picker is invoked (i.e. clicking on a 'input[type=file]')", function() {
        var system = require('system'),
            fileToUpload = system.os.name === "windows" ? "C:\\Windows\\System32\\drivers\\etc\\hosts" : "/etc/hosts",
            server = require("webserver").create(),
            page = require("webpage").create();

        // Create a webserver that returns a page with an "input type=file" element
        server.listen(12345, function(request, response) {
            response.statusCode = 200;
            response.write('<html><body><input type="file" id="fileup" /></body></html>');
            response.close();
        });

        // Register "onFilePicker" handler
        page.onFilePicker = function(oldFile) {
            return fileToUpload;
        };

        runs(function() {
            page.open("http://localhost:12345", function() {
                // Before clicking on the file selector element
                expect(page.evaluate(function() {
                    var fileUp = document.querySelector("#fileup");
                    return fileUp.files.length;
                })).toBe(0);

                // Click on file selector element, so the "onFilePicker" is invoked
                page.evaluate(function() {
                    var fileUp = document.querySelector("#fileup");
                    var ev = document.createEvent("MouseEvents");
                    ev.initEvent("click", true, true);
                    fileUp.dispatchEvent(ev);
                });

                // After clicking on the file selector element
                expect(page.evaluate(function() {
                    var fileUp = document.querySelector("#fileup");
                    return fileUp.files.length;
                })).toBe(1);
                expect(page.evaluate(function() {
                    var fileUp = document.querySelector("#fileup");
                    return fileUp.files[0].name;
                })).toContain("hosts");
            });
        });

        waits(100);

        runs(function() {
            server.close();
        });
    });
});

describe('WebPage navigation events', function() {
    it('should navigate to relative url using window.location', function () {
        var page = require("webpage").create();
        var base = 'https://github.com';
        var path = '/n1k0';
        var expected = 'https://github.com/n1k0';
        var isHandled = false;

        runs(function() {
            page.onNavigationRequested = function(url, navigationType, navigationLocked, isMainFrame) {
                if (!page.testStarted) {
                    return;
                }

                if (url === expected) {
                    isHandled = true;
                }
            };

            page.open(base, function(status) {
                page.testStarted = true;

                page.evaluate(function(path) {
                    window.location = path;
                }, path);
            });
        });

        waits(10000);

        runs(function() {
            expect(isHandled).toEqual(true);
        });
    });
});

describe("WebPage loading/loadingProgress properties", function() {
    var p = require("webpage").create();

    it("should not be loading when page has just been created", function() {
        expect(p.loading).toBeFalsy();
        expect(p.loadingProgress).toEqual(0);
    });

    it("should be loading when 'page.open' is invoked", function() {
        var s = require("webserver").create();

        s.listen(12345, function(request, response) {
            setTimeout(function() {
                response.statusCode = 200;
                response.write('<html><body>Loaded!</body></html>');
                response.close();
            }, 200);
        });

        runs(function() {
            p.open("http://localhost:12345");
            expect(p.loading).toBeTruthy();
            expect(p.loadingProgress).toBeGreaterThan(0);
        });

        waits(500);

        runs(function() {
            s.close();
        });
    });

    it("should be completed when page is fully loaded", function() {
        var s = require("webserver").create();

        s.listen(12345, function(request, response) {
            setTimeout(function() {
                response.statusCode = 200;
                response.write('<html><body>Loaded!</body></html>');
                response.close();
            }, 500);
        });

        var loaded = false;

        runs(function() {
            p.open("http://localhost:12345", function () {
                loaded = true;
            });
        });

        waitsFor(function () {
            return loaded;
        }, 'Can not test loading progress' , 3000);

        runs(function() {
            expect(p.loading).toBeFalsy();
            expect(p.loadingProgress).toEqual(100);
            s.close();
        });
    });
});

describe("WebPage render image", function(){
    var TEST_FILE_DIR = "webpage-spec-renders/";

    var p = require("webpage").create();
    p.paperSize = { width: '300px', height: '300px', border: '0px' };
    p.clipRect = { top: 0, left: 0, width: 300, height: 300};
    p.viewportSize = { width: 300, height: 300};

    function render_test(format, option) {
        var opt = option || {};
        var rendered = false;

        p.open(TEST_FILE_DIR + "index.html", function() {
            var content, expect_content;
            try {
                var FILE_EXTENSION = format;
                var FILE_NAME = "test";
                var EXPECT_FILE;
                if( opt.quality ){
                    EXPECT_FILE = TEST_FILE_DIR + FILE_NAME + opt.quality + "." + FILE_EXTENSION;
                }
                else{
                    EXPECT_FILE = TEST_FILE_DIR + FILE_NAME + "." + FILE_EXTENSION;
                }

                var TEST_FILE;
                if( opt.format ){
                    TEST_FILE = TEST_FILE_DIR + "temp_" + FILE_NAME;
                }
                else{
                    TEST_FILE = TEST_FILE_DIR + "temp_" + FILE_NAME + "." + FILE_EXTENSION;
                }

                p.render(TEST_FILE, opt);

                expect_content = fs.read(EXPECT_FILE, "b");
                content = fs.read(TEST_FILE, "b");

                fs.remove(TEST_FILE);
            } catch (e) { console.log(e) }

            // for PDF test
            if (format === "pdf") {
                content = content.replace(/CreationDate \(D:\d+\)Z\)/,'');
                expect_content = expect_content.replace(/CreationDate \(D:\d+\)Z\)/,'');
            }

            // Files may not be exact, compare rought size (KB) only.
            expect(content.length >> 10).toEqual(expect_content.length >> 10);

            // Content comparison works for PNG and JPEG.
            if (format === "png" || format === "jpg") {
                expect(content).toEqual(expect_content);
            }

            rendered = true;
        });

        waitsFor(function() {
            return rendered;
        }, "page to be rendered", 3000);
    }

    it("should render PDF file", function(){
        render_test("pdf");
    });

    it("should render PDF file with format option", function(){
        render_test("pdf", { format: "pdf" });
    });

    it("should render GIF file", function(){
        render_test("gif");
    });

    it("should render GIF file with format option", function(){
        render_test("gif", { format: "gif" });
    });

    it("should render PNG file", function(){
        render_test("png");
    });

    it("should render PNG file with format option", function(){
        render_test("png", { format: "png" });
    });

    it("should render JPEG file with quality option", function(){
        render_test("jpg", { quality: 50 });
    });

    it("should render JPEG file with format and quality option", function(){
        render_test("jpg", { format: 'jpg', quality: 50 });
    });

    runs(function() {
        p.close();
    });
});
